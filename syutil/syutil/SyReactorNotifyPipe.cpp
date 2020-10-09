
#ifdef SUPPORT_REACTOR

#include "SyReactorNotifyPipe.h"
#include "SyReactorBase.h"
#include "SySocket.h"
#include "SyErrorNetwork.h"

USE_UTIL_NS

CSyReactorNotifyPipe::CSyReactorNotifyPipe()
    : m_pReactor(NULL)
{
    SY_INFO_TRACE_THIS("CSyReactorNotifyPipe::CSyReactorNotifyPipe()");
    bReconnectFail = false;
}

CSyReactorNotifyPipe::~CSyReactorNotifyPipe()
{
    Close();
}

SyResult CSyReactorNotifyPipe::Open(CSyReactorBase *aReactor)
{
    SyResult rv = SY_OK;
    CSyIPCBase ipcNonblock;

    SY_ASSERTE(!m_pReactor);
    m_pReactor = aReactor;
    SY_ASSERTE_RETURN(m_pReactor, SY_ERROR_INVALID_ARG);

    rv = m_PipeNotify.Open();
    if (SY_FAILED(rv)) {
        goto fail;
    }

    ipcNonblock.SetHandle(m_PipeNotify.GetReadHandle());
    if (ipcNonblock.Enable(CSyIPCBase::NON_BLOCK) == -1) {
        SY_ERROR_TRACE_THIS("CSyReactorNotifyPipe::Open, Enable(NON_BLOCK) failed! err=" << errno);
        rv = SY_ERROR_NETWORK_SOCKET_ERROR;
        goto fail;
    }

    rv = m_pReactor->RegisterHandler(this, ASyEventHandler::READ_MASK);
    if (SY_FAILED(rv)) {
        goto fail;
    }

    SY_INFO_TRACE_THIS("CSyReactorNotifyPipe::Open,"
                       " read_fd=" << m_PipeNotify.GetReadHandle() << " write_fd=" << m_PipeNotify.GetWriteHandle());
    return SY_OK;

fail:
    Close();
    SY_ASSERTE(SY_FAILED(rv));
    return rv;
}

SyResult CSyReactorNotifyPipe::ReOpen()
{
    SY_INFO_TRACE_THIS("CSyReactorNotifyPipe::ReOpen()");
    if (m_pReactor) {
        m_pReactor->RemoveHandler(this);
    }
    m_PipeNotify.Close();

    SyResult rv = SY_OK;
    CSyIPCBase ipcNonblock;

    SY_ASSERTE_RETURN(m_pReactor, SY_ERROR_INVALID_ARG);

    rv = m_PipeNotify.Open();
    if (SY_FAILED(rv)) {
        SY_WARNING_TRACE_THIS("CSyReactorNotifyPipe::ReOpen, PipeNotify failed rv=" << rv);
        goto fail;
    }

    ipcNonblock.SetHandle(m_PipeNotify.GetReadHandle());
    if (ipcNonblock.Enable(CSyIPCBase::NON_BLOCK) == -1) {
        SY_ERROR_TRACE_THIS("CSyReactorNotifyPipe::ReOpen, Enable(NON_BLOCK) failed! err=" << errno);
        rv = SY_ERROR_NETWORK_SOCKET_ERROR;
        goto fail;
    }

    rv = m_pReactor->RegisterHandler(this, ASyEventHandler::READ_MASK);
    if (SY_FAILED(rv)) {
        SY_WARNING_TRACE_THIS("CSyReactorNotifyPipe::ReOpen, RegisterHander failed rv=" << rv);
        goto fail;
    }

    SY_INFO_TRACE_THIS("CSyReactorNotifyPipe::ReOpen,"
        " read_fd=" << m_PipeNotify.GetReadHandle() << " write_fd=" << m_PipeNotify.GetWriteHandle());
    return SY_OK;

fail:
    SY_WARNING_TRACE_THIS("CSyReactorNotifyPipe::ReOpen failed rv=" << rv);
    Close();
    bReconnectFail = true;

    SY_ASSERTE(SY_FAILED(rv));
    return rv;
}

