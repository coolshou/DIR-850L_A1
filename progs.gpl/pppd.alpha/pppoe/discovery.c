/* vi: set sw=4 ts=4: */
/***********************************************************************
*
* discovery.c
*
* Perform PPPoE discovery
*
* Copyright (C) 1999 by Roaring Penguin Software Inc.
*
* LIC: GPL
*
***********************************************************************/

static char const RCSID[] =
"$Id: discovery.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $";

#include "pppoe.h"
#include "dtrace.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
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
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include <signal.h>

/**********************************************************************
*%FUNCTION: parseForHostUniq
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data.
* extra -- user-supplied pointer.  This is assumed to be a pointer to int.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* If a HostUnique tag is found which matches our PID, sets *extra to 1.
***********************************************************************/
static int parseForHostUniq(UINT16_t type, UINT16_t len, unsigned char *data, void *extra)
{
	int *val = (int *) extra;
	if (type == TAG_HOST_UNIQ && len == sizeof(pid_t))
	{
		pid_t tmp;
		memcpy(&tmp, data, len);
		if (tmp == getpid())
		{
			*val = 1;
		}
	}
	return 0;
}

/**********************************************************************
*%FUNCTION: packetIsForMe
*%ARGUMENTS:
* conn -- PPPoE connection info
* packet -- a received PPPoE packet
*%RETURNS:
* 1 if packet is for this PPPoE daemon; 0 otherwise.
*%DESCRIPTION:
* If we are using the Host-Unique tag, verifies that packet contains
* our unique identifier.
***********************************************************************/
static int packetIsForMe(PPPoEConnection *conn, PPPoEPacket *packet)
{
	int forMe = 0;

	/* If packet is not directed to our MAC address, forget it */
	if (memcmp(packet->ethHdr.h_dest, conn->myEth, ETH_ALEN)) return 0;

	/* If we're not using the Host-Unique tag, then accept the packet */
	if (!conn->useHostUniq) return 1;

	parsePacket(packet, parseForHostUniq, &forMe);
	return forMe;
}

/**********************************************************************
*%FUNCTION: parsePADOTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data.  Should point to a PacketCriteria structure
*          which gets filled in according to selected AC name and service
*          name.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks interesting tags out of a PADO packet
***********************************************************************/
static int parsePADOTags(UINT16_t type, UINT16_t len, unsigned char *data, void *extra)
{
	struct PacketCriteria *pc = (struct PacketCriteria *) extra;
	PPPoEConnection *conn = pc->conn;

	d_dbg("pppoe: >>> parsePADOTags()\n");
	
	switch(type)
	{
	case TAG_AC_NAME:
		d_dbg("pppoe: PADO: TAG_AC_NAME!\n");
		pc->seenACName = 1;
		if (conn->acName && len == strlen(conn->acName) &&
			!strncmp((char *) data, conn->acName, len))
		{
			pc->acNameOK = 1;
		}
		break;
	case TAG_SERVICE_NAME:
		d_dbg("pppoe: PADO: TAG_SERVICE_NAME!\n");
		pc->seenServiceName = 1;
		if (conn->serviceName && len == strlen(conn->serviceName) &&
			!strncmp((char *) data, conn->serviceName, len))
		{
			pc->serviceNameOK = 1;
		}
		break;
	case TAG_AC_COOKIE:
		d_dbg("pppoe: PADO: TAG_AC_COOKIE!\n");
		conn->cookie.type = htons(type);
		conn->cookie.length = htons(len);
		memcpy(conn->cookie.payload, data, len);
		break;
	case TAG_RELAY_SESSION_ID:
		d_dbg("pppoe: PADO: TAG_RELAY_SESSION_ID!\n");
		conn->relayId.type = htons(type);
		conn->relayId.length = htons(len);
		memcpy(conn->relayId.payload, data, len);
		break;

	case TAG_SERVICE_NAME_ERROR:
		d_error("pppoe: PADO: Service-Name-Error: %.*s\n", (int) len, data);
		return -1;

	case TAG_AC_SYSTEM_ERROR:
		d_error("pppoe: PADO: System-Error: %.*s\n", (int) len, data);
		return -1;

	case TAG_GENERIC_ERROR:
		d_error("pppoe: PADO: Generic-Error: %.*s", (int) len, data);
		return -1;
	}
	return 0;
}

