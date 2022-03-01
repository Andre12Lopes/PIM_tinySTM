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
    struct stm_tx tx;
    int ra, rb, rc, tid;
    unsigned int a, b;
    uint64_t s;
    char buffer[BUFFER_SIZE];
    int idx = 0;
    int t;
    perfcounter_t initial_time;
    uint32_t t_aborts = 0;

    s = (uint64_t)me();
    tid = me();
    // buddy_init(4096);

    initialize_accounts();

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        n_trans = N_TRANSACTIONS * NR_TASKLETS;
        n_tasklets = NR_TASKLETS;
        n_aborts = 0;

        initial_time = perfcounter_config(COUNT_CYCLES, false);
    }


    // ------------------------------------------------------

    for (int i = 0; i < N_TRANSACTIONS; ++i)
    {
        ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;
        rc = (RAND_R_FNC(s) % 100) + 1;

        
#ifdef TX_IN_MRAM
        START(&(tx_mram[tid]));

        a = LOAD(&(tx_mram[tid]), &bank[ra], t_aborts);
        a -= TRANSFER;
        STORE(&(tx_mram[tid]), &bank[ra], a, t_aborts);

        b = LOAD(&(tx_mram[tid]), &bank[rb], t_aborts);
        b += TRANSFER;
        STORE(&(tx_mram[tid]), &bank[rb], b, t_aborts);

        COMMIT(&(tx_mram[tid]), t_aborts);
#else
        START(&tx);

        a = LOAD(&tx, &bank[ra], t_aborts);
        a -= TRANSFER;
        STORE(&tx, &bank[ra], a, t_aborts);

        b = LOAD(&tx, &bank[rb], t_aborts);
        b += TRANSFER;
        STORE(&tx, &bank[rb], b, t_aborts);

        COMMIT(&tx, t_aborts);
#endif

        // START_DEBUG(&tx, tid, buffer, idx);
        // a = LOAD_DEBUG(&tx, &bank[ra], buffer, idx, ra);
        // a -= TRANSFER;
        // STORE_DEBUG(&tx, &bank[ra], a, buffer, idx, ra);
        // b = LOAD_DEBUG(&tx, &bank[rb], buffer, idx, rb);
        // b += TRANSFER;
        // STORE_DEBUG(&tx, &bank[rb], b, buffer, idx, rb);
        // COMMIT_DEBUG(&tx, buffer, idx);

#ifdef RO_TX
        if (rc <= 5)
        {
            START(tx, tid);

            t = 0;
            t += LOAD(tx, &bank[0], t_aborts);
            t += LOAD(tx, &bank[1], t_aborts);

            COMMIT(tx, t_aborts);

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

        assert(N_ACCOUNTS * ACCOUNT_V);
    }

}