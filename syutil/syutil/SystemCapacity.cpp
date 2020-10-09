#include <stdio.h>
#include <vector>
#include "SyDebug.h"
#include "SystemCapacity.h"
#if defined AT_WIN_DESKTOP
    #ifndef PSAPI_VERSION
        #define PSAPI_VERSION 1
    #endif
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
    #include <intrin.h>

#elif defined AT_WIN_PHONE
    using namespace Windows::System;

#elif defined AT_APPLE
    #include <unistd.h>
    #include <mach/mach.h>
    #include <sys/sysctl.h>
    #ifdef AT_IOS
    #include "SyDefaultDir.h"
    #endif

#elif defined AT_ANDROID || defined SY_LINUX_CLIENT
    #include <unistd.h>
    #include <stdlib.h>
    #include <fstream>

#endif

#ifdef AT_WIN_DESKTOP

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
bool GetRealOSVersion(RTL_OSVERSIONINFOW &rtlOsVerInfo) {
	HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
	if (hMod) {
		RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
		if (fxPtr != nullptr) {
			rtlOsVerInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
			if (STATUS_SUCCESS == fxPtr(&rtlOsVerInfo)) {
				return true;
			}
		}
	}
	return false;
}

WIN_OS WbxGetOSVersionInternal(uint32_t& uMajorVersion, uint32_t& uMinorVersion, uint32_t& uBuildNumber)
{
    WIN_OS sWinOS = WINUNKNOW;

	OSVERSIONINFOEX osvi;
	RTL_OSVERSIONINFOW rosvi;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	ZeroMemory(&rosvi, sizeof(RTL_OSVERSIONINFOW));
	rosvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi))) {
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO *)&osvi)) {
			return WINUNKNOW;
		}
	}
    uBuildNumber = osvi.dwBuildNumber;
    uMajorVersion = osvi.dwMajorVersion;
    uMinorVersion = osvi.dwMinorVersion;
	switch (osvi.dwPlatformId) {
		// Tests for Windows NT product family.
	case VER_PLATFORM_WIN32_NT:

		// Test for the product.
		if (osvi.dwMajorVersion <= 4) {
			sWinOS = WINNT;
			return WINNT;
		}

		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
			sWinOS = WIN2000;
			return WIN2000;
		}

		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
			sWinOS = WINXP;
			return WINXP;
		}

		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2) {
			sWinOS = WIN2003;
			return WIN2003;
		}

		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) {
			sWinOS = WINVista;
			return WINVista;
		}

		//fixed bug #344127,#344793,#345305
		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
			sWinOS = WIN7;
			return WIN7;
		}

		if (GetRealOSVersion(rosvi)) {
			osvi.dwMajorVersion = rosvi.dwMajorVersion;
			osvi.dwMinorVersion = rosvi.dwMinorVersion;
            uBuildNumber = rosvi.dwBuildNumber;
		}

        uMajorVersion = osvi.dwMajorVersion;
        uMinorVersion = osvi.dwMinorVersion;
            
		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
			sWinOS = WIN8;
			return WIN8;
		}

		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
			sWinOS = WIN81;
			return WIN81;
		}

		if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
			sWinOS = WIN10;
			return WIN10;
		}

		if (osvi.dwMajorVersion >= 6) {
			sWinOS = WINNew;
			return WINNew;  //20090217
		}

		break;

		// Test for the Windows 95 product family.
	case VER_PLATFORM_WIN32_WINDOWS:

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
			sWinOS = WIN95;
			return WIN95;
		}

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
			sWinOS = WIN98;
			return WIN98;
		}

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
			sWinOS = WINME;
			return WINME;
		}
		break;
	}
	return WINUNKNOW;

}

WIN_OS WbxGetOSVersion()
{
    static WIN_OS sWinOS = WINUNKNOW;
    
    if (sWinOS != WINUNKNOW) {
        return sWinOS;
    }
    uint32_t uMajorVersion = 0, uMinorVersion = 0, uPatchVersion = 0;
    sWinOS = WbxGetOSVersionInternal(uMajorVersion, uMinorVersion, uPatchVersion);
    return sWinOS;
}

bool WbxGetOSVersionNumber(uint32_t& uMajorVer,uint32_t& uMinorVer,uint32_t& uBuildVer)
{
    static uint32_t sMajorVersion = 0,sMinorVersion = 0, sBuildVer = 0;
    
    if (sMajorVersion == 0) {
        WbxGetOSVersionInternal(sMajorVersion, sMinorVersion, sBuildVer);
    }
    uMajorVer = sMajorVersion;
    uMinorVer = sMinorVersion;
    uBuildVer = sBuildVer;
    return true;
}

typedef BOOL(WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}

// return 0 if success, return non-0 if fail
int physical_cpu_core(uint32_t& physical_core_number, uint32_t& logical_core_number)
{
    LPFN_GLPI glpi;
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

    glpi = (LPFN_GLPI)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");
    if (NULL == glpi)
    {
        return (1);
    }

    while (!done)
    {
        DWORD rc = glpi(buffer, &returnLength);

        if (FALSE == rc)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (buffer)
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    returnLength);

                if (NULL == buffer)
                {
                    return (2);
                }
            }
            else
            {
                return (3);
            }
        }
        else
        {
            done = TRUE;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
        switch (ptr->Relationship)
        {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
            Cache = &ptr->Cache;
            if (Cache->Level == 1)
            {
                processorL1CacheCount++;
            }
            else if (Cache->Level == 2)
            {
                processorL2CacheCount++;
            }
            else if (Cache->Level == 3)
            {
                processorL3CacheCount++;
            }
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
            processorPackageCount++;
            break;

        default:
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free(buffer);
    physical_core_number = processorCoreCount;
    logical_core_number = logicalProcessorCount;
    return 0;
}

bool GetFirstActiveDisplayAdapter(DISPLAY_DEVICEA* pDisplayDevice)
{
    SecureZeroMemory(pDisplayDevice, sizeof(DISPLAY_DEVICEA));
    pDisplayDevice->cb = sizeof(DISPLAY_DEVICEA);

    int i = 0;
    while (EnumDisplayDevicesA(NULL, i, pDisplayDevice, 0))
    {
        if (pDisplayDevice->StateFlags & DISPLAY_DEVICE_ACTIVE)
        {
            return true;
        }
        i++;
    }

    return false;
}

VMEType GetVirtualEnvironmentType() {
    DISPLAY_DEVICEA adaptor;

    VMEType eVMEType = VME_PHYSIC;
    if (!GetFirstActiveDisplayAdapter(&adaptor))
    {
        return VME_UNKNOWN; // for safety, we consider unknown case as virtual env so that we would keep use dshow first.
    }
    SY_INFO_TRACE("GetVirtualEnvironmentType DeviceString: " << adaptor.DeviceString);
    strtouppercase_s(adaptor.DeviceString, sizeof(adaptor.DeviceString));
    char* sub = NULL;
    if ((strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "CITRIX", 6, &sub) == EOK && sub)) {
        eVMEType = VME_CITRIX;
    }
    else if (strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "VMWARE", 6, &sub) == EOK && sub) {
        eVMEType = VME_VMWARE;
    }
    else if (strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "VIRTUALBOX", 10, &sub) == EOK && sub) {
        eVMEType = VME_VIRTUALBOX;
    }
    else if ((strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "RDP", 3, &sub) == EOK && sub) ||
        (strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "MICROSOFT REMOTE DISPLAY ADAPTER", 32, &sub) == EOK && sub)) {
        eVMEType = VME_RDP;
    }
    else if (strstr_s(adaptor.DeviceString, sizeof(adaptor.DeviceString), "GRID", 4, &sub) == EOK && sub) {
        eVMEType = VME_UNKNOWN;
    }
    SY_INFO_TRACE("GetVirtualEnvironmentType eVM_TYPE=" << eVMEType);
    return eVMEType;
}

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef HRESULT(WINAPI *FN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI *FN_GetProcessDpiAwareness)(HANDLE, PROCESS_DPI_AWARENESS *);
typedef BOOL(WINAPI *FN_SetProcessDPIAware)();
typedef BOOL(WINAPI *FN_IsProcessDPIAware)();
#include <tchar.h>
bool IsEnableDpiAwareness()
{
	bool bAwareness = true;
	if (IS_WINDOW_81_LATER()) {
		char szSystemPath[MAX_PATH] = { 0 };
		GetSystemDirectoryA(szSystemPath, MAX_PATH);
		std::string szShcorePathfile = szSystemPath;
		szShcorePathfile += "\\Shcore.DLL";
		HMODULE hShcore = LoadLibraryA(szShcorePathfile.c_str());
		if (hShcore) {
			auto *pGetProcessDpiAwareness = (FN_GetProcessDpiAwareness)GetProcAddress(hShcore, "GetProcessDpiAwareness");
			if (pGetProcessDpiAwareness) {
				PROCESS_DPI_AWARENESS ePROCESS_DPI_AWARENESS;
				if (pGetProcessDpiAwareness(::GetCurrentProcess(), &ePROCESS_DPI_AWARENESS) == S_OK) {
					bAwareness = (ePROCESS_DPI_AWARENESS != PROCESS_DPI_UNAWARE);
				}
			}
			FreeLibrary(hShcore);
		}
	}
	else {
		FN_IsProcessDPIAware pIsDPIAware = NULL;

		HMODULE hMod = GetModuleHandle(_T("User32.dll"));
		if (hMod != NULL)
		{
            pIsDPIAware = (FN_IsProcessDPIAware)GetProcAddress(hMod, "IsProcessDPIAware");
			if (pIsDPIAware != NULL)
			{
				bAwareness = (pIsDPIAware() == TRUE);
			}
		}
	}
	return bAwareness;
}