/**********************************************************************
*%FUNCTION: parsePADSTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data (pointer to PPPoEConnection structure)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks interesting tags out of a PADS packet
***********************************************************************/
static int parsePADSTags(UINT16_t type, UINT16_t len, unsigned char *data, void *extra)
{
	PPPoEConnection *conn = (PPPoEConnection *) extra;
	switch (type)
	{
	case TAG_SERVICE_NAME:
		d_info("pppoe: PADS: Service-Name: '%.*s'\n", (int) len, data);
		break;
	case TAG_SERVICE_NAME_ERROR:
		d_error("pppoe: PADS: Service-Name-Error: %.*s\n", (int) len, data);
		return -1;

	case TAG_AC_SYSTEM_ERROR:
		d_error("pppoe: PADS: System-Error: %.*s\n", (int) len, data);
		return -1;

	case TAG_GENERIC_ERROR:
		d_error("pppoe: PADS: Generic-Error: %.*s\n", (int) len, data);
		return -1;

	case TAG_RELAY_SESSION_ID:
		d_info("pppoe: PADS: TAG_RELAY_SESSION_ID!\n");
		conn->relayId.type = htons(type);
		conn->relayId.length = htons(len);
		memcpy(conn->relayId.payload, data, len);
		break;
	}
	return 0;
}

/***********************************************************************
*%FUNCTION: sendPADI
*%ARGUMENTS:
* conn -- PPPoEConnection structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADI packet
***********************************************************************/
static int sendPADI(PPPoEConnection *conn)
{
	PPPoEPacket packet;
	unsigned char *cursor = packet.payload;
	PPPoETag *svc = (PPPoETag *) (&packet.payload);
	UINT16_t namelen = 0;
	UINT16_t plen;

	d_dbg("pppoe: >>> sendPADI()\n");
	
	if (conn->serviceName)
	{
		namelen = (UINT16_t)strlen(conn->serviceName);
	}
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet.payload, plen);

	/* Set destination to Ethernet broadcast address */
	memset(packet.ethHdr.h_dest, 0xFF, ETH_ALEN);
	memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

	packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
	packet.ver = 1;
	packet.type = 1;
	packet.code = CODE_PADI;
	packet.session = 0;

	svc->type = TAG_SERVICE_NAME;
	svc->length = htons(namelen);
	CHECK_ROOM(cursor, packet.payload, namelen+TAG_HDR_SIZE);

	if (conn->serviceName)
	{
		memcpy(svc->payload, conn->serviceName, strlen(conn->serviceName));
	}
	cursor += namelen + TAG_HDR_SIZE;

	/* If we're using Host-Uniq, copy it over */
	if (conn->useHostUniq)
	{
		PPPoETag hostUniq;
		pid_t pid = getpid();
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(pid));
		memcpy(hostUniq.payload, &pid, sizeof(pid));
		CHECK_ROOM(cursor, packet.payload, sizeof(pid) + TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
		cursor += sizeof(pid) + TAG_HDR_SIZE;
		plen += sizeof(pid) + TAG_HDR_SIZE;
	}

	packet.length = htons(plen);

	if (sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE)) < 0)
	{
		d_error("pppoe: sendPADI: SendPacket fail!\n");
		return -1;
	}
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &packet, "SENT");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}
	return 0;
}

