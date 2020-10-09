
#include "SyThread.h"
#include "SyThreadManager.h"
#include "SyConditionVariable.h"
#include "SyDebug.h"

#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

#include "SyUtilMisc.h"

#if defined (WP8 ) || defined (UWP)
    #include "ThreadEmulation.h"
    using namespace ThreadEmulation;
#endif

#ifdef SY_ANDROID
    #include "SyUtilJNI.h"
#endif

USE_UTIL_NS

///////////////////////////////////////////////////////////////
// class ASyThread
///////////////////////////////////////////////////////////////

ASyThread::ASyThread()
    : m_Tid(0)
    , m_Handle((SY_THREAD_HANDLE)SY_INVALID_HANDLE)
    , m_Type(TT_UNKNOWN)
    , m_Flag(TF_NONE)
    , m_pEvent4Start(NULL)
    , m_bRegistered(FALSE)
    , m_bRegister(TRUE)
    , m_bStopFlag(FALSE)
{
}

ASyThread::~ASyThread()
{
    if (m_bRegister) {
        if (m_bRegistered) {
            UnRegisterThread(this);
            m_bRegistered = FALSE;
        }
    }
#ifdef SY_WIN32
    if (m_Handle != NULL) {
        CloseHandle(m_Handle);
    }
    m_Handle = NULL;
#endif
}

void ASyThread::SetStop()
{
    m_bStopFlag = TRUE;
}

BOOL ASyThread::GetStopFlag()
{
    return m_bStopFlag;
}

void ASyThread::Terminate()
{
#ifdef SY_WIN32
    CloseHandle(GetThreadHandle());
    m_Handle = nullptr;
#elif !defined(SY_ANDROID)
    pthread_cancel(this->GetThreadId());
#endif
    m_bStopFlag = TRUE;
}

SyResult ASyThread::Create(const char *name, TType aType, TFlag aFlag, BOOL Register)
{
    SY_INFO_TRACE_THIS("ASyThread::Create,"
                       " aType=" << aType <<
                       " aFlag=" << aFlag <<
                       " Register=" << Register << ", name=" << (name ? name : ""));

    SY_ASSERTE_RETURN(m_Type == TT_UNKNOWN, SY_ERROR_ALREADY_INITIALIZED);
    if (aType == TT_UNKNOWN) {
        SY_WARNING_TRACE("thread type is TT_UNKNOWN");
    }

    if (name) {
        m_name = name;
    } else {
        m_name = "ASyThread";
    }

    m_Type = aType;
    m_Flag = aFlag;
    if (m_Flag == TF_NONE) {
        m_Flag = TF_JOINABLE;
    }

    if (m_Type == TT_MAIN) {
        // We have to assume the current thread is main thread.
        m_Tid = GetThreadSelfId();
#if defined(SY_IOS) || defined(SY_MACOS)
        pthread_setname_np(GetName());
#elif defined(SY_ANDROID) && defined(SY_LINUX)
        pthread_setname_np(pthread_self(),GetName());
#elif defined(SY_WIN32)
        SetThreadName();
#endif
    } else {
        SY_ASSERTE(!m_pEvent4Start);
        m_pEvent4Start = new CSyEventThread();
        if (!m_pEvent4Start) {
            return SY_ERROR_OUT_OF_MEMORY;
        }

#ifdef SY_WIN_PHONE
        m_Handle = ThreadEmulation::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, this, 0, NULL);
        if (m_Handle == 0) {
            SY_ERROR_TRACE_THIS("ThreadEmulation::CreateThread, CreateThread() failed! err=" << errno);
            return SY_ERROR_UNEXPECTED;
        }
#elif defined(SY_WIN_DESKTOP)
        m_Handle = (HANDLE)::_beginthreadex(
                       NULL,
                       0,
                       ThreadProc,
                       this,
                       0,
                       (unsigned int *)(&m_Tid));
        if (m_Handle == 0) {
            SY_ERROR_TRACE_THIS("ASyThread::Create, _beginthreadex() failed! err=" << errno);
            return SY_ERROR_UNEXPECTED;
        }
#else // !SY_WIN32
        pthread_attr_t attr;
        int nRet;
        if ((nRet = ::pthread_attr_init(&attr)) != 0) {
            SY_ERROR_TRACE_THIS("ASyThread::Create, pthread_attr_init() failed! err=" << nRet);
            return SY_ERROR_UNEXPECTED;
        }

        int dstate = PTHREAD_CREATE_JOINABLE;
        if (SY_BIT_ENABLED(m_Flag, TF_JOINABLE)) {
            dstate = PTHREAD_CREATE_JOINABLE;
        } else if (SY_BIT_ENABLED(m_Flag, TF_DETACHED)) {
            dstate = PTHREAD_CREATE_DETACHED;
        }
        if ((nRet = ::pthread_attr_setdetachstate(&attr, dstate)) != 0) {
            SY_ERROR_TRACE_THIS("ASyThread::Create, pthread_attr_setdetachstate() failed! err=" << nRet);
            ::pthread_attr_destroy(&attr);
            return SY_ERROR_UNEXPECTED;
        }

        if ((nRet = ::pthread_create(&m_Tid, &attr, ThreadProc, this)) != 0) {
            SY_ERROR_TRACE_THIS("ASyThread::Create, pthread_create() failed! err=" << nRet);
            ::pthread_attr_destroy(&attr);
            return SY_ERROR_UNEXPECTED;
        }
        ::pthread_attr_destroy(&attr);
        m_Handle = m_Tid;
#endif // SY_WIN32
#ifdef SY_WIN32
        CSyTimeValue tm(120,0);
        if (SY_FAILED(m_pEvent4Start->Wait(&tm))) {
            SY_ERROR_TRACE_THIS("ASyThread::Create m_pEvent4Start->Wait 120s timeout !!!");
        }
#else
        m_pEvent4Start->Wait();
#endif

        delete m_pEvent4Start;
        m_pEvent4Start = NULL;
    }

    m_bRegister = Register;

    SyResult rv = SY_OK;

    if (m_bRegister) {
        rv = SY_ERROR_FAILURE;
        rv = RegisterThread(this);
        if (SY_SUCCEEDED(rv)) {
            m_bRegistered = TRUE;
        } else {
            Stop();
            Join();
        }
    }
    return rv;
}

