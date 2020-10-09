#ifndef SYERROR_H
#define SYERROR_H

#include "SyDef.h"

#define SY_FAILED(rv)           (rv != 0)
#define SY_SUCCEEDED(rv)        (rv == 0)
#define SY_OK                   0
#define SY_FALSE                1

#define COMMON          30000000
#define _UNDEFINE              0
#define _NETWORK         1000000
#define MAKE_ERROR_CODE(comp, mod, sev, code)   (comp + mod + code)

// Add more Common module
// 30000000
#define SY_ERROR_FAILURE                                    MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 1) // Returned when a function fails
#define SY_ERROR_NOT_INITIALIZED                            MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 2) // Returned when an instance is not initialized
#define SY_ERROR_ALREADY_INITIALIZED                        MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 3) // Returned when an instance is already initialized
#define SY_ERROR_NOT_IMPLEMENTED                            MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 4) // Returned by a not implemented function
#define SY_ERROR_NULL_POINTER                               MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 5)
#define SY_ERROR_UNEXPECTED                                 MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 6) // Returned when an unexpected error occurs
#define SY_ERROR_OUT_OF_MEMORY                              MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 7) // Returned when a memory allocation fails
#define SY_ERROR_INVALID_ARG                                MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 8) // Returned when an illegal value is passed
#define SY_ERROR_NOT_AVAILABLE                              MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 9) // Returned when an operation can't complete due to an unavailable resource
#define SY_ERROR_WOULD_BLOCK                                MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 10)
#define SY_ERROR_NOT_FOUND                                  MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 11)
#define SY_ERROR_FOUND                                      MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 12)
#define SY_ERROR_PARTIAL_DATA                               MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 13)
#define SY_ERROR_TIMEOUT                                    MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 14)
#define SY_ERROR_VOIP_NOSOUCEMCS                            MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 15)
#define SY_ERROR_SELECT_NBRMCS                              MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 16)
#define SY_ERROR_SELECT_DSYCS                               MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 17)
#define SY_ERROR_PACKAGE_DROP                               MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 18)
#define SY_ERROR_CAPACITY_LIMIT                             MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 19) //for RTSP describe confirm
#define SY_ERROR_LOAD_CAL                                   MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 20)
#define SY_ERROR_NOT_FOUND_CONFERENCE                       MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 21)
#define SY_ERROR_TERMINATING                                MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 22)
#define SY_ERROR_INVALID_THREAD                             MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 23)
#define SY_ERROR_INVALID_STATUS                             MAKE_ERROR_CODE(COMMON, _UNDEFINE, ERROR, 24)

/////////////////// Error code for Common network  ///////////////////
// 31000000
#define SY_ERROR_NETWORK_SOCKET_ERROR                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 1) // Error code for Common network
#define SY_ERROR_NETWORK_SOCKET_RESET                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 2)
#define SY_ERROR_NETWORK_SOCKET_CLOSE                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 3)
#define SY_ERROR_NETWORK_SOCKET_BIND_ERROR                  MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 4)
#define SY_ERROR_NETWORK_CONNECT_ERROR                      MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 5)
#define SY_ERROR_NETWORK_CONNECT_TIMEOUT                    MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 6)
#define SY_ERROR_NETWORK_DNS_FAILURE                        MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 7)
#define SY_ERROR_NETWORK_PROXY_SERVER_UNAVAILABLE           MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 8)
#define SY_ERROR_NETWORK_UNKNOWN_ERROR                      MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 9)
#define SY_ERROR_NETWORK_NO_SERVICE                         MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 10)
/////////////////// For protocol tp Service ///////////////////
#define SY_ERROR_NETWORK_CONNECTION_RECONNECT               MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 11)
#define SY_ERROR_NETWORK_CONNECTION_RECONNECT_FAILED        MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 12)
#define SY_ERROR_NETWORK_CONNECTION_WRONG_TYPE              MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 13)
#define SY_ERROR_NETWORK_ABATE_CONNECTION                   MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 14)
#define SY_ERROR_NETWORK_DENY_ERROR                         MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 15)

#define SY_ERROR_NETWORK_IMCOMPLETE_CONNECTION              MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 16) // for group connection, such as jambosession
#define SY_ERROR_NETWORK_RETRY_ERROR                        MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 17) // overflow retry times
#define SY_ERROR_NETWORK_PDU_ERROR                          MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 18)
#define SY_ERROR_PROXY_RETRYTIMES_OVER                      MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 19)
#define SY_ERROR_PROXY_CACNEL_BY_USER                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 20)
#define SY_ERROR_NETWORK_NO_PROXY                           MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 21)
#define SY_ERROR_NETWORK_RECEIVED_NONE                      MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 22) //server not get any packets over 30 seconds, failover Mar 23 2008 Victor
#define SY_ERROR_NETWORK_DENY_DUP_PORT                      MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 23)
#define SY_ERROR_START_SECOND_CONNECT                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 24)
#define SY_ERROR_IN_PROGRESS                                MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 25)
#define SY_ERROR_ACCEPT_FAILURE_BURST                       MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 26)
#define SY_ERROR_DISCONNETED_BY_PEER                        MAKE_ERROR_CODE(COMMON, _NETWORK, ERROR, 27)

//fix CSCuq12455
#define ERROR_INTERNET_INVALID_CA                       12045
#define ERROR_INTERNET_SEC_INVALID_CERT                 12169
#define ERROR_INTERNET_SEC_CERT_DATE_INVALID            12037
#define ERROR_INTERNET_SEC_CERT_CN_INVALID              12038
#define ERROR_INTERNET_SEC_CERT_REVOKED                 12170
#define ERROR_INTERNET_SEC_CERT_REV_FAILED              12057


#define ERROR_INTERNET_SEC_REV_PASS                     12888   //used by TP internal
#define ERROR_INTERNET_SEC_REV_PENDING                  12889   //used by TP internal


#endif // SYERROR_H
