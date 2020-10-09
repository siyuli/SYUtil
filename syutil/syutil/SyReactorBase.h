#ifndef SYREACTORBASE_H
#define SYREACTORBASE_H

#ifdef SUPPORT_REACTOR

#include "SyDebug.h"
#include "SyReactorInterface.h"
#include "SyUtilClasses.h"
#include "SyEventQueueBase.h"
#include "SyObserver.h"

#ifdef SY_WIN32
    #include <map>
#endif // SY_WIN32

#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

#ifndef SY_WIN32
    #include <set>
#endif

START_UTIL_NS

#ifdef _MMP_SPA_
    static const DWORD s_dwDefaultTimerTickInterval = 30; // to improve WebEx node performance
#else
    static const DWORD s_dwDefaultTimerTickInterval = 10; // 10ms
#endif

class CSyReactorBase;
class CSyTimerQueueBase;

class SY_OS_EXPORT CSyEventHandlerRepository
{
public:
    CSyEventHandlerRepository();
    ~CSyEventHandlerRepository();

    SyResult Open();
    SyResult Close();

    struct CElement {
        ASyEventHandler *m_pEh;
        ASyEventHandler::MASK m_Mask;

        CElement(ASyEventHandler *aEh = NULL,
                 ASyEventHandler::MASK aMask = ASyEventHandler::NULL_MASK)
            : m_pEh(aEh), m_Mask(aMask)
        {
        }

        void Clear()
        {
            m_pEh = NULL;
            m_Mask = ASyEventHandler::NULL_MASK;
        }

        BOOL IsCleared() const
        {
            return m_pEh == NULL;
        }
    };

    /**
     * If success:
     *    if <aFd> is found, return SY_OK;
     *    else return SY_ERROR_NOT_FOUND;
     */
    SyResult Find(SY_HANDLE aFd, CElement &aEle);

    /**
     * If success:
     *    if <aFd> is found, return SY_ERROR_FOUND;
     *    else return SY_OK;
     */
    SyResult Bind(SY_HANDLE aFd, const CElement &aEle);

    /**
     * If success:
     *    return SY_OK;
     */
    SyResult UnBind(SY_HANDLE aFd);

    BOOL IsVaildHandle(SY_HANDLE aFd)
    {
#ifdef SY_WIN32
        if (aFd != SY_INVALID_HANDLE)
#else
        if (aFd >= 0 && aFd < m_nMaxHandler)
#endif // SY_WIN32
            return TRUE;
        else {
            return FALSE;
        }
    }

#ifndef SY_WIN32
    int GetMaxHandlers()
    {
        return m_nMaxHandler;
    }

    CElement *GetElement()
    {
        return m_pHandlers;
    }

    SY_HANDLE *GetSyHandles()
    {
        return m_cmHandles;
    }
    int GetSyHandlesCount()
    {
        return m_nSyHandlesCount;
    }
#endif // !SY_WIN32

#ifndef SY_WIN32
    static SyResult SetRlimit(int aResource, int aMaxNum, int &aActualNum);
#endif // !SY_WIN32

    int FillFdSets(fd_set &aFsRead, fd_set &aFsWrite, fd_set &aFsException);
private:
    void FillFdSets_i();
    int ChangeSyHandles();

private:
#ifdef SY_WIN32
    typedef std::map<SY_HANDLE, CElement> HandlersType;
    HandlersType m_Handlers;
#else
    CElement *m_pHandlers;
    int m_nMaxHandler;
    SY_HANDLE *m_cmHandles;///<current socket handles array
    int m_nSyHandlesCount;
    const static int s_nSyHandlesStepCount;
    std::set<SY_HANDLE> m_toBindHandles;
    std::set<SY_HANDLE> m_toUnbindHandles;
    std::set<SY_HANDLE> m_curHandles;
#endif // SY_WIN32
    fd_set m_aFsRead;
    fd_set m_aFsWrite;
    fd_set m_aFsException;
    int m_nMaxFd;
    bool m_cmHandlesChanged;
};


// base class for rector,
// we have to inherit from <CSyEventQueueUsingMutex> because we
// will over write PostEvent() to do NotifyHandler().
class SY_OS_EXPORT CSyReactorBase
    : public ISyReactor
    , public CSyStopFlag
    , public CSyEventQueueUsingMutex
{
public:
    CSyReactorBase(PROPERTY aProperty = NULL_PROPERTY);
    virtual ~CSyReactorBase();

    // interface ISyReactor
    virtual SyResult Open();

    virtual SyResult RegisterHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask);

    virtual SyResult RemoveHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask = ASyEventHandler::ALL_EVENTS_MASK);

    virtual SyResult Close();

    // interface ISyTimerQueue
    virtual SyResult ScheduleTimer(ISyTimerHandler *aTh,
                                   LPVOID aArg,
                                   const CSyTimeValue &aInterval,
                                   DWORD aCount);

    virtual SyResult CancelTimer(ISyTimerHandler *aTh);

    // interface ISyEventQueue
    virtual SyResult SendEvent(ISyEvent *aEvent);
    virtual SyResult PostEvent(
        ISyEvent *aEvent,
        EPriority aPri = ISyReactor::EPRIORITY_NORMAL);
    virtual DWORD GetPendingEventsCount();

    SyResult ProcessHandleEvent(
        SY_HANDLE aFd,
        ASyEventHandler::MASK aMask,
        SyResult aReason,
        BOOL aIsNotify,
        BOOL aDropConnect = FALSE);

