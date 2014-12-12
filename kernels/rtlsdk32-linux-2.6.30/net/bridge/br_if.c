/*
 *	Userspace interface
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/if_arp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <net/sock.h>

#include "br_private.h"

#if defined (CONFIG_RTL_IGMP_SNOOPING)
#include <net/rtl/rtl865x_igmpsnooping_glue.h>
#include <net/rtl/rtl865x_igmpsnooping.h>
#include <net/rtl/rtl865x_netif.h>
#include <net/rtl/rtl_nic.h>
struct net_bridge *bridge0=NULL;
unsigned int brIgmpModuleIndex=0xFFFFFFFF;
unsigned int br0SwFwdPortMask=0xFFFFFFFF;
extern void br_mCastQueryTimerExpired(unsigned long arg);
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
struct net_bridge *bridge1=NULL;
unsigned int brIgmpModuleIndex_2=0xFFFFFFFF;
unsigned int br1SwFwdPortMask=0xFFFFFFFF;
#endif
#endif

#if defined (CONFIG_RTL_STP)
#include <net/rtl/rtk_stp.h>
#endif

#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
int rtl865x_generateBridgeDeviceInfo( struct net_bridge *br, rtl_multicastDeviceInfo_t *devInfo);
#endif

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
typedef void (*add_member_cb)(unsigned char *, unsigned char *);
typedef void (*del_member_cb)(unsigned char *,unsigned char *);
typedef void (*clear_group_cb)(unsigned char *);
typedef void (*snoop_init_cb)(void);
typedef void (*snoop_deinit_cb)(void);
add_member_cb add_member=NULL;
del_member_cb del_member=NULL;
clear_group_cb clear_group=NULL;
snoop_init_cb snoop_init=NULL;
snoop_deinit_cb snoop_deinit=NULL;

void register_igmp_callbacks(add_member_cb fun1, del_member_cb fun2, clear_group_cb fun3)
{
	add_member = fun1;
	del_member = fun2;
	clear_group = fun3;	
}

void unregister_igmp_callbacks()
{	
	add_member = NULL;
	del_member = NULL;
	clear_group = NULL;		
}

void register_snoop_init_callback (snoop_init_cb funa,snoop_deinit_cb funb)
{
	snoop_init = funa;
	snoop_deinit = funb;
}	

void unregister_snoop_init_callback()
{
	snoop_init = NULL;
	snoop_deinit =NULL;
}

EXPORT_SYMBOL(register_snoop_init_callback);
EXPORT_SYMBOL(unregister_snoop_init_callback);
EXPORT_SYMBOL(register_igmp_callbacks);
EXPORT_SYMBOL(unregister_igmp_callbacks);
#endif
/*
 * Determine initial path cost based on speed.
 * using recommendations from 802.1d standard
 *
 * Since driver might sleep need to not be holding any locks.
 */
static int port_cost(struct net_device *dev)
{
	if (dev->ethtool_ops && dev->ethtool_ops->get_settings) {
		struct ethtool_cmd ecmd = { .cmd = ETHTOOL_GSET, };

		if (!dev->ethtool_ops->get_settings(dev, &ecmd)) {
			switch(ecmd.speed) {
			case SPEED_10000:
				return 2;
			case SPEED_1000:
				return 4;
			case SPEED_100:
				return 19;
			case SPEED_10:
				return 100;
			}
		}
	}

	/* Old silly heuristics based on name */
	if (!strncmp(dev->name, "lec", 3))
		return 7;

	if (!strncmp(dev->name, "plip", 4))
		return 2500;

	return 100;	/* assume old 10Mbps */
}

#if defined(CONFIG_RTK_MESH) && defined(STP_ADDCOST_ETH)
int br_initial_port_cost(struct net_device *dev)
{
	return port_cost(dev);
}
#endif

/*
 * Check for port carrier transistions.
 * Called from work queue to allow for calling functions that
 * might sleep (such as speed check), and to debounce.
 */
void br_port_carrier_check(struct net_bridge_port *p)
{
	struct net_device *dev = p->dev;
	struct net_bridge *br = p->br;

	if (netif_carrier_ok(dev))
		p->path_cost = port_cost(dev);

	if (netif_running(br->dev)) {
		spin_lock_bh(&br->lock);
		if (netif_carrier_ok(dev)) {
			if (p->state == BR_STATE_DISABLED)
				br_stp_enable_port(p);
		} else {
			if (p->state != BR_STATE_DISABLED)
				br_stp_disable_port(p);
		}
		spin_unlock_bh(&br->lock);
	}
}

static void release_nbp(struct kobject *kobj)
{
	struct net_bridge_port *p
		= container_of(kobj, struct net_bridge_port, kobj);
	kfree(p);
}

static struct kobj_type brport_ktype = {
#ifdef CONFIG_SYSFS
	.sysfs_ops = &brport_sysfs_ops,
#endif
	.release = release_nbp,
};

static void destroy_nbp(struct net_bridge_port *p)
{
	struct net_device *dev = p->dev;

	p->br = NULL;
	p->dev = NULL;
	dev_put(dev);

	kobject_put(&p->kobj);
}

static void destroy_nbp_rcu(struct rcu_head *head)
{
	struct net_bridge_port *p =
			container_of(head, struct net_bridge_port, rcu);
	destroy_nbp(p);
}

/* Delete port(interface) from bridge is done in two steps.
 * via RCU. First step, marks device as down. That deletes
 * all the timers and stops new packets from flowing through.
 *
 * Final cleanup doesn't occur until after all CPU's finished
 * processing packets.
 *
 * Protected from multiple admin operations by RTNL mutex
 */
