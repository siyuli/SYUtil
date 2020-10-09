
#ifdef SUPPORT_REACTOR

#include "SyReactorBase.h"
#include "SyTimerQueueOrderedList.h"

#ifndef SY_WIN32
    #ifndef SY_MACOS
        #include <sys/resource.h>
    #endif
#endif // !SY_WIN32

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSyEventHandlerRepository
//////////////////////////////////////////////////////////////////////
CSyEventHandlerRepository::CSyEventHandlerRepository()
#ifndef SY_WIN32
    : m_pHandlers(NULL)
    , m_nMaxHandler(0)
    , m_cmHandles(NULL)
    , m_nSyHandlesCount(0)
    , m_cmHandlesChanged(false)
#endif // !SY_WIN32
{
    FD_ZERO(&m_aFsRead);
    FD_ZERO(&m_aFsWrite);
    FD_ZERO(&m_aFsException);
    m_nMaxFd=-1;
}

CSyEventHandlerRepository::~CSyEventHandlerRepository()
{
    Close();
}

SyResult CSyEventHandlerRepository::Open()
{
#ifdef SY_WIN32
    SY_ASSERTE_RETURN(m_Handlers.empty(), SY_ERROR_ALREADY_INITIALIZED);

#else
    SY_ASSERTE_RETURN(!m_pHandlers, SY_ERROR_ALREADY_INITIALIZED);

#ifdef SY_LINUX_SERVER
    SyResult rv = SetRlimit(RLIMIT_NOFILE, 8192, m_nMaxHandler);
#else
    SyResult rv = SetRlimit(RLIMIT_NOFILE, 512, m_nMaxHandler);
#endif // 
    if (SY_FAILED(rv)) {
        return rv;
    }

    m_pHandlers = new CElement[m_nMaxHandler];
    if (!m_pHandlers) {
        return SY_ERROR_OUT_OF_MEMORY;
    }
    m_cmHandles = new SY_HANDLE[m_nMaxHandler];
    if (!m_cmHandles) {
        return SY_ERROR_OUT_OF_MEMORY;
    }
#endif // SY_WIN32

    return SY_OK;
}

SyResult CSyEventHandlerRepository::Close()
{
#ifdef SY_WIN32
    m_Handlers.clear();
#else
    if (m_pHandlers) {
        delete []m_pHandlers;
        m_pHandlers = NULL;
    }
    m_nMaxHandler = 0;
    if (m_cmHandles) {
        delete []m_cmHandles;
        m_cmHandles = NULL;
    }
    m_nSyHandlesCount=0;
#endif // SY_WIN32
    return SY_OK;
}

#ifndef SY_WIN32

#if defined (SY_SOLARIS) || defined (SY_MACOS) || defined (SY_ANDROID)
    typedef int __rlimit_resource_t;
#endif // SY_SOLARIS || SY_MACOS

SyResult CSyEventHandlerRepository::SetRlimit(int aResource, int aMaxNum, int &aActualNum)
{
    rlimit rlCur;
    ::memset(&rlCur, 0, sizeof(rlCur));
    int nRet = ::getrlimit((__rlimit_resource_t)aResource, &rlCur);
    if (nRet == -1 || rlCur.rlim_cur == RLIM_INFINITY) {
        SY_ERROR_TRACE("CSyEventHandlerRepository::SetRlimit, getrlimit() failed! err=" << errno);
        return SY_ERROR_UNEXPECTED;
    }

    aActualNum = aMaxNum;
    if (aActualNum > static_cast<int>(rlCur.rlim_cur)) {
        rlimit rlNew;
        ::memset(&rlNew, 0, sizeof(rlNew));
        rlNew.rlim_cur = aActualNum;
        rlNew.rlim_max = aActualNum;
        nRet = ::setrlimit((__rlimit_resource_t)aResource, &rlNew);
        if (nRet == -1) {
            if (errno == EPERM) {
                SY_WARNING_TRACE("CSyEventHandlerRepository::SetRlimit, setrlimit() failed. "
                                 "you should use superuser to setrlimit(RLIMIT_NOFILE)!");
                aActualNum = static_cast<int>(rlCur.rlim_cur);
            } else {
                SY_WARNING_TRACE("CSyEventHandlerRepository::SetRlimit, setrlimit() failed! err=" << errno);
                return SY_ERROR_UNEXPECTED;
            }
        }
    } else {
        aActualNum = static_cast<int>(rlCur.rlim_cur);
    }

    return SY_OK;
}
#endif // !SY_WIN32

