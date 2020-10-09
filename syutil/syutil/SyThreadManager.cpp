
//#include "SyBase.h"
#include "SyThread.h"
#include "SyThreadMisc.h"
#include "SyThreadManager.h"
#include "SyThreadReactor.h"
#include "SyReactorInterface.h"
#include "SyThreadTask.h"
#include "SyAtomicOperationT.h"

#ifdef SUPPORT_DNS
    #include "SyDnsManager.h"
    #include "SyDns6Manager.h"
#endif

#if defined(WP8) || defined(UWP)
    #include "ThreadEmulation.h"
#endif

#include "SySSLUtil.h"

#ifdef _SC_MACOS
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/engine.h>
static void locking_call(int mode, int type, const char *file, int line)
{
    //     static int modes[CRYPTO_NUM_LOCKS] = {0, 0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static CSyMutexThreadRecursive locks[CRYPTO_NUM_LOCKS];
    if (mode & CRYPTO_LOCK) {
        locks[type].Lock();
    } else {
        locks[type].UnLock();
    }
}
#endif //_SC_MACOS

#if (defined SY_WIN32 && !defined UWP && !defined WP8)
    #include "SyReactorSelect.h"
    #include "SyReactorWin32Message.h"

    #ifdef SUPPORT_REACTOR
        extern ATOM g_atomRegisterClass;
        extern HINSTANCE g_pReactorWin3Instance;
    #endif

    extern BOOL g_bRunTimeLoad;

#elif defined (UWP) || defined(WP8)
    #include "SyReactorSelect.h"

#elif defined (SY_MACOS)

    #ifdef SY_IOS
        #include "SyThreadIOSEventPatch.h"
    #else
        #include "SyThreadMacEventPatch.h"
    #endif

#elif defined (SY_LINUX)
    #include <sys/utsname.h>
    #ifdef SY_ENABLE_EPOLL
        #include "SyReactorEpoll.h"
    #endif // SY_ENABLE_EPOLL
#endif // SY_WIN32

#ifdef SY_USE_REACTOR_SELECT
    #include "SyReactorSelect.h"
#endif // SY_USE_REACTOR_SELECT

#ifdef SY_WIN32_ENABLE_AsyncGetHostByName
    extern ATOM g_atomDnsRegisterClass;
#endif // SY_WIN32_ENABLE_AsyncGetHostByName


USE_UTIL_NS


static CSyThreadManager *s_pThreadManagerOnlyOne = nullptr;
static BOOL s_bThreadManagerOnlyOneByNew = FALSE;
namespace util {
    bool g_bAllThreadsExited = false;
    SY_THREAD_ID g_waitingThread = NULL;
};
bool CSyThreadManager::m_bUseHeartbeat = false;


void CSyThreadManager::CleanupOnlyOne()
{
    SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne,");

    //then we can cleanup more than one times
    if (!s_pThreadManagerOnlyOne) {
        SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne() already cleanup, s_bThreadManagerOnlyOneByNew = " << s_bThreadManagerOnlyOneByNew);
        return;
    }

    ASyThread *pMain = s_pThreadManagerOnlyOne->GetThread(TT_MAIN);
    CSyEventQueueBase *pMainEventQueue = NULL;
    if (pMain) {
        pMainEventQueue = dynamic_cast<CSyEventQueueBase *>(pMain->GetEventQueue());
        if (pMainEventQueue == nullptr) {
            pMainEventQueue = static_cast<CSyEventQueueBase *>(pMain->GetEventQueue());
            SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne, pMainEventQueue=" << pMainEventQueue);
        }
    }

    if (pMainEventQueue && pMainEventQueue->IsRunning()) {
        SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne, set hook.");
        pMainEventQueue->SetEventHook(CleanupOnlyOneInternal);
    } else {
        SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne internal direct.");
        CleanupOnlyOneInternal();
    }
}

void CSyThreadManager::EnsureThreadManagerExited()
{
    util::g_waitingThread = GetThreadSelfId();
    for(int i = 0; i < 400; i++) {
        SleepMs(30);
        if(util::g_bAllThreadsExited) {
            SY_INFO_TRACE("CSyThreadManager::EnsureThreadManagerExited(), called from a threadmanager managed thread. id=" << util::g_waitingThread);
            break;
        }
        if(s_pThreadManagerOnlyOne == NULL) {
            SY_INFO_TRACE("CSyThreadManager::EnsureThreadManagerExited(), thread manager has been destroyed.");
            break;
        }
        if (i % 10 == 0) {
            SY_INFO_TRACE("CSyThreadManager::EnsureThreadManagerExited(), i=" << i);
        }
    }
}

