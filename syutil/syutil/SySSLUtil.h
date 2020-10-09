#ifndef __sy_SySSLUtil_h
#define __sy_SySSLUtil_h

#include "SyCommDef.h"
#include "SyDebug.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/ocsp.h>


#define SSLNM


START_TP_NS

SY_OS_EXPORT void SyTraceOpenSslError(LPCSTR aFuncName, LPVOID pThis);
SY_OS_EXPORT void InitThreadForOpenSSL();
SY_OS_EXPORT void UnInitThreadForOpenSSL();

extern "C" SY_OS_EXPORT int SetFIPSMode(int ONOFF);

extern "C" SY_OS_EXPORT const EVP_CIPHER* GetEvpCipherForCbc(int keyBits);
extern "C" SY_OS_EXPORT const EVP_CIPHER* GetEvpCipherForOfb(int keyBits);
extern "C" SY_OS_EXPORT int EvpEncrypt(const uint8_t *ptext, int plen, uint8_t *ctext, const uint8_t *key, const uint8_t *iv, const EVP_CIPHER *cipher, bool padding = true);
extern "C" SY_OS_EXPORT int EvpDecrypt(const uint8_t *ctext, int clen, uint8_t *ptext, const uint8_t *key, const uint8_t *iv, const EVP_CIPHER *cipher, bool padding = true);

END_TP_NS

#endif
