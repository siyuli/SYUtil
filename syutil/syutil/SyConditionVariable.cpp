
//#include "SyBase.h"
#include "SyConditionVariable.h"
#include "SyTimeValue.h"
#include "SyDebug.h"

#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSySemaphore
//////////////////////////////////////////////////////////////////////

CSySemaphore::CSySemaphore(LONG aInitialCount, LPCSTR aName, LONG aMaximumCount)
{
#ifdef SY_WIN32
#if defined (WP8 )|| defined(UWP)
    m_Semaphore = ::CreateSemaphoreEx(nullptr, aInitialCount, aMaximumCount, nullptr, 0, SEMAPHORE_ALL_ACCESS);
#else
    m_Semaphore = ::CreateSemaphoreA(NULL, aInitialCount, aMaximumCount, aName);
#endif
    if (m_Semaphore == 0) {
        SY_WARNING_TRACE_THIS("CSySemaphore::CSySemaphore, CreateSemaphoreA() failed!"
                              " err=" << ::GetLastError());
        SY_ASSERTE(FALSE);
    }
#else // !SY_WIN32
    if (::sem_init(&m_Semaphore, 0, static_cast<unsigned int>(aInitialCount)) == -1) {
        SY_WARNING_TRACE_THIS("CSySemaphore::CSySemaphore, sem_init() failed!"
                              " err=" << errno);
        SY_ASSERTE(FALSE);
    }
#endif
}

CSySemaphore::~CSySemaphore()
{
#ifdef SY_WIN32
    if (!::CloseHandle(m_Semaphore)) {
        SY_WARNING_TRACE_THIS("CSySemaphore::~CSySemaphore, CloseHandle() failed!"
                              " err=" << ::GetLastError());
    }
#else // !SY_WIN32
    if (::sem_destroy(&m_Semaphore) == -1) {
        SY_WARNING_TRACE_THIS("CSySemaphore::~CSySemaphore, sem_destroy() failed!"
                              " err=" << errno);
    }
#endif // SY_WIN32
}

SyResult CSySemaphore::Lock()
{
#ifdef SY_WIN32
#if defined (WP8 )|| defined (UWP)
    DWORD dwRet = ::WaitForSingleObjectEx(m_Semaphore, INFINITE, FALSE);
#else
    DWORD dwRet = ::WaitForSingleObject(m_Semaphore, INFINITE);
#endif
    switch (dwRet) {
        case WAIT_OBJECT_0:
            return SY_OK;
        default:
            SY_WARNING_TRACE_THIS("CSySemaphore::Lock, WaitForSingleObject() failed!"
                                  " dwRet=" << dwRet <<
                                  " err=" << ::GetLastError());
            return SY_ERROR_FAILURE;
    }
#else // !SY_WIN32
    if (::sem_wait(&m_Semaphore) == -1) {
        SY_WARNING_TRACE_THIS("CSySemaphore::Lock, sem_wait() failed!"
                              " err=" << errno);
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#endif // SY_WIN32
}

SyResult CSySemaphore::UnLock()
{
    return PostN(1);
}

SyResult CSySemaphore::PostN(LONG aCount)
{
    SY_ASSERTE(aCount >= 1);
#ifdef SY_WIN32
    if (!::ReleaseSemaphore(m_Semaphore, aCount, NULL)) {
        SY_WARNING_TRACE_THIS("CSySemaphore::UnLock, ReleaseSemaphore() failed!"
                              " err=" << ::GetLastError());
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#else // !SY_WIN32
    for (LONG i = 0; i < aCount; i++) {
        if (::sem_post(&m_Semaphore) == -1) {
            SY_WARNING_TRACE_THIS("CSySemaphore::UnLock, sem_post() failed!"
                                  " err=" << errno);
            return SY_ERROR_FAILURE;
        }
    }
    return SY_OK;
#endif // SY_WIN32
}


//////////////////////////////////////////////////////////////////////
// class CSyConditionVariableThread
//////////////////////////////////////////////////////////////////////

CSyConditionVariableThread::CSyConditionVariableThread(CSyMutexThread &aMutex)
    : m_MutexExternal(aMutex)
#ifdef SY_WIN32
    , sema_(0)
#endif // SY_WIN32
{
#ifdef SY_WIN32
    waiters_ = 0;
    was_broadcast_ = 0;
#if defined (WP8 )|| defined (UWP)
    waiters_done_ = ::CreateEventEx(NULL, 0, 0, NULL);
#else
    waiters_done_= ::CreateEventA(NULL, 0, 0, NULL);
#endif
    if (waiters_done_ == 0) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::CSyConditionVariableThread, CreateEventA() failed!"
                              " err=" << ::GetLastError());
        SY_ASSERTE(FALSE);
    }
#else // !SY_WIN32
    int nRet = ::pthread_cond_init(&m_Condition, NULL);
    if (nRet != 0) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::CSyConditionVariableThread, pthread_cond_init() failed!"
                              " err=" << nRet);
        SY_ASSERTE(FALSE);
    }
