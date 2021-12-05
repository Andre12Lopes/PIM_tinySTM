#ifndef _TINY_INTERNAL_H_
#define _TINY_INTERNAL_H_

#include <stdlib.h>

#include "atomic.h"
#include "utils.h"

#ifndef LOCK_ARRAY_LOG_SIZE
#define LOCK_ARRAY_LOG_SIZE         10 /* Size of lock array: 2^10 = 1024 */
#endif

#ifndef LOCK_SHIFT_EXTRA
#define LOCK_SHIFT_EXTRA            2
#endif

#define OWNED_BITS                  1 /* 1 bit */
#define INCARNATION_BITS            3 /* 3 bits */
#define LOCK_BITS                   (OWNED_BITS + INCARNATION_BITS)

#define WRITE_MASK                  0x01
#define OWNED_MASK                  (WRITE_MASK)
#define INCARNATION_BITS            3 /* 3 bits */
#define INCARNATION_MAX             ((1 << INCARNATION_BITS) - 1)
#define INCARNATION_MASK            (INCARNATION_MAX << 1)

#define LOCK_GET_OWNED(l)           (l & OWNED_MASK)
#define LOCK_GET_WRITE(l)           (l & WRITE_MASK)
#define LOCK_SET_ADDR_WRITE(a)      (a | WRITE_MASK)
#define LOCK_GET_ADDR(l)            (l & ~(stm_word_t)OWNED_MASK)
#define LOCK_GET_TIMESTAMP(l)       (l >> (LOCK_BITS))
#define LOCK_SET_TIMESTAMP(t)       (t << (LOCK_BITS))
#define LOCK_GET_INCARNATION(l)     ((l & INCARNATION_MASK) >> OWNED_BITS)
#define LOCK_SET_INCARNATION(i)     (i << OWNED_BITS) /* OWNED bit not set */
#define LOCK_UPD_INCARNATION(l, i)  ((l & ~(stm_word_t)(INCARNATION_MASK | OWNED_MASK)) | LOCK_SET_INCARNATION(i))

#define LOCK_ARRAY_SIZE             (1 << LOCK_ARRAY_LOG_SIZE)
#define LOCK_MASK                   (LOCK_ARRAY_SIZE - 1)
#define LOCK_SHIFT                  (((sizeof(stm_word_t) == 4) ? 2 : 3) + LOCK_SHIFT_EXTRA)
#define LOCK_IDX(a)                 (((stm_word_t)(a) >> LOCK_SHIFT) & LOCK_MASK)
#define GET_LOCK(a)                 (_tinystm.locks + LOCK_IDX(a))

#define CLOCK                       (_tinystm.gclock)
#define GET_CLOCK                   (ATOMIC_LOAD_ACQ(&CLOCK))
#define FETCH_INC_CLOCK             (ATOMIC_FETCH_INC_FULL(&CLOCK))

enum
{ /* Transaction status */
    TX_IDLE = 0,
    TX_ACTIVE = 1, /* Lowest bit indicates activity */
    TX_COMMITTED = (1 << 1),
    TX_ABORTED = (2 << 1),
    TX_COMMITTING = (1 << 1) | TX_ACTIVE,
    TX_ABORTING = (2 << 1) | TX_ACTIVE,
    TX_KILLED = (3 << 1) | TX_ACTIVE,
};

#define SET_STATUS(s, v) ((s) = (v))
#define UPDATE_STATUS(s, v) ((s) = (v))
#define GET_STATUS(s) ((s))
#define IS_ACTIVE(s) ((GET_STATUS(s) & 0x01) == TX_ACTIVE)

typedef struct r_entry
{                              /* Read set entry */
    stm_word_t version;        /* Version read */
    volatile stm_word_t *lock; /* Pointer to lock (for fast access) */
} r_entry_t;

typedef struct r_set
{                            /* Read set */
    // r_entry_t *entries;      /* Array of entries */
    r_entry_t entries[10];      /* Array of entries */
    unsigned int nb_entries; /* Number of entries */
    unsigned int size;       /* Size of array */
} r_set_t;

typedef struct w_entry
{           /* Write set entry */
    union { /* For padding... */
        struct
        {
            volatile stm_word_t *addr; /* Address written */
            stm_word_t value;          /* New (write-back) or old (write-through) value */
            stm_word_t mask;           /* Write mask */
            stm_word_t version;        /* Version overwritten */
            volatile stm_word_t *lock; /* Pointer to lock (for fast access) */
            struct w_entry *next;      /* WRITE_BACK_ETL || WRITE_THROUGH: Next address
                                          covered by same lock (if any) */
        };
        char padding[CACHELINE_SIZE]; /* Padding (multiple of a cache line) */
                                      /* Note padding is not useful here as long as the address can be defined in
                                       * the lock scheme. */
    };
} w_entry_t;


typedef struct w_set
{                            /* Write set */
    // w_entry_t *entries;      /* Array of entries */
    w_entry_t entries[10];      /* Array of entries */
    unsigned int nb_entries; /* Number of entries */
    unsigned int size;       /* Size of array */
} w_set_t;


