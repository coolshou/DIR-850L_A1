/* Shared library add-on to iptables to add destination-NAT support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <xtables.h>
#include <iptables.h> /* get_kernel_version */
#include <limits.h> /* INT_MAX in ip_tables.h */
#include <linux/netfilter_ipv4/ip_tables.h>
#include <net/netfilter/nf_nat.h>

#include <linux/netfilter_ipv4/ipt_DNAT.h>
//#define IPT_DNAT_OPT_DEST 0x1
//#define IPT_DNAT_OPT_RANDOM 0x2

/* Dest NAT data consists of a multi-range, indicating where to map
   to. */
struct ipt_natinfo
{
	struct xt_entry_target t;
	struct nf_nat_multi_range mr;
};

static void DNAT_help(void)
{
	printf(
"DNAT target options:\n"
" --to-destination <ipaddr>[-<ipaddr>][:port-port]\n"
"				Address to map destination to.\n"
"[--random] [--persistent]\n"
" --to-shift <ipaddr>[:offset]\n"
"               Address to map destination and shift the port range to.\n");
}

static const struct option DNAT_opts[] = {
	{ "to-destination", 1, NULL, '1' },
	{ "random", 0, NULL, '2' },
	{ "persistent", 0, NULL, '3' },
	{ "to-shift"		,	1,	0,	'4'},
	{ .name = NULL }
};

static struct ipt_natinfo *
append_range(struct ipt_natinfo *info, const struct nf_nat_range *range)
{
	unsigned int size;

	/* One rangesize already in struct ipt_natinfo */
	size = XT_ALIGN(sizeof(*info) + info->mr.rangesize * sizeof(*range));

	info = realloc(info, size);
	if (!info)
		xtables_error(OTHER_PROBLEM, "Out of memory\n");

	info->t.u.target_size = size;
	info->mr.range[info->mr.rangesize] = *range;
	info->mr.rangesize++;

	return info;
}

/* Ranges expected in network order. */
static struct xt_entry_target *
parse_to(char *arg, int portok, struct ipt_natinfo *info, int type)
{
	struct nf_nat_range range;
	char *colon, *dash, *error;
	const struct in_addr *ip;

	memset(&range, 0, sizeof(range));
	colon = strchr(arg, ':');
	/* Set dnat-type into flags */
	range.flags|=type;

	if (colon) {
		int port;

		if (!portok)
			xtables_error(PARAMETER_PROBLEM,
				   "Need TCP, UDP, SCTP or DCCP with port specification");

		range.flags |= IP_NAT_RANGE_PROTO_SPECIFIED;

		port = atoi(colon+1);
		if (port <= 0 || port > 65535)
			xtables_error(PARAMETER_PROBLEM,
				   "Port `%s' not valid\n", colon+1);

		error = strchr(colon+1, ':');
		if (error)
			xtables_error(PARAMETER_PROBLEM,
				   "Invalid port:port syntax - use dash\n");

		dash = strchr(colon, '-');
		if (!dash) {
			if(type==IPT_DNAT_TO_DEST) {
			    range.min.tcp.port
				    = range.max.tcp.port
				    = htons(port);
			} else if(type==IPT_DNAT_TO_SHIFT) {
				int		shift=htons(port);

				/* Shift is from 0-65535
				 * ex. 11024-55536, and the transition port will be 1024 */
				if(shift<=0 || shift>65535)
					xtables_error(PARAMETER_PROBLEM, "Shift `%s' not valid\n", dash+1);
				range.max.tcp.port=range.min.tcp.port=shift;
			}
		} else {
			if(type==IPT_DNAT_TO_DEST || type==IPT_DNAT_TO_SHIFT) {
				if(type==IPT_DNAT_TO_DEST) {
				        int maxport;
	        
				        maxport = atoi(dash + 1);
				        if (maxport <= 0 || maxport > 65535)
					        xtables_error(PARAMETER_PROBLEM,
						           "Port `%s' not valid\n", dash+1);
				        if (maxport < port)
					        /* People are stupid. */
					        xtables_error(PARAMETER_PROBLEM,
						           "Port range `%s' funky\n", colon+1);
				        range.min.tcp.port = htons(port);
				        range.max.tcp.port = htons(maxport);
				} else if (type==IPT_DNAT_TO_SHIFT) {
					goto	SHIFT_ERR;
				}
			} else {
				/* Unvalidable */
				goto	SHIFT_ERR;
			}
		}
		/* Starts with a colon? No IP info...*/
		if (colon == arg)
			return &(append_range(info, &range)->t);
		*colon = '\0';
	}

	range.flags |= IP_NAT_RANGE_MAP_IPS;
	dash = strchr(arg, '-');
	if (colon && dash && dash > colon)
		dash = NULL;

	if (dash) {
		*dash = '\0';
		if(type==IPT_DNAT_TO_SHIFT)
			goto	SHIFT_ERR;
	}

	ip = xtables_numeric_to_ipaddr(arg);
	if (!ip)
		xtables_error(PARAMETER_PROBLEM, "Bad IP address \"%s\"\n",
			   arg);
	range.min_ip = ip->s_addr;
	if (dash) {
		ip = xtables_numeric_to_ipaddr(dash+1);
		if (!ip)
			xtables_error(PARAMETER_PROBLEM, "Bad IP address \"%s\"\n",
				   dash+1);
		range.max_ip = ip->s_addr;
	} else
		range.max_ip = range.min_ip;

	return &(append_range(info, &range)->t);
SHIFT_ERR:
	xtables_error(PARAMETER_PROBLEM, "Must specify '--to-shift ip:offset'\n");
}

