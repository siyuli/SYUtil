#include "SyAssert.h"
#include "SyTimerQueueBase.h"
#include "SyObserver.h"

#ifdef SY_WIN32
    #include "SyDebug.h"
#endif

USE_UTIL_NS

CSyTimerQueueBase::CSyTimerQueueBase(ISyObserver *aObserver)
    : m_pObserver(aObserver)
{
}

CSyTimerQueueBase::~CSyTimerQueueBase()
{
}

int CSyTimerQueueBase::CheckExpire(CSyTimeValue *aRemainTime, CSyTimeValue *aEarliest)
{
    int nCout = 0;
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();

    for (; ;) {
        ISyTimerHandler *pEh = NULL;
        LPVOID pToken = NULL;
        {
            CSyTimeValue tvEarliest;
            CSyMutexGuardT<MutexType> theGuard(m_Mutex);
            int nRet = GetEarliestTime_l(tvEarliest);
            if (nRet == -1) {
                if (aRemainTime) {
                    *aRemainTime = CSyTimeValue::get_tvMax();
                }
                if(aEarliest) {
                    *aEarliest = CSyTimeValue::get_tvMax();
                }
                break;
            } else if (tvEarliest > tvCur) {
                if (aRemainTime) {
                    tvCur = CSyTimeValue::GetTimeOfDay();
                    *aRemainTime = tvEarliest > tvCur ? tvEarliest - tvCur : CSyTimeValue(0, 0);
                }
                if(aEarliest) {
                    *aEarliest = tvEarliest;
                }

                break;
            }

            CNode ndFirst;
            nRet = PopFirstNode_l(ndFirst);
            SY_ASSERTE(nRet == 0);
			if (nRet != 0) {
				break;
			}

            pEh = ndFirst.m_pEh;
            pToken = ndFirst.m_pToken;
            if (ndFirst.m_dwCount != (DWORD)-1) {
                ndFirst.m_dwCount--;
            }

            if (ndFirst.m_dwCount > 0 && ndFirst.m_tvInterval > CSyTimeValue::get_tvZero()) {
                //SY_INFO_TRACE_THIS("CSyTimerQueueBase::CheckExpire, before the time gap is tvCur= " << tvCur.GetTotalInMsec() << ", ndFirst.m_tvExpired = " << ndFirst.m_tvExpired.GetTotalInMsec());
                //Jikky if this device change date and time, it spends more time for do{while}, so simple it.
                CSyTimeValue tHour(3600, 0);
                if ((tHour + ndFirst.m_tvExpired) < tvCur) {
                    ndFirst.m_tvExpired = tvCur + ndFirst.m_tvInterval;
                }
                else {
                    do {
                        ndFirst.m_tvExpired += ndFirst.m_tvInterval;
                    } while (ndFirst.m_tvExpired <= tvCur);
                }
                //SY_INFO_TRACE_THIS("CSyTimerQueueBase::CheckExpire, after the time gap is tvCur= " << tvCur.GetTotalInMsec() << ", ndFirst.m_tvExpired = " << ndFirst.m_tvExpired.GetTotalInMsec());
                RePushNode_l(ndFirst);
            }
        }

        SY_ASSERTE(pEh);
		if (!pEh) {
			continue;
		}
        pEh->OnTimeout(tvCur, pToken);
        nCout++;
    }

    return nCout;
}

SyResult CSyTimerQueueBase::
ScheduleTimer(ISyTimerHandler *aEh, LPVOID aToken,
              const CSyTimeValue &aInterval, DWORD aCount)
{
    SY_ASSERTE_RETURN(aEh, SY_ERROR_INVALID_ARG);
    SY_ASSERTE_RETURN(aInterval > CSyTimeValue::get_tvZero() || aCount == 1, SY_ERROR_INVALID_ARG);

    int nRet;
    BOOL bNeedNotify = FALSE;
    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);
        CSyTimeValue tvTmp;
        if (m_pObserver) {
            bNeedNotify = GetEarliestTime_l(tvTmp) == -1 ? TRUE : FALSE;
        }

        CNode ndNew(aEh, aToken);
        ndNew.m_tvInterval = aInterval;
        ndNew.m_tvExpired = CSyTimeValue::GetTimeOfDay() + aInterval;
        if (aCount > 0) {
            ndNew.m_dwCount = aCount;
        } else {
            ndNew.m_dwCount = (DWORD)-1;
        }

        // PushNode_l() will check the same ISyTimerHandler in the queue
        nRet = PushNode_l(ndNew);
    }

    if (bNeedNotify) {
        SY_ASSERTE(m_pObserver);
        //      SY_INFO_TRACE_THIS("CSyTimerQueueBase::ScheduleTimer, time queue is empty, Notify it.");
        m_pObserver->OnObserve("TimerQueue");
    }

    // Timer queue is in the own thread, need not notify it.
    if (nRet == 0) {
        return SY_OK;
    } else if (nRet == 1) {
        return SY_ERROR_FOUND;
    } else {
        return SY_ERROR_FAILURE;
    }
}

SyResult CSyTimerQueueBase::CancelTimer(ISyTimerHandler *aEh)
{
    SY_ASSERTE_RETURN(aEh, SY_ERROR_INVALID_ARG);
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);

    int nRet = EraseNode_l(aEh);
    if (nRet == 0) {
        return SY_OK;
    } else if (nRet == 1) {
        return SY_ERROR_NOT_FOUND;
    } else {
        return SY_ERROR_FAILURE;
    }
}

CSyTimeValue CSyTimerQueueBase::GetEarliestTime()
{
    CSyTimeValue tvRet;
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);

    int nRet = GetEarliestTime_l(tvRet);
    if (nRet == 0) {
        return tvRet;
    } else {
        return CSyTimeValue::get_tvMax();
    }
}

#include "SyTimerQueueOrderedList.h"

#ifdef SY_LINUX_SERVER
SyQueueType m_defaultQueueType = QueueType_MinHeap;
#else
SyQueueType m_defaultQueueType = QueueType_OrderedList;
#endif

void cm_set_default_timer_type(SyQueueType type)
{
    if (type != QueueType_DefaultTimer)
        m_defaultQueueType = type;
}

CSyTimerQueueBase* CSyTimerQueueBase::CreateTimerQueue(ISyObserver *aObserver, SyQueueType type)
{
    if (type == QueueType_DefaultTimer)
        type = m_defaultQueueType;
    
    if (type == QueueType_OrderedList) {
        return new CSyTimerQueueOrderedList(aObserver);
    } else if (type == QueueType_MinHeap) {
        return new CSyTimerMinHeapQueue(aObserver);
    } else if (type == QueueType_Wheel) {
        return new CSyTimerWheelQueue(aObserver);
    }
    return nullptr;
}
