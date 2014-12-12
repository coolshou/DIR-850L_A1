/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2003,2004 USAGI/WIDE Project <http://www.linux-ipv6.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/stddef.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/err.h>
#include <linux/percpu.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/mm.h>
#include <linux/rculist_nulls.h>
#include <linux/ip.h>
#include <linux/tcp.h>


#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>

#if defined(CONFIG_RTL_819X)
#include <net/rtl/features/rtl_ps_hooks.h>
#endif
#include <net/rtl/features/rtl_ps_log.h>

//Alpha
#include <linux/etherdevice.h>
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
#include <net/netfilter/nf_conntrack_tuple.h>
#include <net/netfilter/nf_conntrack_ike.h>
unsigned int (*is_ike_traffic_hook)(const struct nf_conntrack_tuple *tuple , struct sk_buff *skb , unsigned int dataoff);
EXPORT_SYMBOL_GPL(is_ike_traffic_hook);
struct nf_conn_ike *(*nf_ct_ike_ext_add_hook)(struct nf_conn *ct , gfp_t gfp , struct sk_buff *skb , unsigned int dataoff);
EXPORT_SYMBOL_GPL(nf_ct_ike_ext_add_hook);
void (*update_ike_cookie_pair_hook)(struct nf_conn *ct , struct sk_buff *skb , unsigned int dataoff);
EXPORT_SYMBOL_GPL(update_ike_cookie_pair_hook);
#endif

//Alpha
#ifdef CONFIG_BCM_NAT
#include <net/ip.h>
#define NF_IP_PRE_ROUTING (NF_INET_PRE_ROUTING)
#define NF_IP_LOCAL_IN (NF_INET_LOCAL_IN)
#define NF_IP_FORWARD (NF_INET_FORWARD)
#define NF_IP_LOCAL_OUT (NF_INET_LOCAL_OUT)
#define NF_IP_POST_ROUTING (NF_INET_POST_ROUTING)
#define NF_IP_NUMHOOKS (NF_INET_NUMHOOKS)
#endif

//Alpha
#define RESERVE_SESSION 1
#ifdef RESERVE_SESSION
#define FIRST_CLASS_RESERVE_SESSION 400
#define SECOND_CLASS_RESERVE_SESSION 100
static int flag_tcp_timeout_establish = 0;
#include <linux/netfilter/nf_conntrack_qos.h>
#endif

//Alpha
#ifdef CONFIG_HARDWARE_TURBO
#include <linux/hw_tcpip.h>
#endif

#define NF_CONNTRACK_VERSION	"0.5.0"

//Alpha
#ifdef CONFIG_HARDWARE_TURBO
int sw_disable_80 = 0;
#endif

//Alpha
#ifdef CONFIG_HARDWARE_TURBO
EXPORT_SYMBOL(sw_disable_80);
#endif

//Alpha
#ifdef CONFIG_BCM_NAT
#include <linux/bcm_nat.h>
#define	BCM_FASTNAT_DENY	1
extern int ipv4_conntrack_fastnat;
extern struct nf_conntrack_l3proto nf_conntrack_l3proto_ipv4;
extern bcmNatBindHook bcm_nat_bind_hook;
#endif

int (*nfnetlink_parse_nat_setup_hook)(struct nf_conn *ct,
				      enum nf_nat_manip_type manip,
				      struct nlattr *attr) __read_mostly;
EXPORT_SYMBOL_GPL(nfnetlink_parse_nat_setup_hook);

DEFINE_SPINLOCK(nf_conntrack_lock);
EXPORT_SYMBOL_GPL(nf_conntrack_lock);

unsigned int nf_conntrack_htable_size __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_htable_size);

unsigned int nf_conntrack_max __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_max);

struct nf_conn nf_conntrack_untracked __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_untracked);

static struct kmem_cache *nf_conntrack_cachep __read_mostly;

static int nf_conntrack_hash_rnd_initted;
static unsigned int nf_conntrack_hash_rnd;

static u_int32_t __hash_conntrack(const struct nf_conntrack_tuple *tuple,
				  unsigned int size, unsigned int rnd)
{
//Alpha
#ifdef CONFIG_BCM_NAT
	if (ipv4_conntrack_fastnat &&
		tuple->src.l3num == PF_INET && tuple->dst.protonum == PF_INET) {
		/* ntohl because more differences in low bits. */
		/* To ensure that halves of the same connection don't hash
		   clash, we add the source per-proto again. */
		return (ntohl(tuple->src.u3.ip + tuple->dst.u3.ip
			     + tuple->src.u.all + tuple->dst.u.all
			     + tuple->dst.protonum)
			+ ntohs(tuple->src.u.all))
			% nf_conntrack_htable_size;
	} else
#endif
	{
		unsigned int n;
		u_int32_t h;

		/* The direction must be ignored, so we hash everything up to the
		 * destination ports (which is a multiple of 4) and treat the last
		 * three bytes manually.
		 */
		n = (sizeof(tuple->src) + sizeof(tuple->dst.u3)) / sizeof(u32);
		h = jhash2((u32 *)tuple, n,
			   rnd ^ (((__force __u16)tuple->dst.u.all << 16) |
				  tuple->dst.protonum));

		return ((u64)h * size) >> 32;
	}
}

static inline u_int32_t hash_conntrack(const struct nf_conntrack_tuple *tuple)
{
	return __hash_conntrack(tuple, nf_conntrack_htable_size,
				nf_conntrack_hash_rnd);
}

bool
nf_ct_get_tuple(const struct sk_buff *skb,
		unsigned int nhoff,
		unsigned int dataoff,
		u_int16_t l3num,
		u_int8_t protonum,
		struct nf_conntrack_tuple *tuple,
		const struct nf_conntrack_l3proto *l3proto,
		const struct nf_conntrack_l4proto *l4proto)
{
	memset(tuple, 0, sizeof(*tuple));

	tuple->src.l3num = l3num;
	if (l3proto->pkt_to_tuple(skb, nhoff, tuple) == 0)
		return false;

	tuple->dst.protonum = protonum;
	tuple->dst.dir = IP_CT_DIR_ORIGINAL;

	return l4proto->pkt_to_tuple(skb, dataoff, tuple);
}
EXPORT_SYMBOL_GPL(nf_ct_get_tuple);

bool nf_ct_get_tuplepr(const struct sk_buff *skb, unsigned int nhoff,
		       u_int16_t l3num, struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int protoff;
	u_int8_t protonum;
	int ret;

	rcu_read_lock();

	l3proto = __nf_ct_l3proto_find(l3num);
	ret = l3proto->get_l4proto(skb, nhoff, &protoff, &protonum);
	if (ret != NF_ACCEPT) {
		rcu_read_unlock();
		return false;
	}

	l4proto = __nf_ct_l4proto_find(l3num, protonum);

	ret = nf_ct_get_tuple(skb, nhoff, protoff, l3num, protonum, tuple,
			      l3proto, l4proto);

	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_get_tuplepr);

//Alpha
#ifdef CONFIG_BCM_NAT
static int
ipv4_ct_get_tuple(const struct sk_buff *skb,
		unsigned int nhoff,
		unsigned int dataoff,
		u_int8_t protonum,
		struct nf_conntrack_tuple *tuple,
		const struct nf_conntrack_l4proto *l4proto)
{
	const struct iphdr *iph = ip_hdr(skb);

	tuple->src.u.all = tuple->dst.u.all = 0;

	tuple->src.u3.all[0]
	= tuple->src.u3.all[1]
	= tuple->src.u3.all[2]
	= tuple->src.u3.all[3]
	= 0;
	
	tuple->dst.u3.all[0]
	= tuple->dst.u3.all[1]
	= tuple->dst.u3.all[2]
	= tuple->dst.u3.all[3]
	= 0;
	
	tuple->src.l3num = PF_INET;
	tuple->src.u3.ip = iph->saddr;
	tuple->dst.u3.ip = iph->daddr;
	tuple->dst.protonum = protonum;
	tuple->dst.dir = IP_CT_DIR_ORIGINAL;

	return l4proto->pkt_to_tuple(skb, dataoff, tuple);
}
#endif

bool
nf_ct_invert_tuple(struct nf_conntrack_tuple *inverse,
		   const struct nf_conntrack_tuple *orig,
		   const struct nf_conntrack_l3proto *l3proto,
		   const struct nf_conntrack_l4proto *l4proto)
{
//Alpha
#ifdef CONFIG_BCM_NAT
	if (ipv4_conntrack_fastnat &&
		inverse->src.l3num == PF_INET && inverse->dst.protonum == PF_INET){
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
	} 
	else
	{
		memset(inverse, 0, sizeof(*inverse));

		if (l3proto->invert_tuple(inverse, orig) == 0)
			return 0;
	}
	inverse->src.l3num = orig->src.l3num;
#else
	memset(inverse, 0, sizeof(*inverse));

	inverse->src.l3num = orig->src.l3num;
	if (l3proto->invert_tuple(inverse, orig) == 0)
		return false;
#endif
	inverse->dst.dir = !orig->dst.dir;

	inverse->dst.protonum = orig->dst.protonum;
	return l4proto->invert_tuple(inverse, orig);
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuple);

#if !defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
static void clean_from_lists(struct nf_conn *ct)
{
	pr_debug("clean_from_lists(%p)\n", ct);
//Alpha
#ifdef CONFIG_HARDWARE_TURBO
	if(hw_tcpip && hw_tcpip->del_hw_nat_entry) 
	{
		int ret;
		pr_debug("Clean_from_lists: do hardware cleanup\n");
		if((ret = hw_tcpip->del_hw_nat_entry(ct)) == -EPERM) {
			pr_debug("ip_nat_cleanup_conntrack(%ld): retrun(%d)\n", ct->timeout.expires, ret);
			return;
		}
	}
#endif
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_REPLY].hnnode);

	/* Destroy all pending expectations */
	nf_ct_remove_expectations(ct);
}
#endif

