/*===========================================================================*
                              psocket.c
==============================================================================

   low level communication facilities for Ppmtompeg parallel operation

   By Bryan Henderson 2004.10.13.  Contributed to the public domain by
   its author.

============================================================================*/

#define _XOPEN_SOURCE 500 /* Make sure stdio.h contains pclose() */
/* _ALL_SOURCE is needed on AIX to make the C library include the 
   socket services (e.g. define struct sockaddr) 

   Note that AIX standards.h actually sets feature declaration macros such
   as _XOPEN_SOURCE, unless they are already set.
*/
#define _ALL_SOURCE
#define __EXTENSIONS__
  /* __EXTENSIONS__ is for a broken Sun C library (uname SunOS kosh 5.8 
     generic_108528-16 sun4u sparc).  When you define _XOPEN_SOURCE,
     it's vnode.h and resource.h fail to define some data types that they
     need (e.g. timestruct_t).  But with __EXTENSIONS__, they declare the
     needed types anyway.  Our #include <sys/socket.h> causes the broken
     header files to get included.
  */

/* On AIX, pm_config.h includes standards.h, which expects to be included
   after feature declaration macros such as _XOPEN_SOURCE.  So we include
   pm_config.h as late as possible.
*/

#include "pm_config.h" /* For POSIX_IS_IMPLIED */

#ifdef POSIX_IS_IMPLIED
/* The OpenBSD C library, at least, is broken in that when _XOPEN_SOURCE
   is defined, its sys/socket.h refers to types "u_char", etc. but does
   not define them.  But it is also one of the C libraries where
   POSIX is implied so that we don't need to define _XOPEN_SOURCE in order
   to get the POSIX routines such as pclose() defined.  So we circumvent
   the problem by undefining _XOPEN_SOURCE:
*/
#undef _XOPEN_SOURCE
#endif

#include <stdarg.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "pm.h"
#include "pm_c_util.h"
#include "nstring.h"

#include "gethostname.h"

#include "psocket.h"


/* We use type socklenx_t where we should use socklen_t from the C
  library, but older systems don't have socklen_t.  And there doesn't
  appear to be any way for the preprocessor to know whether it exists
  or not.  On older systems with no socklen_t, a message length is a
  signed integer, but on modern systems, socklen_t is an unsigned
  integer.  Until we have some kind of build-time check for the existence
  of socklen_t, we just use this socklenx_t, which is an unsigned
  integer, and accept compiler warnings on older system.
  -Bryan 2001.04.22.
*/
typedef unsigned int socklenx_t;

#ifndef SOMAXCONN
#define SOMAXCONN 5
#endif



static void PM_GNU_PRINTF_ATTR(1,2)
errorExit(const char format[], ...) {

    const char * const hostname = GetHostName();

    va_list args;

    va_start(args, format);

    fprintf(stderr, "%s: FATAL ERROR.  ", hostname);
    strfree(hostname);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);

    exit(1);

    va_end(args);
}



static void
unmarshallInt(unsigned char const buffer[],
              int *         const valueP) {
/*----------------------------------------------------------------------------
  Interpret a number which is formatted for one of our network packets.

  To wit, 32 bit big-endian pure binary.
-----------------------------------------------------------------------------*/
    union {
        uint32_t value;
        unsigned char bytes[4];
    } converter;

    memcpy(&converter.bytes, buffer, 4);

    /* Note that contrary to what the C data types suggest, ntohl() is
       a 32 bit converter, even if a C "long" is bigger than that.
    */
    *valueP = ntohl(converter.value);
}



static void
safeRead(int             const fd, 
         unsigned char * const buf, 
         unsigned int    const nbyte) {
/*----------------------------------------------------------------------------
    Safely read from file 'fd'.  Keep reading until we get
    'nbyte' bytes.
-----------------------------------------------------------------------------*/
    unsigned int numRead;

    numRead = 0;  /* initial value */

    while (numRead < nbyte) {
        int const result = read(fd, &buf[numRead], nbyte-numRead);

        if (result == -1)
            errorExit("read (of %u bytes (total %u) ) returned "
                      "errno %d (%s)",
                      nbyte-numRead, nbyte, errno, strerror(errno));
        else 
            numRead += result;
    }
}



