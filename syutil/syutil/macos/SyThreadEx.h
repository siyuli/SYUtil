#ifndef SY_THREADEX_H
#define SY_THREADEX_H

#include <pthread.h>
#include <semaphore.h>

START_UTIL_NS

typedef int sem_t;
enum {
    PTHREAD_MUTEX_TIMED_NP,
    PTHREAD_MUTEX_RECURSIVE_NP,
    PTHREAD_MUTEX_ERRORCHECK_NP,
    PTHREAD_MUTEX_ADAPTIVE_NP,
    PTHREAD_MUTEX_FAST_NP = PTHREAD_MUTEX_ADAPTIVE_NP
};

END_UTIL_NS


typedef pthread_t SY_THREAD_ID;
typedef SY_THREAD_ID SY_THREAD_HANDLE;
typedef sem_t SY_SEMAPHORE_T;
typedef pthread_mutex_t SY_THREAD_MUTEX_T;


#endif //SY_THREADEX_H