static void del_nbp(struct net_bridge_port *p)
{
	struct net_bridge *br = p->br;
	struct net_device *dev = p->dev;

#if 0
#ifdef STP_DISABLE_ETH
	//Chris: stp+mesh
	p->disable_by_mesh = 0;
#endif
#endif 

	sysfs_remove_link(br->ifobj, dev->name);

	dev_set_promiscuity(dev, -1);

	spin_lock_bh(&br->lock);
	br_stp_disable_port(p);
	spin_unlock_bh(&br->lock);

	br_ifinfo_notify(RTM_DELLINK, p);

	br_fdb_delete_by_port(br, p, 1);

	list_del_rcu(&p->list);

	rcu_assign_pointer(dev->br_port, NULL);

	kobject_uevent(&p->kobj, KOBJ_REMOVE);
	kobject_del(&p->kobj);

	call_rcu(&p->rcu, destroy_nbp_rcu);
}

/* called with RTNL */
static void del_br(struct net_bridge *br)
{
	struct net_bridge_port *p, *n;

	list_for_each_entry_safe(p, n, &br->port_list, list) {
		del_nbp(p);
	}

	del_timer_sync(&br->gc_timer);

	br_sysfs_delbr(br->dev);
	unregister_netdevice(br->dev);
}

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
static int check_mac(char *s)
{
	char * accept = MAC_ACCEPT_CHAR;
	if(!s || !(*s))	return (-1);
	if ( strlen(s) == strspn(s, accept) )
		return 0;
	return (-1);
}
/* search device'name that matched */
/* called under bridge lock */
static struct net_bridge_port * search_device(struct net_bridge * br, char* name)
{
	struct net_bridge_port *p;
	list_for_each_entry(p, &br->port_list, list) {
		if (strcmp(p->dev->name, name) == 0){
			return p;
		}
	}
	return NULL;
}
static uint8_t * hex2dec(char *ch)
{
	if ( *ch >= '0' && *ch <= '9') *ch=*ch-'0';
	else if ( *ch >= 'A' && *ch <= 'F')  *ch=*ch-'A' +10;
	else if ( *ch >= 'a' && *ch <= 'f')  *ch=*ch-'a' +10;
	return ch;
}
static void split_MAC(unsigned char * mac_addr, char * token)
{
	char *macDelim = MAC_DELIM;
	char **pMAC = &token;
	char * macField_char[6];
	int i;
	for (i=0; i<6; ++i)
	{
		int j;
		char temp[2];
		macField_char[i] = strsep(pMAC, macDelim);
		/* copy each char byte and convert to dec number */
		for(j=0; j<2; ++j) {
			memcpy(&temp[j],macField_char[i]+j, sizeof(char));
			hex2dec(&temp[j]);
		}

		temp[0] = temp[0] << 4;
		*(mac_addr + i)= (temp[0]^temp[1]);
	}
}

/* called under bridge lock */
static int table_setsnoop(struct net_bridge *br, int type)
{
	switch(type)
	{
		case ENABLE :
			br->snooping = 1;
			if(snoop_init) 
				snoop_init();
			else 
			{
				printk("No snooping implementation. Please check !! \n");
				return (-1);
			}
			break;
		case DISABLE : 
			br->snooping = 0;
			if(snoop_deinit) 
				snoop_deinit();
			else
			{
				printk("No snooping implementation. Please check !! \n");
				return (-1);
			}
			break;
	}
	return 0;
}


/* set wireless identifier */
/* called under bridge lock */
static int table_setwl(struct net_bridge *br, char * name, int type)
{
	struct net_bridge_port *hit_port;
	hit_port = search_device(br, name);
	if (hit_port != NULL){
		if(type==ENABLE)
			atomic_set(&hit_port->wireless_interface, 1);
		else 
			atomic_set(&hit_port->wireless_interface, 0);
		return 0;
	}else
		return (-1);
}

static void table_add(  struct net_bridge_port *p, unsigned char * group_addr, unsigned char * member_addr)
{
	int found=0;
	unsigned long flags;
	spinlock_t lock;
	struct port_group_mac *pgentry;
	struct port_member_mac *pcentry;
	
	if(group_addr==NULL || member_addr==NULL)
		return;	

	spin_lock_irqsave(&lock,flags);

	//1. find old group if exist
	list_for_each_entry(pgentry, &p->igmp_group_list, list)
	{
		if(!memcmp(pgentry->grpmac,group_addr, 6))
		{
			found = 1;
			break;
		}
	}

	if(!found)	//create new group
	{
		pgentry = (struct port_group_mac *)kmalloc(sizeof(struct port_group_mac), GFP_ATOMIC);
		INIT_LIST_HEAD(&pgentry->list);
		INIT_LIST_HEAD(&pgentry->member_list);
		list_add(&pgentry->list, &p->igmp_group_list);
		memcpy(pgentry->grpmac , group_addr , 6);
		print_debug("brg : Create new group 0x%02x%02x%02x%02x%02x%02x\n", 
			group_addr[0],group_addr[1],group_addr[2],group_addr[3],group_addr[4],group_addr[5]);
	}
	//2. find old client mac if exist
	found = 0;
	list_for_each_entry(pcentry, &pgentry->member_list, list)
	{
		if(!memcmp(pcentry->member_mac,member_addr, 6))
		{	/* member already exist, do nothing ~*/
			found = 1;
			break;
		}
	}
	if(!found)
	{	/* member NOT exist, create NEW ONE and attached it to this group-mac linked list ~*/
		pcentry	= (struct port_member_mac *)kmalloc(sizeof(struct port_member_mac), GFP_ATOMIC);
		INIT_LIST_HEAD(&pcentry->list);
		list_add(&pcentry->list, &pgentry->member_list);
		memcpy( pcentry->member_mac ,member_addr , 6);
		print_debug("brg : Added client mac 0x%02x%02x%02x%02x%02x%02x to group 0x%02x%02x%02x%02x%02x%02x\n", 
			pcentry->member_mac[0],pcentry->member_mac[1],pcentry->member_mac[2],pcentry->member_mac[3],pcentry->member_mac[4],pcentry->member_mac[5],
			pgentry->grpmac[0],pgentry->grpmac[1],pgentry->grpmac[2],pgentry->grpmac[3],pgentry->grpmac[4],pgentry->grpmac[5]
		);
	}
	spin_unlock_irqrestore (&lock, flags);
}

