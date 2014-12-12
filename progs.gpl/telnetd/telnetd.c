/* $Id: telnetd.c,v 1.1.1.1 2005/05/19 10:53:05 r01122 Exp $
 *
 * Simple telnet server
 * Bjorn Wesen, Axis Communications AB (bjornw at axis.com)
 *
 * This file is distributed under the Gnu Public License (GPL),
 * please see the file LICENSE for further information.
 *
 * ---------------------------------------------------------------------------
 * (C) Copyright 2000, Axis Communications AB, LUND, SWEDEN
 ****************************************************************************
 *
 * The telnetd manpage says it all:
 *
 *   Telnetd operates by allocating a pseudo-terminal device (see pty(4))  for
 *   a client, then creating a login process which has the slave side of the
 *   pseudo-terminal as stdin, stdout, and stderr. Telnetd manipulates the
 *   master side of the pseudo-terminal, implementing the telnet protocol and
 *   passing characters between the remote client and the login process.
 *
 * Vladimir Oleynik <dzo at simtreas.ru> 2001
 *  Set process group corrections, initial busybox port
 */

/*#define DEBUG 1 */
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

#include <arpa/telnet.h>
#include <ctype.h>
#include "elbox_config.h"
/* added by Leo, 2006/07/19 15:59:25 */
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP
#   include <syslog.h>
#   include "asyslog.h"
#endif
/* ********************************* */

/* added by Leo, 2007/03/09 17:20:58 */
//#define TRACE
#ifdef TRACE
#define trace(args...) fprintf(stderr,##args)
#else
#define trace(args...)
#endif
/* ********************************* */

#define BUFSIZE 4000

int grantpt(int);
int unlockpt(int);
char * ptsname(int);

static char *loginpath = NULL;

/* shell name and arguments */


#ifdef CONNECTION_TIMEOUT
/* Modified by Leo, 2006/08/25 12:22:58 */
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP /*andy add for log */
static char *argv_init[] = {NULL, NULL, NULL, NULL,NULL, NULL};
#else
static char *argv_init[] = {NULL, NULL, NULL, NULL, NULL};
#endif

/* ********************************* */
/* structure that describes a session */
/*
 * added by Leo 2007/03/07 16:23
 * restricted the number of connections support
 */
static int num_connections = 0;
/* *************************************** */
#else
static char *argv_init[] = {NULL, NULL, NULL, NULL};

/* structure that describes a session */
#endif
struct tsession
{
    struct tsession *next;
    int sockfd, ptyfd;
    int shell_pid;
    /* two circular buffers */
    char *buf1, *buf2;
    int rdidx1, wridx1, size1;
    int rdidx2, wridx2, size2;
};

/*

   This is how the buffers are used. The arrows indicate the movement
   of data.

   +-------+     wridx1++     +------+     rdidx1++     +----------+
   |       | <--------------  | buf1 | <--------------  |          |
   |       |     size1--      +------+     size1++      |          |
   |  pty  |                                            |  socket  |
   |       |     rdidx2++     +------+     wridx2++     |          |
   |       |  --------------> | buf2 |  --------------> |          |
   +-------+     size2++      +------+     size2--      +----------+

   Each session has got two buffers.

*/

static int maxfd;

static struct tsession *sessions;


/*------------------------------*/
/* Added/Modified by StPAUL 20060616 */
/* Add support ilde timeout */
static void show_usage(char *name)
{
    fprintf(stderr, "Usage: %s [-p port] [-i ifname] [-u user:password] [-t timeout] [-l loginpath]\n", name);
}

/*

   Remove all IAC's from the buffer pointed to by bf (recieved IACs are ignored
   and must be removed so as to not be interpreted by the terminal).  Make an
   uninterrupted string of characters fit for the terminal.  Do this by packing
   all characters meant for the terminal sequentially towards the end of bf.

   Return a pointer to the beginning of the characters meant for the terminal.
   and make *processed equal to the number of characters that were actually
   processed and *num_totty the number of characters that should be sent to
   the terminal.

   Note - If an IAC (3 byte quantity) starts before (bf + len) but extends
   past (bf + len) then that IAC will be left unprocessed and *processed will be
   less than len.

   FIXME - if we mean to send 0xFF to the terminal then it will be escaped,
   what is the escape character?  We aren't handling that situation here.

  */
