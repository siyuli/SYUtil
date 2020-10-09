#ifndef SYDEBUG_H
#define SYDEBUG_H

#include "SyDefines.h"
#include "SyTextFormator.h"

START_UTIL_NS

/// Enum of trace level
typedef enum {
    SY_TRACE_LEVEL_NOTRACE = -1,
    SY_TRACE_LEVEL_ERROR = 0,
    SY_TRACE_LEVEL_WARNING,
    SY_TRACE_LEVEL_INFO,
    SY_TRACE_LEVEL_DEBUG,
    SY_TRACE_LEVEL_DETAIL,
    SY_TRACE_LEVEL_ALL
} SYTraceLevelDef;

typedef enum {
    SyTrace_None = 0,
    SyTrace_Wbxtrace = 1,
    SyTrace_DefaultFile = 2,
    SyTrace_UserDefined = 4,
    SyTrace_LogStash = 8
} SyTraceType;

const uint32_t T120TRACE_MAX_TRACE_LEN = 1024;
const uint32_t T120TRACE_MAX_MODULE_NAME = 32;

typedef void (*pfn_trace)(unsigned long trace_level, char *trace_info, unsigned int len);
/*! \brief A function pointer of UTIL Trace Sink. The upper layer can set a pfn_trace pointer
*  to UTIL module. UTIL will check the sink before print trace, if the sink is NULL use SY_T120TRACE
*  to print trace, and if the sink is not NULL, use the sink to print trace.
*   This method can be called multi times.
*/
extern "C" SY_OS_EXPORT void set_external_trace_mask(SYTraceLevelDef uLevel);
extern "C" SY_OS_EXPORT int32_t get_external_trace_mask();
extern "C" SY_OS_EXPORT unsigned char is_module_trace_enabled(unsigned long dwMask,const char *szModuleName);
extern "C" SY_OS_EXPORT void set_util_external_trace_sink(pfn_trace pfn);
extern "C" SY_OS_EXPORT void set_util_logstash_external_trace_sink(pfn_trace pfn);
extern "C" SY_OS_EXPORT void util_adapter_trace(unsigned long trace_level, const char *module, const char *trace_info, unsigned int len);
//add following for QoSVirtualization 9/6/2012 by juntang
extern "C" SY_OS_EXPORT char *get_leading_id();
/// option is combination (OR) of SyTraceType
extern "C" SY_OS_EXPORT void sy_set_trace_option(int option);
extern "C" SY_OS_EXPORT long sy_get_temp_logs_count();
extern "C" SY_OS_EXPORT void sy_add_temp_log();

extern pfn_trace g_util_trace_sink;

#define UTIL_ADAPTER_TRACE(mask, level, str) UNIFIED_TRACE_LEVEL(mask, level, NULL, str, util_adapter_trace, T120TRACE_MAX_TRACE_LEN)
#define UTIL_ADAPTER_TRACE_EX(mask, level, module, str) UNIFIED_TRACE_LEVEL(mask, level, module, str, util_adapter_trace, T120TRACE_MAX_TRACE_LEN)

//#define FILE_LINE   "[" << __FILE__ << ":" << __LINE__ << "]"
#define UTIL_RETURN_CODE(x)           "ret: " << x
#define UTIL_ERROR_CODE(x)            "err: " << x

#define UNIFIED_TRACE_LEVEL(mask, level, module, str, Func_, len) \
    do {    \
        if(mask <= get_external_trace_mask()) {\
            char ch_buffer[len];\
            CSyTextFormator formator(ch_buffer, len);\
            char* trace_info = formator << str; \
            Func_(mask, module, trace_info, formator.tell()); \
        }\
    } while(0)

#define UNIFIED_TRACE(mask, module, str, Func_) UNIFIED_TRACE_LEVEL(mask, "", module, str, Func_, T120TRACE_MAX_TRACE_LEN)

#define SY_ERROR_TRACE(str)      UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_ERROR, "ERROR: ", str)
#define SY_WARNING_TRACE(str)    UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_WARNING, "WARNING: ", str)
#define SY_INFO_TRACE(str)       UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_INFO, "INFO: ", str)
#define SY_STATE_TRACE(str)      UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_DEBUG, "STATE: ", str)
#define SY_DEBUG_TRACE(str)      UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", str)
#define SY_STATISTIC_TRACE(str)  UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_DEBUG, "STATISTIC: ", str)
#define SY_DETAIL_TRACE(str)     UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", str)
#define SY_TEMP_TRACE(str)       UTIL_ADAPTER_TRACE(SY_TRACE_LEVEL_INFO, "INFO: ", str);sy_add_temp_log();

