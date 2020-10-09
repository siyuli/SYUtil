#ifndef SYINETADDR_H
#define SYINETADDR_H

//#include "SyDefines.h"

#include "SyDef.h"
#include "SyDataType.h"
#include "SyNetwork.h"
#include "SyAssert.h"

#ifdef SY_WIN32
    #include <ws2tcpip.h>
#endif

#include "SyStdCpp.h"
#include "SyError.h"

START_UTIL_NS

#ifndef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    #define SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME 1
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

/// The concept of <CSyInetAddr> is mainly copyed by <ACE_INET_Addr>
/// http://www.cs.wustl.edu/~schmidt/ACE.html
class SY_OS_EXPORT CSyInetAddr
{
public:
    CSyInetAddr();

    /// Creates an <CSyInetAddr> from a <aPort> and the remote
    /// <aHostName>. The port number is assumed to be in host byte order.
    CSyInetAddr(LPCSTR aHostName, WORD aPort);
    CSyInetAddr(LPCSTR aHostName, WORD aPort, bool bDnsProxy);

    CSyInetAddr(const CSyInetAddr &aRight):
        m_port(0),
        m_family(AF_INET),
        m_bLock(FALSE),
        m_bIsResolved(FALSE)
    {
        ::memset(&m_SockAddr, 0, sizeof(m_SockAddr));
        ::memset(&m_SockAddr6, 0, sizeof(m_SockAddr6));
        if (this == &aRight) { //if it is same, do nothing, otherwise the memory should be destroyed RT#HD0000002492809
            return;
        }

        m_family = aRight.m_family;
        if (m_family == AF_INET) {
            memcpy(&m_SockAddr, &aRight.m_SockAddr, sizeof(m_SockAddr));
        } else {
            memcpy(&m_SockAddr6, &aRight.m_SockAddr6, sizeof(m_SockAddr6));
        }
        m_port = aRight.m_port;
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        m_strHostName = aRight.m_strHostName.c_str();
#endif
        m_strUserData = aRight.m_strUserData;
        m_bLock = aRight.m_bLock;
        m_bIsResolved = aRight.m_bIsResolved;
        m_bDnsProxy = aRight.m_bDnsProxy;
    }

    CSyInetAddr &operator=(const CSyInetAddr &aRight)
    {
        if (this == &aRight) { //if it is same, do nothing, otherwise the memory should be destroyed RT#HD0000002492809
            return *this;
        }

        m_family = aRight.m_family;
        if (m_family == AF_INET) {
            memcpy(&m_SockAddr, &aRight.m_SockAddr, sizeof(m_SockAddr));
        } else {
            memcpy(&m_SockAddr6, &aRight.m_SockAddr6, sizeof(m_SockAddr6));
        }
        m_port = aRight.m_port;
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        m_strHostName = aRight.m_strHostName.c_str();
#endif
        m_strUserData = aRight.m_strUserData;
        m_bLock = aRight.m_bLock;
        m_bIsResolved = aRight.m_bIsResolved;
        m_bDnsProxy = aRight.m_bDnsProxy;
        return *this;
    }
    /**
    * Initializes an <CSyInetAddr> from the <aIpAddrAndPort>, which can be
    * "ip-number:port-number" (e.g., "tango.cs.wustl.edu:1234" or
    * "128.252.166.57:1234").  If there is no ':' in the <address> it
    * is assumed to be a port number, with the IP address being
    * INADDR_ANY.
    */
    CSyInetAddr(LPCSTR aIpAddrAndPort);

    SyResult Set(LPCSTR aHostName, WORD aPort, bool async = true);
    SyResult Set(LPCSTR aIpAddrAndPort);
    SyResult Set(void);

    SyResult SetIpAddrBySock(struct sockaddr_storage *sock, int len);
    SyResult SetIpAddrPortBySock(struct sockaddr_storage *sock, int len);
    SyResult SetIpAddrByString(LPCSTR aIpAddr, WORD aPort, bool async = true);
    SyResult SetIpAddrBy4Bytes(DWORD aIpAddr, BOOL aIsNetworkOrder = TRUE);
    SyResult SetWithoutResolve(LPCSTR aIpAddr, WORD aPort);
    SyResult SetPort(WORD aPort);

    /// Compare two addresses for equality.  The addresses are considered
    /// equal if they contain the same IP address and port number.
    bool operator == (const CSyInetAddr &aRight) const;

    // Compare two addresses for larger than. IP address has the higher priority
    bool operator > (const CSyInetAddr &aRight) const;

    /**
     * Returns true if <this> is less than <aRight>.  In this context,
     * "less than" is defined in terms of IP address and TCP port
     * number.  This operator makes it possible to use <ACE_INET_Addr>s
     * in STL maps.
     */
    bool operator < (const CSyInetAddr &aRight) const;

