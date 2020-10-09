#ifndef SYREACTORINTERFACE_H
#define SYREACTORINTERFACE_H

#ifndef SY_USE_REACTOR_SELECT
    //#define SY_USE_REACTOR_SELECT
#endif // SY_USE_REACTOR_SELECT

#if defined (SY_WIN32)
    //  #define SY_ENABLE_CALENDAR_TIMER
#elif defined (SY_LINUX)
    #define SY_ENABLE_CALENDAR_TIMER
#endif // SY_WIN32

//#include "SyDefines.h"
#include "SyThreadInterface.h"
#include "stdint.h"

START_UTIL_NS

class CSyTimeValue;
class CSyInetAddr;

/**
 * @class ASyEventHandler
 *
 * @brief Provides an abstract interface for handling various types of I/O events.
 *
 * Subclasses read/write input/output on an I/O descriptor(implemented),
 * handle an exception raised on an I/O descriptor(not implemented yet)
 *
 */
class SY_OS_EXPORT ASyEventHandler
{
public:
    typedef long MASK;
    enum {
        NULL_MASK = 0,
        ACCEPT_MASK = (1 << 0),
        CONNECT_MASK = (1 << 1),
        READ_MASK = (1 << 2),
        WRITE_MASK = (1 << 3),
        EXCEPT_MASK = (1 << 4),
        TIMER_MASK = (1 << 5),
        ALL_EVENTS_MASK = READ_MASK |
                          WRITE_MASK |
                          EXCEPT_MASK |
                          ACCEPT_MASK |
                          CONNECT_MASK |
                          TIMER_MASK,
        SHOULD_CALL = (1 << 6),
        CLOSE_MASK = (1 << 7),
        EVENTQUEUE_MASK = (1 << 8),
        UDP_LINK_MASK = (1 << 9)
    };

    virtual SY_HANDLE GetHandle() const ;

    /// Called when input events occur (e.g., data is ready).
    /// OnClose() will be callbacked if return -1.
    virtual int OnInput(SY_HANDLE aFd = SY_INVALID_HANDLE);

    /// Called when output events are possible (e.g., when flow control
    /// abates or non-blocking connection completes).
    /// OnClose() will be callbacked if return -1.
    virtual int OnOutput(SY_HANDLE aFd = SY_INVALID_HANDLE);

    /// Called when an exceptional events occur (e.g., OOB data).
    /// OnClose() will be callbacked if return -1.
    /// Not implemented yet.
    virtual int OnException(SY_HANDLE aFd = SY_INVALID_HANDLE);

    /**
     * Called when a <On*()> method returns -1 or when the
     * <RemoveHandler> method is called on an <CReactor>.  The
     * <aMask> indicates which event has triggered the
     * <HandleClose> method callback on a particular <aFd>.
     */
    virtual int OnClose(SY_HANDLE aFd, MASK aMask);

    virtual ~ASyEventHandler();
};

/**
 * @class ISyReactor
 *
 * @brief An abstract class for implementing the Reactor Pattern.
 *
 */
class SY_OS_EXPORT ISyReactor : public ISyEventQueue, public ISyTimerQueue
{
public:
    /// Initialization.
    virtual SyResult Open() = 0;

    /**
     * Register <aEh> with <aMask>.  The handle will always
     * come from <GetHandle> on the <aEh>.
     * If success:
     *    if <aEh> is registered, return SY_ERROR_FOUND;
     *    else return SY_OK;
     */
    virtual SyResult RegisterHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask) = 0;

    /**
     * Removes <aEh> according to <aMask>.
     * If success:
     *    if <aEh> is registered
     *       If <aMask> equals or greater than that registered, return SY_OK;
     *       else return SY_ERROR_FOUND;
     *    else return SY_ERROR_NOT_FOUND;
     */
    virtual SyResult RemoveHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask = ASyEventHandler::ALL_EVENTS_MASK) = 0;

    virtual SyResult NotifyHandler(
        ASyEventHandler *aEh,
        ASyEventHandler::MASK aMask) = 0;

    virtual SyResult RunEventLoop() = 0;

    /// this function can be invoked in the different thread.
    virtual SyResult StopEventLoop() = 0;

    /// Close down and release all resources.
    virtual SyResult Close() = 0;

    typedef long PROPERTY;
    enum {
        NULL_PROPERTY = 0,
        SEND_REGISTER_PROPERTY = (1 << 0)
    };

    PROPERTY GetProperty()
    {
        return m_Property;
    }

    ISyReactor(PROPERTY aProperty)
        : m_Property(aProperty)
    {
    }

    virtual ~ISyReactor();

protected:
    PROPERTY m_Property;
};

/**
 * @class ASyConnectorInternal
 *
 * @brief for internal connector, implement TCP, UDP, HTTP, SSL, etc.
 *
 */
class SY_OS_EXPORT ASyConnectorInternal
{
public:
    virtual int Connect(const CSyInetAddr &aAddr, CSyInetAddr *aAddrLocal = NULL) = 0;
    virtual int Close(SyResult aReason) = 0;

    virtual long getExtId() {return -1;}
    virtual int SetLocalPortRange(uint16_t uPortMin, uint16_t uPortMax) { return -1; }

    virtual ~ASyConnectorInternal() { }
};

END_UTIL_NS

#endif // !SYREACTORINTERFACE_H
