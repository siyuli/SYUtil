
#ifdef SUPPORT_REACTOR

//#include "SyBase.h"
#include "SyReactorSelect.h"
#include "SyTimerQueueBase.h"
#include "timer.h"
#include "SyUtilMisc.h"

USE_UTIL_NS

CSyReactorSelect::CSyReactorSelect()
    : CSyReactorBase(SEND_REGISTER_PROPERTY)
{
    m_bNeedCloseNotify = false;
#ifdef SY_WIN32
    m_bNeedNotify = true;
#endif

}

CSyReactorSelect::~CSyReactorSelect()
{
}

SyResult CSyReactorSelect::Open()
{
    SyResult rv = CSyReactorBase::Open();
    if (SY_FAILED(rv)) {
        goto fail;
    }

    rv = m_Notify.Open(this);
#ifdef SY_WIN32
    if (SY_FAILED(rv)) {
        m_bNeedNotify = false;
        SY_INFO_TRACE_THIS("CSyReactorSelect::Open(), although open notify pipe failed, but we still start");
    }
#else
    if (SY_FAILED(rv)) {
        goto fail;
    }
#endif
    CSyStopFlag::SetStartFlag();
    SY_INFO_TRACE_THIS("CSyReactorSelect::Open()");
    return SY_OK;

fail:
    Close();
    SY_ASSERTE(SY_FAILED(rv));
    SY_ERROR_TRACE_THIS("CSyReactorSelect::Open, failed!"
                        " rv=" << rv);
    return rv;
}

SyResult CSyReactorSelect::
NotifyHandler(ASyEventHandler *aEh, ASyEventHandler::MASK aMask)
{
#ifdef SY_WIN32
    if (m_bNeedNotify) {
        return m_Notify.Notify(aEh, aMask);
    }
    else {
        return SY_OK;
    }
#else
    return m_Notify.Notify(aEh, aMask);
#endif
}

SyResult CSyReactorSelect::RunEventLoop()
{
    SY_INFO_TRACE_THIS("CSyReactorSelect::RunEventLoop");
    m_Est.EnsureSingleThread();

    {
        CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_NeedCloseNotifyMutex);
        m_bNeedCloseNotify = true;
    }
