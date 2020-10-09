#ifndef SY_UTILMISC_H
#define SY_UTILMISC_H

#include "SyMutex.h"
#include "errno.h"

#ifdef SY_WIN32
    #include "SyStdCpp.h"
#endif

START_UTIL_NS

class CSyErrnoGuard
{
public:
    CSyErrnoGuard()
    {
#ifdef SY_WIN32
        m_nErr = ::WSAGetLastError();
#else
        m_nErr = errno;
#endif // SY_WIN32
    }

    ~CSyErrnoGuard()
    {
        errno = m_nErr;
    }
private:
    int m_nErr;
};


//a is equal to b
template <typename T>
static inline bool isSequenceEQ(T a, T b)
{
    return a == b;
}

//a isn't equal to b
template <typename T>
static inline bool isSequenceNE(T a, T b)
{
    return a != b;
}

//a is ahead of b
template <typename T>
static bool isSequenceLT(T a, T b)
{
    int shift = sizeof(T) * 8 - 1;
    T tmp = (b > a) ? (b - a) : (a - b);
    if ((tmp >> shift) == 0) {
        return a < b;
    }
    return a > b;
}

//b is ahead of a
template <typename T>
static inline bool isSequenceGT(T a, T b)
{
    int shift = sizeof(T) * 8 - 1;
    T tmp = (b > a) ? (b - a) : (a - b);
    if ((tmp >> shift) == 0) {
        return b < a;
    }
    return b > a;
}

//b is not ahead of a
template <typename T>
static inline bool isSequenceLE(T a, T b)
{
    return !isSequenceGT(a,b);
}

//a is not ahead of b
template <typename T>
static inline bool isSequenceGE(T a, T b)
{
    return !isSequenceLT(a, b);
}

//the numbers a ahead of b
/*usage:
 unsigned int a,b;
 sequenceDiff<int>(a, b));
 */
template <typename RET, typename T>
static inline RET sequenceDiff(T a, T b)
{
    return RET((a)-(b));
}

SY_OS_EXPORT void GenerateCRCTable();
SY_OS_EXPORT WORD CalcCRC16(WORD &wResult, BYTE *pData,DWORD dwLen);

SY_OS_EXPORT unsigned long calculate_tick_interval(unsigned long start, unsigned long end);

extern "C" SY_OS_EXPORT void SleepMs(DWORD aMsec);
extern "C" SY_OS_EXPORT void SleepMsWithLoop(DWORD aMsec);
extern "C" SY_OS_EXPORT int ipv4_enabled(void);
extern "C" SY_OS_EXPORT int ipv6_enabled(void);
extern "C" SY_OS_EXPORT int ip_check(int &ipvn_enabled, int pf);

/////////
SY_OS_EXPORT bool isPureIp6();
SY_OS_EXPORT char *get_local_ip();
//SY_OS_EXPORT char *get_local_ip_address(); //This function has memory bug. It is deprecated.
SY_OS_EXPORT unsigned char is_ip_address(char *sz);
SY_OS_EXPORT void resolve_2_ip(char *host_name, char *ip_address);
SY_OS_EXPORT unsigned long ip_2_id(char *ip_address);
SY_OS_EXPORT char *id_2_ip(unsigned long ip_address);
SY_OS_EXPORT unsigned char transport_address_parse(char *transport_address,
        char *protocol_type, int max_protocol_len,
        char *host_ip, int max_host_ip_len, unsigned short *port);
/////////

SY_OS_EXPORT int strcpy_forsafe(char *dst, const char *src, size_t src_len, size_t dst_len);

typedef enum {
    SyV6Addr_None = 0,
    SyV6Addr_AutoConf = 1,
    SyV6Addr_Temp = 2,
    SyV6Addr_Dynamic = 4,
    SyV6Addr_Invalid = 8,
} SY_IPV6_ADDR_FLAG;

typedef struct local_addr {
    struct sockaddr_storage l_addr;
    int l_addr_len;
    char ifname[64];
    uint64_t expire;
    SY_IPV6_ADDR_FLAG flag6;
    struct local_addr *next;
} local_addr_t;

extern "C" SY_OS_EXPORT int get_local_addr(local_addr_t **res);
//prune local IP addresses from the list, remove non-temp address if temp addresses are found (IPv6 only)
//also prune the local addresses which expired the prefer lifetime
extern "C" SY_OS_EXPORT void prune_local_addr(local_addr_t *&res);
extern "C" SY_OS_EXPORT void free_local_addr(local_addr_t *res);
extern "C" SY_OS_EXPORT int32_t CalcDiffUint24(uint32_t u24_1, uint32_t u24_2);
#define CalcDiffUint16(u16_1, u16_2) (INT16(u16_1 - u16_2))
#define CalcDiffUint32(u32_1, u32_2) (INT32(u32_1 - u32_2))

