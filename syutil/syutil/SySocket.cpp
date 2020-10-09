
#include "SySocket.h"
//#include "SyUtilClasses.h"
#include "SyUtilMisc.h"

#include "SyDebug.h"

#if defined (SY_SUPPORT_QOS) && defined (SY_WIN32)
    #include <Qos.h>
#endif // SY_SUPPORT_QOS && SY_WIN32

#include <random>

USE_UTIL_NS

//////////////////////////////////////////////////////////////////////
// class CSyIPCBase
//////////////////////////////////////////////////////////////////////

int CSyIPCBase::Enable(int aValue) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    switch (aValue) {
        case NON_BLOCK: {
#ifdef SY_WIN32
            u_long nonblock = 1;
            int nRet = ::ioctlsocket((SY_SOCKET)m_Handle, FIONBIO, &nonblock);
            if (nRet == SOCKET_ERROR) {
                errno = ::WSAGetLastError();
                nRet = -1;
            }
            return nRet;

#else // !SY_WIN32
            int nVal = ::fcntl(m_Handle, F_GETFL, 0);
            if (nVal == -1) {
                return -1;
            }
            nVal |= O_NONBLOCK;
            if (::fcntl(m_Handle, F_SETFL, nVal) == -1) {
                return -1;
            }
            return 0;
#endif // SY_WIN32
        }

        default:
            SY_ERROR_TRACE("CSyIPCBase::Enable, aValue=" << aValue);
            return -1;
    }
}

int CSyIPCBase::Disable(int aValue) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    switch (aValue) {
        case NON_BLOCK: {
#ifdef SY_WIN32
            u_long nonblock = 0;
            int nRet = ::ioctlsocket((SY_SOCKET)m_Handle, FIONBIO, &nonblock);
            if (nRet == SOCKET_ERROR) {
                errno = ::WSAGetLastError();
                nRet = -1;
            }
            return nRet;

#else // !SY_WIN32
            int nVal = ::fcntl(m_Handle, F_GETFL, 0);
            if (nVal == -1) {
                return -1;
            }
            nVal &= ~O_NONBLOCK;
            if (::fcntl(m_Handle, F_SETFL, nVal) == -1) {
                return -1;
            }
            return 0;
#endif // SY_WIN32
        }

        default:
            SY_ERROR_TRACE("CSyIPCBase::Disable, aValue=" << aValue);
            return -1;
    }
}

int CSyIPCBase::Control(int aSyd, void *aArg) const
{
    int nRet;
#ifdef SY_WIN32
    nRet = ::ioctlsocket((SY_SOCKET)m_Handle, aSyd, static_cast<unsigned long *>(aArg));
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#elif defined (SY_MACOS)
    nRet = ::ioctl(m_Handle, aSyd, (char *)aArg);
#else
    nRet = ::ioctl(m_Handle, aSyd, aArg);
#endif // SY_WIN32
    return nRet;
}


//////////////////////////////////////////////////////////////////////
// class CSySocketBase
//////////////////////////////////////////////////////////////////////

int CSySocketBase::Open(int aFamily, int aType, int aProtocol, BOOL aReuseAddr)
{
    int nRet = -1;
    Close();

#if defined (SY_SUPPORT_QOS) && defined (SY_WIN32)
#error not support TCP QoS
#endif

    m_Handle = (SY_HANDLE)::socket(aFamily, aType, aProtocol);
    if (m_Handle != SY_INVALID_HANDLE) {
        nRet = 0;
        if (aFamily != PF_UNIX && aReuseAddr) {
            int nReuse = 1;
            nRet = SetOption(SOL_SOCKET, SO_REUSEADDR, &nReuse, sizeof(nReuse));
        }
    }

    if(0 == nRet){
#if !defined (SY_WIN32)
        int iFlags = fcntl(m_Handle, F_GETFD, 0);
        if(0 <=iFlags)
            fcntl(m_Handle, F_SETFD, iFlags | FD_CLOEXEC);
#endif
    }

    if (nRet == -1) {
        CSyErrnoGuard theGuard;
        Close();
    }
    return nRet;
}

int CSySocketBase::GetRemoteAddr(CSyInetAddr &aAddr) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);

    struct sockaddr_storage addr;
    int len;

    len = sizeof addr;
    int nGet = ::getpeername((SY_SOCKET)m_Handle, (sockaddr *)&addr,
#ifdef SY_WIN32
                             &len);
#else
                             reinterpret_cast<socklen_t *>(&len));
#endif
#ifdef SY_WIN32
    if (nGet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nGet = -1;
    }
