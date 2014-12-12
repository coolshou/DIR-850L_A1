/* vi: set sw=4 ts=4: */
/* serverpacket.c
 *
 * Constuct and send DHCP server packets
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"
#include "files.h"

#include <elbox_config.h>
#include <syslog.h>
#include "asyslog.h"

/* prototype */
int sendOffer(struct dhcpMessage *oldpacket);
int sendNAK(struct dhcpMessage *oldpacket);
int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr);
int send_inform(struct dhcpMessage *oldpacket);

/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config.server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast)
{
	unsigned char *chaddr;
	u_int32_t ciaddr;
	
	//if config force broadcast,just always send broadcast.
	if (force_broadcast || server_config.force_bcast)
	{
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	}
	else if (ntohs(payload->flags) & BROADCAST_FLAG)
	{
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	}
	else if (payload->ciaddr)
	{
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	}
	else
	{
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}
	return raw_packet(payload, server_config.server, SERVER_PORT,
			ciaddr, CLIENT_PORT, chaddr, server_config.ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

	if (payload->giaddr) ret = send_packet_to_relay(payload);
	else ret = send_packet_to_client(payload, force_broadcast);
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config.server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet)
{
	packet->siaddr = server_config.siaddr;
	if (server_config.sname)
		strncpy((char *)packet->sname, server_config.sname, sizeof(packet->sname) - 1);
	if (server_config.boot_file)
		strncpy((char *)packet->file, server_config.boot_file, sizeof(packet->file) - 1);
}

/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	u_int32_t req_align, lease_time_align = server_config.lease;
	unsigned char *req, *lease_time;
	/* +++ Joy added hostname */
	unsigned char *host_name;
	char hname[256];
	/* --- Joy added hostname */
	struct option_set *curr;
	struct in_addr addr;
	int static_lease = 0;

	init_packet(&packet, oldpacket, DHCPOFFER);

	/* ADDME: if static, short circuit */
	/* +++ Joy added static leases */
	/* check if the client is in our static lease table */
	int j;
	if ((lease = find_static_lease_by_chaddr(oldpacket->chaddr)))
	{
		packet.yiaddr = lease->yiaddr;

		/* Set the lease time to infinity */
		lease_time_align = 0xFFFFFFFF;

		static_lease = 1;
	}
	/* --- Joy added static leases */
	else
	{
	/* the client is in our lease/offered table */
	addr.s_addr = 0;
	/* RFC 2131, 4.3.1:
	 * o The client's current address as recorded in the client's current binding ... */
	if ((lease = find_lease_by_chaddr(oldpacket->chaddr)))
	{
		/* o The client's previous address as recorded in the client's (now expired or released) binding ... */
		//+++ fix by siyou, the lease->expires is the time of expire time, not the time of lease.
		//if (!lease_expired(lease)) lease_time_align = lease->expires - /*time*/(0);
		if (!lease_expired(lease)) lease_time_align = lease->expires - get_uptime();
		addr.s_addr = lease->yiaddr;
	}
	/* o The address requested in the 'Requested IP Address' option, if that address is valid and not already allocated */
	else
	{
		/* Or the client has a requested ip */
		req = get_option(oldpacket, DHCP_REQUESTED_IP);

		if (req)
		{
			/* Don't look here (ugly hackish thing to do) */
			memcpy(&req_align, req, 4);

			/* and the ip is in the lease range */
			if (ntohl(req_align) >= ntohl(server_config.start) &&
				ntohl(req_align) <= ntohl(server_config.end) &&
				find_static_lease_by_yiaddr(req_align)==NULL &&
				/* and its not already taken/offered or its taken, but expired */
				((!(lease = find_lease_by_yiaddr(req_align)) || lease_expired(lease))))
			{
				addr.s_addr = req_align;
			}
		}
	}

	if (addr.s_addr)
	{
		packet.yiaddr = addr.s_addr;
	}
	/* o A new address allocated from the server's pool of available addresses */
	else
	{
		packet.yiaddr = find_address(0);
		/* try for an expired lease */
		if (!packet.yiaddr) packet.yiaddr = find_address(1);
	}

	if(!packet.yiaddr)
	{
		LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
		return -1;
	}

	/* +++ Joy added hostname */
	host_name = get_option(oldpacket,DHCP_HOST_NAME);
	/* --- Joy added hostname */

#if 0 //Joy modified: hostname
	if (!add_lease(packet.chaddr, packet.yiaddr, server_config.offer_time)) {
#else

	if (host_name)
	{
		memcpy(hname, host_name, *(host_name-1));
		hname[*(host_name-1)]='\0';
		host_name = (unsigned char *)hname;
	}

	uprintf("add lease\n\tchaddr = %02x:%02x:%02x:%02x:%02x:%02x\n\tyiaddr = %d.%d.%d.%d",
			packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
			packet.chaddr[3], packet.chaddr[4], packet.chaddr[5],
			(packet.yiaddr&0xff000000)>>24,
			(packet.yiaddr&0x00ff0000)>>16,
			(packet.yiaddr&0x0000ff00)>>8,
			(packet.yiaddr&0x000000ff));

	if (!add_lease(packet.chaddr, packet.yiaddr, server_config.offer_time, (char *)host_name))
	{
#endif
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
#ifndef LOGNUM
		syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server lease pool is full, OFFER abandoned.");
#else
		syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:021");
#endif
		return -1;
	}

	/* RFC 2131, 4.3.1: 
	 * o IF the client has requested a specific lease in the DHCPDISCOVER message, 
	 *   the server may choose either to return the requested lease or select another lease. */
	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
	{
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease)
			lease_time_align = server_config.lease;
	}

	/* Make sure we aren't just using the lease time from the previous offer */
	//+++ siyou, because when two dhcp discover received in one second, 
	//we will be wrong for the lease time.
	//if (lease_time_align < server_config.min_lease)
	if (lease_time_align <= server_config.min_lease)
		lease_time_align = server_config.lease;
	/* ADDME: end of short circuit */
	
	} /* end else */

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	/*--------------add by wenwen;2012/02/03-for-Static IP assignment-------------*/
	if(static_lease)
	{
		curr=lease->options;
	}
	else
	{
		curr = server_config.options;
	}
	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending OFFER of %s", inet_ntoa(addr));
#ifndef LOGNUM
	if(static_lease) syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server sending OFFER of %s for static DHCP client.", inet_ntoa(addr));
	else
	syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server sending OFFER of %s.", inet_ntoa(addr));
#else
	if(static_lease) syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:020[%s]", inet_ntoa(addr));
	else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:019[%s]", inet_ntoa(addr));
#endif
/* syslog 2008-04-01 dennis start */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
			syslog(ALOG_AP_SYSACT|LOG_NOTICE,"[SYSACT]DHCP Server assign IP %s TO Mac:%02x:%02x:%02x:%02x:%02x:%02x",inet_ntoa(addr),packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
#endif
/* syslog 2008-04-01 dennis end */
	return send_packet(&packet, 0);
}


int sendNAK(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	char mac[32];

	init_packet(&packet, oldpacket, DHCPNAK);

	DEBUG(LOG_INFO, "sending NAK");

	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
			packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);

#ifndef LOGNUM
	syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server sending NAK to %s.", mac);
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:023[%s]", mac);
#endif

	return send_packet(&packet, 1);
}


