#include "SyUtilMisc.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#ifdef SY_WIN32
    #include <ws2tcpip.h>

    #if (_MSC_VER <= 1500)
        typedef unsigned __int16  uint16_t;
    #endif

#endif

#ifdef SY_WIN32
    #include "SyDebug.h"
    #include <windows.h>
    #if defined (WP8 ) 
        #include "wp8helper.h"
    #elif defined (UWP)
        #include "uwphelper.h"
    #else

        #include <wlanapi.h>

        typedef DWORD (WINAPI *WlanOpenHandleFn)(DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle);
        typedef DWORD (WINAPI *WlanCloseHandleFn)(HANDLE hClientHandle, PVOID pReserved);
        typedef DWORD (WINAPI *WlanEnumInterfacesFn)(HANDLE hClientHandle, PVOID pReserved, PWLAN_INTERFACE_INFO_LIST *ppInterfaceList);
        typedef DWORD (WINAPI *WlanGetAvailableNetworkListFn)(HANDLE hClientHandle, CONST GUID *pInterfaceGuid, DWORD dwFlags, PVOID pReserved, PWLAN_AVAILABLE_NETWORK_LIST *ppAvailableNetworkList);
        typedef DWORD (WINAPI *WlanGetNetworkBssListFn)(HANDLE hClientHandle, CONST GUID *pInterfaceGuid, CONST PDOT11_SSID pDot11Ssid, DOT11_BSS_TYPE dot11BssType, BOOL bSecurityEnabled, PVOID pReserved, PWLAN_BSS_LIST *ppWlanBssList);
        typedef DWORD (WINAPI *WlanQueryInterfaceFn)(HANDLE hClientHandle, CONST GUID *pInterfaceGuid, WLAN_INTF_OPCODE OpCode, PVOID pReserved, PDWORD pdwDataSize, PVOID *ppData, PWLAN_OPCODE_VALUE_TYPE pWlanOpcodeValueType);
        typedef VOID (WINAPI *WlanFreeMemoryFn)(PVOID pMemory);

        //#pragma comment(lib, "wlanapi.lib")
        #include <iphlpapi.h>
        #pragma comment(lib, "IPHLPAPI.lib")
    #endif // ~WP8
#endif

#ifdef SY_ANDROID
    #include <net/if.h>
    #include "SyIfAddrsAndroid.h"
#endif

#ifdef SY_LINUX
    #include <net/if.h>
    #include <dlfcn.h>
    #ifndef IFNAMSIZ
        #define IFNAMSIZ 256
    #endif
    #include "SyIfAddrsAndroid.h"
#endif

#ifdef SY_MACOS
    #include <net/if.h>
    #include <dlfcn.h>
    #ifndef IFNAMSIZ
        #define IFNAMSIZ 256
    #endif
#endif

#ifdef SY_MACOS
    #include <netinet/in.h>
    #ifdef SY_IOS
        #include "in6_var_ios.h"
    #else//mac
        #include <netinet/in_var.h>
    #endif//~!SY_IOS
#endif//~SY_MACOS
#if defined (SY_IOS) || defined (SY_MACOS)
    #include <CoreFoundation/CoreFoundation.h>
    #include <SystemConfiguration/SystemConfiguration.h>
    #include <SystemConfiguration/CaptiveNetwork.h>
    #include <CoreFoundation/CFArray.h>
    #include <CoreFoundation/CFString.h>
#endif

#include "SyUtilTemplates.h"
#include "SyThread.h"
#include "SyThreadManager.h"
#include "SyEventQueueBase.h"
#include "timer.h"

#if defined(SY_MACOS) || defined(SY_IOS)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <ifaddrs.h>
#endif

#define pmin(a,b)        ((a) < (b) ? (a) : (b))
#define pmax(a,b)        ((a) > (b) ? (a) : (b))

#define	SY_IN6_IS_ADDR_LOOPBACK(a)		\
    ((*(uint32_t*)&((a)->s6_addr[0]) == 0) && \
    (*(uint32_t*)&((a)->s6_addr[4]) == 0) && \
    (*(uint32_t*)&((a)->s6_addr[8]) == 0) && \
    (*(uint32_t*)&((a)->s6_addr[12]) == htonl(1)))

#define	SY_IN6_IS_ADDR_LINKLOCAL(a)	\
    (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))

#define SY_IN_LINKLOCAL(i)		((ntohl((uint32_t)(i)) & 0xffff0000) == (uint32_t)0xA9FE0000) /* 169.254.0.0 */
#define SY_IN_LOOPBACK(i)		((ntohl((uint32_t)(i)) & 0xff000000) == 0x7f000000)

START_UTIL_NS

#if defined (WP8 ) || defined (UWP)
    using namespace Platform;
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::System::Threading;
    #include "ThreadEmulation.h"
    using namespace ThreadEmulation;
#endif

extern "C"
void SleepMs(DWORD aMsec)
{
#ifdef SY_WIN32
#if defined (WP8 ) || defined (UWP)
    ThreadEmulation::Sleep(aMsec);
#else
    ::Sleep(aMsec);
#endif
#else
    struct timespec ts, rmts;
    ts.tv_sec = aMsec/1000;
    ts.tv_nsec = (aMsec%1000)*1000000;

    for (; ;) {
        int nRet = ::nanosleep(&ts, &rmts);
        if (nRet == 0) {
            break;
        }
        if (errno == EINTR) {
            ts = rmts;
        } else {
            SY_WARNING_TRACE("::SleepMs,"
                             "nanosleep() failed! err=" << errno);
            break;
        }
    }
#endif // SY_WIN32
}

class CSySleepMsWithLoop
{
    public:
        static void DoSleep(unsigned long aMsec)
        {
            unsigned long nowTime = get_tick_count();
            unsigned long endTime = nowTime + aMsec;
            while (endTime > nowTime)
            {
                ASyThread* curThread = getCurrentThread();
                if( nullptr == curThread){
                    SleepMs(aMsec);
                    return;
                }

                CSyEventQueueBase *pQueue = dynamic_cast<CSyEventQueueBase*>(curThread->GetEventQueue());
                if( nullptr == pQueue){
                    SleepMs(aMsec);
                    return;
                }

                CSyEventQueueBase::EventsType listEvents;
                SyResult rv = pQueue->PopPendingEvents(listEvents);
                if (SY_SUCCEEDED(rv))
                {
                    pQueue->ProcessEvents(listEvents);
                }
                nowTime = get_tick_count();
                if(endTime > nowTime)
                    SleepMs(1);
                nowTime = get_tick_count();
            }
        }
};

extern "C"
void SleepMsWithLoop(DWORD aMsec)
{
#if defined(SY_IOS) || defined(SY_MACOS)
    CFTimeInterval sec = 0.001*(CFTimeInterval)aMsec;
    bool done = FALSE;
    do {
        int result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, sec, TRUE);
        if ((result == kCFRunLoopRunStopped) || (result == kCFRunLoopRunFinished) || (result == kCFRunLoopRunTimedOut)) {
            done = TRUE;
        }
    } while (!done);
#elif defined(SY_WIN32)
#if defined (WP8 ) || defined (UWP)
    // wp not imp