static void
destroy_conntrack(struct nf_conntrack *nfct)
{
	struct nf_conn *ct = (struct nf_conn *)nfct;
	struct net *net = nf_ct_net(ct);
	struct nf_conntrack_l4proto *l4proto;
	#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_inso_s	conn_info;
	#endif

	pr_debug("destroy_conntrack(%p)\n", ct);
	NF_CT_ASSERT(atomic_read(&nfct->use) == 0);
	NF_CT_ASSERT(!timer_pending(&ct->timeout));

	if (!test_bit(IPS_DYING_BIT, &ct->status))
		nf_conntrack_event(IPCT_DESTROY, ct);
	set_bit(IPS_DYING_BIT, &ct->status);

	/* To make sure we don't get any weird locking issues here:
	 * destroy_conntrack() MUST NOT be called with a write lock
	 * to nf_conntrack_lock!!! -HW */
	#if defined(CONFIG_RTL_819X)
	conn_info.net = net;
	conn_info.ct = ct;

	rtl_nf_conntrack_destroy_hooks(&conn_info);
	#endif

	rcu_read_lock();
	l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
	if (l4proto && l4proto->destroy)
	{
		l4proto->destroy(ct);
	}
	rcu_read_unlock();

	spin_lock_bh(&nf_conntrack_lock);
	//Alpha
#ifdef CONFIG_HARDWARE_TURBO
	if(hw_tcpip && hw_tcpip->del_hw_nat_entry) 
		hw_tcpip->del_hw_nat_entry(ct);
#endif
	/* Expectations will have been removed in clean_from_lists,
	 * except TFTP can create an expectation on the first packet,
	 * before connection is in the list, so we need to clean here,
	 * too. */
	nf_ct_remove_expectations(ct);

	#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7) || defined(CONFIG_NETFILTER_XT_MATCH_LAYER7_MODULE)
	if(ct->layer7.app_proto)
		kfree(ct->layer7.app_proto);
	if(ct->layer7.app_data)
	kfree(ct->layer7.app_data);
	#endif

	/* We overload first tuple to link into unconfirmed list. */
	if (!nf_ct_is_confirmed(ct)) {
		BUG_ON(hlist_nulls_unhashed(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode));
		hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);
	}

	NF_CT_STAT_INC(net, delete);
	spin_unlock_bh(&nf_conntrack_lock);

	if (ct->master)
		nf_ct_put(ct->master);

	pr_debug("destroy_conntrack: returning ct=%p to slab\n", ct);
	nf_conntrack_free(ct);
}

#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
static void death_by_timeout_forced(unsigned long ul_conntrack)
{
	struct nf_conn *ct = (void *)ul_conntrack;
	struct net *net = nf_ct_net(ct);
	struct nf_conn_help *help;
	struct nf_conntrack_helper *helper;

	help = nfct_help(ct);
	if (help) {
		rcu_read_lock();
		helper = rcu_dereference(help->helper);
		if (helper && helper->destroy)
			helper->destroy(ct);
		rcu_read_unlock();
	}

	spin_lock_bh(&nf_conntrack_lock);
	/* Inside lock so preempt is disabled on module removal path.
	 * Otherwise we can get spurious warnings. */
	NF_CT_STAT_INC(net, delete_list);

	#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	clean_from_lists(ct, net);
	#else
	clean_from_lists(ct);
	#endif


	spin_unlock_bh(&nf_conntrack_lock);
	nf_ct_put(ct);
}
#endif

static void death_by_timeout(unsigned long ul_conntrack)
{
	struct nf_conn *ct = (void *)ul_conntrack;
	struct net *net = nf_ct_net(ct);
	struct nf_conn_help *help;
	struct nf_conntrack_helper *helper;
	#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_inso_s	conn_info;

	conn_info.net = net;
	conn_info.ct = ct;
	if (RTL_PS_HOOKS_RETURN==rtl_nf_conntrack_death_by_timeout_hooks(&conn_info))
		return;
	#endif

	help = nfct_help(ct);
	if (help) {
		rcu_read_lock();
		helper = rcu_dereference(help->helper);
		if (helper && helper->destroy)
			helper->destroy(ct);
		rcu_read_unlock();
	}

	spin_lock_bh(&nf_conntrack_lock);
	/* Inside lock so preempt is disabled on module removal path.
	 * Otherwise we can get spurious warnings. */
	NF_CT_STAT_INC(net, delete_list);
	#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	clean_from_lists((void*)ct, (void*)net);
	#else
	clean_from_lists(ct);
	#endif

	spin_unlock_bh(&nf_conntrack_lock);
	nf_ct_put(ct);
}

/*
 * Warning :
 * - Caller must take a reference on returned object
 *   and recheck nf_ct_tuple_equal(tuple, &h->tuple)
 * OR
 * - Caller must lock nf_conntrack_lock before calling this function
 */
struct nf_conntrack_tuple_hash *
__nf_conntrack_find(struct net *net, const struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;
	unsigned int hash = hash_conntrack(tuple);

	/* Disable BHs the entire time since we normally need to disable them
	 * at least once for the stats anyway.
	 */
	local_bh_disable();
begin:
	hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash], hnnode) {
		if (nf_ct_tuple_equal(tuple, &h->tuple)) {
			NF_CT_STAT_INC(net, found);
			local_bh_enable();
			return h;
		}
		NF_CT_STAT_INC(net, searched);
	}
	/*
	 * if the nulls value we got at the end of this lookup is
	 * not the expected one, we must restart lookup.
	 * We probably met an item that was moved to another chain.
	 */
	if (get_nulls_value(n) != hash)
		goto begin;
	local_bh_enable();

	return NULL;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_find);

/* Find a connection corresponding to a tuple. */
struct nf_conntrack_tuple_hash *
nf_conntrack_find_get(struct net *net, const struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;

	rcu_read_lock();
begin:
	h = __nf_conntrack_find(net, tuple);
	if (h) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		if (unlikely(nf_ct_is_dying(ct) ||
			     !atomic_inc_not_zero(&ct->ct_general.use)))
			h = NULL;
		else {
			if (unlikely(!nf_ct_tuple_equal(tuple, &h->tuple))) {
				nf_ct_put(ct);
				goto begin;
			}
		}
	}
	rcu_read_unlock();

	return h;
}
EXPORT_SYMBOL_GPL(nf_conntrack_find_get);

static void __nf_conntrack_hash_insert(struct nf_conn *ct,
				       unsigned int hash,
				       unsigned int repl_hash)
{
	struct net *net = nf_ct_net(ct);

	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode,
			   &net->ct.hash[hash]);
	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_REPLY].hnnode,
			   &net->ct.hash[repl_hash]);
}

void nf_conntrack_hash_insert(struct nf_conn *ct)
{
	unsigned int hash, repl_hash;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	__nf_conntrack_hash_insert(ct, hash, repl_hash);
}
EXPORT_SYMBOL_GPL(nf_conntrack_hash_insert);

/* Confirm a connection given skb; places it in hash table */
int
__nf_conntrack_confirm(struct sk_buff *skb)
{
	unsigned int hash, repl_hash;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct nf_conn_help *help;
	struct hlist_nulls_node *n;
	enum ip_conntrack_info ctinfo;
	struct net *net;
	#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_inso_s		conn_info;
	#endif

	ct = nf_ct_get(skb, &ctinfo);
	net = nf_ct_net(ct);

	/* ipt_REJECT uses nf_conntrack_attach to attach related
	   ICMP/TCP RST packets in other direction.  Actual packet
	   which created connection will be IP_CT_NEW or for an
	   expected connection, IP_CT_RELATED. */
	if (CTINFO2DIR(ctinfo) != IP_CT_DIR_ORIGINAL)
		return NF_ACCEPT;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	/* We're not in hash table, and we refuse to set up related
	   connections for unconfirmed conns.  But packet copies and
	   REJECT will give spurious warnings here. */
	/* NF_CT_ASSERT(atomic_read(&ct->ct_general.use) == 1); */

	/* No external references means noone else could have
	   confirmed us. */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));
	pr_debug("Confirming conntrack %p\n", ct);

	spin_lock_bh(&nf_conntrack_lock);

	/* See if there's one in the list already, including reverse:
	   NAT could have grabbed it without realizing, since we're
	   not in the hash.  If there is, we lost race. */
	hlist_nulls_for_each_entry(h, n, &net->ct.hash[hash], hnnode)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
				      &h->tuple))
			goto out;
	hlist_nulls_for_each_entry(h, n, &net->ct.hash[repl_hash], hnnode)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_REPLY].tuple,
				      &h->tuple))
			goto out;

	/* Remove from unconfirmed list */
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);

	#if defined(CONFIG_RTL_819X)
	conn_info.net = net;
	conn_info.ct = ct;
	conn_info.skb = skb;
	conn_info.ctinfo = ctinfo;
	rtl_nf_conntrack_confirm_hooks(&conn_info);
	#endif

	/* Timer relative to confirmation time, not original
	   setting time, otherwise we'd get timer wrap in
	   weird delay cases. */
	ct->timeout.expires += jiffies;
	add_timer(&ct->timeout);
	atomic_inc(&ct->ct_general.use);
	set_bit(IPS_CONFIRMED_BIT, &ct->status);

	/* Since the lookup is lockless, hash insertion must be done after
	 * starting the timer and setting the CONFIRMED bit. The RCU barriers
	 * guarantee that no other CPU can find the conntrack before the above
	 * stores are visible.
	 */
	__nf_conntrack_hash_insert(ct, hash, repl_hash);
	NF_CT_STAT_INC(net, insert);

	spin_unlock_bh(&nf_conntrack_lock);

	help = nfct_help(ct);
	if (help && help->helper)
		nf_conntrack_event_cache(IPCT_HELPER, ct);
#ifdef CONFIG_NF_NAT_NEEDED
	if (test_bit(IPS_SRC_NAT_DONE_BIT, &ct->status) ||
	    test_bit(IPS_DST_NAT_DONE_BIT, &ct->status))
		nf_conntrack_event_cache(IPCT_NATINFO, ct);
