//
//  util.h
//  syaudio
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//

#ifndef syaudio_util_h
#define syaudio_util_h

#define SY_NAMESPACE_BG namespace sy {
#define SY_NAMESPACE_ED }

#define TRACE_MAX_TRACE_LEN 1024

#define UNIFIED_TRACE_LEVEL(level, str, len, Func_) \
  do {    \
    char ch_buffer[len];\
    CTextFormator formator(ch_buffer, len);\
    char* trace_info = formator << level << str; \
    Func_(trace_info, formator.tell()); \
  } while(0)

#define ERRTRACE(str)      UTIL_ADAPTER_TRACE_EX("ERROR: ", str<<FILE_LINE, TRACE_MAX_TRACE_LEN, _trace_print)
#define WARNTRACE(str)     UTIL_ADAPTER_TRACE_EX("WARNING: ", str<<FILE_LINE, TRACE_MAX_TRACE_LEN, _trace_print)
#define INFOTRACE(str)     UTIL_ADAPTER_TRACE_EX("INFO: ", str, TRACE_MAX_TRACE_LEN, _trace_print)
#define DBGTRACE(str)      UTIL_ADAPTER_TRACE_EX("DEBUG: ", str, TRACE_MAX_TRACE_LEN, _trace_print)


#endif