#else
    unsigned long endTime = GetTickCount() + aMsec;
    while (1) {
        unsigned long nowTime = GetTickCount();
        if (nowTime >= endTime) {
            break;
        }

        unsigned long waitTime = endTime - nowTime;
        DWORD ret = MsgWaitForMultipleObjects(0, NULL, FALSE, waitTime, QS_ALLEVENTS);
        if (ret == WAIT_TIMEOUT) {
            break;
        }
        if (ret == WAIT_OBJECT_0) {
            MSG msg = { 0 };
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
#endif
#elif defined(SY_LINUX)
    CSySleepMsWithLoop::DoSleep(aMsec);
#else
    SleepMs(aMsec);
    // android not imp
#endif
}

//CRC calculation

#define CRC16_TAB_SIZE                  256
static BYTE CRC16_H_TABLE[CRC16_TAB_SIZE], CRC16_L_TABLE[CRC16_TAB_SIZE];
void GenerateCRCTable()
{
    WORD wtabV = 0;
    int i, j, k;

    for (i=0; i<CRC16_TAB_SIZE; i++) {
        wtabV = 0;
        k = (i << 1);
        for (j = 8; j>0; j--) {
            k >>= 1;
            if ((k ^ wtabV) & 0x0001) {
                wtabV = (wtabV >> 1) ^ 0xa001;
            } else {
                wtabV >>= 1;
            }
        }

        CRC16_H_TABLE[i] = wtabV & 0x00FF;
        CRC16_L_TABLE[i] = wtabV >> 8;
    }
}

class CRCTAbleInit
{
public:
    CRCTAbleInit()
    {
        GenerateCRCTable();
    }
};
typedef CSySingletonT<CRCTAbleInit> CRCTAbleInitialize;


WORD CalcCRC16(WORD &wResult, BYTE *pData,DWORD dwLen)
{
    SY_ASSERTE_RETURN(pData, 0);
    CRCTAbleInitialize::Instance();
    BYTE byCRC16Lo = wResult & 0x00FF;
    BYTE byCRC16Hi = (wResult >> 8);
    BYTE byIndex = 0;

    while (dwLen--) {
        byIndex = byCRC16Lo ^ *pData++ ;
        byCRC16Lo = byCRC16Hi ^ CRC16_H_TABLE[byIndex];
        byCRC16Hi = CRC16_L_TABLE[byIndex] ;
    }
    wResult = ((byCRC16Hi << 8) | byCRC16Lo) ;
    return wResult;
}

#if defined(SY_IOS) || defined(SY_WIN32) || defined(SY_ANDROID) || defined(SY_MACOS) || defined(SY_LINUX_SERVER)
//get primary ip, just for IPv4
int getPrimaryIp(struct sockaddr_in *sin)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        SY_ERROR_TRACE("getPrimaryIp, socket failed");
        return -1;
    }

    const char *kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, kGoogleDnsIp, &(serv.sin_addr));
    //serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const sockaddr *) &serv, sizeof(serv));
    //SY_ASSERTE(err != -1);
    if (err == -1) {
        SY_ERROR_TRACE("getPrimaryIp, connect failed");
#ifdef SY_WIN32
        ::closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (sockaddr *) &name, &namelen);
    if (err == -1) {
        SY_ERROR_TRACE("getPrimaryIp, getsockname failed");
#ifdef SY_WIN32
        ::closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

#ifndef SY_WIN32
    char buffer[100];
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer));
    if (!p) {
        SY_ERROR_TRACE("getPrimaryIp, inet_ntop failed");
        close(sock);
        return -1;
    } else {
        SY_INFO_TRACE("getPrimaryIp is: " << p);
    }
#endif

#ifdef SY_WIN32
    ::closesocket(sock);
#else
    close(sock);
#endif

#ifndef SY_WIN32
    if (strncmp(buffer, "169.", 4) == 0) {
        return -1;
    }
#endif

    if (sin) {
        memcpy(sin, &name, namelen);
    }
    return 0;

}

void prune_local_addr(local_addr_t *&res)
{
#if defined(SY_MACOS) || defined(SY_ANDROID) || defined(SY_WIN_DESKTOP)
    if (res == nullptr) {
        return;
    }
    
    std::list<local_addr_t*> refinedList, deleteList;
    
    local_addr_t *it = res;
    time_t t = time(nullptr);
    int count = 0;
    for (; it != nullptr; it = it->next, count++) {
        if (it->l_addr.ss_family == AF_INET6) {
            //Need to be removed
            if ((it->flag6 == SyV6Addr_Invalid) || (t >= it->expire)) {
                deleteList.push_back(it);
                continue;
            }

            bool bPruned = false;
            for (auto itItem = refinedList.begin(); itItem != refinedList.end(); itItem++) {
                local_addr_t* pItem = *itItem;
                if (strcmp(pItem->ifname, it->ifname) == 0 && pItem->l_addr.ss_family == AF_INET6) {
                    if ((it->flag6 == SyV6Addr_Temp && pItem->flag6 == SyV6Addr_AutoConf) ||
                        (it->flag6 == pItem->flag6 && it->expire < pItem->expire))
                    {
                        local_addr_t *old = *itItem;
                        *itItem = it;
                        deleteList.push_back(old);
                        bPruned = true;
                        break;
                    } else if ((it->flag6 ==SyV6Addr_AutoConf && pItem->flag6 == SyV6Addr_Temp) ||
                               (it->flag6 == pItem->flag6 && it->expire >= pItem->expire)) {
                        deleteList.push_back(it);
                        bPruned = true;
                        break;
                    }
                }
            }
            if (!bPruned) {
                refinedList.push_back(it);
            }
        } else {
            refinedList.push_back(it);
        }
    }
    //Make sure there is no memory leak
    SY_ASSERTE(count == (refinedList.size() + deleteList.size()));
    it = res = nullptr;
    for (local_addr_t *pItem : refinedList) {
        pItem->next = nullptr;
        if (it == nullptr) {
            res = it = pItem;
        } else {
            it->next = pItem;
            it = pItem;
        }
    }
    
    for (local_addr_t *pItem : deleteList) {
        char addressBuffer[64] = {0};
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)&pItem->l_addr)->sin6_addr), addressBuffer, 64);

        SY_INFO_TRACE("prune_local_addr, IP Address= " << pItem->ifname << "," << addressBuffer << ", expire=" << pItem->expire << ", flag=" << pItem->flag6);
        delete pItem;
    }
#endif
}

#if defined(SY_IOS) || defined(SY_MACOS)
#define BUFFERSIZE      4000
#define MAXADDRS 20

bool sy_fill_v6_addr_detail(const char* ifname, local_addr *addr)
{
    int fd = -1;
    int flags6 = 0;
    struct in6_ifreq detail = {0};
    struct sockaddr_in6 *sin = (struct sockaddr_in6*)&addr->l_addr;
    
    strcpy_forsafe(detail.ifr_name, ifname, strlen(ifname), sizeof(detail.ifr_name));
    strcpy_forsafe(addr->ifname, ifname, strlen(ifname), sizeof(addr->ifname));
    
    if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("ifconfig: socket");
        return false;
    }

    detail.ifr_addr = *sin;
    if (ioctl(fd, SIOCGIFAFLAG_IN6, &detail) < 0) {
        SY_INFO_TRACE("get_v6_lifetime: ioctl(SIOCGIFAFLAG_IN6), errno=" << errno);
        close(fd);
        return false;
    }
    flags6 = detail.ifr_ifru.ifru_flags6;
    addr->flag6 = SyV6Addr_Invalid;
    if ((flags6 & IN6_IFF_AUTOCONF) != 0)
        addr->flag6 = SyV6Addr_AutoConf;
    
    if ((flags6 & IN6_IFF_TEMPORARY) != 0)
        addr->flag6 = SyV6Addr_Temp;

    if ((flags6 & IN6_IFF_DYNAMIC) != 0)
        addr->flag6 = SyV6Addr_Dynamic;
    
    detail.ifr_addr = *sin;
    if (ioctl(fd, SIOCGIFALIFETIME_IN6, &detail) < 0) {
        SY_INFO_TRACE("get_v6_lifetime, errno=" << errno);
        close(fd);
        return false;
    }
    //We don't want to use a deprecated address
    addr->expire = detail.ifr_ifru.ifru_lifetime.ia6t_preferred;
    close(fd);
    return true;
}

int get_local_addr(local_addr_t **res)
{
    struct sockaddr_in      *sin, *primSin;

    if (*res) {
        *res = NULL;
    }

    //add the primary address to the first place
    local_addr_t *_paddr = new local_addr_t;
    memset(_paddr, 0, sizeof(local_addr_t));
    _paddr->l_addr_len = sizeof(struct sockaddr_in);
    int err = getPrimaryIp((struct sockaddr_in *)&_paddr->l_addr);
    _paddr->next = NULL;

    primSin = NULL;
    if (err == 0) {
        primSin = (struct sockaddr_in *)&_paddr->l_addr;
    }

    struct ifaddrs *ifAddrStruct=NULL;
    struct ifaddrs *ifa=NULL;
    void *tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if ((ifa->ifa_flags & IFF_UP) == 0) {
            continue;    // ignore if interface not up
        }

        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, addressBuffer, INET_ADDRSTRLEN);
            SY_DEBUG_TRACE("get_local_addr, IP Address = " << ifa->ifa_name << ", " << addressBuffer << ", sa_family=" << AF_INET);
            if (strncmp(addressBuffer, "169.254.", 8) != 0 && strncmp(addressBuffer, "127.0.0.1", 9) != 0) {
                socklen_t namelen = sizeof(struct sockaddr_in);
                SY_INFO_TRACE("get_local_addr, IP Address = " << ifa->ifa_name << ", " << addressBuffer);

                sin = (struct sockaddr_in *)ifa->ifa_addr;
                if (!primSin || sin->sin_addr.s_addr != primSin->sin_addr.s_addr) {

                    local_addr_t *_addr = new local_addr_t;
                    memset(_addr, 0, sizeof(local_addr_t));
                    
                    _addr->l_addr_len = sizeof(struct sockaddr_in);
                    memcpy(&_addr->l_addr, tmpAddrPtr, namelen);
                    _addr->next = NULL;

                    if (!(*res)) {
                        *res = _addr;
                    } else {
                        _addr->next = *res;
                        *res = _addr;
                    }
                }
            }

        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=(struct sockaddr_in6 *)ifa->ifa_addr;
            struct sockaddr_in6 * pSockAddr = (struct sockaddr_in6 *)ifa->ifa_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &pSockAddr->sin6_addr, addressBuffer, INET6_ADDRSTRLEN);
            SY_DEBUG_TRACE("get_local_addr, IP Address = " << ifa->ifa_name << ", " << addressBuffer << ", sa_family=" << AF_INET6);
            //IN6_IS_ADDR_LINKLOCAL()
            if(SY_IN6_IS_ADDR_LOOPBACK(&pSockAddr->sin6_addr) || SY_IN6_IS_ADDR_LINKLOCAL(&pSockAddr->sin6_addr)) {
                continue;
            }
            socklen_t namelen = sizeof(struct sockaddr_in6);
            
            local_addr_t *_addr = new local_addr_t;
            memset(_addr, 0, sizeof(local_addr_t));
            
            _addr->l_addr_len = sizeof(struct sockaddr_in6);
            memcpy(&_addr->l_addr, tmpAddrPtr, namelen);
            _addr->next = NULL;
            
            sy_fill_v6_addr_detail(ifa->ifa_name, _addr);
            SY_INFO_TRACE("get_local_addr, IP Address = " << ifa->ifa_name << ", " << addressBuffer << ", expire="
                          << (_addr->expire - time(nullptr)) << "s, flag6=" << _addr->flag6);
            
            if (!(*res)) {
                *res = _addr;
            } else {
                _addr->next = *res;
                *res = _addr;
            }
        }
    }

    if (err == 0) {
        if (!(*res)) {
            *res = _paddr;
        } else {
            _paddr->next = *res;
            *res = _paddr;
        }
    }

    if (ifAddrStruct!=NULL) { freeifaddrs(ifAddrStruct); }

    if (*res == NULL) {
        return -1;
    }
    return 0;
}

