//
//  TextFormator.cpp
//  syutil
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//

#include "SyTextFormator.h"
#include "SyUtilTemplates.h"

USE_UTIL_NS

/////////////////////////////////////////////////////////////////////////////
// Inner class CSyTextFormator
CSyTextFormator::CSyTextFormator(char *lpszBuf, unsigned int dwBufSize)
{
    m_lpszBuf = lpszBuf;
    m_dwSize  = dwBufSize;

    reset();
}
CSyTextFormator::~CSyTextFormator()
{
}

void CSyTextFormator::reset()
{
    m_dwPos   = 0;

    m_lpszBuf[0] = 0;
    //memset(m_lpszBuf, 0, m_dwSize);
}

CSyTextFormator &CSyTextFormator::operator << (char ch)
{
    return *this << (int)ch;
}

CSyTextFormator &CSyTextFormator::operator << (unsigned char ch)
{
    return *this << (int)ch;
}

CSyTextFormator &CSyTextFormator::operator << (short s)
{
    return *this << (int)s;
}

CSyTextFormator &CSyTextFormator::operator << (unsigned short s)
{
    return *this << (int)s;
}

CSyTextFormator &CSyTextFormator::operator << (int i)
{
    char achBuf[80];

    sy_itoa<int>(i, achBuf, 10);

    advance(achBuf);

    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (unsigned int i)
{
    char achBuf[80];

    sy_itoa<unsigned int>(i, achBuf, 10);

    advance(achBuf);

    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (long l)
{
    char achBuf[80];

    sy_itoa<long>(l, achBuf, 10);

    advance(achBuf);
    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (unsigned long l)
{
    char achBuf[80];

    sy_itoa<unsigned long>(l, achBuf, 10);

    advance(achBuf);

    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (float f)
{
    char achBuf[80];
    sy_ftoa(f, achBuf);

    advance(achBuf);
    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (double d)
{
    char achBuf[80];
    sy_ftoa(d, achBuf);

    advance(achBuf);
    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (const char *lpsz)
{
    advance(lpsz);
    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (const std::string &str)
{
    return (*this << str.c_str());
}

CSyTextFormator &CSyTextFormator::operator << (void *lpv)
{
    *this << "0x" << 0 << (unsigned long)lpv;
    return *this;
}

CSyTextFormator &CSyTextFormator::operator << (int64_t ll)
{
    char buff[65];
    xtoa_wbx<int64_t>(ll, buff, sizeof(buff));
    return *this << buff;
}

CSyTextFormator &CSyTextFormator::operator << (uint64_t ll)
{
    char buff[65];
    xtoa_wbx<uint64_t>(ll, buff, sizeof(buff));
    return *this << buff;
}

CSyTextFormator::operator char *()
{
    return m_lpszBuf;
}

unsigned int CSyTextFormator::tell() const
{
    return m_dwPos;
}

void CSyTextFormator::advance(const char *lpsz)
{
    if (lpsz) {
        unsigned long nLength = (unsigned long)strlen(lpsz);
        if (nLength > m_dwSize - m_dwPos - 64 ) {
            nLength = m_dwSize - m_dwPos - 64;
        }

        if (nLength > 0) {
            memcpy(m_lpszBuf + m_dwPos*sizeof(char), lpsz,
                   nLength*sizeof(char));
            m_dwPos += nLength;
            m_lpszBuf[m_dwPos] = 0;
        }
    }
}
