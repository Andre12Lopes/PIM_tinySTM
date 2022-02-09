#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <alloc.h>
#include <barrier.h>

#include <tiny.h>
#include <tiny_internal.h>
#include <defs.h>

#define TRANSFER 2

#define START(tx)       do { stm_start(tx)

#define LOAD(tx, val)       stm_load(tx, val); if (tx->status == 4) continue

#define STORE(tx, val, v)   stm_store(tx, val, v); if (tx->status == 4) continue

#define COMMIT(tx)          stm_commit(tx); if (tx->status != 4) break; } while (1)

#define RAND_R_FNC(seed) ({ \
   uint64_t next = (seed); \
   uint64_t result; \
   next *= 1103515245; \
   next += 12345; \
   result = (uint64_t) (next >> 16) & (2048-1); \
   next *= 1103515245; \
   next += 12345; \
   result <<= 10; \
   result ^= (uint64_t) (next >> 16) & (1024-1); \
   next *= 1103515245; \
   next += 12345; \
   result <<= 10; \
   result ^= (uint64_t) (next >> 16) & (1024-1); \
   (seed) = next; \
   result; /* returns result */ \
})

BARRIER_INIT(my_barrier, NR_TASKLETS);

unsigned int bank[100];

int main()
{
    struct stm_tx *tx = NULL;
    int ra, rb;
    unsigned int a, b;
    uint64_t s;

    if (me() == 0)
    {
        for (int i = 0; i < 100; ++i)
        {
            bank[i] = 100;
        }

        stm_init();
    }

    barrier_wait(&my_barrier);

    s = (uint64_t)me();

    buddy_init(4096);

    // ------------------------------------------------------

    for (int i = 0; i < 5; ++i)
    {
        tx = buddy_alloc(sizeof(struct stm_tx));

        ra = RAND_R_FNC(s) % 100;
        rb = RAND_R_FNC(s) % 100;

        START(tx);

        a = LOAD(tx, &bank[ra]);
        b = LOAD(tx, &bank[rb]);

        a -= TRANSFER;
        b += TRANSFER;

        STORE(tx, &bank[ra], a);
        STORE(tx, &bank[rb], b);

        COMMIT(tx);

        buddy_free(tx);
    }

    // ------------------------------------------------------

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        printf("[");
        unsigned int total = 0;
        for (int i = 0; i < 100; ++i)
        {
            printf("%u,", bank[i]);
            total += bank[i];
        }
        printf("]\n");

        printf("TOTAL = %u\n", total);
    }

    return 0;
}