inline void FdSet_s(fd_set &aFsRead, fd_set &aFsWrite, fd_set &aFsException,
                    CSyEventHandlerRepository::CElement &aEleGet,
                    int &aMaxFd)
{
    int nSocket = (int)aEleGet.m_pEh->GetHandle();
    if (nSocket > aMaxFd) {
        aMaxFd = nSocket;
    }

    // READ, ACCEPT, and CONNECT flag will place the handle in the read set.
    if (SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::READ_MASK) ||
            SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::ACCEPT_MASK) ||
            SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::CONNECT_MASK)) {
        FD_SET(nSocket, &aFsRead);
    }
    // WRITE and CONNECT flag will place the handle in the write set.
    if (SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::WRITE_MASK) ||
            SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::CONNECT_MASK)) {
        FD_SET(nSocket, &aFsWrite);
    }
#ifdef SY_WIN32
    if (SY_BIT_ENABLED(aEleGet.m_Mask, ASyEventHandler::CONNECT_MASK)) {
        FD_SET(nSocket, &aFsException);
    }
#endif // SY_WIN32
}

int CSyEventHandlerRepository::
FillFdSets(fd_set &aFsRead, fd_set &aFsWrite, fd_set &aFsException)
{
    ChangeSyHandles();
    aFsRead=m_aFsRead;
    aFsWrite=m_aFsWrite;
    aFsException=m_aFsException;
    return m_nMaxFd;
}
void CSyEventHandlerRepository::
FillFdSets_i()
{
    FD_ZERO(&m_aFsRead);
    FD_ZERO(&m_aFsWrite);
    FD_ZERO(&m_aFsException);
    m_nMaxFd=-1;

#ifdef SY_WIN32
    HandlersType::iterator iter = m_Handlers.begin();
    for (; iter != m_Handlers.end(); ++iter) {
        CElement &eleGet = (*iter).second;
        FdSet_s(m_aFsRead, m_aFsWrite, m_aFsException, eleGet, m_nMaxFd);
    }
#else
    for (int i=0; i< m_nSyHandlesCount; ++i) {
        CElement &eleGet = m_pHandlers[m_cmHandles[i]];
        if (!eleGet.IsCleared()) {
            FdSet_s(m_aFsRead, m_aFsWrite, m_aFsException, eleGet, m_nMaxFd);
        }
    }
#endif // SY_WIN32
}

//////////////////////////////////////////////////////////////////////
// class CSyReactorBase
//////////////////////////////////////////////////////////////////////

CSyReactorBase::CSyReactorBase(PROPERTY aProperty)
    : ISyReactor(aProperty)
    , m_pTimerQueue(NULL)
    , m_bNotifyFailed(FALSE)
    , m_bNotifyHandle(FALSE)
{
}

CSyReactorBase::~CSyReactorBase()
{
    // needn't do Close() because the inherited class will do it
    //  Close();
}

SyResult CSyReactorBase::Open()
{
    m_Est.Reset2CurrentThreadInfo();
    CSyEventQueueUsingMutex::Reset2CurrentThreadInfo();
    CSyStopFlag::m_Est.Reset2CurrentThreadInfo();

    // check whether inheried class instanced the timer queue.
    if (!m_pTimerQueue) {
        m_pTimerQueue = CSyTimerQueueBase::CreateTimerQueue(NULL);
        if (!m_pTimerQueue) {
            return SY_ERROR_OUT_OF_MEMORY;
        }
    }

    SyResult rv = m_EhRepository.Open();
    if (SY_FAILED(rv)) {
        return rv;
    }

    return rv;
}

