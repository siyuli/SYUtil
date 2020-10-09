#ifndef __SY_DEF_H__
#define __SY_DEF_H__

#include "SyCommDef.h"

#include "SyCommon.h"


#ifdef WIN32
    #ifndef SY_WIN32
        #define SY_WIN32
    #endif
#endif


#if defined(WINAPI_FAMILY)
    #include <winapifamily.h>
    #if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP
        #define SY_WIN_PHONE             1
    #else
        #define SY_WIN_DESKTOP           1
    #endif
#else
    #define SY_WIN_DESKTOP               1
#endif // WINAPI_FAMILY

#undef EWOULDBLOCK
#undef EINPROGRESS
#undef EALREADY
#undef ENOTSOCK
#undef EDESTADDRREQ
#undef EMSGSIZE
#undef EPROTOTYPE
#undef ENOPROTOOPT
#undef EPROTONOSUPPORT
#undef ESOCKTNOSUPPORT
#undef EOPNOTSUPP
#undef EPFNOSUPPORT
#undef EAFNOSUPPORT
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef ENETDOWN
#undef ENETUNREACH
#undef ENETRESET
#undef ECONNABORTED
#undef ECONNRESET
#undef ENOBUFS
#undef EISCONN
#undef ENOTCONN
#undef ESHUTDOWN
#undef ETOOMANYREFS
#undef ETIMEDOUT
#undef ECONNREFUSED
#undef ELOOP
#undef EHOSTDOWN
#undef EHOSTUNREACH
#undef EPROCLIM
#undef EUSERS
#undef EDQUOT
#undef ESTALE
#undef EREMOTE

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE


#ifdef _MSC_VER
    #ifndef _MT
        #error Error: please use multithread version of C runtime library.
    #endif // _MT

    #pragma warning(disable: 4786) // identifier was truncated to '255' characters in the browser information(mainly brought by stl)
    #pragma warning(disable: 4355) // disable 'this' used in base member initializer list
    #pragma warning(disable: 4275) // deriving exported class from non-exported
    #pragma warning(disable: 4251) // using non-exported as public in exported
#endif // _MSC_VER

#if defined (_LIB) || (SY_OS_BUILD_LIB)
    #define SY_OS_EXPORT
#else
    #if defined (_USRDLL) || (SY_OS_BUILD_DLL)
        #define SY_OS_EXPORT __declspec(dllexport)
    #else
        #define SY_OS_EXPORT __declspec(dllimport)
    #endif // _USRDLL || SY_OS_BUILD_DLL
#endif // _LIB || SY_OS_BUILD_LIB

#define SY_OS_SEPARATE '\\'

#define getpid _getpid
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define vsnprintf _vsnprintf

#define SY_INVALID_HANDLE INVALID_HANDLE_VALUE
#define SY_SD_RECEIVE SD_RECEIVE
#define SY_SD_SEND SD_SEND
#define SY_SD_BOTH SD_BOTH


#define SY_IOV_MAX 64

#define SY_INVALID_HANDLE INVALID_HANDLE_VALUE
#define SY_SD_RECEIVE SD_RECEIVE
#define SY_SD_SEND SD_SEND
#define SY_SD_BOTH SD_BOTH


#endif //__SY_DEF_H__
