//
//  TextFormator.h
//  syutil
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//
#ifndef util_TextFormator_h
#define util_TextFormator_h

#include <string>
#include "SyDebug.h"

START_UTIL_NS

// text-based formatting class
class SY_OS_EXPORT CSyTextFormator
{
public :
    CSyTextFormator(char *lpszBuf, unsigned int dwBufSize);
    virtual ~CSyTextFormator();

public :
    void reset();
    CSyTextFormator &operator << (char ch);
    CSyTextFormator &operator << (unsigned char ch);
    CSyTextFormator &operator << (short s);
    CSyTextFormator &operator << (unsigned short s);
    CSyTextFormator &operator << (int i);
    CSyTextFormator &operator << (unsigned int i);
    CSyTextFormator &operator << (long l);
    CSyTextFormator &operator << (unsigned long l);
    CSyTextFormator &operator << (float f);
    CSyTextFormator &operator << (double d);
    CSyTextFormator &operator << (const char *lpsz);
    CSyTextFormator &operator << (void *lpv);
    CSyTextFormator &operator << (const std::string &str);
    CSyTextFormator &operator << (const int64_t ll);
    CSyTextFormator &operator << (const uint64_t ll);

    operator char *();
    unsigned int tell() const;

private :
    void advance(const char *lpsz);

private :
    char  *m_lpszBuf;
    unsigned int  m_dwSize;
    unsigned int  m_dwPos;
};

END_UTIL_NS

#endif
