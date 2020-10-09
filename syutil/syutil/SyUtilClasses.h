#ifndef SYUTILCLASSES_H
#define SYUTILCLASSES_H

//#include "SyDefines.h"
#include "SyThreadInterface.h"
#include "SyStdCpp.h"

START_UTIL_NS

class CSyMutexThreadRecursive;
class ASyThread;

class SY_OS_EXPORT CSyStopFlag
{
public:
    CSyStopFlag() : m_bStoppedFlag(TRUE)
    {
    }

    void SetStartFlag();

    void SetStopFlag();

    BOOL IsFlagStopped() const
    {
        m_Est.EnsureSingleThread();
        return m_bStoppedFlag;
    }
    void SetStopFlagWithoutThreadCheck(BOOL value);

    CSyEnsureSingleThread m_Est;
    BOOL m_bStoppedFlag;
};

extern "C" SY_OS_EXPORT CSyMutexThreadRecursive *SyGetSingletonMutex();
class SY_OS_EXPORT CSyCleanUpBase
{
public:
    static void CleanupAll();

protected:
    CSyCleanUpBase();
    virtual ~CSyCleanUpBase();
    virtual void CleanUp();

private:
    CSyCleanUpBase *m_pNext;
    static CSyCleanUpBase *s_pHeader;

private:
    typedef CSyMutexThreadRecursive MutexType;

    // = Prevent assignment and initialization.
    void operator = (const CSyCleanUpBase &);
    CSyCleanUpBase(const CSyCleanUpBase &);
};

class SY_OS_EXPORT CSyDataBlockNoMalloc
{
public:
    CSyDataBlockNoMalloc(LPCSTR aStr, DWORD aLen);

    /// Read and advance <aCount> bytes,
    SyResult Read(LPVOID aDst, DWORD aCount, DWORD *aBytesRead = NULL);

    /// Write and advance <aCount> bytes
    SyResult Write(LPCVOID aSrc, DWORD aCount, DWORD *aBytesWritten = NULL);

private:
    LPCSTR m_pBegin;
    LPCSTR m_pEnd;
    LPCSTR m_pCurrentRead;
    LPSTR m_pCurrentWrite;
};

class SY_OS_EXPORT CSyMainThreadExitProtect
{
public:
    CSyMainThreadExitProtect() {
        m_pMainThreadExitFlag = NULL;
    };
    
    ~CSyMainThreadExitProtect() {
        if(m_pMainThreadExitFlag)
            *m_pMainThreadExitFlag = true;
        m_pMainThreadExitFlag = NULL;
    };
    
    bool *m_pMainThreadExitFlag;
};
END_UTIL_NS

#endif // !SYUTILCLASSES_H
