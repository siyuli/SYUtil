#ifndef SYSTDCPP_H
#define SYSTDCPP_H

#include "SyDataType.h"
#include "SyCommon.h"

#include <ctype.h>

#include <memory>
#include <utility>
#include <algorithm>
#include <utility>

#include <string>

#include <set>
#include <map>
#include <vector>
#include <list>
#include <queue>
#include <stack>

START_UTIL_NS

#ifdef _MSC_VER
    #if _MSC_VER > 1600
        #define SY_MIN min
        #define SY_MAX max
    #elif _MSC_VER > 1200
        #define SY_MIN _cpp_min
        #define SY_MAX _cpp_max
    #else
        #define SY_MIN std::_cpp_min
        #define SY_MAX std::_cpp_max
    #endif
#else
    #define SY_MIN std::min
    #define SY_MAX std::max
#endif // _MSC_VER


END_UTIL_NS


#include "SyString.h"

#endif // SYSTDCPP_H