int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr)
{
	struct dhcpMessage packet;
	struct option_set *curr;
    struct dhcpOfferedAddr *Offered = NULL;
	unsigned char *lease_time;
	/* +++ Joy added hostname */
	unsigned char *host_name;
	char hname[256];
	/* --- Joy added hostname */
	u_int32_t lease_time_align = server_config.lease;
	struct in_addr addr;
	int is_static=0;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;
//+++joel modify
    if((Offered=find_static_lease_by_chaddr(oldpacket->chaddr)) != NULL)
	//direct to find the static table check if static client
    {
        is_static=1;
        lease_time_align = 0xFFFFFFFF;
    }
//---joel
	else if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
#if 0 //Joy modified for static lease
		if (lease_time_align > server_config.lease)
#else
		if (lease_time_align != 0xFFFFFFFF &&
			lease_time_align > server_config.lease)
#endif
			lease_time_align = server_config.lease;
		else if (lease_time_align < server_config.min_lease)
			lease_time_align = server_config.lease;
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	
	/*--------------add by wenwen;2012/02/03-for-Static IP assignment-------------*/
	if(is_static)
	{
		curr=Offered->options;
	}
	else
	{
			curr = server_config.options;
	}
	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending ACK to %s", inet_ntoa(addr));

#ifndef LOGNUM
	syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server sending ACK to %s. (Lease time = %ld)", inet_ntoa(addr), (long)lease_time_align);
#else
	syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:024[%s][%ld]", inet_ntoa(addr), (long)lease_time_align);
#endif

	/* +++ Joy added hostname */
	host_name = get_option(oldpacket,DHCP_HOST_NAME);
	if (host_name)
	{
		memcpy(hname, host_name, *(host_name-1));
		hname[*(host_name-1)]='\0';
		host_name = (unsigned char *)hname;
	}
	/* --- Joy added hostname */

	if (send_packet(&packet, 0) < 0) return -1;
	if( Offered != NULL )
	{
		Offered->ACKed = 1;
		if(host_name)
		{
			strncpy(Offered->hostname, (const char *)host_name, sizeof(Offered->hostname));
			Offered->hostname[sizeof(Offered->hostname)-1] = '\0';
		}
	}
#if 0 //Joy modified for static lease
	add_lease(packet.chaddr, packet.yiaddr, lease_time_align);
#else
	/* not add static lease into lease table */
	//is dynnamic write to lease file for web
	if (is_static==0)
	{
		if ((Offered = add_lease(packet.chaddr, packet.yiaddr, lease_time_align, (char *)host_name)))
			Offered->ACKed=1;

		/* write lease file for web to display client list */
		write_leases();
	}
#endif

	return 0;
}


int send_inform(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(&packet, oldpacket, DHCPACK);

	curr = server_config.options;
	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	return send_packet(&packet, 0);
}



