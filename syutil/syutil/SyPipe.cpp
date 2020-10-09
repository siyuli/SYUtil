
#include "SyDataType.h"
#include "SyErrorNetwork.h"
#include "SyPipe.h"
#include "SySocket.h"

//#ifdef SY_WIN32
#include "SyDebug.h"
#include "SyUtilClasses.h"
#include "SyUtilMisc.h"
//#endif

USE_UTIL_NS

CSyPipe::CSyPipe()
{
    m_Handles[0] = SY_INVALID_HANDLE;
    m_Handles[1] = SY_INVALID_HANDLE;
}

CSyPipe::~CSyPipe()
{
    Close();
}

SyResult CSyPipe::Open(DWORD aSize)
{
    SY_ASSERTE(m_Handles[0] == SY_INVALID_HANDLE && m_Handles[1] == SY_INVALID_HANDLE);

    int nRet = 0;
#if defined (SY_UNIX) || defined (SY_MACOS)
    nRet = ::socketpair(AF_UNIX, SOCK_STREAM, 0, m_Handles);
    if (nRet == -1) {
        SY_ERROR_TRACE_THIS("CSyPipe::Open, socketpair() failed! err=" << errno);
        return nRet;
    }
#else //SY_WIN32
    CSySocketTcp socketTcp;
    //CSyInetAddr addrListen("127.0.0.1:0");
    CSyInetAddr addrConnect;
    CSyInetAddr addrPeer;
    int nAddrLen = addrPeer.GetSize();

    if (ipv4_enabled()) {

        CSyInetAddr addrListen("127.0.0.1:0");

        if (socketTcp.Open() == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, open() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        nRet = ::bind(
                   (SY_SOCKET)socketTcp.GetHandle(),
                   reinterpret_cast<const struct sockaddr *>(addrListen.GetPtr()),
                   addrListen.GetSize());
        if (nRet == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, bind() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        if (::listen((SY_SOCKET)socketTcp.GetHandle(), 5) == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, listen() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        nRet = socketTcp.GetLocalAddr(addrConnect);
        WORD wPort = addrConnect.GetPort();
        SY_INFO_TRACE_THIS("CSyPipe::Open, GetLocalAddr, port = " << wPort);
        addrConnect.Set("127.0.0.1:0");
        addrConnect.SetPort(wPort);
        
        SY_ASSERTE(nRet == 0);

        m_Handles[1] = (SY_HANDLE)::socket(AF_INET, SOCK_STREAM, 0);
        SY_ASSERTE(m_Handles[1] != SY_INVALID_HANDLE);

        nRet = ::connect(
                   (SY_SOCKET)m_Handles[1],
                   reinterpret_cast<const struct sockaddr *>(addrConnect.GetPtr()),
                   addrConnect.GetSize());
        if (nRet == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, connect() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        m_Handles[0] = (SY_HANDLE)::accept(
                           (SY_SOCKET)socketTcp.GetHandle(),
                           (struct sockaddr *)(addrPeer.GetPtr()),
                           &nAddrLen);
        if (m_Handles[0] == SY_INVALID_HANDLE) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, accept() failed! err=" << ::WSAGetLastError());
            goto fail;
        }
    } else if (ipv6_enabled() && ipv6_dns_resolve()) {

        CSyInetAddr addrListen;

        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;

        int nError;
        nError = tp_getaddrinfo("::1", "0", &hints, &res);
        if (nError) {
            nError = ::WSAGetLastError();
            SY_ERROR_TRACE_THIS("getaddrinfo failed, err: "<< nError);
            goto fail;
        }
        struct sockaddr_storage ss;
        memcpy(&ss, res->ai_addr, res->ai_addrlen);
        addrListen.SetIpAddrBySock(&ss, res->ai_addrlen);

        tp_freeaddrinfo(res);

        if (socketTcp.Open(FALSE, AF_INET6) == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, open() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        nRet = ::bind(
                   (SY_SOCKET)socketTcp.GetHandle(),
                   reinterpret_cast<const struct sockaddr *>(addrListen.GetPtr()),
                   addrListen.GetSize());
        if (nRet == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, bind() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        if (::listen((SY_SOCKET)socketTcp.GetHandle(), 5) == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, listen() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        nRet = socketTcp.GetLocalAddr(addrConnect);
        SY_ASSERTE(nRet == 0);

        m_Handles[1] = (SY_HANDLE)::socket(AF_INET6, SOCK_STREAM, 0);
        SY_ASSERTE(m_Handles[1] != SY_INVALID_HANDLE);

        nRet = ::connect(
                   (SY_SOCKET)m_Handles[1],
                   reinterpret_cast<const struct sockaddr *>(addrConnect.GetPtr()),
                   addrConnect.GetSize());
        if (nRet == -1) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, connect() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

        addrPeer.SetType(AF_INET6);
        nAddrLen = addrPeer.GetSize();

        m_Handles[0] = (SY_HANDLE)::accept(
                           (SY_SOCKET)socketTcp.GetHandle(),
                           (struct sockaddr *)(addrPeer.GetPtr()),
                           &nAddrLen);
        if (m_Handles[0] == SY_INVALID_HANDLE) {
            SY_ERROR_TRACE_THIS("CSyPipe::Open, accept() failed! err=" << ::WSAGetLastError());
            goto fail;
        }

    } else {
        SY_ERROR_TRACE_THIS("neither ipv4 nor ipv6 are supported!");
        goto fail;
    }

#endif // defined (SY_UNIX) || defined (SY_MACOS)

    if (aSize > SY_DEFAULT_MAX_SOCKET_BUFSIZ) {
        aSize = SY_DEFAULT_MAX_SOCKET_BUFSIZ;
    }
    nRet = ::setsockopt((SY_SOCKET)m_Handles[0], SOL_SOCKET, SO_RCVBUF,
#ifdef SY_WIN32
                        reinterpret_cast<const char *>(&aSize),
#else // !SY_WIN32
                        &aSize,
#endif // SY_WIN32
                        sizeof(aSize));
    if (nRet == -1) {
        SY_ERROR_TRACE_THIS("CSyPipe::Open, setsockopt(0) failde! err=" << errno);
        goto fail;
    }
    nRet = ::setsockopt((SY_SOCKET)m_Handles[1], SOL_SOCKET, SO_SNDBUF,
#ifdef SY_WIN32
                        reinterpret_cast<const char *>(&aSize),
#else // !SY_WIN32
                        &aSize,
#endif // SY_WIN32
                        sizeof(aSize));
    if (nRet == -1) {
        SY_ERROR_TRACE_THIS("CSyPipe::Open, setsockopt(1) failde! err=" << errno);
        goto fail;
    }
    return SY_OK;

fail:
    Close();
    return SY_ERROR_NOT_AVAILABLE;
}

SyResult CSyPipe::Close()
{
    int nRet = 0;
    if (m_Handles[0] != SY_INVALID_HANDLE) {
#if defined(SY_UNIX) || defined(SY_APPLE)
        nRet = ::close(m_Handles[0]);
#else
        nRet = ::closesocket((SY_SOCKET)m_Handles[0]);
#endif // SY_UNIX
        m_Handles[0] = SY_INVALID_HANDLE;
    }
    if (m_Handles[1] != SY_INVALID_HANDLE) {
#if defined(SY_UNIX) || defined(SY_APPLE)
        nRet |= ::close(m_Handles[1]);
#else
        nRet |= ::closesocket((SY_SOCKET)m_Handles[1]);
#endif // SY_UNIX
        m_Handles[1] = SY_INVALID_HANDLE;
    }
    return nRet == 0 ? SY_OK : SY_ERROR_NETWORK_SOCKET_ERROR;
}

SY_HANDLE CSyPipe::GetReadHandle() const
{
    return m_Handles[0];
}

SY_HANDLE CSyPipe::GetWriteHandle() const
{
    return m_Handles[1];
}