void CSyThreadManager::CleanupOnlyOneInternal()
{
    SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOneInternal");
#ifdef SUPPORT_DNS
#if defined SY_LINUX || defined SY_SOLARIS || defined SY_WIN32 || defined SY_IOS
    SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne stop dns thread");
    CSyProxyDnsManager::Instance()->Shutdown();
    CSyDnsManager::Instance()->Shutdown();
#endif
#endif //SUPPORT_DNS

    if (s_bThreadManagerOnlyOneByNew) {
        SY_ASSERTE(s_pThreadManagerOnlyOne);
        delete s_pThreadManagerOnlyOne;
    }
    s_pThreadManagerOnlyOne = NULL;

#if defined SY_WBX_UNIFIED_TRACE /*&& defined SY_WIN32 // the changes for all platform to avoid crash */ //for bug303444, need unload wbxtrace.dll when unload MMP
    SY_INFO_TRACE("CSyThreadManager::CleanupOnlyOne close tracer");
    CSyT120Trace::instance()->Close();
#endif

#ifdef _SC_MACOS
    ::CRYPTO_cleanup_all_ex_data();
    ::ERR_free_strings();
    ::ERR_remove_state(0);
    ::EVP_cleanup();
    ::CRYPTO_set_locking_callback(NULL);
    ::ENGINE_cleanup();
#endif
}

CSyThreadManager::CSyThreadManager()
    : m_TTpyeUserThread(TT_USER_DEFINE_BASE)
    , m_pNetworkThread(NULL)
{
    SY_INFO_TRACE_THIS("CSyThreadManager::CSyThreadManager");
    SY_ASSERTE(!s_pThreadManagerOnlyOne);
    s_pThreadManagerOnlyOne = this;
    util::g_bAllThreadsExited = false;

    InitThreadForOpenSSL();
}

CSyThreadManager::~CSyThreadManager()
{
    SY_INFO_TRACE_THIS("CSyThreadManager::~CSyThreadManager");

    //will unlock the netowrk thread caused by proxy event - arrayt
    //CSyCleanUpBase::CleanupAll();

    StopAllThreads(NULL);
    JoinAllThreads();

    //move this before joining thread since proxy may block the network thread -arrayt
    // cleaup instance before delete threads because some instances will use threads.
    CSyCleanUpBase::CleanupAll();
    UnInitThreadForOpenSSL();

    ThreadsType tmpThreads;
    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);
        m_Threads.swap(tmpThreads);
    }
    ThreadsType::iterator iter = tmpThreads.begin();
    for (; iter != tmpThreads.end(); ++iter) {
        if ((*iter)->IsDetached()) {
            continue;
        }

        (*iter)->Destory(SY_OK);
    }

#if defined SY_WIN32 && !defined UWP && !defined WP8
#ifdef SUPPORT_REACTOR
    if (g_atomRegisterClass) {
        if (!::UnregisterClass(reinterpret_cast<LPCSTR>(g_atomRegisterClass), g_pReactorWin3Instance)) {
            SY_INFO_TRACE_THIS("CSyThreadManager::~CSyThreadManager, UnregisterClass() failed!"
                               " g_atomRegisterClass=" << g_atomRegisterClass <<
                               " g_pReactorWin3Instance=" << g_pReactorWin3Instance <<
                               " err=" << ::GetLastError());
        } else {
            SY_INFO_TRACE_THIS("CSyThreadManager::~CSyThreadManager,"
                               " g_atomRegisterClass=" << g_atomRegisterClass <<
                               " g_pReactorWin3Instance=" << g_pReactorWin3Instance);
        }
        g_atomRegisterClass = 0;
    }
#endif //SUPPORT_REACTOR
#endif // SY_WIN32

#ifdef SY_WIN32_ENABLE_AsyncGetHostByName
#ifdef SUPPORT_REACTOR
    if (g_atomDnsRegisterClass) {
        if (!::UnregisterClass(reinterpret_cast<LPCSTR>(g_atomDnsRegisterClass), g_pReactorWin3Instance)) {
            SY_INFO_TRACE_THIS("CSyThreadManager::~CSyThreadManager, UnregisterClass() failed!"
                               " g_atomDnsRegisterClass=" << g_atomDnsRegisterClass <<
                               " err=" << ::GetLastError());
        } else {
            SY_INFO_TRACE_THIS("CSyThreadManager::~CSyThreadManager,"
                               " g_atomDnsRegisterClass=" << g_atomDnsRegisterClass);
        }
        g_atomDnsRegisterClass = 0;
    }
#endif //SUPPORT_REACTOR
#endif // SY_WIN32_ENABLE_AsyncGetHostByName

    SocketCleanup();
    s_pThreadManagerOnlyOne = NULL;
    //CSyConnectionManager::CleanupInstance();
}

CSyThreadManager *CSyThreadManager::Instance()
{
    //  SY_ASSERTE(s_pThreadManagerOnlyOne);
    if (!s_pThreadManagerOnlyOne) {
        //      SY_WARNING_TRACE_THIS("CSyThreadManager::Instance, s_pThreadManagerOnlyOne is NULL!"
        //          " You should alloc it in the stack or heap first!");

#if !defined (SY_WIN32)
        //ignore SIG_PIPE signal
        ::signal(SIGPIPE, SIG_IGN);
#endif //~WP8
        // We have to new <CSyThreadManager> becauase Connect1.1 UI doesn't
        // alloc it in the main function.
        // we have to assume it's in main thread.
        SY_INFO_TRACE("CSyThreadManager::Instance, new CSyThreadManager.");
        new CSyThreadManager();
        s_bThreadManagerOnlyOneByNew = TRUE;

        SyResult rv = s_pThreadManagerOnlyOne ? s_pThreadManagerOnlyOne->InitMainThread(0, NULL) : SY_ERROR_FAILURE;
        if (SY_FAILED(rv)) {
            delete s_pThreadManagerOnlyOne;
            s_pThreadManagerOnlyOne = NULL;
            return NULL;
        } else {
#ifdef SUPPORT_DNS
            CSyDns6Manager::Instance();
#endif
        }
#ifdef _SC_MACOS
        CRYPTO_set_locking_callback(locking_call);
#endif
    }
    return s_pThreadManagerOnlyOne;
}

