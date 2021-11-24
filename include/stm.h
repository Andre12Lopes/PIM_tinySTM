#ifndef _STM_H_
#define _STM_H_

#include <stdint.h>
#include <setjmp.h> /* NOT SURE IF SUPORTED */

typedef uintptr_t stm_word_t; /* NOT SURE */

sigjmp_buf *stm_start_tx(struct stm_tx *tx);

stm_word_t stm_load_tx(struct stm_tx *tx, volatile stm_word_t *addr);

void stm_store_tx(struct stm_tx *tx, volatile stm_word_t *addr, stm_word_t value);

int stm_commit_tx(struct stm_tx *tx);

#endif /* _STM_H_ */