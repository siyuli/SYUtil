#ifndef SYTIMERQUEUEORDEREDLIST_H
#define SYTIMERQUEUEORDEREDLIST_H

#include "SyTimerQueueBase.h"
#include <list>
#include <queue>

using namespace std;

START_UTIL_NS

class SY_OS_EXPORT CSyTimerQueueOrderedList : public CSyTimerQueueBase
{
public:
    CSyTimerQueueOrderedList(ISyObserver *aObserver);
    virtual ~CSyTimerQueueOrderedList();

protected:
    virtual int PushNode_l(const CNode &aPushNode);
    virtual int EraseNode_l(ISyTimerHandler *aEh);
    virtual int RePushNode_l(const CNode &aPushNode);
    virtual int PopFirstNode_l(CNode &aPopNode);
    virtual int GetEarliestTime_l(CSyTimeValue &aEarliest) const;

protected:
    //Supress copy constructor and operator=
    CSyTimerQueueOrderedList(const CSyTimerQueueOrderedList &rhs);
    CSyTimerQueueOrderedList &operator = (const CSyTimerQueueOrderedList &rhs);

    int EnsureSorted();

    typedef std::list<CNode> NodesType;
    NodesType m_Nodes;
};

struct CNode_Comparator {
    bool operator()(const CSyTimerQueueBase::CNode& a, const CSyTimerQueueBase::CNode& b) {
        return a.m_tvExpired > b.m_tvExpired;
    }
};

class SY_OS_EXPORT CSyTimerMinHeapQueue : public CSyTimerQueueBase
{
public:
    CSyTimerMinHeapQueue(ISyObserver *aObserver) : CSyTimerQueueBase(aObserver) {}
    virtual ~CSyTimerMinHeapQueue() {}

    class MinHeap : public std::priority_queue<CNode, std::vector<CNode>, CNode_Comparator>
    {
    public:
        bool remove(ISyTimerHandler* eh) {
            auto it = c.begin();
            for (; it != c.end(); it++) {
                if (it->m_pEh == eh) {
                    break;
                }
            }
            if (it != this->c.end()) {
                c.erase(it);
                std::make_heap(c.begin(), c.end(), comp);
                return true;
            }
            else {
                return false;
            }
        }
        
    };

protected:
    
    virtual int PushNode_l(const CNode &aPushNode);
    virtual int EraseNode_l(ISyTimerHandler *aEh);
    virtual int RePushNode_l(const CNode &aPushNode);
    virtual int PopFirstNode_l(CNode &aPopNode);
    virtual int GetEarliestTime_l(CSyTimeValue &aEarliest) const;

    //Supress copy constructor
    CSyTimerMinHeapQueue(const CSyTimerMinHeapQueue &rhs);
    CSyTimerMinHeapQueue &operator = (const CSyTimerMinHeapQueue &rhs);

    MinHeap m_nodes;
};

/**
 * it is appropriate for short-term timers. The default wheel is set to 3000 for 10ms slot.
 * For longer timer, we will store them into an ordered list in another slot. (For short term it is unordered list)
 * To be compatible with the TP timer, we will reiterate the wheel in Erase and Pop function to find the next timer to schedule.
 * To remove element quickly we added an ID into the ISyTimerHandler
 * The memory usage for this queue is way bigger than others, but still reasonable.
 */
class SY_OS_EXPORT CSyTimerWheelQueue : public CSyTimerQueueBase
{
public:
    CSyTimerWheelQueue(ISyObserver *aObserver) : CSyTimerQueueBase(aObserver) {
        m_aEarliest = CSyTimeValue::get_tvMax();
    }
    
    virtual ~CSyTimerWheelQueue() {
    }
    
protected:
    virtual int PushNode_l(const CNode &aPushNode);
    virtual int EraseNode_l(ISyTimerHandler *aEh);
    virtual int RePushNode_l(const CNode &aPushNode);
    virtual int PopFirstNode_l(CNode &aPopNode);
    virtual int GetEarliestTime_l(CSyTimeValue &aEarliest) const;

    const static int m_maxBuckets = 3000;  //30s
    const static int m_slot = 10;          //10ms slot
    typedef std::list<CNode> NodesType;
    NodesType m_Nodes[m_maxBuckets];
    NodesType m_NodesFar[m_maxBuckets];
    CSyTimeValue m_aEarliest;
    
    bool remove(NodesType& list, ISyTimerHandler *aEh, CNode &node);
    int pushNode(const CNode &aPushNode);
    int scheduleNext(int j);

    //Supress copy constructor
    CSyTimerWheelQueue(const CSyTimerWheelQueue &rhs);
    CSyTimerWheelQueue &operator = (const CSyTimerWheelQueue &rhs);
};

END_UTIL_NS

#endif // !SYTIMERQUEUEORDEREDLIST_H