/*
1. find old group 
2. find old client mac 
3. if group is empty, delete group
4. snooping : update the group port list 
*/
static void table_remove(struct net_bridge_port *p, unsigned char * group_addr, unsigned char * member_addr)
{
	struct port_group_mac *pgentry;
	struct port_member_mac *pcentry;
	int found = 0;
	
	//0. sanity check
	if(group_addr==NULL || member_addr==NULL)
		return;

	//1. find old group 
	list_for_each_entry(pgentry, &p->igmp_group_list, list)
	{
		if(!memcmp(pgentry->grpmac,group_addr, 6))
		{
			found = 1;
			break;
		}
	}
	if(!found)
	{
		print_debug("dbg : Can't delete 0x%02x%02x%02x%02x%02x%02x, group NOT FOUND.\n", 
			group_addr[0],group_addr[1],group_addr[2],group_addr[3],group_addr[4],group_addr[5] );
		return;
	}

	//2. find old client mac 
	found = 0;
	list_for_each_entry(pcentry, &pgentry->member_list, list)
	{
		if(!memcmp(pcentry->member_mac,member_addr, 6))
		{	
			found = 1;
			break;
		}
	}

	if(found)
	{
		/* member to be deleted FOUND, DELETE IT ! */
		list_del(&pcentry->list);
		kfree(pcentry);
		print_debug("dbg : Delete client 0x%02x%02x%02x%02x%02x%02x in group 0x%02x%02x%02x%02x%02x%02x\n", 
			member_addr[0],member_addr[1],member_addr[2],member_addr[3],member_addr[4],member_addr[5],
			group_addr[0],group_addr[1],group_addr[2],group_addr[3],group_addr[4],group_addr[5] );	
	}else
	{	/* do nothing, just debug */
		print_debug("dbg : Can't delete client 0x%02x%02x%02x%02x%02x%02x, client NOT FOUND in group 0x%02x%02x%02x%02x%02x%02x\n", 
			member_addr[0],member_addr[1],member_addr[2],member_addr[3],member_addr[4],member_addr[5],
			group_addr[0],group_addr[1],group_addr[2],group_addr[3],group_addr[4],group_addr[5] );
	}

	//3. if group is empty, delete group
	if(list_empty(&pgentry->member_list))
	{
		list_del(&pgentry->member_list);
		list_del(&pgentry->list);
		kfree(pgentry);
		//remove group mac from port_list
		print_debug("dbg : Delete group 0x%02x%02x%02x%02x%02x%02x since its empty \n", 
			group_addr[0],group_addr[1],group_addr[2],group_addr[3],group_addr[4],group_addr[5] );	
		return;
	}
}

static int proc_read_alpha_multicast (char *buf, char **start, off_t offset,
								int len, int *eof, void *data)
{
	int count =0;
	struct net_bridge *br = (struct net_bridge *) data;
	struct net_bridge_port *p;
	struct port_group_mac *pgentry;
	struct port_member_mac *pmentry;	

	spin_lock_bh(&br->lock); // bridge lock
	printk( "**********************************************************************\n");
	printk( "* bridge name    : %s\n",br->dev->name);
	printk( "* snooping         : %d\n",br->snooping);
	printk( "**********************************************************************\n");
	list_for_each_entry_rcu(p, &br->port_list, list) {
		printk( "* ==============================================================\n");
		printk( "* <%d> port name : %s\n", p->port_no, p->dev->name);
		printk( "* <%d> wireless_interface -> %d\n", p->port_no, atomic_read(&p->wireless_interface) );
		
		//traverse through all group list, list all the members inside 
		list_for_each_entry(pgentry, &p->igmp_group_list, list)
		{
			printk(" Group Mac  0x%02x%02x%02x%02x%02x%02x\n",pgentry->grpmac[0],
															pgentry->grpmac[1],
															pgentry->grpmac[2],
															pgentry->grpmac[3],
															pgentry->grpmac[4],
															pgentry->grpmac[5]);

			list_for_each_entry(pmentry, &pgentry->member_list, list)
			{
				printk("   membermac 0x%02x%02x%02x%02x%02x%02x\n",pmentry->member_mac[0],
															pmentry->member_mac[1],
															pmentry->member_mac[2],
															pmentry->member_mac[3],
															pmentry->member_mac[4],
															pmentry->member_mac[5]);
			}
		}
		printk( "* ==============================================================\n");
	} // list_for_each_entry_rcu() - END
	printk( "**********************************************************************\n");
	spin_unlock_bh(&br->lock); // bridge unlock

	*eof = 1;
	return count;
}

