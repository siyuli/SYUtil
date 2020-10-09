#ifndef SY_DATATYPE_H
#define SY_DATATYPE_H

#define START_UTIL_NS
#define END_UTIL_NS
#define USE_UTIL_NS

#define START_TP_NS
#define END_TP_NS
#define USE_TP_NS

#ifndef SY_RESULT
    #define SY_RESULT
    typedef int                    SyResult;
#endif

#include "SyCommon.h"

typedef HANDLE SY_HANDLE;
struct iovec {
    u_long iov_len; // byte count to read/write
    char *iov_base; // data to be read/written
};

#endif