    CSyString GetIpDisplayName() const;

    WORD GetPort() const { return m_port; /*return ntohs(m_SockAddr.sin_port);*/ }

    DWORD GetIpAddrIn4Bytes() const
    {
        SY_ASSERTE(m_family == AF_INET);
        return m_SockAddr.sin_addr.s_addr;
    }

    DWORD GetSize() const
    {
        if (m_family == AF_INET) {
            return sizeof(sockaddr_in);
        } else {
            return sizeof(sockaddr_in6);
        }
    }

    DWORD GetType() const { return m_family; /*return m_SockAddr.sin_family;*/ }
    void SetType(int family)
    {
        SY_ASSERTE(family == AF_INET || family == AF_INET6);
        m_family = family;
    }

    const sockaddr *GetPtr() const
    {
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        SY_ASSERTE(IsResolved());
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        if (m_family == AF_INET) {
            return (sockaddr *)&m_SockAddr;
        } else {
            return (sockaddr *)&m_SockAddr6;
        }
    }
    
    const sockaddr *GetPtr(int &nLen) const
    {
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        SY_ASSERTE(IsResolved());
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        if (m_family == AF_INET) {
            nLen = sizeof(m_SockAddr);
            return (sockaddr *)&m_SockAddr;
        } else {
            nLen = sizeof(m_SockAddr6);
            return (sockaddr *)&m_SockAddr6;
        }
    }

    static BOOL IpAddrStringTo4Bytes(LPCSTR aIpStr, DWORD &aIpDword);
    static CSyString IpAddr4BytesToString(DWORD aIpDword);

#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    BOOL IsResolved() const
    {
        //      return m_strHostName.empty() ? TRUE : FALSE;
        return m_bIsResolved;
    }

    void SetUnResolve(void)
    {
        m_bIsResolved = FALSE;
    }

    CSyString GetHostName() const { return m_strHostName; }

    SyResult TryResolve();
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

    //add by Nick
    void SetUserData(const CSyString strUserData) { m_strUserData = strUserData; }
    CSyString GetUserData() const { return m_strUserData;}

    void Lock() { m_bLock = TRUE; }
    void UnLock() { m_bLock = FALSE; }
    BOOL IsLock() { return m_bLock; }

    //Translate IPv4 address to IPv6 address according to RFC6052, you need to determine the prefix.
    SyResult XLAT46(struct in6_addr prefix, unsigned int nPrefixLen);
    SyResult ParsePrefixLength(struct sockaddr_storage *sock, int len, struct in6_addr &prefix, unsigned int &nPrefixLen);
    SyResult XLAT46Sync();
    bool GetDnsType() const { return m_bDnsProxy;}
    void SetDnsType(bool bDnsProxy)
    {
        m_bDnsProxy = bDnsProxy;
    }

public:
    static CSyInetAddr s_InetAddrAny;
    static CSyInetAddr get_InetAddrAny();

private:
    sockaddr_in m_SockAddr;
    sockaddr_in6 m_SockAddr6;
    WORD m_port;

    int m_family;

#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    // m_strHostName is empty that indicates resovled successfully,
    // otherwise it needs resolving.
    CSyString m_strHostName;
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

    //add by Nick
    CSyString   m_strUserData;
    BOOL        m_bLock;
    //add it to idendtifing the resolve status, then we can keep host name
    BOOL        m_bIsResolved;
    bool m_bDnsProxy;
};


// inline functions
inline CSyInetAddr::CSyInetAddr():m_family(AF_INET), m_bLock(FALSE), m_bIsResolved(FALSE), m_bDnsProxy(false)
{
    Set();
}

inline CSyInetAddr::CSyInetAddr(LPCSTR aHostName, WORD aPort):m_family(AF_INET), m_bLock(FALSE), m_bIsResolved(FALSE), m_bDnsProxy(false)
{
    Set(aHostName, aPort);
}

inline CSyInetAddr::CSyInetAddr(LPCSTR aHostName, WORD aPort, bool bDnsProxy):m_family(AF_INET), m_bLock(FALSE), m_bIsResolved(FALSE), m_bDnsProxy(bDnsProxy)
{
    Set(aHostName, aPort);
}

inline CSyInetAddr::CSyInetAddr(LPCSTR aIpAddrAndPort):m_family(AF_INET), m_bLock(FALSE), m_bIsResolved(FALSE), m_bDnsProxy(false)
{
    Set(aIpAddrAndPort);
}