static int proc_write_alpha_multicast (struct file *file, const char *buf,
								unsigned long count, void *data)
{
	int len = MESSAGE_LENGTH+1;
	char message[len];
	char *msgDelim = MESSAGE_DELIM;
	char *pmesg;
	char *action_token, *action;
	struct net_bridge *br;

	if(count > MESSAGE_LENGTH) {len = MESSAGE_LENGTH;}
	else {len = count; }
	if(copy_from_user(message, buf, len))
		return -EFAULT;
	message[len-1] = '\0';

	/* split input message that get from user space
	 * token[0] => action token --> add or remove
	 * token[1] => multicast group mac address
	 * token[2] => member MAC address of host
	 */
	pmesg = message ;

	action_token = strsep(&pmesg, msgDelim);

	br = (struct net_bridge *) data;

	/* ============================  set wireless enhance =====================*/
	action = ACTION_SET_ENHANCE;
	if (memcmp(action_token, action, sizeof(ACTION_SET_ENHANCE) )== 0){
		spin_lock_bh(&br->lock); // bridge lock
		if (table_setwl(br,pmesg, ENABLE) != 0){
			print_debug("[BR_IGMPP_PROC]->WARNING SETWL FAILURE-> %s\n",pmesg);
		}
		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out
		goto proc_write_br_igmpp_out;
	}

	/* ============================  unset wireless enhance  ===================*/
	action = ACTION_UNSET_ENHANCE;
	if ( memcmp(action_token, action, sizeof(ACTION_UNSET_ENHANCE) )== 0){
		spin_lock_bh(&br->lock); // bridge lock
		if (table_setwl(br,pmesg, DISABLE) != 0){
			print_debug("[BR_IGMPP_PROC]->WARNING SETWL FAILURE-> %s\n",pmesg);
		}
		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out
		goto proc_write_br_igmpp_out;
	}

	/* ============================  set snooping ============================*/
	action = ACTION_SET_SNOOPING;
	if (memcmp(action_token, action, sizeof(ACTION_SET_SNOOPING) )== 0){
		spin_lock_bh(&br->lock); // bridge lock
		if (table_setsnoop(br, ENABLE) != 0){
			print_debug("[BR_IGMPP_PROC]->WARNING SET snooping FAILURE-> %s\n",pmesg);
		}
		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out
		goto proc_write_br_igmpp_out;
	}

	/* ============================  unset snooping ==========================*/
	action = ACTION_UNSET_SNOOPING;
	if ( memcmp(action_token, action, sizeof(ACTION_UNSET_SNOOPING) )== 0){
		spin_lock_bh(&br->lock); // bridge lock
		if (table_setsnoop(br, DISABLE) != 0){
			print_debug("[BR_IGMPP_PROC]->WARNING UNSET snooping FAILURE-> %s\n",pmesg);
		}
		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out
		goto proc_write_br_igmpp_out;
	}

	/* ============================  add - START =====================================*/
	action = ACTION_ADD;
	if ( memcmp(action_token, action, sizeof(ACTION_ADD) )== 0){
		/********** add - START of processing input string **********/
		char *token[2]={0,0};
		int i;
		unsigned char mac_addr[6];
		unsigned char grp_mac_addr[6];	
		struct net_bridge_fdb_entry *hit_fdb_entry;

		for(i=0; i<2; ++i)
			token[i] = strsep(&pmesg, msgDelim);

		/* Only accept MAC, split host MAC address */
		if ( check_mac(token[0]) == -1 || check_mac(token[1]) == -1)
		{
			print_debug("[BR_IGMPP_PROC]-> Host MAC address: %s,%s is illegal !!\n",
				(token[0])?(token[0]):"null",
				(token[1])?(token[1]):"null");
			goto proc_write_br_igmpp_out;
		}

		spin_lock_bh(&br->lock); // bridge lock
		split_MAC(grp_mac_addr, token[0]);
		split_MAC(mac_addr, token[1]);
		
		print_debug("brg : group 0x%02x%02x%02x%02x%02x%02x member 0x%02x%02x%02x%02x%02x%02x\n", 
			grp_mac_addr[0],grp_mac_addr[1],grp_mac_addr[2],grp_mac_addr[3],grp_mac_addr[4],grp_mac_addr[5],
			mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);

		/* searching bridge_fdb_entry */
		hit_fdb_entry = __br_fdb_get(br, mac_addr);
		/* NOTE: The effect of successful called br_fdb_get() also takes lock bridge and reference counts. */

		if (hit_fdb_entry != NULL)
		{
			table_add(hit_fdb_entry->dst, grp_mac_addr, mac_addr);
			fdb_delete(hit_fdb_entry); // release br_fdb_get() locks
		}
		else
		{
			print_debug("The return value of __br_fdb_get() is NULL -> MAC: %X:%X:%X:%X:%X:%X \n",
				mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5] );
		}

		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out
		//do snoop if implemented in switch
		if(add_member) add_member(grp_mac_addr,mac_addr);
		
		goto proc_write_br_igmpp_out;
	}


	action = ACTION_REMOVE;
	if ( memcmp(action_token, action, sizeof(ACTION_REMOVE) ) == 0)
	{
		char *token[2]={0,0};
		int i;
		unsigned char mac_addr[6];
		struct net_bridge_fdb_entry *hit_fdb_entry;
		unsigned char grp_mac_addr[6];

		for(i=0; i<2; ++i)
			token[i] = strsep(&pmesg, msgDelim);

		/* Only accept MAC, split host MAC address */
		if (check_mac(token[0]) == -1 || check_mac(token[1]) == -1)
		{
			print_debug("[BR_IGMPP_PROC]-> Host MAC address: %s,%s is illegal !!\n",
				(token[0])?(token[0]):"null",
				(token[1])?(token[1]):"null");
			goto proc_write_br_igmpp_out;
		}

		spin_lock_bh(&br->lock); // bridge lock			
		split_MAC(grp_mac_addr, token[0]);
		split_MAC(mac_addr, token[1]);
		
		print_debug("brg : group 0x%02x%02x%02x%02x%02x%02x member 0x%02x%02x%02x%02x%02x%02x\n", 
			grp_mac_addr[0],grp_mac_addr[1],grp_mac_addr[2],grp_mac_addr[3],grp_mac_addr[4],grp_mac_addr[5],
			mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);

		/* searching bridge_fdb_entry */
		hit_fdb_entry = __br_fdb_get(br, mac_addr);
		/* NOTE: The effect of successful called __br_fdb_get() also takes lock bridge and reference counts. */

		if (hit_fdb_entry != NULL)
		{
			table_remove(hit_fdb_entry->dst, grp_mac_addr, mac_addr);
			fdb_delete(hit_fdb_entry); // release br_fdb_get() locks
		}
		else
		{
			print_debug("The return value of __br_fdb_get() is NULL -> MAC: %X:%X:%X:%X:%X:%X \n",
				mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5] );
		}
		spin_unlock_bh(&br->lock); // bridge unlock for goto proc_write_br_igmpp_out

		//do snoop if implemented in switch
		if(del_member) del_member(grp_mac_addr, mac_addr);
		
		goto proc_write_br_igmpp_out;
	}
	/* ============================= remove - END ======================================*/

	proc_write_br_igmpp_out:
	return len;
}
#endif

