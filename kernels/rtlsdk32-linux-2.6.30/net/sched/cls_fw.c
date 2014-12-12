/*
 * net/sched/cls_fw.c	Classifier mapping ipchains' fwmark to traffic class.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Changes:
 * Karlis Peisenieks <karlis@mt.lv> : 990415 : fw_walk off by one
 * Karlis Peisenieks <karlis@mt.lv> : 990415 : fw_delete killed all the filter (and kernel).
 * Alex <alex@pilotsoft.com> : 2004xxyy: Added Action extension
 *
 * JHS: We should remove the CONFIG_NET_CLS_IND from here
 * eventually when the meta match extension is made available
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <net/act_api.h>
#include <net/pkt_cls.h>
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
#include <net/rtl/rtl_types.h>
#include <net/rtl/rtl865x_netif.h>
#include <net/rtl/rtl865x_outputQueue.h>
#endif
//Alpha
#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
#include <net/ip.h>
#include <net/tcp.h>
#include <net/netfilter/nf_conntrack.h>
#endif

#define HTSIZE (PAGE_SIZE/sizeof(struct fw_filter *))

struct fw_head
{
	struct fw_filter *ht[HTSIZE];
	u32 mask;
};

struct fw_filter
{
	struct fw_filter	*next;
	u32			id;
	struct tcf_result	res;
#ifdef CONFIG_NET_CLS_IND
	char			indev[IFNAMSIZ];
#endif /* CONFIG_NET_CLS_IND */
	struct tcf_exts		exts;
};

static const struct tcf_ext_map fw_ext_map = {
	.action = TCA_FW_ACT,
	.police = TCA_FW_POLICE
};

static __inline__ int fw_hash(u32 handle)
{
	if (HTSIZE == 4096)
		return ((handle >> 24) & 0xFFF) ^
		       ((handle >> 12) & 0xFFF) ^
		       (handle & 0xFFF);
	else if (HTSIZE == 2048)
		return ((handle >> 22) & 0x7FF) ^
		       ((handle >> 11) & 0x7FF) ^
		       (handle & 0x7FF);
	else if (HTSIZE == 1024)
		return ((handle >> 20) & 0x3FF) ^
		       ((handle >> 10) & 0x3FF) ^
		       (handle & 0x3FF);
	else if (HTSIZE == 512)
		return (handle >> 27) ^
		       ((handle >> 18) & 0x1FF) ^
		       ((handle >> 9) & 0x1FF) ^
		       (handle & 0x1FF);
	else if (HTSIZE == 256) {
		u8 *t = (u8 *) &handle;
		return t[0] ^ t[1] ^ t[2] ^ t[3];
	} else
		return handle & (HTSIZE - 1);
}

/* bouble-20101026- 
  * Add policy to decide which mark value will be used by this classifier:
  * policy will be:
  * 0: check skb->mark only. This is linux native default.
  * 1: check connection->mark only.
  * 2: Prefer connection->mark, if connection->mark==0, then check skb->mark later.
  * 3: Prefer skb->mark, if skb->mark==0, then check connection->mark later.
  * 4: check connection->mark only, and speed up TCP small packets.
  */
//Alpha
#if defined(CONFIG_NET_CLS_FW_POLICY)
static int gScheFwPolicy = 0;
#endif

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
static int fw_classifyMark(__u32 mark, struct tcf_proto *tp,
			  struct tcf_result *res)
{
	struct fw_head *head;
	struct fw_filter *f;
	u32 id;

	head = (struct fw_head*)tp->root;
	
	if (head != NULL) {
		id = mark & head->mask;
		for (f=head->ht[fw_hash(id)]; f; f=f->next) {
			if (f->id == id) {
				*res = f->res;
				return 0;
			}
		}
	} else {
		/* old method */
		id = mark;
		if (id && (TC_H_MAJ(id) == 0 || !(TC_H_MAJ(id^tp->q->handle)))) {
			res->classid = id;
			res->class = 0;
			return 0;
		}
	}

	return -1;
}

static int fw_arrange_rules(struct tcf_proto *tp)
{
	rtl865x_qos_rule_t	*qosRule;
	struct tcf_result	res;

	qosRule = rtl865x_qosRuleHead;
	while(qosRule)
	{
		if (fw_classifyMark(qosRule->mark, tp, &res)==0)
		{
			qosRule->handle = res.classid;
		}
		else if(TC_H_MAJ(qosRule->handle)==TC_H_MAJ(tp->classid))
			qosRule->handle = 0;

		qosRule = qosRule->next;
	}

	rtl865x_qosArrangeRuleByNetif();
	
	return SUCCESS;
}

