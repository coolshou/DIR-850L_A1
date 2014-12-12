#ifndef _XT_MPORT_H
#define _XT_MPORT_H

#include <linux/types.h>

enum xt_mport_flags {
	XT_MPORT_SOURCE,
	XT_MPORT_DESTINATION,
	XT_MPORT_EITHER
};

#define XT_MPORTS	15

/* Must fit inside union xt_matchinfo: 16 bytes */
struct xt_mport {
	__u8 flags;				/* Type of comparison */
	__u8 count;				/* Number of ports */
	__u16 ports[XT_MPORTS];	/* Ports */
};

struct xt_mport_v1 {
	__u8 flags;				/* Type of comparison */
	__u8 count;				/* Number of ports */
	__u16 ports[XT_MPORTS];	/* Ports */
	__u8 pflags[XT_MPORTS];	/* Port flags */
	__u8 invert;			/* Invert flag */
};

#endif /*_XT_MPORT_H*/
