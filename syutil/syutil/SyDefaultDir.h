#ifndef SYVALIDDIR_H
#define SYVALIDDIR_H
#include "SyString.h"

#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

START_UTIL_NS
class SY_OS_EXPORT CSyDefaultDir
{
public:
    CSyDefaultDir();
    ~CSyDefaultDir();
    CSyString GetDefaultDir();
};

END_UTIL_NS

#endif // !SYTHREADTASK_H