#endif

static int fw_classify(struct sk_buff *skb, struct tcf_proto *tp,
			  struct tcf_result *res)
{
	struct fw_head *head = (struct fw_head*)tp->root;
	struct fw_filter *f;
	int r;
	u32 id = skb->mark;

/* bouble-20101026- 
  * Add policy to decide which mark value will be used by this classifier:
  * policy will be:
  * 0: check skb->mark only. This is linux native default.
  * 1: check connection->mark only.
  * 2: Prefer connection->mark, if connection->mark==0, then check skb->mark later.
  * 3: Prefer skb->mark, if skb->mark==0, then check connection->mark later.
  * 4: check connection->mark only, and speed up TCP small packets.
  */
//Alpha
#if defined(CONFIG_NET_CLS_FW_POLICY)
	if ( gScheFwPolicy == 4 || gScheFwPolicy == 5)
	{
		struct nf_conn *ct = (struct nf_conn *)skb->nfct;
		int small_packet=0;

		/* just only use 8 - 16 bits,  17-24 bits will be used by other purpose, bit1-7 and bit25-32 will not be used */			
		if (ct != NULL)
		{
			const struct iphdr *ip = ip_hdr(skb);

			id = ct->mark & 0xFF80; 
			
			if (  ip != NULL )
			{
				struct tcphdr *tcph=(struct tcphdr *)(skb_network_header(skb) + (ip_hdr(skb)->ihl * 4));

#if 0
				if ( gScheFwPolicy == 5 )
				{
					char *ptr=(char*)ip;
					int i=0;
					
					printk("ntohs(ip->tot_len)=%d  protocol=%d\n", ntohs(ip->tot_len), ip->protocol);
					if (  tcph != NULL )
					{
						printk("source=%d dest=%d doff=%d syn=%d ack=%d fin=%d\n", ntohs(tcph->source), ntohs(tcph->dest), tcph->doff, tcph->syn,tcph->ack ,tcph->fin);
						printk("\nIP:\n");
						for ( i=0; i< 8;i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=8; i< 16; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=16; i< 24; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=24; i< 32; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=32; i< 40; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");

						printk("\nTCP:\n");
						ptr = (char*)tcph;
						for ( i=0; i< 8;i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=8; i< 16; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
						for ( i=16; i< 24; i++ )
							printk("%02x", ptr[i]&0xFF);
						printk("\n");
					}
				}
#endif
			}

			id = ct->mark & 0xFF80; 

			if ( skb->protocol == htons(ETH_P_IP)  && ntohs(ip_hdr(skb)->tot_len) <= 88 &&  (ip_hdr(skb)->protocol == IPPROTO_TCP))
			{
				struct tcphdr *tcph=(struct tcphdr *)(skb_network_header(skb) + (ip_hdr(skb)->ihl * 4));
				u32 id2 = ct->mark & 0xFF00;
				u32 flagBits = ct->mark & 0xFF0000;
				
				if ( tcph != NULL &&  ( tcph->syn || tcph->ack ))
				{
					/* for un-classify connection, then give it the highest priority */
					if ( flagBits != 0 )
					{
						id = 0x8000;
						small_packet=1;
					}
					else if ( id2 != 0 && (id2 & 0x5500) != 0 )
					{
						id = (id2 & 0x5500) << 1;
						small_packet=2;
					}
				}
			}

#if 0			
			if ( gScheFwPolicy == 5 )
				printk("ctmark=0x%04x   ct->mark=0x%08x  %s\n\n\n", id, ct->mark, small_packet==1? "[SF]":small_packet==2? "[S]":"[B]");
#endif
		}
		else
			id=0;
	}
	else if ( gScheFwPolicy == 1)
	{
		struct nf_conn *ct = (struct nf_conn *)skb->nfct;

		/* just only use 8- 16 bits,  17-24 bits will be used by other purpose, bit1-7 and bit25-32 will not be used */			
		if (ct != NULL)
			id = ct->mark & 0xFF80; 
		else
			id=0;
	}
	else if ( gScheFwPolicy == 2 )
	{
		struct nf_conn *ct = (struct nf_conn *)skb->nfct;

		if (ct != NULL)
		{
			/* just only use 8- 16 bits,  17-24 bits will be used by other purpose, bit1-7 and bit25-32 will not be used */			
			id = ct->mark & 0xFF80;
			if ( id == 0 )
				id = skb->mark;
		}
	}
	else if ( gScheFwPolicy == 3 )
	{
		if ( id == 0 )
		{
			struct nf_conn *ct = (struct nf_conn *)skb->nfct;

			/* just only use 8- 16 bits,  17-24 bits will be used by other purpose, bit1-7 and bit25-32 will not be used */			
			if (ct != NULL)
				id = ct->mark & 0xFF80; 
		}
	}
#endif

	if (head != NULL) {
		id &= head->mask;
		for (f=head->ht[fw_hash(id)]; f; f=f->next) {
			if (f->id == id) {
				*res = f->res;
#ifdef CONFIG_NET_CLS_IND
				if (!tcf_match_indev(skb, f->indev))
					continue;
#endif /* CONFIG_NET_CLS_IND */
				r = tcf_exts_exec(skb, &f->exts, res);
				if (r < 0)
					continue;

				return r;
			}
		}
	} else {
		/* old method */
		if (id && (TC_H_MAJ(id) == 0 || !(TC_H_MAJ(id^tp->q->handle)))) {
			res->classid = id;
			res->class = 0;
			return 0;
		}
	}

	return -1;
}

static unsigned long fw_get(struct tcf_proto *tp, u32 handle)
{
	struct fw_head *head = (struct fw_head*)tp->root;
	struct fw_filter *f;

	if (head == NULL)
		return 0;

	for (f=head->ht[fw_hash(handle)]; f; f=f->next) {
		if (f->id == handle)
			return (unsigned long)f;
	}
	return 0;
}

static void fw_put(struct tcf_proto *tp, unsigned long f)
{
}

static int fw_init(struct tcf_proto *tp)
{
	return 0;
}

static inline void
fw_delete_filter(struct tcf_proto *tp, struct fw_filter *f)
{
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
	fw_arrange_rules(tp);
#endif
	tcf_unbind_filter(tp, &f->res);
	tcf_exts_destroy(tp, &f->exts);
	kfree(f);
}

static void fw_destroy(struct tcf_proto *tp)
{
	struct fw_head *head = tp->root;
	struct fw_filter *f;
	int h;

	if (head == NULL)
		return;

	for (h=0; h<HTSIZE; h++) {
		while ((f=head->ht[h]) != NULL) {
			head->ht[h] = f->next;
			fw_delete_filter(tp, f);
		}
	}
	kfree(head);
}

static int fw_delete(struct tcf_proto *tp, unsigned long arg)
{
	struct fw_head *head = (struct fw_head*)tp->root;
	struct fw_filter *f = (struct fw_filter*)arg;
	struct fw_filter **fp;

	if (head == NULL || f == NULL)
		goto out;

	for (fp=&head->ht[fw_hash(f->id)]; *fp; fp = &(*fp)->next) {
		if (*fp == f) {
			tcf_tree_lock(tp);
			*fp = f->next;
			tcf_tree_unlock(tp);
			fw_delete_filter(tp, f);
			return 0;
		}
	}
out:
	return -EINVAL;
}

static const struct nla_policy fw_policy[TCA_FW_MAX + 1] = {
	[TCA_FW_CLASSID]	= { .type = NLA_U32 },
	[TCA_FW_INDEV]		= { .type = NLA_STRING, .len = IFNAMSIZ },
	[TCA_FW_MASK]		= { .type = NLA_U32 },
};

static int
fw_change_attrs(struct tcf_proto *tp, struct fw_filter *f,
	struct nlattr **tb, struct nlattr **tca, unsigned long base)
{
	struct fw_head *head = (struct fw_head *)tp->root;
	struct tcf_exts e;
	u32 mask;
	int err;

	err = tcf_exts_validate(tp, tb, tca[TCA_RATE], &e, &fw_ext_map);
	if (err < 0)
		return err;

	err = -EINVAL;
	if (tb[TCA_FW_CLASSID]) {
		f->res.classid = nla_get_u32(tb[TCA_FW_CLASSID]);
		tcf_bind_filter(tp, &f->res, base);
	}

#ifdef CONFIG_NET_CLS_IND
	if (tb[TCA_FW_INDEV]) {
		err = tcf_change_indev(tp, f->indev, tb[TCA_FW_INDEV]);
		if (err < 0)
			goto errout;
	}
#endif /* CONFIG_NET_CLS_IND */

	if (tb[TCA_FW_MASK]) {
		mask = nla_get_u32(tb[TCA_FW_MASK]);
		if (mask != head->mask)
			goto errout;
	} else if (head->mask != 0xFFFFFFFF)
		goto errout;

	tcf_exts_change(tp, &f->exts, &e);

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
	fw_arrange_rules(tp);
#endif

	return 0;
errout:
	tcf_exts_destroy(tp, &e);
	return err;
}

static int fw_change(struct tcf_proto *tp, unsigned long base,
		     u32 handle,
		     struct nlattr **tca,
		     unsigned long *arg)
{
	struct fw_head *head = (struct fw_head*)tp->root;
	struct fw_filter *f = (struct fw_filter *) *arg;
	struct nlattr *opt = tca[TCA_OPTIONS];
	struct nlattr *tb[TCA_FW_MAX + 1];
	int err;

	if (!opt)
		return handle ? -EINVAL : 0;

	err = nla_parse_nested(tb, TCA_FW_MAX, opt, fw_policy);
	if (err < 0)
		return err;

	if (f != NULL) {
		if (f->id != handle && handle)
			return -EINVAL;
		return fw_change_attrs(tp, f, tb, tca, base);
	}

	if (!handle)
		return -EINVAL;

	if (head == NULL) {
		u32 mask = 0xFFFFFFFF;
		if (tb[TCA_FW_MASK])
			mask = nla_get_u32(tb[TCA_FW_MASK]);

		head = kzalloc(sizeof(struct fw_head), GFP_KERNEL);
		if (head == NULL)
			return -ENOBUFS;
		head->mask = mask;

		tcf_tree_lock(tp);
		tp->root = head;
		tcf_tree_unlock(tp);
	}

	f = kzalloc(sizeof(struct fw_filter), GFP_KERNEL);
	if (f == NULL)
		return -ENOBUFS;

	f->id = handle;

	err = fw_change_attrs(tp, f, tb, tca, base);
	if (err < 0)
		goto errout;

	f->next = head->ht[fw_hash(handle)];
	tcf_tree_lock(tp);
	head->ht[fw_hash(handle)] = f;
	tcf_tree_unlock(tp);

	*arg = (unsigned long)f;

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
	fw_arrange_rules(tp);
#endif

	return 0;

errout:
	kfree(f);
	return err;
}

static void fw_walk(struct tcf_proto *tp, struct tcf_walker *arg)
{
	struct fw_head *head = (struct fw_head*)tp->root;
	int h;

	if (head == NULL)
		arg->stop = 1;

	if (arg->stop)
		return;

	for (h = 0; h < HTSIZE; h++) {
		struct fw_filter *f;

		for (f = head->ht[h]; f; f = f->next) {
			if (arg->count < arg->skip) {
				arg->count++;
				continue;
			}
			if (arg->fn(tp, (unsigned long)f, arg) < 0) {
				arg->stop = 1;
				return;
			}
			arg->count++;
		}
	}
}

static int fw_dump(struct tcf_proto *tp, unsigned long fh,
		   struct sk_buff *skb, struct tcmsg *t)
{
	struct fw_head *head = (struct fw_head *)tp->root;
	struct fw_filter *f = (struct fw_filter*)fh;
	unsigned char *b = skb_tail_pointer(skb);
	struct nlattr *nest;

	if (f == NULL)
		return skb->len;

	t->tcm_handle = f->id;

	if (!f->res.classid && !tcf_exts_is_available(&f->exts))
		return skb->len;

	nest = nla_nest_start(skb, TCA_OPTIONS);
	if (nest == NULL)
		goto nla_put_failure;

	if (f->res.classid)
		NLA_PUT_U32(skb, TCA_FW_CLASSID, f->res.classid);
#ifdef CONFIG_NET_CLS_IND
	if (strlen(f->indev))
		NLA_PUT_STRING(skb, TCA_FW_INDEV, f->indev);
#endif /* CONFIG_NET_CLS_IND */
	if (head->mask != 0xFFFFFFFF)
		NLA_PUT_U32(skb, TCA_FW_MASK, head->mask);

	if (tcf_exts_dump(skb, &f->exts, &fw_ext_map) < 0)
		goto nla_put_failure;

	nla_nest_end(skb, nest);

	if (tcf_exts_dump_stats(skb, &f->exts, &fw_ext_map) < 0)
		goto nla_put_failure;

	return skb->len;

nla_put_failure:
	nlmsg_trim(skb, b);
	return -1;
}

/* bouble-20101026- 
  * Add policy to decide which mark value will be used by this classifier:
  * policy will be:
  * 0: check skb->mark only. This is linux native default.
  * 1: check connection->mark only.
  * 2: Prefer connection->mark, if connection->mark==0, then check skb->mark later.
  * 3: Prefer skb->mark, if skb->mark==0, then check connection->mark later.
  * 4: check connection->mark only, and speed up TCP small packets.
  */
//Alpha
#if defined(CONFIG_NET_CLS_FW_POLICY)
#define SCHE_ROOT		"sche"
static struct proc_dir_entry * sche_root = NULL;
static struct proc_dir_entry * sche_fw = NULL;

int proc_sche_fw_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret=0;
	
	ret = sprintf(page, "%d\n", gScheFwPolicy);
	return ret;
}

int proc_sche_fw_write(struct file *file, const char * buffer, unsigned long count, void *data)
{
	char *ptr = (char*)buffer;
	int policy=0;

	if ( ptr != NULL )
	{
		policy = simple_strtoul(ptr, NULL, 10);
		if ( policy >= 0 &&  policy <= 5 )
		{
			printk("Set fw policy from %d(%s) to %d(%s)\n", 
					gScheFwPolicy,
					gScheFwPolicy==0? "SKB->mark only":
					gScheFwPolicy==1? "CONNECTION->mark only":
					gScheFwPolicy==2? "CONNECTION->mark prefer":
					gScheFwPolicy==3? "SKB->mark prefer":
					gScheFwPolicy==4? "CONNECTION->mark only plus Small packet enhance":"NA",
					policy,
					policy==0? "SKB->mark only":
					policy==1? "CONNECTION->mark only":
					policy==2? "CONNECTION->mark prefer":
					policy==3? "SKB->mark prefer":
					policy==4? "CONNECTION->mark only plus Small packet enhance":"NA");
			gScheFwPolicy = policy;
		}
	}

	return count;
}


void sche_remove_proc_entry(void)
{
	if (sche_fw) {
		remove_proc_entry("fw_policy", sche_root);
		sche_fw = NULL;
	}
	if (sche_root) {
		remove_proc_entry(SCHE_ROOT, NULL);
		sche_root = NULL;
	}
}

int sche_create_proc_entry(void)
{
	/* create directory */
	sche_root = proc_mkdir(SCHE_ROOT, NULL);
	if (sche_root == NULL) {
		printk("proc_mkdir return NULL!\n");
		goto sche_out;
	}

	/* create entries */
	sche_fw = create_proc_entry("fw_policy", 0644, sche_root);
	if (sche_fw == NULL) {
		printk("create_proc_entry (sche/fw_policy) return NULL!\n");
		goto sche_out;
	}
	sche_fw->read_proc = proc_sche_fw_read;
	sche_fw->write_proc = proc_sche_fw_write;

	return 0;
	
sche_out:
	sche_remove_proc_entry();
	printk("Unable to create %s !!\n", SCHE_ROOT);
	return -1;
}
#endif

static struct tcf_proto_ops cls_fw_ops __read_mostly = {
	.kind		=	"fw",
	.classify	=	fw_classify,
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
	.classifyMark	=	fw_classifyMark,
#endif
	.init		=	fw_init,
	.destroy	=	fw_destroy,
	.get		=	fw_get,
	.put		=	fw_put,
	.change		=	fw_change,
	.delete		=	fw_delete,
	.walk		=	fw_walk,
	.dump		=	fw_dump,
	.owner		=	THIS_MODULE,
};

static int __init init_fw(void)
{
/* bouble-20101022- 
  * Add policy to decide which mark value will be used by this classifier:
  * policy will be:
  * 0: check skb->mark only. This is linux native default.
  * 1: check connection->mark only.
  * 2: Prefer connection->mark, if connection->mark==0, then check skb->mark later.
  * 3: Prefer skb->mark, if skb->mark==0, then check connection->mark later.
  */
//Alpha
#if defined(CONFIG_NET_CLS_FW_POLICY)
	sche_create_proc_entry();
#endif

	return register_tcf_proto_ops(&cls_fw_ops);
}

static void __exit exit_fw(void)
{
/* bouble-20101022- 
  * Add policy to decide which mark value will be used by this classifier:
  * policy will be:
  * 0: check skb->mark only. This is linux native default.
  * 1: check connection->mark only.
  * 2: Prefer connection->mark, if connection->mark==0, then check skb->mark later.
  * 3: Prefer skb->mark, if skb->mark==0, then check connection->mark later.
  */
//Alpha
#if defined(CONFIG_NET_CLS_FW_POLICY)
	sche_remove_proc_entry();
#endif

	unregister_tcf_proto_ops(&cls_fw_ops);
}

module_init(init_fw)
module_exit(exit_fw)
MODULE_LICENSE("GPL");