void CSyThreadManager::EnableHeartbeat(bool bUseHeartbeat)
{
	m_bUseHeartbeat = bUseHeartbeat;
}

SyResult CSyThreadManager::InitMainThread(int aArgc, char **aArgv)
{
    //set seed for rand() just once when initializing on server side
    static unsigned int uTimeSeed = 0;
    unsigned int uTimeNow = (unsigned int)time(NULL);
    if (uTimeNow != uTimeSeed) {
        srand(uTimeNow);
        uTimeSeed = uTimeNow;
    }

#ifdef SUPPORT_REACTOR

    SY_INFO_TRACE_THIS("CSyThreadManager::InitMainThread, argc  = " << aArgc);

    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);

        SY_ASSERTE(m_Threads.empty());
        if (!m_Threads.empty()) {
            SY_WARNING_TRACE_THIS("CSyThreadManager::InitMainThread, You should InitMainThread before creating other thread!");
            return SY_ERROR_ALREADY_INITIALIZED;
        }
    }

    SyResult rv = SocketStartup();
    if (SY_FAILED(rv)) {
        return rv;
    }

    ISyReactor *pReactorMain = NULL;
#if (defined SY_WIN32 && !defined UWP && !defined WP8)
    if (GetNetworkThreadModule() == TM_SINGLE_MAIN) {
        pReactorMain = CreateNetworkReactor();
    } else {
        pReactorMain = new CSyReactorWin32Message();
    }
#else
    if (GetNetworkThreadModule() == TM_SINGLE_MAIN) {
        pReactorMain = CreateNetworkReactor();
    } else {
        // the main thread on Linux is task thread.
        pReactorMain = NULL;
    }
#endif // SY_WIN32

    ASyThread *pThread = NULL;
    if (pReactorMain) {
        rv = CreateReactorThread("t-main",TT_MAIN, pReactorMain, pThread);
    } else {

#if defined SY_MAC
        if(true == m_bUseHeartbeat) {
            pThread = new CSyThreadHeartBeat();
            SY_INFO_TRACE_THIS("CSyThreadManager::InitMainThread, created CSyThreadHeartBeat");
        } else {
            pThread = new CSyThreadMacEventPatch();
            SY_INFO_TRACE_THIS("CSyThreadManager::InitMainThread, created CSyThreadMacEventPatch");
        }
#elif defined SY_IOS
        pThread = new CSyThreadIOSEventPatch();
#elif defined(SY_ANDROID) || defined (SY_WIN_PHONE)
        pThread = new CSyThreadHeartBeat();
#else // linux
        if(true == m_bUseHeartbeat) {
            pThread = new CSyThreadHeartBeat();
            SY_INFO_TRACE_THIS("CSyThreadManager::InitMainThread, created CSyThreadHeartBeat");
        } else {
            pThread = new CSyThreadTask();
            SY_INFO_TRACE_THIS("CSyThreadManager::InitMainThread, created CSyThreadTask");
        }
#endif
        if (pThread) {
            rv = pThread->Create("t-main",TT_MAIN, TF_JOINABLE, TRUE);
        } else {
            rv = SY_ERROR_OUT_OF_MEMORY;
        }
    }

    if (SY_FAILED(rv)) {
        if (pThread) {
            pThread->Destory(rv);
        }
        return rv;
    }

    rv = RegisterThread(pThread);
    if (SY_FAILED(rv)) {
        SY_ERROR_TRACE_THIS("CSyThreadManager::InitMainThread, RegisterThread Failed, ret = "<<rv);
        return rv;
    }

    // create network thread when program startuping.
    if (SpawnNetworkThread_i(TT_NETWORK, "t-network", m_pNetworkThread) != SY_OK) {
        if (pThread) {
            pThread->Destory(SY_OK);
        }
        return SY_ERROR_UNEXPECTED;
    }

    pThread->OnThreadInit();

#endif //SUPPORT_REACTOR
    return SY_OK;
}


