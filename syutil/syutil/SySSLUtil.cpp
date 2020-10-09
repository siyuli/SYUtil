#include "SySSLUtil.h"
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/opensslconf.h>
#include <openssl/aes.h> // for AES_BLOCK_SIZE

#include <memory>

void SyTraceOpenSslError(LPCSTR aFuncName, LPVOID pThis)
{
    SY_ASSERTE(aFuncName);
    unsigned long error_code = SSLNM::ERR_get_error();
    char error_string[512] = {0};

    SSLNM::ERR_error_string(error_code, error_string);
    SY_ERROR_TRACE(aFuncName << " err_str=" << error_string << " this=" << pThis);
}

#ifdef SY_WIN32
static HANDLE *lock_cs = NULL;

void win32_locking_callback(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
#if defined UWP || defined WP8
        WaitForSingleObjectEx(lock_cs[type], INFINITE, false);
#else
        WaitForSingleObject(lock_cs[type], INFINITE);
#endif
    }
    else
    {
        ReleaseMutex(lock_cs[type]);
    }
}

void thread_setup(void)
{
    int i;
    
    lock_cs = (HANDLE *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(HANDLE));
    for (i = 0; i<CRYPTO_num_locks(); i++)
    {
#if defined UWP || defined WP8
        lock_cs[i] = CreateMutexExW(NULL, FALSE, NULL, MUTEX_ALL_ACCESS);
#else
        lock_cs[i] = CreateMutex(NULL, FALSE, NULL);
#endif
    }
    
    CRYPTO_set_locking_callback((void(*)(int, int, const char *, int))win32_locking_callback);
    /* id callback defined */
}

void thread_cleanup(void)
{
    int i;
    
    if (lock_cs == NULL) {
        SY_WARNING_TRACE("thread_cleanup, lock is not initialized by us.");
        return;
    }

    CRYPTO_set_locking_callback(NULL);
    for (i = 0; i<CRYPTO_num_locks(); i++)
        CloseHandle(lock_cs[i]);
    OPENSSL_free(lock_cs);
    lock_cs = NULL;
}



#else //SY_WIN32

static pthread_mutex_t *lock_cs = NULL;
static long *lock_count = NULL;

void pthreads_locking_callback(int mode, int type, const char *file, int line)
{
    
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&lock_cs[type]);
        lock_count[type]++;
    }
    else
    {
        pthread_mutex_unlock(&lock_cs[type]);
    }
}

unsigned long pthreads_thread_id(void)
{
    unsigned long ret;
    
    ret = (unsigned long)pthread_self();
    return(ret);
}


void thread_setup(void)
{
    int i;
    
    lock_cs = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    lock_count = (long *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));
    for (i = 0; i<CRYPTO_num_locks(); i++)
    {
        lock_count[i] = 0;
        pthread_mutex_init(&(lock_cs[i]), NULL);
    }
    
    CRYPTO_set_id_callback((unsigned long(*)())pthreads_thread_id);
    CRYPTO_set_locking_callback((void(*)(int, int, const char *, int))pthreads_locking_callback);
}

void thread_cleanup(void)
{
    if(lock_cs == NULL) {
        return;
    }
    CRYPTO_set_locking_callback(NULL);
    //Comment them to avoid exiting crash, but this should be released, why not?
    for (int i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&(lock_cs[i]));
    }
    OPENSSL_free(lock_cs);
    OPENSSL_free(lock_count);
    lock_cs = NULL;
    lock_count = NULL;
}
#endif //SY_WIN32

void InitThreadForOpenSSL()
{
    if (!CRYPTO_get_locking_callback()) {
        //prepare to setup thread, so need to release it when uninit
        //INFO_TRACE_MS_WITH_TYPE_ALL("setup thread support for OpenSSL");
        thread_setup();
    }
    else {
        SY_WARNING_TRACE("InitThreadForOpenSSL, crypto is not setup by us.");
    }
    SSLNM::SSL_library_init();
    SSLNM::SSL_load_error_strings();
}

