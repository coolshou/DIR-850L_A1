/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
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
#include <linux/seq_file.h>
#include <linux/percpu.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#endif

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_acct.h>


#if defined(CONFIG_RTL_819X)
#include <net/rtl/features/rtl_ps_hooks.h>
#endif

MODULE_LICENSE("GPL");

#ifdef CONFIG_PROC_FS
//ALpha
#define ALPHA_CONN_FLUSH 1
int
print_tuple(struct seq_file *s, const struct nf_conntrack_tuple *tuple,
            const struct nf_conntrack_l3proto *l3proto,
            const struct nf_conntrack_l4proto *l4proto)
{
	return l3proto->print_tuple(s, tuple) || l4proto->print_tuple(s, tuple);
}
EXPORT_SYMBOL_GPL(print_tuple);

struct ct_iter_state {
	struct seq_net_private p;
	unsigned int bucket;
};

static struct hlist_nulls_node *ct_get_first(struct seq_file *seq)
{
	struct net *net = seq_file_net(seq);
	struct ct_iter_state *st = seq->private;
	struct hlist_nulls_node *n;

	for (st->bucket = 0;
	     st->bucket < nf_conntrack_htable_size;
	     st->bucket++) {
		n = rcu_dereference(net->ct.hash[st->bucket].first);
		if (!is_a_nulls(n))
			return n;
	}
	return NULL;
}

static struct hlist_nulls_node *ct_get_next(struct seq_file *seq,
				      struct hlist_nulls_node *head)
{
	struct net *net = seq_file_net(seq);
	struct ct_iter_state *st = seq->private;

	head = rcu_dereference(head->next);
	while (is_a_nulls(head)) {
		if (likely(get_nulls_value(head) == st->bucket)) {
			if (++st->bucket >= nf_conntrack_htable_size)
				return NULL;
		}
		head = rcu_dereference(net->ct.hash[st->bucket].first);
	}
	return head;
}

static struct hlist_nulls_node *ct_get_idx(struct seq_file *seq, loff_t pos)
{
	struct hlist_nulls_node *head = ct_get_first(seq);

	if (head)
		while (pos && (head = ct_get_next(seq, head)))
			pos--;
	return pos ? NULL : head;
}

static void *ct_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(RCU)
{
	rcu_read_lock();
	return ct_get_idx(seq, *pos);
}

static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return ct_get_next(s, v);
}

static void ct_seq_stop(struct seq_file *s, void *v)
	__releases(RCU)
{
	rcu_read_unlock();
}

/* return 0 on success, 1 in case of error */
static int ct_seq_show(struct seq_file *s, void *v)
{
	struct nf_conntrack_tuple_hash *hash = v;
	struct nf_conn *ct = nf_ct_tuplehash_to_ctrack(hash);
	const struct nf_conntrack_l3proto *l3proto;
	const struct nf_conntrack_l4proto *l4proto;
	//hyking add for hw use
	int ret = 0;

	NF_CT_ASSERT(ct);
	if (unlikely(!atomic_inc_not_zero(&ct->ct_general.use)))
		return 0;

	/* we only want to print DIR_ORIGINAL */
	if (NF_CT_DIRECTION(hash))
		goto release;

	l3proto = __nf_ct_l3proto_find(nf_ct_l3num(ct));
	NF_CT_ASSERT(l3proto);
	l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
	NF_CT_ASSERT(l4proto);

	ret = -ENOSPC;
	//hyking add for hw use
	#if defined(CONFIG_RTL_819X)
	if(rtl_ct_seq_show_hooks(s,ct)==RTL_PS_HOOKS_BREAK)
		goto release;		
	#endif
	
	if (seq_printf(s, "%-8s %u %-8s %u %ld ",
		       l3proto->name, nf_ct_l3num(ct),
		       l4proto->name, nf_ct_protonum(ct),
		       timer_pending(&ct->timeout)
		       ? (long)(ct->timeout.expires - jiffies)/HZ : 0) != 0)
		goto release;

	if (l4proto->print_conntrack && l4proto->print_conntrack(s, ct))
		goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
			l3proto, l4proto))
		goto release;

	if (seq_print_acct(s, ct, IP_CT_DIR_ORIGINAL))
		goto release;

	if (!(test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
		if (seq_printf(s, "[UNREPLIED] "))
			goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_REPLY].tuple,
			l3proto, l4proto))
		goto release;

	if (seq_print_acct(s, ct, IP_CT_DIR_REPLY))
		goto release;

	if (test_bit(IPS_ASSURED_BIT, &ct->status))
		if (seq_printf(s, "[ASSURED] "))
			goto release;

