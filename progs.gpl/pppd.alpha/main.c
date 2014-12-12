/* vi: set sw=4 ts=4: */
/*
 * main.c - Point-to-Point Protocol main module
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define RCSID	"$Id: main.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <netdb.h>
#include <utmp.h>
#include <pwd.h>
#include <setjmp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h>
#include "asyslog.h"

#include "pppd.h"
#include "magic.h"
#include "fsm.h"
#include "lcp.h"
#include "ipcp.h"
#ifdef INET6
#include "ipv6cp.h"
#endif
#include "upap.h"
#include "chap.h"
#include "eap.h"
#include "ccp.h"
#include "ecp.h"
#include "pathnames.h"

#ifdef USE_TDB
#include "tdb.h"
#endif

#ifdef CBCP_SUPPORT
#include "cbcp.h"
#endif

#ifdef IPX_CHANGE
#include "ipxcp.h"
#endif /* IPX_CHANGE */
#ifdef AT_CHANGE
#include "atcp.h"
#endif

#include "eloop.h"
#include "dtrace.h"

static const char rcsid[] = RCSID;

/* interface vars */
char ifname[32];						/* Interface name */
int ifunit;								/* Interface unit number */

struct channel *the_channel;

char *progname;							/* Name of this program */
char hostname[MAXNAMELEN];				/* Our hostname */
static char pidfilename[MAXPATHLEN];	/* name of pid file */
static char linkpidfile[MAXPATHLEN];	/* name of linkname pid file */
char ppp_devnam[MAXPATHLEN];			/* name of PPP tty (maybe ttypx) */
uid_t uid;								/* Our real user-id */
struct notifier *pidchange = NULL;
struct notifier *phasechange = NULL;
struct notifier *exitnotify = NULL;
struct notifier *sigreceived = NULL;
struct notifier *fork_notifier = NULL;

int hungup;								/* terminal has been hung up */
int privileged;							/* we're running as real uid root */
int need_holdoff;						/* need holdoff period before restarting */
int detached;							/* have detached from terminal */
volatile int status;					/* exit status for pppd */
int unsuccess;							/* # unsuccessful connection attempts */
int ppp_session_number;					/* Session number, for channels with such a
										   concept (eg PPPoE) */
#ifdef USE_TDB
TDB_CONTEXT *pppdb;						/* database for storing status etc. */
#endif

char db_key[32];

int (*holdoff_hook)(void) = NULL;
int (*new_phase_hook)(int) = NULL;
void (*snoop_recv_hook)(unsigned char *p, int len) = NULL;
void (*snoop_send_hook)(unsigned char *p, int len) = NULL;

static int conn_running;				/* we have a [dis]connector running */
static int devfd;						/* fd of underlying device */
static int fd_ppp = -1;					/* fd for talking PPP */
static int fd_loop;						/* fd for getting demand-dial packets */
static int fd_devnull;					/* fd for /dev/null */

int phase;								/* where the link is at */
int kill_link;
int open_ccp_flag;
int listen_time;
int got_sigusr2;
int got_sigterm = 0;
int got_sighup;

static int need_jmp = 0;
static sigjmp_buf sigjmp;

char **script_env;							/* Env. variable values for scripts */
int s_env_nalloc;							/* # words avail at script_env */

u_char outpacket_buf[PPP_MRU+PPP_HDRLEN];	/* buffer for outgoing packet */
u_char inpacket_buf[PPP_MRU+PPP_HDRLEN];	/* buffer for incoming packet */

static int n_children;						/* # child processes still running */

int privopen;								/* don't lock, open device as root */

char *no_ppp_msg = "Sorry - this system lacks PPP kernel support\n";

GIDSET_TYPE groups[NGROUPS_MAX];			/* groups the user is in */
int ngroups;								/* How many groups valid in groups */

static struct timeval start_time;			/* Time when link was started. */

struct pppd_stats link_stats;
unsigned link_connect_time;
int link_stats_valid;

int error_count;

int pty_pid=-1;

/*
 * We maintain a list of child process pids and
 * functions to call when they exit.
 */
struct subprocess
{
	pid_t	pid;
	char	*prog;
	void	(*done)(void *);
	void	*arg;
	struct subprocess *next;
};

static struct subprocess *children;

/* Prototypes for procedures local to this file. */

static void setup_signals(void);
static void create_pidfile(int pid);
static void create_linkpidfile(int pid);
static void cleanup(void);
static void hup(int);
static void term(int);
static void chld(int);
static void toggle_debug(int);
static void open_ccp(int);
static void bad_signal(int);
static void holdoff_end(void *, void *);

#ifdef USE_TDB
static void update_db_entry(void);
static void add_db_key(const char *);
static void delete_db_key(const char *);
static void cleanup_db(void);
#endif

static void print_link_stats(void);

extern char *ttyname(int);
extern char *getlogin(void);
int main(int, char *[]);

#ifdef ultrix
#undef	O_NONBLOCK
#define	O_NONBLOCK	O_NDELAY
#endif

#ifdef ULTRIX
#define setlogmask(x)
#endif

/* If PPP_DRV_NAME is not defined, use the default "ppp" as the device name. */
#if !defined(PPP_DRV_NAME)
#define PPP_DRV_NAME	"ppp"
#endif /* !defined(PPP_DRV_NAME) */

/* PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 * The last entry must be NULL. */
struct protent *protocols[] =
{
	&lcp_protent,
	&pap_protent,
	&chap_protent,
#ifdef CBCP_SUPPORT
	&cbcp_protent,
#endif
	&ipcp_protent,
#ifdef INET6
	&ipv6cp_protent,
#endif
	&ccp_protent,
	&ecp_protent,
#ifdef IPX_CHANGE
	&ipxcp_protent,
#endif
#ifdef AT_CHANGE
	&atcp_protent,
#endif
	&eap_protent,
	NULL
};


/*
 * setup_signals - initialize signal handling.
 */
static void setup_signals()
{
	struct sigaction sa;
	sigset_t mask;

	/* Compute mask of all interesting signals and install signal handlers
	 * for each.  Only one signal handler may be active at a time.  Therefore,
	 * all other signals should be masked when any handler is executing. */
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGUSR2);

