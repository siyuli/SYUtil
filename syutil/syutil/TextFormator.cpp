//
//  TextFormator.cpp
//  syutil
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//

#include "TextFormator.h"

using namespace sy;

void sy_reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end) {
        std::swap(*(str+start), *(str+end));
        start++;
        end--;
    }
}

template<typename T>
char *sy_itoa(T num, char *str, int base)
{
    int i = 0;
    bool isNegative = false;
    
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < (T)0 && base == 10) {
        isNegative = true;
        num = 0-num;
    }
    
    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
    
    // If number is negative, append '-'
    if (isNegative) {
        str[i++] = '-';
    }
    
    str[i] = '\0'; // Append string terminator
    
    // Reverse the string
    sy_reverse(str, i);
    
    return str;
}

template<typename T>
char *sy_ftoa(T num, char *str)
{
    int i = 0, num_i = (int) num, num_f ;
    bool isNegative = false;
    
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num_i == 0) {
        str[i++] = '0';
        str[i] = '\0';
    }
    
    // For negative num
    if (num < 0) {
        isNegative = true;
        num_f = (int)((num_i - num)*100);
        num_i = -num_i;
    } else {
        num_f = (int)((num - num_i)*100);
    }
    
    // Process individual digits
    while (num_i != 0) {
        int rem = num_i % 10;
        str[i++] = rem + '0';
        num_i = num_i/10;
    }
    
    // If num_iber is negative, append '-'
    if (isNegative) {
        str[i++] = '-';
    }
    str[i] = '\0';
    
    // Reverse the string for int part
    sy_reverse(str, i);
    
    // deal with the 2 digit of float part
    str[i] = '.';
    str[i+1] = num_f/10 + '0';
    str[i+2] = num_f%10 + '0';
    str[i+3] = '\0';
    
    return str;
}

/////////////////////////////////////////////////////////////////////////////
// Inner class CCmTextFormator
CTextFormator::CTextFormator(char *lpszBuf, unsigned int dwBufSize)
{
    m_lpszBuf = lpszBuf;
    m_dwSize  = dwBufSize;

    reset();
}
CTextFormator::~CTextFormator()
{
}

void CTextFormator::reset()
{
    m_dwPos   = 0;

    m_lpszBuf[0] = 0;
    //memset(m_lpszBuf, 0, m_dwSize);
}

CTextFormator &CTextFormator::operator << (char ch)
{
    return *this << (int)ch;
}

CTextFormator &CTextFormator::operator << (unsigned char ch)
{
    return *this << (int)ch;
}

CTextFormator &CTextFormator::operator << (short s)
{
    return *this << (int)s;
}

CTextFormator &CTextFormator::operator << (unsigned short s)
{
    return *this << (int)s;
}

CTextFormator &CTextFormator::operator << (int i)
{
    char achBuf[80];

    sy_itoa<int>(i, achBuf, 10);

    advance(achBuf);

    return *this;
}

CTextFormator &CTextFormator::operator << (unsigned int i)
{
    char achBuf[80];

    sy_itoa<unsigned int>(i, achBuf, 10);

    advance(achBuf);

    return *this;
}

CTextFormator &CTextFormator::operator << (long l)
{
    char achBuf[80];

    sy_itoa<long>(l, achBuf, 10);

    advance(achBuf);
    return *this;
}

CTextFormator &CTextFormator::operator << (unsigned long l)
{
    char achBuf[80];

    sy_itoa<unsigned long>(l, achBuf, 10);

    advance(achBuf);

    return *this;
}

CTextFormator &CTextFormator::operator << (float f)
{
    char achBuf[80];
    sy_ftoa(f, achBuf);

    advance(achBuf);
    return *this;
}

CTextFormator &CTextFormator::operator << (double d)
{
    char achBuf[80];
    sy_ftoa(d, achBuf);

    advance(achBuf);
    return *this;
}

CTextFormator &CTextFormator::operator << (const char *lpsz)
{
    advance(lpsz);
    return *this;
}

CTextFormator &CTextFormator::operator << (const std::string &str)
{
    return (*this << str.c_str());
}

CTextFormator &CTextFormator::operator << (void *lpv)
{
    *this << "0x" << 0 << (unsigned long)lpv;
    return *this;
}

CTextFormator::operator char *()
{
    return m_lpszBuf;
}

unsigned int CTextFormator::tell() const
{
    return m_dwPos;
}

void CTextFormator::advance(const char *lpsz)
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
