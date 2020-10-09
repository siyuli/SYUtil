#include "SyInetAddr.h"

#ifdef SUPPORT_DNS
    #include "SyDnsManager.h"
    #include "SyDns6Manager.h"
#endif //SUPPORT_DNS

#include "SyDebug.h"

USE_UTIL_NS

CSyInetAddr CSyInetAddr::s_InetAddrAny;

CSyInetAddr CSyInetAddr::get_InetAddrAny()
{
    return CSyInetAddr::s_InetAddrAny;
}

SyResult CSyInetAddr::Set(void)
{
    /*
    SyResult rv = SY_OK;
    rv = SetIpAddrByString(NULL, 43134);    //this is arbitrary port for default address
    return rv;
    */

    ::memset(&m_SockAddr, 0, sizeof(m_SockAddr));
    m_SockAddr.sin_family = AF_INET;
    m_port = 0;

    SetIpAddrBy4Bytes(INADDR_ANY);

    ::memset(&m_SockAddr6, 0, sizeof(m_SockAddr6));
    m_SockAddr6.sin6_port = 0;
    m_SockAddr6.sin6_addr = in6addr_any;
    m_SockAddr6.sin6_family = AF_INET6;
    
    return SY_OK;

}

SyResult CSyInetAddr::SetWithoutResolve(LPCSTR aIpAddr, WORD aPort)
{
    m_port = aPort;
    
    ::memset(&m_SockAddr, 0, sizeof(m_SockAddr));
    m_SockAddr.sin_family = AF_INET;
    m_SockAddr.sin_port = htons(aPort);
    
    ::memset(&m_SockAddr6, 0, sizeof(m_SockAddr6));
    m_SockAddr6.sin6_port = htons(aPort);
    m_SockAddr6.sin6_family = AF_INET6;
    
    SyResult rv = SetIpAddrByString(aIpAddr, aPort, true);
    if (SY_FAILED(rv)) {
        m_strHostName = aIpAddr;
        m_bIsResolved = FALSE;
    }
    
    return SY_OK;
}

SyResult CSyInetAddr::Set(LPCSTR aHostName, WORD aPort, bool async)
{
    m_port = aPort;

    ::memset(&m_SockAddr, 0, sizeof(m_SockAddr));
    m_SockAddr.sin_family = AF_INET;
    m_SockAddr.sin_port = htons(aPort);

    ::memset(&m_SockAddr6, 0, sizeof(m_SockAddr6));
    m_SockAddr6.sin6_port = htons(aPort);
    m_SockAddr6.sin6_family = AF_INET6;

    SyResult rv = SetIpAddrByString(aHostName, aPort, async);
    if (SY_FAILED(rv)) {
#ifdef SUPPORT_DNS
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
        m_strHostName = aHostName;
        m_bIsResolved = FALSE;
        //SY_INFO_TRACE_THIS("does not resolve to fix issue");
        if (async) {
            rv = TryResolve();
        }
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
#else
        SY_ERROR_TRACE_THIS("Dns is not supported, need numeric ip address");
#endif //SUPPORT_DNS
    }

    return rv;
}

SyResult CSyInetAddr::Set(LPCSTR aIpAddrAndPort)
{
    WORD wPort = 0;
    SY_ASSERTE_RETURN(aIpAddrAndPort, SY_ERROR_INVALID_ARG);
    char *szFind = const_cast<char *>(::strchr(aIpAddrAndPort, ':'));
    if (!szFind) {
        SY_WARNING_TRACE_THIS("CSyInetAddr::Set, unknow aIpAddrAndPort=" << aIpAddrAndPort);
        //      SY_ASSERTE(FALSE);
        //      return SY_ERROR_INVALID_ARG;
        szFind = const_cast<char *>(aIpAddrAndPort) + strlen(aIpAddrAndPort);
        wPort = 0;
    } else {
        wPort = static_cast<WORD>(::atoi(szFind + 1));
    }

    // 256 bytes is enough, otherwise the ip string is possiblly wrong.
    char szBuf[256];
    int nAddrLen = static_cast<int>(szFind - aIpAddrAndPort);
    SY_ASSERTE_RETURN((size_t)nAddrLen < sizeof(szBuf), SY_ERROR_NOT_AVAILABLE);
    ::memcpy(szBuf, aIpAddrAndPort, nAddrLen);
    szBuf[nAddrLen] = '\0';

    return Set(szBuf, wPort);
}

