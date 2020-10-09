#ifndef util_SyOSAtomicT_h
#define util_SyOSAtomicT_h

#pragma once

#include "stdint.h"
#include "SyMutex.h"

#if defined(SY_APPLE)
    #include <libkern/OSAtomic.h>
    typedef int32_t ATOMIC_INT_TYPE;
    #define SY_HAS_BUILTIN_ATOMIC_OP
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    typedef int ATOMIC_INT_TYPE;
    #define SY_HAS_BUILTIN_ATOMIC_OP
#elif defined(SY_WIN32)
    #define SY_HAS_BUILTIN_ATOMIC_OP
    typedef long ATOMIC_INT_TYPE;
#else
    typedef long ATOMIC_INT_TYPE;
#endif

START_UTIL_NS

#ifdef SY_HAS_BUILTIN_ATOMIC_OP

template<typename T>
T WmeAtomicIncrease(volatile T *value)
{
#ifdef WIN32
    return InterlockedIncrement(value);
#elif defined(SY_APPLE)
    return OSAtomicIncrement32(value);
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    T retVal = __sync_fetch_and_add(value, 1);
    return ++retVal;
#endif
}

template<typename T>
T WmeAtomicDecrease(volatile T *value)
{
#ifdef WIN32
    return InterlockedDecrement(value);
#elif defined(SY_APPLE)
    return OSAtomicDecrement32(value);
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    T retVal = __sync_fetch_and_sub(value, 1);
    return --retVal;
#endif
}

template<typename T>
T WmeAtomicAdd(volatile T *value, T added)
{
#ifdef SY_WIN32
    return ::InterlockedExchangeAdd(value, added) + added;
#elif defined(SY_APPLE)
    return OSAtomicAdd32(added, value);
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    long nRet = __sync_fetch_and_add(value, added);
    return nRet + added;
#endif
}

template<typename T>
void WmeAtomicExchange(volatile T *value, T exchanged)
{
#ifdef SY_WIN32
    ::InterlockedExchange(value, exchanged);
#elif defined(SY_APPLE)
    int i = 0;
    while (!OSAtomicCompareAndSwap32(*value, exchanged, value) && i++ < 100);
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    int prev;
    do {
        prev = *value;
    } while (__sync_val_compare_and_swap(value, prev, exchanged) != prev);
#endif // SY_WIN32
}

#endif

END_UTIL_NS

#endif