SyResult CSyThreadManager::SpawnNetworkThread_i(TType aType, const char *szLabel, ASyThread *&pTread)
{
#ifdef SUPPORT_REACTOR

    // we have to instance CSyThreadManager because it will invoke SpawnNetworkThread_i().
    CSyThreadManager *pInst = CSyThreadManager::Instance();
    if (pTread) {
        return SY_OK;
    }

    CSyThreadManager::TModule tmodule = CSyThreadManager::GetNetworkThreadModule();
    if (tmodule == CSyThreadManager::TM_SINGLE_MAIN) {
        std::unique_ptr<CSyThreadDummy> pThreadReactor(new CSyThreadDummy());
        if (!pThreadReactor.get()) {
            return SY_ERROR_OUT_OF_MEMORY;
        }

        SyResult rv = pThreadReactor->Init(pInst->GetThread(TT_MAIN), aType);
        if (SY_FAILED(rv)) {
            return rv;
        }

        pTread = pThreadReactor.release();
    } else {
        ISyReactor *pReactorNetwork = CSyThreadManager::CreateNetworkReactor();
        SyResult rv = pInst->CreateReactorThread(szLabel, aType, pReactorNetwork, pTread);

        if (SY_FAILED(rv)) {
            return rv;
        }

        rv = RegisterThread(pTread);

        if (SY_FAILED(rv)) {
            SY_ERROR_TRACE_THIS("CSyThreadManager::CreateReactorThread, RegisterThread failed, ret = " << rv);
            return rv;
        }
    }

#endif //SUPPORT_REACTOR

    return SY_OK;
}

ISyReactor *CSyThreadManager::CreateNetworkReactor()
{
    ISyReactor *pReactorRet = NULL;
    
    //Windows(phone) platform defined SY_ENABLE_SELECT_WINOS instead of SY_USE_REACTOR_SELECT
#if defined(SY_USE_REACTOR_SELECT) || defined(SY_ENABLE_SELECT_WINOS)
    pReactorRet = new CSyReactorSelect();
#elif defined (SY_WIN32)
    pReactorRet = new CSyReactorWin32AsyncSelect();
#else // !SY_WIN32
    struct utsname utName;
    if (::uname(&utName) < 0) {
        SY_WARNING_TRACE("CSyThreadManager::CreateNetworkReactor, uname() failed!"
                         "err=" << errno);
    } else {
        int major, minor = 0;
        char *pbuf = NULL;
        pbuf = strtok(utName.release, ".");
        if(pbuf)
        {
            major = atoi(pbuf);
            pbuf = strtok(NULL, ".");
            if(pbuf)
                minor = atoi(pbuf);
        }
        SY_INFO_TRACE("CSyThreadManager::CreateNetworkReactor, major = " << major << " minor = " << minor);
        if (major > 2 || (major == 2 && minor >= 6)) {
            pReactorRet = new CSyReactorEpoll();
        }
        else {
            SY_WARNING_TRACE("CSyThreadManager::CreateNetworkReactor,"
                             " don't support this release of Linux."
                             " release=" << utName.release <<
                             " sysname=" << utName.sysname);
        }
    }
#endif // SY_USE_REACTOR_SELECT

    return pReactorRet;
}

SyResult CSyThreadManager::
CreateUserTaskThread(const char *name,ASyThread *&aThread, TFlag aFlag, BOOL bWithTimerQueue, TType aType)
{
    SY_ASSERTE(!aThread);

    ASyThread *pThread = NULL;
    if (bWithTimerQueue) {
        pThread = new CSyThreadTask();
    } else {
        pThread = new CSyThreadTaskWithEventQueueOnly();
    }
    SY_ASSERTE_RETURN(pThread, SY_ERROR_OUT_OF_MEMORY);

    SyResult rv = pThread->Create(name,aType == TT_UNKNOWN ? GetAndIncreamUserType() : aType, aFlag, TRUE);
    if (SY_FAILED(rv)) {
        pThread->Destory(rv);
        return rv;
    }

    rv = RegisterThread(pThread);
    if (SY_FAILED(rv)) {
        SY_ERROR_TRACE_THIS("CSyThreadManager::CreateUserTaskThread, RegisterThread failed, ret = " << rv);
        return rv;
    }

    aThread = pThread;
    return SY_OK;
}

TType CSyThreadManager::GetAndIncreamUserType()
{
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);
    return ++m_TTpyeUserThread;
}

SyResult CSyThreadManager::
CreateReactorThread(const char *name,TType aType, ISyReactor *aReactor, ASyThread *&aThread)
{
    // Note: aReactor will be delete if failed.
    SyResult rv;
    SY_ASSERTE_RETURN(aReactor, SY_ERROR_INVALID_ARG);
    SY_ASSERTE(!aThread);

    std::unique_ptr<CSyThreadReactor> pThreadReactor(new CSyThreadReactor());
    if (!pThreadReactor.get()) {
        delete aReactor;
        return SY_ERROR_OUT_OF_MEMORY;
    }

    rv = pThreadReactor->Init(aReactor);
    if (SY_FAILED(rv)) {
        return rv;
    }

    rv = pThreadReactor->Create(name,aType);
    if (SY_FAILED(rv)) {
        return rv;
    }

    aThread = pThreadReactor.release();

    /*
    rv = RegisterThread(aThread);
    if (SY_FAILED(rv)) {
        SY_ERROR_TRACE_THIS("CSyThreadManager::CreateReactorThread, RegisterThread failed, ret = " << rv);
        return rv;
    }
    */

    return SY_OK;
}