bool SetDpiAwareness(bool bAwareness) {
	bool ret = false;
	if (IS_WINDOW_81_LATER()) {
		char szSystemPath[MAX_PATH] = { 0 };
		GetSystemDirectoryA(szSystemPath, MAX_PATH);
		std::string szShcorePathfile = szSystemPath;
		szShcorePathfile += "\\Shcore.DLL";
		HMODULE hShcore = LoadLibraryA(szShcorePathfile.c_str());
		if (hShcore) {
			auto *pGetProcessDpiAwareness = (FN_GetProcessDpiAwareness)GetProcAddress(hShcore, "GetProcessDpiAwareness");
			if (pGetProcessDpiAwareness) {
				PROCESS_DPI_AWARENESS ePROCESS_DPI_AWARENESS;
				if (pGetProcessDpiAwareness(::GetCurrentProcess(), &ePROCESS_DPI_AWARENESS) == S_OK) {
					if (bAwareness != (ePROCESS_DPI_AWARENESS != PROCESS_DPI_UNAWARE)) {
						auto *pSetProcessDpiAwareness = (FN_SetProcessDpiAwareness)GetProcAddress(hShcore, "SetProcessDpiAwareness");
						if (pSetProcessDpiAwareness) {
							auto rv = pSetProcessDpiAwareness(bAwareness ? PROCESS_PER_MONITOR_DPI_AWARE : PROCESS_DPI_UNAWARE);
							ret = (rv == S_OK);
						}
					}
					else {
						ret = true;
					}
				}
			}
			FreeLibrary(hShcore);
		}
	}
	else if (bAwareness){
		FN_IsProcessDPIAware pIsDPIAware = NULL;
		FN_SetProcessDPIAware pSetDPIAware = NULL;

		HMODULE hMod = GetModuleHandle(_T("User32.dll"));
		if (hMod != NULL)
		{
			pIsDPIAware = (FN_IsProcessDPIAware)GetProcAddress(hMod, "IsProcessDPIAware");
			if (pIsDPIAware != NULL)
			{
				if (pIsDPIAware() == FALSE) {
					pSetDPIAware = (FN_SetProcessDPIAware)GetProcAddress(hMod, "SetProcessDPIAware");
					if (pSetDPIAware != NULL)
					{
						auto rv = pSetDPIAware();
						ret = (rv != 0);
					}
				}
				else
					ret = true;
			}
		}
	}
    return ret;
}
#endif//#ifdef AT_WIN_DESKTOP

#if defined AT_WIN_DESKTOP
int GetMemoryUsageWindows(uint64_t &memory_used, uint64_t &memory_size)
{
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);

    bool result = GlobalMemoryStatusEx(&statex);
    if (!result) {
        return -1;
    }

    memory_size = (uint64_t)(statex.ullTotalPhys>>10);
    memory_used = (uint64_t)((statex.ullTotalPhys-statex.ullAvailPhys)>>10);
    return 0;
}

int GetProcessMemoryWindows(uint64_t &memory_used)
{
    PROCESS_MEMORY_COUNTERS memCounter;
    bool result = GetProcessMemoryInfo(GetCurrentProcess(),
                                       &memCounter,
                                       sizeof(memCounter));
    if (!result) {
        return -1;
    }
    
    memory_used = memCounter.WorkingSetSize>>10;
    return 0;
}
#elif defined AT_WIN_PHONE
int GetMemoryUsageWinPhone(unsigned int &memory_used, unsigned int &memory_size)
{
    memory_used = (unsigned int)(Windows::System::MemoryManager::AppMemoryUsage>>10);
    memory_size = (unsigned int)(Windows::System::MemoryManager::AppMemoryUsageLimit>>10);
    return 0;
}

