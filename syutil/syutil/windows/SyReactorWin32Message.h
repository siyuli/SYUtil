#ifndef SYREACTORWIN32MESSAGE_H
#define SYREACTORWIN32MESSAGE_H

#ifdef SUPPORT_REACTOR

#include "SyReactorBase.h"

#ifndef SY_WIN32
    #error ERROR: only WIN32 supports Win32 Messages!
#endif // SY_WIN32


#ifdef SY_ENABLE_CALENDAR_TIMER
    #include "SyTimerQueueCalendar.h"
#endif // SY_ENABLE_CALENDAR_TIMER

class CSyReactorWin32Message : public CSyReactorBase
{
public:
    CSyReactorWin32Message();
    virtual ~CSyReactorWin32Message();

    // interface ISyReactor
    virtual SyResult Open();

    virtual SyResult NotifyHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask);

    virtual SyResult RunEventLoop();

    virtual SyResult StopEventLoop();

    virtual SyResult Close();

protected:
    virtual SyResult OnHandleRegister(SY_HANDLE aFd,
                                      ASyEventHandler::MASK aMask, ASyEventHandler *aEh);
    virtual void OnHandleRemoved(SY_HANDLE aFd);

    static LRESULT CALLBACK Win32SocketWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwndNotify;
    UINT_PTR m_dwTimerId;

#ifdef SY_ENABLE_CALENDAR_TIMER
    // interface ISyTimerQueue
    virtual SyResult ScheduleTimer(ISyTimerHandler *aTh,
                                   LPVOID aArg,
                                   const CSyTimeValue &aInterval,
                                   DWORD aCount);

    virtual SyResult CancelTimer(ISyTimerHandler *aTh);

    CSyTimerQueueCalendar m_CalendarTimer;
#endif // SY_ENABLE_CALENDAR_TIMER
};

class CSyReactorWin32AsyncSelect : public CSyReactorWin32Message
{
public:
    CSyReactorWin32AsyncSelect();
    virtual ~CSyReactorWin32AsyncSelect();

protected:
    virtual SyResult OnHandleRegister(SY_HANDLE aFd,
                                      ASyEventHandler::MASK aMask, ASyEventHandler *aEh);
    virtual void OnHandleRemoved(SY_HANDLE aFd);

private:
    SyResult DoAsyncSelect_i(SY_HANDLE aFd, ASyEventHandler::MASK aMask);
};

#endif //SUPPORT_REACTOR

#endif // !SYREACTORWIN32MESSAGE_H
