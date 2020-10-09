#ifndef SY_NETWORK_H
#define SY_NETWORK_H

#ifndef MachOSupport
    #include <DateTimeUtils.h>
    #include <CGBase.h>
    #include <ctype.h>
#else
    //#include <unistd.h>
    #include <sys/uio.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    //#include <semaphore.h>
    #include <sys/resource.h>
    #include <sys/ioctl.h>
    #include <sys/stat.h>
    #include <sys/fcntl.h>
    #include <sys/errno.h>
    #include <sys/time.h>
#endif //MachOSupport

#ifndef MachOSupport
    #include "CFMCallSysBundle.h"
    #include "sys-socket.h"
    #include "netinet-in.h"
    #include "netdb.h"
    #include "fcntl.h"
#endif //MachOSupport

START_UTIL_NS

#ifndef TCP_NODELAY
enum {
    TCP_NODELAY                   = 0x01,
    TCP_MAXSEG                    = 0x02,
    TCP_NOTIFY_THRESHOLD          = 0x10, /** not a real XTI option */
    TCP_ABORT_THRESHOLD           = 0x11, /** not a real XTI option */
    TCP_CONN_NOTIFY_THRESHOLD     = 0x12, /** not a real XTI option */
    TCP_CONN_ABORT_THRESHOLD      = 0x13, /** not a real XTI option */
    TCP_OOBINLINE                 = 0x14, /** not a real XTI option */
    TCP_URGENT_PTR_TYPE           = 0x15, /** not a real XTI option */
    TCP_KEEPALIVE                 = 0x0008 /* keepalive defined in OpenTransport.h */
};
#endif

END_UTIL_NS

#endif  //SY_NETWORK_H
