
#ifdef SUPPORT_REACTOR

//#include "SyBase.h"
#include "SyReactorWin32Message.h"
#include "SyErrorNetwork.h"
#include "SyNetwork.h"

#include "SyTpMisc.h"

//////////////////////////////////////////////////////////////////////
// class CSyReactorWin32Message
//////////////////////////////////////////////////////////////////////

#define WM_WIN32_SOCKET_SELECT    WM_USER+33
#define WM_WIN32_SOCKET_NOTIFY    WM_USER+34
#define WIN32_SOCKET_CLASS_NAME   "SyWin32SocketNotification"

HINSTANCE g_pReactorWin3Instance;
ATOM g_atomRegisterClass;
char g_szClassName[MAX_PATH] = {0};

LRESULT CALLBACK CSyReactorWin32Message::
Win32SocketWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_WIN32_SOCKET_SELECT: {
            CSyReactorWin32Message *pReactor = (CSyReactorWin32Message *)::GetWindowLongPtr(hwnd, 0);
            ASyEventHandler::MASK maskEvent = ASyEventHandler::NULL_MASK;
            SY_HANDLE sockHandle = (SY_HANDLE)wParam;
            SY_ASSERTE(pReactor);
            SY_ASSERTE(sockHandle != SY_INVALID_HANDLE);

            SyResult rvError = SY_OK;
            int nErrorCode = WSAGETSELECTERROR(lParam);
            if (nErrorCode != 0) {
                rvError = SY_ERROR_NETWORK_SOCKET_ERROR;
            }

            BOOL bHandle = TRUE;
            switch (WSAGETSELECTEVENT(lParam)) {
                case FD_ACCEPT:
                    maskEvent |= ASyEventHandler::ACCEPT_MASK;
                    break;
                case FD_READ:
                    maskEvent |= ASyEventHandler::READ_MASK;
                    break;
                case FD_WRITE:
                    maskEvent |= ASyEventHandler::WRITE_MASK;
                    break;
                case FD_CONNECT:
                    maskEvent |= ASyEventHandler::CONNECT_MASK;
                    break;
                case FD_CLOSE:
                    //  SY_ASSERTE(nErrorCode);
                    rvError = SY_ERROR_NETWORK_SOCKET_CLOSE;
                    break;
                default:
                    bHandle = FALSE;
                    break;
            }

            if (WSAGETSELECTEVENT(lParam) == FD_CLOSE && nErrorCode == 0) {
                // this is a graceful close,
                // we must check the remain data in the socket.
                for (; ;) {
                    unsigned long dwRemain = 0;
                    int nRet = ::ioctlsocket(
                                   (SY_SOCKET)sockHandle,
                                   FIONREAD,
                                   &dwRemain);
                    if (nRet == 0 && dwRemain > 0) {
                        SY_WARNING_TRACE("CSyReactorWin32Message::Win32SocketWndProc,"
                                         " data remained in the handle wehn closing, recv it."
                                         " dwRemain=" << dwRemain);
                        pReactor->ProcessHandleEvent(
                            sockHandle,
                            ASyEventHandler::READ_MASK,
                            SY_OK,
                            FALSE);
                    } else {
                        break;
                    }
                }
            }

            if (nErrorCode != 0 || WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
                SY_INFO_TRACE("CSyReactorWin32Message::Win32SocketWndProc, handle is closed."
                              " fd=" << sockHandle <<
                              " mask=" << maskEvent <<
                              " nErrorCode=" << nErrorCode <<
                              " lParam=" << lParam <<
                              " rvError=" << rvError);
                SY_SET_BITS(maskEvent, ASyEventHandler::CLOSE_MASK);
            }

            if (bHandle) {
                SyResult rv = pReactor->ProcessHandleEvent(sockHandle, maskEvent, rvError, FALSE);
                if (rv == SY_ERROR_TERMINATING) {
                    return 0;
                }
            } else {
                SY_WARNING_TRACE("CSyReactorWin32Message::Win32SocketWndProc, unknown SELECTEVENT"
                                 " wParam=" << wParam << " lParam=" << lParam);
            }

#ifndef SY_ENABLE_CALENDAR_TIMER
            pReactor->ProcessTimerTick();
#endif // !SY_ENABLE_CALENDAR_TIMER
            return 0;
        }

        case WM_WIN32_SOCKET_NOTIFY: {
            CSyReactorWin32Message *pReactor = (CSyReactorWin32Message *)::GetWindowLongPtr(hwnd, 0);
            SY_HANDLE fdOn = (SY_HANDLE)wParam;
            ASyEventHandler::MASK maskEh = (ASyEventHandler::MASK)lParam;
            SY_ASSERTE(pReactor);

            SyResult rv = pReactor->ProcessHandleEvent(fdOn, maskEh, SY_OK, TRUE);
            if (rv == SY_ERROR_TERMINATING) {
                return 0;
            }
#ifndef SY_ENABLE_CALENDAR_TIMER
            pReactor->ProcessTimerTick();
#endif // !SY_ENABLE_CALENDAR_TIMER
            return 0;
        }

        case WM_TIMER: {
            CSyReactorWin32Message *pReactor = (CSyReactorWin32Message *)::GetWindowLongPtr(hwnd, 0);
            SY_ASSERTE(pReactor);

#ifdef SY_ENABLE_CALENDAR_TIMER
            pReactor->m_CalendarTimer.TimerTick();
#else
            pReactor->ProcessTimerTick();
#endif // SY_ENABLE_CALENDAR_TIMER

            break;
        }

        default :
            break;
    }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CSyReactorWin32Message::CSyReactorWin32Message()
    : m_hwndNotify(NULL)
    , m_dwTimerId(0)
