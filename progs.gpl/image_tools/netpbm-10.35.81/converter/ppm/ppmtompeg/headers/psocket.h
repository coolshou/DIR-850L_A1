#ifndef PSOCKET_H_INCLUDED
#define PSOCKET_H_INCLUDED

#include <netdb.h>

void
ReadInt(int   const socketFd,
        int * const valueP);

void
ReadBytes(int             const fd,
          unsigned char * const buf,
          unsigned int    const nbyte);

void
WriteInt(int const socketFd,
         int const value);

void
WriteBytes(int             const fd,
           unsigned char * const buf,
           unsigned int    const nbyte);

void
ConnectToSocket(const char *      const machineName, 
                int               const portNum, 
                struct hostent ** const hostEnt,
                int *             const socketFdP,
                const char **     const errorP);

void
CreateListeningSocket(int *         const socketP,
                      int *         const portNumP,
                      const char ** const errorP);

void
AcceptConnection(int           const listenSocketFd,
                 int *         const connectSocketFdP,
                 const char ** const errorP);

#endif
