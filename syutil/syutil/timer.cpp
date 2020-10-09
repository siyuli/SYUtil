
#include "timer_internal.h"
#include "SyThread.h"

#include <chrono>


START_UTIL_NS

static uint64_t get_tick_count_us();

PretInitialationTimer::PretInitialationTimer()
{
#ifdef SY_WIN32
    QueryPerformanceFrequency(&m_InitialValue);
    LARGE_INTEGER tv;
    if (!QueryPerformanceCounter(&tv)) {
        m_startTmfromS = (static_cast<int64_t>(clock()) * us_second)/ CLOCKS_PER_SEC;
    } else {
        m_startTmfromS = static_cast<int64_t>((((double)tv.QuadPart * ms_second) / (double)m_InitialValue.QuadPart) * ms_second);
    }
    m_startTmfromC = static_cast<int64_t>(time(NULL)) * us_second;
#else
    m_InitialValue = (INIT_TYPE)((double)ms_second) /  sysconf(_SC_CLK_TCK);

    SY_INFO_TRACE("PretInitialationTimer::PretInitialationTimer: _SC_CLK_TCK="<<sysconf(_SC_CLK_TCK));

#ifdef _UNIX_HIGH_PRECISION
    struct timeval tv;
    struct tms tm;
    gettimeofday(&tv, 0);
    m_startTmfromC = static_cast<int64_t>(tv.tv_sec) * us_second + tv.tv_usec;
    m_startTmfromS = (static_cast<int64_t>(times(&tm)) * m_InitialValue) * ms_second;
    m_startMillis = m_startTmfromS / us_second;
#endif
#endif
}

int64_t timer_policy::max_time_value()
{
    return MAX_INT64_PLUS_VAL;
}

uint64_t timer_policy::now()
{
#ifdef SY_WIN32
    LARGE_INTEGER tv;
    PretInitialationTimer *pPretInitialationTimer = CSySingletonT<PretInitialationTimer>::Instance();
    SY_ASSERTE_RETURN(pPretInitialationTimer, 0);

    if (!QueryPerformanceCounter(&tv)) {
        return (static_cast<int64_t>(clock()) * us_second)/ CLOCKS_PER_SEC - pPretInitialationTimer->m_startTmfromS +
               pPretInitialationTimer->m_startTmfromC;
    } else {
        return static_cast<int64_t>((((double)tv.QuadPart * ms_second) / (double)pPretInitialationTimer->m_InitialValue.QuadPart) * ms_second)  -
               pPretInitialationTimer->m_startTmfromS +
               pPretInitialationTimer->m_startTmfromC;
    }
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (static_cast<int64_t>(tv.tv_sec) * us_second + tv.tv_usec);
#endif
}

int64_t tick_policy::max_time_value()
{
#ifdef SY_WIN32
    return MAX_TICK_COUNT_IN_US;
#else
    return MAX_INT64_PLUS_VAL;
#endif
}

uint64_t tick_policy::now()
{
#ifdef SY_WIN32
#if 0
    LARGE_INTEGER tv;
    if (!QueryPerformanceCounter(&tv)) {
        return (static_cast<int64_t>(clock()) * us_second)/ CLOCKS_PER_SEC;
    } else {
        return static_cast<int64_t>((((double)tv.QuadPart * ms_second) / (double)PretInitialationTimerSingleT::Instance()->m_InitialValue.QuadPart) * ms_second);
    }
#else
#if defined (WP8 ) || defined (UWP)
    int64_t _now = (int64_t)::GetTickCount64();
#else
    int64_t _now = (int64_t)::GetTickCount();
#endif
    return _now * ms_benchmark ;
#endif

#else
    struct tms tm;
#ifdef _UNIX_HIGH_PRECISION
    struct timeval tv;
    gettimeofday(&tv, 0);
    //first get milliseconds lapsed from PretInitialationTimerSingleT created
    int64_t _amend = (static_cast<int64_t>(tv.tv_sec) * us_second + tv.tv_usec -
                      PretInitialationTimerSingleT::Instance()->m_startTmfromC) % us_second;
    //correct it if timer has been adjusted
    if (_amend < 0) {
        _amend += us_second;
    }
    int64_t val = (static_cast<int64_t>(times(&tm)) * (PretInitialationTimerSingleT::Instance()->m_InitialValue));
    val -= (val % ms_second);
    val *= ms_second;
    val += (_amend + PretInitialationTimerSingleT::Instance()->m_startMillis);
    return val;
#elif defined(SY_ANDROID)
    // fix git issue #1344
    return get_tick_count_us();
#else
    return (static_cast<int64_t>(times(&tm)) * (PretInitialationTimerSingleT::Instance()->m_InitialValue)) * ms_second;
#endif
#endif
}

