#ifndef SYTHREADMANAGER_H
#define SYTHREADMANAGER_H

#include "SyMutex.h"
#include "SyStdCpp.h"

#include <vector>

START_UTIL_NS

class CSyTimeValue;
class ASyThread;
class ISyReactor;
class ISyEventQueue;
class ISyTimerQueue;

/**
 *  CSyThreadManager must be declared in the stack of main fuction!
 *  For example for the usage:
 *    int main(int argc, char** argv)
 *    {
 *      CSyThreadManager theThreadManager;
 *      theThreadManager.InitMainThread(argc, argv);
 *      theThreadManager.GetThread(CSyThreadManager::MAIN)->OnThreadRun();
 *    }
 */
class SY_OS_EXPORT CSyThreadManager
{
public:
    /*!
    \brief the key of the server transport summary
    */
    struct SY_OS_EXPORT CListenElement {
        CListenElement(CSyString strIPAddr, WORD wPort, DWORD dwUDP)
            : m_strIPAddr(strIPAddr)
            , m_wPort(wPort)
            , m_dwUDP(dwUDP)
        {
        }

        bool operator < (const CListenElement &aRight) const
        {
            int ipret = 0;

            ipret = ::memcmp(m_strIPAddr.c_str(), aRight.m_strIPAddr.c_str(), strlen(m_strIPAddr.c_str()));

            if (ipret < 0) {
                return TRUE;
            } else if (ipret == 0 && m_wPort < aRight.m_wPort) {
                return TRUE;
            } else if (ipret == 0 && m_wPort == aRight.m_wPort && m_dwUDP < aRight.m_dwUDP) {
                return TRUE;
            }
            return FALSE;
        }

        CSyString   m_strIPAddr;        /*! the IP address */
        WORD        m_wPort;        /*! the indicate the protocol of the transport UDP should be (1), otherwise is (0) */
        DWORD       m_dwUDP;
    };

    /*!
        \brief the definition for a monitor information
    */
    class SY_OS_EXPORT CLinkSummary
    {
    public:
        CLinkSummary();
        ~CLinkSummary() {};

        void Increase();

        void Decrease();

        DWORD LinkNum() const
        {
            return m_dwValue;
        }

    private:
        DWORD m_dwValue;
    };
    typedef std::map<CListenElement, CLinkSummary > LinkSummaryMap; /*! a type definition for link information */

public:

    CSyThreadManager();
    ~CSyThreadManager();

    static CSyThreadManager *Instance();
    static void EnableHeartbeat(bool bUseHeartbeat);
    static bool m_bUseHeartbeat;
    static void CleanupOnlyOne();

    SyResult InitMainThread(int aArgc, char **aArgv);
    SyResult SpawnNetworkThread_i(TType aType, const char *szLabel, ASyThread *&pTread);

    ASyThread *GetThread(TType aType);
    ISyReactor *GetThreadReactor(TType aType);
    ISyEventQueue *GetThreadEventQueue(TType aType);
    ISyTimerQueue *GetThreadTimerQueue(TType aType);

    // create Reactor Thread (mainly for network)
    //  SyResult CreateUserReactorThread(ASyThread *&aThread);

    // create Task Thread (include EventQueue and TimerQueue only)
    ///Add a TType parameters to let user can specify the thread type,
    //if use default value, the type will set by system automatic
    SyResult CreateUserTaskThread(
        const char *name,
        ASyThread *&aThread,
        TFlag aFlag = TF_JOINABLE,
        BOOL bWithTimerQueue = TRUE, TType aType = TT_UNKNOWN);

    // get user thread type (TT_USER_DEFINE_BASE + x)
    TType GetAndIncreamUserType();

    static void SleepMs(DWORD aMsec);

public:
    // the following member functions are mainly used in TP.
    SyResult CreateReactorThread(const char *name, TType aType, ISyReactor *aReactor, ASyThread *&aThread);

    // mainly invoked by CSyThreadManager::~CSyThreadManager()
    SyResult StopAllThreads(CSyTimeValue *aTimeout = NULL);
    SyResult JoinAllThreads();

    // mainly invoked by ASyThread::Create().
    SyResult RegisterThread(ASyThread *aThread);

    SyResult UnregisterThread(ASyThread *aThread);

    void GetSingletonMutex(CSyMutexThreadRecursive *&aMutex)
    {
        aMutex = &m_SingletonMutex;
    }

    void GetReferenceControlMutex(CSyMutexThread *&aMutex)
    {
        aMutex = &m_ReferenceControlMutexThread;
    }

    // thread module
    enum TModule {
        TM_SINGLE_MAIN,
        TM_MULTI_ONE_DEDICATED,
        TM_MULTI_POOL
    };
    static TModule GetNetworkThreadModule();

    static SY_THREAD_ID GetThreadSelfId()
    {
#ifdef WIN32
        return ::GetCurrentThreadId();
#else
        return ::pthread_self();
#endif // WIN32
    }

    static BOOL IsThreadEqual(SY_THREAD_ID aT1, SY_THREAD_ID aT2)
    {
#ifdef WIN32
        return aT1 == aT2;
#else
#if defined (MACOS) && !defined(MachOSupport)
        return CFM_pthread_equal(aT1, aT2);
#else
        return ::pthread_equal(aT1, aT2);
#endif
#endif // WIN32
    }

    static BOOL IsEqualCurrentThread(SY_THREAD_ID aId)
    {
        return IsThreadEqual(aId, GetThreadSelfId());
    }

    static BOOL IsEqualCurrentThread(TType aType);

    // mainly for init winsock2
    static SyResult SocketStartup();
    static SyResult SocketCleanup();

    static ISyReactor *CreateNetworkReactor();


    //////////////////////////////////////////////////////////////////////////
    void GetLinkSummaryInfo(LinkSummaryMap &SumInfo)
    {
        CSyMutexGuardT<CSyMutexThreadRecursive> autolock(m_LinkSumMutex);
        SumInfo = m_LinkInfo;
    }

    //all of them should work in the network thread
    SyResult AddLink(CSyString strIp, WORD wPort, DWORD dwUDP);
    SyResult RemoveLink(CSyString strIp, WORD wPort, DWORD dwUDP);

    SyResult AddNode(CSyString strIp, WORD wPort, DWORD dwUDP);
    SyResult RemoveNode(CSyString strIp, WORD wPort, DWORD dwUDP);
    //////////////////////////////////////////////////////////////////////////

    static void EnsureThreadManagerExited();

private:
#ifdef WIN32
    static BOOL s_bSocketInited;
#endif // WIN32

    // singleton mutex is recursive due to CSyCleanUpBase() will lock it too.
    CSyMutexThreadRecursive m_SingletonMutex;
    CSyMutexThread m_ReferenceControlMutexThread;

    typedef std::vector<ASyThread *> ThreadsType;
    ThreadsType m_Threads;
    typedef CSyMutexThread MutexType ;
    MutexType m_Mutex;
    TType m_TTpyeUserThread;

protected:
    static void CleanupOnlyOneInternal();

private:
    // = Prevent assignment and initialization.
    void operator = (const CSyThreadManager &);
    CSyThreadManager(const CSyThreadManager &);

    /*! add to count network link information*/
    LinkSummaryMap m_LinkInfo;
    CSyMutexThreadRecursive m_LinkSumMutex;

    ASyThread *m_pNetworkThread;
};

END_UTIL_NS

#endif // !SYTHREADMANAGER_H
