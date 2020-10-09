#include "SyConfigIO.h"
#include "SyAssert.h"
#include "SyString.h"

#if defined(SY_LINUX) || defined(SY_ANDROID)
    #include <sys/types.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

START_UTIL_NS
/**
 * This class SHOULD better be rewritten by CSyString instead of pure C. -- weichen2
 */

/////////////////////////////////////////////////////////////////////////////
// File based get_int_from_profile() and get_string_from_profile()
// implementation
//
char *cm_trim_string(char *str)
{
    SY_ASSERTE(str);

    // trim tail
    char *cur_str = str + strlen(str) - 1;
    while (cur_str >= str) {
        if (strchr(" \t\r\n", *cur_str)) {
            *cur_str = 0;
            cur_str--;
        } else {
            break;
        }
    }

    // trim head
    cur_str = str;
    while (*cur_str) {
        if (strchr(" \t\r\n", *cur_str)) {
            cur_str++;
        } else {
            break;
        }
    }
    return cur_str;
}

unsigned char cm_is_comment_line(char *cur_str)
{
    while (*cur_str) {
        if (strchr(" \t\r\n", *cur_str) != NULL) {
            cur_str++;
        } else {
            return *cur_str == '#';
        }
    }
    return FALSE;
}

unsigned char read_init_file(FILE *f, char *app_name, char *key_name,
                             char *ret_str, unsigned long len)
{
    char buf[2048];
    unsigned char find_section = FALSE;
    if (app_name == nullptr) {
        find_section = TRUE;
    }

    while (!feof(f)) {
        buf[0] = 0;
        if (!fgets(buf, sizeof(buf), f)) {
            break;
        }

        if (cm_is_comment_line(buf)) {
            continue;
        }

        if (strchr(buf, '[') && strchr(buf, ']')) {
            if (find_section) {
                // encounter another section
                return FALSE;
            } else {
                char *token = strchr(buf, '[');
                token++;
                char *p = strchr(buf, ']');
                if (p) {
                    *p = 0;
                }
                //*strchr(buf, ']') = 0;

                token = cm_trim_string(token);
#ifndef SY_WIN32
                if (strcasecmp(token, app_name) == 0) {
                    find_section = TRUE;
                }
#else //SY_WIN32
                if (_stricmp(token, app_name) == 0) {
                    find_section = TRUE;
                }

#endif //SY_WIN32
            }
        } else {
            if (find_section) {
                char *token = strchr(buf, '=');
                if (token) {
                    *token = 0;
                    token++;

                    char *pKey = cm_trim_string(buf);
#ifndef SY_WIN32
                    if (strcasecmp(pKey, key_name) == 0)
#else //SY_WIN32
                    if (_stricmp(pKey, key_name) == 0)
#endif //SY_WIN32
                    {
                        token = cm_trim_string(token);
                        if (*token == '\'' || *token == '"') {
                            int str_len = static_cast<int>(strlen(token));
                            if (token[str_len-1] == *token) {
                                token[str_len-1] = 0;
                                token++;
                                token = cm_trim_string(token);
                            }
                        }
                        strncpy(ret_str, token, len);
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

int get_int_from_profile(char *app_name, char *key_name,
                         int default_value, char *file_name)
{
    int i;

    FILE *f = fopen(file_name, "rt");
    if (!f) {
        return default_value;
    }

    //    SET_CLOSEONEXEC(fileno(f));

    char buf[256];
    if (read_init_file(f, app_name, key_name, buf, 256)) {
        fclose(f);
        sscanf(buf, "%d", &i);
        return i;
    }
    fclose(f);

    return default_value;
}

unsigned long get_string_from_profile(char *app_name, char *key_name,
                                      char *default_str, char *ret_str, unsigned long len, char *file_name)
{
    FILE *f = fopen(file_name, "rt");
    if (!f) {
        strncpy(ret_str, default_str, len - 1);
        return strlen(ret_str);
    }

    //    SET_CLOSEONEXEC(fileno(f));

    if (read_init_file(f, app_name, key_name, ret_str, len)) {
        fclose(f);
        return strlen(ret_str);
    }
    fclose(f);

    strncpy(ret_str, default_str, len - 1);
    return strlen(ret_str);
}


CSyString g_config_file_name = "sy.cfg";

void set_webex_config_file_name(const CSyString &file)
{
    g_config_file_name = file;
}


#include "SyDefaultDir.h"
CSyString g_home_dir_name;
bool m_bEnableConfigIOFeature = false;

const char *get_webex_home_dir()
{
    CSyDefaultDir cdir;
    g_home_dir_name = cdir.GetDefaultDir();

    return g_home_dir_name.c_str();
}

CSyString get_config_file_name()
{
#if defined (WP8 ) || defined (UWP)
    CSyString homeDir = "c:\\tmp";
#else
    CSyDefaultDir cdir;
    CSyString homeDir = cdir.GetDefaultDir();
#endif

#if defined(SY_LINUX_SERVER)
    homeDir += "/";
#else
    homeDir += "/conf/";
#endif
    homeDir += g_config_file_name;

    return homeDir;
}

void enable_config_io_feature(bool bEnable)
{
    m_bEnableConfigIOFeature = bEnable;
}

unsigned char get_string_param_from_configfile(
    const char *config_file_name,
    const char *group,
    const char *item_key,
    char *item_value,
    unsigned long len)
{
    if(!m_bEnableConfigIOFeature) {
        return FALSE;
    }

    return get_string_param_from_ini_force(config_file_name, group, item_key, item_value, len);
}

unsigned char get_string_param_from_ini_force(const char *config_file_name, const char *group,
                                               const char *item_key, char *item_value, unsigned long len)
{
    if (!item_value) {
        return FALSE;
    }

    item_value[0] = 0;

    get_string_from_profile(
        const_cast<char *>(group),
        const_cast<char *>(item_key),
        (char *)"",
        item_value,
        len,
        const_cast<char *>(config_file_name));

    int str_len = static_cast<int>(strlen(item_value));
    if (str_len > 0) {
        // for old INI format
        if (item_value[str_len - 1] == ';') {
            item_value[str_len - 1] = 0;
        }
    }

    return str_len > 0;
}
unsigned char get_string_param(const char *group, const char *item_key,
                               char *item_value, unsigned long len)
{
    CSyString config_file_name = get_config_file_name();
    return get_string_param_from_configfile(config_file_name.c_str(), group,item_key,item_value,len);
}

int get_int_param(char *group, char *item_key)
{
    char str_value[256];

    if (get_string_param(group, item_key, str_value, 256)) {
        int i;
        sscanf(str_value, "%d", &i);

        return i;
    }
    return 0;
}

unsigned short get_uint16_param(char *group, char *item_key)
{
    char str_value[256];

    if (get_string_param(group, item_key, str_value, 256)) {
        int i;
        sscanf(str_value, "%d", &i);

        return (unsigned short)i;
    }

    return 0;
}

unsigned long get_uint32_param(char *group, char *item_key)
{
    char str_value[256];

    if (get_string_param(group, item_key, str_value, 256)) {
        unsigned long i;
        sscanf(str_value, "%lu", &i);

        return i;
    }

    return 0;
}

unsigned char get_bool_param(char *group, char *item_key,
                             unsigned char default_value)
{
    char str_value[256];

    if (get_string_param(group, item_key, str_value, 256))
#ifndef SY_WIN32
        return strcasecmp(str_value, "TRUE") == 0;
#else //SY_WIN32
        return _stricmp(str_value, "TRUE") == 0;
#endif //SY_WIN32

    return default_value;
}

#ifdef SY_MAC
void mac_get_exec_name(char *cmd_name_buf)
{
    ProcessSerialNumber thePSN;
    ProcessInfoRec theInfo;
    OSErr   theErr;
#ifdef __LP64__
    FSRef theSpec;
#else
    FSSpec  theSpec;
#endif

    thePSN.highLongOfPSN = 0;
    thePSN.lowLongOfPSN = kCurrentProcess;

    theInfo.processInfoLength = sizeof(theInfo);
    theInfo.processName = NULL;
#ifdef __LP64__
    theInfo.processAppRef = &theSpec;

    /* Find the application FSSpec */
    theErr = GetProcessInformation(&thePSN, &theInfo);
    CFStringRef name = NULL;
    LSCopyDisplayNameForRef(&theSpec, &name);
    if (name) {
        CFIndex length = CFStringGetLength(name);
        CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
        CFStringGetCString(name, cmd_name_buf, maxSize, kCFStringEncodingUTF8);
        CFRelease(name);
    }
#else
    theInfo.processAppSpec = &theSpec;

    /* Find the application FSSpec */
    theErr = GetProcessInformation(&thePSN, &theInfo);
    memcpy(cmd_name_buf, theSpec.name+1,*theSpec.name);
#endif
}
#endif

/*
 * For compatibility with Solaris, provide get_exec_name(), which attempts to
 * return the name of the current executable by looking it up in /proc.
 */
const char *get_exec_name(void)
{
    const int SYDBUFLEN = 1024; /* PATH_MAX */
    static char cmd_name_buf[SYDBUFLEN + 1]= {0};
    static char *cmd_name = NULL;

    if (cmd_name) {
        return cmd_name;
    }

#if defined(SY_IOS) || defined(SY_WIN_PHONE)
    //iOS and wp8 doesn't support it.
    cmd_name = cmd_name_buf;
#elif defined(SY_MAC)
    mac_get_exec_name(cmd_name_buf);
#elif defined(SY_WIN32)
    ::GetModuleFileNameA(NULL, cmd_name_buf, sizeof(cmd_name_buf));
#else
    char pidfile[64];
    int bytes;
    int fd;

    sprintf(pidfile, "/proc/%d/cmdline", getpid());

    fd = open(pidfile, O_RDONLY, 0);
    bytes = read(fd, cmd_name_buf, 256);
    close(fd);

    cmd_name_buf[SYDBUFLEN] = '\0';
#endif
    cmd_name = cmd_name_buf;

    return (cmd_name);
}

const char *get_process_name()
{
    const char *pExecName = get_exec_name();

    if (pExecName && 0!=*pExecName) {
        const char *pch = pExecName + strlen(pExecName) - 1;
        while (pch > pExecName && *pch != '/' && *pch != '\\') {
            pch--;
        }

        if (*pch == '/' || *pch == '\\') {
            pch++;
        }

        return pch;
    } else {
        return "util";
    }
}


END_UTIL_NS
