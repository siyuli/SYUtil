/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Webex Application Module For Macintosh Platform                         */
/*                                                                         */
/* SyThreadMacEventPatch.cp                                                    */
/*                                                                         */
/*  Author : Weir Lou (weirl@hz.webex.com)                             */
/*  History                                                                */
/*        1/27/2005, Weir Lou                                          */
/*                                                                         */
/* Copyright (C) WebEx Inc.                                                */
/* All rights reserved                                                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/

//#include "SyBase.h"
#include "SyThreadMacEventPatch.h"
#include "SyTimerQueueOrderedList.h"

#include "SyDebug.h"

USE_UTIL_NS

#if 0
    const CFStringRef   kCFWBXMMPObserveName                = CFSTR("WBXMMP_Observe");
    const CFStringRef   kCFWBXMMPKeyName                    = CFSTR("WBXMMP_KeyName");
    const int           kObserverID                         = 100;
#endif
CFStringRef     g_mmpKeyName = NULL;

pascal void ThreadMacEventPatchTimerProc(CFRunLoopTimerRef inTimer, void *inUserData);
pascal void ThreadMacEventPatchTimerProc(CFRunLoopTimerRef inTimer, void *inUserData)
{
    CSyThreadMacEventPatch *pThreadMacEventPatch = (CSyThreadMacEventPatch *)inUserData;
    if (NULL == pThreadMacEventPatch) {
        return;
    }

    pThreadMacEventPatch->ProcessTimer();
    return;
}

#if 0
void myCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo)
{
    unsigned long parameter = 0;
    CFRange range ;

    CFDataRef bufferRef = (CFDataRef)CFDictionaryGetValue(userInfo, g_mmpKeyName);
    range = CFRangeMake(0,4);
    CFDataGetBytes(bufferRef,range,(unsigned char *)(&parameter));

    CSyThreadMacEventPatch *pThreadMacEventPatch = (CSyThreadMacEventPatch *)parameter;
    pThreadMacEventPatch->ProcessEventQueue();
}
#endif
// ------------------------------------------------------------------------
// TpMacSocketMessageHandler
// ------------------------------------------------------------------------
// our deferred call back
pascal OSStatus
TpMacSocketMessageHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void *inUserData)
{
    OSStatus            status = noErr;
    EventParamType  sanityCheck = 0;
    int                *voidRef = nil;

    status = GetEventParameter(inEvent,  //EventRef inEvent
                               kParamNameTpSocketReference, //EventParamName inName
                               kParamTypeTpSocketReference,     //EventParamType inDesiredType
                               &sanityCheck,                    //EventParamType * outActualType /* can be NULL */
                               sizeof(voidRef),             //UInt32 inBufferSize
                               NULL,                        //UInt32 * outActualSize /* can be NULL */
                               &voidRef);                   //void * outData
    if (0 != status) {
        return status;
    }

    if (kParamTypeTpSocketReference != sanityCheck) {
        return 0;    // !!! should return error?
    }

    CSyThreadMacEventPatch *pThreadMacEventPatch = (CSyThreadMacEventPatch *)inUserData;
    pThreadMacEventPatch->ProcessEventQueue();

    return noErr;
}


CSyThreadMacEventPatch::CSyThreadMacEventPatch()
    : m_pTimerQueue(NULL)
    , m_bStopped(FALSE)
{
    m_nTimer = NULL;
    m_TimerFrequency = (kEventDurationMillisecond * ((EventTime)10.0));
    m_handlerUPP = NewEventHandlerUPP(TpMacSocketMessageHandler);
    m_TpEventHandlerRef = NULL;
#if 0
    ProcessSerialNumber thePSN;
    thePSN.highLongOfPSN = 0;
    thePSN.lowLongOfPSN = kCurrentProcess;
    OSErr   theErr;
    pid_t   thePID;
    theErr = GetProcessPID(&thePSN, &thePID);
    m_nObserverID = thePID;
    char cMMPObserveName[255];
    sprintf(cMMPObserveName,"WBXMMP_Observe_%d",m_nObserverID);
    m_cfMMPObserveName = ::CFStringCreateWithCString(NULL,cMMPObserveName, CFStringGetSystemEncoding());
    char cMMPKeyName[255];
    sprintf(cMMPKeyName,"WBXMMP_KeyName_%d",m_nObserverID);
    g_mmpKeyName = ::CFStringCreateWithCString(NULL,cMMPKeyName, CFStringGetSystemEncoding());
#endif
}

CSyThreadMacEventPatch::~CSyThreadMacEventPatch()
{
    SY_INFO_TRACE_THIS("CSyThreadMacEventPatch::~CSyThreadMacEventPatch()");
    if (m_nTimer) {
        CFRunLoopTimerInvalidate(m_nTimer);
        CFRelease(m_nTimer);
        m_nTimer = NULL;
    }
    if (m_TpEventHandlerRef) {
        RemoveEventHandler(m_TpEventHandlerRef);
        m_TpEventHandlerRef = NULL;
    }
    if (m_handlerUPP) {
        DisposeEventHandlerUPP(m_handlerUPP);
        m_handlerUPP = NULL;
    }
#if 0
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
    CFNotificationCenterRemoveObserver(center, &m_nObserverID, m_cfMMPObserveName,NULL);
    if (m_cfMMPObserveName) {
        ::CFRelease(m_cfMMPObserveName);
        m_cfMMPObserveName = NULL;
    }
    if (g_mmpKeyName) {
        ::CFRelease(g_mmpKeyName);
        g_mmpKeyName = NULL;
    }
#endif
}

