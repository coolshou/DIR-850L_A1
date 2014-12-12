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
#include "pppd.h"
#include "fsm.h"
#include "lcp.h"
#include "ccp.h"
#include "ipcp.h"

#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/if_pppox.h>
#include <linux/if_ppp.h>

/* should be added to system's socket.h... */
#ifndef SOL_PPPOL2TP
#define SOL_PPPOL2TP	273
#endif

int Sock = -1;
int pppol2tp_fd = -1;
int pppol2tp_udp_fd = -1;
char Hostname[MAX_HOSTNAME];
static bool pppol2tp_lns_mode = 0;
static bool pppol2tp_recv_seq = 0;
static bool pppol2tp_send_seq = 0;
static int pppol2tp_debug_mask = 0;
static int pppol2tp_reorder_timeout = 0;
static char pppol2tp_ifname[32] = { 0, };
int pppol2tp_tunnel_id = 0;
int pppol2tp_session_id = 0;
static int device_got_set = 0;

int setdevname_pppol2tp(void);

static void network_handler(int sock, void * eloop_ctx, void * sock_ctx)
{
	l2tp_dgram * dgram;
	struct sockaddr_in from;

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
	int flags;
	int reuseaddr_on = 1;

	gethostname(Hostname, sizeof(Hostname));
	Hostname[sizeof(Hostname)-1] = 0;

	if (Sock >= 0) close(Sock);
	Sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (Sock < 0)
	{
		d_error("l2tp: network_init(): socket: %s\n", strerror(errno));
		return -1;
	}

	flags = 1;
	setsockopt(Sock, SOL_SOCKET, SO_NO_CHECK, &flags, sizeof(flags));
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

int kernel_mode_connect(l2tp_session * ses)
{
	int ret = 0;
	int s_pty;
	int flags;
	struct sockaddr_pppol2tp sax;

	while (l2tp_kernel_mode == 1)
	{
		s_pty = socket(AF_PPPOX, SOCK_DGRAM, PX_PROTO_OL2TP);
		if (s_pty < 0) {
			d_dbg("pppol2tp: Unable to allocate PPPoL2TP socket.\n");
			break;
		}
		flags = fcntl(s_pty, F_GETFL);
		if (flags == -1 || fcntl(s_pty, F_SETFL, flags | O_NONBLOCK) == -1) {
			d_dbg("pppol2tp: Unable to set PPPoL2TP socket nonblock.\n");
			break;
		}

		sax.sa_family = AF_PPPOX;
		sax.sa_protocol = PX_PROTO_OL2TP;
		sax.pppol2tp.pid = 0;
		sax.pppol2tp.fd = pppol2tp_udp_fd;
		sax.pppol2tp.addr.sin_addr.s_addr = ses->tunnel->peer_addr.sin_addr.s_addr;
		sax.pppol2tp.addr.sin_port = ses->tunnel->peer_addr.sin_port;
		sax.pppol2tp.addr.sin_family = AF_INET;
		sax.pppol2tp.s_tunnel  = ses->tunnel->my_id;
		sax.pppol2tp.s_session = ses->my_id;
		sax.pppol2tp.d_tunnel  = ses->tunnel->assigned_id;
		sax.pppol2tp.d_session = ses->assigned_id;
		if (connect(s_pty, (struct sockaddr *)&sax, sizeof(sax)) < 0) {
			d_dbg("pppol2tp: Unable to connect PPPoL2TP socket.\n");
			break;
		}

		pppol2tp_fd = s_pty;
		setdevname_pppol2tp();
		ret = 1;
		break;
	}

	return ret;
}

extern u_int32_t exclude_peer;

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
	exclude_peer = the_peer.addr.sin_addr.s_addr;
	d_dbg("%s: exclude_peer = %08x\n",__FUNCTION__, exclude_peer);

	if (network_init() < 0) return -1;
	ses = l2tp_session_call_lns(&the_peer, "number", NULL);
	if (!ses) return -1;

#if 1
	if ((l2tp_kernel_mode != 1) && l2tp_sync)
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
			if (kernel_mode_connect(ses))
			{
				ses->pty_fd = -1;
			} else {
				eloop_register_read_sock(pty_fd, handle_pty, NULL, ses);
			}
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
	if (l2tp_kernel_mode == 1)
	{
		if (pppol2tp_fd >= 0)
		{
			close(pppol2tp_fd);
			pppol2tp_fd = -1;
		}
	}

	if (the_tunnel) l2tp_tunnel_close(the_tunnel);
	d_dbg("l2tp_module_disconnect: Sock=%d\n", Sock);
	if (Sock > 0) close(Sock);
	Sock = -1;
	if (pppol2tp_udp_fd > 0) close(pppol2tp_udp_fd);
	pppol2tp_udp_fd = -1;
	hungup = 1;

	exclude_peer = 0;
	d_dbg("%s: clear exlucde_peer!\n",__FUNCTION__);
}

