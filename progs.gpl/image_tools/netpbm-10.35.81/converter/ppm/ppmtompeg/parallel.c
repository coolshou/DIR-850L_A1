/*===========================================================================*
 * parallel.c              
 *                         
 *  Procedures to make encoder run in parallel   
 *                             
 *===========================================================================*/

/* COPYRIGHT INFORMATION IS AT THE END OF THIS FILE */


/*==============*
 * HEADER FILES *
 *==============*/

#define _XOPEN_SOURCE 500 /* Make sure stdio.h contains pclose() */
/* _ALL_SOURCE is needed on AIX to make the C library include the 
   socket services (e.g. define struct sockaddr) 

   Note that AIX standards.h actually sets feature declaration macros such
   as _XOPEN_SOURCE, unless they are already set.
*/
#define _ALL_SOURCE

/* On AIX, pm_config.h includes standards.h, which expects to be included
   after feature declaration macros such as _XOPEN_SOURCE.  So we include
   pm_config.h as late as possible.
*/


#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/times.h>

#include "mallocvar.h"
#include "nstring.h"

#include "pm.h"

#include "all.h"
#include "param.h"
#include "mpeg.h"
#include "prototypes.h"
#include "readframe.h"
#include "fsize.h"
#include "combine.h"
#include "frames.h"
#include "input.h"
#include "psocket.h"
#include "frametype.h"
#include "gethostname.h"

#include "parallel.h"


struct childState {
    boolean      finished;
    unsigned int startFrame;
    unsigned int numFrames;
    unsigned int lastNumFrames;
    unsigned int numSeconds;
    float        fps;
};


struct scheduler {
    /* This tracks the state of the subsystem that determines the assignments
       for the children
    */
    unsigned int nextFrame;
        /* The next frame that needs to be assigned to a child */
    unsigned int numFramesInJob;
        /* Total number of frames in the whole run of Ppmtompeg */
    unsigned int numMachines;
};



#define MAX_IO_SERVERS  10
#ifndef SOMAXCONN
#define SOMAXCONN 5
#endif

/*==================*
 * CONSTANTS        *
 *==================*/

#define TERMINATE_PID_SIGNAL    SIGTERM  /* signal used to terminate forked childs */
#ifndef MAXARGS
#define MAXARGS     1024   /* Max Number of arguments in safe_fork command */
#endif

/*==================*
 * STATIC VARIABLES *
 *==================*/

static char rsh[256];
static struct hostent *hostEntry = NULL;
static boolean  *frameDone;
static int  outputServerSocket;
static int  decodeServerSocket;
static boolean  parallelPerfect = FALSE;
static  int current_max_forked_pid=0;


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern int yuvHeight, yuvWidth;
extern char statFileName[256];
extern FILE *statFile;
extern boolean debugMachines;
extern boolean debugSockets;
int parallelTestFrames = 10;
int parallelTimeChunks = 60;
const char *IOhostName;
int ioPortNumber;
int decodePortNumber;
boolean niceProcesses = FALSE;
boolean forceIalign = FALSE;
int     machineNumber = -1;
boolean remoteIO = FALSE;
bool separateConversion;
    /* The I/O server will convert from the input format to the base format,
       and the slave will convert from the base format to the YUV internal
       format.  If false, the I/O server assumes the input format is the
       base format and converts from the base format to the YUV internal
       format; the slave does no conversion.
    */
time_t  IOtime = 0;
extern char encoder_name[];
int     ClientPid[MAX_MACHINES+4];


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/


static void PM_GNU_PRINTF_ATTR(1,2)
machineDebug(const char format[], ...) {

    va_list args;

    va_start(args, format);

    if (debugMachines) {
        const char * const hostname = GetHostName();
        fprintf(stderr, "%s: ---", hostname);
        strfree(hostname);
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
    }
    va_end(args);
}



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
TransmitPortNum(const char * const hostName, 
                int          const portNum, 
                int          const newPortNum) {
/*----------------------------------------------------------------------------
   Transmit the port number 'newPortNum' to the master on port 'portNum'
   of host 'hostName'.
-----------------------------------------------------------------------------*/
    int clientSocket;
    const char * error;
    
    ConnectToSocket(hostName, portNum, &hostEntry, &clientSocket, &error);
    
    if (error)
        errorExit("Can't connect in order to transmit port number.  %s",
                  error);

    WriteInt(clientSocket, newPortNum);
    
    close(clientSocket);
}



static void
readYUVDecoded(int          const socketFd,
               unsigned int const Fsize_x,
               unsigned int const Fsize_y,
               MpegFrame *  const frameP) {
    
    unsigned int y;
    
    for (y = 0; y < Fsize_y; ++y)         /* Y */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->decoded_y[y], Fsize_x);
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* U */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->decoded_cb[y], (Fsize_x >> 1));
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* V */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->decoded_cr[y], (Fsize_x >> 1));
}



static void
writeYUVDecoded(int          const socketFd,
                unsigned int const Fsize_x,
                unsigned int const Fsize_y,
                MpegFrame *  const frameP) {
    
    unsigned int y;
    
    for (y = 0; y < Fsize_y; ++y)         /* Y */
        WriteBytes(socketFd, 
                  (unsigned char *)frameP->decoded_y[y], Fsize_x);
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* U */
        WriteBytes(socketFd, 
                   (unsigned char *)frameP->decoded_cb[y], (Fsize_x >> 1));
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* V */
        WriteBytes(socketFd, 
                   (unsigned char *)frameP->decoded_cr[y], (Fsize_x >> 1));
}



static void
writeYUVOrig(int          const socketFd,
             unsigned int const Fsize_x,
             unsigned int const Fsize_y,
             MpegFrame *  const frameP) {
    
    unsigned int y;
    
    for (y = 0; y < Fsize_y; ++y)         /* Y */
        WriteBytes(socketFd, 
                  (unsigned char *)frameP->orig_y[y], Fsize_x);
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* U */
        WriteBytes(socketFd, 
                   (unsigned char *)frameP->orig_cb[y], (Fsize_x >> 1));
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* V */
        WriteBytes(socketFd, 
                   (unsigned char *)frameP->orig_cr[y], (Fsize_x >> 1));
}



static void
readYUVOrig(int          const socketFd,
            unsigned int const Fsize_x,
            unsigned int const Fsize_y,
            MpegFrame *  const frameP) {
    
    unsigned int y;
    
    for (y = 0; y < Fsize_y; ++y)         /* Y */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->orig_y[y], Fsize_x);
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* U */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->orig_cb[y], (Fsize_x >> 1));
    
    for (y = 0; y < (Fsize_y >> 1); ++y)  /* V */
        ReadBytes(socketFd, 
                  (unsigned char *)frameP->orig_cr[y], (Fsize_x >> 1));
}



/*===========================================================================*
 *
 * EndIOServer
 *
 *  called by the master process -- tells the I/O server to commit
 *  suicide
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
  EndIOServer()
{
  /* send signal to IO server:  -1 as frame number */
  GetRemoteFrame(NULL, -1);
}