#endif

    if (nGet != -1) {
        if (addr.ss_family == AF_INET) {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in));
        } else {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in6));
        }

        //SY_INFO_TRACE_THIS("remote addr: " << aAddr.GetIpDisplayName());
    }

    /*
        int nSize = (int)aAddr.GetSize();
        int nGet = ::getpeername((SY_SOCKET)m_Handle,
                        (sockaddr *)aAddr.GetPtr(),
    #ifdef SY_WIN32
                        &nSize
    #else // !SY_WIN32
    #ifdef SY_MACOS
            //      #ifdef __i386__
                        reinterpret_cast<socklen_t*>(&nSize)
            //      #else
            //          &nSize
            //      #endif
    #else
                        reinterpret_cast<socklen_t*>(&nSize)
    #endif
    #endif // SY_WIN32
                        );

    #ifdef SY_WIN32
        if (nGet == SOCKET_ERROR) {
            errno = ::WSAGetLastError();
            nGet = -1;
        }
    #endif // SY_WIN32
    */
    return nGet;
}

int CSySocketBase::GetLocalAddr(CSyInetAddr &aAddr) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);

    struct sockaddr_storage addr;
    int len;

    len = sizeof addr;
    int nGet = ::getsockname((SY_SOCKET)m_Handle, (sockaddr *)&addr,
#ifdef SY_WIN32
                             &len);
#else
                             reinterpret_cast<socklen_t *>(&len));
#endif
#ifdef SY_WIN32
    if (nGet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nGet = -1;
    }
#endif

    if (nGet != -1) {
        if (addr.ss_family == AF_INET) {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in));
        } else {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in6));
        }

        //SY_INFO_TRACE_THIS("local addr: " << aAddr.GetIpDisplayName());
    }

    /*
        int nSize = (int)aAddr.GetSize();
        int nGet = ::getsockname((SY_SOCKET)m_Handle,
                        (sockaddr *)aAddr.GetPtr(),
    #ifdef SY_WIN32
                        &nSize
    #else // !SY_WIN32
    #ifdef SY_MACOS
            //      #ifdef __i386__
                        reinterpret_cast<socklen_t*>(&nSize)
            //      #else
            //          &nSize
            //      #endif
    #else
                        reinterpret_cast<socklen_t*>(&nSize)
    #endif
    #endif // SY_WIN32
                        );

    #ifdef SY_WIN32
        if (nGet == SOCKET_ERROR) {
            errno = ::WSAGetLastError();
            nGet = -1;
        }
    #endif // SY_WIN32
    */
    return nGet;
}

int CSySocketBase::Close()
{
    int nRet = 0;
    if (m_Handle != SY_INVALID_HANDLE) {
#ifdef SY_WIN32
        nRet = ::closesocket((SY_SOCKET)m_Handle);
        if (nRet == SOCKET_ERROR) {
            errno = ::WSAGetLastError();
            nRet = -1;
        }
#else
        nRet = ::close((SY_SOCKET)m_Handle);
#endif
        m_Handle = SY_INVALID_HANDLE;
    }
    return nRet;
}

int CSySocketBase::Bind(const CSyInetAddr &aLocal)
{
    return ::bind((SY_SOCKET)m_Handle,
           reinterpret_cast<const sockaddr *>(aLocal.GetPtr()),
#ifdef SY_WIN32
           aLocal.GetSize()
#else // !SY_WIN32
#ifdef SY_MACOS
#ifdef __i386__
           static_cast<socklen_t>(aLocal.GetSize())
#else
           aLocal.GetSize()
#endif
#else
           static_cast<socklen_t>(aLocal.GetSize())
#endif
#endif // SY_WIN32
       );
}


//////////////////////////////////////////////////////////////////////
// class CSySocketTcp
//////////////////////////////////////////////////////////////////////

int CSySocketTcp::Open(BOOL aReuseAddr, const CSyInetAddr &aLocal)
{
    if (CSySocketBase::Open(aLocal.GetType(), SOCK_STREAM, 0, aReuseAddr) == -1) {
        return -1;
    }

    if (::bind((SY_SOCKET)m_Handle,
               reinterpret_cast<const sockaddr *>(aLocal.GetPtr()),
#ifdef SY_WIN32
               aLocal.GetSize()
#else // !SY_WIN32
#ifdef SY_MACOS
#ifdef __i386__
               static_cast<socklen_t>(aLocal.GetSize())
#else
               aLocal.GetSize()
#endif
#else
               static_cast<socklen_t>(aLocal.GetSize())
#endif
#endif // SY_WIN32
              ) == -1) {
        CSyErrnoGuard theGuard;
        Close();
        return -1;
    }
    return 0;
}

