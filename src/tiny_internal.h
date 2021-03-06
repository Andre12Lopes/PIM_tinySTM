#ifndef _TINY_INTERNAL_H_
#define _TINY_INTERNAL_H_

#include <stdio.h>
#include <stdlib.h>

#include <perfcounter.h>

#include "atomic.h"
#include "utils.h"

#ifndef R_SET_SIZE
# define R_SET_SIZE                 2                /* Initial size of read sets */
#endif /* ! RW_SET_SIZE */

#ifndef W_SET_SIZE
# define W_SET_SIZE                 2                /* Initial size of write sets */
#endif /* ! RW_SET_SIZE */

#ifndef LOCK_ARRAY_LOG_SIZE
#define LOCK_ARRAY_LOG_SIZE         10                  /* Size of lock array: 2^10 = 1024 */
#endif

#ifndef LOCK_SHIFT_EXTRA
#define LOCK_SHIFT_EXTRA            2
#endif

#define OWNED_BITS                  1                   /* 1 bit */
#define INCARNATION_BITS            3                   /* 3 bits */
#define LOCK_BITS                   (OWNED_BITS + INCARNATION_BITS)

#define WRITE_MASK                  0x01
#define OWNED_MASK                  (WRITE_MASK)
#define INCARNATION_MAX             ((1 << INCARNATION_BITS) - 1)
#define INCARNATION_MASK            (INCARNATION_MAX << 1)

#define LOCK_GET_OWNED(l)           (l & OWNED_MASK)
#define LOCK_GET_WRITE(l)           (l & WRITE_MASK)
#define LOCK_SET_ADDR_WRITE(a)      (a | WRITE_MASK)
#define LOCK_GET_ADDR(l)            (l & ~(stm_word_t)OWNED_MASK)
#define LOCK_GET_TIMESTAMP(l)       (l >> (LOCK_BITS))
#define LOCK_SET_TIMESTAMP(t)       (t << (LOCK_BITS))
#define LOCK_GET_INCARNATION(l)     ((l & INCARNATION_MASK) >> OWNED_BITS)
#define LOCK_SET_INCARNATION(i)     (i << OWNED_BITS)   /* OWNED bit not set */
#define LOCK_UPD_INCARNATION(l, i)  ((l & ~(stm_word_t)(INCARNATION_MASK | OWNED_MASK)) | LOCK_SET_INCARNATION(i))

#define LOCK_ARRAY_SIZE             (1 << LOCK_ARRAY_LOG_SIZE)
#define LOCK_MASK                   (LOCK_ARRAY_SIZE - 1)
#define LOCK_SHIFT                  (((sizeof(stm_word_t) == 4) ? 2 : 3) + LOCK_SHIFT_EXTRA)
#define LOCK_IDX(a)                 (((stm_word_t)(a) >> LOCK_SHIFT) & LOCK_MASK)
#define GET_LOCK(a)                 (_tinystm.locks + LOCK_IDX(a))

#define CLOCK                       (_tinystm.gclock)
#define GET_CLOCK                   (ATOMIC_GET_CLOCK_VALUE(&CLOCK))
#define FETCH_INC_CLOCK             (ATOMIC_FETCH_INC_FULL(&CLOCK, 1))

enum
{                                           /* Transaction status */
    TX_IDLE = 0,
    TX_ACTIVE = 1,                          /* Lowest bit indicates activity */
    TX_COMMITTED = (1 << 1),
    TX_ABORTED = (2 << 1),
    TX_COMMITTING = (1 << 1) | TX_ACTIVE,
    TX_ABORTING = (2 << 1) | TX_ACTIVE,
    TX_KILLED = (3 << 1) | TX_ACTIVE,
};

#define SET_STATUS(s, v)    ((s) = (v))
#define UPDATE_STATUS(s, v) ((s) = (v))
#define GET_STATUS(s)       ((s))
#define IS_ACTIVE(s)        ((GET_STATUS(s) & 0x01) == TX_ACTIVE)
#define IS_ABORTED(s)       ((GET_STATUS(s) & 0x04) == TX_ABORTED)

typedef struct r_entry
{                              /* Read set entry */
    stm_word_t version;        /* Version read */
    volatile stm_word_t *lock; /* Pointer to lock (for fast access) */
} r_entry_t;

