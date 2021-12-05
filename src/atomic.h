#ifndef _ATOMIC_H_

# define _ATOMIC_H_

#define ATOMIC_LOAD_ACQ(a) 			(*(a))
#define ATOMIC_LOAD(a)				(*((volatile size_t *)(a)))
#define ATOMIC_STORE(a, v)			(*((volatile size_t *)(a)) = (size_t)(v))
#define ATOMIC_STORE_REL(a, v)		(*((volatile size_t *)(a)) = (size_t)(v))

#define ATOMIC_CAS_FULL(a, e, v)  	(*(a) = (v), 1)
#define ATOMIC_FETCH_INC_FULL(a)    ((*(a))++)


#endif /* _ATOMIC_H_ */