/* vi: set sw=4 ts=4: */

/***********************************************************************
*
* common.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Common functions used by PPPoE client and server
*
* Copyright (C) 2000 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
* LIC: GPL
*
***********************************************************************/

static char const RCSID[] =
"$Id: common.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $";
/* For vsnprintf prototype */
#define _ISOC99_SOURCE 1

#include "pppoe.h"
#include "dtrace.h"


#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


/**********************************************************************
*%FUNCTION: parsePacket
*%ARGUMENTS:
* packet -- the PPPoE discovery packet to parse
* func -- function called for each tag in the packet
* extra -- an opaque data pointer supplied to parsing function
*%RETURNS:
* 0 if everything went well; -1 if there was an error
*%DESCRIPTION:
* Parses a PPPoE discovery packet, calling "func" for each tag in the packet.
* "func" is passed the additional argument "extra".
***********************************************************************/
int parsePacket(PPPoEPacket *packet, ParseFunc *func, void *extra)
{
	UINT16_t len = ntohs(packet->length);
	unsigned char *curTag;
	UINT16_t tagType, tagLen;

	if (packet->ver != 1)
	{
		d_warn("pppoe: Invalid PPPoE version (%d)", (int) packet->ver);
		return -1;
	}
	if (packet->type != 1)
	{
		d_warn("pppoe: Invalid PPPoE type (%d)", (int) packet->type);
		return -1;
	}

	/* Do some sanity checks on packet */
	if (len > ETH_DATA_LEN - 6)
	{	/* 6-byte overhead for PPPoE header */
		d_warn("pppoe: Invalid PPPoE packet length (%u)\n", len);
		return -1;
    }

	/* Step through the tags */
	curTag = packet->payload;
	while (curTag - packet->payload < len)
	{	/* Alignment is not guaranteed, so do this by hand... */
		tagType = (((UINT16_t) curTag[0]) << 8) + (UINT16_t) curTag[1];
		tagLen = (((UINT16_t) curTag[2]) << 8) + (UINT16_t) curTag[3];
		if (tagType == TAG_END_OF_LIST)
		{
			return 0;
		}
		if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len)
		{
			d_warn("Invalid PPPoE tag length (%u)", tagLen);
			return -1;
		}
		if (func(tagType, tagLen, curTag+TAG_HDR_SIZE, extra) < 0) return -1;
		curTag = curTag + TAG_HDR_SIZE + tagLen;
	}
	return 0;
}

/**********************************************************************
*%FUNCTION: findTag
*%ARGUMENTS:
* packet -- the PPPoE discovery packet to parse
* type -- the type of the tag to look for
* tag -- will be filled in with tag contents
*%RETURNS:
* A pointer to the tag if one of the specified type is found; NULL
* otherwise.
*%DESCRIPTION:
* Looks for a specific tag type.
***********************************************************************/
unsigned char * findTag(PPPoEPacket *packet, UINT16_t type, PPPoETag *tag)
{
	UINT16_t len = ntohs(packet->length);
	unsigned char *curTag;
	UINT16_t tagType, tagLen;

	if (packet->ver != 1)
	{
		d_warn("Invalid PPPoE version (%d)", (int) packet->ver);
		return NULL;
	}
	if (packet->type != 1)
	{
		d_warn("Invalid PPPoE type (%d)", (int) packet->type);
		return NULL;
	}

	/* Do some sanity checks on packet */
	if (len > ETH_DATA_LEN - 6)
	{	/* 6-byte overhead for PPPoE header */
		d_warn("Invalid PPPoE packet length (%u)", len);
		return NULL;
	}

	/* Step through the tags */
	curTag = packet->payload;
	while (curTag - packet->payload < len)
	{	/* Alignment is not guaranteed, so do this by hand... */
		tagType = (((UINT16_t) curTag[0]) << 8) + (UINT16_t) curTag[1];
		tagLen = (((UINT16_t) curTag[2]) << 8) + (UINT16_t) curTag[3];
		if (tagType == TAG_END_OF_LIST)
		{
			return NULL;
		}
		if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len)
		{
			d_warn("Invalid PPPoE tag length (%u)", tagLen);
			return NULL;
		}
		if (tagType == type)
		{
			memcpy(tag, curTag, tagLen + TAG_HDR_SIZE);
			return curTag;
		}
		curTag = curTag + TAG_HDR_SIZE + tagLen;
	}
	return NULL;
}

