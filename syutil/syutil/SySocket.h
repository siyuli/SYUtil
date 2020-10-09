#ifndef SYSOCKET_H
#define SYSOCKET_H

//#include "SyDefines.h"
#include "SyError.h"
#include "SyInetAddr.h"
#if defined(LINUX) || defined(SY_ANDROID)
    #include <sys/uio.h>
#endif

START_UTIL_NS

class SY_OS_EXPORT CSyIPCBase
{
public:
    enum { NON_BLOCK = 0 };

    CSyIPCBase() : m_Handle(SY_INVALID_HANDLE) { }

    SY_HANDLE GetHandle() const;
    void SetHandle(SY_HANDLE aNew);

    int Enable(int aValue) const ;
    int Disable(int aValue) const ;
    int Control(int aSyd, void *aArg) const;

protected:
    SY_HANDLE m_Handle;
};


class SY_OS_EXPORT CSySocketBase : public CSyIPCBase
{
protected:
    CSySocketBase();
    ~CSySocketBase();

public:
    /// Wrapper around the BSD-style <socket> system call (no QoS).
    int Open(int aFamily, int aType, int aProtocol, BOOL aReuseAddr);

    /// Close down the socket handle.
    int Close();

    /// Wrapper around the <setsockopt> system call.
    int SetOption(int aLevel, int aOption, const void *aOptval, int aOptlen) const ;

    /// Wrapper around the <getsockopt> system call.
    int GetOption(int aLevel, int aOption, void *aOptval, int *aOptlen) const ;

    /// Return the address of the remotely connected peer (if there is
    /// one), in the referenced <aAddr>.
    int GetRemoteAddr(CSyInetAddr &aAddr) const;

    /// Return the local endpoint address in the referenced <aAddr>.
    int GetLocalAddr(CSyInetAddr &aAddr) const;

    /// Recv an <aLen> byte buffer from the connected socket.
    int Recv(char *aBuf, DWORD aLen, int aFlag = 0) const ;

    /// Recv an <aIov> of size <aCount> from the connected socket.
    int RecvV(iovec aIov[], DWORD aCount) const ;

    /// Send an <aLen> byte buffer to the connected socket.
    int Send(const char *aBuf, DWORD aLen, int aFlag = 0) const ;

    /// Send an <aIov> of size <aCount> from the connected socket.
    int SendV(const iovec aIov[], DWORD aCount) const ;
    
    int Bind(const CSyInetAddr &aLocal);
};


class SY_OS_EXPORT CSySocketTcp : public CSySocketBase
{
public:
    CSySocketTcp();
    ~CSySocketTcp();

    int Open(BOOL aReuseAddr = FALSE, int family = AF_INET);
    int Open(BOOL aReuseAddr, const CSyInetAddr &aLocal);
    int Open(CSyInetAddr &aLocal, uint16_t uLocalPortMin, uint16_t uLocalPortMax);
    int Close(SyResult aReason = SY_OK);
    int CloseWriter();
    int CloseReader();
};

class SY_OS_EXPORT CSySocketUdp : public CSySocketBase
{
public:
    CSySocketUdp();
    ~CSySocketUdp();

    int Open(CSyInetAddr &aLocal, uint16_t uLocalPortMin, uint16_t uLocalPortMax);

    int RecvFrom(char *aBuf,
                 DWORD aLen,
                 CSyInetAddr &aAddr,
                 int aFlag = 0) const ;

    int SendTo(const char *aBuf,
               DWORD aLen,
               const CSyInetAddr &aAddr,
               int aFlag = 0) const ;

    int SendVTo(const iovec aIov[],
                DWORD aCount,
                const CSyInetAddr &aAddr) const ;
};


// inline functions
inline SY_HANDLE CSyIPCBase::GetHandle() const
{
    return m_Handle;
}

inline void CSyIPCBase::SetHandle(SY_HANDLE aNew)
{
    SY_ASSERTE(m_Handle == SY_INVALID_HANDLE || aNew == SY_INVALID_HANDLE);
    m_Handle = aNew;
}

inline CSySocketBase::CSySocketBase()
{
}

inline CSySocketBase::~CSySocketBase()
{
    Close();
}

