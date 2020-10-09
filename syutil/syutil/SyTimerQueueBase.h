#ifndef SYTIMERQUEUEBASE_H
#define SYTIMERQUEUEBASE_H

#include "SyMutex.h"
#include "SyTimeValue.h"
#include "SyThreadInterface.h"

START_UTIL_NS

class ISyObserver;

class SY_OS_EXPORT CSyTimerQueueBase : public ISyTimerQueue
{
public:
    struct CNode {
        CNode(ISyTimerHandler *aEh = NULL, LPVOID aToken = NULL)
            : m_pEh(aEh), m_pToken(aToken), m_dwCount(0)
        {
        }

        ISyTimerHandler *m_pEh;
        LPVOID m_pToken;
        CSyTimeValue m_tvExpired;
        CSyTimeValue m_tvInterval;
        DWORD m_dwCount;
    };

    CSyTimerQueueBase(ISyObserver *aObserver);
    virtual ~CSyTimerQueueBase();

    // interface ISyTimerQueue
    virtual SyResult ScheduleTimer(ISyTimerHandler *aEh,
                                   LPVOID aToken,
                                   const CSyTimeValue &aInterval,
                                   DWORD aCount);

    virtual SyResult CancelTimer(ISyTimerHandler *aEh);

    /// if the queue is empty, return CSyTimeValue::s_tvMax.
    CSyTimeValue GetEarliestTime();

    /// return the number of timer expired.
    /// and fill <aRemainTime> with the sub-value of earliest time and current time,
    /// if no timer items in the queue, fill <aRemainTime> with <CSyTimeValue::s_tvMax>.
    virtual int CheckExpire(CSyTimeValue *aRemainTime = NULL, CSyTimeValue *aEarliest = NULL);
    
    static CSyTimerQueueBase* CreateTimerQueue(ISyObserver *aObserver, SyQueueType type = QueueType_DefaultTimer);

protected:
    /// the sub-classes of CSyTimerQueueBase always use STL contains that
    /// we just let them manage the memery allocation of CNode

    /// the following motheds are all called after locked
    virtual int PushNode_l(const CNode &aPushNode) = 0;
    virtual int EraseNode_l(ISyTimerHandler *aEh) = 0;
    virtual int RePushNode_l(const CNode &aPushNode) = 0;
    virtual int PopFirstNode_l(CNode &aPopNode) = 0;
    virtual int GetEarliestTime_l(CSyTimeValue &aEarliest) const = 0;

    typedef CSyMutexNullSingleThread MutexType;
    MutexType m_Mutex;
    ISyObserver *m_pObserver;

    //Supress copy constructor and operator=
    CSyTimerQueueBase(const CSyTimerQueueBase &rhs);
    CSyTimerQueueBase &operator = (const CSyTimerQueueBase &rhs);
};

END_UTIL_NS

#endif // !SYTIMERQUEUEBASE_H