/*===========================================================================*
 *
 * NotifyDecodeServerReady
 *
 *  called by a slave to the Decode Server to tell it a decoded frame
 *  is ready and waiting
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
NotifyDecodeServerReady(int const id) {

    int   clientSocket;
    time_t  tempTimeStart, tempTimeEnd;
    const char * error;
    
    time(&tempTimeStart);
    
    ConnectToSocket(IOhostName, decodePortNumber, &hostEntry, &clientSocket,
                    &error);
    
    if (error)
        errorExit("CHILD: Can't connect to decode server to tell it a frame "
                "is ready.  %s", error);
    
    WriteInt(clientSocket, id);
    
    close(clientSocket);
    
    time(&tempTimeEnd);
    IOtime += (tempTimeEnd-tempTimeStart);
}



/*===========================================================================*
 *
 * WaitForDecodedFrame
 *
 *  blah blah blah
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
  WaitForDecodedFrame(id)
int id;
{
  int const negativeTwo = -2;
  int   clientSocket;
  int     ready;
  const char * error;

  /* wait for a decoded frame */
  if ( debugSockets ) {
    fprintf(stdout, "WAITING FOR DECODED FRAME %d\n", id);
  }

  ConnectToSocket(IOhostName, decodePortNumber, &hostEntry, &clientSocket,
                  &error);

  if (error)
      errorExit("CHILD: Can't connect to decode server "
                "to get decoded frame.  %s",
                error);

  /* first, tell DecodeServer we're waiting for this frame */
  WriteInt(clientSocket, negativeTwo);

  WriteInt(clientSocket, id);

  ReadInt(clientSocket, &ready);

  if ( ! ready ) {
    int     waitSocket;
    int     waitPort;
    int     otherSock;
    const char * error;

    /* it's not ready; set up a connection and wait for decode server */
    CreateListeningSocket(&waitSocket, &waitPort, &error);
    if (error)
        errorExit("Unable to create socket on which to listen for "
                  "decoded frame.  %s", error);

    /* tell decode server where we are */
    WriteInt(clientSocket, machineNumber);

    WriteInt(clientSocket, waitPort);

    close(clientSocket);

    if ( debugSockets ) {
      fprintf(stdout, "SLAVE:  WAITING ON SOCKET %d\n", waitPort);
      fflush(stdout);
    }

    AcceptConnection(waitSocket, &otherSock, &error);
    if (error)
        errorExit("I/O SERVER: Failed to accept next connection.  %s", error);

    /* should we verify this is decode server? */
    /* for now, we won't */

    close(otherSock);

    close(waitSocket);
  } else {
    close(clientSocket);
  }

  if ( debugSockets ) {
    fprintf(stdout, "YE-HA FRAME %d IS NOW READY\n", id);
  }
}


/*===========================================================================*
 *
 * SendDecodedFrame
 *
 *  Send the frame to the decode server.
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
SendDecodedFrame(MpegFrame * const frameP) {
/*----------------------------------------------------------------------------
   Send frame *frameP to the decode server.
-----------------------------------------------------------------------------*/
    int const negativeTwo = -2;
    
    int clientSocket;
    const char * error;
    
    /* send to IOServer */
    ConnectToSocket(IOhostName, ioPortNumber, &hostEntry,
                    &clientSocket, &error);
    if (error)
        errorExit("CHILD: Can't connect to decode server to "
                  "give it a decoded frame.  %s", error);
    
    WriteInt(clientSocket, negativeTwo);
    
    WriteInt(clientSocket, frameP->id);

    writeYUVDecoded(clientSocket, Fsize_x, Fsize_y, frameP);
    
    close(clientSocket);
}


/*===========================================================================*
 *
 * GetRemoteDecodedFrame
 *
 *  get the decoded frame from the decode server.
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:   
 *
 *===========================================================================*/
void
GetRemoteDecodedRefFrame(MpegFrame * const frameP, 
                         int         const frameNumber) {
/*----------------------------------------------------------------------------
   Get decoded frame number 'frameNumber' *frameP from the decode server.
-----------------------------------------------------------------------------*/
  int const negativeThree = -3;
  int clientSocket;
  const char * error;

  /* send to IOServer */
  ConnectToSocket(IOhostName, ioPortNumber, &hostEntry,
                  &clientSocket, &error);
  if (error)
      errorExit("CHILD: Can't connect to decode server "
                "to get a decoded frame.  %s",
                error);

  /* ask IOServer for decoded frame */
  WriteInt(clientSocket, negativeThree);

  WriteInt(clientSocket, frameP->id);

  readYUVDecoded(clientSocket, Fsize_x, Fsize_y, frameP);

  close(clientSocket);
}


/*********
  routines handling forks, execs, PIDs and signals
  save, system-style forks
  apian@ise.fhg.de
  *******/


/*===========================================================================*
 *
 * cleanup_fork
 *
 *  Kill all the children, to be used when we get killed
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:   kills other processes
 *
 *===========================================================================*/
static void cleanup_fork( dummy )       /* try to kill all child processes */
     int dummy;
{
  register int i;
  for (i = 0;  i < current_max_forked_pid;  ++i ) {

#ifdef DEBUG_FORK
    fprintf(stderr, "cleanup_fork: killing PID %d\n", ClientPid[i]);
#endif

    if (kill(ClientPid[i], TERMINATE_PID_SIGNAL)) {
      fprintf(stderr, "cleanup_fork: killed PID=%d failed (errno %d)\n", 
          ClientPid[i], errno);
    }
  }
}

/*===========================================================================*
 *
 * safe_fork
 *
 *  fork a command
 *
 * RETURNS:     success/failure
 *
 * SIDE EFFECTS:   Fork the command, and save to PID so you can kil it later!
 *
 *===========================================================================*/
static int safe_fork(command)       /* fork child process and remember its PID */
     char *command;
{
  static int init=0;
  char *argis[MAXARGS];
  register int i=1;
  
  if (!(argis[0] = strtok(command, " \t"))) return(0); /* tokenize */
  while ((argis[i] = strtok(NULL, " \t")) && i < MAXARGS) ++i;
  argis[i] = NULL;
  
#ifdef DEBUG_FORK
  {register int i=0; 
   fprintf(stderr, "Command %s becomes:\n", command);
   while(argis[i]) {fprintf(stderr, "--%s--\n", argis[i]); ++i;} }
#endif
  
  if (!init) {          /* register clean-up routine */
    signal (SIGQUIT, cleanup_fork);
    signal (SIGTERM, cleanup_fork);
    signal (SIGINT , cleanup_fork);
    init=1;
  }
  
  if (-1 == (ClientPid[current_max_forked_pid] = fork()) )  {
    perror("safe_fork: fork failed ");
    return(-1);
  }
  if( !ClientPid[current_max_forked_pid]) { /* we are in child process */
    execvp(argis[0], argis );
    perror("safe_fork child: exec failed ");
    exit(1);
  }
#ifdef DEBUG_FORK
  fprintf(stderr, "parallel: forked PID=%d\n", ClientPid[current_max_forked_pid]);
#endif
  current_max_forked_pid++;
  return(0);
}


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

            /*=================*
             * IO SERVER STUFF *
             *=================*/


/*===========================================================================*
 *
 * SetIOConvert
 *
 *  sets the IO conversion to be separate or not.  If separate, then
 *  some post-processing is done at slave end
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
SetIOConvert(bool const separate) {
    separateConversion = separate;
}


/*===========================================================================*
 *
 * SetParallelPerfect
 *
 *  If this is called, then frames will be divided up completely, and
 *  evenly (modulo rounding) between all the processors
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    Sets parallelPerfect ....
 *
 *===========================================================================*/
void
SetParallelPerfect(val)
boolean val;
{
    parallelPerfect = val;
}


/*===========================================================================*
 *
 * SetRemoteShell
 *
 *  sets the remote shell program (usually rsh, but different on some
 *  machines)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
SetRemoteShell(const char * const shell) {
    strcpy(rsh, shell);
}


static void
decodedFrameToDisk(int const otherSock) {
/*----------------------------------------------------------------------------
  Get a decoded from from socket 'otherSock' and write it to disk.
-----------------------------------------------------------------------------*/
    int frameNumber;
    MpegFrame * frameP;

    ReadInt(otherSock, &frameNumber);
    
    if (debugSockets) {
        fprintf(stdout, "INPUT SERVER:  GETTING DECODED FRAME %d\n", 
                frameNumber);
        fflush(stdout);
    }

    /* should read frame from socket, then write to disk */
    frameP = Frame_New(frameNumber, 'i');
    
    Frame_AllocDecoded(frameP, TRUE);
    
    readYUVDecoded(otherSock, Fsize_x, Fsize_y, frameP);

    /* now output to disk */
    WriteDecodedFrame(frameP);

    Frame_Free(frameP);
}        