SyResult CSyThreadManager::RegisterThread(ASyThread *aThread)
{
    SY_ASSERTE_RETURN(aThread, SY_ERROR_INVALID_ARG);
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);

    ThreadsType::iterator iter = m_Threads.begin();
    for (; iter != m_Threads.end(); ++iter) {
        // we don't check equal ThreadId because mutil ASyThreads may use the same id.
        if ((*iter) == aThread ||
                (*iter)->GetThreadType() == aThread->GetThreadType()) {
            SY_WARNING_TRACE_THIS("CSyThreadManager::RegisterThread, have registered."
                                  " aThread=" << aThread <<
                                  " tid=" << aThread->GetThreadId() <<
                                  " type=" << aThread->GetThreadType());
            SY_ASSERTE(FALSE);
            return SY_ERROR_FOUND;
        }
    }

    m_Threads.push_back(aThread);
    return SY_OK;
}

SyResult CSyThreadManager::UnregisterThread(ASyThread *aThread)
{
    SY_ASSERTE_RETURN(aThread, SY_ERROR_INVALID_ARG);
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);

    ThreadsType::iterator iter = m_Threads.begin();
    for (; iter != m_Threads.end(); ++iter) {
        if ((*iter) == aThread ||
                (*iter)->GetThreadType() == aThread->GetThreadType()) {
            m_Threads.erase(iter);
            return SY_OK;
        }
    }
    return SY_ERROR_NOT_FOUND;
}

SyResult CSyThreadManager::StopAllThreads(CSyTimeValue *aTimeout)
{
    // not support timeout now.
    SY_ASSERTE(!aTimeout);

    // only the main thread can stop all threads.
    ASyThread *pMain = GetThread(TT_MAIN);
    if (pMain) {
        SY_INFO_TRACE_THIS("CSyThreadManager::StopAllThreads, pMain->GetThreadId()=" << pMain->GetThreadId() << ", current thread id=" << GetThreadSelfId());
        SY_ASSERTE_RETURN(IsThreadEqual(pMain->GetThreadId(), GetThreadSelfId()), SY_ERROR_FAILURE);
    }

    ThreadsType tmpThreads;
    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);
        tmpThreads = m_Threads;
    }
    ThreadsType::iterator iter = tmpThreads.begin();
    for (; iter != tmpThreads.end(); ++iter) {
        SY_INFO_TRACE_THIS("CSyThreadManager::StopAllThreads thread type = " << (*iter)->GetThreadType());
#if !defined SY_LINUX_SERVER && (defined SY_LINUX || defined SY_SOLARIS) //TO-DO test Android
        if ((*iter)->GetThreadType() != TT_MAIN && (*iter)->GetThreadType() != TT_DNS) {
            (*iter)->Stop(aTimeout);
        }
#else
        if ((*iter)->GetThreadType() != TT_MAIN) {
            (*iter)->Stop(aTimeout);
        }
#endif
    }

    return SY_OK;
}

SyResult CSyThreadManager::JoinAllThreads()
{
    // only the main thread can Join all threads.
    ASyThread *pMain = GetThread(TT_MAIN);
    if (pMain) {
        SY_ASSERTE_RETURN(
            IsThreadEqual(pMain->GetThreadId(), GetThreadSelfId()),
            SY_ERROR_FAILURE);
    }

    ThreadsType tmpThreads;
    {
        CSyMutexGuardT<MutexType> theGuard(m_Mutex);
        tmpThreads = m_Threads;
    }
    ASyThread *pCallingThread = nullptr;
    ThreadsType::iterator iter = tmpThreads.begin();
    for (; iter != tmpThreads.end(); ++iter) {
        if ((*iter)->GetThreadType() != TT_MAIN) {
            if ((*iter)->GetThreadId() == util::g_waitingThread) {
                pCallingThread = *iter;
            } else {
                (*iter)->Join();
            }
        }
    }
    //Join the calling thread as the last one.
    if (pCallingThread) {
        util::g_bAllThreadsExited = true;
        pCallingThread->Join();
    }

    return SY_OK;
}

ASyThread *CSyThreadManager::GetThread(TType aType)
{
    CSyMutexGuardT<MutexType> theGuard(m_Mutex);
    if (TT_CURRENT == aType) {
        SY_THREAD_ID tidSelf = GetThreadSelfId();
        ThreadsType::iterator iter = m_Threads.begin();
        for (; iter != m_Threads.end(); ++iter) {
            if ((*iter)->GetThreadId() == tidSelf) {
                return (*iter);
            }
        }
    } else {
        ThreadsType::iterator iter = m_Threads.begin();
        for (; iter != m_Threads.end(); ++iter) {
            if ((*iter)->GetThreadType() == aType) {
                return (*iter);
            }
        }
    }
    return NULL;
}

ISyReactor *CSyThreadManager::GetThreadReactor(TType aType)
{
    ASyThread *pThread = GetThread(aType);
    if (pThread) {
        return pThread->GetReactor();
    } else {
        return NULL;
    }
}

