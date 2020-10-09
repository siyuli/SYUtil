#ifndef SYREACTORSELECT_H
#define SYREACTORSELECT_H

#ifdef SUPPORT_REACTOR

#include "SyReactorBase.h"
#include "SyReactorNotifyPipe.h"

START_UTIL_NS

class CSyReactorSelect : public CSyReactorBase
{
public:
    CSyReactorSelect();
    virtual ~CSyReactorSelect();

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

    void ProcessFdSets_i(
        fd_set &aFdSet,
        ASyEventHandler::MASK aMask,
        int &aActiveNumber,
        int aMaxFd);

private:
    CSyReactorNotifyPipe m_Notify;
    CSyMutexThreadRecursive m_NeedCloseNotifyMutex;
    BOOL                    m_bNeedCloseNotify;
#ifdef SY_WIN32
    bool m_bNeedNotify;
#endif
};

#endif //SUPPORT_REACTOR

END_UTIL_NS

#endif // !SYREACTORSELECT_H