typedef struct r_set
{                               /* Read set */   /* Array of entries */
    r_entry_t entries[R_SET_SIZE];       /* Array of entries */
    unsigned int nb_entries;    /* Number of entries */
    unsigned int size;          /* Size of array */
} r_set_t;

typedef struct w_entry
{           /* Write set entry */
    struct
    {
        volatile TYPE_ACC stm_word_t *addr;   /* Address written */
        stm_word_t value;                       /* New (write-back) or old (write-through) value */
        stm_word_t mask;                        /* Write mask */
        stm_word_t version;                     /* Version overwritten */
        volatile stm_word_t *lock;              /* Pointer to lock (for fast access) */
        TYPE struct w_entry *next;              /* WRITE_BACK_ETL || WRITE_THROUGH: Next address covered by same lock (if any) */
        stm_word_t no_drop;                     /* WRITE_BACK_CTL: Should we drop lock upon abort? */
    };
} w_entry_t __attribute__((aligned(16)));

typedef struct w_set
{                               /* Write set */     /* Array of entries */
    w_entry_t entries[W_SET_SIZE];       /* Array of entries */
    unsigned int nb_entries;    /* Number of entries */
    unsigned int size;          /* Size of array */
    unsigned int has_writes;
    unsigned int nb_acquired;
       /* WRITE_BACK_CTL: Number of locks acquired */
} w_set_t;


typedef struct stm_tx
{
    volatile stm_word_t status; /* Transaction status */
    stm_word_t start;           /* Start timestamp */
    stm_word_t end;             /* End timestamp (validity range) */
    r_set_t r_set;              /* Read set */
    w_set_t w_set;              /* Write set */
    // unsigned int nesting;    /* Nesting level */
    unsigned int read_only;
    unsigned int TID;
    unsigned long long rng;
    unsigned long retries;
    perfcounter_t time;
    perfcounter_t start_time;
    perfcounter_t start_read;
    perfcounter_t start_write;
    perfcounter_t start_validation;
    uint64_t process_cycles;
    uint64_t read_cycles;
    uint64_t write_cycles;
    uint64_t validation_cycles;    
    uint64_t total_read_cycles;
    uint64_t total_write_cycles;
    uint64_t total_validation_cycles;
    uint64_t total_commit_validation_cycles;
    uint64_t commit_cycles;
    uint64_t total_cycles;
} stm_tx_t;


typedef struct
{
    volatile stm_word_t locks[LOCK_ARRAY_SIZE];
    volatile stm_word_t gclock;
    unsigned int initialized;
} global_t;

extern global_t _tinystm;

static void 
stm_rollback(TYPE stm_tx_t *tx, unsigned int reason);

/*
 * Check if stripe has been read previously.
 */
static inline TYPE r_entry_t *
stm_has_read(TYPE stm_tx_t *tx, volatile stm_word_t *lock)
{
    TYPE r_entry_t *r;
    
    PRINT_DEBUG("==> stm_has_read(%p[%lu-%lu],%p)\n", tx, 
        (unsigned long)tx->start, (unsigned long)tx->end, lock);

    /* Look for read */
    r = tx->r_set.entries;
    for (int i = tx->r_set.nb_entries; i > 0; i--, r++)
    {
        if (r->lock == lock)
        {
            return r;
        }
    }

    return NULL;
}

/*
 * Check if address has been written previously.
 */
static inline TYPE w_entry_t *
stm_has_written(TYPE stm_tx_t *tx, volatile TYPE_ACC stm_word_t *addr)
{
    TYPE w_entry_t *w;

    PRINT_DEBUG("==> stm_has_written(%p[%lu-%lu],%p)\n", tx, 
        (unsigned long)tx->start, (unsigned long)tx->end, addr);

    /* Look for write */
    w = tx->w_set.entries;
    for (int i = tx->w_set.nb_entries; i > 0; i--, w++)
    {
        if (w->addr == addr)
        {
            return w;
        }
    }

    return NULL;
}

