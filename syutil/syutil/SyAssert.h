#ifndef SY_ASSERT_H
#define SY_ASSERT_H

#include "SyDef.h"

#if defined(_DEBUG) || defined(DEBUG)
    #if !defined(SY_DEBUG)
        #define SY_DEBUG
    #endif
    #if !defined(DEBUG)
        #define DEBUG
    #endif
#endif

#if defined SY_IOS
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
        #define SY_DISABLE_ASSERTE
    #endif
#endif

#ifdef SY_WIN32
# define SY_PATH_SEPARATOR '\\'
#else
# define SY_PATH_SEPARATOR '/'
#endif

extern "C" SY_OS_EXPORT void sy_assertion_report();
extern "C" SY_OS_EXPORT long sy_get_assertions_count();

//Both iOS and MAC need this function
#if defined SY_MACOS
#include <signal.h>
#include <sys/sysctl.h>
#include <unistd.h>
inline static bool SyIsDebuggerAttached() {
    ///https://developer.apple.com/library/mac/qa/qa1361/_index.html
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    struct kinfo_proc info;
    size_t size;
    
    info.kp_proc.p_flag = 0;
    size = sizeof(info);
    if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0) == 0) {
        return ((info.kp_proc.p_flag & P_TRACED) != 0);
    }
    return false;
}
#endif

#if (defined(DEBUG) || defined(SY_FORCE_ASSERTIONS)) && !defined(SY_DISABLE_ASSERTE)
    #if defined(SY_WIN32)
        #include <crtdbg.h>
        #define SY_ASSERTE _ASSERTE
    #elif defined(SY_MACOS)
        #define SY_ASSERTE(expr) if(!(expr)) { \
                __builtin_trap(); \
        } \


    #else
        #include <assert.h>
        #define SY_ASSERTE assert
    #endif
    #define SY_ASSERTE_DEFINED  1
#endif // SY_DEBUG

#ifdef SY_DISABLE_ASSERTE
#include "SyDebug.h"
#ifdef SY_ASSERTE
    #undef SY_ASSERTE
#endif

#define SY_ASSERTE(expr)  \
    do { \
        if (!(expr)) { \
            auto *fn = __FILE__; \
            auto *p = strrchr(fn, SY_PATH_SEPARATOR); \
            if(p) fn = ++p; \
            SY_ERROR_TRACE(fn << ":" << __LINE__ << " Assert failed: " << #expr); \
            sy_assertion_report(); \
        } \
    } while (0)

#endif // SY_DISABLE_ASSERTE

#ifndef SY_ASSERTE
    #define SY_ASSERTE(expr) 
#endif // SY_ASSERTE


#if defined(SY_DISABLE_ASSERTE) || !defined(SY_ASSERTE_DEFINED)
#define SY_ASSERTE_RETURN(expr, rv) \
    do { \
        if (!(expr)) { \
            auto *fn = __FILE__; \
            auto *p = strrchr(fn, SY_PATH_SEPARATOR); \
            if(p) fn = ++p; \
            SY_ERROR_TRACE(fn << ":" << __LINE__ << " Assert failed: " << #expr); \
            sy_assertion_report(); \
            return rv; \
        } \
    } while (0)

#define SY_ASSERTE_RETURN_VOID(expr) SY_ASSERTE_RETURN(expr, )

#else // SY_DISABLE_ASSERTE
#define SY_ASSERTE_RETURN(expr, rv) \
    do { \
        if (!(expr)) { \
            auto *fn = __FILE__; \
            auto *p = strrchr(fn, SY_PATH_SEPARATOR); \
            if(p) fn = ++p; \
            SY_ERROR_TRACE(fn << ":" << __LINE__ << " Assert failed: " << #expr); \
            SY_ASSERTE(false); \
            sy_assertion_report(); \
            return rv; \
        } \
    } while (0)

#define SY_ASSERTE_RETURN_VOID(expr) SY_ASSERTE_RETURN(expr, )

#endif // SY_DISABLE_ASSERTE



#endif //SY_ASSERT_H
