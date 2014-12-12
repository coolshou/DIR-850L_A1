/* vi: set sw=4 ts=4: */
/***********************************************************************
*
* if.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Functions for opening a raw socket and reading/writing raw Ethernet frames.
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
"$Id: if.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $";

#include "pppoe.h"
#include "dtrace.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#elif defined(HAVE_LINUX_IF_PACKET_H)
#include <linux/if_packet.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

/* Initialize frame types to RFC 2516 values.  Some broken peers apparently
   use different frame types... sigh... */

UINT16_t Eth_PPPOE_Discovery = ETH_PPPOE_DISCOVERY;
UINT16_t Eth_PPPOE_Session   = ETH_PPPOE_SESSION;

/**********************************************************************
*%FUNCTION: etherType
*%ARGUMENTS:
* packet -- a received PPPoE packet
*%RETURNS:
* ethernet packet type (see /usr/include/net/ethertypes.h)
*%DESCRIPTION:
* Checks the ethernet packet header to determine its type.
* We should only be receveing DISCOVERY and SESSION types if the BPF
* is set up correctly.  Logs an error if an unexpected type is received.
* Note that the ethernet type names come from "pppoe.h" and the packet
* packet structure names use the LINUX dialect to maintain consistency
* with the rest of this file.  See the BSD section of "pppoe.h" for
* translations of the data structure names.
***********************************************************************/
UINT16_t etherType(PPPoEPacket *packet)
{
	UINT16_t type = (UINT16_t)ntohs(packet->ethHdr.h_proto);
	if (type != Eth_PPPOE_Discovery && type != Eth_PPPOE_Session)
	{
		d_error("pppoe: etherType(): Invalid ether type 0x%x\n", type);
	}
	return type;
}

/**********************************************************************
*%FUNCTION: openInterface
*%ARGUMENTS:
* ifname -- name of interface
* type -- Ethernet frame type
* hwaddr -- if non-NULL, set to the hardware address
*%RETURNS:
* A raw socket for talking to the Ethernet card.  Exits on error.
*%DESCRIPTION:
* Opens a raw Ethernet socket
***********************************************************************/
int openInterface(char const *ifname, UINT16_t type, unsigned char *hwaddr)
{
	int optval=1;
	int fd;
	struct ifreq ifr;
	int domain, stype;
	struct sockaddr_ll sa;

	memset(&sa, 0, sizeof(sa));
	domain = PF_PACKET;
	stype = SOCK_RAW;

	if ((fd = socket(domain, stype, htons(type))) < 0)
	{	/* Give a more helpful message for the common error case */
		if (errno == EPERM)
		{
			d_error("pppoe: openInterface(): Cannot create raw socket.\n");
		}
		return -1;
    }

	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0)
	{
		d_error("pppoe: openInterface(): setsockopt fail!\n");
		close(fd);
		return -1;
	}

	/* Fill in hardware address */
	if (hwaddr)
	{
		strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
		{
			d_error("pppoe: openInterface(): ioctl(SIOCGIFHWADDR)\n");
			close(fd);
			return -1;
		}
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#ifdef ARPHRD_ETHER
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
		{
			d_error("pppoe: Interface %.16s is not Ethernet\n", ifname);
			close(fd);
			return -1;
		}
#endif
		if (NOT_UNICAST(hwaddr))
		{
			d_error("pppoe: Interface %.16s has broadcast/multicast MAC address??\n", ifname);
			close(fd);
			return -1;
		}
	}

	/* Sanity check on MTU */
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFMTU, &ifr) < 0)
	{
		d_error("pppoe: ioctl(SIOCGIFMTU)\n");
		close(fd);
		return -1;
	}
	if (ifr.ifr_mtu < ETH_DATA_LEN)
	{

		/* restore default mtu value of physical interface. */
		ifr.ifr_mtu = ETH_DATA_LEN;
		
		if (ioctl(fd, SIOCSIFMTU, &ifr) < 0)
		{
			d_error("Couldn't set interface MTU to %d\n", ETH_DATA_LEN);
			close(fd);
			return -1;
		}
		
		/*
		d_error("pppoe: Interface %.16s has MTU of %d -- should be %d.  You may have serious connection problems.\n",
					ifname, ifr.ifr_mtu, ETH_DATA_LEN);
		close(fd);
		return -1;
		*/
	}

	/* Get interface index */
	sa.sll_family = AF_PACKET;
	sa.sll_protocol = htons(type);

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
	{
		d_error("pppoe: ioctl(SIOCFIGINDEX): Could not get interface index\n");
		close(fd);
		return -1;
	}
	sa.sll_ifindex = ifr.ifr_ifindex;

	/* We're only interested in packets on specified interface */
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
	{
		d_error("pppoe: bind fail!\n");
		close(fd);
		return -1;
	}

	return fd;
}

/***********************************************************************
*%FUNCTION: sendPacket
*%ARGUMENTS:
* sock -- socket to send to
* pkt -- the packet to transmit
* size -- size of packet (in bytes)
*%RETURNS:
* 0 on success; -1 on failure
*%DESCRIPTION:
* Transmits a packet
***********************************************************************/
int
sendPacket(PPPoEConnection *conn, int sock, PPPoEPacket *pkt, int size)
{
	if (send(sock, pkt, size, 0) < 0)
	{
		d_error("pppoe: sendPacket(): send fail!\n");
		return -1;
	}
	return 0;
}

/***********************************************************************
*%FUNCTION: receivePacket
*%ARGUMENTS:
* sock -- socket to read from
* pkt -- place to store the received packet
* size -- set to size of packet in bytes
*%RETURNS:
* >= 0 if all OK; < 0 if error
*%DESCRIPTION:
* Receives a packet
***********************************************************************/
int
receivePacket(int sock, PPPoEPacket *pkt, int *size)
{
	if ((*size = recv(sock, pkt, sizeof(PPPoEPacket), 0)) < 0)
	{
		d_error("pppoe: receivePacket(): receive fail!\n");
		return -1;
	}
	return 0;
}