static int DNAT_parse(int c, char **argv, int invert, unsigned int *flags,
                      const void *e, struct xt_entry_target **target)
{
	const struct ipt_entry *entry = e;
	struct ipt_natinfo *info = (void *)*target;
	int portok;

	if (entry->ip.proto == IPPROTO_TCP
	    || entry->ip.proto == IPPROTO_UDP
	    || entry->ip.proto == IPPROTO_SCTP
	    || entry->ip.proto == IPPROTO_DCCP
	    || entry->ip.proto == IPPROTO_ICMP)
		portok = 1;
	else
		portok = 0;

	switch (c) {
	case '1':
		if (xtables_check_inverse(optarg, &invert, NULL, 0, argv))
			xtables_error(PARAMETER_PROBLEM,
				   "Unexpected `!' after --to-destination");

		if (*flags & IPT_DNAT_TO_DEST) {
			if (!kernel_version)
				get_kernel_version();
			if (kernel_version > LINUX_VERSION(2, 6, 10))
				xtables_error(PARAMETER_PROBLEM,
					   "Multiple --to-destination not supported");
		}
		*target = parse_to(optarg, portok, info, IPT_DNAT_TO_DEST);
		/* WTF do we need this for?? */
		if (*flags & IPT_DNAT_TO_RANDOM)
			info->mr.range[0].flags |= IP_NAT_RANGE_PROTO_RANDOM;
		*flags |= IPT_DNAT_TO_DEST;
		return 1;
		
	case '4':
		if (xtables_check_inverse(optarg, &invert, NULL, 0, argv))
			xtables_error(PARAMETER_PROBLEM,
				   "Unexpected `!' after --to-shift");

		if (*flags & IPT_DNAT_TO_SHIFT) {
			xtables_error(PARAMETER_PROBLEM,
				   "--to-shift only once!");
		}
		*target = parse_to(optarg, portok, info, IPT_DNAT_TO_SHIFT);
		/* WTF do we need this for?? */
		if (*flags & IPT_DNAT_TO_RANDOM)
			info->mr.range[0].flags |= IP_NAT_RANGE_PROTO_RANDOM;
		*flags |= IPT_DNAT_TO_SHIFT;
		return 1;

	case '2':
		if (*flags & IPT_DNAT_TO_DEST) {
			info->mr.range[0].flags |= IP_NAT_RANGE_PROTO_RANDOM;
			*flags |= IPT_DNAT_TO_RANDOM;
		} else
			*flags |= IPT_DNAT_TO_RANDOM;
		return 1;

	case '3':
		info->mr.range[0].flags |= IP_NAT_RANGE_PERSISTENT;
		return 1;

	default:
		return 0;
	}
}

static void DNAT_check(unsigned int flags)
{
	if (!flags)
		xtables_error(PARAMETER_PROBLEM,
			   "You must specify --to-destination");
}

static void print_range(const struct nf_nat_range *r)
{
	if (r->flags & IP_NAT_RANGE_MAP_IPS) {
		struct in_addr a;

		a.s_addr = r->min_ip;
		printf("%s", xtables_ipaddr_to_numeric(&a));
		if (r->max_ip != r->min_ip) {
			a.s_addr = r->max_ip;
			printf("-%s", xtables_ipaddr_to_numeric(&a));
		}
	}
	if (r->flags & IP_NAT_RANGE_PROTO_SPECIFIED) {
		printf(":");
		printf("%hu", ntohs(r->min.tcp.port));
		if (r->max.tcp.port != r->min.tcp.port)
			printf("-%hu", ntohs(r->max.tcp.port));
	}
}

static void DNAT_print(const void *ip, const struct xt_entry_target *target,
                       int numeric)
{
	const struct ipt_natinfo *info = (const void *)target;
	unsigned int i = 0;

	if(info->mr.range[0].flags&IPT_DNAT_TO_DEST)
		printf("to:");
	else if(info->mr.range[0].flags&IPT_DNAT_TO_SHIFT)
		printf("shift:");
	else	/* Unvalidable */
		;;

 	for (i = 0; i < info->mr.rangesize; i++) {
		print_range(&info->mr.range[i]);
		printf(" ");
		if (info->mr.range[i].flags & IP_NAT_RANGE_PROTO_RANDOM)
			printf("random ");
		if (info->mr.range[i].flags & IP_NAT_RANGE_PERSISTENT)
			printf("persistent ");
	}
}

static void DNAT_save(const void *ip, const struct xt_entry_target *target)
{
	const struct ipt_natinfo *info = (const void *)target;
	unsigned int i = 0;

	for (i = 0; i < info->mr.rangesize; i++) {
		printf("--to-destination ");
		print_range(&info->mr.range[i]);
		printf(" ");
		if (info->mr.range[i].flags & IP_NAT_RANGE_PROTO_RANDOM)
			printf("--random ");
		if (info->mr.range[i].flags & IP_NAT_RANGE_PERSISTENT)
			printf("--persistent ");
	}
}

static struct xtables_target dnat_tg_reg = {
	.name		= "DNAT",
	.version	= XTABLES_VERSION,
	.family		= NFPROTO_IPV4,
	.size		= XT_ALIGN(sizeof(struct nf_nat_multi_range)),
	.userspacesize	= XT_ALIGN(sizeof(struct nf_nat_multi_range)),
	.help		= DNAT_help,
	.parse		= DNAT_parse,
	.final_check	= DNAT_check,
	.print		= DNAT_print,
	.save		= DNAT_save,
	.extra_opts	= DNAT_opts,
};

void _init(void)
{
	xtables_register_target(&dnat_tg_reg);
}
