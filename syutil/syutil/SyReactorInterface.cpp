
#include "SyReactorInterface.h"
#include "SyAssert.h"

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class ASyEventHandler
//////////////////////////////////////////////////////////////////////

ASyEventHandler::~ASyEventHandler()
{
}

SY_HANDLE ASyEventHandler::GetHandle() const
{
    SY_ASSERTE(!"ASyEventHandler::GetHandle()");
    return SY_INVALID_HANDLE;
}

int ASyEventHandler::OnInput(SY_HANDLE)
{
    SY_ASSERTE(!"ASyEventHandler::OnInput()");
    return -1;
}

int ASyEventHandler::OnOutput(SY_HANDLE)
{
    SY_ASSERTE(!"ASyEventHandler::OnOutput()");
    return -1;
}

int ASyEventHandler::OnException(SY_HANDLE)
{
    SY_ASSERTE(!"ASyEventHandler::OnException()");
    return -1;
}

int ASyEventHandler::OnClose(SY_HANDLE , MASK)
{
    SY_ASSERTE(!"ASyEventHandler::OnClose()");
    return -1;
}


//////////////////////////////////////////////////////////////////////
// class ISyReactor
//////////////////////////////////////////////////////////////////////

ISyReactor::~ISyReactor()
{
}