#if defined(CONFIG_NF_CONNTRACK_MARK)
	if (seq_printf(s, "mark=%u ", ct->mark))
		goto release;
#endif

#ifdef CONFIG_NF_CONNTRACK_SECMARK
	if (seq_printf(s, "secmark=%u ", ct->secmark))
		goto release;
#endif


#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7) || defined(CONFIG_NETFILTER_XT_MATCH_LAYER7_MODULE)
	if(ct->layer7.app_proto &&
           seq_printf(s, "l7proto=%s ", ct->layer7.app_proto))
		return -ENOSPC;
#endif

	if (seq_printf(s, "use=%u\n", atomic_read(&ct->ct_general.use)))
		goto release;

	ret = 0;
release:
	nf_ct_put(ct);
	return 0;
}

static const struct seq_operations ct_seq_ops = {
	.start = ct_seq_start,
	.next  = ct_seq_next,
	.stop  = ct_seq_stop,
	.show  = ct_seq_show
};

static int ct_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ct_seq_ops,
			sizeof(struct ct_iter_state));
}

static const struct file_operations ct_file_ops = {
	.owner   = THIS_MODULE,
	.open    = ct_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

static void *ct_cpu_seq_start(struct seq_file *seq, loff_t *pos)
{
	struct net *net = seq_file_net(seq);
	int cpu;

	if (*pos == 0)
		return SEQ_START_TOKEN;

	for (cpu = *pos-1; cpu < nr_cpu_ids; ++cpu) {
		if (!cpu_possible(cpu))
			continue;
		*pos = cpu + 1;
		return per_cpu_ptr(net->ct.stat, cpu);
	}

	return NULL;
}

static void *ct_cpu_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct net *net = seq_file_net(seq);
	int cpu;

	for (cpu = *pos; cpu < nr_cpu_ids; ++cpu) {
		if (!cpu_possible(cpu))
			continue;
		*pos = cpu + 1;
		return per_cpu_ptr(net->ct.stat, cpu);
	}

	return NULL;
}

static void ct_cpu_seq_stop(struct seq_file *seq, void *v)
{
}

static int ct_cpu_seq_show(struct seq_file *seq, void *v)
{
	struct net *net = seq_file_net(seq);
	unsigned int nr_conntracks = atomic_read(&net->ct.count);
	const struct ip_conntrack_stat *st = v;

	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "entries  searched found new invalid ignore delete delete_list insert insert_failed drop early_drop icmp_error  expect_new expect_create expect_delete\n");
		return 0;
	}

	seq_printf(seq, "%08x  %08x %08x %08x %08x %08x %08x %08x "
			"%08x %08x %08x %08x %08x  %08x %08x %08x \n",
		   nr_conntracks,
		   st->searched,
		   st->found,
		   st->new,
		   st->invalid,
		   st->ignore,
		   st->delete,
		   st->delete_list,
		   st->insert,
		   st->insert_failed,
		   st->drop,
		   st->early_drop,
		   st->error,

		   st->expect_new,
		   st->expect_create,
		   st->expect_delete
		);
	return 0;
}

static const struct seq_operations ct_cpu_seq_ops = {
	.start	= ct_cpu_seq_start,
	.next	= ct_cpu_seq_next,
	.stop	= ct_cpu_seq_stop,
	.show	= ct_cpu_seq_show,
};

static int ct_cpu_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ct_cpu_seq_ops,
			    sizeof(struct seq_net_private));
}