#endif
	nf_conntrack_event_cache(master_ct(ct) ?
				 IPCT_RELATED : IPCT_NEW, ct);
	return NF_ACCEPT;

out:
	NF_CT_STAT_INC(net, insert_failed);
	spin_unlock_bh(&nf_conntrack_lock);
	return NF_DROP;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_confirm);

/* Returns true if a connection correspondings to the tuple (required
   for NAT). */
int
nf_conntrack_tuple_taken(const struct nf_conntrack_tuple *tuple,
			 const struct nf_conn *ignored_conntrack)
{
	struct net *net = nf_ct_net(ignored_conntrack);
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;
	unsigned int hash = hash_conntrack(tuple);

	/* Disable BHs the entire time since we need to disable them at
	 * least once for the stats anyway.
	 */
	rcu_read_lock_bh();
	hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash], hnnode) {
		if (nf_ct_tuplehash_to_ctrack(h) != ignored_conntrack &&
		    nf_ct_tuple_equal(tuple, &h->tuple)) {
			NF_CT_STAT_INC(net, found);
			rcu_read_unlock_bh();
			return 1;
		}
		NF_CT_STAT_INC(net, searched);
	}
	rcu_read_unlock_bh();

	return 0;
}
EXPORT_SYMBOL_GPL(nf_conntrack_tuple_taken);

#define NF_CT_EVICTION_RANGE	8

#if !defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)	/* used only when NOT define GARBAGE_NEW */
/* There's a small race here where we may free a just-assured
   connection.  Too bad: we're in trouble anyway. */
static noinline int early_drop(struct net *net, unsigned int hash)
{
	/* Use oldest entry, which is roughly LRU */
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct = NULL, *tmp;
	struct hlist_nulls_node *n;
	unsigned int i, cnt = 0;
	int dropped = 0;

	rcu_read_lock();
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash],
					 hnnode) {
			tmp = nf_ct_tuplehash_to_ctrack(h);
			if (!test_bit(IPS_ASSURED_BIT, &tmp->status))
				ct = tmp;
			cnt++;
		}

		if (ct && unlikely(nf_ct_is_dying(ct) ||
				   !atomic_inc_not_zero(&ct->ct_general.use)))
			ct = NULL;
		if (ct || cnt >= NF_CT_EVICTION_RANGE)
			break;
		hash = (hash + 1) % nf_conntrack_htable_size;
	}
	rcu_read_unlock();

	if (!ct)
		return dropped;

	if (del_timer(&ct->timeout)) {
		death_by_timeout((unsigned long)ct);
		dropped = 1;
		NF_CT_STAT_INC_ATOMIC(net, early_drop);
	}
	nf_ct_put(ct);
	return dropped;
}
#endif

//Alpha
#ifdef RESERVE_SESSION
/* tune_tcp_timeout_establish(): 
 * explore all ct and modify the timeout of ct with establish state to the input argument 
 *
 * Input: timeout secs 
 * Output: Null
 */
void tune_tcp_timeout_establish(int timeout, 
								struct sk_buff *skb, 
								struct nf_conntrack_tuple *tuple,
								struct net *net)
{
	int			index=0;
	struct nf_conn *ct;
	extern	void set_tcp_timeout_established(int timeout);
	set_tcp_timeout_established(timeout);
	/* If ip_conntrack_count < TCP_TRAFFIC_LOW: 
	 * Just modify the global variable, ip_ct_tcp_timeout_established.
	 * Don't alter the obsolete ct's timeout value.
	 */
	if(flag_tcp_timeout_establish)
		return;

	read_lock_bh(&nf_conntrack_lock); //kwest++
	for(; index<nf_conntrack_htable_size; index++) 
	{
		if(list_empty(&net->ct.hash[index]))
			continue;
		else 
		{
			struct hlist_nulls_node *n;
			struct nf_conntrack_tuple_hash	*pos=NULL;
			hlist_nulls_for_each_entry(pos, n, &net->ct.hash[index], hnnode)
			{
				switch(pos->tuple.dst.protonum) 
				{
					case	IPPROTO_TCP:
						ct = nf_ct_tuplehash_to_ctrack(pos);
						if(ct->proto.tcp.state==TCP_CONNTRACK_ESTABLISHED) 
						{
							//FIX ME: skb is not useful in ip_ct_refresh() function, for event cache is not enabled.
							nf_ct_refresh(ct, skb, timeout*HZ);
						} 
						break;
				default:
					break;
				}
			}
		}
	}
	read_unlock_bh(&nf_conntrack_lock); //kwest++
}
#endif//RESERVE_SESSION



struct nf_conn *nf_conntrack_alloc(struct net *net,
				   const struct nf_conntrack_tuple *orig,
				   const struct nf_conntrack_tuple *repl,
				   gfp_t gfp)
{
	struct nf_conn *ct;

	if (unlikely(!nf_conntrack_hash_rnd_initted)) {
		get_random_bytes(&nf_conntrack_hash_rnd,
				sizeof(nf_conntrack_hash_rnd));
		nf_conntrack_hash_rnd_initted = 1;
	}

	/* We don't want any race condition at early drop stage */
	atomic_inc(&net->ct.count);
#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	if(nf_conntrack_max &&
		((atomic_read(&net->ct.count) > rtl_nf_conntrack_threshold))&&
		(atomic_read(&net->ct.count) < (nf_conntrack_max-1)))
	{
		if(isReservedConntrack(orig,repl))
		{
			/*use reserved conntrack,continue to allocate*/
			goto alloc_reserved_conn;
		}
	}
#endif

#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	if (nf_conntrack_max && unlikely(atomic_read(&net->ct.count) > rtl_nf_conntrack_threshold))
#else
	if (nf_conntrack_max && unlikely(atomic_read(&net->ct.count) > nf_conntrack_max))
#endif
	{
		/* Try dropping from this hash chain. */
#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
		if(!drop_one_conntrack(orig,repl))
#else
		unsigned int hash = hash_conntrack(orig);
		if (!early_drop(net, hash))
#endif
		{
			atomic_dec(&net->ct.count);
			if (net_ratelimit())
				printk(KERN_WARNING
				       "nf_conntrack: table full, dropping"
				       " packet.\n");
			return ERR_PTR(-ENOMEM);
		}
	}
//Alpha
//marco, we reserve the last 400 session for usual connection(e.g. port 80,53,21)
//and last 100 only for connection to device from lan
#ifdef RESERVE_SESSION
	else if (nf_conntrack_max &&  (nf_conntrack_max-atomic_read(&net->ct.count) <= FIRST_CLASS_RESERVE_SESSION) && (nf_conntrack_max-atomic_read(&net->ct.count) >= SECOND_CLASS_RESERVE_SESSION) )
	{
		if(ips_check_entry(orig) )
		{
			goto alloc_reserved_conn;//accept connection
		}
		else//drop it
		{
			goto EARLY_DROP;
		}
	}
	else if (nf_conntrack_max &&  (nf_conntrack_max-atomic_read(&net->ct.count) <= SECOND_CLASS_RESERVE_SESSION) )
	{
		u_int32_t	dip=orig->dst.u3.ip;
		//only accept connection to device from lan
		if(inet_addr_type(net,dip)==RTN_LOCAL)
		{
			goto alloc_reserved_conn;
		}
		else
		{
			goto EARLY_DROP;
		}
	}
	else
		goto alloc_reserved_conn;//accept connection
EARLY_DROP:
	{
#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
		if(!drop_one_conntrack(orig,repl))
#else
		unsigned int hash = hash_conntrack(orig);
		if (!early_drop(net, hash)) 
#endif
		{
			atomic_dec(&net->ct.count);
			if (net_ratelimit())
			{
			}
			return ERR_PTR(-ENOMEM);
		}	
	}		
#endif

alloc_reserved_conn:
	/*
	 * Do not use kmem_cache_zalloc(), as this cache uses
	 * SLAB_DESTROY_BY_RCU.
	 */
	ct = kmem_cache_alloc(nf_conntrack_cachep, gfp);
	if (ct == NULL) {
		LOG_WARN("nf_conntrack_alloc: Can't alloc conntrack.\n");
		atomic_dec(&net->ct.count);
		return ERR_PTR(-ENOMEM);
	}
	/*
	 * Let ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode.next
	 * and ct->tuplehash[IP_CT_DIR_REPLY].hnnode.next unchanged.
	 */
	memset(&ct->tuplehash[IP_CT_DIR_MAX], 0,
	       sizeof(*ct) - offsetof(struct nf_conn, tuplehash[IP_CT_DIR_MAX]));
	ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple = *orig;
	ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode.pprev = NULL;
	ct->tuplehash[IP_CT_DIR_REPLY].tuple = *repl;
	ct->tuplehash[IP_CT_DIR_REPLY].hnnode.pprev = NULL;
	/* Don't set timer yet: wait for confirmation */
	setup_timer(&ct->timeout, death_by_timeout, (unsigned long)ct);
#ifdef CONFIG_NET_NS
	ct->ct_net = net;
#endif

//Alpha
#ifdef CONFIG_BCM_NAT
	ct->bcm_flags = 0;
#endif

	/*
	 * changes to lookup keys must be done before setting refcnt to 1
	 */
	smp_wmb();
	atomic_set(&ct->ct_general.use, 1);
	return ct;
}
EXPORT_SYMBOL_GPL(nf_conntrack_alloc);

void nf_conntrack_free(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);

	nf_ct_ext_destroy(ct);
	atomic_dec(&net->ct.count);
	nf_ct_ext_free(ct);
	kmem_cache_free(nf_conntrack_cachep, ct);
}
EXPORT_SYMBOL_GPL(nf_conntrack_free);

/* Allocate a new conntrack: we return -ENOMEM if classification
   failed due to stress.  Otherwise it really is unclassifiable. */