std::string get_primary_bsdName()
{
    int                 len, flags;
    char                buffer[BUFFERSIZE], *ptr, lastname[IFNAMSIZ], *cptr;
    struct ifconf       ifc;
    struct ifreq        *ifr, ifrcopy;
    struct sockaddr_in      *sin, *primSin;

    int sockfd;

    //add the primary address to the first place
    local_addr_t addr;
    addr.l_addr_len = sizeof(struct sockaddr_in);
    int err = getPrimaryIp((struct sockaddr_in *)&addr.l_addr);
    addr.next = NULL;

    primSin = NULL;
    if (err == 0) {
        //*res = _addr;
        primSin = (struct sockaddr_in *)&addr.l_addr;
    }

    if (!primSin) {
        return "";
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return "";
    }

    ifc.ifc_len = BUFFERSIZE;
    ifc.ifc_buf = buffer;

    if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
        return "";
    }

    lastname[0] = 0;

    for (ptr = buffer; ptr < buffer + ifc.ifc_len;) {

        ifr = (struct ifreq *)ptr;
        len = pmax(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
        ptr += sizeof(ifr->ifr_name) + len;     // for next one in buffer

        if (ifr->ifr_addr.sa_family != AF_INET) {
            continue;    // ignore if not desired address family
        }

        if ((cptr = (char *)strchr(ifr->ifr_name, ':')) != NULL) {
            *cptr = 0;    // replace colon will null
        }

        if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0) {
            continue;    /* already processed this interface */
        }
        memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

        ifrcopy = *ifr;
        ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        if ((flags & IFF_UP) == 0) {
            continue;    // ignore if interface not up
        }

        sin = (struct sockaddr_in *)&ifr->ifr_addr;

        if (sin->sin_addr.s_addr == primSin->sin_addr.s_addr) {
            close(sockfd);

            return lastname;
        }
        //SY_INFO_TRACE("addr is: " << inet_ntoa(sin->sin_addr));
    }

    close(sockfd);
    return "";
}
#elif defined(SY_WIN_DESKTOP)
GetAdaptersAddressesFuc g_func_GetAdaptersAddresses = GetAdaptersAddresses;

void SetGetAdaptersAddressesFuc(GetAdaptersAddressesFuc fn)
{
    SY_INFO_TRACE("SetGetAdaptersAddressesFuc, fn=" << fn << ", old=" << g_func_GetAdaptersAddresses);
    g_func_GetAdaptersAddresses = fn;
}

GetAdaptersAddressesFuc GetGetAdaptersAddressesFuc()
{
    return g_func_GetAdaptersAddresses;
}

int get_local_addr(local_addr_t **res)
{
    SY_ASSERTE_RETURN(res != nullptr, SY_ERROR_INVALID_ARG);

    struct sockaddr_in primSin = { 0 };
    int errPrim = getPrimaryIp(&primSin);
    local_addr_t *tail = nullptr;
    *res = nullptr;

    IP_ADAPTER_ADDRESSES *pAddresses = nullptr;
    ULONG outBufLen = 32768;
    ULONG dwRetVal = 0;
    char szAddrName[256];

    for (int i = 0; i < 5; i++) {
        pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
        if (pAddresses == nullptr) {
            SY_ERROR_TRACE("get_local_addr, Memory allocation failed for IP_ADAPTER_ADDRESSES struct");
            return -1;
        }

        dwRetVal = g_func_GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            SY_WARNING_TRACE("get_local_addr, buffer overflow, needed buffer=" << outBufLen);
            free(pAddresses);
            pAddresses = nullptr;
        } else {
            break;
        }
    }

    if (dwRetVal == NO_DATA || dwRetVal != NO_ERROR) {
        SY_ERROR_TRACE("get_local_addr, ERROR on GetAdaptersAddresses, code=" << dwRetVal << ", errPrim=" << errPrim);
        *res = nullptr;
        if (errPrim == 0) {
            local_addr_t *_addr = new local_addr_t;
            memset(_addr, 0, sizeof(local_addr_t));

            _addr->l_addr_len = sizeof(struct sockaddr_in);
            memcpy(&_addr->l_addr, &primSin, sizeof(primSin));
            _addr->next = NULL;

            *res = _addr;
        }
        return -1;
    }

    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    for (; pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next) {
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
        for (; pUnicast != nullptr; pUnicast = pUnicast->Next) {
            if (IpDadStatePreferred != pUnicast->DadState) {
                continue;
            }

            szAddrName[0] = 0;
            LPSOCKADDR pSockAddr = pUnicast->Address.lpSockaddr;
            if (pSockAddr->sa_family == AF_INET) {
                IN_ADDR *pSinAddr = &(((struct sockaddr_in *)pSockAddr)->sin_addr);
                if (SY_IN_LINKLOCAL(pSinAddr->s_addr) || SY_IN_LOOPBACK(pSinAddr->s_addr)) {
                    continue;
                }

                local_addr_t *_addr = new local_addr_t;
                memset(_addr, 0, sizeof(local_addr_t));

                _addr->l_addr_len = sizeof(struct sockaddr_in);
                memcpy(&_addr->l_addr, pSockAddr, sizeof(struct sockaddr_in));
                _addr->next = NULL;
                strcpy_forsafe(_addr->ifname, pCurrAddresses->AdapterName, strlen(pCurrAddresses->AdapterName), sizeof(_addr->ifname));

                inet_ntop(AF_INET, &(((struct sockaddr_in *)(&_addr->l_addr))->sin_addr), szAddrName, sizeof(szAddrName));
                bool bPrim = (errPrim == 0) && (pSinAddr->s_addr == primSin.sin_addr.s_addr);
                if (!tail) {
                    *res = tail = _addr;
                }
                else if (bPrim){
                    _addr->next = *res;
                    *res = _addr;
                }
                else {
                    tail->next = _addr;
                    tail = _addr;
                }

            }
            else if (pSockAddr->sa_family == AF_INET6) {
                IN6_ADDR *pSin6Addr = &(((struct sockaddr_in6 *)pSockAddr)->sin6_addr);
                if (SY_IN6_IS_ADDR_LOOPBACK(pSin6Addr) || SY_IN6_IS_ADDR_LINKLOCAL(pSin6Addr)) {
                    continue;
                }

                local_addr_t *_addr = new local_addr_t;
                memset(_addr, 0, sizeof(local_addr_t));
                _addr->l_addr_len = sizeof(struct sockaddr_in6);
                memcpy(&_addr->l_addr, pSockAddr, sizeof(struct sockaddr_in6));
                strcpy_forsafe(_addr->ifname, pCurrAddresses->AdapterName, strlen(pCurrAddresses->AdapterName), sizeof(_addr->ifname));
                _addr->expire = time(nullptr) + pUnicast->PreferredLifetime;

                _addr->next = NULL;
                inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(&_addr->l_addr))->sin6_addr), szAddrName, sizeof(szAddrName));

                if (!tail) {
                    *res = tail = _addr;
                }
                else {
                    tail->next = _addr;
                    tail = _addr;
                }

            }

            SY_INFO_TRACE("get_local_addr, addr=" << szAddrName << ", ifname=" << pCurrAddresses->AdapterName 
                << ", lifetime=" << pUnicast->PreferredLifetime << ", state=" << pUnicast->DadState << ", now=" << time(nullptr));
        }
    }
    if (pAddresses != nullptr) {
        free(pAddresses);
    }

    if (*res == nullptr) {
        return -1;
    }

    return 0;
}