static void
decodedFrameFromDisk(int const otherSock) {

    /* request for decoded frame from disk */
            
    int frameNumber;
    MpegFrame * frameP;

    ReadInt(otherSock, &frameNumber);
    
    if (debugSockets) {
        fprintf(stdout, "INPUT SERVER:  READING DECODED FRAME %d "
                "from DISK\n", frameNumber);
        fflush(stdout);
    }

    /* should read frame from disk, then write to socket */
    frameP = Frame_New(frameNumber, 'i');
    
    Frame_AllocDecoded(frameP, TRUE);
    
    ReadDecodedRefFrame(frameP, frameNumber);
    
    writeYUVDecoded(otherSock, Fsize_x, Fsize_y, frameP);
    
    Frame_Free(frameP);
}        



static void
routeFromSocketToDisk(int              const otherSock,
                      unsigned char ** const bigBufferP,
                      unsigned int *   const bigBufferSizeP) {

    /* routing output frame from socket to disk */

    int frameNumber;
    int numBytes;
    unsigned char * bigBuffer;
    unsigned int bigBufferSize;
    const char * fileName;
    FILE * filePtr;

    bigBuffer     = *bigBufferP;
    bigBufferSize = *bigBufferSizeP;

    ReadInt(otherSock, &frameNumber);
    ReadInt(otherSock, &numBytes);

    /* Expand bigBuffer if necessary to fit this frame */
    if (numBytes > bigBufferSize) {
        bigBufferSize = numBytes;
        if (bigBuffer != NULL)
            free(bigBuffer);

        MALLOCARRAY_NOFAIL(bigBuffer, bigBufferSize);
    }
    
    /* now read in the bytes */
    ReadBytes(otherSock, bigBuffer, numBytes);
    
    /* open file to output this stuff to */
    asprintfN(&fileName, "%s.frame.%d", outputFileName, frameNumber);
    filePtr = fopen(fileName, "wb");

    if (filePtr == NULL)
        errorExit("I/O SERVER: Could not open output file(3):  %s", fileName);

    strfree(fileName);

    /* now write the bytes here */
    fwrite(bigBuffer, sizeof(char), numBytes, filePtr);
    
    fclose(filePtr);
    
    if (debugSockets) {
        fprintf(stdout, "====I/O SERVER:  WROTE FRAME %d to disk\n",
                frameNumber);
        fflush(stdout);
    }

    *bigBufferP     = bigBuffer;
    *bigBufferSizeP = bigBufferSize;
}        



static void
readConvertWriteToSocket(struct inputSource * const inputSourceP,
                         int                  const otherSock,
                         int                  const frameNumber,
                         bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   Get the frame numbered 'frameNumber' from input source
   *inputSourceP, apply format conversion User requested, and write
   the "base format" result to socket 'otherSock'.
-----------------------------------------------------------------------------*/
    FILE * convertedFileP;
    
    convertedFileP = ReadIOConvert(inputSourceP, frameNumber);
    if (convertedFileP) {
        bool eof;
        eof = FALSE;  /* initial value */
        while (!eof) {
            unsigned char buffer[1024];
            unsigned int numBytes;
            
            numBytes = fread(buffer, 1, sizeof(buffer), convertedFileP);
            
            if (numBytes > 0) {
                WriteInt(otherSock, numBytes);
                WriteBytes(otherSock, buffer, numBytes);
            } else
                eof = TRUE;
        }
        
        if (strcmp(ioConversion, "*") == 0 )
            fclose(convertedFileP);
        else
            pclose(convertedFileP);
        
        *endOfStreamP = FALSE;
    } else
        *endOfStreamP = TRUE;
}



static void
readWriteYuvToSocket(struct inputSource * const inputSourceP,
                     int                  const otherSock,
                     int                  const frameNumber,
                     bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   Read Frame number 'frameNumber' from the input source *inputSourceP,
   assuming it is in base format, and write its contents in YUV format
   to socket 'otherSock'.

   Wait for acknowledgement that consumer has received it.
-----------------------------------------------------------------------------*/
    MpegFrame * frameP;

    frameP = Frame_New(frameNumber, 'i');
    
    ReadFrame(frameP, inputSourceP, frameNumber, inputConversion,
              endOfStreamP);
    
    if (!*endOfStreamP) {
        writeYUVOrig(otherSock, Fsize_x, Fsize_y, frameP);
        
        {
            /* Make sure we don't leave until other processor read
               everything
            */
            int dummy;
            ReadInt(otherSock, &dummy);
            assert(dummy == 0);
        }
    }
    Frame_Free(frameP);
}



static void
readFrameWriteToSocket(struct inputSource * const inputSourceP,
                       int                  const otherSock,
                       int                  const frameNumber,
                       bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   Read Frame number 'frameNumber' from the input source *inputSourceP
   and write it to socket 'otherSock'.
-----------------------------------------------------------------------------*/
    if (debugSockets) {
        fprintf(stdout, "I/O SERVER GETTING FRAME %d\n", frameNumber);
        fflush(stdout);
    }

    if (separateConversion)
        readConvertWriteToSocket(inputSourceP, otherSock, frameNumber,
                                 endOfStreamP);
    else
        readWriteYuvToSocket(inputSourceP, otherSock, frameNumber,
                             endOfStreamP);

    if (debugSockets) {
        fprintf(stdout, "====I/O SERVER:  READ FRAME %d\n", frameNumber);
    }
}



static void
processNextConnection(int                  const serverSocket,
                      struct inputSource * const inputSourceP,
                      bool *               const doneP,
                      unsigned char **     const bigBufferP,
                      unsigned int *       const bigBufferSizeP) {

    int          otherSock;
    int          command;
    const char * error;
    
    AcceptConnection(serverSocket, &otherSock, &error);
    if (error)
        errorExit("I/O SERVER: Failed to accept next connection.  %s", error);

    ReadInt(otherSock, &command);

    switch (command) {
    case -1:
        *doneP = TRUE;
        break;
    case -2:
        decodedFrameToDisk(otherSock);
        break;
    case -3:
        decodedFrameFromDisk(otherSock);
        break;
    case -4:
        routeFromSocketToDisk(otherSock, bigBufferP, bigBufferSizeP);
        break;
    default: {
        unsigned int const frameNumber = command;

        bool endOfStream;

        readFrameWriteToSocket(inputSourceP, otherSock, frameNumber,
                               &endOfStream);

        if (endOfStream) {
            /* We don't do anything.  Closing the socket with having written
               anything is our signal that there is no more input.

               (Actually, Ppmtompeg cannot handle stream input in parallel
               mode -- this code is just infrastructure so maybe it can some
               day).
            */
        }
    }
    }
    close(otherSock);
}
 