#elif defined AT_APPLE
#if defined AT_MAC64
int GetMemoryUsageMac64(uint64_t &memory_used, uint64_t &memory_size)
{
    uint64_t memUsed = 0;
    uint64_t memTotal = 0;
    vm_statistics64         meminfo;
    unsigned long pagesize = (size_t)sysconf( _SC_PAGESIZE );
    kern_return_t           error;
    mach_msg_type_number_t  count;

    count = HOST_VM_INFO64_COUNT;
    error = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&meminfo, &count);
    if (error != KERN_SUCCESS) {
        return -1;
    }

    memUsed = (meminfo.active_count + meminfo.inactive_count + meminfo.speculative_count + meminfo.wire_count + meminfo.compressor_page_count - meminfo.purgeable_count - meminfo.external_page_count) * pagesize;
    memTotal = (meminfo.active_count + meminfo.inactive_count + meminfo.speculative_count + meminfo.wire_count + meminfo.compressor_page_count + meminfo.free_count) * pagesize;
    memory_used = (unsigned int)(memUsed>>10);
    memory_size = (unsigned int)(memTotal>>10);
    return 0;
}
int GetProcessMemoryMac64(uint64_t &memory_used)
{
    struct task_basic_info_64	info;
    unsigned long pagesize = (size_t)sysconf( _SC_PAGESIZE );
    mach_msg_type_number_t count = TASK_BASIC_INFO_64_COUNT;
    kern_return_t error = task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&info, &count);
    if (error != KERN_SUCCESS) {
        return -1;
    }
    memory_used = (uint64_t)(info.resident_size>>10);
    return 0;
}
#elif defined AT_MAC32
int GetMemoryUsageMac32(uint64_t &memory_used, uint64_t &memory_size)
{
    uint64_t memUsed = 0;
    uint64_t memTotal = 0;
    vm_statistics           meminfo;
    vm_size_t               pagesize;
    kern_return_t           error;
    mach_msg_type_number_t  count;

    host_page_size(mach_host_self(), &pagesize);
    count = HOST_VM_INFO_COUNT;
    error = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&meminfo, &count);
    if (error != KERN_SUCCESS) {
        return -1;
    }
    memUsed = (meminfo.active_count + meminfo.inactive_count + meminfo.wire_count)*pagesize;
    memTotal = memUsed + meminfo.free_count * pagesize;
    memory_used = (uint64_t)(memUsed>>10);
    memory_size = (uint64_t)(memTotal>>10);
    return 0;
}
int GetProcessMemoryMac32(uint64_t &memory_used)
{
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return -1;    /* Can't access? */
    }
    memory_used = (uint64_t)(info.resident_size>>10);
    return 0;
}
#elif defined AT_IOS
int GetMemoryUsageIOS(uint64_t &memory_used, uint64_t &memory_size)
{
    uint64_t memUsed = 0;
    uint64_t memTotal = 0;
    vm_statistics           meminfo;
    vm_size_t               pagesize;
    kern_return_t           error;
    mach_msg_type_number_t  count;
    
    host_page_size(mach_host_self(), &pagesize);
    count = HOST_VM_INFO_COUNT;
    error = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&meminfo, &count);
    if (error != KERN_SUCCESS) {
        return -1;
    }
    memUsed = (meminfo.active_count + meminfo.inactive_count + meminfo.wire_count)*pagesize;
    memTotal = get_iphone_total_ram();
    memory_used = (uint64_t)(memUsed>>10);
    memory_size = (uint64_t)(memTotal>>10);
    return 0;
}
int GetProcessMemoryIOS(uint64_t &memory_used)
{
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return -1;    /* Can't access? */
    }
    memory_used = (uint64_t)(info.resident_size>>10);
    return 0;
}
#endif
#elif defined AT_ANDROID || defined SY_LINUX_CLIENT
int GetMemoryUsageAndroid(uint64_t &memory_used, uint64_t &memory_size)
{
    std::string token;
    std::ifstream file("/proc/meminfo");
    uint64_t memory_free = 0;
    unsigned short result = 0;
    while (file >> token) {
        if (token == "MemTotal:") {
            file >> memory_size;
            result |= 0x01;
        } else if (token == "MemFree:") {
            file >> memory_free;
            result |= 0x02;
        }
        if ((result ^ 0x03) == 0) {
            break;
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    memory_used = memory_size-memory_free;
    return 0;
}
int GetProcessMemoryAndroid(uint64_t &memory_used)
{
    std::string token;
    std::ifstream file("/proc/self/status");
    while (file >> token) {
        if (token == "VmRSS:") {
            file >> memory_used;
            break;
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0;
}

#endif
int get_memory_usage(float &memory_usage, uint64_t &memory_used, uint64_t &memory_size)
{
    memory_used = 0;
    memory_size = 0;
    memory_usage = 0.0f;
    int ret = 0;
#if defined AT_WIN_DESKTOP
    ret = GetMemoryUsageWindows(memory_used, memory_size);
#elif defined AT_WIN_PHONE
    ret = GetMemoryUsageWinPhone(memory_used, memory_size);
#elif defined AT_APPLE
#if defined AT_MAC64
    ret = GetMemoryUsageMac64(memory_used, memory_size);
#elif defined AT_MAC32
    ret = GetMemoryUsageMac32(memory_used, memory_size);
#elif defined AT_IOS
    ret = GetMemoryUsageIOS(memory_used, memory_size);
#endif
#elif defined AT_ANDROID || defined SY_LINUX_CLIENT
    ret = GetMemoryUsageAndroid(memory_used, memory_size);
#endif
    if (ret != 0) {
        //get memory usage failed
        return ret;
    }
    if (memory_size == 0) {
        memory_usage = 0;
    } else {
        memory_usage = 100.0*memory_used/memory_size;
    }
    return 0;
}

int get_process_memory(uint64_t &memory_used)
{
    memory_used = 0;
#if defined AT_WIN_DESKTOP
    return GetProcessMemoryWindows(memory_used);
#elif defined AT_WIN_PHONE
    SY_DEBUG_TRACE("get_process_memory not implemented in wp8");
    return -1;
#elif defined AT_APPLE
#if defined AT_MAC64
    return GetProcessMemoryMac64(memory_used);
#elif defined AT_MAC32
    return GetProcessMemoryMac32(memory_used);
#elif defined AT_IOS
    return GetProcessMemoryIOS(memory_used);
#endif
#elif defined AT_ANDROID || defined SY_LINUX_CLIENT
    return GetProcessMemoryAndroid(memory_used);
#endif
    SY_DEBUG_TRACE("get_process_memory not implemented");
    return -1;
}

#if defined AT_APPLE

static struct host_cpu_load_info    hostinfo_prev;

int get_cpu_usage(float &cpu_usage)
{
    kern_return_t           error;
    mach_msg_type_number_t  count;
    host_cpu_load_info  hostinfo;

    count = HOST_CPU_LOAD_INFO_COUNT;

    error = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&hostinfo, &count);
    if (error != KERN_SUCCESS) {
        return 1;
    }
    float used = (hostinfo.cpu_ticks[CPU_STATE_USER]+hostinfo.cpu_ticks[CPU_STATE_SYSTEM]) - (hostinfo_prev.cpu_ticks[CPU_STATE_USER]+hostinfo_prev.cpu_ticks[CPU_STATE_SYSTEM]);
    float total = used + (hostinfo.cpu_ticks[CPU_STATE_IDLE]-hostinfo_prev.cpu_ticks[CPU_STATE_IDLE]);
    float usage = 0;
    if (total != 0) {
        usage = 100.0*used/total;
    }

    cpu_usage = usage;

    hostinfo_prev = hostinfo;

    return 0;
}

#endif


#ifdef AT_WIN_DESKTOP

unsigned int Hyberthread_Supported(void)
{
    int Regs0[4] = { 0 }, Regs1[4] = { 0 };

    __try {
        __cpuid(Regs0, 0);
        __cpuid(Regs1, 1);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return (0);
    }

    if (((Regs1[0] & CPU_family_id) == CPU_pentium4_id) ||
            (Regs1[0] & CPU_family_ext_id)) {
        if (Regs0[1] == 'uneG') {
            if (Regs0[3] == 'Ieni') {
                if (Regs0[2] == 'letn') {
                    return (Regs1[3] & Hyperthread_Bit_Mask);
                }
            }
        }
    }

    return 0;
}

unsigned char LogicalProc(void)
{
    if (!Hyberthread_Supported()) {
        return (unsigned char)1;
    }

    int Regs0[4] = { 0 };
    __cpuid(Regs0, 1);
    return (unsigned char)((Regs0[1] & Log_Num_Bits) >> SHIFT_SHORT);
}

unsigned char Acquire_APIC_ID(void)
{
    if (!Hyberthread_Supported()) {
        return (unsigned char)-1;
    }

    int Regs0[4] = { 0 };
    __cpuid(Regs0, 1);

    return (unsigned char)((Regs0[1] & Init_apic_id) >> SHIFT_LONG);
}

#endif

#if defined(SY_ANDROID) || defined(SY_LINUX)
int get_cpu_core(unsigned int &configure_core_num, unsigned int &online_core_num)
{
    configure_core_num = sysconf(_SC_NPROCESSORS_CONF);
    online_core_num = sysconf(_SC_NPROCESSORS_ONLN);
    return 0;
}
#else   //#if defined AT_MAC || defined AT_IOS || defined AT_WIN
int get_cpu_core(unsigned int &physical_core_num, unsigned int &logical_core_num)
{
#if defined AT_MAC || defined AT_IOS
    kern_return_t           error;
    mach_msg_type_number_t  count = HOST_BASIC_INFO_COUNT;
    host_basic_info         basicInfo;

    error = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&basicInfo, &count);
    if (error != KERN_SUCCESS) {
        return 1;
    }
    physical_core_num = basicInfo.physical_cpu;
    if (0 != physical_core_num) {
        logical_core_num = basicInfo.logical_cpu/physical_core_num;
    }

#endif

#ifdef AT_WIN

    unsigned char bFlag = 0;
    unsigned char i, phy_Mask, Phy_Shift;
    unsigned char cpu_apic_id, log_core_id, phy_core_id;

    physical_core_num = 2;
    logical_core_num = 1;

    SYSTEM_INFO system_info = {0};
#ifdef AT_WIN_DESKTOP

    if (!physical_cpu_core(physical_core_num, logical_core_num)) {
        // added call to physical_cpu_core, if succeed, done. if fail, use legacy code.
        // reason of adding this new code is: legacy code can't differentiate physical/logical core well.
        logical_core_num /= physical_core_num;
        return 0;
    }

    GetSystemInfo(&system_info);
#else
    GetNativeSystemInfo(&system_info);
#endif

    physical_core_num = (unsigned char)system_info.dwNumberOfProcessors;
#ifdef AT_WIN_DESKTOP
    if (Hyberthread_Supported()) {
        BOOL Ht_Activate = FALSE;
        logical_core_num = LogicalProc();
        if (logical_core_num >= 1) {
            HANDLE hCpuHandle;
            DWORD_PTR  dwCpuAffinity;
            DWORD_PTR  dwSysAffinity;
            DWORD_PTR  dwAffinityMask;

            i = 1;
            phy_Mask = 0xFF; Phy_Shift = 0;

            for (i = 1; i < logical_core_num; i *= 2) {
                phy_Mask <<= 1;
                Phy_Shift++;
            }

            hCpuHandle = GetCurrentProcess();
            GetProcessAffinityMask(hCpuHandle, &dwCpuAffinity, &dwSysAffinity);

            if (dwCpuAffinity != dwSysAffinity) {
                bFlag = HT_NO_DETECT;
                physical_core_num = (unsigned char)-1;
                return bFlag;
            }
            dwAffinityMask = 1;

            for (dwAffinityMask = 1; (dwAffinityMask != 0 && dwAffinityMask <= dwCpuAffinity); dwAffinityMask <<= 1) {
                if (dwAffinityMask & dwCpuAffinity) {
                    if (SetProcessAffinityMask(hCpuHandle, dwAffinityMask)) {
                        Sleep(0);
                        cpu_apic_id = Acquire_APIC_ID();
                        log_core_id = cpu_apic_id & ~phy_Mask;
                        phy_core_id = cpu_apic_id >> Phy_Shift;

                        if (log_core_id != 0) {
                            Ht_Activate = TRUE;
                        }
                    }
                }
            }

            SetProcessAffinityMask(hCpuHandle, dwCpuAffinity);
            if (logical_core_num == 1) {
                bFlag = HT_HAVENT;
            } else {
                if (Ht_Activate) {
                    if (logical_core_num != 0) {
                        physical_core_num /= (logical_core_num);
                        bFlag = HT_HAVE;
                    }
                } else {
                    bFlag = HT_DISABLE;
                }
            }
        }
    } else {
        bFlag = HT_NO_SUPPORT;
        logical_core_num = 1;
    }
#endif
    return bFlag;
#endif
    return 0;
}
#endif

int get_memory_size(uint64_t &memory_size)
{
#if defined(SY_LINUX)
    uint64_t mem_size;
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    mem_size = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGESIZE);
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
    mem_size = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGE_SIZE);
#endif
    memory_size = (uint64_t)(mem_size/(1.0e6));
#endif

#if defined(SY_ANDROID)
    float memory_usage = 0;
    uint64_t m_used = 0;
    uint64_t m_size = 0;
    get_memory_usage(memory_usage, m_used, m_size);
    memory_size = m_size /1000;
    SY_INFO_TRACE("SystemCapacity.cpp::_line_632_::get_memory_size, memory size = " << memory_size <<" MB");
    if(0 == memory_size) {
    	memory_size = 2000;
    }
#endif

#if defined AT_MAC || AT_IOS
    size_t length=0;
    int mib[2];
    uint64_t memory_size_byte=0;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(memory_size_byte);
    sysctl(mib, 2, &memory_size_byte, &length, NULL, 0);
    memory_size = (uint64_t)(memory_size_byte/(1.0e6));/*(1e+6);*/
#endif

#if defined(SY_WIN_DESKTOP) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
    /* Cygwin under Windows. ------------------------------------ */
    /* New 64-bit MEMORYSTATUSEX isn't available.  Use old 32.bit */
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    memory_size = (uint64_t)(status.dwTotalPhys/(1.0e6));
#elif defined(SY_WIN_DESKTOP)
    /* Windows. ------------------------------------------------- */
    /* Use new 64-bit MEMORYSTATUSEX, not old 32-bit MEMORYSTATUS */
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    memory_size = (uint64_t)(status.ullTotalPhys/(1.0e6));
#elif defined AT_WIN_PHONE
    memory_size = (uint64_t)(Windows::System::MemoryManager::AppMemoryUsageLimit/(1.0e6));
#endif
    return 0;
}

