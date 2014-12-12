/*
 * Connection state tracking for netfilter.  This is separated from,
 * but required by, the (future) NAT layer; it can also be used by an iptables
 * extension.
 *
 * 16 Dec 2003: Yasuyuki Kozakai @USAGI <yasuyuki.kozakai@toshiba.co.jp>
 *	- generalize L3 protocol dependent part.
 *
 * Derived from include/linux/netfiter_ipv4/ip_conntrack.h
 */

#ifndef _NF_CONNTRACK_H
#define _NF_CONNTRACK_H

#if defined(CONFIG_RTL_819X)
/* by default disable */
#if defined(CONFIG_FAST_PATH_SPI_ENABLED)
#define FAST_PATH_SPI_ENABLED		1
#endif

#endif

#include <linux/netfilter/nf_conntrack_common.h>

#ifdef __KERNEL__
#include <linux/bitops.h>
#include <linux/compiler.h>
#include <asm/atomic.h>

#include <linux/netfilter/nf_conntrack_tcp.h>
#include <linux/netfilter/nf_conntrack_dccp.h>
#include <linux/netfilter/nf_conntrack_sctp.h>
#include <linux/netfilter/nf_conntrack_proto_gre.h>
#include <net/netfilter/ipv4/nf_conntrack_icmp.h>
#include <net/netfilter/ipv6/nf_conntrack_icmpv6.h>

#include <net/netfilter/nf_conntrack_tuple.h>

//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
#include <linux/netfilter/nf_conntrack_ipsec_pass.h>
#endif

/* per conntrack: protocol private data */
union nf_conntrack_proto {
	/* insert conntrack proto private data here */
	struct nf_ct_dccp dccp;
	struct ip_ct_sctp sctp;
	struct ip_ct_tcp tcp;
	struct ip_ct_icmp icmp;
	struct nf_ct_icmpv6 icmpv6;
	struct nf_ct_gre gre;
//Alpha
#if defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH) || defined(CONFIG_NF_CONNTRACK_IPSEC_PASSTHROUGH_MODULE)
	struct nf_ct_esp esp;
#endif
};

union nf_conntrack_expect_proto {
	/* insert expect proto private data here */
};

/* Add protocol helper include file here */
#include <linux/netfilter/nf_conntrack_ftp.h>
#include <linux/netfilter/nf_conntrack_pptp.h>
#include <linux/netfilter/nf_conntrack_h323.h>
#include <linux/netfilter/nf_conntrack_sane.h>
#include <linux/netfilter/nf_conntrack_sip.h>

/* per conntrack: application helper private data */
union nf_conntrack_help {
	/* insert conntrack helper private data (master) here */
	struct nf_ct_ftp_master ct_ftp_info;
	struct nf_ct_pptp_master ct_pptp_info;
	struct nf_ct_h323_master ct_h323_info;
	struct nf_ct_sane_master ct_sane_info;
	struct nf_ct_sip_master ct_sip_info;
};

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/timer.h>

#ifdef CONFIG_NETFILTER_DEBUG
#define NF_CT_ASSERT(x)		WARN_ON(!(x))
#else
#define NF_CT_ASSERT(x)
#endif

struct nf_conntrack_helper;


/* Must be kept in sync with the classes defined by helpers */
#define NF_CT_MAX_EXPECT_CLASSES	3

/* nf_conn feature for connections that have a helper */
struct nf_conn_help {
	/* Helper. if any */
	struct nf_conntrack_helper *helper;

	union nf_conntrack_help help;

	struct hlist_head expectations;

	/* Current number of expected connections */
	u8 expecting[NF_CT_MAX_EXPECT_CLASSES];
};

#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/netfilter/ipv6/nf_conntrack_ipv6.h>

struct nf_conn {
#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
	/* Usage count in here is 1 for hash table/destruct timer, 1 per skb,
           plus 1 for any connection(s) we are `master' for */
	struct nf_conntrack ct_general;
#endif

	/* XXX should I move this to the tail ? - Y.K */
	/* These are my tuples; original and reply */
	struct nf_conntrack_tuple_hash tuplehash[IP_CT_DIR_MAX];

