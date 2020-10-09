#ifndef SYPIPE_H
#define SYPIPE_H

#include "SyDef.h"
#include "SyDataType.h"

START_UTIL_NS

#define SY_DEFAULT_MAX_SOCKET_BUFSIZ 65535



class SY_OS_EXPORT CSyPipe
{
public:
    CSyPipe();
    ~CSyPipe();

    SyResult Open(DWORD aSize = SY_DEFAULT_MAX_SOCKET_BUFSIZ);
    SyResult Close();

    SY_HANDLE GetReadHandle() const;
    SY_HANDLE GetWriteHandle() const;

private:
    SY_HANDLE m_Handles[2];
};

END_UTIL_NS

#endif // !SYPIPE_H
