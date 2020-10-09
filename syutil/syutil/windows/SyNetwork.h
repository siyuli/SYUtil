#ifndef SY_NETWORK_H
#define SY_NETWORK_H

#include "SyCommon.h"
#include "SyDef.h"

typedef SOCKET SY_SOCKET;

extern "C" SY_OS_EXPORT int SY_inet_pton(int af, const char *src, void *dst);
extern "C" SY_OS_EXPORT const char *SY_inet_ntop(int af, const void *src, char *dst, int size);

#ifdef WP8
#if (_WIN32_WINNT < 0x0600) || defined(SY_WIN_PHONE)
#define inet_ntop   SY_inet_ntop
#define inet_pton   SY_inet_pton
#endif
#else
#if (_WIN32_WINNT < 0x0600)
#define inet_ntop   SY_inet_ntop
#define inet_pton   SY_inet_pton
#endif

#endif

#endif //SY_NETWORK_H
