/* vi: set sw=4 ts=4: */

/***********************************************************************
*
* pppoe.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Copyright (C) 2000-2001 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
* LIC: GPL
*
***********************************************************************/

static char const RCSID[] =
"$Id: pppoe.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $";

#include "pppoe.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_LINUX_PACKET

/* TIOCSETD is defined in asm/ioctls.h ,
 * but in broadcom's toolchain, the 'tIOC' is used to define TIOCSETD,
 * which is defined only when __USE_MISC or __KERNEL__.
 * But __USE_MISC is undefined, so define tIOC here, only for sentry5.
 * 				David Hsieh (david_hsieh@alphanetworks.com)
 */
#ifdef _sentry5_
#define tIOC ('t' << 8)
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include <signal.h>

#ifdef HAVE_N_HDLC
#ifndef N_HDLC
#include <linux/termios.h>
#endif
#endif

#include "ppp_fcs.h"
#include "eloop.h"
#include "dtrace.h"

extern void modem_hungup(void);
extern int hungup;

/***********************************************************************
*%FUNCTION: sendSessionPacket
*%ARGUMENTS:
* conn -- PPPoE connection
* packet -- the packet to send
* len -- length of data to send
*%RETURNS:
* 0 if successful, -1 if error occurs.
*%DESCRIPTION:
* Transmits a session packet to the peer.
***********************************************************************/
int sendSessionPacket(PPPoEConnection *conn, PPPoEPacket *packet, int len)
{
	packet->length = htons(len);
	if (pppoe_mss)
	{
		clampMSS(packet, "outgoing", pppoe_mss);
	}
	if (sendPacket(conn, conn->sessionSocket, packet, len + HDR_SIZE) < 0)
	{
		if (errno == ENOBUFS)
		{	/* No buffer space is a transient error */
			return 0;
		}
		return -1;
	}
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, packet, "SENT");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}
	return 0;
}

static PPPoEPacket pkt_eth;
static unsigned char pppBuf[4096];

