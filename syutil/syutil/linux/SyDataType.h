#ifndef SY_DATATYPE_H
#define SY_DATATYPE_H

#define START_UTIL_NS
#define END_UTIL_NS
#define USE_UTIL_NS
#define START_TP_NS
#define END_TP_NS
#define USE_TP_NS


#include <stdint.h>

typedef int SyResult;

typedef int SY_HANDLE;
typedef SY_HANDLE SY_SOCKET;

typedef long long           LONGLONG;
typedef unsigned int       DWORD;
typedef long                LONG;
typedef unsigned long       ULONG;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short        WORD;
typedef float                 FLOAT;
typedef int                   INT;
typedef unsigned int          UINT;
typedef FLOAT                *PFLOAT;
typedef BOOL                 *LPBOOL;
typedef int                  *LPINT;
typedef WORD                 *LPWORD;
typedef long                 *LPLONG;
typedef DWORD                *LPDWORD;
typedef unsigned int         *LPUINT;
typedef void                 *LPVOID;
typedef const void           *LPCVOID;
typedef char                  CHAR;
typedef char                  TCHAR;
typedef unsigned short        WCHAR;
typedef const char           *LPCSTR;
typedef char                 *LPSTR;
typedef const unsigned short *LPCWSTR;
typedef unsigned short       *LPWSTR;
typedef BYTE                 *LPBYTE;
typedef const BYTE           *LPCBYTE;

typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t  UINT8;

typedef int64_t  INT64;
typedef int32_t  INT32;
typedef int16_t  INT16;
typedef int8_t   INT8;


#if defined(SY_LINUX_SERVER)
    #if (__WORDSIZE == 64)
    typedef long int _int64;
    #else
    typedef long long int _int64;
    #endif
#endif
typedef _int64        __int64;


#endif //SY_DATATYPE_H