protected:
    SyResult ProcessTimerTick();

    virtual void OnHandleRemoved(SY_HANDLE aFd) = 0;
    virtual SyResult OnHandleRegister(
        SY_HANDLE aFd,
        ASyEventHandler::MASK aMask,
        ASyEventHandler *aEh) = 0;

    CSyEnsureSingleThread m_Est;
    CSyTimerQueueBase *m_pTimerQueue;
    BOOL m_bNotifyFailed;
    BOOL m_bNotifyHandle;

private:
    SyResult RemoveHandleWithoutFinding_i(
        SY_HANDLE aFd,
        const CSyEventHandlerRepository::CElement &aHe,
        ASyEventHandler::MASK aMask);

protected:
    CSyEventHandlerRepository m_EhRepository;
};


// inline functions
inline SyResult CSyEventHandlerRepository::Find(SY_HANDLE aFd, CElement &aEle)
{
#ifdef SY_WIN32
    SY_ASSERTE_RETURN(IsVaildHandle(aFd), SY_ERROR_INVALID_ARG);
    HandlersType::iterator iter = m_Handlers.find(aFd);
    if (iter == m_Handlers.end()) {
        return SY_ERROR_NOT_FOUND;
    } else {
        aEle = (*iter).second;
        SY_ASSERTE(!aEle.IsCleared());
        return SY_OK;
    }
#else
    // CAcceptor maybe find fd after closed when program shutting down.
    if (!m_pHandlers) {
        return SY_ERROR_NOT_INITIALIZED;
    }
    SY_ASSERTE_RETURN(IsVaildHandle(aFd), SY_ERROR_INVALID_ARG);

    CElement &eleFind = m_pHandlers[aFd];
    if (eleFind.IsCleared()) {
        return SY_ERROR_NOT_FOUND;
    } else {
        aEle = eleFind;
        return SY_OK;
    }
#endif // SY_WIN32
}

inline SyResult CSyEventHandlerRepository::Bind(SY_HANDLE aFd, const CElement &aEle)
{
    SY_ASSERTE_RETURN(IsVaildHandle(aFd), SY_ERROR_INVALID_ARG);
    SY_ASSERTE_RETURN(!aEle.IsCleared(), SY_ERROR_INVALID_ARG);

#ifdef SY_WIN32
    CElement &eleBind = m_Handlers[aFd];
#else
    SY_ASSERTE_RETURN(m_pHandlers, SY_ERROR_NOT_INITIALIZED);
    CElement &eleBind = m_pHandlers[aFd];
    m_toUnbindHandles.erase(aFd);
    m_toBindHandles.insert(aFd);
#endif // SY_WIN32

    m_cmHandlesChanged = true;
    BOOL bNotBound = eleBind.IsCleared();
    eleBind = aEle;
    return bNotBound ? SY_OK : SY_ERROR_FOUND;
}

inline SyResult CSyEventHandlerRepository::UnBind(SY_HANDLE aFd)
{
    SY_ASSERTE_RETURN(IsVaildHandle(aFd), SY_ERROR_INVALID_ARG);

#ifdef SY_WIN32
    m_Handlers.erase(aFd);
#else
    SY_ASSERTE_RETURN(m_pHandlers, SY_ERROR_NOT_INITIALIZED);
    m_pHandlers[aFd].Clear();
    m_toUnbindHandles.insert(aFd);
    m_toBindHandles.erase(aFd);
#endif // SY_WIN32
    m_cmHandlesChanged = true;
    return SY_OK;
}

inline SyResult CSyEventHandlerRepository::ChangeSyHandles()
{
    if (!m_cmHandlesChanged) {
        return SY_OK;
    }
    m_cmHandlesChanged=false;
#ifndef SY_WIN32
    std::set<SY_HANDLE>::iterator iter;
    for (iter=m_toBindHandles.begin(); m_toBindHandles.end()!=iter; ++iter) {
        SY_HANDLE aFd=*iter;
        if (m_curHandles.end()==m_curHandles.find(aFd)) {
            m_cmHandles[m_nSyHandlesCount]=aFd;
            ++m_nSyHandlesCount;
            m_curHandles.insert(aFd);
        }
    }
    m_toBindHandles.clear();
    for (iter=m_toUnbindHandles.begin(); m_toUnbindHandles.end()!=iter; ++iter) {
        SY_HANDLE aFd=*iter;
        m_curHandles.erase(aFd);
        for (int i=0; m_nSyHandlesCount>i; ++i) {
            if (aFd==m_cmHandles[i]) {
                m_cmHandles[i]=m_cmHandles[m_nSyHandlesCount-1];
                --m_nSyHandlesCount;
                break;
            }
        }
    }
    m_toUnbindHandles.clear();
#endif
    FillFdSets_i();
    return SY_OK;
}
#endif //SUPPORT_REACTOR

END_UTIL_NS

#endif // !SYREACTORBASE_H
