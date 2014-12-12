/* vi: set sw=4 ts=4: */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "pppd.h"
#include "fsm.h"
#include "lcp.h"
#include "ipcp.h"
#include "ccp.h"

#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ppp_defs.h>
#include "if_ppp.h"
#include "if_pppox.h"

#include <syslog.h>
#include <asyslog.h>
#include <elbox_config.h>
#include <dtrace.h>

#include "pppoe.h"

extern char linkname[];

extern char kpppoe_dev[];
extern char kpppoe_ac_name[];
extern char kpppoe_srv_name[];
extern int kpppoe_hostuniq;

extern void kpppoe_discovery(PPPoEConnection * conn);
void kpppoe_sendPADT(PPPoEConnection * conn, const char * msg);

struct channel kpppoe_channel;
static PPPoEConnection poeconn;
static PPPoEConnection *conn = NULL;

static int PPPOEConnectDevice(void)
{
	struct sockaddr_pppox sp;

	d_dbg("[%d]: PPPOEConnectDevice() >>>\n", getpid());

	conn->acName = kpppoe_ac_name[0] ? kpppoe_ac_name : NULL;
	conn->serviceName = kpppoe_srv_name[0] ? kpppoe_srv_name : NULL;
	conn->ifName = kpppoe_dev;
	conn->discoverySocket = -1;
	conn->sessionSocket = -1;
	conn->useHostUniq = kpppoe_hostuniq;
	
	strlcpy(ppp_devnam, kpppoe_dev, 32);
	kpppoe_discovery(conn);
	if (conn->discoveryState != STATE_SESSION)
	{
		d_error("[%d]: Unable to complete PPPoE Discovery!\n", getpid());
		return -1;
	}

	/* Set PPPoE session-number for further consumption */
	ppp_session_number = ntohs(conn->session);

	/* Make the session socket */
	conn->sessionSocket = socket(AF_PPPOX, SOCK_STREAM, PX_PROTO_OE);
	if (conn->sessionSocket < 0)
	{	
		d_error("[%d]: Failed to create PPPoE socket\n", getpid());
		return -1;
	}
	sp.sa_family = AF_PPPOX;
	sp.sa_protocol = PX_PROTO_OE;
	sp.sa_addr.pppoe.sid = conn->session;
	memcpy(sp.sa_addr.pppoe.dev, conn->ifName, IFNAMSIZ);
	memcpy(sp.sa_addr.pppoe.remote, conn->peerEth, ETH_ALEN);

	/* Set remote_number for ServPoET */
	sprintf(remote_number, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned) conn->peerEth[0],
			(unsigned) conn->peerEth[1],
			(unsigned) conn->peerEth[2],
			(unsigned) conn->peerEth[3],
			(unsigned) conn->peerEth[4],
			(unsigned) conn->peerEth[5]);

	if (connect(conn->sessionSocket, (struct sockaddr *)&sp,
				sizeof(struct sockaddr_pppox)) < 0)
	{
		d_error("[%d]: Failed to connect PPPoE socket: %d\n", getpid(), errno);
		return -1;
	}
	
	return conn->sessionSocket;
}

static void PPPOEDisconnectDevice(void)
{
	struct sockaddr_pppox sp;

	kpppoe_sendPADT(conn, "User Request");
	
	sp.sa_family = AF_PPPOX;
	sp.sa_protocol = PX_PROTO_OE;
	sp.sa_addr.pppoe.sid = 0;
	memcpy(sp.sa_addr.pppoe.dev, conn->ifName, IFNAMSIZ);
	memcpy(sp.sa_addr.pppoe.remote, conn->peerEth, ETH_ALEN);
	if (connect(conn->sessionSocket, (struct sockaddr *)&sp, sizeof(struct sockaddr_pppox)) < 0)
	{
		d_error("[%d]: Failed to discconect PPPoE socket: %d\n", getpid(), errno);
	}
	close(conn->sessionSocket);
}

extern unsigned int sts_mtu;

static void PPPOESendConfig(int mtu, u_int32_t asyncmap, int pcomp, int accomp)
{
	int sock;
	struct ifreq ifr;

	if (mtu > MAX_PPPOE_MTU)
	{
		d_warn("[%d]: Couldn't increase MTU to %d\n", getpid(), mtu);
		mtu = MAX_PPPOE_MTU;
	}
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		d_error("[%d]: Couldn't create IP socket\n", getpid());
		return;
	}
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = mtu;
	if (ioctl(sock, SIOCSIFMTU, &ifr) < 0)
	{
		d_error("[%d]: Couldn't set interface MTU to %d\n", getpid(), mtu);
	}
	(void) close(sock);

	sts_mtu = mtu;
}

static void PPPOERecvConfig(int mru, u_int32_t asyncmap, int pcomp, int accomp)
{
	if (mru > MAX_PPPOE_MTU)
		d_warn("[%d]: Couldn't increase MRU to %d\n", getpid(), mru);
}