SyResult CSyReactorBase::
RegisterHandler(ASyEventHandler *aEh, ASyEventHandler::MASK aMask)
{
    // FIXME TODO: Register handler after OnClose!

    m_Est.EnsureSingleThread();
    SyResult rv;
    SY_ASSERTE_RETURN(aEh, SY_ERROR_INVALID_ARG);

    BOOL IsUDP = aMask & ASyEventHandler::UDP_LINK_MASK;
    ASyEventHandler::MASK maskNew = aMask & ASyEventHandler::ALL_EVENTS_MASK;
    /*
        if(IsUDP){
            maskNew |= ASyEventHandler::UDP_LINK_MASK;
            SY_INFO_TRACE_THIS("CSyReactorBase::RegisterHandler is UDP socket mask = " << maskNew);
        }
    */
    if (maskNew == ASyEventHandler::NULL_MASK) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::RegisterHandler, NULL_MASK. aMask=" << aMask);
        return SY_ERROR_INVALID_ARG;
    }

    CSyEventHandlerRepository::CElement eleFind;
    SY_HANDLE fdNew = aEh->GetHandle();
    rv = m_EhRepository.Find(fdNew, eleFind);
    if (maskNew == eleFind.m_Mask && aEh == eleFind.m_pEh) {
/*        SY_WARNING_TRACE_THIS("CSyReactorBase::RegisterHandler, mask is equal."
                              " aEh=" << aEh <<
                              " aMask=" << aMask <<
                              " fdNew=" << fdNew <<
                              " rv=" << rv);
 */
        return SY_OK;
    }

    if (eleFind.IsCleared()) {
        rv = OnHandleRegister(fdNew, IsUDP ? maskNew | ASyEventHandler::UDP_LINK_MASK : maskNew, aEh);

        // needn't remove handle when OnHandleRegister() failed
        // because the handle didn't be inserted at all
        if (SY_FAILED(rv)) {
            return rv;
        }
    }

    CSyEventHandlerRepository::CElement eleNew(aEh, maskNew);
    rv = m_EhRepository.Bind(fdNew, eleNew);
    return rv;
}

SyResult CSyReactorBase::
RemoveHandler(ASyEventHandler *aEh, ASyEventHandler::MASK aMask)
{
    m_Est.EnsureSingleThread();
    SyResult rv;
    SY_ASSERTE_RETURN(aEh, SY_ERROR_INVALID_ARG);

    ASyEventHandler::MASK maskNew = aMask & ASyEventHandler::ALL_EVENTS_MASK;
    if (maskNew == ASyEventHandler::NULL_MASK) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::RemoveHandler, NULL_MASK. aMask=" << aMask);
        return SY_ERROR_INVALID_ARG;
    }

    CSyEventHandlerRepository::CElement eleFind;
    SY_HANDLE fdNew = aEh->GetHandle();
    rv = m_EhRepository.Find(fdNew, eleFind);
    if (SY_FAILED(rv)) {
#if !defined SY_LINUX_SERVER
        // the trace has no useful for us and too much in the trace file, disable it by Victor Mar 21 2008
        SY_WARNING_TRACE_THIS("CSyReactorBase::RemoveHandler, handle not registed."
                              " aEh=" << aEh <<
                              " aMask=" << aMask <<
                              " fdNew=" << fdNew <<
                              " rv=" << rv);
#endif
        return rv;
    }

    rv = RemoveHandleWithoutFinding_i(fdNew, eleFind, maskNew);
    return rv;
}

SyResult CSyReactorBase::Close()
{
    if (m_pTimerQueue) {
        // I am sorry to comment it because PostMessage(WM_QUIT) will fail if
        // in atexit() route.
        //      m_Est.EnsureSingleThread();
        delete m_pTimerQueue;
        m_pTimerQueue = NULL;
    }
    m_EhRepository.Close();
    CSyEventQueueBase::DestoryPendingEvents();
    return SY_OK;
}

SyResult CSyReactorBase::
ScheduleTimer(ISyTimerHandler *aTh, LPVOID aArg,
              const CSyTimeValue &aInterval, DWORD aCount)
{
    m_Est.EnsureSingleThread();
    if (!m_pTimerQueue) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::ScheduleTimer, m_pTimerQueue not inited or closed.");
        return SY_ERROR_NOT_INITIALIZED;
    }

    return m_pTimerQueue->ScheduleTimer(aTh, aArg, aInterval, aCount);
}