	/* Have we seen traffic both ways yet? (bitset) */
	unsigned long status;

	/* If we were expected by an expectation, this will be it */
	struct nf_conn *master;

	/* Timer function; drops refcnt when it goes off. */
	struct timer_list timeout;

#if defined(CONFIG_NF_CONNTRACK_MARK)
	u_int32_t mark;
#endif

//Alpha
//(tom, 20100513)
#ifdef CONFIG_SOFTWARE_TURBO
	void *sw;
#endif

//Alpha
#ifdef CONFIG_BCM_NAT
	u_int32_t bcm_flags;
#endif

#ifdef CONFIG_NF_CONNTRACK_SECMARK
	u_int32_t secmark;
#endif

#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7) || \
    defined(CONFIG_NETFILTER_XT_MATCH_LAYER7_MODULE)
	struct {
		/*
		 * e.g. "http". NULL before decision. "unknown" after decision
		 * if no match.
		 */
		char *app_proto;
		/*
		 * application layer data so far. NULL after match decision.
		 */
		char *app_data;
		unsigned int app_data_len;
	} layer7;
#endif

	/* Storage reserved for other modules: */
	union nf_conntrack_proto proto;

	/* Extensions */
	struct nf_ct_ext *ext;

	#ifdef CONFIG_NET_NS
	struct net *ct_net;
	#endif

	#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
       struct list_head 	state_tuple;
	   char drop_flag;
	   char removed;
	#endif
};


extern int routerTypeFlag;

#define CONFIG_RTL_ROUTER_FAST_PATH 1
#if defined (CONFIG_RTL_ROUTER_FAST_PATH)
extern unsigned int _br0_ip;
extern unsigned int _br0_mask;

static inline int rtl_isRouterType(struct nf_conn *ct)
{
	if(((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip) &&
	    (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip)))
	    		return 1;
	return 0;
}

static inline int rtl_isRouterTypeWantoLan(struct nf_conn * ct)
{
	if((ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip != _br0_ip) &&
		((ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip & _br0_mask) == (_br0_ip & _br0_mask)) &&
		((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip & _br0_mask) != (_br0_ip & _br0_mask)))
			return 1;

	return 0;
}


static inline int rtl_isNatTypeWantoLan(struct nf_conn* ct)
{
	if(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip== ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip)
		return 1;

	return 0;
}

static inline int rtl_isRouterTypeLantoWan(struct nf_conn* ct)
{
	if((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip != _br0_ip) &&
	  	 ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip & _br0_mask) == (_br0_ip & _br0_mask)) &&
	 	 ((ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip & _br0_mask) != (_br0_ip & _br0_mask)))
			return 1;

	return 0;
}

static inline int rtl_isNatTypeLantoWan(struct nf_conn* ct)
{
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip== ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip)
		return 1;

	return 0;
}

#endif


static inline struct nf_conn *
nf_ct_tuplehash_to_ctrack(const struct nf_conntrack_tuple_hash *hash)
{
	return container_of(hash, struct nf_conn,
			    tuplehash[hash->tuple.dst.dir]);
}

static inline u_int16_t nf_ct_l3num(const struct nf_conn *ct)
{
	return ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num;
}

static inline u_int8_t nf_ct_protonum(const struct nf_conn *ct)
{
	return ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
}

/* get master conntrack via master expectation */
#define master_ct(conntr) (conntr->master)

extern struct net init_net;

static inline struct net *nf_ct_net(const struct nf_conn *ct)
{
#ifdef CONFIG_NET_NS
	return ct->ct_net;
#else
	return &init_net;
#endif
}

/* Alter reply tuple (maybe alter helper). */
extern void
nf_conntrack_alter_reply(struct nf_conn *ct,
			 const struct nf_conntrack_tuple *newreply);

/* Is this tuple taken? (ignoring any belonging to the given
   conntrack). */
extern int
nf_conntrack_tuple_taken(const struct nf_conntrack_tuple *tuple,
			 const struct nf_conn *ignored_conntrack);

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)

