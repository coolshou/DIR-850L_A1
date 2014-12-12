/*
 *	Forwarding decision
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
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"
#if defined (CONFIG_RTL_IGMP_SNOOPING)
#include <linux/ip.h>
#include <linux/in.h>
#if defined (CONFIG_RTL_MLD_SNOOPING)
#include <linux/ipv6.h>
#include <linux/in6.h>
#endif
#include <linux/igmp.h>
#include <net/checksum.h>
#include <net/rtl/rtl865x_igmpsnooping_glue.h>
#include <net/rtl/rtl865x_igmpsnooping.h>
#include <net/rtl/rtl865x_netif.h>
#include <net/rtl/rtl_nic.h>
extern int igmpsnoopenabled;
extern unsigned int brIgmpModuleIndex;
extern unsigned int br0SwFwdPortMask;
#if defined (CONFIG_RTL_MLD_SNOOPING)
extern int mldSnoopEnabled;
#endif
#endif

#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
extern unsigned int brIgmpModuleIndex_2;
extern unsigned int br1SwFwdPortMask;
extern struct net_bridge *bridge1;
#endif

#if	defined(CONFIG_RTL_819X)
#include <net/rtl/features/rtl_ps_hooks.h>
#endif
#include <net/rtl/features/rtl_ps_log.h>

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
unsigned char bcast_mac_addr[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#endif

//Alpha
#ifdef CONFIG_BRIDGE_E_PARTITION_BWCTRL
#define ETHERNET_INTERFACE  "eth1"  // the e_partition_deliver function will drop the arp pkg from ETHERNET_INTERFACE to wlan0
static int e_partition_deliver(const struct net_bridge_port *p,
				 const struct sk_buff *skb)
{
	struct net_bridge *br = p->br;
	unsigned char * dest;
	dest = eth_hdr(skb)->h_dest;
	struct iphdr e_partition_iph;
	struct iphdr *e_partition_ih; //add by yuda
	
	unsigned short eth_protocol = eth_hdr(skb)->h_proto;
	if((br->e_partition==1) &&(dest[0]==0xFF))
	{
		if(!(strcmp(skb->dev->name,ETHERNET_INTERFACE))){
			if(ntohs(eth_protocol)==0x0800) 
			{
				e_partition_ih = skb_header_pointer(skb, 0, sizeof(e_partition_iph), &e_partition_iph);
				if(e_partition_ih->protocol==0x11) //protocol is UDP
				{
					struct udphdr _ports, *pptr;
					pptr = skb_header_pointer(skb, e_partition_ih->ihl*4,sizeof(_ports), &_ports);
					if(pptr->dest==0x43 || pptr->dest==0x44) //pass bootps and bootpc by yuda
					{
					}
					else
					{
						return 0;
					}
				}
				else
				{
					return 0;
				}
			}
			/*pass DHCP packet by yuda end*/
			else
			{
			//other pack(arp packet) so drop it
			     return 0;
			}
		}
	}	
	/*block multicast packet if e_partition = 1 by yuda start*/
	else if(br->e_partition==1 && (dest[0]==0x01) && (dest[1]==0x00) && (dest[2]==0x5E) && (dest[3]<=0x7F))
	{
		if(!(strcmp(skb->dev->name,"eth0")))
		{
			return 0;
		}
	}
	/*block multicast packet if e_partition = 1 by yuda end*/
	return 1;
 }
#endif