#if defined(CONFIG_RTL_BATTLENET_ALG)
unsigned int wan_ip = 0;
unsigned int wan_mask = 0;

struct nf_conn *rtl_find_ct_by_tuple_dst(struct nf_conntrack_tuple *tuple, int *flag)
{
	int i;
	extern unsigned int _br0_ip;
	extern unsigned int _br0_mask;
	struct nf_conn *ct;
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;

	for(i=0; i< nf_conntrack_htable_size; i++)
	{
		hlist_nulls_for_each_entry(h, n, &init_net.ct.hash[i], hnnode)
		{
			if((__nf_ct_tuple_dst_equal(tuple, &h->tuple)) &&
			  ((h->tuple.src.u3.ip & _br0_mask) != (_br0_ip & _br0_mask))&&
			   (h->tuple.src.u.all == BATTLENET_PORT))
			  {
				//memcpy(&reply_tuple_temp, h, sizeof(struct nf_conntrack_tuple_hash));
				ct = nf_ct_tuplehash_to_ctrack(h);

				if(((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip & _br0_mask) == (_br0_ip & _br0_mask)) &&
				    ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip & _br0_mask) != (_br0_ip & _br0_mask))&&
				     (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip != tuple->src.u3.ip)){
						*flag = 1;
						return ct;
					}
			  }
		}
	}

	return NULL;

}

struct nf_conn *rtl_find_ct_by_tuple_src(struct nf_conntrack_tuple *tuple, int *flag)
{
	int i;
	extern unsigned int _br0_ip;
	extern unsigned int _br0_mask;
	struct nf_conn *ct;
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;

	for(i=0; i< nf_conntrack_htable_size; i++)
	{
		hlist_nulls_for_each_entry(h, n, &init_net.ct.hash[i], hnnode)
		{
			if((__nf_ct_tuple_src_equal(tuple, &h->tuple)) &&
			   (h->tuple.src.u3.ip  != wan_ip))
			  {
				ct = nf_ct_tuplehash_to_ctrack(h);

				if(((ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip & _br0_mask) != (_br0_ip & _br0_mask)) &&
				     (ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all == BATTLENET_PORT)&&
				     (ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip == wan_ip)){
						*flag = 1;
						return ct;
					}
			  }
		}
	}

	return NULL;

}

static void rtl_reconfig_reply_tupe(struct sk_buff *skb, const struct nf_conntrack_tuple *tuple,
	struct nf_conntrack_tuple *repl_tuple)
{

	extern unsigned int _br0_ip;
	extern unsigned int _br0_mask;

	int flag_reply = 0;
	int flag_ori = 0;
	struct nf_conn *ct_temp_reply = NULL;
	struct nf_conn *ct_temp_ori = NULL;
	struct net_device *wan_device = NULL;

	//struct nf_conntrack_tuple_hash reply_tuple_temp;
	//memset(&reply_tuple_temp, 0, sizeof(struct nf_conntrack_tuple_hash));

	wan_device = rtl865x_getBattleNetWanDev();
	rtl865x_getBattleNetDevIpAndNetmask(wan_device, &wan_ip, &wan_mask);
	/*
	1. protocol is udp;
	2. original src port is 6112;
	3. original src ip is lan ip;
	4. original dst ip is wan ip(ppp dev ip);
	e.g:
	original:192.168.1.100:6112->192.168.123.2:6112
	reply_1: 192.168.123.2:6112->192.168.1.100:6112
	reply_2: 192.168.1.101:6112->192.168.123.2:6112
	*/
	//printk("_br0_ip is %2x, _br0_mask is %2x\n", _br0_ip, _br0_mask);
	if((ip_hdr(skb)->protocol == IPPROTO_UDP) &&
		(ntohs(tuple->src.u.all) == BATTLENET_PORT) &&
		((tuple->src.u3.ip & _br0_mask) == (_br0_ip & _br0_mask)) &&
		((wan_ip!=0)&&(tuple->dst.u3.ip == wan_ip)))
	{
		spin_lock_bh(&nf_conntrack_lock);
		ct_temp_reply = rtl_find_ct_by_tuple_dst(tuple, &flag_reply);

		if((flag_reply == 1) && (ct_temp_reply != NULL))
		{
			/*printk("%s[%d],  sip is %2x, sport is %d; dip is %2x, dport is %d\n", __FUNCTION__, __LINE__,
				h->tuple.src.u3.ip, h->tuple.src.u.all,
				h->tuple.dst.u3.ip, h->tuple.dst.u.all);*/

			/*printk("%s[%d],  sip is %2x, sport is %d; dip is %2x, dport is %d\n", __FUNCTION__, __LINE__,
					ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip,
					ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,
					ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip,
					ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);*/

			ct_temp_ori = rtl_find_ct_by_tuple_src(tuple, &flag_ori);

			if((flag_ori == 1) && (ct_temp_ori != NULL))
			{
				/*change ct's reply src ip and port as the original tuple found*/
				repl_tuple->src.u3.ip = ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
				repl_tuple->src.u.all  = ct_temp_reply->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all;

				/*change ct's reply dst ip and port as original dst ip and port*/
				repl_tuple->dst.u3.ip = tuple->dst.u3.ip;
				repl_tuple->dst.u.all  = ct_temp_ori->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all;
			}
		}

		spin_unlock_bh(&nf_conntrack_lock);

	}

}
#endif

static struct nf_conntrack_tuple_hash *
init_conntrack(struct net *net,
	       const struct nf_conntrack_tuple *tuple,
	       struct nf_conntrack_l3proto *l3proto,
	       struct nf_conntrack_l4proto *l4proto,
	       struct sk_buff *skb,
	       unsigned int dataoff)
{
	struct nf_conn *ct;
	struct nf_conn_help *help;
	struct nf_conntrack_tuple repl_tuple;
	struct nf_conntrack_expect *exp;
//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	typeof(is_ike_traffic_hook) is_ike_traffic;
	typeof(nf_ct_ike_ext_add_hook) nf_ct_ike_ext_add;
#endif
//Alpha
#ifdef RESERVE_SESSION
	if(atomic_read(&net->ct.count)>TCP_TRAFFIC_THROTTLE 
			&& !flag_tcp_timeout_establish) 
	{
		tune_tcp_timeout_establish(TCP_TIMEOUT_ESTABLISH_BT, skb, tuple,net);
		flag_tcp_timeout_establish=1;
	}
	else if(atomic_read(&net->ct.count)<TCP_TRAFFIC_LOW 
			&& flag_tcp_timeout_establish) 
	{
		tune_tcp_timeout_establish(TCP_TIMEOUT_ESTABLISH_NORMAL, skb, tuple,net);
		flag_tcp_timeout_establish=0;
	}
#endif
	#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_inso_s	conn_info;
	#endif

	if (!nf_ct_invert_tuple(&repl_tuple, tuple, l3proto, l4proto)) {
		pr_debug("Can't invert tuple.\n");
		return NULL;
	}

#if defined(CONFIG_RTL_BATTLENET_ALG)
	rtl_reconfig_reply_tupe(skb, tuple, &repl_tuple);
#endif

	ct = nf_conntrack_alloc(net, tuple, &repl_tuple, GFP_ATOMIC);
	if (IS_ERR(ct)) {
		pr_debug("Can't allocate conntrack.\n");
		return (struct nf_conntrack_tuple_hash *)ct;
	}
	
	if (!l4proto->new(ct, skb, dataoff)) {
		nf_conntrack_free(ct);
		pr_debug("init conntrack: can't track with proto module\n");
//BUG Description:
//If the lan pc send out a packet with invalid tcp flags, 
//it can passthrough our router and the packet hvn't done NAT operation
//Root Cause:
//This is due to above checking, the tcp_new function will return false due to invalid flag
//.Linux original code will return NULL and conntrack isn't set up. But 
//the nf_conntrack_in function accept the connection if the return value is NULL
//Since the conntrack isn't set up, so the MASQ will not match.
//So I change the return value to ERR_PTR, and the connection will be drop.
		//marco, return NULL;
//Alpha
		return ERR_PTR(-ENETRESET);
	}

	nf_ct_acct_ext_add(ct, GFP_ATOMIC);

//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	is_ike_traffic = rcu_dereference(is_ike_traffic_hook);
	if(is_ike_traffic && is_ike_traffic(tuple , skb , dataoff))
	{
		nf_ct_ike_ext_add = rcu_dereference(nf_ct_ike_ext_add_hook);
		if(nf_ct_ike_ext_add)
			nf_ct_ike_ext_add(ct , GFP_ATOMIC , skb , dataoff);
	}
#endif

	spin_lock_bh(&nf_conntrack_lock);
	exp = nf_ct_find_expectation(net, tuple);
	if (exp) {
		pr_debug("conntrack: expectation arrives ct=%p exp=%p\n",
			 ct, exp);
		/* Welcome, Mr. Bond.  We've been expecting you... */
		__set_bit(IPS_EXPECTED_BIT, &ct->status);
		ct->master = exp->master;
		if (exp->helper) {
			help = nf_ct_helper_ext_add(ct, GFP_ATOMIC);
			if (help)
				rcu_assign_pointer(help->helper, exp->helper);
		}

#ifdef CONFIG_NF_CONNTRACK_MARK
		ct->mark = exp->master->mark;
#endif
#ifdef CONFIG_NF_CONNTRACK_SECMARK
		ct->secmark = exp->master->secmark;
#endif
		nf_conntrack_get(&ct->master->ct_general);
		NF_CT_STAT_INC(net, expect_new);
	} else {
		__nf_ct_try_assign_helper(ct, GFP_ATOMIC);
		NF_CT_STAT_INC(net, new);
	}

	#if defined(CONFIG_RTL_819X)
	conn_info.net = net;
	conn_info.ct = ct;
	conn_info.skb = skb;
	conn_info.l3proto = l3proto;
	conn_info.l4proto = l4proto;
	rtl_nf_init_conntrack_hooks(&conn_info);
	#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	ct->drop_flag = -1;
	ct->removed   = 0;
	#endif
	#endif

	/* Overload tuple linked list to put us in unconfirmed list. */
	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode,
		       &net->ct.unconfirmed);

	spin_unlock_bh(&nf_conntrack_lock);

	if (exp) {
		if (exp->expectfn)
			exp->expectfn(ct, exp);
		nf_ct_expect_put(exp);
	}

	return &ct->tuplehash[IP_CT_DIR_ORIGINAL];
}

