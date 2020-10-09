#ifndef SY_DATA_TYPE_H
#define SY_DATA_TYPE_H

//

#include <stdint.h>
#include <objc/objc.h>

typedef int SyResult;

typedef long long           LONGLONG;
typedef long                LONG;
typedef unsigned long       ULONG;
//reserved for 64bit
#ifndef OBJC_BOOL_DEFINED
    typedef bool         BOOL;
#endif
typedef unsigned char       BYTE;
typedef unsigned short        WORD;
typedef float                 FLOAT;
typedef int                   INT;
typedef FLOAT                *PFLOAT;
typedef BOOL                 *LPBOOL;
typedef int                  *LPINT;
typedef WORD                 *LPWORD;
typedef long                 *LPLONG;
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

//DWORD, UINT32, INT32, UINT were conflicted with train iOS
//The best way of fixing this would be to remove all of them
//but they are widely used, so it is difficult to do so.
#ifndef __LP64__
    typedef unsigned long       DWORD;
    typedef unsigned long       UINT;
#else
    typedef unsigned int        DWORD;
    typedef unsigned int        UINT;
#endif
//End of train compatible define

typedef DWORD                *LPDWORD;
typedef UINT                 *LPUINT;

//
typedef uint64_t UINT64;
typedef uint16_t UINT16;
typedef uint8_t  UINT8;

typedef int64_t  INT64;
typedef int16_t  INT16;
typedef int8_t   INT8;

typedef int SY_HANDLE;
typedef SY_HANDLE SY_SOCKET;

//#include "atdefs.h"

#ifndef MachOSupport
struct iovec {
    char   *iov_base;  /* Base address. */
    unsigned long iov_len;    /* Length. */
};
#endif //MachOSupport


typedef long long int _int64;
typedef _int64  __int64;



#endif

