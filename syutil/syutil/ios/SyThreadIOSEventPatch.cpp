/*
 *  SyThreadIOSEventPatch.cpp
 *  mm_tp
 *
 *  Created by Soya Li on 3/31/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "SyThreadIOSEventPatch.h"
#include "SyTimerQueueOrderedList.h"
#include "SyAssert.h"
#include "SyDebug.h"

CSyThreadIOSEventPatch::CSyThreadIOSEventPatch()
    : m_pTimerQueue(NULL)
{
}

CSyThreadIOSEventPatch::~CSyThreadIOSEventPatch()
{
    KillTimer();
    delete m_pTimerQueue;
}

void CSyThreadIOSEventPatch::OnThreadInit()
{
    // have to new timerqueue in the task thread.
    SY_ASSERTE(!m_pTimerQueue);
    m_pTimerQueue = CSyTimerQueueBase::CreateTimerQueue(nullptr);
    SY_ASSERTE(m_pTimerQueue);

    m_EventQueue.Reset2CurrentThreadInfo();

    SetTimer(100);
}

ISyEventQueue *CSyThreadIOSEventPatch::GetEventQueue()
{
    return &m_EventQueue;
}

ISyTimerQueue *CSyThreadIOSEventPatch::GetTimerQueue()
{
    return m_pTimerQueue;
}

void CSyThreadIOSEventPatch::OnThreadRun()
{
    DoHeartBeat();
}

SyResult CSyThreadIOSEventPatch::Stop(CSyTimeValue *aTimeout)
{
    KillTimer();
    return SY_OK;
}

SyResult CSyThreadIOSEventPatch::DoHeartBeat()
{
    if (m_pTimerQueue) {
        m_pTimerQueue->CheckExpire();
    }

    CSyEventQueueBase::EventsType listEvents;
    SyResult rv = m_EventQueue.PopPendingEventsWithoutWait(listEvents, (DWORD)-1);
    if (SY_SUCCEEDED(rv)) {
        m_EventQueue.ProcessEvents(listEvents);
    }

    return SY_OK;
}

void CSyThreadIOSEventPatch::OnTimer(CFRunLoopTimerRef aId)
{
    if (aId == m_tmID) {
        OnThreadRun();
    }
}

SyResult CSyThreadIOSEventPatch::KillTimer()
{
    if (m_tmID) {
        SY_INFO_TRACE_THIS("CSyThreadIOSEventPatch::KillTimer: Killing timer = " << m_tmID);
        CFRunLoopTimerInvalidate(m_tmID);
        CFRelease(m_tmID);
        m_tmID = NULL;
    }
    return SY_OK;
}

SyResult CSyThreadIOSEventPatch::SetTimer(uint32_t intervalMS)
{
    //SY_ASSERTE( !m_tmID );
    CFRunLoopTimerContext context = { 0, this, NULL, NULL, NULL };
    CFTimeInterval ti = 0.001 * (CFTimeInterval)intervalMS;
    CFAbsoluteTime fireDate = CFAbsoluteTimeGetCurrent() + ti;// + (1.0e-3 * timeout);
    m_tmID = CFRunLoopTimerCreate(NULL, fireDate, ti, 0, 0, timer_callback, &context);
    SY_INFO_TRACE_THIS("AT::CTimerID::SetTimer: Setting timer = " << m_tmID << " with interval = " << intervalMS);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), m_tmID, kCFRunLoopCommonModes);
    return SY_OK;
}