/* On success, returns conntrack ptr, sets skb->nfct and ctinfo */
static inline struct nf_conn *
resolve_normal_ct(struct net *net,
		  struct sk_buff *skb,
		  unsigned int dataoff,
		  u_int16_t l3num,
		  u_int8_t protonum,
		  struct nf_conntrack_l3proto *l3proto,
		  struct nf_conntrack_l4proto *l4proto,
		  int *set_reply,
		  enum ip_conntrack_info *ctinfo)
{
	struct nf_conntrack_tuple tuple;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	int hit_ct = 0;
#endif

	if (!nf_ct_get_tuple(skb, skb_network_offset(skb),
			     dataoff, l3num, protonum, &tuple, l3proto,
			     l4proto)) {
		pr_debug("resolve_normal_ct: Can't get tuple\n");
		return NULL;
	}

	/* look for tuple match */
	h = nf_conntrack_find_get(net, &tuple);
	if (!h) {
		h = init_conntrack(net, &tuple, l3proto, l4proto, skb, dataoff);
		if (!h)
			return NULL;
		if (IS_ERR(h))
			return (void *)h;
	}
//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	else
		hit_ct = 1;
#endif

	ct = nf_ct_tuplehash_to_ctrack(h);
//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	if(hit_ct && 
	   NF_CT_DIRECTION(h) == IP_CT_DIR_ORIGINAL &&
	   nf_ct_ext_find(ct , NF_CT_EXT_IKE))
	{
		//this conntrack has ike information and this packet is from connection initiator
		//we need to update ike cookie, only initiator can update cookie information
		typeof(update_ike_cookie_pair_hook) update_ike_cookie_pair;

		update_ike_cookie_pair = rcu_dereference(update_ike_cookie_pair_hook);
		if(update_ike_cookie_pair)
			update_ike_cookie_pair(ct , skb , dataoff);
	}
#endif

	/* It exists; we have (non-exclusive) reference. */
	if (NF_CT_DIRECTION(h) == IP_CT_DIR_REPLY) {
		*ctinfo = IP_CT_ESTABLISHED + IP_CT_IS_REPLY;
		/* Please set reply bit if this packet OK */
		*set_reply = 1;
	} else {
		/* Once we've had two way comms, always ESTABLISHED. */
		if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
			pr_debug("nf_conntrack_in: normal packet for %p\n", ct);
			*ctinfo = IP_CT_ESTABLISHED;
		} else if (test_bit(IPS_EXPECTED_BIT, &ct->status)) {
			pr_debug("nf_conntrack_in: related packet for %p\n",
				 ct);
			*ctinfo = IP_CT_RELATED;
		} else {
			pr_debug("nf_conntrack_in: new packet for %p\n", ct);
			*ctinfo = IP_CT_NEW;
		}
		*set_reply = 0;
	}
	skb->nfct = &ct->ct_general;
	skb->nfctinfo = *ctinfo;
	return ct;
}

//Alpha
#ifdef CONFIG_BCM_NAT
static struct nf_conn *
ipv4_resolve_normal_ct(struct sk_buff *skb,
		  unsigned int dataoff,
		  u_int8_t protonum,
		  struct nf_conntrack_l4proto *l4proto,
		  int *set_reply,
		  enum ip_conntrack_info *ctinfo , 
		  struct net *net)
{
	struct nf_conntrack_tuple tuple;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;

	if (!ipv4_ct_get_tuple(skb, skb_network_offset(skb),
			     dataoff, protonum, &tuple, l4proto)) {
		//DEBUGP("resolve_normal_ct: Can't get tuple\n");
		return NULL;
	}

	/* look for tuple match */
	h = nf_conntrack_find_get(net , &tuple);
	if (!h) 
	{
		h = init_conntrack(net , &tuple, &nf_conntrack_l3proto_ipv4, l4proto, skb, dataoff);
		if (!h)
			return NULL;
		if (IS_ERR(h))
			return (void *)h;
	}

	ct = nf_ct_tuplehash_to_ctrack(h);

	/* It exists; we have (non-exclusive) reference. */
	if (NF_CT_DIRECTION(h) == IP_CT_DIR_REPLY) {
		*ctinfo = IP_CT_ESTABLISHED + IP_CT_IS_REPLY;
		/* Please set reply bit if this packet OK */
		*set_reply = 1;
	} else {
		/* Once we've had two way comms, always ESTABLISHED. */
		if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
			//DEBUGP("nf_conntrack_in: normal packet for %p\n", ct);
			*ctinfo = IP_CT_ESTABLISHED;
		} else if (test_bit(IPS_EXPECTED_BIT, &ct->status)) {
			//DEBUGP("nf_conntrack_in: related packet for %p\n", ct);
			*ctinfo = IP_CT_RELATED;
		} else {
			//DEBUGP("nf_conntrack_in: new packet for %p\n", ct);
			*ctinfo = IP_CT_NEW;
		}
		*set_reply = 0;
	}
	skb->nfct = &ct->ct_general;
	skb->nfctinfo = *ctinfo;
	return ct;
}
#endif

unsigned int
nf_conntrack_in(struct net *net, u_int8_t pf, unsigned int hooknum,
		struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int dataoff;
	u_int8_t protonum;
	int set_reply = 0;
	int ret;
	#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_inso_s		conn_info;
	#endif

	/* Previously seen (loopback or untracked)?  Ignore. */
	if (skb->nfct) {
		NF_CT_STAT_INC_ATOMIC(net, ignore);
		return NF_ACCEPT;
	}

	/* rcu_read_lock()ed by nf_hook_slow */
	l3proto = __nf_ct_l3proto_find(pf);
	ret = l3proto->get_l4proto(skb, skb_network_offset(skb),
				   &dataoff, &protonum);
	if (ret <= 0) {
		pr_debug("not prepared to track yet or error occured\n");
		NF_CT_STAT_INC_ATOMIC(net, error);
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		return -ret;
	}

	l4proto = __nf_ct_l4proto_find(pf, protonum);

	/* It may be an special packet, error, unclean...
	 * inverse of the return code tells to the netfilter
	 * core what to do with the packet. */
	if (l4proto->error != NULL) {
		ret = l4proto->error(net, skb, dataoff, &ctinfo, pf, hooknum);
		if (ret <= 0) {
			NF_CT_STAT_INC_ATOMIC(net, error);
			NF_CT_STAT_INC_ATOMIC(net, invalid);
			return -ret;
		}
	}

	ct = resolve_normal_ct(net, skb, dataoff, pf, protonum,
			       l3proto, l4proto, &set_reply, &ctinfo);
	if (!ct) {
		/* Not valid part of a connection */
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		return NF_ACCEPT;
	}

	if (IS_ERR(ct)) {
		/* Too stressed to deal. */
		NF_CT_STAT_INC_ATOMIC(net, drop);
		return NF_DROP;
	}

	NF_CT_ASSERT(skb->nfct);

	ret = l4proto->packet(ct, skb, dataoff, ctinfo, pf, hooknum);
	if (ret <= 0) {
		/* Invalid: inverse of the return code tells
		 * the netfilter core what to do */
		pr_debug("nf_conntrack_in: Can't track with proto module\n");
		nf_conntrack_put(skb->nfct);
		skb->nfct = NULL;
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		if (ret == -NF_DROP)
			NF_CT_STAT_INC_ATOMIC(net, drop);
		return -ret;
	}

 	#if defined(CONFIG_RTL_819X)
	conn_info.net = net;
	conn_info.ct = ct;
	conn_info.skb = skb;
	conn_info.l3proto = l3proto;
	conn_info.l4proto = l4proto;
	conn_info.protonum = protonum;
	conn_info.hooknum = hooknum;
	conn_info.ctinfo = ctinfo;
	rtl_nf_conntrack_in_hooks(&conn_info);
	#endif

	if (set_reply && !test_and_set_bit(IPS_SEEN_REPLY_BIT, &ct->status))
		nf_conntrack_event_cache(IPCT_STATUS, ct);

	return ret;
}
EXPORT_SYMBOL_GPL(nf_conntrack_in);

//Alpha
#ifdef CONFIG_BCM_NAT

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
extern int nf_ct_ipv4_gather_frags(struct sk_buff *skb, u_int32_t user);
#else
extern struct sk_buff *nf_ct_ipv4_gather_frags(struct sk_buff *skb, u_int32_t user);
#endif

static int is_ready_for_fastnat(struct nf_conn *ct)
{
	return ((ct->status & IPS_NAT_DONE_MASK) != 0);
}

static int skb_ptr_may_changed(struct sk_buff *skb)
{
	if(skb_cloned(skb) && !skb->sk)
		return 1;
	else
		return 0;
}
//Alpha
#ifdef CONFIG_ALPHA_FAST_ROUTE
static int is_routing(struct nf_conn *ct)
{
	struct nf_conntrack_tuple *t1, *t2;
	t1 = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	t2 = &ct->tuplehash[IP_CT_DIR_REPLY].tuple;
/*printk("ORIG %u.%u.%u.%u:%hu->",NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip),ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
printk("%u.%u.%u.%u:%hu\n",NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip),ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);
printk("REPLY %u.%u.%u.%u:%hu->",NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip),ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all);
printk("%u.%u.%u.%u:%hu\n",NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip),ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);*/
	return (t1->dst.u3.ip == t2->src.u3.ip &&
		t1->src.u3.ip == t2->dst.u3.ip &&
		t1->dst.u.all == t2->src.u.all &&
		t1->src.u.all == t2->dst.u.all);
}
#endif

