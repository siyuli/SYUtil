#ifndef SY_THREADMISC_H
#define SY_THREADMISC_H

#include "SyDef.h"
#include "SyDataType.h"
#include "SyThreadEx.h"

#ifdef SY_MACOS
    #include "SyCommon.h"
    #include "SyNetwork.h"
#endif

#include <list>

START_UTIL_NS

/*
typedef int TType;

enum {
    TT_MAIN,
    TT_NETWORK,
    TT_DNS,
    TT_PROXYDNS,
    TT_CURRENT,
    WUTIL_TT_TIMER,
    TT_NETWORK_USER_DEFINED,
    TT_NETWORK_USER_DEFINED_MAX = TT_NETWORK_USER_DEFINED + 8,
    TT_UNKNOWN = -1,

    TT_USER_DEFINE_BASE = 1000,
    TT_USER_SESSION = TT_USER_DEFINE_BASE + 1000,
    TT_CLIENT_SESSION = TT_USER_SESSION,
    TT_AB_SESSION,
    TT_SESSION_SYD,
    TT_DNS6,
    TT_SIGNAL,
    TT_CFENGINE,
    TT_RTP_SMOOTH_SEND,
    TT_RTCP_REPORT,
    TT_OCSP,
};
*/

typedef int TType;

enum {
    TT_MAIN,
    TT_NETWORK,
    TT_DNS,
    TT_PROXYDNS,
    TT_CURRENT,
    WUTIL_TT_TIMER,
    TT_NETWORK_USER_DEFINED,
    TT_NETWORK_USER_DEFINED_MAX = TT_NETWORK_USER_DEFINED + 8,
    TT_AB_SESSION,
    TT_SESSION_SYD,
    TT_DNS6,
    TT_SIGNAL,
    TT_CFENGINE,
    TT_RTP_SMOOTH_SEND,
    TT_RTCP_REPORT,
    TT_OCSP,
    TT_CERT,

    TT_USER_SESSION =  1000,
    TT_CLIENT_SESSION = TT_USER_SESSION + 1000,

    TT_USER_DEFINE_BASE = 3000,

    TT_UNKNOWN = -1,
};

enum TFlag {
    TF_NONE = 0,
    TF_JOINABLE = (1 << 0),
    TF_DETACHED = (1 << 1)
};


class SY_OS_EXPORT CSyEnsureSingleThread
{
public:
    CSyEnsureSingleThread();
    void EnsureSingleThread() const;
    void Reset2CurrentThreadInfo();
    void Reset2ThreadId(SY_THREAD_ID aTid);

protected:
    SY_THREAD_ID m_ThreadIdOpen;
    char  m_ThreadNameOpen[512];
};


SY_OS_EXPORT SY_THREAD_ID GetThreadSelfId();
SY_OS_EXPORT BOOL IsThreadEqual(SY_THREAD_ID aT1, SY_THREAD_ID aT2);
SY_OS_EXPORT BOOL IsEqualCurrentThread(SY_THREAD_ID aId);

SY_OS_EXPORT int SyGetProcessID();
SY_OS_EXPORT int SyGetThreadID();

class ASyThread;
SY_OS_EXPORT ASyThread *getCurrentThread(void);

//get thread by type
SY_OS_EXPORT ASyThread *GetThread(TType aType);

//get thread by id
SY_OS_EXPORT ASyThread *GetThreadById(SY_THREAD_ID id);

//for get current thread
extern std::list<ASyThread *> g_threadList;
SyResult RegisterThread(ASyThread *thread);
SyResult UnRegisterThread(ASyThread *thread);
SY_OS_EXPORT SyResult TPCleanThreadList();
//end

END_UTIL_NS

#endif //SY_THREADMISC_H