#ifdef SY_ENABLE_CALENDAR_TIMER
    , m_CalendarTimer(50, 1000*60*60*2, static_cast<CSyEventQueueUsingMutex *>(this))
#endif // SY_ENABLE_CALENDAR_TIMER
{
}

CSyReactorWin32Message::~CSyReactorWin32Message()
{
    Close();
    SY_INFO_TRACE_THIS("CSyReactorWin32Message::~CSyReactorWin32Message()");
}

SyResult CSyReactorWin32Message::Open()
{
    SyResult rv = SY_ERROR_UNEXPECTED;
    g_pReactorWin3Instance = (HMODULE)GetTPDllHandle(); //::GetModuleHandle(NULL);
    SY_INFO_TRACE_THIS("CSyReactorWin32Message::Open() Handle = " << g_pReactorWin3Instance);
    SY_ASSERTE_RETURN(!m_hwndNotify, SY_ERROR_ALREADY_INITIALIZED);

#ifdef SY_ENABLE_CALENDAR_TIMER
    m_CalendarTimer.m_Est.Reset2CurrentThreadInfo();
#endif // SY_ENABLE_CALENDAR_TIMER
    rv = CSyReactorBase::Open();
    if (SY_FAILED(rv)) {
        goto fail;
    }

    if (g_atomRegisterClass == 0) {
        DWORD dwCount = GetTickCount();
        snprintf(g_szClassName,sizeof(g_szClassName),"%s_%d",WIN32_SOCKET_CLASS_NAME,dwCount);

        WNDCLASS wc;
        wc.style = 0;
        wc.lpfnWndProc = Win32SocketWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(void *);
        wc.hInstance = g_pReactorWin3Instance;
        wc.hIcon = 0;
        wc.hCursor = 0;
        wc.hbrBackground = 0;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = g_szClassName;

        if ((g_atomRegisterClass = ::RegisterClass(&wc)) == 0) {
            SY_ERROR_TRACE_THIS("CSyReactorWin32Message::Open, RegisterClass() failed!"
                                " err=" << ::GetLastError());
            rv = SY_ERROR_FAILURE;
            goto fail;
        }
    }

    m_hwndNotify = ::CreateWindow(g_szClassName, NULL, WS_OVERLAPPED, 0,
                                  0, 0, 0, NULL, NULL, g_pReactorWin3Instance, 0);
    if (!m_hwndNotify) {
        SY_ERROR_TRACE_THIS("CSyReactorWin32Message::Open, CreateWindow() failed!"
                            " err=" << ::GetLastError());
        rv = SY_ERROR_FAILURE;
        goto fail;
    }

    ::SetLastError(0);
    if (::SetWindowLongPtr(m_hwndNotify, 0, (LONG_PTR)this) == 0  && ::GetLastError() != 0) {
        SY_ERROR_TRACE_THIS("CSyReactorWin32Message::Open, SetWindowLongPtr() failed!"
                            " err=" << ::GetLastError());
        rv = SY_ERROR_FAILURE;
        goto fail;
    }

    m_dwTimerId = ::SetTimer(m_hwndNotify, m_dwTimerId, 30, NULL);
    if (m_dwTimerId == 0) {
        SY_ERROR_TRACE_THIS("CSyReactorWin32Message::Open, SetTimer() failed!"
                            " err=" << ::GetLastError());
        rv = SY_ERROR_FAILURE;
        goto fail;
    }

    CSyStopFlag::m_bStoppedFlag = FALSE;
    return SY_OK;

fail:
    Close();
    SY_ASSERTE(SY_FAILED(rv));
    return rv;
}