static const struct file_operations ct_cpu_seq_fops = {
	.owner	 = THIS_MODULE,
	.open	 = ct_cpu_seq_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = seq_release_net,
};
//Alpha
#ifdef ALPHA_CONN_FLUSH
//#define FLUSH_ALL_WITHOUT_LOCAL 
//joel flush the icmp only.that will have less side effect.
//in FLUSH_ALL_WITHOUT_LOCAL will cause the pptp something wrong.(the gre init by server,will flushed after wan up,then the traffic fail.)
#define FLUSH_ICMP_ONLY
//struct para_struct
//{
//	char interface[1024];
//	struct net *net;
//};
//static int g_local_ip[128];
//static int g_index=0;
//marco, I have implemnet servel condition to flush conntrack
//EXCLUDE_INTF: echo interface name, we flush all traffic exclude those from interface which echo in
//FLUSH_ALL_WITHOUT_LOCAL: flush all traffic without from local
//FLUSH_UNREPLIED: only flush conntrack which states is unreplied
//and flush all conntrack except pptp and l2tp control port and gre protcol
struct parameter
{
	unsigned int proto_num;
	unsigned int ip;
	unsigned int port;
	unsigned int status;
	struct net *net;
};
struct proto_table
{
	char proto[8];
	int proto_num;
}PROTO_TABLE[]={{"IP",IPPROTO_IP},{"ICMP",IPPROTO_ICMP},\
{"TCP",IPPROTO_TCP},{"UDP",IPPROTO_UDP},{"GRE",IPPROTO_GRE},\
{"IPV6",IPPROTO_IPV6},{"RAW",IPPROTO_RAW}};
enum{ALPHA_PROTO_NUM,ALPHA_IP,ALPHA_PORT,ALPHA_STATUS};
int match(struct nf_conn *i,int condition,int parameter)
{
	if(parameter==0)
		return 1;//ignore this condition
	switch(condition)
	{
		case ALPHA_PROTO_NUM:
			if(i->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum==parameter)
				return 1;
			else
				break;
		case ALPHA_IP:
			if(i->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip==parameter)
				return 1;
			else	
				break;
		case ALPHA_PORT:
			if(ntohs(i->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all)==parameter)
				return 1;
			else
				break;
		case ALPHA_STATUS:
			if(parameter==IPS_SEEN_REPLY_BIT)
			{
				if (!(test_bit(IPS_SEEN_REPLY_BIT, &i->status)))//status is unreply
					return 1;
				else
					return 0;
			}
			else if(parameter==IPS_ASSURED_BIT)
			{
				if (test_bit(IPS_ASSURED_BIT, &i->status))
					return 1;
				else
					return 0;	
			}
			else
				break;
		default:
				break;
	}
	return 0;//doesn't match
	
}
static int kill_all(struct nf_conn *i, void *data)
{
	struct parameter *para=data;

	if( !(para->port==0 && para->proto_num==0 && para->ip==0 && para->status==0) )
	{
		if( match(i,ALPHA_PROTO_NUM,para->proto_num) &&
			match(i,ALPHA_IP,para->ip) &&
			match(i,ALPHA_PORT,para->port) &&
			match(i,ALPHA_STATUS,para->status) )
			return 1;//flush conntrack if match all condition
		else
			return 0;
	}
	else
	{
		if (i->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum==IPPROTO_ICMP)
			return 1;
		else 
			return 0;
	}
}
int parse_status(char *buffer,struct parameter *para)
{
	if(!strncmp(buffer,"UNREPLIED",strlen("UNREPLIED")) )
	{
		para->status=IPS_SEEN_REPLY_BIT;
		return strlen("UNREPLIED");
	}
	else if(!strncmp(buffer,"ASSURED",strlen("ASSURED")) )
	{
		para->status=IPS_ASSURED_BIT;
		return strlen("ASSURED");
	}
	return 0;
}
int parse_port(char *buffer,struct parameter *para)
{
	int offset=0;
	char *tmp=strstr(buffer," ");
	if(tmp!=NULL)
	{
		offset=tmp-buffer;
		tmp=0;
	}
	para->port=simple_strtol(buffer,NULL,10);
	
	return offset;
}
int parse_ip(char *buffer,struct parameter *para)
{
	char *dot=NULL;
	char *tmp=&buffer[0];
	int i=0;
	int offset=0;
	int test=0;
	char *last=strstr(buffer," ");
	if(last==NULL)
		last=strstr(buffer,"\0");

	while(1)
	{
		offset=dot-buffer;
		dot=strsep(&tmp,".");
		if(dot==NULL)
		{
			if(last)
				offset=last-buffer;		
			return offset;
		}
		test=simple_strtol(dot,NULL,10);
		((unsigned char *)&para->ip)[i]=test;
		
		i++;
	}
}
int parse_proto(char *buffer,struct parameter *para)
{
	int i=0;
	int support_proto=sizeof(PROTO_TABLE)/sizeof(struct proto_table );
	for(i=0;i<support_proto;i++)
	{
		if(!strncmp(buffer,PROTO_TABLE[i].proto,strlen(PROTO_TABLE[i].proto) ) )
		{
			para->proto_num=PROTO_TABLE[i].proto_num;
			return strlen(PROTO_TABLE[i].proto);
		}	
	}
	return 0;
}
void parse_parameter(char *buffer,struct parameter *para)
{
	char *tmp=NULL;
	int offset=0;
	
	tmp=strstr(buffer,"--proto");
	if(tmp!=NULL)
	{
		offset=parse_proto(tmp+8,para);
	}
	
	tmp=strstr(buffer+offset,"--port");
	if(tmp!=NULL)
	{
		offset+=parse_port(tmp+7,para);	
	}
	
	tmp=strstr(buffer+offset,"--status");
	if(tmp!=NULL)
	{
		offset+=parse_status(tmp+9,para);
	}	
	
	tmp=strstr(buffer+offset,"--ip");
	if(tmp!=NULL)
	{
		offset+=parse_ip(tmp+5,para);
	}
}
int alpha_nf_conntrack_flush(struct file *file, const char __user *buffer,
                           unsigned long count, void *data)
{
	//--proto TCP --port 12 --status UNREPLIED --ip 192.168.0.100
	//1.parse parameter to array first
	//2.if parmeter is null , that means no need to filter this condition
	//3.flush if all condition match
	struct net *net=data;
	struct parameter para;
	char *tmp=NULL;
	
	tmp=kmalloc(count,GFP_ATOMIC);
	if(tmp==NULL)
		return count;
	memset(&para,0x0,sizeof(struct parameter) );
	if( copy_from_user(tmp,buffer,count) )
	{
		kfree(tmp);
		return count;
	}
	
	parse_parameter(tmp,&para);

	para.net=net;
	nf_ct_iterate_cleanup(net,kill_all, &para);
	kfree(tmp);
	return count;
}
int nf_conntrack_flush_show_usage(char *page, char **start, off_t off,
                            int count, int *eof, void *data)
{
	int i=0;
	char *str=page;
	int support_proto=sizeof(PROTO_TABLE)/sizeof(struct proto_table );
	
	str+=sprintf(str,"The command should be used in following ways,\n");
	str+=sprintf(str,"echo -n \"--proto TCP --port 12 --status UNREPLIED --ip 192.168.0.100\" > /proc/nf_conntrack_flush\n");
	str+=sprintf(str,"The protocol we supported are:");
	for(i=0;i<support_proto;i++)
	{
		str+=sprintf(str,"%s ",PROTO_TABLE[i].proto);
	}
	str+=sprintf(str,"\n And the status we only supported UNREPLIED/ASSURED\n");
	str+=sprintf(str,"You can skip one or more condition but please use it in order\n");
	str+=sprintf(str,"e.g.echo -n \"--proto TCP --ip 192.168.0.100\" > /proc/nf_conntrack_flush\necho -n \"--port 12 --ip 192.168.0.100\" > /proc/nf_conntrack_flush\n");
	str+=sprintf(str,"echo -n \"--proto TCP --status ASSURED\" > /proc/nf_conntrack_flush\n");
	return str-page;
}

