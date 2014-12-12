#include <linux/netfilter/nf_conntrack_qos.h>

static struct list_head	*ips_filter_list[IPS_FILTER_LIST_MAX];

/* TCP filter */
static IPS_FILTER	ips_tcp_ftpdata= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"ftp-data", 
	.u.tcp.port	=	20, 
};
static IPS_FILTER	ips_tcp_ftpctrl= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"ftp-ctrl", 
	.u.tcp.port	=	21, 
};
static IPS_FILTER	ips_tcp_ssh= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"ssh", 
	.u.tcp.port	=	22, 
};
static IPS_FILTER	ips_tcp_telnet= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"telnet", 
	.u.tcp.port	=	23, 
};
static IPS_FILTER	ips_tcp_smtp= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"smtp", 
	.u.tcp.port	=	25, 
};
static IPS_FILTER	ips_tcp_pop3= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"pop3", 
	.u.tcp.port	=	110, 
};
static IPS_FILTER	ips_tcp_imap= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"imap", 
	.u.tcp.port	=	143, 
};
static IPS_FILTER	ips_tcp_http1= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"http", 
	.u.tcp.port	=	80, 
};
static IPS_FILTER	ips_tcp_https= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"https", 
	.u.tcp.port	=	443, 
};
static IPS_FILTER	ips_tcp_msn= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"msn", 
	.u.tcp.port	=	1863, 
};
static IPS_FILTER	ips_tcp_http2= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"http", 
	.u.tcp.port	=	8080, 
};
static IPS_FILTER	ips_tcp_rtsp= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"rtsp", 
	.u.tcp.port	=	554, 
};
static IPS_FILTER	ips_tcp_pptp= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"pptp", 
	.u.tcp.port	=	1723, 
};
static IPS_FILTER	ips_tcp_socks= {
	.protonum		=	IPPROTO_TCP, 
	.name			=	"socks", 
	.u.tcp.port	=	1080, 
};

static IPS_FILTER	*ips_tcp_filter[] = {
	&ips_tcp_ftpdata,
	&ips_tcp_ftpctrl,
	&ips_tcp_ssh,
	&ips_tcp_telnet,
	&ips_tcp_smtp,
	&ips_tcp_pop3,
	&ips_tcp_imap,
	&ips_tcp_http1,
	&ips_tcp_https,
	&ips_tcp_msn,
	&ips_tcp_http2,
	&ips_tcp_rtsp,
	&ips_tcp_pptp,
	&ips_tcp_socks,
	NULL
	/* add more here */
};

/* UDP filter */
static IPS_FILTER	ips_udp_dns= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"dns", 
	.u.udp.port	=	53, 
};
static IPS_FILTER	ips_udp_dhcp= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"dhcp", 
	.u.udp.port	=	67, 
};
static IPS_FILTER	ips_udp_dhcpr= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"dhcp-reply", 
	.u.udp.port	=	68, 
};
static IPS_FILTER	ips_udp_ipsec= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"ipsec", 
	.u.udp.port	=	500, 
};
static IPS_FILTER	ips_udp_rtsp= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"rtsp", 
	.u.udp.port	=	554, 
};
static IPS_FILTER	ips_udp_l2tp= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"l2tp", 
	.u.udp.port	=	1701, 
};
static IPS_FILTER	ips_udp_qq= {
	.protonum		=	IPPROTO_UDP, 
	.name			=	"qq", 
	.u.udp.port	=	8000, 
};

static IPS_FILTER 	*ips_udp_filter[] = {
	&ips_udp_dns,
	&ips_udp_dhcp,
	&ips_udp_dhcpr,
	&ips_udp_ipsec,
	&ips_udp_rtsp,
	&ips_udp_l2tp,
	&ips_udp_qq,
	NULL
	/* add more here */
};

/* ICMP filter */
static IPS_FILTER	ips_icmp_echo= {
	.protonum		=	IPPROTO_ICMP, 
	.name			=	"echo", 
	.u.icmp.id	=	8, //??
};
static IPS_FILTER	ips_icmp_reply= {
	.protonum		=	IPPROTO_ICMP, 
	.name			=	"reply", 
	.u.icmp.id	=	0, //??
};

static IPS_FILTER	*ips_icmp_filter[] = {
	&ips_icmp_echo,
	&ips_icmp_reply,
	NULL
};