SyResult CSyReactorBase::CancelTimer(ISyTimerHandler *aTh)
{
    m_Est.EnsureSingleThread();
    if (!m_pTimerQueue) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::CancelTimer, m_pTimerQueue not inited or closed.");
        return SY_ERROR_NOT_INITIALIZED;
    }

    return m_pTimerQueue->CancelTimer(aTh);
}

SyResult CSyReactorBase::
ProcessHandleEvent(SY_HANDLE aFd, ASyEventHandler::MASK aMask,
                   SyResult aReason, BOOL aIsNotify, BOOL aDropConnect)
{
    m_Est.EnsureSingleThread();
    if (aFd == SY_INVALID_HANDLE) {
        SY_ASSERTE(aMask == ASyEventHandler::EVENTQUEUE_MASK);

        // get one pending event once,
        // so that the events and signals are serial.
        // can't do this because it causes signal queue overflow.

        DWORD dwRemainSize = 0;
        CSyEventQueueBase::EventsType tmpListEvents_ProcessHandleEvent;//only for ProcessHandleEvent and one thread
        SyResult rv = CSyEventQueueUsingMutex::PopPendingEventsWithoutWait(
                          tmpListEvents_ProcessHandleEvent, CSyEventQueueBase::MAX_GET_ONCE, &dwRemainSize);
        if (SY_SUCCEEDED(rv)) {
            rv = CSyEventQueueBase::ProcessEvents(tmpListEvents_ProcessHandleEvent);
        }

        if (rv == SY_ERROR_TERMINATING) {
            return rv;
        }

        if (dwRemainSize) {
            rv = NotifyHandler(NULL, ASyEventHandler::EVENTQUEUE_MASK);
            if (SY_FAILED(rv)) {
                m_bNotifyFailed = TRUE;
            } else if (m_bNotifyFailed) {
                m_bNotifyFailed = FALSE;
            }
        }
        return rv;
    }

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
#endif // !SY_DISABLE_EVENT_REPORT

    CSyEventHandlerRepository::CElement eleFind;
    SyResult rv = m_EhRepository.Find(aFd, eleFind);
    if (SY_FAILED(rv)) {
        if (!aDropConnect) {
            SY_WARNING_TRACE_THIS("CSyReactorBase::ProcessHandleEvent, handle not registed."
                                  " aFd=" << aFd <<
                                  " aMask=" << aMask <<
                                  " aReason=" << aReason <<
                                  " rv=" << rv <<
                                  " tid=" << m_pTid <<
                                  " name="<< m_nameTid);
        }
        return rv;
    }

    if (SY_BIT_DISABLED(aMask, ASyEventHandler::CLOSE_MASK)) {
        ASyEventHandler::MASK maskActual = eleFind.m_Mask & aMask;
        // needn't check the registered mask if it is notify.
        if (!maskActual && !aIsNotify) {
            SY_WARNING_TRACE_THIS("CSyReactorBase::ProcessHandleEvent, mask not registed."
                                  " aFd=" << aFd <<
                                  " aMask=" << aMask <<
                                  " m_Mask=" << eleFind.m_Mask <<
                                  " aReason=" << aReason);
            return SY_OK;
        }

        int nOnCall = 0;
        if (aDropConnect && maskActual & ASyEventHandler::CONNECT_MASK) {
            SY_WARNING_TRACE_THIS("CSyReactorBase::ProcessHandleEvent, drop connect."
                                  " aFd=" << aFd <<
                                  " aMask=" << aMask <<
                                  " m_Mask=" << eleFind.m_Mask);
            nOnCall = -1;
        } else {
            if (maskActual & ASyEventHandler::ACCEPT_MASK
                    || maskActual & ASyEventHandler::READ_MASK) {
                nOnCall = eleFind.m_pEh->OnInput(aFd);
            }
            if ((nOnCall == 0 || nOnCall == -2) &&
                    (maskActual & ASyEventHandler::CONNECT_MASK
                     || maskActual & ASyEventHandler::WRITE_MASK)) {
                nOnCall = eleFind.m_pEh->OnOutput(aFd);
            }
        }

        if (nOnCall == 0) {
            rv = SY_OK;
        } else if (nOnCall == -2) {
            rv = SY_ERROR_WOULD_BLOCK;
        } else {
            // maybe the handle is reregiested or removed when doing callbacks.
            // so we have to refind it.
            CSyEventHandlerRepository::CElement eleFindAgain;
            rv = m_EhRepository.Find(aFd, eleFindAgain);
            if (SY_FAILED(rv) || eleFind.m_pEh != eleFindAgain.m_pEh) {
                //SY_ERROR_TRACE_THIS("CSyReactorBase::ProcessHandleEvent,"
                //  " callback shouldn't return fail after the fd is reregiested or removed!"
                //  " aFd=" << aFd <<
                //  " EHold=" << eleFind.m_pEh <<
                //  " EHnew=" << eleFindAgain.m_pEh <<
                //  " find=" << rv);
                SY_ASSERTE(FALSE);
            } else {
                rv = RemoveHandleWithoutFinding_i(aFd, eleFindAgain,
                                                  ASyEventHandler::ALL_EVENTS_MASK | ASyEventHandler::SHOULD_CALL);
            }
            rv = SY_ERROR_FAILURE;
        }
    } else {
        //      SY_INFO_TRACE_THIS("CSyReactorBase::ProcessHandleEvent, handle is closed."
        //          " aFd=" << aFd <<
        //          " aMask=" << aMask <<
        //          " aReason=" << aReason);

        //{ 2013/09/05, fix bug when notifying READ and CLOSE at same time
        int nOnCall = 0;
        ASyEventHandler::MASK maskActual = eleFind.m_Mask & aMask;
        if (maskActual & ASyEventHandler::READ_MASK) {
            nOnCall = eleFind.m_pEh->OnInput(aFd);
        }
        if (0 == nOnCall || -2 == nOnCall) {
            rv = RemoveHandleWithoutFinding_i(aFd, eleFind,
                                              ASyEventHandler::ALL_EVENTS_MASK | ASyEventHandler::SHOULD_CALL);
        } else {
            // maybe the handle is reregiested or removed when doing callbacks.
            // so we have to refind it.
            CSyEventHandlerRepository::CElement eleFindAgain;
            rv = m_EhRepository.Find(aFd, eleFindAgain);
            if (SY_FAILED(rv) || eleFind.m_pEh != eleFindAgain.m_pEh) { // how about m_pEh is reused???
                //SY_ERROR_TRACE_THIS("CSyReactorBase::ProcessHandleEvent,"
                //  " callback shouldn't return fail after the fd is reregiested or removed!"
                //  " aFd=" << aFd <<
                //  " EHold=" << eleFind.m_pEh <<
                //  " EHnew=" << eleFindAgain.m_pEh <<
                //  " find=" << rv);
                //SY_ASSERTE(FALSE);
            } else {
                rv = RemoveHandleWithoutFinding_i(aFd, eleFindAgain,
                                                  ASyEventHandler::ALL_EVENTS_MASK | ASyEventHandler::SHOULD_CALL);
            }
        }
        //}

        //rv = RemoveHandleWithoutFinding_i(aFd, eleFind,
        //  ASyEventHandler::ALL_EVENTS_MASK | ASyEventHandler::SHOULD_CALL);
        rv = SY_ERROR_FAILURE;
    }

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvSub = CSyTimeValue::GetTimeOfDay() - tvCur;
    if (tvSub > CSyEventQueueBase::s_tvReportInterval) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::ProcessHandleEvent, report,"
                              " sec=" << tvSub.GetSec() <<
                              " usec=" << tvSub.GetUsec() <<
                              " aFd=" << aFd <<
                              " aMask=" << aMask <<
                              " maskFind=" << eleFind.m_Mask <<
                              " ehFind=" << eleFind.m_pEh <<
                              " aReason=" << aReason <<
                              " tid="<< m_pTid <<
                              " name="<< m_nameTid);
    }