static char *
remove_iacs(unsigned char *bf, int len, int *processed, int *num_totty)
{
    unsigned char *ptr = bf;
    unsigned char *totty = bf;
    unsigned char *end = bf + len;

    while(ptr < end)
    {
        if(*ptr != IAC)
        {
            *totty++ = *ptr++;
        }
        else
        {
            if((ptr+2) < end)
            {
                /* the entire IAC is contained in the buffer
                   we were asked to process. */
#ifdef DEBUG
                fprintf(stderr, "Ignoring IAC %s,%s\n",
                        *ptr, TELCMD(*(ptr+1)), TELOPT(*(ptr+2)));
#endif
                ptr += 3;
            }
            else
            {
                /* only the beginning of the IAC is in the
                   buffer we were asked to process, we can't
                   process this char. */
                break;
            }
        }
    }

    *processed = ptr - bf;
    *num_totty = totty - bf;
    /* move the chars meant for the terminal towards the end of the
       buffer. */
    return memmove(ptr - *num_totty, bf, *num_totty);
}


static int
getpty(char *line)
{
    int p;
#ifdef HAVE_DEVPTS_FS
    p = open("/dev/ptmx", 2);
    if(p > 0)
    {
        grantpt(p);
        unlockpt(p);
        strcpy(line, ptsname(p));
        return(p);
    }
#else
    struct stat stb;
    int i;
    int j;

    strcpy(line, "/dev/ptyXX");

    for(i = 0; i < 16; i++)
    {
        line[8] = "pqrstuvwxyzabcde"[i];
        line[9] = '0';
        if(stat(line, &stb) < 0)
        {
            continue;
        }
        for(j = 0; j < 16; j++)
        {
            line[9] = "0123456789abcdef"[j];
            if((p = open(line, O_RDWR | O_NOCTTY)) >= 0)
            {
                line[5] = 't';
                return p;
            }
        }
    }
#endif /* HAVE_DEVPTS_FS */
    return -1;
}


static void
send_iac(struct tsession *ts, unsigned char command, int option)
{
    /* We rely on that there is space in the buffer for now.  */
    char *b = ts->buf2 + ts->rdidx2;
    *b++ = IAC;
    *b++ = command;
    *b++ = option;
    ts->rdidx2 += 3;
    ts->size2 += 3;
}


