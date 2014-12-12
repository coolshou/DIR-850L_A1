/* vi: set sw=4 ts=4: */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <syslog.h>

#include <elbox_config.h>
#include <dtrace.h>
#include <asyslog.h>

#include "pppd.h"
#include "pppoe.h"

extern char linkname[];
extern int got_sigterm;

extern void kpppoe_sendPADT(PPPoEConnection * conn, const char * msg);
extern void kpppoe_parsePacket(PPPoEPacket * packet, ParseFunc * func, void * extra);

static void parseForHostUniq(UINT16_t type, UINT16_t len, unsigned char * data, void * extra)
{
	int * val = (int *)extra;
	if (type == TAG_HOST_UNIQ && len == sizeof(pid_t))
	{
		pid_t tmp;
		memcpy(&tmp, data, len);
		if (tmp == getpid()) *val = 1;
	}
}

static int packetIsForMe(PPPoEConnection * conn, PPPoEPacket * packet)
{
	int forMe = 0;

	/* If packet is not directed to our MAC address, forget it */
	if (memcmp(packet->ethHdr.h_dest, conn->myEth, ETH_ALEN)) return 0;

	/* If we're not using the Host-Uniq tag, then accept the packet */
	if (!conn->useHostUniq) return 1;

	parsePacket(packet, parseForHostUniq, &forMe);
	return forMe;
}

static void parsePADOTags(UINT16_t type, UINT16_t len, unsigned char * data, void * extra)
{
	struct PacketCriteria *pc = (struct PacketCriteria *)extra;
	PPPoEConnection *conn = pc->conn;
	int i;

	d_dbg("[%d]: parsePADOTags >>>\n", getpid());

	switch (type)
	{
	case TAG_AC_NAME:
		d_dbg("[%d]: TAG_AC_NAME\n", getpid());
		d_dbg("[%d]: Access-Concentrator: [%.*s]\n", getpid(), (int)len, data);
		d_dbg("[%d]: acName:[%s]\n", getpid(), conn->acName);
		pc->seenACName = 1;
		if (conn->printACNames) printf("Access-Concentrator: %.*s\n", (int)len, data);
		if (conn->acName && len == strlen(conn->acName) &&
			!strncmp((char *)data, conn->acName, len))
			pc->acNameOK = 1;
		break;

	case TAG_SERVICE_NAME:
		d_dbg("[%d]: TAG_SERVICE_NAME\n", getpid());
		pc->seenServiceName = 1;
		if (conn->printACNames && len > 0) printf("       Service-Name: %.*s\n", (int)len, data);
		if (conn->serviceName && len == strlen(conn->serviceName) &&
			!strncmp((char *)data, conn->serviceName, len))
			pc->serviceNameOK = 1;
		break;

	case TAG_AC_COOKIE:
		d_dbg("[%d]: TAG_AC_COOKIE\n", getpid());
		if (conn->printACNames)
		{
			printf("Got a cookie:");
			/* print first 20 bytes of cookie */
			for (i=0; i<len && i<20; i++) printf(" %02x", (unsigned)data[i]);
			if (i < len) printf("...");
			printf("\n");
		}
		conn->cookie.type = htons(type);
		conn->cookie.length = htons(len);
		memcpy(conn->cookie.payload, data, len);
		break;

	case TAG_RELAY_SESSION_ID:
		d_dbg("[%d]: TAG_RELAY_SESSION_ID\n", getpid());
		if (conn->printACNames)
		{
			printf("Got a Relay-ID:");
			/* print first 20 bytes of relay ID */
			for (i=0; i<len && i<20; i++) printf(" %02x", (unsigned) data[i]);
			if (i < len) printf("...");
			printf("\n");
		}
		conn->relayId.type = htons(type);
		conn->relayId.length = htons(len);
		memcpy(conn->relayId.payload, data, len);
		break;

	case TAG_SERVICE_NAME_ERROR:
		d_dbg("[%d]: TAG_SERVICE_NAME_ERROR\n", getpid());
		if (conn->printACNames)
		{
			printf("Got a Service-Name-Error tag: %.*s\n", (int)len, data);
		}
		else
		{
			d_error("[%d]: PADO: Service-Name-Error: %.*s\n", getpid(), (int)len, data);
			exit(1);
		}
		break;

	case TAG_AC_SYSTEM_ERROR:
		d_dbg("[%d]: TAG_AC_SYSTEM_ERROR\n", getpid());
		if (conn->printACNames)
		{
			printf("Got a System-Error tag: %.*s\n", (int)len, data);
		}
		else
		{
			d_error("[%d]: PADO: System-Error: %.*s\n", getpid(), (int)len, data);
			exit(1);
		}
		break;

	case TAG_GENERIC_ERROR:
		d_dbg("[%d]: TAG_GENERIC_ERROR\n", getpid());
		if (conn->printACNames)
		{
			printf("Got a Generic-Error tag: %.*s\n", (int)len, data);
		}
		else
		{
			d_error("[%d]: PADO: Generic-Error: %.*s\n", getpid(), (int)len, data);
			exit(1);
		}
		break;
	}
	d_dbg("[%d]: parsePADOTag <<<\n", getpid());
}

