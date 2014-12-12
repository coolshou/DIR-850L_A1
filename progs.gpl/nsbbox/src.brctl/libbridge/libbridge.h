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

#ifndef _LIBBRIDGE_H
#define _LIBBRIDGE_H

#include <net/if.h>
#include <linux/if_bridge.h>
#include <elbox_config.h>

struct bridge;
struct bridge_info;
struct fdb_entry;
struct port;
struct port_info;

struct bridge_id
{
	unsigned char prio[2];
	unsigned char addr[6];
};

struct bridge_info
{
	struct bridge_id designated_root;
	struct bridge_id bridge_id;
	int root_path_cost;
	struct timeval max_age;
	struct timeval hello_time;
	struct timeval forward_delay;
	struct timeval bridge_max_age;
	struct timeval bridge_hello_time;
	struct timeval bridge_forward_delay;
	unsigned topology_change:1;
	unsigned topology_change_detected:1;
	int root_port;
	unsigned stp_enabled:1;
	struct timeval ageing_time;
	struct timeval gc_interval;
	struct timeval hello_timer_value;
	struct timeval tcn_timer_value;
	struct timeval topology_change_timer_value;
	struct timeval gc_timer_value;
};

struct bridge
{
	struct bridge *next;

	int ifindex;
	char ifname[IFNAMSIZ];
	struct port *firstport;
	struct port *ports[256];
	struct bridge_info info;
};

struct fdb_entry
{
	u_int8_t mac_addr[6];
	int port_no;
	unsigned is_local:1;
	struct timeval ageing_timer_value;
};

struct port_info
{
	struct bridge_id designated_root;
	struct bridge_id designated_bridge;
	u_int16_t port_id;
	u_int16_t designated_port;
	int path_cost;
	int designated_cost;
	int state;
	unsigned top_change_ack:1;
	unsigned config_pending:1;
	struct timeval message_age_timer_value;
	struct timeval forward_delay_timer_value;
	struct timeval hold_timer_value;
};

struct port
{
	struct port *next;

	int index;
	int ifindex;
	struct bridge *parent;
	struct port_info info;
};

extern struct bridge *bridge_list;

int br_init(void);
int br_refresh(void);
struct bridge *br_find_bridge(char *brname);
struct port *br_find_port(struct bridge *br, char *portname);
char *br_get_state_name(int state);


int br_add_bridge(char *brname);
int br_del_bridge(char *brname);
int br_add_interface(struct bridge *br, int ifindex);
int br_del_interface(struct bridge *br, int ifindex);
int br_set_bridge_forward_delay(struct bridge *br, struct timeval *tv);
int br_set_bridge_hello_time(struct bridge *br, struct timeval *tv);
int br_set_bridge_max_age(struct bridge *br, struct timeval *tv);
int br_set_ageing_time(struct bridge *br, struct timeval *tv);
int br_set_gc_interval(struct bridge *br, struct timeval *tv);
int br_set_stp_state(struct bridge *br, int stp_state);

int br_set_bridge_priority(struct bridge *br, int bridge_priority);
int br_set_port_priority(struct port *p, int port_priority);
int br_set_path_cost(struct port *p, int path_cost);
int br_read_fdb(struct bridge *br, struct fdb_entry *fdbs, int offset, int num);
#ifdef CONFIG_NSBBOX_BRCTL_BWCTRL
int br_set_mlticst_bw(struct port * p, int bandwidth);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTFORWARD
int br_set_forward_state(struct port * p, int forward_state);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTDENYLIST
int br_reject_forward_list(struct port *p0, struct port *p1, int forward_action);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IGMP	/* IGMP Snooping dennis 2008-01-29 */
int br_set_igmp_state(struct bridge *br, int igmpenable);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_AP_OPERATION_MODE
int br_set_apmode(struct bridge *br, int apmode);
#endif
/* libc5 combatability */
char *if_indextoname(unsigned int __ifindex, char *__ifname);
unsigned int if_nametoindex(const char *__ifname);
#ifdef CONFIG_NSBBOX_BRCTL_APC_OPERATION_MODE /* AP Client function 2007-01-21 start */
int br_set_apc_state(struct bridge *br, int apc_state);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_DHCP_SERVER_ENABLE /*dhcp server no provide to eth0's pc 2008-01-23 dennis start */
int br_set_dhcp_server_enable(struct bridge *br, int e_partition);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_E_PARTITION /* e_partition 2008-02-1 start */
int br_set_e_partition_state(struct bridge *br, int e_partition);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_W_PARTITION
/* wlan_partition, eric, 2008/10/24 */
int br_set_w_partition_state(struct bridge *br, int ifindex, int w_partition);
#endif

#ifdef CONFIG_NSBBOX_BRCTL_ETHLINK /* ethernet integration dennis2008-02-05 start */
int br_set_ethlink_state(struct bridge *br, int ethlink);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_LIMITED_ADMIN /* Limited Administration, Builder, 2008/03/11 */
int br_set_ladtype(struct bridge *br, int ladtype);	/*limited administration type*/
int br_set_ladvid(struct bridge *br, int ladvid);	/*limited administration vlan ID*/
int br_set_ladippool(struct bridge *br, unsigned int poolidx,
	unsigned int ladipstart, unsigned int ladipend);/*limited administration IP start from*/
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PING_CONTROL /*+++ Ping Control, Builder, 2008/4/10 +++*/
int br_set_pingctl(struct bridge *br, unsigned short pingctl);
#endif
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MULTI_VLAN /* Jack add 25/03/08 +++ */
int br_set_pvid(struct bridge *br, int ifindex, unsigned short  vlan_id);
int br_set_group_vid(struct bridge *br, int ifindex, unsigned short  vlan_id, int tag);
int br_del_group_vid(struct bridge *br, int ifindex, unsigned short  vlan_id);
int br_show_vid(struct bridge *br, int ifindex, unsigned short  vlan_id);
int br_set_vlan_state(struct bridge *br, unsigned short  vlan_state);
int br_set_vlan_mode(struct bridge *br, unsigned short  vlan_mode);
int br_set_system_vid(struct bridge *br, unsigned short  vlan_id);
int br_set_nap_mac_vid_add(struct bridge *br, unsigned long mac0, unsigned long mac1, unsigned short  vlan_id);
int br_set_nap_mac_vid_del(struct bridge *br, unsigned long mac0, unsigned long mac1);
int br_del_all_group_vid(struct bridge *br, int ifindex);
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IOAPNL
int br_set_autobr_port(struct bridge *br, unsigned char *ifname);
int br_set_allowlist(struct bridge *br, unsigned int *allowed_ipaddr);
int br_set_autobridge(struct bridge *br, unsigned int status);
#endif /*CONFIG_NSBBOX_BRCTL_IOAPNL*/
#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
int br_set_matsta(struct bridge *br, int matsta);
int br_set_clonetype(struct bridge *br, unsigned int clonetype);
int br_get_clonetype(struct bridge *br, unsigned int *clonetype);
int br_set_cloneaddr(struct bridge *br, unsigned char *mac);
int br_get_cloneaddr(struct bridge *br, unsigned char *mac);
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/

#ifdef CONFIG_NSBBOX_BRCTL_FORWARDING_DB 
int br_set_flushfdb(struct bridge *br, int type);	
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MDNSFILTER
int br_set_mdns_filter_state(struct bridge *br, int mdns_filter);
#endif
