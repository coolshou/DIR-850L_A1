#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/seq_file.h>
#include <net/dst.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_ike.h>
#include <linux/udp.h>

#define NF_CT_IPSEC_VERSION "1.0"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Netfilter connection tracking helper module for IPSEC");
MODULE_ALIAS("ip_conntrack_ipsec");
MODULE_ALIAS_NFCT_HELPER("ipsec");

#define ESP_TIMEOUT (10 * HZ)
#define ESP_STREAM_TIMEOUT (180 * HZ)

static unsigned int nf_ct_esp_timeout __read_mostly = ESP_TIMEOUT;
static unsigned int nf_ct_esp_stream_timeout __read_mostly = ESP_STREAM_TIMEOUT;

struct netns_proto_esp
{
	rwlock_t                lock;
	struct list_head        spi_pair_list;
};

static rwlock_t ike_cookie_lock;
static struct list_head ike_cookie_list;

struct spi_pair
{
	struct list_head list;
	struct nf_conntrack_tuple tuple[IP_CT_DIR_MAX];
	struct nf_conn *binding;
};

struct esp_hdr
{
	__be32 spi;
	__be32 seq;
};

struct ike_hdr
{
	unsigned char i_cookie[8];
	unsigned char r_cookie[8];
};

static int proto_esp_net_id __read_mostly;

/* invert esp part of tuple */
static bool esp_invert_tuple(struct nf_conntrack_tuple *tuple,
                             const struct nf_conntrack_tuple *orig)
{
	tuple->dst.u.all = orig->src.u.all;
	tuple->src.u.all = orig->dst.u.all;
	return true;
};

static inline int spi_pair_cmpfn(const struct nf_conntrack_tuple *spi ,
                                 const struct nf_conntrack_tuple *t)
{
	//we need exactly match here
	return spi->src.l3num == t->src.l3num &&
	       !memcmp(&spi->src.u3, &t->src.u3, sizeof(t->src.u3)) &&
	       !memcmp(&spi->dst.u3, &t->dst.u3, sizeof(t->dst.u3)) &&
	       spi->dst.protonum == t->dst.protonum &&
	       spi->dst.u.all == t->dst.u.all &&
	       spi->src.u.all == t->src.u.all;
}

//#define LOC_FMT "[%s %d] "
//#define LOC_ARG __FILE__ , __LINE__
//static void debug_dump_tuple(struct nf_conntrack_tuple *t)
//{
//	printk("tuple %p: %u %pI4:%x -> %pI4:%x\n",
//	       t, t->dst.protonum,&t->src.u3.ip, ntohs(t->src.u.all),&t->dst.u3.ip, ntohs(t->dst.u.all));
//}

static int spi_pair_add(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);
	struct netns_proto_esp *net_esp = net_generic(net, proto_esp_net_id);
	struct spi_pair *spi;
	struct nf_conntrack_tuple *t = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;

	read_lock_bh(&net_esp->lock);

	list_for_each_entry(spi , &net_esp->spi_pair_list , list)
	{
		if(spi_pair_cmpfn(&spi->tuple[IP_CT_DIR_ORIGINAL] , t))
		{
			if(spi->binding == NULL)
			{
				//build binding for pair that is created by trigger rule
				ct->proto.esp.spi = spi;
				spi->binding = ct;

				//printk(LOC_FMT"binding: " , LOC_ARG);
				//debug_dump_tuple(&spi->tuple[IP_CT_DIR_ORIGINAL]);
				//debug_dump_tuple(&spi->tuple[IP_CT_DIR_REPLY]);
			}

			read_unlock_bh(&net_esp->lock);
			return 0; //maybe retransmission or new section created
		}
	}

	read_unlock_bh(&net_esp->lock);

	spi = kmalloc(sizeof(*spi), GFP_ATOMIC);
	if(!spi)
		return -ENOMEM;

	memcpy(&spi->tuple[IP_CT_DIR_ORIGINAL] ,  &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple , sizeof(*t));
	memcpy(&spi->tuple[IP_CT_DIR_REPLY] ,  &ct->tuplehash[IP_CT_DIR_REPLY].tuple , sizeof(*t));
	spi->binding = ct;

	pr_debug("adding new entry %p: ", spi);
	//printk(LOC_FMT"add: " , LOC_ARG);
	//debug_dump_tuple(&spi->tuple[IP_CT_DIR_ORIGINAL]);
	//debug_dump_tuple(&spi->tuple[IP_CT_DIR_REPLY]);
	nf_ct_dump_tuple(&spi->tuple[IP_CT_DIR_ORIGINAL]);

	write_lock_bh(&net_esp->lock);
	list_add_tail(&spi->list , &net_esp->spi_pair_list);
	write_unlock_bh(&net_esp->lock);

	ct->proto.esp.spi = spi;

	return 0;
}

