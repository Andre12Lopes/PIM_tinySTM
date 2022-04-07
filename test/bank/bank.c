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
__host uint32_t nb_process_cycles;
__host uint32_t nb_commit_cycles;
__host uint32_t nb_wasted_cycles;
__host uint32_t n_aborts;
__host uint32_t n_trans;
__host uint32_t n_tasklets;

#ifdef ACC_IN_MRAM
unsigned int __mram_noinit bank[N_ACCOUNTS];
#else
unsigned int bank[N_ACCOUNTS];
#endif

#ifdef TX_IN_MRAM
struct stm_tx __mram_noinit tx_mram[NR_TASKLETS];
#endif

void initialize_accounts();
void check_total();

int main()
{
    stm_tx_t tx;
    int tid;
    int ra, rb;
    unsigned int a, b;
    uint64_t s;
    // char buffer[BUFFER_SIZE];
    perfcounter_t initial_time;
    uint32_t t_aborts = 0;
#ifdef RO_TX
    int rc;
    int t;
#endif

    s = (uint64_t)me();
    tid = me();
    // buddy_init(4096);

#ifdef TX_IN_MRAM
    tx_mram[tid].TID = tid;
    tx_mram[tid].process_cycles = 0;
    tx_mram[tid].commit_cycles = 0;
#else
    tx.TID = tid;
    tx.process_cycles = 0;
    tx.commit_cycles = 0;
#endif

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
        // if (tid == 0)
        // {
        //     ra = RAND_R_FNC(s) % (N_ACCOUNTS / 2);
        //     rb = RAND_R_FNC(s) % (N_ACCOUNTS / 2);
        // }
        // else
        // {
        //     ra = (RAND_R_FNC(s) % (N_ACCOUNTS / 2)) + N_ACCOUNTS / 2;
        //     rb = (RAND_R_FNC(s) % (N_ACCOUNTS / 2)) + N_ACCOUNTS / 2;
        // }

        ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;
        
#ifdef RO_TX
        rc = (RAND_R_FNC(s) % 100) + 1;
#endif

      
#ifdef TX_IN_MRAM
        START(&(tx_mram[tid]));

        a = LOAD(&(tx_mram[tid]), &bank[ra], t_aborts, tid);
        a -= TRANSFER;
        STORE(&(tx_mram[tid]), &bank[ra], a, t_aborts);

        b = LOAD(&(tx_mram[tid]), &bank[rb], t_aborts, tid);
        b += TRANSFER;
        STORE(&(tx_mram[tid]), &bank[rb], b, t_aborts);

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

        nb_process_cycles = 0;
        nb_commit_cycles = 0;
        nb_wasted_cycles = 0;
    }

    for (int i = 0; i < NR_TASKLETS; ++i)
    {
        if (me() == i)
        {
            n_aborts += t_aborts;

#ifdef TX_IN_MRAM
            nb_process_cycles += ((double) tx_mram[tid].process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_cycles += (tx_mram[tid].commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_wasted_cycles += ((tx_mram[tid].total_cycles - (tx_mram[tid].process_cycles + tx_mram[tid].commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#else
            nb_process_cycles += (tx.process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_cycles += (tx.commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_wasted_cycles += ((tx.total_cycles - (tx.process_cycles + tx.commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#endif
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
        printf("[");
        unsigned int total = 0;
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            printf("%u,", bank[i]);
            total += bank[i];
        }
        printf("]\n");

        printf("TOTAL = %u\n", total);

        assert(total == (N_ACCOUNTS * ACCOUNT_V));
    }
}