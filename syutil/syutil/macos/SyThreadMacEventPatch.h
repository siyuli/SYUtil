#ifndef _H_SyThreadMacEventPatch_
#define _H_SyThreadMacEventPatch_
#pragma once

#include "SyThread.h"
#include "SyEventQueueBase.h"

START_UTIL_NS

class CSyTimerQueueBase;
class CSyTimeValue;

// WebEx custom event types
const UINT32 kEventClassWebExTpEvent = 'WeTp';

// WebEx custom event kinds
const UINT32 kEventKindWebExTpEvent = 'EKTP';

// WebEx custom event parameter names
const UINT32 kParamNameTpSocketReference = 'nTpS';
const UINT32 kParamTypeTpSocketReference = 'tTpS';

class CSyThreadMacEventPatch
    : public ASyThread
    , public CSyEventQueueUsingMutex
{
public:
    CSyThreadMacEventPatch();
    ~CSyThreadMacEventPatch();

    // interface ASyThread
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);
    virtual void OnThreadInit();
    virtual void OnThreadRun();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

    // interface ISyEventQueue
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL);

    SyResult ProcessEventQueue();
    SyResult ProcessTimer();

private:
    void TpMacSocketMessage();

private:
    CSyTimerQueueBase *m_pTimerQueue;
    BOOL m_bStopped;

    CFRunLoopTimerRef   m_nTimer;
    EventTime           m_TimerFrequency ;

    EventHandlerUPP     m_handlerUPP;
    EventHandlerRef     m_TpEventHandlerRef;        // cache this and dispose of at app quit

    CFStringRef         m_cfMMPObserveName;
    //  CFStringRef         m_cfMMPKeyName;
    DWORD               m_nObserverID;
};

END_UTIL_NS

#endif //_H_SyThreadMacEventPatch_