#elif defined(SY_WIN32)
int get_local_addr(local_addr_t **addr)
{
    int ret;
    char buffer[256];
    struct sockaddr_in *sin, *primSin;
    struct sockaddr_in6 *sin6;

    if (gethostname(buffer, sizeof(buffer)) != 0) {
        return -1;
    }

    //add the primary address to the first place
    local_addr_t *_paddr = new local_addr_t;
    memset(_paddr, 0, sizeof(local_addr_t));
    
    _paddr->l_addr_len = sizeof(struct sockaddr_in);
    int err = getPrimaryIp((struct sockaddr_in *)&_paddr->l_addr);
    _paddr->next = NULL;

    primSin = NULL;
    if (err == 0) {
        primSin = (struct sockaddr_in *)&_paddr->l_addr;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;

    ret = getaddrinfo(buffer, NULL, &hints, &res);

    if (ret) {
        return -1;
    }

    if (*addr) {
        *addr = NULL;
    }

    struct addrinfo *tmp = res;
    while (tmp) {
        if (tmp->ai_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            sin = (struct sockaddr_in *)tmp->ai_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)tmp->ai_addr)->sin_addr, addressBuffer, INET_ADDRSTRLEN);
            SY_DEBUG_TRACE("get_local_addr, IP Address = " << tmp->ai_canonname << ", " << addressBuffer << ", sa_family=" << AF_INET);
            if (strncmp(addressBuffer, "169.254.", 8) != 0 && strncmp(addressBuffer, "127.0.0.1", 9) != 0) {
                socklen_t namelen = sizeof(struct sockaddr_in);
                SY_INFO_TRACE("get_local_addr, IP Address = " << tmp->ai_canonname << ", " << addressBuffer);

                if (!primSin || sin->sin_addr.s_addr != primSin->sin_addr.s_addr) {

                    local_addr_t *_addr = new local_addr_t;
                    memset(_addr, 0, sizeof(local_addr_t));
                    
                    _addr->l_addr_len = sizeof(struct sockaddr_in);
                    memcpy(&_addr->l_addr, sin, namelen);
                    _addr->next = NULL;

                    if (!(*addr)) {
                        *addr = _addr;
                    } else {
                        _addr->next = *addr;
                        *addr = _addr;
                    }
                }
            }

        } else if (tmp->ai_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            sin6 = (struct sockaddr_in6 *)tmp->ai_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)tmp->ai_addr)->sin6_addr, addressBuffer, INET6_ADDRSTRLEN);
            SY_DEBUG_TRACE("get_local_addr, IP Address = " << tmp->ai_canonname << ", " << addressBuffer << ", sa_family=" << AF_INET6);
            if (strncmp(addressBuffer, "fe80:", 5) != 0 && strncmp(addressBuffer, "fe80::1", 7) != 0 && strncmp(addressBuffer, "::1", 3) != 0) {
                socklen_t namelen = sizeof(struct sockaddr_in6);
                SY_INFO_TRACE("get_local_addr, IP Address = " << tmp->ai_canonname << ", " << addressBuffer);

                local_addr_t *_addr = new local_addr_t;
                memset(_addr, 0, sizeof(local_addr_t));
                
                _addr->l_addr_len = sizeof(struct sockaddr_in6);
                memcpy(&_addr->l_addr, sin6, namelen);
                _addr->next = NULL;

                if (!(*addr)) {
                    *addr = _addr;
                } else {
                    _addr->next = *addr;
                    *addr = _addr;
                }
            }
        }
        tmp = tmp->ai_next;
    }

    if (err == 0) {
        if (!(*addr)) {
            *addr = _paddr;
        } else {
            _paddr->next = *addr;
            *addr = _paddr;
        }
    }

    freeaddrinfo(res);
    if (*addr == NULL) {
        return -1;
    }

    return 0;
}
#elif defined(SY_ANDROID) || defined(SY_LINUX)

#define BUFFERSIZE      8192
#define MAXADDRS 20
int get_local_addr(local_addr_t **res)
{
    SY_ASSERTE_RETURN(res != nullptr, -4);
    
    //add the primary address to the first place
    struct sockaddr_in primSin = {0};
    int errPrim = getPrimaryIp(&primSin);
    local_addr_t *tail = nullptr;
    *res = nullptr;
    
    std::vector<SyIfAddr> addrs;
    SyGetIfAddrs(addrs);
    
    char szAddrName[256];
    for (SyIfAddr addrIt : addrs) {
        if (!(addrIt.ifa_flags & IFF_UP)) {
            continue;
        }
        
        szAddrName[0] = 0;
        if (addrIt.ifa_address.ss_family == AF_INET) {
            struct sockaddr_in * pSockAddr = (sockaddr_in *)&addrIt.ifa_address;
            if (SY_IN_LINKLOCAL(pSockAddr->sin_addr.s_addr) || SY_IN_LOOPBACK(pSockAddr->sin_addr.s_addr)) {
                continue;
            }
            
            local_addr_t *_addr = new local_addr_t;
            memset(_addr, 0, sizeof(local_addr_t));
            
            _addr->l_addr_len = sizeof(struct sockaddr_in);
            memcpy(&_addr->l_addr, pSockAddr, sizeof(struct sockaddr_in));
            _addr->next = NULL;
            strcpy_forsafe(_addr->ifname, addrIt.ifa_name.c_str(), addrIt.ifa_name.length(), sizeof(_addr->ifname));

            inet_ntop(AF_INET, &(((struct sockaddr_in *)(&_addr->l_addr))->sin_addr), szAddrName, sizeof(szAddrName));
            SY_INFO_TRACE("get_local_addr, v4 it=" << szAddrName << ", if=" << _addr->ifname);

            bool bPrim = (errPrim == 0) && (pSockAddr->sin_addr.s_addr == primSin.sin_addr.s_addr);
            if (!tail) {
                 *res = tail = _addr;
            } else if (bPrim){
                _addr->next = *res;
                *res = _addr;
            } else {
                tail->next = _addr;
                tail = _addr;
            }
        } else if (addrIt.ifa_address.ss_family == AF_INET6) {
            struct sockaddr_in6 * pSockAddr = (struct sockaddr_in6 *)&addrIt.ifa_address;
            //Link local fe:80::0/10, or loopback (::1)
            if(SY_IN6_IS_ADDR_LOOPBACK(&pSockAddr->sin6_addr) || SY_IN6_IS_ADDR_LINKLOCAL(&pSockAddr->sin6_addr)) {
                continue;
            }

            local_addr_t *_addr = new local_addr_t;
            memset(_addr, 0, sizeof(local_addr_t));
            _addr->l_addr_len = sizeof(struct sockaddr_in6);
            memcpy(&_addr->l_addr, pSockAddr, sizeof(struct sockaddr_in6));
            strcpy_forsafe(_addr->ifname, addrIt.ifa_name.c_str(), addrIt.ifa_name.length(), sizeof(_addr->ifname));
            _addr->expire = time(nullptr) + addrIt.ifa_prefered;

            _addr->next = NULL;
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(&_addr->l_addr))->sin6_addr), szAddrName, sizeof(szAddrName));
            SY_INFO_TRACE("get_local_addr, v6 it=" << szAddrName << ", if=" << _addr->ifname << ", flag=" << HEX_RESULT(addrIt.ifa_flags)
                          << ", lifetime=" << _addr->expire);

            if (!tail) {
               *res = tail = _addr;
            } else {
                tail->next = _addr;
                tail = _addr;
            }
        }
    }
    
    if (*res == nullptr && errPrim == 0) {
        local_addr_t *_addr = new local_addr_t;
        memset(_addr, 0, sizeof(local_addr_t));
        
        _addr->l_addr_len = sizeof(struct sockaddr_in);
        memcpy(&_addr->l_addr, &primSin, sizeof(struct sockaddr_in));
        _addr->next = NULL;
        SY_WARNING_TRACE("get_local_addr, put primary address into local address list while netlink failed.");
        *res = _addr;
    }

    if (*res == nullptr) {
        return -1;
    }
    return 0;
}

#endif

void free_local_addr(local_addr_t *res)
{
    local_addr_t *tmp;

    while (res) {
        tmp = res;
        res = res->next;
        delete tmp;
    }
}

#endif

#if defined(SY_WIN32)