#define SIGNAL(s, handler)	do { \
	sa.sa_handler = handler; \
	if (sigaction(s, &sa, NULL) < 0) \
		fatal("Couldn't establish signal handler (%d): %m", s); \
	} while (0)

	sa.sa_mask = mask;
	sa.sa_flags = 0;
	SIGNAL(SIGHUP, hup);		/* Hangup */
	SIGNAL(SIGINT, term);		/* Interrupt */
	SIGNAL(SIGTERM, term);		/* Terminate */
	SIGNAL(SIGCHLD, chld);

	SIGNAL(SIGUSR1, toggle_debug);	/* Toggle debug flag */
	SIGNAL(SIGUSR2, open_ccp);		/* Reopen CCP */

	/* Install a handler for other signals which would otherwise
	 * cause pppd to exit without cleaning up. */
	SIGNAL(SIGABRT, bad_signal);
	SIGNAL(SIGALRM, bad_signal);
	SIGNAL(SIGFPE, bad_signal);
	SIGNAL(SIGILL, bad_signal);
	SIGNAL(SIGPIPE, bad_signal);
	SIGNAL(SIGQUIT, bad_signal);
	SIGNAL(SIGSEGV, bad_signal);
#ifdef SIGBUS
	SIGNAL(SIGBUS, bad_signal);
#endif
#ifdef SIGEMT
	SIGNAL(SIGEMT, bad_signal);
#endif
#ifdef SIGPOLL
	SIGNAL(SIGPOLL, bad_signal);
#endif
#ifdef SIGPROF
	SIGNAL(SIGPROF, bad_signal);
#endif
#ifdef SIGSYS
	SIGNAL(SIGSYS, bad_signal);
#endif
#ifdef SIGTRAP
	SIGNAL(SIGTRAP, bad_signal);
#endif
#ifdef SIGVTALRM
	SIGNAL(SIGVTALRM, bad_signal);
#endif
#ifdef SIGXCPU
	SIGNAL(SIGXCPU, bad_signal);
#endif
#ifdef SIGXFSZ
	SIGNAL(SIGXFSZ, bad_signal);
#endif

	/* Apparently we can get a SIGPIPE when we call syslog, if
	 * syslogd has died and been restarted.  Ignoring it seems
	 * be sufficient. */
	signal(SIGPIPE, SIG_IGN);
}

/*
 * set_ifunit - do things we need to do once we know which ppp
 * unit we are using.
 */
void set_ifunit(int iskey)
{
	d_info("pppd: Using interface %s%d\n", PPP_DRV_NAME, ifunit);
	slprintf(ifname, sizeof(ifname), "%s%d", PPP_DRV_NAME, ifunit);
	script_setenv("IFNAME", ifname, iskey);
	if (iskey)
	{
		create_pidfile(getpid());	/* write pid to file */
		create_linkpidfile(getpid());
	}
}

/*
 * reopen_log - (re)open our connection to syslog.
 */
void reopen_log()
{
	openlog("pppd", LOG_PID | LOG_NDELAY, LOG_PPP);
	setlogmask(LOG_UPTO(LOG_INFO));
}

/*
 * Create a file containing our process ID.
 */
static void create_pidfile(int pid)
{
	FILE *pidfile;

	slprintf(pidfilename, sizeof(pidfilename), "%s%s.pid", _PATH_VARRUN, ifname);
	if ((pidfile = fopen(pidfilename, "w")) != NULL)
	{
		fprintf(pidfile, "%d\n", pid);
		(void)fclose(pidfile);
	}
	else
	{
		d_error("pppd: Failed to create pid file %s\n", pidfilename);
		pidfilename[0] = 0;
	}
}

static void create_linkpidfile(int pid)
{
	FILE *pidfile;

	if (linkname[0] == 0) return;
	script_setenv("LINKNAME", linkname, 1);
	slprintf(linkpidfile, sizeof(linkpidfile), "%sppp-%s.pid", _PATH_VARRUN, linkname);
	if ((pidfile = fopen(linkpidfile, "w")) != NULL)
	{
		fprintf(pidfile, "%d\n", pid);
		if (ifname[0]) fprintf(pidfile, "%s\n", ifname);
		(void) fclose(pidfile);
	}
	else
	{
		d_error("pppd: Failed to create pid file %s\n", linkpidfile);
		linkpidfile[0] = 0;
    }
}

unsigned short	sts_session_id = 0;
unsigned int	sts_mtu = 0;


void update_statusfile(const char * message)
{
	static char statusfile[MAXPATHLEN] = {0};
	char cmdstr[64];
	FILE * fh;

	if (linkname[0] == 0) return;
	if (statusfile[0] == 0)
		slprintf(statusfile, sizeof(statusfile), "%sppp-%s.status", _PATH_VARRUN, linkname);

	if (message)
	{
		if ((fh = fopen(statusfile, "w")) != NULL)
		{
			fprintf(fh, "%s\n", message);
			fclose(fh);

			sprintf(cmdstr, "/etc/ppp/ppp-status \"%s\" \"%s\" \"%x\" \"%u\"", linkname, message, sts_session_id, sts_mtu);
			system(cmdstr);
		}
	}
	else
	{
		unlink(statusfile);
	}
}

/* record ppp6 status */
void update_statusfile6(const char * message)
{
	static char statusfile[MAXPATHLEN] = {0};
	char cmdstr[64];
	FILE * fh;

	if (linkname[0] == 0) return;
	if (statusfile[0] == 0)
		slprintf(statusfile, sizeof(statusfile), "%sppp-%s.status", _PATH_VARRUN, linkname);

	if (message)
	{
		if ((fh = fopen(statusfile, "w")) != NULL)
		{
			fprintf(fh, "%s\n", message);
			fclose(fh);

			sprintf(cmdstr, "/etc/ppp/ppp6-status \"%s\" \"%s\" \"%x\" \"%u\"", linkname, message, sts_session_id, sts_mtu);
			system(cmdstr);
		}
	}
	else
	{
		unlink(statusfile);
	}
}