unsigned int
ipv4_nf_conntrack_in(int pf, unsigned int hooknum, struct sk_buff **pskb , struct net *net)
{
	struct nf_conn *ct;
	//struct nf_conn_nat *nat;
	enum ip_conntrack_info ctinfo;
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int dataoff;
	u_int8_t protonum;
	int set_reply = 0;
	int ret;
	struct nf_conn_help *help;
	int ready_for_fastnat , skb_ptr_changed;

	/* Previously seen (loopback or untracked)?  Ignore. */
	if ((*pskb)->nfct) {
		NF_CT_STAT_INC_ATOMIC(net , ignore);
		return NF_ACCEPT;
	}

	/* rcu_read_lock()ed by nf_hook_slow */
	l3proto = &nf_conntrack_l3proto_ipv4;
	dataoff = skb_network_offset(*pskb) + ip_hdrlen(*pskb);
	protonum = ip_hdr(*pskb)->protocol;

	/* Gather fragments. */
	if (ip_hdr(*pskb)->frag_off & htons(IP_MF | IP_OFFSET)) 
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		int result = nf_ct_ipv4_gather_frags(*pskb,
						hooknum == NF_IP_PRE_ROUTING ?
						IP_DEFRAG_CONNTRACK_IN :
						IP_DEFRAG_CONNTRACK_OUT);
		if (result < 0)
			return NF_STOLEN;
#else
		*pskb = nf_ct_ipv4_gather_frags(*pskb,
						hooknum == NF_IP_PRE_ROUTING ?
						IP_DEFRAG_CONNTRACK_IN :
						IP_DEFRAG_CONNTRACK_OUT);
		if (!pskb)
			return NF_STOLEN;
#endif
	}

	l4proto = __nf_ct_l4proto_find((u_int16_t)pf, protonum);

	/* It may be an special packet, error, unclean...
	 * inverse of the return code tells to the netfilter
	 * core what to do with the packet. */

	if (l4proto->error != NULL &&
	    (ret = l4proto->error(net , *pskb, dataoff, &ctinfo, pf, hooknum)) <= 0) {
		NF_CT_STAT_INC_ATOMIC(net , error);
		NF_CT_STAT_INC_ATOMIC(net , invalid);
		return -ret;
	}

	ct = ipv4_resolve_normal_ct(*pskb, dataoff, protonum, l4proto,
			       &set_reply, &ctinfo , net);
	if (!ct) {
		/* Not valid part of a connection */
		NF_CT_STAT_INC_ATOMIC(net , invalid);
		return NF_ACCEPT;
	}

	if (IS_ERR(ct)) {
		/* Too stressed to deal. */
		NF_CT_STAT_INC_ATOMIC(net , drop);
		return NF_DROP;
	}

	NF_CT_ASSERT(*pskb->nfct);

	ret = l4proto->packet(ct, *pskb, dataoff, ctinfo, pf, hooknum);
	if (ret < 0) {
		/* Invalid: inverse of the return code tells
		 * the netfilter core what to do */
		//DEBUGP("nf_conntrack_in: Can't track with proto module\n");
		nf_conntrack_put((*pskb)->nfct);
		(*pskb)->nfct = NULL;
		NF_CT_STAT_INC_ATOMIC(net , invalid);
		return -ret;
	}

	help = nfct_help(ct);
	//nat = nfct_nat(ct);
	ready_for_fastnat = is_ready_for_fastnat(ct);
	skb_ptr_changed = skb_ptr_may_changed(*pskb);
	
#define TCP_ESTABLISH_STATE (ct->proto.tcp.state==TCP_CONNTRACK_ESTABLISHED)

//Alpha
#ifdef CONFIG_ALPHA_FAST_ROUTE
	extern int g_fast_route;
	extern int ip_rcv_finish(struct sk_buff *skb);

	if ( g_fast_route>0 
		&& is_routing(ct)
		&& (*pskb)->pkt_type!=PACKET_BROADCAST  && (*pskb)->pkt_type!=PACKET_MULTICAST
		&&!skb_ptr_changed && (!help || !help->helper)
		&& (ctinfo == IP_CT_ESTABLISHED || ctinfo == IP_CT_IS_REPLY)
		)
	{
 		//+++ michael_lee: (2011.11.08 13:38:40)
		// change status from new to seen_reply. when receive reply
		//packet the status will set to establish
       if (set_reply && !test_and_set_bit(IPS_SEEN_REPLY_BIT, &ct->status))
           nf_conntrack_event_cache(IPCT_STATUS, ct);
       //--- michael_lee
		
		if(hooknum == NF_IP_PRE_ROUTING)
		{
			int ret;
			(*pskb)->cb[FAST_ROUTE]=1;
			// this function will handle routing decision. the next hoook will be input or
			// forward chain
			if ((ret = ip_rcv_finish(*pskb)) == NF_FAST_NAT)
			{
				struct net_device *dev = skb_dst(*pskb)->dev;
				(*pskb)->dev = dev;
				(*pskb)->protocol = htons(ETH_P_IP);
				return NF_FAST_NAT;
			}
			// this tell system no need to handle this packet. we will handle this.
			return NF_STOLEN;//marco, I just want the hook stop checking			
		}
		else if(hooknum == NF_IP_LOCAL_OUT)
		{
			struct net_device *dev = skb_dst(*pskb)->dev;
			IP_UPD_PO_STATS(dev_net(dev), IPSTATS_MIB_OUT, (*pskb)->len);
			(*pskb)->dev = dev;
			(*pskb)->protocol = htons(ETH_P_IP);
			return NF_FAST_NAT;
		}
	}
#endif

	if (ipv4_conntrack_fastnat && bcm_nat_bind_hook && ready_for_fastnat && !skb_ptr_changed
		&& !(ct->bcm_flags & BCM_FASTNAT_DENY)
		&& (!help || !help->helper)
		&& (ctinfo == IP_CT_ESTABLISHED || ctinfo == IP_CT_IS_REPLY)
		&& (hooknum == NF_IP_PRE_ROUTING) && 
/* marco, only allow tcp packet with establish state pass fastnat
    or the mss in syn will not the same as expect
*/
		( (protonum == IPPROTO_TCP && TCP_ESTABLISH_STATE )|| protonum == IPPROTO_UDP) 
/* bouble-20101105 - Only fastnat for those packets that connection mark's flag == 0	*/
#if defined(CONFIG_NF_CONNTRACK_MARK)
		&& (( ct->mark & 0xFF0000 ) == 0 )
#endif
		) {
		struct nf_conntrack_tuple *t1, *t2;
		t1 = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
		t2 = &ct->tuplehash[IP_CT_DIR_REPLY].tuple;

		//skip the web port for url filter
		if(protonum == IPPROTO_TCP && t1->dst.u.tcp.port == htons(80))
			ct->bcm_flags |= BCM_FASTNAT_DENY;
		else
		if (!(t1->dst.u3.ip == t2->src.u3.ip &&
			t1->src.u3.ip == t2->dst.u3.ip &&
			t1->dst.u.all == t2->src.u.all &&
			t1->src.u.all == t2->dst.u.all)) 
		{
			ret = bcm_nat_bind_hook(ct, ctinfo, pskb, l4proto);
		}
	}
	else
	{
//Alpha
#if defined(CONFIG_RTL_819X)
		rtl_nf_conntrack_inso_s		conn_info;
		conn_info.net = net;
		conn_info.ct = ct;
		conn_info.skb = *pskb;
		conn_info.l3proto = l3proto;
		conn_info.l4proto = l4proto;
		conn_info.protonum = protonum;
		conn_info.hooknum = hooknum;
		conn_info.ctinfo = ctinfo;
		rtl_nf_conntrack_in_hooks(&conn_info);
#endif
	}
	
	if (set_reply && !test_and_set_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
		if (hooknum == NF_IP_LOCAL_OUT)
				ct->bcm_flags |= BCM_FASTNAT_DENY;

		nf_conntrack_event_cache(IPCT_STATUS, ct);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(ipv4_nf_conntrack_in);
#endif

bool nf_ct_invert_tuplepr(struct nf_conntrack_tuple *inverse,
			  const struct nf_conntrack_tuple *orig)
{
	bool ret;

	rcu_read_lock();
	ret = nf_ct_invert_tuple(inverse, orig,
				 __nf_ct_l3proto_find(orig->src.l3num),
				 __nf_ct_l4proto_find(orig->src.l3num,
						      orig->dst.protonum));
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuplepr);

/* Alter reply tuple (maybe alter helper).  This is for NAT, and is
   implicitly racy: see __nf_conntrack_confirm */
void nf_conntrack_alter_reply(struct nf_conn *ct,
			      const struct nf_conntrack_tuple *newreply)
{
	struct nf_conn_help *help = nfct_help(ct);

	/* Should be unconfirmed, so not in hash table yet */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));

	pr_debug("Altering reply tuple of %p to ", ct);
	nf_ct_dump_tuple(newreply);

	ct->tuplehash[IP_CT_DIR_REPLY].tuple = *newreply;
	if (ct->master || (help && !hlist_empty(&help->expectations)))
		return;

	rcu_read_lock();
	__nf_ct_try_assign_helper(ct, GFP_ATOMIC);
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(nf_conntrack_alter_reply);

