#ifndef SYCONDITIONVARIABLE_H
#define SYCONDITIONVARIABLE_H

#include "SyMutex.h"

START_UTIL_NS

/**
 * Mainly copied from <ACE_Semaphore>
 * Wrapper for Dijkstra style general semaphores.
 */
class SY_OS_EXPORT CSySemaphore
{
public:
    CSySemaphore(LONG aInitialCount = 0,
                 LPCSTR aName = NULL,
                 LONG aMaximumCount = 0x7fffffff);

    ~CSySemaphore();

    /// Block the thread until the semaphore count becomes
    /// greater than 0, then decrement it.
    SyResult Lock();

    /// Increment the semaphore by 1, potentially unblocking a waiting thread.
    SyResult UnLock();

    /// Not supported yet.
    //  SyResult TryLock();

    // No time wait function due to pthread restriction.
    //  SyResult Wait(CSyTimeValue *aTimeout = NULL);

    /// Increment the semaphore by <aCount>, potentially
    /// unblocking waiting threads.
    SyResult PostN(LONG aCount);

    SY_SEMAPHORE_T &GetSemaphoreType() { return m_Semaphore;}

private:
    SY_SEMAPHORE_T m_Semaphore;
};

class CSyTimeValue;

/**
 * Mainly copyed from <ACE_Condition>.
 * <CSyConditionVariableThread> allows threads to block until shared
 * data changes state.
 */
class SY_OS_EXPORT CSyConditionVariableThread
{
public:
    CSyConditionVariableThread(CSyMutexThread &aMutex);
    ~CSyConditionVariableThread();

    /// Block on condition.
    /// <aTimeout> is relative time.
    SyResult Wait(CSyTimeValue *aTimeout = NULL);

    /// Signal one waiting hread.
    SyResult Signal();

    /// Signal all waiting thread.
    SyResult Broadcast();

    /// Return the underlying mutex.
    CSyMutexThread &GetUnderlyingMutex() { return m_MutexExternal; }

private:
    CSyMutexThread &m_MutexExternal;

#ifdef SY_WIN32
    /// Number of waiting threads.
    long waiters_;
    /// Serialize access to the waiters count.
    CSyMutexThread waiters_lock_;
    /// Queue up threads waiting for the condition to become signaled.
    CSySemaphore sema_;
    /**
     * An auto reset event used by the broadcast/signal thread to wait
     * for the waiting thread(s) to wake up and get a chance at the
     * semaphore.
     */
    HANDLE waiters_done_;
    /// Keeps track of whether we were broadcasting or just signaling.
    size_t was_broadcast_;
#else
    pthread_cond_t m_Condition;
#endif // SY_WIN32
};

/**
 * Mainly copied from <ACE_Event>
 *
 * @brief A wrapper around the Win32 event locking mechanism.
 *
 * Portable implementation of an Event mechanism, which is
 * native to Win32, but must be emulated on UNIX.  Note that
 * this only provides global naming support on Win32.
 */
class SY_OS_EXPORT CSyEventThread
{
public:
    CSyEventThread(BOOL aManualReset = FALSE,
                   BOOL aInitialState = FALSE,
#if defined (WP8) || defined(UWP) 
                   LPCWSTR aName = NULL);
#else
                   LPCSTR aName = NULL);
#endif

    ~CSyEventThread();

    /**
     * if MANUAL reset
     *    sleep till the event becomes signaled
     *    event remains signaled after wait() completes.
     * else AUTO reset
     *    sleep till the event becomes signaled
     *    event resets wait() completes.
     * <aTimeout> is relative time.
     */
    SyResult Wait(CSyTimeValue *aTimeout = NULL);

    /**
     * if MANUAL reset
     *    wake up all waiting threads
     *    set to signaled state
     * else AUTO reset
     *    if no thread is waiting, set to signaled state
     *    if thread(s) are waiting, wake up one waiting thread and
     *    reset event
     */
    SyResult Signal();

    /// Set to nonsignaled state.
    SyResult Reset();

private:
#ifdef SY_WIN32
    HANDLE handle_;
#else
    /// Protect critical section.
    CSyMutexThread lock_;

    /// Keeps track of waiters.
    CSyConditionVariableThread condition_;

    /// Specifies if this is an auto- or manual-reset event.
    int manual_reset_;

    /// "True" if signaled.
    int is_signaled_;

    /// Number of waiting threads.
    u_long waiting_threads_;
#endif
};

END_UTIL_NS

#endif // !SYCONDITIONVARIABLE_H