static struct tsession *
make_new_session(int sockfd)
{
    struct termios termbuf;
    int pty, pid;
    static char tty_name[32];
#ifdef CONNECTION_TIMEOUT
    struct tsession *ts;
    /*
     * added by Leo 2007/03/07 16:23
     * limited the number of connections support
     */
    if(num_connections >= MAX_CONNECTIONS)
    {
        fprintf(stderr,"TELNETD: Over max. connections!\n");
        return 0;
    }
    /* **************************************** */
    ts = (struct tsession *)malloc(sizeof(struct tsession));

#else
    struct tsession *ts = (struct tsession *)malloc(sizeof(struct tsession));
#endif
    ts->buf1 = (char *)malloc(BUFSIZE);
    ts->buf2 = (char *)malloc(BUFSIZE);

    ts->sockfd = sockfd;

    ts->rdidx1 = ts->wridx1 = ts->size1 = 0;
    ts->rdidx2 = ts->wridx2 = ts->size2 = 0;

    /* Got a new connection, set up a tty and spawn a shell.  */

    pty = getpty(tty_name);

    if(pty < 0)
    {
        fprintf(stderr, "TELNETD: All network ports in use!\n");
        return 0;
    }

    if(pty > maxfd)
        maxfd = pty;

    ts->ptyfd = pty;

    /* Make the telnet client understand we will echo characters so it
     * should not do it locally. We don't tell the client to run linemode,
     * because we want to handle line editing and tab completion and other
     * stuff that requires char-by-char support.
     */

    send_iac(ts, DO, TELOPT_ECHO);
    send_iac(ts, DO, TELOPT_LFLOW);
    send_iac(ts, WILL, TELOPT_ECHO);
    send_iac(ts, WILL, TELOPT_SGA);


    if((pid = vfork()) < 0)
    {
        perror("vfork");
    }
    if(pid == 0)
    {
        /* In child, open the child's side of the tty.  */
        int i, t;

        for(i = 0; i <= maxfd; i++)
            close(i);
        /* make new process group */
        if(setsid() < 0)
        {
            perror("setsid");
            exit(-1);
        }

        t = open(tty_name, O_RDWR | O_NOCTTY);
        if(t < 0)
        {
            perror("Could not open tty");
            exit(-1);
        }
        dup(0);
        dup(1);

        tcsetpgrp(0, getpid());

        /* The pseudo-terminal allocated to the client is configured to operate in
         * cooked mode, and with XTABS CRMOD enabled (see tty(4)).
         */

        tcgetattr(t, &termbuf);
        termbuf.c_lflag |= ECHO; /* if we use readline we dont want this */
        termbuf.c_oflag |= ONLCR|XTABS;
        termbuf.c_iflag |= ICRNL;
        termbuf.c_iflag &= ~IXOFF;
        /*termbuf.c_lflag &= ~ICANON;*/
        tcsetattr(t, TCSANOW, &termbuf);

        /* exec shell, with correct argv and env */
        execv(loginpath, argv_init);

        /* NOT REACHED */
        perror("execv");
        exit(-1);
    }

    ts->shell_pid = pid;

    return ts;
}

static void
free_session(struct tsession *ts)
{
    struct tsession *t = sessions;

    /* Unlink this telnet session from the session list.  */
    if(t == ts)
        sessions = ts->next;
    else
    {
        while(t->next != ts)
            t = t->next;
        t->next = ts->next;
    }

    free(ts->buf1);
    free(ts->buf2);

    kill(ts->shell_pid, SIGKILL);

    wait4(ts->shell_pid, NULL, 0, NULL);

    close(ts->ptyfd);
    close(ts->sockfd);

    if(ts->ptyfd == maxfd || ts->sockfd == maxfd)
        maxfd--;
    if(ts->ptyfd == maxfd || ts->sockfd == maxfd)
        maxfd--;

    free(ts);
#ifdef CONNECTION_TIMEOUT
    /* added by Leo, 2007/03/20 12:28:13 */
    if(num_connections) num_connections--;
    /* ********************************* */
#endif
}

