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
#include <errno.h>
#include <asm/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <elbox_config.h>

#include <libxmldbc.h>

#include "libbridge.h"
#include "brctl.h"

void br_cmd_addbr(struct bridge *br, char *brname, char *arg1)
{
	int err;

	if ((err = br_add_bridge(brname)) == 0)
		return;

	switch (err) {
	case EEXIST:
		fprintf(stderr,	"device %s already exists; can't create "
			"bridge with the same name\n", brname);
		break;

	default:
		perror("br_add_bridge");
		break;
	}
}

void br_cmd_delbr(struct bridge *br, char *brname, char *arg1)
{
	int err;

	if ((err = br_del_bridge(brname)) == 0)
		return;

	switch (err) {
	case ENXIO:
		fprintf(stderr, "bridge %s doesn't exist; can't delete it\n",
			brname);
		break;

	case EBUSY:
		fprintf(stderr, "bridge %s is still up; can't delete it\n",
			brname);
		break;

	default:
		perror("br_del_bridge");
		break;
	}
}

void br_cmd_addif(struct bridge *br, char *ifname, char *arg1)
{
	int err;
	int ifindex;

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		return;
	}

	if ((err = br_add_interface(br, ifindex)) == 0)
		return;

	switch (err) {
	case EBUSY:
		fprintf(stderr,	"device %s is already a member of a bridge; "
			"can't enslave it to bridge %s.\n", ifname,
			br->ifname);
		break;

	case ELOOP:
		fprintf(stderr, "device %s is a bridge device itself; "
			"can't enslave a bridge device to a bridge device.\n",
			ifname);
		break;

	default:
		perror("br_add_interface");
		break;
	}
}

void br_cmd_delif(struct bridge *br, char *ifname, char *arg1)
{
	int err;
	int ifindex;

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		return;
	}

	if ((err = br_del_interface(br, ifindex)) == 0)
		return;

	switch (err) {
	case EINVAL:
		fprintf(stderr, "device %s is not a slave of %s\n",
			ifname, br->ifname);
		break;

	default:
		perror("br_del_interface");
		break;
	}
}

void br_cmd_setageing(struct bridge *br, char *time, char *arg1)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_ageing_time(br, &tv);
}

void br_cmd_setbridgeprio(struct bridge *br, char *_prio, char *arg1)
{
	int prio;

	sscanf(_prio, "%i", &prio);
	br_set_bridge_priority(br, prio);
}

void br_cmd_setfd(struct bridge *br, char *time, char *arg1)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_forward_delay(br, &tv);
}

void br_cmd_setgcint(struct bridge *br, char *time, char *arg1)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_gc_interval(br, &tv);
}

void br_cmd_sethello(struct bridge *br, char *time, char *arg1)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_hello_time(br, &tv);
}

void br_cmd_setmaxage(struct bridge *br, char *time, char *arg1)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_max_age(br, &tv);
}

