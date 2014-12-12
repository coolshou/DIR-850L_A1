/*
 *  Point-to-Point Tunneling Protocol for Linux
 *
 *	Authors: Kozlov D. (xeb@mail.ru)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 */

/*  pptp-accel.c is ported from accel-pptp open source.
	Although kernel 2.6.37 above start support pptp,
	but it will crash sometimes. Root cause is still not found.
	This version is ok. Hope someone someday can fix the pptp.c.
	
	BTW, I remove the buf_work & ack_work, they are for pptp gre ack,
	we actually no need to maintain the ack, since tcp/ip or application
	layer will cover that.
	
	siyou 2011/8/24 12:59pm
*/
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/if_pppox.h>
#include <linux/notifier.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/workqueue.h>
#include <linux/version.h>

#include <net/sock.h>
#include <net/protocol.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/route.h>
#include <asm/uaccess.h>

#define PPTP_DRIVER_VERSION "0.7"

MODULE_DESCRIPTION("Point-to-Point Tunneling Protocol for Linux");
MODULE_AUTHOR("Kozlov D. (xeb@mail.ru)");
MODULE_LICENSE("GPL");

static int log_level=1;
static int min_window=5;
static int max_window=100;
module_param(min_window,int,0644);
MODULE_PARM_DESC(min_window,"Minimum sliding window size (default=5)");
module_param(max_window,int,0644);
MODULE_PARM_DESC(max_window,"Maximum sliding window size (default=100)");
module_param(log_level,int,0644);

static struct proc_dir_entry* proc_dir;

#define HASH_SIZE  16
#define HASH(addr) ((addr^(addr>>4))&0xF)
static DEFINE_RWLOCK(chan_lock);
static struct pppox_sock *chans[HASH_SIZE];

static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb);
//static int read_proc(char *page, char **start, off_t off,int count,
//                     int *eof, void *data);
static int __pptp_rcv(struct pppox_sock *po,struct sk_buff *skb,int new);

static struct ppp_channel_ops pptp_chan_ops= {
	.start_xmit = pptp_xmit,
};


/* gre header structure: -------------------------------------------- */

#define PPTP_GRE_PROTO  0x880B
#define PPTP_GRE_VER    0x1

#define PPTP_GRE_FLAG_C	0x80
#define PPTP_GRE_FLAG_R	0x40
#define PPTP_GRE_FLAG_K	0x20
#define PPTP_GRE_FLAG_S	0x10
#define PPTP_GRE_FLAG_A	0x80

#define PPTP_GRE_IS_C(f) ((f)&PPTP_GRE_FLAG_C)
#define PPTP_GRE_IS_R(f) ((f)&PPTP_GRE_FLAG_R)
#define PPTP_GRE_IS_K(f) ((f)&PPTP_GRE_FLAG_K)
#define PPTP_GRE_IS_S(f) ((f)&PPTP_GRE_FLAG_S)
#define PPTP_GRE_IS_A(f) ((f)&PPTP_GRE_FLAG_A)

struct pptp_gre_header {
  u_int8_t flags;		/* bitfield */
  u_int8_t ver;			/* should be PPTP_GRE_VER (enhanced GRE) */
  u_int16_t protocol;		/* should be PPTP_GRE_PROTO (ppp-encaps) */
  u_int16_t payload_len;	/* size of ppp payload, not inc. gre header */
  u_int16_t call_id;		/* peer's call_id for this session */
  u_int32_t seq;		/* sequence number.  Present if S==1 */
  u_int32_t ack;		/* seq number of highest packet recieved by */
  				/*  sender in this session */
};

struct gre_statistics {
  /* statistics for GRE receive */
  unsigned int rx_accepted;  // data packet accepted
  unsigned int rx_lost;      // data packet did not arrive before timeout
  unsigned int rx_underwin;  // data packet was under window (arrived too late
                             // or duplicate packet)
  unsigned int rx_buffered;  // data packet arrived earlier than expected,
                             // packet(s) before it were lost or reordered
  unsigned int rx_errors;    // OS error on receive
  unsigned int rx_truncated; // truncated packet
  unsigned int rx_invalid;   // wrong protocol or invalid flags
  unsigned int rx_acks;      // acknowledgement only