/* Don't forward packets to originating port or forwarding diasabled */
static inline int should_deliver(
//Alpha
#ifdef CONFIG_BRIDGE_MULTICAST_BWCTRL
				 struct net_bridge_port *p,
#else
				 const struct net_bridge_port *p,
#endif
				 const struct sk_buff *skb)
{
//Alpha
#ifdef CONFIG_BRIDGE_MULTICAST_BWCTRL
	unsigned char * dest;
	struct ethhdr *eth = eth_hdr(skb);
#endif
	if (skb->dev == p->dev ||
	    p->state != BR_STATE_FORWARDING)
		return 0;

#if defined(CONFIG_RTK_VLAN_NEW_FEATURE)
		if (skb->src_info) {
			struct vlan_info_item *pitem = rtl_get_vlan_info_item_by_dev(p->dev);

			/* index == 1, it means skb is cloned skb in rx_vlan_process */
			if (skb->src_info->index) {
				if (pitem && pitem->info.forwarding_rule!=1)
					return 0;
			}

			/* vlan_br can't send packet to vlan_nat */
			if (skb->src_info->forwarding_rule==1) {
				if (pitem && pitem->info.forwarding_rule==2)
					return 0;
			}

			/* vlan_nat can't send packet to vlan_br */
			if (skb->src_info->forwarding_rule==2) {
				if (pitem && pitem->info.forwarding_rule==1)
					return 0;
			}
		}
#endif
//Alpha
#ifdef CONFIG_BRIDGE_MULTICAST_BWCTRL
	dest = eth->h_dest;
	/* Bouble- 20100514 - 
	  * Original code will limit bandwidth on broadcast and multicast 
	  * Current code will limit on multicase only
	  * Another, for CPU overhead consideration, will not use memncmp(),
	  * just only partial compare with dest[0] only
	  */
	//if ((dest[0] & 1) && p->bandwidth !=0) {
	if ((dest[0] & 1) && (dest[0] != 0xFF) && p->bandwidth !=0) {
		if ((p->accumulation + skb->len) > p->bandwidth) 
			return 0;
		p->accumulation += skb->len;
	}
#endif
//Alpha
#ifdef CONFIG_BRIDGE_E_PARTITION_BWCTRL
	if(!e_partition_deliver(p, skb))
                return 0;
#endif
	return 1;
}

static inline unsigned packet_length(const struct sk_buff *skb)
{
	return skb->len - (skb->protocol == htons(ETH_P_8021Q) ? VLAN_HLEN : 0);
}

int br_dev_queue_push_xmit(struct sk_buff *skb)
{
	/* drop mtu oversized packets except gso */
	if (packet_length(skb) > skb->dev->mtu && !skb_is_gso(skb))
		kfree_skb(skb);
	else {
		/* ip_refrag calls ip_fragment, doesn't copy the MAC header. */
		if (nf_bridge_maybe_copy_header(skb))
			kfree_skb(skb);
		else {
			skb_push(skb, ETH_HLEN);
			#if	defined(CONFIG_RTL_819X)
			rtl_br_dev_queue_push_xmit_before_xmit_hooks(skb);
			#endif
			dev_queue_xmit(skb);
		}
	}

	return 0;
}

int br_forward_finish(struct sk_buff *skb)
{
	return NF_HOOK(PF_BRIDGE, NF_BR_POST_ROUTING, skb, NULL, skb->dev,
		       br_dev_queue_push_xmit);

}

static void __br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
	skb->dev = to->dev;
	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_OUT, skb, NULL, skb->dev,
			br_forward_finish);
}

static void __br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	struct net_device *indev;

	if (skb_warn_if_lro(skb)) {
		kfree_skb(skb);
		return;
	}

	indev = skb->dev;
	skb->dev = to->dev;
	skb_forward_csum(skb);

	NF_HOOK(PF_BRIDGE, NF_BR_FORWARD, skb, indev, skb->dev,
			br_forward_finish);
}

/* called with rcu_read_lock */
void br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_deliver(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called with rcu_read_lock */
void br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_forward(to, skb);
		return;
	}

	kfree_skb(skb);
}

#ifdef CONFIG_RTK_INBAND_HOST_HACK
#define ETH_P_RTK_NOTIFY 0x9000 //mark_issue
#define ETH_P_RTK_NOTIFY1 0x9001
#define ETH_P_RTK		0x8899	// Realtek Remote Control Protocol (RRCP)
#define LOCAL_INBAND_IF1 "eth1"
#define LOCAL_INBAND_IF0 "eth0"

static inline int inband_deliver_check(struct net_bridge_port *p, struct sk_buff *skb)
{
	struct ethhdr *eth_hdr_p;
	eth_hdr_p = eth_hdr(skb);

	if(eth_hdr_p->h_proto == ETH_P_RTK_NOTIFY || eth_hdr_p->h_proto == ETH_P_RTK
							  || eth_hdr_p->h_proto == ETH_P_RTK_NOTIFY1 )
		if(memcmp(p->dev->name,LOCAL_INBAND_IF1,4) && memcmp(p->dev->name,LOCAL_INBAND_IF0,4) ) //not foward if not eth1 or eth0
			return 0;
	return 1;

}
#endif