#define SY_ERROR_TRACE_EX(module,str)      UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_ERROR, "ERROR: ", module, str)
#define SY_WARNING_TRACE_EX(module,str)    UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_WARNING, "WARNING: ", module, str)
#define SY_INFO_TRACE_EX(module,str)       UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_INFO, "INFO: ", module,str)
#define SY_STATE_TRACE_EX(module,str)      UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_DEBUG, "STATE: ", module,str)
#define SY_DEBUG_TRACE_EX(module,str)      UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", module,str)
#define SY_STATISTIC_TRACE_EX(module,str)  UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_DEBUG, "STATISTIC: ", module,str)
#define SY_DETAIL_TRACE_EX(module,str)     UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", module,str)
#define SY_TEMP_TRACE_EX(module,str)       UTIL_ADAPTER_TRACE_EX(SY_TRACE_LEVEL_INFO, "INFO: ", module,str);sy_add_temp_log();

#define SY_INFO_TRACE_BIG_EX(module, str)   UNIFIED_TRACE_LEVEL(SY_TRACE_LEVEL_INFO, "INFO: ", module, str, util_adapter_trace, 8192)


#define SY_ERROR_TRACE_THIS(str)    SY_ERROR_TRACE(str << " this=" << this)
#define SY_WARNING_TRACE_THIS(str)  SY_WARNING_TRACE(str << " this=" << this)
#define SY_INFO_TRACE_THIS(str)     SY_INFO_TRACE(str << " this=" << this)
#define SY_STATE_TRACE_THIS(str)    SY_STATE_TRACE(str << " this=" << this)
#define SY_DEBUG_TRACE_THIS(str)    SY_DEBUG_TRACE(str << " this=" << this)
#define SY_STATISTIC_TRACE_THIS(str)  SY_STATISTIC_TRACE(str << " this=" << this)
#define SY_DETAIL_TRACE_THIS(str)     SY_DETAIL_TRACE(str << " this=" << this)
#define SY_TEMP_TRACE_THIS(str)     SY_TEMP_TRACE(str << " this=" << this)

#define SY_ERROR_TRACE_THIS_EX(module,str)      SY_ERROR_TRACE_EX(module,str << " this=" << this)
#define SY_WARNING_TRACE_THIS_EX(module,str)    SY_WARNING_TRACE_EX(module,str << " this=" << this)
#define SY_INFO_TRACE_THIS_EX(module,str)       SY_INFO_TRACE_EX(module,str << " this=" << this)
#define SY_STATE_TRACE_THIS_EX(module,str)      SY_STATE_TRACE_EX(module,str << " this=" << this)
#define SY_DETAIL_TRACE_THIS_EX(module,str)     SY_DETAIL_TRACE_EX(module,str << " this=" << this)
#define SY_DEBUG_TRACE_THIS_EX(module,str)      SY_DEBUG_TRACE_EX(module,str << " this=" << this)
#define SY_STATISTIC_TRACE_THIS_EX(module,str)  SY_STATISTIC_TRACE_EX(module,str << " this=" << this)
#define SY_TEMP_TRACE_THIS_EX(module,str)       SY_TEMP_TRACE_EX(module,str << " this=" << this)

#define SY_API_TRACE(module,str)  \
    char ch_buffer[T120TRACE_MAX_TRACE_LEN];\
    CSyTextFormator formator(ch_buffer, T120TRACE_MAX_TRACE_LEN);\
    char* trace_info = formator << str; \
    CSyUtilFuncTracer theFUNCTRACE(module, trace_info);

#define SY_API_TRACE_THIS(module,str)           SY_API_TRACE(module, str << " this=" << this)

#define SY_LOG_IF(mask, level, condition, str) \
    if (condition) UTIL_ADAPTER_TRACE(mask, level, str)

