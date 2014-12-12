#ifndef __BCM_NAT__
#define __BCM_NAT__

#include <linux/config.h>
#include <linux/cache.h>
#include <linux/skbuff.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter_ipv4/ip_tables.h>

typedef int (*bcmNatHitHook)(struct sk_buff *skb);
typedef int (*bcmNatBindHook)(struct nf_conn *ct,enum ip_conntrack_info ctinfo,
	    						struct sk_buff **pskb, struct nf_conntrack_l4proto *l4proto);
								
int bcm_nat_bind_hook_func(bcmNatBindHook hook_func);
int bcm_nat_hit_hook_func(bcmNatHitHook hook_func);
#endif