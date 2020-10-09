#ifndef SY_DEF_H
#define SY_DEF_H

#include "SyCommDef.h"

#define SY_INVALID_HANDLE -1
#define SY_SD_RECEIVE 0
#define SY_SD_SEND 1
#define SY_SD_BOTH 2

#define SY_OS_EXPORT

#define SY_OS_SEPARATE '/'

#if !defined (IOV_MAX)
    #define IOV_MAX 16
#endif

#define SY_IOV_MAX IOV_MAX

#ifndef SY_IOS
    #define SY_IOS
#endif

#ifndef SY_MACOS
    #define SY_MACOS    1
#endif

#define SY_APPLE

#endif
