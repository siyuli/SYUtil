
//#include "SyBase.h"
#include "SyThreadInterface.h"

USE_UTIL_NS

///////////////////////////////////////////////////////////////
// class ISyEvent
///////////////////////////////////////////////////////////////

void ISyEvent::OnDestorySelf()
{
    delete this;
}

ISyEvent::ISyEvent(EVENT_TYPE EventType) :m_EventType(EventType)
{
#ifdef WIN32
    m_Tid = ::GetCurrentThreadId();
#else
    m_Tid = ::pthread_self();
#endif // WIN32
}

ISyEvent::~ISyEvent()
{
}
ISyTimerHandler::ISyTimerHandler()
{

}
ISyTimerHandler::~ISyTimerHandler()
{

}