int get_cpu_brand(char *brand_str, int len)
{
#if defined(SY_MAC) || defined(SY_IOS)
    size_t buflen = len;
    sysctlbyname("machdep.cpu.brand_string", brand_str, &buflen, NULL, 0);

    return 0;
#elif SY_ANDROID
    //This need to be get in Java layer
#elif SY_WIN_DESKTOP
    char CPUBrandString[0x40];
    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i;
    int nCacheLineSize = 0;
    int nL2Associativity = 0;
    int nCacheSizeK = 0;
    BOOL error_code = 0;
    char *lpCPUBrandStr = NULL;

    __cpuid(CPUInfo, 0x80000000);

    nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));


    for (i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);


        if (i == 0x80000002) {
            memcpy_s(CPUBrandString, sizeof(CPUBrandString), CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000003) {
            memcpy_s(CPUBrandString + 16, sizeof(CPUBrandString) - 16, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000004) {
            memcpy_s(CPUBrandString + 32, sizeof(CPUBrandString) - 32, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000006) {
            nCacheLineSize = CPUInfo[2] & 0xff;
            nL2Associativity = (CPUInfo[2] >> 12) & 0xf;
            nCacheSizeK = (CPUInfo[2] >> 16) & 0xffff;
        }
    }

    SY_INFO_TRACE("get_cpu_brand = " << CPUBrandString);
    strcpy_s(brand_str, len, CPUBrandString);

    return 0;
#elif  SY_LINUX
    FILE *ptr_file;
    char *ptrBuffer;
    size_t bufLen = 1024;
    int nread;

    ptrBuffer = (char*) calloc(sizeof(char), bufLen);
    ptr_file = popen("lscpu", "r");
    if (ptr_file && ptrBuffer) {
        while((nread = getline(&ptrBuffer, &bufLen, ptr_file)) != -1) {
            //SY_INFO_TRACE(" branchName - " << "get frequency with line " << ptrBuffer);

            // Example output from lscpu:
            // Model name:            Intel(R) Xeon(R) W-2123 CPU @ 3.60GHz
            char *sub = nullptr;
            if (strstr_s(ptrBuffer, nread, "Model name:", 11, &sub) == EOK && sub) {
                char *ptr = sub;
                // move pointer after ':'
                while (ptr[0] != ':') ptr++;
                ptr++;
                // remove space
                while (ptr[0] == ' ') ptr++;

                strcpy_s(brand_str, len, ptr);
                SY_INFO_TRACE("get_cpu_brand = " << brand_str);
                break;
            }
        }
    }

    if (ptrBuffer) {
        free(ptrBuffer);
    }
    if (ptr_file) {
        fclose(ptr_file);
    }
    return 0;
#endif

    return -1;
}

int get_cpu_frequency(unsigned int &cpu_frequency)
{
#ifdef AT_MAC
    size_t length=0;
    int mib[2];
    unsigned int cpu_frequency_hz=0;

    mib[0] = CTL_HW;
    mib[1] = HW_CPU_FREQ;
    length = sizeof(cpu_frequency_hz);
    sysctl(mib, 2, &cpu_frequency_hz, &length, NULL, 0);
    cpu_frequency = (unsigned int)(cpu_frequency_hz/(1.0e6));/*(1e+6);*/
#elif AT_IOS
    //this logic doesn't work
    size_t length=0;
    int mib[2];
    unsigned long long cpu_frequency_hz=0;

    mib[0] = CTL_HW;
    mib[1] = HW_CPU_FREQ;
    length = sizeof(cpu_frequency_hz);
    sysctl(mib, 2, &cpu_frequency_hz, &length, NULL, 0);
    /*
     if(-1 == re) {
     printf("error=%d\n", errno);
     }*/
    cpu_frequency = (unsigned int)(cpu_frequency_hz/(1.0e6));/*(1e+6);*/
#elif AT_ANDROID
    char buf[1000] = {0};
    FILE *ptr_file;
    ptr_file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq","r");
    if (!ptr_file) {
        return -1;
    }
    fgets(buf, 1000, ptr_file);
    fclose(ptr_file);
    cpu_frequency = atoi(buf);
    cpu_frequency /= 1.0e3;

#elif AT_WIN_DESKTOP
#define _MAX_LENGTH 260
    unsigned long BufSize = _MAX_LENGTH;
    unsigned long dwMHz = _MAX_LENGTH;
    HKEY hKey;

    long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                               0, KEY_READ, &hKey);

    if (lError != ERROR_SUCCESS) {
        return -2;
    }

    // query the key:
    RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE)&dwMHz, &BufSize);
    RegCloseKey(hKey);

    cpu_frequency = (unsigned int)dwMHz;