/*
 * holdoff_end - called via a timeout when the holdoff period ends.
 */
static void holdoff_end(void * eloop_ctx, void * timeout_ctx)
{
    new_phase(PHASE_DORMANT);
}

/* List of protocol names, to make our messages a little more informative. */
struct protocol_list {
    u_short	proto;
    const char	*name;
} protocol_list[] = {
    { 0x21,	"IP" },
    { 0x23,	"OSI Network Layer" },
    { 0x25,	"Xerox NS IDP" },
    { 0x27,	"DECnet Phase IV" },
    { 0x29,	"Appletalk" },
    { 0x2b,	"Novell IPX" },
    { 0x2d,	"VJ compressed TCP/IP" },
    { 0x2f,	"VJ uncompressed TCP/IP" },
    { 0x31,	"Bridging PDU" },
    { 0x33,	"Stream Protocol ST-II" },
    { 0x35,	"Banyan Vines" },
    { 0x39,	"AppleTalk EDDP" },
    { 0x3b,	"AppleTalk SmartBuffered" },
    { 0x3d,	"Multi-Link" },
    { 0x3f,	"NETBIOS Framing" },
    { 0x41,	"Cisco Systems" },
    { 0x43,	"Ascom Timeplex" },
    { 0x45,	"Fujitsu Link Backup and Load Balancing (LBLB)" },
    { 0x47,	"DCA Remote Lan" },
    { 0x49,	"Serial Data Transport Protocol (PPP-SDTP)" },
    { 0x4b,	"SNA over 802.2" },
    { 0x4d,	"SNA" },
    { 0x4f,	"IP6 Header Compression" },
    { 0x6f,	"Stampede Bridging" },
    { 0xfb,	"single-link compression" },
    { 0xfd,	"1st choice compression" },
    { 0x0201,	"802.1d Hello Packets" },
    { 0x0203,	"IBM Source Routing BPDU" },
    { 0x0205,	"DEC LANBridge100 Spanning Tree" },
    { 0x0231,	"Luxcom" },
    { 0x0233,	"Sigma Network Systems" },
    { 0x8021,	"Internet Protocol Control Protocol" },
    { 0x8023,	"OSI Network Layer Control Protocol" },
    { 0x8025,	"Xerox NS IDP Control Protocol" },
    { 0x8027,	"DECnet Phase IV Control Protocol" },
    { 0x8029,	"Appletalk Control Protocol" },
    { 0x802b,	"Novell IPX Control Protocol" },
    { 0x8031,	"Bridging NCP" },
    { 0x8033,	"Stream Protocol Control Protocol" },
    { 0x8035,	"Banyan Vines Control Protocol" },
    { 0x803d,	"Multi-Link Control Protocol" },
    { 0x803f,	"NETBIOS Framing Control Protocol" },
    { 0x8041,	"Cisco Systems Control Protocol" },
    { 0x8043,	"Ascom Timeplex" },
    { 0x8045,	"Fujitsu LBLB Control Protocol" },
    { 0x8047,	"DCA Remote Lan Network Control Protocol (RLNCP)" },
    { 0x8049,	"Serial Data Control Protocol (PPP-SDCP)" },
    { 0x804b,	"SNA over 802.2 Control Protocol" },
    { 0x804d,	"SNA Control Protocol" },
    { 0x804f,	"IP6 Header Compression Control Protocol" },
    { 0x006f,	"Stampede Bridging Control Protocol" },
    { 0x80fb,	"Single Link Compression Control Protocol" },
    { 0x80fd,	"Compression Control Protocol" },
    { 0xc021,	"Link Control Protocol" },
    { 0xc023,	"Password Authentication Protocol" },
    { 0xc025,	"Link Quality Report" },
    { 0xc027,	"Shiva Password Authentication Protocol" },
    { 0xc029,	"CallBack Control Protocol (CBCP)" },
    { 0xc081,	"Container Control Protocol" },
    { 0xc223,	"Challenge Handshake Authentication Protocol" },
    { 0xc281,	"Proprietary Authentication Protocol" },
    { 0,	NULL },
};

/*
 * protocol_name - find a name for a PPP protocol.
 */
const char * protocol_name(int proto)
{
	struct protocol_list *lp;

	for (lp = protocol_list; lp->proto != 0; ++lp)
		if (proto == lp->proto)
			return lp->name;
	return NULL;
}

static void read_ppp_loop(int sock, void * eloop_ctx, void * sock_ctx)
{
	if (get_loop_output()) eloop_terminate();
}

void modem_hungup()
{
	d_dbg("pppd: lower is down!\n");
	status = EXIT_HANGUP;
	hungup = 1;
	lcp_lowerdown(0);
	link_terminated(0);
	return;
}

/*
 * get_ppp_input - called when incoming data is available.
 */
void get_ppp_input(int sock, void * eloop_ctx, void * sock_ctx)
{
	int len, i;
	u_char *p;
	u_short protocol;
	struct protent *protp;

	//d_dbg("pppd: >>> get_ppp_input()\n");
	
	p = inpacket_buf;	/* point to beginning of packet buffer */

	len = read_packet(inpacket_buf);
	if (len < 0) return;
	if (len == 0)
	{
		notice("Modem hangup");
		hungup = 1;
		status = EXIT_HANGUP;
		lcp_lowerdown(0);	/* serial link is no longer available */
		link_terminated(0);
		return;
	}

	if (len < PPP_HDRLEN)
	{
		dbglog("received short packet:%.*B", len, p);
		return;
	}

	dump_packet("rcvd", p, len);
	if (snoop_recv_hook) snoop_recv_hook(p, len);

	p += 2;				/* Skip address and control */
	GETSHORT(protocol, p);
	len -= PPP_HDRLEN;

    /* Toss all non-LCP packets unless LCP is OPEN. */
	if (protocol != PPP_LCP && lcp_fsm[0].state != OPENED)
	{
		dbglog("Discarded non-LCP packet when LCP not open");
		return;
	}

	/* Until we get past the authentication phase, toss all packets
	 * except LCP, LQR and authentication packets. */
	if (phase <= PHASE_AUTHENTICATE &&
		!(protocol == PPP_LCP || protocol == PPP_LQR ||
		  protocol == PPP_PAP || protocol == PPP_CHAP ||
		  protocol == PPP_EAP))
	{
		dbglog("discarding proto 0x%x in phase %d", protocol, phase);
		return;
	}

	/* Upcall the proper protocol input routine. */
	for (i = 0; (protp = protocols[i]) != NULL; ++i)
	{
		if (protp->protocol == protocol && protp->enabled_flag)
		{
			(*protp->input)(0, p, len);
			return;
		}
		if (protocol == (protp->protocol & ~0x8000) &&
			protp->enabled_flag && protp->datainput != NULL)
		{
			(*protp->datainput)(0, p, len);
			return;
		}
	}

	if (debug)
	{
		const char *pname = protocol_name(protocol);
		if (pname != NULL)
			warn("Unsupported protocol '%s' (0x%x) received", pname, protocol);
		else
			warn("Unsupported protocol 0x%x received", protocol);
	}
	lcp_sprotrej(0, p - PPP_HDRLEN, len + PPP_HDRLEN);
}

