#ifndef SYUTILTEMPLATES_H
#define SYUTILTEMPLATES_H

#include "SyReferenceControl.h"
#include "SyTimeValue.h"
#include "SyUtilClasses.h"
#include "SyUtilMisc.h"
#include "SyDebug.h"
#if defined(SY_LINUX)
#include <cmath>
#define ISNAN(var) std::isnan(var)
#else
#define ISNAN(var) isnan(var)
#endif
#include "math.h"

START_UTIL_NS

/**
 * @class CSySingletonT
 *
 * @brief A Singleton Adapter uses the Adapter pattern to turn ordinary
 * classes into Singletons optimized with the Double-Checked
 * Locking optimization pattern.
 */

//Victor 10/16 2006, That static var not be init on MAC if unload dll, why?
template <class Type>
class CSySingletonT : public CSyCleanUpBase
{
public:
    static Type *Instance()
    {
        if (!m_psInstance) {
            MutexType *pMutex = SyGetSingletonMutex();
            if (pMutex) {
                CSyMutexGuardT<MutexType> theGuard(*pMutex);
                if (!m_psInstance) {
                    m_psInstance = new CSySingletonT<Type>();
                }
            }
            SY_ASSERTE_RETURN(m_psInstance, NULL);  //to-do there compile issue for xmppSDK, need more check
        }
        return &m_psInstance->m_Instance;
    }

protected:
    CSySingletonT() {};

    virtual ~CSySingletonT()
    {
        SY_INFO_TRACE_THIS("CSySingletonT::~CSySingletonT() instance = " << &m_Instance);
        m_psInstance = NULL;
    }
    Type m_Instance;

private:
    typedef CSyMutexThreadRecursive MutexType;

    // = Prevent assignment and initialization.
    void operator = (const CSySingletonT &);
    CSySingletonT(const CSySingletonT &);
    static CSySingletonT *m_psInstance;
};

template< typename Type>
CSySingletonT<Type> *CSySingletonT<Type>::m_psInstance = NULL;

template<typename T>
SyResult xtoa_wbx(T value, char *pOutBuff, int nOutBuffLen)
{
    if (nOutBuffLen <= 2 || !pOutBuff) { //have no enough space to store it
        return SY_ERROR_FAILURE;
    }

    BOOL isLessZero = FALSE;
    if (value <= (T)0) {
        isLessZero =  TRUE;
        value = -1*value;
    }
    pOutBuff[nOutBuffLen - 1] = 0;
    for (int i = nOutBuffLen - 2; i >= 0; --i) {
        pOutBuff[i] = value % 10 + char('0');
        value /= 10;
        if (value == 0) { //over
            if (isLessZero) {
                pOutBuff[--i] = '-';
            }
            memmove(pOutBuff, pOutBuff + i, nOutBuffLen - i);
            return SY_OK;
        }
    }
    return SY_ERROR_FAILURE;
}

SY_OS_EXPORT void sy_reverse(char str[], int length);

template<typename T>
char *sy_itoa(T num, char *str, int base)
{
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < (T)0 && base == 10) {
        isNegative = true;
        num = 0-num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    // If number is negative, append '-'
    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    sy_reverse(str, i);

    return str;
}

template<typename T>
char *sy_ftoa(T num, char *str)
{
    int i = 0, num_i = (int) num, num_f ;
    bool isNegative = false;

    if(ISNAN(num)) {
        str[0] = 'N';
        str[1] = 'a';
        str[2] = 'N';
        str[3] = '\0';
        return str;
    }
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num_i == 0) {
        str[i++] = '0';
        str[i] = '\0';
    }

    // For negative num
    if (num < 0) {
        isNegative = true;
        num_f = (int)((num_i - num)*1000);
        num_i = -num_i;
    } else {
        num_f = (int)((num - num_i)*1000);
    }

    // Process individual digits
    while (num_i != 0) {
        int rem = num_i % 10;
        str[i++] = rem + '0';
        num_i = num_i/10;
    }

    // If num_iber is negative, append '-'
    if (isNegative) {
        str[i++] = '-';
    }
    str[i] = '\0';

    // Reverse the string for int part
    sy_reverse(str, i);

    // deal with the 3 digit of float part
    str[i] = '.';
    str[i+1] = num_f/100 + '0';
    str[i+2] = num_f%100/10 + '0';
    str[i+3] = num_f%10 + '0';
    str[i+4] = '\0';

    return str;
}
END_UTIL_NS

#endif // !SYUTILTEMPLATES_H