  /* statistics for GRE transmit */
  unsigned int tx_sent;      // data packet sent
  unsigned int tx_failed;    //
  unsigned int tx_acks;      // sent packet with just ACK

  __u32 pt_seq;
  struct timeval pt_time;
  unsigned int rtt;
};

static struct pppox_sock * lookup_chan(__u16 call_id)
{
	struct pppox_sock *po;
	read_lock_bh(&chan_lock);
	for(po=chans[HASH(call_id)]; po; po=po->next)
		if (po->proto.pptp.src_addr.call_id==call_id)
			break;
	read_unlock_bh(&chan_lock);
	return po;
}

static void add_chan(struct pppox_sock *po)
{
	write_lock_bh(&chan_lock);
	po->next=chans[HASH(po->proto.pptp.src_addr.call_id)];
	chans[HASH(po->proto.pptp.src_addr.call_id)]=po;
	write_unlock_bh(&chan_lock);
}

static void add_free_chan(struct pppox_sock *po)
{
	__u16 call_id=1;
	
	static __u16 call_id_used=0;	
	
	struct pppox_sock *p;

	write_lock_bh(&chan_lock);
#if 0	
	for(call_id=1; call_id<65535; call_id++) {
#else
	if(call_id_used >= 65535) call_id_used=0;
	call_id_used++;
	for(call_id=call_id_used; call_id<65535; call_id++) {
#endif
		for(p=chans[HASH(call_id)]; p; p=p->next)
			if (p->proto.pptp.src_addr.call_id==call_id)
				break;
		if (!p){
			po->proto.pptp.src_addr.call_id=call_id;
			po->next=chans[HASH(call_id)];
			chans[HASH(call_id)]=po;
			break;
		}
	}
	write_unlock_bh(&chan_lock);
}

static void del_chan(struct pppox_sock *po)
{
	struct pppox_sock *p1,*p2;
	write_lock_bh(&chan_lock);
	for(p2=NULL,p1=chans[HASH(po->proto.pptp.src_addr.call_id)]; p1 && p1!=po;
				p2=p1,p1=p1->next);
	if (p2) p2->next=p1->next;
	else chans[HASH(po->proto.pptp.src_addr.call_id)]=p1->next;
	write_unlock_bh(&chan_lock);
}

void dump(unsigned char *data, int len)
{
int i;

printk("ptr=%p, len=%d\n", data, len);
if (len > 96 ) len = 96;

	for(i=0; i < len; i++)
	{
		printk("%02x ", data[i]);
		if ((i+1)%16 == 0) printk("\n");
	}
printk("\n");
}

#define LOC_FMT "[%s %d %s] "
#define LOC_ARG __FILE__ , __LINE__ , __FUNCTION__
static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct sock *sk = (struct sock *) chan->private;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	struct pptp_gre_header *hdr;
	unsigned int header_len=sizeof(*hdr);
	int len=skb?skb->len:0;
	int err=0;

	struct rtable *rt;     			/* Route to the other host */
	struct net_device *tdev;			/* Device to other host */
	struct iphdr  *iph;			/* Our new IP header */
	int    max_headroom;			/* The extra header space needed */

//printk("skb->len = %d\n", len);

//	if (skb && opt->seq_sent-opt->ack_recv>opt->window){
//		opt->pause=1;
//		return 0;
//	}

	{
		struct flowi fl = { .oif = 0,
				    .nl_u = { .ip4_u =
					      { .daddr = opt->dst_addr.sin_addr.s_addr,
						.saddr = opt->src_addr.sin_addr.s_addr,
						.tos = RT_TOS(0) } },
				    .proto = IPPROTO_GRE };
		if ((err=ip_route_output_key(&init_net,&rt, &fl))) {
			goto tx_error;
		}
	}

	//for pptp loop issue, cause by not suitable routing rule (tom, 20111011)
	if((skb->dev != NULL) &&
	    skb->dev->ifindex == rt->u.dst.dev->ifindex)
	{
		printk(LOC_FMT"(drop) source interface and destination interface is the same, loop occurs, check the routing table\n" , LOC_ARG);
		goto tx_error;
	}

	tdev = rt->u.dst.dev;

	max_headroom = LL_RESERVED_SPACE(tdev) + sizeof(*iph)+sizeof(*hdr)+2;

	if (!skb){
		skb=dev_alloc_skb(max_headroom);
		skb_reserve(skb,max_headroom-skb_headroom(skb));
	}else if (skb_headroom(skb) < max_headroom ||
						skb_cloned(skb) || skb_shared(skb)) {
		struct sk_buff *new_skb = skb_realloc_headroom(skb, max_headroom);
		if (!new_skb) {
			ip_rt_put(rt);
			goto tx_error;
		}
		if (skb->sk)
			skb_set_owner_w(new_skb, skb->sk);
		dev_kfree_skb(skb);
		skb = new_skb;
	}

	if (skb->len){
		int islcp;
		unsigned char *data=skb->data;
		islcp=((data[0] << 8) + data[1])== PPP_LCP && 1 <= data[2] && data[2] <= 7;
		if (islcp) {
			data=skb_push(skb,2);
			data[0]=0xff;
			data[1]=0x03;
		}
	}
	len=skb->len;

//printk("2. skb->len = %d\n", len);

	if (len==0) header_len-=sizeof(hdr->seq);
	if (opt->ack_sent == opt->seq_recv) header_len-=sizeof(hdr->ack);

	//skb->nh.raw = skb_push(skb, sizeof(*iph)+header_len);
	skb_push(skb, sizeof(*iph)+header_len);
	skb_reset_network_header(skb);

	memset(&(IPCB(skb)->opt), 0, sizeof(IPCB(skb)->opt));
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	IPCB(skb)->flags &= ~(IPSKB_XFRM_TUNNEL_SIZE | IPSKB_XFRM_TRANSFORMED |
			      IPSKB_REROUTED);
	#endif
	dst_release(skb->dst);
	skb_dst_set(skb, &rt->u.dst);

	/*
	 *	Push down and install the IP header.
	 */

	//iph 			=	skb->nh.iph;
	iph 			=	ip_hdr(skb);
	iph->version		=	4;
	iph->ihl		=	sizeof(struct iphdr) >> 2;
	iph->frag_off		=	0;//df;
	iph->protocol		=	IPPROTO_GRE;
	iph->tos		=	0;
	iph->daddr		=	rt->rt_dst;
	iph->saddr		=	rt->rt_src;
	iph->ttl = dst_metric(&rt->u.dst, RTAX_HOPLIMIT);

	hdr=(struct pptp_gre_header *)(iph+1);
	//skb->h.raw = (char*)hdr;
	skb->transport_header = (char*)hdr;

	hdr->flags       = PPTP_GRE_FLAG_K;
	hdr->ver         = PPTP_GRE_VER;
	hdr->protocol    = htons(PPTP_GRE_PROTO);
	hdr->call_id     = htons(opt->dst_addr.call_id);

	if (!len){
		hdr->payload_len = 0;
		hdr->ver |= PPTP_GRE_FLAG_A;
		/* ack is in odd place because S == 0 */
		hdr->seq = htonl(opt->seq_recv);
		opt->ack_sent = opt->seq_recv;
		opt->stat->tx_acks++;
	}else {
		//if (!opt->seq_sent){
		//}

		hdr->flags |= PPTP_GRE_FLAG_S;
		hdr->seq    = htonl(opt->seq_sent++);
		if (log_level>=2)
			printk("PPTP: send packet: seq=%i",opt->seq_sent);
		if (opt->ack_sent != opt->seq_recv)	{
		/* send ack with this message */
			hdr->ver |= PPTP_GRE_FLAG_A;
			hdr->ack  = htonl(opt->seq_recv);
			opt->ack_sent = opt->seq_recv;
			if (log_level>=2)
				printk(" ack=%i",opt->seq_recv);
		}
		hdr->payload_len = htons(len);
		if (log_level>=2)
			printk("\n");
	}

	nf_reset(skb);

	skb->ip_summed = CHECKSUM_NONE;
	iph->tot_len = htons(skb->len);
	ip_select_ident(iph, &rt->u.dst, NULL);
	ip_send_check(iph);

//dump(skb->data, skb->len);

	err = NF_HOOK(PF_INET, NF_INET_LOCAL_OUT, skb, NULL, rt->u.dst.dev, dst_output);
	if (err == NET_XMIT_SUCCESS || err == NET_XMIT_CN) {
		opt->stat->tx_sent++;
		if (!opt->stat->pt_seq){
			opt->stat->pt_seq  = opt->seq_sent;
			do_gettimeofday(&opt->stat->pt_time);
		}
	}else goto tx_error;

	return 1;

tx_error:
	printk("pptp xmit fail\n");
	opt->stat->tx_failed++;
	if (!len) dev_kfree_skb(skb);
	return 1;
}

