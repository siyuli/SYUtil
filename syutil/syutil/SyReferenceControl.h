#ifndef SYREFERENCECONTROL_H
#define SYREFERENCECONTROL_H

#include "SyAtomicOperationT.h"
#include "SyAssert.h"

START_UTIL_NS

class SY_OS_EXPORT ISyReferenceControl
{
public:
    virtual DWORD AddReference() = 0;
    virtual DWORD ReleaseReference() = 0;

protected:
    virtual ~ISyReferenceControl() {}
};

/**
 *  ReferenceControl basic classes, for mutil-thread.
 *  TODO: use AtomicIncrement instead MutexThread!
 */
template <class MutexType> class CSyReferenceControlT
{
public:
    CSyReferenceControlT()
    {
    }

    virtual ~CSyReferenceControlT()
    {
    }

    DWORD AddReference()
    {
        return static_cast<DWORD>(++m_Atomic);
    }

    DWORD ReleaseReference()
    {
        DWORD dwRef = static_cast<DWORD>(--m_Atomic);
        if (dwRef == 0) {
            OnReferenceDestory();
        }
        return dwRef;
    }

    DWORD GetReference()
    {
        return static_cast<DWORD>(m_Atomic.GetValue());
    }

protected:
    virtual void OnReferenceDestory()
    {
        delete this;
    }

    CSyAtomicOperationT<MutexType> m_Atomic;
};

typedef CSyReferenceControlT<CSyMutexNullSingleThread> CSyReferenceControlSingleThread;
typedef CSyReferenceControlT<CSyMutexThread> CSyReferenceControlMutilThread;


/**
 *  Auto pointer for ReferenceControl
 */
template <class T> class CSyComAutoPtr
{
public:
    CSyComAutoPtr(T *aPtr = NULL)
        : m_pRawPtr(aPtr)
    {
        if (m_pRawPtr) {
            m_pRawPtr->AddReference();
        }
    }

    CSyComAutoPtr(const CSyComAutoPtr &aAutoPtr)
        : m_pRawPtr(aAutoPtr.m_pRawPtr)
    {
        if (m_pRawPtr) {
            m_pRawPtr->AddReference();
        }
    }

    ~CSyComAutoPtr()
    {
        if (m_pRawPtr) {
            m_pRawPtr->ReleaseReference();
        }
    }

    CSyComAutoPtr &operator = (const CSyComAutoPtr &aAutoPtr)
    {
        return (*this = aAutoPtr.m_pRawPtr);
    }

    CSyComAutoPtr &operator = (T *aPtr)
    {
        if (m_pRawPtr == aPtr) {
            return *this;
        }

        if (aPtr) {
            aPtr->AddReference();
        }
        if (m_pRawPtr) {
            m_pRawPtr->ReleaseReference();
        }
        m_pRawPtr = aPtr;
        return *this;
    }

    operator void *() const
    {
        return m_pRawPtr;
    }

    T *operator -> () const
    {
        SY_ASSERTE(m_pRawPtr);
        return m_pRawPtr;
    }

    T *Get() const
    {
        return m_pRawPtr;
    }

    T *ParaIn() const
    {
        return m_pRawPtr;
    }

    // Ricky, 2012-12-13 10:10:25
    // We need a function like auto_ptr::release()
    T *Release()
    {
        T *pPtr = m_pRawPtr;
        m_pRawPtr = NULL;

        return pPtr;
    }

    T *&ParaOut()
    {
        if (m_pRawPtr) {
            m_pRawPtr->ReleaseReference();
            m_pRawPtr = NULL;
        }
        return static_cast<T *&>(m_pRawPtr);
    }

    T *&ParaInOut()
    {
        return static_cast<T *&>(m_pRawPtr);
    }

    T &operator * () const
    {
        SY_ASSERTE(m_pRawPtr);
        return *m_pRawPtr;
    }

private:
    T *m_pRawPtr;
};

END_UTIL_NS

#endif // !SYREFERENCECONTROL_H
