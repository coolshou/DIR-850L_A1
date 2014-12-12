/* vi: set sw=4 ts=4: */
/* peer.c */

#include <stddef.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "l2tp.h"
#include "dtrace.h"

l2tp_peer the_peer;

int l2tp_peer_init(void)
{
	struct hostent *he;

	if(inet_pton(AF_INET6 , l2tp_peer_addr , &the_peer.addr.u.addr6.sin6_addr) == 1)
	{
		//ipv6 address string
		the_peer.addr_type = AF_INET6;
		the_peer.addr.u.addr6.sin6_port = htons((unsigned short)l2tp_port);
		the_peer.addr.u.addr6.sin6_family = AF_INET6;
	}
	else
	{
		//domain name or ipv4 ip string
		he = gethostbyname(l2tp_peer_addr);
		if (!he)
		{
			return -1;
		}

		the_peer.addr_type = he->h_addrtype;
		if(the_peer.addr_type == AF_INET)
		{
			memcpy(&the_peer.addr.u.addr4.sin_addr.s_addr, he->h_addr, sizeof(he->h_addr));
			the_peer.addr.u.addr4.sin_port = htons((unsigned short)l2tp_port);
			the_peer.addr.u.addr4.sin_family = AF_INET;
		}
		else if(the_peer.addr_type == AF_INET6)
		{
			//use first address in address list
			memcpy(the_peer.addr.u.addr6.sin6_addr.s6_addr , he->h_addr , sizeof(struct in6_addr));
			the_peer.addr.u.addr6.sin6_port = htons((unsigned short)l2tp_port);
			the_peer.addr.u.addr6.sin6_family = AF_INET6;
		}	
	}

	the_peer.secret_len = strlen(l2tp_secret);
	memcpy(the_peer.secret, l2tp_secret, the_peer.secret_len);
	//the_peer.lns_ops = NULL;
	//the_peer.lac_ops = NULL;
	the_peer.hide_avps = l2tp_hide_avps;
	the_peer.validate_peer_ip = 0;

	return 0;
}