tick_type::tick_type()
{
    reset(tick_policy::now());
}

tick_type::tick_type(int64_t val)
{
    reset(val);
}

void tick_type::reset(int64_t val)
{
    tk_microsec = static_cast<DWORD>(val % ms_second);
    tk_millisec = static_cast<DWORD>((val / ms_second) % ms_second);
    tk_sec = static_cast<DWORD>((val / us_second) % 60);
    tk_min = static_cast<DWORD>((val / us_minute) % 60);
    tk_hour = static_cast<DWORD>(val / us_hour);
}


formatted_ticker::VALUE_TYPE formatted_ticker::now()
{
    tick_type lstore;
    return lstore;
}

time_type::time_type()
{
    int64_t localtm = timer_policy::now();
    reset(localtm);
}

time_type::time_type(int64_t val)
{
    reset(val);
}
void time_type::reset(int64_t val)
{
    tm_microsec = static_cast<DWORD>(val % ms_second);
    tm_millisec = static_cast<DWORD>((val / ms_second) % ms_second);
    time_t timer_sec = static_cast<time_t>(val / us_second);
#ifdef SY_WIN32
    //it is not thread safe on windows
    struct tm *tm_v = localtime(&timer_sec);
#else
    struct tm *tm_v, tmstore;
    tm_v = localtime_r(&timer_sec, &tmstore);
#endif
    if (tm_v) {
        tm_sec = tm_v->tm_sec;
        tm_min = tm_v->tm_min;
        tm_hour = tm_v->tm_hour;
        tm_mday = tm_v->tm_mday;
        tm_mon = tm_v->tm_mon + 1;
        tm_year = tm_v->tm_year + 1900;
        tm_wday = tm_v->tm_wday;
        tm_yday = tm_v->tm_yday;
        tm_isdst = tm_v->tm_isdst;
    } else {
        tm_sec = 0;
        tm_min = 0;
        tm_hour = 0;
        tm_mday = 0;
        tm_mon = 0;
        tm_year = 0;
        tm_wday = 0;
        tm_yday = 0;
        tm_isdst = 0;

    }
}

formatted_timer::VALUE_TYPE formatted_timer::now()
{
    time_type lstore;
    return lstore;
}

///
void low_tick_generator::OnTimer(CSyTimerWrapperID *aId)
{
    m_tickNow = tick_policy::now();
}

uint64_t low_tick_generator::now()
{
    if (!m_InitTimerSucc) {
        return tick_policy::now();
    }

    if (sizeof(int) < 4) { //it is not 32 bits system
        return tick_policy::now();
    }
    return m_tickNow;
}

low_tick_generator::low_tick_generator() {
    SY_INFO_TRACE_THIS("low_tick_generator::low_tick_generator");
    m_InitTimerSucc = false;
}

low_tick_generator::~low_tick_generator() {
    SY_INFO_TRACE_THIS("low_tick_generator::~low_tick_generator");
}

low_tick_generator *low_tick_generator::instance()
{
    return CSySingletonT<low_tick_generator>::Instance();
}

void low_tick_generator::start()
{
    SY_INFO_TRACE_THIS("low_tick_generator::start");
    m_tickNow = tick_policy::now();
    SyResult rv = m_RefreshTimer.Schedule(this, CSyTimeValue(0, MIN_INTERVAL));
    if (SY_FAILED(rv)) {
        SY_ERROR_TRACE_THIS("low_tick_generator::start, FAILED to schedule ticket timer in t-tick thread. rv = " << rv);
        m_InitTimerSucc = false;
    } else {
        m_InitTimerSucc = true;
    }
}

