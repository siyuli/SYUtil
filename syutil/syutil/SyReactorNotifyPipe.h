#ifndef SYREACTORNOTIFYPIPE_H
#define SYREACTORNOTIFYPIPE_H

#ifdef SUPPORT_REACTOR

//#ifndef SY_LINUX
//  #error ERROR: ReactorNotifyPipe only supports LINUX now!
//#endif // SY_LINUX

#include "SyPipe.h"
#include "SyReactorInterface.h"

START_UTIL_NS

class CSyReactorBase;

class CSyReactorNotifyPipe : public ASyEventHandler
{
public:
    CSyReactorNotifyPipe();
    virtual ~CSyReactorNotifyPipe();

    SyResult Open(CSyReactorBase *aReactor);
    SyResult Close();
    SyResult ReOpen();

    // interface ASyEventHandler
    virtual SY_HANDLE GetHandle() const ;
    virtual int OnInput(SY_HANDLE aFd = SY_INVALID_HANDLE);

    SyResult Notify(ASyEventHandler *aEh, ASyEventHandler::MASK aMask);
    bool IsReconnectFail() { return bReconnectFail;}

private:
    struct CBuffer {
        CBuffer(SY_HANDLE aFd = SY_INVALID_HANDLE,
                ASyEventHandler::MASK aMask = ASyEventHandler::NULL_MASK)
            : m_Fd(aFd), m_Mask(aMask)
        {
        }

        SY_HANDLE m_Fd;
        ASyEventHandler::MASK m_Mask;
    };

    CSyPipe m_PipeNotify;
    CSyReactorBase *m_pReactor;
    bool bReconnectFail;
};

#endif //SUPPORT_REACTOR

END_UTIL_NS

#endif // !SYREACTORNOTIFYPIPE_H
