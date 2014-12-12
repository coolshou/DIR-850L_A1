/*
 * Packet matching code.
 *
 * Copyright (C) 1999 Paul `Rusty' Russell & Michael J. Neuling
 * Copyright (C) 2009-2002 Netfilter core team <coreteam@netfilter.org>
 *
 * 19 Jan 2002 Harald Welte <laforge@gnumonks.org>
 * 	- increase module usage count as soon as we have rules inside
 * 	  a table
 */
#include <linux/bcm_nat.h>

#ifdef HNDCTF
#include <typedefs.h>
extern bool ip_conntrack_is_ipc_allowed(struct sk_buff *skb, u_int32_t hooknum);
extern void ip_conntrack_ipct_add(struct sk_buff *skb, u_int32_t hooknum,
                             struct nf_conn *ct, enum ip_conntrack_info ci,
                             struct nf_conntrack_tuple *manip);
#endif /* HNDCTF */

#define NF_IP_PRE_ROUTING       (0)
/* If the packet is destined for this box. */
#define NF_IP_LOCAL_IN          (1)
/* If the packet is destined for another interface. */
#define NF_IP_FORWARD           (2)
/* Packets coming from a local process. */
#define NF_IP_LOCAL_OUT         (3)
/* Packets about to hit the wire. */
#define NF_IP_POST_ROUTING      (4)
#define NF_IP_NUMHOOKS          (5)

#define DEBUGP(format, args...)

bcmNatBindHook bcm_nat_bind_hook = NULL;
bcmNatHitHook bcm_nat_hit_hook = NULL;

int bcm_nat_bind_hook_func(bcmNatBindHook hook_func) 
{
	bcm_nat_bind_hook = hook_func;
	return 1;
};

int bcm_nat_hit_hook_func(bcmNatHitHook hook_func) 
{
	bcm_nat_hit_hook = hook_func;
	return 1;
};

extern int
bcm_manip_pkt(u_int16_t proto,
	  struct sk_buff **pskb,
	  unsigned int iphdroff,
	  const struct nf_conntrack_tuple *target,
	  enum nf_nat_manip_type maniptype);

/* 
 * Send packets to output.
 */
static inline int bcm_fast_path_output(struct sk_buff *skb)
{
	int ret = 0;
	struct dst_entry *dst = skb_dst(skb);
	struct hh_cache *hh = dst->hh;

	if (hh) {
		unsigned seq;
		int hh_len;

		do {
			int hh_alen;
			seq = read_seqbegin(&hh->hh_lock);
			hh_len = hh->hh_len;
			hh_alen = HH_DATA_ALIGN(hh_len);
			memcpy(skb->data - hh_alen, hh->hh_data, hh_alen);
			} while (read_seqretry(&hh->hh_lock, seq));

			skb_push(skb, hh_len);
		ret = hh->hh_output(skb); 
		if (ret==1) 
			return 0; /* Don't return 1 */
	} else if (dst->neighbour) {
		ret = dst->neighbour->output(skb);  
		if (ret==1) 
			return 0; /* Don't return 1 */
	}
	return ret;
}

static inline int ip_skb_dst_mtu(struct sk_buff *skb)
{
	struct inet_sock *inet = skb->sk ? inet_sk(skb->sk) : NULL;

	return (inet && inet->pmtudisc == IP_PMTUDISC_PROBE) ?
	       skb_dst(skb)->dev->mtu : dst_mtu(skb_dst(skb));
}

static inline int bcm_fast_path(struct sk_buff *skb)
{
	if (skb_dst(skb) == NULL) {
		struct iphdr *iph = ip_hdr(skb);
		struct net_device *dev = skb->dev;

		if (ip_route_input(skb, iph->daddr, iph->saddr, iph->tos, dev)) {
			return NF_DROP;
		}
		/*  Change skb owner to output device */
		skb->dev = skb_dst(skb)->dev;
	}

	if (skb_dst(skb)) {
		if (skb->len > ip_skb_dst_mtu(skb) && !skb_is_gso(skb))
			return ip_fragment(skb, bcm_fast_path_output);
		else
			return bcm_fast_path_output(skb);
	}

	kfree_skb(skb);
	return -EINVAL;
}