#if defined (CONFIG_RTK_MESH) || defined (CONFIG_RTL_IGMP_SNOOPING)
struct net_bridge *find_br_by_name(char *name)
{
	struct net_bridge *br;
	struct net_device *dev =NULL;;

	dev = __dev_get_by_name(&init_net, name);
	if (dev == NULL) 
	{
		return NULL;

	}	
	else if (!(dev->priv_flags & IFF_EBRIDGE)) {
		
		return NULL;
	}
	else 
	{
		br = netdev_priv(dev);
	}
	return br;
}


#endif

static struct net_device *new_bridge_dev(struct net *net, const char *name)
{
	struct net_bridge *br;
	struct net_device *dev;

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	char alpha_proc_name[32];
#endif
	dev = alloc_netdev(sizeof(struct net_bridge), name,
			   br_dev_setup);

	if (!dev)
		return NULL;
	dev_net_set(dev, net);

	br = netdev_priv(dev);
	br->dev = dev;

	spin_lock_init(&br->lock);
	INIT_LIST_HEAD(&br->port_list);
	spin_lock_init(&br->hash_lock);

	br->bridge_id.prio[0] = 0x80;
	br->bridge_id.prio[1] = 0x00;

	memcpy(br->group_addr, br_group_address, ETH_ALEN);

	br->feature_mask = dev->features;
	br->stp_enabled = BR_NO_STP;
	br->designated_root = br->bridge_id;
	br->root_path_cost = 0;
	br->root_port = 0;
	br->bridge_max_age = br->max_age = 20 * HZ;
	br->bridge_hello_time = br->hello_time = 2 * HZ;
#if defined(CONFIG_RTL_819X)
	br->bridge_forward_delay = br->forward_delay = 10 * HZ;
#else
	br->bridge_forward_delay = br->forward_delay = 15 * HZ;
#endif
	br->topology_change = 0;
	br->topology_change_detected = 0;
	br->ageing_time = 300 * HZ;
//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	snprintf(alpha_proc_name, sizeof(alpha_proc_name), "alpha/multicast_%s", name);
	br->alpha_multicast_proc = create_proc_entry (alpha_proc_name, 0644, NULL);
	if (br->alpha_multicast_proc == NULL)
	{
		printk("create  proc FAILED %s\n", name);
		return ERR_PTR(-ENOMEM);
	}	
	br->alpha_multicast_proc->data = (void*)br;
	br->alpha_multicast_proc->read_proc = proc_read_alpha_multicast;
	br->alpha_multicast_proc->write_proc = proc_write_alpha_multicast;
	br->snooping = 0;
#endif
//Alpha
#ifdef CONFIG_BRIDGE_E_PARTITION_BWCTRL
	br->e_partition = 0;
#endif
//Alpha
#ifdef CONFIG_BRIDGE_MDNSFILTER_BWCTRL
	br->mdns_filter= 0;
#endif

#if defined (CONFIG_RTK_MESH)
	br->eth0_monitor_interval = MESH_PORTAL_EXPIRE * HZ;
	br->mesh_pathsel_pid=0;
#if defined (STP_ADDCOST_ETH)
	br->is_cost_changed = 0;
#endif
#endif

	br_netfilter_rtable_init(br);

#if defined (CONFIG_RTL_IGMP_SNOOPING)
	br->igmpProxy_pid=0;
#endif
	INIT_LIST_HEAD(&br->age_list);

	br_stp_timer_init(br);

	return dev;
}

/* find an available port number */
static int find_portno(struct net_bridge *br)
{
	int index;
	struct net_bridge_port *p;
	unsigned long *inuse;

	inuse = kcalloc(BITS_TO_LONGS(BR_MAX_PORTS), sizeof(unsigned long),
			GFP_KERNEL);
	if (!inuse)
		return -ENOMEM;

	set_bit(0, inuse);	/* zero is reserved */
	list_for_each_entry(p, &br->port_list, list) {
		set_bit(p->port_no, inuse);
	}
	index = find_first_zero_bit(inuse, BR_MAX_PORTS);
	kfree(inuse);

	return (index >= BR_MAX_PORTS) ? -EXFULL : index;
}