/* Refresh conntrack for this many jiffies and do accounting if do_acct is 1 */
void __nf_ct_refresh_acct(struct nf_conn *ct,
			  enum ip_conntrack_info ctinfo,
			  const struct sk_buff *skb,
			  unsigned long extra_jiffies,
			  int do_acct)
{
	int event = 0;

	NF_CT_ASSERT(ct->timeout.data == (unsigned long)ct);
	NF_CT_ASSERT(skb);

	spin_lock_bh(&nf_conntrack_lock);

	/* Only update if this is not a fixed timeout */
	if (test_bit(IPS_FIXED_TIMEOUT_BIT, &ct->status))
		goto acct;

	/* If not in hash table, timer will not be active yet */
	if (!nf_ct_is_confirmed(ct)) {
		ct->timeout.expires = extra_jiffies;
		event = IPCT_REFRESH;
	} else {
		unsigned long newtime = jiffies + extra_jiffies;

		/* Only update the timeout if the new timeout is at least
		   HZ jiffies from the old timeout. Need del_timer for race
		   avoidance (may already be dying). */
		if (newtime - ct->timeout.expires >= HZ
		    && del_timer(&ct->timeout)) {
			ct->timeout.expires = newtime;
			add_timer(&ct->timeout);
			event = IPCT_REFRESH;
		}
	}

acct:
	if (do_acct) {
		struct nf_conn_counter *acct;

		acct = nf_conn_acct_find(ct);
		if (acct) {
			acct[CTINFO2DIR(ctinfo)].packets++;
			acct[CTINFO2DIR(ctinfo)].bytes +=
				skb->len - skb_network_offset(skb);
		}
	}

	spin_unlock_bh(&nf_conntrack_lock);

	/* must be unlocked when calling event cache */
	if (event)
		nf_conntrack_event_cache(event, ct);
}
EXPORT_SYMBOL_GPL(__nf_ct_refresh_acct);

//Alpha
void nf_ct_refresh_acct_stub(struct nf_conn *ct,
               enum ip_conntrack_info ctinfo,
               const struct sk_buff *skb,
               unsigned long extra_jiffies)
{
	__nf_ct_refresh_acct(ct, ctinfo, skb, extra_jiffies, 1);
}
EXPORT_SYMBOL(nf_ct_refresh_acct_stub);

bool __nf_ct_kill_acct(struct nf_conn *ct,
		       enum ip_conntrack_info ctinfo,
		       const struct sk_buff *skb,
		       int do_acct)
{
	if (do_acct) {
		struct nf_conn_counter *acct;

		spin_lock_bh(&nf_conntrack_lock);
		acct = nf_conn_acct_find(ct);
		if (acct) {
			acct[CTINFO2DIR(ctinfo)].packets++;
			acct[CTINFO2DIR(ctinfo)].bytes +=
				skb->len - skb_network_offset(skb);
		}
		spin_unlock_bh(&nf_conntrack_lock);
	}

	if (del_timer(&ct->timeout)) {
		ct->timeout.function((unsigned long)ct);
		return true;
	}
	return false;
}
EXPORT_SYMBOL_GPL(__nf_ct_kill_acct);

#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_conntrack.h>
#include <linux/mutex.h>

/* Generic function for tcp/udp/sctp/dccp and alike. This needs to be
 * in ip_conntrack_core, since we don't want the protocols to autoload
 * or depend on ctnetlink */
int nf_ct_port_tuple_to_nlattr(struct sk_buff *skb,
			       const struct nf_conntrack_tuple *tuple)
{
	NLA_PUT_BE16(skb, CTA_PROTO_SRC_PORT, tuple->src.u.tcp.port);
	NLA_PUT_BE16(skb, CTA_PROTO_DST_PORT, tuple->dst.u.tcp.port);
	return 0;

nla_put_failure:
	return -1;
}
EXPORT_SYMBOL_GPL(nf_ct_port_tuple_to_nlattr);

const struct nla_policy nf_ct_port_nla_policy[CTA_PROTO_MAX+1] = {
	[CTA_PROTO_SRC_PORT]  = { .type = NLA_U16 },
	[CTA_PROTO_DST_PORT]  = { .type = NLA_U16 },
};
EXPORT_SYMBOL_GPL(nf_ct_port_nla_policy);

int nf_ct_port_nlattr_to_tuple(struct nlattr *tb[],
			       struct nf_conntrack_tuple *t)
{
	if (!tb[CTA_PROTO_SRC_PORT] || !tb[CTA_PROTO_DST_PORT])
		return -EINVAL;

	t->src.u.tcp.port = nla_get_be16(tb[CTA_PROTO_SRC_PORT]);
	t->dst.u.tcp.port = nla_get_be16(tb[CTA_PROTO_DST_PORT]);

	return 0;
}
EXPORT_SYMBOL_GPL(nf_ct_port_nlattr_to_tuple);

int nf_ct_port_nlattr_tuple_size(void)
{
	return nla_policy_len(nf_ct_port_nla_policy, CTA_PROTO_MAX + 1);
}
EXPORT_SYMBOL_GPL(nf_ct_port_nlattr_tuple_size);
#endif

/* Used by ipt_REJECT and ip6t_REJECT. */
static void nf_conntrack_attach(struct sk_buff *nskb, struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;

	/* This ICMP is in reverse direction to the packet which caused it */
	ct = nf_ct_get(skb, &ctinfo);
	if (CTINFO2DIR(ctinfo) == IP_CT_DIR_ORIGINAL)
		ctinfo = IP_CT_RELATED + IP_CT_IS_REPLY;
	else
		ctinfo = IP_CT_RELATED;

	/* Attach to new skbuff, and increment count */
	nskb->nfct = &ct->ct_general;
	nskb->nfctinfo = ctinfo;
	nf_conntrack_get(nskb->nfct);
}

/* Bring out ya dead! */
static struct nf_conn *
get_next_corpse(struct net *net, int (*iter)(struct nf_conn *i, void *data),
		void *data, unsigned int *bucket)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct hlist_nulls_node *n;

	spin_lock_bh(&nf_conntrack_lock);
	for (; *bucket < nf_conntrack_htable_size; (*bucket)++) {
		hlist_nulls_for_each_entry(h, n, &net->ct.hash[*bucket], hnnode) {
			ct = nf_ct_tuplehash_to_ctrack(h);
			if (iter(ct, data))
				goto found;
		}
	}
	hlist_nulls_for_each_entry(h, n, &net->ct.unconfirmed, hnnode) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		if (iter(ct, data))
			set_bit(IPS_DYING_BIT, &ct->status);
	}
	spin_unlock_bh(&nf_conntrack_lock);
	return NULL;
found:
	atomic_inc(&ct->ct_general.use);
	spin_unlock_bh(&nf_conntrack_lock);
	return ct;
}

void nf_ct_iterate_cleanup(struct net *net,
			   int (*iter)(struct nf_conn *i, void *data),
			   void *data)
{
	struct nf_conn *ct;
	unsigned int bucket = 0;

	while ((ct = get_next_corpse(net, iter, data, &bucket)) != NULL) {
		/* Time to push up daises... */
		if (del_timer(&ct->timeout))
		{
			#if defined (CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
			death_by_timeout_forced((unsigned long)ct);
			#else
			death_by_timeout((unsigned long)ct);
			#endif
		}
		/* ... else the timer will get him soon. */

		nf_ct_put(ct);
	}
}
EXPORT_SYMBOL_GPL(nf_ct_iterate_cleanup);

struct __nf_ct_flush_report {
	u32 pid;
	int report;
};

static int kill_all(struct nf_conn *i, void *data)
{
	struct __nf_ct_flush_report *fr = (struct __nf_ct_flush_report *)data;

	/* get_next_corpse sets the dying bit for us */
	nf_conntrack_event_report(IPCT_DESTROY,
				  i,
				  fr->pid,
				  fr->report);
	return 1;
}

void nf_ct_free_hashtable(void *hash, int vmalloced, unsigned int size)
{
	if (vmalloced)
		vfree(hash);
	else
		free_pages((unsigned long)hash,
			   get_order(sizeof(struct hlist_head) * size));
}
EXPORT_SYMBOL_GPL(nf_ct_free_hashtable);

void nf_conntrack_flush(struct net *net, u32 pid, int report)
{
	struct __nf_ct_flush_report fr = {
		.pid 	= pid,
		.report = report,
	};
	nf_ct_iterate_cleanup(net, kill_all, &fr);
}
EXPORT_SYMBOL_GPL(nf_conntrack_flush);

static void nf_conntrack_cleanup_init_net(void)
{
	nf_conntrack_helper_fini();
	nf_conntrack_proto_fini();
	kmem_cache_destroy(nf_conntrack_cachep);
}

static void nf_conntrack_cleanup_net(struct net *net)
{
	nf_ct_event_cache_flush(net);
	nf_conntrack_ecache_fini(net);
 i_see_dead_people:
	nf_conntrack_flush(net, 0, 0);
	if (atomic_read(&net->ct.count) != 0) {
		schedule();
		goto i_see_dead_people;
	}
	/* wait until all references to nf_conntrack_untracked are dropped */
	while (atomic_read(&nf_conntrack_untracked.ct_general.use) > 1)
		schedule();

	nf_ct_free_hashtable(net->ct.hash, net->ct.hash_vmalloc,
			     nf_conntrack_htable_size);
	nf_conntrack_acct_fini(net);
	nf_conntrack_expect_fini(net);
	free_percpu(net->ct.stat);
}

/* Mishearing the voices in his head, our hero wonders how he's
   supposed to kill the mall. */
void nf_conntrack_cleanup(struct net *net)
{
	if (net_eq(net, &init_net))
		rcu_assign_pointer(ip_ct_attach, NULL);

	/* This makes sure all current packets have passed through
	   netfilter framework.  Roll on, two-stage module
	   delete... */
	synchronize_net();

	nf_conntrack_cleanup_net(net);

	if (net_eq(net, &init_net)) {
		rcu_assign_pointer(nf_ct_destroy, NULL);
		nf_conntrack_cleanup_init_net();
	}
}

