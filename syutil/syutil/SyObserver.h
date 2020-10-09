#ifndef SYOBSERVER_H
#define SYOBSERVER_H

#include "SyThreadInterface.h"
#include "SyDebug.h"

START_UTIL_NS

class SY_OS_EXPORT ISyObserver
{
public:
    virtual void OnObserve(LPCSTR aTopic, LPVOID aData = NULL) = 0;

protected:
    virtual ~ISyObserver() { }
};

template <class T>
class CSyObserverEventT : public ISyEvent
{
public:
    CSyObserverEventT(T *aT, ISyObserver *aObserver)
        : m_pT(aT)
        , m_pObserver(aObserver)
    {
    }

    // interface ISyEvent
    virtual SyResult OnEventFire()
    {
        SY_ASSERTE_RETURN(m_pT, SY_ERROR_NULL_POINTER);
        return m_pT->OnObserverEvent(m_pObserver);
    }

private:
    T *m_pT;
    ISyObserver *m_pObserver;
};

END_UTIL_NS

#endif // !SYOBSERVER_H