/* called under bridge lock */
static void br_flood(struct net_bridge *br, struct sk_buff *skb,
	void (*__packet_hook)(const struct net_bridge_port *p,
			      struct sk_buff *skb))
{
	struct net_bridge_port *p;
	struct net_bridge_port *prev;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	prev = NULL;
//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
	int alpha_multicast_chk;
	if( br->snooping &&
		(memcmp(eth_hdr(skb)->h_dest, bcast_mac_addr, 6) != 0) &&       // non-broadcast packet ?!
		( (eth_hdr(skb)->h_proto == htons(ETH_P_IP)) || (eth_hdr(skb)->h_proto == htons(ETH_P_IPV6)))   ) // either IPv4 or IPv6
		alpha_multicast_chk = 1;
	else
		alpha_multicast_chk = 0;
	
#endif
	{
		prev = NULL;
		list_for_each_entry_rcu(p, &br->port_list, list) {
#ifndef CONFIG_RTK_INBAND_HOST_HACK
			if (should_deliver(p, skb)) {
#else
			if ((should_deliver(p, skb)) && (inband_deliver_check(p, skb))) {
#endif

//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP			
			if ( (alpha_multicast_chk == 1) &&
				(atomic_read(&p->wireless_interface) == 1))  // wireless interface
				//&& (iph->protocol == IPPROTO_UDP) ) //  only allow UDP packets ( IPPROTO_UDP: 17)   //rbj
			{
				do_enhance(p, br, skb,__packet_hook);
				continue;
			}
#endif
				/*patch for wan/lan receive duplicate unknown unicast/broadcast packet when pppoe/ipv6 passthrough enable*/
			/*except the packet dmac=33:33:xx:xx:xx:xx*/
			if((strcmp(skb->dev->name,"peth0")==0)&&(!(dest[0]==0x33&&dest[1]==0x33)))
				{
					 if((strncmp(p->dev->name,"eth",3)==0))
					 {
						continue;
					 }
				}

			/*patch for lan->wan duplicat packet(dmac=33:33:ff:xx:xx:xx) when pppoe/ipv6 passthrough enable*/
			if((strcmp(skb->dev->name,"eth0")==0)&&(	
			#if defined(CONFIG_RTK_VLAN_NEW_FEATURE)
				(rtk_vlan_support_enable==0)&&
			#endif
				(dest[0]==0x33&&dest[1]==0x33&&dest[2]==0xff)))
			{
				 if((strncmp(p->dev->name,"peth0",5)==0))
				 {
					continue;
				 }
			}

				if (prev != NULL) {
					struct sk_buff *skb2;

					if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
						LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
						br->dev->stats.tx_dropped++;
						kfree_skb(skb);
						return;
					}

					__packet_hook(prev, skb2);
				}

				prev = p;
			}
		}

		if (prev != NULL) {
			__packet_hook(prev, skb);
			return;
		}

		kfree_skb(skb);
	}
}

//Alpha
#if defined (CONFIG_BRIDGE_MDNSFILTER_BWCTRL)
/* called under bridge lock */
/*send multicast to one dev*/
static void br_flood_to_dev_(struct net_bridge *br, struct sk_buff *skb,
	void (*__packet_hook)(const struct net_bridge_port *p,
			      struct sk_buff *skb),char *dev)
{
	struct net_bridge_port *p;
		list_for_each_entry_rcu(p, &br->port_list, list) {
#ifndef CONFIG_RTK_INBAND_HOST_HACK
			if (should_deliver(p, skb)) {
#else
			if ((should_deliver(p, skb)) && (inband_deliver_check(p, skb))) {
#endif
 			      if(p!=NULL&&p->dev!=NULL&&(strcmp(p->dev->name,dev)==0)){
				if ( p->state != BR_STATE_DISABLED){
					struct sk_buff *skb2;
					if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
						LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
						br->dev->stats.tx_dropped++;
						kfree_skb(skb);
						return;
					}
					__packet_hook(p, skb2);
					break ;
				}
 			      	
			}
		}
		kfree_skb(skb);
	}

}