SY_HANDLE CSyReactorNotifyPipe::GetHandle() const
{
    return m_PipeNotify.GetReadHandle();
}

int CSyReactorNotifyPipe::OnInput(SY_HANDLE aFd)
{
    SY_ASSERTE(aFd == m_PipeNotify.GetReadHandle());

    CBuffer bfNew;
    int nRecv = static_cast<int>(::recv(
                                     (SY_SOCKET)m_PipeNotify.GetReadHandle(),
                                     (char *)&bfNew, sizeof(bfNew), 0));

    if (nRecv < (int)sizeof(bfNew)) {
#ifdef SY_WIN32
        errno = ::WSAGetLastError();
        // fix issue: https://jira-eng-gpk2.cisco.com/jira/browse/WEBEX-17296
        if (errno == 10054 || errno == 10057) {
            SY_ERROR_TRACE_THIS("CSyReactorNotifyPipe::OnInput,"
                " nRecv=" << nRecv <<
                " fd=" << m_PipeNotify.GetReadHandle() <<
                " err=" << errno);
            ReOpen();
            return 0;
        }
#endif // SY_WIN32
        SY_ERROR_TRACE_THIS("CSyReactorNotifyPipe::OnInput,"
                            " nRecv=" << nRecv <<
                            " fd=" << m_PipeNotify.GetReadHandle() <<
                            " err=" << errno);

        return 0;
    }

    // we use sigqueue to notify close
    // so that we needn't this pipi to stop the reactor.
#if 0
    BOOL bStopReactor = FALSE;
    if (bfNew.m_Fd == m_PipeNotify.GetReadHandle()) {
        SY_ASSERTE(bfNew.m_Mask == ASyEventHandler::CLOSE_MASK);
        bfNew.m_Fd = SY_INVALID_HANDLE;
        bStopReactor = TRUE;
    }
#else
    if (bfNew.m_Fd == m_PipeNotify.GetReadHandle()) {
        return 0;
    }
#endif

    SY_ASSERTE(m_pReactor);
    if (m_pReactor) {
        m_pReactor->ProcessHandleEvent(bfNew.m_Fd, bfNew.m_Mask, SY_OK, TRUE);
    }

#if 0
    if (bStopReactor) {
        SY_INFO_TRACE_THIS("CSyReactorNotifyPipe::OnInput, reactor is stopped.");
        m_pReactor->CSyStopFlag::SetStopFlag();
    }
#endif
    return 0;
}

SyResult CSyReactorNotifyPipe::
Notify(ASyEventHandler *aEh, ASyEventHandler::MASK aMask)
{
    // this function can be invoked in the different thread.
    if (m_PipeNotify.GetWriteHandle() == SY_INVALID_HANDLE) {
        SY_WARNING_TRACE_THIS("CSyReactorNotifyPipe::Notify, WriteHandle INVALID.");
        return SY_ERROR_NOT_INITIALIZED;
    }

    SY_HANDLE fdNew = SY_INVALID_HANDLE;
    if (aEh) {
        fdNew = aEh->GetHandle();
        SY_ASSERTE(fdNew != SY_INVALID_HANDLE);
    }

    CBuffer bfNew(fdNew, aMask);
    int nSend = static_cast<int>(::send(
                                     (SY_SOCKET)m_PipeNotify.GetWriteHandle(),
                                     (char *)&bfNew, sizeof(bfNew), 0));
    if (nSend < (int)sizeof(bfNew)) {
        SY_ERROR_TRACE_THIS("CSyReactorNotifyPipe::Notify,"
                            " nSend=" << nSend <<
                            " fd=" << m_PipeNotify.GetWriteHandle() <<
                            " err=" << errno);
        return SY_ERROR_UNEXPECTED;
    }
    return SY_OK;
}

SyResult CSyReactorNotifyPipe::Close()
{
    if (m_pReactor) {
        m_pReactor->RemoveHandler(this);
        m_pReactor = NULL;
    }
    return m_PipeNotify.Close();
}

#endif //SUPPORT_REACTOR
