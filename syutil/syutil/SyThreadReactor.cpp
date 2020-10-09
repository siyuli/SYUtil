
//#include "SyBase.h"
#include "SyAssert.h"
#include "SyDebug.h"
#include "SyThreadManager.h"
#include "SyThreadReactor.h"
#include "SyReactorInterface.h"
#include "SyConditionVariable.h"

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSyThreadReactor
//////////////////////////////////////////////////////////////////////

CSyThreadReactor::CSyThreadReactor()
    : m_pReactor(NULL)
{
}

CSyThreadReactor::~CSyThreadReactor()
{
    delete m_pReactor;
}

SyResult CSyThreadReactor::Init(ISyReactor *aReactor)
{
    SY_ASSERTE_RETURN(!m_pReactor, SY_ERROR_ALREADY_INITIALIZED);
    SY_ASSERTE_RETURN(aReactor, SY_ERROR_INVALID_ARG);

    m_pReactor = aReactor;
    return SY_OK;
}

SyResult CSyThreadReactor::
Create(const char *name,TType aType, TFlag aFlag)
{
    SyResult rv = ASyThread::Create(name , aType, aFlag, TRUE);
    if (SY_SUCCEEDED(rv)) {
        // have to open reactor here because main function will do some initial stuffs.
        if (m_Type == TT_MAIN) {
            rv = m_pReactor->Open();
            if (SY_FAILED(rv)) {
                SY_ERROR_TRACE_THIS("CSyThreadReactor::OnThreadRun, m_pReactor->Open() failed! rv=" << rv);
            }
        }
    }
    return rv;
}

void CSyThreadReactor::OnThreadInit()
{
    SY_ASSERTE_RETURN_VOID(m_pReactor);

    ASyThread::OnThreadInit();
    if (m_Type != TT_MAIN) {
        SyResult rv = m_pReactor->Open();
        if (SY_FAILED(rv)) {
            SY_ERROR_TRACE_THIS("CSyThreadReactor::OnThreadInit, m_pReactor->Open() failed! rv=" << rv);
            SY_ASSERTE(FALSE);
        }
    }
}

void CSyThreadReactor::OnThreadRun()
{
    SY_ASSERTE_RETURN_VOID(m_pReactor);
    SY_INFO_TRACE_THIS("CSyThreadReactor::OnThreadRun, m_pReactor = " << m_pReactor);

    m_pReactor->RunEventLoop();

    // close the notify avoid Close in other thread .
    // because it will remove handler in the reactor.
    m_pReactor->Close();
}

SyResult CSyThreadReactor::Stop(CSyTimeValue *aTimeout)
{
    //  SY_ASSERTE_RETURN(!aTimeout, SY_ERROR_NOT_IMPLEMENTED);
    SY_ASSERTE_RETURN(m_pReactor, SY_ERROR_NOT_INITIALIZED);
    return m_pReactor->StopEventLoop();
}

ISyReactor *CSyThreadReactor::GetReactor()
{
    return m_pReactor;
}

ISyEventQueue *CSyThreadReactor::GetEventQueue()
{
    return m_pReactor;
}

ISyTimerQueue *CSyThreadReactor::GetTimerQueue()
{
    return m_pReactor;
}

//////////////////////////////////////////////////////////////////////
// class CSyThreadDummy
//////////////////////////////////////////////////////////////////////

CSyThreadDummy::CSyThreadDummy()
    : m_pActualThread(NULL)
{
}

CSyThreadDummy::~CSyThreadDummy()
{
}

SyResult CSyThreadDummy::Init(ASyThread *aThread, TType aType)
{
    SY_ASSERTE_RETURN(!m_pActualThread, SY_ERROR_ALREADY_INITIALIZED);
    SY_ASSERTE_RETURN(aThread, SY_ERROR_INVALID_ARG);
    SY_ASSERTE(aThread->GetThreadType() != aType);

    m_Type = aType;
    m_Tid = aThread->GetThreadId();
    m_Handle = aThread->GetThreadHandle();
    m_pActualThread = aThread;

    return CSyThreadManager::Instance()->RegisterThread(this);
}

SyResult CSyThreadDummy::
Create(TType aType, TFlag aFlag)
{
    SY_ASSERTE(!"CSyThreadDummy::Create");
    return SY_ERROR_NOT_IMPLEMENTED;
}

SyResult CSyThreadDummy::Stop(CSyTimeValue *aTimeout)
{
    if (m_pActualThread) {
        return m_pActualThread->Stop(aTimeout);
    }
    return
        SY_ERROR_NOT_INITIALIZED;
}

void CSyThreadDummy::OnThreadRun()
{
    SY_ASSERTE(!"CSyThreadDummy::OnThreadRun");
}

ISyReactor *CSyThreadDummy::GetReactor()
{
    if (m_pActualThread) {
        return m_pActualThread->GetReactor();
    } else {
        return NULL;
    }
}

ISyEventQueue *CSyThreadDummy::GetEventQueue()
{
    if (m_pActualThread) {
        return m_pActualThread->GetEventQueue();
    } else {
        return NULL;
    }
}

ISyTimerQueue *CSyThreadDummy::GetTimerQueue()
{
    if (m_pActualThread) {
        return m_pActualThread->GetTimerQueue();
    } else {
        return NULL;
    }
}