/**********************************************************************
*%FUNCTION: asyncReadFromEth
*%ARGUMENTS:
* conn -- PPPoE connection info
* sock -- Ethernet socket
* clampMss -- if non-zero, do MSS-clamping
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Reads a packet from the Ethernet interface and sends it to async PPP
* device.
***********************************************************************/
void asyncReadFromEth(PPPoEConnection *conn, int sock, int clampMss)
{
//	PPPoEPacket packet;
	int len;
	int plen;
	int i;
//	unsigned char pppBuf[4096];
	unsigned char *ptr = pppBuf;
	unsigned char c;
	UINT16_t fcs;
	unsigned char header[2] = {FRAME_ADDR, FRAME_CTRL};
	unsigned char tail[2];

	//d_dbg("pppoe: >>> asyncReadFromEth()\n");
	
	if (receivePacket(sock, &pkt_eth, &len) < 0)
	{
		return;
	}

	/* Check length */
	if (ntohs(pkt_eth.length) + HDR_SIZE > len)
	{
		d_warn("pppoe: asyncReadFromEth(): Bogus PPPoE length field (%u)\n", (unsigned int) ntohs(pkt_eth.length));
		return;
	}
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &pkt_eth, "RCVD");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}

	/* Sanity check */
	if (pkt_eth.code != CODE_SESS)
	{
		d_warn("pppoe: asyncReadFromEth(): Unexpected packet code %d\n", (int) pkt_eth.code);
		return;
	}
	if (pkt_eth.ver != 1)
	{
		d_warn("pppoe: asyncReadFromEth(): Unexpected packet version %d\n", (int) pkt_eth.ver);
		return;
	}
	if (pkt_eth.type != 1)
	{
		d_warn("pppoe: asyncReadFromEth(): Unexpected packet type %d\n", (int) pkt_eth.type);
		return;
	}
	if (memcmp(pkt_eth.ethHdr.h_dest, conn->myEth, ETH_ALEN))
	{
		return;
	}
	if (memcmp(pkt_eth.ethHdr.h_source, conn->peerEth, ETH_ALEN))
	{	/* Not for us -- must be another session. This is not an error, so don't log anything. */
		return;
	}

	if (pkt_eth.session != conn->session)
	{	/* Not for us -- must be another session. This is not an error, so don't log anything. */
		return;
	}
	plen = ntohs(pkt_eth.length);
	if (plen + HDR_SIZE > len)
	{
		d_warn("pppoe: asyncReadFromEth(): Bogus length field in session packet %d (%d)\n", (int) plen, (int) len);
		return;
	}

	/* Clamp MSS */
	if (clampMss) clampMSS(&pkt_eth, "incoming", clampMss);

	/* Compute FCS */
	//fcs = pppFCS16(PPPINITFCS16, header, 2);
	//fcs = pppFCS16(fcs, pkt_eth.payload, plen) ^ 0xffff;
	fcs = pppfcs16(PPPINITFCS16, header, 2);
	fcs = pppfcs16(fcs, pkt_eth.payload, plen) ^ 0xffff;
	tail[0] = fcs & 0x00ff;
	tail[1] = (fcs >> 8) & 0x00ff;

	/* Build a buffer to send to PPP */
	*ptr++ = FRAME_FLAG;
	*ptr++ = FRAME_ADDR;
	*ptr++ = FRAME_ESC;
	*ptr++ = FRAME_CTRL ^ FRAME_ENC;

	for (i=0; i<plen; i++)
	{
		c = pkt_eth.payload[i];
		if (c == FRAME_FLAG || c == FRAME_ADDR || c == FRAME_ESC || c < 0x20)
		{
			*ptr++ = FRAME_ESC;
			*ptr++ = c ^ FRAME_ENC;
		}
		else
		{
			*ptr++ = c;
		}
	}
	for (i=0; i<2; i++)
	{
		c = tail[i];
		if (c == FRAME_FLAG || c == FRAME_ADDR || c == FRAME_ESC || c < 0x20)
		{
			*ptr++ = FRAME_ESC;
			*ptr++ = c ^ FRAME_ENC;
		}
		else
		{
			*ptr++ = c;
		}
	}
	*ptr++ = FRAME_FLAG;

	/* Ship it out */
	if (write(master_pty_fd, pppBuf, (ptr-pppBuf)) < 0)
	{
		d_error("pppoe: asyncReadFromEth(): write fail !\n");
	}
}

/**********************************************************************
*%FUNCTION: syncReadFromEth
*%ARGUMENTS:
* conn -- PPPoE connection info
* sock -- Ethernet socket
* clampMss -- if true, clamp MSS.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Reads a packet from the Ethernet interface and sends it to sync PPP
* device.
***********************************************************************/
void syncReadFromEth(PPPoEConnection *conn, int sock, int clampMss)
{
//	PPPoEPacket packet;
	int len;
	int plen;
	struct iovec vec[2];
	unsigned char dummy[2];

	//d_dbg("pppoe: >>> syncReadFromEth()\n");
	
	if (receivePacket(sock, &pkt_eth, &len) < 0) return;

	/* Check length */
	if (ntohs(pkt_eth.length) + HDR_SIZE > len)
	{
		d_warn("pppoe: syncReadFromEth(): Bogus PPPoE length field (%u)\n", (unsigned int) ntohs(pkt_eth.length));
		return;
	}
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &pkt_eth, "RCVD");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}

	/* Sanity check */
	if (pkt_eth.code != CODE_SESS)
	{
		d_warn("pppoe: syncReadFromEth(): Unexpected packet code %d\n", (int) pkt_eth.code);
		return;
	}
	if (pkt_eth.ver != 1)
	{
		d_warn("pppoe: syncReadFromEth(): Unexpected packet version %d\n", (int) pkt_eth.ver);
		return;
	}
	if (pkt_eth.type != 1)
	{
		d_warn("pppoe: syncReadFromEth(): Unexpected packet type %d\n", (int) pkt_eth.type);
		return;
	}
	if (memcmp(pkt_eth.ethHdr.h_dest, conn->myEth, ETH_ALEN))
	{	/* Not for us -- must be another session. This is not an error, so don't log anything. */
		return;
	}
	if (memcmp(pkt_eth.ethHdr.h_source, conn->peerEth, ETH_ALEN))
	{	/* Not for us -- must be another session. This is not an error, so don't log anything. */
		return;
	}
	if (pkt_eth.session != conn->session)
	{	/* Not for us -- must be another session. This is not an error, so don't log anything. */
		return;
	}
	plen = ntohs(pkt_eth.length);
	if (plen + HDR_SIZE > len)
	{
		d_warn("pppoe: syncReadFromEth(): Bogus length field in session packet %d (%d)\n", (int) plen, (int) len);
		return;
	}

	/* Clamp MSS */
	if (clampMss) clampMSS(&pkt_eth, "incoming", clampMss);

	/* Ship it out */
	vec[0].iov_base = (void *) dummy;
	dummy[0] = FRAME_ADDR;
	dummy[1] = FRAME_CTRL;
	vec[0].iov_len = 2;
	vec[1].iov_base = (void *) pkt_eth.payload;
	vec[1].iov_len = plen;

	if (writev(master_pty_fd, vec, 2) < 0)
	{
		d_error("pppoe: syncReadFromEth(): write fail!\n");
	}
}