//get primary ip address, just for IPv4
std::string getPrimaryIpStr()
{

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        SY_ERROR_TRACE("getPrimaryIp, socket failed");
        return "";
    }
    const char *kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    //serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    inet_pton(AF_INET, kGoogleDnsIp, &(serv.sin_addr));
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const sockaddr *)&serv, sizeof(serv));
    //SY_ASSERTE(err != -1);
    if (err == -1) {
        SY_ERROR_TRACE("getPrimaryIpStr, connect failed");
        ::closesocket(sock);
        return "";
    }

    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (sockaddr *)&name, &namelen);
    if (err == -1) {
        SY_ERROR_TRACE("getPrimaryIpStr, getsockname failed");
        ::closesocket(sock);
        return "";
    }
    ::closesocket(sock);
    return inet_ntoa(name.sin_addr);
}

#endif


//////////////////////////////////////////////////
int sy_ipv4_enabled = -1;
int sy_ipv6_enabled = -1;

int ip_check(int &ipvn_enabled, int pf)
{
    if (ipvn_enabled == -1) {
        // Determine if the kernel has IPv6 support by attempting to
        // create a PF_INET6 socket and see if it fails.

        SY_HANDLE s = (SY_HANDLE)::socket(pf, SOCK_DGRAM, 0);
        if (s == SY_INVALID_HANDLE) {
            ipvn_enabled = 0;
        } else {
            ipvn_enabled = 1;
#ifdef SY_WIN32
            ::closesocket((SY_SOCKET)s);
#else
            ::close((SY_SOCKET)s);
#endif
        }
    }
    return ipvn_enabled;
}

int ipv6_enabled(void)
{
    return sy_ipv6_enabled == -1 ?
           ip_check(sy_ipv6_enabled, PF_INET6) :
           sy_ipv6_enabled;
}

int ipv4_enabled(void)
{
    return sy_ipv4_enabled == -1?
           ip_check(sy_ipv4_enabled, PF_INET) :
           sy_ipv4_enabled;
}


unsigned char is_ip_address(char *lpsz)
{
    if (!lpsz) {
        SY_ERROR_TRACE("is_ip_address: ip string is null");
        return FALSE;
    }

    char *lpszTemp = lpsz;
    while (*lpszTemp) {
        if (*lpszTemp != ' ' &&  *lpszTemp != '\t') {
            if (*lpszTemp != '.' && !(*lpszTemp >= '0' && *lpszTemp <= '9')) {
                return FALSE;
            }
        }
        lpszTemp++;
    }
    return TRUE;
}

void resolve_2_ip(char *host_name, char *ip_address)
{
    SY_ASSERTE(host_name != NULL);
    SY_ASSERTE(ip_address != NULL);
    if (!host_name || !ip_address) {
        SY_ERROR_TRACE("resolve_2_ip: No host_name: " << host_name
                       << "; or ip_address: " << ip_address << "; given");
        return;
    }

    struct hostent *he = gethostbyname(host_name);
    if (he) {
        //char* tmp = (char*)inet_ntoa((in_addr&)(*he->h_addr_list[0]));
        char *tmp = NULL;
        char addressBuffer4[INET_ADDRSTRLEN];
        char addressBuffer[INET6_ADDRSTRLEN];
        if (he->h_addrtype == AF_INET) {
            tmp = (char *)inet_ntop(AF_INET, (void *)he->h_addr, addressBuffer4, INET_ADDRSTRLEN);

        } else if (he->h_addrtype == AF_INET6) {
            tmp = (char *)inet_ntop(AF_INET6, (void *)he->h_addr, addressBuffer, INET6_ADDRSTRLEN);
        }

        if (tmp) {
            strcpy(ip_address, tmp);
        }
    } else {
        SY_ERROR_TRACE("resolve_2_ip: can't resolve host_name: " << host_name);
    }
}

unsigned long ip_2_id(char *ip_address)
{
    unsigned long id;
    char digit_ip[128] = "";
    if (is_ip_address(ip_address)) {
        strcpy(digit_ip, ip_address);
    } else {
        resolve_2_ip(ip_address, digit_ip);
    }

    if (*digit_ip == 0) {
        return 0;
    }

    int tmp_ip[4];
    sscanf(digit_ip, "%d.%d.%d.%d", &tmp_ip[0], &tmp_ip[1], &tmp_ip[2], &tmp_ip[3]);
    id = tmp_ip[0];
    id <<= 8;
    id += tmp_ip[1];
    id <<= 8;
    id += tmp_ip[2];
    id <<= 8;
    id += tmp_ip[3];

    return id;
}

char *id_2_ip(unsigned long ip_address)
{
    static char ip_str[64];

    unsigned int tmp_ip[4];
    tmp_ip[3] = ip_address & 0xff;
    tmp_ip[2] = (ip_address >> 8) & 0xff;
    tmp_ip[1] = (ip_address >> 16) & 0xff;
    tmp_ip[0] = (ip_address >> 24) & 0xff;

    sprintf(ip_str, "%d.%d.%d.%d", tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);

    return ip_str;
}


#ifdef SY_WIN32
char g_local_ip[64] = "";
char *get_local_ip()
{
    if (*g_local_ip != 0) {
        return g_local_ip;
    }

    char buffer[256];
    WSADATA data;
    WORD version = 0x0101;
    int result = WSAStartup(version, &data);
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        HOSTENT *host = gethostbyname(buffer);
        SY_ASSERTE_RETURN(host, NULL);
        in_addr in;
        memcpy(&in.S_un.S_addr, host->h_addr, host->h_length);
        char *addr = inet_ntoa(in);
        if (addr) {
            strcpy(g_local_ip, addr);
        } else {
            strcpy(g_local_ip, "0.0.0.0");
        }

        WSACleanup();
    } else {
        WSACleanup();
        strcpy(g_local_ip, "0.0.0.0");
    }

    return g_local_ip;
}
#else
char g_local_ip[64] = "";
char *get_local_ip()
{
    if (*g_local_ip != 0) {
        return g_local_ip;
    }

    strcpy(g_local_ip, "0.0.0.0");
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        struct hostent *host = gethostbyname(buffer);
        SY_ASSERTE(host);
        //char* addr = inet_ntoa(in);
        char *addr = NULL;
        if (host->h_addrtype == AF_INET) {
            char addressBuffer[INET_ADDRSTRLEN];
            addr = (char *)inet_ntop(AF_INET, (void *)host->h_addr, addressBuffer, INET_ADDRSTRLEN);

        } else if (host->h_addrtype == AF_INET6) {
            char addressBuffer[INET6_ADDRSTRLEN];
            addr = (char *)inet_ntop(AF_INET6, (void *)host->h_addr, addressBuffer, INET6_ADDRSTRLEN);
        }
        if (addr) {
            strcpy(g_local_ip, addr);
        }
    }

    return g_local_ip;
}
#endif  // SY_WIN32

/*void resolve_to_ip(char *lpszHostName, char *lpszIPAddress)
{
    struct hostent *he = gethostbyname((char *)lpszHostName);
    if (he) {
        //char* lpszTemp = (char*)inet_ntoa((in_addr&)(*he->h_addr_list[0]));
        char *lpszTemp = NULL;
        char addressBuffer[INET_ADDRSTRLEN];
        char addressBuffer6[INET6_ADDRSTRLEN];
        if (he->h_addrtype == AF_INET) {
            lpszTemp = (char *)inet_ntop(AF_INET, (void *)he->h_addr, addressBuffer, INET_ADDRSTRLEN);

        } else if (he->h_addrtype == AF_INET6) {
            lpszTemp = (char *)inet_ntop(AF_INET6, (void *)he->h_addr, addressBuffer6, INET6_ADDRSTRLEN);
        }
        if (lpszTemp && lpszIPAddress) {
            strcpy(lpszIPAddress, lpszTemp);
        }
    }
}*/

/*char *get_local_ip_address()
{
    char achBuf[256];
    achBuf[0] = 0;
    char *addr = NULL;

    if (gethostname(achBuf, sizeof(achBuf)) == 0) {
        struct hostent *pHost = gethostbyname(achBuf);

        if (pHost) {
            if (pHost->h_addrtype == AF_INET) {
                char addressBuffer[INET_ADDRSTRLEN];
                addr = (char *)inet_ntop(AF_INET, (void *)pHost->h_addr, addressBuffer, INET_ADDRSTRLEN);

            } else if (pHost->h_addrtype == AF_INET6) {
                char addressBuffer[INET6_ADDRSTRLEN];
                addr = (char *)inet_ntop(AF_INET6, (void *)pHost->h_addr, addressBuffer, INET6_ADDRSTRLEN);
            }

            //char* addr = inet_ntoa(in);
            SY_INFO_TRACE("get_local_ip_address(), IP Address = " << addr);

            return addr;
        }
    }

    // return the local host predefined address
    return (char *)"127.0.0.1";
}*/

