#include <mram.h>

#include "dpu_alloc.h"


#define META_SIZE sizeof(struct block_meta)

struct block_meta
{
    size_t size;
    int free;
    __mram_ptr struct block_meta *next;
};

__mram_ptr void *meta_base = NULL;
__mram_ptr void *mram_heap_pointer = DPU_MRAM_HEAP_POINTER;

__mram_ptr struct block_meta *new_block(__mram_ptr struct block_meta *last, size_t size)
{
	__mram_ptr struct block_meta *block;

	/* MUTUAL EXCLUSION? */

	block = mram_heap_pointer;

	mram_heap_pointer += size + META_SIZE;

	/* MUTUAL EXCLUSION? */

	if (last)
	{
		last->next = block;
	}

	block->size = size;
	block->free = 0;
	block->next = NULL;

	return block;
}


__mram_ptr struct block_meta *find_free_block(__mram_ptr struct block_meta **last, size_t size)
{
	__mram_ptr struct block_meta *current = meta_base;

	while (current && (!current->free || current->size < size))
	{
		*last = current;
		current = current->next;
	}

	return current;
}


__mram_ptr void *mram_malloc(size_t size)
{
	__mram_ptr struct block_meta *block = NULL;
	__mram_ptr struct block_meta *last = NULL;

	if (size <= 0)
	{
		return NULL;
	}

	if (!meta_base)
	{
		block = new_block(NULL, size);
		meta_base = block;
	}
	else 
	{
		last = meta_base;
		block = find_free_block(&last, size);
		if (!block)
		{
			block = new_block(last, size);
		}
		else 
		{
			block->free = 0;
		}
	}

	// Returns pointer to memory position after the metadata block,
	// which is the alocated memory
	return (block + 1); 
}

void mram_free(__mram_ptr void *ptr)
{
	if (!ptr)
	{
		return;
	}

	__mram_ptr struct block_meta *block = ((__mram_ptr struct block_meta *)ptr) - 1;

	block->free = 1;
}