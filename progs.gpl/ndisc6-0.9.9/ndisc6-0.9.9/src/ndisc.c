/*
 * ndisc.c - ICMPv6 neighbour discovery command line tool
 * $Id: ndisc.c 625 2008-07-11 17:10:23Z remi $
 */

/*************************************************************************
 *  Copyright © 2004-2007 Rémi Denis-Courmont.                           *
 *  This program is free software: you can redistribute and/or modify    *
 *  it under the terms of the GNU General Public License as published by *
 *  the Free Software Foundation, versions 2 or 3 of the license.        *
 *                                                                       *
 *  This program is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gettext.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* div() */
#include <inttypes.h> /* uint8_t */
#include <limits.h> /* UINT_MAX */
#include <locale.h>
#include <stdbool.h>

#include <errno.h> /* EMFILE */
#include <sys/types.h>
#include <unistd.h> /* close() */
#include <time.h> /* clock_gettime() */
#include <poll.h> /* poll() */
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
/* rbj */
/* >>>> */
#include <sys/queue.h>
#include <signal.h>
#include <sys/time.h>
/* <<<< */

#include "gettime.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include <netdb.h> /* getaddrinfo() */
#include <arpa/inet.h> /* inet_ntop() */
#include <net/if.h> /* if_nametoindex() */

#include <netinet/in.h>
#include <netinet/icmp6.h>

#ifndef IPV6_RECVHOPLIMIT
/* Using obsolete RFC 2292 instead of RFC 3542 */ 
# define IPV6_RECVHOPLIMIT IPV6_HOPLIMIT
#endif

#ifndef AI_IDN
# define AI_IDN 0
#endif

/* rbj */
/* >>>> */
/************************************************************/
#define DEFAULT_CHECK_TIME 1800;
#define NDISC_DURATION_INFINITE 0xffffffff

char **script_env;  
int s_env_nalloc;   
char configfile[128]; 
char eui64[8]; 
bool script_run; 
bool gverbose;
int script_envc;
int state; 
const char g_ifname[32];
bool need_check_timer;
bool is_STATELESS;
bool using_mac;
bool is_forever;
bool rm_gw = false; //IOL test
unsigned int rtlft = 0; //IOL test
char *pid_file = "/var/run/rdisc6.pid";//rbj
static volatile int termsig = 0;

#ifndef USE_QUEUE
struct pi_entry *find_empty_entry();
#endif

#ifdef USE_QUEUE
struct pi_entry
{
	u_int32_t  valid_lft;
	u_int32_t  preferred_lft;
	struct in6_addr addr;
	u_int32_t  prefix_len;
	u_int32_t  expire_time;
	bool 	lft_forever;
	TAILQ_ENTRY(pi_entry) entries;
};
TAILQ_HEAD(, pi_entry) ndisc_pi_head;
#else
#define MAX_PI_ENTRY 20
int pi_entry_cnt;
struct pi_entry
{
	u_int32_t  valid_lft;
	u_int32_t  preferred_lft;
	struct in6_addr addr;
	u_int32_t  prefix_len;
	u_int32_t  expire_time;
	bool 	lft_forever;
	bool 	use;
};
struct pi_entry ndisc_pi_list[MAX_PI_ENTRY];
#endif

enum {NDISC_INIT,NDISC_RECV};
typedef enum{IFADDRCONF_ADD,IFADDRCONF_REMOVE}ifaddrconf_cmd_t;

struct in6_ifreq
{
	struct in6_addr ifr6_addr;
	u_int32_t ifr6_prefixlen;
	unsigned int ifr6_ifindex;
};

void script_setenv(char *var, char *value);
static struct pi_entry *find_addr(struct in6_addr *addr);
u_int32_t find_min_lifetime();
void ndisc_alarm_handler();
int update_ia(const struct nd_opt_prefix_info *pi);
pid_t run_program(char *prog,int must_exist);
int set_pi_timer(struct itimerval *t);
void check_pi_lifetime();
void remove_pi_timer();
void cleanup_addr();
void showpilist();
/************************************************************/
/* <<<< */

enum ndisc_flags
{
	NDISC_VERBOSE1=0x1,
	NDISC_VERBOSE2=0x2,
	NDISC_VERBOSE3=0x3,
	NDISC_VERBOSE =0x3,
	NDISC_NUMERIC =0x4,
	NDISC_SINGLE  =0x8,
};

/* rbj */
/* >>>> */
static void term_handler(int signum)
{
	if(gverbose) printf("[%s]signum:%d\n",__FUNCTION__,signum);
	termsig = signum;
}
/* <<<< */

static int
getipv6byname (const char *name, const char *ifname, int numeric,
               struct sockaddr_in6 *addr)
{
	struct addrinfo hints, *res;
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_DGRAM; /* dummy */
	hints.ai_flags = numeric ? AI_NUMERICHOST : 0;

	int val = getaddrinfo (name, NULL, &hints, &res);
	if (val)
	{
		fprintf (stderr, _("%s: %s\n"), name, gai_strerror (val));
		return -1;
	}

	memcpy (addr, res->ai_addr, sizeof (struct sockaddr_in6));
	freeaddrinfo (res);

	val = if_nametoindex (ifname);
	if (val == 0)
	{
		perror (ifname);
		return -1;
	}
	addr->sin6_scope_id = val;

	return 0;
}


static inline int
sethoplimit (int fd, int value)
{
	return (setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
	                    &value, sizeof (value))
	     || setsockopt (fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
	                    &value, sizeof (value))) ? -1 : 0;
}


static void
printmacaddress (const uint8_t *ptr, size_t len)
{
	while (len > 1)
	{
		printf ("%02X:", *ptr);
		ptr++;
		len--;
	}

	if (len == 1)
		printf ("%02X\n", *ptr);
}

/* rbj */
/* >>>> */
static void
setenv_macaddress (const uint8_t *ptr, size_t len)
{
	char str[64];
	sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",*ptr,*(ptr+1),*(ptr+2),
			*(ptr+3),*(ptr+4),*(ptr+5));
	script_setenv("MAC",str);
}
/* <<<< */