unsigned char transport_address_parse(
    char *transport_address, char *protocol_type, int max_protocol_len,
    char *host_ip, int max_host_ip_len, unsigned short *port)
{
    SY_ASSERTE(transport_address);
    SY_ASSERTE(protocol_type);
    SY_ASSERTE(host_ip);
    SY_ASSERTE(port);

    char tmp_protocol[64] = "";
    char tmp_host_name[64] = "";
    char tmp_host_ip[64] = "";
    char tmp_port[64] = "";

    if (strlen(transport_address)>64) {
        return FALSE;
    }

    char *tmp = strstr(transport_address, "://");
    if (!tmp) {
        return FALSE;
    }
    strncpy(tmp_protocol, transport_address, tmp-transport_address);
    tmp_protocol[tmp-transport_address] = 0;
    if ((int)strlen(tmp_protocol)<max_protocol_len) {
        strcpy(protocol_type, tmp_protocol);
    } else {
        strncpy(protocol_type, tmp_protocol, max_protocol_len-1);
        protocol_type[max_protocol_len-1] = 0;
    }

    char *tmp1 = strstr(tmp+3, ":");
    if (!tmp1) {
        return FALSE;
    }
    strncpy(tmp_host_name, tmp+3, tmp1-tmp-3);
    tmp_host_name[tmp1-tmp-3] = 0;

    if (!is_ip_address(tmp_host_name)) {
        resolve_2_ip(tmp_host_name, tmp_host_ip);
        if (*tmp_host_ip == 0) {
            return FALSE;
        }
    } else {
        strcpy(tmp_host_ip, tmp_host_name);
    }


    if ((int)strlen(tmp_host_ip)<max_host_ip_len) {
        strcpy(host_ip, tmp_host_ip);
    } else {
        strncpy(host_ip, tmp_host_ip, max_host_ip_len-1);
        host_ip[max_host_ip_len-1] = 0;
    }

    tmp1++;
    if (!(*tmp1)) {
        SY_ASSERTE(FALSE);
        return FALSE;
    }
    strcpy(tmp_port, tmp1);
    int port_1;
    sscanf(tmp_port, "%d", &port_1);
    *port = port_1;

    return TRUE;
}

void sy_reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end) {
        std::swap(*(str+start), *(str+end));
        start++;
        end--;
    }
}

#if defined(SY_MACOS) || defined(SY_IOS)
bool isPureIp6()
{
    struct ifaddrs *ifAddrStruct=NULL;
    struct ifaddrs *ifa=NULL;
    void *tmpAddrPtr=NULL;
    bool bIp4 = false;
    bool bIp6 = false;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if ((ifa->ifa_flags & IFF_UP) == 0) {
            continue;    // ignore if interface not up
        }

        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=(struct sockaddr_in *)ifa->ifa_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, addressBuffer, INET_ADDRSTRLEN);
            SY_DEBUG_TRACE("isPureIp6(), IP Address = " << ifa->ifa_name << ", " << addressBuffer << ", sa_family=" << AF_INET);
            if (strncmp(addressBuffer, "169.254.", 8) != 0 && strncmp(addressBuffer, "127.0.0.1", 9) != 0) {
                bIp4 = true;
            }

        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=(struct sockaddr_in6 *)ifa->ifa_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, addressBuffer, INET6_ADDRSTRLEN);
            if (strncmp(addressBuffer, "fe80:", 5) != 0 && strncmp(addressBuffer, "fe80::1", 7) != 0 && strncmp(addressBuffer, "::1", 3) != 0) {
                bIp6 = true;
            }
            SY_DEBUG_TRACE("isPureIp6(), IP Address = " << ifa->ifa_name << ", " << addressBuffer << ", sa_family=" << AF_INET6);
        }
    }
    if (ifAddrStruct!=NULL) { freeifaddrs(ifAddrStruct); }
    if (!bIp4 && bIp6) {
        return true;
    }
    return false;
}
#elif defined(SY_WIN32)
bool isPureIp6()
{
    int ret;
    bool bIp4 = false;
    bool bIp6 = false;
    char buffer[256];
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;

    if (gethostname(buffer, sizeof(buffer)) != 0) {
        return false;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;

    ret = getaddrinfo(buffer, NULL, &hints, &res);

    if (ret) {
        return false;
    }

    struct addrinfo *tmp = res;
    while (tmp) {
        if (tmp->ai_family == AF_INET) {
            sin = (struct sockaddr_in *)tmp->ai_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)tmp->ai_addr)->sin_addr, addressBuffer, INET_ADDRSTRLEN);
            if (strncmp(addressBuffer, "169.254.", 8) != 0 && strncmp(addressBuffer, "127.0.0.1", 9) != 0) {
                bIp4 = true;
            }
        } else if (tmp->ai_family == AF_INET6) {
            sin6 = (struct sockaddr_in6 *)tmp->ai_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)tmp->ai_addr)->sin6_addr, addressBuffer, INET6_ADDRSTRLEN);
            if (strncmp(addressBuffer, "fe80:", 5) != 0 && strncmp(addressBuffer, "fe80::1", 7) != 0 && strncmp(addressBuffer, "::1", 3) != 0) {
                bIp6 = true;
            }
        }
        tmp = tmp->ai_next;
    }

    freeaddrinfo(res);

    if (!bIp4 && bIp6) {
        return true;
    }
    return false;
}
#elif defined(SY_ANDROID)
bool isPureIp6()
{
    bool bIp4 = false;
    bool bIp6 = false;
    int                 i, len, flags;
    char                buffer[BUFFERSIZE], *ptr, lastname[IFNAMSIZ], *cptr;
    struct ifconf       ifc;
    struct ifreq        *ifr, ifrcopy;
    struct sockaddr_in      *sin;
    void *tmpAddrPtr;

    char temp[80];

    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return false;
    }

    ifc.ifc_len = BUFFERSIZE;
    ifc.ifc_buf = buffer;

    if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
        return false;
    }

    lastname[0] = 0;
    for (ptr = buffer; ptr < buffer + ifc.ifc_len;) {
        ifr = (struct ifreq *)ptr;

        if (ifr->ifr_addr.sa_family == AF_INET6) {
            len = sizeof(struct sockaddr_in6);
        } else {
            len = sizeof(struct sockaddr);
        }
        SY_DEBUG_TRACE("get_local_addr,ifc_len = " << ifc.ifc_len << " ifr_name:" << ifr->ifr_name << " struct len: " << len);
        ptr += sizeof(struct ifreq);//sizeof(ifr->ifr_name) + len;     // for next one in buffer

        if ((cptr = (char *)strchr(ifr->ifr_name, ':')) != NULL) {
            *cptr = 0;    // replace colon will null
        }

        if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0) {
            continue;    // already processed this interface
        }
        memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

        ifrcopy = *ifr;
        ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        if ((flags & IFF_UP) == 0) {
            continue;    // ignore if interface not up
        }

        if (ifr->ifr_addr.sa_family == AF_INET) {
            tmpAddrPtr = &ifr->ifr_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(&ifr->ifr_addr)->sin_addr, addressBuffer, INET_ADDRSTRLEN);
            if (strncmp(addressBuffer, "169.254.", 8) != 0 && strncmp(addressBuffer, "127.0.0.1", 9) != 0) {
                bIp4 = true;
            }

        }
    }

    close(sockfd);

    FILE *f;
    int ret, scope, prefix;
    unsigned char ipv6[16];
    char dname[IFNAMSIZ];
    char addressBuffer[INET6_ADDRSTRLEN];
    char *scopestr;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return false;
    }

    while (19 == fscanf(f,
                        " %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %*x %x %x %*x %s",
                        &ipv6[0],
                        &ipv6[1],
                        &ipv6[2],
                        &ipv6[3],
                        &ipv6[4],
                        &ipv6[5],
                        &ipv6[6],
                        &ipv6[7],
                        &ipv6[8],
                        &ipv6[9],
                        &ipv6[10],
                        &ipv6[11],
                        &ipv6[12],
                        &ipv6[13],
                        &ipv6[14],
                        &ipv6[15],
                        &prefix,
                        &scope,
                        dname)) {

        if (inet_ntop(AF_INET6, ipv6, addressBuffer, sizeof(addressBuffer)) == NULL) {
            continue;
        }

        if (strncmp(addressBuffer, "fe80:", 5) != 0 && strncmp(addressBuffer, "fe80::1", 7) != 0 && strncmp(addressBuffer, "::1", 3) != 0) {
            bIp6 = true;
        }
    }

    fclose(f);
    if (!bIp4 && bIp6) {
        return true;
    }
    return false;
}
#else
bool isPureIp6()
{
    if (ipv6_enabled() && !ipv4_enabled()) {
        return true;
    }
    return false;
}
#endif

