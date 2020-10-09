#ifndef __SY_DEF_H__
#define __SY_DEF_H__

#include "SyCommDef.h"

#ifndef SY_LINUX
    #define SY_LINUX
#endif

#ifndef SY_UNIX
    #define SY_UNIX
#endif

#define SY_OS_EXPORT

#define EWOULDBLOCK EAGAIN

#ifdef SY_SOLARIS
    #define INADDR_NONE             0xffffffff
#endif

#define SY_OS_SEPARATE '/'

#define SY_INVALID_HANDLE -1
#define SY_SD_RECEIVE 0
#define SY_SD_SEND 1
#define SY_SD_BOTH 2

#if !defined (IOV_MAX)
    #define IOV_MAX 16
#endif // !IOV_MAX

#define SY_IOV_MAX IOV_MAX

#endif //__SY_DEF_H__