void *nf_ct_alloc_hashtable(unsigned int *sizep, int *vmalloced, int nulls)
{
	struct hlist_nulls_head *hash;
	unsigned int nr_slots, i;
	size_t sz;

	*vmalloced = 0;

	BUILD_BUG_ON(sizeof(struct hlist_nulls_head) != sizeof(struct hlist_head));
	nr_slots = *sizep = roundup(*sizep, PAGE_SIZE / sizeof(struct hlist_nulls_head));
	sz = nr_slots * sizeof(struct hlist_nulls_head);
	hash = (void *)__get_free_pages(GFP_KERNEL | __GFP_NOWARN | __GFP_ZERO,
					get_order(sz));
	if (!hash) {
		*vmalloced = 1;
		printk(KERN_WARNING "nf_conntrack: falling back to vmalloc.\n");
		hash = __vmalloc(sz, GFP_KERNEL | __GFP_ZERO, PAGE_KERNEL);
	}

	if (hash && nulls)
		for (i = 0; i < nr_slots; i++)
			INIT_HLIST_NULLS_HEAD(&hash[i], i);

	return hash;
}
EXPORT_SYMBOL_GPL(nf_ct_alloc_hashtable);

int nf_conntrack_set_hashsize(const char *val, struct kernel_param *kp)
{
	int i, bucket, vmalloced, old_vmalloced;
	unsigned int hashsize, old_size;
	int rnd;
	struct hlist_nulls_head *hash, *old_hash;
	struct nf_conntrack_tuple_hash *h;

	/* On boot, we can set this without any fancy locking. */
	if (!nf_conntrack_htable_size)
		return param_set_uint(val, kp);

	hashsize = simple_strtoul(val, NULL, 0);
	if (!hashsize)
		return -EINVAL;

	hash = nf_ct_alloc_hashtable(&hashsize, &vmalloced, 1);
	if (!hash)
		return -ENOMEM;

	/* We have to rehahs for the new table anyway, so we also can
	 * use a newrandom seed */
	get_random_bytes(&rnd, sizeof(rnd));

	/* Lookups in the old hash might happen in parallel, which means we
	 * might get false negatives during connection lookup. New connections
	 * created because of a false negative won't make it into the hash
	 * though since that required taking the lock.
	 */
	spin_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		while (!hlist_nulls_empty(&init_net.ct.hash[i])) {
			h = hlist_nulls_entry(init_net.ct.hash[i].first,
					struct nf_conntrack_tuple_hash, hnnode);
			hlist_nulls_del_rcu(&h->hnnode);
			bucket = __hash_conntrack(&h->tuple, hashsize, rnd);
			hlist_nulls_add_head_rcu(&h->hnnode, &hash[bucket]);
		}
	}
	old_size = nf_conntrack_htable_size;
	old_vmalloced = init_net.ct.hash_vmalloc;
	old_hash = init_net.ct.hash;

	nf_conntrack_htable_size = hashsize;
	init_net.ct.hash_vmalloc = vmalloced;
	init_net.ct.hash = hash;
	nf_conntrack_hash_rnd = rnd;
	spin_unlock_bh(&nf_conntrack_lock);

	nf_ct_free_hashtable(old_hash, old_vmalloced, old_size);
	return 0;
}
EXPORT_SYMBOL_GPL(nf_conntrack_set_hashsize);

module_param_call(hashsize, nf_conntrack_set_hashsize, param_get_uint,
		  &nf_conntrack_htable_size, 0600);

static int nf_conntrack_init_init_net(void)
{
	int max_factor = 8;
	int ret;

	/* Idea from tcp.c: use 1/16384 of memory.  On i386: 32MB
	 * machine has 512 buckets. >= 1GB machines have 16384 buckets. */
	if (!nf_conntrack_htable_size) {
		nf_conntrack_htable_size
			= (((num_physpages << PAGE_SHIFT) / 16384)
			   / sizeof(struct hlist_head));
		if (num_physpages > (1024 * 1024 * 1024 / PAGE_SIZE))
			nf_conntrack_htable_size = 16384;
		if (nf_conntrack_htable_size < 32)
			nf_conntrack_htable_size = 32;

		/* Use a max. factor of four by default to get the same max as
		 * with the old struct list_heads. When a table size is given
		 * we use the old value of 8 to avoid reducing the max.
		 * entries. */
		max_factor = 4;
	}
	nf_conntrack_max = max_factor * nf_conntrack_htable_size;

	printk("nf_conntrack version %s (%u buckets, %d max)\n",
	       NF_CONNTRACK_VERSION, nf_conntrack_htable_size,
	       nf_conntrack_max);

	nf_conntrack_cachep = kmem_cache_create("nf_conntrack",
						sizeof(struct nf_conn),
						0, SLAB_DESTROY_BY_RCU, NULL);
	if (!nf_conntrack_cachep) {
		printk(KERN_ERR "Unable to create nf_conn slab cache\n");
		ret = -ENOMEM;
		goto err_cache;
	}

	ret = nf_conntrack_proto_init();
	if (ret < 0)
		goto err_proto;

	ret = nf_conntrack_helper_init();
	if (ret < 0)
		goto err_helper;

#if defined(CONFIG_RTL_819X)
	rtl_nf_conntrack_init_hooks();
#endif

	return 0;

err_helper:
	nf_conntrack_proto_fini();
err_proto:
	kmem_cache_destroy(nf_conntrack_cachep);
err_cache:
	return ret;
}

static int nf_conntrack_init_net(struct net *net)
{
	int ret;

	atomic_set(&net->ct.count, 0);
	INIT_HLIST_NULLS_HEAD(&net->ct.unconfirmed, 0);
	net->ct.stat = alloc_percpu(struct ip_conntrack_stat);
	if (!net->ct.stat) {
		ret = -ENOMEM;
		goto err_stat;
	}
	ret = nf_conntrack_ecache_init(net);
	if (ret < 0)
		goto err_ecache;
	net->ct.hash = nf_ct_alloc_hashtable(&nf_conntrack_htable_size,
					     &net->ct.hash_vmalloc, 1);
	if (!net->ct.hash) {
		ret = -ENOMEM;
		printk(KERN_ERR "Unable to create nf_conntrack_hash\n");
		goto err_hash;
	}
	ret = nf_conntrack_expect_init(net);
	if (ret < 0)
		goto err_expect;
	ret = nf_conntrack_acct_init(net);
	if (ret < 0)
		goto err_acct;

	/* Set up fake conntrack:
	    - to never be deleted, not in any hashes */
#ifdef CONFIG_NET_NS
	nf_conntrack_untracked.ct_net = &init_net;
#endif
	atomic_set(&nf_conntrack_untracked.ct_general.use, 1);
	/*  - and look it like as a confirmed connection */
	set_bit(IPS_CONFIRMED_BIT, &nf_conntrack_untracked.status);

	return 0;

err_acct:
	nf_conntrack_expect_fini(net);
err_expect:
	nf_ct_free_hashtable(net->ct.hash, net->ct.hash_vmalloc,
			     nf_conntrack_htable_size);
err_hash:
	nf_conntrack_ecache_fini(net);
err_ecache:
	free_percpu(net->ct.stat);
err_stat:
	return ret;
}

#ifdef RTL_NF_ALG_CTL

struct alg_entry alg_list[alg_type_end] =
{
    ALG_CTL_DEF(ftp,  1),
    ALG_CTL_DEF(tftp, 1),
    ALG_CTL_DEF(rtsp, 1),
    ALG_CTL_DEF(pptp, 1),
    ALG_CTL_DEF(l2tp, 1),
    ALG_CTL_DEF(ipsec,1),
    ALG_CTL_DEF(sip,  1),
    ALG_CTL_DEF(h323, 1),
};

int alg_enable(int type)
{
    return alg_list[type].enable;
}
EXPORT_SYMBOL_GPL(alg_enable);

static struct proc_dir_entry *proc_alg = NULL;

static int proc_alg_debug_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
    char *out = page;
	int len = 0;
    int i = 0;

    out += sprintf(out, "\n===================================================\n");
    for (i = 0; i < alg_type_end; i++)
    {
        out += sprintf(out, "| %s=%d\n", alg_list[i].name, alg_list[i].enable);
    }
    out += sprintf(out, "---------------------------------------------------\n");

	len = out - page;
	len -= off;
	if (len < count) {
		*eof = 1;
		if (len <= 0)
            return 0;
	} else
		len = count;

	*start = page + off;
	return len;
}

static int proc_alg_debug_write( struct file *filp, const char __user *buf,unsigned long len, void *data )
{
	int ret;
	char str_buf[256];
	char action[20] = {0};
	int val = 0;
	int i = 0;

	if(len > 255)
	{
		printk("Usage: echo ftp 1 > /proc/alg \n");
		return len;
	}

	copy_from_user(str_buf,buf,len);
	str_buf[len] = '\0';

	ret = sscanf(str_buf, "%s %d", action, (int*)&val);
	if(ret != 2 || val < 0 )
	{
		printk("Error. Sample: echo ftp 1 > /proc/alg \n");
		return len;
	}

	for (i = 0; i < alg_type_end; i++)
	{
	    if (0 == strcmp(action, alg_list[i].name))
	    {
	        alg_list[i].enable = val;
	        return len;
	    }
	}

	printk("Error: Unkown command.\n");

	return len;
}
#endif


int nf_conntrack_init(struct net *net)
{
	int ret;
//Alpha
#ifdef RESERVE_SESSION
	ips_qos_initial();
#endif	
	if (net_eq(net, &init_net)) {
		ret = nf_conntrack_init_init_net();
		if (ret < 0)
			goto out_init_net;
	}
	ret = nf_conntrack_init_net(net);
	if (ret < 0)
		goto out_net;

	if (net_eq(net, &init_net)) {
		/* For use by REJECT target */
		rcu_assign_pointer(ip_ct_attach, nf_conntrack_attach);
		rcu_assign_pointer(nf_ct_destroy, destroy_conntrack);
	}

#ifdef RTL_NF_ALG_CTL
	proc_alg = create_proc_entry("alg", 0, NULL);
	if (proc_alg) {
		proc_alg->read_proc = proc_alg_debug_read;
		proc_alg->write_proc = proc_alg_debug_write;
	}
#endif
	return 0;

out_net:
	if (net_eq(net, &init_net))
		nf_conntrack_cleanup_init_net();
out_init_net:
	return ret;
}