int strcpy_forsafe(char *dst, const char *src, size_t src_len, size_t dst_len)
{
    if (!src || !dst || src_len == 0 || dst_len == 0)
        return -1;
    if (src_len < dst_len) {
        memcpy(dst, src, src_len);
        dst[src_len] = 0;
    }
    else {
        memcpy(dst, src, dst_len - 1);
        dst[dst_len - 1] = 0;
        SY_ASSERTE(FALSE);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// UNIX misc functions
//
unsigned long calculate_tick_interval(unsigned long start, unsigned long end)
{
    if (end >= start) {
        return end - start;
    }

    return (unsigned long)-1 - start + 1 + end;
}


//////////////////////////////////////////////////////////////////////////////
// dynamic library load/unload
//
#ifdef SY_WIN32
#define PATH_SEPARATOR  '\\'
#else
#define PATH_SEPARATOR  '/'
#endif
SyResult sy_get_module_path(const void* addr_in_module, char *path_buf, size_t buf_size)
{
    if (!addr_in_module) {
        return SY_ERROR_INVALID_ARG;
    }
    std::string str_path;
#ifdef SY_WIN32
    HMODULE hmodule = 0;
    auto flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    auto ret = ::GetModuleHandleEx(flags, (LPCTSTR)addr_in_module, &hmodule);
    if (!ret) {
        return SY_ERROR_FAILURE;
    }
    char file_name[2048] = {0};
    auto count = ::GetModuleFileName(hmodule, file_name, sizeof(file_name));
    if (count == 0) {
        return SY_ERROR_FAILURE;
    }
    str_path = file_name;
#elif defined(SY_MACOS) || defined(SY_LINUX)
    Dl_info dl_info;
    dladdr((void*)addr_in_module, &dl_info);
    
    str_path = dl_info.dli_fname;
#else
    return SY_ERROR_NOT_IMPLEMENTED;
#endif
    
    auto pos = str_path.rfind(PATH_SEPARATOR);
    if (pos == std::string::npos) {
        return SY_ERROR_FAILURE;
    }
    str_path.erase(pos);
    
#ifdef SY_MACOS
    /*
     for test.framework(or test.bundle) the structure is
        test.framework/Version/A/test
        test.framework/test(it just softlink)
    */
    auto erase_framework = [&str_path] (size_t pos1, size_t pos2) -> bool {
        if (pos2 != std::string::npos) {
            auto name3 = str_path.substr(pos2+1, pos1-pos2-1);
            pos1 = name3.rfind('.');
            if (pos1 != std::string::npos) {
                auto ext = name3.substr(pos1+1);
                if (ext == "framework" || ext == "bundle") {
                    str_path.erase(pos2);
                    return true;
                }
            }
        }
        return false;
    };
    auto pos1 = str_path.rfind(PATH_SEPARATOR);
    if (!erase_framework(str_path.length(), pos1)) {
        auto pos2 = pos1;
        int count = 3;
        while (pos2 != std::string::npos && pos2 > 0 && --count > 0) {
            pos1 = pos2;
            pos2 = str_path.rfind(PATH_SEPARATOR, pos1-1);
        }
        if (pos2 != std::string::npos) {
            erase_framework(pos1, pos2);
        }
    }
#endif
    
    if (str_path.size() >= buf_size) {
        return SY_ERROR_CAPACITY_LIMIT;
    }
    memcpy(path_buf, str_path.c_str(), str_path.size());
    path_buf[str_path.size()] = '\0';
    return SY_OK;
}

SY_HMODULE sy_load_library(const char* file_name, size_t name_size)
{
    if (!file_name || !name_size) {
        return nullptr;
    }
#ifdef SY_WIN32
    return LoadLibrary(file_name);
#elif defined(SY_MACOS)
    bool is_bundle = false;
    auto ptr = strrchr(file_name, '.');
    if (ptr) {
        std::string ext = ptr + 1;
        if (ext == "framework" || ext == "bundle") {
            is_bundle = true;
        }
    }
    
    if (is_bundle) {
        auto url_ref = CFURLCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)file_name, name_size, kCFStringEncodingUTF8, nil);
        
        if(!url_ref) {
            return {};
        }
        
        CFBundleRef bundle_ref = CFBundleCreate(kCFAllocatorSystemDefault, url_ref);
        CFRelease(url_ref);
        
        if(bundle_ref) {
            //	bReturn = CFBundleLoadExecutable(bundleRef);
        }
        
        return {bundle_ref};
    } else {
        auto hmod = dlopen(file_name, RTLD_LAZY);
        return {hmod};
    }
#elif defined(SY_LINUX)
    return dlopen(file_name, RTLD_LAZY);
#else
    return nullptr;
#endif
}

void* sy_get_proc_address(SY_HMODULE hmodule, const char* proc_name)
{
    if(!hmodule) {
        return nullptr;
    }
#ifdef SY_WIN32
    return GetProcAddress(hmodule, proc_name);
#elif defined(SY_MACOS)
    if (hmodule.is_bundle && hmodule.bundle) {
        CFStringRef name_ref = CFStringCreateWithCString(NULL,proc_name,kCFStringEncodingUTF8);
        void* proc_addr = CFBundleGetFunctionPointerForName(hmodule.bundle, name_ref);
        CFRelease(name_ref);
        return proc_addr;
    } else if (!hmodule.is_bundle && hmodule.handle) {
        return dlsym(hmodule.handle, proc_name);
    }
    return nullptr;
#elif defined(SY_LINUX)
    return dlsym(hmodule, proc_name);
#else
    return nullptr
#endif
}

void sy_free_library(SY_HMODULE hmodule)
{
    if(!hmodule) {
        return ;
    }
#ifdef SY_WIN32
    FreeLibrary(hmodule);
#elif defined(SY_MACOS)
    if (hmodule.is_bundle && hmodule.bundle) {
        CFRelease(hmodule.bundle);
    } else if (!hmodule.is_bundle && hmodule.handle) {
        dlclose(hmodule.handle);
    }
#elif defined(SY_LINUX)
    dlclose(hmodule);
#endif
}

static const char* file_path_name_keywords[] = {
    "\\appdata",
    "/application support",
};
SyResult sy_get_filepath_trace(const char* file_path, char *file_path_trace, size_t buf_size) {
    if (file_path == NULL || file_path_trace == NULL || buf_size <= 0) {
        return SY_ERROR_FAILURE;
    }
    std::string strFilePath = file_path;
    std::transform(strFilePath.begin(), strFilePath.end(), strFilePath.begin(), ::tolower);

    for (auto keyword : file_path_name_keywords) {
        auto pos = strFilePath.find(keyword);
        if (pos != std::string::npos) {
#ifdef SY_WIN32
            std::string strTrace = strFilePath.substr(0, 3) + "*" + strFilePath.substr(pos);
#else
            std::string strTrace = strFilePath.substr(0, 1) + "*" + strFilePath.substr(pos);
#endif
            strcpy_forsafe(file_path_trace, strTrace.c_str(), strTrace.length(), buf_size - 1);
            return SY_OK;
        }
    }
#if defined(DEBUG) || defined(_DEBUG)
    strcpy_forsafe(file_path_trace, file_path, buf_size - 1, buf_size - 1);
#endif
    return SY_ERROR_FAILURE;
}

#if defined(SY_WIN32)
int sy_launch_process(int argc, const char *argv[], const char *env) {
    if (argc <= 0 || argv == NULL) {
        return 0;
    }
    
    int pid = 0;
    std::string strSydLine;
    std::vector<std::string> vArgs = { argv, argv + argc };
    std::stringstream strstream;
    std::copy(vArgs.begin(),
        vArgs.end(),
        std::ostream_iterator<std::string>(strstream, " "));
    strSydLine = strstream.str();

    STARTUPINFOA startupInfo = { 0 };
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation = { 0 };
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;
    LPVOID lpEnvironment = nullptr;
    if (env) {
        lpEnvironment = (LPVOID)env;
    }
    if (::CreateProcessA(nullptr, (LPSTR)strSydLine.c_str(), nullptr, nullptr,
                         FALSE, dwCreationFlags, lpEnvironment, nullptr,
                         &startupInfo, &processInformation)) {
        ::CloseHandle(processInformation.hProcess);
        ::CloseHandle(processInformation.hThread);
        pid = processInformation.dwProcessId;
    }
    else {
        SY_ERROR_TRACE("sy_launch_process failed, reason=" << ::GetLastError()
                      << ",path=" << sy_get_filepath_trace_string(std::move(strSydLine)));
    }
    return pid;
}
#elif defined(SY_MACOS)
    //refer to file SyUtilMisc.mm
#else
int sy_launch_process(int argc, const char *argv[], const char *env) {
    if (argc <= 0 || argv == NULL) {
        return 0;
    }
    //don't implement yet.
    return 0;
}
#endif

