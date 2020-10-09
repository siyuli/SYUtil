#ifndef SYTHREAD_H
#define SYTHREAD_H

#include "SyAtomicOperationT.h"
#include "SyThreadMisc.h"
#include <string>
#include <map>
#include "SyThreadInterface.h"
#include "SyDebug.h"

START_UTIL_NS

class ISyEventQueue;
class ISyTimerQueue;
class ISyReactor;
class CSyEventThread;
class CSyTimeValue;



class SY_OS_EXPORT ASyThread
{
public:
    SY_THREAD_ID GetThreadId();
#if defined (WP8 ) || defined (UWP)
    void SetThreadId(unsigned long id) { m_Tid = id; }
#endif
    TType GetThreadType();
    SY_THREAD_HANDLE GetThreadHandle();
    void Terminate();

    // Create thread.
    // The function won't return until OnThreadInit() returns.
    virtual SyResult Create(
        const char *name,
        TType aType,
        TFlag aFlag, BOOL Register);

    const char *GetName() {return m_name.c_str();};

    // Stop thread so that let the thread function return.
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);

    // Wait until the thread function return.
    SyResult Join();

    // Delete this.
    SyResult Destory(SyResult aReason);

    virtual void OnThreadInit();
    virtual void OnThreadRun() = 0;

    virtual ISyReactor *GetReactor();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

    virtual void SetStop();
    virtual BOOL GetStopFlag();
    virtual bool IsDetached();

protected:
    ASyThread();
    virtual ~ASyThread();

    ASyThread(const ASyThread &);
    ASyThread &operator=(const ASyThread &);

private:
#ifdef SY_WIN32
    static unsigned WINAPI ThreadProc(void *aPara);
#else
    static void *ThreadProc(void *aPara);
#endif // SY_WIN32

#ifdef SY_WIN32
    void SetThreadName();
#endif

protected:
    SY_THREAD_ID m_Tid;
    SY_THREAD_HANDLE m_Handle;
    TType m_Type;
    TFlag m_Flag;
    std::string m_name;
    BOOL m_bStopFlag;


private:
    CSyEventThread *m_pEvent4Start;
    BOOL m_bRegistered;
    BOOL m_bRegister;
    CSyAtomicOperationT<CSyMutexThread> m_NeedDelete;

};

typedef SyResult(*pfn_CreateUserTaskThread)(const char *name, ASyThread *&aThread, TFlag aFlag, BOOL bWithTimerQueue, TType aType);

extern "C" SY_OS_EXPORT SyResult CreateUserTaskThread(const char *name, ASyThread *&aThread, TFlag aFlag = TF_JOINABLE,
        BOOL bWithTimerQueue = TRUE, TType aType = TT_UNKNOWN);

extern "C" SY_OS_EXPORT pfn_CreateUserTaskThread setCreateUserTaskThread(pfn_CreateUserTaskThread fn);


class  SY_OS_EXPORT ASyThreadSingletonFactory
{
public:
    static ASyThreadSingletonFactory &Instance();

    SyResult GetSingletonThread(const char *pThreadName,ASyThread *&aThread);
    void ResleseSingletonThread(const char *pThreadName,ASyThread *aThread);
protected:
    struct ASyThreadSingleton {
        ASyThread *m_pASyThread;
        int m_nRefNum;
    };
protected:
    ASyThreadSingletonFactory();
    ~ASyThreadSingletonFactory();
    CSyMutexThread m_LockForMap;
    std::map<std::string,ASyThreadSingleton> m_mapThreads;
    static ASyThreadSingletonFactory m_pASyThreadSingletonFactory;
};


template <typename Func>
class CSyInvokeEvent : public ISyEvent
{
public:
    CSyInvokeEvent(const Func &func) : m_callback(func) { }
    
    virtual SyResult OnEventFire()
    {
        return m_callback();
    }
    
protected:
    Func m_callback;
};

template <typename Func>
SyResult SyInvokeInThread(ASyThread *pThread, bool bNonblock, const Func &func) {
    SY_ASSERTE_RETURN(pThread != NULL, SY_ERROR_INVALID_ARG);
    ISyEventQueue *pEventQueue = pThread->GetEventQueue();
    SY_ASSERTE_RETURN(pEventQueue != NULL, SY_ERROR_INVALID_ARG);
    
    CSyInvokeEvent<Func> *pEvent = new CSyInvokeEvent<Func>(func);
    
    if (bNonblock) {
        return pEventQueue->PostEvent(pEvent);
    }
    return pEventQueue->SendEvent(pEvent);
}



END_UTIL_NS

#endif // !SYTHREAD_H