int
main(int argc, char **argv)
{
    struct sockaddr_in sa;
    char *user=NULL, *passwd=NULL, *ifname=NULL;
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP
    char hostip[20];
    char hostip_temp[20];
#endif
    int master_fd;
    fd_set rdfdset, wrfdset;
    int selret;
    int timeout = 300;
    int on = 1;
    int portnbr = 23;
    int c;
    /*------------------------------*/
    /* Added/Modified by StPAUL 20060616 */
    /* Add support ilde timeout */
    struct timeval tv;
    /*------------------------------*/
    /* Added by Leo, 2006/08/25 12:22:58 */
    char *shell=NULL;
    /* ********************************* */
#if 0//def SYSLOG
    syslog(ALOG_SYSACT|LOG_NOTICE,"Telnetd started");
#endif
    /* check if user supplied a port number */

    for(;;)
    {
        c = getopt(argc, argv, "p:u:l:i:t:h:s:");
        if(c == EOF) break;
        switch(c)
        {
            case 'p':
                portnbr = atoi(optarg);
                break;
            case 'i':
                ifname = optarg;
                break;
            case 'u':
                user = strdup(optarg);
                passwd=strchr(user, ':');
                if(passwd)
                    *passwd++=0;
                else
                    passwd="";
                break;
                /*------------------------------*/
                /* Added/Modified by StPAUL 20060616 */
                /* Add support ilde timeout */
            case 't':
                timeout=atoi(optarg);
                break;
                /*------------------------------*/
            case 'l':
                loginpath = strdup(optarg);
                break;
                /* Added by Leo, 2006/08/25 12:22:58 */
            case 's':
                shell = strdup(optarg);
                break;
                /* ********************************* */
            case 'h':
            default:
                show_usage(argv[0]);
                exit(1);
        }
    }

    if(!loginpath)
        loginpath = "/bin/sh";

    if(access(loginpath, X_OK) < 0)
    {
        {
            printf("'%s' unavailable.", loginpath);
            exit(-1);
        }
    }

    argv_init[0] = loginpath;
    if(user)
    {
        argv_init[1]=user;
        argv_init[2]=passwd;
    }
#ifdef CONNECTION_TIMEOUT
    /* Added by Leo, 2006/08/25 12:22:58 */
    if(shell)
        argv_init[3]=shell;
    /* ********************************* */
#endif
    sessions = 0;

    /* Grab a TCP socket.  */
#ifdef ELBOX_USE_IPV6//add by wenwen
    struct sockaddr_in6 sa6;
    struct in6_addr addr6;
    int n;
    memset((void *)&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port=htons(portnbr);
    if((n=inet_pton(AF_INET6, "0::0", &addr6))!=1)
    {
        printf("in inet_pton error :%d in telnetd-----------\n",n);
        exit(1);
    }
    sa6.sin6_addr = addr6;
    //master_fd = socket(AF_INET, SOCK_STREAM, 0);
    master_fd = socket(sa6.sin6_family, SOCK_STREAM, 0);
    (void)setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#else
    master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(master_fd < 0)
    {
        perror("socket");
        return 1;
    }
    (void)setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* Set it to listen to specified port.  */

    memset((void *)&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(portnbr);
#endif
    if(ifname)
    {

        struct ifreq interface;
        strncpy(interface.ifr_ifrn.ifrn_name, ifname, IFNAMSIZ);
        int trytime=0;
        for(trytime=0; trytime < 5; trytime++)
        {
            if(setsockopt(master_fd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&interface, sizeof(interface)) < 0)
            {
                printf("bind %s fail try again\n",ifname);
                sleep(3);
                continue;
            }
            break;
        }
        if(trytime==5)
        {
            perror("bind dev fail");
            return 1;
        }
    }
#ifdef ELBOX_USE_IPV6//add by wenwen
    if(bind(master_fd, (struct sockaddr *) &sa6, sizeof(sa6)) < 0)
    {
        perror("bind");
        return 1;
    }
#else
    if(bind(master_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
        perror("bind");
        return 1;
    }
#endif
    if(listen(master_fd, 1) < 0)
    {
        perror("listen");
        return 1;
    }

//  if (daemon(0, 1) < 0)
//      {perror("daemon");exit(-1);}


    maxfd = master_fd;

    do
    {
        struct tsession *ts;

        FD_ZERO(&rdfdset);
        FD_ZERO(&wrfdset);

        /* select on the master socket, all telnet sockets and their
         * ptys if there is room in their respective session buffers.
         */

        FD_SET(master_fd, &rdfdset);

        ts = sessions;
        while(ts)
        {
            /* buf1 is used from socket to pty
             * buf2 is used from pty to socket
             */
            if(ts->size1 > 0)
            {
                FD_SET(ts->ptyfd, &wrfdset);  /* can write to pty */
            }
            if(ts->size1 < BUFSIZE)
            {
                FD_SET(ts->sockfd, &rdfdset); /* can read from socket */
            }
            if(ts->size2 > 0)
            {
                FD_SET(ts->sockfd, &wrfdset); /* can write to socket */
            }
            if(ts->size2 < BUFSIZE)
            {
                FD_SET(ts->ptyfd, &rdfdset);  /* can read from pty */
            }
            ts = ts->next;
        }

        /*------------------------------*/
        /* Added/Modified by StPAUL 20060616 */
        /* Add support ilde timeout */
#ifdef CONNECTION_TIMEOUT
        trace("timeout=%d, num_connections=%d\n",timeout, num_connections);
#endif
        if(timeout<=0)
            selret = select(maxfd + 1, &rdfdset, &wrfdset, 0, 0);
        else
        {
            tv.tv_sec=timeout;
            tv.tv_usec=0;
            selret = select(maxfd + 1, &rdfdset, &wrfdset, 0, &tv);
        }
        if(selret==-1)
            break;
        if(selret==0)
        {
            if(sessions)
            {
                /*
                 * added and modified by Leo 2007/03/07 14:40
                 * support client's connection timeout
                 */
#ifdef CONNECTION_TIMEOUT
                free_session(sessions);
#endif
                continue;
            }
#ifndef CONNECTION_TIMEOUT
            else
            {
                fprintf(stderr,"TELNETD: timeout terminated !!\n");
                break;
            }
#endif
        }
        /*------------------------------*/

        /* First check for and accept new sessions.  */
        if(FD_ISSET(master_fd, &rdfdset))
        {
            int fd;
            socklen_t salen;
#ifdef ELBOX_USE_IPV6
            salen = sizeof(sa6);
            if((fd = accept(master_fd, (struct sockaddr *)&sa6,
                            &salen)) < 0)
            {
                continue;
            }
#else
            salen = sizeof(sa);
            if((fd = accept(master_fd, (struct sockaddr *)&sa,
                            &salen)) < 0)
            {
                continue;
            }
#endif            
            else
            {
                /* Create a new session and link it into
                   our active list.  */

#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP

#ifdef ELBOX_USE_IPV6

                sprintf(hostip,"%d.%d.%d.%d",(sa6.sin6_addr.s6_addr32[3] & 0xff000000)>>24
                        ,(sa6.sin6_addr.s6_addr32[3] & 0x00ff0000) >>16
                        ,(sa6.sin6_addr.s6_addr32[3] & 0x0000ff00) >>8
                        ,sa6.sin6_addr.s6_addr32[3] & 0x000000ff);

#else
                sprintf(hostip,"%d.%d.%d.%d",(sa.sin_addr.s_addr & 0xff000000)>>24
                        ,(sa.sin_addr.s_addr & 0x00ff0000) >>16
                        ,(sa.sin_addr.s_addr & 0x0000ff00) >>8
                        ,sa.sin_addr.s_addr & 0x000000ff);

#endif
                sprintf(hostip_temp,"-h%s",hostip);
                argv_init[4]=hostip_temp;

#endif

                struct tsession *new_ts = make_new_session(fd);
                if(new_ts)
                {
                    new_ts->next = sessions;
                    sessions = new_ts;
                    if(fd > maxfd)
                        maxfd = fd;
#ifdef CONNECTION_TIMEOUT
                    /*
                     * added by Leo 2007/03/07 16:23
                     * restricted the number of connections support
                     */
                    num_connections++;
                    /* **************************************** */
#endif
                }
                else
                {
                    close(fd);
                }
            }
        }

        /* Then check for data tunneling.  */

        ts = sessions;
        while(ts)    /* For all sessions...  */
        {
            int maxlen, w, r;
            struct tsession *next = ts->next; /* in case we free ts. */

            if(ts->size1 && FD_ISSET(ts->ptyfd, &wrfdset))
            {
                int processed, num_totty;
                char *ptr;
                /* Write to pty from buffer 1.  */

                maxlen = MIN(BUFSIZE - ts->wridx1,
                             ts->size1);
                ptr = remove_iacs((unsigned char *)(ts->buf1 + ts->wridx1), maxlen,
                                  &processed, &num_totty);

                /* the difference between processed and num_totty
                   is all the iacs we removed from the stream.
                   Adjust buf1 accordingly. */
                ts->wridx1 += processed - num_totty;
                ts->size1 -= processed - num_totty;

                w = write(ts->ptyfd, ptr, num_totty);
                if(w < 0)
                {
                    perror("write");
                    free_session(ts);
                    ts = next;
                    continue;
                }
                ts->wridx1 += w;
                ts->size1 -= w;
                if(ts->wridx1 == BUFSIZE)
                    ts->wridx1 = 0;
            }

            if(ts->size2 && FD_ISSET(ts->sockfd, &wrfdset))
            {
                /* Write to socket from buffer 2.  */
                maxlen = MIN(BUFSIZE - ts->wridx2,
                             ts->size2);
                w = write(ts->sockfd, ts->buf2 + ts->wridx2, maxlen);
                if(w < 0)
                {
                    perror("write");
                    free_session(ts);
                    ts = next;
                    continue;
                }
                ts->wridx2 += w;
                ts->size2 -= w;
                if(ts->wridx2 == BUFSIZE)
                    ts->wridx2 = 0;
            }

            if(ts->size1 < BUFSIZE && FD_ISSET(ts->sockfd, &rdfdset))
            {
                /* Read from socket to buffer 1. */
                maxlen = MIN(BUFSIZE - ts->rdidx1,
                             BUFSIZE - ts->size1);
                r = read(ts->sockfd, ts->buf1 + ts->rdidx1, maxlen);
#ifdef CONNECTION_TIMEOUT
                trace("read len %d from socket\n",r);
#endif
                if(!r || (r < 0 && errno != EINTR))
                {
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP
#ifdef CONNECTION_TIMEOUT
                    if(num_connections)
                        syslog(ALOG_AP_SYSACT | LOG_NOTICE, "[SYSACT]Telnet logout from %s\n",hostip);
#endif
#endif
                    free_session(ts);
                    ts = next;
#ifdef CONNECTION_TIMEOUT
                    trace("p1: free session errno=%x\n",errno);
#endif
                    continue;
                }
                if(!*(ts->buf1 + ts->rdidx1 + r - 1))
                {
                    r--;
#ifdef CONNECTION_TIMEOUT
                    trace("p2 r=%d\n",r);
#endif
                    if(!r)
                        continue;
                }
                ts->rdidx1 += r;
                ts->size1 += r;
                if(ts->rdidx1 == BUFSIZE)
                    ts->rdidx1 = 0;
            }

            if(ts->size2 < BUFSIZE && FD_ISSET(ts->ptyfd, &rdfdset))
            {
                /* Read from pty to buffer 2.  */
                maxlen = MIN(BUFSIZE - ts->rdidx2,
                             BUFSIZE - ts->size2);
                r = read(ts->ptyfd, ts->buf2 + ts->rdidx2, maxlen);
                if(!r || (r < 0 && errno != EINTR))
                {


#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP
#ifdef CONNECTION_TIMEOUT
                    if(num_connections)
                        syslog(ALOG_AP_SYSACT | LOG_NOTICE, "[SYSACT]Telnet logout from %s\n",hostip);
#endif
#endif

                    free_session(ts);
                    ts = next;
                    /* added by Leo, 2007/03/09 17:40:16 */
#ifdef CONNECTION_TIMEOUT
                    if(num_connections) num_connections--;
                    /* ********************************* */
                    trace("p3 r=%d, errno=%x, num_connections=%d\n",r,errno,num_connections);
#endif
                    continue;
                }
                ts->rdidx2 += r;
                ts->size2 += r;
                if(ts->rdidx2 == BUFSIZE)
                    ts->rdidx2 = 0;
            }

            if(ts->size1 == 0)
            {
                ts->rdidx1 = 0;
                ts->wridx1 = 0;
            }
            if(ts->size2 == 0)
            {
                ts->rdidx2 = 0;
                ts->wridx2 = 0;
            }
            ts = next;
        }
    }
    while(1);
#ifdef CONNECTION_TIMEOUT
    trace("Terminate daemon!\n");
#endif
    return 0;
}