SyResult CSyInetAddr::SetIpAddrByString(LPCSTR aIpAddr, WORD aPort, bool async)
{

    char svc[16];

    sprintf(svc, "%d", aPort);

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (async) {
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    }

    int nError = getaddrinfo(aIpAddr, svc, &hints, &res);
    if (nError) {
        //SY_ERROR_TRACE_THIS("getaddrinfo failed, errno: " << nError);
        return SY_ERROR_FAILURE;
    }

    m_family = res->ai_family;
    if (m_family == AF_INET6) {
        ::memcpy(&m_SockAddr6, res->ai_addr, res->ai_addrlen);
    } else {
        ::memcpy(&m_SockAddr, res->ai_addr, res->ai_addrlen);
    }

    SetPort(aPort);

    m_bIsResolved = TRUE;

    // Ricky, 2013-3-21 14:53:28
    // Fixed for memory leak of res
    freeaddrinfo(res);

    return SY_OK;

}

CSyString CSyInetAddr::IpAddr4BytesToString(DWORD aIpDword)
{

#ifdef SY_WIN32
    struct in_addr inAddr;
    inAddr.s_addr = aIpDword;
    LPCSTR pAddr = ::inet_ntoa(inAddr);
#elif defined (SY_LINUX) || defined (SY_MACOS)
    char szBuf[INET_ADDRSTRLEN];
    LPCSTR pAddr = ::inet_ntop(AF_INET, &aIpDword, szBuf, sizeof(szBuf));
#else//by now, is SY_SOLARIS
    char szBuf[128];
    memset(szBuf, 0, 128);
    aIpDword = ntohl(aIpDword);
    sprintf(szBuf,"%d.%d.%d.%d", (aIpDword>>24), (aIpDword&0x00FFFFFF)>>16, (aIpDword&0x0000ffff)>>8, (aIpDword&0x000000ff));
    LPCSTR pAddr = szBuf;
#endif // SY_WIN32
    return CSyString(pAddr);
}

#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
SyResult CSyInetAddr::TryResolve()
{
#ifdef SUPPORT_DNS

    SY_ASSERTE_RETURN(!IsResolved(), SY_OK);

    // try to get ip addr from DNS
    SyResult rv;
    if(m_bDnsProxy) {
        CSyComAutoPtr<CSyDnsRecord> pRecord;
        rv = CSyProxyDnsManager::Instance()->AsyncResolve(
                      pRecord.ParaOut(),
                      m_strHostName,
                      NULL);
    }
    else {
        CSyComAutoPtr<CSyDnsRecord> pRecord;
        rv = CSyDnsManager::Instance()->AsyncResolve(
                      pRecord.ParaOut(),
                      m_strHostName,
                      NULL);
    }

    //for ipv6 resolve

    CSyComAutoPtr<CSyDns6Record> pRecord6;
    rv = CSyDns6Manager::Instance()->AsyncResolve(
             pRecord6.ParaOut(),
             m_strHostName,
             NULL);

    /*
    if (SY_SUCCEEDED(rv)) {
        DWORD dwIp = *(pRecord->begin());
        SetIpAddrBy4Bytes(dwIp);
    }
    else {
        SY_ASSERTE(!IsResolved());
    }
    */
    return rv;
#else
    SY_WARNING_TRACE("Proxy is not supported");
    return SY_ERROR_FAILURE;
#endif //SUPPORT_DNS
}
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME

