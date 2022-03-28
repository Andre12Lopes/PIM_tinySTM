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
#include <mram.h>

#include "macros.h"

#define TRANSFER 2
#define N_ACCOUNTS 800
#define ACCOUNT_V 1000
#define N_TRANSACTIONS 1000

BARRIER_INIT(my_barrier, NR_TASKLETS);

__host uint32_t nb_cycles;
__host uint32_t n_aborts;
__host uint32_t n_trans;
__host uint32_t n_tasklets;

unsigned int bank[N_ACCOUNTS];

void initialize_accounts();
void check_total();

#ifdef TX_IN_MRAM
struct stm_tx __mram_noinit tx_mram[NR_TASKLETS];
#endif

int main()
{
    stm_tx_t tx;
    int rand, tid;
    int ra, rb, rc;
    unsigned int a, b;
    uint64_t s;
    // char buffer[BUFFER_SIZE];
    int idx = 0;
    int t;
    perfcounter_t initial_time;
    uint32_t t_aborts = 0;

    s = (uint64_t)me();
    tid = me();
    // buddy_init(4096);

    initialize_accounts();

    barrier_wait(&my_barrier);

    if (tid == 0)
    {
        n_trans = N_TRANSACTIONS * NR_TASKLETS;
        n_tasklets = NR_TASKLETS;
        n_aborts = 0;

        initial_time = perfcounter_config(COUNT_CYCLES, false);
    }

    // ------------------------------------------------------

    for (int i = 0; i < N_TRANSACTIONS; ++i)
    {
        // ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;
#ifdef RO_TX
        rc = (RAND_R_FNC(s) % 100) + 1;
#endif

      
#ifdef TX_IN_MRAM
        START(&(tx_mram[tid]));

        ra = RAND_R_FNC(s) % N_ACCOUNTS;

        a = LOAD(&tx, &bank[ra], t_aborts, tid);
        a -= TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);


        ra = RAND_R_FNC(s) % N_ACCOUNTS;

        a = LOAD(&tx, &bank[ra], t_aborts, tid);
        a -= TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);


        ra = RAND_R_FNC(s) % N_ACCOUNTS;

        a = LOAD(&tx, &bank[ra], t_aborts, tid);
        a += TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);


        ra = RAND_R_FNC(s) % N_ACCOUNTS;

        a = LOAD(&tx, &bank[ra], t_aborts, tid);
        a += TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);

        COMMIT(&(tx_mram[tid]), t_aborts);
#else
        START(&tx);

        a = LOAD(&tx, &bank[ra], t_aborts, tid);
        a -= TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);

        b = LOAD(&tx, &bank[rb], t_aborts, tid);
        b += TRANSFER;
        STORE(&tx, &bank[rb], b, t_aborts);

        COMMIT(&tx, t_aborts);
#endif

#if defined(RO_TX)
        if (rc <= 10)
        {
            START(&(tx_mram[tid]));

            t = 0;

            for (int j = 0; j < N_ACCOUNTS; ++j)
            {
                t += LOAD_RO(&(tx_mram[tid]), &bank[j], t_aborts);
            }

            if (tx_mram[tid].status == 4)
            {
                continue;
            }

            COMMIT(&(tx_mram[tid]), t_aborts);

            assert(t == (N_ACCOUNTS * ACCOUNT_V));
        }
#endif
    }

    // ------------------------------------------------------

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        nb_cycles = perfcounter_get() - initial_time;
    }

    for (int i = 0; i < NR_TASKLETS; ++i)
    {
        if (me() == i)
        {
            n_aborts += t_aborts;
        }

        barrier_wait(&my_barrier);
    }

    check_total();
    
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

void check_total()
{
    if (me() == 0)
    {
        // printf("[");
        unsigned int total = 0;
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            // printf("%u,", bank[i]);
            total += bank[i];
        }
        // printf("]\n");

        printf("TOTAL = %u\n", total);

        assert(total == (N_ACCOUNTS * ACCOUNT_V));
    }
}