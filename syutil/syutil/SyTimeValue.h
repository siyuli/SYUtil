#ifndef SYTIMEVALUE_H
#define SYTIMEVALUE_H

//#include "SyDefines.h"
#include "SyDef.h"
#include "SyCommon.h"

#if defined(SY_LINUX) || defined(SY_APPLE)
    #include <sys/time.h>
#endif

#if defined(SY_LINUX_SERVER)
    #include <time.h>
#endif

START_UTIL_NS

#define SY_ONE_MSEC_IN_USECS   1000L
#define SY_ONE_SECOND_IN_MSECS 1000L
#define SY_ONE_SECOND_IN_USECS 1000000L
#define SY_ONE_SECOND_IN_NSECS 1000000000L

class SY_OS_EXPORT CSyTimeValue
{
public:
    // add the follwoing two functions to avoid call Normalize().
    CSyTimeValue();
    CSyTimeValue(long aSec);
    CSyTimeValue(long aSec, long aUsec);
    CSyTimeValue(const timeval &aTv);
    CSyTimeValue(double aSec);

    void Set(long aSec, long aUsec);
    void Set(const timeval &aTv);
    void Set(double aSec);

    long GetSec() const ;
    long GetUsec() const ;

    void SetByTotalMsec(long aMilliseconds);
    long GetTotalInMsec() const;

    void operator += (const CSyTimeValue &aRight);
    void operator -= (const CSyTimeValue &aRight);

    friend SY_OS_EXPORT CSyTimeValue operator + (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT CSyTimeValue operator - (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator < (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator > (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator <= (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator >= (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator == (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);
    friend SY_OS_EXPORT int operator != (const CSyTimeValue &aLeft, const CSyTimeValue &aRight);

    static CSyTimeValue GetTimeOfDay();
    void GetLocalTime(struct tm *pTmVal);
    void GetUTCTime(struct tm *pTmVal);

    static CSyTimeValue get_tvZero();
    static CSyTimeValue get_tvMax();

private:
    void Normalize();

    long m_lSec;
    long m_lUsec;
};


END_UTIL_NS

#endif // !SYTIMEVALUE_H