static void parsePADSTags(UINT16_t type, UINT16_t len, unsigned char *data, void *extra)
{
	PPPoEConnection * conn = (PPPoEConnection *)extra;
	switch (type)
	{
	case TAG_SERVICE_NAME:
		d_dbg("[%d]: PADS: Service-Name: '%.*s'\n", getpid(), (int) len, data);
		break;
	case TAG_SERVICE_NAME_ERROR:
		d_error("[%d]: PADS: Service-Name-Error: %.*s\n", getpid(), (int)len, data);
		exit(1);
		break;
	case TAG_AC_SYSTEM_ERROR:
		d_error("[%d]: PADS: System-Error: %.*s\n", getpid(), (int)len, data);
		exit(1);
		break;
	case TAG_GENERIC_ERROR:
		d_error("[%d]: PADS: Generic-Error: %.*s\n", getpid(), (int)len, data);
		exit(1);
		break;
	case TAG_RELAY_SESSION_ID:
		conn->relayId.type = htons(type);
		conn->relayId.length = htons(len);
		memcpy(conn->relayId.payload, data, len);
		break;
	}
}

static void sendPADI(PPPoEConnection * conn)
{
	PPPoEPacket packet;
	unsigned char * cursor = packet.payload;
	PPPoETag *svc = (PPPoETag *)(&packet.payload);
	UINT16_t namelen = 0;
	UINT16_t plen;

	if (conn->serviceName) namelen = (UINT16_t)strlen(conn->serviceName);
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet.payload, plen);

	/* Set destination to Ethernet broadcast address */
	memset(packet.ethHdr.h_dest, 0xff, ETH_ALEN);
	memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

	packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
	packet.ver = 1;
	packet.type = 1;
	packet.code = CODE_PADI;
	packet.session = 0;

	svc->type = TAG_SERVICE_NAME;
	svc->length = htons(namelen);
	CHECK_ROOM(cursor, packet.payload, namelen+TAG_HDR_SIZE);

	if (namelen)
	{
		memcpy(svc->payload, conn->serviceName, strlen(conn->serviceName));
	}
	cursor += namelen + TAG_HDR_SIZE;

	d_dbg("[%d]: %s\n", getpid(), (conn->useHostUniq? "useHostUniq":"not useHostUniq"));
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

#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
	syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Sending PADI for %s.", linkname);
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Sending PADI for %s.", linkname);
#endif	
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:025[%s]", linkname);
#endif
	update_statusfile("PPPoE:PADI");
	sendPacket(conn, conn->discoverySocket, &packet, (int)(plen + HDR_SIZE));
}

