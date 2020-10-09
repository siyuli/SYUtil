
// #include "SyBase.h"
#include "SyTimerWrapperID.h"
#include "SyThread.h"
#include "SyError.h"
#include "SyAssert.h"
#include "SyTimeValue.h"
#include "SyDebug.h"
#include "SyThreadMisc.h"
#include "SyUtilMisc.h"
#include <atomic>
#include <functional>

USE_UTIL_NS

class CSyTimerWrapperRef : public CSyReferenceControlMutilThread, public ISyTimerHandler
{
public:
    CSyTimerWrapperRef(CSyTimerWrapperID* pParent, SY_THREAD_ID threadId) {
        m_pParent = pParent;
        m_pTimerQueue = NULL;
        m_threadId = threadId;
        m_bInTimer = false;
        m_bTimerCancelling = false;
    }
    
    ~CSyTimerWrapperRef() {
        bool bSameThread = Cancel();
        if (bSameThread) {
            CancelTimer();
        }
        m_threadId = 0;
    }
    
    bool Cancel() {
        bool bSameThread = false;
        m_bTimerCancelling = true;
        
        ASyThread *pThread = getCurrentThread();
        if (pThread && pThread->GetTimerQueue()) {
            bSameThread = IsSameThread(pThread->GetTimerQueue());
        }
        
        if (!bSameThread) {
            int i = 0;
            for (i = 0; i < 25; i++) {
                if (!m_bInTimer) {
                    break;
                }
                if (i < 5) {
                    SleepMs(30);
                } else {
                    SleepMs(100);
                }
            }
            if (i >= 25 || m_bInTimer) {
                SY_ERROR_TRACE_THIS("CSyTimerWrapperRef::Cancel, the timer has been executed too long time, parent=" << m_pParent << ", thread=" << m_threadId);
            }
        } else {
            m_pParent = NULL;
        }
        
        return bSameThread;
    }
    
    SyResult ScheduleTimer(CSyTimerWrapperIDSink *aSink, const CSyTimeValue& aInterval, DWORD aCount) {
        SY_ASSERTE_RETURN(m_pTimerQueue != NULL, SY_ERROR_NOT_INITIALIZED);
        m_bTimerCancelling = false;
        return m_pTimerQueue->ScheduleTimer(this, aSink, aInterval, aCount);
    }
    
    SyResult CancelTimer() {
        return m_pTimerQueue->CancelTimer(this);
    }
    
    SY_THREAD_ID GetThreadId() {
        return m_threadId;
    }
    
    bool IsSameThread(ISyTimerQueue *pTimerQueue) {
        return (pTimerQueue == m_pTimerQueue);
    }
    
    void SetTimerQueue(ISyTimerQueue *pTimerQueue) {
        m_pTimerQueue = pTimerQueue;
    }
    
    bool IsInitialized() {
        return m_pTimerQueue != NULL;
    }
    
protected:
    CSyTimerWrapperRef(const CSyTimerWrapperRef&);
    const CSyTimerWrapperRef& operator=(const CSyTimerWrapperRef&);
    
    virtual void OnTimeout(const CSyTimeValue &aCurTime, LPVOID aArg) {
        m_bInTimer = true;
        if (m_bTimerCancelling) {
            m_bInTimer = false;
            return;
        }
        
        AddReference();
        
        if(m_pParent) {
            m_pParent->OnTimeout(aCurTime, aArg);
        }
        
        m_bInTimer = false;
        ReleaseReference();
    }
    
    CSyTimerWrapperID *m_pParent;
    ISyTimerQueue *m_pTimerQueue;
    SY_THREAD_ID m_threadId;
    
    std::atomic<bool> m_bInTimer;
    std::atomic<bool> m_bTimerCancelling;
};

//////////////////////////////////////////////////////////////////////
// class CSyTimerWrapperIDSink
//////////////////////////////////////////////////////////////////////
CSyTimerWrapperIDSink::CSyTimerWrapperIDSink()
{

}
CSyTimerWrapperIDSink::~CSyTimerWrapperIDSink()
{
}

//////////////////////////////////////////////////////////////////////
// class CSyTimerWrapperID
//////////////////////////////////////////////////////////////////////

CSyTimerWrapperID::CSyTimerWrapperID()
{
    // Don't get timer queue in the contruct function.
    // get timer queue in the Schedule() function.
}

CSyTimerWrapperID::~CSyTimerWrapperID()
{
    Cancel();
}

SyResult CSyTimerWrapperID::
Schedule(CSyTimerWrapperIDSink *aSink, const CSyTimeValue &aInterval, DWORD aCount)
{
    SY_ASSERTE_RETURN(aSink, SY_ERROR_INVALID_ARG);

    ASyThread *pThread = getCurrentThread();
    if (!pThread) {
        SY_ERROR_TRACE_THIS("CSyTimerWrapperID::Schedule, your thread doesn't support TP timer queue.");
        return SY_ERROR_NULL_POINTER;
    }
    
    ISyTimerQueue *pTimerQueue = pThread->GetTimerQueue();
    if (pTimerQueue == nullptr) {
        SY_WARNING_TRACE_THIS("CSyTimerWrapperID::Schedule, your thread doesn't support TP timer queue 2.");
        return SY_ERROR_UNEXPECTED;
    }
    CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_mutexCancel);
    if(!m_TimerWrapRef) {
        m_TimerWrapRef = new CSyTimerWrapperRef(this, pThread->GetThreadId());
    }
    if(m_TimerWrapRef->IsInitialized() && !m_TimerWrapRef->IsSameThread(pTimerQueue)) {
        SY_ERROR_TRACE_THIS("CSyTimerWrapperID::Schedule, schedule in different thread is not allowed.");
        return SY_ERROR_FAILURE;
    }
    m_TimerWrapRef->SetTimerQueue(pTimerQueue);
    return m_TimerWrapRef->ScheduleTimer(aSink, aInterval, aCount);
}

