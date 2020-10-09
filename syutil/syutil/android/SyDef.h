#ifndef __SY_DEF_H__
#define __SY_DEF_H__

#include "SyCommDef.h"

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

#ifndef SY_ANDROID
    #define SY_ANDROID
#endif

#if defined(ANDROID)
    //https://developer.android.com/ndk/guides/cpu-features
    #if defined(__arm__)
        #define SY_ANDROID_ABI "armeabi-v7a"
    #elif defined(__aarch64__)
        #define SY_ANDROID_ABI "arm64-v8a"
    #elif defined(__i386__)
        #define SY_ANDROID_ABI "x86"
    #elif defined(__x86_64__)
        #define SY_ANDROID_ABI "x86_64"
    #else
        #define SY_ANDROID_ABI "unknown"
    #endif
#endif

#endif //__SY_DEF_H__
