#include "SyAssert.h"
#include "SyTimerQueueOrderedList.h"

#include "SyDebug.h"

USE_UTIL_NS

CSyTimerQueueOrderedList::CSyTimerQueueOrderedList(ISyObserver *aObserver)
    : CSyTimerQueueBase(aObserver)
{
}

CSyTimerQueueOrderedList::~CSyTimerQueueOrderedList()
{
}

int CSyTimerQueueOrderedList::EraseNode_l(ISyTimerHandler *aEh)
{
    NodesType::iterator iter = m_Nodes.begin();
    while (iter != m_Nodes.end()) {
        if ((*iter).m_pEh == aEh) {
            m_Nodes.erase(iter);
            return 0;
        } else {
            ++iter;
        }
    }
    return 1;
}

int CSyTimerQueueOrderedList::PopFirstNode_l(CNode &aPopNode)
{
    SY_ASSERTE_RETURN(!m_Nodes.empty(), -1);

    aPopNode = m_Nodes.front();
    m_Nodes.pop_front();
    return 0;
}

int CSyTimerQueueOrderedList::RePushNode_l(const CNode &aPushNode)
{
    NodesType::iterator iter = m_Nodes.begin();
    for (; iter != m_Nodes.end(); ++iter) {
        if ((*iter).m_tvExpired >= aPushNode.m_tvExpired) {
            m_Nodes.insert(iter, aPushNode);
            break;
        }
    }
    if (iter == m_Nodes.end()) {
        m_Nodes.insert(iter, aPushNode);
    }

    //  EnsureSorted();
    return 0;
}

int CSyTimerQueueOrderedList::GetEarliestTime_l(CSyTimeValue &aEarliest) const
{
    if (!m_Nodes.empty()) {
        aEarliest = m_Nodes.front().m_tvExpired;
        return 0;
    } else {
        return -1;
    }
}

int CSyTimerQueueOrderedList::EnsureSorted()
{
#ifdef SY_DEBUG
    if (m_Nodes.size() <= 1) {
        return 0;
    }
    CSyTimeValue tvMin = (*m_Nodes.begin()).m_tvExpired;
    NodesType::iterator iter1 = m_Nodes.begin();
    for (++iter1; iter1 != m_Nodes.end(); ++iter1) {
        SY_ASSERTE_RETURN((*iter1).m_tvExpired >= tvMin, -1);
        tvMin = (*iter1).m_tvExpired;
    }
#endif // SY_DEBUG
    return 0;
}

int CSyTimerQueueOrderedList::PushNode_l(const CNode &aPushNode)
{
    BOOL bFoundEqual = FALSE;
    BOOL bInserted = FALSE;
    NodesType::iterator iter = m_Nodes.begin();
    while (iter != m_Nodes.end()) {
        if ((*iter).m_pEh == aPushNode.m_pEh) {
            SY_ASSERTE(!bFoundEqual);
            iter = m_Nodes.erase(iter);
            bFoundEqual = TRUE;
            if (bInserted || iter == m_Nodes.end()) {
                break;
            }
        }

        if (!bInserted && (*iter).m_tvExpired >= aPushNode.m_tvExpired) {
            iter = m_Nodes.insert(iter, aPushNode);
            ++iter;
            bInserted = TRUE;
            if (bFoundEqual) {
                break;
            }
        }
        ++iter;
    }

#ifdef SY_DEBUG
    if (iter != m_Nodes.end()) {
        SY_ASSERTE(bInserted && bFoundEqual);
    }
#endif // SY_DEBUG

    if (!bInserted) {
        m_Nodes.push_back(aPushNode);
    }

    EnsureSorted();
    return bFoundEqual ? 1 : 0;
}

////////////////////////////////////////////////////
//CSyTimerLeastHeapQueue
////////////////////////////////////////////////////
int CSyTimerMinHeapQueue::PushNode_l(const CNode &aPushNode)
{
    bool bFoundEqual = m_nodes.remove(aPushNode.m_pEh);
    m_nodes.push(aPushNode);
    
    return bFoundEqual ? 1 : 0;
}

int CSyTimerMinHeapQueue::EraseNode_l(ISyTimerHandler *aEh)
{
    bool bFoundEqual = m_nodes.remove(aEh);
    return bFoundEqual ? 0 : 1;
}

int CSyTimerMinHeapQueue::RePushNode_l(const CNode &aPushNode)
{
    m_nodes.push(aPushNode);

    return 0;
}

int CSyTimerMinHeapQueue::PopFirstNode_l(CNode &aPopNode)
{
    SY_ASSERTE_RETURN(!m_nodes.empty(), -1);
    
    aPopNode = m_nodes.top();
    m_nodes.pop();
    return 0;
}

int CSyTimerMinHeapQueue::GetEarliestTime_l(CSyTimeValue &aEarliest) const
{
    if (!m_nodes.empty()) {
        aEarliest = m_nodes.top().m_tvExpired;
        return 0;
    }
    return -1;
}

/////////////////////////////////////////////////////////
//CSyTimerWheelQueue
/////////////////////////////////////////////////////////
int CSyTimerWheelQueue::PushNode_l(const CNode &aPushNode)
{
    bool bFoundEqual = (EraseNode_l(aPushNode.m_pEh) == 0);
    
    pushNode(aPushNode);

    return bFoundEqual ? 1 : 0;
}