/* called with RTNL but without bridge lock */
static struct net_bridge_port *new_nbp(struct net_bridge *br,
				       struct net_device *dev)
{
	int index;
	struct net_bridge_port *p;
#if defined (CONFIG_RTL_STP)
	int retval=0, Port;
	char name[IFNAMSIZ];
	struct net_device *pseudo_dev;
#endif
#if defined(CONFIG_RTL_HW_STP)
	int retval, i;
	uint32 vid, portMask;
#endif
	index = find_portno(br);
	if (index < 0)
		return ERR_PTR(index);

	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (p == NULL)
		return ERR_PTR(-ENOMEM);

	p->br = br;
	dev_hold(dev);
	p->dev = dev;
	p->path_cost = port_cost(dev);
	p->priority = 0x8000 >> BR_PORT_BITS;
	p->port_no = index;
	br_init_port(p);
	p->state = BR_STATE_DISABLED;

//Chris: stp+mesh
#if 0
 #if defined STP_DISABLE_ETH
	p->disable_by_mesh = 0;
 #endif
#endif

#if defined (CONFIG_RTL_STP)
	{
		strcpy(name, p->dev->name);
		Port=STP_PortDev_Mapping[name[strlen(name)-1]-'0'];
		#ifdef CONFIG_RTK_MESH
		if ((Port != WLAN_PSEUDO_IF_INDEX) && (Port !=WLAN_MESH_PSEUDO_IF_INDEX) && (Port!=NO_MAPPING))
		#else
		if ((Port != WLAN_PSEUDO_IF_INDEX) && (Port!=NO_MAPPING))
		#endif
		{
			retval = rtl865x_setMulticastSpanningTreePortState(Port , RTL8651_PORTSTA_DISABLED);

			retval = rtl865x_setSpanningTreePortState(Port, RTL8651_PORTSTA_DISABLED);
		}
		else  if (Port == WLAN_PSEUDO_IF_INDEX)
		{
			if ((pseudo_dev = __dev_get_by_name(&init_net, WLAN_IF_NAME)) == NULL)
			{	
				return NULL;
			}
			pseudo_dev->br_port->state = BR_STATE_DISABLED;
			retval = SUCCESS;
		}
		#ifdef CONFIG_RTK_MESH
		else  if (Port == WLAN_MESH_PSEUDO_IF_INDEX)
		{
			if ((pseudo_dev = __dev_get_by_name(&init_net, WLAN_MESH_IF_NAME)) == NULL)
			{	
				return NULL;
			}
			pseudo_dev->br_port->state = BR_STATE_DISABLED;
			retval = SUCCESS;
		}
		#endif
		else if(Port == NO_MAPPING)
		{
			p->state = BR_STATE_DISABLED;
		}
	}
	
#endif

#if defined(CONFIG_RTL_HW_STP)
	vid=0;
	portMask=0;

	#if define(CONFIG_RTL_NETIF_MAPPING)
	{
		ps_drv_netif_mapping_t *entry;
		entry = rtl_get_ps_drv_netif_mapping_by_psdev(p->dev);

		retval = rtl865x_getNetifVid(entry?entry->drvName:p->dev->name,&vid);
	}
	#else
	if(strcmp(p->dev->name,"eth0")==0)
		retval=rtl865x_getNetifVid("br0", &vid);
	else
		retval=rtl865x_getNetifVid(p->dev->name, &vid);
	#endif
	
	if(retval==FAILED){
//		printk("%s(%d): rtl865x_getNetifVid failed.\n",__FUNCTION__,__LINE__);
	}
	else{
		portMask=rtl865x_getVlanPortMask(vid);
		for ( i = 0 ; i < MAX_RTL_STP_PORT_WH; i ++ ){
			if((1<<i)&portMask){
				retval = rtl865x_setMulticastSpanningTreePortState(i , BR_STATE_DISABLED);
				if(retval==FAILED)
					printk("%s(%d): rtl865x_setMulticastSpanningTreePortState port(%d) failed.\n",__FUNCTION__,__LINE__,i);
				
				retval = rtl865x_setSpanningTreePortState(i, BR_STATE_DISABLED);
				if(retval==FAILED)
					printk("%s(%d): rtl865x_setSpanningTreePortState port(%d) failed.\n",__FUNCTION__,__LINE__,i);
			}
		}
	}
#endif

	br_stp_port_timer_init(p);

	return p;
}

#if defined (CONFIG_RTK_MESH)
/*notice:this function is different from kernel 2.4 sdk*/
int br_set_meshpathsel_pid(int pid)
{
	struct net_device *dev=NULL;
	struct net_bridge *br=NULL;
	
	int ret=0;

	dev = __dev_get_by_name(&init_net,RTL_PS_BR0_DEV_NAME);
	if (dev == NULL) 
	{
		ret =  -ENXIO; 	/* Could not find device */

	}	
	else if (!(dev->priv_flags & IFF_EBRIDGE)) {
		
		ret = -EPERM;
	}
	else 
	{
		br = netdev_priv(dev);
	}

	br->mesh_pathsel_pid = pid;

	if( !pid )
		br->eth0_received = 0;

	printk("Receive Pathsel daemon pid:%d\n",br->mesh_pathsel_pid);
	return 1;
}
#endif

#if defined (CONFIG_RTL_IGMP_SNOOPING)
int br_set_igmpProxy_pid(int pid)
{
	struct net_device *dev;
	struct net_bridge *br=NULL;
	int ret=0;

	dev = __dev_get_by_name(&init_net,RTL_PS_BR0_DEV_NAME);
	if (dev == NULL) 
	{
		ret =  -ENXIO; 	/* Could not find device */

	}	
	else if (!(dev->priv_flags & IFF_EBRIDGE)) {
		
		ret = -EPERM;
	}
	else 
	{
		br = netdev_priv(dev);
	}
	
	if(br!=NULL)
	{
		br->igmpProxy_pid= pid;
	}

	return 1;
}
#endif