#define SY_IF_ERROR_TRACE(condition, str)   SY_LOG_IF(SY_TRACE_LEVEL_ERROR, "ERROR: ", condition, str)
#define SY_IF_WARNING_TRACE(condition, str) SY_LOG_IF(SY_TRACE_LEVEL_WARNING, "WARNING: ", condition, str)
#define SY_IF_INFO_TRACE(condition, str)    SY_LOG_IF(SY_TRACE_LEVEL_INFO, "INFO: ", condition, str)
#define SY_IF_DEBUG_TRACE(condition, str)   SY_LOG_IF(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", condition, str)
#define SY_IF_DETAIL_TRACE(condition, str)  SY_LOG_IF(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", condition, str)

#define SY_LOG_IF_EX(mask, level, condition, module, str) \
if (condition) UTIL_ADAPTER_TRACE_EX(mask, level, module, str)

#define SY_IF_ERROR_TRACE_EX(condition, module, str)   SY_LOG_IF_EX(SY_TRACE_LEVEL_ERROR, "ERROR: ", condition, module, str)
#define SY_IF_WARNING_TRACE_EX(condition, module, str) SY_LOG_IF_EX(SY_TRACE_LEVEL_WARNING, "WARNING: ", condition, module, str)
#define SY_IF_INFO_TRACE_EX(condition, module, str)    SY_LOG_IF_EX(SY_TRACE_LEVEL_INFO, "INFO: ", condition, module, str)
#define SY_IF_DEBUG_TRACE_EX(condition, module, str)   SY_LOG_IF_EX(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", condition, module, str)
#define SY_IF_DETAIL_TRACE_EX(condition, module, str)  SY_LOG_IF_EX(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", condition, module, str)

#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define SY_LOG_EVERY_N(mask, level, n, str) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
    if (LOG_OCCURRENCES_MOD_N == 1) UTIL_ADAPTER_TRACE(mask, level, str)

#define SY_LOG_EVERY_N_EX(mask, level, n, module, str) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
    if (LOG_OCCURRENCES_MOD_N == 1) UTIL_ADAPTER_TRACE_EX(mask, level, module, str)

#define SY_EVERY_N_ERROR_TRACE(n, str)   SY_LOG_EVERY_N(SY_TRACE_LEVEL_ERROR, "ERROR: ", n, str)
#define SY_EVERY_N_WARNING_TRACE(n, str) SY_LOG_EVERY_N(SY_TRACE_LEVEL_WARNING, "WARNING: ", n, str)
#define SY_EVERY_N_INFO_TRACE(n, str)    SY_LOG_EVERY_N(SY_TRACE_LEVEL_INFO, "INFO: ", n, str)
#define SY_EVERY_N_DEBUG_TRACE(n, str)   SY_LOG_EVERY_N(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", n, str)
#define SY_EVERY_N_DETAIL_TRACE(n, str)  SY_LOG_EVERY_N(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", n, str)

#define SY_EVERY_N_ERROR_TRACE_THIS(n, str)     SY_EVERY_N_ERROR_TRACE(n, str<<" this="<<this)
#define SY_EVERY_N_WARNING_TRACE_THIS(n, str)   SY_EVERY_N_WARNING_TRACE(n, str<<" this="<<this)
#define SY_EVERY_N_INFO_TRACE_THIS(n, str)      SY_EVERY_N_INFO_TRACE(n, str<<" this="<<this)
#define SY_EVERY_N_DEBUG_TRACE_THIS(n, str)     SY_EVERY_N_DEBUG_TRACE(n, str<<" this="<<this)
#define SY_EVERY_N_DETAIL_TRACE_THIS(n, str)    SY_EVERY_N_DETAIL_TRACE(n, str<<" this="<<this)

#define SY_EVERY_N_ERROR_TRACE_EX(n, module, str)   SY_LOG_EVERY_N_EX(SY_TRACE_LEVEL_ERROR, "ERROR: ", n, module, str)
#define SY_EVERY_N_WARNING_TRACE_EX(n, module, str) SY_LOG_EVERY_N_EX(SY_TRACE_LEVEL_WARNING, "WARNING: ", n, module, str)
#define SY_EVERY_N_INFO_TRACE_EX(n, module, str)    SY_LOG_EVERY_N_EX(SY_TRACE_LEVEL_INFO, "INFO: ", n, module, str)
#define SY_EVERY_N_DEBUG_TRACE_EX(n, module, str)   SY_LOG_EVERY_N_EX(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", n, module, str)
#define SY_EVERY_N_DETAIL_TRACE_EX(n, module, str)  SY_LOG_EVERY_N_EX(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", n, module, str)