#elif SY_LINUX
    FILE *ptr_file;
    char *ptrBuffer;
    size_t len = 1024;
    int nread;

    ptrBuffer = (char*) calloc(sizeof(char), len);
    ptr_file = popen("lscpu", "r");
    if (ptr_file && ptrBuffer) {
        while((nread = getline(&ptrBuffer, &len, ptr_file)) != -1) {
            //SY_INFO_TRACE(" branchName - " << "get frequency with line " << ptrBuffer);

            // Example output from lscpu:
            // CPU max MHz:           3900.0000
            char *sub = nullptr;
            if (strstr_s(ptrBuffer, nread, "CPU max MHz:", 12, &sub) == EOK && sub) {
                char *ptr = sub;
                // move pointer after ':'
                while (ptr[0] != ':') ptr++;
                ptr++;
                // remove space
                while (ptr[0] == ' ') ptr++;

                std::string frequencyStr(ptr);
                cpu_frequency = (unsigned int) std::stof(frequencyStr);
                break;
            }
        }
    }

    if (ptrBuffer) {
        free(ptrBuffer);
    }
    if (ptr_file) {
        fclose(ptr_file);
    }
#endif

    return 0;
}

#ifdef AT_MAC
int get_cpu_arch(char *arch_str, int bufLen)
{
    char machbuf[32];
    size_t len = (bufLen>32)? 32:bufLen;
    int oid[] = { CTL_HW, HW_MACHINE};
    int error =  sysctl(oid, 2, machbuf, &len, NULL, 0);
    if (error) {
        SY_ERROR_TRACE("get_cpu_arch failed");
        return -1;
    }
    //SY_INFO_TRACE("get_cpu_arch: " << machbuf);
    strcpy_s(arch_str, len, machbuf);
    return 0;
}
#elif AT_IOS
int get_cpu_arch(char *arch_str, int bufLen)
{
    SY_ERROR_TRACE("get_cpu_arch no implemented");
    return -1;
}
#elif AT_WIN_DESKTOP
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
int get_cpu_arch(char *arch_str, int bufLen)
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
                                            GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process){
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)){
            SY_ERROR_TRACE("get_cpu_arch failed");
            return -1;
        }
    }
    if (bIsWow64) {
        strcpy_s(arch_str, bufLen, "x86_64");
    }
    else
        strcpy_s(arch_str, bufLen, "x86_32");
    return 0;
}
#elif AT_ANDROID
int get_cpu_arch(char *arch_str, int bufLen)
{
    SY_ERROR_TRACE("get_cpu_arch no implemented");
    return -1;
}
#elif SY_LINUX_CLIENT
int get_cpu_arch(char *arch_str, int bufLen)
{
    std::string token;
    std::ifstream file("/proc/cpuinfo");
    bool bHaslm = false;

    while(file >> token) {
        if (token == "flags") {
            char name[2048];
            file.getline(name, 2048);
            std::string strFlags = name;
            bHaslm = (strFlags.find(" lm ") != std::string::npos);
            //SY_INFO_TRACE("find = " << bHaslm <<", branchName = " << strFlags.c_str());
            break;
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    if (bHaslm) {
        strcpy_s(arch_str, bufLen, "x86_64");
    }
    else {
        strcpy_s(arch_str, bufLen, "x86_32");
    }
    return 0;
}
#else
int get_cpu_arch(char *arch_str, int bufLen)
{
    SY_ERROR_TRACE("get_cpu_arch no implemented");
    return -1;
}
#endif

#ifdef AT_APPLE
#ifdef AT_MAC
// call getGpuInfo in SyUtilMisc.mm
#else
int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList)
{
    SY_ERROR_TRACE("getGpuInfo: not implemented");
    return -1;
}
#endif
#elif AT_WIN_DESKTOP
std::string WStringToString(const std::wstring &wstr)
{
    std::string str(wstr.length(), ' ');
    std::copy(wstr.begin(), wstr.end(), str.begin());
    return str;
}

int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList)
{
    IDXGIFactory * pFactory;
    IDXGIAdapter * pAdapter;
    std::vector <IDXGIAdapter*> vAdapters;// Graphic Card
    int iAdapterNum = 0;

    std::string dllName = "DXGI.dll";
    WME_HMODULE m_hModule = wme_load_library(dllName.c_str(), dllName.size());
    if (!m_hModule) {
        return 1;
    }
    using PFN_CreateDXGIFactory1 = HRESULT(WINAPI *)(REFIID riid, _Out_ void **ppFactory);
    PFN_CreateDXGIFactory1 m_pfnCreateDXGIFactory1 = nullptr;
    m_pfnCreateDXGIFactory1 = (PFN_CreateDXGIFactory1) wme_get_proc_address(m_hModule, "CreateDXGIFactory1");
    // Create DXGI
    HRESULT hr = m_pfnCreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)(&pFactory));

    if (FAILED(hr))
        return -1;

    // Enum all the adapters
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    SY_INFO_TRACE("getGpuInfo: total Adapter number = " << iAdapterNum);
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        // Query
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);
        std::wstring aa(adapterDesc.Description);
        std::string name = WStringToString(aa);

        unsigned int systemMemory = adapterDesc.DedicatedSystemMemory / 1024 / 1024;
        unsigned int videoMemory = adapterDesc.DedicatedVideoMemory / 1024 / 1024;
        unsigned int sharedMemory = adapterDesc.SharedSystemMemory / 1024 / 1024;

        SY_INFO_TRACE("getGpuInfo: System Memory in MB " << systemMemory);
        SY_INFO_TRACE("getGpuInfo: Video Memory in MB " << videoMemory);
        SY_INFO_TRACE("getGpuInfo: Shared Memory in MB " << sharedMemory);
        SY_INFO_TRACE("getGpuInfo: Device Description : " << name.c_str());

        gpuList.push_back(name);
        if (systemMemory > 0) memoryList.push_back(systemMemory);
        else if (videoMemory > 0) memoryList.push_back(videoMemory);
        else memoryList.push_back(sharedMemory);
    }
    vAdapters.clear();

    if (m_hModule) {
        wme_free_library(m_hModule);
        m_hModule = nullptr;
    }

    return 0;
}
#elif AT_ANDROID
int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList)
{
    SY_ERROR_TRACE("getGpuInfo: not implemented");
    return -1;
}
#elif SY_LINUX
int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList)
{
    FILE *ptr_file;
    char *ptrBuffer;
    size_t len = 1024;
    int nread;

    // So far, do not know how to use lspci to get the multiple Graphic Card memory.
    // Only query the first GPU
    ptrBuffer = (char*) calloc(sizeof(char), len);
    ptr_file = popen("lshw -C Video", "r");
    if (ptr_file && ptrBuffer) {
        while((nread = getline(&ptrBuffer, &len, ptr_file)) != -1) {
            //SY_INFO_TRACE(" getGpuInfo - " << "get frequency with line " << ptrBuffer);

            // Example output from lscpu:
            //   *-display
            //       description: VGA compatible controller
            //       product: GP107GL [Quadro P620]
            //       vendor: NVIDIA Corporation
            //       clock: 33MHz
            char *subStr = nullptr;
            if (strstr_s(ptrBuffer, nread, "product:", 8, &subStr) == EOK && subStr) {
                char *ptr = subStr;
                // move pointer after ':'
                while (ptr[0] != ':') ptr++;
                ptr++;
                // remove space
                while (ptr[0] == ' ') ptr++;

                std::string gpuName(ptr);
                gpuList.push_back(gpuName);
                SY_INFO_TRACE("getGpuInfo name =" << gpuName.c_str());
                break;
            }
        }
    }
    if (ptr_file) {
        fclose(ptr_file);
        ptr_file = nullptr;
    }

    len = 1024;
    ptr_file = popen("lspci -v -s $(lspci | grep VGA | cut -d\" \" -f 1) | grep \" prefetchable\"", "r");
    if (ptr_file && ptrBuffer) {
        while ((nread = getline(&ptrBuffer, &len, ptr_file)) != -1) {
            // Example output:
            //  Memory at c0000000 (64-bit, prefetchable) [size=256M]
            //  Memory at d0000000 (64-bit, prefetchable) [size=32M]
            char *sizeStr;
            if (strstr_s(ptrBuffer, nread, "size=", 5, &sizeStr) == EOK && sizeStr) {
                char *ptr = sizeStr;
                // move pointer after '='
                while (ptr[0] != '=') ptr++;
                ptr++;
                // remove space
                while (ptr[0] == ' ') ptr++;

                std::string data(ptr);
                unsigned int memorySize = 0;
                int index = (int)data.size();
                while (ptr[0]>='0' && ptr[0]<='9' && index >= 0) {
                    memorySize = memorySize * 10 + (ptr[0] - '0');
                    index --;
                    ptr++;
                }
                memoryList.push_back(memorySize);
                SY_INFO_TRACE("getGpuInfo memory size = " << memorySize);
                break;
            }
        }
    }

    if (ptrBuffer) {
        free(ptrBuffer);
    }
    if (ptr_file) {
        fclose(ptr_file);
    }

    return 0;
}
#else
int getGpuInfo(std::vector<std::string> &gpuList, std::vector<unsigned int> &memoryList)
{
    SY_ERROR_TRACE("getGpuInfo: not implemented");
    return -1;
}
#endif

