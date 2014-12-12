/* vi: set sw=4 ts=4: */
/*
 * tty.c - code for handling serial ports in pppd.
 *
 * Copyright (C) 2000-2002 Paul Mackerras. All rights reserved.
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
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Portions derived from main.c, which is:
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

#define RCSID	"$Id: tty.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $"

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

#include "pppd.h"
#include "fsm.h"
#include "lcp.h"
#include "dtrace.h"

void tty_process_extra_options(void);
void tty_check_options(void);
int  connect_tty(void);
void disconnect_tty(void);
void tty_close_fds(void);
void cleanup_tty(void);
void tty_do_send_config(int, u_int32_t, int, int);

static int setspeed(char *, char **, int);
static int setescape(char **);
static void printescape(option_t *, void (*)(void *, char *,...),void *);
static void maybe_relock(void *, int);

static int pty_master;			/* fd for master side of pty */
static int pty_slave;			/* fd for slave side of pty */
//static int real_ttyfd;			/* fd for actual serial port (not pty) */
#ifdef HAVE_PPP3G_CHAT
static void finish_tty (void);
static int real_ttyfd;			/* fd for actual serial port (not pty) */

extern int ppp3g_module_connect(int *fd, char* connector);
extern void ppp3g_module_disconnect(void); 
#endif
static int ttyfd;				/* Serial port file descriptor */
static char speed_str[16];		/* Serial port speed as string */

mode_t tty_mode = (mode_t)-1;	/* Original access permissions to tty */
int baud_rate;					/* Actual bits/second for serial device */
char *callback_script;			/* script for doing callback */
int charshunt_pid;				/* Process ID for charshunt */
int locked;						/* lock() has succeeded */
struct stat devstat;			/* result of stat() on devnam */

/* option variables */
int		crtscts = 0;			/* Use hardware flow control */
bool	modem = 1;				/* Use modem control lines */
int		inspeed = 0;			/* Input/Output speed requested */
bool	lockflag = 0;			/* Create lock file to lock the serial dev */
char	*initializer = NULL;	/* Script to initialize physical link */
char	*connect_script = NULL;	/* Script to establish physical link */
char	*disconnect_script = NULL; /* Script to disestablish physical link */
char	*welcomer = NULL;		/* Script to run after phys link estab. */
char	*ptycommand = NULL;		/* Command to run on other side of pty */
bool	notty = 0;				/* Stdin/out is not a tty */
//char	*record_file = NULL;	/* File to record chars sent/received */
int		max_data_rate;			/* max bytes/sec through charshunt */
bool	sync_serial = 0;		/* Device is synchronous serial device */
char	*pty_socket = NULL;		/* Socket to connect to pty */
int		using_pty = 0;			/* we're allocating a pty as the device */

extern uid_t uid;
extern int kill_link;

/* XXX */
extern int privopen;		/* don't lock, open device as root */

u_int32_t xmit_accm[8];		/* extended transmit ACCM */

