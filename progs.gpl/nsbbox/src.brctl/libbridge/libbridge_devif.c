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
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "libbridge.h"
#include "libbridge_private.h"

int br_device_ioctl32(struct bridge *br, unsigned long arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	unsigned long args[4];
	struct ifreq ifr;

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;

	memcpy(ifr.ifr_name, br->ifname, IFNAMSIZ);
	((unsigned long *)(&ifr.ifr_data))[0] = (unsigned long)args;

	return ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);
}

#ifdef __sparc__
int br_device_ioctl64(struct bridge *br, unsigned long arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	unsigned long long args[4];
	struct ifreq ifr;

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;

	memcpy(ifr.ifr_name, br->ifname, IFNAMSIZ);
	((unsigned long long *)(&ifr.ifr_data))[0] = (unsigned long long)(unsigned long)args;

	return ioctl(br_socket_fd, SIOCDEVPRIVATE + 3, &ifr);
}
#endif

int br_device_ioctl(struct bridge *br, unsigned long arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
#ifdef __sparc__
	if (__kernel_is_64_bit())
		return br_device_ioctl64(br, arg0, arg1, arg2, arg3);
#endif

	return br_device_ioctl32(br, arg0, arg1, arg2, arg3);
}

int br_add_interface(struct bridge *br, int ifindex)
{
	if (br_device_ioctl(br, BRCTL_ADD_IF, ifindex, 0, 0) < 0)
		return errno;

	return 0;
}

