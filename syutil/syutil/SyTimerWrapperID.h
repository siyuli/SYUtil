#ifndef SYTIMERWRAPPERID_H
#define SYTIMERWRAPPERID_H

#include "SyThreadInterface.h"
#include "SyReferenceControl.h"

START_UTIL_NS

class CTimeValue;
class CSyTimerWrapperID;

class SY_OS_EXPORT CSyTimerWrapperIDSink
{
public:
    virtual void OnTimer(CSyTimerWrapperID *aId) = 0;

protected:
    CSyTimerWrapperIDSink();
    virtual ~CSyTimerWrapperIDSink();
};

class CSyTimerWrapperRef;
class SY_OS_EXPORT CSyTimerWrapperID
{
public:
    CSyTimerWrapperID();
    virtual ~CSyTimerWrapperID();

    /// Schedule an timer that will expire after <aInterval> for <aCount> times.
    /// if <aCount> is 0, schedule infinite times.
    SyResult Schedule(
        CSyTimerWrapperIDSink *aSink,
        const CSyTimeValue &aInterval,
        DWORD aCount = 0);

    // For Android, use specific thread to get timer
    SyResult ScheduleInThread(
        TType aType,
        CSyTimerWrapperIDSink *aSink,
        const CSyTimeValue &aInterval,
        DWORD aCount = 0);
    SyResult ScheduleInThread(
        ASyThread *pThread,
        CSyTimerWrapperIDSink *aSink,
        const CSyTimeValue &aInterval,
        DWORD aCount = 0);

    /// Cancel the timer, this parameter is deprecated now, it will never block so it is always false.
    SyResult Cancel(bool bOnlySameThread = false);

protected:    
    friend class CSyTimerWrapperRef;
    virtual void OnTimeout(const CSyTimeValue &aCurTime, LPVOID aArg);

private:
    CSyTimerWrapperID(const CSyTimerWrapperID&);
    const CSyTimerWrapperID& operator=(const CSyTimerWrapperID&);
    
    CSyComAutoPtr<CSyTimerWrapperRef> m_TimerWrapRef;
    CSyMutexThreadRecursive m_mutexCancel;
};

END_UTIL_NS

#endif // !SYTIMERWRAPPERID_H