int br_add_bridge(struct net *net, const char *name)
{
	struct net_device *dev;
	int ret;
	
	dev = new_bridge_dev(net, name);
	if (!dev)
		return -ENOMEM;

	rtnl_lock();
	if (strchr(dev->name, '%')) {
		ret = dev_alloc_name(dev, dev->name);
		if (ret < 0)
			goto out_free;
	}

	ret = register_netdevice(dev);
	if (ret)
		goto out_free;

	ret = br_sysfs_addbr(dev);
	if (ret)
		unregister_netdevice(dev);

#if defined (CONFIG_RTL_IGMP_SNOOPING)
	if(strcmp(name,RTL_PS_BR0_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t devInfo;
		memset(&devInfo, 0, sizeof(rtl_multicastDeviceInfo_t ));
		strcpy(devInfo.devName, RTL_PS_BR0_DEV_NAME);
		
		ret=rtl_registerIgmpSnoopingModule(&brIgmpModuleIndex);
		#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
		if(ret==0)
		{
			 rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex,&devInfo);
		}
		#endif
		bridge0=netdev_priv(dev);
		if(bridge0!=NULL)
		{
			init_timer(&bridge0->mCastQuerytimer);
			bridge0->mCastQuerytimer.data=bridge0;
			bridge0->mCastQuerytimer.expires=jiffies+MCAST_QUERY_INTERVAL*HZ;
			bridge0->mCastQuerytimer.function=(void*)br_mCastQueryTimerExpired;
			add_timer(&bridge0->mCastQuerytimer);
		}
		
		rtl_setIpv4UnknownMCastFloodMap(brIgmpModuleIndex, 0xFFFFFFFF);
		rtl_setIpv6UnknownMCastFloodMap(brIgmpModuleIndex, 0xFFFFFFFF);
	}
    #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(name,RTL_PS_BR1_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t devInfo2;
		memset(&devInfo2, 0, sizeof(rtl_multicastDeviceInfo_t ));
		strcpy(devInfo2.devName, RTL_PS_BR1_DEV_NAME);
		
		ret=rtl_registerIgmpSnoopingModule(&brIgmpModuleIndex_2);
		#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
		if(ret==0)
		{
			 rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex_2,&devInfo2);
		}
		#endif
		bridge1=netdev_priv(dev);
		if(bridge1!=NULL)
		{
			init_timer(&bridge1->mCastQuerytimer);
			bridge1->mCastQuerytimer.data=bridge1;
			bridge1->mCastQuerytimer.expires=jiffies+MCAST_QUERY_INTERVAL*HZ;
			bridge1->mCastQuerytimer.function=(void*)br_mCastQueryTimerExpired;
			add_timer(&bridge1->mCastQuerytimer);
		}
	}
#endif
#endif
 out:
	rtnl_unlock();
	return ret;

out_free:
	free_netdev(dev);
	goto out;
}

int br_del_bridge(struct net *net, const char *name)
{
	struct net_device *dev;
	int ret = 0;

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	char alpha_proc_name[32];
#endif
	rtnl_lock();
	dev = __dev_get_by_name(net, name);
	if (dev == NULL)
		ret =  -ENXIO; 	/* Could not find device */

	else if (!(dev->priv_flags & IFF_EBRIDGE)) {
		/* Attempt to delete non bridge device! */
		ret = -EPERM;
	}

	else if (dev->flags & IFF_UP) {
		/* Not shutdown yet. */
		ret = -EBUSY;
	}

	else
		del_br(netdev_priv(dev));

	#if defined (CONFIG_RTL_IGMP_SNOOPING)
	if(strcmp(name,RTL_PS_BR0_DEV_NAME)==0)
	{
		rtl_unregisterIgmpSnoopingModule(brIgmpModuleIndex);
		#if defined (CONFIG_RTL_MLD_SNOOPING)
		if(bridge0 && timer_pending(&bridge0->mCastQuerytimer))
		{
			del_timer(&bridge0->mCastQuerytimer);
		}
		#endif
		bridge0=NULL;
	}
    #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(name,RTL_PS_BR1_DEV_NAME)==0)
	{
		rtl_unregisterIgmpSnoopingModule(brIgmpModuleIndex_2);
		#if defined (CONFIG_RTL_MLD_SNOOPING)
		if(bridge1 && timer_pending(&bridge1->mCastQuerytimer))
		{
			del_timer(&bridge1->mCastQuerytimer);
		}
		#endif
		bridge1=NULL;
	}
	#endif
	#endif

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	print_debug("remove proc entry %s\n", name);
	snprintf(alpha_proc_name, sizeof(alpha_proc_name), "alpha/multicast_%s", name);
	remove_proc_entry(alpha_proc_name, 0);
#endif
	rtnl_unlock();
	return ret;
}

/* MTU of the bridge pseudo-device: ETH_DATA_LEN or the minimum of the ports */
int br_min_mtu(const struct net_bridge *br)
{
	const struct net_bridge_port *p;
	int mtu = 0;

	ASSERT_RTNL();

	if (list_empty(&br->port_list))
		mtu = ETH_DATA_LEN;
	else {
		list_for_each_entry(p, &br->port_list, list) {
			if (!mtu  || p->dev->mtu < mtu)
				mtu = p->dev->mtu;
		}
	}
	return mtu;
}

/*
 * Recomputes features using slave's features
 */
