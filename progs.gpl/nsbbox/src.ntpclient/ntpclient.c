/*
 * ntpclient.c - NTP client
 *
 * Copyright 1997, 1999, 2000  Larry Doolittle  <larry@doolittle.boa.org>
 * Last hack: 2 December, 2000
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License (Version 2,
 *  June 1991) as published by the Free Software Foundation.  At the
 *  time of writing, that license was published by the FSF with the URL
 *  http://www.gnu.org/copyleft/gpl.html, and is incorporated herein by
 *  reference.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Possible future improvements:
 *      - Double check that the originate timestamp in the received packet
 *        corresponds to what we sent.
 *      - Verify that the return packet came from the host we think
 *        we're talking to.  Not necessarily useful since UDP packets
 *        are so easy to forge.
 *      - Complete phase locking code.
 *      - Write more documentation  :-(
 *
 *  Compile with -D_PRECISION_SIOCGSTAMP if your machine really has it.
 *  There are patches floating around to add this to Linux, but
 *  usually you only get an answer to the nearest jiffy.
 *  Hint for Linux hacker wannabes: look at the usage of get_fast_time()
 *  in net/core/dev.c, and its definition in kernel/time.c .
 *
 *  If the compile gives you any flak, check below in the section
 *  labelled "XXXX fixme - non-automatic build configuration".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <unistd.h>
#include <errno.h>
#ifdef _PRECISION_SIOCGSTAMP
#include <sys/ioctl.h>
#endif
#include <fcntl.h>
#include <syslog.h>

#include <asyslog.h>
#include <elbox_config.h>

#define ENABLE_DEBUG

extern char *optarg;

static int timediff=0;
static int result=-1;

/* XXXX fixme - non-automatic build configuration */
#ifdef linux
typedef u_int32_t __u32;
#include <sys/timex.h>
#else
extern int h_errno;
#define herror(hostname) \
	fprintf(stderr,"Error %d looking up hostname %s\n", h_errno,hostname)
typedef uint32_t __u32;
#endif

#define JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via settimeofday) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ( (x) * 15.2587890625 )

struct ntptime {
	unsigned int coarse;
	unsigned int fine;
};

static void send_packet(int usd);
static void rfc1305print(char *data, struct ntptime *arrival);
static void udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len);

/* global variables (I know, bad form, but this is a short program) */
static char incoming[1500];
static struct timeval time_of_send;
static int live=0;
static int set_clock=0;   /* non-zero presumably needs root privs */

#ifdef ENABLE_DEBUG
int debug=0;
#define DEBUG_OPTION "d"
#else
#define debug 0
#define DEBUG_OPTION
#endif

int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

