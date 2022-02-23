#ifndef _TINY_H_
#define _TINY_H_

#include <stdint.h>

struct stm_tx;

typedef uintptr_t stm_word_t;

#ifndef RWS_IN_MRAM
#define RWS_IN_MRAM
#endif

enum
{
    /**
     * Abort upon writing a memory location being written by another
     * transaction.
     */
    STM_ABORT_WW_CONFLICT = (1 << 6) | (0x04 << 8), // 1088
    /**
     * Abort upon read due to failed validation.
     */
    STM_ABORT_VAL_READ = (1 << 6) | (0x05 << 8), // 1344
    /**
     * Abort upon writing a memory location being read by another
     * transaction.
     */
    STM_ABORT_RW_CONFLICT = (1 << 6) | (0x02 << 8), // 576
    /**
     * Abort upon write due to failed validation.
     */
    STM_ABORT_VAL_WRITE = (1 << 6) | (0x06 << 8), // 1600
    /**
     * Abort upon commit due to failed validation.
     */
    STM_ABORT_VALIDATE = (1 << 6) | (0x07 << 8),
    /**
     * Abort due to reaching the write set size limit.
     */
    STM_ABORT_EXTEND_WS = (1 << 6) | (0x0C << 8)
};

void stm_init(void);

void stm_start(__mram_ptr struct stm_tx *tx, int tid);

/* TODO: Read does not deal well with floating point operations */
/* TODO: Change return type */
stm_word_t stm_load(__mram_ptr struct stm_tx *tx, volatile stm_word_t *addr);

void stm_store(__mram_ptr struct stm_tx *tx, volatile stm_word_t *addr, stm_word_t value);

int stm_commit(__mram_ptr struct stm_tx *tx);

#endif /* _TINY_H_ */