/* Return conntrack_info and tuple hash for given skb. */
static inline struct nf_conn *
nf_ct_get(const struct sk_buff *skb, enum ip_conntrack_info *ctinfo)
{
	*ctinfo = skb->nfctinfo;
	return (struct nf_conn *)skb->nfct;
}

/* decrement reference count on a conntrack */
static inline void nf_ct_put(struct nf_conn *ct)
{
	NF_CT_ASSERT(ct);
	nf_conntrack_put(&ct->ct_general);
}
#endif

/* Protocol module loading */
extern int nf_ct_l3proto_try_module_get(unsigned short l3proto);
extern void nf_ct_l3proto_module_put(unsigned short l3proto);

/*
 * Allocate a hashtable of hlist_head (if nulls == 0),
 * or hlist_nulls_head (if nulls == 1)
 */
extern void *nf_ct_alloc_hashtable(unsigned int *sizep, int *vmalloced, int nulls);

extern void nf_ct_free_hashtable(void *hash, int vmalloced, unsigned int size);

extern struct nf_conntrack_tuple_hash *
__nf_conntrack_find(struct net *net, const struct nf_conntrack_tuple *tuple);

extern void nf_conntrack_hash_insert(struct nf_conn *ct);

extern void nf_conntrack_flush(struct net *net, u32 pid, int report);

extern bool nf_ct_get_tuplepr(const struct sk_buff *skb,
			      unsigned int nhoff, u_int16_t l3num,
			      struct nf_conntrack_tuple *tuple);
extern bool nf_ct_invert_tuplepr(struct nf_conntrack_tuple *inverse,
				 const struct nf_conntrack_tuple *orig);

extern void __nf_ct_refresh_acct(struct nf_conn *ct,
				 enum ip_conntrack_info ctinfo,
				 const struct sk_buff *skb,
				 unsigned long extra_jiffies,
				 int do_acct);

#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
extern void __nf_ct_refresh_acct_proto(void *ct,
				      enum ip_conntrack_info ctinfo,
				      const void *skb,
				      unsigned long extra_jiffies,
				      int do_acct,
				      unsigned char proto,
				      void * extra1,
				      void * extra2);

static inline void nf_ct_refresh_acct_tcp(struct nf_conn *ct,
				      enum ip_conntrack_info ctinfo,
				      const struct sk_buff *skb,
				      unsigned long extra_jiffies,
				      enum tcp_conntrack oldstate,
				      enum tcp_conntrack newstate)
{
	__nf_ct_refresh_acct_proto(ct, ctinfo, skb, extra_jiffies, 1, 6, (void *)oldstate, (void *)newstate);
}


static inline void nf_ct_refresh_acct_udp(struct nf_conn *ct,
				      enum ip_conntrack_info ctinfo,
				      const struct sk_buff *skb,
				      unsigned long extra_jiffies, char * status)
{
	__nf_ct_refresh_acct_proto(ct, ctinfo, skb, extra_jiffies, 1, 17, (void *)status, (void *)0);
}
#endif


/* Refresh conntrack for this many jiffies and do accounting */
static inline void nf_ct_refresh_acct(struct nf_conn *ct,
				      enum ip_conntrack_info ctinfo,
				      const struct sk_buff *skb,
				      unsigned long extra_jiffies)
{
	__nf_ct_refresh_acct(ct, ctinfo, skb, extra_jiffies, 1);
}
//Alpha
//for fastnat (tom, 20100518)
extern void nf_ct_refresh_acct_stub(struct nf_conn *ct,
				      enum ip_conntrack_info ctinfo,
				      const struct sk_buff *skb,
				      unsigned long extra_jiffies);

/* Refresh conntrack for this many jiffies */
static inline void nf_ct_refresh(struct nf_conn *ct,
				 const struct sk_buff *skb,
				 unsigned long extra_jiffies)
{
	__nf_ct_refresh_acct(ct, 0, skb, extra_jiffies, 0);
}

extern bool __nf_ct_kill_acct(struct nf_conn *ct,
			      enum ip_conntrack_info ctinfo,
			      const struct sk_buff *skb,
			      int do_acct);

/* kill conntrack and do accounting */
static inline bool nf_ct_kill_acct(struct nf_conn *ct,
				   enum ip_conntrack_info ctinfo,
				   const struct sk_buff *skb)
{
	return __nf_ct_kill_acct(ct, ctinfo, skb, 1);
}