static void spi_pair_del(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);
	struct netns_proto_esp *net_esp = net_generic(net, proto_esp_net_id);
	struct spi_pair *spi = ct->proto.esp.spi;
	struct nf_conntrack_tuple t[IP_CT_DIR_MAX];

	pr_debug("entering for ct %p\n", ct);

	write_lock_bh(&net_esp->lock);
	pr_debug("removing %p from list\n" , spi);
	memcpy(&t , &spi->tuple[IP_CT_DIR_ORIGINAL] , sizeof(t));
	list_del(&spi->list);
	kfree(spi);
	ct->proto.esp.spi = NULL;
	write_unlock_bh(&net_esp->lock);

	//printk(LOC_FMT"del: " , LOC_ARG);
	//debug_dump_tuple(&t[IP_CT_DIR_ORIGINAL]);
	//debug_dump_tuple(&t[IP_CT_DIR_REPLY]);
}

static void spi_pairs_flush(struct net *net)
{
	struct netns_proto_esp *net_esp = net_generic(net, proto_esp_net_id);
	struct spi_pair *spi , *tmp;

	write_lock_bh(&net_esp->lock);
	list_for_each_entry_safe(spi , tmp , &net_esp->spi_pair_list , list)
	{
		list_del(&spi->list);
		kfree(spi);
	}
	write_unlock_bh(&net_esp->lock);
}

static bool modify_tuple_by_spi_pair(struct net *net , struct nf_conntrack_tuple *t)
{
	struct netns_proto_esp *net_esp = net_generic(net, proto_esp_net_id);
	struct spi_pair *spi , *copied_spi;
	struct nf_conntrack_tuple *perfact[IP_CT_DIR_MAX] , *partial[IP_CT_DIR_MAX];
	int index;
	bool rv = true;

	for(index = 0 ; index < IP_CT_DIR_MAX ; ++index)
	{
		perfact[index] = NULL;
		partial[index] = NULL;
	}

	write_lock_bh(&net_esp->lock);
	list_for_each_entry(spi , &net_esp->spi_pair_list , list)
	{
		//check IP_CT_DIR_ORIGINAL
		if(!memcmp(&spi->tuple[IP_CT_DIR_ORIGINAL].src.u3, &t->src.u3, sizeof(t->src.u3)) &&
		        !memcmp(&spi->tuple[IP_CT_DIR_ORIGINAL].dst.u3, &t->dst.u3, sizeof(t->dst.u3)) &&
		        spi->tuple[IP_CT_DIR_ORIGINAL].dst.u.all == t->dst.u.all)
		{
			partial[IP_CT_DIR_ORIGINAL] = &spi->tuple[IP_CT_DIR_ORIGINAL];
			if(spi->tuple[IP_CT_DIR_ORIGINAL].src.u.all != 0)
				perfact[IP_CT_DIR_ORIGINAL] = &spi->tuple[IP_CT_DIR_ORIGINAL];
		}

		//check IP_CT_DIR_REPLY
		if(!memcmp(&spi->tuple[IP_CT_DIR_REPLY].src.u3, &t->src.u3, sizeof(t->src.u3)))
		{
			if(spi->tuple[IP_CT_DIR_REPLY].dst.u.all == 0)
			{
				//this session is waiting for reply packet, and this packet may be its reply
				partial[IP_CT_DIR_REPLY] =  &spi->tuple[IP_CT_DIR_REPLY];
				copied_spi = spi;
			}

			if(spi->tuple[IP_CT_DIR_REPLY].dst.u.all == t->dst.u.all)
				perfact[IP_CT_DIR_REPLY] =  &spi->tuple[IP_CT_DIR_REPLY];
		}
	}

	//choose prefer one
	if(perfact[IP_CT_DIR_ORIGINAL])
	{
		//the tunnel is established, everything is good
		t->src.u.all = perfact[IP_CT_DIR_ORIGINAL]->src.u.all;
	}
	else if(partial[IP_CT_DIR_ORIGINAL])
	{
		//before reply received
	}
	else if(perfact[IP_CT_DIR_REPLY])
	{
		//the tunnel is established, everything is good
		t->src.u.all = perfact[IP_CT_DIR_REPLY]->src.u.all;
	}
	else if(partial[IP_CT_DIR_REPLY])
	{
		//the first reply packet for this connection
		struct nf_conntrack_tuple tmp_t[IP_CT_DIR_MAX];
		__be16 dst;

		//printk(LOC_FMT"packet tuple: " , LOC_ARG);
		//debug_dump_tuple(t);

		//here is trigger rule point, we need create new spi pair for real conntrack
		t->src.u.all = partial[IP_CT_DIR_REPLY]->src.u.all;

		dst = t->dst.u.all;
		t->dst.u.all = 0; //let this packet match ephemeral conntrack

		//copy tuple for new spi pair
		memcpy(&tmp_t[IP_CT_DIR_ORIGINAL] , &copied_spi->tuple[IP_CT_DIR_ORIGINAL] , sizeof(struct nf_conntrack_tuple));
		memcpy(&tmp_t[IP_CT_DIR_REPLY] , &copied_spi->tuple[IP_CT_DIR_REPLY] , sizeof(struct nf_conntrack_tuple));

		//leave protection section first, we need to allocate new entry, it may take a long time
		write_unlock_bh(&net_esp->lock);

		tmp_t[IP_CT_DIR_ORIGINAL].src.u.all = dst;
		tmp_t[IP_CT_DIR_REPLY].dst.u.all = dst;

		spi = kmalloc(sizeof(*spi), GFP_ATOMIC);
		if(spi)
		{
			spi->binding = NULL;
			memcpy(spi->tuple , tmp_t , 2 * sizeof(struct nf_conntrack_tuple));
			//printk(LOC_FMT"create: " , LOC_ARG);
			//debug_dump_tuple(&spi->tuple[IP_CT_DIR_ORIGINAL]);
			//debug_dump_tuple(&spi->tuple[IP_CT_DIR_REPLY]);
		}
		else
			rv = false;

		write_lock_bh(&net_esp->lock);
		//enter protection section

		if(spi)
			list_add_tail(&spi->list , &net_esp->spi_pair_list); //add this to spi pair list, it is real conntrack record
	}

	write_unlock_bh(&net_esp->lock);

	return rv;
}

