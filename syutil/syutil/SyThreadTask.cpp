#include "SyDebug.h"
#include "SyThreadTask.h"
#include "SyTimerQueueOrderedList.h"
#include "timer.h"
#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSyThreadTaskWithEventQueueOnly
//////////////////////////////////////////////////////////////////////

CSyThreadTaskWithEventQueueOnly::CSyThreadTaskWithEventQueueOnly()
{
}

CSyThreadTaskWithEventQueueOnly::~CSyThreadTaskWithEventQueueOnly()
{
}

void CSyThreadTaskWithEventQueueOnly::OnThreadInit()
{
    m_EventQueue.Reset2CurrentThreadInfo();

    CSyStopFlag::m_Est.Reset2CurrentThreadInfo();
    SetStartFlag();
}

void CSyThreadTaskWithEventQueueOnly::OnThreadRun()
{
    SY_INFO_TRACE_THIS("CSyThreadTaskWithEventQueueOnly::OnThreadRun");


    CSyEventQueueBase::EventsType listEvents;
    while (!IsFlagStopped()) {
        listEvents.clear();
        SyResult rv = m_EventQueue.PopOrWaitPendingEvents(listEvents, NULL, (DWORD)-1);

        if (SY_SUCCEEDED(rv)) {
            m_EventQueue.ProcessEvents(listEvents);
        }
    }

    m_EventQueue.DestoryPendingEvents();
}

ISyEventQueue *CSyThreadTaskWithEventQueueOnly::GetEventQueue()
{
    return &m_EventQueue;
}

SyResult CSyThreadTaskWithEventQueueOnly::Stop(CSyTimeValue *aTimeout)
{
    SY_INFO_TRACE_THIS("CSyThreadTaskWithEventQueueOnly::Stop");

    SyResult rv = CSyEventStopT<CSyThreadTaskWithEventQueueOnly>::PostStopEvent(this);

    // stop event queue after post stop event.
    m_EventQueue.Stop();
    return rv;
}


//////////////////////////////////////////////////////////////////////
// class CSyThreadTask
//////////////////////////////////////////////////////////////////////

CSyThreadTask::CSyThreadTask()
    : m_pTimerQueue(NULL)
{
}

CSyThreadTask::~CSyThreadTask()
{
    delete m_pTimerQueue;
}

void CSyThreadTask::OnThreadInit()
{
    // have to new timerqueue in the task thread.
    SY_ASSERTE(!m_pTimerQueue);
    m_pTimerQueue = CSyTimerQueueBase::CreateTimerQueue(NULL);
    SY_ASSERTE(m_pTimerQueue);

    CSyThreadTaskWithEventQueueOnly::OnThreadInit();
}

void CSyThreadTask::OnThreadRun()
{
    SY_INFO_TRACE_THIS("CSyThreadTask::OnThreadRun, Begin. name=" << m_name);

    CSyEventQueueBase::EventsType listEvents;
    while (!IsFlagStopped()) {
        // improve the performance.
#if 0
        // CheckExpire before Wait.
        m_pTimerQueue->CheckExpire();

        CSyTimeValue tvTimeout = CSyTimeValue::s_tvZero;
        CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
        CSyTimeValue tvEarliest = m_pTimerQueue->GetEarliestTime();
        if (tvCur < tvEarliest) {
            if (tvEarliest != CSyTimeValue::get_tvMax()) {
                tvTimeout = tvEarliest - tvCur;
            } else {
                tvTimeout = CSyTimeValue::get_tvMax();
            }
        }
#else
        CSyTimeValue tvTimeout(CSyTimeValue::get_tvMax());
        if (m_pTimerQueue) {
            // process timer prior to wait event.
            m_pTimerQueue->CheckExpire(&tvTimeout);
        }
#endif

        CSyTimeValue *pTvPara;
        if (tvTimeout == CSyTimeValue::get_tvMax()) {
            pTvPara = NULL;
        } else {
            pTvPara = &tvTimeout;
        }

        listEvents.clear();
        SyResult rv = m_EventQueue.PopOrWaitPendingEvents(listEvents, pTvPara);

        // CheckExpire after Wait.
        //      m_pTimerQueue->CheckExpire();

        if (SY_SUCCEEDED(rv)) {
            m_EventQueue.ProcessEvents(listEvents);
        }
    }

    m_EventQueue.DestoryPendingEvents();
    
    SY_INFO_TRACE_THIS("CSyThreadTask::OnThreadRun, End. name=" << m_name);
}