#ifdef AT_IOS
int get_iphone_name(char buf[], size_t buf_size)
{
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    if (buf_size < size) {
        memset(buf, 0, buf_size);
        return -1;
    }
    sysctlbyname("hw.machine", buf, &size, NULL, 0);

    /*
     if ([platform isEqualToString:@"iPhone1,1"])    return @"iPhone 1G";
     if ([platform isEqualToString:@"iPhone1,2"])    return @"iPhone 3G";
     if ([platform isEqualToString:@"iPhone2,1"])    return @"iPhone 3GS";
     if ([platform isEqualToString:@"iPhone3,1"])    return @"iPhone 4 (GSM)";
     if ([platform isEqualToString:@"iPhone3,3"])    return @"iPhone 4 (CDMA)";
     if ([platform isEqualToString:@"iPhone4,1"])    return @"iPhone 4S";
     if ([platform isEqualToString:@"iPhone5,1"])    return @"iPhone 5 (GSM)";
     if ([platform isEqualToString:@"iPhone5,2"])    return @"iPhone 5 (GSM+CDMA)";
     if ([platform isEqualToString:@"iPhone5,3"])    return @"iPhone 5c (GSM)";
     if ([platform isEqualToString:@"iPhone5,4"])    return @"iPhone 5c (GSM+CDMA)";
     if ([platform isEqualToString:@"iPhone6,1"])    return @"iPhone 5s (GSM)";
     if ([platform isEqualToString:@"iPhone6,2"])    return @"iPhone 5s (GSM+CDMA)";
     if ([platform isEqualToString:@"iPhone7,1"])    return @"iPhone 6 Plus";
     if ([platform isEqualToString:@"iPhone7,2"])    return @"iPhone 6";
     if ([platform isEqualToString:@"iPhone8,1"])    return @"iPhone 6s";
     if ([platform isEqualToString:@"iPhone8,2"])    return @"iPhone 6s Plus";
     if ([platform isEqualToString:@"iPod1,1"])      return @"iPod Touch 1G";
     if ([platform isEqualToString:@"iPod2,1"])      return @"iPod Touch 2G";
     if ([platform isEqualToString:@"iPod3,1"])      return @"iPod Touch 3G";
     if ([platform isEqualToString:@"iPod4,1"])      return @"iPod Touch 4G";
     if ([platform isEqualToString:@"iPod5,1"])      return @"iPod Touch 5G";
     if ([platform isEqualToString:@"iPad1,1"])      return @"iPad";
     if ([platform isEqualToString:@"iPad2,1"])      return @"iPad 2 (WiFi)";
     if ([platform isEqualToString:@"iPad2,2"])      return @"iPad 2 (GSM)";
     if ([platform isEqualToString:@"iPad2,3"])      return @"iPad 2 (CDMA)";
     if ([platform isEqualToString:@"iPad2,4"])      return @"iPad 2 (WiFi)";
     if ([platform isEqualToString:@"iPad2,5"])      return @"iPad Mini (WiFi)";
     if ([platform isEqualToString:@"iPad2,6"])      return @"iPad Mini (GSM)";
     if ([platform isEqualToString:@"iPad2,7"])      return @"iPad Mini (GSM+CDMA)";
     if ([platform isEqualToString:@"iPad3,1"])      return @"iPad 3 (WiFi)";
     if ([platform isEqualToString:@"iPad3,2"])      return @"iPad 3 (GSM+CDMA)";
     if ([platform isEqualToString:@"iPad3,3"])      return @"iPad 3 (GSM)";
     if ([platform isEqualToString:@"iPad3,4"])      return @"iPad 4 (WiFi)";
     if ([platform isEqualToString:@"iPad3,5"])      return @"iPad 4 (GSM)";
     if ([platform isEqualToString:@"iPad3,6"])      return @"iPad 4 (GSM+CDMA)";
     if ([platform isEqualToString:@"iPad4,1"])      return @"iPad Air (WiFi)";
     if ([platform isEqualToString:@"iPad4,2"])      return @"iPad Air (GSM)";
     if ([platform isEqualToString:@"iPad4,4"])      return @"iPad Mini Retina (WiFi)";
     if ([platform isEqualToString:@"iPad4,5"])      return @"iPad Mini Retina (GSM)";
     if ([platform isEqualToString:@"iPad6,7"])      return @"iPad Pro (WiFi)";
     if ([platform isEqualToString:@"iPad6,8"])      return @"iPad Pro Cellular";
     if ([platform isEqualToString:@"i386"])         return @"Simulator";
     if ([platform isEqualToString:@"x86_64"])       return @"Simulator";
     */

    return 0;
}
#endif

