#ifndef _NF_CONNTRACK_IKE_H
#define _NF_CONNTRACK_IKE_H

extern unsigned int (*is_ike_traffic_hook)(const struct nf_conntrack_tuple *tuple , struct sk_buff *skb , unsigned int dataoff);

struct nf_conn_ike
{
	struct list_head list;
	struct nf_conn *ct;
	unsigned int timeout;
	unsigned char i_cookie[8];
	unsigned char r_cookie[8];
};

extern struct nf_conn_ike *(*nf_ct_ike_ext_add_hook)(struct nf_conn *ct , gfp_t gfp , struct sk_buff *skb , unsigned int dataoff);
extern void (*update_ike_cookie_pair_hook)(struct nf_conn *ct , struct sk_buff *skb , unsigned int dataoff);
extern int (*redirect_to_real_peer_hook)(struct nf_conntrack_tuple *tuple , const struct sk_buff *skb , unsigned int dataoff);

#endif //_NF_CONNTRACK_IKE_H