ISyEventQueue *CSyThreadManager::GetThreadEventQueue(TType aType)
{
    ASyThread *pThread = GetThread(aType);
    if (pThread) {
        return pThread->GetEventQueue();
    } else {
        return NULL;
    }
}

ISyTimerQueue *CSyThreadManager::GetThreadTimerQueue(TType aType)
{
    ASyThread *pThread = GetThread(aType);
    if (pThread) {
        return pThread->GetTimerQueue();
    } else {
        return NULL;
    }
}

BOOL CSyThreadManager::IsEqualCurrentThread(TType aType)
{
    ASyThread *pThread = Instance()->GetThread(aType);
    if (pThread) {
        return IsThreadEqual(pThread->GetThreadId(), GetThreadSelfId());
    } else {
        return FALSE;
    }
}

void CSyThreadManager::SleepMs(DWORD aMsec)
{
#ifdef SY_WIN32
#if defined UWP || defined WP8 
    ThreadEmulation::Sleep(aMsec);
#else
    ::Sleep(aMsec);
#endif
#else
    struct timespec ts, rmts;
    ts.tv_sec = aMsec/1000;
    ts.tv_nsec = (aMsec%1000)*1000000;

    for (; ;) {
        int nRet = ::nanosleep(&ts, &rmts);
        if (nRet == 0) {
            break;
        }
        if (errno == EINTR) {
            ts = rmts;
        } else {
            SY_WARNING_TRACE("CSyThreadManager::SleepMs,"
                             "nanosleep() failed! err=" << errno);
            break;
        }
    }
#endif // SY_WIN32
}

#ifdef SY_WIN32
    BOOL CSyThreadManager::s_bSocketInited = FALSE;
#endif // SY_WIN32

SyResult CSyThreadManager::SocketStartup()
{
#ifdef SY_WIN32
    if (s_bSocketInited) {
        return SY_OK;
    }
    s_bSocketInited = TRUE;

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    if (::WSAStartup(wVersionRequested, &wsaData) != 0) {
        SY_WARNING_TRACE("CSyThreadManager::SocketStartup, WSAStartup() failed!"
                         " err=" << ::WSAGetLastError());
        s_bSocketInited = FALSE;
        return SY_ERROR_FAILURE;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        SY_WARNING_TRACE("CSyThreadManager::SocketStartup, version error!"
                         " wsaData.wVersion=" << wsaData.wVersion <<
                         " wsaData.wVersion=" << wsaData.wVersion <<
                         " err=" << ::WSAGetLastError());
        SocketCleanup();
        return SY_ERROR_NOT_AVAILABLE;
    }
#endif // SY_WIN32
    return SY_OK;
}

SyResult CSyThreadManager::SocketCleanup()
{
#ifdef SY_WIN32
    if (!s_bSocketInited) {
        return SY_OK;
    }
    s_bSocketInited = FALSE;

    if (::WSACleanup() != 0) {
        SY_WARNING_TRACE("CSyThreadManager::SocketCleanup, WSACleanup() failed!"
                         " err=" << ::WSAGetLastError());
        return SY_ERROR_UNEXPECTED;
    }
#endif // SY_WIN32
    return SY_OK;
}

//#define SY_NETWORK_THREAD_SINGLE_MAIN

CSyThreadManager::TModule CSyThreadManager::GetNetworkThreadModule()
{
#ifdef SY_NETWORK_THREAD_SINGLE_MAIN
    return TM_SINGLE_MAIN;
#else
    return TM_MULTI_ONE_DEDICATED;
#endif // SY_NETWORK_THREAD_SINGLE_MAIN
}


SyResult CSyThreadManager::AddLink(CSyString strIp, WORD wPort, DWORD dwUDP)
{
    /*! define a add not event */
    class CAddLinkEvent : public ISyEvent
    {
    public:
        CAddLinkEvent(CSyString strIp, WORD wPort, DWORD dwUDP)
            :m_strIP(strIp)
            ,m_wPort(wPort)
            ,m_dwUDP(dwUDP)
        {
        }
        virtual SyResult OnEventFire()
        {
            CSyThreadManager *pManager = CSyThreadManager::Instance();
            if (pManager) {
                pManager->AddLink(m_strIP, m_wPort, m_dwUDP);
                return SY_OK;
            } else {
                return SY_ERROR_FAILURE;
            }
        }
    private:
        CSyString   m_strIP;
        WORD        m_wPort;
        DWORD       m_dwUDP;
    };


    if (!m_pNetworkThread) {
        m_pNetworkThread = this->GetThread(TT_NETWORK);
    }
    ASyThread *pCurrThread =GetThread(TT_CURRENT);

    if (pCurrThread == m_pNetworkThread) {
        CListenElement ele(strIp, wPort, dwUDP);
        LinkSummaryMap::iterator it = m_LinkInfo.find(ele);
        if (it == m_LinkInfo.end()) { //had not find it
            AddNode(strIp, wPort, dwUDP);
            it = m_LinkInfo.find(ele);
            if (it == m_LinkInfo.end()) {
                /*
                CSyInetAddr Addr;
                Addr.SetIpAddrBy4Bytes(dwIp);
                Addr.SetPort(wPort);
                */
                SY_ERROR_TRACE_THIS("CSyThreadManager::AddLink have no node find this item,IP = " <<strIp<< ", wPort = " << wPort << ", IsUDP = " << dwUDP);
                return SY_ERROR_FAILURE;
            }
        }
        it->second.Increase();
        return SY_OK;
    } else {
        SY_ASSERTE_RETURN(m_pNetworkThread, SY_ERROR_UNEXPECTED);
        ISyEvent *pEvent = new CAddLinkEvent(strIp, wPort, dwUDP);
        m_pNetworkThread->GetEventQueue()->PostEvent(pEvent);
        return SY_OK;
    }
}