int setdevname_pppol2tp(void)
{
	union {
		char buffer[128];
		struct sockaddr pppol2tp;
	} s;
	int len = sizeof(s);
	int tmp;
	int tmp_len = sizeof(tmp);

	d_dbg("pppol2tp: %s\n", __func__);

	if (device_got_set)
		return 0;

	if (pppol2tp_fd < 0)
		return 0;

	if(getsockname(pppol2tp_fd, (struct sockaddr *)&s, (socklen_t *)&len) < 0) {
		d_dbg("Given FD for PPPoL2TP socket invalid (%s)\n",
		      strerror(errno));
	}
	if(s.pppol2tp.sa_family != AF_PPPOX) {
		d_dbg("Socket of not a PPPoX socket\n");
		return 0;
	}

	/* Do a test getsockopt() to ensure that the kernel has the necessary
	 * feature available.
	 */
	if (getsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_DEBUG,
		       &tmp, (socklen_t *)&tmp_len) < 0) {
		d_dbg("PPPoL2TP kernel driver not installed\n");
		return 0;
	}

	/* Setup option defaults. Compression options are disabled! */

	modem = 0;

	lcp_allowoptions[0].neg_accompression = 1;
	lcp_wantoptions[0].neg_accompression = 0;

	lcp_allowoptions[0].neg_pcompression = 1;
	lcp_wantoptions[0].neg_pcompression = 0;

	ccp_allowoptions[0].deflate = 0;
	ccp_wantoptions[0].deflate = 0;

	ipcp_allowoptions[0].neg_vj = 0;
	ipcp_wantoptions[0].neg_vj = 0;

	ccp_allowoptions[0].bsd_compress = 0;
	ccp_wantoptions[0].bsd_compress = 0;

	device_got_set = 1;

	return 1;
}

void send_config_pppol2tp(int mtu,
			      u_int32_t asyncmap,
			      int pcomp,
			      int accomp)
{
	int on = 1;
	char reorderto[16];
	char tid[8];
	char sid[8];

	d_dbg("pppol2tp: %s\n", __func__);

	if (pppol2tp_ifname[0]) {
		struct ifreq ifr;
		int fd;

		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd >= 0) {
			memset (&ifr, '\0', sizeof (ifr));
			strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
			strlcpy(ifr.ifr_newname, pppol2tp_ifname,
				sizeof(ifr.ifr_name));
			ioctl(fd, SIOCSIFNAME, (caddr_t) &ifr);
			strlcpy(ifname, pppol2tp_ifname, 32);
			if (pppol2tp_debug_mask & PPPOL2TP_MSG_CONTROL) {
				d_dbg("ppp%d: interface name %s\n",
				       ifunit, ifname);
			}
		}
		close(fd);
	}

	if ((lcp_allowoptions[0].mru > 0) && (mtu > lcp_allowoptions[0].mru)) {
		warn("Overriding mtu %d to %d", mtu, lcp_allowoptions[0].mru);
		mtu = lcp_allowoptions[0].mru;
	}
	netif_set_mtu(ifunit, mtu);

	reorderto[0] = '\0';
	if (pppol2tp_reorder_timeout > 0)
		sprintf(&reorderto[0], "%d ", pppol2tp_reorder_timeout);
	tid[0] = '\0';
	if (pppol2tp_tunnel_id > 0)
		sprintf(&tid[0], "%hu ", pppol2tp_tunnel_id);
	sid[0] = '\0';
	if (pppol2tp_session_id > 0)
		sprintf(&sid[0], "%hu ", pppol2tp_session_id);

	d_dbg("PPPoL2TP options: %s%s%s%s%s%s%s%s%sdebugmask %d\n",
	       pppol2tp_recv_seq ? "recvseq " : "",
	       pppol2tp_send_seq ? "sendseq " : "",
	       pppol2tp_lns_mode ? "lnsmode " : "",
	       pppol2tp_reorder_timeout ? "reorderto " : "", reorderto,
	       pppol2tp_tunnel_id ? "tid " : "", tid,
	       pppol2tp_session_id ? "sid " : "", sid,
	       pppol2tp_debug_mask);

	if (pppol2tp_recv_seq)
		if (setsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_RECVSEQ,
			       &on, sizeof(on)) < 0)
			fatal("setsockopt(PPPOL2TP_RECVSEQ): %m");
	if (pppol2tp_send_seq)
		if (setsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_SENDSEQ,
			       &on, sizeof(on)) < 0)
			fatal("setsockopt(PPPOL2TP_SENDSEQ): %m");
	if (pppol2tp_lns_mode)
		if (setsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_LNSMODE,
			       &on, sizeof(on)) < 0)
			fatal("setsockopt(PPPOL2TP_LNSMODE): %m");
	if (pppol2tp_reorder_timeout)
		if (setsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_REORDERTO,
			       &pppol2tp_reorder_timeout,
			       sizeof(pppol2tp_reorder_timeout)) < 0)
			fatal("setsockopt(PPPOL2TP_REORDERTO): %m");
	if (pppol2tp_debug_mask)
		if (setsockopt(pppol2tp_fd, SOL_PPPOL2TP, PPPOL2TP_SO_DEBUG,
			       &pppol2tp_debug_mask, sizeof(pppol2tp_debug_mask)) < 0)
			fatal("setsockopt(PPPOL2TP_DEBUG): %m");
}