bool ASyThread::IsDetached()
{
    return SY_BIT_ENABLED(m_Flag, TF_DETACHED);
}

SyResult ASyThread::Destory(SyResult aReason)
{
    SY_INFO_TRACE_THIS("ASyThread::Destory, aReason=" << aReason);

    if (m_bRegister) {
        if (m_bRegistered) {
            UnRegisterThread(this);
            m_bRegistered = FALSE;
        }
    }
    if (SY_BIT_DISABLED(m_Flag, TF_JOINABLE) &&
            ++m_NeedDelete < 2) {
        return SY_OK;
    }

    delete this;
    return SY_OK;
}

#ifdef SY_WIN32
    unsigned WINAPI ASyThread::ThreadProc(void *aPara)
#else
    void *ASyThread::ThreadProc(void *aPara)
#endif // SY_WIN32
{
    ASyThread *pThread = static_cast<ASyThread *>(aPara);
    SY_ASSERTE_RETURN(pThread, NULL);
#ifdef SY_ANDROID
    CSyJvmThreadStub jvmStub(pThread->GetName());
#endif
#if defined (WP8 ) || defined (UWP)
    pThread->SetThreadId(GetCurrentThreadId());
#endif //~WP8
#if defined(SY_IOS) || defined(SY_MACOS)
    pthread_setname_np(pThread->GetName());
#elif defined(SY_ANDROID) || defined(SY_LINUX)
    if(0!=pthread_setname_np(pthread_self(),pThread->GetName())){
        SY_ERROR_TRACE("pthread_setname_np failed,name="<<pThread->GetName()<<",tid="<<SyGetThreadID());
    }
#elif defined(SY_WIN32)
    pThread->SetThreadName();
#endif

    pThread->OnThreadInit();
    if (pThread->m_Type != TT_MAIN) {
        SY_ASSERTE(pThread->m_pEvent4Start);
        if (pThread->m_pEvent4Start) {
            pThread->m_pEvent4Start->Signal();
        }
    }

    pThread->OnThreadRun();
    
    bool ifDelete = (SY_BIT_DISABLED(pThread->m_Flag, TF_JOINABLE) &&
                     ++pThread->m_NeedDelete >= 2);

    pThread->SetStop();
    
    if (ifDelete) {
        delete pThread;
    }

    return NULL;
}

SyResult ASyThread::Stop(CSyTimeValue *aTimeout)
{
    SY_ASSERTE(!"ASyThread::Stop");
    return SY_ERROR_NOT_IMPLEMENTED;
}