//esp hdr info to tuple
static bool esp_pkt_to_tuple(const struct sk_buff *skb, unsigned int dataoff,
                             struct nf_conntrack_tuple *tuple)
{
	struct net *net = dev_net(skb->dev ? skb->dev : skb_dst(skb)->dev);
	const struct esp_hdr *esphdr;
	struct esp_hdr _esphdr;

	esphdr = skb_header_pointer(skb, dataoff, sizeof(_esphdr), &_esphdr);
	if(!esphdr)
	{
		//try to behave like "nf_conntrack_proto_generic"
		tuple->src.u.all = 0;
		tuple->dst.u.all = 0;
		return true;
	}

	tuple->dst.u.all = (__be16)(esphdr->spi & 0xffff);
	if(modify_tuple_by_spi_pair(net , tuple) == false)
	{
		//try to behave like "nf_conntrack_proto_generic"
		tuple->src.u.all = 0;
		tuple->dst.u.all = 0;
		return true;
	}

	return true;
}

/* print esp part of tuple */
static int esp_print_tuple(struct seq_file *s,
                           const struct nf_conntrack_tuple *tuple)
{
	return seq_printf(s, "srckey=0x%x dstkey=0x%x ",
	                  ntohs(tuple->src.u.all),
	                  ntohs(tuple->dst.u.all));
}

/* print private data for conntrack */
static int esp_print_conntrack(struct seq_file *s, const struct nf_conn *ct)
{
	return seq_printf(s, "timeout=%u, stream_timeout=%u ",
	                  (ct->proto.esp.timeout / HZ),
	                  (ct->proto.esp.stream_timeout / HZ));
	return 0;
}

// Returns verdict for packet, and may modify conntrack
static int esp_packet(struct nf_conn *ct,
                      const struct sk_buff *skb,
                      unsigned int dataoff,
                      enum ip_conntrack_info ctinfo,
                      u_int8_t pf,
                      unsigned int hooknum)
{
	//If we've seen traffic both ways, this is a IPSec connection.
	//Extend timeout.
	if(ct->status & IPS_SEEN_REPLY)
	{
		nf_ct_refresh_acct(ct, ctinfo, skb,
		                   ct->proto.esp.stream_timeout);
		//Also, more likely to be important, and not a probe.
		set_bit(IPS_ASSURED_BIT, &ct->status);
		nf_conntrack_event_cache(IPCT_STATUS, ct);
	}
	else
		nf_ct_refresh_acct(ct, ctinfo, skb,
		                   ct->proto.esp.timeout);

	return NF_ACCEPT;
}

