#include "SyThreadMisc.h"
#include "SyAssert.h"
#include "SyThread.h"

#include "SyDebug.h"

#ifdef SY_APPLE
    #include <pthread.h>
#endif

#ifdef SY_LINUX_SERVER
    #include <sys/syscall.h>
    #define gettid() syscall(__NR_gettid)
#endif

START_UTIL_NS

CSyEnsureSingleThread::CSyEnsureSingleThread()
{
    m_ThreadIdOpen = GetThreadSelfId();
    memset(m_ThreadNameOpen,0,sizeof(m_ThreadNameOpen));
#if defined(SY_IOS) || defined(SY_MACOS)
    pthread_getname_np(pthread_self(), m_ThreadNameOpen,sizeof(m_ThreadNameOpen)-1);
#endif

}

void CSyEnsureSingleThread::EnsureSingleThread() const
{
#ifdef SY_DEBUG
    SY_THREAD_ID tidCur = GetThreadSelfId();
    SY_ASSERTE(IsThreadEqual(m_ThreadIdOpen, tidCur));
#endif // SY_DEBUG
}

void CSyEnsureSingleThread::Reset2CurrentThreadInfo()
{
    m_ThreadIdOpen = GetThreadSelfId();
#if defined(SY_IOS) || defined(SY_MACOS)
    pthread_getname_np(pthread_self(), m_ThreadNameOpen,sizeof(m_ThreadNameOpen)-1);
#endif
}

void CSyEnsureSingleThread::Reset2ThreadId(SY_THREAD_ID aTid)
{
    m_ThreadIdOpen = aTid;
}


int SyGetProcessID()
{
#if defined(SY_WIN32)
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

int SyGetThreadID()
{
#if defined(SY_WIN32)
    return GetCurrentThreadId();
#elif defined(SY_ANDROID) || defined(SY_LINUX_SERVER)
    return gettid();
#else
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return (int)tid;
#endif
}

SY_OS_EXPORT SY_THREAD_ID GetThreadSelfId()
{
#ifdef WIN32
    return ::GetCurrentThreadId();
#else
    return ::pthread_self();
#endif // WIN32
}

SY_OS_EXPORT BOOL IsThreadEqual(SY_THREAD_ID aT1, SY_THREAD_ID aT2)
{
#ifdef WIN32
    return aT1 == aT2;
#else
    return ::pthread_equal(aT1, aT2);
#endif // WIN32
}

SY_OS_EXPORT BOOL IsEqualCurrentThread(SY_THREAD_ID aId)
{
    return IsThreadEqual(aId, GetThreadSelfId());
}

std::list<ASyThread *> g_threadList;
typedef CSyMutexThread MutexType ;
MutexType g_thread_Mutex;
ASyThread *getCurrentThread(void)
{
    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    SY_THREAD_ID tidSelf = GetThreadSelfId();
    std::list<ASyThread *>::iterator iter = g_threadList.begin();
    for (; iter != g_threadList.end(); ++iter) {
        if ((*iter)->GetThreadId() == tidSelf) {
            return (*iter);
        }
    }

    return NULL;
}

SyResult TPCleanThreadList()
{
    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    SY_INFO_TRACE("CleanThreadList, list size=" << g_threadList.size());
    std::list<ASyThread *>::iterator it = g_threadList.begin();
    for (; it != g_threadList.end(); it++) {
        if (*it) {
            (*it)->Stop();
        }
    }
    g_threadList.clear();

    return SY_OK;
}

//thread management
SyResult RegisterThread(ASyThread *thread)
{
    SY_INFO_TRACE("RegisterThread(" << thread << ")" << thread->GetThreadId() << "event Queue:" << thread->GetEventQueue());

    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    SY_ASSERTE(thread);

    g_threadList.push_back(thread);

    return SY_OK;
}

SyResult UnRegisterThread(ASyThread *thread)
{
    SY_INFO_TRACE("UnRegisterThread(" << thread << ")" << thread->GetThreadId());

    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    std::list<ASyThread *>::iterator iter = g_threadList.begin();
    SY_THREAD_ID tid = thread->GetThreadId();

    for (; iter != g_threadList.end(); ++iter) {
        if ((*iter)->GetThreadId() == tid && *iter == thread) {
            g_threadList.erase(iter);
            return SY_OK;
        }
    }

    return SY_ERROR_NOT_FOUND;
}

//get thread by type
ASyThread *GetThread(TType aType)
{
    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    if (TT_CURRENT == aType) {
        SY_THREAD_ID tidSelf = GetThreadSelfId();
        std::list<ASyThread *>::iterator iter = g_threadList.begin();
        for (; iter != g_threadList.end(); ++iter) {
            if ((*iter)->GetThreadId() == tidSelf) {
                return (*iter);
            }
        }
    } else {
        std::list<ASyThread *>::iterator iter = g_threadList.begin();
        for (; iter != g_threadList.end(); ++iter) {
            if ((*iter)->GetThreadType() == aType) {
                return (*iter);
            }
        }
    }
    SY_WARNING_TRACE("GetThread, aType=" << aType);
    return NULL;
}

//get thread by id
ASyThread *GetThreadById(SY_THREAD_ID id)
{
    CSyMutexGuardT<MutexType> theGuard(g_thread_Mutex);
    std::list<ASyThread *>::iterator iter = g_threadList.begin();
    for (; iter != g_threadList.end(); ++iter) {
        if ((*iter)->GetThreadId() == id) {
            ASyThread *thread = *iter;
            SY_DEBUG_TRACE("GetThreadById(" << thread << ")" << thread->GetThreadId() << "event Queue:" << thread->GetEventQueue());
            return (*iter);
        }
    }
    return NULL;
}

//end


END_UTIL_NS