/* option descriptors */
option_t tty_options[] = {
    /* device name must be first, or change connect_tty() below! */

    { "tty speed", o_wild, (void *) &setspeed,
      "Baud rate for serial port",
      OPT_PRIO | OPT_NOARG | OPT_A2STRVAL | OPT_STATIC, speed_str },

    { "init", o_string, &initializer,
      "A program to initialize the device", OPT_PRIO | OPT_PRIVFIX },

    { "connect", o_string, &connect_script,
      "A program to set up a connection", OPT_PRIO | OPT_PRIVFIX },

    { "disconnect", o_string, &disconnect_script,
      "Program to disconnect serial device", OPT_PRIO | OPT_PRIVFIX },

    { "welcome", o_string, &welcomer,
      "Script to welcome client", OPT_PRIO | OPT_PRIVFIX },

    { "crtscts", o_int, &crtscts,
      "Set hardware (RTS/CTS) flow control",
      OPT_PRIO | OPT_NOARG | OPT_VAL(1) },
    { "cdtrcts", o_int, &crtscts,
      "Set alternate hardware (DTR/CTS) flow control",
      OPT_PRIOSUB | OPT_NOARG | OPT_VAL(2) },
    { "nocrtscts", o_int, &crtscts,
      "Disable hardware flow control",
      OPT_PRIOSUB | OPT_NOARG | OPT_VAL(-1) },
    { "-crtscts", o_int, &crtscts,
      "Disable hardware flow control",
      OPT_PRIOSUB | OPT_ALIAS | OPT_NOARG | OPT_VAL(-1) },
    { "nocdtrcts", o_int, &crtscts,
      "Disable hardware flow control",
      OPT_PRIOSUB | OPT_ALIAS | OPT_NOARG | OPT_VAL(-1) },

    { "modem", o_bool, &modem,
      "Use modem control lines", OPT_PRIO | 1 },
    { "local", o_bool, &modem,
      "Don't use modem control lines", OPT_PRIOSUB | 0 },

    { "sync", o_bool, &sync_serial,
      "Use synchronous HDLC serial encoding", 1 },

    { "datarate", o_int, &max_data_rate,
      "Maximum data rate in bytes/sec (with pty, notty or record option)",
      OPT_PRIO },

    { "escape", o_special, (void *)setescape,
      "List of character codes to escape on transmission",
      OPT_A2PRINTER, (void *)printescape },

    { NULL }
};


struct channel tty_channel =
{
	tty_options,
	&tty_process_extra_options,
	&tty_check_options,
	&connect_tty,
	&disconnect_tty,
	&tty_establish_ppp,
	&tty_disestablish_ppp,
	&tty_do_send_config,
	&tty_recv_config,
	&cleanup_tty,
	&tty_close_fds
};

/*
 * setspeed - Set the serial port baud rate.
 * If doit is 0, the call is to check whether this option is
 * potentially a speed value.
 */
static int
setspeed(arg, argv, doit)
    char *arg;
    char **argv;
    int doit;
{
    char *ptr;
    int spd;

    spd = strtol(arg, &ptr, 0);
    if (ptr == arg || *ptr != 0 || spd == 0)
        return 0;
    if (doit) {
        inspeed = spd;
        slprintf(speed_str, sizeof(speed_str), "%d", spd);
    }
    return 1;
}

/*
 * setescape - add chars to the set we escape on transmission.
 */
static int setescape(char **argv)
{
	int n, ret;
	char *p, *endp;

	p = *argv;
	ret = 1;
	while (*p)
	{
		n = strtol(p, &endp, 16);
		if (p == endp)
		{
			option_error("escape parameter contains invalid hex number '%s'", p);
			return 0;
		}
		p = endp;
		if (n < 0 || n == 0x5E || n > 0xFF)
		{
			option_error("can't escape character 0x%x", n);
			ret = 0;
		}
		else
			xmit_accm[n >> 5] |= 1 << (n & 0x1F);
		while (*p == ',' || *p == ' ') ++p;
	}
	lcp_allowoptions[0].asyncmap = xmit_accm[0];
	return ret;
}

static void printescape(option_t *opt, void (*printer)(void *, char *, ...), void *arg)
{
	int n;
	int first = 1;

	for (n = 0; n < 256; ++n)
	{
		if (n == 0x7d) n += 2;		/* skip 7d, 7e */
		if (xmit_accm[n >> 5] & (1 << (n & 0x1f)))
		{
			if (!first) printer(arg, ",");
			else first = 0;
			printer(arg, "%x", n);
		}
	}
	if (first) printer(arg, "oops # nothing escaped");
}

/*
 * tty_init - do various tty-related initializations.
 */
void tty_init()
{
    add_notifier(&pidchange, maybe_relock, 0);
    the_channel = &tty_channel;
    xmit_accm[3] = 0x60000000;
}

/*
 * tty_process_extra_options - work out which tty device we are using
 * and read its options file.
 */
void tty_process_extra_options()
{
}

/*
 * tty_check_options - do consistency checks on the options we were given.
 */
void tty_check_options()
{
}

/* implemented in pppoe.c */
extern int pppoe_module_connect(int fd);
extern void pppoe_module_disconnect(void);

