/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pppd.h"
#include "dtrace.h"
#include "l2tp.h"

char l2tp_peer_addr[32] = {0};
char l2tp_secret[96+1] = {0};
int l2tp_sync = 0;
int l2tp_port = 1701;
int l2tp_lns = 0;
int l2tp_hide_avps = 0;
int l2tp_kernel_mode = -1;

static option_t l2tp_options[] =
{
	{ "l2tp_peer", o_string, l2tp_peer_addr,
	  "L2TP peer ip address",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
	{ "l2tp_secret", o_string, l2tp_secret,
	  "L2TP shared secret",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 96 },
	{ "l2tp_sync", o_int, &l2tp_sync,
	  "L2TP use synchronous PPP encapsulation",
	  OPT_INC | OPT_NOARG | 1 },
	{ "l2tp_port", o_int, &l2tp_port,
	  "L2TP port number (default is 1701)",
	  OPT_PRIO },
	{ "l2tp_lns", o_int, &l2tp_lns,
	  "Be LNS",
	  OPT_INC | OPT_NOARG | 1 },
	{ "l2tp_hide_avps", o_int, &l2tp_hide_avps,
	  "Hide AVPs with this peer",
	  OPT_INC | OPT_NOARG | 1 },
	{ "kernel-mode", o_int, &l2tp_kernel_mode},
	{ NULL }
};

void module_l2tp_init(void)
{
	add_options(l2tp_options);
	pppol2tp_init_network();
}