//Called when a new connection for this protocol found.
static bool esp_new(struct nf_conn *ct, const struct sk_buff *skb,
                    unsigned int dataoff)
{
	if(spi_pair_add(ct) < 0)
		return false;

	pr_debug(": ");
	nf_ct_dump_tuple(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);

	ct->proto.esp.stream_timeout = nf_ct_esp_stream_timeout;
	ct->proto.esp.timeout = nf_ct_esp_timeout;

	return true;
}

static void esp_destroy(struct nf_conn *ct)
{
	pr_debug(" entering\n");

	if(!ct->proto.esp.spi)
		pr_debug("no spi !?!\n");
	else
		spi_pair_del(ct);
}

static unsigned int is_ike_traffic(const struct nf_conntrack_tuple *tuple , struct sk_buff *skb , unsigned int dataoff)
{
	//check protocol and port number first
	if(tuple->src.l3num != PF_INET ||
	        tuple->dst.protonum != IPPROTO_UDP ||
	        tuple->src.u.udp.port != htons(500) ||
	        tuple->dst.u.udp.port != htons(500))
		return 0;

	return 1;
}

static void dump_ike_cookie(unsigned char *cookie)
{
	printk("%02X %02X %02X %02X %02X %02X %02X %02X" ,
	       (unsigned int)cookie[0] & 0xff ,
	       (unsigned int)cookie[1] & 0xff ,
	       (unsigned int)cookie[2] & 0xff ,
	       (unsigned int)cookie[3] & 0xff ,
	       (unsigned int)cookie[4] & 0xff ,
	       (unsigned int)cookie[5] & 0xff ,
	       (unsigned int)cookie[6] & 0xff ,
	       (unsigned int)cookie[7] & 0xff);
}

static struct nf_conn_ike *nf_ct_ike_ext_add(struct nf_conn *ct , gfp_t gfp , struct sk_buff *skb , unsigned int dataoff)
{
	struct nf_conn_ike *ike_ext;
	struct ike_hdr _ikehdr;
	const struct ike_hdr *ikehdr;

	//check ike header
	ikehdr = (const struct ike_hdr*)skb_header_pointer(skb, dataoff + sizeof(struct udphdr), sizeof(_ikehdr), &_ikehdr);
	if(!ikehdr)
		return NULL;

	//we got ike header, extract cookie pair
	ike_ext = (struct nf_conn_ike*)nf_ct_ext_add(ct, NF_CT_EXT_IKE, gfp);
	if(!ike_ext)
		return NULL;

	ike_ext->timeout = 60 * 60 * 24 * HZ; //one day
	ike_ext->ct = ct;
	memcpy(ike_ext->i_cookie , ikehdr->i_cookie , 8);
	memcpy(ike_ext->r_cookie , ikehdr->r_cookie , 8);

	//printk(LOC_FMT"i cookie: " , LOC_ARG);
	//dump_ike_cookie(ike_ext->i_cookie);
	//printk(", r cookie: ");
	//dump_ike_cookie(ike_ext->r_cookie);
	//printk("\n");

	//put this to our list
	write_lock_bh(&ike_cookie_lock);
	list_add_tail(&ike_ext->list , &ike_cookie_list);
	write_unlock_bh(&ike_cookie_lock);

	return ike_ext;
}

static int update_cookie(unsigned char *dst , const unsigned char *src)
{
	int changed = 0;

	read_lock_bh(&ike_cookie_lock);
	if(memcmp(dst , src , 8) != 0)
		changed = 1;
	read_unlock_bh(&ike_cookie_lock);

	if(changed)
	{
		write_lock_bh(&ike_cookie_lock);
		memcpy(dst , src , 8);
		write_unlock_bh(&ike_cookie_lock);
	}

	return changed;
}

