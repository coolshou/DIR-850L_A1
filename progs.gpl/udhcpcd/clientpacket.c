/* vi: set sw=4 ts=4: */
/* clientpacket.c
 *
 * Packet generation and dispatching functions for the DHCP client.
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

#include <string.h>
#include <sys/socket.h>
#include <features.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "dhcpd.h"
#include "packet.h"
#include "options.h"
#include "dhcpc.h"
#include "debug.h"
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
/* 32 bit change to 64 bit dennis 20080311 end */

/* prototype */
uint32_t random_xid(void);
int send_discover(uint32_t xid, uint32_t requested);
int send_selecting(uint32_t xid, uint32_t server, uint32_t requested);
int send_renew(uint32_t xid, uint32_t server, uint32_t ciaddr);
int send_release(uint32_t server, uint32_t ciaddr);
int get_raw_packet(struct dhcpMessage *payload, int fd);
/*Erick DHCP Decline 20110415*/
int send_decline(uint32_t xid, uint32_t server, uint32_t ciaddr);

extern int use_unicast;		//add for use unicast

/* Create a random xid */
//unsigned long random_xid(void)
uint32_t random_xid(void)
{
	static int initialized;
	if (!initialized)
	{
		int fd;
/* 32 bit change to 64 bit dennis 20080311 start */
		uint32_t seed;
/* 32 bit change to 64 bit dennis 20080311 end */
		fd = open("/dev/urandom", 0);
		if (fd < 0 || read(fd, &seed, sizeof(seed)) < 0)
		{
			LOG(LOG_WARNING, "Could not load seed from /dev/urandom: %s", strerror(errno));
			seed = time(0);
		}
		if (fd >= 0) close(fd);
		srand(seed);
		initialized++;
	}
	return rand();
}

/* initialize a packet with the proper defaults */
static void init_packet(struct dhcpMessage *packet, char type)
{
#if 0
	struct vendor
	{
		char vendor, length;
		char str[sizeof("udhcp "VERSION)];
	}
	vendor_id = { DHCP_VENDOR,  sizeof("udhcp "VERSION) - 1, "udhcp "VERSION};
#endif

	init_header(packet, type);
	memcpy(packet->chaddr, client_config.arp, 6);
	add_option_string(packet->options, client_config.clientid);
	if (client_config.hostname) add_option_string(packet->options, client_config.hostname);
	//+++ mark by siyou. Dlink said vendor option cause problem in German big ISP. 
	//add_option_string(packet->options, (unsigned char *) &vendor_id);
}


/* Add a paramater request list for stubborn DHCP servers. Pull the data
 * from the struct in options.c. Don't do bounds checking here because it
 * goes towards the head of the packet. */
static void add_requests(struct dhcpMessage *packet)
{
	int end = end_option(packet->options);
	int i, len = 0;

	packet->options[end + OPT_CODE] = DHCP_PARAM_REQ;
	for (i = 0; options[i].code; i++)
		if (options[i].flags & OPTION_REQ)
			packet->options[end + OPT_DATA + len++] = options[i].code;
	/*phelpsll: request option 43.
		if we don't request option 43, it can't get option 43 from cisco switch.*/
	packet->options[end + OPT_DATA + len++] = 43;
	packet->options[end + OPT_LEN] = len;

	/*traveller:request option 60,
		if we don't have option 60 in option 55,some server will not read option 60 data*/
	packet->options[end + OPT_DATA + len++] = 60;
	packet->options[end + OPT_LEN] = len;

	/* UNH-IOL add 6rd option*/
	packet->options[end + OPT_DATA + len++] = 212;
	packet->options[end + OPT_LEN] = len;

	packet->options[end + OPT_DATA + len] = DHCP_END;
#ifdef ELBOX_PROGS_GPL_UDHCP_REQUEST_BROADCAST
	/*   use unicast or broadcast    */	
	if(use_unicast != 1)
        packet->flags=packet->flags | htons(BROADCAST_FLAG);
    else
#endif
		packet->flags=packet->flags & htons((unsigned short)(~BROADCAST_FLAG));
}
//start add by phelpsll for get ac IP
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
static unsigned char* type_len_value_append(unsigned char* ptr, unsigned char type, unsigned char len, char* value)
{
	ptr[0]=type;
	ptr[1]=len;
	memcpy(ptr+2, value, len);
	return ptr+2+len;
}
static int add_option_enterprise_code(unsigned char *optionptr, unsigned short code)
{
	unsigned char option[256];
	unsigned char option2[256];
	char* vendor="dlink";
	char* category="accesspoint";
	char* model = "dap2690";
	char* sw_ver = "v101";
	unsigned short *u16 = (unsigned short*)(option+2);
	unsigned char* ptr;
	option[OPT_CODE]=DHCP_VENDOR;
	*u16 = htons(code);
	ptr = option+4;
	ptr = type_len_value_append(ptr,1,strlen(vendor),vendor);
	ptr = type_len_value_append(ptr,2,strlen(category),category);
	ptr = type_len_value_append(ptr,3,strlen(model), model);
	ptr = type_len_value_append(ptr,4,strlen(sw_ver), sw_ver);
	option[OPT_LEN]=ptr - option - 2;
	add_option_string(optionptr, option);
	option2[OPT_CODE]=DHCP_VENDOR_43;
	option2[OPT_LEN] = 2;
	u16 = (unsigned short*)(option2+2);
	*u16 = htons(code);
	add_option_string(optionptr,option2);
	return 0;
}