void alpha_conntrack_flush_proc_init(struct net *net)
{
	struct proc_dir_entry *proc_flu;
	proc_flu = create_proc_entry("nf_conntrack_flush", 0644, NULL);
	if (proc_flu)
	{
		proc_flu ->write_proc = alpha_nf_conntrack_flush;
		proc_flu ->read_proc = nf_conntrack_flush_show_usage;
		proc_flu ->data = net;
	}
}
#endif

static int nf_conntrack_standalone_init_proc(struct net *net)
{
	struct proc_dir_entry *pde;

	pde = proc_net_fops_create(net, "nf_conntrack", 0440, &ct_file_ops);
	if (!pde)
		goto out_nf_conntrack;

	pde = proc_create("nf_conntrack", S_IRUGO, net->proc_net_stat,
			  &ct_cpu_seq_fops);
	if (!pde)
		goto out_stat_nf_conntrack;
//Alpha		
#ifdef ALPHA_CONN_FLUSH
	alpha_conntrack_flush_proc_init(net);
#endif

	return 0;

out_stat_nf_conntrack:
	proc_net_remove(net, "nf_conntrack");
out_nf_conntrack:
	return -ENOMEM;
}

static void nf_conntrack_standalone_fini_proc(struct net *net)
{
	remove_proc_entry("nf_conntrack", net->proc_net_stat);
	proc_net_remove(net, "nf_conntrack");
}
#else
static int nf_conntrack_standalone_init_proc(struct net *net)
{
	return 0;
}