#define SY_LOG_IF_EVERY_N(mask, level, condition, n, str) \
    static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
    ++LOG_OCCURRENCES; \
    if (condition && \
        ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
        UTIL_ADAPTER_TRACE(mask, level, str)

#define SY_IF_EVERY_N_ERROR_TRACE(condition, n, str)   SY_LOG_IF_EVERY_N(SY_TRACE_LEVEL_ERROR, "ERROR: ", condition, n, str)
#define SY_IF_EVERY_N_WARNING_TRACE(condition, n, str) SY_LOG_IF_EVERY_N(SY_TRACE_LEVEL_WARNING, "WARNING: ", condition, n, str)
#define SY_IF_EVERY_N_INFO_TRACE(condition, n, str)    SY_LOG_IF_EVERY_N(SY_TRACE_LEVEL_INFO, "INFO: ", condition, n, str)
#define SY_IF_EVERY_N_DEBUG_TRACE(condition, n, str)   SY_LOG_IF_EVERY_N(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", condition, n, str)
#define SY_IF_EVERY_N_DETAIL_TRACE(condition, n, str)  SY_LOG_IF_EVERY_N(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", condition, n, str)

#define SY_LOG_IF_EVERY_N_EX(mask, level, condition, n, module, str) \
static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
++LOG_OCCURRENCES; \
if (condition && \
((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
UTIL_ADAPTER_TRACE_EX(mask, level, module, str)

#define SY_IF_EVERY_N_ERROR_TRACE_EX(condition, n, module, str)   SY_LOG_IF_EVERY_N_EX(SY_TRACE_LEVEL_ERROR, "ERROR: ", condition, n, module, str)
#define SY_IF_EVERY_N_WARNING_TRACE_EX(condition, n, module, str) SY_LOG_IF_EVERY_N_EX(SY_TRACE_LEVEL_WARNING, "WARNING: ", condition, n, module, str)
#define SY_IF_EVERY_N_INFO_TRACE_EX(condition, n, module, str)    SY_LOG_IF_EVERY_N_EX(SY_TRACE_LEVEL_INFO, "INFO: ", condition, n, module, str)
#define SY_IF_EVERY_N_DEBUG_TRACE_EX(condition, n, module, str)   SY_LOG_IF_EVERY_N_EX(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", condition, n, module, str)
#define SY_IF_EVERY_N_DETAIL_TRACE_EX(condition, n, module, str)  SY_LOG_IF_EVERY_N_EX(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", condition, n, module, str)

// WARNING: It is your duty to supply the counter, just give the variable pointer, the macro will modify its value
#define SY_LOG_FIRST_N(mask, level, counter_ptr, n, str) \
    if ((*counter_ptr) < n) { \
        ++(*counter_ptr); \
        UTIL_ADAPTER_TRACE(mask, level, str); }

#define SY_FIRST_N_ERROR_TRACE(counter_ptr, n, str)   SY_LOG_FIRST_N(SY_TRACE_LEVEL_ERROR, "ERROR: ", counter_ptr, n, str)
#define SY_FIRST_N_WARNING_TRACE(counter_ptr, n, str) SY_LOG_FIRST_N(SY_TRACE_LEVEL_WARNING, "WARNING: ", counter_ptr, n, str)
#define SY_FIRST_N_INFO_TRACE(counter_ptr, n, str)    SY_LOG_FIRST_N(SY_TRACE_LEVEL_INFO, "INFO: ", counter_ptr, n, str)
#define SY_FIRST_N_DEBUG_TRACE(counter_ptr, n, str)   SY_LOG_FIRST_N(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", counter_ptr, n, str)
#define SY_FIRST_N_DETAIL_TRACE(counter_ptr, n, str)  SY_LOG_FIRST_N(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", counter_ptr, n, str)

#define SY_LOG_FIRST_N_EX(mask, level, counter_ptr, n, module, str) \
if ((*counter_ptr) < n) { \
++(*counter_ptr); \
UTIL_ADAPTER_TRACE_EX(mask, level, module, str); }

#define SY_FIRST_N_ERROR_TRACE_EX(counter_ptr, n, module, str)   SY_LOG_FIRST_N_EX(SY_TRACE_LEVEL_ERROR, "ERROR: ", counter_ptr, n, module, str)
#define SY_FIRST_N_WARNING_TRACE_EX(counter_ptr, n, module, str) SY_LOG_FIRST_N_EX(SY_TRACE_LEVEL_WARNING, "WARNING: ", counter_ptr, n, module, str)
#define SY_FIRST_N_INFO_TRACE_EX(counter_ptr, n, module, str)    SY_LOG_FIRST_N_EX(SY_TRACE_LEVEL_INFO, "INFO: ", counter_ptr, n, module, str)
#define SY_FIRST_N_DEBUG_TRACE_EX(counter_ptr, n, module, str)   SY_LOG_FIRST_N_EX(SY_TRACE_LEVEL_DEBUG, "DEBUG: ", counter_ptr, n, module, str)
#define SY_FIRST_N_DETAIL_TRACE_EX(counter_ptr, n, module, str)  SY_LOG_FIRST_N_EX(SY_TRACE_LEVEL_DETAIL, "DETAIL: ", counter_ptr, n, module, str)

class SY_OS_EXPORT CSyUtilFuncTracer
{
    public :
    CSyUtilFuncTracer(const char *mould, const char *str);
    virtual ~CSyUtilFuncTracer();
    
private:
    const char *pmould;
    const char *buffer;
    uint32_t m_tick;
};

////////////////////////////////////////
///Below is trace macro for train
////////////////////////////////////////

#define SY_T120TRACE_EX(mask,module, str)                   \
    {                                               \
        if(is_module_trace_enabled(mask,module))\
        {\
            char achFormatBuf[T120TRACE_MAX_TRACE_LEN];    \
            CSyTextFormator formator(achFormatBuf, T120TRACE_MAX_TRACE_LEN); \
            char *content = formator << str; \
            util_adapter_trace(mask, module, content, formator.tell()); \
        }\
    }

#define SY_T120TRACE(mask, str)     SY_T120TRACE_EX(mask, "", str)

#define SYQOS_INFO_TRACE(str) SY_T120TRACE_EX(SY_TRACE_LEVEL_INFO, "", str)
#define SYQOS_INFO_TRACE_THIS(str) SYQOS_INFO_TRACE(str<<" this="<<this)

#define SYSIP_INFO_TRACE(str) SY_T120TRACE_EX(SY_TRACE_LEVEL_INFO, "", str)
#define SYSIP_INFO_TRACE_THIS(str) SYSIP_INFO_TRACE(str<<" this="<<this)


#define LOG_METRIC(logger, path, value, timestamp)                            \
    SY_T120TRACE(SY_TRACE_LEVEL_DETAIL, "[METRICS] - "<<get_leading_id()<<path<<" "<<value<<" "<<timestamp)

#define LOG_EVENT(logger, what, tag, data)                            \
    SY_T120TRACE(SY_TRACE_LEVEL_DETAIL, "[EVENTS] - "<<  "{ \
             \"what\":\""<< what << "\",\
             \"tags\":\""<< get_leading_id() << tag <<"\",\
             \"data\":\""<<data <<"\"\
}")
//



#define ERRTRACE(str)       SY_T120TRACE(SY_TRACE_LEVEL_ERROR, str)
#define WARNINGTRACE(str)   SY_T120TRACE(SY_TRACE_LEVEL_WARNING, str)
#define INFOTRACE(str)      SY_T120TRACE(SY_TRACE_LEVEL_INFO, str)
#define STATETRACE(str)     SY_T120TRACE(SY_TRACE_LEVEL_DEBUG, str)
#define PDUTRACE(str)       SY_T120TRACE(SY_TRACE_LEVEL_DEBUG, str)
#define TICKTRACE(str)      SY_T120TRACE(SY_TRACE_LEVEL_DEBUG, str)
#define DETAILTRACE(str)    SY_T120TRACE(SY_TRACE_LEVEL_DETAIL, str)

#define ERRTRACE_THIS(str)      ERRTRACE(str << " this=" << this)
#define WARNINGTRACE_THIS(str)  WARNINGTRACE(str << " this=" << this)
#define INFOTRACE_THIS(str)     INFOTRACE(str << " this=" << this)
#define STATETRACE_THIS(str)    STATETRACE(str << " this=" << this)
#define PDUTRACE_THIS(str)      PDUTRACE(str << " this=" << this)
#define TICKTRACE_THIS(str)     TICKTRACE(str << " this=" << this)
#define DETAILTRACE_THIS(str)   DETAILTRACE(str << " this=" << this)

END_UTIL_NS

#endif // SYDEBUG_H