// TODO change alocation to DPU
static void 
stm_allocate_rs_entries(TYPE stm_tx_t *tx, int extend)
{
    (void) tx;
    (void) extend;

    PRINT_DEBUG("==> stm_allocate_rs_entries(%p[%lu-%lu],%d)\n", tx, 
        (unsigned long)tx->start, (unsigned long)tx->end,
                extend);

    SET_STATUS(tx->status, TX_ABORTED);

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

    printf("[Warning!] Reached RS allocation function, aborting\n");
    assert(0);
}


/*
 * (Re)allocate write set entries.
 */
static void 
stm_allocate_ws_entries(TYPE stm_tx_t *tx, int extend)
{
    (void) tx;
    (void) extend;

    PRINT_DEBUG("==> stm_allocate_ws_entries(%p[%lu-%lu],%d)\n", tx, 
        (unsigned long)tx->start, (unsigned long)tx->end,
                extend);

    SET_STATUS(tx->status, TX_ABORTED);

    // if (extend)
    // {
    //     /* Extend write set */
    //     /* Transaction must be inactive for WRITE_THROUGH or WRITE_BACK_ETL */
    //     tx->w_set.size *= 2;
    //     tx->w_set.entries = (w_entry_t *)xrealloc(tx->w_set.entries, tx->w_set.size * sizeof(w_entry_t));
    // }
    // else
    // {
    //     /* Allocate write set */
    //     tx->w_set.entries = (w_entry_t *)xmalloc_aligned(tx->w_set.size * sizeof(w_entry_t));
    // }
    // /* Ensure that memory is aligned. */
    // assert((((stm_word_t)tx->w_set.entries) & OWNED_MASK) == 0);

    printf("[Warning!] Reached WS allocation function, aborting\n");
    assert(0);
}

#ifdef WRITE_BACK_CTL
#include "tiny_wbctl.h"
#endif

#ifdef WRITE_BACK_ETL
#include "tiny_wbetl.h"
#endif

#ifdef WRITE_THROUGH_ETL
#include "tiny_wtetl.h"
#endif

static inline unsigned long long 
MarsagliaXORV(unsigned long long x)
{
    if (x == 0)
    {
        x = 1;
    }

    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;

    return x;
}

static inline unsigned long long 
MarsagliaXOR(TYPE unsigned long long *seed)
{
    unsigned long long x = MarsagliaXORV(*seed);
    *seed = x;

    return x;
}

static inline unsigned long long 
TSRandom(TYPE stm_tx_t *tx)
{
    return MarsagliaXOR(&tx->rng);
}

static inline void 
backoff(TYPE stm_tx_t *tx, long attempt)
{
    unsigned long long stall = TSRandom(tx) & 0xF; //0XFF
    stall += attempt >> 2; 
    stall *= 10;

    // stall = stall << attempt;

    /* CCM: timer function may misbehave */
    volatile unsigned long long  i = 0;
    while (i++ < stall)
    {
        PAUSE();
    }
}

/*
 * Initialize the transaction descriptor before start or restart.
 */