/*
 * ppp_send_config - configure the transmit-side characteristics of
 * the ppp interface.  Returns -1, indicating an error, if the channel
 * send_config procedure called error() (or incremented error_count
 * itself), otherwise 0.
 */
int ppp_send_config(int unit, int mtu, u_int32_t accm, int pcomp, int accomp)
{
	int errs;

	if (the_channel->send_config == NULL) return 0;
	errs = error_count;
	(*the_channel->send_config)(mtu, accm, pcomp, accomp);
	return (error_count != errs)? -1: 0;
}

/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.  Returns -1, indicating an error, if the channel
 * recv_config procedure called error() (or incremented error_count
 * itself), otherwise 0.
 */
int ppp_recv_config(int unit, int mru, u_int32_t accm, int pcomp, int accomp)
{
	int errs;

	if (the_channel->recv_config == NULL) return 0;
	errs = error_count;
	(*the_channel->recv_config)(mru, accm, pcomp, accomp);
	return (error_count != errs)? -1: 0;
}

/*
 * new_phase - signal the start of a new phase of pppd's operation.
 */
void new_phase(int p)
{
	phase = p;
	if (new_phase_hook) (*new_phase_hook)(p);
	notify(phasechange, p);
}

/*
 * die - clean up state and exit with the specified status.
 */
void die(int status)
{
	if (!dryrun)
	{
		print_link_stats();
		cleanup();
		notify(exitnotify, status);
		syslog(LOG_INFO, "Exit.");
	}
	exit(status);
}

/*
 * cleanup - restore anything which needs to be restored before we exit
 */
/* ARGSUSED */
static void cleanup()
{
	sys_cleanup();

	if (fd_ppp >= 0) the_channel->disestablish_ppp(devfd);
	if (the_channel->cleanup) (*the_channel->cleanup)();

	if (pidfilename[0] != 0 && unlink(pidfilename) < 0 && errno != ENOENT)
		d_warn("pppd: cleanup(): unable to delete pid file %s\n", pidfilename);
	pidfilename[0] = 0;
	if (linkpidfile[0] != 0 && unlink(linkpidfile) < 0 && errno != ENOENT)
		d_warn("pppd: cleanup(): unable to delete pid file %s\n", linkpidfile);
	linkpidfile[0] = 0;

	update_statusfile(NULL);

#ifdef USE_TDB
	if (pppdb != NULL) cleanup_db();
#endif
}

void print_link_stats()
{
	/* Print connect time and statistics. */
	if (link_stats_valid)
	{
 		int t = (link_connect_time + 5) / 6;    /* 1/10ths of minutes */
 		info("Connect time %d.%d minutes.", t/10, t%10);
 		info("Sent %u bytes, received %u bytes.", link_stats.bytes_out, link_stats.bytes_in);
	}
}

/*
 * update_link_stats - get stats at link termination.
 */
void update_link_stats(int u)
{
	struct timeval now;
	char numbuf[32];

	if (!get_ppp_stats(u, &link_stats) || gettimeofday(&now, NULL) < 0) return;
	link_connect_time = now.tv_sec - start_time.tv_sec;
	link_stats_valid = 1;

	slprintf(numbuf, sizeof(numbuf), "%u", link_connect_time);
	script_setenv("CONNECT_TIME", numbuf, 0);
	slprintf(numbuf, sizeof(numbuf), "%u", link_stats.bytes_out);
	script_setenv("BYTES_SENT", numbuf, 0);
	slprintf(numbuf, sizeof(numbuf), "%u", link_stats.bytes_in);
	script_setenv("BYTES_RCVD", numbuf, 0);
}


/*
 * hup - Catch SIGHUP signal.
 *
 * Indicates that the physical layer has been disconnected.
 * We don't rely on this indication; if the user has sent this
 * signal, we just take the link down.
 */
static void hup(int sig)
{
	d_dbg("pppd: Hangup (SIGHUP) (conn=%d)\n", conn_running);
}


/*
 * term - Catch SIGTERM signal and SIGINT signal (^C/del).
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
/*ARGSUSED*/
static void term(int sig)
{
	d_dbg("pppd: Terminating on signal %d (conn=%d)\n", sig, conn_running);
	got_sigterm = 1;
	if (need_jmp)
	{
		d_dbg("pppd: need jmp in term()!\n");
		need_jmp = 0;
		siglongjmp(sigjmp, 1);
	}
	else
	{
		d_dbg("pppd: lcp_close() by user request!\n");
		lcp_close(0, "User request");
	}
}

/*
 * chld - Catch SIGCHLD signal.
 * Sets a flag so we will call reap_kids in the mainline.
 */
static void chld(int sig)
{
	d_dbg("pppd: Child (SIGCHLD)\n");
}


/*
 * toggle_debug - Catch SIGUSR1 signal.
 *
 * Toggle debug flag.
 */
/*ARGSUSED*/
static void toggle_debug(int sig)
{
	d_dbg("pppd: toggle_debug()!\n");

	debug = !debug;
	if (debug) setlogmask(LOG_UPTO(LOG_DEBUG));
	else setlogmask(LOG_UPTO(LOG_WARNING));
}


