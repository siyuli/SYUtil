#ifndef SY_COMMON_H
#define SY_COMMON_H


// supports Windows NT 4.0 and later, not support Windows 95.
// mainly for using winsock2 functions
#ifndef _WIN32_WINNT
    #ifdef UWP
        #define _WIN32_WINNT 0x0A00
    #elif defined (WP8)
         #define _WIN32_WINNT 0x0602
    #else
        #define _WIN32_WINNT 0x0501
    #endif
#endif

// These macros are copied form qoscodevdefs.h, thank Jeromy for providing them.
#define PTODO_LINENUMBER_TO_STRING(x) #x
#define PTODO_LINENUMBER(x) PTODO_LINENUMBER_TO_STRING(x)
#ifndef UTIL_INCLUDE_FILE_AND_LINE
    #define UTIL_INCLUDE_FILE_AND_LINE(s) PTODO_LINENUMBER((__FILE__ : __LINE__ ): s)
#endif // ~UTIL_INCLUDE_FILE_AND_LINE

#if _MSC_VER < 1200
    typedef _int64  INT64;
#endif

#if _MSC_VER <= 1200
    #include <windows.h>
    #include <winsock2.h>
#else
    #include <winsock2.h>
    #include <windows.h>
#endif


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <wchar.h>

#include <crtdbg.h>
#include <process.h>

#endif //SY_COMMON_H
