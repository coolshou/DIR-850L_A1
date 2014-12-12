/*
 * Copyright (C) 2000 Lennert Buytenhek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      
#include <sys/time.h>
#include <asm/param.h>
#include "libbridge.h"
#include "brctl.h"

char *help_message =
"commands:\n"
"\taddbr\t\t<bridge>\t\tadd bridge\n"
"\taddif\t\t<bridge> <device>\tadd interface to bridge\n"
"\tdelbr\t\t<bridge>\t\tdelete bridge\n"
"\tdelif\t\t<bridge> <device>\tdelete interface from bridge\n"
"\tshow\t\t\t\t\tshow a list of bridges\n"
"\tshowmacs\t<bridge>\t\tshow a list of mac addrs\n"
"\tshowstp\t\t<bridge>\t\tshow bridge stp info\n"
"\n"
"\tsetageing\t<bridge> <time>\t\tset ageing time\n"
"\tsetbridgeprio\t<bridge> <prio>\t\tset bridge priority\n"
"\tsetfd\t\t<bridge> <time>\t\tset bridge forward delay\n"
"\tsetgcint\t<bridge> <time>\t\tset garbage collection interval\n"
"\tsethello\t<bridge> <time>\t\tset hello time\n"
"\tsetmaxage\t<bridge> <time>\t\tset max message age\n"
"\tsetpathcost\t<bridge> <port> <cost>\tset path cost\n"
"\tsetportprio\t<bridge> <port> <prio>\tset port priority\n"
"\tstp\t\t<bridge> <state>\tturn stp on/off\n"
#ifdef CONFIG_NSBBOX_BRCTL_BWCTRL
"\tsetbwctrl\t<bridge> <port> <bdwh>\tset multicast bandwidth (kbps)\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTFORWARD
"\tsetfdctrl\t<bridge> <port> <state>\tturn port forwarding on/off\n"
#endif 
#ifdef CONFIG_NSBBOX_BRCTL_PORTDENYLIST
"\taddrejfwlist\t<bridge> <port1> <port2>add port2 to deny list of port1\n"
"\tdelrejfwlist\t<bridge> <port1> <port2>del port2 from deny list of port1\n"
"\tshwrejfwlist\t<bridge> <port>\t\tshow deny list of port\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IGMP
"\tigmp_snooping\t\t<bridge> <state>\tturn igmp snooping on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_AP_OPERATION_MODE
"\tapmode\t\t<bridge> <state>\tpass AP operation mode to bridge\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_APC_OPERATION_MODE
"\tapc\t\t<bridge> <state>\tturn AP Clint mode on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_DHCP_SERVER_ENABLE
"\tdhcp_server_enable\t\t<bridge> <state>\tturn dhcp_server on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_E_PARTITION
"\te_partition\t\t<bridge> <state>\tturn e_partition on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MDNSFILTER
"\tmdns_filter\t\t<bridge> <state>\tturn mdns_filter on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_W_PARTITION
"\tw_partition\t\t<bridge> <device> <state>\tturn wlan partition on/off on device/interface\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_ETHLINK
"\tethlink\t\t<bridge> <state>\tturn ethlink on/off\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_LIMITED_ADMIN
"\tladtype\t\t<bridge> <type>\t\tlimited administration type\n"
"\t\t\t\t\t\t0: Limited Adminstrator Disabled\n"
"\t\t\t\t\t\t1: Administrate with VID\n"
"\t\t\t\t\t\t2: Administrate with Limited IP\n"
"\t\t\t\t\t\t3: Both\n"
"\tladvid\t\t<bridge> <vlan id>\tadministrator's vlan ID\n"
"\tladippool\t<bridge> <pool idx> <start ip> <end ip>\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_FORWARDING_DB
"\tflushfdb\t<bridge> <state>\tflush bridge forwarding database\n"
#endif

#ifdef CONFIG_NSBBOX_BRCTL_PING_CONTROL
"\tpingctl\t\t<bridge> <enable/disable>\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MULTI_VLAN  /* Jack add 31/03/08 +++ */
"\tsetvlanstate\t\t<bridge> <state> \tset pvid on device/interface\n"
"\tsetvlanmode\t\t<bridge> <state> \tset pvid on device/interface\n"
"\tsetpvidif\t\t<bridge> <device> <pvid>\tset pvid on device/interface\n"
"\tsetgroupvidif\t\t<bridge> <device> <group vid> <tag>\tset group vid on device/interface\n"
"\tdelgroupvidif\t\t<bridge> <device> <group vid>\tdelete group vid on device/interface\n"
"\tshowvidif\t\t<bridge> <device> \tshow vid on device/interface\n"
"\tsetsysvid\t\t<bridge> <vlan id> \tset system VID\n"
"\tsetnapmacvidadd\t\t<bridge> <mac0> <mac1> <vlan id>\tadd a VID for a MAC address in NAP\n"
"\tsetnapmacviddel\t\t<bridge> <mac0> <mac1>\tdel a VID for a MAC address in NAP\n"
"\tdelallgroupvidif\t\t<bridge> <device> \tdelete all group vid on device/interface\n"
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IOAPNL
"\tautobrconf\t<bridge> <port> <ipaddr> <mask> <ip1~ip8>\tset allowed list\n"
"\tautobr\t\t<bridge> <status>\tenable/disable\n"
#endif /*CONFIG_NSBBOX_BRCTL_IOAPNL*/
#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
"\tmatsta\t\t<bridge> <state> \tMAC address translator status(enable/disable)\n"
"\tsetclonetype\t<bridge> <state> \t0:disable/1:auto/2:manual\n"
"\tclonetype\t<bridge>\n"
"\tsetcloneaddr\t<bridge> <MAC address>\n"
"\tcloneaddr\t<bridge>\n"
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/
;

void help()
{
	fprintf(stderr, help_message);
}

#ifdef NSBBOX
int brctl_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	int argindex;
	struct bridge *br;
	struct command *cmd;

	br_init();

	if (argc < 2)
		goto help;

	if ((cmd = br_command_lookup(argv[1])) == NULL) {
		fprintf(stderr, "never heard of command [%s]\n", argv[1]);
		goto help;
	}

	argindex = 2;
	br = NULL;
	if (cmd->needs_bridge_argument) {
		if (argindex >= argc) {
			fprintf(stderr, "this option requires a bridge name as argument\n");
			return 1;
		}

		br = br_find_bridge(argv[argindex]);

		if (br == NULL) {
			fprintf(stderr, "bridge %s doesn't exist!\n", argv[argindex]);
			return 1;
		}

		argindex++;
	}

	if (argc - argindex != cmd->num_string_arguments) {
		fprintf(stderr, "incorrect number of arguments for command\n");
		//return 1;
	}

	if (cmd->num_string_arguments == 3)
		cmd->func3(br, argv[argindex], argv[argindex+1], argv[argindex+2]);
	else if (cmd->num_string_arguments > 3)
	    cmd->func_main(br, argc-3, &argv[argindex]);
	else
		cmd->func(br, argv[argindex], argv[argindex+1]);

	return 0;

help:
	help();
	return 1;
}
