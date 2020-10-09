
//#include "SyBase.h"
#include "SyEventQueueBase.h"
#include "SyAssert.h"

#include "SyDebug.h"
#include "SyUtilMisc.h"

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSyEventSynchronous
//////////////////////////////////////////////////////////////////////

CSyEventSynchronous::CSyEventSynchronous(ISyEvent *aEventPost,
        CSyEventQueueBase *aEventQueue)
    : m_pEventPost(aEventPost)
    , m_Result(SY_ERROR_NOT_AVAILABLE)
    , m_pEventQueue(aEventQueue)
    , m_bHasDestoryed(FALSE)
{
    SY_ASSERTE(m_pEventPost);
    SY_ASSERTE(m_pEventQueue);
}

CSyEventSynchronous::~CSyEventSynchronous()
{
    if (m_pEventPost) {
        m_pEventPost->OnDestorySelf();
    }
    if (!m_bHasDestoryed) {
        m_SendEvent.Signal();
    }
}

SyResult CSyEventSynchronous::OnEventFire()
{
    SyResult rv = SY_OK;
    if (m_bHasDestoryed) {
        return rv;
    }

    if (m_pEventPost) {
        rv = m_pEventPost->OnEventFire();
    } else {
        rv = SY_ERROR_NULL_POINTER;
    }

    m_Result = rv;
    m_SendEvent.Signal();
    return rv;
}

void CSyEventSynchronous::OnDestorySelf()
{
    if (m_bHasDestoryed) {
        // delete this in the cecond time of OnDestorySelf.
        delete this;
    } else {
        // Don't assign <m_bHasDestoryed> in the function WaitResultAndDeleteThis().
        // Do operations on <m_bHasDestoryed> in the same thread.
        m_bHasDestoryed = TRUE;
        if (m_pEventPost) {
            m_pEventPost->OnDestorySelf();
            m_pEventPost = NULL;
        }
    }
}

SyResult CSyEventSynchronous::WaitResultAndDeleteThis()
{
    SyResult rv = m_SendEvent.Wait();
    if (SY_FAILED(rv)) {
        SY_WARNING_TRACE_THIS("CSyEventSynchronous::WaitResultAndDeleteThis,"
                              " m_SendEvent.Wait() failed!");
        return rv;
    }

    rv = m_Result;
    if (m_pEventQueue) {
        m_pEventQueue->PostEvent(this);
    }
    return rv;
}


//////////////////////////////////////////////////////////////////////
// class CSyEventQueueBase
//////////////////////////////////////////////////////////////////////
CSyTimeValue CSyEventQueueBase::s_tvReportInterval(0, 500*1000);

CSyEventQueueBase::CSyEventQueueBase()
    : m_dwSize(0)
    , m_bIsStopped(FALSE)
    , m_pTid(0)
{
    m_bIsRunning = false;
    m_pEventHook = NULL;
    m_tvReportSize = CSyTimeValue::GetTimeOfDay();
    memset(m_nameTid,0,sizeof(m_nameTid));
}

CSyEventQueueBase::~CSyEventQueueBase()
{
    DestoryPendingEvents();
}

void CSyEventQueueBase::DestoryPendingEvents()
{
    EventsType::iterator iter = m_Events.begin();
    for (; iter != m_Events.end(); ++iter) {
        (*iter)->OnDestorySelf();
    }
    m_Events.clear();
    m_dwSize = 0;
}

SyResult CSyEventQueueBase::PostEvent(ISyEvent *aEvent, EPriority aPri)
{
    SY_ASSERTE_RETURN(aEvent, SY_ERROR_INVALID_ARG);

    if (m_bIsStopped) {
        SY_WARNING_TRACE_THIS("CSyEventQueueBase::PostEvent, has been stopped.");
        aEvent->OnDestorySelf();
        return SY_ERROR_NOT_INITIALIZED;
    }

    m_Events.push_back(aEvent);
    m_dwSize++;

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
    if (tvCur - m_tvReportSize > CSyTimeValue(3, 0)) {
        if (m_dwSize > 100)
            SY_WARNING_TRACE_THIS("CSyEventQueueBase::PostEvent,"
                                  " m_dwSize=" << m_dwSize <<
                                  " m_Tid=" << m_pTid <<
                                  " name=" << m_nameTid);
        m_tvReportSize = tvCur;
    }
#endif // !SY_DISABLE_EVENT_REPORT

    return SY_OK;
}