void recv_config_pppol2tp(int mru,
			      u_int32_t asyncmap,
			      int pcomp,
			      int accomp)
{
	d_dbg("pppol2tp: %s\n", __func__);

	if ((lcp_allowoptions[0].mru > 0) && (mru > lcp_allowoptions[0].mru)) {
		d_dbg("pppol2tp: Overriding mru %d to mtu value %d", mru,
		     lcp_allowoptions[0].mru);
		mru = lcp_allowoptions[0].mru;
	}
	if ((ifunit >= 0) && ioctl(pppol2tp_fd, PPPIOCSMRU, (caddr_t) &mru) < 0)
		d_dbg("pppol2tp: Couldn't set PPP MRU: %m");
}

void pppol2tp_init_network()
{
	if (l2tp_kernel_mode == 0)
	{
		d_dbg("pppol2tp: Not looking for kernel support.\n");
	} else {
		int kernel_fd = socket(AF_PPPOX, SOCK_DGRAM, PX_PROTO_OL2TP);
		if (kernel_fd < 0)
		{
			d_dbg("pppol2tp: L2TP kernel support not detected.\n");
			l2tp_kernel_mode = 0;
		}
		else
		{
			close(kernel_fd);
			d_dbg("pppol2tp: Using l2tp kernel support.\n");
			l2tp_kernel_mode = 1;
		}
	}
}

int pppol2tp_tunnel(l2tp_tunnel *t)
{
	int ufd = -1, fd2 = -1;
	int flags;
	struct sockaddr_pppol2tp sax;
	struct sockaddr_in server;

	if (l2tp_kernel_mode != 1)
		goto tunnel_err;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons((uint16_t)l2tp_port);
	if ((ufd = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		l2tp_set_errmsg("%s: Unable to allocate UDP socket. Terminating.\n",
				__FUNCTION__);
		goto tunnel_err;
	};

	flags=1;
	setsockopt(ufd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
	setsockopt(ufd, SOL_SOCKET, SO_NO_CHECK, &flags, sizeof(flags));

	if (bind (ufd, (struct sockaddr *) &server, sizeof (server)))
	{
		close (ufd);
		l2tp_set_errmsg("%s: Unable to bind UDP socket: %s. Terminating.\n",
				__FUNCTION__, strerror(errno));
		goto tunnel_err;
	};
	server = t->peer_addr;
	flags = fcntl(ufd, F_GETFL);
	if (flags == -1 || fcntl(ufd, F_SETFL, flags | O_NONBLOCK) == -1) {
		l2tp_set_errmsg("%s: Unable to set UDP socket nonblock.\n",
				__FUNCTION__);
		goto tunnel_err;
	}
	if (connect (ufd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		l2tp_set_errmsg("%s: Unable to connect UDP peer. Terminating.\n",
				__FUNCTION__);
		goto tunnel_err;
	}

	pppol2tp_udp_fd = ufd;

	fd2 = socket(AF_PPPOX, SOCK_DGRAM, PX_PROTO_OL2TP);
	if (fd2 < 0) {
		l2tp_set_errmsg("%s: Unable to allocate PPPoL2TP socket.\n",
				__FUNCTION__);
		goto tunnel_err;
	}
	flags = fcntl(fd2, F_GETFL);
	if (flags == -1 || fcntl(fd2, F_SETFL, flags | O_NONBLOCK) == -1) {
		l2tp_set_errmsg("%s: Unable to set PPPoL2TP socket nonblock.\n",
				__FUNCTION__);
		goto tunnel_err;
	}
	sax.sa_family = AF_PPPOX;
	sax.sa_protocol = PX_PROTO_OL2TP;
	sax.pppol2tp.pid = 0;
	sax.pppol2tp.fd = ufd;
	sax.pppol2tp.addr.sin_addr.s_addr = t->peer_addr.sin_addr.s_addr;
	sax.pppol2tp.addr.sin_port = t->peer_addr.sin_port;
	sax.pppol2tp.addr.sin_family = AF_INET;
	sax.pppol2tp.s_tunnel  = t->my_id;
	sax.pppol2tp.s_session = 0;
	sax.pppol2tp.d_tunnel  = t->assigned_id;
	sax.pppol2tp.d_session = 0;
	if ((connect(fd2, (struct sockaddr *)&sax, sizeof(sax))) < 0) {
		l2tp_set_errmsg("%s: Unable to connect PPPoL2TP socket. %d %s\n",
				__FUNCTION__, errno, strerror(errno));
		close(fd2);
		goto tunnel_err;
	}

	t->pppox_fd = fd2;
	eloop_register_read_sock(pppol2tp_udp_fd, network_handler, NULL, NULL);
	return 1;

tunnel_err:
	return 0;
}


