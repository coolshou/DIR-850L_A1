/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pppd.h"
#include "dtrace.h"

int  kpppoe_debug = 0;			/* debug mode. */
char kpppoe_dev[32] = {0};		/* Specify interface */
char kpppoe_srv_name[64] = {0};	/* desired service name */
char kpppoe_ac_name[64] = {0};	/* desired access concentrator name. */
int  kpppoe_hostuniq = 0;
int  kpppoe_synchronous = 0;
int  kpppoe_mss = 0;

static option_t kpppoe_options[] =
{
	{ "pppoe_device", o_string, kpppoe_dev,
	  "PPPoE device name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 32 },
	{ "pppoe_srv_name", o_string, kpppoe_srv_name,
	  "PPPoE desired service name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 64 },
	{ "pppoe_ac_name", o_string, kpppoe_ac_name,
	  "PPPoE desired access concentrator name",
	  OPT_PRIO | OPT_PRIV | OPT_STATIC, NULL, 64 },
	{ "pppoe_hostuniq", o_int, &kpppoe_hostuniq,
	  "PPPoE use Host-Unique to allow multiple PPPoE sessions",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pppoe_sync", o_int, &kpppoe_synchronous,
	  "PPPoE use synchronous PPP encapsulation",
	  OPT_INC | OPT_NOARG | 1 },
	{ "pppoe_mss", o_int, &kpppoe_mss,
	  "Clamp MSS to this value",
	  OPT_PRIO, &kpppoe_mss },
	{ NULL }
};

extern int PPPOEInitDevice(void);

void module_kpppoe_init(void)
{
	d_dbg("module_kpppoe_init >>>\n");
	add_options(kpppoe_options);
	PPPOEInitDevice();
	d_dbg("module_kpppoe_init <<<\n");
}