#endif
//end get ac IP
/* Broadcast a DHCP discover packet to the network, with an optionally requested IP */
/* 32 bit change to 64 bit dennis 20080311 start */
int send_discover(uint32_t xid, uint32_t requested)
/* 32 bit change to 64 bit dennis 20080311 end */
{
	struct dhcpMessage packet;

	init_packet(&packet, DHCPDISCOVER);
	packet.xid = xid;
	if (requested)
		add_simple_option(packet.options, DHCP_REQUESTED_IP, requested);
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	add_option_enterprise_code(packet.options, ENTERPRISE_CODE);
#endif
	add_requests(&packet);
	LOG(LOG_DEBUG, "Sending discover...");
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
			SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}


/* Broadcasts a DHCP request message */
/* 32 bit change to 64 bit dennis 20080311 start */
int send_selecting(uint32_t xid, uint32_t server, uint32_t requested)
/* 32 bit change to 64 bit dennis 20080311 end */
{
	struct dhcpMessage packet;
	struct in_addr addr;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;

	add_simple_option(packet.options, DHCP_REQUESTED_IP, requested);
	add_simple_option(packet.options, DHCP_SERVER_ID, server);
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	add_option_enterprise_code(packet.options, ENTERPRISE_CODE);
#endif	
	add_requests(&packet);
	addr.s_addr = requested;
	LOG(LOG_DEBUG, "Sending select for %s...", inet_ntoa(addr));
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
			SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}


/* Unicasts or broadcasts a DHCP renew message */
/* 32 bit change to 64 bit dennis 20080311 start */
int send_renew(uint32_t xid, uint32_t server, uint32_t ciaddr)
/* 32 bit change to 64 bit dennis 20080311 end */
{
	struct dhcpMessage packet;
	int ret = 0;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;
	packet.ciaddr = ciaddr;
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	add_option_enterprise_code(packet.options, ENTERPRISE_CODE);
#endif
	add_requests(&packet);
	LOG(LOG_DEBUG, "Sending renew...");
	if (server)
		ret = kernel_packet(&packet, ciaddr, CLIENT_PORT, server, SERVER_PORT);
	else
		ret = raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
				SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
	return ret;
}


/* Unicasts a DHCP release message */

/* 32 bit change to 64 bit dennis 20080311 start */
int send_release(uint32_t server, uint32_t ciaddr)
/* 32 bit change to 64 bit dennis 20080311 end */
{
	struct dhcpMessage packet;

	init_packet(&packet, DHCPRELEASE);
	packet.xid = random_xid();
	packet.ciaddr = ciaddr;

	add_simple_option(packet.options, DHCP_REQUESTED_IP, ciaddr);
	add_simple_option(packet.options, DHCP_SERVER_ID, server);
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	add_option_enterprise_code(packet.options, ENTERPRISE_CODE);
#endif
	LOG(LOG_DEBUG, "Sending release...");
	return kernel_packet(&packet, ciaddr, CLIENT_PORT, server, SERVER_PORT);
}