#  include <sys/ioctl.h>
static int
getmacaddress (const char *ifname, uint8_t *addr)
{
//# ifdef SIOCGIFHWADDR /* rbj */
#if 1
	struct ifreq req;
	//printf("Enter %s\n",__FUNCTION__);
	memset (&req, 0, sizeof (req));

	if (((unsigned)strlen (ifname)) >= (unsigned)IFNAMSIZ)
		return -1; /* buffer overflow = local root */
	strcpy (req.ifr_name, ifname);

	int fd = socket (AF_INET6, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;

	if (ioctl (fd, SIOCGIFHWADDR, &req))
	{
		perror (ifname);
		close (fd);
		return -1;
	}
	close (fd);
	//for(int i=0;i<6;i++)
	//	printf("%02x:",0xff&req.ifr_hwaddr.sa_data[i]);

	//printf("%s:%d\n",__FUNCTION__,__LINE__);//rbj

	memcpy (addr, req.ifr_hwaddr.sa_data, 6);
	return 0;
# else
	(void)ifname;
	(void)addr;
	return -1;
# endif
}

#ifndef RDISC 
# ifdef __linux__
#  include <sys/ioctl.h>
# endif

static const uint8_t nd_type_advert = ND_NEIGHBOR_ADVERT;
static const unsigned nd_delay_ms = 1000;
static const unsigned ndisc_default = NDISC_VERBOSE1 | NDISC_SINGLE;
static const char ndisc_usage[] = N_(
	"Usage: %s [options] <IPv6 address> <interface>\n"
	"Looks up an on-link IPv6 node link-layer address (Neighbor Discovery)\n");
static const char ndisc_dataname[] = N_("link-layer address");

typedef struct
{
	struct nd_neighbor_solicit hdr;
	struct nd_opt_hdr opt;
	uint8_t hw_addr[6];
} solicit_packet;


static ssize_t
buildsol (solicit_packet *ns, struct sockaddr_in6 *tgt, const char *ifname)
{
	/* builds ICMPv6 Neighbor Solicitation packet */
	ns->hdr.nd_ns_type = ND_NEIGHBOR_SOLICIT;
	ns->hdr.nd_ns_code = 0;
	ns->hdr.nd_ns_cksum = 0; /* computed by the kernel */
	ns->hdr.nd_ns_reserved = 0;
	memcpy (&ns->hdr.nd_ns_target, &tgt->sin6_addr, 16);

	/* determines actual multicast destination address */
	memcpy (tgt->sin6_addr.s6_addr, "\xff\x02\x00\x00\x00\x00\x00\x00"
	                                "\x00\x00\x00\x01\xff", 13);

	/* gets our own interface's link-layer address (MAC) */
	if (getmacaddress (ifname, ns->hw_addr))
		return sizeof (ns->hdr);
	
	ns->opt.nd_opt_type = ND_OPT_SOURCE_LINKADDR;
	ns->opt.nd_opt_len = 1; /* 8 bytes */
	return sizeof (*ns);
}


static int
parseadv (const uint8_t *buf, size_t len, const struct sockaddr_in6 *tgt,
          bool verbose)
{
	const struct nd_neighbor_advert *na =
		(const struct nd_neighbor_advert *)buf;
	const uint8_t *ptr;
	
	/* checks if the packet is a Neighbor Advertisement, and
	 * if the target IPv6 address is the right one */
	if ((len < sizeof (struct nd_neighbor_advert))
	 || (na->nd_na_type != ND_NEIGHBOR_ADVERT)
	 || (na->nd_na_code != 0)
	 || memcmp (&na->nd_na_target, &tgt->sin6_addr, 16))
		return -1;

	len -= sizeof (struct nd_neighbor_advert);

	/* looks for Target Link-layer address option */
	ptr = buf + sizeof (struct nd_neighbor_advert);

	while (len >= 8)
	{
		uint16_t optlen;

		optlen = ((uint16_t)(ptr[1])) << 3;
		if (optlen == 0)
			break; /* invalid length */

		if (len < optlen) /* length > remaining bytes */
			break;
		len -= optlen;


		/* skips unrecognized option */
		if (ptr[0] != ND_OPT_TARGET_LINKADDR)
		{
			ptr += optlen;
			continue;
		}

		/* Found! displays link-layer address */
		ptr += 2;
		optlen -= 2;
		if (verbose)
			fputs (_("Target link-layer address: "), stdout);

		printmacaddress (ptr, optlen);
		return 0;
	}

	return -1;
}
#else
static const uint8_t nd_type_advert = ND_ROUTER_ADVERT;
static const unsigned nd_delay_ms = 4000;
static const unsigned ndisc_default = NDISC_VERBOSE1;
static const char ndisc_usage[] = N_(
	"Usage: %s [options] [IPv6 address] <interface>\n"
	"Solicits on-link IPv6 routers (Router Discovery)\n");
static const char ndisc_dataname[] = N_("advertised prefixes");

typedef struct nd_router_solicit solicit_packet;

static ssize_t
buildsol (solicit_packet *rs, struct sockaddr_in6 *tgt, const char *ifname)
{
	(void)tgt;
	(void)ifname;

	/* builds ICMPv6 Router Solicitation packet */
	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	rs->nd_rs_code = 0;
	rs->nd_rs_cksum = 0; /* computed by the kernel */
	rs->nd_rs_reserved = 0;
	return sizeof (*rs);
}


static void
print32time (uint32_t v)
{
	v = ntohl (v);

	if (v == 0xffffffff)
		fputs (_("    infinite (0xffffffff)\n"), stdout);
	else
		printf (_("%12u (0x%08x) %s\n"),
		        v, v, ngettext ("second", "seconds", v));
}


static int
parseprefix (const struct nd_opt_prefix_info *pi, size_t optlen, bool verbose)
{
	char str[INET6_ADDRSTRLEN];

	if (optlen < sizeof (*pi))
		return -1;

#if 0
	/* IOL test */
	/* displays flags information */
	//>>>>
	int lflag = (pi->nd_opt_pi_flags_reserved & 0x80)? 1:0;
	if (verbose)
	{
		fputs (_(" L flag                   : "), stdout);
		printf ("%u\n", lflag);
	}
	if(!lflag) return -1;
	//<<<<
#endif

	/* displays flags information */
	//>>>>
	int lflag = (pi->nd_opt_pi_flags_reserved & 0x80)? 1:0;
	int aflag = (pi->nd_opt_pi_flags_reserved & 0x40)? 1:0;
	if (verbose)
	{
		fputs (_(" L flag                   : "), stdout);
		printf ("%u\n", lflag);
		fputs (_(" A flag                   : "), stdout);
		printf ("%u\n", aflag);
	}
	//<<<<
	
	/* displays prefix informations */
	if (inet_ntop (AF_INET6, &pi->nd_opt_pi_prefix, str,
	               sizeof (str)) == NULL)
		return -1;

	//IOL test  2011/11/08
	/*
	if(lflag)
	{
		char temp[128];
		sprintf(temp,"ip -6 route add %s/%u dev %s", str, pi->nd_opt_pi_prefix_len, g_ifname);
		system(temp);
	}
	*/

	if(!aflag) return 0;

	if (verbose)
	{
		fputs (_(" Prefix                   : "), stdout);
		printf ("%s/%u\n", str, pi->nd_opt_pi_prefix_len);
	}
	
	/* rbj */
	/* >>>> */
	script_setenv("PREFIX",str);
	sprintf(str,"%u",pi->nd_opt_pi_prefix_len);
	script_setenv("PFXLEN",str);
	/* <<<< */

	if (verbose)
	{	
		fputs (_("  Valid time              : "), stdout);
		print32time (pi->nd_opt_pi_valid_time);
		fputs (_("  Pref. time              : "), stdout);
		print32time (pi->nd_opt_pi_preferred_time);
	}

	/* rbj */
	/* >>>> */
	sprintf(str,"%u",ntohl(pi->nd_opt_pi_valid_time));
	script_setenv("PIVLFT",str);
	sprintf(str,"%u",ntohl(pi->nd_opt_pi_preferred_time));
	script_setenv("PIPLFT",str);
	/* <<<< */

	if(is_STATELESS)
		update_ia(pi);
	return 0;
}


static void
parsemtu (const struct nd_opt_mtu *m)
{
	unsigned mtu = ntohl (m->nd_opt_mtu_mtu);
	char str[64];
	if (gverbose & NDISC_VERBOSE)
	{
		fputs (_(" MTU                      : "), stdout);
		printf ("       %5u %s (%s)\n", mtu,
	        	ngettext ("byte", "bytes", mtu),
				gettext((mtu >= 1280) ? N_("valid") : N_("invalid")));
	}
	sprintf(str,"%u",mtu);
	script_setenv("MTU",str);
}


static const char *
pref_i2n (unsigned val)
{
	static const char *values[] =
		{ N_("medium"), N_("high"), N_("medium (invalid)"), N_("low") };
	return gettext (values[(val >> 3) & 3]);
}


static int
parseroute (const uint8_t *opt)
{
	uint8_t optlen = opt[1], plen = opt[2];
	if ((optlen > 3) || (plen > 128) || (optlen < ((plen + 127) >> 6)))
		return -1;

	char str[INET6_ADDRSTRLEN];
	struct in6_addr dst = in6addr_any;
	memcpy (dst.s6_addr, opt + 8, (optlen - 1) << 3);
	if (inet_ntop (AF_INET6, &dst, str, sizeof (str)) == NULL)
		return -1;

	if (gverbose & NDISC_VERBOSE)
	{
		printf (_(" Route                    : %s/%"PRIu8"\n"), str, plen);
		printf (_("  Route preference        :       %6s\n"), pref_i2n (opt[3]));
		fputs (_("  Route lifetime          : "), stdout);
		print32time (((const uint32_t *)opt)[1]);
	}

	return 0;
}


static int
parserdnss (const uint8_t *opt)
{
	uint8_t optlen = opt[1];
	char envstr[128];//rbj
	if (((optlen & 1) == 0) || (optlen < 3))
		return -1;

	optlen /= 2;
	for (unsigned i = 0; i < optlen; i++)
	{
		char str[INET6_ADDRSTRLEN];

		if (inet_ntop (AF_INET6, opt + (16 * i + 8), str,
		               sizeof (str)) == NULL)
			return -1;

		if (gverbose & NDISC_VERBOSE)
			printf (_(" Recursive DNS server     : %s\n"), str);
		if(i==0)//rbj
			strcpy(envstr,str);
		else
			strcat(envstr,str);
		strcat(envstr," ");//rbj
	}
	script_setenv("RDNSS",envstr);//rbj

	if (gverbose & NDISC_VERBOSE)
	{
		fputs (ngettext ("  DNS server lifetime     : ",
	        	         "  DNS servers lifetime    : ", optlen), stdout);
		print32time (((const uint32_t *)opt)[1]);
	}
	sprintf(envstr,"%u",ntohl(((const uint32_t *)opt)[1]));
	script_setenv("RDNSSLFT",envstr);//rbj
	return 0;
}


//for IOL
//DNSSL
#define MAXDNAME 255
static int
parsednssl (const uint8_t *opt)
{
	uint8_t optlen = opt[1];
	char name[MAXDNAME+1];
	char *cp, *ep, *sp;
	int i = 0, l;
	char tmpbuf[MAXDNAME+1];
	char envstr[MAXDNAME+1];

	if (((optlen & 1) == 0) || (optlen < 3))
		return -1;

	memset(name, 0, sizeof(name));

	sp = opt+8;
	cp = sp;
	ep = opt+optlen*8;

	if(cp>= ep) return -1;

	while(cp < ep)
	{
		i = *cp;
		if (i == 0 || cp != sp)
		{
			if(strncat((char *)name, ".", sizeof(name))==NULL)
				return -1; /* result overrun */
		}
		if (i == 0) break;

		cp++;

		if (i > 0x3f) return -1; /* invalid label */

		if (i > ep - cp)
			return -1;

		while(i-- > 0 && cp < ep)
		{
			if(!isprint(*cp)) /* we don't accept non-printables */
				return -1;	

			l = snprintf(tmpbuf, sizeof(tmpbuf), "%c", *cp);		
			if(l > sizeof(tmpbuf) || l < 0) return -1;
			//if(strlcat(name, tmpbuf, sizeof(name)) >= sizeof(name))
			//if(strncat(name, tmpbuf, sizeof(name)))
			if(strcat(name, tmpbuf) == NULL)
				return -1;

			cp++;
		}
	}	
	if( i != 0) return -1; /* not termined */

	if (gverbose & NDISC_VERBOSE)
		printf("DNSSL : %s\n", name);		
	sprintf(envstr,"%s",name);
	script_setenv("DNSSL",envstr);//rbj
	return 0;
}

static int
parseadv (const uint8_t *buf, size_t len, const struct sockaddr_in6 *tgt,
          bool verbose)
{
	const struct nd_router_advert *ra =
		(const struct nd_router_advert *)buf;
	const uint8_t *ptr;

	(void)tgt;

	/* checks if the packet is a Router Advertisement */
	if ((len < sizeof (struct nd_router_advert))
	 || (ra->nd_ra_type != ND_ROUTER_ADVERT)
	 || (ra->nd_ra_code != 0))
		return -1;

	//if (verbose)
	if(1)
	{
		unsigned v;
		char str[64];//rbj

		/* Hop limit */
		if(verbose)
		{
			puts ("");
			fputs (_("Hop limit                 :    "), stdout);
		}
		v = ra->nd_ra_curhoplimit;
		if (v != 0)
		{
			if(verbose)
				printf (_("      %3u"), v);
			sprintf(str,"%d",v);
			script_setenv("HOPLIMIT", str);//rbj
		}
		else
		{
			if(verbose)
				fputs (_("undefined"), stdout);
		}
		if(verbose)
			printf (_(" (      0x%02x)\n"), v);

		v = ra->nd_ra_flags_reserved;

		if(verbose)
		{
			printf (_("Stateful address conf.    :          %3s\n"),
		        	gettext ((v & ND_RA_FLAG_MANAGED) ? N_ ("Yes") : N_("No")));
			printf (_("Stateful other conf.      :          %3s\n"),
		        	gettext ((v & ND_RA_FLAG_OTHER) ? N_ ("Yes") : N_("No")));
			printf (_("Router preference         :       %6s\n"), pref_i2n (v));
		}

		//rbj
		(v & ND_RA_FLAG_MANAGED) ? script_setenv("MFLAG","1") : script_setenv("MFLAG", "0");
		(v & ND_RA_FLAG_OTHER) ? script_setenv("OFLAG","1") : script_setenv("OFLAG", "0");
		script_setenv("PREFERENCE",pref_i2n(v));
		//is_STATELESS = (v & ND_RA_FLAG_MANAGED)? false:true;
		is_STATELESS = true; /* TR-124 issue 2. no matter m flag is, always use this prefix */

		/* Router lifetime */
		if(verbose)
			fputs (_("Router lifetime           : "), stdout);
		v = ntohs (ra->nd_ra_router_lifetime);
		if(verbose)
			printf (_("%12u (0x%08x) %s\n"), v, v,
		        	ngettext ("second", "seconds", v));

		//rbj
		sprintf(str,"%d",v);
		script_setenv("ROUTERLFT", str);//rbj

		//IOL test, check router lifetime if zero
		//>>>>
		if(v==0)
		{
			//char tmp[64];
			printf("Receive zero ra lifetime, remove default route\n");
			rm_gw = true;
			system("ip -6 route del default");
			//sprintf(tmp,"echo 1 > /var/run/%s_ralft_zero",g_ifname);
			system("echo 1 > /var/run/wan_ralft_zero");
		}
		rtlft = v;
		//<<<<
		

		/* ND Reachable time */
		if(verbose)
			fputs (_("Reachable time            : "), stdout);
		v = ntohl (ra->nd_ra_reachable);

		if(verbose)
		{
			if (v != 0)
				printf (_("%12u (0x%08x) %s\n"), v, v,
			        	ngettext ("millisecond", "milliseconds", v));
			else
				fputs (_(" unspecified (0x00000000)\n"), stdout);
		}
		//rbj
		if (v != 0)
			script_setenv("REACHLFT", str);//rbj

		/* ND Retransmit time */
		if(verbose)
			fputs (_("Retransmit time           : "), stdout);
		v = ntohl (ra->nd_ra_retransmit);

		if(verbose)
		{
			if (v != 0)
				printf (_("%12u (0x%08x) %s\n"), v, v,
			        	ngettext ("millisecond", "milliseconds", v));
			else
				fputs (_(" unspecified (0x00000000)\n"), stdout);
		}
		//rbj
		if (v != 0)
			script_setenv("RETRANSLFT", str);//rbj
	}
	len -= sizeof (struct nd_router_advert);

	/* parses options */
	ptr = buf + sizeof (struct nd_router_advert);

	while (len >= 8)
	{
		uint16_t optlen;

		optlen = ((uint16_t)(ptr[1])) << 3;
		if ((optlen == 0) /* invalid length */
		 || (len < optlen) /* length > remaining bytes */)
			break;

		len -= optlen;

		/* only prefix are shown if not verbose */
		//switch (ptr[0] * (verbose ? 1
		//                          : (ptr[0] == ND_OPT_PREFIX_INFORMATION)))
		switch (ptr[0])
		{
			case ND_OPT_SOURCE_LINKADDR:
				if(verbose)
				{
					fputs (_(" Source link-layer address: "), stdout);
					printmacaddress (ptr + 2, optlen - 2);
				}
				setenv_macaddress(ptr+2, optlen-2);//rbj
				break;

			case ND_OPT_TARGET_LINKADDR:
				break; /* ignore */

			case ND_OPT_PREFIX_INFORMATION:
				if (parseprefix ((const struct nd_opt_prefix_info *)ptr,
				                 optlen, verbose))
					return -1;

			case ND_OPT_REDIRECTED_HEADER:
				break; /* ignore */

			case ND_OPT_MTU:
				parsemtu ((const struct nd_opt_mtu *)ptr);
				break;

			case 24: // RFC4191
				parseroute (ptr);
				break;

			case 25: // RFC5006
				parserdnss (ptr);
				break;

			case 31: // RFC6106  //rbj
				parsednssl (ptr);
				break;
		}
		/* skips unrecognized option */

		ptr += optlen;
	}

	return 0;
}
#endif


static ssize_t
recvfromLL (int fd, void *buf, size_t len, int flags,
            struct sockaddr_in6 *addr)
{
	char cbuf[CMSG_SPACE (sizeof (int))];
	struct iovec iov =
	{
		.iov_base = buf,
		.iov_len = len
	};
	struct msghdr hdr =
	{
		.msg_name = addr,
		.msg_namelen = sizeof (*addr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = cbuf,
		.msg_controllen = sizeof (cbuf)
	};

	ssize_t val = recvmsg (fd, &hdr, flags);
	if (val == -1)
		return val;

	/* ensures the hop limit is 255 */
	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR (&hdr);
	     cmsg != NULL;
	     cmsg = CMSG_NXTHDR (&hdr, cmsg))
	{
		if ((cmsg->cmsg_level == IPPROTO_IPV6)
		 && (cmsg->cmsg_type == IPV6_HOPLIMIT))
		{
			if (255 != *(int *)CMSG_DATA (cmsg))
			{
				// pretend to be a spurious wake-up
				errno = EAGAIN;
				return -1;
			}
		}
	}

	return val;
}


static ssize_t
recvadv (int fd, const struct sockaddr_in6 *tgt, unsigned wait_ms,
         unsigned flags)
{
	struct timespec now, end;
	unsigned responses = 0;

	/* computes deadline time */
	mono_gettime (&now);
	{
		div_t d;
		
		d = div (wait_ms, 1000);
		end.tv_sec = now.tv_sec + d.quot;
		end.tv_nsec = now.tv_nsec + (d.rem * 1000000);
	}

	/* receive loop */
	for (;;)
	{
		/* waits for reply until deadline */
		ssize_t val = 0;
		if (end.tv_sec >= now.tv_sec)
		{
			val = (end.tv_sec - now.tv_sec) * 1000
				+ (int)((end.tv_nsec - now.tv_nsec) / 1000000);
			if (val < 0)
				val = 0;
		}

		val = poll (&(struct pollfd){ .fd = fd, .events = POLLIN }, 1, val);
		if (val < 0)
			break;

		if (val == 0)
			return responses;

		/* receives an ICMPv6 packet */
		// TODO: use interface MTU as buffer size
		uint8_t buf[1460];
		struct sockaddr_in6 addr;

		val = recvfromLL (fd, buf, sizeof (buf), MSG_DONTWAIT, &addr);
		if (val == -1)
		{
			if (errno != EAGAIN)
				perror (_("Receiving ICMPv6 packet"));
			continue;
		}

		/* ensures the response came through the right interface */
		if (addr.sin6_scope_id
		 && (addr.sin6_scope_id != tgt->sin6_scope_id))
			continue;

		if (parseadv (buf, val, tgt, (flags & NDISC_VERBOSE) != 0) == 0)
		{
			char str[INET6_ADDRSTRLEN];
			if (inet_ntop (AF_INET6, &addr.sin6_addr, str,
					sizeof (str)) != NULL)
			{
				if (flags & NDISC_VERBOSE)
					printf (_(" from %s\n"), str);
				script_setenv("LLADDR",str);//rbj

				//IOL test
				//>>>>
				if(rm_gw && rtlft != 0)
				{
					//restore default route
					printf("Add default gw\n");
					char tmp[64];
					sprintf(tmp,"ip -6 route add default via %s dev %s",str,g_ifname);		
					system(tmp);
					rm_gw = false;
					//sprintf(tmp,"rm -f /var/run/%s_ralft_zero",g_ifname);
					system("rm -f /var/run/wan_ralft_zero");
				}
				//<<<<
			}
			if (responses < INT_MAX)
				responses++;

			if (flags & NDISC_SINGLE)
				return 1 /* = responses */;
		}
		mono_gettime (&now);
	}

	return -1; /* error */
}


static int fd;

static int
ndisc (const char *name, const char *ifname, unsigned flags, unsigned retry,
       unsigned wait_ms)
{
	struct sockaddr_in6 tgt;
	struct sigaction act;//rbj
	//sigset_t handled;//rbj
	bool found = false;//rbj
	bool run_timer = false;

	if (fd == -1)
	{
		perror (_("Raw IPv6 socket"));
		return -1;
	}

	fcntl (fd, F_SETFD, FD_CLOEXEC);

	/* set ICMPv6 filter */
	{
		struct icmp6_filter f;

		ICMP6_FILTER_SETBLOCKALL (&f);
		ICMP6_FILTER_SETPASS (nd_type_advert, &f);
		setsockopt (fd, IPPROTO_ICMPV6, ICMP6_FILTER, &f, sizeof (f));
	}

	setsockopt (fd, SOL_SOCKET, SO_DONTROUTE, &(int){ 1 }, sizeof (int));

	/* sets Hop-by-hop limit to 255 */
	sethoplimit (fd, 255);
	setsockopt (fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
	            &(int){ 1 }, sizeof (int));

	/* rbj */
	//>>>>
	//sigemptyset(&handled);
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = term_handler;
	sigaction(SIGTERM, &act, NULL);
	//sigaddset(&handled, SIGTERM);

	act.sa_handler = term_handler;
	sigaction(SIGINT, &act, NULL);
	//sigaddset(&handled, SIGINT);

	act.sa_handler = ndisc_alarm_handler;
	//sigaddset(&handled, SIGALRM);
	sigaction(SIGALRM , &act, NULL);
	//<<<<

	/* resolves target's IPv6 address */
	if (getipv6byname (name, ifname, (flags & NDISC_NUMERIC) ? 1 : 0, &tgt))
		goto error;
	else
	{
		char s[INET6_ADDRSTRLEN];

		inet_ntop (AF_INET6, &tgt.sin6_addr, s, sizeof (s));
		strcpy(g_ifname,ifname);//rbj
		if (flags & NDISC_VERBOSE)
			printf (_("Soliciting %s (%s) on %s...\n"), name, s, ifname);
	}

	{
		solicit_packet packet;
		struct sockaddr_in6 dst;
		ssize_t plen;

		memcpy (&dst, &tgt, sizeof (dst));
		plen = buildsol (&packet, &dst, ifname);
		if (plen == -1)
			goto error;

		//while (retry > 0)//rbj
		while (retry > 0 && termsig == 0)
		{
#if 0  //rbj
			/* sends a Solitication */
			if (sendto (fd, &packet, plen, 0,
			            (const struct sockaddr *)&dst,
			            sizeof (dst)) != plen)
			{
				perror (_("Sending ICMPv6 packet"));
				goto error;
			}
			retry--;
#endif
			if(!found)//rbj
			{
				/* sends a Solitication */
				if (sendto (fd, &packet, plen, 0,
			        	    (const struct sockaddr *)&dst,
			            	sizeof (dst)) != plen)
				{
					perror (_("Sending ICMPv6 packet"));
					goto error;
				}
				//printf("sends a Solicitation\n");//rbj
				retry--;
			}

			if(retry==0 && is_forever)  /* for cable network */ 
			{
				retry = 1;
				found = true;
			}

			/* receives an Advertisement */
			ssize_t val = recvadv (fd, &tgt, wait_ms, flags);
			if (val > 0)
			{
				script_setenv("IFNAME",ifname);
				//if(!found)
				//	run_program(configfile,1);//rbj
				if(!script_run)
				{
					script_run = true;
					run_program(configfile,1);//rbj
				}
				found = true;//rbj
				retry = 1;//rbj
				if(state == NDISC_INIT) state = NDISC_RECV;
				//close (fd); //rbj for daemon
				//return 0; //rbj for daemon
				//goto success; //rbj for daemon

				if(!run_timer && is_STATELESS) 
				{
					struct itimerval t;
					/* find the min lft time */
					t.it_interval.tv_sec = find_min_lifetime();
					t.it_interval.tv_usec = 0;
					t.it_value.tv_sec = t.it_interval.tv_sec;
					t.it_value.tv_usec = 0;
	
					set_pi_timer(&t);
					run_timer = true;
				}
			}
			else
			if (val == 0)
			{
				if (flags & NDISC_VERBOSE)
					puts (_("Timed out."));
			}
			else
			{
				if(gverbose) printf("recv error: %s\n",strerror(errno));
				//goto error;
			}
			//if we need to check timer
			if(need_check_timer)
			{
				check_pi_lifetime();
				need_check_timer = false;	
			}
		}
	}

	if(is_STATELESS)
		remove_pi_timer();
	cleanup_addr();
	close (fd);
	unlink(pid_file);
	if (flags & NDISC_VERBOSE)
		puts (_("No response."));
	//return -2;
	return 0;

#if 0
//success://rbj
	remove_pi_timer();
	unlink(pid_file);
	cleanup_addr();
	close (fd); 
	return 0; 
#endif

error:
	remove_pi_timer();
	cleanup_addr();
	close (fd);
	return -1;
}


static int
quick_usage (const char *path)
{
	fprintf (stderr, _("Try \"%s -h\" for more information.\n"), path);
	return 2;
}


static int
usage (const char *path)
{
	printf (gettext (ndisc_usage), path);

	printf (_("\n"
"  -1, --single   display first response and exit\n"
"  -h, --help     display this help and exit\n"
"  -m, --multiple wait and display all responses\n"
"  -n, --numeric  don't resolve host names\n"
"  -q, --quiet    only print the %s (mainly for scripts)\n"
"  -r, --retry    maximum number of attempts (default: 3)\n"
"  -V, --version  display program version and exit\n"
"  -v, --verbose  verbose display (this is the default)\n"
"  -c, --config   config file\n"//rbj
"  -e, --eui64    eui64\n"//rbj
"  -f, --forever  wait RA forever\n"//rbj
"  -w, --wait     how long to wait for a response [ms] (default: 1000)\n"
	           "\n"), gettext (ndisc_dataname));

	return 0;
}


static int
version (void)
{
	printf (_(
"ndisc6: IPv6 Neighbor/Router Discovery userland tool %s (%s)\n"), VERSION, "$Rev: 625 $");
	printf (_(" built %s on %s\n"), __DATE__, PACKAGE_BUILD_HOSTNAME);

	printf (_("Configured with: %s\n"), PACKAGE_CONFIGURE_INVOCATION);
	puts (_("Written by Remi Denis-Courmont\n"));

	printf (_("Copyright (C) %u-%u Remi Denis-Courmont\n"), 2004, 2007);
	puts (_("This is free software; see the source for copying conditions.\n"
	        "There is NO warranty; not even for MERCHANTABILITY or\n"
	        "FITNESS FOR A PARTICULAR PURPOSE.\n"));
	return 0;
}


static const struct option opts[] = 
{
	{ "single",   no_argument,       NULL, '1' },
	{ "help",     no_argument,       NULL, 'h' },
	{ "multiple", required_argument, NULL, 'm' },
	{ "numeric",  no_argument,       NULL, 'n' },
	{ "quiet",    no_argument,       NULL, 'q' },
	{ "retry",    required_argument, NULL, 'r' },
	{ "version",  no_argument,       NULL, 'V' },
	{ "verbose",  no_argument,       NULL, 'v' },
	{ "config",   no_argument,       NULL, 'c' },
	{ "eui64",    no_argument,       NULL, 'e' },
	{ "forever",    no_argument,     NULL, 'f' },
	{ "wait",     required_argument, NULL, 'w' },
	{ NULL,       0,                 NULL, 0   }
};


int
main (int argc, char *argv[])
{
	fd = socket (PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	int errval = errno;
	script_env = NULL;
	script_run = false; /* rbj */
	gverbose = false; /* rbj */
	using_mac = true; /* rbj */
	is_forever = false; /* rbj */
	int pid;/* rbj */
	FILE *pidfp; /* rbj */
	script_envc = 0;

	/* Drops root privileges (if setuid not run by root).
	 * Also make sure the socket is not STDIN/STDOUT/STDERR. */
	if (setuid (getuid ()) || ((fd >= 0) && (fd <= 2)))
		return 1;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	int val;
	unsigned retry = 3, flags = ndisc_default, wait_ms = nd_delay_ms;
	const char *hostname, *ifname;

#ifdef USE_QUEUE
	/* rbj.. Initialize the tail queue */
	TAILQ_INIT(&ndisc_pi_head);
#else
	pi_entry_cnt = 0;
	memset(ndisc_pi_list, 0, sizeof(struct pi_entry));	
#endif
	state = NDISC_INIT;	
	need_check_timer = false;
	is_STATELESS = false;

	while ((val = getopt_long (argc, argv, "1hmnqfr:Vvc:w:e:p:", opts, NULL)) != EOF)//rbj
	{
		switch (val)
		{
			case '1':
				flags |= NDISC_SINGLE;
				break;

			case 'h':
				return usage (argv[0]);

			case 'm':
				flags &= ~NDISC_SINGLE;
				break;

			case 'n':
				flags |= NDISC_NUMERIC;
				break;

			case 'q':
				flags &= ~NDISC_VERBOSE;
				break;

			case 'r':
			{
				unsigned long l;
				char *end;

				l = strtoul (optarg, &end, 0);
				if (*end || l > UINT_MAX)
					return quick_usage (argv[0]);
				retry = l;
				break;
			}
				
			case 'V':
				return version ();

			case 'v':
				/* NOTE: assume NDISC_VERBOSE occupies low-order bits */
				if ((flags & NDISC_VERBOSE) < NDISC_VERBOSE)
					flags++;
				break;

			case 'c'://rbj
				if(optarg!=NULL)
				{
					strcpy(configfile,optarg);
					if(gverbose) printf("The config file is %s\n",configfile);
				}
				else
					printf("Please input the file name\n");				
				break;

			case 'e'://rbj //eui64
				if(optarg!=NULL)
				{
					struct in6_addr addr;
					inet_pton(AF_INET6,optarg,&addr);
					memcpy(eui64,addr.s6_addr+8,8);
					using_mac = false;
					if(gverbose) printf("The host identity is %s\n",optarg);
				}
				else
					printf("Please input the host identity\n");				
				break;

			case 'f':/* rbj */
				is_forever = true;
				break;

			case 'p': //rbj
				pid_file = optarg;
				break;

			case 'w':
			{
				unsigned long l;
				char *end;

				l = strtoul (optarg, &end, 0);
				if (*end || l > UINT_MAX)
					return quick_usage (argv[0]);
				wait_ms = l;
				break;
			}

			case '?':
			default:
				return quick_usage (argv[0]);
		}
	}

	gverbose = flags;

	if (optind < argc)
	{
		hostname = argv[optind++];

		if (optind < argc)
			ifname = argv[optind++];
		else
			ifname = NULL;
	}
	else
		return quick_usage (argv[0]);

#ifdef RDISC
	if (ifname == NULL)
	{
		ifname = hostname;
		hostname = "ff02::2";
	}
	else
#endif
	if ((optind != argc) || (ifname == NULL))
		return quick_usage (argv[0]);

	/* dump current PID */ //rbj
	pid = getpid();
	if((pidfp = fopen(pid_file,"w")) != NULL)
	{
		fprintf(pidfp, "%d\n", pid);
		fclose(pidfp);
	} 

	errno = errval; /* restore socket() error value */
	return -ndisc (hostname, ifname, flags, retry, wait_ms);
}


//add by rbj
void script_setenv(char *var, char *value)
{
	size_t var1 = strlen(var);
	size_t v1 = var1 + strlen(value) +2;
	int i;
	char *p, *newstring;

	//printf("%s: parse %s=%s\n",__FUNCTION__,var,value);	
	//newstring = (char *)malloc(v1+1);
	newstring = (char *)malloc(v1);
	if(newstring == 0) return;
	//slprintf(newstring, v1, "%s=%s", var, value);
	//memset(newstring, 0, (v1));
	sprintf(newstring,"%s=%s",var,value);
	
	/* check if this variable is already set */
	if(script_env != 0)
	{
		for(i = 0; (p = script_env[i]) != 0; ++i)
		{
			if(strncmp(p, var, var1) == 0 && p[var1] == '=')
			{
				//free(p-1);
				free(p);
				script_env[i] = newstring;
				return;
			}
		} 
	}
	else
	{
		/* no space allocated for script env. ptrs. yet */
		i = 0;
		script_env = (char **)malloc(16 * sizeof(char *));
		if (script_env == 0)
		{
			if(newstring) free(newstring);
			return;
		}
		s_env_nalloc = 16;
	}

	/* reallocate script_env with more space if needed */
	if (i+1 >= s_env_nalloc)
	{
		int new_n = i + 17;
		char **newenv = (char **)realloc((void *)script_env,
					new_n * sizeof(char *));
		if(newenv == 0)
		{
			if(newstring) free(newstring);
			return;
		}
		script_env = newenv;
		s_env_nalloc = new_n;
	}

	script_env[i] = newstring;
	//printf("script_env[%d]: %s\n",i,script_env[i]);	
	script_env[i+1] = 0;
	script_envc++;
}

pid_t run_program(char *prog,int must_exist)
{
	int pid;
	int ret = 0;
	struct stat sbuf;
	errno = EINVAL;

	if(gverbose) printf("Enter %s\n",__FUNCTION__);	
	if(stat(prog, &sbuf) < 0 || !S_ISREG(sbuf.st_mode) ||
		(sbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))==0)
	{
		//if(must_exist || errno != ENOENT) warn("Can't execute %s:%m",prog);
		ret = -1;
		goto clean;
	}
	pid = fork();
	if(pid < 0)
	{
		//error("Failed to create child process for %s: %m",prog);
		ret = -1;
		goto clean;
	}
	else if(pid)
	{
		int pid_status;
		pid_t wpid;
#if 0
		if(waitpid(pid, &pid_status, 0) < 0)
		{
			;//error("error waiting for run_program fork children : %m");
		}
		return 0;
#endif
		do{
			wpid = wait(&pid_status);
		}while(wpid != pid && wpid >0);
	}
	else
	{
		char *argv[2];

		argv[0] = prog;
		argv[1] = NULL;
		//printf("%s\n",script_env[0]);
		/* run the program */
		execve(prog, argv, script_env);
		exit(0);
	}

clean:
	if(gverbose) printf("cleaning malloc: %d, actual envc: %d\n",s_env_nalloc,script_envc);
	for(int i=0; i < script_envc; i++)
	{
		free(script_env[i]);
	}
	free(script_env);
	script_env = 0;
	script_envc = 0;
	return ret;	
}

char *addr2str(struct sockaddr *sa)
{
	static char addbuf[8][NI_MAXHOST];
	static int round = 0;
	char *cp;
	
	round = (round +1)%7;
	cp = addbuf[round];
	
	getnameinfo(sa,sizeof(struct sockaddr_in6),cp,
			NI_MAXHOST,NULL,0,NI_NUMERICHOST);
	return cp;
}

char *in6addr2str(struct in6_addr *in6,
		int scopeid
		)
{
	struct sockaddr_in6 sa6;
	
	memset(&sa6, 0, sizeof(sa6));
	sa6.sin6_family = AF_INET6;
	sa6.sin6_addr = *in6;
	sa6.sin6_scope_id = scopeid;
	
	return (addr2str((struct sockadr *)&sa6));
}
int generate_ip6_eui64(struct in6_addr *addr,
			char *addrprefix)
{
	uint8_t macaddr[6];
	char *j;

	//printf("Enter %s, interface name : %s\n",__FUNCTION__,g_ifname);
	if(!using_mac)
	{
		j = eui64;
		memcpy(addr->s6_addr,addrprefix,8);
		memcpy(addr->s6_addr+8,j,8);
	}
	else
	{
		getmacaddress(g_ifname,macaddr);
		j = macaddr;
		memcpy(addr->s6_addr,addrprefix,8);
		memcpy(addr->s6_addr+8,j,3);
		memcpy(addr->s6_addr+13,j+3,3);
		addr->s6_addr[11] = 0xff;
		addr->s6_addr[12] = 0xfe;
		addr->s6_addr[8]^=2;
	}
	return 0;
}

int ifaddrconf(ifaddrconf_cmd_t cmd,
		char *ifname,
		struct sockaddr_in6 *addr,
		int plen)
{
	struct in6_ifreq req;
	struct ifreq ifr;
	unsigned long ioctl_cmd;
	char *cmdstr;
	int s;

	switch(cmd)
	{
		case IFADDRCONF_ADD:
			cmdstr = "add";
			ioctl_cmd = SIOCSIFADDR;
			break;

		case IFADDRCONF_REMOVE:
			cmdstr = "remove";
			ioctl_cmd = SIOCDIFADDR;
			break;

		default:
			return (-1);
	}

	if((s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Cannot open a temporary socket: %s",strerror(errno));
		return (-1);
	}

	memset(&req, 0, sizeof(req));
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name,ifname,IFNAMSIZ-1);
	if(ioctl(s,SIOGIFINDEX,&ifr)<0)
	{
		printf("Failed to get the index of %s: %s\n",ifname,strerror(errno));
		close(s);
		return (-1);
	}
	memcpy(&req.ifr6_addr,&addr->sin6_addr,sizeof(struct in6_addr));
	req.ifr6_prefixlen = plen;
	req.ifr6_ifindex = ifr.ifr_ifindex;
	
	if(ioctl(s,ioctl_cmd,&req))
	{
		printf("Failed to %s an address on %s: %s\n",
			cmdstr,ifname,strerror(errno));
		close(s);
		return(-1);
	}
	printf("%s an address %s/%d on %s\n",cmdstr,
			addr2str((struct sockaddr *)addr),plen,ifname);
	close(s);
	return 0;
}

int na_ifaddrconf(ifaddrconf_cmd_t cmd,
		struct in6_addr *addr)
{
	struct sockaddr_in6 sin6;
	
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	//sin6.sin6_len = sizeof(sin6);
	sin6.sin6_addr = *addr;

	//return (ifaddrconf(cmd,g_ifname,&sin6,128));	
	return (ifaddrconf(cmd,g_ifname,&sin6,64));//rbj	
}

/* UNH-IOL */
int na_ifaddrconf_128(ifaddrconf_cmd_t cmd,
		struct in6_addr *addr)
{
	struct sockaddr_in6 sin6;
	
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	//sin6.sin6_len = sizeof(sin6);
	sin6.sin6_addr = *addr;

	return (ifaddrconf(cmd,g_ifname,&sin6,128));//rbj	
}

int remove_addr(struct in6_addr *addr)
{
	int ret;
	//printf("Remove an address %s\n",in6addr2str(addr,0));
	ret = na_ifaddrconf(IFADDRCONF_REMOVE,addr);
	return (ret);
}

int remove_addr_128(struct in6_addr *addr)
{
	int ret;
	//printf("Remove an address %s\n",in6addr2str(addr,0));
	ret = na_ifaddrconf_128(IFADDRCONF_REMOVE,addr);
	return (ret);
}

void cleanup_addr()
{
	struct pi_entry *item;
	struct in6_addr address;

#ifdef USE_QUEUE	
	while((item = TAILQ_FIRST(&ndisc_pi_head))!= NULL)
	{
		generate_ip6_eui64(&address,(char *)&item->addr);

		//UNH-IOL		
		if(item->lft_forever==false)
		{
			char tmpstr[128];
			struct sockaddr_in6 sin6;

			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			//sin6.sin6_len = sizeof(sin6);
			sin6.sin6_addr = address;

			memset(tmpstr, 0, sizeof(tmpstr));
			sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft forever preferred_lft forever",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname);	
			//printf(tmpstr);
			system(tmpstr);
		}

		//remove_addr(&address);
		//UNH-IOL		
		if(item->prefix_len==128)
		{
			remove_addr_128(&address);
		}
		else
		{
			remove_addr(&address);
		}
		TAILQ_REMOVE(&ndisc_pi_head, item, entries);
		free(item);
	}
#else
	for(int i = 0;i < MAX_PI_ENTRY; i++)
	{
		item = ndisc_pi_list+i;
		if(!item->use) continue;
		generate_ip6_eui64(&address,(char *)&item->addr);

		//UNH-IOL		
		if(item->lft_forever==false)
		{
			char tmpstr[128];
			struct sockaddr_in6 sin6;

			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			//sin6.sin6_len = sizeof(sin6);
			sin6.sin6_addr = address;

			memset(tmpstr, 0, sizeof(tmpstr));
			sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft forever preferred_lft forever",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname);	
			//printf(tmpstr);
			system(tmpstr);
		}

		//remove_addr(&address);
		//UNH-IOL		
		if(item->prefix_len==128)
		{
			remove_addr_128(&address);
		}
		else
		{
			remove_addr(&address);
		}
	}
#endif
}

static struct pi_entry *find_addr(struct in6_addr *addr)
{
	struct pi_entry *item;
#ifdef USE_QUEUE
	for(item = TAILQ_FIRST(&ndisc_pi_head);item;item = TAILQ_NEXT(item,entries))
	{
		if(!IN6_ARE_ADDR_EQUAL(&item->addr,addr))
			continue;
		return item;
	}
#else
	for(int i = 0;i < MAX_PI_ENTRY; i++)
	{
		item = ndisc_pi_list+i;
		if(!item->use) continue;
		if(!IN6_ARE_ADDR_EQUAL(&item->addr,addr))
			continue;
		return item;
	}
# endif
	return (NULL);
}

int update_ia(const struct nd_opt_prefix_info *pi)
{
	//char str[INET6_ADDRSTRLEN];
	struct pi_entry *item;//,*tmp_item;
	struct in6_addr address;
	int i;
	u_int32_t pltime,vltime,now;

	pltime = ntohl(pi->nd_opt_pi_preferred_time);
	vltime = ntohl(pi->nd_opt_pi_valid_time);
	now = time((time_t *)NULL);
	if(gverbose) printf("Now is %d\n",now);

	/* 
 	* A client discards any addresses for which the preferred
 	* lifetime is greater than the valid lifetime.
 	* [RFC3315 22.6]
 	*/
	if(vltime != NDISC_DURATION_INFINITE &&
	(pltime == NDISC_DURATION_INFINITE || pltime > vltime))
	{
		printf("Invalid address %s:"
		"pltime (%d) is larger than vltime (%d)\n",
		in6addr2str(&pi->nd_opt_pi_prefix,0),
		pltime,vltime);
		return -1;
	}
	if(state == NDISC_INIT)
	{
		if(vltime == 0 || pltime == 0) return -1;
#ifdef USE_QUEUE
		/* add items to the tailq queue */
		item = malloc(sizeof(*item));
		if(item == NULL)
		{
			printf("malloc failed\n");
			exit(-1);
		}
#else
		for(i=0;i < MAX_PI_ENTRY; i++)
		{
			item = ndisc_pi_list+i;
			if(item->use) continue;
			else break;
		}
#endif
		/* set the value */
		if(pltime == NDISC_DURATION_INFINITE)
		{
			item->valid_lft = NDISC_DURATION_INFINITE;
			item->preferred_lft = NDISC_DURATION_INFINITE;
			item->lft_forever = true;
		}
		else
		{
			item->valid_lft = vltime;
			item->preferred_lft = pltime;
			item->lft_forever = false;
		}
		item->prefix_len = pi->nd_opt_pi_prefix_len;
		memcpy(&item->addr,&pi->nd_opt_pi_prefix,sizeof(item->addr));
		generate_ip6_eui64(&address,(char *)&item->addr);

		/* UNH-IOL check L flag */
		/* A=0, L=1 can't set static route for this prefix */
		int lflag = (pi->nd_opt_pi_flags_reserved & 0x80)? 1:0;
		//int aflag = (pi->nd_opt_pi_flags_reserved & 0x40)? 1:0;
		//>>>>
		if(lflag==0)
		{
			/*
			char str[INET6_ADDRSTRLEN];
			char temp[128];
	
			if (inet_ntop (AF_INET6, &pi->nd_opt_pi_prefix, str,
	               		sizeof (str)) == NULL)
				return -1;
			sprintf(temp,"ip -6 route del %s/%u dev %s", str, pi->nd_opt_pi_prefix_len, g_ifname);
			system(temp);
			*/
			/* set ip address */
			na_ifaddrconf_128(IFADDRCONF_ADD,&address);
			item->prefix_len = 128;
		}
		else
		{
		//<<<<

			/* set ip address */
			na_ifaddrconf(IFADDRCONF_ADD,&address);

		}//UNH-IOL

		item->expire_time = now + vltime;
#ifdef USE_QUEUE	
		TAILQ_INSERT_TAIL(&ndisc_pi_head, item, entries);
#else
		item->use = true;
		pi_entry_cnt++;
#endif

		/* add plft and vlft to specific address */
		if(item->lft_forever==false)
		{
			char tmpstr[128];
			struct sockaddr_in6 sin6;
	
			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			//sin6.sin6_len = sizeof(sin6);
			sin6.sin6_addr = address;

			memset(tmpstr, 0, sizeof(tmpstr));
			sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft %d preferred_lft %d",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname, vltime, pltime);	
			//printf(tmpstr);
			system(tmpstr);
		}
	}
	else if(state == NDISC_RECV)
	{
		if((item = find_addr(&pi->nd_opt_pi_prefix)) == NULL)
		{
			if(vltime == 0 || pltime == 0) return -1;
#ifdef USE_QUEUE
			if((item = malloc(sizeof(*item)))==NULL)
			{
				printf("Memory allocation failed\n");
				return (-1);
			}
#else
			item = find_empty_entry();	
			if(item == NULL) 
			{
				printf("Can't find the empty entry(max:%d)\n",MAX_PI_ENTRY);
				return (-1);
			}
#endif
			memset(item, 0, sizeof(*item));
			item->valid_lft = vltime;
			item->preferred_lft = pltime;
			item->prefix_len = pi->nd_opt_pi_prefix_len;
			memcpy(&item->addr,&pi->nd_opt_pi_prefix,sizeof(item->addr));
			generate_ip6_eui64(&address,(char *)&item->addr);

			/* set ip address */
			na_ifaddrconf(IFADDRCONF_ADD,&address);
			if(pltime == NDISC_DURATION_INFINITE)
			{
				item->lft_forever = true;
			}
			else
			{
				item->expire_time = now + vltime;
				item->lft_forever = false;
			}
#ifdef USE_QUEUE
			TAILQ_INSERT_TAIL(&ndisc_pi_head, item, entries);
#else
			item->use = true;
#endif

			/* add plft and vlft to specific address */
			if(item->lft_forever==false)
			{
				char tmpstr[128];
				struct sockaddr_in6 sin6;
	
				memset(&sin6, 0, sizeof(sin6));
				sin6.sin6_family = AF_INET6;
				//sin6.sin6_len = sizeof(sin6);
				sin6.sin6_addr = address;

				memset(tmpstr, 0, sizeof(tmpstr));
				sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft %d preferred_lft %d",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname, vltime, pltime);	
				//printf(tmpstr);
				system(tmpstr);
			}

			/* reset timer */
			struct itimerval t;
			t.it_interval.tv_sec = find_min_lifetime();
			t.it_interval.tv_usec = 0;
			t.it_value.tv_sec = t.it_interval.tv_sec;
			t.it_value.tv_usec = 0;
			set_pi_timer(&t);
		}
		else 
		{
			if(pltime == NDISC_DURATION_INFINITE)
			{
				if(gverbose)
					printf("[NDISC]Update an address %s, pltime= INFINITE, vltime= INFINITE\n",
					in6addr2str(&pi->nd_opt_pi_prefix,0));
			}
			else
			{
				if(gverbose)
					printf("[NDISC]Update an address %s, pltime=%d, vltime=%d\n",
					in6addr2str(&pi->nd_opt_pi_prefix,0),pltime,vltime);
			}
			if(vltime == 0 || pltime == 0)
			{
				memcpy(&item->addr,&pi->nd_opt_pi_prefix,sizeof(item->addr));
				generate_ip6_eui64(&address,(char *)&item->addr);

				//UNH-IOL		
				if(item->lft_forever==false)
				{
					char tmpstr[128];
					struct sockaddr_in6 sin6;
	
					memset(&sin6, 0, sizeof(sin6));
					sin6.sin6_family = AF_INET6;
					//sin6.sin6_len = sizeof(sin6);
					sin6.sin6_addr = address;

					memset(tmpstr, 0, sizeof(tmpstr));
					sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft forever preferred_lft forever",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname);	
					//printf(tmpstr);
					system(tmpstr);
				}

				//remove_addr(&address);
				if(item->prefix_len==128)
				{
					remove_addr_128(&address);
				}
				else
				{
					remove_addr(&address);
				}
#ifdef USE_QUEUE				
				TAILQ_REMOVE(&ndisc_pi_head, item, entries);
#else
				item->use = false;		
#endif
				return 0;
			}	
			if(pltime != NDISC_DURATION_INFINITE)
			{	
				item->valid_lft = vltime;
				item->preferred_lft = pltime;
				item->expire_time = now + vltime;
				item->lft_forever = false;
			}
			else
			{
				item->valid_lft = NDISC_DURATION_INFINITE;
				item->preferred_lft = NDISC_DURATION_INFINITE;
				item->lft_forever = true;
			}

			/* add plft and vlft to specific address */
			if(item->lft_forever==false)
			{
				char tmpstr[128];
				struct sockaddr_in6 sin6;
	
				memcpy(&item->addr,&pi->nd_opt_pi_prefix,sizeof(item->addr));
				generate_ip6_eui64(&address,(char *)&item->addr);

				memset(&sin6, 0, sizeof(sin6));
				sin6.sin6_family = AF_INET6;
				//sin6.sin6_len = sizeof(sin6);
				sin6.sin6_addr = address;

				memset(tmpstr, 0, sizeof(tmpstr));
				sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft %d preferred_lft %d",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname, vltime, pltime);	
				//printf(tmpstr);
				system(tmpstr);
			}
		}
	}

	/* show the tailq queue */
	if(gverbose) showpilist();
	return 0;
}

void showpilist()
{
	struct pi_entry *item;	
	char str[INET6_ADDRSTRLEN];

#ifdef USE_QUEUE	
	printf("Forward traversal[QUEUE]: \n");
#else
	printf("Forward traversal[ARRAY]: \n");
#endif
#ifdef USE_QUEUE
	TAILQ_FOREACH(item, &ndisc_pi_head, entries)
	{
		inet_ntop (AF_INET6, &item->addr, str, sizeof (str));
		if(item->lft_forever)
			printf("%s/%d %d(0x%08x)-expire : INFINITE",str,item->prefix_len,item->valid_lft,item->valid_lft);
		else
			printf("%s/%d %d(0x%08x)-expire : %d ",str,item->prefix_len,item->valid_lft,item->valid_lft,item->expire_time);
		printf("\n");
	}
#else
	for(int i=0;i < MAX_PI_ENTRY;i++)
	{
		item = ndisc_pi_list+i;
		if(!item->use) continue;
		inet_ntop (AF_INET6, &item->addr, str, sizeof (str));
		if(item->lft_forever)
			printf("%s/%d %d(0x%08x)-expire : INFINITE",str,item->prefix_len,item->valid_lft,item->valid_lft);
		else
			printf("%s/%d %d(0x%08x)-expire : %d ",str,item->prefix_len,item->valid_lft,item->valid_lft,item->expire_time);
		printf("\n");
	}
#endif
}

void ndisc_alarm_handler()
{
	if(gverbose) printf("ndisc timer\n");
	if(!need_check_timer)
		need_check_timer = true;
	else 
		return;
#if 0
	struct pi_entry *item;
	u_int32_t now = time((time_t *)NULL);
	char str[INET6_ADDRSTRLEN];

	for(item = TAILQ_FIRST(&ndisc_pi_head);item;item = TAILQ_NEXT(item,entries))
	{
		if(now > item->expire_time)
		{
			inet_ntop (AF_INET6, &item->addr, str, sizeof (str));
			printf("()")
		}
	}
#endif
}

void check_pi_lifetime()
{
	char str[INET6_ADDRSTRLEN];
	struct pi_entry *item;
	struct in6_addr address;
	u_int32_t now = time((time_t *)NULL);
	struct itimerval t;

#ifdef USE_QUEUE
	for(item = TAILQ_FIRST(&ndisc_pi_head);item;item = TAILQ_NEXT(item,entries))
	{
		if(item->lft_forever) continue;
		if(now >= item->expire_time)
		{
			inet_ntop (AF_INET6, &item->addr, str, sizeof (str));
			printf("(%s) is timeout..cleaning\n",str);
			generate_ip6_eui64(&address,&item->addr);
		
			//UNH-IOL		
			if(item->lft_forever==false)
			{
				char tmpstr[128];
				struct sockaddr_in6 sin6;

				memset(&sin6, 0, sizeof(sin6));
				sin6.sin6_family = AF_INET6;
				//sin6.sin6_len = sizeof(sin6);
				sin6.sin6_addr = address;

				memset(tmpstr, 0, sizeof(tmpstr));
				sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft forever preferred_lft forever",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname);	
				//printf(tmpstr);
				system(tmpstr);
			}

			//remove_addr(&address);				
			//UNH-IOL		
			if(item->prefix_len==128)
			{
				remove_addr_128(&address);
			}
			else
			{
				remove_addr(&address);
			}
	
			TAILQ_REMOVE(&ndisc_pi_head, item, entries);
		}
	}
#else
	for(int i=0;i<MAX_PI_ENTRY;i++)
	{
		item = ndisc_pi_list+i;
		if(item->lft_forever) continue;
		if(!item->use) continue;
		if(now >= item->expire_time)
		{
			inet_ntop (AF_INET6, &item->addr, str, sizeof (str));
			printf("(%s) is timeout..cleaning\n",str);
			generate_ip6_eui64(&address,(char *)&item->addr);
	
			//UNH-IOL		
			if(item->lft_forever==false)
			{
				char tmpstr[128];
				struct sockaddr_in6 sin6;

				memset(&sin6, 0, sizeof(sin6));
				sin6.sin6_family = AF_INET6;
				//sin6.sin6_len = sizeof(sin6);
				sin6.sin6_addr = address;

				memset(tmpstr, 0, sizeof(tmpstr));
				sprintf(tmpstr, "ip -6 addr change %s/%d dev %s valid_lft forever preferred_lft forever",addr2str((struct sockaddr *)&sin6), item->prefix_len, g_ifname);	
				//printf(tmpstr);
				system(tmpstr);
			}
		
			//remove_addr(&address);
			//UNH-IOL		
			if(item->prefix_len==128)
			{
				remove_addr_128(&address);
			}
			else
			{
				remove_addr(&address);
			}	
	
			item->use = false;
		}
	}
#endif

	/* reset timer */
	t.it_interval.tv_sec = find_min_lifetime();
	t.it_interval.tv_usec = 0;
	t.it_value.tv_sec = t.it_interval.tv_sec;
	t.it_value.tv_usec = 0;
	set_pi_timer(&t);	
}

u_int32_t find_min_lifetime()
{
	u_int32_t min_lft=0;
	struct pi_entry *item;
	
	if(gverbose) printf("Enter find_min_lifetime\n");
#ifdef USE_QUEUE	
	for(item = TAILQ_FIRST(&ndisc_pi_head);item;item = TAILQ_NEXT(item,entries))
	{
		if(item->lft_forever == true) continue;
		if(min_lft == 0 || min_lft > item->preferred_lft)
		{
			min_lft = item->preferred_lft;
		}
	}
#else
	for(int i=0;i < MAX_PI_ENTRY;i++)
	{
		item = ndisc_pi_list+i;
		if(item->lft_forever == true) continue;
		if(!item->use) continue;
		if(min_lft == 0 || min_lft > item->preferred_lft)
		{
			min_lft = item->preferred_lft;
		}
	}
#endif
	if(min_lft == 0) min_lft = DEFAULT_CHECK_TIME;
	if(gverbose) printf("[%s] min lifetime is %d\n",__FUNCTION__,min_lft);
	return min_lft;
}

int set_pi_timer(struct itimerval *t)
{
	//signal(SIGALRM, ndisc_alarm_handler);
	//if(setitimer(ITIMER_REAL, &t, NULL) < 0)
	//signal(SIGPROF, ndisc_alarm_handler);
	if(setitimer(ITIMER_REAL, t, NULL) < 0)
	{
		if(gverbose) printf("Set timer error\n");
		return -1;
	}
	if(gverbose) printf("Set timer\n");

	return 0;
}
	
void remove_pi_timer()
{
	struct itimerval t;
	t.it_interval.tv_usec = 0;
	t.it_interval.tv_sec = 0;
	t.it_value.tv_usec = 0;
	t.it_value.tv_sec = 0;

	if(setitimer(ITIMER_REAL, &t, NULL) < 0)
	{
		if(gverbose) printf("Remove timer error\n");
		return;
	}
	if(gverbose) printf("Remove timer\n");
	/*signal(SIGALRM, ndisc_alarm_handler);*/

	return;
	
}

#ifndef USE_QUEUE
struct pi_entry *find_empty_entry()
{
	struct pi_entry *item;
	int i;
	for(i = 0;i < MAX_PI_ENTRY; i++)
	{
		item = ndisc_pi_list+i;
		if(!item->use) return item;
	}
	return NULL;
}
#endif