static inline void 
int_stm_prepare(TYPE stm_tx_t *tx)
{
    /* Read/write set */
    tx->w_set.has_writes = 0;
    tx->w_set.nb_acquired = 0;

    tx->r_set.nb_entries = 0;
    tx->w_set.nb_entries = 0;

    tx->r_set.size = R_SET_SIZE;
    tx->w_set.size = W_SET_SIZE;

    tx->read_only = 0;

    tx->read_cycles = 0;
    tx->write_cycles = 0;
    tx->validation_cycles = 0;

    // start:
    /* Start timestamp */
    tx->start = tx->end = GET_CLOCK; /* OPT: Could be delayed until first read/write */
    if (tx->start == 0)
    {
    }

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
int_stm_start(TYPE stm_tx_t *tx)
{
    PRINT_DEBUG("==> stm_start(%p)\n", tx);

    /* TODO Nesting.  */
    /* Increment nesting level */
    // if (tx->nesting++ > 0)
    // {
    //     return;
    // }

    /* Initialize transaction descriptor */
    if (tx->start_time == 0)
    {
        tx->start_time = perfcounter_config(COUNT_CYCLES, false);
    }
    tx->time = perfcounter_config(COUNT_CYCLES, false);

    int_stm_prepare(tx);
}

/*
 * Rollback transaction.
 */
static void 
stm_rollback(TYPE stm_tx_t *tx, unsigned int reason)
{
    (void) reason;

    assert(IS_ACTIVE(tx->status));

#if defined(WRITE_BACK_CTL)
    stm_wbctl_rollback(tx);
#elif defined(WRITE_BACK_ETL)
    stm_wbetl_rollback(tx);
#elif defined(WRITE_THROUGH_ETL) 
    stm_wtetl_rollback(tx);
#endif

#ifdef BACKOFF
    tx->retries++;
    if (tx->retries > 3)
    { /* TUNABLE */
        backoff(tx, tx->retries);
    }
#endif

    /* Set status to ABORTED */
    SET_STATUS(tx->status, TX_ABORTED);

    /* Abort for extending the write set */
    // if (reason == STM_ABORT_EXTEND_WS)
    // {
    //     stm_allocate_ws_entries(tx, 1); // TODO
    // }

    /* Reset nesting level */
    // tx->nesting = 1;

    /* Reset field to restart transaction */
    //int_stm_prepare(tx);
}

static inline stm_word_t 
int_stm_load(TYPE stm_tx_t *tx, volatile __mram_ptr stm_word_t *addr)
{
    stm_word_t val = -1;

    tx->start_read = perfcounter_config(COUNT_CYCLES, false);

#if defined(WRITE_BACK_CTL)
    val = stm_wbctl_read(tx, addr);
#elif defined(WRITE_BACK_ETL)
    val = stm_wbetl_read(tx, addr);
#elif defined(WRITE_THROUGH_ETL) 
    val = stm_wtetl_read(tx, addr);
#endif

    tx->read_cycles += perfcounter_get() - tx->start_read;

    return val;
}

static inline void 
int_stm_store(TYPE stm_tx_t *tx, volatile __mram_ptr stm_word_t *addr, stm_word_t value)
{
    PRINT_DEBUG("==> int_stm_store(t=%p[%lu-%lu],a=%p,d=%p-%lu)\n",
               tx, (unsigned long)tx->start, (unsigned long)tx->end, addr, 
               (void *)value, (unsigned long)value);

    tx->start_write = perfcounter_config(COUNT_CYCLES, false);

#if defined(WRITE_BACK_CTL)
    stm_wbctl_write(tx, addr, value, ~(stm_word_t)0);
#elif defined(WRITE_BACK_ETL)
    stm_wbetl_write(tx, addr, value, ~(stm_word_t)0);
#elif defined(WRITE_THROUGH_ETL) 
    stm_wtetl_write(tx, addr, value, ~(stm_word_t)0);
#endif

    tx->write_cycles += perfcounter_get() - tx->start_write;
}

static inline int
int_stm_commit(TYPE stm_tx_t *tx)
{
    uint64_t t_process_cycles;

    PRINT_DEBUG("==> stm_commit(%p[%lu-%lu])\n", tx, 
        (unsigned long)tx->start, (unsigned long)tx->end);

    /* Decrement nesting level */
    // if (--tx->nesting > 0)
    // {
    //     return 1;
    // }

    assert(IS_ACTIVE(tx->status));

    t_process_cycles = perfcounter_get() - tx->time;
    tx->time = perfcounter_config(COUNT_CYCLES, false);

    /* A read-only transaction can commit immediately */
    if (tx->w_set.nb_entries != 0)
    {
#if defined(WRITE_BACK_CTL)
        stm_wbctl_commit(tx);
#elif defined(WRITE_BACK_ETL)
        stm_wbetl_commit(tx);
#elif defined(WRITE_THROUGH_ETL)
        stm_wtetl_commit(tx);
#endif
    }

    if (IS_ABORTED(tx->status))
    {
        return 0;
    }

    /* Set status to COMMITTED */
    SET_STATUS(tx->status, TX_COMMITTED);

    tx->process_cycles += t_process_cycles;
    tx->commit_cycles += perfcounter_get() - tx->time;
    tx->total_cycles += perfcounter_get() - tx->start_time;
    tx->total_read_cycles += tx->read_cycles;
    tx->total_write_cycles += tx->write_cycles;
    tx->total_validation_cycles += tx->validation_cycles;

    tx->start_time = 0;
    tx->retries = 0;

    return 1;
}

#endif /* _TINY_INTERNAL_H_ */