#if 0
static void ack_work(struct work_struct *work)
{
	struct PPTP_worker *worker = container_of(work, struct PPTP_worker, work);
	struct pppox_sock *po = (struct pppox_sock *) worker->sk;
	struct pptp_opt *opt=&po->proto.pptp;
	if (opt->ack_sent != opt->seq_recv)
		pptp_xmit(&po->chan,0);

	if (!opt->proc){
			char unit[10];
			opt->proc=1;
			sprintf(unit,"ppp%i",ppp_unit_number(&po->chan));
			create_proc_read_entry(unit,0,proc_dir,read_proc,po);
	}
}
#endif


#if 0
static int get_seq(struct sk_buff *skb)
{
	struct iphdr *iph;
	u8 *payload;
	struct pptp_gre_header *header;

	iph = (struct iphdr*)skb->data;
	payload = skb->data + (iph->ihl << 2);
	header = (struct pptp_gre_header *)(payload);

	return ntohl(header->seq);
}
#endif

#if 0
static void buf_work(struct work_struct *work)
{
	struct PPTP_worker *worker = container_of(work, struct PPTP_worker, work);
	struct pppox_sock *po = (struct pppox_sock *) worker->sk;
	struct pptp_opt *opt=&po->proto.pptp;
	struct timeval tv1,tv2;
	struct sk_buff *skb;
	unsigned int t;

	do_gettimeofday(&tv1);
	spin_lock_bh(&opt->skb_buf_lock);
	while((skb=skb_dequeue(&opt->skb_buf))){
		if (!__pptp_rcv(po,skb,0)){
			//skb_get_timestamp(skb,&tv2);
			tv2 = ktime_to_timeval(skb->tstamp);
			t=(tv1.tv_sec-tv2.tv_sec)*1000000+(tv1.tv_usec-tv2.tv_usec);
			if (t<opt->stat->rtt){
				skb_queue_head(&opt->skb_buf,skb);
				schedule_delayed_work((struct delayed_work*)&opt->buf_work.work,t/100*HZ/10000);
				goto exit;
			}
			t=get_seq(skb)-1;
			opt->stat->rx_lost+=t-opt->seq_recv;
			opt->seq_recv=t;
			__pptp_rcv(po,skb,0);
		}
	}
exit:
	spin_unlock_bh(&opt->skb_buf_lock);
}
#endif