int br_del_interface(struct bridge *br, int ifindex)
{
	if (br_device_ioctl(br, BRCTL_DEL_IF, ifindex, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_bridge_forward_delay(struct bridge *br, struct timeval *tv)
{
	unsigned long jif = __tv_to_jiffies(tv);

	if (br_device_ioctl(br, BRCTL_SET_BRIDGE_FORWARD_DELAY,
			    jif, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_bridge_hello_time(struct bridge *br, struct timeval *tv)
{
	unsigned long jif = __tv_to_jiffies(tv);

	if (br_device_ioctl(br, BRCTL_SET_BRIDGE_HELLO_TIME, jif, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_bridge_max_age(struct bridge *br, struct timeval *tv)
{
	unsigned long jif = __tv_to_jiffies(tv);

	if (br_device_ioctl(br, BRCTL_SET_BRIDGE_MAX_AGE, jif, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_ageing_time(struct bridge *br, struct timeval *tv)
{
	unsigned long jif = __tv_to_jiffies(tv);

	if (br_device_ioctl(br, BRCTL_SET_AGEING_TIME, jif, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_gc_interval(struct bridge *br, struct timeval *tv)
{
	unsigned long jif = __tv_to_jiffies(tv);

	if (br_device_ioctl(br, BRCTL_SET_GC_INTERVAL, jif, 0, 0) < 0)
		return errno;

	return 0;
}

int br_set_stp_state(struct bridge *br, int stp_state)
{
	if (br_device_ioctl(br, BRCTL_SET_BRIDGE_STP_STATE, stp_state,
			    0, 0) < 0)
		return errno;

	return 0;
}

int br_set_bridge_priority(struct bridge *br, int bridge_priority)
{
	if (br_device_ioctl(br, BRCTL_SET_BRIDGE_PRIORITY, bridge_priority,
			    0, 0) < 0)
		return errno;

	return 0;
}

int br_set_port_priority(struct port *p, int port_priority)
{
	if (br_device_ioctl(p->parent, BRCTL_SET_PORT_PRIORITY, p->index,
			    port_priority, 0) < 0)
		return errno;

	return 0;
}

int br_set_path_cost(struct port *p, int path_cost)
{
	if (br_device_ioctl(p->parent, BRCTL_SET_PATH_COST, p->index,
			    path_cost, 0) < 0)
		return errno;

	return 0;
}

void __copy_fdb(struct fdb_entry *ent, struct __fdb_entry *f)
{
	memcpy(ent->mac_addr, f->mac_addr, 6);
	ent->port_no = f->port_no;
	ent->is_local = f->is_local;
	__jiffies_to_tv(&ent->ageing_timer_value, f->ageing_timer_value);
}

int br_read_fdb(struct bridge *br, struct fdb_entry *fdbs, int offset, int num)
{
	struct __fdb_entry f[num];
	int i;
	int numread;

	if ((numread = br_device_ioctl(br, BRCTL_GET_FDB_ENTRIES,
				       (unsigned long)f, num, offset)) < 0)
		return errno;

	for (i=0;i<numread;i++)
		__copy_fdb(fdbs+i, f+i);

	return numread;
}

#ifdef CONFIG_NSBBOX_BRCTL_BWCTRL
int br_set_mlticst_bw(struct port * p, int bandwidth)
{
	if (br_device_ioctl(p->parent, 103, p->index, bandwidth, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTFORWARD
int br_set_forward_state(struct port *p, int forward_state)
{
    if (br_device_ioctl(p->parent, 104, p->index, forward_state, 0) < 0) return errno;
    return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTDENYLIST
int br_reject_forward_list(struct port *p0, struct port *p1, int forward_action)
{
	if (br_device_ioctl(p0->parent, 105, p0->index, p1->index, forward_action) < 0) return errno;
	return 0;	
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IGMP
/* IGMP Snooping dennis 2008-01-29 start */
int br_set_igmp_state(struct bridge *br, int igmpenable)
{
	if (br_device_ioctl(br, 110, igmpenable, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_AP_OPERATION_MODE
int br_set_apmode(struct bridge *br, int apmode)
{	
	if (br_device_ioctl(br, 120/*BRCTL_SET_APMODE*/, apmode, 0, 0) < 0)
		return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_APC_OPERATION_MODE
/* AP Client function 2007-01-21 start */
int br_set_apc_state(struct bridge *br, int apc_state)
{	
	if (br_device_ioctl(br, 121/* BRCTL_SET_BRIDGE_APC_STATE */, apc_state, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_DHCP_SERVER_ENABLE
/*dhcp server no provide to eth0's pc 2008-01-23 dennis start */
int br_set_dhcp_server_enable(struct bridge *br, int dhcp_server_enable)
{
	if (br_device_ioctl(br, 160, dhcp_server_enable, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_E_PARTITION
/* e_partition 2008-02-1 start */
int br_set_e_partition_state(struct bridge *br, int e_partition)
{
	if (br_device_ioctl(br, 122, e_partition, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_W_PARTITION
/* wlan_partition, eric, 2008/10/24 */
int br_set_w_partition_state(struct bridge *br, int ifindex, int w_partition)
{
    if(br_device_ioctl(br, 145, ifindex, w_partition, 0) < 0)
        return errno;
    return 0;
}
#endif

#ifdef CONFIG_NSBBOX_BRCTL_ETHLINK
/* ethernet integration dennis2008-02-05 start */
int br_set_ethlink_state(struct bridge *br, int ethlink)
{
	if (br_device_ioctl(br, 123, ethlink, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_LIMITED_ADMIN
/*+++ Limited Administration, Builder, 2008/03/11 +++*/
/*limited administration type*/
int br_set_ladtype(struct bridge *br, int ladtype)
{
	if (br_device_ioctl(br, 126/*LIMITED_ADMIN_TYPE*/, ladtype, 0, 0) < 0) return errno;
	return 0;
}

/*limited administration vlan ID*/
int br_set_ladvid(struct bridge *br, int ladvid)
{
	if (br_device_ioctl(br, 127/*LIMITED_ADMIN_VID*/, ladvid, 0, 0) < 0) return errno;
	return 0;
}

/*limited administration IP start from*/
int br_set_ladippool(struct bridge *br, unsigned int poolidx, unsigned int ladipstart, unsigned int ladipend)
{
	if (br_device_ioctl(br, 128/*LIMITED_ADMIN_IP_POOL*/, poolidx, ladipstart, ladipend) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PING_CONTROL
/*+++ Ping Control, Builder, 2008/4/10 +++*/
int br_set_pingctl(struct bridge *br, unsigned short pingctl)
{
	if (br_device_ioctl(br, 129/*PING_CONTROL*/, pingctl, 0, 0) < 0) return errno;
	return 0; 
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MULTI_VLAN /* Jack add 25/03/08 +++ */
int br_set_pvid(struct bridge *br, int ifindex, unsigned short  vlan_id)
{
	if (br_device_ioctl(br, 130, ifindex, vlan_id, 0) < 0) return errno;
	return 0;
}

int br_set_group_vid(struct bridge *br, int ifindex, unsigned short  vlan_id, int tag)
{
	if (br_device_ioctl(br, 131, ifindex, vlan_id, tag) < 0) return errno;
	return 0;
}

int br_del_group_vid(struct bridge *br, int ifindex, unsigned short  vlan_id)
{
	if (br_device_ioctl(br, 132, ifindex, vlan_id, 0) < 0) return errno;
	return 0;
}

int br_show_vid(struct bridge *br, int ifindex, unsigned short  vlan_id)
{
	if (br_device_ioctl(br, 133, ifindex, 0, 0) < 0) return errno;
	return 0;
}

int br_set_vlan_state(struct bridge *br, unsigned short  vlan_state)
{
	if (br_device_ioctl(br, 134, vlan_state, 0, 0) < 0) return errno;
	return 0;
}

int br_set_system_vid(struct bridge *br, unsigned short  vlan_id)
{
	if (br_device_ioctl(br, 135, vlan_id, 0, 0) < 0) return errno;
	return 0;
}

int br_set_vlan_mode(struct bridge *br, unsigned short  vlan_state)
{
	if (br_device_ioctl(br, 136, vlan_state, 0, 0) < 0) return errno;
	return 0;
}

int br_set_nap_mac_vid_add(struct bridge *br, unsigned long mac0, unsigned long mac1, unsigned short  vlan_id)
{
	if (br_device_ioctl(br, 137, mac0, mac1, vlan_id) < 0) return errno;
	return 0;
}

int br_set_nap_mac_vid_del(struct bridge *br, unsigned long mac0, unsigned long mac1)
{
	if (br_device_ioctl(br, 138, mac0, mac1, 0) < 0) return errno;
	return 0;
}

int br_del_all_group_vid(struct bridge *br, int ifindex)
{
	if (br_device_ioctl(br, 139, ifindex, 0, 0) < 0) return errno;
	return 0;
}
#endif

#ifdef CONFIG_NSBBOX_BRCTL_IOAPNL
int br_set_autobr_port(struct bridge *br, unsigned char *ifname)
{
    if (br_device_ioctl(br, 140, ifname, 0, 0) < 0)
        return errno;
    return 0;
}

int br_set_allowlist(struct bridge *br, unsigned int *allowed_ipaddr)
{
    if (br_device_ioctl(br, 141, allowed_ipaddr, 0, 0) < 0)
        return errno;

    return 0;
}

int br_set_autobridge(struct bridge *br, unsigned int status)
{
    if (br_device_ioctl(br, 142, status, 0, 0) < 0)
        return errno;
    return 0;
}
#endif /*CONFIG_NSBBOX_BRCTL_IOAPNL*/

#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
int br_set_matsta(struct bridge *br, int matsta)
{	
	if (br_device_ioctl(br, 143, matsta, 0, 0) < 0)
		return errno;
	return 0;
}

int br_set_clonetype(struct bridge *br, unsigned int clonetype)
{
	if (br_device_ioctl(br, 144, clonetype, 0, 0) < 0)
		return errno;
	return 0;
}

int br_get_clonetype(struct bridge *br, unsigned int *clonetype)
{
	if (br_device_ioctl(br, 145, clonetype, 0, 0) < 0)
		return errno;
	return 0;
}

int br_set_cloneaddr(struct bridge *br, unsigned char *mac)
{
    unsigned char addrhex[6];
    int i,j;
    for (i=0,j=0; i<6; i++,j+=3){
        addrhex[i] = strtol(&mac[j], NULL, 16);
    }
    if(br_device_ioctl(br, 146, addrhex, 0, 0) < 0){
        return errno;
    }
    return 0;
	return 0;
}

int br_get_cloneaddr(struct bridge *br, unsigned char *mac)
{
    if(br_device_ioctl(br, 147, mac, 0, 0) < 0){
        return errno;
    }
    return 0;
}
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/


#ifdef CONFIG_NSBBOX_BRCTL_FORWARDING_DB
int br_set_flushfdb(struct bridge *br, int ladtype)
{
	if (br_device_ioctl(br, 148, ladtype, 0, 0) < 0) return errno;
	return 0;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MDNSFILTER
int br_set_mdns_filter_state(struct bridge *br, int mdns_filter)
{
	if (br_device_ioctl(br, 149, mdns_filter, 0, 0) < 0) return errno;
	
	return 0;
}
#endif