void UnInitThreadForOpenSSL()
{
    SY_INFO_TRACE("cleanup thread support for OpenSSL");
    thread_cleanup();
}

int SetFIPSMode(int ONOFF)
{
    return SSLNM::FIPS_mode_set(ONOFF);
}

const EVP_CIPHER* GetEvpCipherForCbc(int keyBits)
{
    if (keyBits == 128) {
        return SSLNM::EVP_aes_128_cbc();
    } else if (keyBits == 192) {
        return SSLNM::EVP_aes_192_cbc();
    } else if (keyBits == 256) {
        return SSLNM::EVP_aes_256_cbc();
    } else {
        return nullptr;
    }
}

const EVP_CIPHER* GetEvpCipherForOfb(int keyBits)
{
    if (keyBits == 128) {
        return SSLNM::EVP_aes_128_ofb();
    } else if (keyBits == 192) {
        return SSLNM::EVP_aes_192_ofb();
    } else if (keyBits == 256) {
        return SSLNM::EVP_aes_256_ofb();
    } else {
        return nullptr;
    }
}

using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&SSLNM::EVP_CIPHER_CTX_free)>;
int EvpEncrypt(const uint8_t *ptext, int plen, uint8_t *ctext, const uint8_t *key, const uint8_t *iv, const EVP_CIPHER *cipher, bool padding)
{
    EVP_CIPHER_CTX_ptr ctx(SSLNM::EVP_CIPHER_CTX_new(), SSLNM::EVP_CIPHER_CTX_free);
    int rc = SSLNM::EVP_EncryptInit_ex(ctx.get(), cipher, NULL, key, iv);
    if (rc != 1){
        return -1;
    }
    SSLNM::EVP_CIPHER_CTX_set_padding(ctx.get(), padding?1:0);
    
    int clen = plen + AES_BLOCK_SIZE;
    int clen1 = clen;
    rc = SSLNM::EVP_EncryptUpdate(ctx.get(), ctext, &clen1, ptext, plen);
    if (rc != 1){
        return -1;
    }
    int rlen = (plen % AES_BLOCK_SIZE);
    if (!padding && rlen) {
        rlen = AES_BLOCK_SIZE - rlen;
        static uint8_t pad[AES_BLOCK_SIZE] = {0};
        int clen3 = clen - clen1;
        rc = SSLNM::EVP_EncryptUpdate(ctx.get(), ctext + clen1, &clen3, pad, rlen);
        if (rc != 1){
            return -1;
        }
        clen1 += clen3;
    }
    int clen2 = clen - clen1;
    rc = SSLNM::EVP_EncryptFinal_ex(ctx.get(), ctext + clen1, &clen2);
    if (rc != 1){
        return -1;
    }
    return clen1 + clen2;
}

int EvpDecrypt(const uint8_t *ctext, int clen, uint8_t *ptext, const uint8_t *key, const uint8_t *iv, const EVP_CIPHER *cipher, bool padding)
{
    EVP_CIPHER_CTX_ptr ctx(SSLNM::EVP_CIPHER_CTX_new(), SSLNM::EVP_CIPHER_CTX_free);
    int rc = SSLNM::EVP_DecryptInit_ex(ctx.get(), cipher, NULL, key, iv);
    if (rc != 1){
        return -1;
    }
    SSLNM::EVP_CIPHER_CTX_set_padding(ctx.get(), padding?1:0);

    int plen = clen;
    int plen1 = plen;
    rc = SSLNM::EVP_DecryptUpdate(ctx.get(), ptext, &plen1, ctext, clen);
    if (rc != 1){
        return -1;
    }
    int plen2 = plen - plen1;
    rc = SSLNM::EVP_DecryptFinal_ex(ctx.get(), ptext + plen1, &plen2);
    if (rc != 1){
        return -1;
    }
    return plen1 + plen2;
}