static void nf_conntrack_standalone_fini_proc(struct net *net)
{
}
#endif /* CONFIG_PROC_FS */

/* Sysctl support */

#ifdef CONFIG_SYSCTL
/* Log invalid packets of a given protocol */
static int log_invalid_proto_min = 0;
static int log_invalid_proto_max = 255;

static struct ctl_table_header *nf_ct_netfilter_header;

#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
extern unsigned int conntrack_min;
extern unsigned int conntrack_max;
extern unsigned int prot_limit[];

extern int conntrack_dointvec(ctl_table *table, int write, struct file *filp,
		     void *buffer, size_t *lenp, loff_t *ppos);
extern int conntrack_dointvec_minmax(ctl_table *table, int write, struct file *filp,
		     void *buffer, size_t *lenp, loff_t *ppos);
#endif 


static ctl_table nf_ct_sysctl_table[] = {
#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
	{
		.ctl_name	= NET_NF_CONNTRACK_MAX,
		.procname	= "nf_conntrack_max",
		.data		= &nf_conntrack_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &conntrack_dointvec,
	},
#else
	{
		.ctl_name	= NET_NF_CONNTRACK_MAX,
		.procname	= "nf_conntrack_max",
		.data		= &nf_conntrack_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
#endif
	{
		.ctl_name	= NET_NF_CONNTRACK_COUNT,
		.procname	= "nf_conntrack_count",
		.data		= &init_net.ct.count,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name       = NET_NF_CONNTRACK_BUCKETS,
		.procname       = "nf_conntrack_buckets",
		.data           = &nf_conntrack_htable_size,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0444,
		.proc_handler   = proc_dointvec,
	},
	{
		.ctl_name	= NET_NF_CONNTRACK_CHECKSUM,
		.procname	= "nf_conntrack_checksum",
		.data		= &init_net.ct.sysctl_checksum,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= NET_NF_CONNTRACK_LOG_INVALID,
		.procname	= "nf_conntrack_log_invalid",
		.data		= &init_net.ct.sysctl_log_invalid,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.strategy	= sysctl_intvec,
		.extra1		= &log_invalid_proto_min,
		.extra2		= &log_invalid_proto_max,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_expect_max",
		.data		= &nf_ct_expect_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
#if defined(CONFIG_RTL_CONNTRACK_GARBAGE_NEW)
	{ 
		.ctl_name	= NET_NF_CONNTRACK_GARBAGE_TCP,
		.procname	= "nf_conntrack_tcp",
		.data		= &prot_limit[PROT_TCP],
		.maxlen		= sizeof(prot_limit[PROT_TCP]), 
		.mode		= 0644,
		.proc_handler	= &conntrack_dointvec_minmax,
		.extra1		= &conntrack_min,
		.extra2		= &conntrack_max,
	},
	{ 
		.ctl_name	= NET_NF_CONNTRACK_GARBAGE_UDP,
		.procname	= "nf_conntrack_udp",
		.data		= &prot_limit[PROT_UDP],
		.maxlen		= sizeof(prot_limit[PROT_UDP]), 
		.mode		= 0644,
		.proc_handler	= &conntrack_dointvec_minmax,
		.extra1		= &conntrack_min,
		.extra2		= &conntrack_max,
	},
#endif

	{ .ctl_name = 0 }
};

#define NET_NF_CONNTRACK_MAX 2089

static ctl_table nf_ct_netfilter_table[] = {
	{
		.ctl_name	= NET_NF_CONNTRACK_MAX,
		.procname	= "nf_conntrack_max",
		.data		= &nf_conntrack_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
#if defined(CONFIG_RTL_NF_CONNTRACK_GARBAGE_NEW)
		.proc_handler	= &conntrack_dointvec,
#else
		.proc_handler	= proc_dointvec,
#endif
	},
	{ .ctl_name = 0 }
};

static struct ctl_path nf_ct_path[] = {
	{ .procname = "net", .ctl_name = CTL_NET, },
	{ }
};

static int nf_conntrack_standalone_init_sysctl(struct net *net)
{
	struct ctl_table *table;

	if (net_eq(net, &init_net)) {
		nf_ct_netfilter_header =
		       register_sysctl_paths(nf_ct_path, nf_ct_netfilter_table);
		if (!nf_ct_netfilter_header)
			goto out;
	}

	table = kmemdup(nf_ct_sysctl_table, sizeof(nf_ct_sysctl_table),
			GFP_KERNEL);
	if (!table)
		goto out_kmemdup;

	table[1].data = &net->ct.count;
	table[3].data = &net->ct.sysctl_checksum;
	table[4].data = &net->ct.sysctl_log_invalid;

	net->ct.sysctl_header = register_net_sysctl_table(net,
					nf_net_netfilter_sysctl_path, table);
	if (!net->ct.sysctl_header)
		goto out_unregister_netfilter;

	return 0;

out_unregister_netfilter:
	kfree(table);
out_kmemdup:
	if (net_eq(net, &init_net))
		unregister_sysctl_table(nf_ct_netfilter_header);
out:
	printk("nf_conntrack: can't register to sysctl.\n");
	return -ENOMEM;
}

static void nf_conntrack_standalone_fini_sysctl(struct net *net)
{
	struct ctl_table *table;

	if (net_eq(net, &init_net))
		unregister_sysctl_table(nf_ct_netfilter_header);
	table = net->ct.sysctl_header->ctl_table_arg;
	unregister_net_sysctl_table(net->ct.sysctl_header);
	kfree(table);
}
#else
static int nf_conntrack_standalone_init_sysctl(struct net *net)
{
	return 0;
}

static void nf_conntrack_standalone_fini_sysctl(struct net *net)
{
}
#endif /* CONFIG_SYSCTL */

static int nf_conntrack_net_init(struct net *net)
{
	int ret;

	ret = nf_conntrack_init(net);
	if (ret < 0)
		goto out_init;
	ret = nf_conntrack_standalone_init_proc(net);
	if (ret < 0)
		goto out_proc;
	net->ct.sysctl_checksum = 1;
	net->ct.sysctl_log_invalid = 0;
	ret = nf_conntrack_standalone_init_sysctl(net);
	if (ret < 0)
		goto out_sysctl;
	return 0;

out_sysctl:
	nf_conntrack_standalone_fini_proc(net);
out_proc:
	nf_conntrack_cleanup(net);
out_init:
	return ret;
}

static void nf_conntrack_net_exit(struct net *net)
{
	nf_conntrack_standalone_fini_sysctl(net);
	nf_conntrack_standalone_fini_proc(net);
	nf_conntrack_cleanup(net);
}

static struct pernet_operations nf_conntrack_net_ops = {
	.init = nf_conntrack_net_init,
	.exit = nf_conntrack_net_exit,
};

static int __init nf_conntrack_standalone_init(void)
{
	return register_pernet_subsys(&nf_conntrack_net_ops);
}

static void __exit nf_conntrack_standalone_fini(void)
{
	unregister_pernet_subsys(&nf_conntrack_net_ops);
}

module_init(nf_conntrack_standalone_init);
module_exit(nf_conntrack_standalone_fini);

/* Some modules need us, but don't depend directly on any symbol.
   They should call this. */
void need_conntrack(void)
{
}
EXPORT_SYMBOL_GPL(need_conntrack);