static void sync_read_eth(int sock, void * eloop_ctx, void * sock_ctx)
{
	PPPoEConnection * conn = (PPPoEConnection *)sock_ctx;
	syncReadFromEth(conn, sock, pppoe_mss);
}

static void async_read_eth(int sock, void * eloop_ctx, void * sock_ctx)
{
	PPPoEConnection * conn = (PPPoEConnection *)sock_ctx;
	asyncReadFromEth(conn, sock, pppoe_mss);
}

static void discovery_read(int sock, void * eloop_ctx, void * sock_ctx)
{
	static PPPoEPacket packet;
	PPPoEConnection * conn = (PPPoEConnection *)sock_ctx;
	int len;

	if (receivePacket(sock, &packet, &len) < 0) return;

	/* Check length */
	if (ntohs(packet.length) + HDR_SIZE > len)
	{
		d_warn("pppoe: discovery_read(): Bogus PPPoE length field (%u)\n", (unsigned int)ntohs(packet.length));
		return;
	}

	/* Not PADT, ignore it */
	if (packet.code != CODE_PADT) return;
	/* It's a PADT, all right. Is it for us ? */
	if (memcmp(packet.ethHdr.h_dest, conn->myEth, ETH_ALEN)) return;
	if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) return;
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &packet, "RCVD");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}

	d_info("pppoe: Session %d terminated -- received PADT from peer!\n", (int)ntohs(packet.session));
	parsePacket(&packet, parseLogErrs, NULL);
	conn->session = 0;
	sendPADT(conn, "Received PADT from peer");
	modem_hungup();
	eloop_terminate();
	return;
}

static PPPoEPacket pkt_ppp;

static void sync_read_ppp(int sock, void * eloop_ctx, void * sock_ctx)
{
	PPPoEConnection * conn = (PPPoEConnection *)sock_ctx;
	syncReadFromPPP(conn, &pkt_ppp);
}

static void async_read_ppp(int sock, void * eloop_ctx, void * sock_ctx)
{
	PPPoEConnection * conn = (PPPoEConnection *)sock_ctx;
	asyncReadFromPPP(conn, &pkt_ppp);
}

