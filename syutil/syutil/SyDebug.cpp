#include "SyDebug.h"
#include "SyConfigIO.h"
#include "timer.h"
#include <algorithm>
#include "SyMutex.h"

START_UTIL_NS

pfn_trace g_util_trace_sink = NULL;
pfn_trace g_util_logstash_sink = NULL;
SYTraceLevelDef g_traceMask = SY_TRACE_LEVEL_INFO;
SYTraceLevelDef g_traceMask_TroubleShootConfig = SY_TRACE_LEVEL_ERROR;
int32_t m_cpuLoadTroubleshootingFlag = -1;

long g_cm_assertion_count = 0;
long g_cm_temp_logs_count = 0;

#if defined(SY_IOS) || defined(WP8) || defined(UWP)
int g_trace_option = SyTrace_UserDefined | SyTrace_DefaultFile;
#elif defined(SY_LINUX_CLIENT)
int g_trace_option = SyTrace_UserDefined;
#elif defined(SY_LINUX_SERVER)
int g_trace_option = SyTrace_UserDefined | SyTrace_DefaultFile | SyTrace_LogStash;
#else
int g_trace_option = SyTrace_UserDefined | SyTrace_Wbxtrace;
#endif

long cm_get_temp_logs_count() {
    return g_cm_temp_logs_count;
}

void cm_add_temp_log() {
    g_cm_temp_logs_count++;
}

void set_util_external_trace_sink(pfn_trace p_sink)
{
    g_util_trace_sink = p_sink;
}

void set_util_logstash_external_trace_sink(pfn_trace p_sink)
{
    g_util_logstash_sink = p_sink;
}

void set_external_trace_mask(SYTraceLevelDef uLevel)
{
    g_traceMask = uLevel;
}

int32_t get_external_trace_mask()
{
    return (std::max)(g_traceMask, g_traceMask_TroubleShootConfig);
}

// API methods
unsigned char is_module_trace_enabled(unsigned long dwMask,const char *szModuleName)
{
    // todo
    return true;
}

void cm_set_trace_option(int option) {
    g_trace_option = option;
}

void util_adapter_trace(unsigned long trace_level, const char *module, const char *trace_info, unsigned int len)
{
    pfn_trace trace_sink = g_util_trace_sink;

    int trace_option = g_trace_option;

    char ch_buffer[T120TRACE_MAX_TRACE_LEN + 200];
    int trace_len = T120TRACE_MAX_TRACE_LEN + 200;
    char * pbuf = ch_buffer;
    
    if (len > T120TRACE_MAX_TRACE_LEN) {
        trace_len = len + 200;
        pbuf = new char[trace_len];
    }
    
    CSyTextFormator formator(pbuf, trace_len);
    char* trace_format = formator << "[" << (module ? module : "UTIL") << "] " << trace_info;

    if (NULL != trace_sink && (trace_option & SyTrace_UserDefined)) {
        (*trace_sink)(trace_level, (char *)trace_format, formator.tell());
    } else {
        const char *szModuleName = module;
        std::string sModuleName;
        if (module == NULL || module[0] == 0) {
            // todo
            //sModuleName = getCallerModuleName(trace_info);
            szModuleName = sModuleName.c_str();
        }

        if (trace_option & SyTrace_Wbxtrace) {
#ifdef SY_ANDROID
            const int nWbxTraceMaxLen = 900;
#else
            const int nWbxTraceMaxLen = 2047;
#endif
            unsigned int itLen = len;
            while (itLen > nWbxTraceMaxLen) {
                //CSyWbxTrace::instance()->trace_string(trace_level, szModuleName, (char *)trace_info + len - itLen);
                itLen -= (nWbxTraceMaxLen);
            }
            if (itLen > 0) {
                //CSyWbxTrace::instance()->trace_string(trace_level, szModuleName, (char *)trace_info + len - itLen);
            }
        }

        if (trace_option & SyTrace_DefaultFile) {
            //(CSyT120Trace::instance())->trace_string(trace_level, szModuleName, (char *)trace_info);
        }

        pfn_trace logstash_sink = g_util_logstash_sink;

        if ((trace_option & SyTrace_LogStash) && logstash_sink) {
            logstash_sink(trace_level, (char *)trace_format, formator.tell());
        }
    }
    
    if(pbuf != ch_buffer)
        delete[] pbuf;
}

//add for QoSVirtualization 9/6/2012 by juntang
char *g_LeadingId = NULL;

static CSyString get_host_id()
{
    char str_value[256] = {0};

    get_string_param((char *)"Trace", (char *)"MachineID", str_value, 256);

    return CSyString(str_value);
}

static void init_leading_id()
{
    CSyString conf_host_id = get_host_id();

    char szBuf[512];
    int nErr = ::gethostname(szBuf, sizeof(szBuf));
    if (nErr != 0) {
#ifdef SY_WIN32
        errno = ::WSAGetLastError();
#endif // SY_WIN32
        SY_WARNING_TRACE("CSyDnsManager::GetLocalIps, gethostname() failed! err=" << errno);
    } else if (conf_host_id == "") {
        conf_host_id = szBuf;
    }

    CSyString tmp = get_process_name();
    size_t pos = tmp.rfind(".exe");
    if (pos != CSyString::npos) {
        if (pos == tmp.length()-4) {
            tmp = tmp.substr(0, tmp.length()-4);
        }
    }
    tmp += ".";
    tmp += conf_host_id;
    tmp += ".";

    static char buf[1024];
    memset(buf, 0, sizeof(buf));
    strncpy(buf,tmp.c_str(),sizeof(buf) - 1);

    g_LeadingId = buf;
}

SY_OS_EXPORT char *get_leading_id()
{
    if (g_LeadingId) {
        return g_LeadingId;
    }

    static CSyMutexThreadRecursive mutex;
    CSyMutexGuardT<CSyMutexThreadRecursive> theLock(mutex);

    if (g_LeadingId == NULL) {
        init_leading_id();
    }

    return g_LeadingId;
}

SY_OS_EXPORT void cm_assertion_report() {
    g_cm_assertion_count++;
}

SY_OS_EXPORT long cm_get_assertions_count() {
    return g_cm_assertion_count;
}

///////////////////////////////////////////////////////////CSyUtilFuncTracer
CSyUtilFuncTracer::CSyUtilFuncTracer(const char *mould, const char *str)
{
    pmould = mould;
    buffer = str;
    m_tick = uint32_t(low_ticker::now() / 1000);
    SY_INFO_TRACE_EX(pmould, "WMEAPI Enter " << ((char *) buffer));
}

CSyUtilFuncTracer::~CSyUtilFuncTracer()
{
    uint32_t diff = uint32_t(low_ticker::now() / 1000) - m_tick;
    SY_INFO_TRACE_EX(pmould, "WMEAPI Leave diff=" << diff << " " << ((char *) buffer));
}


END_UTIL_NS