ISyTimerQueue *CSyThreadTask::GetTimerQueue()
{
    return m_pTimerQueue;
}

void CSyThreadTask::SleepMsWithLoop(DWORD aMsec)
{
    CSyEventQueueBase::EventsType listEvents;
    unsigned long endTime = get_tick_count() + aMsec;
    while (1) {
        unsigned long nowTime = get_tick_count();
        if (nowTime >= endTime) {
            break;
        }
        unsigned long waitTime = endTime - nowTime;

        CSyTimeValue tvTimeout;
        tvTimeout.SetByTotalMsec(waitTime);

        listEvents.clear();
        SyResult rv = m_EventQueue.PopOrWaitPendingEvents(listEvents, &tvTimeout);

        // CheckExpire after Wait.
        //      m_pTimerQueue->CheckExpire();

        if (SY_SUCCEEDED(rv)) {
            m_EventQueue.ProcessEvents(listEvents);
        }
    }
    m_EventQueue.DestoryPendingEvents();
}

//////////////////////////////////////////////////////////////////////
// class CSyEventQueueUsingConditionVariable
//////////////////////////////////////////////////////////////////////

CSyEventQueueUsingConditionVariable::CSyEventQueueUsingConditionVariable()
    : m_Condition(m_Mutex)
{
}

CSyEventQueueUsingConditionVariable::~CSyEventQueueUsingConditionVariable()
{
}

SyResult CSyEventQueueUsingConditionVariable::
PostEvent(ISyEvent *aEvent, EPriority aPri)
{
    // Don't hold the mutex when signaling the condition variable.
    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);
        SyResult rv = CSyEventQueueBase::PostEvent(aEvent, aPri);
        if (SY_FAILED(rv)) {
            return rv;
        }
    }

    // Don't care the error if Signal() failed.
    m_Condition.Signal();
    return SY_OK;
}

SyResult CSyEventQueueUsingConditionVariable::
PopOrWaitPendingEvents(CSyEventQueueBase::EventsType &aEvents,
                       CSyTimeValue *aTimeout, DWORD aMaxCount)
{
    SyResult rv;
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);

    if (m_Events.empty()) {
        rv = m_Condition.Wait(aTimeout);
        if (SY_FAILED(rv) && rv != SY_ERROR_TIMEOUT) {
            SY_ERROR_TRACE_THIS("CSyEventQueueUsingConditionVariable::PopOrWaitPendingEvents,"
                                "m_Events is not empty. nSize=" << m_dwSize << " rv=" << rv);
            //  return rv;
        }
    }

    return PopPendingEvents(aEvents, aMaxCount);
}

//////////////////
//
//////////////////
class CSyTimerQueueOnDemand : public CSyTimerQueueOrderedList
{
public:
    CSyTimerQueueOnDemand(ISyObserver *aObserver) : CSyTimerQueueOrderedList(aObserver) { }
    virtual ~CSyTimerQueueOnDemand() {}

    virtual SyResult ScheduleTimer(ISyTimerHandler *aEh,
                                   LPVOID aToken,
                                   const CSyTimeValue &aInterval,
                                   DWORD aCount)
    {
        SyResult ret = CSyTimerQueueOrderedList::ScheduleTimer(aEh, aToken, aInterval, aCount);
        if (SY_SUCCEEDED(ret) && m_pObserver) {
            m_pObserver->OnObserve("TimerQueueOnDemand");
            //SY_INFO_TRACE("CSyThreadHeartBeat, ScheduleTimer, size = " << m_Nodes.size());
        }
        return ret;
    }
};

//////////////////////////////////////////////////////////////////////
// class CSyThreadHeartBeat
//////////////////////////////////////////////////////////////////////

CSyThreadHeartBeat::CSyThreadHeartBeat()
    : m_pTimerQueue(NULL),
      m_scheduler(NULL),
      m_earliest(CSyTimeValue::get_tvMax())
{
}

