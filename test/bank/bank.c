#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <alloc.h>
#include <assert.h>
#include <barrier.h>
#include <perfcounter.h>

#include <tiny.h>
#include <tiny_internal.h>
#include <defs.h>

#define TRANSFER 2
#define N_ACCOUNTS 800
#define ACCOUNT_V 1000

#define BUFFER_SIZE 100

#define START_DEBUG(tx, tid, b, idx)    do { \
                                            stm_start(tx, tid); \
                                            b[idx] = 'S'; \
                                            idx = (idx+1) % BUFFER_SIZE

#define LOAD_DEBUG(tx, val, b, idx, acc)    stm_load(tx, val); \
                                            b[idx] = 'L'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            b[idx] = acc + '0'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            if (tx->status == 4) { b[idx] = 'R'; idx = (idx+1) % BUFFER_SIZE; continue; }

#define STORE_DEBUG(tx, val, v, b, idx, acc)     b[idx] = 'w'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            stm_store(tx, val, v); \
                                            b[idx] = 'W'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            b[idx] = acc + '0'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            if (tx->status == 4) { b[idx] = 'R'; idx = (idx+1) % BUFFER_SIZE; continue; }

#define COMMIT_DEBUG(tx, b, idx)            stm_commit(tx); \
                                            b[idx] = 'C'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                            if (tx->status != 4) { break; } \
                                            b[idx] = 'R'; \
                                            idx = (idx+1) % BUFFER_SIZE; \
                                        } while (1)


#define START(tx, tid)      do { \
                                stm_start(tx, tid);

#define LOAD(tx, val)           stm_load(tx, val); \
                                if (tx->status == 4) { continue; }

#define STORE(tx, val, v)       stm_store(tx, val, v); \
                                if (tx->status == 4) { continue; }

#define COMMIT(tx)              stm_commit(tx); \
                                if (tx->status != 4) { break; } \
                            } while (1)

#define RAND_R_FNC(seed) ({                         \
   uint64_t next = (seed);                          \
   uint64_t result;                                 \
   next *= 1103515245;                              \
   next += 12345;                                   \
   result = (uint64_t) (next >> 16) & (2048-1);     \
   next *= 1103515245;                              \
   next += 12345;                                   \
   result <<= 10;                                   \
   result ^= (uint64_t) (next >> 16) & (1024-1);    \
   next *= 1103515245;                              \
   next += 12345;                                   \
   result <<= 10;                                   \
   result ^= (uint64_t) (next >> 16) & (1024-1);    \
   (seed) = next;                                   \
   result; /* returns result */                     \
})

void initialize_accounts();
void print_accounts();

BARRIER_INIT(my_barrier, NR_TASKLETS);

unsigned int bank[N_ACCOUNTS];

__host uint32_t nb_cycles;

int main()
{
    struct stm_tx *tx = NULL;
    int ra, rb, rc, tid;
    unsigned int a, b;
    uint64_t s;
    char buffer[BUFFER_SIZE];
    int idx = 0;
    int t;
    perfcounter_t initial_time;

    s = (uint64_t)me();
    tid = me();
    buddy_init(4096);

    initialize_accounts();

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        initial_time = perfcounter_config(COUNT_CYCLES, false);
    }
    

    // ------------------------------------------------------

    for (int i = 0; i < 5000; ++i)
    {
        tx = buddy_alloc(sizeof(struct stm_tx));

        ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;
        // rc = (RAND_R_FNC(s) % 100) + 1;

        // START_DEBUG(tx, tid, buffer, idx);
        START(tx, tid);

        // a = LOAD_DEBUG(tx, &bank[ra], buffer, idx, ra);
        a = LOAD(tx, &bank[ra]);
        a -= TRANSFER;
        // STORE_DEBUG(tx, &bank[ra], a, buffer, idx, ra);
        STORE(tx, &bank[ra], a);

        // b = LOAD_DEBUG(tx, &bank[rb], buffer, idx, rb);
        b = LOAD(tx, &bank[rb]);
        b += TRANSFER;
        // STORE_DEBUG(tx, &bank[rb], b, buffer, idx, rb);
        STORE(tx, &bank[rb], b);

        // COMMIT_DEBUG(tx, buffer, idx);
        COMMIT(tx);

        buddy_free(tx);

        // if (rc <= 10)
        // {
        //     tx = buddy_alloc(sizeof(struct stm_tx));
        //     START(tx, tid);

        //     t = 0;
        //     t += LOAD(tx, &bank[0]);
        //     t += LOAD(tx, &bank[1]);

        //     COMMIT(tx);

        //     buddy_free(tx);

        //     assert(t == (N_ACCOUNTS * ACCOUNT_V));
        // }
    }

    // ------------------------------------------------------

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        nb_cycles = perfcounter_get() - initial_time;
    }

    // print_accounts();
    
    return 0;
}

void initialize_accounts()
{
    if (me() == 0)
    {
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            bank[i] = ACCOUNT_V;
        }    
    }
}

void print_accounts()
{
    if (me() == 0)
    {
        printf("[");
        unsigned int total = 0;
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            printf("%u,", bank[i]);
            total += bank[i];
        }
        printf("]\n");

        printf("TOTAL = %u\n", total);
    }

}