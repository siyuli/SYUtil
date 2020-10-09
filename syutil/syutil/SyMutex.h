#ifndef SYMUTEX_H
#define SYMUTEX_H

#include "SyError.h"
#include "SyThreadMisc.h"

START_UTIL_NS

class SY_OS_EXPORT CSyMutexThreadBase
{
public:
    CSyMutexThreadBase();
    virtual ~CSyMutexThreadBase();

public:
    SyResult Lock();
    SyResult UnLock();
    SyResult TryLock();

    SY_THREAD_MUTEX_T &GetMutexType() { return m_Lock;}

protected:
    SY_THREAD_MUTEX_T m_Lock;
};


/*!
that spin_lock has been enabled from linux kernel 2.6 with higher performance than mutex
*/
#ifdef SY_ENABLE_SPINLOCK
class SY_OS_EXPORT CSySpinLockBase
{
protected:
    CSySpinLockBase();
    virtual ~CSySpinLockBase();

public:
    SyResult Lock();
    SyResult UnLock();
    SyResult TryLock();

    pthread_spinlock_t &GetMutexType() { return m_Lock;}
protected:
    pthread_spinlock_t m_Lock;

};


class SY_OS_EXPORT CSySpinLockSP : public CSySpinLockBase
{
public:
    CSySpinLockSP();

private:
    // = Prevent assignment and initialization.
    void operator = (const CSySpinLockSP &);
    CSySpinLockSP(const CSySpinLockSP &);

};

class SY_OS_EXPORT CSySpinLockMP : public CSySpinLockBase
{
public:
    CSySpinLockMP();

private:
    // = Prevent assignment and initialization.
    void operator = (const CSySpinLockMP &);
    CSySpinLockMP(const CSySpinLockMP &);
};

#endif

/**
 * Mainly copyed from <ACE_Recursive_Thread_Mutex>.
 * <CSyMutexThreadRecursive> allows mutex locking many times in the same threads.
 */
class SY_OS_EXPORT CSyMutexThreadRecursive : public CSyMutexThreadBase
{
public:
    CSyMutexThreadRecursive();
    ~CSyMutexThreadRecursive();

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyMutexThreadRecursive &);
    CSyMutexThreadRecursive(const CSyMutexThreadRecursive &);
};

class SY_OS_EXPORT CSyMutexThread : public CSyMutexThreadBase
{
public:
    CSyMutexThread();
    ~CSyMutexThread();

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyMutexThread &);
    CSyMutexThread(const CSyMutexThread &);
};

/**
 *  <CSyMutexNullSingleThread> checks to ensure running on the same thread
 */
class SY_OS_EXPORT CSyMutexNullSingleThread
{
public:
    CSyMutexNullSingleThread()
    {
    }

    // this function may be invoked in the different thread.
    ~CSyMutexNullSingleThread()
    {
        //      m_Est.EnsureSingleThread();
    }

    SyResult Lock()
    {
        m_Est.EnsureSingleThread();
        return SY_OK;
    }

    SyResult UnLock()
    {
        m_Est.EnsureSingleThread();
        return SY_OK;
    }

    SyResult TryLock()
    {
        m_Est.EnsureSingleThread();
        return SY_OK;
    }

private:
    CSyEnsureSingleThread m_Est;

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyMutexNullSingleThread &);
    CSyMutexNullSingleThread(const CSyMutexNullSingleThread &);
};

// Use <CSyMutexNullSingleThread> instead <CSyMutexNull> to ensure in the single thread.
#if 1
/**
 *  <CSyMutexNull> runs on different threads without operating and checking
 */
class SY_OS_EXPORT CSyMutexNull
{
public:
    CSyMutexNull()
    {
    }

    ~CSyMutexNull()
    {
    }

    SyResult Lock()
    {
        return SY_OK;
    }

    SyResult UnLock()
    {
        return SY_OK;
    }

    SyResult TryLock()
    {
        return SY_OK;
    }

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyMutexNull &);
    CSyMutexNull(const CSyMutexNull &);
};
#endif

template <class MutexType>
class CSyMutexGuardT
{
public:
    CSyMutexGuardT(MutexType &aMutex)
        : m_Mutex(aMutex)
        , m_bLocked(FALSE)
    {
        Lock();
    }

    ~CSyMutexGuardT()
    {
        UnLock();
    }

    SyResult Lock()
    {
        SyResult rv = m_Mutex.Lock();
        m_bLocked = SY_SUCCEEDED(rv) ? TRUE : FALSE;
        return rv;
    }

    SyResult UnLock()
    {
        if (m_bLocked) {
            m_bLocked = FALSE;
            return m_Mutex.UnLock();
        } else {
            return SY_OK;
        }
    }

private:
    MutexType &m_Mutex;
    BOOL m_bLocked;

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyMutexGuardT &);
    CSyMutexGuardT(const CSyMutexGuardT &);
};

/**
 *  <CSyFakeMutex> used to fake lock code block
 */
class SY_OS_EXPORT CSyFakeMutex
{
public:
    CSyFakeMutex();
    ~CSyFakeMutex();
    SyResult Lock();
    SyResult UnLock();
    SyResult TryLock();
private:
    BOOL m_bLocked;
};
END_UTIL_NS

#endif // !SYMUTEX_H
