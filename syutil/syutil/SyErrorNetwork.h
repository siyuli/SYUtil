#ifndef SYERRORNETWORK_H
#define SYERRORNETWORK_H

#include "SyError.h"

#define SY_OPT_TRANSPORT_BASE           100

//Param. is Pointer to DWORD
//IO Read len
#define SY_OPT_TRANSPORT_FIO_NREAD      (SY_OPT_TRANSPORT_BASE+1)

//Param. is Pointer to DWORD
//Transport unread len
#define SY_OPT_TRANSPORT_TRAN_NREAD     (SY_OPT_TRANSPORT_BASE+2)

//Param. is Pointer to SY_HANDLE
//Get fd
#define SY_OPT_TRANSPORT_FD             (SY_OPT_TRANSPORT_BASE+3)

//Param. is Pointer to CInetAddr
//Get address socket binded
#define SY_OPT_TRANSPORT_LOCAL_ADDR     (SY_OPT_TRANSPORT_BASE+4)

//Param. is Pointer to CInetAddr
//Get peer addr
#define SY_OPT_TRANSPORT_PEER_ADDR      (SY_OPT_TRANSPORT_BASE+5)

//If the socket is still alive
//Param. is Pointer to BOOL
#define SY_OPT_TRANSPORT_SOCK_ALIVE     (SY_OPT_TRANSPORT_BASE+6)

//Param. is Pointer to DWORD
//TYPE_TCP, TYPE_UDP, TYPE_SSL...
#define SY_OPT_TRANSPORT_TRAN_TYPE      (SY_OPT_TRANSPORT_BASE+7)

// budingc add to support SO_KEEPALIVE function
//Param. is Pointer to DWORD
#define SY_OPT_TRANSPORT_TCP_KEEPALIVE  (SY_OPT_TRANSPORT_BASE+8)

//Param. is Pointer to DWORD
#define SY_OPT_TRANSPORT_RCV_BUF_LEN    (SY_OPT_TRANSPORT_BASE+9)

//Param. is Pointer to DWORD
#define SY_OPT_TRANSPORT_SND_BUF_LEN    (SY_OPT_TRANSPORT_BASE+10)

//Param. is Pointer to CSyMessageBlock*,
//send some user data in the connect request
#define SY_OPT_CONNECTION_CONNCET_DATA  (SY_OPT_TRANSPORT_BASE+11)

//Param. is Pointer to ISyTransport** ,
//get lower transport, for class CSyTransportThreadProxy
#define SY_OPT_LOWER_TRANSPORT          (SY_OPT_TRANSPORT_BASE+12)

//Param. is Pointer to int ,
//for DSCP setting, Type of Service (TOS) settings.
#define SY_OPT_TRANSPORT_TOS            (SY_OPT_TRANSPORT_BASE+13)

//Added for proxy http client, SY_OPT_TRANSPORT_PEER_ADDR
//return the real remote url instead of proxy's address
#define SY_OPT_TRANSPORT_PEER_URL       (SY_OPT_TRANSPORT_BASE+14)

//for proxy's metrics
#define SY_OPT_TRANSPORT_PROXY_METRICS  (SY_OPT_TRANSPORT_BASE+15)
//for TLS's metrics
#define SY_OPT_TLS_PROXY_METRICS  (SY_OPT_TRANSPORT_BASE+16)

///////////For Connection Service/////////////
//Param. is Pointer to BOOL
#define CS_OPT_NEED_KEEPALIVE           (SY_OPT_TRANSPORT_BASE+31)

//Param. is Pointer to DWORD
#define CS_OPT_MAX_SENDBUF_LEN          (SY_OPT_TRANSPORT_BASE+32)

//Param. is Pointer to BOOL
#define CS_OPT_PKG_NEED_BUF             (SY_OPT_TRANSPORT_BASE+33)

//Param. is Pointer to DWORD (Seconds)
#define CS_OPT_KEEPALIVE_INTERVAL       (SY_OPT_TRANSPORT_BASE+34)
//Param. is Pointer to BOOL
#define CS_OPT_DISABLE_RCVDATA_FLAG     (SY_OPT_TRANSPORT_BASE+35)

//Param. is for connection abate overflow time, WORD
#define CS_OPT_ABATE_TIME               (SY_OPT_TRANSPORT_BASE+36)

//Param. is for connection reconnect overflow time, WORD
#define CS_OPT_RECONNECT_TIME           (SY_OPT_TRANSPORT_BASE+37)

//Param. inquire the RTT, BOOL
#define CS_OPT_DETECT_RTT               (SY_OPT_TRANSPORT_BASE+38)
//Param. inquire the RTT
#define CS_OPT_INQUIRE_RTT              (SY_OPT_TRANSPORT_BASE+39)

#define CS_OPT_SERVER_UNAVAIL_TIMEOUT   (SY_OPT_TRANSPORT_BASE+40)

/*! enable negative on application layer */
#define CS_OPT_TCP_APP_NEGATIVE_LEN     (SY_OPT_TRANSPORT_BASE+41)

#define CS_OPT_TCP_APP_NEGATIVE_DELAY   (SY_OPT_TRANSPORT_BASE+42)

#define CS_OPT_PDU_ENABLE_CRC           (SY_OPT_TRANSPORT_BASE+43)
#define CS_OPT_PDU_MTK                  (SY_OPT_TRANSPORT_BASE+44)
//for security_issue
#define CS_OPT_PDU_IV                   (SY_OPT_TRANSPORT_BASE+45)