/* called with rcu_read_lock */
void br_flood_to_dev(struct net_bridge *br, struct sk_buff *skb,char * dev)
{
	br_flood_to_dev_(br, skb, __br_deliver,dev);
}
#endif

/* called with rcu_read_lock */
void br_flood_deliver(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_deliver);
}

/* called under bridge lock */
void br_flood_forward(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_forward);
}

#if defined (CONFIG_RTL_IGMP_SNOOPING)
int bitmask_to_id(unsigned char val)
{
	int i;
	for (i=0; i<8; i++) {
		if (val & (1 <<i))
			break;
	}

	if(i>=8)
	{
		i=7;
	}
	return (i);
}

static void br_multicast(struct net_bridge *br, unsigned int fwdPortMask, struct sk_buff *skb, int clone,
		  void (*__packet_hook)(const struct net_bridge_port *p, struct sk_buff *skb))
{
//	char i;
	struct net_bridge_port *prev;
	struct net_bridge_port *p, *n;
	unsigned short port_bitmask=0;
        if (clone) {
                struct sk_buff *skb2;

                if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
			LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
                        br->dev->stats.tx_dropped++;
                        return;
                }

                skb = skb2;
        }

	prev = NULL;

	list_for_each_entry_safe(p, n, &br->port_list, list) {
		port_bitmask = (1 << p->port_no);
                if ((port_bitmask & fwdPortMask) && should_deliver(p, skb)) {
                        if (prev != NULL) {
                                struct sk_buff *skb2;

                                if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
					LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
                                        br->dev->stats.tx_dropped++;
                                        kfree_skb(skb);
                                        return;
                                }

                                __packet_hook(prev, skb2);
                        }

                        prev = p;
                }
	}

        if (prev != NULL) {
                __packet_hook(prev, skb);
                return;
        }

	kfree_skb(skb);
}

void br_multicast_deliver(struct net_bridge *br, unsigned int fwdPortMask, struct sk_buff *skb, int clone)
{
	br_multicast(br, fwdPortMask, skb, clone, __br_deliver);
}
void br_multicast_forward(struct net_bridge *br, unsigned int fwdPortMask, struct sk_buff *skb, int clone)
{
	br_multicast(br, fwdPortMask, skb, clone, __br_forward);
}

extern struct net_bridge *bridge0;
extern int ipMulticastFastFwd;
extern int needCheckMfc;
#if defined (CONFIG_IP_MROUTE)
#if defined (CONFIG_RTL_IGMP_PROXY)
extern int rtl865x_checkMfcCache(struct net *net,struct net_device *dev,__be32 origin,__be32 mcastgrp);
#endif
#endif

#if defined(CONFIG_RTL_MLD_SNOOPING)
extern int re865x_getIpv6TransportProtocol(struct ipv6hdr* ipv6h);
#endif

extern int rtl865x_blockMulticastFlow(unsigned int srcVlanId, unsigned int srcPort,unsigned int srcIpAddr, unsigned int destIpAddr);
extern int rtl865x_curOpMode;

#define MAX_UNKNOWN_MULTICAST_NUM 16
#define MAX_UNKNOWN_MULTICAST_PPS 1500
#define BLOCK_UNKNOWN_MULTICAST 1

struct rtl865x_unKnownMCastRecord
{
	unsigned int groupAddr;
	unsigned long lastJiffies;
	unsigned long pktCnt;
	unsigned int valid;
};
struct rtl865x_unKnownMCastRecord unKnownMCastRecord[MAX_UNKNOWN_MULTICAST_NUM];

