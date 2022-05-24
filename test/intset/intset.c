#include <assert.h>
#include <barrier.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <defs.h>
#include <tiny.h>
#include <tiny_internal.h>

#include "linked_list.h"
#include "intset_macros.h"

#define UPDATE_PERCENTAGE   20
#define SET_INITIAL_SIZE    10
#define RAND_RANGE          100

#define N_TRANSACTIONS      100

BARRIER_INIT(barrier, NR_TASKLETS);

__host uint64_t nb_cycles;
__host uint64_t n_aborts;
__host uint64_t n_trans;
__host uint64_t n_tasklets;

__mram_ptr intset_t *set;

void test(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, uint64_t *seed);
void print_linked_list(__mram_ptr intset_t *set);

int main()
{
    stm_tx_t tx;
    
    uint64_t t_aborts = 0;
    int val, tid;
    uint64_t seed;
    int i = 0;
    perfcounter_t initial_time;

    seed = me();
    tid = me();

    tx.TID = tid;
    tx.rng = tid + 1;
    tx.process_cycles = 0;
    tx.read_cycles = 0;
    tx.write_cycles = 0;
    tx.validation_cycles = 0;
    tx.total_read_cycles = 0;
    tx.total_write_cycles = 0;
    tx.total_validation_cycles = 0;
    tx.total_commit_validation_cycles = 0;
    tx.commit_cycles = 0;
    tx.total_cycles = 0;
    tx.start_time = 0;
    tx.start_read = 0;
    tx.start_write = 0;
    tx.start_validation = 0;
    tx.retries = 0;

    if (tid == 0)
    {
        set = set_new(INIT_SET_PARAMETERS);

        while (i < SET_INITIAL_SIZE) 
        {
            val = (RAND_R_FNC(seed) % RAND_RANGE) + 1;
            if (set_add(set, val, 0))
            {
                i++;
            }            
        }

        n_trans = N_TRANSACTIONS * NR_TASKLETS;
        n_tasklets = NR_TASKLETS;
        n_aborts = 0;

        initial_time = perfcounter_config(COUNT_CYCLES, false);
    }

    barrier_wait(&barrier);

    for (int i = 0; i < N_TRANSACTIONS; ++i)
    {
        test(&tx, &t_aborts, set, &seed);
    }

    barrier_wait(&barrier);

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

        barrier_wait(&barrier);
    }

    // print_linked_list(set);

    return 0;
}


void test(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, uint64_t *seed)
{
    int val;
    // int op, last = -1;

    // op = RAND_R_FNC(seed) % 100;
    // if (op < UPDATE_PERCENTAGE)
    // {
    //     if (last < 0)
    //     {
    //         /* Add random value */
    //         val = (RAND_R_FNC(seed) % RAND_RANGE) + 1;
    //         if (set_add(set, val, 1))
    //         {
    //             last = val;
    //         }
    //     }
    //     else
    //     {
    //         /* Remove last value */
    //         set_remove(set, last);
    //         last = -1;
    //     }
    // }
    // else
    // {
    //     /* Look for random value */
        val = (RAND_R_FNC(*seed) % RAND_RANGE) + 1;
        set_contains(tx, t_aborts, set, val);
    // }
}


void print_linked_list(__mram_ptr intset_t *set)
{
    for (__mram_ptr node_t *n = set->head->next; n->next != NULL; n = n->next)
    {
        printf("%p -> %u\n", n, n->val);
    }
}