#ifndef _HASH_SET_H_
#define _HASH_SET_H_

#define INIT_SET_PARAMETERS            /* Nothing */

typedef intptr_t val_t;

typedef struct bucket
{
    val_t val;
    __mram_ptr struct bucket *next;
} bucket_t;

typedef struct intset
{
    __mram_ptr bucket_t **buckets;
} intset_t;

__mram_ptr intset_t *
set_new();

int 
set_contains(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val);

int 
set_add(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val, int transactional);

int 
set_remove(stm_tx_t *tx, uint64_t *t_aborts, __mram_ptr intset_t *set, val_t val);

#endif /* _HASH_SET_H_ */