void br_features_recompute(struct net_bridge *br)
{
	struct net_bridge_port *p;
	unsigned long features, mask;

	features = mask = br->feature_mask;
	if (list_empty(&br->port_list))
		goto done;

	features &= ~NETIF_F_ONE_FOR_ALL;

	list_for_each_entry(p, &br->port_list, list) {
		features = netdev_increment_features(features,
						     p->dev->features, mask);
	}

done:
	br->dev->features = netdev_fix_features(features, NULL);
}

/* called with RTNL */
int br_add_if(struct net_bridge *br, struct net_device *dev)
{
	struct net_bridge_port *p;
	int err = 0;
	
	if (dev->flags & IFF_LOOPBACK || dev->type != ARPHRD_ETHER)
		return -EINVAL;

	if (dev->netdev_ops->ndo_start_xmit == br_dev_xmit)
		return -ELOOP;

	if (dev->br_port != NULL)
		return -EBUSY;

	p = new_nbp(br, dev);
	if (IS_ERR(p))
		return PTR_ERR(p);

	err = dev_set_promiscuity(dev, 1);
	if (err)
		goto put_back;

	err = kobject_init_and_add(&p->kobj, &brport_ktype, &(dev->dev.kobj),
				   SYSFS_BRIDGE_PORT_ATTR);
	if (err)
		goto err0;

	err = br_fdb_insert(br, p, dev->dev_addr);
	if (err)
		goto err1;

	err = br_sysfs_addif(p);
	if (err)
		goto err2;

	rcu_assign_pointer(dev->br_port, p);
	dev_disable_lro(dev);

	list_add_rcu(&p->list, &br->port_list);

	spin_lock_bh(&br->lock);
//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	INIT_LIST_HEAD(&p->igmp_group_list);	
	atomic_set(&p->wireless_interface, 0);
#endif

	br_stp_recalculate_bridge_id(br);
	br_features_recompute(br);

	if ((dev->flags & IFF_UP) && netif_carrier_ok(dev) &&
	    (br->dev->flags & IFF_UP))
		br_stp_enable_port(p);
	spin_unlock_bh(&br->lock);

	br_ifinfo_notify(RTM_NEWLINK, p);

	dev_set_mtu(br->dev, br_min_mtu(br));

	kobject_uevent(&p->kobj, KOBJ_ADD);

	#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t brDevInfo;
		rtl865x_generateBridgeDeviceInfo(br, &brDevInfo);
		if(brIgmpModuleIndex!=0xFFFFFFFF)
		{
			rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex,&brDevInfo);
		}
		br0SwFwdPortMask=brDevInfo.swPortMask;
	}
	#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t brDevInfo;
		rtl865x_generateBridgeDeviceInfo(br, &brDevInfo);
		if(brIgmpModuleIndex!=0xFFFFFFFF)
		{
			rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex_2,&brDevInfo);
		}
		br1SwFwdPortMask=brDevInfo.swPortMask;
	}
	#endif
	#endif

	return 0;
err2:
	br_fdb_delete_by_port(br, p, 1);
err1:
	kobject_del(&p->kobj);
err0:
	dev_set_promiscuity(dev, -1);
put_back:
	dev_put(dev);
	kfree(p);
	return err;
}

/* called with RTNL */
int br_del_if(struct net_bridge *br, struct net_device *dev)
{
	struct net_bridge_port *p = dev->br_port;

	if (!p || p->br != br)
		return -EINVAL;

	del_nbp(p);

	spin_lock_bh(&br->lock);
	br_stp_recalculate_bridge_id(br);
	br_features_recompute(br);
	spin_unlock_bh(&br->lock);

	#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t brDevInfo;
		rtl865x_generateBridgeDeviceInfo(br, &brDevInfo);
		if(brIgmpModuleIndex!=0xFFFFFFFF)
		{
			rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex,&brDevInfo);
		}
		br0SwFwdPortMask=brDevInfo.swPortMask;
	}
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)==0)
	{
		rtl_multicastDeviceInfo_t brDevInfo;
		rtl865x_generateBridgeDeviceInfo(br, &brDevInfo);
		if(brIgmpModuleIndex!=0xFFFFFFFF)
		{
			rtl_setIgmpSnoopingModuleDevInfo(brIgmpModuleIndex_2,&brDevInfo);
		}
		br1SwFwdPortMask=brDevInfo.swPortMask;
	}
#endif
	#endif

	return 0;
}

void br_net_exit(struct net *net)
{
	struct net_device *dev;

	rtnl_lock();
restart:
	for_each_netdev(net, dev) {
		if (dev->priv_flags & IFF_EBRIDGE) {
			del_br(netdev_priv(dev));
			goto restart;
		}
	}
	rtnl_unlock();

}

#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
int rtl865x_generateBridgeDeviceInfo( struct net_bridge *br, rtl_multicastDeviceInfo_t *devInfo)
{
	struct net_bridge_port *p, *n;
	
	if((br==NULL) || (devInfo==NULL))
	{
		return -1;
	}
	
	memset(devInfo, 0, sizeof(rtl_multicastDeviceInfo_t));

#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)!=0 && strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)!=0 )
#else
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)!=0)
#endif
	{
		return -1;
	}

	strcpy(devInfo->devName,br->dev->name);
	
		
	list_for_each_entry_safe(p, n, &br->port_list, list) 
	{
		if(memcmp(p->dev->name, RTL_PS_ETH_NAME,3)!=0)
		{
			devInfo->swPortMask|=(1<<p->port_no);
		}
		devInfo->portMask|=(1<<p->port_no);
		
	}
	
	return 0;
}
#endif
