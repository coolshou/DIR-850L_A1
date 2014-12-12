/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pppd.h"

int  pppoe_debug = 0;			/* debuggin mode. */
char pppoe_dev[32] = {0};		/* Specify interface. */
char pppoe_srv_name[64] = {0};	/* desired service name. */
char pppoe_ac_name[64] = {0};	/* desired access concentrator name. */
int  pppoe_hostuniq = 0;		/* use Host-Unique to allow multiple PPPoE sessions. */
int  pppoe_synchronous = 0;		/* use synchronous PPP encapsulation. */
int  pppoe_mss = 0;				/* Clamp MSS to this value. */

static option_t pppoe_options[] =
{
	{ "pppoe_device", o_string, pppoe_dev,
	  "PPPoE device name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
	{ "pppoe_srv_name", o_string, pppoe_srv_name,
	  "PPPoE desired service name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 64 },
	{ "pppoe_ac_name", o_string, pppoe_ac_name,
	  "PPPoE desired access concentrator name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 64 },
	{ "pppoe_hostuniq", o_int, &pppoe_hostuniq,
	  "PPPoE use Host-Unique to allow multiple PPPoE sessions",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pppoe_sync", o_int, &pppoe_synchronous,
	  "PPPoE use synchronous PPP encapsulation",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pppoe_mss", o_int, &pppoe_mss,
	  "Clamp MSS to this value",
	  OPT_PRIO, &pppoe_mss },
	{ "pppoe_debug", o_int, &pppoe_debug,
	  "PPPoE debugging",
	  OPT_INC | OPT_NOARG | 1 },
	{ NULL }
};

void module_pppoe_init(void)
{
	add_options(pppoe_options);
}