#endif // SY_WIN32
}

CSyConditionVariableThread::~CSyConditionVariableThread()
{
#ifdef SY_WIN32
    if (!::CloseHandle(waiters_done_)) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::~CSyConditionVariableThread, CloseHandle() failed!"
                              " err=" << ::GetLastError());
    }
#else // !SY_WIN32
    int nRet = ::pthread_cond_destroy(&m_Condition);
    if (nRet != 0) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::~CSyConditionVariableThread, pthread_cond_destroy() failed!"
                              " err=" << nRet);
    }
#endif // SY_WIN32
}

SyResult CSyConditionVariableThread::Signal()
{
#ifdef SY_WIN32
    waiters_lock_.Lock();
    int have_waiters = waiters_ > 0;
    waiters_lock_.UnLock();

    if (have_waiters != 0) {
        return sema_.UnLock();
    } else {
        return SY_OK;
    }
#else // !SY_WIN32
    int nRet = ::pthread_cond_signal(&m_Condition);
    if (nRet != 0) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Signal, pthread_cond_signal() failed!"
                              " err=" << nRet);
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#endif // SY_WIN32
}

SyResult CSyConditionVariableThread::Wait(CSyTimeValue *aTimeout)
{
#ifdef SY_WIN32
    // Prevent race conditions on the <waiters_> count.
    waiters_lock_.Lock();
    waiters_++;
    waiters_lock_.UnLock();

    int msec_timeout;
    if (!aTimeout) {
        msec_timeout = INFINITE;
    } else {
        msec_timeout = aTimeout->GetTotalInMsec();
        if (msec_timeout < 0) {
            msec_timeout = 0;
        }
        if (0 == msec_timeout && 0 < aTimeout->GetUsec()) {
            msec_timeout = 1;
        }
    }

    // We keep the lock held just long enough to increment the count of
    // waiters by one.  Note that we can't keep it held across the call
    // to WaitForSingleObject since that will deadlock other calls to
    // ACE_OS::cond_signal().
    SyResult rv = m_MutexExternal.UnLock();
    SY_ASSERTE_RETURN(SY_SUCCEEDED(rv), rv);

    // <CSySemaphore> has not time wait function due to pthread restriction,
    // so we have to use WaitForSingleObject() directly.
#if defined (WP8 )|| defined (UWP)
    DWORD result = ::WaitForSingleObjectEx(sema_.GetSemaphoreType(), msec_timeout, FALSE);
#else
    DWORD result = ::WaitForSingleObject(sema_.GetSemaphoreType(), msec_timeout);
#endif

    waiters_lock_.Lock();
    waiters_--;
    int last_waiter = was_broadcast_ && waiters_ == 0;
    waiters_lock_.UnLock();

    switch (result) {
        case WAIT_OBJECT_0:
            rv = SY_OK;
            break;
        case WAIT_TIMEOUT:
            rv = SY_ERROR_TIMEOUT;
            break;
        default:
            SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Wait, WaitForSingleObject() failed!"
                                  " result=" << result <<
                                  " err=" << ::GetLastError());
            rv = SY_ERROR_FAILURE;
            break;
    }

    if (last_waiter) {
        if (!::SetEvent(waiters_done_)) {
            SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Wait, SetEvent() failed!"
                                  " err=" << ::GetLastError());
        }
    }

    // We must always regain the <external_mutex>, even when errors
    // occur because that's the guarantee that we give to our callers.
    m_MutexExternal.Lock();
    return rv;
