#ifndef _NF_CONNTRACK_IPSEC_PASS_H
#define _NF_CONNTRACK_IPSEC_PASS_H

#include <net/netfilter/nf_conntrack_tuple.h>

struct spi_pair;

struct nf_ct_esp
{
	unsigned int stream_timeout;
	unsigned int timeout;

	struct spi_pair *spi;
};

#endif //_NF_CONNTRACK_IPSEC_PASS_H