SyResult CSyTimerWrapperID::
ScheduleInThread(TType aType, CSyTimerWrapperIDSink *aSink, const CSyTimeValue &aInterval, DWORD aCount)
{
    SY_ASSERTE_RETURN(aSink, SY_ERROR_INVALID_ARG);
    ASyThread *pThread = GetThread(aType);
    SY_ASSERTE_RETURN(pThread != NULL, SY_ERROR_INVALID_ARG);

    SyResult rv = ScheduleInThread(pThread,aSink,aInterval,aCount);
    return rv;
}

SyResult CSyTimerWrapperID::
ScheduleInThread(ASyThread *pThread, CSyTimerWrapperIDSink *aSink, const CSyTimeValue &aInterval, DWORD aCount)
{
    SY_ASSERTE_RETURN(pThread != NULL, SY_ERROR_INVALID_ARG);
    SY_ASSERTE_RETURN(aSink, SY_ERROR_INVALID_ARG);

    SyResult rv = SY_ERROR_NULL_POINTER;
    ASyThread *pCurrent = getCurrentThread();
    if (pCurrent != NULL && pCurrent == pThread) {
        rv = Schedule(aSink, aInterval, aCount);
    } else {
        ISyTimerQueue *pTimerQueue = pThread->GetTimerQueue();
        SY_ASSERTE_RETURN(pTimerQueue != NULL, SY_ERROR_UNEXPECTED);
        CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_mutexCancel);
        if (!m_TimerWrapRef) {
            m_TimerWrapRef = new CSyTimerWrapperRef(this, pThread->GetThreadId());
        }
        if (m_TimerWrapRef->IsInitialized() && !m_TimerWrapRef->IsSameThread(pTimerQueue)) {
            SY_ERROR_TRACE_THIS("CSyTimerWrapperID::ScheduleInThread, schedule 2 timers in different thread is not supported.");
            return SY_ERROR_NULL_POINTER;
        }
        
        CSyComAutoPtr<CSyTimerWrapperRef> aTimerWrapRef = m_TimerWrapRef;
        auto scheduleEventInline = [aTimerWrapRef, aSink, aInterval, aCount] () -> SyResult
        {
            if (!aTimerWrapRef->IsInitialized()) {
                ASyThread *pCurrent = getCurrentThread();
                aTimerWrapRef->SetTimerQueue(pCurrent->GetTimerQueue());
            }
            return aTimerWrapRef->ScheduleTimer(aSink, aInterval, aCount);
        };
        
        ISyEvent *pEvent = new CSyInvokeEvent<decltype(scheduleEventInline)>(scheduleEventInline);
        rv = pThread->GetEventQueue()->PostEvent(pEvent);
        //SY_INFO_TRACE_THIS("CSyTimerWrapperID::ScheduleInThread, POST EVENT to " << m_eventQueue << ", thread=" << pThread);
    }
    return rv;
}

SyResult CSyTimerWrapperID::Cancel(bool bOnlySameThread)
{
    CSyComAutoPtr<CSyTimerWrapperRef> aTimerWrapRef;
    {
        CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_mutexCancel);
        if (!m_TimerWrapRef) {
            return SY_ERROR_NOT_FOUND;
        }
        aTimerWrapRef = m_TimerWrapRef;
        m_TimerWrapRef = NULL;
    }
    
    SyResult rv = SY_ERROR_NULL_POINTER;
    bool bSameThread = aTimerWrapRef->Cancel();
    if (!bSameThread) {
        ASyThread *pThreadTimer = GetThreadById(aTimerWrapRef->GetThreadId());
        if (pThreadTimer && pThreadTimer->GetEventQueue()) {
            
            auto cancelEventInline = [aTimerWrapRef] () -> SyResult
            {
                return aTimerWrapRef->CancelTimer();
            };
            
            ISyEvent *pEvent = new CSyInvokeEvent<decltype(cancelEventInline)>(cancelEventInline);
            pThreadTimer->GetEventQueue()->PostEvent(pEvent);
        }
    } else {
        rv = aTimerWrapRef->CancelTimer();
    }
    
    return rv;
}

void CSyTimerWrapperID::OnTimeout(const CSyTimeValue &, LPVOID aArg)
{
    CSyTimerWrapperIDSink *pSink = static_cast<CSyTimerWrapperIDSink *>(aArg);
    SY_ASSERTE(pSink);
    if (m_TimerWrapRef && pSink) {
        pSink->OnTimer(this);
    }
}