#define MISSING_WINDOW 20
#define WRAPPED( curseq, lastseq) \
    ((((curseq) & 0xffffff00) == 0) && \
     (((lastseq) & 0xffffff00 ) == 0xffffff00))
static int __pptp_rcv(struct pppox_sock *po,struct sk_buff *skb,int new)
{
	struct pptp_opt *opt=&po->proto.pptp;
	int headersize,payload_len,seq;
	__u8 *payload;
	struct pptp_gre_header *header;

	header = (struct pptp_gre_header *)(skb->data);

	if (new){
		/* test if acknowledgement present */
		if (PPTP_GRE_IS_A(header->ver)){
				__u32 ack = (PPTP_GRE_IS_S(header->flags))?
						header->ack:header->seq; /* ack in different place if S = 0 */
				ack = ntohl( ack);
				if (ack > opt->ack_recv) opt->ack_recv = ack;
				/* also handle sequence number wrap-around  */
				if (WRAPPED(ack,opt->ack_recv)) opt->ack_recv = ack;
				/* mark by siyou.
				if (opt->stat->pt_seq && opt->ack_recv > opt->stat->pt_seq){
					struct timeval tv;
					unsigned int rtt;
					do_gettimeofday(&tv);
					rtt = (tv.tv_sec - opt->stat->pt_time.tv_sec)*1000000+
						tv.tv_usec-opt->stat->pt_time.tv_usec;
					opt->stat->rtt = (opt->stat->rtt + rtt) / 2;
					if (opt->stat->rtt>opt->timeout) opt->stat->rtt=opt->timeout;
					opt->stat->pt_seq=0;
				}
				*/
				if (opt->pause){
					opt->pause=0;
					ppp_output_wakeup(&po->chan);
				}
		}

		/* test if payload present */
		if (!PPTP_GRE_IS_S(header->flags)){
			opt->stat->rx_acks++;
			goto drop;
		}
	}

	headersize  = sizeof(*header);
	payload_len = ntohs(header->payload_len);
	seq         = ntohl(header->seq);

	/* no ack present? */
	if (!PPTP_GRE_IS_A(header->ver)) headersize -= sizeof(header->ack);
	/* check for incomplete packet (length smaller than expected) */
	if (skb->len- headersize < payload_len){
		if (log_level>=1)
			printk("PPTP: discarding truncated packet (expected %d, got %d bytes)\n",
						payload_len, skb->len- headersize);
		opt->stat->rx_truncated++;
		goto drop;
	}

	payload=skb->data+headersize;
	/* check for expected sequence number */
	/* mark by siyou.
	if ((seq == opt->seq_recv + 1) || (!opt->timeout &&
			(seq > opt->seq_recv + 1 || WRAPPED(seq, opt->seq_recv)))){
	*/
	if (1){

		if ( log_level >= 2 )
			printk("PPTP: accepting packet %d size=%i (%02x %02x %02x %02x %02x %02x)\n", seq,payload_len,
				*(payload +0),
				*(payload +1),
				*(payload +2),
				*(payload +3),
				*(payload +4),
				*(payload +5));
		opt->stat->rx_accepted++;
		opt->stat->rx_lost+=seq-(opt->seq_recv + 1);
		opt->seq_recv = seq;
		//schedule_work(&opt->ack_work.work);

		skb_pull(skb,headersize);

		if (payload[0] == PPP_ALLSTATIONS && payload[1] == PPP_UI){
			/* chop off address/control */
			if (skb->len < 3)
				goto drop;
			skb_pull(skb,2);
		}

		if ((*skb->data) & 1){
			/* protocol is compressed */
			skb_push(skb, 1)[0] = 0;
		}

		ppp_input(&po->chan,skb);

		return 1;
	/* out of order, check if the number is too low and discard the packet.
	* (handle sequence number wrap-around, and try to do it right) */
	}else if ( seq < opt->seq_recv + 1 || WRAPPED(opt->seq_recv, seq) ){
		if ( log_level >= 1)
			printk("PPTP: discarding duplicate or old packet %d (expecting %d)\n",
							seq, opt->seq_recv + 1);
		opt->stat->rx_underwin++;
	/* sequence number too high, is it reasonably close? */
	}else if ( seq < opt->seq_recv + MISSING_WINDOW ||
						 WRAPPED(seq, opt->seq_recv + MISSING_WINDOW) ){
		opt->stat->rx_buffered++;
		if ( log_level >= 1 && new )
				printk("PPTP: buffering packet %d (expecting %d, lost or reordered)\n",
						seq, opt->seq_recv+1);
		return 0;
	/* no, packet must be discarded */
	}else{
		if ( log_level >= 1 )
			printk("PPTP: discarding bogus packet %d (expecting %d)\n",
							seq, opt->seq_recv + 1);
	}
drop:
	kfree_skb(skb);
	return -1;
}


