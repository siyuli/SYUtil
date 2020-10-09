#ifndef SY_DATA_TYPE_H
#define SY_DATA_TYPE_H

#include <stdint.h>
#include "SyCommDef.h"

START_UTIL_NS

typedef int SyResult;

//We should deprecate to use those definitions because of stdint is now part of standard.

typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t  UINT8;

typedef int64_t  INT64;
typedef int32_t  INT32;
typedef int16_t  INT16;
typedef int8_t   INT8;

typedef int         SY_HANDLE;
typedef SY_HANDLE   SY_SOCKET;

typedef long long               LONGLONG;
typedef unsigned int            DWORD;
typedef long                    LONG;
typedef signed char             BOOL;
typedef unsigned char           BYTE;
typedef unsigned short          WORD;
typedef float                   FLOAT;
typedef int                     INT;
typedef unsigned int            UINT;
typedef FLOAT                   *PFLOAT;
typedef BOOL                    *LPBOOL;
typedef int                     *LPINT;
typedef WORD                    *LPWORD;
typedef long                    *LPLONG;
typedef DWORD                   *LPDWORD;
typedef unsigned int            *LPUINT;
typedef void                    *LPVOID;
typedef const void              *LPCVOID;
typedef char                    CHAR;
typedef char                    TCHAR;
typedef unsigned short          WCHAR;
typedef const char              *LPCSTR;
typedef char                    *LPSTR;
typedef const unsigned short    *LPCWSTR;
typedef unsigned short          *LPWSTR;
typedef BYTE                    *LPBYTE;
typedef const BYTE              *LPCBYTE;
typedef float                   FLOAT;
typedef const void              *LPCVOID;

typedef long long int           _int64;
typedef _int64                  __int64;


END_UTIL_NS


#endif