int rtl865x_checkUnknownMCastLoading(struct rtl_multicastDataInfo *mCastInfo)
{
	int i;
	if(mCastInfo==NULL)
	{
		return 0;
	}
	/*check entry existed or not*/
	for(i=0; i<MAX_UNKNOWN_MULTICAST_NUM; i++)
	{
		if((unKnownMCastRecord[i].valid==1) && (unKnownMCastRecord[i].groupAddr==mCastInfo->groupAddr[0]))
		{
			break;
		}
	}

	/*find an empty one*/
	if(i==MAX_UNKNOWN_MULTICAST_NUM)
	{
		for(i=0; i<MAX_UNKNOWN_MULTICAST_NUM; i++)
		{
			if(unKnownMCastRecord[i].valid!=1)
			{
				break;
			}
		}
	}

	/*find an exipired one */
	if(i==MAX_UNKNOWN_MULTICAST_NUM)
	{
		for(i=0; i<MAX_UNKNOWN_MULTICAST_NUM; i++)
		{
			if(	time_before(unKnownMCastRecord[i].lastJiffies+HZ,jiffies)
				|| time_after(unKnownMCastRecord[i].lastJiffies,jiffies+HZ)	)
			{
		
				break;
			}
		}
	}

	if(i==MAX_UNKNOWN_MULTICAST_NUM)
	{
		return 0;
	}

	unKnownMCastRecord[i].groupAddr=mCastInfo->groupAddr[0];
	unKnownMCastRecord[i].valid=1;
	
	if(time_after(unKnownMCastRecord[i].lastJiffies+HZ,jiffies))
	{
		unKnownMCastRecord[i].pktCnt++;
	}
	else
	{
		unKnownMCastRecord[i].lastJiffies=jiffies;
		unKnownMCastRecord[i].pktCnt=0;
	}

	if(unKnownMCastRecord[i].pktCnt>MAX_UNKNOWN_MULTICAST_PPS)
	{
		return BLOCK_UNKNOWN_MULTICAST;
	}

	return 0;
}
int rtl865x_ipMulticastFastFwd(struct sk_buff *skb)
{
	const unsigned char *dest = NULL;
	unsigned char *ptr;
	struct iphdr *iph=NULL;
	unsigned char proto=0;
	unsigned char reserved=0;
	int ret=-1;

	struct net_bridge_port *prev;
	struct net_bridge_port *p, *n;
	struct rtl_multicastDataInfo multicastDataInfo;
	struct rtl_multicastFwdInfo multicastFwdInfo;
	struct sk_buff *skb2;

	unsigned short port_bitmask=0;
	#if defined (CONFIG_RTL_MLD_SNOOPING)
	struct ipv6hdr * ipv6h=NULL;
	#endif
	unsigned int fwdCnt;
#if	defined (CONFIG_RTL_IGMP_PROXY)
	struct net_device *dev=skb->dev;
#endif
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	struct net_bridge *bridge = bridge0;
	unsigned int brSwFwdPortMask = br0SwFwdPortMask;
#endif	
#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
	unsigned int srcPort=skb->srcPort;
	unsigned int srcVlanId=skb->srcVlanId;
#endif
	/*check fast forward enable or not*/
	if(ipMulticastFastFwd==0)
	{
		return -1;
	}

	/*check dmac is multicast or not*/
	dest=eth_hdr(skb)->h_dest;
	if((dest[0]&0x01)==0)
	{
		return -1;
	}

	//printk("%s:%d,dest is 0x%x-%x-%x-%x-%x-%x\n",__FUNCTION__,__LINE__,dest[0],dest[1],dest[2],dest[3],dest[4],dest[5]);
	if(igmpsnoopenabled==0)
	{
		return -1;
	}

	/*check bridge0 exist or not*/
	if((bridge0==NULL) ||(bridge0->dev->flags & IFF_PROMISC))
	{
		return -1;
	}

	if((skb->dev==NULL) ||(strncmp(skb->dev->name,RTL_PS_BR0_DEV_NAME,3)==0))
	{
		return -1;
	}
    #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if((strncmp(skb->dev->name,RTL_PS_BR1_DEV_NAME,3)==0))
	{
		return -1;
	}
	#endif

	/*check igmp snooping enable or not, and check dmac is ipv4 multicast mac or not*/
	if  ((dest[0]==0x01) && (dest[1]==0x00) && (dest[2]==0x5e))
	{
		//printk("%s:%d,skb->dev->name is %s\n",__FUNCTION__,__LINE__,skb->dev->name );
		ptr=(unsigned char *)eth_hdr(skb)+12;
		/*check vlan tag exist or not*/
		if(*(int16 *)(ptr)==(int16)htons(0x8100))
		{
			ptr=ptr+4;
		}

		/*check it's ipv4 packet or not*/
		if(*(int16 *)(ptr)!=(int16)htons(ETH_P_IP))
		{
			return -1;
		}

		iph=(struct iphdr *)(ptr+2);

		if(iph->daddr== 0xEFFFFFFA)
		{
			/*for microsoft upnp*/
			reserved=1;
		}

		/*only speed up udp and tcp*/
		proto =  iph->protocol;
		//printk("%s:%d,proto is %d\n",__FUNCTION__,__LINE__,proto);
		 if(((proto ==IPPROTO_UDP) ||(proto ==IPPROTO_TCP)) && (reserved ==0))
		{

			#if defined (CONFIG_IP_MROUTE)
			/*multicast data comes from wan, need check multicast forwardig cache*/
			if((strncmp(skb->dev->name,RTL_PS_WAN0_DEV_NAME,4)==0) && needCheckMfc )
			{
				#if	defined (CONFIG_RTL_IGMP_PROXY)
				if(rtl865x_checkMfcCache(&init_net,dev,iph->saddr,iph->daddr)!=0)
				#endif	
				{
					if(rtl865x_checkUnknownMCastLoading(&multicastDataInfo)==BLOCK_UNKNOWN_MULTICAST)
					{
#if defined( CONFIG_RTL865X_HARDWARE_MULTICAST) || defined(CONFIG_RTL865X_LANPORT_RESTRICTION)
						if((skb->srcVlanId!=0) && (skb->srcPort!=0xFFFF))
						{
							rtl865x_blockMulticastFlow(srcVlanId, srcPort, iph->saddr,iph->daddr);
						}
						else
#endif
						{
							kfree_skb(skb);
							return 0;
						}
					}
				
					return -1;
				}
			}
			#endif

			multicastDataInfo.ipVersion=4;
			multicastDataInfo.sourceIp[0]=  (unsigned int)(iph->saddr);
			multicastDataInfo.groupAddr[0]=  (unsigned int)(iph->daddr);

            #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT //fix tim
			if(!strcmp(skb->dev->name,RTL_PS_ETH_NAME_ETH2)){
					ret= rtl_getMulticastDataFwdInfo(brIgmpModuleIndex_2, &multicastDataInfo, &multicastFwdInfo);
					bridge = bridge1;
					brSwFwdPortMask = br1SwFwdPortMask;
			}
			else
		    #endif
			ret= rtl_getMulticastDataFwdInfo(brIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);

			//printk("%s:%d,ret is %d\n",__FUNCTION__,__LINE__,ret);
			 if((ret!=0)||multicastFwdInfo.reservedMCast || multicastFwdInfo.unknownMCast)
			{
				if( multicastFwdInfo.unknownMCast && 
					(strncmp(skb->dev->name,RTL_PS_WAN0_DEV_NAME,4)==0) && 		//only block heavyloading multicast data from wan
					(rtl865x_checkUnknownMCastLoading(&multicastDataInfo)==BLOCK_UNKNOWN_MULTICAST))
				{
#if defined( CONFIG_RTL865X_HARDWARE_MULTICAST) || defined(CONFIG_RTL865X_LANPORT_RESTRICTION)
					if((skb->srcVlanId!=0) && (skb->srcPort!=0xFFFF))
					{
						rtl865x_blockMulticastFlow(srcVlanId, srcPort, iph->saddr,iph->daddr);
					}
					else
#endif
					{
						kfree_skb(skb);
						return 0;
					}
				}
				return -1;
			}


			//printk("%s:%d,br0SwFwdPortMask is 0x%x,multicastFwdInfo.fwdPortMask is 0x%x\n",__FUNCTION__,__LINE__,br0SwFwdPortMask,multicastFwdInfo.fwdPortMask);
			#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
			if((skb->srcVlanId!=0) && (skb->srcPort!=0xFFFF))
			{
				/*multicast data comes from ethernet port*/
				#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
				if( (brSwFwdPortMask & multicastFwdInfo.fwdPortMask)==0)
				#else
				if( (br0SwFwdPortMask & multicastFwdInfo.fwdPortMask)==0)
				#endif
				{
					/*hardware forwarding ,let slow path handle packets trapped to cpu*/
					return -1;
				}
			}
			#endif

			skb_push(skb, ETH_HLEN);

			prev = NULL;
			fwdCnt=0;

        #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
			list_for_each_entry_safe(p, n, &bridge->port_list, list) 
		#else
			list_for_each_entry_safe(p, n, &bridge0->port_list, list)
		#endif
			{
				port_bitmask = (1 << p->port_no);
				if ((port_bitmask & multicastFwdInfo.fwdPortMask) && should_deliver(p, skb)) 
				{
					if (prev != NULL)
					{
						if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL)
						{
							LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
                            #ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
							bridge->dev->stats.tx_dropped++;
			    			#else
							bridge0->dev->stats.tx_dropped++;
							#endif
							kfree_skb(skb);
							return 0;
						}
						skb2->dev=prev->dev;
						//printk("%s:%d,prev->dev->name is %s\n",__FUNCTION__,__LINE__,prev->dev->name);
						#if defined(CONFIG_COMPAT_NET_DEV_OPS)
						prev->dev->hard_start_xmit(skb2, prev->dev);
						#else
						prev->dev->netdev_ops->ndo_start_xmit(skb2,prev->dev);
						#endif
						fwdCnt++;
					}

					prev = p;
				}
			}

			if (prev != NULL)
			{
				skb->dev=prev->dev;
				//printk("%s:%d,prev->dev->name is %s\n",__FUNCTION__,__LINE__,prev->dev->name);
			       #if defined(CONFIG_COMPAT_NET_DEV_OPS)
				prev->dev->hard_start_xmit(skb, prev->dev);
				#else
				prev->dev->netdev_ops->ndo_start_xmit(skb,prev->dev);
				#endif
				fwdCnt++;
			}

			if(fwdCnt==0)
			{
				/*avoid memory leak*/
				skb_pull(skb, ETH_HLEN);
				return -1;
			}

			return 0;

		}

	}