static int pptp_rcv(struct sk_buff *skb)
{
	struct pptp_gre_header *header;
	struct pppox_sock *po;
	//struct pptp_opt *opt;


	if (unlikely(!pskb_may_pull(skb, 12)))
		goto drop;

	header = (struct pptp_gre_header *)skb->data;
	if (    /* version should be 1 */
					((header->ver & 0x7F) != PPTP_GRE_VER) ||
					/* PPTP-GRE protocol for PPTP */
					(ntohs(header->protocol) != PPTP_GRE_PROTO)||
					/* flag C should be clear   */
					PPTP_GRE_IS_C(header->flags) ||
					/* flag R should be clear   */
					PPTP_GRE_IS_R(header->flags) ||
					/* flag K should be set     */
					(!PPTP_GRE_IS_K(header->flags)) ||
					/* routing and recursion ctrl = 0  */
					((header->flags&0xF) != 0)){
			/* if invalid, discard this packet */
		if (log_level>=1)
			printk("PPTP: Discarding GRE: %X %X %X %X %X %X\n",
							header->ver&0x7F, ntohs(header->protocol),
							PPTP_GRE_IS_C(header->flags),
							PPTP_GRE_IS_R(header->flags),
							PPTP_GRE_IS_K(header->flags),
							header->flags & 0xF);
		goto drop;
	}

	dst_release(skb->dst);
	skb->dst = NULL;

	nf_reset(skb);

	if (likely((po=lookup_chan(htons(header->call_id))))) {
		if (unlikely(!(sk_pppox(po)->sk_state&PPPOX_BOUND)))
			goto drop;
		if (__pptp_rcv(po,skb,1))
		{
			//buf_work(po);
			//opt=&po->proto.pptp;
			//buf_work(&opt->buf_work.work);
		}
		else{
		/* No need work. siyou.
			struct timeval tv;
			do_gettimeofday(&tv);
			//skb_set_timestamp(skb,&tv);
			skb->tstamp = timeval_to_ktime(tv);
			opt=&po->proto.pptp;
			spin_lock(&opt->skb_buf_lock);
			skb_queue_tail(&opt->skb_buf, skb);
			spin_unlock(&opt->skb_buf_lock);
			schedule_delayed_work(&opt->buf_work.work,opt->stat->rtt/100*HZ/10000);
		*/
		}
		goto out;
	}else{
		if (log_level>=1)
			printk("PPTP: Discarding packet from unknown call_id %i\n",header->call_id);
		icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, 0);
	}

