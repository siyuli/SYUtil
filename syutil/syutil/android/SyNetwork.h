#ifndef SY_NETWORK_H
#define SY_NETWORK_H

#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#include <assert.h>

#include <netinet/tcp.h>
#include <semaphore.h>

#ifdef SY_SOLARIS
    #include <sys/filio.h>
#endif


#define closesocket             close
#define ioctlsocket             ioctl

#ifndef SOCKET_ERROR
    #define SOCKET_ERROR    -1
#endif

#endif