void low_tick_generator::stop()
{
    SY_INFO_TRACE_THIS("low_tick_generator::stop");
    m_RefreshTimer.Cancel();
    m_InitTimerSucc = false;
}

int64_t low_tick_policy::max_time_value()
{
    return tick_policy::max_time_value();
}

uint64_t low_tick_policy::now()
{
    return low_tick_generator::instance()->now();
}

void low_tick_policy::start()
{
    low_tick_generator::instance()->start();
}

void low_tick_policy::stop()
{
    low_tick_generator::instance()->stop();
}


ASyThread *g_tickThread = NULL;
CSyMutexThreadRecursive g_tickThreadMutex;
int refNum = 0;

void startLowTick(void)
{
    MutexGuard guard(g_tickThreadMutex);

    if (g_tickThread) {
        SY_WARNING_TRACE("startLowTick: g_tickThread has been initialized");
        refNum++;
        return;
    }

    SyResult ret = ::CreateUserTaskThread("t-tick", g_tickThread);
    if (SY_FAILED(ret)) {
        SY_ERROR_TRACE("startLowTick: failed to create a user thread, " << ret);
        return;
    }

    ISyEvent *startTickEvent = new CTickEvent();
    if (startTickEvent) {
        SyResult ret = g_tickThread->GetEventQueue()->SendEvent(startTickEvent);
        if (SY_FAILED(ret)) {
            SY_ERROR_TRACE("startLowTick: failed to send start tick event to tick thread, " << ret);
            return;
        }
    }
    refNum++;
}

void stopLowTick(void)
{
    MutexGuard guard(g_tickThreadMutex);

    refNum--;

    if (refNum == 0) {
        if (g_tickThread) {

            low_tick_policy::stop();

            g_tickThread->Stop();
            g_tickThread->Join();
            g_tickThread->Destory(SY_OK);
            g_tickThread = NULL;
        }
    }

    if (refNum < 0) { refNum = 0; }
}

#ifdef SY_WIN32
void setThreadAffinity()
{
//#if defined (WP8 ) || defined (UWP)
    //#pragma message(UTIL_INCLUDE_FILE_AND_LINE("Todo: need to implement"))
//#else
	//The more threads use one CPU, remove it.
    //if (0 == ::SetThreadAffinityMask(GetCurrentThread(), 1)) {
    //    SY_WARNING_TRACE("timer_fact::timer_fact let the thread run on a fixed CPU, thread ID = " << GetCurrentThreadId() << " failed, errno = " << ::GetLastError());
    //} else {
    //    SY_INFO_TRACE("timer_fact::timer_fact let the thread run on a fixed CPU, thread ID = " << GetCurrentThreadId());
    //}
//#endif //~WP8 || ~UWP
}

bool checkIsTimeValid(int64_t remain, int32_t units)
{
    int64_t MAX_SUPPORT_CYCLE_S = ((int64_t)31536000) * units; //365 * 24 * 3600
    if (remain >= MAX_SUPPORT_CYCLE_S) { //only support over less than a year
        SY_WARNING_TRACE("timer_fact::elapsed_sec over one year, no support, remain = " << remain);
        return false;
    }
    return true;
}

#endif


#ifndef SY_WIN32
unsigned long get_tick_count()
{
    unsigned long   ret;
    struct  timeval time_val;

    gettimeofday(&time_val, NULL);
    ret = time_val.tv_sec * 1000 + time_val.tv_usec / 1000;

    return ret;
}
#endif //SY_WIN32

void output_debug_string(char *)
{
    //    T120_LOG(str);
}

static uint64_t get_tick_count_us()
{
    std::chrono::steady_clock::time_point _now = std::chrono::steady_clock::now();
    std::chrono::microseconds _now_us = std::chrono::duration_cast<std::chrono::microseconds>(_now.time_since_epoch());
    return _now_us.count();
}


END_UTIL_NS

