
//#include "SyBase.h"
#include "SyUtilClasses.h"
#include "SyThreadMisc.h"

#include "SyUtilMisc.h"

#include "SyAssert.h"

#include "SyDebug.h"
#include "SyNetwork.h"

//#include "tp_p.h"

START_UTIL_NS

///////////////////////////////////////////////////////////////
// class CSyCleanUpBase
///////////////////////////////////////////////////////////////

CSyCleanUpBase *CSyCleanUpBase::s_pHeader = NULL;
CSyMutexThreadRecursive m_sSingletonMutex;

CSyMutexThreadRecursive *SyGetSingletonMutex()
{
    return &m_sSingletonMutex;
}

CSyCleanUpBase::CSyCleanUpBase()
{
    MutexType *pMutex = SyGetSingletonMutex();

    SY_ASSERTE(pMutex);

    CSyMutexGuardT<MutexType> theGuard(*pMutex);
    m_pNext = s_pHeader;
    s_pHeader = this;
}

CSyCleanUpBase::~CSyCleanUpBase()
{
}

void CSyCleanUpBase::CleanUp()
{
    delete this;
}

void CSyCleanUpBase::CleanupAll()
{
    MutexType *pMutex = SyGetSingletonMutex();
    SY_ASSERTE(pMutex);

    CSyMutexGuardT<MutexType> theGuard(*pMutex);
    while (s_pHeader) {
        CSyCleanUpBase *pTmp = s_pHeader->m_pNext;
        s_pHeader->CleanUp();
        s_pHeader = pTmp;
    }
}


///////////////////////////////////////////////////////////////
// class CSyDataBlockNoMalloc
///////////////////////////////////////////////////////////////

CSyDataBlockNoMalloc::CSyDataBlockNoMalloc(LPCSTR aStr, DWORD aLen)
    : m_pBegin(aStr)
    , m_pEnd(aStr + aLen)
    , m_pCurrentRead(m_pBegin)
    , m_pCurrentWrite(const_cast<LPSTR>(m_pBegin))
{
    SY_ASSERTE(m_pBegin);
}

SyResult CSyDataBlockNoMalloc::
Read(LPVOID aDst, DWORD aCount, DWORD *aBytesRead)
{
    SY_ASSERTE_RETURN(aDst, SY_ERROR_INVALID_ARG);
    SY_ASSERTE_RETURN(m_pCurrentRead, SY_ERROR_NOT_INITIALIZED);
    SY_ASSERTE_RETURN(m_pCurrentRead <= m_pEnd, SY_ERROR_NOT_INITIALIZED);

    //2009 5.14 Victor we need check the data is enough or not for the read request
    //DWORD dwLen = SY_MIN(aCount, static_cast<DWORD>(m_pCurrentRead - m_pEnd));
    DWORD dwLen = SY_MIN(aCount, static_cast<DWORD>(m_pEnd - m_pCurrentRead));
    if (dwLen > 0) {
        ::memcpy(aDst, m_pCurrentRead, dwLen);
        m_pCurrentRead += dwLen;
    }
    if (aBytesRead) {
        *aBytesRead = dwLen;
    }
    return dwLen == aCount ? SY_OK : SY_ERROR_PARTIAL_DATA;
}

SyResult CSyDataBlockNoMalloc::
Write(LPCVOID aSrc, DWORD aCount, DWORD *aBytesWritten)
{
    SY_ASSERTE_RETURN(aSrc, SY_ERROR_INVALID_ARG);
    SY_ASSERTE_RETURN(m_pCurrentWrite, SY_ERROR_NOT_INITIALIZED);
    SY_ASSERTE_RETURN(m_pCurrentWrite <= m_pEnd, SY_ERROR_NOT_INITIALIZED);

    DWORD dwLen = SY_MIN(aCount, static_cast<DWORD>(m_pEnd - m_pCurrentWrite));
    if (dwLen > 0) {
        ::memcpy(m_pCurrentWrite, aSrc, dwLen);
        m_pCurrentWrite += dwLen;
    }
    if (aBytesWritten) {
        *aBytesWritten = dwLen;
    }
    return dwLen == aCount ? SY_OK : SY_ERROR_PARTIAL_DATA;
}


///////////////////////////////////////////////////////////////
// class CSyDataBlockNoMalloc
///////////////////////////////////////////////////////////////

void CSyStopFlag::SetStartFlag()
{
    m_Est.EnsureSingleThread();
    SY_ASSERTE(m_bStoppedFlag);
    m_bStoppedFlag = FALSE;
}

void CSyStopFlag::SetStopFlag()
{
    m_Est.EnsureSingleThread();
    m_bStoppedFlag = TRUE;
}

void CSyStopFlag::SetStopFlagWithoutThreadCheck(BOOL value)
{
    m_bStoppedFlag = value;
}

END_UTIL_NS

