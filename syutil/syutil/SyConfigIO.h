#ifndef util_SyConfigIO_h
#define util_SyConfigIO_h

#include "SyError.h"
#include "SyDef.h"
#include "SyString.h"

START_UTIL_NS
SY_OS_EXPORT unsigned char get_string_param(const char *group, const char *item_key, char *item_value,
                                            unsigned long len);

SY_OS_EXPORT unsigned char get_string_param_from_configfile(const char *config_file_name, const char *group,
                                                            const char *item_key, char *item_value, unsigned long len);

///CAUTION, please make sure this won't violate any security rules before you start to use this function.
///Previously, we use enable_config_io_feature to disable the functionality of reading config file but there is some valid case to read from system
///So I add a new function to be called when the caller was clear about the security ASSERTION.
SY_OS_EXPORT unsigned char get_string_param_from_ini_force(const char *config_file_name, const char *group,
                                                     const char *item_key, char *item_value, unsigned long len);

SY_OS_EXPORT int get_int_param(char *group, char *item_key);
SY_OS_EXPORT unsigned short get_uint16_param(char *group, char *item_key);
SY_OS_EXPORT unsigned long get_uint32_param(char *group, char *item_key);
SY_OS_EXPORT unsigned char get_bool_param(char *group, char *item_key, unsigned char default_value);

SY_OS_EXPORT void set_webex_home_env(char *home_env);
SY_OS_EXPORT const char *get_process_name();
SY_OS_EXPORT const char *get_exec_name(void);

SY_OS_EXPORT void set_webex_config_file_name(const CSyString &file);
SY_OS_EXPORT const char *get_webex_home_dir();

SY_OS_EXPORT void enable_config_io_feature(bool bEnable);


END_UTIL_NS

#endif