static int get_current_freq()
{
	/* OS dependent routine to get the current value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes=0;
	if (adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

static int set_freq(int new_freq)
{
	/* OS dependent routine to set a new value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes = ADJ_FREQUENCY;
	txc.freq = new_freq;
	if (adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

static void send_packet(int usd)
{
	__u32 data[12];
	struct timeval now;
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6

	if (debug) fprintf(stderr,"Sending ...\n");
	if (sizeof(data) != 48) {
		fprintf(stderr,"size error\n");
		return;
	}
	/* replace bzero() with memset() to avoid compiler warning.
	 * David Hsieh <david_hsieh@alphanetworks.com>
	bzero(data,sizeof(data)); */
	memset(data, 0, sizeof(data));
	data[0] = htonl (
		( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
		( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	data[1] = htonl(1<<16);  /* Root Delay (seconds) */
	data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
	gettimeofday(&now,NULL);
	data[10] = htonl(now.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
	data[11] = htonl(NTPFRAC(now.tv_usec));  /* Transmit Timestamp fine   */
	send(usd,data,48,0);
	time_of_send=now;
}


static void udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len)
{
	struct timeval udp_arrival;
	struct ntptime udp_arrival_ntp;

#ifdef _PRECISION_SIOCGSTAMP
	if ( ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0 ) {
		perror("ioctl-SIOCGSTAMP");
		gettimeofday(&udp_arrival,NULL);
	}
#else
	gettimeofday(&udp_arrival,NULL);
#endif
	udp_arrival_ntp.coarse = udp_arrival.tv_sec + JAN_1970;
	udp_arrival_ntp.fine   = NTPFRAC(udp_arrival.tv_usec);

	if (debug) {
		printf("packet of length %d received\n",data_len);
		printf("Source: Address family %d\n",sa_source->sa_family);
		if (sa_source->sa_family==AF_INET) {
			struct sockaddr_in *sa_in=(struct sockaddr_in *)sa_source;
			printf("Source: INET Port %d host %s\n",
				ntohs(sa_in->sin_port),inet_ntoa(sa_in->sin_addr));
		} else if (sa_source->sa_family==AF_INET6) {
			struct sockaddr_in6 *sa_in=(struct sockaddr_in6 *)sa_source;
			char buf[512];
			printf("Source: INET6 Port %d host %s\n",
				sa_in->sin6_port, inet_ntop(AF_INET6, &sa_in->sin6_addr, buf, sizeof(buf)));
		} else {
			printf("Source: Address family %d\n",sa_source->sa_family);
		}
	}
	rfc1305print(data,&udp_arrival_ntp);
}

static double ntpdiff( struct ntptime *start, struct ntptime *stop)
{
	int a;
	unsigned int b;
	a = stop->coarse - start->coarse;
	if (stop->fine >= start->fine) {
		b = stop->fine - start->fine;
	} else {
		b = start->fine - stop->fine;
		b = ~b;
		a -= 1;
	}
	
	return a*1.e6 + b * (1.e6/4294967296.0);
}

static void rfc1305print(char *data, struct ntptime *arrival)
{
/* straight out of RFC-1305 Appendix A */
	int li, vn, mode, stratum, poll, prec;
	int delay, disp, refid;
	struct ntptime reftime, orgtime, rectime, xmttime;
	double etime,stime,skew1,skew2;
	int freq;

	time_t tm;
	struct tm tm_time;
	char tmp1[80], tmp2[40];

#define Data(i) ntohl(((unsigned int *)data)[i])
	li      = Data(0) >> 30 & 0x03;
	vn      = Data(0) >> 27 & 0x07;
	mode    = Data(0) >> 24 & 0x07;
	stratum = Data(0) >> 16 & 0xff;
	poll    = Data(0) >>  8 & 0xff;
	prec    = Data(0)       & 0xff;
	if (prec & 0x80) prec|=0xffffff00;
	delay   = Data(1);
	disp    = Data(2);
	refid   = Data(3);
	reftime.coarse = Data(4);
	reftime.fine   = Data(5);
	orgtime.coarse = Data(6);
	orgtime.fine   = Data(7);
	rectime.coarse = Data(8);
	rectime.fine   = Data(9);
	xmttime.coarse = Data(10);
	xmttime.fine   = Data(11);
#undef Data

	if (set_clock) {   /* you'd better be root, or ntpclient will crash! */
		struct timeval tv_set;
		/* it would be even better to subtract half the slop */
		tv_set.tv_sec  = xmttime.coarse - JAN_1970+timediff;
		/* divide xmttime.fine by 4294.967296 */
		tv_set.tv_usec = USEC(xmttime.fine);
		//printf("timediff=%d\n", timediff);
		if (settimeofday(&tv_set,NULL)<0) {
			perror("settimeofday");
			exit(1);
		}
		if (debug) {
			printf("set time to %lu.%.6lu\n", tv_set.tv_sec, tv_set.tv_usec);
		}
		time(&tm);
		memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
		strftime(tmp2, sizeof(tmp2)-1, "[%Y][%m][%d][%H][%M][%S]", &tm_time);
#ifdef LOGNUM	
		strcpy(tmp1, "SYS:002");
		strcat(tmp1, tmp2);
		//printf("%s\n", tmp1);
		syslog(ALOG_SYSACT|LOG_NOTICE, tmp1);
#else
		tmp1[0] = '\0';	// to avoid compiler warning. David Hsieh <david_hsieh@alphanetworks.com>
		syslog(ALOG_SYSACT|LOG_NOTICE,"Time synchronized");
#endif
	}

	if (debug) {
	printf("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d\n",
		li, vn, mode, stratum, poll, prec);
	printf("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u\n",
		sec2u(delay),sec2u(disp),
		refid>>24&0xff, refid>>16&0xff, refid>>8&0xff, refid&0xff);
	printf("Reference %u.%.10u\n", reftime.coarse, reftime.fine);
	printf("Originate %u.%.10u\n", orgtime.coarse, orgtime.fine);
	printf("Receive   %u.%.10u\n", rectime.coarse, rectime.fine);
	printf("Transmit  %u.%.10u\n", xmttime.coarse, xmttime.fine);
	printf("Our recv  %u.%.10u\n", arrival->coarse, arrival->fine);
	}
	etime=ntpdiff(&orgtime,arrival);
	stime=ntpdiff(&rectime,&xmttime);
	skew1=ntpdiff(&orgtime,&rectime);
	skew2=ntpdiff(&xmttime,arrival);
	freq=get_current_freq();
	if (debug) {
	printf("Total elapsed: %9.2f\n"
	       "Server stall:  %9.2f\n"
	       "Slop:          %9.2f\n",
		etime, stime, etime-stime);
	printf("Skew:          %9.2f\n"
	       "Frequency:     %9d\n"
	       " day   second     elapsed    stall     skew  dispersion  freq\n",
		(skew1-skew2)/2, freq);
	printf("%d %5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d\n",
		arrival->coarse/86400+15020, arrival->coarse%86400,
		arrival->fine/4294967, etime, stime,
		(skew1-skew2)/2, sec2u(disp), freq);
	}
	fflush(stdout);
	if (live) {
		int new_freq;
		new_freq = contemplate_data(arrival->coarse, (skew1-skew2)/2,
			etime+sec2u(disp), freq);
		if (!debug && new_freq != freq) set_freq(new_freq);
	}
	result = 0;
}

static void stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *ntpserver;
	ntpserver=gethostbyname(hostname);
	if (ntpserver == NULL) {
		herror(hostname);
		exit(1);
	}
	if (ntpserver->h_length != 4) {
		fprintf(stderr,"oops %d\n",ntpserver->h_length);
		exit(1);
	}
	memcpy(&(p->s_addr),ntpserver->h_addr_list[0],4);
}

static void setup_receive(int usd, unsigned int interface, short port)
{
	struct sockaddr_in sa_rcvr;
	/* replace bzero with memset() to avoid compiler warning
	 * David Hsieh <david_hsieh@alphanetworks.com>
	bzero((char *) &sa_rcvr, sizeof(sa_rcvr)); */
	memset(&sa_rcvr, 0, sizeof(sa_rcvr));
	sa_rcvr.sin_family=AF_INET;
	sa_rcvr.sin_addr.s_addr=htonl(interface);
	sa_rcvr.sin_port=htons(port);
	if(bind(usd,(struct sockaddr *) &sa_rcvr,sizeof(sa_rcvr)) == -1) {
		fprintf(stderr,"could not bind to udp port %d\n",port);
		perror("bind");
		exit(1);
	}
	listen(usd,3);
}

static void setup_transmit(int usd, char *host, short port)
{
	struct sockaddr_in sa_dest;
	/* replace bzero with memset() to avoid compiler warning
	 * David Hsieh <david_hsieh@alphanetworks.com>
	bzero((char *) &sa_dest, sizeof(sa_dest)); */
	memset(&sa_dest, 0, sizeof(sa_dest));
	sa_dest.sin_family=AF_INET;
	stuff_net_addr(&(sa_dest.sin_addr),host);
	sa_dest.sin_port=htons(port);
	if (connect(usd,(struct sockaddr *)&sa_dest,sizeof(sa_dest))==-1)
		{perror("connect");exit(1);}
}

//support IPv6 method
static int setup_connect(short port, char *host, short local_port)
{
	struct addrinfo hints, *res;
	int error, usd;
	char lport[6];

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	memset(lport, 0, sizeof(lport));
	sprintf(lport, "%d", local_port);
	if ((error=getaddrinfo(NULL, lport, &hints, &res)) != 0)	//mean INADDR_ANY(IPv4 address) or IN6ADDR_ANY_INIT(IPv6 address).
	{
		printf("getaddrinfo failed: %s", gai_strerror(error));
		perror("getaddrinfo");
		exit(1);
	}
	
	if ((usd=socket(res->ai_family, res->ai_socktype, 0))==-1)
		{perror ("socket");exit(1);}
		
	if (bind (usd, res->ai_addr, res->ai_addrlen)!=0) {
		perror("bind");
		exit(1);
	}
	freeaddrinfo(res);
	listen(usd, 3);
	
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	memset(lport, 0, sizeof(lport));
	sprintf(lport, "%d", port);
	
	if ((error=getaddrinfo(host, lport, &hints, &res)) != 0)
	{
		printf("getaddrinfo failed: %s", gai_strerror(error));
		perror("getaddrinfo 2");
		exit(1);
	}
	if (connect(usd, res->ai_addr, res->ai_addrlen) !=0)
	{
		perror ("connect");
		exit(1);
	}
	freeaddrinfo(res);
	return usd;
}

static void primary_loop(int usd, int num_probes, int interval)
{
	fd_set fds;
	struct sockaddr sa_xmit;
	int i, pack_len, sa_xmit_len, probes_sent;
	struct timeval to;

	if (debug) printf("Listening...\n");

	probes_sent=0;
	sa_xmit_len=sizeof(sa_xmit);
	to.tv_sec=0;
	to.tv_usec=0;
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(usd,&fds);
		i=select(usd+1,&fds,NULL,NULL,&to);  /* Wait on read or error */
		if ((i!=1)||(!FD_ISSET(usd,&fds))) {
			if (i==EINTR) continue;
			if (i<0) perror("select");
			if (to.tv_sec == 0) {
				if (probes_sent >= num_probes &&
					num_probes != 0) break;
				send_packet(usd);
				++probes_sent;
				to.tv_sec=interval;
				to.tv_usec=0;
			}	
			continue;
		}
		pack_len=recvfrom(usd,incoming,sizeof(incoming),0,
		                  &sa_xmit,(socklen_t *)&sa_xmit_len);
		if (pack_len<0) {
			perror("recvfrom");
		} else if (pack_len>0 && pack_len<sizeof(incoming)){
			udp_handle(usd,incoming,pack_len,&sa_xmit,sa_xmit_len);
		} else {
			printf("Ooops.  pack_len=%d\n",pack_len);
			fflush(stdout);
		}
		if (probes_sent >= num_probes && num_probes != 0)	break;
	}
}

static void do_replay(void)
{
	char line[100];
	int n, day, freq, absolute;
	float sec, etime, stime, disp;
	double skew, errorbar;
	int simulated_freq = 0;
	unsigned int last_fake_time = 0;
	double fake_delta_time = 0.0;

	while (fgets(line,sizeof(line),stdin)) {
		n=sscanf(line,"%d %f %f %f %lf %f %d",
			&day, &sec, &etime, &stime, &skew, &disp, &freq);
		if (n==7) {
			fputs(line,stdout);
			absolute=(day-15020)*86400+(int)sec;
			errorbar=etime+disp;
			if (debug) printf("contemplate %u %.1f %.1f %d\n",
				absolute,skew,errorbar,freq);
			if (last_fake_time==0) simulated_freq=freq;
			fake_delta_time += (absolute-last_fake_time)*((double)(freq-simulated_freq))/65536;
			if (debug) printf("fake %f %d \n", fake_delta_time, simulated_freq);
			skew += fake_delta_time;
			freq = simulated_freq;
			last_fake_time=absolute;
			simulated_freq = contemplate_data(absolute, skew, errorbar, freq);
		} else {
			fprintf(stderr,"Replay input error\n");
			exit(2);
		}
	}
}

static void ntpclient_usage(char *argv0)
{
	fprintf(stderr,
	"Usage: %s [-c count] [-d] -h hostname [-i interval] [-l]\n"
	"\t[-p port] [-r] [-s] [-t time-difference] [-4] [-6]\n",
	argv0);
}

#ifdef NSBBOX
int ntpclient_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	int usd;  /* socket */
	int c;
	/* These parameters are settable from the command line
	   the initializations here provide default behavior */
	short int udp_local_port=0;   /* default of 0 means kernel chooses */
	int cycle_time=600;           /* seconds */
	int probe_count=0;            /* default of 0 means loop forever */
	/* int debug=0; is a global above */
	char *hostname=NULL;          /* must be set */
	int replay=0;                 /* replay mode overrides everything */
	int supportV4=0;
	int supportV6=0;

	
	for (;;) {
		c = getopt( argc, argv, "c:" DEBUG_OPTION "h:i:lp:rst:46");
		if (c == EOF) break;
		switch (c) {
			case 'c':
				probe_count = atoi(optarg);
				break;
#ifdef ENABLE_DEBUG
			case 'd':
				++debug;
				break;
#endif
			case 'h':
				hostname = optarg;
				break;
			case 'i':
				cycle_time = atoi(optarg);
				break;
			case 'l':
				live++;
				break;
			case 'p':
				udp_local_port = atoi(optarg);
				break;
			case 'r':
				replay++;
				break;
			case 's':
				set_clock++;
				probe_count = 1;
				break;
			case 't':
				timediff = atoi(optarg);
			        break;
			case '4':
				supportV4++;
				break;
			case '6':
				supportV6++;
				break;
			default:
				ntpclient_usage(argv[0]);
				exit(1);
		}
	}
	if (replay) {
		do_replay();
		exit(0);
	}
	if (hostname == NULL) {
		ntpclient_usage(argv[0]);
		exit(1);
	}
	if (debug) {
		printf("Configuration:\n"
		"  -c probe_count %d\n"
		"  -d (debug)     %d\n"
		"  -h hostname    %s\n"
		"  -i interval    %d\n"
		"  -l live        %d\n"
		"  -p local_port  %d\n"
		"  -s set_clock   %d\n"
		"  -t timediff    %d\n"
		"  -4 use_ipv4(default)\n"
		"  -6 use_ipv6		\n",
		probe_count, debug, hostname, cycle_time,
		live, udp_local_port, set_clock, timediff);
	}
	//default support IPv4
	if(supportV6 > 0)
		usd = setup_connect(NTP_PORT, hostname, udp_local_port);
	else
	{
		/* Startup sequence */
		if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
			{perror ("socket");exit(1);}
	
		setup_receive(usd, INADDR_ANY, udp_local_port);
		setup_transmit(usd, hostname, NTP_PORT);
	}
	primary_loop(usd, probe_count, cycle_time);

	close(usd);
	return result;
}