static void waitForPADO(PPPoEConnection * conn, int timeout)
{
	fd_set readable;
	int r = -1;
	struct timeval tv;
	PPPoEPacket packet;
	int len;

	struct PacketCriteria pc;
	pc.conn = conn;
	pc.acNameOK = (conn->acName) ? 0 : 1;
	pc.serviceNameOK = (conn->serviceName) ? 0 : 1;
	pc.seenACName = 0;
	pc.seenServiceName = 0;

	do
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		FD_ZERO(&readable);
		FD_SET(conn->discoverySocket, &readable);

		while (!got_sigterm) /* Modified by Kwest Wan 20080919 to terminate pppd immediately if PPPoE server don't reponse */
		{
			d_dbg("[%d]: waitPADO select >>>>\n", getpid());
			r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
			d_dbg("[%d]: waitPADO select (%d) <<<<\n", getpid(), r);
			if (r >= 0 || errno != EINTR) break;
		}
		if (r < 0)
		{
			d_error("[%d]: select (waitForPADS)\n", getpid());
			return;
		}
		if (r == 0) return;

		/* Get the packet */
		d_dbg("[%d]: receivePacket >>>\n", getpid());
		receivePacket(conn->discoverySocket, &packet, &len);
		d_dbg("[%d]: receivePacket <<<\n", getpid());

		/* Check length */
		if (ntohs(packet.length) + HDR_SIZE > len)
		{
			d_error("[%d]: Bogus PPPoE length field (%u)\n", getpid(), (unsigned int)ntohs(packet.length));
			continue;
		}

		/* If it's not for us, loop again */
		if (!packetIsForMe(conn, &packet))
		{
			d_dbg("[%d]: packet is not for me!\n", getpid());
			continue;
		}

		/* Is it PADS ? */
		if (packet.code == CODE_PADO)
		{
			if (NOT_UNICAST(packet.ethHdr.h_source))
			{
				d_info("[%d]: Ignoring PADO packet from non-unicast MAC address\n", getpid());
#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
				syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Ignore PADO packet from non-unicast MAC address. (%s)", linkname);
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Ignore PADO packet from non-unicast MAC address. (%s)", linkname);
#endif	
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:029[%s]", linkname);
#endif
				continue;
			}
			kpppoe_parsePacket(&packet, parsePADOTags, &pc);
			if (!pc.seenACName)
			{
				d_info("[%d]: Ignoring PADO packet with no AC-Name tag\n", getpid());
#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
				syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Ignore PADO packet with no AC-Name tag. (%s)", linkname);
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Ignore PADO packet with no AC-Name tag. (%s)", linkname);
#endif	
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:030[%s]", linkname);
#endif
				continue;
			}
			if (!pc.seenServiceName)
			{
				d_info("[%d]: Ignoring PADO packet with no Service-Name tag\n", getpid());
#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
				syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Ignore PADO packet with no Service-Name tag. (%s)", linkname);
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Ignore PADO packet with no Service-Name tag. (%s)", linkname);
#endif	
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:031[%s]", linkname);
#endif
				continue;
			}
			conn->numPADOs++;
			if (conn->printACNames) printf("------------------------------------------------------\n");
			update_statusfile("PPPoE:PADO");
			d_dbg("[%d]: acNameOK:%d, serviceNameOK:%d\n", getpid(), pc.acNameOK, pc.serviceNameOK);
			if (pc.acNameOK && pc.serviceNameOK)
			{
				memcpy(conn->peerEth, packet.ethHdr.h_source, ETH_ALEN);
				if (conn->printACNames)
				{
					printf("AC-Ethernet-Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
							(unsigned)conn->peerEth[0],
							(unsigned)conn->peerEth[1],
							(unsigned)conn->peerEth[2],
							(unsigned)conn->peerEth[3],
							(unsigned)conn->peerEth[4],
							(unsigned)conn->peerEth[5]);
					continue;
				}
				conn->discoveryState = STATE_RECEIVED_PADO;
#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
				syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Received PADO for %s, from: %02x:%02x:%02x:%02x:%02x:%02x",
						linkname,
						(unsigned)conn->peerEth[0],
						(unsigned)conn->peerEth[1],
						(unsigned)conn->peerEth[2],
						(unsigned)conn->peerEth[3],
						(unsigned)conn->peerEth[4],
						(unsigned)conn->peerEth[5]);
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Received PADO for %s, from: %02x:%02x:%02x:%02x:%02x:%02x",
						linkname,
						(unsigned)conn->peerEth[0],
						(unsigned)conn->peerEth[1],
						(unsigned)conn->peerEth[2],
						(unsigned)conn->peerEth[3],
						(unsigned)conn->peerEth[4],
						(unsigned)conn->peerEth[5]);
#endif	
#else
				syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:026[%s][%02x:%02x:%02x:%02x:%02x:%02x]",
						linkname,
						(unsigned)conn->peerEth[0],
						(unsigned)conn->peerEth[1],
						(unsigned)conn->peerEth[2],
						(unsigned)conn->peerEth[3],
						(unsigned)conn->peerEth[4],
						(unsigned)conn->peerEth[5]);
#endif
				break;
			}
		}
		else
		{
			d_dbg("[%d]: this is not PADO!\n", getpid());
		}
	} while (conn->discoveryState != STATE_RECEIVED_PADO);
}

static void sendPADR(PPPoEConnection * conn)
{
	PPPoEPacket packet;
	PPPoETag * svc = (PPPoETag *)packet.payload;
	unsigned char * cursor = packet.payload;

	UINT16_t namelen = 0;
	UINT16_t plen;

	if (conn->serviceName) namelen = (UINT16_t)strlen(conn->serviceName);
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
	if (namelen) memcpy(svc->payload, conn->serviceName, namelen);
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

#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
	syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Sending PADR for %s.", linkname);
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Sending PADR for %s.", linkname);
#endif	
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:027[%s]", linkname);
#endif
	packet.length = htons(plen);
	sendPacket(conn, conn->discoverySocket, &packet, (int)(plen + HDR_SIZE));
}

extern unsigned short sts_session_id;

