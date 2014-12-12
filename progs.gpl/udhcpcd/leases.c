/* vi: set sw=4 ts=4: */
/*
 * leases.c -- tools to manage DHCP leases
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "arpping.h"
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
/* 32 bit change to 64 bit dennis 20080311 end */
#include <elbox_config.h>
unsigned char blank_chaddr[] = {[0 ... 15] = 0};

/* clear every lease out that chaddr OR yiaddr matches and is nonzero */
void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr)
{
	unsigned int i, j;

	for (j = 0; j < 16 && !chaddr[j]; j++);

	for (i = 0; i < server_config.max_leases; i++)
	{
		if ((j != 16 && !memcmp(leases[i].chaddr, chaddr, 16))
			|| (yiaddr && leases[i].yiaddr == yiaddr))
		{
			memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
		}
	}
}


/* add a lease into the table, clearing out any old ones */
#if 0 //Joy modified: add hostname
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease)
#else
/* 32 bit change to 64 bit dennis 20080311 start */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, uint32_t lease, char *hostname)
/* 32 bit change to 64 bit dennis 20080311 end */

#endif
{
	struct dhcpOfferedAddr *oldest;
	struct in_addr in;

	/* clean out any old ones */
	clear_lease(chaddr, yiaddr);

	oldest = oldest_expired_lease();

	if (oldest)
	{
		memcpy(oldest->chaddr, chaddr, 16);
		oldest->yiaddr = yiaddr;
		#if 0 //hendry
		// Kloat Liu modified to store relative lease interval instead of time point.
		oldest->expires = /*time(0) + */lease;
		// Kloat Liu
		#else
		oldest->expires = get_uptime() + lease;
		#endif 
		
		/* +++ Joy added hostname */
		if (hostname)
		{
			strncpy(oldest->hostname, hostname, sizeof(oldest->hostname));
			oldest->hostname[sizeof(oldest->hostname)-1] = '\0';
		}
		/* --- Joy added hostname */
		in.s_addr = yiaddr;
		DEBUG(LOG_INFO, "add lease %02x:%02x:%02x:%02x:%02x:%02x %s %d",
				chaddr[0], chaddr[1], chaddr[2], chaddr[3], chaddr[4], chaddr[5],
				inet_ntoa(in), oldest->expires);
	}

	return oldest;
}


/* true if a lease has expired */
int lease_expired(struct dhcpOfferedAddr *lease)
{
	DEBUG(LOG_INFO, "%s: lease=%d, uptime=%u",__FUNCTION__, lease->expires, get_uptime());
	#if 0 
	/* 32 bit change to 64 bit dennis 20080311 start */
		// Kloat Liu modified to store relative lease interval instead of time point.
	    return (lease->expires <= 0 /*uint32_t) time(0)*/);
	    //Kloat Liu end
	/* 32 bit change to 64 bit dennis 20080311 end */
	#else
		return (lease->expires <= get_uptime());
	#endif 
	
}


/* Find the oldest expired lease, NULL if there are no expired leases */
 struct dhcpOfferedAddr *oldest_expired_lease(void)
{
	struct dhcpOfferedAddr *oldest = NULL;
	#if 0 //hendry
	/* 32 bit change to 64 bit dennis 20080311 start */
		// Kloat Liu modified to store relative lease interval instead of time point.
		uint32_t oldest_lease = /*time*/(0);
		// Kloat Liu end
	/* 32 bit change to 64 bit dennis 20080311 enc */
	#else
	uint32_t oldest_lease = get_uptime();
	#endif
	
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
	{
		if (oldest_lease >= leases[i].expires)
		{
			oldest_lease = leases[i].expires;
			oldest = &(leases[i]);
		}
	}
	return oldest;
}

/* +++ Joy added static leases */
/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_static_lease_by_chaddr(u_int8_t *chaddr)
{
	unsigned int i;

	for (i = 0; (i < MAX_STATIC_LEASES) && (static_leases[i].yiaddr); i++)
		if (!memcmp(static_leases[i].chaddr, chaddr, sizeof(static_leases[i].chaddr)))
			return &(static_leases[i]);

	return NULL;
}

struct dhcpOfferedAddr *find_static_lease_by_yiaddr(u_int32_t yiaddr)
{
	unsigned int i;

	for (i = 0; (i < MAX_STATIC_LEASES) && (static_leases[i].yiaddr); i++)
		if (static_leases[i].yiaddr == yiaddr) return &(static_leases[i]);

	return NULL;
}
/* --- Joy added static leases */

/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (!memcmp(leases[i].chaddr, chaddr, 16)) return &(leases[i]);

	return NULL;
}


/* Find the first lease that matches yiaddr, NULL is no match */
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (leases[i].yiaddr == yiaddr) return &(leases[i]);

	return NULL;
}


/* find an assignable address, it check_expired is true, we check all the expired leases as well.
 * Maybe this should try expired leases by age... */
u_int32_t find_address(int check_expired)
{
	u_int32_t addr, ret;
	struct dhcpOfferedAddr *lease = NULL;
	struct in_addr in;

	addr = ntohl(server_config.start); /* addr is in host order here */
	for (;addr <= ntohl(server_config.end); addr++)
	{
		/* ie, 192.168.55.0 */
		//if (!(addr & 0xFF)) continue;
		if (!(addr & server_config.mask)) continue;

		/* ie, 192.168.55.255 */
		//if ((addr & 0xFF) == 0xFF) continue;
		// Sam Chen add for leasing the IP (ex. 192.168.1.255/16) to client
		if ((addr & server_config.mask) == server_config.mask) continue;
		// Sam Chen end
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP			
	        if(addr==ntohl(server_config.lan_ip)){					
               	continue;
			}
#endif		

		/* lease is not taken */
		ret = htonl(addr);
		in.s_addr = ret;
		DEBUG(LOG_INFO, "trying %s...", inet_ntoa(in));
		if (find_static_lease_by_yiaddr(ret))
		{
			DEBUG(LOG_INFO, "in static list !!!");
			continue;
		}

		if ((!(lease = find_lease_by_yiaddr(ret)) ||

		     /* or it expired and we are checking for expired leases */
		     (check_expired  && lease_expired(lease))) &&

		     /* and it isn't on the network */
	    	     !check_ip(ret))
		{
			return ret;
		}
	}
	return 0;
}


/* check is an IP is taken, if it is, add it to the lease table */
int check_ip(u_int32_t addr)
{
	struct in_addr temp;

	uprintf("Send ARP to check IP.");
	if (arpping(addr, server_config.server, server_config.arp, server_config.interface) == 0)
	{
		temp.s_addr = addr;
	 	LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds",
				inet_ntoa(temp), server_config.conflict_time);
#if 0 //Joy modified: hostname
		add_lease(blank_chaddr, addr, server_config.conflict_time);
#else
		// kwest modified: don't reserving expired address for 1 hour,
		// or else GUI would show wrong Expired Time on DHCP Client List.
		//add_lease(blank_chaddr, addr, server_config.conflict_time, 0);
#endif
		return 1;
	}

	return 0;
}

