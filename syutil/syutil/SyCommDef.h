#ifndef SY_COMMDEF_H
#define SY_COMMDEF_H

#ifndef USE_NAMESPACE
    #define START_UTIL_NS
    #define END_UTIL_NS
    #define USE_UTIL_NS
    #define START_TP_NS
    #define END_TP_NS
    #define USE_TP_NS
#endif

#define SY_BIT_ENABLED(dword, bit) (((dword) & (bit)) != 0)
#define SY_BIT_DISABLED(dword, bit) (((dword) & (bit)) == 0)
#define SY_BIT_SYP_MASK(dword, bit, mask) (((dword) & (bit)) == mask)
#define SY_SET_BITS(dword, bits) (dword |= (bits))
#define SY_CLR_BITS(dword, bits) (dword &= ~(bits))

#define PARAM_IN
#define PARAM_OUT
#define PARAM_INOUT

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#endif //SY_COMMDEF_H
