/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYS_SELECT_H
#define __SYS_SELECT_H


#include <sys/time.h>






#define __FD_SETSIZE		1024


/* The fd_set member is required to be an array of longs.  */
typedef long int __fd_mask;

#define __NFDBITS	(8 * (int) sizeof (__fd_mask))

#define	FD_SETSIZE		__FD_SETSIZE

#define	__FD_ELT(d)	((d) / __NFDBITS)

# define __FDS_BITS(set) ((set)->__fds_bits)
/* fd_set for select and pselect.  */
typedef struct
{
  __fd_mask __fds_bits[__FD_SETSIZE / __NFDBITS];
} fd_set;


#define	__FD_MASK(d)	((__fd_mask) (1UL << ((d) % __NFDBITS)))


/* We don't use `memset' because this would require a prototype and
the array isn't too big.  */
# define __FD_ZERO(set)  \
do {									      \
  unsigned int __i;							      \
    fd_set *__arr = (set);						      \
      for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i)	      \
        __FDS_BITS (__arr)[__i] = 0;					      \
} while (0)

#define __FD_SET(d, set) \
((void) (__FDS_BITS (set)[__FD_ELT (d)] |= __FD_MASK (d)))

#define	FD_ZERO(fdsetp)		__FD_ZERO (fdsetp)
#define	FD_SET(fd, fdsetp)	__FD_SET (fd, fdsetp)

typedef __fd_mask fd_mask;

#define __FD_ISSET(d, set) \
((__FDS_BITS (set)[__FD_ELT (d)] & __FD_MASK (d)) != 0)
#define	FD_ISSET(fd, fdsetp)	__FD_ISSET (fd, fdsetp)





/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
(if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
after waiting the interval specified therein.  Returns the number of ready
descriptors, or -1 for errors.

This function is a cancellation point and therefore not marked with
__THROW.  */
extern int select (int __nfds, fd_set *restrict __readfds,
                   fd_set *restrict __writefds,
                   fd_set *restrict __exceptfds,
                   struct timeval *restrict __timeout);
  
#endif /* __SYS_SELECT_H */