#define WHEEL_MAKE_SLOT(x) (x.GetSec() * 100 + x.GetUsec() / 10000) % m_maxBuckets

int CSyTimerWheelQueue::EraseNode_l(ISyTimerHandler *aEh)
{
    int j = aEh->m_id;
    if (j < 0 || j >= m_maxBuckets)
        return 1;
    
    CNode node;
    bool bFound = remove(m_Nodes[j], aEh, node);
    if (!bFound) {
        bFound = remove(m_NodesFar[j], aEh, node);
    }
    
    if ((WHEEL_MAKE_SLOT(m_aEarliest) == j) && m_Nodes[j].empty()) {
        scheduleNext(j);
    }
    
    return bFound ? 0 : 1;
}

bool CSyTimerWheelQueue::remove(NodesType& list, ISyTimerHandler *aEh, CNode& node)
{
    NodesType::iterator itNode = list.begin();
    for (; itNode != list.end(); itNode++) {
        if (itNode->m_pEh == aEh) {
            node = *itNode;
            list.erase(itNode);
            return true;
        }
    }
    return false;
}

int CSyTimerWheelQueue::pushNode(const CNode &aPushNode) {
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
    bool bFar = false;
    if (aPushNode.m_tvExpired.GetSec() > tvCur.GetSec() + (m_maxBuckets / 100 - 1)) {
        bFar = true;
    }
    int j = WHEEL_MAKE_SLOT(aPushNode.m_tvExpired);
    if (!bFar) {
        m_Nodes[j].push_back(aPushNode);
    } else {
        NodesType::iterator it = m_NodesFar[j].begin();
        for (; it != m_NodesFar[j].end(); it++) {
            if (it->m_tvExpired >= aPushNode.m_tvExpired)
                break;
        }
        m_NodesFar[j].insert(it, aPushNode);
    }
    if (aPushNode.m_tvExpired < m_aEarliest) {
        m_aEarliest = aPushNode.m_tvExpired;
    }
    aPushNode.m_pEh->m_id = j;
    
    return j;
}

int CSyTimerWheelQueue::RePushNode_l(const CNode &aPushNode)
{
    pushNode(aPushNode);
    return 0;
}

int CSyTimerWheelQueue::scheduleNext(int j)
{
    CSyTimeValue tvCur = CSyTimeValue::GetTimeOfDay();
    bool bFound = false;
    for (int i = 0; i < m_maxBuckets; i++) {
        int idx = (i + j) % m_maxBuckets;
        if (m_Nodes[idx].empty()) {
            if (m_NodesFar[idx].empty())
                continue;
            else {
                while (!m_NodesFar[idx].empty() && (m_NodesFar[idx].front().m_tvExpired.GetSec() < tvCur.GetSec() + 1 + (i / 100))) {
                    m_Nodes[idx].push_back(m_NodesFar[idx].front());
                    m_NodesFar[idx].pop_front();
                }
            }
        }
        if (!m_Nodes[idx].empty()) {
            bFound = true;
            m_aEarliest = m_Nodes[idx].front().m_tvExpired;
            break;
        }
    }
    if (!bFound) {
        m_aEarliest = CSyTimeValue::get_tvMax();
    }
    return 0;
}

int CSyTimerWheelQueue::PopFirstNode_l(CNode &aPopNode)
{
    int j = WHEEL_MAKE_SLOT(m_aEarliest);
    if (m_Nodes[j].empty()) {
        for (int i = 0; i < m_maxBuckets; i++) {
            SY_ASSERTE(m_Nodes[i].empty() && m_NodesFar[i].empty());
        }
    }
    SY_ASSERTE(!m_Nodes[j].empty());
    
    aPopNode = m_Nodes[j].front();
    m_Nodes[j].pop_front();
    
    if (m_Nodes[j].empty()) {
        scheduleNext(j);
    }
    
    return 0;
}

int CSyTimerWheelQueue::GetEarliestTime_l(CSyTimeValue &aEarliest) const
{
    if (m_aEarliest == CSyTimeValue::get_tvMax()) {
        return -1;
    }
    aEarliest = m_aEarliest;
    return 0;
}

/**
 Test result:
 Done:20020 (60%)
 ==========================
 interval=10, times=396940, ref=400400
 interval=50, times=159393, ref=160160
 interval=200, times=19823, ref=20020
 interval=300, times=13200, ref=13346
 interval=1000, times=11470, ref=12012
 interval=2000, times=3646, ref=4004
 interval=6000, times=600, ref=667
 interval=8000, times=400, ref=500
 
 Done:20020 (25%)
 ==========================
 interval=10, times=399944, ref=400400
 interval=50, times=159874, ref=160160
 interval=200, times=19937, ref=20020
 interval=300, times=13200, ref=13346
 interval=1000, times=11810, ref=12012
 interval=2000, times=3873, ref=4004
 interval=6000, times=600, ref=667
 interval=8000, times=400, ref=500
 Program ended with exit code: 0
 
 Done:20040 (6%)
 ==========================
 interval=10, times=399919, ref=400800
 interval=50, times=160000, ref=160320
 interval=200, times=20000, ref=20040
 interval=300, times=13200, ref=13360
 interval=1000, times=12000, ref=12024
 interval=2000, times=4000, ref=4008
 interval=6000, times=600, ref=668
 interval=8000, times=400, ref=501
 Program ended with exit code: 0
 */