#else // !SY_WIN32
    if (!aTimeout) {
        int nRet = ::pthread_cond_wait(&m_Condition, &(m_MutexExternal.GetMutexType()));
        if (nRet != 0) {
            SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Wait, pthread_cond_wait() failed!"
                                  " err=" << nRet);
            return SY_ERROR_FAILURE;
        }
    } else {
#if defined(SY_IOS) || defined(SY_MACOS)
        //TODO, whether pthread_cond_timedwait_relative_np is supported on linux or android
        struct timespec tsBuf;
        tsBuf.tv_sec = aTimeout->GetSec();
        tsBuf.tv_nsec = aTimeout->GetUsec() * 1000;
        int nRet = ::pthread_cond_timedwait_relative_np(
                       &m_Condition,
                       &(m_MutexExternal.GetMutexType()),
                       &tsBuf);

#else
        struct timespec tsBuf;
        CSyTimeValue tvAbs = CSyTimeValue::GetTimeOfDay() + *aTimeout;
        tsBuf.tv_sec = tvAbs.GetSec();
        tsBuf.tv_nsec = tvAbs.GetUsec() * 1000;
        int nRet = ::pthread_cond_timedwait(
                       &m_Condition,
                       &(m_MutexExternal.GetMutexType()),
                       &tsBuf);
#endif
        if (nRet != 0) {
            if (nRet == ETIMEDOUT) {
                return SY_ERROR_TIMEOUT;
            }
            // EINTR is OK.
            else if (nRet == EINTR) {
                return SY_OK;
            } else {
                SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Wait, pthread_cond_timedwait() failed!"
                                      " err=" << nRet);
                return SY_ERROR_FAILURE;
            }
        }
    }
    return SY_OK;
#endif // SY_WIN32
}

SyResult CSyConditionVariableThread::Broadcast()
{
    // The <external_mutex> must be locked before this call is made.
#ifdef SY_WIN32
    // This is needed to ensure that <waiters_> and <was_broadcast_> are
    // consistent relative to each other.
    waiters_lock_.Lock();
    int have_waiters = 0;
    if (waiters_ > 0) {
        // We are broadcasting, even if there is just one waiter...
        // Record the fact that we are broadcasting.  This helps the
        // cond_wait() method know how to optimize itself.  Be sure to
        // set this with the <waiters_lock_> held.
        was_broadcast_ = 1;
        have_waiters = 1;
    }
    waiters_lock_.UnLock();

    SyResult rv = SY_OK;
    if (have_waiters) {
        SyResult rv1 = sema_.PostN(waiters_);
        if (SY_FAILED(rv1)) {
            rv = rv1;
        }
#if defined (WP8 )|| defined (UWP)
        DWORD result = ::WaitForSingleObjectEx(waiters_done_, INFINITE, FALSE);
#else
        DWORD result = ::WaitForSingleObject(waiters_done_, INFINITE);
#endif
        if (result != WAIT_OBJECT_0) {
            SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Broadcast, WaitForSingleObject() failed!"
                                  " result=" << result <<
                                  " err=" << ::GetLastError());
            rv = SY_ERROR_FAILURE;
        }
        was_broadcast_ = 0;
    }
    return rv;
#else // !SY_WIN32
    int nRet = ::pthread_cond_broadcast(&m_Condition);
    if (nRet != 0) {
        SY_WARNING_TRACE_THIS("CSyConditionVariableThread::Signal, pthread_cond_broadcast() failed!"
                              " err=" << nRet);
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#endif // SY_WIN32
}


//////////////////////////////////////////////////////////////////////
// class CSyEventThread
//////////////////////////////////////////////////////////////////////

CSyEventThread::CSyEventThread(BOOL aManualReset, BOOL aInitialState,
#if defined (WP8 )|| defined (UWP)
                               LPCWSTR aName)
#else
                               LPCSTR aName)
#endif
#ifndef SY_WIN32
    : condition_(lock_)