/* return -1 on errors that are fatal for the socket, -2 for those that aren't */
int get_raw_packet(struct dhcpMessage *payload, int fd)
{
	int bytes;
	struct udp_dhcp_packet packet;
	u_int32_t source, dest;
	u_int16_t check;

	memset(&packet, 0, sizeof(struct udp_dhcp_packet));
	bytes = read(fd, &packet, sizeof(struct udp_dhcp_packet));
	if (bytes < 0)
	{
		DEBUG(LOG_INFO, "couldn't read on raw listening socket -- ignoring");
		usleep(500000); /* possible down interface, looping condition */
		return -1;
	}

	if (bytes < (int) (sizeof(struct iphdr) + sizeof(struct udphdr)))
	{
		DEBUG(LOG_INFO, "message too short, ignoring");
		return -2;
	}

	if (bytes < (int)ntohs(packet.ip.tot_len))
	{
		DEBUG(LOG_INFO, "Truncated packet");
		return -2;
	}

	/* ignore any extra garbage bytes */
	bytes = ntohs(packet.ip.tot_len);

	/* Make sure its the right packet for us, and that it passes sanity checks */
	if (packet.ip.protocol != IPPROTO_UDP || packet.ip.version != IPVERSION ||
			packet.ip.ihl != sizeof(packet.ip) >> 2 || packet.udp.dest != htons(CLIENT_PORT) ||
			bytes > (int) sizeof(struct udp_dhcp_packet) ||
			ntohs(packet.udp.len) != (unsigned short)(bytes - sizeof(packet.ip)))
	{
		DEBUG(LOG_INFO, "unrelated/bogus packet");
		return -2;
	}

	/* check IP checksum */
	check = packet.ip.check;
	packet.ip.check = 0;
	if (check != checksum(&(packet.ip), sizeof(packet.ip)))
	{
		DEBUG(LOG_INFO, "bad IP header checksum, ignoring");
		return -1;
	}

	/* verify the UDP checksum by replacing the header with a psuedo header */
	source = packet.ip.saddr;
	dest = packet.ip.daddr;
	check = packet.udp.check;
	packet.udp.check = 0;
	memset(&packet.ip, 0, sizeof(packet.ip));

	packet.ip.protocol = IPPROTO_UDP;
	packet.ip.saddr = source;
	packet.ip.daddr = dest;
	packet.ip.tot_len = packet.udp.len; /* cheat on the psuedo-header */
	if (check && check != checksum(&packet, bytes))
	{
		DEBUG(LOG_ERR, "packet with bad UDP checksum received, ignoring");
		return -2;
	}

	memcpy(payload, &(packet.data), bytes - (sizeof(packet.ip) + sizeof(packet.udp)));

	if (ntohl(payload->cookie) != DHCP_MAGIC)
	{
		LOG(LOG_ERR, "received bogus message (bad magic) -- ignoring");
		return -2;
	}
	DEBUG(LOG_INFO, "oooooh!!! got some!");
	return bytes - (sizeof(packet.ip) + sizeof(packet.udp));
}

/* Broadcast a DHCP discover packet to the network, with an optionally requested IP */
//int send_decline(unsigned long xid)
/* 32 bit change to 64 bit dennis 20080311 start */
int send_decline(uint32_t xid, uint32_t server, uint32_t ciaddr)
/* 32 bit change to 64 bit dennis 20080311 end */
{
	struct dhcpMessage packet;

	init_packet(&packet, DHCPDECLINE);
	packet.xid = xid;
/*Erick DHCP Decline 20110415*/
	add_simple_option(packet.options, DHCP_REQUESTED_IP, ciaddr);
	add_simple_option(packet.options, DHCP_SERVER_ID, server);
	
	LOG(LOG_DEBUG, "Sending decline...");
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
			SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}

