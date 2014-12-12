#ifndef _XT_MAC_H
#define _XT_MAC_H
#include <elbox_config.h>

#ifdef ELBOX_IPTABLES_RTL_DST_MAC_SUPPORT
#define MAC_SRC		0x01	/* Match source MAC address */
#define MAC_DST		0x02	/* Match destination MAC address */
#define MAC_SRC_INV		0x10	/* Negate the condition */
#define MAC_DST_INV		0x20	/* Negate the condition */

struct xt_mac {
    unsigned char macaddr[ETH_ALEN];
};

struct xt_mac_info {
   struct xt_mac srcaddr;
   struct xt_mac dstaddr;
//    int invert;
    u_int8_t flags;
};
#else
struct xt_mac_info {
    unsigned char srcaddr[ETH_ALEN];
    int invert;
};
#endif /*IPTABLES_RTL_DST_MAC_SUPPORT*/

#endif /*_XT_MAC_H*/