CSyThreadHeartBeat::~CSyThreadHeartBeat()
{
    delete m_pTimerQueue;
}

void CSyThreadHeartBeat::OnThreadInit()
{
    SY_INFO_TRACE_THIS("CSyThreadHeartBeat::OnThreadInit");
    // have to new timerqueue in the task thread.
    SY_ASSERTE(!m_pTimerQueue);
    m_pTimerQueue = new CSyTimerQueueOnDemand(this);
    SY_ASSERTE(m_pTimerQueue);

    Reset2CurrentThreadInfo();
}

ISyEventQueue *CSyThreadHeartBeat::GetEventQueue()
{
    return this;
}

ISyTimerQueue *CSyThreadHeartBeat::GetTimerQueue()
{
    return m_pTimerQueue;
}

void CSyThreadHeartBeat::OnThreadRun()
{
    if (m_scheduler == NULL) {
        DoHeartBeat();
    } else {
        SY_ERROR_TRACE("CSyThreadHeartBeat::OnThreadRun, with scheduler you should never call OnThreadRun it will be driven by scheduler automatically.");
    }
}

SyResult CSyThreadHeartBeat::Stop(CSyTimeValue *aTimeout)
{
    m_scheduler = NULL;
    return SY_OK;
}

SyResult CSyThreadHeartBeat::PostEvent(ISyEvent *aEvent, EPriority aPri)
{
    SyResult rv = CSyEventQueueUsingMutex::PostEvent(aEvent, aPri);
    //SY_INFO_TRACE_THIS("CSyThreadHeartBeat::PostEvent, m_scheduler=" << (void*)m_scheduler << ", rv=" << rv << ", size=" << m_dwSize);

    //Todo, schedule a timer in target thread immediately
    if (m_scheduler) {
        m_scheduler(0);
    }

    return rv;
}

SyResult CSyThreadHeartBeat::DoHeartBeat()
{
    CSyTimeValue tvWait = CSyTimeValue::get_tvMax();
    CSyTimeValue tvEarliest = CSyTimeValue::get_tvMax();
    if (m_pTimerQueue) {
        m_pTimerQueue->CheckExpire(&tvWait, &tvEarliest);
    }

    CSyEventQueueBase::EventsType tmpListEvents_DoHeartBeat;
    SyResult rv = PopPendingEventsWithoutWait(tmpListEvents_DoHeartBeat, (DWORD)-1);
    //SY_INFO_TRACE_THIS("CSyThreadHeartBeat::DoHeartBeat Timer, m_scheduler=" << (void*)m_scheduler << ", tvWait=" << tvWait.GetTotalInMsec() << ", events list=" << listEvents.size());
    if (SY_SUCCEEDED(rv)) {
        rv = ProcessEvents(tmpListEvents_DoHeartBeat);
        if (rv == SY_ERROR_TERMINATING) {
            return rv;
        }
    }

    bool bScheduled = false;
    if (m_scheduler && tvEarliest != m_earliest && tvWait != CSyTimeValue::get_tvMax()) {
        //Schedule a timer in target thread in future time
        long ms=tvWait.GetTotalInMsec()+5;
        if(ms <= 10)
            ms = 10;
        if (ms > 1000) {
            SY_EVERY_N_INFO_TRACE_THIS(100, "CSyThreadHeartBeat::DoHeartBeat schedule in ms =" << ms);
            ms = 1000;
        }
        m_scheduler(ms);
        bScheduled = true;
    }
    if (m_scheduler && !bScheduled) {
        //SY_WARNING_TRACE_THIS("CSyThreadHeartBeat::DoHeartBeat, not scheduled for some reason (bug?), let it kick at every 200ms.");
        m_scheduler(200);
    }
    m_earliest = tvEarliest;

    return SY_OK;
}

void CSyThreadHeartBeat::OnObserve(LPCSTR aTopic, LPVOID aData)
{
    //SY_INFO_TRACE_THIS("CSyThreadHeartBeat::OnObserve, m_scheduler=" << (void*)m_scheduler);
    if (m_scheduler) {
        m_scheduler(10);
    }
}

