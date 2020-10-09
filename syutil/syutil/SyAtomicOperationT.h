#ifndef SYATOMICOPERATIONT_H
#define SYATOMICOPERATIONT_H

#include "SyOSAtomicT.h"

START_UTIL_NS

extern CSyMutexThread s_ReferenceControlMutexThread;

template <class MutexType>
class CSyAtomicOperationT
{
public:
    CSyAtomicOperationT(long aValue = 0)
        : m_pMutex(NULL)
        , m_lValue(aValue)
    {
        m_pMutex = &s_ReferenceControlMutexThread;
    }

    long operator++ ()
    {
        CSyMutexGuardT<MutexType> theGuard(*m_pMutex);
        return ++m_lValue;
    }

    long operator++ (int)
    {
        return ++*this - 1;
    }

    long operator-- (void)
    {
        CSyMutexGuardT<MutexType> theGuard(*m_pMutex);
        return --m_lValue;
    }

    long operator-- (int)
    {
        return --*this + 1;
    }

    int operator== (long aRight) const
    {
        return (this->m_lValue == aRight);
    }

    int operator!= (long aRight) const
    {
        return (this->m_lValue != aRight);
    }

    int operator>= (long aRight) const
    {
        return (this->m_lValue >= aRight);
    }

    int operator> (long aRight) const
    {
        return (this->m_lValue > aRight);
    }

    int operator<= (long aRight) const
    {
        return (this->m_lValue <= aRight);
    }

    int operator< (long aRight) const
    {
        return (this->m_lValue < aRight);
    }

    void operator= (long aRight)
    {
        CSyMutexGuardT<MutexType> theGuard(*m_pMutex);
        m_lValue = aRight;
    }

    void operator= (const CSyAtomicOperationT &aRight)
    {
        CSyMutexGuardT<MutexType> theGuard(*m_pMutex);
        m_lValue = aRight.m_lValue;
    }

    long GetValue() const
    {
        return this->m_lValue;
    }

private:
    MutexType *m_pMutex;
    long m_lValue;
};

#if defined(SY_HAS_BUILTIN_ATOMIC_OP)
template<> class CSyAtomicOperationT<CSyMutexThread>
{
public:
    CSyAtomicOperationT(ATOMIC_INT_TYPE aValue = 0)
        : m_lValue(aValue)
    {
    }

    /// Atomically pre-increment <m_lValue>.
    long operator++ ()
    {
        return WmeAtomicIncrease(&m_lValue);
    }

    /// Atomically post-increment <m_lValue>.
    long operator++ (int)
    {
        return ++*this - 1;
    }

    /// Atomically pre-decrement <m_lValue>.
    long operator-- (void)
    {
        return WmeAtomicDecrease(&m_lValue);
    }

    /// Atomically post-decrement <m_lValue>.
    long operator-- (int)
    {
        return --*this + 1;
    }

    /// Atomically increment <m_lValue> by aRight.
    long operator+= (ATOMIC_INT_TYPE aRight)
    {
        return WmeAtomicAdd(&m_lValue, aRight);
    }

    /// Atomically decrement <m_lValue> by aRight.
    long operator-= (ATOMIC_INT_TYPE aRight)
    {
        return WmeAtomicAdd(&m_lValue, -aRight);
    }

    /// Atomically compare <m_lValue> with aRight.
    int operator== (long aRight) const
    {
        return (this->m_lValue == aRight);
    }

    /// Atomically compare <m_lValue> with aRight.
    int operator!= (long aRight) const
    {
        return (this->m_lValue != aRight);
    }

    /// Atomically check if <m_lValue> greater than or equal to aRight.
    int operator>= (long aRight) const
    {
        return (this->m_lValue >= aRight);
    }

    /// Atomically check if <m_lValue> greater than aRight.
    int operator> (long aRight) const
    {
        return (this->m_lValue > aRight);
    }

    /// Atomically check if <m_lValue> less than or equal to aRight.
    int operator<= (long aRight) const
    {
        return (this->m_lValue <= aRight);
    }

    /// Atomically check if <m_lValue> less than aRight.
    int operator< (long aRight) const
    {
        return (this->m_lValue < aRight);
    }

    /// Atomically assign aRight to <m_lValue>.
    void operator= (ATOMIC_INT_TYPE aRight)
    {
        WmeAtomicExchange(&m_lValue, aRight);
    }

    /// Atomically assign <aRight> to <m_lValue>.
    void operator= (const CSyAtomicOperationT &aRight)
    {
        WmeAtomicExchange(&m_lValue, aRight.m_lValue);
    }

    /// Explicitly return <m_lValue>.
    long GetValue() const
    {
        return this->m_lValue;
    }

private:
    /// Current object decorated by the atomic opearation.
    volatile ATOMIC_INT_TYPE m_lValue;
};

#endif // SY_HAS_BUILTIN_ATOMIC_OP


/// Specialization of <CSyAtomicOperationT> for single-thread.
/// Every <CSyAtomicOperationT> has its own <CSyMutexNullSingleThread>.
template<> class CSyAtomicOperationT<CSyMutexNullSingleThread>
{
public:
    CSyAtomicOperationT(long aValue = 0)
        : m_lValue(aValue)
    {
    }

    /// Atomically pre-increment <m_lValue>.
    long operator++ ()
    {
        m_Est.EnsureSingleThread();
        return ++this->m_lValue;
    }

    /// Atomically post-increment <m_lValue>.
    long operator++ (int)
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue++;
    }

    /// Atomically increment <m_lValue> by aRight.
    long operator+= (long aRight)
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue += aRight;
    }

    /// Atomically pre-decrement <m_lValue>.
    long operator-- (void)
    {
        m_Est.EnsureSingleThread();
        return --this->m_lValue;
    }

    /// Atomically post-decrement <m_lValue>.
    long operator-- (int)
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue--;
    }

    /// Atomically decrement <m_lValue> by aRight.
    long operator-= (long aRight)
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue -= aRight;
    }

    /// Atomically compare <m_lValue> with aRight.
    int operator== (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue == aRight;
    }

    /// Atomically compare <m_lValue> with aRight.
    int operator!= (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return !(*this == aRight);
    }

    /// Atomically check if <m_lValue> greater than or equal to aRight.
    int operator>= (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue >= aRight;
    }

    /// Atomically check if <m_lValue> greater than aRight.
    int operator> (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue > aRight;
    }

    /// Atomically check if <m_lValue> less than or equal to aRight.
    int operator<= (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue <= aRight;
    }

    /// Atomically check if <m_lValue> less than aRight.
    int operator< (long aRight) const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue < aRight;
    }

    /// Atomically assign aRight to <m_lValue>.
    void operator= (long aRight)
    {
        m_Est.EnsureSingleThread();
        this->m_lValue = aRight;
    }

    /// Atomically assign <aRight> to <m_lValue>.
    void operator= (const CSyAtomicOperationT &aRight)
    {
        if (&aRight == this) {
            return;    // Avoid deadlock...
        }

        m_Est.EnsureSingleThread();
        this->m_lValue = aRight.GetValue();
    }

    /// Explicitly return <m_lValue>.
    long GetValue() const
    {
        m_Est.EnsureSingleThread();
        return this->m_lValue;
    }

private:
    CSyEnsureSingleThread m_Est;
    /// Current object decorated by the atomic opearation.
    volatile long m_lValue;
};

END_UTIL_NS

#endif // SYATOMICOPERATIONT_H
