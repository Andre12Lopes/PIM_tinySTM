#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#define ATOMIC_LOAD_ACQ(a) (AO_load_read((volatile size_t *)(a)))
#define ATOMIC_LOAD(a) (*((volatile size_t *)(a)))
#define ATOMIC_STORE(a, v) (*((volatile size_t *)(a)) = (size_t)(v))
#define ATOMIC_STORE_REL(a, v) (AO_store_write((volatile size_t *)(a), (size_t)(v)))

#define ATOMIC_FETCH_INC_FULL(a, v) (AO_int_fetch_and_add_full((volatile size_t *)(a), v))

#define ATOMIC_B_WRITE __asm__ __volatile__("" : : : "memory")

static inline size_t 
AO_load_read(const volatile size_t *addr)
{
    size_t result = *((size_t *)addr);
    __asm__ __volatile__("" : : : "memory");

    return result;
}

static inline void 
AO_store_write(volatile size_t *addr, size_t val)
{
    __asm__ __volatile__("" : : : "memory");
    (*(size_t *)addr) = val;
}

static inline size_t 
AO_int_fetch_and_add_full(volatile size_t *addr, size_t inc)
{
    size_t result;

    __asm__ __volatile__("acquire %[p], 0, nz, ." : : [p] "r"(addr) :);

    (*addr) += inc;
    result = (*addr);

    __asm__ __volatile__("release %[p], 0, nz, .+1" : : [p] "r"(addr) :);

    return result;
}

static inline void
acquire(volatile size_t *addr)
{
    __asm__ __volatile__("acquire %[p], 0, nz, ." : : [p] "r"(addr) :);
}

static inline void
release(volatile size_t *addr)
{
    __asm__ __volatile__("release %[p], 0, nz, .+1" : : [p] "r"(addr) :);
}

#endif /* _ATOMIC_H_ */