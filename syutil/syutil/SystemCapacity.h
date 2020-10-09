#ifndef _SYSTEMCAPACITY_H_
#define _SYSTEMCAPACITY_H_

#include "SyDef.h"

#ifdef _UNIT_TEST
    #ifdef AT_IOS
        int test_main();
    #endif
#endif

#if defined AT_APPLE
    //cpu_usage=10 means 10%
    int get_cpu_usage(float &cpu_usage);

#endif

//memory_usage=10 means 10%, memory_used = 1000 means 1000 MBs, memory_size = 2000 means 2000MBs
SY_OS_EXPORT int get_memory_usage(float &memory_usage, uint64_t &memory_used, uint64_t &memory_size);
SY_OS_EXPORT int get_process_memory(uint64_t &memory_used);

#ifdef AT_WIN
    #include <winsock2.h>
    #include <windows.h>
    #include <VersionHelpers.h>
    #include <string.h>
    #include <DXGI.h>
    #include "SyUtilMisc.h"

    #define HT_NO_SUPPORT    0
    #define HT_HAVE          1
    #define HT_HAVENT        2
    #define HT_DISABLE       3
    #define HT_NO_DETECT     4
    #define SHIFT_SHORT     16
    #define SHIFT_LONG      24
    #define CPU_family_id          0x0F00
    #define CPU_pentium4_id        0x0F00
    #define CPU_family_ext_id      0x0F00000
    #define Log_Num_Bits           0x00FF0000
    #define Hyperthread_Bit_Mask   0x10000000
    #define Init_apic_id           0xFF000000

    unsigned int Hyberthread_Supported(void);
    unsigned char LogicalProc(void);
    unsigned char Acquire_APIC_ID(void);

    typedef
    enum {
        WINUNKNOW = 0, /*!< don't know OS */
        WIN31,      /*!< don't WINNT 3.1 OS */
        WINNT,      /*!< don't WINNT    OS */
        WIN95,      /*!< don't Window 95 OS */
        WIN98,      /*!< don't Window 98 OS */
        WINME,      /*!< don't Window ME 3.1 OS */
		WIN2000,    /*!< don't Window 2000 OS */
		WINXP,      /*!< don't Window XP OS */
		WIN2003,        /*!< don't Window 2003 OS */
		WINVista,   /*!< don't Window Vista OS */  //fixed bug #344127,#344793,#345305
		WIN7,
		WIN8,
		WIN81,
		WIN10,

		WINNew = 20     /* */
    } WIN_OS;
    
	typedef LONG NTSTATUS, *PNTSTATUS;
    #define STATUS_SUCCESS  ((NTSTATUS)(0x00000000))
    #define STATUS_ERROR    ((NTSTATUS)(0xC0000000))
    #define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004)

    SY_OS_EXPORT WIN_OS WbxGetOSVersion();
    bool WbxGetOSVersionNumber(uint32_t& uMajorVer, uint32_t& uMinorVer, uint32_t& uBuildVer);

    typedef
    enum {
        VME_PHYSIC = 0,     /*!< physics */
        VME_VMWARE,         /*!< VM of VMWARE */
        VME_CITRIX,         /*!< VM of Citrix */
        VME_VIRTUALBOX,     /*!< VM of VIRTUALBOX */
        VME_RDP,            /*!< use RDP connect remote computer */
        VME_UNKNOWN = 100    /* UNKNOW*/
    } VMEType;

    SY_OS_EXPORT VMEType GetVirtualEnvironmentType();
	SY_OS_EXPORT bool IsEnableDpiAwareness();
	SY_OS_EXPORT bool SetDpiAwareness(bool bAwareness);
    SY_OS_EXPORT int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList);

    inline const char* WbxOSVersion2String() {
        WIN_OS winOS = WbxGetOSVersion();
        switch (winOS) {
        case WIN31:
            return "WIN31";
        case WINNT:
            return "WINNT";
        case WIN95:
            return "WIN95";
        case WIN98:
            return "WIN98";
        case WINME:
            return "WINME";
        case WIN2000:
            return "WIN2000";
        case WINXP:
            return "WINXP";
        case WIN2003:
            return "WIN2003";
        case WINVista:
            return "WINVista";
        case WIN7:
            return "WIN7";
        case WIN8:
            return "WIN8";
        case WIN81:
            return "WIN81";
        case WIN10:
            return "WIN10";
        default:
            return "WINUNKNOW";
        }
        return "WINUNKNOW";
    }
#define IS_WINDOW_VISTA_LATER() (WbxGetOSVersion() >= WINVista)
#define IS_WINDOW_7_LATER() (WbxGetOSVersion() >= WIN7)
#define IS_WINDOW_8_LATER() (WbxGetOSVersion() >= WIN8)
#define IS_WINDOW_81_LATER() (WbxGetOSVersion() >= WIN81)
#define IS_WINDOW_10_LATER() (WbxGetOSVersion() >= WIN10)

#endif

#ifdef AT_ANDROID
    int get_cpu_core(unsigned int &configure_core_num, unsigned int &online_core_num);
#else   //#if defined AT_MAC || defined AT_WIN || defined AT_IOS
    SY_OS_EXPORT int get_cpu_core(unsigned int &physical_core_num, unsigned int &logical_core_num);
#endif

#ifdef AT_IOS
    int get_iphone_name(char buf[], size_t buf_size);
    float get_iphone_version();
    unsigned long long get_iphone_total_ram();

#endif

//memory_size = 1000 means 1000MBs
SY_OS_EXPORT int get_memory_size(uint64_t &memory_size);

//cpu_frequency=1000 means 1000Mhz
SY_OS_EXPORT int get_cpu_frequency(unsigned int &cpu_frequency);

SY_OS_EXPORT int get_cpu_brand(char *brand_str, int len);

SY_OS_EXPORT int get_cpu_arch(char *arch_str, int bufLen);

class IWMESystemResourceMonitor {
public:
    virtual uint32_t getCPUCores() = 0;
    virtual bool getTotalCPUUsage(uint32_t& nCPUUsage) = 0;
    virtual bool getProcessUsage(uint32_t& nCPUUsage) = 0;
    //virtual bool getProcessCPUUsage(uint32_t pid, uint32_t& nCPUUsage) = 0;
    //virtual bool getAllProcessCPUUsage(uint32_t& nCount, uint32_t* nCPUUsage) = 0;
    //virtual bool getAllThreadsCPUUsage(uint32_t pid,uint32_t& nCount, uint32_t* nCPUUsage) = 0;
    //virtual bool getTotalMemoryUsage(uint32_t& nMEMUsage) = 0;
};
SY_OS_EXPORT IWMESystemResourceMonitor * getWMESystemResourceMonitor();
SY_OS_EXPORT void releaseWMESystemResourceMonitor(IWMESystemResourceMonitor * pIWMESystemResourceMonitor);

#endif