int CSySocketTcp::Open(CSyInetAddr &aLocal, uint16_t uLocalPortMin, uint16_t uLocalPortMax)
{
    BOOL aReuseAddr = TRUE;
    if (CSySocketBase::Open(aLocal.GetType(), SOCK_STREAM, 0, aReuseAddr) == -1) {
        SY_ERROR_TRACE_THIS("CSyConnectorTcpT::Open Open() failed!"
                            " laddr=" << aLocal.GetIpDisplayName() <<
                            " lport=" << aLocal.GetPort() <<
                            " err=" << errno);
        return -1;
    }

    std::random_device rd;
    std::mt19937_64 generator{rd()};
    std::uniform_int_distribution<uint16_t> dist;
    if (uLocalPortMax > uLocalPortMin && uLocalPortMin > 1024) {
        uint16_t nStart = uLocalPortMin + dist(generator) % (uLocalPortMax - uLocalPortMin);
        int i = uLocalPortMin;
        for (; i < uLocalPortMax; i++, nStart++) {
            uint16_t nPort = uLocalPortMin + nStart % (uLocalPortMax - uLocalPortMin);
            aLocal.SetPort(nPort);
            if (Bind(aLocal) != -1) {
                CSyErrnoGuard theGuard;
                break;
            }
        }
        if (i == uLocalPortMax) {
            Close();
            SY_ERROR_TRACE_THIS("CSyConnectorTcpT::Open Bind() failed!"
                                " laddr=" << aLocal.GetIpDisplayName() <<
                                " lport=" << aLocal.GetPort() <<
                                " err=" << errno);

            return -1;
        }
    } else {
        if (Bind(aLocal) == -1) {
            CSyErrnoGuard theGuard;
            Close();
            SY_ERROR_TRACE_THIS("CSyConnectorTcpT::Open Bind() failed!"
                                " laddr=" << aLocal.GetIpDisplayName() <<
                                " lport=" << aLocal.GetPort() <<
                                " err=" << errno);

            return -1;
        }
    }
    
    CSyInetAddr addr;
    int ret = GetLocalAddr(addr);
    
    if (ret != -1) {
        SY_DEBUG_TRACE_THIS("CSySocketTcp::Open, after bind, local addr: " << addr.GetIpDisplayName() <<", port: " << addr.GetPort());
        aLocal = addr;
    }
    
    return 0;
}

//////////////////////////////////////////////////////////////////////
// class CSySocketUdp
//////////////////////////////////////////////////////////////////////

int CSySocketUdp::Open(CSyInetAddr &aLocal, uint16_t uLocalPortMin, uint16_t uLocalPortMax)
{
    if (CSySocketBase::Open(aLocal.GetType(), SOCK_DGRAM, 0, FALSE) == -1) {
        return -1;
    }
    
    std::random_device rd;
    std::mt19937_64 generator{rd()};
    std::uniform_int_distribution<uint16_t> dist;
    if (uLocalPortMax > uLocalPortMin && uLocalPortMin > 1024) {
        uint16_t nStart = uLocalPortMin + dist(generator) % (uLocalPortMax - uLocalPortMin);
        int i = uLocalPortMin;
        for (; i < uLocalPortMax; i++, nStart++) {
            uint16_t nPort = uLocalPortMin + nStart % (uLocalPortMax - uLocalPortMin);
            aLocal.SetPort(nPort);
            if (Bind(aLocal) != -1) {
                CSyErrnoGuard theGuard;
                break;
            }
        }
        if (i == uLocalPortMax) {
            Close();
            return -1;
        }
    } else {
        if (Bind(aLocal) == -1) {
            CSyErrnoGuard theGuard;
            Close();
            return -1;
        }
    }

    CSyInetAddr addr;
    int ret = GetLocalAddr(addr);

    if (ret != -1) {
        SY_DEBUG_TRACE_THIS("CSySocketUdp::Open, after bind, local addr: " << addr.GetIpDisplayName() <<", port: " << addr.GetPort());
        aLocal = addr;
    }
    
#ifdef SY_IOS
    
//#define SY_NDA
#ifdef SY_NDA
    
#ifndef SO_TRAFFIC_CLASS
#define SO_TRAFFIC_CLASS        0x1086  /* Traffic service class (int) */
    
#define  SO_TC_BK_SYS   100		/* lowest class */
#define  SO_TC_BK       200
#define  SO_TC_BE       0
#define  SO_TC_RD       300
#define  SO_TC_OAM      400
#define  SO_TC_AV       500
#define  SO_TC_RV       600
#define  SO_TC_VI       700
#define  SO_TC_VO       800
#define  SO_TC_CTL      900		/* highest class */
#define  SO_TC_MAX      10		/* Total # of traffic classes */
#endif // SO_TRAFFIC_CLASS
    
    int opt = SO_TC_BE;
    SetOption(SOL_SOCKET, SO_TRAFFIC_CLASS, &opt, sizeof(opt));
#endif // SY_NDA
    
#endif // SY_IOS

    return 0;
}