/*
 * open_ccp - Catch SIGUSR2 signal.
 *
 * Try to (re)negotiate compression.
 */
/*ARGSUSED*/
static void open_ccp(int sig)
{
	d_dbg("pppd: SIGUSR2() --> open_ccp()\n");
	if (phase == PHASE_NETWORK || phase == PHASE_RUNNING)
	{
		ccp_fsm[0].flags = OPT_RESTART;	/* clears OPT_SILENT */
		(*ccp_protent.open)(0);
	}
}


/*
 * bad_signal - We've caught a fatal signal.  Clean up state and exit.
 */
static void bad_signal(int sig)
{
	static int crashed = 0;

	if (crashed) _exit(127);
	crashed = 1;
	d_error("pppd: Fatal signal %d\n", sig);
	notify(sigreceived, sig);
	die(127);
}

/*
 * safe_fork - Create a child process.  The child closes all the
 * file descriptors that we don't want to leak to a script.
 * The parent waits for the child to do this before returning.
 */
pid_t safe_fork()
{
	pid_t pid;
	int pipefd[2];
	char buf[1];

	if (pipe(pipefd) == -1) pipefd[0] = pipefd[1] = -1;
#ifdef EMBED
	pid = vfork();
#else
	pid = fork();
#endif
	if (pid < 0) return -1;
	if (pid > 0)
	{
		close(pipefd[1]);
		/* this read() blocks until the close(pipefd[1]) below */
		complete_read(pipefd[0], buf, 1);
		close(pipefd[0]);
		return pid;
	}
	sys_close();
#ifdef USE_TDB
	tdb_close(pppdb);
#endif
	notify(fork_notifier, 0);
	close(pipefd[0]);
	/* this close unblocks the read() call above in the parent */
	close(pipefd[1]);
	return 0;
}

/*
 * device_script - run a program to talk to the specified fds
 * (e.g. to run the connector or disconnector script).
 * stderr gets connected to the log fd or to the _PATH_CONNERRS file.
 */
int device_script(char * program, int in, int out, int dont_wait)
{
	int pid;
	int status = -1;
	int errfd;

	++conn_running;
#ifdef EMBED
	pid = vfork();
#else
	pid = safe_fork();
#endif
	if (pid < 0)
	{
		--conn_running;
		error("Failed to create child process: %m");
		return -1;
    }

	if (pid != 0)
	{
		if (dont_wait)
		{
#ifndef EMBED
			record_child(pid, program, NULL, NULL);
#endif
			fprintf(stderr, "pppd: %s %d\n", program, pid);
			pty_pid = pid;
			status = 0;
		}
		else
		{
			while (waitpid(pid, &status, 0) < 0)
			{
				if (errno == EINTR) continue;
				fatal("error waiting for (dis)connection process: %m");
			}
			--conn_running;
		}
		return (status == 0 ? 0 : -1);
	}

    /* here we are executing in the child */

	/* dup in and out to fds > 2 */
	{
		int fd1 = in, fd2 = out, fd3 = log_to_fd;

		in = dup(in);
		out = dup(out);
		if (log_to_fd >= 0)
		{
			errfd = dup(log_to_fd);
		}
		else
		{
			errfd = open(_PATH_CONNERRS, O_WRONLY | O_APPEND | O_CREAT, 0600);
		}
		close(fd1);
		close(fd2);
		close(fd3);
	}

    /* close fds 0 - 2 and any others we can think of */
	close(0);
	close(1);
	close(2);
	if (the_channel->close) (*the_channel->close)();
	closelog();
	close(fd_devnull);

	/* dup the in, out, err fds to 0, 1, 2 */
	dup2(in, 0);
	close(in);
	dup2(out, 1);
	close(out);
	if (errfd >= 0)
	{
		dup2(errfd, 2);
		close(errfd);
	}

	setuid(uid);
	if (getuid() != uid)
	{
		error("setuid failed");
		exit(1);
	}
	setgid(getgid());

#ifdef EMBED
	/* On uClinux we don't have a full shell, just call chat
	 * program directly (obviously it can't be a sh script!). */
	{
		char string[256];
		char *argv[16], *sp;
        int  argc = 0, prevspace = 1;

		strcpy(string, program);
		for (sp = string; (*sp != 0); )
		{
			if (prevspace && !isspace(*sp)) argv[argc++] = sp;
			if ((prevspace = isspace(*sp))) *sp = 0;
			sp++;
		}
		argv[argc] = 0;
		execv(argv[0], argv);
	}
#else
	execl("/bin/sh", "sh", "-c", program, (char *)0);
#endif
    error("could not exec /bin/sh: %m");
    exit(99);
    /* NOTREACHED */
}


/*
 * run-program - execute a program with given arguments,
 * but don't wait for it.
 * If the program can't be executed, logs an error unless
 * must_exist is 0 and the program file doesn't exist.
 * Returns -1 if it couldn't fork, 0 if the file doesn't exist
 * or isn't an executable plain file, or the process ID of the child.
 * If done != NULL, (*done)(arg) will be called later (within
 * reap_kids) iff the return value is > 0.
 */
