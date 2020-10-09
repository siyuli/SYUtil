#ifndef SY_THREADEX_H
#define SY_THREADEX_H

#include <pthread.h>
#include <semaphore.h>

typedef pthread_t SY_THREAD_ID;
typedef SY_THREAD_ID SY_THREAD_HANDLE;
typedef sem_t SY_SEMAPHORE_T;
typedef pthread_mutex_t SY_THREAD_MUTEX_T;

#endif //SY_THREADEX_H
