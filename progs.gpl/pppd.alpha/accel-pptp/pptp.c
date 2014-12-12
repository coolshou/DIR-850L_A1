/***************************************************************************
 *   Copyright (C) 2006 by Kozlov D.   *
 *   xeb@mail.ru   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "pppd.h"

#include "pptp_msg.h"
#include "pptp_ctrl.h"
#include <net/if.h>
#include <net/ethernet.h>
#include "if_pppox.h"

#include "eloop.h"
#include "dtrace.h"

#ifndef N_HDLC
#include <linux/termios.h>
#endif


static PPTP_CONN * g_conn = NULL;
struct in_addr localbind = { INADDR_NONE };
static int pptp_fd = -1;
static int pptp_gre_fd = -1;
static u_int16_t pptp_gre_call_id, pptp_gre_peer_call_id;

static int pptp_ready = 0;
static int pptp_exit = 0;

char *pptp_server_ip=NULL;
char *pptp_client=NULL;
int pptp_sync;
int pptp_window=10;
int pptp_timeout=100000;
char pptp_localbind[];
extern void pptp_handler(int sock, void * eloop_ctx, void * sock_ctx);
extern void modem_hungup(void);
extern int hungup;
extern int pptp_destroy_conn;

extern u_int32_t exclude_peer;  /* defined in IPCP. */
static int pptp_sync_extra = 0; 
static option_t Options[] =
{
    { "pptp_server_ip", o_string, &pptp_server_ip,
      "PPTP Server" },
    { "pptp_client", o_string, &pptp_client,
      "PPTP Client" },
/*
    { "pptp_sock",o_int, &pptp_sock,
      "PPTP socket" },
    { "pptp_phone", o_string, &pptp_phone,
      "PPTP Phone number" },
*/
    { "pptp_window", o_int, &pptp_window,
      "PPTP sliding window size" },
    { "pptp_timeout", o_int, &pptp_timeout,
      "timeout for waiting reordered packets and acks"},
    { "sync", o_int, &pptp_sync,
	  "Enable Synchronous HDLC (pppd must use it too)",
	  OPT_INC | OPT_NOARG | 1 },
    { NULL }
};