static int inline
ipv4_ct_invert_tuple(struct nf_conntrack_tuple *inverse,
		   const struct nf_conntrack_tuple *orig,
		   const struct nf_conntrack_l4proto *l4proto)
{
	inverse->src.u.all = inverse->dst.u.all = 0;
	inverse->src.u3.all[0]
	= inverse->src.u3.all[1]
	= inverse->src.u3.all[2]
	= inverse->src.u3.all[3]
	= 0;
	inverse->dst.u3.all[0]
	= inverse->dst.u3.all[1]
	= inverse->dst.u3.all[2]
	= inverse->dst.u3.all[3]
	= 0;
	inverse->src.u3.ip = orig->dst.u3.ip;
	inverse->dst.u3.ip = orig->src.u3.ip;
	inverse->src.l3num = orig->src.l3num;
	inverse->dst.dir = !orig->dst.dir;
	inverse->dst.protonum = orig->dst.protonum;
	return l4proto->invert_tuple(inverse, orig);
}

static inline int
bcm_do_bindings(struct nf_conn *ct,
	    enum ip_conntrack_info ctinfo,
	    struct sk_buff **pskb,
		struct nf_conntrack_l4proto *l4proto)
{
	struct iphdr *iph = ip_hdr(*pskb);
	unsigned int i;
	static int hn[2] = {NF_IP_PRE_ROUTING, NF_IP_POST_ROUTING};
	enum ip_conntrack_dir dir = CTINFO2DIR(ctinfo);
#ifdef HNDCTF
	bool enabled = ip_conntrack_is_ipc_allowed(*pskb, NF_IP_PRE_ROUTING);
#endif /* HNDCTF */

	for (i = 0; i < 2; i++) {
		enum nf_nat_manip_type mtype = HOOK2MANIP(hn[i]);
		unsigned long statusbit;

		if (mtype == IP_NAT_MANIP_SRC)
			statusbit = IPS_SRC_NAT;
		else
			statusbit = IPS_DST_NAT;

		/* Invert if this is reply dir. */
		if (dir == IP_CT_DIR_REPLY)
			statusbit ^= IPS_NAT_MASK;

		if (ct->status & statusbit) {
			struct nf_conntrack_tuple target;

			if (skb_cloned(*pskb) && !(*pskb)->sk) 
			{
				struct sk_buff *nskb = skb_copy(*pskb, GFP_ATOMIC);

				printk("[debug %s:%d] should not happen!!\n" , __FILE__ , __LINE__);
				if (!nskb)
					return NF_DROP;

				kfree_skb(*pskb);
				*pskb = nskb;
			}

			if (skb_dst((*pskb)) == NULL && mtype == IP_NAT_MANIP_SRC) {
				struct net_device *dev = (*pskb)->dev;
				if (ip_route_input((*pskb), iph->daddr, iph->saddr, iph->tos, dev))
					return NF_DROP;
				/* Change skb owner */
				(*pskb)->dev = skb_dst((*pskb))->dev;
			}

			/* We are aiming to look like inverse of other direction. */
			ipv4_ct_invert_tuple(&target, &ct->tuplehash[!dir].tuple, l4proto);
#ifdef HNDCTF
			if (enabled)
				ip_conntrack_ipct_add(*pskb, hn[i], ct, ctinfo, &target);
#endif /* HNDCTF */
			if (!bcm_manip_pkt(target.dst.protonum, pskb, 0, &target, mtype))
				return NF_DROP;
		}
	}

	return NF_FAST_NAT;
}

static int __init bcm_nat_init(void)
{
	bcm_nat_hit_hook_func(bcm_fast_path);
	bcm_nat_bind_hook_func(bcm_do_bindings);
	printk("BCM fast NAT: INIT\n");
	return 0;
}

static void __exit bcm_nat_fini(void)
{
	bcm_nat_hit_hook_func(NULL);
	bcm_nat_bind_hook_func(NULL);
	printk("BCM fast NAT: EXIT\n");
}

module_init(bcm_nat_init);
module_exit(bcm_nat_fini);
MODULE_LICENSE("Proprietary");