/* kill conntrack without accounting */
static inline bool nf_ct_kill(struct nf_conn *ct)
{
	return __nf_ct_kill_acct(ct, 0, NULL, 0);
}

/* These are for NAT.  Icky. */
/* Update TCP window tracking data when NAT mangles the packet */
extern void nf_conntrack_tcp_update(const struct sk_buff *skb,
				    unsigned int dataoff,
				    struct nf_conn *ct, int dir,
				    s16 offset);

/* Fake conntrack entry for untracked connections */
extern struct nf_conn nf_conntrack_untracked;

#if defined(CONFIG_RTL_BATTLENET_ALG)
#define BATTLENET_PORT 6112
#define RTL_DEV_NAME_NUM(name,num)	name#num
#define RTL_PS_PPP_NAME	"ppp"
#define RTL_PS_PPP0_DEV_NAME RTL_DEV_NAME_NUM(RTL_PS_PPP_NAME,0)
extern unsigned int wan_ip;
extern unsigned int wan_mask;
extern struct net_device *rtl865x_getBattleNetWanDev(void );
extern int rtl865x_getBattleNetDevIpAndNetmask(struct net_device * dev, unsigned int *ipAddr, unsigned int *netMask );
extern struct nf_conn *rtl_find_ct_by_tuple_src(struct nf_conntrack_tuple *tuple, int *flag);
extern struct nf_conn *rtl_find_ct_by_tuple_dst(struct nf_conntrack_tuple *tuple, int *flag);

#endif

/* Iterate over all conntracks: if iter returns true, it's deleted. */
extern void
nf_ct_iterate_cleanup(struct net *net, int (*iter)(struct nf_conn *i, void *data), void *data);
extern void nf_conntrack_free(struct nf_conn *ct);
extern struct nf_conn *
nf_conntrack_alloc(struct net *net,
		   const struct nf_conntrack_tuple *orig,
		   const struct nf_conntrack_tuple *repl,
		   gfp_t gfp);

/* It's confirmed if it is, or has been in the hash table. */
static inline int nf_ct_is_confirmed(struct nf_conn *ct)
{
	return test_bit(IPS_CONFIRMED_BIT, &ct->status);
}

static inline int nf_ct_is_dying(struct nf_conn *ct)
{
	return test_bit(IPS_DYING_BIT, &ct->status);
}

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
static inline int nf_ct_is_untracked(const struct sk_buff *skb)
{
	return (skb->nfct == &nf_conntrack_untracked.ct_general);
}
#endif

extern int nf_conntrack_set_hashsize(const char *val, struct kernel_param *kp);
extern unsigned int nf_conntrack_htable_size;
extern unsigned int nf_conntrack_max;

#define NF_CT_STAT_INC(net, count)	\
	(per_cpu_ptr((net)->ct.stat, raw_smp_processor_id())->count++)
#define NF_CT_STAT_INC_ATOMIC(net, count)		\
do {							\
	local_bh_disable();				\
	per_cpu_ptr((net)->ct.stat, raw_smp_processor_id())->count++;	\
	local_bh_enable();				\
} while (0)

#define MODULE_ALIAS_NFCT_HELPER(helper) \
        MODULE_ALIAS("nfct-helper-" helper)


#define RTL_NF_ALG_CTL 1

#ifdef RTL_NF_ALG_CTL
extern int alg_enable(int type);

enum alg_type
{
    alg_type_ftp,
    alg_type_tftp,
    alg_type_rtsp,
    alg_type_pptp,
    alg_type_l2tp,
    alg_type_ipsec,
    alg_type_sip,
    alg_type_h323,
    alg_type_end
};

struct alg_entry
{
    char *name;
    int enable;
};

#define ALG_CTL_DEF(type, val)  [alg_type_##type] = {#type, val}

#define ALG_CHECK_ONOFF(type)   \
if (!alg_enable(type))\
{\
    return NF_DROP;\
}
#endif


#endif /* __KERNEL__ */
#endif /* _NF_CONNTRACK_H */
