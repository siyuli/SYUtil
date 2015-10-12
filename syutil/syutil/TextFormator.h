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
#include "util.h"

SY_NAMESPACE_BG

// text-based formatting class
class CTextFormator
{
public :
    CTextFormator(char *lpszBuf, unsigned int dwBufSize);
    virtual ~CTextFormator();

public :
    void reset();
    CTextFormator &operator << (char ch);
    CTextFormator &operator << (unsigned char ch);
    CTextFormator &operator << (short s);
    CTextFormator &operator << (unsigned short s);
    CTextFormator &operator << (int i);
    CTextFormator &operator << (unsigned int i);
    CTextFormator &operator << (long l);
    CTextFormator &operator << (unsigned long l);
    CTextFormator &operator << (float f);
    CTextFormator &operator << (double d);
    CTextFormator &operator << (const char *lpsz);
    CTextFormator &operator << (void *lpv);
    CTextFormator &operator << (const std::string &str);

    operator char *();
    unsigned int tell() const;

private :
    void advance(const char *lpsz);

private :
    char  *m_lpszBuf;
    unsigned int  m_dwSize;
    unsigned int  m_dwPos;
};

SY_NAMESPACE_ED

#endif