void
IoServer(struct inputSource * const inputSourceP,
         const char *         const parallelHostName, 
         int                  const portNum) {
/*----------------------------------------------------------------------------
   Execute an I/O server.

   An I/O server is the partner on the master machine of a child process
   on a "remote" system.  "Remote" here doesn't just mean on another system.
   It means on a system that isn't even in the same cluster -- specifically,
   a system that doesn't have access to the same filesystem as the master.

   The child process passes frame contents between it and the master via
   the I/O server.
-----------------------------------------------------------------------------*/
    int       ioPortNum;
    int       serverSocket;
    boolean   done;
    unsigned char   *bigBuffer;
        /* A work buffer that we keep around permanently.  We increase
           its size as needed, but never shrink it.
        */
    unsigned int bigBufferSize;
        /* The current allocated size of bigBuffer[] */
    const char * error;

    bigBufferSize = 0;  /* Start with no buffer */
    bigBuffer = NULL;
    
    /* once we get IO port num, should transmit it to parallel server */

    CreateListeningSocket(&serverSocket, &ioPortNum, &error);
    if (error)
        errorExit("Unable to create socket on which to listen for "
                  "reports from children.  %s", error);

    if (debugSockets)
        fprintf(stdout, "====I/O USING PORT %d\n", ioPortNum);

    TransmitPortNum(parallelHostName, portNum, ioPortNum);

    if (separateConversion)
        SetFileType(ioConversion);  /* for reading */
    else
        SetFileType(inputConversion);

    done = FALSE;  /* initial value */

    while (!done)
        processNextConnection(serverSocket, inputSourceP,
                              &done, &bigBuffer, &bigBufferSize);

    close(serverSocket);

    if ( debugSockets ) {
        fprintf(stdout, "====I/O SERVER:  Shutting Down\n");
    }
}



/*===========================================================================*
 *
 * SendRemoteFrame
 *
 *  called by a slave to the I/O server; sends an encoded frame
 *  to the server to be sent to disk
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
SendRemoteFrame(int const frameNumber, BitBucket * const bb) {

    int const negativeFour = -4;
    int clientSocket;
    time_t  tempTimeStart, tempTimeEnd;
    const char * error;

    time(&tempTimeStart);

    ConnectToSocket(IOhostName, ioPortNumber, &hostEntry,
                    &clientSocket, &error);
    if (error)
        errorExit("CHILD: Can't connect to I/O server to deliver results.  %s",
                  error);

    WriteInt(clientSocket, negativeFour);

    WriteInt(clientSocket, frameNumber);
    
    if (frameNumber != -1) {
        /* send number of bytes */
        
        WriteInt(clientSocket, (bb->totalbits+7)>>3);
    
        /* now send the bytes themselves */
        Bitio_WriteToSocket(bb, clientSocket);
    }

    close(clientSocket);

    time(&tempTimeEnd);
    IOtime += (tempTimeEnd-tempTimeStart);
}



void
GetRemoteFrame(MpegFrame * const frameP, 
               int         const frameNumber) {
/*----------------------------------------------------------------------------
   Get a frame from the I/O server.
   
   This is intended for use by a child.
-----------------------------------------------------------------------------*/
    int           clientSocket;
    const char * error;

    Fsize_Note(frameNumber, yuvWidth, yuvHeight);

    if (debugSockets) {
        fprintf(stdout, "MACHINE %s REQUESTING connection for FRAME %d\n",
                getenv("HOST"), frameNumber);
        fflush(stdout);
    }

    ConnectToSocket(IOhostName, ioPortNumber, &hostEntry,
                    &clientSocket, &error);

    if (error)
        errorExit("CHILD: Can't connect to I/O server to get a frame.  %s", 
                  error);

    WriteInt(clientSocket, frameNumber);

    if (frameNumber != -1) {
        if (separateConversion) {
            unsigned char buffer[1024];
            /* This is by design the exact size of the data per message (except
               the last message for a frame) the I/O server sends.
            */
            int numBytes;  /* Number of data bytes in message */
            FILE * filePtr = pm_tmpfile();

            /* read in stuff, write to file, perform local conversion */
            do {
                ReadInt(clientSocket, &numBytes);

                if (numBytes > sizeof(buffer))
                    errorExit("Invalid message received: numBytes = %d, "
                              "which is greater than %d\n", 
                              numBytes, sizeof(numBytes));
                ReadBytes(clientSocket, buffer, numBytes);

                fwrite(buffer, 1, numBytes, filePtr);
            } while ( numBytes == sizeof(buffer) );
            fflush(filePtr);
            {
                bool endOfStream;
                rewind(filePtr);
                /* I/O Server gave us base format.  Read it as an MpegFrame */
                ReadFrameFile(frameP, filePtr, slaveConversion, &endOfStream);
                assert(!endOfStream);
            }
            fclose(filePtr);
        } else {
            Frame_AllocYCC(frameP);

            if (debugSockets) {
                fprintf(stdout, "MACHINE %s allocated YCC FRAME %d\n",
                        getenv("HOST"), frameNumber);
                fflush(stdout);
            }
            /* I/O Server gave us internal YUV format.  Read it as MpegFrame */
            readYUVOrig(clientSocket, yuvWidth, yuvHeight, frameP);
        }
    }

    WriteInt(clientSocket, 0);

    close(clientSocket);

    if (debugSockets) {
        fprintf(stdout, "MACHINE %s READ COMPLETELY FRAME %d\n",
                getenv("HOST"), frameNumber);
        fflush(stdout);
    }
}


struct combineControl {
    unsigned int numFrames;
};




static void
getAndProcessACombineConnection(int const outputServerSocket) {
    int          otherSock;
    int          command;
    const char * error;

    AcceptConnection(outputServerSocket, &otherSock, &error);

    if (error)
        errorExit("COMBINE SERVER: "
                  "Failed to accept next connection.  %s", error);
    
    ReadInt(otherSock, &command);
    
    if (command == -2) {
        /* this is notification from non-remote process that a
           frame is done.
            */
        int frameStart, frameEnd;

        ReadInt(otherSock, &frameStart);
        ReadInt(otherSock, &frameEnd);
            
        machineDebug("COMBINE_SERVER: Frames %d - %d done",
                     frameStart, frameEnd);
        {
            unsigned int i;
            for (i = frameStart; i <= frameEnd; ++i)
                frameDone[i] = TRUE;
        }
    } else
        errorExit("COMBINE SERVER: Unrecognized command %d received.",
                  command);

    close(otherSock);
}



#define READ_ATTEMPTS 5 /* number of times (seconds) to retry an input file */


static void
openInputFile(const char * const fileName,
              FILE **      const inputFilePP) {

    FILE * inputFileP;
    unsigned int attempts;
    
    inputFileP = NULL;
    attempts = 0;

    while (!inputFileP && attempts < READ_ATTEMPTS) {
        inputFileP = fopen(fileName, "rb");
        if (inputFileP == NULL) {
            pm_message("ERROR  Couldn't read frame file '%s' errno = %d (%s)"
                       "attempt %d", 
                       fileName, errno, strerror(errno), attempts);
            sleep(1);
        }
        ++attempts;
    }
    if (inputFileP == NULL)
        pm_error("Unable to open file '%s' after %d attempts.", 
                 fileName, attempts);

    *inputFilePP = inputFileP;
}



static void
waitForOutputFile(void *        const inputHandle,
                  unsigned int  const frameNumber,
                  FILE **       const ifPP) {
/*----------------------------------------------------------------------------
   Keep handling output events until we get the specified frame number.
   Open the file it's in and return the stream handle.
-----------------------------------------------------------------------------*/
    struct combineControl * const combineControlP = (struct combineControl *)
        inputHandle;

    if (frameNumber >= combineControlP->numFrames)
        *ifPP = NULL;
    else {
        const char * fileName;

        while (!frameDone[frameNumber]) {
            machineDebug("COMBINE_SERVER: Waiting for frame %u done", 
                         frameNumber);

            getAndProcessACombineConnection(outputServerSocket);
        }
        machineDebug("COMBINE SERVER: Wait for frame %u over", frameNumber);

        asprintfN(&fileName, "%s.frame.%u", outputFileName, frameNumber);

        openInputFile(fileName, ifPP);

        strfree(fileName);
    }
}



