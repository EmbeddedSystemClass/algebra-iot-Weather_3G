/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYS_TIME_H
#define __SYS_TIME_H



#include <time.h>
#include <sys/types.h>

struct  timeval {
   time_t         tv_sec  ;  //  Seconds. 
suseconds_t    tv_usec  ;  // Microseconds. 
};  

int gettimeofday(struct timeval *restrict tp, void *restrict tzp);

//#define CLOCKS_PER_SEC (1000)
time_t __time32 (time_t * p);
clock_t clock (void) ;
//time_t __mktime32(struct tm *info);

//MOJA F-ja za vrijeme
int getSystemTime();

#endif /* __SYS_TIME_H */