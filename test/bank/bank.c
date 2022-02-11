#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <alloc.h>
#include <assert.h>
#include <barrier.h>

#include <tiny.h>
#include <tiny_internal.h>
#include <defs.h>

#define TRANSFER 2
#define N_ACCOUNTS 17
#define ACCOUNT_V 1000

#define BUFFER_SIZE 100

#define START(tx, b, idx)           do { stm_start(tx); \
                                        b[idx] = 'S'; \
                                        idx = (idx+1) % BUFFER_SIZE

#define LOAD(tx, val, b, idx)           /*b[idx] = 'l'; \
                                        idx = (idx+1) % BUFFER_SIZE;*/ \
                                        stm_load(tx, val); \
                                        b[idx] = 'L'; \
                                        idx = (idx+1) % BUFFER_SIZE; \
                                        /*b[idx] = (a); \
                                        idx = (idx+1) % BUFFER_SIZE; */\
                                        if (tx->status == 4) { b[idx] = 'R'; idx = (idx+1) % BUFFER_SIZE; continue; }

#define STORE(tx, val, v, b, idx)       b[idx] = 'w'; \
                                        idx = (idx+1) % BUFFER_SIZE; \
                                        stm_store(tx, val, v); \
                                        b[idx] = 'W'; \
                                        /*idx = (idx+1) % BUFFER_SIZE; \
                                        b[idx] = (a); */\
                                        idx = (idx+1) % BUFFER_SIZE; \
                                        if (tx->status == 4) { b[idx] = 'R'; idx = (idx+1) % BUFFER_SIZE; continue; }

#define COMMIT(tx, b, idx)              stm_commit(tx); \
                                        b[idx] = 'C'; \
                                        idx = (idx+1) % BUFFER_SIZE; \
                                        if (tx->status != 4) { break; } \
                                        b[idx] = 'R'; \
                                        idx = (idx+1) % BUFFER_SIZE; \
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

int main()
{
    struct stm_tx *tx = NULL;
    int ra, rb, tid;
    unsigned int a, b;
    uint64_t s;
    char buffer[BUFFER_SIZE];
    int idx = 0;

    s = (uint64_t)me();
    tid = me();
    buddy_init(4096);

    initialize_accounts();

    barrier_wait(&my_barrier);

    // ------------------------------------------------------

    for (int i = 0; i < 5000; ++i)
    {
        tx = buddy_alloc(sizeof(struct stm_tx));

        ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;

        START(tx, buffer, idx);

        a = LOAD(tx, &bank[ra], buffer, idx);
        a -= TRANSFER;
        STORE(tx, &bank[ra], a, buffer, idx);

        b = LOAD(tx, &bank[rb], buffer, idx);
        b += TRANSFER;
        STORE(tx, &bank[rb], b, buffer, idx);

        COMMIT(tx, buffer, idx);

        // printf("A = %u B = %u, TID = %d\n", a, b, s1);

        buddy_free(tx);
    }

    // ------------------------------------------------------
    // __asm__ __volatile__("" : : : "memory");

    // printf("ENDED %d\n", tid);

    barrier_wait(&my_barrier);

    // for (int i = 0; i < 10; ++i)
    // {
    //     if (tid == i)
    //     {
    //         printf("TID = %d ->", tid);
    //         for (int i = 0; i < idx; ++i)
    //         {
    //             printf("%c, ",buffer[i]);
    //         }
    //         printf("\n\n");
    //     }

    //     barrier_wait(&my_barrier);
    // }



    print_accounts();
    
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