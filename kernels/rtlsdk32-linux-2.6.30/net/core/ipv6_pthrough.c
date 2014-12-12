/*
 * IPv6 passthrough 
 * This module is used to pass through the IPv6 packets
 * 
 * Peter Wu 20050804
 */

#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <net/sock.h>

extern struct net_device	*dev_get_by_name(struct net *net, const char *name);
extern int		dev_set_promiscuity(struct net_device *dev, int inc);
extern int dev_queue_xmit(struct sk_buff *skb);

int ipv6_pt_enable = 0;

char ipv6_pt_landev[IFNAMSIZ];
char ipv6_pt_wandev[IFNAMSIZ];
#define ETH_TYPE_IPV6	0x86dd

/* return 1, if we want to handle this packet, or
 * return 0, let other ones do this	 */
int ipv6_pthrough(struct sk_buff *skb)
{
	unsigned short proto;
	unsigned char *smac;
	unsigned char *dmac;
	struct net_device *dev;
	struct ethhdr *eth = eth_hdr(skb);
	struct net *net = sock_net(skb->sk);

	/* check if ipv6 pass through enabled or not
	 * if not set yet, just return and do nothing */
	if (!ipv6_pt_enable) return 0;
	
	// check and forward packets
	proto = eth->h_proto;
	dmac = eth->h_dest;
	smac = eth->h_source;
//	proto = skb->mac.ethernet->h_proto;
//	smac = skb->mac.ethernet->h_source;
//	dmac = skb->mac.ethernet->h_dest;

	if (proto == htons(ETH_TYPE_IPV6)) {
		if (strcmp(skb->dev->name, ipv6_pt_landev) == 0) {
//			printk("PeTeR: IPv6 OutGoing packet (%s)\n", skb->dev->name);
//			printk("PeTeR: skb->dev (%s->%s)\n", ipv6_pt_landev, ipv6_pt_wandev);
			dev = dev_get_by_name(net, ipv6_pt_wandev);
			if (!dev)
				return 0;
			else {
				skb->dev=dev;
				dev_put(skb->dev);
			}
			skb_push(skb, ETH_HLEN);
			dev_queue_xmit(skb);
			return 1;
		}
		if (strcmp(skb->dev->name, ipv6_pt_wandev) == 0) {
//			printk("PeTeR: IPv6 Incoming packet (%s)\n", skb->dev->name);
			dev = dev_get_by_name(net, ipv6_pt_landev);
			if (!dev)
				return 0;
			else {
				skb->dev=dev;
				dev_put(skb->dev);
			}
			skb_push(skb, ETH_HLEN);
			dev_queue_xmit(skb);
			return 1;
		}
	}

	return 0;
}

int proc_ipv6_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret=0;
	if (ipv6_pt_enable) {
		ret = sprintf(page, "%s,%s\n", ipv6_pt_landev, ipv6_pt_wandev);
	} else {
		ret = sprintf(page, "null,null\n");
	}

	return ret;
}

#define isCHAR(x) ((x >= 'a') && (x <= 'z')) ? 1:((x >= '0') && (x <= '9')) ? 1:((x >= 'A') && (x <= 'Z')) ? 1:(x == '.') ? 1:0
int proc_ipv6_write(struct file *file, const char * buffer, unsigned long count, void *data)
{
	char *pt;
	struct net_device *dev;

	if (ipv6_pt_enable) {
		ipv6_pt_enable = 0;
		if ((dev = dev_get_by_name(&init_net, ipv6_pt_landev))) {
			rtnl_lock();
			dev_set_promiscuity(dev, -1);
			rtnl_unlock();
			dev_put(dev);
		}
		if ((dev = dev_get_by_name(&init_net, ipv6_pt_wandev))) {
			rtnl_lock();
			dev_set_promiscuity(dev, -1);
			rtnl_unlock();
			dev_put(dev);
		}
	}

	/* we expect that buffer contain format of "ipv6_pt_landev,ipv6_pt_wandev" */
	memset(ipv6_pt_landev, 0x0, sizeof (ipv6_pt_landev));
	for (pt=ipv6_pt_landev; *buffer && (*buffer != ','); buffer++) {
		if ((*buffer != ' ') && isCHAR(*buffer)) {
			*pt = *buffer;
			pt++;
		}
	}
	
	if (!(*buffer))	goto ppw_failed;
	
	memset(ipv6_pt_wandev, 0x0, sizeof (ipv6_pt_wandev));
	for (pt=ipv6_pt_wandev, buffer++; *buffer; buffer++) {
		if ((*buffer != ' ') && isCHAR(*buffer)) {
			*pt = *buffer;
			pt++;
		}
	}
	
	if (!(dev = dev_get_by_name(&init_net, ipv6_pt_landev))) goto ppw_failed;
	else 
	{
		rtnl_lock();
		dev_set_promiscuity(dev, 1);
		rtnl_unlock();
		dev_put(dev);
	}
	if (!(dev = dev_get_by_name(&init_net, ipv6_pt_wandev))) goto ppw_failed;
	else 
	{
		rtnl_lock();
		dev_set_promiscuity(dev, 1);
		rtnl_unlock();
		dev_put(dev);
	}
	
	ipv6_pt_enable = 1;
	printk("ipv6 pass through (%s<->%s)\n",ipv6_pt_landev, ipv6_pt_wandev);
	return count;
	
ppw_failed:
	ipv6_pt_enable = 0;
	memset(ipv6_pt_landev, 0x0, sizeof (ipv6_pt_landev));
	memset(ipv6_pt_wandev, 0x0, sizeof (ipv6_pt_wandev));
//	printk("failed\n");

	return count;
}