#ifdef _UNIT_TEST
#ifdef AT_IOS
    int test_main()
#else
    int main(int argc, const char *argv[])
#endif
{
    // CPU core num, memery size
    printf("CPU core num and memery size was tested on WINDOWS/MAC/IOS/ADNROID:\n");
    unsigned int pnum = 0, lnum = 0;
    uint64_t memory_size = 0;
    unsigned int cpu_frequency=0;


    get_cpu_core(pnum, lnum);
    get_memory_size(memory_size);
    get_cpu_frequency(cpu_frequency);

#ifdef AT_ANDROID
    printf("configure core num =%d\n", pnum);
    printf("online core num =%d\n", lnum);
#else
    printf("physical core num =%d\n", pnum);
    printf("logical core num =%d\n", lnum);
#endif

    printf("memory size=%lld\n", memory_size);
    printf("cpu frequency =%u\n\n", cpu_frequency);

#ifdef AT_IOS
#define IPHONE_NAME_SIZE 50
    char buffer[IPHONE_NAME_SIZE] = {0};
    int re = get_iphone_name(buffer, IPHONE_NAME_SIZE);
    if (-1 == re) {
        return re;
    }
    printf("iphone name = %s\n", buffer);
#endif

    //--------------------------------------------
#if defined AT_MAC || defined AT_IOS
    printf("\nMetrics below only tested on MAC/IOS:\n");
    //cpu frequency, cpu usage, memory usage
    float cpu_usage=0;
    get_cpu_usage(cpu_usage);
    printf("cpu usage =%.1f%%\n", cpu_usage);

#endif
    float memory_usage=0;
    uint64_t memory_used=0;
    get_memory_usage(memory_usage, memory_used, memory_size);
    printf("memory usage=%.1f%%, memory used=%u, memory size=%lld\n", memory_usage, memory_used, memory_size);

#ifdef AT_WIN
    getchar();
#endif
    return 0;
}
#endif

#include <mutex>
#include <thread>
#include "timer.h"
class WMESystemResourceMonitor : public IWMESystemResourceMonitor {
public:
#if defined(AT_WIN)
    typedef struct {
        FILETIME time;

        FILETIME idleTime;
        FILETIME kernelTime;
        FILETIME userTime;

        FILETIME createTime;
        FILETIME exitTime;
    } CPUTimes;
#endif
    static WMESystemResourceMonitor &Instance() {
        return m_sWMESystemResourceMonitor;
    }
    virtual uint32_t getCPUCores() {
        return m_nCPUCores;
    }
    virtual bool getTotalCPUUsage(uint32_t& nCPUUsage) {
        refresh();
        nCPUUsage = m_nCPUUsageTotal;
        return  true;
    }
    virtual bool getProcessUsage(uint32_t& nCPUUsage) {
        refresh();
        nCPUUsage = m_nCPUUsageCurProcess;
        return  true;
    }
    