#ifdef SY_WIN32
    SOCKET tmpHandle;
    if (!m_bNeedNotify) {
        tmpHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
#endif
    low_ticker select_err_ticker = low_ticker::now();
    while (!CSyStopFlag::IsFlagStopped()) {
        low_ticker select_ticker = low_ticker::now();
        CSyTimeValue tvTimeout(CSyTimeValue::get_tvMax());
        if (m_pTimerQueue) {
            // process timer prior to wait event.
            m_pTimerQueue->CheckExpire(&tvTimeout);
        }

        timeval tvSelect;
        tvSelect.tv_sec = tvTimeout.GetSec();
        tvSelect.tv_usec = static_cast<int>(tvTimeout.GetUsec());
        if(tvSelect.tv_usec / 1000000 > 0) {
            tvSelect.tv_sec += (tvSelect.tv_usec / 1000000);
            tvSelect.tv_usec = tvSelect.tv_usec % 1000000;
            SY_INFO_TRACE_THIS("CSyReactorSelect::RunEventLoop, who schedule a timer more than 1 seconds by useconds.");
        }

        fd_set fsRead, fsWrite, fsException;
        int nMaxFd = m_EhRepository.FillFdSets(fsRead, fsWrite, fsException);
        //SY_ASSERTE(nMaxFd >= 0);

        int nSelect;
#ifdef SY_WIN32
        if (m_Notify.IsReconnectFail())
            m_bNeedNotify = false;

        if (m_bNeedNotify) {
            nSelect = ::select(nMaxFd + 1, &fsRead, &fsWrite, &fsException,
                tvTimeout == CSyTimeValue::get_tvMax() ? NULL : &tvSelect);
        }
        else {
            ProcessHandleEvent(SY_INVALID_HANDLE, ASyEventHandler::EVENTQUEUE_MASK, SY_OK, TRUE);

            timeval tvDefaultTimeout;
            tvDefaultTimeout.tv_sec = 0;
            tvDefaultTimeout.tv_usec = 10 * 1000;

            if (nMaxFd != -1)
                nSelect = ::select(nMaxFd + 1, &fsRead, &fsWrite, &fsException,
                    tvTimeout == CSyTimeValue::get_tvMax() ? &tvDefaultTimeout : &tvSelect);
            else {
                FD_ZERO(&fsRead);
                FD_SET(tmpHandle, &fsRead);
                nSelect = ::select(tmpHandle + 1, &fsRead, NULL, NULL,
                    tvTimeout == CSyTimeValue::get_tvMax() ? &tvDefaultTimeout : &tvSelect);
            }
        }
#else
        nSelect = ::select(nMaxFd + 1, &fsRead, &fsWrite, &fsException,
            tvTimeout == CSyTimeValue::get_tvMax() ? NULL : &tvSelect);
#endif
        int64_t elapsed_time = select_ticker.elapsed_mills();
        if(elapsed_time > 100) {
            SY_EVERY_N_INFO_TRACE(20, "CSyReactorSelect::RunEventLoop ::select() takes a long time, elapsed_time = " << elapsed_time);
        }

        if (nSelect == 0 || (nSelect == -1 && errno == EINTR)) {
            continue;
        } else if (nSelect == -1) {
#ifdef SY_WIN32
            if (m_bNeedNotify) {
                SY_EVERY_N_WARNING_TRACE(6000, "CSyReactorSelect::RunEventLoop, select() failed!"
                    " nMaxFd=" << nMaxFd << " err=" << errno <<
                    ", tvSelect=" << tvSelect.tv_sec << " - " << tvSelect.tv_usec);
                SleepMs(20);
            }
#else
            int64_t elapsed_time = select_err_ticker.elapsed_mills();
            if(elapsed_time > 1000) {
                SY_WARNING_TRACE_THIS("CSyReactorSelect::RunEventLoop, select() failed!"
                                         " nMaxFd=" << nMaxFd << " err=" << errno <<
                                         ", tvSelect=" << tvSelect.tv_sec << " - " << tvSelect.tv_usec);
                select_err_ticker = low_ticker::now();
            }

#endif
            continue;
            //return SY_ERROR_FAILURE;
        }

        int nActiveNumber = nSelect;
        ProcessFdSets_i(
            fsRead,
            ASyEventHandler::READ_MASK | ASyEventHandler::ACCEPT_MASK | ASyEventHandler::CONNECT_MASK,
            nActiveNumber, nMaxFd);
        ProcessFdSets_i(
            fsWrite,
            ASyEventHandler::WRITE_MASK | ASyEventHandler::CONNECT_MASK,
            nActiveNumber, nMaxFd);
#ifdef SY_WIN32
        ProcessFdSets_i(
            fsException,
            ASyEventHandler::CLOSE_MASK,
            nActiveNumber, nMaxFd);
#endif // SY_WIN32

        // Needn't check nActiveNumber due to because
        // fd maybe removed when doing callback.
        //      SY_ASSERTE(nActiveNumber == 0);
    }
#ifdef SY_WIN32
    if (!m_bNeedNotify) {
        ::closesocket(tmpHandle);
    }
#endif
    {
        CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_NeedCloseNotifyMutex);
        m_bNeedCloseNotify = false;
    }
    return SY_OK;
}

void CSyReactorSelect::
ProcessFdSets_i(fd_set &aFdSet, ASyEventHandler::MASK aMask,
                int &aActiveNumber, int aMaxFd)
{
#ifdef SY_WIN32
    for (unsigned i = 0; i < aFdSet.fd_count && aActiveNumber > 0; i++) {
        SY_HANDLE fdGet = (SY_HANDLE)aFdSet.fd_array[i];
        if (fdGet == SY_INVALID_HANDLE) {
            continue;
        }
        aActiveNumber--;
        ProcessHandleEvent(fdGet, aMask, SY_OK, FALSE);
    }
#else
    SY_HANDLE *pSyHandles=m_EhRepository.GetSyHandles();
    const int nSyHandlesCount=m_EhRepository.GetSyHandlesCount();
    for (int i=0; nSyHandlesCount>i; ++i) {
        if (FD_ISSET(pSyHandles[i],&aFdSet)) {
            ProcessHandleEvent(pSyHandles[i], aMask, SY_OK, FALSE);
        }
    }
#endif // SY_WIN32
}

SyResult CSyReactorSelect::StopEventLoop()
{
    SetStopFlagWithoutThreadCheck(TRUE);
    CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(m_NeedCloseNotifyMutex);
    if(m_bNeedCloseNotify)
        m_Notify.Notify(&m_Notify, ASyEventHandler::NULL_MASK);
    return SY_OK;
}

SyResult CSyReactorSelect::Close()
{
    m_Notify.Close();
    return CSyReactorBase::Close();
}

SyResult CSyReactorSelect::
OnHandleRegister(SY_HANDLE aFd, ASyEventHandler::MASK aMask, ASyEventHandler *aEh)
{
    return SY_OK;
}

void CSyReactorSelect::OnHandleRemoved(SY_HANDLE aFd)
{
}

#endif //SUPPORT_REACTOR
