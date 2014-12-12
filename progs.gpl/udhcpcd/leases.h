/* vi: set sw=4 ts=4: */
/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H
#include <stdint.h>
#include "dhcpd.h"
struct dhcpOfferedAddr
{
	u_int8_t chaddr[16];
	u_int32_t yiaddr;	/* network order */
	u_int32_t expires;	/* host order */
	u_int32_t ACKed;
	char hostname[64];
	struct option_set *options;	
};

extern unsigned char blank_chaddr[];

void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr);
#if 0 //Joy modified: store hostname
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease);
#else

/* 32 bit change to 64 bit dennis 20080311 start */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, uint32_t lease, char *hostname); 
/* 32 bit change to 64 bit dennis 20080311 end */
#endif
int lease_expired(struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *oldest_expired_lease(void);
/* +++ Joy added static leases */
struct dhcpOfferedAddr *find_static_lease_by_chaddr(u_int8_t *chaddr);
struct dhcpOfferedAddr *find_static_lease_by_yiaddr(u_int32_t yiaddr);
/* --- Joy added static leases */
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr);
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr);
u_int32_t find_address(int check_expired);
int check_ip(u_int32_t addr);


#endif
