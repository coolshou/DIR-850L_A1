/* vi: set sw=4 ts=4: */
/* l2tp.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#ifndef N_HDLC
#include <linux/termios.h>
#endif

#include "l2tp.h"
#include "dtrace.h"
#include "md5.h"
#include "eloop.h"

#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int Sock = -1;
char Hostname[MAX_HOSTNAME];

static void network_handler(int sock, void * eloop_ctx, void * sock_ctx)
{
	l2tp_dgram * dgram;
	struct universal_sockaddr from;

	dgram = l2tp_dgram_take_from_wire(&from);
	if (!dgram) return;

	/* It's a control packet if we get here */
	l2tp_tunnel_handle_received_control_datagram(dgram, &from);

	l2tp_dgram_free(dgram);
	return;
}

static int network_init(void)
{
	struct sockaddr_in me;
	struct sockaddr_in6 me6;
	int flags;
	int reuseaddr_on = 1;

	gethostname(Hostname, sizeof(Hostname));
	Hostname[sizeof(Hostname)-1] = 0;

	if (Sock >= 0) close(Sock);

	if(the_peer.addr_type == AF_INET)
		Sock = socket(PF_INET, SOCK_DGRAM, 0);
	else if(the_peer.addr_type == AF_INET6)
		Sock = socket(PF_INET6, SOCK_DGRAM, 0);

	if (Sock < 0)
	{
		d_error("l2tp: network_init(): socket: %s\n", strerror(errno));
		return -1;
	}

#if 1
	/* set SO_REUSEADDR */
	if (setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr_on, sizeof(int)) < 0)
	{
		d_error("l2tp: Unable to to SO_REUSEADDR ! ($s)\n", strerror(errno));
		close(Sock);
		Sock = -1;
		return -1;
	}
#endif

	if(the_peer.addr_type == AF_INET)
	{
		me.sin_family = AF_INET;
		me.sin_addr.s_addr = htonl(INADDR_ANY);
		me.sin_port = htons((uint16_t)l2tp_port);
		if (bind(Sock, (struct sockaddr *)&me, sizeof(me)) < 0)
		{
			d_error("l2tp: network_init(): bind: %s\n", strerror(errno));
			close(Sock);
			Sock = -1;
			return -1;
		}
	}
	else if(the_peer.addr_type == AF_INET6)
	{
		memset(&me6 , 0 , sizeof(me6));
		me6.sin6_family = AF_INET6;
		me6.sin6_port = htons((uint16_t)l2tp_port);
		//memset(me6.sin6_addr.s6_addr , 0 , sizeof(struct in6_addr)); //IN6ADDR_ANY
		if (bind(Sock, (struct sockaddr *)&me6, sizeof(me6)) < 0)
		{
			d_error("l2tp: network_init(): bind: %s\n", strerror(errno));
			close(Sock);
			Sock = -1;
			return -1;
		}
	}

	/* Set socket non-blocking */
	flags = fcntl(Sock, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(Sock, F_SETFL, flags);
	eloop_register_read_sock(Sock, network_handler, NULL, NULL);
	return Sock;
}

void l2tp_generate_response(uint16_t msg_type, char const * secret,
		unsigned char const * challenge, size_t chal_len, unsigned char buf[16])
{
	MD5_CTX ctx;
	unsigned char id = (unsigned char)msg_type;

	MD5Init(&ctx);
	MD5Update(&ctx, &id, 1);
	MD5Update(&ctx, (unsigned char *)secret, strlen(secret));
	MD5Update(&ctx, challenge, chal_len);
	MD5Final(buf, &ctx);

	d_dbg("l2tp: gen response(secret=%s) -> %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		secret,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
}

static void handle_pty(int sock, void * eloop_ctx, void * sock_ctx)
{
	l2tp_session * ses = (l2tp_session *)sock_ctx;
	d_dbg("handle_pty >>>\n");
	ses->handle_frame_to_tunnel(ses);
	d_dbg("handle_pty <<<\n");
}

extern u_int32_t exclude_peer;
extern struct in6_addr exclude_peer6;

int l2tp_module_connect(int pty_fd)
{
	l2tp_session * ses;
	int disc = N_HDLC;
	long flags;

	d_dbg("l2tp: >>> l2tp_module_connect()\n");

	l2tp_random_init();
	if (l2tp_peer_init() < 0) return -1;

	/* record the server ip in 'exclude_peer' (IPCP).
	 * This IP can not be used as peer IP.
	 * by David Hsieh <david_hsieh@alphanetworks.com> */
	if(the_peer.addr_type == AF_INET)
	{
		exclude_peer = the_peer.addr.u.addr4.sin_addr.s_addr;
		d_dbg("%s: exclude_peer = %08x\n",__FUNCTION__, exclude_peer);
	}
	else if(the_peer.addr_type == AF_INET6)
	{
		char ipv6_str[1024];
		exclude_peer6 = the_peer.addr.u.addr6.sin6_addr;
		inet_ntop(AF_INET6 , &exclude_peer6 , ipv6_str , sizeof(ipv6_str));
		d_dbg("%s: exclude_peer6 = %s\n",__FUNCTION__, ipv6_str);
	}

	if (network_init() < 0) return -1;
	ses = l2tp_session_call_lns(&the_peer, "number", NULL);
	if (!ses) return -1;

#if 1
	if (l2tp_sync)
	{
		if (ioctl(pty_fd, TIOCSETD, &disc) < 0)
		{
			d_error("l2tp: unable to set link discipline to N_HDLC.\n"
					"      Make sure your kernel supports the N_HDLC line discipline,\n"
					"      or do not use the SYNCHOUNOUS option. Quitting.\n");
			return -1;
		}
		else
		{
			d_info("l2tp: Changed pty line discipline to N_HDLC for synchrounous mode.\n");
		}
		/* There is a bug in Linux's select which returns a descriptor
		 * as readable if N_HDLC line discipline is on, even if
		 * it isn't really readable. This return happens only when
		 * select() times out. To avoid blocking forever in read(),
		 * make descriptor non-blocking. */
		flags = fcntl(pty_fd, F_GETFL);
		if (flags < 0)
		{
			d_error("l2tp: error fcntl(F_GETFL)\n");
			return -1;
		}
		if (fcntl(pty_fd, F_SETFL, (long)flags | O_NONBLOCK) < 0)
		{
			d_error("l2tp: error fcntl(F_SETFL)\n");
			return -1;
		}
	}
#endif
	
	ses->pty_fd = pty_fd;
	eloop_run();
	if (the_session)
	{
		if (the_session->state == SESSION_ESTABLISHED)
		{
			d_dbg("l2tp: session established!\n");
			eloop_register_read_sock(pty_fd, handle_pty, NULL, ses);
			eloop_continue();
			return 0;
		}
		l2tp_session_free(the_session, "");
	}
	dassert(the_session==NULL);
	dassert(the_tunnel==NULL);
	return -1;
}

extern int hungup;

void l2tp_module_disconnect(void)
{
	d_dbg("l2tp: >>> l2tp_module_disconnect()\n");
	if (the_tunnel) l2tp_tunnel_close(the_tunnel);
	d_dbg("l2tp_module_disconnect: Sock=%d\n", Sock);
	if (Sock > 0) close(Sock);
	Sock = -1;
	hungup = 1;

	exclude_peer = 0;
	d_dbg("%s: clear exlucde_peer!\n",__FUNCTION__);
}