#if 0 //defined (CONFIG_RTL_MLD_SNOOPING)
	/*check igmp snooping enable or not, and check dmac is ipv4 multicast mac or not*/
	if  ((dest[0]==0x33) && (dest[1]==0x33) && (dest[2]!=0xff))
	{
		struct net_bridge_port *p;
		if(mldSnoopEnabled==0)
		{
			return -1;
		}

		/*due to ipv6 passthrough*/
		p= rcu_dereference(skb->dev->br_port);
		if(p==NULL)
		{
			return -1;
		}

		//printk("%s:%d,skb->dev->name is %s\n",__FUNCTION__,__LINE__,skb->dev->name );
		ptr=(unsigned char *)eth_hdr(skb)+12;
		/*check vlan tag exist or not*/
		if(*(int16 *)(ptr)==(int16)htons(0x8100))
		{
			ptr=ptr+4;
		}

		/*check it's ipv6 packet or not*/
		if(*(int16 *)(ptr)!=(int16)htons(ETH_P_IPV6))
		{
			return -1;
		}

		ipv6h=(struct ipv6hdr *)(ptr+2);
		proto =  re865x_getIpv6TransportProtocol(ipv6h);

		//printk("%s:%d,proto is %d\n",__FUNCTION__,__LINE__,proto);
		 if((proto ==IPPROTO_UDP) ||(proto ==IPPROTO_TCP))
		{
			multicastDataInfo.ipVersion=6;
			memcpy(&multicastDataInfo.sourceIp, &ipv6h->saddr, sizeof(struct in6_addr));
			memcpy(&multicastDataInfo.groupAddr, &ipv6h->daddr, sizeof(struct in6_addr));
			/*
			printk("%s:%d,sourceIp is %x-%x-%x-%x\n",__FUNCTION__,__LINE__,
				multicastDataInfo.sourceIp[0],multicastDataInfo.sourceIp[1],multicastDataInfo.sourceIp[2],multicastDataInfo.sourceIp[3]);
			printk("%s:%d,groupAddr is %x-%x-%x-%x\n",__FUNCTION__,__LINE__,
				multicastDataInfo.groupAddr[0],multicastDataInfo.groupAddr[1],multicastDataInfo.groupAddr[2],multicastDataInfo.groupAddr[3]);
			*/
			ret= rtl_getMulticastDataFwdInfo(brIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);
			//printk("%s:%d,ret is %d\n",__FUNCTION__,__LINE__,ret);
			if(ret!=0)
			{
				if(multicastFwdInfo.unknownMCast)
				{
					multicastFwdInfo.fwdPortMask=0xFFFFFFFF;
				}
				else
				{
					return -1;
				}

			}

			//printk("%s:%d,multicastFwdInfo.fwdPortMask is 0x%x\n",__FUNCTION__,__LINE__,multicastFwdInfo.fwdPortMask);

			skb_push(skb, ETH_HLEN);

			prev = NULL;
			fwdCnt=0;
			list_for_each_entry_safe(p, n, &bridge0->port_list, list)
			{
				port_bitmask = (1 << p->port_no);
				if ((port_bitmask & multicastFwdInfo.fwdPortMask) && (skb->dev != p->dev && p->state == BR_STATE_FORWARDING))
				{
					if (prev != NULL)
					{
						if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL)
						{
							kfree_skb(skb);
							return 0;
						}
						skb2->dev=prev->dev;
						//printk("%s:%d,prev->dev->name is %s\n",__FUNCTION__,__LINE__,prev->dev->name);
						#if defined(CONFIG_COMPAT_NET_DEV_OPS)
						prev->dev->hard_start_xmit(skb2, prev->dev);
						#else
						prev->dev->netdev_ops->ndo_start_xmit(skb2,prev->dev);
						#endif
						fwdCnt++;
					}

					prev = p;
				}
			}

			if (prev != NULL)
			{
				skb->dev=prev->dev;
				//printk("%s:%d,prev->dev->name is %s\n",__FUNCTION__,__LINE__,prev->dev->name);
			       #if defined(CONFIG_COMPAT_NET_DEV_OPS)
				prev->dev->hard_start_xmit(skb, prev->dev);
				#else
				prev->dev->netdev_ops->ndo_start_xmit(skb,prev->dev);
				#endif
				fwdCnt++;
			}

			if(fwdCnt==0)
			{
				//printk("%s:%d\n",__FUNCTION__,__LINE__);
				/*avoid memory leak*/
				skb_pull(skb, ETH_HLEN);
				return -1;
			}

			return 0;
		}

	}