SyResult CSyThreadManager::RemoveLink(CSyString strIp, WORD wPort, DWORD dwUDP)
{
    /*! define a add not event */
    class CRemoveLinkEvent : public ISyEvent
    {
    public:
        CRemoveLinkEvent(CSyString strIp, WORD wPort, DWORD dwUDP)
            :m_strIP(strIp)
            ,m_wPort(wPort)
            ,m_dwUDP(dwUDP)
        {
        }
        virtual SyResult OnEventFire()
        {
            CSyThreadManager *pManager = CSyThreadManager::Instance();
            if (pManager) {
                pManager->RemoveLink(m_strIP, m_wPort, m_dwUDP);
                return SY_OK;
            } else {
                return SY_ERROR_FAILURE;
            }
        }
    private:
        CSyString   m_strIP;
        WORD        m_wPort;
        DWORD       m_dwUDP;
    };



    if (!m_pNetworkThread) {
        m_pNetworkThread = this->GetThread(TT_NETWORK);
    }
    ASyThread *pCurrThread =GetThread(TT_CURRENT);

    if (pCurrThread == m_pNetworkThread) {
        CListenElement ele(strIp, wPort, dwUDP);
        LinkSummaryMap::iterator it = m_LinkInfo.find(ele);
        if (it == m_LinkInfo.end()) { //had not find it
            /*
            CSyInetAddr Addr;
            Addr.SetIpAddrBy4Bytes(dwIp);
            Addr.SetPort(wPort);
            */
            SY_WARNING_TRACE("CSyThreadManager::RemoveLink have not find this item,IP = " <<strIp<< ", wPort = "<<wPort<<", IsUDP = " <<dwUDP);
            return SY_ERROR_FAILURE;
        }
        it->second.Decrease();
        return SY_OK;
    } else {
        SY_ASSERTE_RETURN(m_pNetworkThread, SY_ERROR_UNEXPECTED);
        ISyEvent *pEvent = new CRemoveLinkEvent(strIp, wPort, dwUDP);
        m_pNetworkThread->GetEventQueue()->PostEvent(pEvent);
        return SY_OK;
    }

}

SyResult CSyThreadManager::AddNode(CSyString strIp, WORD wPort, DWORD dwUDP)
{
    /*! define a add not event */
    class CAddNodeEvent : public ISyEvent
    {
    public:
        CAddNodeEvent(CSyString strIp, WORD wPort, DWORD dwUDP)
            :m_strIP(strIp)
            ,m_wPort(wPort)
            ,m_dwUDP(dwUDP)
        {
        }
        virtual SyResult OnEventFire()
        {
            CSyThreadManager *pManager = CSyThreadManager::Instance();
            if (pManager) {
                pManager->AddNode(m_strIP, m_wPort, m_dwUDP);
                return SY_OK;
            } else {
                return SY_ERROR_FAILURE;
            }
        }
    private:
        CSyString   m_strIP;
        WORD        m_wPort;
        DWORD       m_dwUDP;
    };

    if (!m_pNetworkThread) {
        m_pNetworkThread = this->GetThread(TT_NETWORK);
    }
    ASyThread *pCurrThread =GetThread(TT_CURRENT);

    /*
    CSyInetAddr Addr;
    Addr.SetIpAddrBy4Bytes(dwIp);
    Addr.SetPort(wPort);
    */
    SY_INFO_TRACE_THIS("CSyThreadManager::AddNode dwIP = " << strIp << ", wPort = " << wPort <<", UDP = " << dwUDP);
    if (m_pNetworkThread == pCurrThread) {
        CSyMutexGuardT<CSyMutexThreadRecursive> autolock(m_LinkSumMutex);
        CListenElement ele(strIp, wPort, dwUDP);
        LinkSummaryMap::iterator it = m_LinkInfo.find(ele);
        if (it == m_LinkInfo.end()) { //had not find it
            CLinkSummary node;
            m_LinkInfo[ele] = node;
            return SY_OK;
        }
        SY_ERROR_TRACE_THIS("CSyThreadManager::AddNode the node already available,IP = "<< strIp<< ", wPort = " << wPort << ", IsUDP = " << dwUDP);
        return SY_ERROR_FAILURE;
    } else { //should post a message to network thread do it
        SY_ASSERTE_RETURN(m_pNetworkThread, SY_ERROR_UNEXPECTED);
        ISyEvent *pEvent = new CAddNodeEvent(strIp, wPort, dwUDP);
        m_pNetworkThread->GetEventQueue()->PostEvent(pEvent);
        return SY_OK;
    }
}

