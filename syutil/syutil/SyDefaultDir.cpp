#include "SyDefaultDir.h"
#include "SyDebug.h"
#include "SyUtilMisc.h"
#ifdef SY_WIN32
    #include "SyAssert.h"
#endif

#if defined (WP8 )
    #include "wp8helper.h"
#elif defined (UWP)
    #include "uwphelper.h"
#endif


USE_UTIL_NS

char g_cur_env[512] = "HOME";

void set_webex_home_env(char *home_env)
{
    strcpy_forsafe(g_cur_env, home_env, strlen(home_env), sizeof(g_cur_env));
}


CSyDefaultDir::CSyDefaultDir()
{
}

CSyDefaultDir::~CSyDefaultDir()
{
}

CSyString CSyDefaultDir::GetDefaultDir()
{
#ifdef SY_ANDROID
    return "/sdcard";
#elif defined(WP8) || defined(UWP)
    Windows::Storage::StorageFolder ^LocalFolderPath = ApplicationData::Current->LocalFolder;
    Platform::String ^TraceFilePath = Platform::String::Concat(LocalFolderPath->Path, "\\Shared\\");
    std::wstring ws(TraceFilePath->Data());
    std::string dirStr(ws.begin(), ws.end());

    return dirStr;
#elif defined(SY_WIN32)
    std::string tempPath;
    tempPath.resize(MAX_PATH);
    GetModuleFileNameA(NULL, (char *)tempPath.c_str(), tempPath.size());
    size_t found = tempPath.rfind('\\');
    if (found != std::string::npos) {
        tempPath.replace(found, std::string::npos, "\\");
    }
    return tempPath;
#elif defined(SY_LINUX_SERVER)
    return ".";
#else
    char *p = getenv(g_cur_env);
    if (p) {
        return CSyString(p);
    }

    return "/tmp";
#endif  //
}
