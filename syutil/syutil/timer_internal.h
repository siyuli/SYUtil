#ifndef SY_TIMER_INC_
#define SY_TIMER_INC_

#include "timer.h"
#include "SyUtilTemplates.h"
#include "SyTimerWrapperID.h"

#if defined(_MSC_VER)
    #pragma warning(disable:4307)
    #pragma warning(disable:4308)
#endif

#ifndef SY_WIN32
    //#define _UNIX_HIGH_PRECISION
#endif

#ifdef SY_WIN32
    #include <time.h>
    #include <winnt.h>
#else
    #include <sys/time.h>
    #include <sys/times.h>
    #include <sys/types.h>
#endif

START_UTIL_NS


///Warning that enum value can not be large int
#if defined(_MSC_VER) && (_MSC_VER <= 1300) //for VC7.0 or before
template <unsigned long base>
struct ExpTraits {
    template <unsigned long exp>
    struct Exp_Inner {
        enum { VAL = base * ExpTraits<base>::Exp_Inner<exp - 1>::VAL };
    };
    template<> struct Exp_Inner<0> {
        enum { VAL = 1 };
    };
};
template<unsigned long base, unsigned long exponential>
struct ExpT {
    enum { VAL = ExpTraits<base>::Exp_Inner<exponential>::VAL };
};

#else

template <unsigned long base, unsigned long exponential>
struct ExpT {
    enum { VAL = base * ExpT<base, exponential - 1>::VAL };
};
template <unsigned long base>
struct ExpT<base, 0> {
    enum { VAL = 1};
};
#endif

//template instantiation depth exceeds maximum of 17 on MAC, so need a function replace it
template <typename ResultType>
ResultType exp_cal(unsigned long base, unsigned long exponential)
{
    ResultType val = 1;
    for (unsigned long idx = 0; idx < exponential; ++idx) {
        val *= base;
    }
    return val;
}

const int64_t us_second = us_benchmark;//microseconds in one seconds
const int64_t us_minute = us_benchmark * 60;
const int64_t us_hour = us_benchmark * 60 * 60;
const int64_t us_day = us_hour * 24;
const int64_t ms_hour = ms_benchmark * 60 * 60;
const int64_t ms_minute = ms_benchmark * 60;
const int64_t ms_second = ms_benchmark;//milliseconds in one seconds

static const int64_t MAX_INT64_PLUS_VAL = (_int64)0X7FFFFFFF << 32 | 0XFFFFFFFF;
static const int64_t MAX_TICK_COUNT_IN_US = (((_int64)1 << 32) - 1) * ms_benchmark;

///Warning the time will overflow from year 2038 on 32bit machine
struct SY_OS_EXPORT PretInitialationTimer {
#ifdef SY_WIN32
    typedef LARGE_INTEGER INIT_TYPE;
#else
    typedef unsigned INIT_TYPE;
#endif
    INIT_TYPE m_InitialValue;
#if defined SY_WIN32 || defined _UNIX_HIGH_PRECISION
    int64_t m_startTmfromS; //time from system started
    int64_t m_startTmfromC; //time from 1970.1.1
#ifdef _UNIX_HIGH_PRECISION
    int64_t m_startMillis;
#endif
#endif
    PretInitialationTimer();
    friend class CSySingletonT<PretInitialationTimer>;
};

typedef CSySingletonT<PretInitialationTimer> PretInitialationTimerSingleT;

class low_tick_generator: public CSyTimerWrapperIDSink
{
public:
    low_tick_generator();
    virtual ~low_tick_generator();
    
    enum {
        MIN_INTERVAL = 10000, //10ms
    };//100ms
    void OnTimer(CSyTimerWrapperID *aId);
    uint64_t now();
    static low_tick_generator *instance();

    void start();
    void stop();
private:
    low_tick_generator(const low_tick_generator &);
    low_tick_generator &operator=(const low_tick_generator &);
    int64_t m_tickNow;
    CSyTimerWrapperID   m_RefreshTimer;
    bool m_InitTimerSucc;
};

typedef struct SY_OS_EXPORT tick_type {
    DWORD tk_hour;
    DWORD tk_min; //minute after hour 0-59
    DWORD tk_sec;// second after minute 0-59
    DWORD tk_millisec;//millisecond after second 0-999
    DWORD tk_microsec;//microsecond after millisecond 0-999

    tick_type();
    tick_type(int64_t val);
protected:
    void reset(int64_t val);
} *lptick_type;

struct SY_OS_EXPORT formatted_ticker: public tick_type {
    typedef tick_type   VALUE_TYPE;
    static tick_type now();
};

typedef struct SY_OS_EXPORT time_type: public tm {
    DWORD tm_millisec; //millisecond after second, 0 - 999
    DWORD tm_microsec; //microsecond after millisecond 0-999
    time_type();
    time_type(int64_t val);
protected:
    void reset(int64_t val);
} *lptime_type;

struct SY_OS_EXPORT formatted_timer: public tick_type {
    typedef time_type  VALUE_TYPE;
    static VALUE_TYPE now();
};

typedef CSyMutexGuardT<CSyMutexThreadRecursive> MutexGuard;
extern ASyThread *g_tickThread;
extern CSyMutexThreadRecursive g_tickThreadMutex;

class CTickEvent : public ISyEvent
{
public:
    CTickEvent() {}
    ~CTickEvent() {}

    // functions from ISyEvent
    virtual SyResult OnEventFire()
    {
        low_tick_policy::now();
        low_tick_policy::start();

        return SY_OK;
    }

};


END_UTIL_NS

#endif  //!define SY_TIMER_INC_