/**********************************************************************
*%FUNCTION: computeTCPChecksum
*%ARGUMENTS:
* ipHdr -- pointer to IP header
* tcpHdr -- pointer to TCP header
*%RETURNS:
* The computed TCP checksum
***********************************************************************/
UINT16_t computeTCPChecksum(unsigned char *ipHdr, unsigned char *tcpHdr)
{
	UINT32_t sum = 0;
	UINT16_t count = ipHdr[2] * 256 + ipHdr[3];
	unsigned char *addr = tcpHdr;
	unsigned char pseudoHeader[12];

	/* Count number of bytes in TCP header and data */
	count -= (ipHdr[0] & 0x0F) * 4;

	memcpy(pseudoHeader, ipHdr+12, 8);
	pseudoHeader[8] = 0;
	pseudoHeader[9] = ipHdr[9];
	pseudoHeader[10] = (count >> 8) & 0xFF;
	pseudoHeader[11] = (count & 0xFF);

	/* Checksum the pseudo-header */
	sum += * (UINT16_t *) pseudoHeader;
	sum += * ((UINT16_t *) (pseudoHeader+2));
	sum += * ((UINT16_t *) (pseudoHeader+4));
	sum += * ((UINT16_t *) (pseudoHeader+6));
	sum += * ((UINT16_t *) (pseudoHeader+8));
	sum += * ((UINT16_t *) (pseudoHeader+10));

	/* Checksum the TCP header and data */
	while (count > 1)
	{
		sum += * (UINT16_t *) addr;
		addr += 2;
		count -= 2;
	}
	if (count > 0)
	{
		sum += *addr;
	}

	while(sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (UINT16_t) (~sum & 0xFFFF);
}

/**********************************************************************
*%FUNCTION: clampMSS
*%ARGUMENTS:
* packet -- PPPoE session packet
* dir -- either "incoming" or "outgoing"
* clampMss -- clamp value
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Clamps MSS option if TCP SYN flag is set.
***********************************************************************/
void clampMSS(PPPoEPacket *packet, char const *dir, int clampMss)
{
	unsigned char *tcpHdr;
	unsigned char *ipHdr;
	unsigned char *opt;
	unsigned char *endHdr;
	unsigned char *mssopt = NULL;
	UINT16_t csum;
	int len, minlen;

	/* check PPP protocol type */
	if (packet->payload[0] & 0x01)
	{	/* 8 bit protocol type */
		/* Is it IPv4? */
		if (packet->payload[0] != 0x21)
		{	/* Nope, ignore it */
			return;
		}

		ipHdr = packet->payload + 1;
		minlen = 41;
	}
	else
	{	/* 16 bit protocol type */
		/* Is it IPv4? */
		if (packet->payload[0] != 0x00 ||
			packet->payload[1] != 0x21)
		{	/* Nope, ignore it */
			return;
		}

		ipHdr = packet->payload + 2;
		minlen = 42;
	}

	/* Is it too short? */
	len = (int) ntohs(packet->length);
	if (len < minlen)
	{	/* 20 byte IP header; 20 byte TCP header; at least 1 or 2 byte PPP protocol */
		return;
	}

	/* Verify once more that it's IPv4 */
	if ((ipHdr[0] & 0xF0) != 0x40)
	{
		return;
	}

	/* Is it a fragment that's not at the beginning of the packet? */
	if ((ipHdr[6] & 0x1F) || ipHdr[7])
	{	/* Yup, don't touch! */
		return;
	}
	/* Is it TCP? */
	if (ipHdr[9] != 0x06)
	{
		return;
	}

	/* Get start of TCP header */
	tcpHdr = ipHdr + (ipHdr[0] & 0x0F) * 4;

	/* Is SYN set? */
	if (!(tcpHdr[13] & 0x02))
	{
		return;
	}

	/* Compute and verify TCP checksum -- do not touch a packet with a bad
	 * checksum */
	csum = computeTCPChecksum(ipHdr, tcpHdr);
	if (csum)
	{
		d_warn("Bad TCP checksum %x", (unsigned int) csum);
		/* Upper layers will drop it */
		return;
	}

	/* Look for existing MSS option */
	endHdr = tcpHdr + ((tcpHdr[12] & 0xF0) >> 2);
	opt = tcpHdr + 20;
	while (opt < endHdr)
	{
		if (!*opt) break;	/* End of options */
		switch(*opt)
		{
		case 1:
			opt++;
			break;

		case 2:
			if (opt[1] != 4)
			{	/* Something fishy about MSS option length. */
				d_warn("Bogus length for MSS option (%u) from %u.%u.%u.%u",
		 				(unsigned int) opt[1],
		 				(unsigned int) ipHdr[12],
		 				(unsigned int) ipHdr[13],
		 				(unsigned int) ipHdr[14],
		 				(unsigned int) ipHdr[15]);
				return;
			}
			mssopt = opt;
			break;
		default:
			if (opt[1] < 2)
			{	/* Someone's trying to attack us? */
				d_warn("Bogus TCP option length (%u) from %u.%u.%u.%u",
		 				(unsigned int) opt[1],
		 				(unsigned int) ipHdr[12],
		 				(unsigned int) ipHdr[13],
		 				(unsigned int) ipHdr[14],
		 				(unsigned int) ipHdr[15]);
				return;
			}
			opt += (opt[1]);
			break;
		}
		/* Found existing MSS option? */
		if (mssopt) break;
	}

	/* If MSS exists and it's low enough, do nothing */
	if (mssopt)
	{
		unsigned mss = mssopt[2] * 256 + mssopt[3];
		if (mss <= clampMss)
		{
			return;
		}

		mssopt[2] = (((unsigned) clampMss) >> 8) & 0xFF;
		mssopt[3] = ((unsigned) clampMss) & 0xFF;
	}
	else
	{	/* No MSS option.  Don't add one; we'll have to use 536. */
		return;
	}

	/* Recompute TCP checksum */
	tcpHdr[16] = 0;
	tcpHdr[17] = 0;
	csum = computeTCPChecksum(ipHdr, tcpHdr);
	(* (UINT16_t *) (tcpHdr+16)) = csum;
}

/***********************************************************************
*%FUNCTION: sendPADT
*%ARGUMENTS:
* conn -- PPPoE connection
* msg -- if non-NULL, extra error message to include in PADT packet.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADT packet
***********************************************************************/
int sendPADT(PPPoEConnection *conn, char const *msg)
{
	PPPoEPacket packet;
	unsigned char *cursor = packet.payload;
	UINT16_t plen = 0;

	d_dbg("pppoe: >>> sendPADT(%s)\n", msg);
	
	/* Do nothing if no session established yet */
	if (!conn->session) return 0;

	/* Do nothing if no discovery socket */
	if (conn->discoverySocket < 0) return 0;

	memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
	memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

	packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
	packet.ver = 1;
	packet.type = 1;
	packet.code = CODE_PADT;
	packet.session = conn->session;

	/* Reset Session to zero so there is no possibility of
	 * recursive calls to this function by any signal handler */
	conn->session = 0;

	/* If we're using Host-Uniq, copy it over */
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
	sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));
	if (conn->debugFile)
	{
		dumpPacket(conn->debugFile, &packet, "SENT");
		fprintf(conn->debugFile, "\n");
		fflush(conn->debugFile);
	}
	d_info("pppoe: Sent PADT!\n");
	return 0;
}

/***********************************************************************
*%FUNCTION: sendPADTf
*%ARGUMENTS:
* conn -- PPPoE connection
* msg -- printf-style format string
* args -- arguments for msg
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADT packet with a formatted message
***********************************************************************/
int sendPADTf(PPPoEConnection *conn, char const *fmt, ...)
{
	char msg[512];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[511] = 0;
	return sendPADT(conn, msg);
}

/**********************************************************************
*%FUNCTION: parseLogErrs
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks error tags out of a packet and logs them.
***********************************************************************/
int parseLogErrs(UINT16_t type, UINT16_t len, unsigned char *data, void *extra)
{
	switch(type)
	{
	case TAG_SERVICE_NAME_ERROR:
		d_error("PADT: Service-Name-Error: %.*s\n", (int) len, data);
		break;
	case TAG_AC_SYSTEM_ERROR:
		d_error("PADT: System-Error: %.*s\n", (int) len, data);
		break;
	case TAG_GENERIC_ERROR:
		d_error("PADT: Generic-Error: %.*s\n", (int) len, data);
		break;
	}
	return 0;
}
