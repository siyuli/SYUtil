#ifndef TIMER_INC_
#define TIMER_INC_

#include "SyDef.h"
#include "stdint.h"

START_UTIL_NS

const int64_t ms_benchmark = 1000;
const int64_t us_benchmark = 1000000;

#ifdef __cplusplus
extern "C"
{
#endif
SY_OS_EXPORT void startLowTick();
SY_OS_EXPORT void stopLowTick();

#ifdef SY_WIN32
SY_OS_EXPORT void setThreadAffinity();
SY_OS_EXPORT bool checkIsTimeValid(int64_t remain, int32_t units);
#endif

#ifdef __cplusplus
}
#endif

struct SY_OS_EXPORT timer_policy {
    ///get time (us) from 1970.1.1
    static uint64_t now();
    static int64_t max_time_value();
};

struct SY_OS_EXPORT tick_policy {
    ///get time (micro s) from system start or process start
    static uint64_t now();
    static int64_t max_time_value();
};

struct SY_OS_EXPORT low_tick_policy { //low precision tick, 10ms
public:
    ///get time (micro s) from system start or process start
    static uint64_t now();
    static int64_t max_time_value();

    static void start();
    static void stop();
};

///rigour to millisecond
template<typename policy_t>
struct SY_OS_EXPORT timer_fact {
    typedef int64_t VALUE_TYPE;
    ///Constructor
    timer_fact(VALUE_TYPE initTime = -1)
    {
#ifdef SY_WIN32
        setThreadAffinity();
#endif
        reset(initTime);
    }
    ///reset the time tag
    void reset(VALUE_TYPE initTime = -1)
    {
        if (initTime == -1) {
            tag_ = policy_t::now();
        } else {
            tag_ = initTime;
        }
    }

    ///get the elapsed times from reset(microseconds)
    VALUE_TYPE elapsed()
    {
        VALUE_TYPE nowt = policy_t::now();
        VALUE_TYPE remain = nowt >= tag_ ? 0 : policy_t::max_time_value() - tag_;
#ifdef SY_WIN32
        if (!checkIsTimeValid(remain, 1000000)) {
            return 0;
        }
#endif
        return remain == 0 ? nowt -  tag_ : nowt + remain;
    }

    VALUE_TYPE elapsed_mills()
    {
        VALUE_TYPE nowt = ((VALUE_TYPE)(policy_t::now())) / ms_benchmark;
        VALUE_TYPE remain = nowt >= tag_  / ms_benchmark ? 0 : (policy_t::max_time_value() - tag_) / ms_benchmark;
#ifdef SY_WIN32
        if (!checkIsTimeValid(remain, 1000)) {
            return 0;
        }
#endif
        return remain == 0 ? nowt -  tag_ / ms_benchmark : nowt + remain;
    }

    VALUE_TYPE elapsed_sec()
    {
        VALUE_TYPE nowt = ((VALUE_TYPE)(policy_t::now())) / us_benchmark;
        VALUE_TYPE remain = nowt >= tag_  / us_benchmark ? 0 : (policy_t::max_time_value() - tag_) / us_benchmark;
#ifdef SY_WIN32
        if (!checkIsTimeValid(remain, 1)) {
            return 0;
        }
#endif
        return remain == 0 ? nowt -  tag_ / us_benchmark : nowt + remain;
    }

    ///get the elapsed microseconds
    const static uint64_t now()
    {
        return policy_t::now();
    }

    bool overtime(VALUE_TYPE microTimes)
    {
        return elapsed() >= microTimes;
    }

    bool overtime_mills(VALUE_TYPE millsTimes)
    {
        return elapsed_mills()  >= millsTimes;
    }

    bool overtime_sec(VALUE_TYPE secTimes)
    {
        return elapsed_sec() >= secTimes;
    }

    VALUE_TYPE latest_tag() const
    {
        return tag_;
    }

private:
    ///the time tag
    VALUE_TYPE  tag_;
};

template SY_OS_EXPORT struct timer_fact<timer_policy>;
template SY_OS_EXPORT struct timer_fact<tick_policy>;
template SY_OS_EXPORT struct timer_fact<low_tick_policy>;

typedef timer_fact<timer_policy>  timer;
typedef timer_fact<tick_policy>  ticker;
typedef timer_fact<low_tick_policy>  low_ticker;

#if defined (WP8) || defined(UWP) 
    #define get_tick_count GetTickCount64
    #define output_debug_string OutputDebugString
#elif defined(SY_WIN32)
    #define get_tick_count GetTickCount
    #define output_debug_string OutputDebugString
#else
    unsigned long get_tick_count();
    void output_debug_string(char *str);
#endif

END_UTIL_NS

#endif