#if 0
DWORD CSyInetAddr::ResolveSynchAndCache(LPCSTR aHostName)
{
    // TODO: Async resolve.
    typedef std::map<CSyString, DWORD> Host2IpType;
    static Host2IpType s_mapHost2Ip;
    Host2IpType::iterator iter = s_mapHost2Ip.find(aHostName);
    if (iter != s_mapHost2Ip.end()) {
        return (*iter).second;
    }

    DWORD dwRet = INADDR_NONE;
    struct hostent *pHeResult = NULL;
#ifdef SY_LINUX
    struct hostent heResultBuf;
    struct hostent *pheResultBuf = &heResultBuf;
    char szHostentData[4*1024];
    int nHErr;
    if (::gethostbyname_r(aHostName,
                          pheResultBuf,
                          szHostentData,
                          sizeof(szHostentData),
                          &pheResultBuf,
                          &nHErr) == 0) {
        pHeResult = &heResultBuf;
    }
#else // !SY_LINUX
    pHeResult = ::gethostbyname(aHostName);
#endif
    if (!pHeResult) {
        SY_ERROR_TRACE_THIS("CSyInetAddr::ResolveSynchAndCache, gethostbyname() failed!"
                            " name=" << aHostName <<
                            " err=" << errno);
        dwRet = INADDR_NONE;
    } else {
        ::memcpy(&dwRet, pHeResult->h_addr, pHeResult->h_length);
        s_mapHost2Ip.insert(std::make_pair(CSyString(aHostName), dwRet));
    }
    return dwRet;
}
#endif

SyResult CSyInetAddr::SetIpAddrBySock(struct sockaddr_storage *sock, int len)
{
    if (!sock) {
        return SY_ERROR_FAILURE;
    }

    m_family = sock->ss_family;

    if (m_family == AF_INET) {
        ::memcpy(&m_SockAddr, sock, sizeof(m_SockAddr));
        m_SockAddr.sin_port = htons(m_port);
    } else {
        ::memcpy(&m_SockAddr6, sock, sizeof(m_SockAddr6));
        m_SockAddr6.sin6_port = htons(m_port);
    }

    m_bIsResolved = TRUE;
    return SY_OK;
}

SyResult CSyInetAddr::SetIpAddrPortBySock(struct sockaddr_storage *sock, int len)
{
    if (!sock) {
        return SY_ERROR_FAILURE;
    }

    m_family = sock->ss_family;

    if (m_family == AF_INET) {
        ::memcpy(&m_SockAddr, sock, sizeof(m_SockAddr));
        m_port = ntohs(m_SockAddr.sin_port);
    } else {
        ::memcpy(&m_SockAddr6, sock, sizeof(m_SockAddr6));
        m_port = ntohs(m_SockAddr6.sin6_port);
    }

    m_bIsResolved = TRUE;
    return SY_OK;
}

bool cm_validate_ipv4(const struct in6_addr &prefix, int8_t a, int8_t b, int8_t c, int8_t d)
{
    //{0xc0, 0x00, 0x00, 0xaa}
    if ((prefix.s6_addr[a] == 0xc0) &&
            (prefix.s6_addr[b] == 0) &&
            (prefix.s6_addr[c] == 0) &&
            (prefix.s6_addr[d] == 0xaa || prefix.s6_addr[d] == 0xab)) {
        return true;
    }
    return false;
}

void cm_write_ipv4_to_ipv6(struct in6_addr &prefix, struct in_addr ipv4, int8_t a, int8_t b, int8_t c, int8_t d)
{
    uint8_t *pIpv4 = (uint8_t *)&(ipv4.s_addr);

    prefix.s6_addr[a] = pIpv4[0];
    prefix.s6_addr[b] = pIpv4[1];
    prefix.s6_addr[c] = pIpv4[2];
    prefix.s6_addr[d] = pIpv4[3];
}

SyResult CSyInetAddr::ParsePrefixLength(struct sockaddr_storage *sock,
                                        int len,
                                        struct in6_addr &prefix,
                                        unsigned int &nPrefixLen)
{
    struct sockaddr_in6 sockAddr6;
    SY_ASSERTE_RETURN(len == sizeof(sockAddr6), SY_ERROR_INVALID_ARG);

    ::memcpy(&sockAddr6, sock, len);
    memcpy(prefix.s6_addr, sockAddr6.sin6_addr.s6_addr, sizeof(prefix));
    SyResult ret = SY_OK;
    if (cm_validate_ipv4(prefix, 12, 13, 14, 15)) {
        nPrefixLen = 96;
    } else if (cm_validate_ipv4(prefix, 9, 10, 11, 12)) {
        nPrefixLen = 64;
    } else if (cm_validate_ipv4(prefix, 7, 9, 10, 11)) {
        nPrefixLen = 56;
    } else if (cm_validate_ipv4(prefix, 6, 7, 9, 10)) {
        nPrefixLen = 48;
    } else if (cm_validate_ipv4(prefix, 5, 6, 7, 9)) {
        nPrefixLen = 40;
    } else if (cm_validate_ipv4(prefix, 4, 5, 6, 7)) {
        nPrefixLen = 32;
    } else {
        nPrefixLen = 0;
        ret = SY_ERROR_NOT_FOUND;
    }
    return ret;
}