static inline int ips_hash(uint16_t port)
{
	return (port & (IPS_HASH_SIZE - 1));
}

static int ips_hash_insert(int proto, IPS_FILTER *entry)
{
	int hash;

	hash = ips_hash(entry->u.all);

	list_add(&entry->list, &ips_filter_list[proto][hash]);

	return 0;
}

static int ipsqoshash_proc_read(char *buffer, char **start, off_t offset, int len, int *eof, void *data)
{
	int index, count;
	IPS_FILTER *pos;
	
	printk("TCP filter hash table:\n");
	for (index=0; index < IPS_HASH_SIZE; index++) {
		count = 0;
		if(list_empty(&ips_filter_list[IPS_FILTER_TCP][index])) {
			printk(" ");
			continue;
		}
		list_for_each_entry(pos, &ips_filter_list[IPS_FILTER_TCP][index], list) {
			count++;
		}
		printk("%X", count);
	}
	printk("\n");

	printk("UDP filter hash table:\n");
	for (index=0; index < IPS_HASH_SIZE; index++) {
		count = 0;
		if(list_empty(&ips_filter_list[IPS_FILTER_UDP][index])) {
			printk(" ");
			continue;
		}
		list_for_each_entry(pos, &ips_filter_list[IPS_FILTER_UDP][index], list) {
			count++;
		}
		printk("%X", count);
	}
	printk("\n");
	return 0;
}

//marco, I have a little bit change the return value of this function
//In previous version, the return value of it is the prio and the vaiable keep 
//is use as call by reference
//Since we no need to check for qos, so change the return to keep the connection or not
//ret: 1, keep connection   0, drop it
int	ips_check_entry(const struct nf_conntrack_tuple *tp)
{
	/* Checking packet type by ips_qos filter */
	int	i=-1;
	int	hash=0;
	IPS_FILTER	*temp=NULL;

	uint16_t			protonum_4_layer=tp->dst.protonum;
	uint16_t			protonum_7_layer=0;
	
	switch(protonum_4_layer) 
	{
		case	IPPROTO_ICMP:
			goto	KEEP;
		case	IPPROTO_TCP:
			i=IPS_FILTER_TCP;
			protonum_7_layer=ntohs(tp->dst.u.tcp.port);
			break;
		case	IPPROTO_UDP:
			i=IPS_FILTER_UDP;
			protonum_7_layer=ntohs(tp->dst.u.udp.port);
			break;
		default:
			goto	END;
	}

	hash = ips_hash(protonum_7_layer);
	
	list_for_each_entry(temp, &ips_filter_list[i][hash], list) 
	{
		switch(i) 
		{
			case	IPS_FILTER_TCP:
				if(protonum_7_layer!=temp->u.tcp.port)
					continue;
				else
					goto	KEEP;
			case	IPS_FILTER_UDP:
				if(protonum_7_layer!=temp->u.udp.port)
					continue;
				else
					goto	KEEP;
		}
	}
	goto	END;
KEEP:
	return 1;
END:
	return 0;
}


void	ips_qos_initial(void)
{
	int		i, j;
	
	for(i=0; i<IPS_FILTER_LIST_MAX; i++) {
		ips_filter_list[i] = kmalloc(sizeof(struct list_head)*IPS_HASH_SIZE, GFP_ATOMIC);
		if(!ips_filter_list[i]) {
			printk("ips_qos: initial failed\n");
			return;
		}
	}

	for(i=0; i<IPS_FILTER_LIST_MAX; i++) {
		for(j=0; j<IPS_HASH_SIZE; j++)
			INIT_LIST_HEAD(&ips_filter_list[i][j]);
	}

	/* Register TCP filter */
	for(i=0; ips_tcp_filter[i]!=NULL; i++)
		ips_hash_insert(IPS_FILTER_TCP, ips_tcp_filter[i]);
	/* Register UDP filter */
	for(i=0; ips_udp_filter[i]!=NULL; i++)
		ips_hash_insert(IPS_FILTER_UDP, ips_udp_filter[i]);
	/* Register ICMP filter */
	for(i=0; ips_icmp_filter[i]!=NULL; i++)
		ips_hash_insert(IPS_FILTER_ICMP, ips_icmp_filter[i]);

	create_proc_read_entry("net/ips_hash", 0, NULL, ipsqoshash_proc_read, NULL);
}

EXPORT_SYMBOL(ips_check_entry);