void
ReadBytes(int             const fd,
          unsigned char * const buf,
          unsigned int    const nbyte) {

    safeRead(fd, buf, nbyte);
}



void
ReadInt(int   const socketFd,
        int * const valueP) {

    unsigned char buffer[4];

    safeRead(socketFd, buffer, sizeof(buffer));

    unmarshallInt(buffer, valueP);
}



static void
marshallInt(int              const value,
            unsigned char (* const bufferP)[]) {
/*----------------------------------------------------------------------------
   Put the number 'value' into the buffer at *bufferP in the form required
   for one of our network packets.

   To wit, 32 bit big-endian pure binary.
-----------------------------------------------------------------------------*/
    union {
        uint32_t value;
        unsigned char bytes[4];
    } converter;

    unsigned char testbuffer[4];

    /* Note that contrary to what the C data types suggest, htonl() is
       a 32 bit converter, even if a C "long" is bigger than that.
    */
    converter.value = htonl(value);

    (*bufferP)[0] = 7;
    memcpy(testbuffer, &converter.bytes, 4);
    memcpy(*bufferP, &converter.bytes, 4);
}



static void
safeWrite(int             const fd, 
          unsigned char * const buf, 
          unsigned int    const nbyte) {
/*----------------------------------------------------------------------------
  Safely write to file 'fd'.  Keep writing until we write 'nbyte'
  bytes.
-----------------------------------------------------------------------------*/
    unsigned int numWritten;

    numWritten = 0;  /* initial value */

    while (numWritten < nbyte) {
        int const result = write(fd, &buf[numWritten], nbyte-numWritten);

        if (result == -1) 
            errorExit("write (of %u bytes (total %u) ) returned "
                      "errno %d (%s)",
                      nbyte-numWritten, nbyte, errno, strerror(errno));
        numWritten += result;
    }
}



void
WriteBytes(int             const fd,
           unsigned char * const buf,
           unsigned int    const nbyte) {

    safeWrite(fd, buf, nbyte);
}



void
WriteInt(int const socketFd,
         int const value) {

    unsigned char buffer[4];

    marshallInt(value, &buffer);

    safeWrite(socketFd, buffer, sizeof(buffer));
}



void
ConnectToSocket(const char *      const machineName, 
                int               const portNum, 
                struct hostent ** const hostEnt,
                int *             const socketFdP,
                const char **     const errorP) {
/*----------------------------------------------------------------------------
   Create a socket and connect it to the specified TCP endpoint.

   That endpoint is fundamentally defined by 'machineName' and
   'portNum', but *hostEnt is the address of a host entry that caches
   the results of the host name lookup.  If *hostEnt is non-null, we
   use it.  If *hostEnt is NULL, we look up the information and update
   **hostEnt.
-----------------------------------------------------------------------------*/
    int rc;
    
    *errorP = NULL;  /* initial value */

    if ((*hostEnt) == NULL) {
        (*hostEnt) = gethostbyname(machineName);
        if ((*hostEnt) == NULL)
            asprintfN(errorP, "Couldn't get host by name (%s)", machineName);
    }
    if (!*errorP) {
        rc = socket(AF_INET, SOCK_STREAM, 0);
        if (rc < 0)
            asprintfN(errorP, "socket() failed with errno %d (%s)", 
                      errno, strerror(errno));
        else {
            int const socketFd = rc;
            
            int rc;
            unsigned short tempShort;
            struct sockaddr_in  nameEntry;
            
            nameEntry.sin_family = AF_INET;
            memset((void *) nameEntry.sin_zero, 0, 8);
            memcpy((void *) &(nameEntry.sin_addr.s_addr),
                   (void *) (*hostEnt)->h_addr_list[0],
                   (size_t) (*hostEnt)->h_length);
            tempShort = portNum;
            nameEntry.sin_port = htons(tempShort);
            
            rc = connect(socketFd, (struct sockaddr *) &nameEntry,
                         sizeof(struct sockaddr));
            
            if (rc != 0)
                asprintfN(errorP, 
                          "connect() to host '%s', port %d failed with "
                          "errno %d (%s)",
                          machineName, portNum, errno, strerror(errno));
            else {
                *errorP = NULL;
                *socketFdP = socketFd;
            }
            if (*errorP)
                close(socketFd);
        }
    }
}