inline int CSySocketBase::SetOption(int aLevel, int aOption, const void *aOptval, int aOptlen) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    int nRet = ::setsockopt((SY_SOCKET)m_Handle, aLevel, aOption,
#ifdef SY_WIN32
                            static_cast<const char *>(aOptval),
#else // !SY_WIN32
                            aOptval,
#endif // SY_WIN32
                            aOptlen);

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32
    return nRet;
}

inline int CSySocketBase::GetOption(int aLevel, int aOption, void *aOptval, int *aOptlen) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    int nRet = ::getsockopt((SY_SOCKET)m_Handle, aLevel, aOption,
#ifdef SY_WIN32
                            static_cast<char *>(aOptval),
                            aOptlen
#else // !SY_WIN32
                            aOptval,
#ifdef SY_MACOS
                            //  #ifdef __i386__
                            reinterpret_cast<socklen_t *>(aOptlen)
                            //  #else
                            //      aOptlen
                            //  #endif
#else
                            reinterpret_cast<socklen_t *>(aOptlen)
#endif
#endif // SY_WIN32
                           );

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32
    return nRet;
}

inline int CSySocketBase::Recv(char *aBuf, DWORD aLen, int aFlag) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    SY_ASSERTE(aBuf);

    int nRet = static_cast<int>(::recv((SY_SOCKET)m_Handle, aBuf, aLen, aFlag));
#ifndef SY_WIN32
#ifdef SY_MACOS
#ifndef MachOSupport
    if (nRet == -1 && errno == 35) {
        CFM_seterrno(35);
    }
#else
    if (nRet == -1 && errno == EAGAIN) {
        errno = EWOULDBLOCK;
    }
#endif    //MachOSupport  
#else
    if (nRet == -1 && errno == EAGAIN) {
        errno = EWOULDBLOCK;
    }
#endif
#else // !SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32

    return nRet;
}

inline int CSySocketBase::RecvV(iovec aIov[], DWORD aCount) const
{
    int nRet;
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    SY_ASSERTE(aIov);

#ifdef SY_WIN32
    DWORD dwBytesReceived = 0;
    DWORD dwFlags = 0;
    nRet = ::WSARecv((SY_SOCKET)m_Handle,
                     (WSABUF *)aIov,
                     aCount,
                     &dwBytesReceived,
                     &dwFlags,
                     0,
                     0);
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    } else {
        nRet = (int)dwBytesReceived;
    }
#else // !SY_WIN32
    nRet = static_cast<int>(::readv(m_Handle, aIov, aCount));
#endif // SY_WIN32
    return nRet;
}

inline int CSySocketBase::Send(const char *aBuf, DWORD aLen, int aFlag) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    SY_ASSERTE(aBuf);

    int nRet = static_cast<int>(::send((SY_SOCKET)m_Handle, aBuf, aLen, aFlag));
#ifndef SY_WIN32
#ifdef SY_MACOS
#ifndef MachOSupport
    if (nRet == -1 && errno == 35) {
        CFM_seterrno(35);
    }
#else
    if (nRet == -1 && errno == EAGAIN) {
        errno = EWOULDBLOCK;
    }
#endif    //MachOSupport
#else
    if (nRet == -1 && errno == EAGAIN) {
        errno = EWOULDBLOCK;
    }
#endif
#else // !SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32
    return nRet;
}

inline int CSySocketBase::SendV(const iovec aIov[], DWORD aCount) const
{
    int nRet;
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    SY_ASSERTE(aIov);

#ifdef SY_WIN32
    DWORD dwBytesSend = 0;
    nRet = ::WSASend((SY_SOCKET)m_Handle,
                     (WSABUF *)aIov,
                     aCount,
                     &dwBytesSend,
                     0,
                     0,
                     0);
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    } else {
        nRet = (int)dwBytesSend;
    }
#else // !SY_WIN32
    nRet = static_cast<int>(::writev(m_Handle, aIov, aCount));
#endif // SY_WIN32
    return nRet;
}

inline CSySocketTcp::CSySocketTcp()
{
}

inline CSySocketTcp::~CSySocketTcp()
{
    Close();
}

inline int CSySocketTcp::Open(BOOL aReuseAddr, int family)
{
    return CSySocketBase::Open(family, SOCK_STREAM, 0, aReuseAddr);
}

