/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pppd.h"

char pptp_server_ip[32] = {0};
int pptp_sync = 0;
char pptp_localbind[32] = {0};

static option_t pptp_options[] =
{
	{ "pptp_server_ip", o_string, pptp_server_ip,
	  "PPTP server ip address",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
	{ "pptp_sync", o_int, &pptp_sync,
	  "Enable Synchronous HDLC (pppd must use it too)",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pptp_localbind", o_string, pptp_localbind,
	  "Bind to specified IP address instead of wildcard",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
	{ NULL }
};

void module_pptp_init(void)
{
	add_options(pptp_options);
}
