
//#include "SyBase.h"
#include "SyDebug.h"
#include "SyMutex.h"
//////////////////////////////////////////////////////////////////////
// class CSyMutexThreadBase
//////////////////////////////////////////////////////////////////////

USE_UTIL_NS

CSyMutexThreadBase::CSyMutexThreadBase()
{
}

CSyMutexThreadBase::~CSyMutexThreadBase()
{
#ifdef SY_WIN32
    ::DeleteCriticalSection(&m_Lock);
#else
    int nRet = ::pthread_mutex_destroy(&m_Lock);
    if (nRet != 0)  {
        SY_ERROR_TRACE_THIS("CSyMutexThreadBase::~CSyMutexThreadBase, pthread_mutex_destroy() failed! err=" << nRet);
    }
#endif // SY_WIN32
}

SyResult CSyMutexThreadBase::Lock()
{
#ifdef SY_WIN32
    ::EnterCriticalSection(&m_Lock);
    return SY_OK;
#else
    int nRet = ::pthread_mutex_lock(&m_Lock);
    if (nRet == 0) {
        return SY_OK;
    } else {
        //SY_ERROR_TRACE_THIS("CSyMutexThreadBase::Lock, pthread_mutex_lock() failed! err=" << nRet);
        return SY_ERROR_FAILURE;
    }
#endif // SY_WIN32
}

SyResult CSyMutexThreadBase::UnLock()
{
#ifdef SY_WIN32
    ::LeaveCriticalSection(&m_Lock);
    return SY_OK;
#else
    int nRet = ::pthread_mutex_unlock(&m_Lock);
    if (nRet == 0) {
        return SY_OK;
    } else {
        SY_ERROR_TRACE_THIS("CSyMutexThreadBase::UnLock, pthread_mutex_unlock() failed! err=" << nRet);
        return SY_ERROR_FAILURE;
    }
#endif // SY_WIN32
}

SyResult CSyMutexThreadBase::TryLock()
{
#ifdef SY_WIN32
    BOOL bRet = ::TryEnterCriticalSection(&m_Lock);
    return bRet ? SY_OK : SY_ERROR_FAILURE;
#else
    int nRet = ::pthread_mutex_trylock(&m_Lock);
    return (nRet == 0) ? SY_OK : SY_ERROR_FAILURE;
#endif // SY_WIN32
}


//////////////////////////////////////////////////////////////////////
// class CSyMutexThreadRecursive
//////////////////////////////////////////////////////////////////////

CSyMutexThreadRecursive::CSyMutexThreadRecursive()
{
#ifdef SY_WIN32
#if defined (WP8 ) || defined (UWP)
    ::InitializeCriticalSectionEx(&m_Lock, 0, 0);
#else
    ::InitializeCriticalSection(&m_Lock);
#endif
#else
    pthread_mutexattr_t mutexattr;
    ::pthread_mutexattr_init(&mutexattr);
#if !defined SY_SOLARIS && !defined SY_APPLE
    ::pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
    ::pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
#endif
    int nRet = ::pthread_mutex_init(&m_Lock, &mutexattr);
    ::pthread_mutexattr_destroy(&mutexattr);
    if (nRet != 0)  {
        SY_ERROR_TRACE_THIS("CSyMutexThreadRecursive::CSyMutexThreadRecursive, pthread_mutex_init() failed! err=" << nRet);
    }
#endif // SY_WIN32
}

CSyMutexThreadRecursive::~CSyMutexThreadRecursive()
{
}


//////////////////////////////////////////////////////////////////////
// class CSyMutexThreadRecursive
//////////////////////////////////////////////////////////////////////

CSyMutexThread::CSyMutexThread()
{
#ifdef SY_WIN32
#if defined (WP8 ) || defined (UWP)
    InitializeCriticalSectionEx(&m_Lock, 0, 0);
#else
    ::InitializeCriticalSection(&m_Lock);
#endif
#else
    pthread_mutexattr_t mutexattr;
    ::pthread_mutexattr_init(&mutexattr);
#ifndef SY_SOLARIS //todo: how is solaris
#ifdef SY_ANDROID
#define PTHREAD_MUTEX_FAST_NP 0
#endif
    ::pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_FAST_NP);
#endif
    int nRet = ::pthread_mutex_init(&m_Lock, &mutexattr);
    ::pthread_mutexattr_destroy(&mutexattr);
    if (nRet != 0)  {
        SY_ERROR_TRACE_THIS("CSyMutexThread::CSyMutexThread, pthread_mutex_init() failed! err=" << nRet);
    }
#endif // SY_WIN32
}

CSyMutexThread::~CSyMutexThread()
{
}

CSyFakeMutex::CSyFakeMutex():m_bLocked(FALSE)
{
}

CSyFakeMutex::~CSyFakeMutex()
{
}

SyResult CSyFakeMutex::Lock()
{
    m_bLocked = TRUE;
    return SY_OK;
}

SyResult CSyFakeMutex::UnLock()
{
    m_bLocked = FALSE;
    return SY_OK;
}

SyResult CSyFakeMutex::TryLock()
{
    if (m_bLocked) {
        return SY_ERROR_FAILURE;
    }
    Lock();
    return SY_OK;
}

/*! spin lock */

#ifdef SY_ENABLE_SPINLOCK
CSySpinLockBase::CSySpinLockBase()
{

}
CSySpinLockBase::~CSySpinLockBase()
{
    pthread_spin_destroy(&m_Lock);
}

SyResult CSySpinLockBase::Lock()
{
    return pthread_spin_lock(&m_Lock);
}
SyResult CSySpinLockBase::UnLock()
{
    return pthread_spin_unlock(&m_Lock);
}
SyResult CSySpinLockBase::TryLock()
{
    return pthread_spin_trylock(&m_Lock);
}


CSySpinLockSP::CSySpinLockSP()
{
    pthread_spin_init(&m_Lock, PTHREAD_PROCESS_PRIVATE);
}


CSySpinLockMP::CSySpinLockMP()
{
    pthread_spin_init(&m_Lock, PTHREAD_PROCESS_SHARED);

}

#endif

