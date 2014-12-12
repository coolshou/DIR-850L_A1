/* vi: set sw=4 ts=4: */
/* dhcpc.h */
#ifndef _DHCPC_H
#define _DHCPC_H

#include "libbb_udhcp.h"
uint32_t get_uptime(void);

#define INIT_SELECTING	0
#define REQUESTING		1
#define BOUND			2
#define RENEWING		3
#define REBINDING		4
#define INIT_REBOOT		5
#define RENEW_REQUESTED	6
#define RELEASED		7
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
#define ENTERPRISE_CODE 171
#endif
struct client_config_t
{
	char foreground;			/* Do not fork */
	char quit_after_lease;		/* Quit after obtaining lease */
	char abort_if_no_lease;		/* Abort if no lease */
	char background_if_no_lease;/* Fork to background if no lease */
	char *interface;			/* The name of the interface to use */
	char *pidfile;				/* Optionally store the process ID */
	char *script;				/* User script to run at dhcp events */
	unsigned char *clientid;	/* Optional client id to use */
	unsigned char *hostname;	/* Optional hostname to use */
	int ifindex;				/* Index number of the interface to use */
	unsigned char arp[6];		/* Our arp address */
	int discover_delay;			/* initial delay time. */
	int discover_retry;			/* max discover retry count */
	int discover_sleep;			/* sleep time when discover failed. */
};

extern struct client_config_t client_config;
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
extern int got_ACs_IP;
extern char str_ACs_IP[];
#endif
#endif