inline int CSySocketTcp::Close(SyResult aReason)
{
#ifdef SY_WIN32
    // We need the following call to make things work correctly on
    // Win32, which requires use to do a <CloseWriter> before doing the
    // close in order to avoid losing data.  Note that we don't need to
    // do this on UNIX since it doesn't have this "feature".  Moreover,
    // this will cause subtle problems on UNIX due to the way that
    // fork() works.
    if (m_Handle != SY_INVALID_HANDLE && SY_SUCCEEDED(aReason)) {
        CloseWriter();
    }
#endif // SY_WIN32

    return CSySocketBase::Close();
}

inline int CSySocketTcp::CloseWriter()
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    int nRet = ::shutdown((SY_SOCKET)m_Handle, SY_SD_SEND);

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32
    return nRet;
}

inline int CSySocketTcp::CloseReader()
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    int nRet = ::shutdown((SY_SOCKET)m_Handle, SY_SD_RECEIVE);

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32
    return nRet;
}

inline CSySocketUdp::CSySocketUdp()
{
}

inline CSySocketUdp::~CSySocketUdp()
{
    Close();
}

inline int CSySocketUdp::
RecvFrom(char *aBuf, DWORD aLen, CSyInetAddr &aAddr, int aFlag) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);


    struct sockaddr_storage addr;
    int len;

    len = sizeof addr;
    int nRet = static_cast<int>(::recvfrom((SY_SOCKET)m_Handle,
                                           aBuf,
                                           aLen,
                                           aFlag,
                                           (sockaddr *)&addr,
#ifdef SY_WIN32
                                           &len));
#else
                                           reinterpret_cast<socklen_t *>(&len)));
#endif

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif

    if (nRet != -1) {
        if (addr.ss_family == AF_INET) {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in));
        } else {
            aAddr.SetIpAddrPortBySock(&addr, sizeof(sockaddr_in6));
        }

    }

    return nRet;
}

inline int CSySocketUdp::
SendTo(const char *aBuf, DWORD aLen, const CSyInetAddr &aAddr, int aFlag) const
{
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);

    int nRet = static_cast<int>(::sendto((SY_SOCKET)m_Handle,
                                         aBuf,
                                         aLen,
                                         aFlag,
                                         reinterpret_cast<const sockaddr *>(aAddr.GetPtr()),
#ifdef SY_WIN32
                                         aAddr.GetSize()
#else // !SY_WIN32
#ifdef SY_MACOS
#ifdef __i386__
                                         static_cast<socklen_t>(aAddr.GetSize())
#else
                                         aAddr.GetSize()
#endif
#else
                                         static_cast<socklen_t>(aAddr.GetSize())
#endif
#endif // SY_WIN32
                                        ));

#ifdef SY_WIN32
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    }
#endif // SY_WIN32

    return nRet;
}

inline int CSySocketUdp::
SendVTo(const iovec aIov[], DWORD aCount, const CSyInetAddr &aAddr) const
{
    int nRet;
    //  SY_ASSERTE(m_Handle != SY_INVALID_HANDLE);
    SY_ASSERTE(aIov);

#ifdef SY_WIN32
    DWORD dwBytesSend = 0;
    nRet = ::WSASendTo((SY_SOCKET)m_Handle,
                       (WSABUF *)aIov,
                       aCount,
                       &dwBytesSend,
                       0,
                       reinterpret_cast<const sockaddr *>(aAddr.GetPtr()),
                       aAddr.GetSize(),
                       NULL,
                       NULL);
    if (nRet == SOCKET_ERROR) {
        errno = ::WSAGetLastError();
        nRet = -1;
    } else {
        nRet = (int)dwBytesSend;
    }
#else // !SY_WIN32
    msghdr send_msg;
    send_msg.msg_iov = (iovec *)aIov;
    send_msg.msg_iovlen = aCount;
#ifdef SY_MACOS
    send_msg.msg_name = (char *)aAddr.GetPtr();
#else
    send_msg.msg_name = (struct sockaddr *)aAddr.GetPtr();
#endif
    send_msg.msg_namelen = aAddr.GetSize();
    send_msg.msg_control = 0;
    send_msg.msg_controllen = 0;
    send_msg.msg_flags = 0;
    nRet = static_cast<int>(::sendmsg(m_Handle, &send_msg, 0));
#endif // SY_WIN32
    return nRet;
}

END_UTIL_NS

#endif // !SYSOCKET_H
