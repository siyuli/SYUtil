/*
 *  SyThreadIOSEventPatch.h
 *  
 *
 *  Created by Soya Li on 3/31/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _CSYTHREAD_ISO_EVENTPATCH__
#define _CSYTHREAD_ISO_EVENTPATCH__

#include "SyThread.h"
#include "SyEventQueueBase.h"
#include "SytimerWrapperID.h"
#include "SyUtilMac.h"

class CSyTimerQueueBase;

class CSyThreadIOSEventPatch : public ASyThread
{
public:
    CSyThreadIOSEventPatch();
    virtual ~CSyThreadIOSEventPatch();
    CSyThreadIOSEventPatch(const CSyThreadIOSEventPatch &);
    CSyThreadIOSEventPatch &operator=(const CSyThreadIOSEventPatch &);

    // interface ASyThread
    virtual SyResult Stop(CSyTimeValue *aTimeout = NULL);
    virtual void OnThreadInit();
    virtual void OnThreadRun();
    virtual ISyEventQueue *GetEventQueue();
    virtual ISyTimerQueue *GetTimerQueue();

    void OnTimer(CFRunLoopTimerRef aId);

    SyResult DoHeartBeat();

protected:
    CSyEventQueueUsingMutex m_EventQueue;
    CSyTimerQueueBase *m_pTimerQueue;

private:
    SyResult KillTimer();
    SyResult SetTimer(uint32_t intervalMS);
    static void timer_callback(CFRunLoopTimerRef timer, void *info)
    {
        CSyThreadIOSEventPatch *proc = (CSyThreadIOSEventPatch *)info;
        proc->OnTimer(timer);
    }

private:
    CFRunLoopTimerRef m_tmID;
};

#endif
