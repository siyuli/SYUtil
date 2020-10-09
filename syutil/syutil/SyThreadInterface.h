#ifndef SYTHREADINTERFACE_H
#define SYTHREADINTERFACE_H

#include "SyDataType.h"
#include "SyTimeValue.h"
#include "SyThreadMisc.h"

START_UTIL_NS

class CSyTimeValue;

//if add event type, append it in the enum
typedef DWORD   EVENT_TYPE;
enum {
    EVENT_UNDEFINED = 0,
    EVENT_TP_BASE = 1,          //1 - 9999 are for TP layer
    EVENT_TCP_ONINPUT,


    EVENT_SESSION_BASE = 10000, //10000 - 19999 are for session layer
    EVENT_SESSION_MMP_SDK = EVENT_SESSION_BASE + 100, //MMP SDK need set this event type

    //for TelephoneDecoder
    EVENT_SESSION_DECODE_START = EVENT_SESSION_BASE + 400,
    EVENT_SESSION_DECODE_STOP  = EVENT_SESSION_BASE + 401,
    EVENT_SESSION_PING_START = EVENT_SESSION_BASE + 402,
    EVENT_SESSION_PING_STOP = EVENT_SESSION_BASE + 403,
    EVENT_SESSION_PING_RESULT = EVENT_SESSION_BASE + 404,
    EVENT_SESSION_PING_DELETE = EVENT_SESSION_BASE + 405,


    ET_TelephoneUserEvent = EVENT_SESSION_BASE + 500,
    ET_CreateDecoderEvent = EVENT_SESSION_BASE + 501,
    ET_TelephonyDataEvent = EVENT_SESSION_BASE + 502,
    ET_TeleDataToUserEvent= EVENT_SESSION_BASE + 503,
    ET_SessionSubInfoEvent= EVENT_SESSION_BASE + 504,

    EVENT_APP_BASE = 20000,     //others are for app layer
};

typedef enum {
    QueueType_DefaultTimer = 0,///< Default timer queue, it is ordered list for client, and min-heap for server
    QueueType_OrderedList = 1, ///< ordered list, it is O(N) when push and O(1) when remove/kick, it is good for client usage
    QueueType_MinHeap = 2,     ///< min-heap queue, it is O(log N) when push and O(log N) when remove/kick.
    QueueType_Wheel = 3        ///< wheel queue, it is O(1) when push and O(1) when remove/kick.
} SyQueueType;

class SY_OS_EXPORT ISyEvent
{
public:
    virtual SyResult OnEventFire() = 0;

    virtual void OnDestorySelf();
    ISyEvent(EVENT_TYPE EventType = EVENT_UNDEFINED);

protected:
    EVENT_TYPE  m_EventType;        //the event type for tracking
    SY_THREAD_ID    m_Tid;          //the thread id which thread create the event
    virtual ~ISyEvent();
};

class SY_OS_EXPORT ISyEventQueue
{
public:
    enum EPriority {
        EPRIORITY_HIGH,
        EPRIORITY_NORMAL,
        EPRIORITY_LOW
    };

    /// this function could be invoked in the different thread.
    /// like PostMessage() in Win32 API.
    virtual SyResult PostEvent(ISyEvent *aEvent, EPriority aPri = EPRIORITY_NORMAL) = 0;

    /// this function could be invoked in the different thread.
    /// like SendMessage() in Win32 API.
    virtual SyResult SendEvent(ISyEvent *aEvent) = 0;

    /// get the number of pending events.
    virtual DWORD GetPendingEventsCount() = 0;

protected:
    virtual ~ISyEventQueue() { }
};

/**
 * @class ISyTimerHandler
 *
 * @brief Provides an abstract interface for handling timer event.
 *
 * Subclasses handle a timer's expiration.
 *
 */
class SY_OS_EXPORT ISyTimerHandler
{
public:
    /**
     * Called when timer expires.  <aCurTime> represents the current
     * time that the <ASyEventHandler> was selected for timeout
     * dispatching and <aArg> is the asynchronous completion token that
     * was passed in when <ScheduleTimer> was invoked.
     * the return value is ignored.
     */
    virtual void OnTimeout(const CSyTimeValue &aCurTime, LPVOID aArg) = 0;

protected:
    ISyTimerHandler();
    virtual ~ISyTimerHandler();
    int m_id = -1;
    friend class CSyTimerWheelQueue;
};

class SY_OS_EXPORT ISyTimerQueue
{
public:
    /**
     * this function must be invoked in the own thread.
     * <aInterval> must be greater than 0.
     * If success:
     *    if <aTh> exists in queue, return SY_ERROR_FOUND;
     *    else return SY_OK;
     */
    virtual SyResult ScheduleTimer(ISyTimerHandler *aTh,
                                   LPVOID aArg,
                                   const CSyTimeValue &aInterval,
                                   DWORD aCount) = 0;

    /**
     * this function must be invoked in the own thread.
     * If success:
     *    if <aTh> exists in queue, return SY_OK;
     *    else return SY_ERROR_NOT_FOUND;
     */
    virtual SyResult CancelTimer(ISyTimerHandler *aTh) = 0;

protected:
    virtual ~ISyTimerQueue() { }
};

extern "C" SY_OS_EXPORT void sy_set_default_timer_type(SyQueueType type);

END_UTIL_NS

#endif // SYTHREADINTERFACE_H
