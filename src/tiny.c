#include <assert.h>
#include <string.h>

#include "tiny.h"
#include "tiny_internal.h"

global_t _tinystm;

void 
stm_init(void)
{
    if (_tinystm.initialized)
    {
        return;
    }

    memset((void *)_tinystm.locks, 0, LOCK_ARRAY_SIZE * sizeof(stm_word_t));
  	// CLOCK = 0

  	_tinystm.initialized = 1;
}

stm_word_t 
stm_load(stm_tx_t *tx, volatile stm_word_t *addr)
{
    return int_stm_load(tx, addr);
}

void 
stm_store(stm_tx_t *tx, volatile stm_word_t *addr, stm_word_t value)
{
  int_stm_store(tx, addr, value);
}