drop:
	kfree_skb(skb);
out:
	return 0;
}


#if 0
static int proc_output (struct pppox_sock *po,char *buf)
{
	struct gre_statistics *stat=po->proto.pptp.stat;
	char *p=buf;
	p+=sprintf(p,"rx accepted  = %d\n",stat->rx_accepted);
	p+=sprintf(p,"rx lost      = %d\n",stat->rx_lost);
	p+=sprintf(p,"rx under win = %d\n",stat->rx_underwin);
	p+=sprintf(p,"rx buffered  = %d\n",stat->rx_buffered);
	p+=sprintf(p,"rx invalid   = %d\n",stat->rx_invalid);
	p+=sprintf(p,"rx acks      = %d\n",stat->rx_acks);
	p+=sprintf(p,"tx sent      = %d\n",stat->tx_sent);
	p+=sprintf(p,"tx failed    = %d\n",stat->tx_failed);
	p+=sprintf(p,"tx acks      = %d\n",stat->tx_acks);

	return p-buf;
}
#endif

#if 0
static int read_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	struct pppox_sock *po = data;
	int len = proc_output (po,page);
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}
#endif


static int pptp_bind(struct socket *sock,struct sockaddr *uservaddr,int sockaddr_len)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppox *sp = (struct sockaddr_pppox *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int error=0;

	if (log_level>=1)
		printk("PPTP: bind: addr=%X call_id=%i\n",sp->sa_addr.pptp.sin_addr.s_addr,
						sp->sa_addr.pptp.call_id);
	lock_sock(sk);

	opt->src_addr=sp->sa_addr.pptp;
	if (sp->sa_addr.pptp.call_id){
		if (lookup_chan(sp->sa_addr.pptp.call_id)){
			error=-EBUSY;
			goto end;
		}
		add_chan(po);
	}else{
		add_free_chan(po);
		if (!opt->src_addr.call_id)
			error=-EBUSY;
		if (log_level>=1)
			printk("PPTP: using call_id %i\n",opt->src_addr.call_id);
	}

 end:
	release_sock(sk);
	return error;
}