/////////////////////////////////////////////////////////////////////
// dynamic library load/unload
#ifdef SY_WIN32
#define SY_HMODULE		HMODULE
SY_OS_EXPORT bool IsWindowsWow64Process();
SY_OS_EXPORT bool IsWindowsOS64();
#elif defined(SY_MACOS)
#include <CoreFoundation/CFBundle.h>
typedef struct _SY_HMODULE {
    bool is_bundle;
    union {
        void* handle;
        CFBundleRef bundle;
    };
    
    _SY_HMODULE()
    : is_bundle(false), handle(NULL)
    {}
    // C++11 is not enabled in MMP nbr_xxx projects
#if __cplusplus >= 201103L
    _SY_HMODULE(std::nullptr_t)
    : is_bundle(false), handle(nullptr)
    {}
#endif
    _SY_HMODULE(void* h)
    : is_bundle(false), handle(h)
    {}
    _SY_HMODULE(CFBundleRef b)
    : is_bundle(true), bundle(b)
    {}
    operator bool() const
    {
        return is_bundle?bundle:handle;
    }
} SY_HMODULE;
#else
#define SY_HMODULE		void*
#endif

SY_OS_EXPORT SyResult sy_get_module_path(const void* addr_in_module, char *path_buf, size_t buf_size);
SY_OS_EXPORT SY_HMODULE sy_load_library(const char* file_name, size_t name_size);
SY_OS_EXPORT void* sy_get_proc_address(SY_HMODULE hmodule, const char* proc_name);
SY_OS_EXPORT void sy_free_library(SY_HMODULE hmodule);
SY_OS_EXPORT SyResult sy_get_filepath_trace(const char* file_path, char *file_path_trace, size_t buf_size);
//! launch new process.
/*!
 \param argc command line params count.
 \param argv command line params.
 \param env environment variable.
 \return pid
 */
SY_OS_EXPORT int sy_launch_process(int argc, const char *argv[], const char *env);
SY_OS_EXPORT SyResult sy_get_process_name(char *process_name, size_t buf_size);
SY_OS_EXPORT const char* sy_get_platform();
SY_OS_EXPORT const char* sy_get_devicemodel();
SY_OS_EXPORT const char* sy_get_osver();
SY_OS_EXPORT const char* sy_get_osarch();
SY_OS_EXPORT const char* sy_get_osinfo();

SY_OS_EXPORT void getMacSystemVersion(long *pMajorVer, long *pMinorVer, long *pPatchVer);

#ifdef SY_WIN32

#include <Ws2tcpip.h>

typedef int (WSAAPI *pf_getaddrinfo)(
                                     _In_opt_  PCSTR pNodeName,
                                     _In_opt_  PCSTR pServiceName,
                                     _In_opt_  const ADDRINFOA *pHints,
                                     _Out_     PADDRINFOA *ppResult
                                     );

typedef int (WSAAPI *pf_getnameinfo)(
                                     __in   const struct sockaddr FAR *sa,
                                     __in   socklen_t salen,
                                     __out  char FAR *host,
                                     __in   DWORD hostlen,
                                     __out  char FAR *serv,
                                     __in   DWORD servlen,
                                     __in   int flags
                                     );

typedef void (WSAAPI *pf_freeaddrinfo)(
                                       __in  struct addrinfo *ai
                                       );

#ifdef SY_UNITTEST
extern __declspec(dllimport) pf_getaddrinfo tp_getaddrinfo;
extern __declspec(dllimport) pf_getnameinfo tp_getnameinfo;
extern __declspec(dllimport) pf_freeaddrinfo tp_freeaddrinfo;
#else
extern SY_OS_EXPORT pf_getaddrinfo tp_getaddrinfo;
extern SY_OS_EXPORT pf_getnameinfo tp_getnameinfo;
extern SY_OS_EXPORT pf_freeaddrinfo tp_freeaddrinfo;
#endif


extern HMODULE g_hmod_ws2_32;
extern int g_support_ipv6_resolve;
int SY_OS_EXPORT ipv6_dns_resolve(void);

#endif //SY_WIN32

END_UTIL_NS

#endif  //SY_UTILMISC_H