static void update_ike_cookie_pair(struct nf_conn *ct , struct sk_buff *skb , unsigned int dataoff)
{
	struct nf_conn_ike *ike_ext = (struct nf_conn_ike*)nf_ct_ext_find(ct , NF_CT_EXT_IKE);
	struct ike_hdr _ikehdr;
	const struct ike_hdr *ikehdr;
	int updated = 0;

	if(!ike_ext)
		return; //should not happen

	//check ike header
	ikehdr = (const struct ike_hdr*)skb_header_pointer(skb, dataoff + sizeof(struct udphdr), sizeof(_ikehdr), &_ikehdr);
	if(!ikehdr)
		return;

	//update cookies
	if(update_cookie(ike_ext->i_cookie , ikehdr->i_cookie))
		updated = 1;

	if(update_cookie(ike_ext->r_cookie , ikehdr->r_cookie))
		updated = 1;

	//if(updated)
	//{
	//	printk(LOC_FMT"update i cookie: " , LOC_ARG);
	//	dump_ike_cookie(ike_ext->i_cookie);
	//	printk(", r cookie: ");
	//	dump_ike_cookie(ike_ext->r_cookie);
	//	printk("\n");
	//}
}

enum {PERFACT , PARTIAL};
static int redirect_to_real_peer(struct nf_conntrack_tuple *tuple , const struct sk_buff *skb , unsigned int dataoff)
{
	struct nf_conn_ike *ike_ext;
	__be16 dst_port[2] = {0 , 0};
	struct ike_hdr _ikehdr;
	const struct ike_hdr *ikehdr;

	ikehdr = (const struct ike_hdr*)skb_header_pointer(skb, dataoff + sizeof(struct udphdr), sizeof(_ikehdr), &_ikehdr);
	if(!ikehdr)
		return 0; //no ike cookie, do nothing

	read_lock_bh(&ike_cookie_lock);

	list_for_each_entry(ike_ext , &ike_cookie_list , list)
	{
		if(nf_inet_addr_cmp(&tuple->src.u3 , &ike_ext->ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3))
		{
			//this packet is from remote peer, we must find its real destination
			//compare cookie pair
			if(memcmp(ike_ext->i_cookie , ikehdr->i_cookie , 8) == 0)
			{
				dst_port[PARTIAL] = ike_ext->ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;

				if(memcmp(ike_ext->r_cookie , ikehdr->r_cookie , 8) == 0)
					dst_port[PERFACT] = ike_ext->ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
			}
		}
	}

	read_unlock_bh(&ike_cookie_lock);

	if(dst_port[PERFACT])
	{
		//i cookie and r cookis are matched
		tuple->dst.u.udp.port = dst_port[PERFACT];
		return 1;
	}
	else if(dst_port[PARTIAL])
	{
		//only i cookie is matched, this is mean that this packet is the first reply packet
		tuple->dst.u.udp.port = dst_port[PARTIAL];
		return 1;
	}

	return 0;
}

static void nf_ike_cleanup_conntrack(struct nf_conn *ct)
{
	struct nf_conn_ike *ike_ext = (struct nf_conn_ike*)nf_ct_ext_find(ct , NF_CT_EXT_IKE);

	if(ike_ext == NULL)
		return;

	//printk(LOC_FMT"kill i cookie: " , LOC_ARG);
	//dump_ike_cookie(ike_ext->i_cookie);
	//printk(", r cookie: ");
	//dump_ike_cookie(ike_ext->r_cookie);
	//printk("\n");

	//remove ike cookie pair from db
	write_lock_bh(&ike_cookie_lock);
	list_del(&ike_ext->list);
	write_unlock_bh(&ike_cookie_lock);
}

static int proto_esp_net_init(struct net *net)
{
	int rv;
	struct netns_proto_esp *net_esp = kmalloc(sizeof(struct netns_proto_esp), GFP_KERNEL);
    if (!net_esp) return -ENOMEM;
	rwlock_init(&net_esp->lock);
	INIT_LIST_HEAD(&net_esp->spi_pair_list);
	rv = net_assign_generic(net, proto_esp_net_id, net_esp);
	if (rv < 0)  kfree(net_esp);
	else
	{
		rwlock_init(&ike_cookie_lock);
		INIT_LIST_HEAD(&ike_cookie_list);
	}
	return rv;
}

static void proto_esp_net_exit(struct net *net)
{
	struct netns_proto_esp *net_esp = net_generic(net, proto_esp_net_id);
	spi_pairs_flush(net);
	if(net_esp) kfree(net_esp);
}

static struct pernet_operations proto_esp_net_ops =
{
	.init = proto_esp_net_init ,
	.exit = proto_esp_net_exit ,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
	.id   = &proto_esp_net_id ,
	.size = sizeof(struct netns_proto_esp) ,
#endif
};