void CSyThreadMacEventPatch::OnThreadInit()
{
    Reset2CurrentThreadInfo();

    SY_ASSERTE(!m_pTimerQueue);
    m_pTimerQueue = CSyTimerQueueBase::CreateTimerQueue(NULL);
#if 0
    EventTypeSpec       eventType;
    eventType.eventClass = kEventClassWebExTpEvent;
    eventType.eventKind = kEventKindWebExTpEvent;

    if (!m_TpEventHandlerRef) {
        InstallEventHandler(
            GetApplicationEventTarget(),        // EventTargetRef inTarget
            m_handlerUPP,                       // EventHandlerUPP inHandler
            1,                              // UInt32 inNumTypes
            &eventType,                     // const EventTypeSpec * inList
            this,                               // void *                 inUserData,
            &m_TpEventHandlerRef);                          // EventHandlerRef * outRef
    }
#endif
    if (!m_nTimer) {
        CFRunLoopTimerContext context = { 0, this, NULL, NULL, NULL };
        m_nTimer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(),
                                        m_TimerFrequency, 0, 0,
                                        ThreadMacEventPatchTimerProc,
                                        &context);
        if (m_nTimer) {
            CFRunLoopAddTimer(CFRunLoopGetCurrent(),
                              m_nTimer,
                              kCFRunLoopCommonModes);
        }
    }
#if 0
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
    CFNotificationCenterAddObserver(center, &m_nObserverID, myCallback,
                                    m_cfMMPObserveName,NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
#endif
}

void CSyThreadMacEventPatch::OnThreadRun()
{
#ifdef __LP64__
    CFRunLoopRun();
#else
    RunApplicationEventLoop();
#endif
}

SyResult CSyThreadMacEventPatch::Stop(CSyTimeValue *aTimeout)
{
    SY_INFO_TRACE_THIS("CSyThreadMacEventPatch::Stop");
    if (m_nTimer) {
        CFRunLoopTimerInvalidate(m_nTimer);
        CFRelease(m_nTimer);
        m_nTimer = NULL;
    }
    // todo, let RunApplicationEventLoop() return.
#ifdef __LP64__
    CFRunLoopStop(CFRunLoopGetCurrent());
#else
    QuitApplicationEventLoop();
#endif

    return SY_OK;
}

ISyEventQueue *CSyThreadMacEventPatch::GetEventQueue()
{
    return this;
}

ISyTimerQueue *CSyThreadMacEventPatch::GetTimerQueue()
{
    return m_pTimerQueue;
}

SyResult CSyThreadMacEventPatch::PostEvent(ISyEvent *aEvent, EPriority aPri)
{
    SyResult rv = CSyEventQueueUsingMutex::PostEvent(aEvent, aPri);
    if (SY_FAILED(rv)) {
        return rv;
    }

    // notify main thread.
    TpMacSocketMessage();

    return SY_OK;
}

SyResult CSyThreadMacEventPatch::ProcessEventQueue()
{
    DWORD dwRemainSize = 0;
    CSyEventQueueBase::EventsType tmpListEvents_ProcessEventQueue;
    SyResult rv = CSyEventQueueUsingMutex::PopPendingEventsWithoutWait(
                      tmpListEvents_ProcessEventQueue, 1000/*CSyEventQueueBase::MAX_GET_ONCE*/, &dwRemainSize);


    if (SY_SUCCEEDED(rv)) {
        rv = CSyEventQueueUsingMutex::ProcessEvents(tmpListEvents_ProcessEventQueue);
    }
    if (dwRemainSize) {
        //  TpMacSocketMessage();
        //  DebugStr("\pCSyThreadMacEventPatch::ProcessEventQueue dwRemainSize>0");
        SY_INFO_TRACE_THIS("CSyThreadMacEventPatch::ProcessEventQueue dwRemainSize = "<<dwRemainSize);
    }
    return rv;
}

SyResult CSyThreadMacEventPatch::ProcessTimer()
{
    if (m_pTimerQueue) {
        m_pTimerQueue->CheckExpire();
    }
    //
    ProcessEventQueue();
    //
    return SY_OK;
}


// ------------------------------------------------------------------------
// TpMacSocketMessage
// ------------------------------------------------------------------------
//
void
CSyThreadMacEventPatch::TpMacSocketMessage()
{
#if 0
    EventRef theTpSocketEvent = NULL;

    try {
        OSStatus err;

        err = CreateEvent(NULL,                         //CFAllocatorRef inAllocator
                          kEventClassWebExTpEvent,      //UInt32 inClassID
                          kEventKindWebExTpEvent,   //UInt32 kind
                          (EventTime)0.0,               //EventTime when
                          kEventAttributeUserEvent,     //EventAttributes flags
                          &theTpSocketEvent);   //EventRef* outEvent


        err = SetEventParameter(theTpSocketEvent,   // EventRef inEvent
                                kParamNameTpSocketReference,    // EventParamName   inName
                                kParamTypeTpSocketReference,    // EventParamType inType
                                sizeof(void *),             // UInt32 inSize
                                this);                      // const void * inDataPtr

        err = PostEventToQueue(GetMainEventQueue(),
                               theTpSocketEvent,
                               kEventPriorityLow);

    } catch (...) {
        if (NULL != theTpSocketEvent) {
            ReleaseEvent(theTpSocketEvent);
        }
    }
#endif
#if 0
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();

    unsigned long parameter = (unsigned long)this;

    CFMutableDictionaryRef stockInfoDict = CFDictionaryCreateMutable(NULL, 4,
                                           &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFDataRef parameterRef = CFDataCreate(NULL,(UInt8 *)&parameter,4);
    CFDictionaryAddValue(stockInfoDict, g_mmpKeyName, parameterRef);
    CFRelease(parameterRef);

    CFNotificationCenterPostNotification(center, m_cfMMPObserveName, NULL, stockInfoDict, FALSE);
    CFRelease(stockInfoDict);
#endif
}