pid_t run_program(char *prog, char **args, int must_exist, void (*done)(void *), void *arg)
{
	int pid;
	struct stat sbuf;

	/* First check if the file exists and is executable.
	 * We don't use access() because that would use the
	 * real user-id, which might not be root, and the script
	 * might be accessible only to root. */
	errno = EINVAL;
	if (stat(prog, &sbuf) < 0 || !S_ISREG(sbuf.st_mode) ||
		(sbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)
	{
		if (must_exist || errno != ENOENT) warn("Can't execute %s: %m", prog);
		return 0;
	}
#ifdef EMBED
	//+++ Mark 2009/1/15 05:12pm
	// Under vfork() all of the stack and memory are shared between parent and children. 
	// This creates a problem when we attempt to call the_channel->close to close fd before execve.
	// When the children the_channel->close closed the fd and changed the it to -1, it also changed the parent's fd to -1.
	// The fd tables apparently are not shared between parent and child; 
	// therefore, we ended up with a parent that has an open fd but no variable to reference it.
	// Hence, we change vfork() to fork() here.
//	pid = vfork();
	pid = fork();
	//--- Mark
#else
	pid = safe_fork();
#endif
	if (pid == -1)
	{
		error("Failed to create child process for %s: %m", prog);
		return -1;
	}
	if (pid != 0)
	{
		//+++Mark wait for children to terminate 2009/1/8 10:45am
		//This prevents ip-up script becomes zombie process that will result in ip-up not being called at reconnection.
		int pid_status;
		if (debug) dbglog("Script %s started (pid %d)", prog, pid);
//		record_child(pid, prog, done, arg);
//		return pid;
		if (waitpid(pid, &pid_status, 0) < 0)
		{
			error("error waiting for run_program fork children: %m");
		}
		return 0;
		//---Mark
	}

	/* Leave the current location */
	(void) setsid();	/* No controlling tty. */
	(void) umask (S_IRWXG|S_IRWXO);
	(void) chdir ("/");	/* no current directory. */
	setuid(0);		/* set real UID = root */
	setgid(getegid());

	/* Ensure that nothing of our device environment is inherited. */
	closelog();
	if (the_channel->close) (*the_channel->close)();

	/* Don't pass handles to the PPP device, even by accident. */
	dup2(fd_devnull, 0);
	dup2(fd_devnull, 1);
	dup2(fd_devnull, 2);
	close(fd_devnull);

#ifdef BSD
	/* Force the priority back to zero if pppd is running higher. */
	if (setpriority (PRIO_PROCESS, 0, 0) < 0) warn("can't reset priority to 0: %m");
#endif

	/* SysV recommends a second fork at this point. */

 	/* run the program */
	execve(prog, args, script_env);
	if (must_exist || errno != ENOENT)
	{
		/* have to reopen the log, there's nowhere else
		 * for the message to go. */
		reopen_log();
		syslog(LOG_ERR, "Can't execute %s: %m", prog);
		closelog();
	}
	_exit(-1);
}


/*
 * record_child - add a child process to the list for reap_kids
 * to use.
 */
void record_child(int pid, char *prog, void (*done)(void *), void *arg)
{
	struct subprocess *chp;

	++n_children;

	chp = (struct subprocess *) malloc(sizeof(struct subprocess));
	if (chp == NULL)
	{
		warn("losing track of %s process", prog);
	}
	else
	{
		chp->pid = pid;
		chp->prog = prog;
		chp->done = done;
		chp->arg = arg;
		chp->next = children;
		children = chp;
	}
}

/*
 * add_notifier - add a new function to be called when something happens.
 */
void add_notifier(struct notifier **notif, notify_func func, void *arg)
{
	struct notifier *np;

	np = malloc(sizeof(struct notifier));
	if (np == 0) novm("notifier struct");
	np->next = *notif;
	np->func = func;
	np->arg = arg;
	*notif = np;
}

/*
 * remove_notifier - remove a function from the list of things to
 * be called when something happens.
 */
void remove_notifier(struct notifier **notif, notify_func func, void *arg)
{
	struct notifier *np;

	for (; (np = *notif) != 0; notif = &np->next)
	{
		if (np->func == func && np->arg == arg)
		{
			*notif = np->next;
			free(np);
			break;
		}
	}
}

/*
 * notify - call a set of functions registered with add_notifier.
 */
void notify(struct notifier *notif, int val)
{
	struct notifier *np;

	while ((np = notif) != 0)
	{
		notif = np->next;
		(*np->func)(np->arg, val);
	}
}

/*
 * novm - log an error message saying we ran out of memory, and die.
 */
void novm(char *msg)
{
    fatal("Virtual memory exhausted allocating %s\n", msg);
}

/*
 * script_setenv - set an environment variable value to be used
 * for scripts that we run (e.g. ip-up, auth-up, etc.)
 */
void script_setenv(char *var, char *value, int iskey)
{
	size_t varl = strlen(var);
	size_t vl = varl + strlen(value) + 2;
	int i;
	char *p, *newstring;

	newstring = (char *) malloc(vl+1);
	if (newstring == 0) return;
	*newstring++ = iskey;
	slprintf(newstring, vl, "%s=%s", var, value);

	/* check if this variable is already set */
	if (script_env != 0)
	{
		for (i = 0; (p = script_env[i]) != 0; ++i)
		{
			if (strncmp(p, var, varl) == 0 && p[varl] == '=')
			{
#ifdef USE_TDB
				if (p[-1] && pppdb != NULL) delete_db_key(p);
#endif
				free(p-1);
				script_env[i] = newstring;
#ifdef USE_TDB
				if (iskey && pppdb != NULL) add_db_key(newstring);
				update_db_entry();
#endif
				return;
			}
		}
	}
	else
	{
		/* no space allocated for script env. ptrs. yet */
		i = 0;
		script_env = (char **) malloc(16 * sizeof(char *));
		if (script_env == 0) return;
		s_env_nalloc = 16;
	}

	/* reallocate script_env with more space if needed */
	if (i + 1 >= s_env_nalloc)
	{
		int new_n = i + 17;
		char **newenv = (char **) realloc((void *)script_env,
									new_n * sizeof(char *));
		if (newenv == 0) return;
		script_env = newenv;
		s_env_nalloc = new_n;
	}

	script_env[i] = newstring;
	script_env[i+1] = 0;

#ifdef USE_TDB
	if (pppdb != NULL)
	{
		if (iskey) add_db_key(newstring);
		update_db_entry();
	}
#endif
}

/*
 * script_unsetenv - remove a variable from the environment
 * for scripts.
 */
void script_unsetenv(char *var)
{
	int vl = strlen(var);
	int i;
	char *p;

	
	if (script_env == 0) return;
	for (i = 0; (p = script_env[i]) != 0; ++i)
	{
		if (strncmp(p, var, vl) == 0 && p[vl] == '=')
		{
#ifdef USE_TDB
			if (p[-1] && pppdb != NULL) delete_db_key(p);
#endif
			free(p-1);
			while ((script_env[i] = script_env[i+1]) != 0) ++i;
			break;
		}
	}
#ifdef USE_TDB
	if (pppdb != NULL) update_db_entry();
#endif
}

#ifdef USE_TDB
/*
 * update_db_entry - update our entry in the database.
 */
static void update_db_entry()
{
	TDB_DATA key, dbuf;
	int vlen, i;
	char *p, *q, *vbuf;

	if (script_env == NULL) return;
	vlen = 0;
	for (i = 0; (p = script_env[i]) != 0; ++i)
		vlen += strlen(p) + 1;
	vbuf = malloc(vlen);
	if (vbuf == 0) novm("database entry");
	q = vbuf;
	for (i = 0; (p = script_env[i]) != 0; ++i)
		q += slprintf(q, vbuf + vlen - q, "%s;", p);

	key.dptr = db_key;
	key.dsize = strlen(db_key);
	dbuf.dptr = vbuf;
	dbuf.dsize = vlen;
	if (tdb_store(pppdb, key, dbuf, TDB_REPLACE))
		error("tdb_store failed: %s", tdb_error(pppdb));

	if (vbuf)
		free(vbuf);
}

/*
 * add_db_key - add a key that we can use to look up our database entry.
 */
static void add_db_key(const char *str)
{
	TDB_DATA key, dbuf;

	key.dptr = (char *) str;
	key.dsize = strlen(str);
	dbuf.dptr = db_key;
	dbuf.dsize = strlen(db_key);
	if (tdb_store(pppdb, key, dbuf, TDB_REPLACE))
		error("tdb_store key failed: %s", tdb_error(pppdb));
}

/*
 * delete_db_key - delete a key for looking up our database entry.
 */
static void delete_db_key(const char *str)
{
	TDB_DATA key;

	key.dptr = (char *) str;
	key.dsize = strlen(str);
	tdb_delete(pppdb, key);
}

/*
 * cleanup_db - delete all the entries we put in the database.
 */
static void cleanup_db()
{
	TDB_DATA key;
	int i;
	char *p;

	key.dptr = db_key;
	key.dsize = strlen(db_key);
	tdb_delete(pppdb, key);
	for (i = 0; (p = script_env[i]) != 0; ++i)
		if (p[-1])
			delete_db_key(p);
}
#endif /* USE_TDB */

/* implemented in pppoe.c */
extern int pppoe_module_connect(int fd);

/* main routine */
int main(int argc, char * argv[])
{
	int i, t;
	char *p;
	struct passwd * pw;
	struct protent * protp;
	char numbuf[16];

	link_stats_valid = 0;
	new_phase(PHASE_INITIALIZE);
	script_env = NULL;

	/* Initialize syslog facilities */
	reopen_log();
	if (gethostname(hostname, MAXNAMELEN) < 0)
	{
		option_error("Couldn't get hostname: %m");
		exit(1);
	}
	hostname[MAXNAMELEN-1] = 0;

	/* make sure we don't create world or group writable files. */
	umask(umask(0777) | 022);

	uid = getuid();
	privileged = uid == 0;
	slprintf(numbuf, sizeof(numbuf), "%d", uid);
	script_setenv("ORIG_UID", numbuf, 0);
	ngroups = getgroups(NGROUPS_MAX, groups);

	/* Initialize magic number generator now so that protocols may
	 * use magic numbers in initialization. */
	magic_init();

	/* Initialize each protocol */
	for (i=0; (protp = protocols[i]) != NULL; i++) (*protp->init)(0);

	/* Initialize the default channel */
	tty_init();

	progname = *argv;
	
	/* Parse, in order, the system options file and the command line arguments. */
	if (!parse_args(argc-1, argv+1)) exit(EXIT_OPTION_ERROR);
	devnam_fixed = 1;	/* can no loger change device name */

	/* Work out the device name, if it hasn't already been specified,
	 * and parse the tty's options file. */
	if (the_channel->process_extra_options)
		(*the_channel->process_extra_options)();

	if (debug) setlogmask(LOG_UPTO(LOG_DEBUG));

	/* Check that we are running as root. */
	if (geteuid() != 0)
	{
		option_error("must be root to run %s, since it is not setuid-root",argv[0]);
		exit(EXIT_NOT_ROOT);
	}

	if (pppoe_discovery)
	{
		if (pppoe_module_connect(0)==0)
		{
			printf("yes\n");
			exit(0);
		}
		else
		{
			printf("no\n");
			exit(9);
		}
	}

	if (!ppp_available())
	{
		option_error("%s", no_ppp_msg);
		exit(EXIT_NO_KERNEL_SUPPORT);
	}

	/* Check that the options given are valid and consistent. */
	check_options();
	if (!sys_check_options()) exit(EXIT_OPTION_ERROR);
	auth_check_options();

#ifdef HAVE_MULTILINK
	mp_check_options();
#endif
	for (i=0; (protp = protocols[i]) != NULL; i++)
	{
		if (protp->check_options != NULL) (*protp->check_options)();
	}

	if (the_channel->check_options) (*the_channel->check_options)();

	if (dump_options || dryrun)
	{
		init_pr_log(NULL, LOG_INFO);
		print_options(pr_log, NULL);
		end_pr_log();
	}

	if (dryrun) die(0);

	/* Initialize system-dependent stuff. */
	sys_init();

	/* Make sure fds 0, 1, 2 are open to somewhere. */
	fd_devnull = open(_PATH_DEVNULL, O_RDWR);
	if (fd_devnull < 0) fatal("Coudn't open %s: %m", _PATH_DEVNULL);
	while (fd_devnull <= 2)
	{
		i = dup(fd_devnull);
		if (i < 0) fatal("Critical shortage of file descriptors: dup failed: %m");
		fd_devnull = i;
	}

	p = getlogin();
	if (p == NULL)
	{
		pw = getpwuid(uid);
		if (pw != NULL && pw->pw_name != NULL) p = pw->pw_name;
		else p = "(unknown)";
	}

	syslog(LOG_NOTICE, "pppd %s started by %s, uid %d", VERSION, p, uid);
	script_setenv("PPPLOGNAME", p, 0);
	slprintf(numbuf, sizeof(numbuf), "%d", getpid());
	script_setenv("PPPD_PID", numbuf, 1);
	d_dbg("pppd: pid=%d\n", getpid());

	setup_signals();

	/* crate a link pid file now.... David */
	create_linkpidfile(getpid());

	/* If we're doing dail-on-demand, setup the interface now. */
	if (demand)
	{
		/* Open the loopback channel and set it up to be the ppp interface. */
		fd_loop = open_ppp_loopback();
		set_ifunit(1);
		/* Configure the interface and mark it up, etc. */
		demand_conf();
		create_linkpidfile(getpid());
	}

	//got_sigterm = 0;
	while (!got_sigterm)
	{
		listen_time = 0;
		need_holdoff = 1;
		devfd = -1;
		status = EXIT_OK;
		++unsuccess;

		d_dbg("pppd: main loop ......................\n");

		if (demand)
		{
			/* Don't do anything until we see some activity. */
			d_dbg("pppd: entering demand blocking!\n");
			syslog(ALOG_DEBUG|LOG_DEBUG, "pppd: entering demand blocking!");
			update_statusfile("on demand");
			new_phase(PHASE_DORMANT);
			demand_unblock();
			eloop_init(NULL);
			eloop_register_read_sock(fd_loop, read_ppp_loop, NULL, NULL);
			need_jmp = 1;
			if (sigsetjmp(sigjmp, 1) == 0) eloop_run();
			d_dbg("pppd: exit eloop_run(), got_sigterm = %d\n", got_sigterm);
			eloop_destroy();
			if (got_sigterm) break;
			//+++ michael_lee: (2013.03.05 16:35:35)
			// because of conntrack. the queued packed will be send
			// with private ip. so we clean the queue to avoid this
			// problem.
			//demand_block();
			demand_discard();
			//--- michael_lee
			info("Starting link");
			syslog(ALOG_DEBUG|LOG_DEBUG, "Starting link");
		}

		update_statusfile("connecting");
		new_phase(PHASE_SERIALCONN);

		/* init eloop */
		eloop_init(NULL);

		/* connect to pty and return slave pty to establish ppp interface. */
		devfd = the_channel->connect();
		d_dbg("pppd: devfd = %d\n", devfd);
		if (devfd < 0) goto fail;

		/* got slave pty, establish ppp interface. */
		fd_ppp = the_channel->establish_ppp(devfd);
		d_dbg("pppd: fd_ppp = %d\n", fd_ppp);
		if (fd_ppp < 0)
		{
			status = EXIT_FATAL_ERROR;
			goto disconnect;
		}

		/* create the pid file, now that we've obtained a ppp interface */
		if (!demand) create_linkpidfile(getpid());
		if (!demand && ifunit >= 0) set_ifunit(1);

		/* Start opening the connection and wait for
		 * incoming events (reply, timeout, etc.). */
		if (ifunit >= 0) d_info("pppd: Connect: %s <--> %s\n", ifname, ppp_devnam);
		else d_info("pppd: Starting negotiatioin on %s\n", ppp_devnam);
		gettimeofday(&start_time, NULL);
		script_unsetenv("CONNECT_TIME");
		script_unsetenv("BYTES_SENT");
		script_unsetenv("BYTES_RCVD");

		d_dbg("pppd: register ppp fd (%d)\n", fd_ppp);
		eloop_register_read_sock(fd_ppp, get_ppp_input, NULL, NULL);

		lcp_lowerup(0);
		lcp_open(0);

		status = EXIT_NEGOTIATION_FAILED;
		new_phase(PHASE_ESTABLISH);
		need_jmp = 0;
		eloop_run();
		d_dbg("pppd: exit main eloop_run()\n");
		print_link_stats();
		update_statusfile("disconnecting");

		/* Delete pid file before disestablishing ppp. Otherwise it
		 * can happen that another pppd gets the same unit and then
		 * we delete its pid file. */
		if (!demand)
		{
			if (pidfilename[0] != 0 &&
				unlink(pidfilename) < 0 && errno != ENOENT)
			{
				d_warn("pppd: unable to delete pid file %s\n");
			}
			pidfilename[0] = 0;
		}

		/* If we may want to bring the link up again, transfer
		 * the ppp unit back to the loopback. Set the
		 * real serial device back to its normal mode of operation. */
		clean_check();
		the_channel->disestablish_ppp(devfd);
		d_dbg("pppd: disestablished.............\n");
		fd_ppp = -1;
		sleep(1);
		if (!hungup) lcp_lowerdown(0);
		if (!demand) script_unsetenv("IFNAME");

		/* Run disconnector script, if requested.
		 * XXX we may not be able to do this if the line has hung up! */
disconnect:
		new_phase(PHASE_DISCONNECT);
		the_channel->disconnect();
		d_dbg("pppd: disconnected.............\n");
fail:
		update_statusfile("disconnected"); /* kwest: move to here is more suitable. */
		if (the_channel->cleanup) the_channel->cleanup();
		d_dbg("pppd: cleanup.............\n");
		if (!demand)
		{
			if (pidfilename[0] != 0 && unlink(pidfilename) < 0 && errno != ENOENT)
				d_warn("pppd: unable to delete pid file %s.\n", pidfilename);
			pidfilename[0] = 0;
		}

		eloop_destroy();
		d_dbg("pppd: eloop_destroyed...............\n");

		if (!persist || (maxfail > 0 && unsuccess >= maxfail))
			break;

		d_dbg("pppd: holdoff ...............\n");
		t = need_holdoff ? holdoff : 0;
		if (t > 0 && !got_sigterm)
		{
			new_phase(PHASE_HOLDOFF);
			eloop_init(NULL);
			TIMEOUT(holdoff_end, NULL, t);
			d_dbg("pppd: entering holdoff for %d secs.\n", t);
			do
			{
				need_jmp = 1;
				if (sigsetjmp(sigjmp, 1) == 0) eloop_run();
				if (got_sigterm) new_phase(PHASE_DORMANT);
			} while (phase == PHASE_HOLDOFF);
			eloop_destroy();
			if (!persist) break;
		}
	}
	
	die(status);
	return 0;
}
