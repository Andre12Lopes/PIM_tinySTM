#if defined(HASH_SET)

#include <alloc.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include <tiny.h>
#include <tiny_internal.h>

#include "hash_set.h"
#include "dpu_alloc.h"
#include "intset_macros.h"

#define NB_BUCKETS	(1UL << 10)

#define HASH(a)		(hash((uint32_t)a) & (NB_BUCKETS - 1))


static uint32_t hash(uint32_t a)
{
    /* Knuth's multiplicative hash function */
    a *= 2654435761UL;
    return a;
}


static __mram_ptr bucket_t *new_entry(val_t val, __mram_ptr bucket_t *next, int transactional)
{
    __mram_ptr bucket_t *b;

    // if (!transactional)
    // {
        b = (__mram_ptr bucket_t *)mram_malloc(sizeof(bucket_t));
    // }
    // else
    // {
    //     b = (bucket_t *)TM_MALLOC(sizeof(bucket_t));
    // }

    if (b == NULL)
    {
        exit(1);
    }

    b->val = val;
    b->next = next;

    return b;
}

__mram_ptr intset_t *set_new()
{
    __mram_ptr intset_t *set;

    if ((set = (__mram_ptr intset_t *)mram_malloc(sizeof(intset_t))) == NULL)
    {
        exit(1);
    }

    if ((set->buckets = (__mram_ptr bucket_t **)mram_malloc(NB_BUCKETS * sizeof(bucket_t *))) == NULL)
    {
        exit(1);
    }

    printf(">>> %p\n", set->buckets);

    // for (int i = 0; i < NB_BUCKETS; ++i)
    // {
    // 	// set->buckets[i] = NULL;
    // 	printf("%p\n", set->buckets[i]);
    // }

    return set;
}

int set_contains(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val)
{
    int result, i;
//     bucket_t *b;

// #ifdef DEBUG
//     printf("++> set_contains(%d)\n", val);
//     IO_FLUSH;
// #endif

//     if (!td)
//     {
//         i = HASH(val);
//         b = set->buckets[i];
//         result = 0;
//         while (b != NULL)
//         {
//             if (b->val == val)
//             {
//                 result = 1;
//                 break;
//             }
//             b = b->next;
//         }
//     }
//     else
//     {
//         TM_START(0, RO);
//         i = HASH(val);
//         b = (bucket_t *)TM_LOAD(&set->buckets[i]);
//         result = 0;
//         while (b != NULL)
//         {
//             if (TM_LOAD(&b->val) == val)
//             {
//                 result = 1;
//                 break;
//             }
//             b = (bucket_t *)TM_LOAD(&b->next);
//         }
//         TM_COMMIT;
//     }

    return result;
}

int set_add(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val, int transactional)
{
    int result, i;
    __mram_ptr bucket_t *b, *first;

    if (!transactional)
    {
        i = HASH(val);
        first = b = set->buckets[i];
        result = 1;
        while (b != NULL)
        {
            if (b->val == val)
            {
                result = 0;
                break;
            }
            b = b->next;
        }

        if (result)
        {
            set->buckets[i] = new_entry(val, first, 0);
        }
    }
    else
    {
        // TM_START(0, RW);
        // i = HASH(val);
        // first = b = (bucket_t *)TM_LOAD(&set->buckets[i]);
        // result = 1;
        // while (b != NULL)
        // {
        //     if (TM_LOAD(&b->val) == val)
        //     {
        //         result = 0;
        //         break;
        //     }
        //     b = (bucket_t *)TM_LOAD(&b->next);
        // }
        // if (result)
        // {
        //     TM_STORE(&set->buckets[i], new_entry(val, first, 1));
        // }
        // TM_COMMIT;
    }

    return result;
}

int set_remove(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val)
{
    int result, i;
//     bucket_t *b, *prev;

// #ifdef DEBUG
//     printf("++> set_remove(%d)\n", val);
//     IO_FLUSH;
// #endif

//     if (!td)
//     {
//         i = HASH(val);
//         prev = b = set->buckets[i];
//         result = 0;
//         while (b != NULL)
//         {
//             if (b->val == val)
//             {
//                 result = 1;
//                 break;
//             }
//             prev = b;
//             b = b->next;
//         }
//         if (result)
//         {
//             if (prev == b)
//             {
//                 /* First element of bucket */
//                 set->buckets[i] = b->next;
//             }
//             else
//             {
//                 prev->next = b->next;
//             }
//             free(b);
//         }
//     }
//     else
//     {
//         TM_START(0, RW);
//         i = HASH(val);
//         prev = b = (bucket_t *)TM_LOAD(&set->buckets[i]);
//         result = 0;
//         while (b != NULL)
//         {
//             if (TM_LOAD(&b->val) == val)
//             {
//                 result = 1;
//                 break;
//             }
//             prev = b;
//             b = (bucket_t *)TM_LOAD(&b->next);
//         }
//         if (result)
//         {
//             if (prev == b)
//             {
//                 /* First element of bucket */
//                 TM_STORE(&set->buckets[i], TM_LOAD(&b->next));
//             }
//             else
//             {
//                 TM_STORE(&prev->next, TM_LOAD(&b->next));
//             }
//             /* Free memory (delayed until commit) */
//             TM_FREE2(b, sizeof(bucket_t));
//         }
//         TM_COMMIT;
//     }

    return result;
}

#endif