#endif // !SY_WIN32
{
#ifdef SY_WIN32
#if defined (WP8 )|| defined (UWP)
    DWORD dwFlag = 0;
    if (aManualReset) {
        dwFlag |= CREATE_EVENT_MANUAL_RESET;
    }
    if (aInitialState) {
        dwFlag |= CREATE_EVENT_INITIAL_SET;
    }
    handle_ = ::CreateEventEx(NULL, aName, dwFlag, SEMAPHORE_ALL_ACCESS);
#else
    handle_ = ::CreateEventA(NULL, aManualReset, aInitialState, aName);
#endif
    if (handle_ == 0) {
        SY_WARNING_TRACE_THIS("CSyEventThread::CSyEventThread, CreateEventA() failed!"
                              " err=" << ::GetLastError());
        SY_ASSERTE(FALSE);
    }
#else // !SY_WIN32
    manual_reset_ = aManualReset;
    is_signaled_ = aInitialState;
    waiting_threads_ = 0;
#endif // SY_WIN32
}

CSyEventThread::~CSyEventThread()
{
#ifdef SY_WIN32
    if (!::CloseHandle(handle_)) {
        SY_WARNING_TRACE_THIS("CSyEventThread::~CSyEventThread, CloseHandle() failed!"
                              " err=" << ::GetLastError());
    }
#else // !SY_WIN32
    // not need do cleanup.
#endif // SY_WIN32
}

SyResult CSyEventThread::Wait(CSyTimeValue *aTimeout)
{
    SyResult rv;
#ifdef SY_WIN32
    int msec_timeout;
    if (!aTimeout) {
        msec_timeout = INFINITE;
    } else {
        msec_timeout = aTimeout->GetTotalInMsec();
        if (msec_timeout < 0) {
            msec_timeout = 0;
        }
        if (0 == msec_timeout && 0 < aTimeout->GetUsec()) {
            msec_timeout = 1;
        }
    }

#if defined (WP8 )|| defined (UWP)
    DWORD result = ::WaitForSingleObjectEx(handle_, INFINITE, FALSE);
#else
    DWORD result = ::WaitForSingleObject(handle_, msec_timeout);
#endif
    switch (result) {
        case WAIT_OBJECT_0:
            rv = SY_OK;
            break;
        case WAIT_TIMEOUT:
            rv = SY_ERROR_TIMEOUT;
            break;
        default:
            SY_WARNING_TRACE_THIS("CSyEventThread::Wait, WaitForSingleObject() failed!"
                                  " result=" << result <<
                                  " err=" << ::GetLastError());
            rv = SY_ERROR_FAILURE;
            break;
    }
#else // !SY_WIN32
    rv = lock_.Lock();
    SY_ASSERTE_RETURN(SY_SUCCEEDED(rv), rv);

    if (is_signaled_ == 1) {
        // event is currently signaled
        if (manual_reset_ == 0)
            // AUTO: reset state
        {
            is_signaled_ = 0;
        }
    } else {
        // event is currently not signaled
        waiting_threads_++;
        rv = condition_.Wait(aTimeout);
        waiting_threads_--;
    }

    lock_.UnLock();
#endif // SY_WIN32
    return rv;
}

SyResult CSyEventThread::Signal()
{
#ifdef SY_WIN32
    if (!::SetEvent(handle_)) {
        SY_WARNING_TRACE_THIS("CSyEventThread::Signal, SetEvent failed!"
                              " err=" << ::GetLastError());
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#else // !SY_WIN32
    SyResult rv;
    rv = lock_.Lock();
    SY_ASSERTE_RETURN(SY_SUCCEEDED(rv), rv);

    if (manual_reset_ == 1) {
        // Manual-reset event.
        is_signaled_ = 1;
        rv = condition_.Broadcast();
    } else {
        // Auto-reset event
        if (waiting_threads_ == 0) {
            is_signaled_ = 1;
        } else {
            rv = condition_.Signal();
        }
    }

    lock_.UnLock();
    return rv;
#endif // SY_WIN32
}

SyResult CSyEventThread::Reset()
{
#ifdef SY_WIN32
    if (!::ResetEvent(handle_)) {
        SY_WARNING_TRACE_THIS("CSyEventThread::Reset, Reset() failed!"
                              " err=" << ::GetLastError());
        return SY_ERROR_FAILURE;
    } else {
        return SY_OK;
    }
#endif// !SY_WIN32
    return SY_ERROR_FAILURE;
}