void br_cmd_setpathcost(struct bridge *br, char *arg0, char *arg1)
{
	int cost;
	struct port *p;

	if ((p = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	sscanf(arg1, "%i", &cost);
	br_set_path_cost(p, cost);
}

void br_cmd_setportprio(struct bridge *br, char *arg0, char *arg1)
{
	int cost;
	struct port *p;

	if ((p = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	sscanf(arg1, "%i", &cost);
	br_set_port_priority(p, cost);
}

void br_cmd_stp(struct bridge *br, char *arg0, char *arg1)
{
	int stp;

	stp = 0;
	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		stp = 1;

	br_set_stp_state(br, stp);
}

void br_cmd_showstp(struct bridge *br, char *arg0, char *arg1)
{
	br_dump_info(br);
}

void br_cmd_show(struct bridge *br, char *arg0, char *arg1)
{
	printf("bridge name\tbridge id\t\tSTP enabled\tinterfaces\n");
	br = bridge_list;
	while (br != NULL) {
		printf("%s\t\t", br->ifname);
		br_dump_bridge_id((unsigned char *)&br->info.bridge_id);
		printf("\t%s\t\t", br->info.stp_enabled?"yes":"no");
		br_dump_interface_list(br);

		br = br->next;
	}
}

static int compare_fdbs(const void *_f0, const void *_f1)
{
	const struct fdb_entry *f0 = _f0;
	const struct fdb_entry *f1 = _f1;

#if 0
	if (f0->port_no < f1->port_no)
		return -1;

	if (f0->port_no > f1->port_no)
		return 1;
#endif

	return memcmp(f0->mac_addr, f1->mac_addr, 6);
}

void __dump_fdb_entry(struct fdb_entry *f)
{
	printf("%3i\t", f->port_no);
	printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t",
	       f->mac_addr[0], f->mac_addr[1], f->mac_addr[2],
	       f->mac_addr[3], f->mac_addr[4], f->mac_addr[5]);
	printf("%s\t\t", f->is_local?"yes":"no");
	br_show_timer(&f->ageing_timer_value);
	printf("\n");
}

void br_cmd_showmacs(struct bridge *br, char *arg0, char *arg1)
{
	struct fdb_entry fdb[1024];
	int offset;

	printf("port no\tmac addr\t\tis local?\tageing timer\n");

	offset = 0;
	while (1) {
		int i;
		int num;

		num = br_read_fdb(br, fdb, offset, 1024);
		if (!num)
			break;

		qsort(fdb, num, sizeof(struct fdb_entry), compare_fdbs);

#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
#define MACADDRPOOL_CLONE_PATH "/runtime/phyinf:1/macaddrpool/entry"
{
		struct fdb_entry *pfdb;
		unsigned char value[20];
		unsigned char path[50];
		int j=1;
		
		xmldbc_del(NULL, 0, MACADDRPOOL_CLONE_PATH);
		for (i=0;i<num;i++){
		    pfdb = (struct fdb_entry *)(fdb+i);
			__dump_fdb_entry(fdb+i);
			if ((pfdb->port_no==1)&&(pfdb->is_local != 1)){
                memset(path, 0x0, sizeof(path));
                memset(value, 0x0, sizeof(value));
                sprintf(path, "%s:%d/macaddr", MACADDRPOOL_CLONE_PATH, j);
                sprintf(value, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
                    pfdb->mac_addr[0], pfdb->mac_addr[1], pfdb->mac_addr[2],
                    pfdb->mac_addr[3], pfdb->mac_addr[4], pfdb->mac_addr[5]);
                //printf("%s %s\n", path, value);
                xmldbc_set(NULL, 0, path, value);
                j++;
            }
		}
}
#else
		for (i=0;i<num;i++)
			__dump_fdb_entry(fdb+i);
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/

		offset += num;
	}
}

#ifdef CONFIG_NSBBOX_BRCTL_BWCTRL
void br_cmd_setbwctrl(struct bridge * br, char * arg0, char * arg1)
{
	int bandwidth;
	struct port * p;

	if ((p = br_find_port(br, arg0)) == NULL)
	{
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	sscanf(arg1, "%i", &bandwidth);
	br_set_mlticst_bw(p, bandwidth);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTFORWARD
void br_cmd_setfdctrl(struct bridge *br, char *arg0, char *arg1)
{
    int forward;
	struct port *p;

    if ((p = br_find_port(br, arg0)) == NULL)
    {
        fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
        return;
    }

    forward = 1;
    if (!strcmp(arg1, "off") || !strcmp(arg1, "no") || !strcmp(arg1, "0"))
        forward = 0;

    br_set_forward_state(p, forward);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTDENYLIST
void br_cmd_addrejfwlist(struct bridge *br, char *arg0, char *arg1)
{
	struct port *p0, *p1;

	if ((p0 = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	if ((p1 = br_find_port(br, arg1)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg1, br->ifname);
		return;
	}

	br_reject_forward_list(p0, p1, 1);	
	return;
}

void br_cmd_delrejfwlist(struct bridge *br, char *arg0, char *arg1)
{
	struct port *p0, *p1;

	if ((p0 = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	if ((p1 = br_find_port(br, arg1)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg1, br->ifname);
		return;
	}

	br_reject_forward_list(p0, p1, 0);	
	return;
}
void br_cmd_shwrejfwlist(struct bridge *br, char *arg0, char *arg1)
{
	struct port *p0;

	if ((p0 = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return;
	}

	br_reject_forward_list(p0, p0, 2);	
	return;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IGMP
/* IGMP Snooping dennis 2008-01-29 start */
void br_cmd_igmp(struct bridge *br, char *arg0, char *arg1)
{
	int igmpenable = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		igmpenable = 1;
	br_set_igmp_state(br, igmpenable);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_AP_OPERATION_MODE
void br_cmd_apmode(struct bridge *br, char *arg0, char *arg1)
{
	int apmode;

	apmode = atol(arg0);
	br_set_apmode(br, apmode);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_APC_OPERATION_MODE
/* AP Client function 2007-01-21 start */
void br_cmd_apc(struct bridge *br, char *arg0, char *arg1)
{
	int apc = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		apc = 1;
	br_set_apc_state(br, apc);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_DHCP_SERVER_ENABLE
/*dhcp server no provide to eth0's pc 2008-01-23 dennis start */
void br_cmd_dhcp_server_enable(struct bridge *br, char *arg0, char *arg1)
{
	int dhcp_server_enable = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		dhcp_server_enable = 1;
	br_set_dhcp_server_enable(br, dhcp_server_enable);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_E_PARTITION
/* e_partition 2008-02-1 start */
void br_cmd_e_partition(struct bridge *br, char *arg0, char *arg1)
{
	int e_partition = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		e_partition = 1;
	br_set_e_partition_state(br, e_partition);
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MDNSFILTER
/*for mdns filter about the shareport*/
void br_cmd_mdns_filter(struct bridge *br, char *arg0, char *arg1)
{
	int mdns_filter = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		mdns_filter = 1;
	br_set_mdns_filter_state(br, mdns_filter);
}
#endif
#ifdef	CONFIG_NSBBOX_BRCTL_W_PARTITION
/* wlan_partition, eric, 2008/10/24 */
void br_cmd_w_partition(struct bridge *br, char *arg0, char *arg1)
{
	int ifindex, w_partition;

	ifindex = if_nametoindex(arg0);
	if (!ifindex) {
		fprintf(stderr, "interface %s does not exist!\n", arg0);
		return;
	}

	w_partition = 0;

	if (!strcmp(arg1, "on") || !strcmp(arg1, "yes") || !strcmp(arg1, "1"))
		w_partition = 1;

	if(br_set_w_partition_state(br, ifindex, w_partition) != 0){
		perror("set wlan partition state error:");
	}
	return;
}
#endif

#ifdef CONFIG_NSBBOX_BRCTL_ETHLINK
/* ethernet integration dennis2008-02-05 start */
void br_cmd_ethlink(struct bridge *br, char *arg0, char *arg1)
{
	int ethlink = 0;

	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		ethlink = 1;
	br_set_ethlink_state(br, ethlink);
}
#endif


#ifdef CONFIG_NSBBOX_BRCTL_FORWARDING_DB
void br_cmd_flushfdb(struct bridge *br, char *arg0, char *arg1)
{
	br_set_flushfdb(br, atoi(arg0));
}
#endif


#ifdef CONFIG_NSBBOX_BRCTL_LIMITED_ADMIN
/*+++ Limited Administration, Builder, 2008/03/11 +++*/
/*limited administration type*/
void br_cmd_ladtype(struct bridge *br, char *arg0, char *arg1)
{
	int type;

	if		(!strcmp(arg0, "0")) type = 0;	/*Limited Admin Disable*/
	else if	(!strcmp(arg0, "1")) type = 1;	/*Admin with VID*/
	else if	(!strcmp(arg0, "2")) type = 2;	/*Admin with Limited IP*/
	else if	(!strcmp(arg0, "3")) type = 3;	/*Both IP and VID*/
	else						 type = 0;	/*Default: Limited Admin Disabled*/
	br_set_ladtype(br, type);
}

/*limited administration vlan ID*/
void br_cmd_ladvid(struct bridge *br, char *arg0, char *arg1)
{
	br_set_ladvid(br, atoi(arg0));
}

/*limited administration IP pool*/
void br_cmd_ladippool(struct bridge *br, char *arg1, char *arg2, char *arg3)
{
	unsigned int poolidx=0;
	struct in_addr ladipstart, ladipend;

	memset(&ladipstart, 0x0, sizeof(ladipstart));
	memset(&ladipend, 0x0, sizeof(ladipend));
	poolidx = atoi(arg1);
	if ((poolidx<=4)&&(poolidx>=1)) poolidx--;
	if (inet_aton(arg2, &ladipstart)&&inet_aton(arg3, &ladipend))
		br_set_ladippool(br, poolidx, ladipstart.s_addr, ladipend.s_addr);
	return;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PING_CONTROL
/*+++ Ping Control, Builder, 2008/4/10 +++*/
void br_cmd_pingctl(struct bridge *br, char *arg0, char *arg1)
{
	int ctl;

	if		(!strcmp(arg0, "enable"))	ctl = 1;	/*Ping Control: don't accept ping request.*/
	else if	(!strcmp(arg0, "disable"))	ctl = 0;	/*Ping Control: accept ping request.*/
	else								ctl = 0;	/*Default: accept ping request.*/
	br_set_pingctl(br, ctl);
	return;
}
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MULTI_VLAN /* Jack add 25/03/08 +++ */
#define VLAN_ID_MAX			4094
#define VLAN_ID_MIN			1
#define SYSTEM_PORT			"sys"
#define SYSTEM_PORT_IF_NUM	-100

void br_cmd_setvlanstate(struct bridge *br, char *arg0, char *arg1)
{
	if (atoi(arg0) == 1)
	{
	    printf(" enable VLAN \n"); 
	    br_set_vlan_state(br, 1);
    }
    else
    {
	    printf(" disable VLAN \n"); 
        br_set_vlan_state(br, 0);
    }
}    
void br_cmd_setvlanmode(struct bridge *br, char *arg0, char *arg1)
{
	if (atoi(arg0) == 1)
	{
	    printf(" VLAN mode is dynamic. (NAP) \n"); 
	    br_set_vlan_mode(br, 1);
    }
    else
    {
	    printf(" VLAN mode is static. \n"); 
        br_set_vlan_mode(br, 0);
    }
}    

void br_cmd_setsysvid(struct bridge *br, char *arg0, char *arg1)
{
	unsigned short vlan_id;

	vlan_id = atoi(arg0);  
	if (vlan_id >= VLAN_ID_MIN && vlan_id <= VLAN_ID_MAX)
	{
		printf("br_cmd_setsysvid, system vid:%d \n",vlan_id);
		br_set_system_vid(br, vlan_id);
	}
	else
	{
		printf("br_cmd_setsysvid, VID range should be %d-%d \n", VLAN_ID_MIN, VLAN_ID_MAX);
	}
}    

void br_cmd_setpvidif(struct bridge *br, char *ifname, char *arg1)
{
	int ifindex;
	unsigned short vlan_id;

	//printf(" ifname:%s \n",ifname); 
	ifindex = if_nametoindex(ifname);
	if (!ifindex)
	{
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		printf(" ifname:%s does not exist!\n",ifname);
		return;
	}
	vlan_id = atoi(arg1);
	if (vlan_id >= VLAN_ID_MIN && vlan_id <= VLAN_ID_MAX)
	{
		//printf("br_cmd_setpvidif, ifindex:%d pvid:%d \n",ifindex, vlan_id);
		br_set_pvid(br, ifindex, vlan_id);
	}
	else
	{
		printf("!!!br_cmd_setpvid, VID range should be %d-%d \n", VLAN_ID_MIN, VLAN_ID_MAX);
	}
}

void br_cmd_setgroupvidif(struct bridge *br, char *ifname, char *arg1, char *arg2)
{
	int ifindex = 0;
	unsigned short vlan_id;
	int tag, is_system_port=0;

	printf(" ifname:%s \n",ifname);
	if (!strncmp(ifname, SYSTEM_PORT, 3))
	{
		is_system_port=1;
		//printf(" is system port, ");
	}
	else
	{
		ifindex = if_nametoindex(ifname);
		if (!ifindex)
		{
			fprintf(stderr, "interface %s does not exist!\n", ifname);
			printf(" ifname:%s does not exist!\n",ifname);
			return;
		}
	}
	vlan_id = atoi(arg1);
	tag = atoi(arg2);
	if (vlan_id >= VLAN_ID_MIN && vlan_id <= VLAN_ID_MAX)
	{
		if (tag == 1 || tag == 0)
		{
			if (is_system_port==1)
			{
				br_set_group_vid(br, SYSTEM_PORT_IF_NUM, vlan_id, tag);
			}
			else
			{
				printf("br_cmd_setgroupvidif, ifindex:%d pvid:%d, tag:%d \n",ifindex, vlan_id, tag);
				br_set_group_vid(br, ifindex, vlan_id, tag);
			}
		}
		else
		{
			printf("br_cmd_setgroupvidif, tag should be 0 or 1 \n");
		}
	}
	else
	{
		printf("br_cmd_setgroupvidif, VID range should be %d-%d \n", VLAN_ID_MIN, VLAN_ID_MAX);
	}
}

void br_cmd_delgroupvidif(struct bridge *br, char *ifname, char *arg1)
{
	int ifindex = 0, is_system_port=0;	
	unsigned short vlan_id;

	//printf(" ifname:%s \n",ifname);
	if (!strncmp(ifname, SYSTEM_PORT, 3))
	{
		is_system_port=1;
		//printf(" is system port, ");
	}
	else
	{
		ifindex = if_nametoindex(ifname);
		if (!ifindex)
		{
			fprintf(stderr, "interface %s does not exist!\n", ifname);
			printf(" ifname:%s does not exist!\n",ifname);
			return;
		}
	}
	vlan_id = atoi(arg1);
	if (vlan_id >= VLAN_ID_MIN && vlan_id <= VLAN_ID_MAX)
	{
		if (is_system_port==1)
		{
			br_del_group_vid(br, SYSTEM_PORT_IF_NUM, vlan_id);
		}
		else
		{
			printf("br_cmd_delgroupvidif, ifindex:%d pvid:%d \n",ifindex, vlan_id);
			br_del_group_vid(br, ifindex, vlan_id);
		}
	}
	else
	{
		printf("br_cmd_delgroupvidif, VID range should be %d-%d \n", VLAN_ID_MIN, VLAN_ID_MAX);
	}
}

void br_cmd_showvidif(struct bridge *br, char *ifname, char *arg1)
{
	int ifindex;

	//printf(" ifname:%s \n",ifname); 
	ifindex = if_nametoindex(ifname);
	if (!ifindex)
	{
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		printf(" ifname:%s does not exist!\n",ifname);
		return;
	}
	//printf("br_cmd_showvidif, ifindex:%d \n",ifindex);
	br_show_vid(br, ifindex, 0);
}

void br_cmd_setmacnapvidadd(struct bridge *br, char *arg0, char *arg1, char *arg2)
{
	unsigned short vlan_id;
	unsigned long mac0, mac1;

	mac0 = atoi(arg0);
	mac1 = atoi(arg1);
	//printf("br_cmd_wtp_acl_mac_add, mac0:%ld, mac1:%ld, pvid:%d \n", mac0, mac1, vlan_id);
	vlan_id = atoi(arg2);
	if (vlan_id >= VLAN_ID_MIN && vlan_id <= VLAN_ID_MAX)
	{
		br_set_nap_mac_vid_add(br, mac0, mac1, vlan_id);
		//printf("br_cmd_setmacnapvidadd, pvid:%d\n", vlan_id);
	}
	else
	{
		printf("br_cmd_setmacnapvidadd, VID range should be %d-%d \n", VLAN_ID_MIN, VLAN_ID_MAX);
	}
}

void br_cmd_setmacnapviddel(struct bridge *br, char *arg0, char *arg1)
{
	unsigned long mac0, mac1;

	mac0 = atoi(arg0);
	mac1 = atoi(arg1);
	//printf("br_cmd_setmacnapviddel, mac0:%ld, mac1:%ld \n", mac0, mac1);
	br_set_nap_mac_vid_del(br, mac0, mac1);
}

void br_cmd_delallgroupvidif(struct bridge *br, char *ifname, char *arg1)
{
	int ifindex = 0;
	int is_system_port = 0;

	if (!strncmp(ifname, SYSTEM_PORT, 3))
	{
		is_system_port=1;
		//printf(" is system port, ");
	}
	else
	{
		ifindex = if_nametoindex(ifname);
		if (!ifindex)
		{
			fprintf(stderr, "interface %s does not exist!\n", ifname);
			//printf(" ifname:%s does not exist!\n",ifname);
			return;
		}
	}
	br_del_all_group_vid(br, (is_system_port==1) ? SYSTEM_PORT_IF_NUM : ifindex);
}
#endif  /* Jack add 25/03/08 --- */

#ifdef CONFIG_NSBBOX_BRCTL_IOAPNL
void br_cmd_setautobrconf(struct bridge *br, int argc, char *argv[])
{
    int             i;
    struct in_addr  tempip;
    unsigned int    allowed_ipaddr[10];
/*    
    printf("argc: %d\n", argc);
    for(i=0;i<argc;i++)
        printf("%s\n", argv[i]);
*/
    
    if (argc>11){
        printf("Too many parameters!!\n");
        return;
    }
        
    /*Port of bridge*/
    br_set_autobr_port(br, argv[0]);
    
    memset(allowed_ipaddr, 0x0, sizeof(allowed_ipaddr));
    for(i=0;i<argc-1;i++){
        memset(&tempip, 0x0, sizeof(struct in_addr));
        inet_aton(argv[i+1],&tempip);		
        allowed_ipaddr[i] = tempip.s_addr;    
        //printf("%x\n", allowed_ipaddr[i]);
    }
    
	/* Allowed list:                                       */
	/* allowed_ipaddr[0~1]: First two are ipaddr and mask. */
	/* allowed_ipaddr[2~9]: Last eight are allowed list.   */
	br_set_allowlist(br, allowed_ipaddr);

	return;
}

void br_cmd_setautobr(struct bridge *br, char *arg0, char *arg1)
{
    int status=0;

    if (!strcmp(arg0, "enable"))         status = 1;
    else if (!strcmp(arg0, "disable"))   status = 0;
    else                                return;
    br_set_autobridge(br, status);
    return;
}
#endif /*CONFIG_NSBBOX_BRCTL_IOAPNL*/

#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
void br_cmd_matsta(struct bridge *br, char *arg0, char *arg1)
{
	int matsta;

	if (!strcmp(arg0, "enable"))				matsta = 1;
	else if (!strcmp(arg0, "disable"))	matsta = 0;
	else																return;

	br_set_matsta(br, matsta);
}

void br_cmd_setclonetype(struct bridge *br, char *arg0, char *arg1)
{
	int clonetype=0;
	clonetype = atoi(arg0);
	if ((clonetype<0)||(clonetype>2))
		return;
	br_set_clonetype(br, clonetype);
	return;
};

void br_cmd_clonetype(struct bridge *br, char *arg0, char *arg1)
{
	unsigned int clonetype=9;
	br_get_clonetype(br, &clonetype);
	return;
};

void br_cmd_setcloneaddr(struct bridge *br, char *arg0, char *arg1)
{
	br_set_cloneaddr(br, arg0);
	return;
};

#define MACADDR_CLONE_PATH "/runtime/phyinf:2/macclone/current/macaddr"
void br_cmd_cloneaddr(struct bridge *br, char *arg0, char *arg1)
{
	unsigned char mac[6];
	unsigned char value[50];
	
	memset(mac, 0x0, sizeof(mac));
	memset(mac, 0x0, sizeof(value));
	br_get_cloneaddr(br, mac);
	printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf(value, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	xmldbc_del(NULL, 0, MACADDR_CLONE_PATH);
	xmldbc_set(NULL, 0, MACADDR_CLONE_PATH, value);
	
	return;
};
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/

static struct command commands[] =
{
	{0, 1, "addbr",				{br_cmd_addbr}},
	{1, 1, "addif",				{br_cmd_addif}},
	{0, 1, "delbr",				{br_cmd_delbr}},
	{1, 1, "delif",				{br_cmd_delif}},
	{1, 1, "setageing",			{br_cmd_setageing}},
	{1, 1, "setbridgeprio",		{br_cmd_setbridgeprio}},
	{1, 1, "setfd",				{br_cmd_setfd}},
	{1, 1, "setgcint",			{br_cmd_setgcint}},
	{1, 1, "sethello",			{br_cmd_sethello}},
	{1, 1, "setmaxage",			{br_cmd_setmaxage}},
	{1, 2, "setpathcost",		{br_cmd_setpathcost}},
	{1, 2, "setportprio",		{br_cmd_setportprio}},
	{0, 0, "show",				{br_cmd_show}},
	{1, 0, "showmacs",			{br_cmd_showmacs}},
	{1, 0, "showstp",			{br_cmd_showstp}},
	{1, 1, "stp",				{br_cmd_stp}},
#ifdef CONFIG_NSBBOX_BRCTL_BWCTRL
	{1, 2, "setbwctrl",			{br_cmd_setbwctrl}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTFORWARD
	{1, 2, "setfdctrl",			{br_cmd_setfdctrl}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PORTDENYLIST
	{1,	2, "addrejfwlist",		{br_cmd_addrejfwlist}},
	{1, 2, "delrejfwlist",		{br_cmd_delrejfwlist}},
	{1, 1, "shwrejfwlist",		{br_cmd_shwrejfwlist}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IGMP
	{1, 1, "igmp_snooping",		{br_cmd_igmp}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_AP_OPERATION_MODE
	{1, 1, "apmode",			{br_cmd_apmode}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_APC_OPERATION_MODE
	{1, 1, "apc",				{br_cmd_apc}},
#endif	
#ifdef CONFIG_NSBBOX_BRCTL_DHCP_SERVER_ENABLE
	{1, 1, "dhcp_server_enable",{br_cmd_dhcp_server_enable}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_E_PARTITION
	{1, 1, "e_partition",		{br_cmd_e_partition}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MDNSFILTER
	{1, 1, "mdns_filter",		{br_cmd_mdns_filter}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_W_PARTITION
	{1, 2, "w_partition",		{br_cmd_w_partition}},
#endif	
#ifdef CONFIG_NSBBOX_BRCTL_ETHLINK
	{1, 1, "ethlink",			{br_cmd_ethlink}},
#endif	
#ifdef CONFIG_NSBBOX_BRCTL_LIMITED_ADMIN
    {1, 1, "ladtype",			{br_cmd_ladtype}},
    {1, 1, "ladvid",			{br_cmd_ladvid}},
    {1, 3, "ladippool",			{func3:br_cmd_ladippool}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_FORWARDING_DB
    {1, 1, "flushfdb",			{br_cmd_flushfdb}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_PING_CONTROL
    {1, 1, "pingctl",			{br_cmd_pingctl}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_MULTI_VLAN /* Jack add 25/03/08 */
	{1, 1, "setvlanstate",		{br_cmd_setvlanstate}},
	{1, 1, "setvlanmode",		{br_cmd_setvlanmode}},
	{1, 1, "setsysvid",			{br_cmd_setsysvid}},
	{1, 2, "setpvidif",			{br_cmd_setpvidif}},
	{1, 3, "setgroupvidif",		{func3:br_cmd_setgroupvidif}},
	{1, 2, "delgroupvidif",		{br_cmd_delgroupvidif}},
	{1, 1, "showvidif",			{br_cmd_showvidif}},
	{1, 3, "setnapmacvidadd",	{func3:br_cmd_setmacnapvidadd}},
	{1, 2, "setnapmacviddel",	{br_cmd_setmacnapviddel}},
	{1, 1, "delallgroupvidif",	{br_cmd_delallgroupvidif}},
#endif
#ifdef CONFIG_NSBBOX_BRCTL_IOAPNL
    {1, 5, "autobrconf",        {br_cmd_setautobrconf}},
    {1, 1, "autobr",            {br_cmd_setautobr}},
#endif /*CONFIG_NSBBOX_BRCTL_IOAPNL*/
#ifdef CONFIG_NSBBOX_BRCTL_MAT_STATUS
	{1, 1, "matsta",			{br_cmd_matsta}},
	{1, 1, "setclonetype",	{br_cmd_setclonetype}},
	{1, 0, "clonetype",			{br_cmd_clonetype}},
	{1, 1, "setcloneaddr",	{br_cmd_setcloneaddr}},
	{1, 0, "cloneaddr",			{br_cmd_cloneaddr}},
#endif /*CONFIG_NSBBOX_BRCTL_MAT_STATUS*/
};

struct command *br_command_lookup(char *cmd)
{
	int i;
	int numcommands;

	numcommands = sizeof(commands)/sizeof(commands[0]);

	for (i=0;i<numcommands;i++)
		if (!strcmp(cmd, commands[i].name))
			return &commands[i];

	return NULL;
}