static bool
portInUseErrno(int const testErrno) {
/*----------------------------------------------------------------------------
   Return TRUE iff 'testErrno' is what a bind() would return if one requestd
   a port number that is unavailable (but other port numbers might be).
-----------------------------------------------------------------------------*/
    bool retval;

    switch (testErrno) {
    case EINVAL:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
        retval = TRUE;
        break;
    default:
        retval = FALSE;
    }
    return retval;
}



static void
bindToUnusedPort(int              const socketFd,
                 unsigned short * const portNumP,
                 const char **    const errorP) {
    
    bool foundPort;
    unsigned short trialPortNum;

    *errorP = NULL;  /* initial value */

    for (foundPort = FALSE, trialPortNum = 2048; 
         !foundPort && trialPortNum < 16384 && !*errorP; 
         ++trialPortNum) {
        
        struct sockaddr_in nameEntry;
        int rc;
        
        memset((char *) &nameEntry, 0, sizeof(nameEntry));
        nameEntry.sin_family = AF_INET;
        nameEntry.sin_port   = htons(trialPortNum);

        rc = bind(socketFd, (struct sockaddr *) &nameEntry,
                  sizeof(struct sockaddr));

        if (rc == 0) {
            foundPort = TRUE;
            *portNumP = trialPortNum;
        } else if (!portInUseErrno(errno))
            asprintfN(errorP, "bind() of TCP port number %hu failed "
                      "with errno %d (%s)", 
                      trialPortNum, errno, strerror(errno));
    }
    
    if (!*errorP && !foundPort)
        asprintfN(errorP, "Unable to find a free port.  Every TCP port "
                  "in the range 2048-16383 is in use");
}



void
CreateListeningSocket(int *         const socketP,
                      int *         const portNumP,
                      const char ** const errorP) {
/*----------------------------------------------------------------------------
   Create a TCP socket and bind it to the first unused port number we
   can find.

   Return as *socketP a file handle for the socket (on which Caller can
   listen()), and as *portNumP the TCP port number (to which Caller's
   partner can connect).
-----------------------------------------------------------------------------*/
    int rc;
    
    rc = socket(AF_INET, SOCK_STREAM, 0);
    if (rc < 0)
        asprintfN(errorP,
                  "Unable to create socket.  "
                  "socket() failed with errno %d (%s)",
                  errno, strerror(errno));
    else {
        int const socketFd = rc;

        unsigned short portNum;

        *socketP = socketFd;

        bindToUnusedPort(socketFd, &portNum, errorP);
        if (!*errorP) {
            int rc;

            *portNumP = portNum;

            /* would really like to wait for 1+numMachines machines,
              but this is max allowable, unfortunately
            */
            rc = listen(socketFd, SOMAXCONN);
            if (rc != 0)
                asprintfN(errorP, "Unable to listen on TCP socket.  "
                          "listen() fails with errno %d (%s)", 
                          errno, strerror(errno));
        }
        if (*errorP)
            close(socketFd);
    }
}



void
AcceptConnection(int           const listenSocketFd,
                 int *         const connectSocketFdP,
                 const char ** const errorP) {

    struct sockaddr otherSocket;
    socklenx_t      otherSize;
        /* This is an ugly dual-meaning variable.  As input to accept(),
           it is the storage size of 'otherSocket'.  As output, it is the 
           data length of 'otherSocket'.
        */
    int             rc;
    
    otherSize = sizeof(otherSocket);

    rc = accept(listenSocketFd, &otherSocket, &otherSize);

    if (rc < 0)
        asprintfN(errorP, "accept() failed with errno %d (%s).  ",
                  errno, strerror(errno));
    else {
        *connectSocketFdP = rc;
        *errorP = NULL;
    }
}