SyResult CSyReactorWin32Message::
NotifyHandler(ASyEventHandler *aEh, ASyEventHandler::MASK aMask)
{
    // Different threads may call this function due to EventQueue.
    SY_HANDLE fdNew = SY_INVALID_HANDLE;
    if (aEh) {
        m_Est.EnsureSingleThread();
        fdNew = aEh->GetHandle();
        SY_ASSERTE(fdNew != SY_INVALID_HANDLE);
    }

    BOOL bRet = ::PostMessage(m_hwndNotify, WM_WIN32_SOCKET_NOTIFY,
                              (WPARAM)fdNew, (LPARAM)aMask);
    if (!bRet) {
        SY_ERROR_TRACE_THIS("CSyReactorWin32Message::NotifyHandler, PostMessage() failed!"
                            " err=" << ::GetLastError());
        return SY_ERROR_UNEXPECTED;
    } else {
        return SY_OK;
    }
}

SyResult CSyReactorWin32Message::RunEventLoop()
{
    SY_INFO_TRACE_THIS("CSyReactorWin32Message::RunEventLoop");
    m_Est.EnsureSingleThread();

    MSG msg;
    while (!CSyStopFlag::m_bStoppedFlag && ::GetMessage(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return SY_OK;
}

// this function can be invoked in the different thread.
SyResult CSyReactorWin32Message::StopEventLoop()
{
    SY_INFO_TRACE_THIS("CSyReactorWin32Message::StopEventLoop");

    //  ::PostQuitMessage(0);
    CSyStopFlag::m_bStoppedFlag = TRUE;

    CSyEventQueueUsingMutex::Stop();
    return SY_OK;
}

SyResult CSyReactorWin32Message::Close()
{
    if (m_hwndNotify) {
        ::DestroyWindow(m_hwndNotify);
        m_hwndNotify = NULL;
    }
    return CSyReactorBase::Close();
}

SyResult CSyReactorWin32Message::
OnHandleRegister(SY_HANDLE aFd, ASyEventHandler::MASK aMask, ASyEventHandler *aEh)
{
    m_Est.EnsureSingleThread();
    SY_ERROR_TRACE_THIS("CSyReactorWin32Message::OnHandleRegister,"
                        " aFd=" << aFd <<
                        " aMask=" << aMask);
    return SY_ERROR_UNEXPECTED;
}

void CSyReactorWin32Message::OnHandleRemoved(SY_HANDLE aFd)
{
    m_Est.EnsureSingleThread();
    SY_ERROR_TRACE_THIS("CSyReactorWin32Message::OnHandleRemoved, aFd=" << aFd);
}

#ifdef SY_ENABLE_CALENDAR_TIMER
SyResult CSyReactorWin32Message::
ScheduleTimer(ISyTimerHandler *aTh, LPVOID aArg,
              const CSyTimeValue &aInterval, DWORD aCount)
{
    return m_CalendarTimer.ScheduleTimer(aTh, aArg, aInterval, aCount);
}

SyResult CSyReactorWin32Message::CancelTimer(ISyTimerHandler *aTh)
{
    return m_CalendarTimer.CancelTimer(aTh);
}
#endif // SY_ENABLE_CALENDAR_TIMER


//////////////////////////////////////////////////////////////////////
// class CSyReactorWin32AsyncSelect
//////////////////////////////////////////////////////////////////////

CSyReactorWin32AsyncSelect::CSyReactorWin32AsyncSelect()
{
}

CSyReactorWin32AsyncSelect::~CSyReactorWin32AsyncSelect()
{
}

SyResult CSyReactorWin32AsyncSelect::
OnHandleRegister(SY_HANDLE aFd, ASyEventHandler::MASK aMask, ASyEventHandler *aEh)
{
    return DoAsyncSelect_i(aFd, ASyEventHandler::ALL_EVENTS_MASK);
}

void CSyReactorWin32AsyncSelect::OnHandleRemoved(SY_HANDLE aFd)
{
    DoAsyncSelect_i(aFd, ASyEventHandler::NULL_MASK);
}

SyResult CSyReactorWin32AsyncSelect::
DoAsyncSelect_i(SY_HANDLE aFd, ASyEventHandler::MASK aMask)
{
    long lEvent = 0;
    if (aMask & ASyEventHandler::CONNECT_MASK) {
        lEvent |= FD_CONNECT;
    }
    if (aMask & ASyEventHandler::ACCEPT_MASK) {
        lEvent |= FD_ACCEPT;
    }
    if (aMask & ASyEventHandler::READ_MASK) {
        lEvent |= FD_READ;
    }
    if (aMask & ASyEventHandler::WRITE_MASK) {
        lEvent |= FD_WRITE;
    }

    if (lEvent != 0) {
        lEvent |= FD_CLOSE;
    }
    if (::WSAAsyncSelect((SOCKET)aFd, m_hwndNotify, WM_WIN32_SOCKET_SELECT, lEvent) != 0) {
        SY_ERROR_TRACE_THIS("CSyReactorWin32AsyncSelect::DoAsyncSelect_i, WSAAsyncSelect() failed!"
                            " aFd=" << aFd <<
                            " err=" << ::WSAGetLastError());
        return SY_ERROR_UNEXPECTED;
    } else {
        return SY_OK;
    }
}


#endif //SUPPORT_REACTOR
