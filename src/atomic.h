#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#define ATOMIC_LOAD_ACQ(a) 		(AO_load_read((volatile size_t *)(a)))
#define ATOMIC_LOAD(a) 			(*((volatile size_t *)(a)))
#define ATOMIC_STORE(a, v) 		(*((volatile size_t *)(a)) = (size_t)(v))
#define ATOMIC_STORE_REL(a, v) 	(*((volatile size_t *)(a)) = (size_t)(v))

#define ATOMIC_CAS_FULL(a, e, v) (*(a) = (v), 1)
#define ATOMIC_FETCH_INC_FULL(a) ((*(a))++)

static inline size_t 
AO_load_read(const volatile size_t *addr)
{
    size_t result = *((size_t *)addr);
    __asm__ __volatile__("" : : : "memory");

    return result;
}

#endif /* _ATOMIC_H_ */