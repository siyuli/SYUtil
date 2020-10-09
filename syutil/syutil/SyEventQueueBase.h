#ifndef SYEVENTQUEUEBASE_H
#define SYEVENTQUEUEBASE_H

#include "SyThreadInterface.h"
#include "SyConditionVariable.h"
#include "SyTimeValue.h"
#include <list>

#include "SyThreadMisc.h"

START_UTIL_NS

class CSyEventQueueBase;

class CSyEventSynchronous : public ISyEvent
{
public:
    CSyEventSynchronous(ISyEvent *aEventPost, CSyEventQueueBase *aEventQueue);
    virtual ~CSyEventSynchronous();

    // interface ISyEvent
    virtual SyResult OnEventFire();
    virtual void OnDestorySelf();

    SyResult WaitResultAndDeleteThis();

private:
    ISyEvent *m_pEventPost;
    SyResult m_Result;
    CSyEventQueueBase *m_pEventQueue;
    BOOL m_bHasDestoryed;
    CSyEventThread m_SendEvent;
};

class SY_OS_EXPORT CSyEventQueueBase : public ISyEventQueue
{
public:
    CSyEventQueueBase();
    virtual ~CSyEventQueueBase();

    // interface ISyEventQueue
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL);
    virtual SyResult SendEvent(ISyEvent *aEvent);
    virtual DWORD GetPendingEventsCount();

    void Stop();

#ifdef  MMP_UNIT_TEST_ENABLED
    //Add to support unit test
    void Start() { this->m_bIsStopped = FALSE; };
    //add by shine
#endif  //MMP_UNIT_TEST_ENABLED

    void DestoryPendingEvents();

    void Reset2CurrentThreadInfo();

    enum { MAX_GET_ONCE = 5 };
    typedef std::list<ISyEvent * > EventsType;

    // Don't make the following two functions static because we want trace size.
    SyResult ProcessEvents(const EventsType &aEvents);
    SyResult ProcessOneEvent(ISyEvent *aEvent);

    static CSyTimeValue s_tvReportInterval;

    typedef void(*f_OnEventHook)();
    bool IsRunning() { return m_bIsRunning; }
    void SetEventHook(f_OnEventHook pHook) { m_pEventHook = pHook; }
protected:
    SyResult PopPendingEvents(
        EventsType &aEvents,
        DWORD aMaxCount = MAX_GET_ONCE,
        DWORD *aRemainSize = NULL);

    EventsType m_Events;
    // we have to record the size of events list due to limition of std::list in Linux.
    DWORD m_dwSize;
    CSyTimeValue m_tvReportSize;
    char  m_nameTid[512];
    SY_THREAD_ID m_pTid;
private:
    BOOL m_bIsStopped;
    bool m_bIsRunning;
    f_OnEventHook m_pEventHook;

    friend class CSyEventSynchronous;
    friend class CSySleepMsWithLoop;
};

class SY_OS_EXPORT CSyEventQueueUsingMutex : public CSyEventQueueBase
{
public:
    CSyEventQueueUsingMutex();
    virtual ~CSyEventQueueUsingMutex();

    // interface ISyEventQueue
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL);

    // Pop <aMaxCount> pending events in the queue,
    // if no events are pending, return at once.
    SyResult PopPendingEventsWithoutWait(
        CSyEventQueueBase::EventsType &aEvents,
        DWORD aMaxCount = MAX_GET_ONCE,
        DWORD *aRemainSize = NULL);

    // Pop one pending events, and fill <aRemainSize> with remain size.
    // if no events are pending, return at once.
    SyResult PopOnePendingEventWithoutWait(
        ISyEvent *&aEvent,
        DWORD *aRemainSize = NULL);

    SyResult PostEventWithOldSize(
        ISyEvent *aEvent,
        EPriority aPri = EPRIORITY_NORMAL,
        DWORD *aOldSize = NULL);

private:
#ifdef SY_ENABLE_SPINLOCK
    typedef CSySpinLockSP MutexType;
#else
    typedef CSyMutexThread MutexType;
#endif
    MutexType m_Mutex;
};


// inline functions
inline void CSyEventQueueBase::Reset2CurrentThreadInfo()
{
    //m_Tid = CSyThreadManager::GetThreadSelfId();
    m_pTid = GetThreadSelfId();
#if defined(SY_IOS) || defined(SY_MACOS)
    pthread_getname_np(pthread_self(), m_nameTid,sizeof(m_nameTid)-1);
#endif
}

inline void CSyEventQueueBase::Stop()
{
    m_bIsStopped = TRUE;
}


inline CSyEventQueueUsingMutex::CSyEventQueueUsingMutex()
{
}

inline SyResult CSyEventQueueUsingMutex::
PopPendingEventsWithoutWait(CSyEventQueueBase::EventsType &aEvents,
                            DWORD aMaxCount, DWORD *aRemainSize)
{
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);
    return PopPendingEvents(aEvents, aMaxCount, aRemainSize);
}


END_UTIL_NS

#endif // !SYEVENTQUEUEBASE_H
