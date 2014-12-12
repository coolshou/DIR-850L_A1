/* vi: set sw=4 ts=4: */
/* peer.c */

#include <stddef.h>
#include <string.h>
#include <netdb.h>

#include "l2tp.h"
#include "dtrace.h"



l2tp_peer the_peer;

int l2tp_peer_init(void)
{
	struct hostent *he;

	he = gethostbyname(l2tp_peer_addr);
	if (!he)
	{
		return -1;
	}

	memcpy(&the_peer.addr.sin_addr.s_addr, he->h_addr, sizeof(he->h_addr));
	the_peer.addr.sin_port = htons((unsigned short)l2tp_port);
	the_peer.addr.sin_family = AF_INET;
	the_peer.secret_len = strlen(l2tp_secret);
	memcpy(the_peer.secret, l2tp_secret, the_peer.secret_len);
	//the_peer.lns_ops = NULL;
	//the_peer.lac_ops = NULL;
	the_peer.hide_avps = l2tp_hide_avps;
	the_peer.validate_peer_ip = 0;

	return 0;
}