typedef struct stm_tx
{
    volatile stm_word_t status; /* Transaction status */
    stm_word_t start;           /* Start timestamp */
    stm_word_t end;             /* End timestamp (validity range) */
    r_set_t r_set;              /* Read set */
    w_set_t w_set;              /* Write set */
    unsigned int nesting;       /* Nesting level */
    unsigned int read_only;
} stm_tx_t;


typedef struct
{
    volatile stm_word_t locks[LOCK_ARRAY_SIZE]; /* TODO: aligned??? */
    volatile stm_word_t gclock;                 /* TODO: aligned??? */
    unsigned int initialized;                   /* Has the library been initialized? */
} global_t;                                     /* TODO: aligned??? */

extern global_t _tinystm;

static void stm_rollback(stm_tx_t *tx, unsigned int reason);

/*
 * Check if stripe has been read previously.
 */
static inline r_entry_t *
stm_has_read(stm_tx_t *tx, volatile stm_word_t *lock)
{
    r_entry_t *r;
    int i;

    PRINT_DEBUG("==> stm_has_read(%p[%lu-%lu],%p)\n", tx, (unsigned long)tx->start, (unsigned long)tx->end, lock);

    /* Look for read */
    r = tx->r_set.entries;
    for (i = tx->r_set.nb_entries; i > 0; i--, r++)
    {
        if (r->lock == lock)
        {
            return r;
        }
    }
    return NULL;
}

// TODO change alocation to DPU
static void 
stm_allocate_rs_entries(stm_tx_t *tx, int extend)
{
    PRINT_DEBUG("==> stm_allocate_rs_entries(%p[%lu-%lu],%d)\n", tx, (unsigned long)tx->start, (unsigned long)tx->end,
                extend);

    // if (extend)
    // {
    //     tx->r_set.size *= 2;
    //     tx->r_set.entries = (r_entry_t *)xrealloc(tx->r_set.entries, tx->r_set.size * sizeof(r_entry_t));
    // }
    // else
    // {
    //     /* Allocate read set */
    //     tx->r_set.entries = (r_entry_t *)xmalloc_aligned(tx->r_set.size * sizeof(r_entry_t));
    // }

    printf("[Warning!] Reached allocation function, aborting\n");
    exit(1);
}

#include "tiny_wtetl.h"

/*
 * Initialize the transaction descriptor before start or restart.
 */
static inline void 
int_stm_prepare(stm_tx_t *tx)
{
    /* Read/write set */
    /* tx->w_set.nb_acquired = 0; */
    tx->w_set.nb_entries = 0;
    tx->r_set.nb_entries = 0;

    tx->w_set.size = 10;
    tx->r_set.size = 10;

    // start:
    /* Start timestamp */
    tx->start = tx->end = GET_CLOCK; /* OPT: Could be delayed until first read/write */
    // if (tx->start >= VERSION_MAX)
    // {
    //     /* Block all transactions and reset clock
    //     stm_quiesce_barrier(tx, rollover_clock, NULL);
    //     goto start;
    // }

    /* Set status */
    UPDATE_STATUS(tx->status, TX_ACTIVE);

    // stm_check_quiesce(tx);
}

static inline void
int_stm_start(stm_tx_t *tx)
{
    PRINT_DEBUG("==> stm_start(%p)\n", tx);

    /* TODO Nested transaction attributes are not checked if they are coherent
     * with parent ones.  */

    /* Increment nesting level */
    if (tx->nesting++ > 0)
    {
        return;
    }

    /* Initialize transaction descriptor */
    int_stm_prepare(tx);
}

/*
 * Rollback transaction.
 */
static void 
stm_rollback(stm_tx_t *tx, unsigned int reason)
{
    PRINT_DEBUG("==> stm_rollback(%p[%lu-%lu])\n", tx, 
                (unsigned long)tx->start, (unsigned long)tx->end);

    assert(IS_ACTIVE(tx->status));

    stm_wtetl_rollback(tx); // TODO

    /* Set status to ABORTED */
    SET_STATUS(tx->status, TX_ABORTED);

    /* Abort for extending the write set */
    // if (reason == STM_ABORT_EXTEND_WS)
    // {
    //     stm_allocate_ws_entries(tx, 1); // TODO
    // }

    /* Reset nesting level */
    tx->nesting = 1;

    /* Reset field to restart transaction */
    int_stm_prepare(tx);
}

static inline stm_word_t 
int_stm_load(stm_tx_t *tx, volatile stm_word_t *addr)
{
    return stm_wtetl_read(tx, addr);
}

static inline void 
int_stm_store(stm_tx_t *tx, volatile stm_word_t *addr, stm_word_t value)
{
    stm_wtetl_write(tx, addr, value, ~(stm_word_t)0);
}

static inline int
int_stm_commit(stm_tx_t *tx)
{
    PRINT_DEBUG("==> stm_commit(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

    /* Decrement nesting level */
    if (--tx->nesting > 0)
    {
        return 1;
    }

    assert(IS_ACTIVE(tx->status));

    /* A read-only transaction can commit immediately */
    if (tx->w_set.nb_entries != 0)
    {
        stm_wtetl_commit(tx);
    }

    /* Set status to COMMITTED */
    SET_STATUS(tx->status, TX_COMMITTED);

    return 1;
}

#endif /* _TINY_INTERNAL_H_ */