#endif // !SY_DISABLE_EVENT_REPORT
    return rv;
}

SyResult CSyReactorBase::ProcessTimerTick()
{
#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
#endif // !SY_DISABLE_EVENT_REPORT

    m_Est.EnsureSingleThread();
    SY_ASSERTE_RETURN(m_pTimerQueue, SY_ERROR_NOT_INITIALIZED);
    if (m_pTimerQueue) {
        m_pTimerQueue->CheckExpire();
    }

#ifndef SY_DISABLE_EVENT_REPORT
    CSyTimeValue tvSub = CSyTimeValue::GetTimeOfDay() - tvCur;
    if (tvSub > CSyEventQueueBase::s_tvReportInterval) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::ProcessTimerTick, report,"
                              " sec=" << tvSub.GetSec() <<
                              " usec=" << tvSub.GetUsec() <<
                              " tid=" << m_pTid <<
                              " name=" << m_nameTid);
    }
#endif // !SY_DISABLE_EVENT_REPORT
    return SY_OK;
}

SyResult CSyReactorBase::
RemoveHandleWithoutFinding_i(SY_HANDLE aFd,
                             const CSyEventHandlerRepository::CElement &aHe,
                             ASyEventHandler::MASK aMask)
{
    ASyEventHandler::MASK maskNew = aMask & ASyEventHandler::ALL_EVENTS_MASK;
    ASyEventHandler::MASK maskEh = aHe.m_Mask;
    ASyEventHandler::MASK maskSelect = (maskEh & maskNew) ^ maskEh;
    if (maskSelect == maskEh) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::RemoveHandleWithoutFinding_i, mask is equal. aMask=" << aMask);
        return SY_OK;
    }

    if (maskSelect == ASyEventHandler::NULL_MASK) {
        SyResult rv = m_EhRepository.UnBind(aFd);
        if (SY_FAILED(rv)) {
            SY_WARNING_TRACE_THIS("CSyReactorBase::RemoveHandleWithoutFinding_i, UnBind() failed!"
                                  " aFd=" << aFd <<
                                  " aMask=" << aMask <<
                                  " rv=" << rv);
        }
        OnHandleRemoved(aFd);
        if (aMask & ASyEventHandler::SHOULD_CALL) {
            aHe.m_pEh->OnClose(aFd, maskEh);
        }
        return SY_OK;
    } else {
        CSyEventHandlerRepository::CElement eleBind = aHe;
        eleBind.m_Mask = maskSelect;
        SyResult rvBind = m_EhRepository.Bind(aFd, eleBind);
        SY_ASSERTE(rvBind == SY_ERROR_FOUND);
        return rvBind;
    }
}