#if defined(SY_WIN32)
SyResult sy_get_process_name(char *process_name, size_t buf_size) {
    if (!process_name || !buf_size) {
        return SY_ERROR_INVALID_ARG;
    }
    char path[MAX_PATH] = { 0 };
    ::GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string str = path;
    auto pos = str.rfind('\\');
    if (pos != std::string::npos) {
        strcpy_forsafe(process_name, str.substr(pos + 1).c_str(), buf_size - 1, buf_size - 1);
    }
    else {
        strcpy_forsafe(process_name, str.c_str(), buf_size - 1, buf_size - 1);
    }
    return SY_OK;
}
#elif defined(SY_MACOS)
//refer to file SyUtilMisc.mm
#else
SyResult sy_get_process_name(char *process_name, size_t buf_size) {
    return SY_ERROR_NOT_IMPLEMENTED;
}
#endif

const char* sy_get_platform() {
#if defined(SY_IOS)
    return "ios";
#elif defined(SY_ANDROID)
    return "android";
#elif defined(SY_MAC)
    return "mac";
#elif defined(SY_WIN_DESKTOP)
    return "win";
#elif defined(SY_WIN_PHONE)
    return "winphone";
#elif defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
    return "linux";
#endif
    return "unknown";
}

#if defined(SY_WIN32)
#include <Windows.h>
#include "SyOptional.h"
bool IsWindowsWow64Process() {
#if defined(SY_WIN_PHONE)
    return false;
#endif
    static sy_optional<bool> isWow64Process;
    if (!isWow64Process.has_value()) {
        typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
        BOOL bIsWow64 = FALSE;
        LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
        if (NULL != fnIsWow64Process) {
            if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
                bIsWow64 = FALSE;
            }
        }
        isWow64Process = (bIsWow64 == TRUE) ? true : false;
    }
    return (*isWow64Process);
}

SY_OS_EXPORT bool IsWindowsOS64() {
#if defined(SY_WIN_PHONE)
    return false;
#endif
    
#if defined(_WIN64)
    return true;
#else
    return IsWindowsWow64Process();
#endif
}
#endif


#if defined(SY_ANDROID)
#include <sys/system_properties.h>
#elif defined(SY_IOS) || defined(SY_MAC)
#include <sys/sysctl.h>
#include "SystemCapacity.h"
#elif defined(SY_WIN_DESKTOP)
#include "SystemCapacity.h"
#include <windows.h>
#elif defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
#include <unistd.h>
#include <limits.h>
#endif

const char* sy_get_devicemodel() {
    static char device_model[256] = {0};
    if (device_model[0] == 0) {
        std::string model;
#if defined(SY_IOS)
        size_t size=0;
        sysctlbyname("hw.machine", NULL, &size, NULL, 0);
        if(size > 0){
            char *s = (char*)malloc(size);
            sysctlbyname("hw.machine", s, &size, NULL, 0);
            model = s;
            free(s);
        }
#elif defined(SY_MAC)
        size_t size=0;
        sysctlbyname("hw.model", NULL, &size, NULL, 0);
        if(size > 0){
            char *s = (char*)malloc(size);
            sysctlbyname("hw.model", s, &size, NULL, 0);
            model = s;
            free(s);
        }
#elif defined(SY_ANDROID)
        char mod[PROP_VALUE_MAX + 1] = {0};
        if (__system_property_get("ro.product.model", mod) > 0) {
            model =  mod;
        }
#elif defined(SY_WIN_DESKTOP)
        HKEY  hKey = NULL;
        DWORD keyOption = KEY_QUERY_VALUE;
        if (IsWindowsWow64Process())
            keyOption = KEY_QUERY_VALUE | KEY_WOW64_64KEY;
        
        LRESULT lRet = ERROR_SUCCESS;
        if ((lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, keyOption, &hKey)) == ERROR_SUCCESS) {
            DWORD dwValueLen = 0;
            lRet = RegQueryValueExA(hKey, "SystemProductName", 0, NULL, NULL, &dwValueLen);
            if (lRet == ERROR_SUCCESS && dwValueLen > 0) {
                if (model.size() < dwValueLen)
                    model.resize(dwValueLen);
                lRet = RegQueryValueExA(hKey, "SystemProductName", 0, NULL, reinterpret_cast<LPBYTE>(const_cast<char *>(model.data())), &dwValueLen);
            }
        }
        if (hKey) {
            RegCloseKey(hKey);
        }
#elif defined(SY_WIN_PHONE)
        model="winphone";
#elif defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
        char hostname[HOST_NAME_MAX] = {0};
        gethostname(hostname, HOST_NAME_MAX);
        model = hostname;
#endif
        if (model.length() <= 0) {
            model="unknown";
        }
        strcpy_forsafe(device_model, model.c_str(), 256, 256);
    }
    return device_model;
}

#if defined(SY_IOS) || defined(SY_MAC) || defined(SY_ANDROID) || defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
#include <sys/utsname.h>
#endif

const char* sy_get_osver() {
    static char osver[256] = {0};
    if (osver[0] == 0) {
        std::string info;
#if defined(SY_IOS)
        info = std::to_string(get_iphone_version());
#elif defined(SY_MAC)
        long sMajorVer = 0, sMinorVer = 0, sPatchVer = 0;
        getMacSystemVersion(&sMajorVer, &sMinorVer, &sPatchVer);
        info = std::to_string(sMajorVer) + "." + std::to_string(sMinorVer) + "." + std::to_string(sPatchVer);
        
        size_t size;
        cpu_type_t type;
        
        size = sizeof(type);
        sysctlbyname("hw.cputype", &type, &size, NULL, 0);
#elif defined(SY_ANDROID)
        char sdk_ver_str[PROP_VALUE_MAX] = {0};
        if(__system_property_get("ro.build.version.sdk", sdk_ver_str) > 0) {
            info = sdk_ver_str;
        }
#elif defined(SY_WIN_DESKTOP)
        uint32_t uMajorVer = 0, uMinorVer = 0, uBuildNumber = 0;
        WbxGetOSVersionNumber(uMajorVer, uMinorVer, uBuildNumber);
        info = std::to_string(uMajorVer) + "." + std::to_string(uMinorVer) + "." + std::to_string(uBuildNumber);
#elif defined(SY_WIN_PHONE)
#elif defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
        struct utsname name;
        if (::uname(&name) == 0) {
            info = name.release;
        }
#endif
        if (info.length() <= 0) {
            info="unknown";
        }
        strcpy_forsafe(osver, info.c_str(), 256, 256);
    }
    return osver;
}

const char* sy_get_osarch() {
    static char osarch[256] = {0};
    if (osarch[0] == 0) {
        std::string info;
#if defined(SY_IOS) || defined(SY_MAC) || defined(SY_ANDROID) || defined(SY_LINUX) || defined(SY_LINUX_SERVER) || defined(SY_LINUX_CLIENT)
        struct utsname name = {0};
        if (::uname(&name) == 0) {
            info = name.machine;
        }
#elif defined(SY_WIN_DESKTOP)
        info = IsWindowsOS64() ? "64bit" : "32bit";
#endif
        if (info.length() <= 0) {
            info="unknown";
        }
        strcpy_forsafe(osarch, info.c_str(), 256, 256);
    }
    return osarch;
}

const char* sy_get_osinfo() {
    static char osinfo[256] = {0};
    if (osinfo[0] == 0) {
        std::ostringstream info;
#if defined(SY_WIN_DESKTOP)
        HKEY  hKey = NULL;
        DWORD keyOption = KEY_QUERY_VALUE;
        if (IsWindowsWow64Process())
            keyOption = KEY_QUERY_VALUE | KEY_WOW64_64KEY;
        
        LRESULT lRet = ERROR_SUCCESS;
        if ((lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, keyOption, &hKey)) == ERROR_SUCCESS) {
            char szBuffer[64] = {0};
            DWORD dwValueLen = 63;
            if(RegQueryValueExA(hKey, "ProductName", 0, NULL, (LPBYTE)szBuffer, &dwValueLen) == ERROR_SUCCESS) {
                szBuffer[dwValueLen] = 0;
                std::string platform = szBuffer;
                platform.erase(std::remove_if(platform.begin(), platform.end(), ::isspace), platform.end());
                info << platform << "," << sy_get_osver() << "," << sy_get_osarch();
            }
        }
        if (hKey) {
            RegCloseKey(hKey);
        }
#else
        info << sy_get_platform() << "," << sy_get_osver() << "," << sy_get_osarch();
#endif
        if (info.str().length() <= 0) {
            info << "unknown";
        }
        strcpy_forsafe(osinfo, info.str().c_str(), 256, 256);
    }
    return osinfo;
}
END_UTIL_NS