SyResult CSyInetAddr::XLAT46(struct in6_addr prefix, unsigned int nPrefixLen)
{
    if (!m_bIsResolved) {
        SY_WARNING_TRACE_THIS("CSyInetAddr::XLAT46, XLAT can only apply to the resolved IPv4 address, for FQDN, DNS64 will help do the translation.");
        return SY_ERROR_NOT_AVAILABLE;
    }

    if (m_family != AF_INET) {
        SY_WARNING_TRACE_THIS("CSyInetAddr::XLAT46, can only translate IPv4 address to IPv6. m_family=" << m_family);
        return SY_ERROR_INVALID_ARG;
    }

    memset(&m_SockAddr6, 0, sizeof(m_SockAddr6));
    m_SockAddr6.sin6_family = AF_INET6;
    m_SockAddr6.sin6_port = m_SockAddr.sin_port;
    memcpy(m_SockAddr6.sin6_addr.s6_addr, prefix.s6_addr, sizeof(prefix));
    if (nPrefixLen == 96) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,12, 13, 14, 15);
    } else if (nPrefixLen == 64) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,9, 10, 11, 12);
    } else if (nPrefixLen == 56) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,7, 9, 10, 11);
    } else if (nPrefixLen == 48) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,6, 7, 9, 10);
    } else if (nPrefixLen == 40) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,5, 6, 7, 9);
    } else if (nPrefixLen == 32) {
        cm_write_ipv4_to_ipv6(m_SockAddr6.sin6_addr, m_SockAddr.sin_addr,4, 5, 6, 7);
    } else {
        return SY_ERROR_UNEXPECTED;
    }

    m_family = AF_INET6;

    return SY_OK;
}

SyResult CSyInetAddr::XLAT46Sync()
{
    CSyInetAddr addrIpv4;
    addrIpv4.Set("ipv4only.arpa.", 0, false);
    struct in6_addr prefix;
    unsigned int nPrefixLen = 0;
    if (addrIpv4.GetType() == AF_INET) {
        SY_ERROR_TRACE_THIS("CSyInetAddr::XLAT46Sync, pure IP v6 network, cannot get prefix, use well-known prefix instad.");
        prefix = {{{ 0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}};
        nPrefixLen = 96;
    } else {
        ParsePrefixLength((struct sockaddr_storage*)addrIpv4.GetPtr(), sizeof(sockaddr_in6), prefix, nPrefixLen);
    }
    
    return XLAT46(prefix, nPrefixLen);
}

CSyString CSyInetAddr::GetIpDisplayName() const
{
#ifdef SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    if (!IsResolved()) {
        return m_strHostName;
    }
#endif // SY_SUPPORT_ASYNC_RESOLVE_HOSTNAME
    if (m_family == AF_INET) {
        return IpAddr4BytesToString(m_SockAddr.sin_addr.s_addr);
    } else {

        char ip[INET6_ADDRSTRLEN] = {0};
#ifdef SY_WIN32
        int ret;

        if (ipv6_dns_resolve())
            ret = tp_getnameinfo((struct sockaddr *)&m_SockAddr6, sizeof(m_SockAddr6),
                                 ip, INET6_ADDRSTRLEN, NULL, 0,
                                 NI_NUMERICHOST|NI_NUMERICSERV);
        else {
            ret = -1;
        }
#else
        int ret = ::getnameinfo((struct sockaddr *)&m_SockAddr6, sizeof(m_SockAddr6),
                                ip, INET6_ADDRSTRLEN, NULL, 0,
                                NI_NUMERICHOST|NI_NUMERICSERV);
#endif
        if (ret != 0) {
            return CSyString(NULL);
        }

        return CSyString(ip);
    }
}
