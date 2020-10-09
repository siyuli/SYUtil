#ifndef util_SyString_h
#define util_SyString_h

#include "SyDataType.h"
#include "SyError.h"
#include <string>
#if defined(SY_LINUX_SERVER)
    #include <cstring>
#endif

START_UTIL_NS

class CSyString : public std::string
{
public:
    typedef std::string SuperType;

public:
    CSyString()
        : SuperType()
    {
    }

    CSyString(const char *s)
        : SuperType(s ? s : "")
    {
    }

    CSyString(const char *s, SuperType::size_type n)
        : SuperType(s ? s : "", s ? n : 0)
    {
    }

#if defined(SY_WIN32) || defined(SY_MACOS) || defined(SY_SOLARIS) || defined(SY_ANDROID)
    template <class IterType>
    CSyString(IterType s1, IterType s2)
        : SuperType(s1, s2, SuperType::allocator_type())
    {
    }
#else
    CSyString(const char *s1, const char *s2)
        : SuperType(s1, s2 - s1)
    {
    }
#endif

    CSyString(const SuperType &str)
#if defined SY_WIN32
        : SuperType(str.c_str(), str.length())
#else
        : SuperType(str)
#endif
    {
    }

    CSyString(const CSyString &str)
#if defined SY_WIN32
        : SuperType(str.c_str(), str.length())
#else
        : SuperType(str)
#endif
    {
    }

    CSyString(SuperType::size_type n, char c)
        : SuperType(n, c, SuperType::allocator_type())
    {
    }

    CSyString &operator = (const char *s)
    {
#if defined SY_WIN32
        if (s > this->c_str() && s < (this->c_str() + this->length())) { //in the string scope
            CSyString tmpStr(s);
            *this = tmpStr;
            return *this;
        }
#endif
        SuperType::operator = (s ? s : "");
        return *this;
    }

    CSyString &operator = (const CSyString &str)
    {
#if defined SY_WIN32
        if (&str == this) { //if same, do nothing, otherwise it should be has some memory issue
            return *this;
        }
        SuperType::assign(str.c_str(), str.length());
#else
        SuperType::operator = (str);
#endif
        return *this;
    }

    CSyString &operator = (char c)
    {
        SuperType::operator = (c);
        return *this;
    }

    CSyString &operator = (int n)
    {
        SuperType::operator = (n);
        return *this;
    }

    CSyString &toUpperCase()
    {
        unsigned char *szInner = reinterpret_cast<unsigned char *>(const_cast<char *>(this->c_str()));
        for (CSyString::size_type i = 0; i < length(); ++i, szInner++) {
            if (isalpha(*szInner) && islower(*szInner)) {
                *szInner = toupper(*szInner);
            }
        }
        return *this;
    }

    CSyString &toLowerCase()
    {
        unsigned char *szInner = reinterpret_cast<unsigned char *>(const_cast<char *>(this->c_str()));
        for (CSyString::size_type i = 0; i < length(); ++i, szInner++) {
            if (isalpha(*szInner) && isupper(*szInner)) {
                *szInner = tolower(*szInner);
            }
        }
        return *this;
    }

};

class CSyIsSpace
{
public:
    int operator()(const char c)
    {
        return c == ' ';
    }
};

template<class IS>
void LTrimString(CSyString &aTrim, IS aIs)
{
    LPCSTR pStart = aTrim.c_str();
    LPCSTR pMove = pStart;

    for (; *pMove; ++pMove) {
        if (!aIs(*pMove)) {
            if (pMove != pStart) {
                size_t nLen = strlen(pMove);
                aTrim.replace(0, nLen, pMove, nLen);
                aTrim.resize(nLen);
            }
            return;
        }
    }
    
    if (pMove != pStart) {
        aTrim.resize(0);
    }
};

template<class IS>
void RTrimString(CSyString &aTrim, IS aIs)
{
    if (aTrim.empty()) {
        return;
    }

    LPCSTR pStart = aTrim.c_str();
    LPCSTR pEnd = pStart + aTrim.length() - 1;
    LPCSTR pMove = pEnd;

    for (; pMove >= pStart; --pMove) {
        if (!aIs(*pMove)) {
            if (pMove != pEnd) {
                aTrim.resize(pMove - pStart + 1);
            }
            return;
        }
    }
};

template<class IS>
void TrimString(CSyString &aTrim, IS aIs)
{
    LTrimString(aTrim, aIs);
    RTrimString(aTrim, aIs);
};

END_UTIL_NS

#endif