SyResult CSyThreadManager::RemoveNode(CSyString strIp, WORD wPort, DWORD dwUDP)
{

    /*! define a remove not event */
    class CRemoveNodeEvent : public ISyEvent
    {
    public:
        CRemoveNodeEvent(CSyString strIp, WORD wPort, DWORD dwUDP)
            :m_strIP(strIp)
            ,m_wPort(wPort)
            ,m_dwUDP(dwUDP)
        {
        }
        virtual SyResult OnEventFire()
        {
            CSyThreadManager *pManager = CSyThreadManager::Instance();
            if (pManager) {
                pManager->RemoveNode(m_strIP, m_wPort, m_dwUDP);
                return SY_OK;
            } else {
                return SY_ERROR_FAILURE;
            }
        }
    private:
        CSyString   m_strIP;
        WORD        m_wPort;
        DWORD       m_dwUDP;
    };
    if (!m_pNetworkThread) {
        m_pNetworkThread = this->GetThread(TT_NETWORK);
    }
    ASyThread *pCurrThread =GetThread(TT_CURRENT);

    /*
    CSyInetAddr Addr;
    Addr.SetIpAddrBy4Bytes(strIp);
    Addr.SetPort(wPort);
    */
    SY_INFO_TRACE_THIS("CSyThreadManager::RemoveNode IP = " << strIp << ", wPort = " << wPort <<", UDP = " << dwUDP);
    if (m_pNetworkThread == pCurrThread) {
        CSyMutexGuardT<CSyMutexThreadRecursive> autolock(m_LinkSumMutex);
        CListenElement ele(strIp, wPort, dwUDP);
        LinkSummaryMap::iterator it = m_LinkInfo.find(ele);
        if (it != m_LinkInfo.end()) { //had not find it
            m_LinkInfo.erase(it);
            return SY_OK;
        }
        SY_WARNING_TRACE_THIS("CSyThreadManager::RemoveNode the node is not found,IP = " <<strIp<< ", wPort = "<<wPort << ", IsUDP = " << dwUDP);
        return SY_ERROR_FAILURE;
    } else { //should post a message to network thread do it
        SY_ASSERTE_RETURN(m_pNetworkThread, SY_ERROR_UNEXPECTED);
        ISyEvent *pEvent = new CRemoveNodeEvent(strIp, wPort, dwUDP);
        m_pNetworkThread->GetEventQueue()->PostEvent(pEvent);
        return SY_OK;
    }

}

CSyThreadManager::CLinkSummary::CLinkSummary():m_dwValue(0)
{
    //SY_INFO_TRACE_THIS("CSyThreadManager::CLinkSummary::CLinkSummary()");
}
/*
CSyThreadManager::CLinkSummary::~CLinkSummary()
{
    //SY_INFO_TRACE_THIS("CSyThreadManager::CLinkSummary::~CLinkSummary()");
}
*/
void CSyThreadManager::CLinkSummary::Increase()
{
#ifdef OUTPUT_DEBUG
    SY_INFO_TRACE_THIS("CSyThreadManager::CLinkSummary::Increase() m_dwValue = " << m_dwValue);
#endif
    ++m_dwValue;
}

void CSyThreadManager::CLinkSummary::Decrease()
{
#ifdef OUTPUT_DEBUG
    SY_INFO_TRACE_THIS("CSyThreadManager::CLinkSummary::Decrease() m_dwValue = " << m_dwValue);
#endif
    SY_ASSERTE_RETURN_VOID(m_dwValue > 0);
    --m_dwValue;
}

static uint32_t g_uGlobalRefInitUtiltp = 0;
static CSyMutexThreadRecursive	g_MutexInitUtiltp;

bool sy_util_init()
{
    CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(g_MutexInitUtiltp);
    if(g_uGlobalRefInitUtiltp>0){
        g_uGlobalRefInitUtiltp++;
        SY_INFO_TRACE("sy_util_uninit, RefCount="<<g_uGlobalRefInitUtiltp);
        return true;
    }
    CSyThreadManager::Instance();
    
    g_uGlobalRefInitUtiltp = 1;
    SY_INFO_TRACE("sy_util_uninit, RefCount="<<g_uGlobalRefInitUtiltp);
   return true;

}

bool sy_util_uninit()
{
    CSyMutexGuardT<CSyMutexThreadRecursive> theGuard(g_MutexInitUtiltp);
    if(g_uGlobalRefInitUtiltp==0){
        SY_WARNING_TRACE("Call sy_util_uninit when RefCount is 0");
        return false;
    }
    g_uGlobalRefInitUtiltp--;
    if(g_uGlobalRefInitUtiltp>0){
        SY_INFO_TRACE("sy_util_uninit, RefCount="<<g_uGlobalRefInitUtiltp);
        return true;
    }
    
    CSyThreadManager::CleanupOnlyOne();
    SY_INFO_TRACE("sy_util_uninit, RefCount="<<g_uGlobalRefInitUtiltp);
    return true;
 
}