    void refresh() {
        std::lock_guard<std::recursive_mutex> g(m_mutex);
        uint64_t tick_now = low_ticker::now();
        if ((tick_now - m_tick) < 1000 * 1000) { //1s
            return;
        }
        refresh_total_cpu();
        refresh_cur_process_cpu();
        m_tick = tick_now;
    }
protected:
    WMESystemResourceMonitor() {
        m_nCPUCores = std::thread::hardware_concurrency();
    }
    ~WMESystemResourceMonitor() {}
    void refresh_total_cpu() {
#if defined(AT_WIN)
        CPUTimes cpuTimes;
        GetSystemTimeAsFileTime(&cpuTimes.time);
        GetSystemTimes(&cpuTimes.idleTime, &cpuTimes.kernelTime, &cpuTimes.userTime);
        if (m_tick == 0) {//first time
            m_lastCPUTimes = cpuTimes;
            return;
        }
        m_nCPUUsageTotal = ComputeCpuUsage(cpuTimes, m_lastCPUTimes);
        m_lastCPUTimes = cpuTimes;
#elif defined(AT_MAC)
        kern_return_t            error;
        mach_msg_type_number_t    count;
        host_cpu_load_info    hostinfo;
        
        count = HOST_CPU_LOAD_INFO_COUNT;
        
        error = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&hostinfo, &count);
        if(error != KERN_SUCCESS) {
            return ;
        }
        if (m_tick == 0) {//first time
            m_lastCPULoadInfo = hostinfo;
            return;
        }
        float used = (hostinfo.cpu_ticks[CPU_STATE_USER]+hostinfo.cpu_ticks[CPU_STATE_SYSTEM]) - (m_lastCPULoadInfo.cpu_ticks[CPU_STATE_USER]+m_lastCPULoadInfo.cpu_ticks[CPU_STATE_SYSTEM]);
        float total = used + (hostinfo.cpu_ticks[CPU_STATE_IDLE]-m_lastCPULoadInfo.cpu_ticks[CPU_STATE_IDLE]);
        uint32_t usage = 0;
        if(total != 0)
            usage = 100.0 * used/total;
        
        m_nCPUUsageTotal = usage;
        m_lastCPULoadInfo = hostinfo;
#endif
    }
    void refresh_cur_process_cpu() {
#if defined(AT_WIN)
        CPUTimes cpuTimes;
        GetSystemTimeAsFileTime(&cpuTimes.time);
        GetProcessTimes(GetCurrentProcess(), &cpuTimes.createTime, &cpuTimes.exitTime, &cpuTimes.kernelTime, &cpuTimes.userTime);
        if (m_tick == 0) {//first time
            m_lastProcessCPUTimes = cpuTimes;
            return;
        }
        m_nCPUUsageCurProcess = ComputeProcessUsage(cpuTimes, m_lastProcessCPUTimes);
        m_lastProcessCPUTimes = cpuTimes;
#elif defined(AT_MAC)
        task_thread_times_info info;
        mach_msg_type_number_t count = TASK_THREAD_TIMES_INFO_COUNT;
        kern_return_t error = task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t)&info, &count);
        if (error != KERN_SUCCESS) {
            return;
        }
        
        struct task_basic_info_64    task_basic_info;
        mach_msg_type_number_t task_basic_count = TASK_BASIC_INFO_64_COUNT;
        error = task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&task_basic_info, &task_basic_count);
        if (error != KERN_SUCCESS) {
            return;
        }
        uint64_t time_now = timer_policy::now()/1000;
        uint64_t task_time_now = (info.system_time.seconds + info.user_time.seconds + task_basic_info.system_time.seconds + task_basic_info.user_time.seconds) * 1000
        + (info.system_time.microseconds + info.user_time.microseconds + task_basic_info.system_time.microseconds + task_basic_info.user_time.microseconds)/1000;
        
        if (m_tick == 0) {//first time
            m_lastTime = time_now;
            m_lastTaskTime = task_time_now;
            return;
        }
        int64_t delta_time = time_now - m_lastTime;
        int64_t delta_task = task_time_now - m_lastTaskTime;
        if (delta_time > 0 &&  delta_task > 0) {
            m_nCPUUsageCurProcess = 100.0*delta_task/delta_time;
        }
        m_lastTime = time_now;
        m_lastTaskTime = task_time_now;
#endif
    }

#if defined(AT_WIN)

    LONGLONG FileTimeDiff(FILETIME time1, FILETIME time2)
    {
        // https://msdn.microsoft.com/en-us/9baf8a0e-59e3-4fbd-9616-2ec9161520d1
        // Do not cast a pointer to a FILETIME structure to either a ULARGE_INTEGER* or __int64* value because it can cause alignment faults on 64-bit Windows.

        //LONGLONG diffInTicks = ((LARGE_INTEGER *)(&time1))->QuadPart - ((LARGE_INTEGER *)(&time2))->QuadPart;
        //LONGLONG diffInMillis = diffInTicks / 10000;
        //return diffInMillis;
        ULONGLONG t1 = (ULONGLONG(time1.dwHighDateTime) << 32) | time1.dwLowDateTime;
        ULONGLONG t2 = (ULONGLONG(time2.dwHighDateTime) << 32) | time2.dwLowDateTime;
        return t1 - t2;
    }

    int ComputeCpuUsage(CPUTimes &newTimes, CPUTimes &oldTimes)
    {
        LONGLONG usr = FileTimeDiff(newTimes.userTime, oldTimes.userTime);
        LONGLONG ker = FileTimeDiff(newTimes.kernelTime, oldTimes.kernelTime);
        LONGLONG idl = FileTimeDiff(newTimes.idleTime, oldTimes.idleTime);

        LONGLONG total = (usr + ker);
        if (total <= 0) {
            return 1;
        }

        if (idl < 0) {
            idl = 0;
        }
        else if (idl > total) {
            idl = total;
        }

        return int((total - idl) * 100 / total);
    }

    int ComputeProcessUsage(CPUTimes &newTimes, CPUTimes &oldTimes)
    {
        LONGLONG user = FileTimeDiff(newTimes.userTime, oldTimes.userTime);
        LONGLONG kernel = FileTimeDiff(newTimes.kernelTime, oldTimes.kernelTime);
        LONGLONG div = FileTimeDiff(newTimes.time, oldTimes.time);
        if (div == 0 || m_nCPUCores == 0) {
            return 1;
        }
        int nUsage = int(((user + kernel) * 100.0) / (div*m_nCPUCores));
        nUsage = (std::min)(100, nUsage);
        nUsage = (std::max)(0, nUsage);
        return nUsage;
    }
#endif
protected:
#if defined(AT_WIN)
    CPUTimes m_lastCPUTimes;
    CPUTimes m_lastProcessCPUTimes;
#elif defined(AT_MAC)
    host_cpu_load_info m_lastCPULoadInfo = {0};
    uint64_t m_lastTaskTime = 0; //ms
    uint64_t m_lastTime = 0; //ms
#endif
    uint64_t m_tick = 0;
    std::recursive_mutex m_mutex;
    uint32_t m_nCPUUsageTotal = 0;
    uint32_t m_nCPUUsageCurProcess = 0;
    uint32_t m_nCPUCores = 0;
    /*
     struct process_cpu {
     uint32_t m_pid;
     float m_fCPUUage;
     };
     float m_vPorcessCPUUsage[10] = {0};
     */
    
    static WMESystemResourceMonitor m_sWMESystemResourceMonitor;
};
WMESystemResourceMonitor WMESystemResourceMonitor::m_sWMESystemResourceMonitor;

IWMESystemResourceMonitor * getWMESystemResourceMonitor() {
    WMESystemResourceMonitor::Instance().refresh();
    return &WMESystemResourceMonitor::Instance();
}

void releaseWMESystemResourceMonitor(IWMESystemResourceMonitor * pIWMESystemResourceMonitor) {
    
}