static void session(PPPoEConnection * conn)
{
	/* Open a session socket */
	conn->sessionSocket = openInterface(conn->ifName, Eth_PPPOE_Session, conn->myEth);

	/* Fill in the constant fields of the packet to save time. */
	memcpy(pkt_ppp.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
	memcpy(pkt_ppp.ethHdr.h_source, conn->myEth, ETH_ALEN);
	pkt_ppp.ethHdr.h_proto = htons(Eth_PPPOE_Session);
	pkt_ppp.ver = 1;
	pkt_ppp.type = 1;
	pkt_ppp.code = CODE_SESS;
	pkt_ppp.session = conn->session;

	initPPP();

	d_dbg("pppoe: register discovery socket (%d)\n", conn->discoverySocket);
	eloop_register_read_sock(conn->discoverySocket, discovery_read, NULL, conn);

	if (conn->synchronous)
	{
		d_dbg("pppoe: register session socket (%d)\n", conn->sessionSocket);
		d_dbg("pppoe: register master pty fd  (%d)\n", master_pty_fd);
		eloop_register_read_sock(conn->sessionSocket, sync_read_eth, NULL, conn);
		eloop_register_read_sock(master_pty_fd, sync_read_ppp, NULL, conn);
	}
	else
	{
		d_dbg("pppoe: register session socket (%d)\n", conn->sessionSocket);
		d_dbg("pppoe: register master pty fd  (%d)\n", master_pty_fd);
		eloop_register_read_sock(conn->sessionSocket, async_read_eth, NULL, conn);
		eloop_register_read_sock(master_pty_fd, async_read_ppp, NULL, conn);
	}
}


static PPPoEConnection conn;
int master_pty_fd = -1;

int pppoe_module_connect(int pty_fd)
{
	int disc = N_HDLC;
	long flags;
	
	memset(&conn, 0, sizeof(conn));
	conn.discoverySocket = -1;
	conn.sessionSocket = -1;

	d_dbg("pppoe: >>> pppoe_module_connect()\n");
	d_dbg("pppoe: pppoe_mss = %d\n", pppoe_mss);
	
	conn.ifName = pppoe_dev;
	conn.serviceName = pppoe_srv_name[0] ? pppoe_srv_name : NULL;
	conn.acName = pppoe_ac_name[0] ? pppoe_ac_name : NULL;;
	conn.useHostUniq = pppoe_hostuniq;
	conn.synchronous = pppoe_synchronous;
	//conn.debugFile = stderr;

	/* set master pty fd for send/recv ppp packet. */
	master_pty_fd = pty_fd;

	if (pty_fd == 0)
	{
		int rlt=discovery(&conn);
		/* Do discovery only. */
		if(rlt==0)
			sendPADT(&conn, "Discovery finished");
		return rlt;
	}

//#ifdef HAVE_N_HDLC
#if 1
	if (conn.synchronous)
	{
		if (ioctl(master_pty_fd, TIOCSETD, &disc) < 0)
		{
			d_error("pppoe: unable to set line discipline to N_HDLC.\n");
			d_error("       Make sure your kernel supports the N_HDLC line discipline,\n");
			d_error("       or do not use the SYNCHOUNOUS option. Quitting.\n");
			return -1;
		}
		else
		{
			d_info("pppoe: Changed pty line discipline to N_HDLC for synchronous mode\n");
		}
		/* There is a bug in Linux's select which returns a descriptor
		 * as readable if N_HDLC line discipline is on, even  if
		 * it isn't really readable. This return happens only when
		 * select() times out. To avoid blocking forever in read(),
		 * make descriptor non-blocking*/
		flags = fcntl(master_pty_fd, F_GETFL);
		if (flags < 0)
		{
			d_error("pppoe: error: fcntl(F_GETFL)\n");
			return -1;
		}
		if (fcntl(master_pty_fd, F_SETFL, (long)flags | O_NONBLOCK) < 0)
		{
			d_error("pppoe: error: fcntl(F_SETFL)\n");
			return -1;
		}
	}
#endif

	if (discovery(&conn)<0) return -1;
	session(&conn);
	return 0;
}

void pppoe_module_disconnect(void)
{
	d_dbg("pppoe: >>> pppoe_module_disconnect()\n");

	sendPADT(&conn, "User request");
	hungup = 1;
	eloop_terminate();

	if (conn.discoverySocket >= 0) close(conn.discoverySocket);
	if (conn.sessionSocket >= 0) close(conn.sessionSocket);
	memset(&conn, 0, sizeof(conn));
	conn.discoverySocket = -1;
	conn.sessionSocket = -1;
}