static void waitForPADS(PPPoEConnection * conn, int timeout)
{
	fd_set readable;
	int r;
	struct timeval tv;
	PPPoEPacket packet;
	int len;

	d_dbg("[%d]: waitForPADS: >>> \n", getpid());

	do
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		FD_ZERO(&readable);
		FD_SET(conn->discoverySocket, &readable);

		while (1)
		{
			r = select(conn->discoverySocket + 1, &readable, NULL, NULL, &tv);
			if (r >= 0 || errno != EINTR) break;
		}
		if (r < 0)
		{
			d_error("[%d]: select (waitForPADS)\n", getpid());
			return;
		}
		if (r == 0) return;

		/* Get the packet */
		receivePacket(conn->discoverySocket, &packet, &len);
		if (packet.code == CODE_PADS) d_dbg("[%d]: GOT a PADS!\n", getpid());

		/* Check length */
		if (ntohs(packet.length) + HDR_SIZE > len)
		{
			d_error("[%d]: Bogus PPPoE length field (%u)\n", getpid(), (unsigned int)ntohs(packet.length));
			continue;
		}

		/* If it's not from the AC, it's not for me */
		if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) continue;

		/* If it's not for us, loop again */
		if (!packetIsForMe(conn, &packet)) continue;

		/* Is it PADS ? */
		if (packet.code == CODE_PADS)
		{
			/* Parse for goodies */
			kpppoe_parsePacket(&packet, parsePADSTags, conn);
			conn->discoveryState = STATE_SESSION;
			break;
		}

	} while (conn->discoveryState != STATE_SESSION);

	/* Don't bother with ntohs; we'll just end up converting it back... */
	conn->session = packet.session;
	d_info("[%d]: PPP session is %d\n", getpid(), (int)ntohs(conn->session));
#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
	syslog(ALOG_NOTICE|LOG_NOTICE,"[Notice]PPPoE: Received PADS for %s. (Session ID: %x)", linkname, (int)ntohs(conn->session));
#else
	syslog(ALOG_NOTICE|LOG_NOTICE,"PPPoE: Received PADS for %s. (Session ID: %x)", linkname, (int)ntohs(conn->session));
#endif
#else
	syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:028[%s][%x]", linkname, (int)ntohs(conn->session));
#endif
	sts_session_id = conn->session;

	/* RFC 2516 says session id MUST NOT be zero of 0xffff */
	if (ntohs(conn->session) == 0 || ntohs(conn->session) == 0xffff)
		d_info("[%d]: Access concentrator used a session value of %x -- the AC is violating RFC 2516\n", getpid(), (unsigned int) ntohs(conn->session));
	d_dbg("[%d]: waitForPADS: <<< \n", getpid());
}

void kpppoe_discovery(PPPoEConnection * conn)
{
	int padiAttempts = 0;
	int padrAttempts = 0;
	int timeout = PADI_TIMEOUT;

	d_dbg("[%d]: kpppoe_discovery() >>>\n", getpid());
	
	/* Skip discovery and don't open discovery socket ? */
	if (conn->skipDiscovery && conn->noDiscoverySocket)
	{
		conn->discoveryState = STATE_SESSION;
		return;
	}

	conn->discoverySocket =
		openInterface(conn->ifName, Eth_PPPOE_Discovery, conn->myEth);

	/* Skip discovery ? */
	if (conn->skipDiscovery)
	{
		conn->discoveryState = STATE_SESSION;
		if (conn->killSession)
		{
			kpppoe_sendPADT(conn, "Session killed manually");
			exit(0);
		}
		return;
	}

	do
	{
		padiAttempts++;
		if (padiAttempts > MAX_PADI_ATTEMPTS)
		{
			d_warn("[%d]: Timeout waiting for PADO packets\n", getpid());
			return;
		}
		sendPADI(conn);
		conn->discoveryState = STATE_SENT_PADI;
		waitForPADO(conn, timeout);

		/* If we're just probing for access concentrators, don't do
		 * exponential backoff. This reduces the time for an unsuccessful
		 * probe to 15 seconds. */
		if (!conn->printACNames) timeout *= 2;
		if (conn->printACNames && conn->numPADOs) break;
	} while (conn->discoveryState == STATE_SENT_PADI);

	/* If we're only printing access concentrator names, we're done */
	if (conn->printACNames)
	{
		printf("-----------------------------------------------------------\n");
		exit(0);
	}

	timeout = PADI_TIMEOUT;
	do
	{
		padrAttempts++;
		if (padrAttempts > MAX_PADI_ATTEMPTS)
		{
			d_warn("[%d]: Timeout waiting for PADS packets, timeout=[%d]\n", getpid(), timeout);
			return;
		}
		sendPADR(conn);
		conn->discoveryState =  STATE_SENT_PADR;
		waitForPADS(conn, timeout);
		timeout *= 2;
	} while (conn->discoveryState == STATE_SENT_PADR);

	/* We're done. */
	conn->discoveryState = STATE_SESSION;
	return;
}