extern "C" SyResult do_CreateUserTaskThread(const char *name, ASyThread *&aThread, TFlag aFlag, BOOL bWithTimerQueue, TType aType)
{
    SY_ASSERTE(!aThread);

    ASyThread *pThread = NULL;
    if (bWithTimerQueue) {
        pThread = new CSyThreadTask();
    } else {
        pThread = new CSyThreadTaskWithEventQueueOnly();
    }
    SY_ASSERTE_RETURN(pThread, SY_ERROR_OUT_OF_MEMORY);

    SyResult rv = pThread->Create(name, aType, aFlag, TRUE);
    if (SY_FAILED(rv)) {
        pThread->Destory(rv);
        return rv;
    }

    /*
        rv = RegisterThread(pThread);
                  if (SY_FAILED(rv)) {
                        SY_ERROR_TRACE_THIS("::CreateUserTaskThread, RegisterThread failed, ret = " << rv);
                            return rv;
                  }
        */

    aThread = pThread;
    return SY_OK;
}


pfn_CreateUserTaskThread p_CreateUserTaskThread = do_CreateUserTaskThread;

extern "C" pfn_CreateUserTaskThread setCreateUserTaskThread(pfn_CreateUserTaskThread fn)
{
	pfn_CreateUserTaskThread oldFnc = p_CreateUserTaskThread;
    p_CreateUserTaskThread = fn;
	return oldFnc;
}

extern "C" SyResult CreateUserTaskThread(const char *name, ASyThread *&aThread, TFlag aFlag, BOOL bWithTimerQueue, TType aType)
{
    return p_CreateUserTaskThread(name, aThread, aFlag, bWithTimerQueue, aType);
}

ASyThreadSingletonFactory ASyThreadSingletonFactory::m_pASyThreadSingletonFactory;
ASyThreadSingletonFactory &ASyThreadSingletonFactory::Instance()
{
    return m_pASyThreadSingletonFactory;
}
SyResult ASyThreadSingletonFactory::GetSingletonThread(const char *pThreadName,ASyThread *&aThread)
{
    SY_ASSERTE(!aThread);
    if (pThreadName == NULL) {
        return SY_ERROR_INVALID_ARG;
    }

    CSyMutexGuardT<CSyMutexThread> lock(m_LockForMap);

    if (m_mapThreads.find(pThreadName)!=m_mapThreads.end()) {
        ASyThreadSingleton &aASyThreadSingleton = m_mapThreads[pThreadName];
        aThread = aASyThreadSingleton.m_pASyThread;
        aASyThreadSingleton.m_nRefNum++;
        return SY_OK;
    }
    CreateUserTaskThread(pThreadName,aThread);
    if (aThread!=NULL) {
        ASyThreadSingleton aSyThreadSingleton;
        aSyThreadSingleton.m_nRefNum = 1;
        aSyThreadSingleton.m_pASyThread = aThread;
        m_mapThreads[pThreadName] = aSyThreadSingleton;
    }
    return SY_OK;
}
void ASyThreadSingletonFactory::ResleseSingletonThread(const char *pThreadName,ASyThread *aThread)
{
    if (pThreadName==NULL || aThread ==NULL) {
        return ;
    }

    CSyMutexGuardT<CSyMutexThread> lock(m_LockForMap);
    std::map<std::string,ASyThreadSingleton>::iterator it;

    if ((it=m_mapThreads.find(pThreadName))!=m_mapThreads.end()) {
        if (aThread == m_mapThreads[pThreadName].m_pASyThread) {
            ASyThreadSingleton &aASyThreadSingleton = m_mapThreads[pThreadName];
            if (aASyThreadSingleton.m_pASyThread == aThread) {
                aASyThreadSingleton.m_nRefNum--;
                if (aASyThreadSingleton.m_nRefNum==0 && aASyThreadSingleton.m_pASyThread) {
                    aASyThreadSingleton.m_pASyThread->Stop();
                    aASyThreadSingleton.m_pASyThread->Join();
                    aASyThreadSingleton.m_pASyThread->Destory(SY_OK);
                    m_mapThreads.erase(it);
                }
            }
        }
    }
}
ASyThreadSingletonFactory::ASyThreadSingletonFactory()
{

}
ASyThreadSingletonFactory::~ASyThreadSingletonFactory()
{
    SY_ASSERTE(m_mapThreads.size()==0);
    m_mapThreads.clear();
}