static void
unlinkFile(void *       const inputHandle,
           unsigned int const frameNumber) {

    if (!keepTempFiles) {
        const char * fileName;

        asprintfN(&fileName, "%s.frame.%u", outputFileName, frameNumber);

        unlink(fileName);

        strfree(fileName);
    }
}



void
CombineServer(int          const numFrames, 
              const char * const masterHostName, 
              int          const masterPortNum,
              const char * const outputFileName) {
/*----------------------------------------------------------------------------
   Execute a combine server.

   This handles combination of frames.
-----------------------------------------------------------------------------*/
  int    combinePortNum;
  FILE * ofP;
  const char * error;
  struct combineControl combineControl;
  
  /* once we get Combine port num, should transmit it to parallel server */
  
  CreateListeningSocket(&outputServerSocket, &combinePortNum, &error);
  if (error)
      errorExit("Unable to create socket on which to listen.  %s", error);

  machineDebug("COMBINE SERVER: LISTENING ON PORT %d", combinePortNum);
  
  TransmitPortNum(masterHostName, masterPortNum, combinePortNum);
  
  MALLOCARRAY_NOFAIL(frameDone, numFrames);
  {
      unsigned int i;
      for (i = 0; i < numFrames; ++i)
          frameDone[i] = FALSE;
  }
  ofP = pm_openw(outputFileName);
  
  combineControl.numFrames = numFrames;

  FramesToMPEG(ofP, &combineControl, &waitForOutputFile, &unlinkFile);

  machineDebug("COMBINE SERVER: Shutting down");
  
  /* tell Master server we are done */
  TransmitPortNum(masterHostName, masterPortNum, combinePortNum);
  
  close(outputServerSocket);

  fclose(ofP);
}


/*=====================*
 * MASTER SERVER STUFF *
 *=====================*/


static void
startCombineServer(const char * const encoderName,
                   unsigned int const numMachines,
                   const char * const masterHostName,
                   int          const masterPortNum,
                   unsigned int const numInputFiles,
                   const char * const paramFileName,
                   int          const masterSocket,
                   int *        const combinePortNumP) {

    char         command[1024];
    int          otherSock;
    const char * error;

    snprintf(command, sizeof(command), 
             "%s %s -max_machines %d -output_server %s %d %d %s",
             encoderName, 
             debugMachines ? "-debug_machines" : "",
             numMachines, masterHostName, masterPortNum, 
             numInputFiles, paramFileName);

    machineDebug("MASTER: Starting combine server with shell command '%s'",
                 command);

    safe_fork(command);
    
    machineDebug("MASTER: Listening for connection back from "
                 "new Combine server");

    AcceptConnection(masterSocket, &otherSock, &error);
    if (error)
        errorExit("MASTER SERVER: "
                  "Failed to accept next connection.  %s", error);

    ReadInt(otherSock, combinePortNumP);
    close(otherSock);

    machineDebug("MASTER:  Combine port number = %d", *combinePortNumP);
}



static void
startDecodeServer(const char * const encoderName,
                  unsigned int const numMachines,
                  const char * const masterHostName,
                  int          const masterPortNum,
                  unsigned int const numInputFiles,
                  const char * const paramFileName,
                  int          const masterSocket,
                  int *        const decodePortNumP) {

    char         command[1024];
    int          otherSock;
    const char * error;

    snprintf(command, sizeof(command), 
             "%s %s -max_machines %d -decode_server %s %d %d %s",
             encoder_name, 
             debugMachines ? "-debug_machines" : "",
             numMachines, masterHostName, masterPortNum,
             numInputFiles, paramFileName);

    machineDebug("MASTER: Starting decode server with shell command '%s'",
                 command);

    safe_fork(command);

    machineDebug("MASTER: Listening for connection back from "
                 "new Decode server");

    AcceptConnection(masterSocket, &otherSock, &error);
    if (error)
        errorExit("MASTER SERVER: "
                  "Failed to accept connection back from the new "
                  "decode server.  %s", error);

    ReadInt(otherSock, decodePortNumP);
    
    close(otherSock);
    
    machineDebug("MASTER:  Decode port number = %d", *decodePortNumP);
}



static void
startIoServer(const char *   const encoderName,
              unsigned int   const numChildren,
              const char *   const masterHostName,
              int            const masterPortNum,
              int            const masterSocket,
              const char *   const paramFileName,
              int *          const ioPortNumP) {
              
    char         command[1024];
    int          otherSock;
    const char * error;
    
    sprintf(command, "%s -max_machines %d -io_server %s %d %s",
            encoderName, numChildren, masterHostName, masterPortNum,
            paramFileName);

    machineDebug("MASTER: Starting I/O server with remote shell command '%s'",
                 command);

    safe_fork(command);
    
    machineDebug("MASTER: Listening for connection back from "
                 "new I/O server");

    AcceptConnection(masterSocket, &otherSock, &error);
    if (error)
        errorExit("MASTER SERVER: "
                  "Failed to accept connection back from the new "
                  "I/O server.  %s", error);
    
    ReadInt(otherSock, ioPortNumP);
    close(otherSock);
    
    machineDebug("MASTER:  I/O port number = %d", *ioPortNumP);
}    
    
    

static void
extendToEndOfPattern(unsigned int * const nFramesP,
                     unsigned int   const startFrame,
                     unsigned int   const framePatternLen,
                     unsigned int   const numFramesInStream) {

    assert(framePatternLen >= 1);
        
    while (startFrame + *nFramesP < numFramesInStream &&
           (startFrame + *nFramesP) % framePatternLen != 0)
        ++(*nFramesP);
}



static void
allocateInitialFrames(struct scheduler * const schedulerP,
                      boolean            const parallelPerfect,
                      boolean            const forceIalign,
                      unsigned int       const framePatternLen,
                      unsigned int       const parallelTestFrames,
                      unsigned int       const childNum,
                      unsigned int *     const startFrameP,
                      unsigned int *     const nFramesP) {
/*----------------------------------------------------------------------------
   Choose which frames, to hand out to the new child numbered 'childNum'.
-----------------------------------------------------------------------------*/
    unsigned int const framesPerChild = 
        MAX(1, ((schedulerP->numFramesInJob - schedulerP->nextFrame) /
                (schedulerP->numMachines - childNum)));

    unsigned int nFrames;

    if (parallelPerfect)
        nFrames = framesPerChild;
    else {
        assert(parallelTestFrames >= 1);

        nFrames = MIN(parallelTestFrames, framesPerChild);
    }
    if (forceIalign)
        extendToEndOfPattern(&nFrames, schedulerP->nextFrame,
                             framePatternLen, schedulerP->numFramesInJob);

    nFrames = MIN(nFrames, schedulerP->numFramesInJob - schedulerP->nextFrame);

    *startFrameP = schedulerP->nextFrame;
    *nFramesP = nFrames;
    schedulerP->nextFrame += nFrames;
}



static float
taperedGoalTime(struct childState const childState[],
                unsigned int      const remainingFrameCount) {
        
    float        goalTime;
    float        allChildrenFPS;
    float        remainingJobTime;
        /* How long we expect it to be before the whole movie is encoded*/
    float        sum;
    int          numMachinesToEstimate;
    unsigned int childNum;
    
    /* frames left = lastFrameInStream - startFrame + 1 */
    for (childNum = 0, sum = 0.0, numMachinesToEstimate = 0; 
         childNum < numMachines; ++childNum) {
        if (!childState[childNum].finished) {
            if (childState[childNum].fps < 0.0 )
                ++numMachinesToEstimate;
            else
                sum += childState[childNum].fps;
        }
    }
    
    allChildrenFPS = (float)numMachines *
        (sum/(float)(numMachines-numMachinesToEstimate));
    
    remainingJobTime = (float)remainingFrameCount/allChildrenFPS;
    
    goalTime = MAX(5.0, remainingJobTime/2);

    return goalTime;
}



