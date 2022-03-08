#ifndef _TINY_WBCTL_H_
#define _TINY_WBCTL_H_

static inline int 
stm_wbctl_validate(stm_tx_t *tx)
{
    r_entry_t *r;
    int i;
    stm_word_t l;

    PRINT_DEBUG("==> stm_wbctl_validate(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

    /* Validate reads */
    r = tx->r_set.entries;
    for (i = tx->r_set.nb_entries; i > 0; i--, r++)
    {
        /* Read lock */
        l = ATOMIC_LOAD(r->lock);
        /* Unlocked and still the same version? */
        if (LOCK_GET_OWNED(l))
        {
            /* Do we own the lock? */
            w_entry_t *w = (w_entry_t *)LOCK_GET_ADDR(l);

            /* Simply check if address falls inside our write set (avoids non-faulting load) */
            if (!(tx->w_set.entries <= w && w < tx->w_set.entries + tx->w_set.nb_entries))
            {
                /* Locked by another transaction: cannot validate */
                return 0;
            }

            /* We own the lock: OK */
            if (w->version != r->version)
            {
                /* Other version: cannot validate */
                return 0;
            }
        }
        else
        {
            if (LOCK_GET_TIMESTAMP(l) != r->version)
            {
                /* Other version: cannot validate */
                return 0;
            }
            /* Same version: OK */
        }
    }

    return 1;
}


/*
 * Extend snapshot range.
 */
static inline int 
stm_wbctl_extend(TYPE stm_tx_t *tx)
{
    stm_word_t now;

    PRINT_DEBUG("==> stm_wbctl_extend(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

    /* Get current time */
    now = GET_CLOCK;
    /* No need to check clock overflow here. The clock can exceed up to MAX_THREADS and it will be reset when the
     * quiescence is reached. */

    /* Try to validate read set */
    if (stm_wbctl_validate(tx))
    {
        /* It works: we can extend until now */
        tx->end = now;
        return 1;
    }

    return 0;
}


static inline void 
stm_wbctl_rollback(TYPE stm_tx_t *tx)
{
    w_entry_t *w;

    PRINT_DEBUG("==> stm_wbctl_rollback(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

    assert(IS_ACTIVE(tx->status));

    if (tx->w_set.nb_acquired > 0)
    {
        w = tx->w_set.entries + tx->w_set.nb_entries;
        do
        {
            w--;
            if (!w->no_drop)
            {
                if (--tx->w_set.nb_acquired == 0)
                {
                    /* Make sure that all lock releases become visible to other threads */
                    ATOMIC_STORE_REL(w->lock, LOCK_SET_TIMESTAMP(w->version));
                }
                else
                {
                    ATOMIC_STORE(w->lock, LOCK_SET_TIMESTAMP(w->version));
                }
            }
        } while (tx->w_set.nb_acquired > 0);
    }
}


static inline stm_word_t 
stm_wbctl_read(TYPE stm_tx_t *tx, volatile stm_word_t *addr)
{
    volatile stm_word_t *lock;
    stm_word_t l, l2, value, version;
    r_entry_t *r;
    w_entry_t *written = NULL;

    PRINT_DEBUG("==> stm_wbctl_read(t=%p[%lu-%lu],a=%p)\n", tx, (unsigned long)tx->start, (unsigned long)tx->end,
                 addr);

    assert(IS_ACTIVE(tx->status));

    /* Did we previously write the same address? */
    written = stm_has_written(tx, addr);
    if (written != NULL)
    {
        /* Yes: get value from write set if possible */
        if (written->mask == ~(stm_word_t)0)
        {
            value = written->value;
            /* No need to add to read set */
            return value;
        }
    }

    /* Get reference to lock */
    lock = GET_LOCK(addr);

    /* Note: we could check for duplicate reads and get value from read set */

    /* Read lock, value, lock */
restart:
    l = ATOMIC_LOAD_ACQ(lock);
restart_no_load:
    if (LOCK_GET_WRITE(l))
    {
        /* Locked */
        /* Do we own the lock? */
        /* Spin while locked (should not last long) */
        goto restart;
    }
    else
    {
        /* Not locked */
        value = ATOMIC_LOAD_ACQ(addr);
        l2 = ATOMIC_LOAD_ACQ(lock);
        if (l != l2)
        {
            l = l2;
            goto restart_no_load;
        }

        /* Check timestamp */
        version = LOCK_GET_TIMESTAMP(l);
        /* Valid version? */
        if (version > tx->end)
        {
            /* No: try to extend first (except for read-only transactions: no read set) */
            if (tx->read_only || !stm_wbctl_extend(tx))
            {
                /* Not much we can do: abort */
                stm_rollback(tx, STM_ABORT_VAL_READ);
                return 0;
            }
            /* Verify that version has not been overwritten (read value has not
             * yet been added to read set and may have not been checked during
             * extend) */
            l = ATOMIC_LOAD_ACQ(lock);
            if (l != l2)
            {
                l = l2;
                goto restart_no_load;
            }
            /* Worked: we now have a good version (version <= tx->end) */
        }
    }
    /* We have a good version: add to read set (update transactions) and return value */

    /* Did we previously write the same address? */
    if (written != NULL)
    {
        value = (value & ~written->mask) | (written->value & written->mask);
        /* Must still add to read set */
    }

    if (!tx->read_only)
    {
        /* Add address and version to read set */
        if (tx->r_set.nb_entries == tx->r_set.size)
        {
            stm_allocate_rs_entries(tx, 1);
        }

        r = &tx->r_set.entries[tx->r_set.nb_entries++];
        r->version = version;
        r->lock = lock;
    }

    return value;
}


static inline w_entry_t *
stm_wbctl_write(stm_tx_t *tx, volatile stm_word_t *addr, stm_word_t value, stm_word_t mask)
{
    volatile stm_word_t *lock;
    stm_word_t l, version;
    w_entry_t *w;

    PRINT_DEBUG("==> stm_wbctl_write(t=%p[%lu-%lu],a=%p,d=%p-%lu,m=0x%lx)\n", tx, (unsigned long)tx->start,
                 (unsigned long)tx->end, addr, (void *)value, (unsigned long)value, (unsigned long)mask);

    /* Get reference to lock */
    lock = GET_LOCK(addr);

    /* Try to acquire lock */
restart:
    l = ATOMIC_LOAD_ACQ(lock);
restart_no_load:
    if (LOCK_GET_OWNED(l))
    {
        /* Locked */
        /* Spin while locked (should not last long) */
        goto restart;
    }

    /* Not locked */
    w = stm_has_written(tx, addr);
    if (w != NULL)
    {
        w->value = (w->value & ~mask) | (value & mask);
        w->mask |= mask;

        return w;
    }

    /* Handle write after reads (before CAS) */
    version = LOCK_GET_TIMESTAMP(l);

acquire:
    if (version > tx->end)
    {
        /* We might have read an older version previously */
        if (stm_has_read(tx, lock) != NULL)
        {
            /* Read version must be older (otherwise, tx->end >= version) */
            /* Not much we can do: abort */
            stm_rollback(tx, STM_ABORT_VAL_WRITE);
            return NULL;
        }
    }

    /* Acquire lock (ETL) */
    /* We own the lock here (ETL) */
do_write:
    /* Add address to write set */
    if (tx->w_set.nb_entries == tx->w_set.size)
    {
        stm_allocate_ws_entries(tx, 1);
    }

    w = &tx->w_set.entries[tx->w_set.nb_entries++];
    w->addr = addr;
    w->mask = mask;
    w->lock = lock;

    if (mask == 0)
    {
        /* Do not write anything */
    }
    else
    {
        /* Remember new value */
        w->value = value;
    }

    w->no_drop = 1;

    return w;
}

static inline int 
stm_wbctl_commit(stm_tx_t *tx)
{
    w_entry_t *w;
    stm_word_t t;
    int i;
    stm_word_t l, l1, value;

    PRINT_DEBUG("==> stm_wbctl_commit(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

    /* Acquire locks (in reverse order) */
    w = tx->w_set.entries + tx->w_set.nb_entries;
    do
    {
        w--;
        /* Try to acquire lock */
    restart:
        l = ATOMIC_LOAD(w->lock);
        if (LOCK_GET_OWNED(l))
        {
            /* Do we already own the lock? */
            if (tx->w_set.entries <= (w_entry_t *)LOCK_GET_ADDR(l) &&
                (w_entry_t *)LOCK_GET_ADDR(l) < tx->w_set.entries + tx->w_set.nb_entries)
            {
                /* Yes: ignore */
                continue;
            }

            /* Abort self */
            stm_rollback(tx, STM_ABORT_WW_CONFLICT);
            return 0;
        }

        acquire(w->lock);

        l1 = ATOMIC_LOAD_ACQ(w->lock);

        if (l != l1)
        {
            release(w->lock);
            goto restart;
        }

        ATOMIC_STORE(w->lock, LOCK_SET_ADDR_WRITE((stm_word_t)w));

        release(w->lock);

        /* We own the lock here */
        w->no_drop = 0;
        
        /* Store version for validation of read set */
        w->version = LOCK_GET_TIMESTAMP(l);
        tx->w_set.nb_acquired++;
    } while (w > tx->w_set.entries);

    /* Get commit timestamp (may exceed VERSION_MAX by up to MAX_THREADS) */
    t = FETCH_INC_CLOCK + 1;

    /* Try to validate (only if a concurrent transaction has committed since tx->start) */
    if (tx->start != t - 1 && !stm_wbctl_validate(tx))
    {
        /* Cannot commit */
        stm_rollback(tx, STM_ABORT_VALIDATE);
        return 0;
    }

    /* Install new versions, drop locks and set new timestamp */
    w = tx->w_set.entries;
    for (i = tx->w_set.nb_entries; i > 0; i--, w++)
    {
        if (w->mask == ~(stm_word_t)0)
        {
            ATOMIC_STORE(w->addr, w->value);
        }
        else if (w->mask != 0)
        {
            value = (ATOMIC_LOAD(w->addr) & ~w->mask) | (w->value & w->mask);
            ATOMIC_STORE(w->addr, value);
        }
        /* Only drop lock for last covered address in write set (cannot be "no drop") */
        if (!w->no_drop)
    	{
        	ATOMIC_STORE_REL(w->lock, LOCK_SET_TIMESTAMP(t));
    	}
    }

end:
    return 1;
}

#endif /* _TINY_WBCTL_H_ */