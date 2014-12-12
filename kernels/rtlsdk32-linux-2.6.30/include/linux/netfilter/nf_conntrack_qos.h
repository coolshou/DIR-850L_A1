#include <linux/list.h>
#include <net/netfilter/nf_conntrack_tuple.h>
typedef struct	inet_protocol_ips_qos_filter {
	u_int16_t		protonum, 
					reserve;
	union nf_conntrack_man_proto u;
	char			name[16];
	struct list_head	list;
}IPS_FILTER;

/* Value for ip_ct_tcp_timeout_establish */
#define TCP_TIMEOUT_ESTABLISH_NORMAL	7200	/*sec, normal mode or high priority ct*/
#define TCP_TIMEOUT_ESTABLISH_BT		60		/*sec, bt mode*/
#define TCP_TRAFFIC_THROTTLE				1024
#define TCP_TRAFFIC_LOW					768
#define TCP_TIMEOUT_NORMAL_MODE			0
#define TCP_TIMEOUT_BT_MODE				1
	
#define IPS_HASH_SIZE				128	
#define IPS_FILTER_LIST_MAX			4
#define IPS_FILTER_ICMP				0	/*IPPROTO_ICMP = 1*/
#define IPS_FILTER_IGMP				1	/*IPPROTO_IGMP = 2*/
#define IPS_FILTER_TCP				2	/*IPPROTO_TCP = 6*/
#define IPS_FILTER_UDP				3	/*IPPROTO_UDP = 17*/

void	ips_qos_initial(void);
int	ips_check_entry(const struct nf_conntrack_tuple *tp);