static void
allocateMoreFrames(struct scheduler * const schedulerP,
                   unsigned int       const childNum,
                   struct childState  const childState[],
                   bool               const forceIalign,
                   unsigned int       const framePatternLen,
                   bool               const goalTimeSpecified,
                   unsigned int       const goalTimeArg,
                   unsigned int *     const startFrameP,
                   unsigned int *     const nFramesP) {
/*----------------------------------------------------------------------------
   Decide which frames should be child 'childNum''s next assignment,
   given the state/history of all children is childState[].

   The lowest numbered frame which needs yet to be encoded is frame
   number 'startFrame' and 'lastFrameInStream' is the highest.

   The allocation always starts at the lowest numbered frame that
   hasn't yet been allocated and is sequential.  We return as
   *startFrameP the frame number of the first frame in the allocation
   and as *nFramesP the number of frames.

   If 'goalTimeSpecified' is true, we try to make the assignment take
   'goalTimeArg' seconds.  If 'goalTimeSpecified' is not true, we choose
   a goal time ourselves, which is based on how long we think it will
   take for all the children to finish all the remaining frames.
-----------------------------------------------------------------------------*/
    float goalTime;
        /* Number of seconds we want the assignment to take.  We size the
           assignment to try to meet this goal.
        */
    unsigned int nFrames;
    float avgFps;

    if (!goalTimeSpecified) {
        goalTime = taperedGoalTime(childState,
                                   schedulerP->numFramesInJob - 
                                   schedulerP->nextFrame);
    
        pm_message("MASTER: ASSIGNING %s %.2f seconds of work",
                   machineName[childNum], goalTime);
    } else
        goalTime = goalTimeArg;
    
    if (childState[childNum].numSeconds != 0)
        avgFps = (float)childState[childNum].numFrames / 
            childState[childNum].numSeconds;
    else
        avgFps = 0.1;       /* arbitrary small value */

    nFrames = MAX(1u, (unsigned int)(goalTime * avgFps + 0.5));
    
    nFrames = MIN(nFrames, 
                  schedulerP->numFramesInJob - schedulerP->nextFrame);

    if (forceIalign)
        extendToEndOfPattern(&nFrames, schedulerP->nextFrame,
                             framePatternLen, schedulerP->numFramesInJob);

    *startFrameP = schedulerP->nextFrame;
    *nFramesP = nFrames;
    schedulerP->nextFrame += nFrames;
}



static void
startChildren(struct scheduler *   const schedulerP,
              const char *         const encoderName,
              const char *         const masterHostName,
              int                  const masterPortNum,
              const char *         const paramFileName,
              boolean              const parallelPerfect,
              boolean              const forceIalign,
              unsigned int         const framePatternLen,
              unsigned int         const parallelTestFrames,
              boolean              const beNice,
              int                  const masterSocket,
              int                  const combinePortNum,
              int                  const decodePortNum,
              int *                const ioPortNum,
              unsigned int *       const numIoServersP,
              struct childState ** const childStateP) {
/*----------------------------------------------------------------------------
   Start up the children.  Tell them to work for the master at
   'masterHostName':'masterPortNum'.

   Start I/O servers (as processes on this system) as required and return
   the port numbers of the TCP ports on which they listen as
   ioPortNum[] and the number of them as *numIoServersP.

   Give each of the children some initial work to do.  This may be just
   a small amount for timing purposes.

   We access and manipulate the various global variables that represent
   the state of the children, and the scheduler structure.
-----------------------------------------------------------------------------*/
    struct childState * childState;  /* malloc'ed */
    unsigned int childNum;
    unsigned int numIoServers;
    unsigned int childrenLeftCurrentIoServer;
        /* The number of additional children we can hook up to the
           current I/O server before reaching our maximum children per
           I/O server.  0 if there is no current I/O server.
        */

    MALLOCARRAY_NOFAIL(childState, schedulerP->numMachines);

    childrenLeftCurrentIoServer = 0;  /* No current I/O server yet */
    
    numIoServers = 0;  /* None created yet */

    for (childNum = 0; childNum < schedulerP->numMachines; ++childNum) {
        char command[1024];
        unsigned int startFrame;
        unsigned int nFrames;

        childState[childNum].fps        = -1.0;  /* illegal value as flag */
        childState[childNum].numSeconds = 0;

        allocateInitialFrames(schedulerP, parallelPerfect, forceIalign,
                              framePatternLen, parallelTestFrames,
                              childNum, &startFrame, &nFrames);

        if (nFrames == 0) {
            childState[childNum].finished = TRUE;
            machineDebug("MASTER: No more frames; not starting child '%s'",
                         machineName[childNum]);
        } else {
            childState[childNum].finished   = FALSE;
        
            if (remote[childNum]) {
                if (childrenLeftCurrentIoServer == 0) {
                    startIoServer(encoderName, schedulerP->numMachines, 
                                  masterHostName, masterPortNum, masterSocket,
                                  paramFileName, &ioPortNum[numIoServers++]);
                    
                    childrenLeftCurrentIoServer = SOMAXCONN;
                }
                --childrenLeftCurrentIoServer;
            } 
            snprintf(command, sizeof(command),
                     "%s %s -l %s %s "
                     "%s %s -child %s %d %d %d %d %d %d "
                     "-frames %d %d %s",
                     rsh,
                     machineName[childNum], userName[childNum],
                     beNice ? "nice" : "",
                     executable[childNum],
                     debugMachines ? "-debug_machines" : "",
                     masterHostName, masterPortNum, 
                     remote[childNum] ? ioPortNum[numIoServers-1] : 0,
                     combinePortNum, decodePortNum, childNum,
                     remote[childNum] ? 1 : 0,
                     startFrame, startFrame + nFrames - 1,
                     remote[childNum] ? 
                         remoteParamFile[childNum] : paramFileName
            );
        
            machineDebug("MASTER: Starting child server "
                         "with shell command '%s'", command);

            safe_fork(command);

            machineDebug("MASTER: Frames %d-%d assigned to new child %s",
                         startFrame, startFrame + nFrames - 1, 
                         machineName[childNum]);
        }
        childState[childNum].startFrame = startFrame;
        childState[childNum].lastNumFrames = nFrames;
        childState[childNum].numFrames = childState[childNum].lastNumFrames;
    }
    *childStateP   = childState;
    *numIoServersP = numIoServers;
}



static void
noteFrameDone(const char * const combineHostName,
              int          const combinePortNum,
              unsigned int const frameStart, 
              unsigned int const frameEnd) {
/*----------------------------------------------------------------------------
   Tell the Combine server that frames 'frameStart' through 'frameEnd'
   are done.
-----------------------------------------------------------------------------*/
    int const negativeTwo = -2;
    int clientSocket;
    time_t  tempTimeStart, tempTimeEnd;
    const char * error;
    struct hostent * hostEntP;

    time(&tempTimeStart);

    hostEntP = NULL;

    ConnectToSocket(combineHostName, combinePortNum, &hostEntP,
                    &clientSocket, &error);
    
    if (error)
        errorExit("MASTER: Can't connect to Combine server to tell it frames "
                  "are done.  %s", error);

    WriteInt(clientSocket, negativeTwo);

    WriteInt(clientSocket, frameStart);

    WriteInt(clientSocket, frameEnd);

    close(clientSocket);

    time(&tempTimeEnd);
    IOtime += (tempTimeEnd-tempTimeStart);
}



