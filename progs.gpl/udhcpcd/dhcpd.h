/* dhcpd.h */
#ifndef _DHCPD_H
#define _DHCPD_H

#include <netinet/ip.h>
#include <netinet/udp.h>
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
/* 32 bit change to 64 bit dennis 20080311 end */
#include "libbb_udhcp.h"
#include "leases.h"
#include <elbox_config.h>
/************************************/
/* Defaults _you_ may want to tweak */
/************************************/

/* the period of time the client is allowed to use that address */
#define LEASE_TIME              (60*60*24*10) /* 10 days of seconds */

/* +++ Joy added static leases */
/* the maximum number of static leases */
#define MAX_STATIC_LEASES 32
/* --- Joy added static leases */

/* where to find the DHCP server configuration file */
#define DHCPD_CONF_FILE         "/etc/udhcpd.conf"

/*****************************************************************/
/* Do not modify below here unless you know what you are doing!! */
/*****************************************************************/

/* DHCP protocol -- see RFC 2131 */
#define SERVER_PORT		67
#define CLIENT_PORT		68

#define DHCP_MAGIC		0x63825363

/* DHCP option codes (partial list) */
#define DHCP_PADDING		0x00
#define DHCP_SUBNET		0x01
#define DHCP_TIME_OFFSET	0x02
#define DHCP_ROUTER		0x03
#define DHCP_TIME_SERVER	0x04
#define DHCP_NAME_SERVER	0x05
#define DHCP_DNS_SERVER		0x06
#define DHCP_LOG_SERVER		0x07
#define DHCP_COOKIE_SERVER	0x08
#define DHCP_LPR_SERVER		0x09
#define DHCP_HOST_NAME		0x0c
#define DHCP_BOOT_SIZE		0x0d
#define DHCP_DOMAIN_NAME	0x0f
#define DHCP_SWAP_SERVER	0x10
#define DHCP_ROOT_PATH		0x11
#define DHCP_IP_TTL		0x17
#define DHCP_MTU		0x1a
#define DHCP_BROADCAST		0x1c
#define DHCP_STATIC_ROUTE	0x21
#define DHCP_NTP_SERVER		0x2a
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
#define DHCP_VENDOR_43          0x2b
#endif
#define DHCP_WINS_SERVER	0x2c
#define DHCP_REQUESTED_IP	0x32
#define DHCP_LEASE_TIME		0x33
#define DHCP_OPTION_OVER	0x34
#define DHCP_MESSAGE_TYPE	0x35
#define DHCP_SERVER_ID		0x36
#define DHCP_PARAM_REQ		0x37
#define DHCP_MESSAGE		0x38
#define DHCP_MAX_SIZE		0x39
#define DHCP_T1			0x3a
#define DHCP_T2			0x3b
#define DHCP_VENDOR		0x3c
#define DHCP_CLIENT_ID		0x3d
#define DHCP_CLSLESS_ROUTE	0x79

#ifdef SIX_RD_OPTION
#define DHCP_6RD		(0xd4)
#endif //SIX_RD_OPTION

#define DHCP_MS_ROUTE 		0xf9

#define DHCP_END		0xFF


#define BOOTREQUEST		1
#define BOOTREPLY		2

#define ETH_10MB		1
#define ETH_10MB_LEN		6

#define DHCPDISCOVER		1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPDECLINE		4
#define DHCPACK			5
#define DHCPNAK			6
#define DHCPRELEASE		7
#define DHCPINFORM		8

#define BROADCAST_FLAG		0x8000

#define OPTION_FIELD		0
#define FILE_FIELD		1
#define SNAME_FIELD		2

/* miscellaneous defines */
#define MAC_BCAST_ADDR		(unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define OPT_CODE 0
#define OPT_LEN 1
#define OPT_DATA 2

struct option_set {
	unsigned char *data;
	struct option_set *next;
};

struct server_config_t {
	u_int32_t server;		/* Our IP, in network order */
	u_int32_t start;		/* Start address of leases, network order */
	u_int32_t end;			/* End of leases, network order */
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP
	u_int32_t lan_ip;			/* LAN's IP */ 
#endif
	struct option_set *options;	/* List of DHCP options loaded from the config file */
	char *interface;		/* The name of the interface to use */
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	char *wlanif;
#endif
	int ifindex;			/* Index number of the interface to use */
	unsigned char arp[6];		/* Our arp address */
/* 32 bit change to 64 bit dennis 20080311 start */
	uint32_t lease;
	uint32_t max_leases;
/* 32 bit change to 64 bit dennis 20080311 end */

	char remaining; 		/* should the lease file be interpreted as lease time remaining, or
			 		 * as the time the lease expires */
 /* 32 bit change to 64 bit dennis 20080311 start */
  uint32_t auto_time;
  uint32_t decline_time;
  uint32_t conflict_time;
  uint32_t offer_time;
  uint32_t min_lease;
/* 32 bit change to 64 bit dennis 20080311 end */
	char *lease_file;
	char *pidfile;
	char *notify_file;		/* What to run whenever leases are written */
	u_int32_t siaddr;		/* next server bootp option */
	char *sname;			/* bootp server name */
	char *boot_file;		/* bootp boot file option */
	// Sam Chen add for project seattle to call dhcp_helper to process leases file
	char *dhcp_helper;
	// Sam Chen end
	// Sam Chen add for leasing the IP (ex. 192.168.1.255/16) to client
	uint32_t mask;
	// Sam Chen end
	//joel for force send response using broadcast
	char force_bcast;
};

extern struct server_config_t server_config;
extern struct dhcpOfferedAddr *leases;
/* +++ Joy added static leases */
extern struct dhcpOfferedAddr *static_leases;
/* --- Joy added static leases */

uint32_t get_uptime(void);
#endif