static int pptp_connect(struct socket *sock, struct sockaddr *uservaddr,
		  int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppox *sp = (struct sockaddr_pppox *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int error=0;

	if (log_level>=1)
		printk("PPTP: connect: addr=%X call_id=%i\n",
						sp->sa_addr.pptp.sin_addr.s_addr,sp->sa_addr.pptp.call_id);

	lock_sock(sk);

	if (sp->sa_protocol != PX_PROTO_PPTP){
		error = -EINVAL;
		goto end;
	}

	/* Check for already bound sockets */
	if (sk->sk_state & PPPOX_CONNECTED){
		error = -EBUSY;
		goto end;
	}

	/* Check for already disconnected sockets, on attempts to disconnect */
	if (sk->sk_state & PPPOX_DEAD){
		error = -EALREADY;
		goto end;
	}

	if (!opt->src_addr.sin_addr.s_addr || !sp->sa_addr.pptp.sin_addr.s_addr){
		error = -EINVAL;
		goto end;
	}

	po->chan.private=sk;
	po->chan.ops=&pptp_chan_ops;
	po->chan.mtu=PPP_MTU;
	po->chan.hdrlen=2+sizeof(struct pptp_gre_header);
	error = ppp_register_channel(&po->chan);
	if (error){
		printk(KERN_ERR "PPTP: failed to register PPP channel (%d)\n",error);
		goto end;
	}

	opt->dst_addr=sp->sa_addr.pptp;
	sk->sk_state = PPPOX_CONNECTED;

 end:
	release_sock(sk);
	return error;
}

static int pptp_getname(struct socket *sock, struct sockaddr *uaddr,
		  int *usockaddr_len, int peer)
{
	int len = sizeof(struct sockaddr_pppox);
	struct sockaddr_pppox sp;

	sp.sa_family	= AF_PPPOX;
	sp.sa_protocol	= PX_PROTO_PPTP;
	sp.sa_addr.pptp=pppox_sk(sock->sk)->proto.pptp.src_addr;

	memcpy(uaddr, &sp, len);

	*usockaddr_len = len;

	return 0;
}

static int pptp_setsockopt(struct socket *sock, int level, int optname,
	char *optval, int optlen)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int val;

	if (optlen!=sizeof(int))
		return -EINVAL;

	if (get_user(val,(int __user*)optval))
		return -EFAULT;

	switch(optname) {
		case PPTP_SO_TIMEOUT:
			opt->timeout=val;
			break;
		case PPTP_SO_WINDOW:
			opt->window=val;
			break;
		default:
				return -ENOPROTOOPT;
	}

	return 0;
}

static int pptp_getsockopt(struct socket *sock, int level, int optname,
	char* optval, int *optlen)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int len,val;

	if (get_user(len,(int __user*)optlen))
		return -EFAULT;

	if (len<sizeof(int))
		return -EINVAL;

	switch(optname) {
		case PPTP_SO_TIMEOUT:
			val=opt->timeout;
			break;
		case PPTP_SO_WINDOW:
			val=opt->window;
			break;
		default:
				return -ENOPROTOOPT;
	}

	if (put_user(sizeof(int),(int __user*)optlen))
		return -EFAULT;

	if (put_user(val,(int __user*)optval))
		return -EFAULT;

	return 0;
}

static int pptp_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po;
	struct pptp_opt *opt;
	int error = 0;

	if (!sk)
		return 0;

	if (sock_flag(sk, SOCK_DEAD))
		return -EBADF;

	po = pppox_sk(sk);
	opt=&po->proto.pptp;
	if (opt->src_addr.sin_addr.s_addr) {
		//cancel_delayed_work((struct delayed_work*)&opt->buf_work.work);
		//flush_scheduled_work();
		del_chan(po);
		skb_queue_purge(&opt->skb_buf);

		if (opt->proc){
			char unit[10];
			sprintf(unit,"ppp%i",ppp_unit_number(&po->chan));
			remove_proc_entry(unit,proc_dir);
		}
	}

	pppox_unbind_sock(sk);

	kfree(opt->stat);

	/* Signal the death of the socket. */
	sk->sk_state = PPPOX_DEAD;

	sock_orphan(sk);
	sock->sk = NULL;

	skb_queue_purge(&sk->sk_receive_queue);
	sock_put(sk);

	return error;
}


static struct proto pptp_sk_proto = {
	.name	  = "PPTP",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct pppox_sock),
};