static void
feedTheChildren(struct scheduler * const schedulerP,
                struct childState        childState[],
                int                const masterSocket,
                const char *       const combineHostName,
                int                const combinePortNum,
                bool               const forceIalign,
                unsigned int       const framePatternLen,
                bool               const goalTimeSpecified,
                unsigned int       const goalTime) {
/*----------------------------------------------------------------------------
   Listen for children to tell us they have finished their assignments
   and give them new assignments, until all the frames have been assigned
   and all the children have finished.

   As children finish assignments, inform the combine server at
   'combineHostName':'combinePortNum' of such.

   Note that the children got initial assigments when they were created.
   So the first thing we do is wait for them to finish those.
-----------------------------------------------------------------------------*/
    unsigned int numFinished;
        /* Number of child machines that have been excused because there
           is no more work for them.
        */
    unsigned int framesDone;

    numFinished = 0;
    framesDone = 0;

    while (numFinished != schedulerP->numMachines) {
        int                 otherSock;
        int                 childNum;
        int                 seconds;
        float               framesPerSecond;
        struct childState * csP;
        const char *        error;
        unsigned int nextFrame;
        unsigned int nFrames;

        machineDebug("MASTER: Listening for a connection...");

        AcceptConnection(masterSocket, &otherSock, &error);
        if (error)
            errorExit("MASTER SERVER: "
                      "Failed to accept next connection.  %s", error);

        ReadInt(otherSock, &childNum);
        ReadInt(otherSock, &seconds);

        csP = &childState[childNum];
        
        csP->numSeconds += seconds;
        csP->fps = (float)csP->numFrames / (float)csP->numSeconds;

        if (seconds != 0)
            framesPerSecond = (float)csP->lastNumFrames / (float)seconds;
        else
            framesPerSecond = (float)csP->lastNumFrames * 2.0;

        machineDebug("MASTER: Child %s FINISHED ASSIGNMENT.  "
                     "%f frames per second", 
                     machineName[childNum], framesPerSecond);

        noteFrameDone(combineHostName, combinePortNum, csP->startFrame,
                      csP->startFrame + csP->lastNumFrames - 1);

        framesDone += csP->lastNumFrames;

        allocateMoreFrames(schedulerP, childNum, childState,
                           forceIalign, framePatternLen,
                           goalTimeSpecified, goalTime,
                           &nextFrame, &nFrames);

        if (nFrames == 0) {
            WriteInt(otherSock, -1);
            WriteInt(otherSock, 0);

            ++numFinished;

            machineDebug("MASTER: NO MORE WORK FOR CHILD %s.  "
                         "(%d of %d children now done)",
                         machineName[childNum], numFinished, numMachines);
        } else {
            WriteInt(otherSock, nextFrame);
            WriteInt(otherSock, nextFrame + nFrames - 1);

            machineDebug("MASTER: Frames %d-%d assigned to child %s",
                         nextFrame, nextFrame + nFrames - 1,
                         machineName[childNum]);

            csP->startFrame    = nextFrame;
            csP->lastNumFrames = nFrames;
            csP->numFrames    += csP->lastNumFrames;
        }
        close(otherSock);

        machineDebug("MASTER: %d/%d DONE; %d ARE ASSIGNED",
                     framesDone, schedulerP->numFramesInJob, 
                     schedulerP->nextFrame - framesDone);
    }
}



static void
stopIoServers(const char * const hostName,
              int          const ioPortNum[],
              unsigned int const numIoServers) {

    unsigned int childNum;

    IOhostName = hostName;
    for (childNum = 0; childNum < numIoServers; ++childNum) {
        ioPortNumber = ioPortNum[childNum];
        EndIOServer();
    }
}



static void
waitForCombineServerToTerminate(int const masterSocket) {

    int otherSock;
    const char * error;

    machineDebug("MASTER SERVER: Waiting for combine server to terminate");

    AcceptConnection(masterSocket, &otherSock, &error);
    if (error)
        errorExit("MASTER SERVER: "
                  "Failed to accept connection expected from a "
                  "terminating combine server.  %s", error);

    {
        int dummy;
        ReadInt(otherSock, &dummy);
    }
    close(otherSock);
}
    


static void
printFinalStats(FILE *            const statfileP,
                time_t            const startUpBegin,
                time_t            const startUpEnd,
                time_t            const shutDownBegin,
                time_t            const shutDownEnd,
                unsigned int      const numChildren,
                struct childState const childState[],
                unsigned int      const numFrames) {

    unsigned int pass;
    FILE * fileP;

    for (pass = 0; pass < 2; ++pass) {
        if (pass == 0)
            fileP = stdout;
        else
            fileP = statfileP;

        if (fileP) {
            unsigned int childNum;
            float totalFPS;

            fprintf(fileP, "\n\n");
            fprintf(fileP, "PARALLEL SUMMARY\n");
            fprintf(fileP, "----------------\n");
            fprintf(fileP, "\n");
            fprintf(fileP, "START UP TIME:  %u seconds\n",
                    (unsigned int)(startUpEnd - startUpBegin));
            fprintf(fileP, "SHUT DOWN TIME:  %u seconds\n",
                    (unsigned int)(shutDownEnd - shutDownBegin));
            
            fprintf(fileP, 
                    "%14.14s %8.8s %8.8s %12.12s %9.9s\n",
                    "MACHINE", "Frames", "Seconds", "Frames/Sec",
                    "Self Time");

            fprintf(fileP, 
                    "%14.14s %8.8s %8.8s %12.12s %9.9s\n",
                    "--------------", "--------", "--------", "------------",
                    "---------");

            totalFPS = 0.0;
            for (childNum = 0; childNum < numChildren; ++childNum) {
                float const localFPS = 
                    (float)childState[childNum].numFrames /
                    childState[childNum].numSeconds;
                fprintf(fileP, "%14.14s %8u %8u %12.4f %8u\n",
                        machineName[childNum], 
                        childState[childNum].numFrames, 
                        childState[childNum].numSeconds,
                        localFPS, 
                        (unsigned int)((float)numFrames/localFPS));
                totalFPS += localFPS;
            }

            fprintf(fileP, 
                    "%14.14s %8.8s %8.8s %12.12s %9.9s\n",
                    "--------------", "--------", "--------", "------------",
                    "---------");

            fprintf(fileP, "%14s %8.8s %8u %12.4f\n", 
                    "OPTIMAL", "", 
                    (unsigned int)((float)numFrames/totalFPS),
                    totalFPS);
            
            {
                unsigned int const diffTime = shutDownEnd - startUpBegin;
                
                fprintf(fileP, "%14s %8.8s %8u %12.4f\n", 
                        "ACTUAL", "", diffTime, 
                        (float)numFrames / diffTime);
            }
            fprintf(fileP, "\n\n");
        }
    }
}