// this function can be invoked in the different thread.
SyResult CSyReactorBase::SendEvent(ISyEvent *aEvent)
{
    return CSyEventQueueUsingMutex::SendEvent(aEvent);
}

// this function can be invoked in the different thread.
SyResult CSyReactorBase::PostEvent(ISyEvent *aEvent, EPriority aPri)
{

    //it should got crash after the event already stopped, 9/2 2009
    SY_ASSERTE_RETURN(!CSyStopFlag::m_bStoppedFlag, SY_ERROR_NOT_INITIALIZED);
    if (CSyStopFlag::m_bStoppedFlag) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::PostEvent CSyStopFlag::m_bStoppedFlag = " << CSyStopFlag::m_bStoppedFlag);
    }
    if (m_bNotifyHandle) {
        return SY_ERROR_NOT_INITIALIZED;
    }
        
    DWORD dwOldSize = 0;
    SyResult rv = CSyEventQueueUsingMutex::
                  PostEventWithOldSize(aEvent, aPri, &dwOldSize);
    if (dwOldSize && m_bNotifyFailed) {
        SY_WARNING_TRACE_THIS("CSyReactorBase::PostEvent dwOldSize = " << dwOldSize);
    }
    if ((SY_SUCCEEDED(rv) && dwOldSize == 0)|| m_bNotifyFailed) {
        rv = NotifyHandler(NULL, ASyEventHandler::EVENTQUEUE_MASK);
        if (rv == SY_ERROR_NOT_INITIALIZED)
            m_bNotifyHandle = TRUE;
        if (SY_FAILED(rv)) {
            m_bNotifyFailed = TRUE;
        } else if (m_bNotifyFailed) {
            m_bNotifyFailed = FALSE;
        }
    }
    return rv;
}

// this function can be invoked in the different thread.
DWORD CSyReactorBase::GetPendingEventsCount()
{
    return CSyEventQueueUsingMutex::GetPendingEventsCount();
}

#endif //SUPPORT_REACTOR