int PPPOEInitDevice(void)
{
	the_channel = &kpppoe_channel;
	modem = 0;

	lcp_allowoptions[0].neg_accompression = 0;
	lcp_wantoptions[0].neg_accompression = 0;

	lcp_allowoptions[0].neg_asyncmap = 0;
	lcp_wantoptions[0].neg_asyncmap = 0;

	lcp_allowoptions[0].neg_pcompression = 0;
	lcp_wantoptions[0].neg_pcompression = 0;

	ccp_allowoptions[0].deflate = 0;
	ccp_wantoptions[0].deflate = 0;

	ipcp_allowoptions[0].neg_vj = 0;
	ipcp_wantoptions[0].neg_vj = 0;

	ccp_allowoptions[0].bsd_compress = 0;
	ccp_wantoptions[0].bsd_compress = 0;

	conn = &poeconn;
	memset(conn, 0, sizeof(PPPoEConnection));

	conn->acName = kpppoe_ac_name[0] ? kpppoe_ac_name : NULL;
	conn->serviceName = kpppoe_srv_name[0] ? kpppoe_srv_name : NULL;
	conn->ifName = kpppoe_dev;
	conn->discoverySocket = -1;
	conn->sessionSocket = -1;
	conn->useHostUniq = 0;
	return 1;
}

void kpppoe_sendPADT(PPPoEConnection * conn, const char * msg)
{
	PPPoEPacket packet;
	unsigned char *cursor = packet.payload;
	UINT16_t plen = 0;

	/* Do nothing if no session established yet */
	if (!conn->session) return;

	/* Do nothing if no discovery socket */
	if (conn->discoverySocket < 0) return;

	memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
	memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

	packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
	packet.ver = 1;
	packet.type = 1;
	packet.code = CODE_PADT;
	packet.session = conn->session;

	/* Restart Session to zero so there is no possibility of
	 * recursuve calls to this function by any signal handler */
	conn->session = 0;

	/* If we're using Host-Uniq, copy it over. */
	if (conn->useHostUniq)
	{
		PPPoETag hostUniq;
		pid_t pid = getpid();
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(pid));
		memcpy(hostUniq.payload, &pid, sizeof(pid));
		memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
		cursor += sizeof(pid) + TAG_HDR_SIZE;
		plen += sizeof(pid) + TAG_HDR_SIZE;
	}

	/* Copy error message */
	if (msg)
	{
		PPPoETag err;
		size_t elen = strlen(msg);
		err.type = htons(TAG_GENERIC_ERROR);
		err.length = htons(elen);
		strcpy((char *)err.payload, msg);
		memcpy(cursor, &err, elen + TAG_HDR_SIZE);
		cursor += elen + TAG_HDR_SIZE;
		plen += elen + TAG_HDR_SIZE;
	}

	/* Copy cookie and relay-ID if needed */
	if (conn->cookie.type)
	{
		CHECK_ROOM(cursor, packet.payload,
					ntohs(conn->cookie.length) + TAG_HDR_SIZE);
		memcpy(cursor, &conn->cookie, ntohs(conn->cookie.length) + TAG_HDR_SIZE);
		cursor += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
		plen += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
	}

	if (conn->relayId.type)
	{
		CHECK_ROOM(cursor, packet.payload,
					ntohs(conn->relayId.length) + TAG_HDR_SIZE);
		memcpy(cursor, &conn->relayId, ntohs(conn->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
	}

#ifndef LOGNUM
#ifdef ELBOX_PROGS_GPL_SYSLOGD_AP	
	syslog(ALOG_NOTICE|LOG_NOTICE, "[Notice]PPPoE: Sending PADT for %s. (Session ID: %x)", linkname, (int)ntohs(packet.session));
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "PPPoE: Sending PADT for %s. (Session ID: %x)", linkname, (int)ntohs(packet.session));
#endif	
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:032[%s][%x]", linkname, (int)ntohs(packet.session));
#endif
	packet.length = htons(plen);
	sendPacket(conn, conn->discoverySocket, &packet, (int)(plen + HDR_SIZE));
}

int kpppoe_parsePacket(PPPoEPacket * packet, ParseFunc * func, void * extra)
{
	UINT16_t len = ntohs(packet->length);
	unsigned char * curTag;
	UINT16_t tagType, tagLen;

	if (packet->ver != 1)
	{
		d_error("[%d]: Invalid PPPoE version (%d)\n", getpid(), (int)packet->ver);
		return -1;
	}
	if (packet->type != 1)
	{
		d_error("[%d]: Invalid PPPoE type (%d)\n", getpid(), (int)packet->type);
		return -1;
	}

	/* Do some sanity checks on packet */
	if (len > ETH_DATA_LEN - 6) /* 6-byte overhead for PPPoE header */
	{
		d_error("[%d]: Invalid PPPoE packet length (%u)\n", getpid(), len);
		return -1;
	}

	/* Step through the tags */
	curTag = packet->payload;
	while (curTag - packet->payload < len)
	{
		/* Alignment is not guaranteed, so do this by hand ... */
		tagType = (((UINT16_t)curTag[0]) << 8) + (UINT16_t)curTag[1];
		tagLen  = (((UINT16_t)curTag[2]) << 8) + (UINT16_t)curTag[3];
		if (tagType == TAG_END_OF_LIST) return 0;
		if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len)
		{
			d_error("[%d]: Invalid PPPoE tag length (%u)\n", getpid(), tagLen);
			return -1;
		}
		func(tagType, tagLen, curTag+TAG_HDR_SIZE, extra);
		curTag = curTag + TAG_HDR_SIZE + tagLen;
	}
	return 0;
}

struct channel kpppoe_channel =
{
	options: NULL,
	process_extra_options: NULL,
	check_options: NULL,
	connect: &PPPOEConnectDevice,
	disconnect: &PPPOEDisconnectDevice,
	establish_ppp: &generic_establish_ppp,
	disestablish_ppp: &generic_disestablish_ppp,
	send_config: &PPPOESendConfig,
	recv_config: &PPPOERecvConfig,
	close: NULL,
	cleanup: NULL
};