void
MasterServer(struct inputSource * const inputSourceP,
             const char *         const paramFileName, 
             const char *         const outputFileName) {
/*----------------------------------------------------------------------------
   Execute the master server function.

   Start all the other servers.
-----------------------------------------------------------------------------*/
    const char *hostName;
    int       portNum;
    int       masterSocket;
        /* The file descriptor for the socket on which the master listens */
    int ioPortNum[MAX_IO_SERVERS];
    int       combinePortNum, decodePortNum;
    struct childState * childState;  /* malloc'ed */
    unsigned int numIoServers;
    time_t  startUpBegin, startUpEnd;
    time_t  shutDownBegin, shutDownEnd;
    const char * error;
    struct scheduler scheduler;

    time(&startUpBegin);

    scheduler.nextFrame = 0;
    scheduler.numFramesInJob = inputSourceP->numInputFiles;
    scheduler.numMachines = numMachines;

    PrintStartStats(startUpBegin, FALSE, 0, 0, inputSourceP);

    hostName = GetHostName();

    hostEntry = gethostbyname(hostName);
    if (hostEntry == NULL)
        errorExit("Could not find host name '%s' in database", hostName);

    CreateListeningSocket(&masterSocket, &portNum, &error);
    if (error)
        errorExit("Unable to create socket on which to listen.  %s", error);

    if (debugSockets)
        fprintf(stdout, "---MASTER USING PORT %d\n", portNum);

    startCombineServer(encoder_name, numMachines, hostName, portNum,
                       inputSourceP->numInputFiles, 
                       paramFileName, masterSocket, 
                       &combinePortNum);

    if (referenceFrame == DECODED_FRAME)
        startDecodeServer(encoder_name, numMachines, hostName, portNum,
                          inputSourceP->numInputFiles, 
                          paramFileName, masterSocket,
                          &decodePortNum);

    startChildren(&scheduler, encoder_name, hostName, portNum,
                  paramFileName, parallelPerfect, forceIalign,
                  framePatternLen, parallelTestFrames, 
                  niceProcesses,
                  masterSocket, combinePortNum, decodePortNum, 
                  ioPortNum, &numIoServers,
                  &childState);

    time(&startUpEnd);

    feedTheChildren(&scheduler, childState,
                    masterSocket, hostName, combinePortNum,
                    forceIalign, framePatternLen,
                    parallelTimeChunks != -1, parallelTimeChunks);

    assert(scheduler.nextFrame == scheduler.numFramesInJob);

    time(&shutDownBegin);

    stopIoServers(hostName, ioPortNum, numIoServers);

    waitForCombineServerToTerminate(masterSocket);

    close(masterSocket);

    time(&shutDownEnd);

    printFinalStats(statFile, startUpBegin, startUpEnd,
                    shutDownBegin, shutDownEnd, numMachines,
                    childState, inputSourceP->numInputFiles);

    if (statFile)
        fclose(statFile);

    free(childState);
    strfree(hostName);
}



void
NotifyMasterDone(const char * const masterHostName, 
                 int          const masterPortNum, 
                 int          const childNum,
                 unsigned int const seconds, 
                 boolean *    const moreWorkToDoP,
                 int *        const nextFrameStartP,
                 int *        const nextFrameEndP) {
/*----------------------------------------------------------------------------
   Tell the master, at 'masterHostName':'masterPortNum' that child
   number 'childNum' has finished its assignment, and the decoding
   took 'seconds' wall clock seconds.  Get the next assignment, if
   any, from the master.

   If the master gives us a new assignment, return *moreWorkToDoP ==
   TRUE and the frames the master wants us to do as *nextFrameStartP
   and nextFrameEndP.  Otherwise (there is no more work for machine
   'childNum' to do), return *moreWorkToDoP == FALSE.
-----------------------------------------------------------------------------*/
    int    clientSocket;
    time_t tempTimeStart, tempTimeEnd;
    const char * error;

    machineDebug("CHILD: NOTIFYING MASTER Machine %d assignment complete", 
                 childNum);

    time(&tempTimeStart);
    
    ConnectToSocket(masterHostName, masterPortNum, &hostEntry,
                    &clientSocket, &error);
    if (error)
        errorExit("CHILD: Can't connect to master to tell him we've finished "
                  "our assignment.  %s", error);

    WriteInt(clientSocket, childNum);
    WriteInt(clientSocket, seconds);

    ReadInt(clientSocket, nextFrameStartP);
    ReadInt(clientSocket, nextFrameEndP);

    *moreWorkToDoP = (*nextFrameStartP >= 0);

    if (*moreWorkToDoP)
        machineDebug("CHILD: Master says next assignment: start %d end %d",
                     *nextFrameStartP, *nextFrameEndP);
    else
        machineDebug("CHILD: Master says no more work for us.");

    close(clientSocket);

    time(&tempTimeEnd);
    IOtime += (tempTimeEnd-tempTimeStart);
}



void
DecodeServer(int          const numInputFiles, 
             const char * const decodeFileName, 
             const char * const masterHostName, 
             int          const masterPortNum) {
/*----------------------------------------------------------------------------
   Execute the decode server.

   The decode server handles transfer of decoded frames to/from processes.

   It is necessary only if referenceFrame == DECODED_FRAME.

   Communicate to the master at hostname 'masterHostName':'masterPortNum'.

-----------------------------------------------------------------------------*/
    int     otherSock;
    int     decodePortNum;
    int     frameReady;
    boolean *ready;
    int     *waitMachine;
    int     *waitPort;
    int     *waitList;
    int     slaveNumber;
    int     slavePort;
    int     waitPtr;
    struct hostent *nullHost = NULL;
    int     clientSocket;
    const char * error;

    /* should keep list of port numbers to notify when frames become ready */

    ready = (boolean *) calloc(numInputFiles, sizeof(boolean));
    waitMachine = (int *) calloc(numInputFiles, sizeof(int));
    waitPort = (int *) malloc(numMachines*sizeof(int));
    waitList = (int *) calloc(numMachines, sizeof(int));

    CreateListeningSocket(&decodeServerSocket, &decodePortNum, &error);
    if (error)
        errorExit("Unable to create socket on which to listen.  %s", error);

    machineDebug("DECODE SERVER LISTENING ON PORT %d", decodePortNum);

    TransmitPortNum(masterHostName, masterPortNum, decodePortNum);

    frameDone = (boolean *) malloc(numInputFiles*sizeof(boolean));
    memset((char *)frameDone, 0, numInputFiles*sizeof(boolean));

    /* wait for ready signals and requests */
    while ( TRUE ) {
        const char * error;

        AcceptConnection(decodeServerSocket, &otherSock, &error);
        if (error)
            errorExit("DECODE SERVER: "
                      "Failed to accept next connection.  %s", error);

        ReadInt(otherSock, &frameReady);

        if ( frameReady == -2 ) {
            ReadInt(otherSock, &frameReady);

            machineDebug("DECODE SERVER:  REQUEST FOR FRAME %d", frameReady);

            /* now respond if it's ready yet */
            WriteInt(otherSock, frameDone[frameReady]);

            if ( ! frameDone[frameReady] ) {
                /* read machine number, port number */
                ReadInt(otherSock, &slaveNumber);
                ReadInt(otherSock, &slavePort);

                machineDebug("DECODE SERVER: WAITING:  SLAVE %d, PORT %d",
                             slaveNumber, slavePort);

                waitPort[slaveNumber] = slavePort;
                if ( waitMachine[frameReady] == 0 ) {
                    waitMachine[frameReady] = slaveNumber+1;
                } else {
                    /* someone already waiting for this frame */
                    /* follow list of waiters to the end */
                    waitPtr = waitMachine[frameReady]-1;
                    while ( waitList[waitPtr] != 0 ) {
                        waitPtr = waitList[waitPtr]-1;
                    }

                    waitList[waitPtr] = slaveNumber+1;
                    waitList[slaveNumber] = 0;
                }
            }
        } else {
            frameDone[frameReady] = TRUE;
            
            machineDebug("DECODE SERVER:  FRAME %d READY", frameReady);

            if ( waitMachine[frameReady] ) {
                /* need to notify one or more machines it's ready */
                waitPtr = waitMachine[frameReady]-1;
                while ( waitPtr >= 0 ) {
                    const char * error;
                    ConnectToSocket(machineName[waitPtr], waitPort[waitPtr],
                                    &nullHost,
                                    &clientSocket, &error);
                    if (error)
                        errorExit("DECODE SERVER: "
                                  "Can't connect to child machine.  %s", 
                                  error);
                    close(clientSocket);
                    waitPtr = waitList[waitPtr]-1;
                }
            }
        }

        close(otherSock);
    }
    
    machineDebug("DECODE SERVER:  Shutting down");

    /* tell Master server we are done */
    TransmitPortNum(masterHostName, masterPortNum, decodePortNum);

    close(decodeServerSocket);
}



/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