#endif

	return -1;
}

#endif
//Alpha
#ifdef CONFIG_BRIDGE_ALPHA_MULTICAST_SNOOP
void do_enhance(struct net_bridge_port *p, struct net_bridge *br, struct sk_buff *skb,
				void (*__packet_hook)(const struct net_bridge_port *p, struct sk_buff *skb))
{
	struct port_group_mac *g;
	struct sk_buff *skb2;
	int found =0;
	/*  does group address stored in table ? */
	list_for_each_entry(g, &p->igmp_group_list, list)
	{
		struct ethhdr * dest;
		struct port_member_mac *m;
		dest = eth_hdr(skb);
		if(!memcmp( dest->h_dest, g->grpmac, 6))
		{
			list_for_each_entry(m, &g->member_list, list)
			{
				if ((skb2 = skb_copy(skb, GFP_ATOMIC)) == NULL)
				{
					br->dev->stats.tx_dropped++;
					//kfree_skb(skb);
					return;
				}

				dest = eth_hdr(skb2);					
				memcpy(dest->h_dest, m->member_mac, sizeof(uint8_t)*6);
				if (should_deliver(p, skb2))
				{
					__packet_hook(p, skb2);
					found=1;
				}
				else
					kfree_skb(skb2);
			}
		}
	}
	if(!found)
	{
			/* it's wired interface or non-UDP packets*/
			/* skb_clone.....(flooding) */
			if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
				br->dev->stats.tx_dropped++;
				//kfree_skb(skb);
				return;
			}
			if (should_deliver(p,skb2))
			{
				__packet_hook(p, skb2);
			}
			else
				kfree_skb(skb2);

	}
}
#endif