inline SyResult CSyInetAddr::SetPort(WORD aPort)
{
    m_port = aPort;

    if (m_family == AF_INET) {
        m_SockAddr.sin_port = htons(aPort);
    } else {
        m_SockAddr6.sin6_port = htons(aPort);
    }

    return SY_OK;
}

inline bool CSyInetAddr::operator == (const CSyInetAddr &aRight) const
{
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    SY_ASSERTE(IsResolved());
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

    // don't compare m_SockAddr.sin_zero due to getpeername() or getsockname()
    // will fill it with non-zero value.
    if (this->GetType() != aRight.GetType()) {
        return false;
    }
    if (m_family == AF_INET) {
        return (::memcmp(
                    &m_SockAddr,
                    &aRight.m_SockAddr,
                    sizeof(m_SockAddr) - sizeof(m_SockAddr.sin_zero)) == 0);
    } else {
        return (m_port == aRight.m_port
                && memcmp((void *)&m_SockAddr6.sin6_addr, (void *)&aRight.m_SockAddr6.sin6_addr, sizeof(struct in6_addr)) == 0);
    }

}

inline bool CSyInetAddr::operator > (const CSyInetAddr &aRight) const
{
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    SY_ASSERTE(IsResolved());
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

    // don't compare m_SockAddr.sin_zero due to getpeername() or getsockname()
    // will fill it with non-zero value.

    if (this->GetType() > aRight.GetType()) {
        return true;
    } else if (this->GetType() < aRight.GetType()) {
        return false;
    }
    if (m_family == AF_INET) {
        if (::memcmp(&m_SockAddr, &aRight.m_SockAddr, sizeof(m_SockAddr) - sizeof(m_SockAddr.sin_zero)) > 0) {
            return TRUE;
        }
        if (::memcmp(&m_SockAddr, &aRight.m_SockAddr, sizeof(m_SockAddr) - sizeof(m_SockAddr.sin_zero)) == 0 &&
                m_SockAddr.sin_port > aRight.m_SockAddr.sin_port) {
            return TRUE;
        }
        return FALSE;
    } else if (m_family == AF_INET6) {
        if (::memcmp((void *)&m_SockAddr6.sin6_addr, (void *)&aRight.m_SockAddr6.sin6_addr, sizeof(struct in6_addr)) > 0) {
            return TRUE;
        }
        if (::memcmp((void *)&m_SockAddr6.sin6_addr, (void *)&aRight.m_SockAddr6.sin6_addr, sizeof(struct in6_addr)) == 0 &&
                m_SockAddr6.sin6_port > aRight.m_SockAddr6.sin6_port) {
            return TRUE;
        }

        return FALSE;
    } else {
        //SY_ERROR_TRACE_THIS("Un-Supported socket type: " << (int)m_family);

        return FALSE;
    }

}

inline bool CSyInetAddr::operator < (const CSyInetAddr &aRight) const
{
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    SY_ASSERTE(IsResolved());
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    if (this->GetType() > aRight.GetType()) {
        return false;
    } else if (this->GetType() < aRight.GetType()) {
        return true;
    }
    if (m_family == AF_INET) {
        return m_SockAddr.sin_addr.s_addr < aRight.m_SockAddr.sin_addr.s_addr
               || (m_SockAddr.sin_addr.s_addr == aRight.m_SockAddr.sin_addr.s_addr
                   && m_SockAddr.sin_port < aRight.m_SockAddr.sin_port);
    } else {
        return strcmp((char *)m_SockAddr6.sin6_addr.s6_addr, (char *)aRight.m_SockAddr6.sin6_addr.s6_addr) < 0
               || (strcmp((char *)m_SockAddr6.sin6_addr.s6_addr, (char *)aRight.m_SockAddr6.sin6_addr.s6_addr) == 0
                   && m_port < aRight.m_port);
    }

}

inline SyResult CSyInetAddr::SetIpAddrBy4Bytes(DWORD aIpAddr, BOOL aIsNetworkOrder)
{
    SY_ASSERTE(m_family == AF_INET);

    if (aIsNetworkOrder) {
        m_SockAddr.sin_addr.s_addr = aIpAddr;
    } else {
        m_SockAddr.sin_addr.s_addr = htonl(aIpAddr);
    }

    m_bIsResolved = TRUE;
    return SY_OK;
}

inline BOOL CSyInetAddr::IpAddrStringTo4Bytes(LPCSTR aIpStr, DWORD &aIpDword)
{
    aIpDword = INADDR_ANY;
    BOOL bAddrOk = TRUE;
    if (aIpStr && *aIpStr) {
        bAddrOk = ::inet_pton(AF_INET, aIpStr, &aIpDword) > 0 ? TRUE : FALSE;
    }
    return bAddrOk;
}

END_UTIL_NS

#endif // !SYINETADDR_H