/**********************************************************************
*%FUNCTION: waitForPADO
*%ARGUMENTS:
* conn -- PPPoEConnection structure
* timeout -- how long to wait (in seconds)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Waits for a PADO packet and copies useful information
***********************************************************************/
static int waitForPADO(PPPoEConnection *conn, int timeout)
{
	fd_set readable;
	int r;
	struct timeval tv;
	PPPoEPacket packet;
	int len;
	struct PacketCriteria pc;

	pc.conn          = conn;
	pc.acNameOK      = (conn->acName)      ? 0 : 1;
	pc.serviceNameOK = (conn->serviceName) ? 0 : 1;
	pc.seenACName    = 0;
	pc.seenServiceName = 0;

	d_dbg("pppoe: >>> waitForPADO()\n");
	d_dbg("pppoe: init acNameOK=%d, serviceNameOK=%d\n", pc.acNameOK, pc.serviceNameOK);
	
	do
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		FD_ZERO(&readable);
		FD_SET(conn->discoverySocket, &readable);

		while(1)
		{
			r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
			if (r >= 0 || errno != EINTR) break;
		}
		if (r < 0)
		{
			d_error("pppoe: waitForPADO(): select error!\n");
			return -1;
		}
		if (r == 0) return 0; /* Timed out */

		/* Get the packet */
		if (receivePacket(conn->discoverySocket, &packet, &len) < 0)
		{
			d_error("pppoe: waitForPADO(): receivePacket error!\n");
			return -1;
		}

		/* Check length */
		if (ntohs(packet.length) + HDR_SIZE > len)
		{
			d_info("pppoe: Bogus PPPoE length field (%u)\n", (unsigned int) ntohs(packet.length));
			continue;
		}

		if (conn->debugFile)
		{
			dumpPacket(conn->debugFile, &packet, "RCVD");
			fprintf(conn->debugFile, "\n");
			fflush(conn->debugFile);
		}

		/* If it's not for us, loop again */
		if (!packetIsForMe(conn, &packet))
		{
			d_info("pppoe: waitForPADO(): packet is not for me!\n");
			continue;
		}

		if (packet.code == CODE_PADO)
		{
			d_info("pppoe: waitForPADO(): got PADO!\n");
			if (NOT_UNICAST(packet.ethHdr.h_source))
			{
				d_info("pppoe: Ignoring PADO packet from non-unicast MAC address\n");
				continue;
			}
			if (parsePacket(&packet, parsePADOTags, &pc) < 0)
			{
				d_error("pppoe(): waitForPADO(): parsePacket error!\n");
				return -1;
			}
			if (!pc.seenACName)
			{
				d_info("pppoe: Ignoring PADO packet with no AC-Name tag\n");
				continue;
			}
			if (!pc.seenServiceName)
			{
				d_info("pppoe: Ignoring PADO packet with no Service-Name tag\n");
				continue;
			}
			conn->numPADOs++;
			d_dbg("pppoe: acNameOK=%d, serviceNameOK=%d\n", pc.acNameOK, pc.serviceNameOK);
			if (pc.acNameOK && pc.serviceNameOK)
			{
				memcpy(conn->peerEth, packet.ethHdr.h_source, ETH_ALEN);
				conn->discoveryState = STATE_RECEIVED_PADO;
				break;
			}
		}
		else
		{
			d_dbg("pppoe: waitForPADO(): not PADO packet!\n");
		}
	} while (conn->discoveryState != STATE_RECEIVED_PADO);

	return 0;
}

/***********************************************************************
*%FUNCTION: sendPADR
*%ARGUMENTS:
* conn -- PPPoE connection structur
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADR packet
***********************************************************************/
static int sendPADR(PPPoEConnection *conn)
{
	PPPoEPacket packet;
	PPPoETag *svc = (PPPoETag *) packet.payload;
	unsigned char *cursor = packet.payload;
	UINT16_t namelen = 0;
	UINT16_t plen;

	d_dbg("pppoe: >>> sendPADR()\n");
	
	if (conn->serviceName)
	{
		namelen = (UINT16_t) strlen(conn->serviceName);
	}
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet.payload, plen);

	memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
	memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

	packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
	packet.ver = 1;
	packet.type = 1;
	packet.code = CODE_PADR;
	packet.session = 0;

	svc->type = TAG_SERVICE_NAME;
	svc->length = htons(namelen);
	if (conn->serviceName)
	{
		memcpy(svc->payload, conn->serviceName, namelen);
	}
	cursor += namelen + TAG_HDR_SIZE;

	/* If we're using Host-Uniq, copy it over */
	if (conn->useHostUniq)
	{
		PPPoETag hostUniq;
		pid_t pid = getpid();
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(pid));
		memcpy(hostUniq.payload, &pid, sizeof(pid));
		CHECK_ROOM(cursor, packet.payload, sizeof(pid)+TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
		cursor += sizeof(pid) + TAG_HDR_SIZE;
		plen += sizeof(pid) + TAG_HDR_SIZE;
	}

	/* Copy cookie and relay-ID if needed */
	if (conn->cookie.type)
	{
		CHECK_ROOM(cursor, packet.payload, ntohs(conn->cookie.length) + TAG_HDR_SIZE);
		memcpy(cursor, &conn->cookie, ntohs(conn->cookie.length) + TAG_HDR_SIZE);
		cursor += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
		plen += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
	}

	if (conn->relayId.type)
	{
		CHECK_ROOM(cursor, packet.payload, ntohs(conn->relayId.length) + TAG_HDR_SIZE);
		memcpy(cursor, &conn->relayId, ntohs(conn->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
	}

	packet.length = htons(plen);
	if (sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE)) < 0)
	{
		d_error("pppoe: sendPADR(): sendPacket fail!\n");
		return -1;
	}
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &packet, "SENT");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}
	return 0;
}

