
//#include "SyBase.h"
#include "SyTimeValue.h"
#include <limits.h>  // for LONG_MAX

#if !defined (SY_WIN32) && !defined(SY_ANDROID)
    #include <sys/time.h>
#else //!defined (WIN32)
    #include <time.h>
#endif //!defined (SY_WIN32)

USE_UTIL_NS

namespace SyTimeStatic
{
const CSyTimeValue s_tvZero;
const CSyTimeValue s_tvMax(LONG_MAX, SY_ONE_SECOND_IN_USECS-1);
}

CSyTimeValue CSyTimeValue::get_tvZero()
{
    return SyTimeStatic::s_tvZero;
}

CSyTimeValue CSyTimeValue::get_tvMax()
{
    return SyTimeStatic::s_tvMax;
}


void CSyTimeValue::Normalize()
{
    //  m_lSec += m_lUsec / SY_ONE_SECOND_IN_USECS;
    //  m_lUsec %= SY_ONE_SECOND_IN_USECS;
    if (m_lUsec >= SY_ONE_SECOND_IN_USECS) {
        do {
            m_lSec++;
            m_lUsec -= SY_ONE_SECOND_IN_USECS;
        } while (m_lUsec >= SY_ONE_SECOND_IN_USECS);
    } else if (m_lUsec <= -SY_ONE_SECOND_IN_USECS) {
        do {
            m_lSec--;
            m_lUsec += SY_ONE_SECOND_IN_USECS;
        } while (m_lUsec <= -SY_ONE_SECOND_IN_USECS);
    }

    if (m_lSec >= 1 && m_lUsec < 0) {
        m_lSec--;
        m_lUsec += SY_ONE_SECOND_IN_USECS;
    } else if (m_lSec < 0 && m_lUsec > 0) {
        m_lSec++;
        m_lUsec -= SY_ONE_SECOND_IN_USECS;
    }
}

void CSyTimeValue::GetLocalTime(struct tm *pTmVal)
{
#if defined(SY_WIN32)
    time_t timeSec = m_lSec;
    localtime_s(pTmVal, &timeSec);
#else
    localtime_r((time_t *)&m_lSec, pTmVal);
#endif
}

void CSyTimeValue::GetUTCTime(struct tm *pTmVal)
{
#if defined(SY_WIN32)
    time_t timeSec = m_lSec;
    gmtime_s(pTmVal, &timeSec);
#else
    gmtime_r((time_t *)&m_lSec, pTmVal);
#endif
}

// inline functions
CSyTimeValue::CSyTimeValue()
    : m_lSec(0)
    , m_lUsec(0)
{
}

CSyTimeValue::CSyTimeValue(long aSec)
    : m_lSec(aSec)
    , m_lUsec(0)
{
}

CSyTimeValue::CSyTimeValue(long aSec, long aUsec)
{
    Set(aSec, aUsec);
}

CSyTimeValue::CSyTimeValue(const timeval &aTv)
{
    Set(aTv);
}

CSyTimeValue::CSyTimeValue(double aSec)
{
    Set(aSec);
}

void CSyTimeValue::Set(long aSec, long aUsec)
{
    m_lSec = aSec;
    m_lUsec = aUsec;
    Normalize();
}

void CSyTimeValue::Set(const timeval &aTv)
{
    m_lSec = aTv.tv_sec;
    m_lUsec = aTv.tv_usec;
    Normalize();
}

void CSyTimeValue::Set(double aSec)
{
    long l = (long)aSec;
    m_lSec = l;
    m_lUsec = (long)((aSec - (double)l) * SY_ONE_SECOND_IN_USECS);
    Normalize();
}

void CSyTimeValue::SetByTotalMsec(long aMilliseconds)
{
    m_lSec = aMilliseconds / 1000;
    m_lUsec = (aMilliseconds - (m_lSec * 1000)) * 1000;
}

long CSyTimeValue::GetSec() const
{
    return m_lSec;
}

long CSyTimeValue::GetUsec() const
{
    return m_lUsec;
}

long CSyTimeValue::GetTotalInMsec() const
{
    return m_lSec * 1000 + m_lUsec / 1000;
}

#ifndef SY_WIN32
CSyTimeValue CSyTimeValue::GetTimeOfDay()
{
    timeval tvCur;
    gettimeofday(&tvCur, NULL);
    return CSyTimeValue(tvCur);
}
#endif // !SY_WIN32

#ifdef SY_WIN32
CSyTimeValue CSyTimeValue::GetTimeOfDay()
{
    FILETIME tfile;
    ::GetSystemTimeAsFileTime(&tfile);

    ULARGE_INTEGER _100ns;
    _100ns.LowPart = tfile.dwLowDateTime;
    _100ns.HighPart = tfile.dwHighDateTime;
    _100ns.QuadPart -= (DWORDLONG)0x19db1ded53e8000;
    return CSyTimeValue((long)(_100ns.QuadPart / (10000 * 1000)),
                        (long)((_100ns.QuadPart % (10000 * 1000)) / 10));
}
#endif // SY_WIN32

int operator > (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    if (aLeft.GetSec() > aRight.GetSec()) {
        return 1;
    } else if (aLeft.GetSec() == aRight.GetSec() && aLeft.GetUsec() > aRight.GetUsec()) {
        return 1;
    } else {
        return 0;
    }
}

int operator >= (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    if (aLeft.GetSec() > aRight.GetSec()) {
        return 1;
    } else if (aLeft.GetSec() == aRight.GetSec() && aLeft.GetUsec() >= aRight.GetUsec()) {
        return 1;
    } else {
        return 0;
    }
}

int operator < (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return !(aLeft >= aRight);
}

int operator <= (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return aRight >= aLeft;
}

int operator == (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return aLeft.GetSec() == aRight.GetSec() &&
           aLeft.GetUsec() == aRight.GetUsec();
}

int operator != (const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return !(aLeft == aRight);
}

void CSyTimeValue::operator += (const CSyTimeValue &aRight)
{
    m_lSec = GetSec() + aRight.GetSec();
    m_lUsec = GetUsec() + aRight.GetUsec();
    Normalize();
}

void CSyTimeValue::operator -= (const CSyTimeValue &aRight)
{
    m_lSec = GetSec() - aRight.GetSec();
    m_lUsec = GetUsec() - aRight.GetUsec();
    Normalize();
}

CSyTimeValue operator +
(const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return CSyTimeValue(aLeft.GetSec() + aRight.GetSec(),
                        aLeft.GetUsec() + aRight.GetUsec());
}

CSyTimeValue operator -
(const CSyTimeValue &aLeft, const CSyTimeValue &aRight)
{
    return CSyTimeValue(aLeft.GetSec() - aRight.GetSec(),
                        aLeft.GetUsec() - aRight.GetUsec());
}