#define CS_OPT_PDU_USE_AES_CBC          (SY_OPT_TRANSPORT_BASE+46)

//Param. is Pointer to DWORD, get available space in sendbuf
#define CS_OPT_FREE_SENDBUF_LEN         (SY_OPT_TRANSPORT_BASE+47)

//Param. is Pointer to DWORD, get available space in TCP send buf
#define SY_OPT_TRANSPORT_FREE_SENDBUF_LEN (SY_OPT_TRANSPORT_BASE+48)

#define SY_OPT_TRANSPORT_DIRECT     (SY_OPT_TRANSPORT_BASE+49)

//the interval between 2 sending data
#define SY_OPT_ICE_TRANSPORT_KEEPALIVETIMEOUT       (SY_OPT_TRANSPORT_BASE+50)

//define the max missed stun packages, if send these packages without and response, disconnect the transport
#define SY_OPT_ICE_TRANSPORT_MAX_MISSED_STUN_PACKAGES       (SY_OPT_TRANSPORT_BASE+51)

//define the response time between send a request and receive a response
#define SY_OPT_ICE_TRANSPORT_STUN_RESPONSE_TIME     (SY_OPT_TRANSPORT_BASE+52)

//define if we will verify peer certificate for security, the data is BOOL type
#define SY_OPT_TLS_VERIFY_PEER_CERTIFICATE      (SY_OPT_TRANSPORT_BASE+53)

//define set the peer certificate fingerprint for verification, the data is a null terminated string
#define SY_OPT_TLS_PEER_CERT_FINGERPRINT         (SY_OPT_TRANSPORT_BASE+54)

//define set the server certificate, argument is an object to CSyString
#define SY_OPT_TLS_SET_CERTIFICATE          (SY_OPT_TRANSPORT_BASE+55)

//Enable TCP transport to do the statistics in TP layer, bool is the value type
#define SY_OPT_ENABLE_TCP_STATS             (SY_OPT_TRANSPORT_BASE+56)

//Set RlbTcpClient alive detection timeout, long is the value type
#define SY_OPT_ALIVE_DETECT_TIMEOUT             (SY_OPT_TRANSPORT_BASE+57)

//Added for train mobile client as a reason to disconnect of a reliable TCP connection, this reason will trigger a force reconnect.
#define SY_OPT_FORCE_RECONNECT                  (SY_OPT_TRANSPORT_BASE+58)

//Added FastLane options
#define SY_OPT_NET_SERVICE_TYPE (SY_OPT_TRANSPORT_BASE+59)
#define SY_OPT_NETSVC_MARKING_LEVEL (SY_OPT_TRANSPORT_BASE+60)

//Set the local port range
//As a connector, if we set local address that will bind to a port
//but if we want the local port in a range and if bind failed, it can try other ports, we need to set a range
#define SY_OPT_BIND_LOCAL_PORT_MIN      (SY_OPT_TRANSPORT_BASE+61)
#define SY_OPT_BIND_LOCAL_PORT_MAX      (SY_OPT_TRANSPORT_BASE+62)

//TCP/UDP net statistics prefix to easily correlate them with media, const char*
#define SY_OPT_NET_STATS_PREFIX         (SY_OPT_TRANSPORT_BASE+63)

//////////////////////////////////////////////

/// for channel
#define SY_OPT_CHANNEL_BASE             200

//Param. is Pointer to BOOL
#define SY_OPT_CHANNEL_FILE_SYNC        (SY_OPT_CHANNEL_BASE + 101)

//Param. is Pointer to BOOL
#define SY_OPT_CHANNEL_HTTP_HEADER_NO_CONTENT_LENGTH    (SY_OPT_CHANNEL_BASE + 111)

//Param. is Pointer to BOOL
#define SY_OPT_CHANNEL_HTTP_PARSER_SKIP_CONTENT_LENGTH  (SY_OPT_CHANNEL_BASE + 112)

#define SY_OPT_CHANNEL_HTTP_APPEND_HTTPHEAD_FOR_EACH    (SY_OPT_CHANNEL_BASE + 113)

//#define SY_OPT_CHANNEL_HTTP_DELEVER_PARTIAL_DATA      (SY_OPT_CHANNEL_BASE + 114)

#define SY_OPT_CHANNEL_HTTP_LOCK_ADDRESS                (SY_OPT_CHANNEL_BASE + 115)

#define SY_OPT_SET_HTTP_CONN_TIMEOUT                    (SY_OPT_CHANNEL_BASE + 116)

#define SY_OPT_GET_WEBSOCKET_PATH                   (SY_OPT_CHANNEL_BASE + 117)
#define SY_OPT_GET_USERNAME_IN_BINDMSG                  (SY_OPT_CHANNEL_BASE + 118)

#define SY_OPT_SET_WEBSOCKET_FRAME_TYPE                 (SY_OPT_CHANNEL_BASE + 119)

#define SY_OPT_CHANNEL_HTTP_FOR_RTP                     (SY_OPT_CHANNEL_BASE + 120)

#define SY_OPT_HTTP_CLIENT_CFLAG                        (SY_OPT_CHANNEL_BASE + 121)


#endif // SYERRORNETWORK_H