SyResult CSyEventQueueBase::SendEvent(ISyEvent *aEvent)
{
    SY_ASSERTE_RETURN(aEvent, SY_ERROR_INVALID_ARG);

    if (m_bIsStopped) {
        SY_WARNING_TRACE_THIS("CSyEventQueueBase::SendEvent, has been stopped.");
        aEvent->OnDestorySelf();
        return SY_ERROR_NOT_INITIALIZED;
    }

    // if send event to the current thread, just do callbacks.
    if (IsEqualCurrentThread(m_pTid)) {
        SyResult rv = aEvent->OnEventFire();
        aEvent->OnDestorySelf();
        return rv;
    }

    CSyEventSynchronous *pSend = new CSyEventSynchronous(aEvent, this);
    SyResult rv = PostEvent(pSend);
    if (SY_FAILED(rv)) {
        return rv;
    }

    rv = pSend->WaitResultAndDeleteThis();
    return rv;
}

DWORD CSyEventQueueBase::GetPendingEventsCount()
{
    return m_dwSize;
}

//#include "SyReactorBase.h"
SyResult CSyEventQueueBase::
PopPendingEvents(EventsType &aEvents, DWORD aMaxCount, DWORD *aRemainSize)
{
    SY_ASSERTE(aEvents.empty());
    SY_ASSERTE(aMaxCount > 0);

    DWORD dwTotal = m_dwSize;
    if (dwTotal == 0) {
        return SY_ERROR_NOT_FOUND;
    }

    if (dwTotal <= aMaxCount) {
        //aEvents.swap(m_Events);
        aEvents.assign(m_Events.begin(), m_Events.end());
        m_Events.clear();
        m_dwSize = 0;
        SY_ASSERTE(m_Events.empty());
    } else {
        for (DWORD i = 0; i < aMaxCount; i++) {
            aEvents.push_back(m_Events.front());
            m_Events.pop_front();
            m_dwSize--;
        }
        SY_ASSERTE(!m_Events.empty());
    }

    if (aRemainSize) {
        *aRemainSize = m_dwSize;
    }
    return SY_OK;
}

SyResult CSyEventQueueBase::ProcessEvents(const EventsType &aEvents)
{
    SyResult ret = SY_OK;
    m_bIsRunning = true;
    EventsType::const_iterator iter = aEvents.begin();
    for (; iter != aEvents.end(); ++iter) {
        ProcessOneEvent(*iter);
    }
    if (m_pEventHook) {
        m_pEventHook();
        return SY_ERROR_TERMINATING;
    }
    m_bIsRunning = false;

    return ret;
}

SyResult CSyEventQueueBase::ProcessOneEvent(ISyEvent *aEvent)
{
    SY_ASSERTE_RETURN(aEvent, SY_ERROR_INVALID_ARG);

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
#endif // !SY_DISABLE_EVENT_REPORT

    aEvent->OnEventFire();
    aEvent->OnDestorySelf();

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvSub = CSyTimeValue::GetTimeOfDay() - tvCur;
    if (tvSub > s_tvReportInterval) {
        SY_WARNING_TRACE_THIS("CSyEventQueueBase::ProcessOneEvent, report,"
                              " sec=" << tvSub.GetSec() <<
                              " usec=" << tvSub.GetUsec() <<
                              " aEvent=" << aEvent <<
                              " m_dwSize=" << m_dwSize <<
                              " tid=" << m_pTid <<
                              " name=" << m_nameTid);
    }
#endif // !SY_DISABLE_EVENT_REPORT

    return SY_OK;
}


//////////////////////////////////////////////////////////////////////
// class CSyEventQueueUsingMutex
//////////////////////////////////////////////////////////////////////

CSyEventQueueUsingMutex::~CSyEventQueueUsingMutex()
{
}

SyResult CSyEventQueueUsingMutex::PostEvent(ISyEvent *aEvent, EPriority aPri)
{
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);
    return CSyEventQueueBase::PostEvent(aEvent, aPri);
}

//#include "SyReactorBase.h"
SyResult CSyEventQueueUsingMutex::
PostEventWithOldSize(ISyEvent *aEvent, EPriority aPri, DWORD *aOldSize)
{
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);
    if (aOldSize) {
        *aOldSize = m_dwSize;
    }
    return CSyEventQueueBase::PostEvent(aEvent, aPri);
}