extern int pptp_module_connect(int fd);
extern void pptp_module_disconnect(void);

extern int l2tp_module_connect(int fd);
extern void l2tp_module_disconnect(void);
extern void send_config_pppol2tp(int mtu, u_int32_t asyncmap, int pcomp, int accomp);

extern int l2tp_kernel_mode;
extern int pppol2tp_fd;

/*
 * connect_tty - get the serial port ready to start doing PPP.
 * That is, open the serial port, set its speed and mode, and run
 * the connector and/or welcomer.
 */
int connect_tty()
{
	char *connector;
	char numbuf[16];

	d_dbg("pppd: >>> connect_tty()\n");
	
	/* Get a pty master/slave pair if the pty, notty, socket,
	 * or record options were specified. */
	strlcpy(ppp_devnam, devnam, sizeof(ppp_devnam));
	d_dbg("pppd: ppp_devnam = [%s]\n", ppp_devnam);
	
	pty_master = -1;
	pty_slave = -1;
#ifdef HAVE_PPP3G_CHAT
	real_ttyfd = -1;
	if( pty_module != 3 /* ppp 3g */)
	{
#endif

		if (!get_pty(&pty_master, &pty_slave, ppp_devnam, uid))
		{
			d_error("pppd: Couldn't allocate pseudo-tty\n");
			status = EXIT_FATAL_ERROR;
			return -1;
		}
		d_dbg("pppd: get pty master=%d, slave=%d\n", pty_master, pty_slave);
		
		set_up_tty(pty_slave, 1);
#ifdef HAVE_PPP3G_CHAT
	}
#endif

	/* Lock the device if we've been asked to. */
	status = EXIT_LOCK_FAILED;

	/* Open the serial device and set it up to be the ppp interface.
	 * First we open it in non-blocking mode so we can set the
	 * various termios flags appropriately.  If we aren't dialling
	 * out and we want to use the modem lines, we reopen it later
	 * in order to wait for the carrier detect signal from the modem. */
	hungup = 0;
	kill_link = 0;
	connector = connect_script;

	/* If the pty, socket, notty and/or record option was specified,
	 * start up the character shunt now. */
	status = EXIT_PTYCMD_FAILED;
	switch (pty_module)
	{
	case 0: /* using pty_pppoe module */
		if (pppoe_module_connect(pty_master) < 0)
		{
			hungup = 1;
			return -1;
		}
		/* pppd will use slave pty. */
		ttyfd = pty_slave;
		break;
	case 1: /* using pty_pptp module */
		if (pptp_module_connect(pty_master) < 0)
		{
			hungup = 1;
			return -1;
		}
		/* pppd will use slave pty. */
		ttyfd = pty_slave;
		break;
	case 2:	/* using pty_l2tp module */
		if (l2tp_module_connect(pty_master) < 0)
		{
			hungup = 1;
			return -1;
		}
		if (l2tp_kernel_mode == 1)
		{
			ttyfd = pppol2tp_fd;
		} else {
			/* pppd will use slave pty. */
			ttyfd = pty_slave;
		}
		break;
#ifdef HAVE_PPP3G_CHAT
	case 3:
		if (ppp3g_module_connect(&real_ttyfd , connector) < 0)
		{
			hungup = 1;
			return -1;
		}
		ttyfd = real_ttyfd;
#endif
		break;
	default:
		fatal("!!! SOULD NOT BE HERE !!!");
		die(1);
		break;
	}

#if 0
	/* run connection script */
	if ((connector && connector[0]) || initializer)
	{
		if (initializer && initializer[0]) 
		{
			if (device_script(initializer, ttyfd, ttyfd, 0) < 0) 
			{
				d_error("pppd: Initializer script failed\n");
				status = EXIT_INIT_FAILED;
				return -1;
			}
			if (kill_link) 
			{
				disconnect_tty();
				return -1;
			}
			d_info("pppd: Serial port initialized.\n");
		}
		if (connector && connector[0]) 
		{
			if (device_script(connector, ttyfd, ttyfd, 0) < 0) 
			{
				d_error("pppd: Connect script failed\n");
				status = EXIT_CONNECT_FAILED;
				return -1;
			}
			if (kill_link) 
			{
				disconnect_tty();
				return -1;
			}
			d_info("pppd: Serial connection established.\n");
		}

		//if (doing_callback == CALLBACK_DIALIN) connector = NULL;
	}
#endif

	slprintf(numbuf, sizeof(numbuf), "%d", baud_rate);
	d_dbg("pppd: baud rate: %s\n", numbuf);
	script_setenv("SPEED", numbuf, 0);

	/* run welcome script, if any */
	if (welcomer && welcomer[0])
	{
		if (device_script(welcomer, ttyfd, ttyfd, 0) < 0)
		{
			d_warn("Welcome script failed");
		}
	}

	/* If we are initiating this connection, wait for a short
	 * time for something from the peer.  This can avoid bouncing
	 * our packets off his tty before he has it set up. */
	if (connector != NULL) listen_time = connect_delay;

	return ttyfd;
}

