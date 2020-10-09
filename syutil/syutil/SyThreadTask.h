#ifndef SYTHREADTASK_H
#define SYTHREADTASK_H

#include "SyThread.h"
#include "SyEventQueueBase.h"
#include "SyUtilClasses.h"
#include "SyObserver.h"

#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

START_UTIL_NS

class CSyTimerQueueBase;
class CSyTimeValue;

template <class QueueType>
class CSyEventStopT : public ISyEvent
{
public:
    CSyEventStopT(QueueType *aQueue)
        : m_pQueue(aQueue)
    {
        SY_ASSERTE(m_pQueue);
    }

    virtual ~CSyEventStopT()
    {
    }

    virtual SyResult OnEventFire()
    {
        if (m_pQueue) {
            m_pQueue->SetStopFlag();
        }
        return SY_OK;
    }

    static SyResult PostStopEvent(QueueType *aQueue)
    {
        CSyEventStopT<QueueType> *pEvent = new CSyEventStopT<QueueType>(aQueue);
        if (!pEvent) {
            return SY_ERROR_OUT_OF_MEMORY;
        }
        return aQueue->GetEventQueue()->PostEvent(pEvent);
    }

private:
    QueueType *m_pQueue;
};

class SY_OS_EXPORT CSyEventQueueUsingConditionVariable
    : public CSyEventQueueBase
{
public:
    CSyEventQueueUsingConditionVariable();
    virtual ~CSyEventQueueUsingConditionVariable();

    // interface ISyEventQueue
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL);

    // Pop <aMaxCount> pending events in the queue,
    // if no events are pending, wait <aTimeout>.
    SyResult PopOrWaitPendingEvents(
        CSyEventQueueBase::EventsType &aEvents,
        CSyTimeValue *aTimeout = NULL,
        DWORD aMaxCount = MAX_GET_ONCE);

private:
    typedef CSyMutexThread MutexType;
    MutexType m_Mutex;
    CSyConditionVariableThread m_Condition;
};

class SY_OS_EXPORT CSyThreadTaskWithEventQueueOnly
    : public ASyThread
    , public CSyStopFlag
{
public:
    CSyThreadTaskWithEventQueueOnly();
    virtual ~CSyThreadTaskWithEventQueueOnly();

    //add to support unit test
#ifdef  MMP_UNIT_TEST_ENABLED
    void Start()
    {
        m_EventQueue.Start();
        SetStartFlag();
    };
    //by shine
#endif  //MMP_UNIT_TEST_ENABLED

    // interface ASyThread
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);
    virtual void OnThreadInit();
    virtual void OnThreadRun();
    virtual ISyEventQueue *GetEventQueue();

protected:
    CSyEventQueueUsingConditionVariable m_EventQueue;

    friend class CSyEventStopT<CSyThreadTaskWithEventQueueOnly>;
};

class SY_OS_EXPORT CSyThreadTask : public CSyThreadTaskWithEventQueueOnly
{
    CSyThreadTask(const CSyThreadTask &);
    CSyThreadTask &operator=(const CSyThreadTask &);
public:
    CSyThreadTask();
    virtual ~CSyThreadTask();

    // interface ASyThread
    virtual void OnThreadInit();
    virtual void OnThreadRun();
    virtual ISyTimerQueue *GetTimerQueue();

    void SleepMsWithLoop(DWORD aMsec);

protected:
    CSyTimerQueueBase *m_pTimerQueue;
};


class SY_OS_EXPORT CSyThreadHeartBeat : public ASyThread,
    public CSyEventQueueUsingMutex,
    public ISyObserver
{
    CSyThreadHeartBeat(const CSyThreadHeartBeat &);
    CSyThreadHeartBeat &operator=(const CSyThreadHeartBeat &);
public:
    typedef void(*TimerScheduler)(long tms); // time in milliseconds

    CSyThreadHeartBeat();
    virtual ~CSyThreadHeartBeat();

    // interface ASyThread
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);
    virtual void OnThreadInit();
    virtual void OnThreadRun();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

    // interface ISyEventQueue
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL);
    virtual void OnObserve(LPCSTR aTopic, LPVOID aData);

    SyResult DoHeartBeat();
    //Fix a potential deadloop issue found in android.
    void SetThreadId(SY_THREAD_ID tid) {
        m_Tid = tid;
        m_pTid = tid;
    }
    void SetScheduler(TimerScheduler scheduler) { m_scheduler = scheduler; }

protected:
    CSyTimeValue m_earliest;
    CSyTimerQueueBase *m_pTimerQueue;
    TimerScheduler m_scheduler;
};

END_UTIL_NS

#endif // !SYTHREADTASK_H