/**********************************************************************
*%FUNCTION: waitForPADS
*%ARGUMENTS:
* conn -- PPPoE connection info
* timeout -- how long to wait (in seconds)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Waits for a PADS packet and copies useful information
***********************************************************************/
static int waitForPADS(PPPoEConnection *conn, int timeout)
{
	fd_set readable;
	int r;
	struct timeval tv;
	PPPoEPacket packet;
	int len;

	d_dbg("pppoe: >>> waitForPADS()\n");
	
	do
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		FD_ZERO(&readable);
		FD_SET(conn->discoverySocket, &readable);

		while(1)
		{
			r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
			if (r >= 0 || errno != EINTR) break;
		}
		if (r < 0)
		{
			d_error("pppoe: waitForPADS(): select error!\n");
			return -1;
		}
		if (r == 0) return 0; /* Timeout */

		/* Get the packet */
		if (receivePacket(conn->discoverySocket, &packet, &len) < 0)
		{
			d_error("pppoe: waitForPADS(): receivePacket fail!\n");
			return -1;
		}

		/* Check length */
		if (ntohs(packet.length) + HDR_SIZE > len)
		{
			d_info("pppoe: Bogus PPPoE length field (%u)\n", (unsigned int) ntohs(packet.length));
			continue;
		}

		if (conn->debugFile)
		{
			dumpPacket(conn->debugFile, &packet, "RCVD");
			fprintf(conn->debugFile, "\n");
			fflush(conn->debugFile);
		}

		/* If it's not from the AC, it's not for me */
		if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) continue;

		/* If it's not for us, loop again */
		if (!packetIsForMe(conn, &packet)) continue;

		/* Is it PADS?  */
		if (packet.code == CODE_PADS)
		{	/* Parse for goodies */
			if (parsePacket(&packet, parsePADSTags, conn) < 0) return -1;
			conn->discoveryState = STATE_SESSION;
			break;
		}
	} while (conn->discoveryState != STATE_SESSION);

	/* Don't bother with ntohs; we'll just end up converting it back... */
	conn->session = packet.session;

	d_info("pppoe: PPPoE session is %d\n", (int) ntohs(conn->session));

	/* RFC 2516 says session id MUST NOT be zero or 0xFFFF */
	if (ntohs(conn->session) == 0 || ntohs(conn->session) == 0xFFFF)
	{
		d_error("Access concentrator used a session value of %x -- the AC is violating RFC 2516 !!\n", (unsigned int) ntohs(conn->session));
	}

	return 0;
}

/**********************************************************************
*%FUNCTION: discovery
*%ARGUMENTS:
* conn -- PPPoE connection info structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Performs the PPPoE discovery phase
***********************************************************************/
int discovery(PPPoEConnection *conn)
{
	int padiAttempts = 0;
	int padrAttempts = 0;
	int timeout = PADI_TIMEOUT;

	d_dbg("pppoe: >>> discovery()\n");
	conn->discoverySocket = openInterface(conn->ifName, Eth_PPPOE_Discovery, conn->myEth);

	do
	{
		padiAttempts++;
		if (padiAttempts > MAX_PADI_ATTEMPTS)
		{
			d_info("pppoe: Timeout waiting for PADO packets!\n");
			return -1;
		}
		if (sendPADI(conn) < 0) return -1;
		conn->discoveryState = STATE_SENT_PADI;

		if (waitForPADO(conn, timeout) < 0) return -1;
		timeout *= 2;
	} while (conn->discoveryState == STATE_SENT_PADI);

	/* If we're only printing access concentrator names, we're done */

	timeout = PADI_TIMEOUT;
	do
	{
		padrAttempts++;
		if (padrAttempts > MAX_PADI_ATTEMPTS)
		{
			sendPADT(conn, "Timeout waiting for PADS packets");
			return -1;
		}
		if (sendPADR(conn) < 0) return -1;
		conn->discoveryState = STATE_SENT_PADR;

		if (waitForPADS(conn, timeout) < 0) return -1;
		timeout *= 2;
	} while (conn->discoveryState == STATE_SENT_PADR);

	/* We're done. */
	conn->discoveryState = STATE_SESSION;
	return 0;
}