void disconnect_tty()
{
	d_dbg("pppd: >>> disconnect_tty()\n");
	switch (pty_module)
	{
		case 0:
			pppoe_module_disconnect();
			break;
		case 1:
			pptp_module_disconnect();
			break;
		case 2:
			l2tp_module_disconnect();
			break;
#ifdef HAVE_PPP3G_CHAT
		case 3:
			ppp3g_module_disconnect();
			break;
#endif
	}
	return;
}

void tty_close_fds()
{
	int i;
	d_dbg("pppd: tty_close_fds(master=%d, slave=%d)\n", pty_master, pty_slave);
	if (pty_master >= 0)
	{
		i = close(pty_master);
		d_dbg("pppd: closing pty_master return %d\n", i);
	}
	if (pty_slave >= 0)
	{
		i = close(pty_slave);
		d_dbg("pppd: closing pty_slave return %d\n", i);
	}
#ifdef HAVE_PPP3G_CHAT
	if (real_ttyfd >= 0) 
	{
        finish_tty();		
        if( real_ttyfd >= 0 )
        {
        	close(real_ttyfd);
			real_ttyfd = -1;
		}
	}
#endif
	pty_master = pty_slave = -1;
	/* N.B. ttyfd will == either pty_slave or real_ttyfd */
}

void cleanup_tty()
{
	tty_close_fds();
	if (locked)
	{
		unlock();
		locked = 0;
	}
}

/*
 * tty_do_send_config - set transmit-side PPP configuration.
 * We set the extended transmit ACCM here as well.
 */
void tty_do_send_config(int mtu, u_int32_t accm, int pcomp, int accomp)
{
	d_dbg("pppd: >>> tty_do_send_config()\n");
	if (l2tp_kernel_mode == 1)
	{
		send_config_pppol2tp(mtu, accm, pcomp, accomp);
		return;
	}
	tty_set_xaccm(xmit_accm);
	tty_send_config(mtu, accm, pcomp, accomp);
}

#ifdef HAVE_PPP3G_CHAT
/*
 * finish_tty - restore the terminal device to its original settings
 */
static void finish_tty()
{
//	/* drop dtr to hang up */
//	if (!default_device && modem) 
//	{
//		setdtr(real_ttyfd, 0);
//		/*
//		 * This sleep is in case the serial port has CLOCAL set by default,
//		 * and consequently will reassert DTR when we close the device.
//		 */
//		sleep(1);
//	}
//
//	restore_tty(real_ttyfd);

	if (tty_mode != (mode_t) -1) 
	{
		if (fchmod(real_ttyfd, tty_mode) != 0) 
		{
			/* XXX if devnam is a symlink, this will change the link */
			chmod(devnam, tty_mode);
		}
	}

	close(real_ttyfd);
	real_ttyfd = -1;
}
#endif

/*
 * maybe_relock - our PID has changed, maybe update the lock file.
 */
static void
maybe_relock(arg, pid)
    void *arg;
    int pid;
{
    if (locked)
	relock(pid);
}