SyResult ASyThread::Join()
{
    if (IsEqualCurrentThread(m_Tid)) {
        return SY_ERROR_FAILURE;
    }

#ifdef SY_WIN32
    int i = 0;
    while (!GetStopFlag() && (i++ < 5000)) {
        SleepMs(10);
    }
    SY_INFO_TRACE_THIS("GetStopFlag()=" << GetStopFlag());
    return SY_OK;
#else
    /*! the thread already stopped or not joinable*/
    if (0 == m_Tid || SY_BIT_DISABLED(m_Flag, TF_JOINABLE)) {
        return SY_OK;
    }
    
    void *pThreadReturn;
    int nRet = ::pthread_join(m_Tid, &pThreadReturn);
    if (nRet != 0) {
        SY_ERROR_TRACE_THIS("ASyThread::Join, pthread_join() failed! err=" << nRet);
    }else {
        m_Tid = 0;
    }
    
    while (!GetStopFlag()) {
        usleep(50000);
        //SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    }
    SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    return SY_OK;
    
    /*
#if defined(SY_ANDROID) || defined(SY_IOS)
    void *pThreadReturn;
    int nRet = ::pthread_join(m_Tid, &pThreadReturn);
    if (nRet != 0) {
        SY_ERROR_TRACE_THIS("ASyThread::Join, pthread_join() failed! err=" << nRet);
    }else {
        m_Tid = 0;
    }
    
    while (!GetStopFlag()) {
        usleep(50000);
        //SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    }
    SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    return SY_OK;
#else
     void *pThreadReturn;
     int nRet = ::pthread_join(m_Tid, &pThreadReturn);
     if (nRet != 0) {
        SY_ERROR_TRACE_THIS("ASyThread::Join, pthread_join() failed! err=" << nRet);
        return SY_ERROR_FAILURE;
     } else {
        m_Tid = 0;
        return SY_OK;
     }

#endif
    /*
#if defined(SY_ANDROID) || defined(SY_IOS)
    // Is there any evidence of pthread_join doesn't work on android and ios?
    // I didn't find anything on google, is it possible there is bug in other part?
    // weichen2, commented at 2015/7/29
    while (!GetStopFlag()) {
        usleep(50000);
        //SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    }
    SY_INFO_TRACE_THIS("GetStopFlag()="<<GetStopFlag());
    return SY_OK;
#else
     void *pThreadReturn;
     int nRet = ::pthread_join(m_Tid, &pThreadReturn);
     if (nRet != 0) {
        SY_ERROR_TRACE_THIS("ASyThread::Join, pthread_join() failed! err=" << nRet);
        return SY_ERROR_FAILURE;
     } else {
        m_Tid = 0;
        return SY_OK;
     }

#endif //SY_ANDROID || SY_IOS
*/

#endif // SY_WIN32
}

SY_THREAD_ID ASyThread::GetThreadId()
{
    return m_Tid;
}

TType ASyThread::GetThreadType()
{
    return m_Type;
}

SY_THREAD_HANDLE ASyThread::GetThreadHandle()
{
    return m_Handle;
}

ISyReactor *ASyThread::GetReactor()
{
    return NULL;
}

ISyEventQueue *ASyThread::GetEventQueue()
{
    return NULL;
}

ISyTimerQueue *ASyThread::GetTimerQueue()
{
    return NULL;
}

void ASyThread::OnThreadInit()
{
#ifdef SY_WIN32
    srand(time(NULL) + m_Tid);
#endif
}

#ifdef SY_WIN32
typedef HRESULT(__stdcall *SYSetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

void ASyThread::SetThreadName()
{
    HINSTANCE hKernel32 = LoadLibrary("kernel32.dll");
    if (hKernel32 != NULL) {
        SYSetThreadDescription lpSetThreadDescription = (SYSetThreadDescription)GetProcAddress(hKernel32, "SetThreadDescription");
        if (lpSetThreadDescription != NULL) {
            wchar_t name[260] = { 0 };
            MultiByteToWideChar(CP_UTF8, 0, m_name.c_str(), m_name.length(), name, 259);
            if (m_Handle == SY_INVALID_HANDLE) {
                // main thread, use current thread
                SY_THREAD_HANDLE handle = OpenThread(THREAD_ALL_ACCESS, FALSE, m_Tid);
                if (handle) {
                    lpSetThreadDescription(handle, name);
                    CloseHandle(handle);
                }
            }
            else {
                lpSetThreadDescription(m_Handle, name);
            }
        }
        FreeLibrary(hKernel32);
        hKernel32 = NULL;
    }

}
#endif