static const struct proto_ops pptp_ops = {
    .family		= AF_PPPOX,
    .owner		= THIS_MODULE,
    .release		= pptp_release,
    .bind		=  pptp_bind,
    .connect		= pptp_connect,
    .socketpair		= sock_no_socketpair,
    .accept		= sock_no_accept,
    .getname		= pptp_getname,
    .poll		= sock_no_poll,
    .listen		= sock_no_listen,
    .shutdown		= sock_no_shutdown,
    .setsockopt		= pptp_setsockopt,
    .getsockopt		= pptp_getsockopt,
    .sendmsg		= sock_no_sendmsg,
    .recvmsg		= sock_no_recvmsg,
    .mmap		= sock_no_mmap,
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    .ioctl		= pppox_ioctl,
    #endif
};

static int pptp_create(struct net *net, struct socket *sock)
{
	int error = -ENOMEM;
	struct sock *sk=NULL;
	struct pppox_sock *po;
	struct pptp_opt *opt;

	sk = sk_alloc(net,PF_PPPOX, GFP_KERNEL, &pptp_sk_proto);
	if (!sk)
		goto out;

	sock_init_data(sock, sk);

	sock->state = SS_UNCONNECTED;
	sock->ops   = &pptp_ops;

	//sk->sk_backlog_rcv = pppoe_rcv_core;
	sk->sk_state	   = PPPOX_NONE;
	sk->sk_type	   = SOCK_STREAM;
	sk->sk_family	   = PF_PPPOX;
	sk->sk_protocol	   = PX_PROTO_PPTP;

	po = pppox_sk(sk);
	opt=&po->proto.pptp;

	opt->window=min_window;
	opt->timeout=0;
	opt->seq_sent=0; opt->seq_recv=-1;
	opt->ack_recv=0; opt->ack_sent=-1;
	skb_queue_head_init(&opt->skb_buf);
	opt->skb_buf_lock=SPIN_LOCK_UNLOCKED;

	opt->ack_work.sk = sk;
	opt->buf_work.sk = sk;
	//INIT_WORK(&opt->ack_work.work, ack_work);
	//INIT_DELAYED_WORK(&opt->buf_work.work, buf_work);
	opt->stat=kzalloc(sizeof(*opt->stat),GFP_KERNEL);

	error = 0;
out:
	return error;
}


static struct pppox_proto pppox_pptp_proto = {
    .create	= pptp_create,
    //.ioctl	= pptp_ioctl,
    .owner	= THIS_MODULE,
};

static struct net_protocol net_pptp_protocol = {
	.handler	= pptp_rcv,
	//.err_handler	=	ipgre_err,
};


static int pptp_init_module(void)
{
	int err=0;
	printk(KERN_INFO "PPTP driver version " PPTP_DRIVER_VERSION "\n");

	if (inet_add_protocol(&net_pptp_protocol, IPPROTO_GRE) < 0) {
		printk(KERN_INFO "PPTP: can't add protocol\n");
		goto out;
	}

	err = proto_register(&pptp_sk_proto, 0);
	if (err){
		printk(KERN_INFO "PPTP: can't register sk_proto\n");
		goto out_inet_del_protocol;
	}

 	err = register_pppox_proto(PX_PROTO_PPTP, &pppox_pptp_proto);
	if (err){
		printk(KERN_INFO "PPTP: can't register pppox_proto\n");
		goto out_unregister_sk_proto;
	}

	proc_dir=proc_mkdir("pptp",NULL);
	if (!proc_dir){
		printk(KERN_ERR "PPTP: failed to create proc dir\n");
	}
	//console_verbose();

out:
	return err;
out_unregister_sk_proto:
	proto_unregister(&pptp_sk_proto);
out_inet_del_protocol:
	inet_del_protocol(&net_pptp_protocol, IPPROTO_GRE);
	goto out;
}

static void pptp_exit_module(void)
{
	unregister_pppox_proto(PX_PROTO_PPTP);
	proto_unregister(&pptp_sk_proto);
	inet_del_protocol(&net_pptp_protocol, IPPROTO_GRE);

	if (proc_dir)
		remove_proc_entry("pptp",NULL);
}


module_init(pptp_init_module);
module_exit(pptp_exit_module);