#ifdef CONFIG_SYSCTL
static unsigned int esp_sysctl_table_users;
static struct ctl_table_header *esp_sysctl_header;
static struct ctl_table esp_sysctl_table[] =
{
	{
		.procname       = "nf_conntrack_esp_timeout",
		.data           = &nf_ct_esp_timeout,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec_jiffies,
	},

	{
		.procname       = "nf_conntrack_esp_stream_timeout",
		.data           = &nf_ct_esp_stream_timeout,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec_jiffies,
	},
	{}
};
#endif //CONFIG_SYSCTL

/* protocol helper struct */
static struct nf_conntrack_l4proto nf_conntrack_l4proto_esp4 __read_mostly =
{
	.l3proto         = AF_INET ,
	.l4proto         = IPPROTO_ESP ,
	.name            = "esp" ,
	.pkt_to_tuple    = esp_pkt_to_tuple ,
	.invert_tuple    = esp_invert_tuple ,
	.print_tuple     = esp_print_tuple ,
	.print_conntrack = esp_print_conntrack ,
	.packet          = esp_packet ,
	.new             = esp_new ,
	.destroy         = esp_destroy ,
	.me              = THIS_MODULE ,

#ifdef CONFIG_SYSCTL
	.ctl_table_users        = &esp_sysctl_table_users,
	.ctl_table_header       = &esp_sysctl_header,
	.ctl_table              = esp_sysctl_table,
#endif //CONFIG_SYSCTL
};

static struct nf_ct_ext_type ike_extend __read_mostly =
{
	.len            = sizeof(struct nf_conn_ike),
	.align          = __alignof__(struct nf_conn_ike) ,
	.destroy        = nf_ike_cleanup_conntrack ,
	.move           = NULL ,
	.id             = NF_CT_EXT_IKE ,
	.flags          = NF_CT_EXT_F_PREALLOC ,
};

static int __init nf_conntrack_ipsec_init(void)
{
	int rv;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
	rv = register_pernet_subsys(&proto_esp_net_ops);
#else
	rv = register_pernet_gen_subsys(&proto_esp_net_id, &proto_esp_net_ops);
#endif
	if(rv < 0)
		goto out;
		
	rv = nf_conntrack_l4proto_register(&nf_conntrack_l4proto_esp4);
	if(rv < 0)
		goto out_proto_esp;

	rv = nf_ct_extend_register(&ike_extend);
	if(rv < 0)
		goto out_proto_esp4;

	BUG_ON(is_ike_traffic_hook != NULL);
	rcu_assign_pointer(is_ike_traffic_hook , is_ike_traffic);

	BUG_ON(nf_ct_ike_ext_add_hook != NULL);
	rcu_assign_pointer(nf_ct_ike_ext_add_hook , nf_ct_ike_ext_add);

	BUG_ON(update_ike_cookie_pair_hook != NULL);
	rcu_assign_pointer(update_ike_cookie_pair_hook , update_ike_cookie_pair);

	BUG_ON(redirect_to_real_peer_hook != NULL);
	rcu_assign_pointer(redirect_to_real_peer_hook , redirect_to_real_peer);

	return rv;

out_proto_esp4:
	nf_conntrack_l4proto_unregister(&nf_conntrack_l4proto_esp4);
out_proto_esp:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
	unregister_pernet_subsys(&proto_esp_net_ops);
#else
	unregister_pernet_gen_subsys(proto_esp_net_id, &proto_esp_net_ops);
#endif
out:
	return rv;
}

static void __exit nf_conntrack_ipsec_fini(void)
{
	rcu_assign_pointer(redirect_to_real_peer_hook , NULL);
	rcu_assign_pointer(update_ike_cookie_pair_hook , NULL);
	rcu_assign_pointer(nf_ct_ike_ext_add_hook , NULL);
	rcu_assign_pointer(is_ike_traffic_hook , NULL);
	synchronize_rcu();

	nf_ct_extend_unregister(&ike_extend);
	nf_conntrack_l4proto_unregister(&nf_conntrack_l4proto_esp4);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
	unregister_pernet_subsys(&proto_esp_net_ops);
#else
	unregister_pernet_gen_subsys(proto_esp_net_id, &proto_esp_net_ops);
#endif
}

module_init(nf_conntrack_ipsec_init);
module_exit(nf_conntrack_ipsec_fini);