static option_t Options_extra[] =
{
	{ "pptp_sync", o_int, &pptp_sync_extra,
	  "Enable Synchronous HDLC (pppd must use it too)",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pptp_localbind", o_string, pptp_localbind,
	  "Bind to specified IP address instead of wildcard",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
    { NULL }

};

static int pptp_connect(void);
static void pptp_disconnect(void);

struct channel pptp_channel = {
    options: Options,
    //process_extra_options: &PPPOEDeviceOptions,
    check_options: NULL,
    connect: &pptp_connect,
    disconnect: &pptp_disconnect,
    establish_ppp: &generic_establish_ppp,
    disestablish_ppp: &generic_disestablish_ppp,
    //send_config: &pptp_send_config,
    //recv_config: &pptp_recv_config,
    close: NULL,
    cleanup: NULL
};


/* init_vars */
static void init_vars(void)
{
	g_conn = NULL;
	pptp_fd = -1;
	pptp_gre_fd = -1;

	pptp_ready = pptp_exit = 0;
}

static void call_callback(PPTP_CONN * conn, PPTP_CALL * call, enum call_state state)
{
	switch (state)
	{
	case CALL_OPEN_RQST:
		d_dbg("pptp: call_callback(): CALL_OPEN_RQST! (We should not see this packet!)\n");
		break;
	case CALL_OPEN_DONE:
		d_dbg("pptp: call_callback(): CALL_OPEN_DONE!\n");
		pptp_call_get_ids(conn, call, &pptp_gre_call_id, &pptp_gre_peer_call_id);
		eloop_terminate();
		pptp_ready = 1;
		break;
	case CALL_OPEN_FAIL:
		d_dbg("pptp: call_callback(): CALL_OPEN_FAIL!\n");
		pptp_ready = 0;
		pptp_conn_close(conn, PPTP_STOP_NONE);
		break;
	case CALL_CLOSE_RQST:
		d_dbg("pptp: call_callback(): CALL_CLOSE_RQST!\n");
		break;
	case CALL_CLOSE_DONE:
		d_dbg("pptp: call_callback(): CALL_CLOSE_DONE!\n");
		pptp_ready = 0;
		if (!pptp_destroy_conn) pptp_conn_close(conn, PPTP_STOP_NONE);
		break;
	}
}

static void conn_callback(PPTP_CONN * conn, enum conn_state state)
{
	switch (state)
	{
	case CONN_OPEN_RQST:
		d_dbg("pptp: conn_callback(): CONN_OPEN_RQST!\n");
		break;
	case CONN_OPEN_DONE:
		d_dbg("pptp: conn_callback(): CONN_OPEN_DONE!\n");
		pptp_call_open(conn, pptp_gre_call_id, call_callback, NULL);
		break;
	case CONN_OPEN_FAIL:
		d_dbg("pptp: conn_callback(): CONN_OPEN_FAIL!\n");
		eloop_terminate();
		break;
	case CONN_CLOSE_RQST:
		d_dbg("pptp: conn_callback(): CONN_CLOSE_RQST!\n");
		pptp_ready = 0;
		break;
	case CONN_CLOSE_DONE:
		d_dbg("pptp: conn_callback(): CONN_CLOSE_DONE!\n");
		modem_hungup();
		eloop_terminate();
		g_conn = NULL;
		if (pptp_fd > 0) close(pptp_fd);
		if (pptp_gre_fd > 0) close(pptp_gre_fd);
		pptp_fd = -1;
		pptp_gre_fd = -1;
		pptp_ready = 0;
		pptp_exit = 1;
		break;
	}
}

static int open_inetsock(struct in_addr inetaddr)
{
	struct sockaddr_in dest;
	int s;

	d_dbg("pptp: >>> openSocket()\n");
	
	dest.sin_family = AF_INET;
	dest.sin_port = htons(PPTP_PORT);
	dest.sin_addr = inetaddr;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		d_warn("pptp: open_inetsocket(): socket: %s\n", strerror(errno));
		return s;
	}

	if (connect(s, (struct sockaddr *)&dest, sizeof(dest)) < 0)
	{
		d_warn("pptp: open_inetsocket(): connect: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	d_dbg("pptp: openSocket(%d)\n", s);
	return s;
}

static struct in_addr get_ip_address(char * name)
{
    struct in_addr retval;
    struct hostent * host = gethostbyname(name);
    retval.s_addr = -1;
    if (host == NULL)
    {
        if (h_errno == HOST_NOT_FOUND) d_error("pptp: gethostbyname '%s' : HOST NOT FOUND\n", name);
        else if (h_errno == NO_ADDRESS) d_error("pptp: gethostbyname '%s' : NO IP ADDRESS\n", name);
        else d_error("pptp: gethostbyname '%s' : name server error\n", name);
        return retval;
    }
    if (host->h_addrtype != AF_INET)
    {
        d_error("pptp: Host '%s' has non-internet address\n", name);
        return retval;
    }
    memcpy(&retval.s_addr, host->h_addr, sizeof(retval.s_addr));
    return retval;
}


static int pptp_start_client(void)
{
	int len;
	struct sockaddr_pppox src_addr,dst_addr;
	struct in_addr inetaddr;
	PPTP_CONN *conn = NULL;

        /* Get server IP address */
        inetaddr = get_ip_address(pptp_server_ip);

        /* record the server ip in 'exclude_peer' (IPCP).
         * This IP can not be used as peer IP.
         * by David Hsieh <david_hsieh@alphanetworks.com> */
         exclude_peer = inetaddr.s_addr;
         d_dbg("%s: exclude_peer = %08x, pptp_server_ip=%s\n",__FUNCTION__, exclude_peer, pptp_server_ip);

	dst_addr.sa_addr.pptp.sin_addr=inetaddr;
	{
		int sock;
		struct sockaddr_in addr;
		len=sizeof(addr);
		addr.sin_addr=dst_addr.sa_addr.pptp.sin_addr;
		addr.sin_family=AF_INET;
		addr.sin_port=htons(1700);
		sock=socket(AF_INET,SOCK_DGRAM,0);
		if (connect(sock,(struct sockaddr*)&addr,sizeof(addr)))
		{
			d_error("PPTP: connect failed (%s)\n",strerror(errno));
			return -1;
		}
		getsockname(sock,(struct sockaddr*)&addr,&len);
		src_addr.sa_addr.pptp.sin_addr=addr.sin_addr;
		close(sock);
	}
	//info("PPTP: connect server=%s\n",inet_ntoa(conn.sin_addr));
	//conn.loc_addr.s_addr=INADDR_NONE;
	//conn.timeout=1;
	//conn.window=pptp_window;

	src_addr.sa_family=AF_PPPOX;
	src_addr.sa_protocol=PX_PROTO_PPTP;
	src_addr.sa_addr.pptp.call_id=0;

	dst_addr.sa_family=AF_PPPOX;
	dst_addr.sa_protocol=PX_PROTO_PPTP;
	dst_addr.sa_addr.pptp.call_id=0;

	pptp_gre_fd=socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_PPTP);
	if (pptp_gre_fd<0)
	{
		d_error("PPTP: failed to create PPTP gre socket (%s)\n",strerror(errno));
		return -1;
	}
	if (setsockopt(pptp_gre_fd,0,PPTP_SO_WINDOW,&pptp_window,sizeof(pptp_window)))
		d_warn("PPTP: failed to setsockopt PPTP_SO_WINDOW (%s)\n",strerror(errno));
	if (setsockopt(pptp_gre_fd,0,PPTP_SO_TIMEOUT,&pptp_timeout,sizeof(pptp_timeout)))
		d_warn("PPTP: failed to setsockopt PPTP_SO_TIMEOUT (%s)\n",strerror(errno));
	if (bind(pptp_gre_fd,(struct sockaddr*)&src_addr,sizeof(src_addr)))
	{
		d_error("PPTP: failed to bind PPTP gre socket (%s)\n",strerror(errno));
		return -1;
	}
	len=sizeof(src_addr);
	getsockname(pptp_gre_fd,(struct sockaddr*)&src_addr,&len);
	pptp_gre_call_id=src_addr.sa_addr.pptp.call_id;

        if ((pptp_fd = open_inetsock(inetaddr)) < 0) return -1;
	if ((conn = pptp_conn_open(pptp_fd, 1, conn_callback)) == NULL)
	{
		d_error("pptp: pptp_conn_open failed!\n");
		close(pptp_fd);
		close(pptp_gre_fd);
		return -1;
	}

	g_conn = conn;

	pptp_ready = pptp_exit = 0;
	eloop_register_read_sock(pptp_fd, pptp_handler, NULL, conn);
	eloop_run();
	if (pptp_ready)
	{
            d_dbg("pptp: pptp connected!!\n");
            dst_addr.sa_addr.pptp.call_id = pptp_gre_peer_call_id;
            if (connect(pptp_gre_fd,(struct sockaddr*)&dst_addr,sizeof(dst_addr)))
            {
                d_error("PPTP: failed to connect PPTP gre socket (%s)\n",strerror(errno));
                return -1;
            }
            sprintf(ppp_devnam,"pptp (%s)",pptp_server_ip);

            eloop_continue();
            return pptp_gre_fd;
	}
	else
	{
		return -1;
	}
}

static int pptp_connect(void)
{
	d_dbg("pppd: >>> pptp_connect()\n");
	init_vars();
	return pptp_start_client();

}

static void pptp_disconnect(void)
{
	d_dbg("pppd: >>> pptp_disconnect()\n");

	hungup = 1;
	if (g_conn) pptp_conn_destroy(g_conn);

	exclude_peer = 0;
	d_dbg("%s: clear exlucde_peer!\n",__FUNCTION__);

	dassert(g_conn == NULL);
	dassert(pptp_fd < 0);

}

void module_pptp_init(void)
{
	d_dbg("pptp: >>> module_pptp_init()\n");
	add_options(Options_extra);
	
	the_channel = &pptp_channel;

}


int pptp_module_connect(int pty_fd)
{

	d_dbg("pptp: >>> pptp_module_connect()\n");
	return -1;
}

void pptp_module_disconnect(void)
{
	d_dbg("pptp: >>> pptp_module_disconnect()\n");
	return;
}
