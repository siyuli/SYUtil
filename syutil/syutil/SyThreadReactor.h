#ifndef SYTHREADREACTOR_H
#define SYTHREADREACTOR_H

#include "SyThread.h"
#include "SyThreadInterface.h"

START_UTIL_NS

class ISyReactor;

class CSyThreadReactor : public ASyThread
{
    CSyThreadReactor(const CSyThreadReactor &);
    CSyThreadReactor &operator=(const CSyThreadReactor &);
public:
    CSyThreadReactor();
    virtual ~CSyThreadReactor();

    SyResult Init(ISyReactor *aReactor);

    // interface ASyThread
    virtual SyResult Create(const char *name,TType aType, TFlag aFlag = TF_JOINABLE);
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);

    virtual void OnThreadInit();
    virtual void OnThreadRun();

    virtual ISyReactor *GetReactor();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

private:
    ISyReactor *m_pReactor;
};

/// used for netwok thread is the same as main thread.
class CSyThreadDummy : public ASyThread
{
public:
    CSyThreadDummy();
    virtual ~CSyThreadDummy();

    SyResult Init(ASyThread *aThread, /*CSyThreadManager::TType aType*/TType aType);

    // interface ASyThread
    virtual SyResult Create(
        //CSyThreadManager::TType aType,
        //CSyThreadManager::TFlag aFlag = CSyThreadManager::TF_JOINABLE);
        TType aType,
        TFlag aFlag = TF_JOINABLE);

    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);

    virtual void OnThreadRun();

    virtual ISyReactor *GetReactor();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

private:
    ASyThread *m_pActualThread;
};

END_UTIL_NS

#endif // !SYTHREADREACTOR_H
