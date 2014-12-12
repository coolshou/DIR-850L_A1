/* vi: set sw=4 ts=4: */
/* dhcpc.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "packet.h"
#include "script.h"
#include "socket.h"
#include "debug.h"
#include "pidfile.h"
#include "arpping.h"

#include <elbox_config.h>
#include <syslog.h>
#include <asyslog.h>
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
/* 32 bit change to 64 bit dennis 20080311 end */
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
#include <netdb.h>
#endif
static int state;

/* 32 bit change to 64 bit dennis 20080311 start */
static uint32_t requested_ip = 0;
static uint32_t server_addr;
static uint32_t timeout;
/* 32 bit change to 64 bit dennis 20080311 end */


static int packet_num = 0;
static int fd;
static int signal_pipe[2];

//hendry, this function also needed in other files
uint32_t get_uptime(void)
{
	struct sysinfo info;
	sysinfo(&info);
	return (uint32_t)info.uptime;
}
			
#ifdef DHCPPLUS
#define DHCPPLUS_PID_FILE	"/var/run/dhcpplus.pid"
#define DHCPPLUS_STATUS_FILE	"/var/run/dhcpplus.status"

static int dhcpplus=0;
static int dhcpplus_status=0;
static int dhcpplus_check_status(void)
{
	FILE *fp=NULL;
	char req_ip[16], srv_ip[16];
	
	/* get status/yip/serverip from DHCPPLUS_STATUS_FILE */
	if((fp=fopen(DHCPPLUS_STATUS_FILE, "r"))==NULL){
		return -1;
	}
	fscanf(fp, "%d %s %s", &dhcpplus_status, req_ip, srv_ip); 
	fclose(fp);

	return dhcpplus_status;
}

static int dhcpplus_update_address(void)
{
	FILE *fp=NULL;
	struct in_addr temp_addr;
	char *ipstr1, *ipstr2;
	
	/* get status/yip/serverip from DHCPPLUS_STATUS_FILE */
	if((fp=fopen(DHCPPLUS_STATUS_FILE, "w+"))==NULL){
		return -1;
	}

	temp_addr.s_addr = requested_ip;
	ipstr1 = strdup(inet_ntoa(temp_addr));
	temp_addr.s_addr = server_addr;
	ipstr2 = strdup(inet_ntoa(temp_addr));
	
	fprintf(fp, "%d %s %s\n", dhcpplus_status, ipstr1, ipstr2); 
	fclose(fp);

	free(ipstr1);
	free(ipstr2);

	system("killall -SIGUSR1 dhcpplus");
	return 0;
}

#endif

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;
int use_unicast = 0;	//add for use_unicast

extern int optind;
extern char * optarg;

#define DEFAULT_SCRIPT	"/usr/share/udhcpc/default.script"

struct client_config_t client_config =
{
	/* Default options. */
	abort_if_no_lease: 0,
	foreground: 0,
	quit_after_lease: 0,
	background_if_no_lease: 0,
	interface: "eth0",
	pidfile: NULL,
	script: DEFAULT_SCRIPT,
	clientid: NULL,
	hostname: NULL,
	ifindex: 0,
	arp: "\0\0\0\0\0\0",		/* appease gcc-3.0 */
};

/* prototype */
#ifdef COMBINED_BINARY
int udhcpc_main(int argc, char *argv[]);
#endif

#ifndef BB_VER
static void show_usage(void)
{
	printf(
			"Usage: udhcpc [OPTIONS]\n\n"
			"  -c, --clientid=CLIENTID         Client identifier\n"
			"  -H, --hostname=HOSTNAME         Client hostname\n"
			"  -h                              Alias for -H\n"
			"  -f, --foreground                Do not fork after getting lease\n"
			"  -b, --background                Fork to background if lease cannot be\n"
			"                                  immediately negotiated.\n"
			"  -i, --interface=INTERFACE       Interface to use (default: eth0)\n"
			"  -n, --now                       Exit with failure if lease cannot be\n"
			"                                  immediately negotiated.\n"
			"  -p, --pidfile=file              Store process ID of daemon in file\n"
			"  -q, --quit                      Quit after obtaining lease\n"
			"  -r, --request=IP                IP address to request (default: none)\n"
			"  -s, --script=file               Run file at dhcp events (default:\n"
			"                                  " DEFAULT_SCRIPT ")\n"
			"  -v, --version                   Display version\n"
		  );
	exit(0);
}
#endif


/* just a little helper */
static void change_mode(int new_mode)
{
	DEBUG(LOG_INFO, "%s: entering %s listen mode",__FUNCTION__, new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
	if(fd>=0) close(fd);
	fd = -1;
	listen_mode = new_mode;
}

/* perform a renew */
static void perform_renew(void)
{
	LOG(LOG_INFO, "Performing a DHCP renew");
#ifndef LOGNUM
	syslog(ALOG_NOTICE|LOG_NOTICE,"DHCP: Client performing a DHCP renew.");
#else
	syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:016");
#endif
	switch (state)
	{
	case BOUND:
		change_mode(LISTEN_KERNEL);
	case RENEWING:
	case REBINDING:
		state = RENEW_REQUESTED;
		break;
	case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
		run_script(NULL, "deconfig");
	case REQUESTING:
	case RELEASED:
		change_mode(LISTEN_RAW);
		state = INIT_SELECTING;
		break;
	case INIT_SELECTING:
		break;
	}

	/* start things over */
	packet_num = 0;

	/* Kill any timeouts because the user wants this to hurry along */
	timeout = 0;

}


/* perform a release */
static void perform_release(void)
{
	char buffer[16];
	struct in_addr temp_addr;

	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING)
	{
		temp_addr.s_addr = server_addr;
		sprintf(buffer, "%s", inet_ntoa(temp_addr));
		temp_addr.s_addr = requested_ip;
		LOG(LOG_INFO, "Unicasting a release of %s to %s",
				inet_ntoa(temp_addr), buffer);
#ifndef LOGNUM
		syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Client release IP %s to server %s.", inet_ntoa(temp_addr), buffer);
#else
		syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:017[%s][%s]", inet_ntoa(temp_addr), buffer);
#endif
		send_release(server_addr, requested_ip); /* unicast */
		run_script(NULL, "deconfig");
	}
	LOG(LOG_INFO, "Entering released state");

	change_mode(LISTEN_NONE);
	state = RELEASED;
	timeout = 0x7fffffff;
}


/* Exit and cleanup */
static void exit_client(int retval)
{
	if (fd > 0) close(fd);
	fd = -1;
	pidfile_delete(client_config.pidfile);
	CLOSE_LOG();
	exit(retval);
}


/* Signal handler */
static void signal_handler(int sig)
{
	DEBUG(LOG_INFO, "Got signal (%d) ...", sig);
	if (write(signal_pipe[1], &sig, sizeof(sig)) < 0)
	{
		LOG(LOG_ERR, "Could not send signal: %s", strerror(errno));
	}
}

static void background(void)
{
	int pid_fd;

	pid_fd = pidfile_acquire(client_config.pidfile); /* hold lock during fork. */
	while (pid_fd >= 0 && pid_fd < 3) pid_fd = dup(pid_fd); /* don't let daemon close it */
#if 0
	if (daemon(0, 0) == -1)
	{
		perror("fork");
		exit_client(1);
	}
#endif
	client_config.foreground = 1; /* Do not fork again. */
	pidfile_write_release(pid_fd);
}

#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
static void dump(const void * p, int len)
{
	int i;
	const unsigned char* ptr = p;
	for (i=0; i<len; i++)
	{
		if ((i && (i%16)==0)) fprintf(stderr, "\n");
		printf("%02x ", ptr[i]);
	}
	printf("\n");
}
static int get_type_len_value_by_type(unsigned char* opt_ptr,int opt_len,int type, unsigned char* buff)
{
	int remain = opt_len;
	int i=0;
	dump(opt_ptr, opt_len);
	while(remain>0)
	{
		int tmp_len = opt_ptr[i+1];
		printf("type %d,opt_ptr[%d] %d\n", type, i, opt_ptr[i]);
		if (type == opt_ptr[i])
		{/*tlv type match*/
			memcpy(buff, opt_ptr+i+2, tmp_len);
			buff[tmp_len+1]='\0';
			return 0;
		}
		else
		{   /*point to next tlv*/
			printf("tmp_len=%d,remain=%d\n",tmp_len, remain);
			remain -= 2+tmp_len;
			i += 2+tmp_len;
			printf("after tmp_len=%d,remain=%d, i=%d, opt_ptr[i]=%d\n",tmp_len, remain, i, opt_ptr[i]);
		}
	}
	return -1;
}
int got_ACs_IP;
char str_ACs_IP[256];
static int get_ACs_IP_addr(struct dhcpMessage* pPkt)
{
	unsigned char* temp;
	struct in_addr addr;
	int got = 0;
	unsigned char buff[256];//256 is enough,because len is only one byte.
	unsigned short code = ENTERPRISE_CODE;
	memset(buff, 0, sizeof(buff));
	if((temp = get_option(pPkt, DHCP_VENDOR)))//option 60
	{
		//get AC's IP address
		//temp[0],temp[1] is enterprise code.
		unsigned short *u16=(unsigned short*)temp;
		code=ntohs(*u16);
		if (ntohs(*u16) == ENTERPRISE_CODE && 
			0 == get_type_len_value_by_type(temp+2, *(temp-1)-2, 100, buff))
		{
			got = 1;
			printf("get AC's IP addr at opt 60: %s\n",buff);
		}else{
			printf("can not get from 60\n");
		}
	}
	if (!got && (temp = get_option(pPkt, DHCP_VENDOR_43)))//option 43
	{
		printf("get dhcp_vendor 43\n");
		if (0 == get_type_len_value_by_type(temp,*(temp-1),2,buff))
		{
			unsigned short *u16=(unsigned short*)buff;
			code=ntohs(*u16);
		}
		if (code == ENTERPRISE_CODE &&
			0 == get_type_len_value_by_type(temp,*(temp-1),128,buff))
		{
			got = 1;
			printf("get AC's IP addr at opt 43: %d.%d.%d.%d\n", buff[0],buff[1],buff[2],buff[3]);
			addr.s_addr = *((uint32_t *)buff);
			strcpy(buff,inet_ntoa(addr));
		}
	}
	if (got)
	{
		got_ACs_IP = 1;
		//got AC's IP, trans to dhcpc script.
		setenv("ac_ip", buff, 1);
		strcpy(str_ACs_IP, buff);
		printf("str_ac_ip:%s",str_ACs_IP);
	}
	else if(!got_ACs_IP)//dhcp ack will parse by this funcation too,so do not print even get ac ip
	{
		//does not get , use DNS.see templates/dhcp/udhcpc.php
		unsetenv("ac_ip");
		printf("can not get AC's IP by opt 60 and 43\n");
		printf("use DNS instead\n");
	}
	return 0;
}

#endif
#ifdef COMBINED_BINARY
int udhcpc_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	unsigned char *temp, *message;
	/* 32 bit change to 64 bit dennis 20080311 start */
	uint32_t t1 = 0, t2 = 0, xid = 0;
	uint32_t start = 0, lease;
	/* 32 bit change to 64 bit dennis 20080311 end */

	fd_set rfds;
	int retval;
	int err=0;
	struct timeval tv;
	int c, len;
	struct dhcpMessage packet;
	struct in_addr temp_addr;
	int pid_fd;
	time_t now;
	int max_fd;
	int sig;
	int isQuerySrv = 0;	// for query server
	int isDetectSrv = 0; 	/* for detest DHCPS by sandy*/
	char * ipstr1, *ipstr2;

	static struct option arg_options[] =
	{
		{"clientid",	required_argument,	0, 'c'},
		{"foreground",	no_argument,		0, 'f'},
		{"background",	no_argument,		0, 'b'},
		{"hostname",	required_argument,	0, 'H'},
		{"hostname",    required_argument,	0, 'h'},
		{"interface",	required_argument,	0, 'i'},
		{"now", 		no_argument,		0, 'n'},
		{"pidfile",		required_argument,	0, 'p'},
		{"quit",		no_argument,		0, 'q'},
		{"request",		required_argument,	0, 'r'},
		{"script",		required_argument,	0, 's'},
		{"version",		no_argument,		0, 'v'},
		{"help",		no_argument,		0, '?'},
		{"queryserver",	no_argument,		0, 'd'},	/* for query server */
		{"detectserver",	no_argument,		0, 't'},	/* for detest DHCPS add by sandy */
		{"delay",		required_argument,	0, 'D'},	/* discover delay time. */
		{"retry",		required_argument,	0, 'R'},	/* discover retry count. */
		{"sleep",		required_argument,	0, 'S'},	/* discover sleep time (when fails). */
		{0, 0, 0, 0}
	};

	client_config.discover_delay = 4;
	client_config.discover_retry = 5;
	client_config.discover_sleep = 60;

	/* get options */
	while (1)
	{
		int option_index = 0;
		// for query server
		//c = getopt_long(argc, argv, "c:fbH:h:i:np:qr:s:v", arg_options, &option_index);
#ifdef DHCPPLUS
		c = getopt_long(argc, argv, "c:fbH:h:i:np:qr:s:vdD:R:S:um", arg_options, &option_index);
#else	
		c = getopt_long(argc, argv, "c:fbH:h:i:np:qr:s:vdD:R:S:ut", arg_options, &option_index);
#endif

		if (c == -1) break;

		switch (c)
		{
			case 'c':
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.clientid) free(client_config.clientid);
				client_config.clientid = xmalloc(len + 2);
				client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
				client_config.clientid[OPT_LEN] = len;
				client_config.clientid[OPT_DATA] = '\0';
				strncpy((char *)client_config.clientid + OPT_DATA, optarg, len);
				break;
			case 'f':
				client_config.foreground = 1;
				break;
			case 'b':
				client_config.background_if_no_lease = 1;
				break;
			case 'h':
			case 'H':
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.hostname) free(client_config.hostname);
				client_config.hostname = xmalloc(len + 2);
				client_config.hostname[OPT_CODE] = DHCP_HOST_NAME;
				client_config.hostname[OPT_LEN] = len;
				strncpy((char *)client_config.hostname + 2, optarg, len);
				break;
			case 'i':
				client_config.interface =  optarg;
				break;
			case 'n':
				client_config.abort_if_no_lease = 1;
				break;
			case 'p':
				client_config.pidfile = optarg;
				break;
			case 'q':
				client_config.quit_after_lease = 1;
				break;
			case 'r':
				requested_ip = inet_addr(optarg);
				break;
			case 's':
				client_config.script = optarg;
				break;
			case 'v':
				printf("udhcpcd, version %s\n\n", VERSION);
				exit_client(0);
				break;
				// for query server
			case 'd':
				isQuerySrv++;
				client_config.discover_delay = 2;
				client_config.discover_retry = 2;
				client_config.discover_sleep = 0;
				break;
			case 'D':
				client_config.discover_delay = atoi(optarg);
				break;
			case 'R':
				client_config.discover_retry = atoi(optarg);
				break;
			case 'S':
				client_config.discover_sleep = atoi(optarg);
				break;
			case 'u':					//add for use unicast
				use_unicast = 1;
				break;
			case 't': // for detect DHCPS add by sandy  
				isDetectSrv++;
				client_config.discover_delay = 2;
				client_config.discover_retry = 2;
				client_config.discover_sleep = 0;
				break;		
#ifdef DHCPPLUS			
			case 'm':  //marked for dhcpplus add by tsrites
				dhcpplus=1;
				break;
#endif			
			default:
				show_usage();
		}
	}

	DEBUG(LOG_INFO, "delay = %d", client_config.discover_delay);
	DEBUG(LOG_INFO, "retry = %d", client_config.discover_retry);
	DEBUG(LOG_INFO, "sleep = %d", client_config.discover_sleep);

	OPEN_LOG("udhcpc");
	LOG(LOG_INFO, "udhcp client (v%s) started", VERSION);

	/* Create pid file */
	pid_fd = pidfile_acquire(client_config.pidfile);
	pidfile_write_release(pid_fd);

	/* Get interface information */
	if (read_interface(client_config.interface, &client_config.ifindex,
				NULL, client_config.arp) < 0)
	{
		exit_client(1);
	}

	/* Fill up client id with hardware address */
	if (!client_config.clientid)
	{
		client_config.clientid = xmalloc(6 + 3);
		client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
		client_config.clientid[OPT_LEN] = 7;
		client_config.clientid[OPT_DATA] = 1;
		memcpy(client_config.clientid + 3, client_config.arp, 6);
	}

	/* ensure that stdin/stdout/stderr are never returned by pipe() */
	if (fcntl(STDIN_FILENO, F_GETFL) == -1)		(void)open("/dev/null", O_RDONLY);
	if (fcntl(STDOUT_FILENO, F_GETFL) == -1)	(void)open("/dev/null", O_WRONLY);
	if (fcntl(STDERR_FILENO, F_GETFL) == -1)	(void)open("/dev/null", O_WRONLY);

	/* setup signal handlers */
	pipe(signal_pipe);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	signal(SIGTERM, signal_handler);

	/* init all states. run deconfig script first.  */
	state = INIT_SELECTING;
	//if (!isQuerySrv) run_script(NULL, "deconfig");
	change_mode(LISTEN_RAW);
	packet_num = 0;
	timeout = get_uptime();

	for (;;)
	{
		now = get_uptime();
		DEBUG(LOG_INFO, "%s:timeout = %d, now = %d", __FUNCTION__, timeout, now);

		tv.tv_sec = timeout - now;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);

		if (listen_mode != LISTEN_NONE && fd < 0)
		{
			if (listen_mode == LISTEN_KERNEL)
				fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config.interface);
			else
				fd = raw_socket(client_config.ifindex);

			if (fd < 0)
			{
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %s", strerror(errno));
				exit_client(0);
			}
		}
		if (fd >= 0)
		{
			FD_SET(fd, &rfds);
			DEBUG(LOG_INFO, "%s: set fd = %d", __FUNCTION__, fd);
		}
		FD_SET(signal_pipe[0], &rfds);
		DEBUG(LOG_INFO, "%s: set pipe fd = %d", __FUNCTION__, signal_pipe[0]);

		if (tv.tv_sec > 0)
		{
			max_fd = signal_pipe[0] > fd ? signal_pipe[0] : fd;
			DEBUG(LOG_INFO, "%s: Waiting on select max_fd=%d, timeout in %d secs !!",__FUNCTION__, max_fd, tv.tv_sec);
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
			err = errno;
		}
		else
			retval = 0; /* If we already timed out, fall through */

		if(retval==-1)
		{
			DEBUG(LOG_INFO,"ERR = %s\n",err==EBADF?"EBADF":err==EINTR?"EINTR":err==EINVAL?"EINVAL":"OTHER");
			;
		}
		DEBUG(LOG_INFO, "Return from select ... (ret = %d)", retval);

		now = get_uptime();
		if (retval == 0)
		{
			/* timeout dropped to zero */
			switch (state)
			{
			case INIT_SELECTING:
				DEBUG(LOG_INFO, "INIT_SELECTING ....");
				/* if (packet_num < 3) */
				if (packet_num < client_config.discover_retry)
				{
					if (packet_num == 0) xid = random_xid();

					/* send discover packet */
					send_discover(xid, requested_ip); /* broadcast */
#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
					syslog(ALOG_DEBUG|LOG_NOTICE, "DHCP: Client send DISCOVER.");
#else
					syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Client send DISCOVER.");
#endif
#else
					syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:010");
#endif

					/* timeout = now + ((packet_num == 2) ? 4 : 2); */
					timeout = ((unsigned int)client_config.discover_delay) << packet_num;
					if (timeout > 64) timeout = 64;
					DEBUG(LOG_INFO, "packet_num=%d, timeout = %u", packet_num, timeout);
					timeout += now;
					packet_num++;
				}
				else
				{
					/* for query server */
					if (isQuerySrv)
					{
						printf("no\n");
						exit_client(1);
					}
					if (client_config.background_if_no_lease)
					{
						LOG(LOG_INFO, "No lease, forking to background.");
						background();
					}
					else if (client_config.abort_if_no_lease)
					{
						LOG(LOG_INFO, "No lease, failing.");
						exit_client(1);
					}
					/* wait to try again */
					packet_num = 0;
					/* timeout = now + 60; */
					if (client_config.discover_sleep == 0) exit_client(1);
					timeout = now + client_config.discover_sleep;
				}
				break;

			case RENEW_REQUESTED:
			case REQUESTING:
				DEBUG(LOG_INFO, "REQUESTING....");
				/* if (packet_num < 3) */
				if (packet_num < client_config.discover_retry)
				{
					/* send request packet */
					if (state == RENEW_REQUESTED)
						send_renew(xid, server_addr, requested_ip); /* unicast */
					else
						send_selecting(xid, server_addr, requested_ip); /* broadcast */

					/* Add log ... */
					temp_addr.s_addr = requested_ip;
					ipstr1 = strdup(inet_ntoa(temp_addr));
					temp_addr.s_addr = server_addr;
					ipstr2 = strdup(inet_ntoa(temp_addr));

#if ELBOX_PROGS_GPL_SYSLOGD_AP							/*	syslog_2007_04_10,	System -> SYSACT, Jordan */
					syslog(ALOG_AP_SYSACT|LOG_NOTICE,"[SYSACT] DHCP Client get IP %s",ipstr1);//Dennis  syslog
#endif


#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
					syslog(ALOG_DEBUG|LOG_NOTICE,"DHCP: Client send REQUEST, Request IP %s from %s.", ipstr1, ipstr2);
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"DHCP: Client send REQUEST, Request IP %s from %s.", ipstr1, ipstr2);
#endif
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:012[%s][%s]", ipstr1, ipstr2);
#endif
					free(ipstr1);
					free(ipstr2);

					/* timeout = now + ((packet_num == 2) ? 10 : 2); */
					timeout = ((unsigned int)client_config.discover_delay) << packet_num;
					if (timeout > 64) timeout = 64;
					timeout += now;
					packet_num++;
				}
				else
				{
					/* timed out, go back to init state */
					if (state == RENEW_REQUESTED) run_script(NULL, "deconfig");
					if (client_config.discover_sleep == 0) exit_client(1);
					state = INIT_SELECTING;
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				}
				break;

			case BOUND:
				DEBUG(LOG_INFO, "BOUND ....");
				/* Lease is starting to run out, time to enter renewing state */
				state = RENEWING;
				change_mode(LISTEN_KERNEL);
				/* fall right through */
			case RENEWING:
				DEBUG(LOG_INFO, "RENEWING ....");
				/* Either set a new T1, or enter REBINDING state */
				if ((t2 - t1) <= (lease / 14400 + 1))
				{
					/* timed out, enter rebinding state */
					state = REBINDING;
					timeout = now + (t2 - t1);
					DEBUG(LOG_INFO, "Entering rebinding state");
				}
				else
				{
					/* send a request packet */
					send_renew(xid, server_addr, requested_ip); /* unicast */
					t1 = (t2 - t1) / 2 + t1;
					timeout = t1 + start;
				}
				break;

			case REBINDING:
				DEBUG(LOG_INFO, "REBINDING ....");
				/* Either set a new T2, or enter INIT state */
				if ((lease - t2) <= (lease / 14400 + 1))
				{
					/* timed out, enter init state */
					state = INIT_SELECTING;
					LOG(LOG_INFO, "Lease lost, entering init state");
					run_script(NULL, "deconfig");
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				}
				else
				{
					/* send a request packet */
					send_renew(xid, 0, requested_ip); /* broadcast */

					t2 = (lease - t2) / 2 + t2;
					timeout = t2 + start;
				}
				break;

			case RELEASED:
				DEBUG(LOG_INFO, "RELEASED ....");
				/* yah, I know, *you* say it would never happen */
				timeout = 0x7fffffff;
				break;
			}
		}
		else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(fd, &rfds))
		{
			/* a packet is ready, read it */
			if (listen_mode == LISTEN_KERNEL)
				len = get_packet(&packet, fd);
			else
				len = get_raw_packet(&packet, fd);

			if (len == -1 && errno != EINTR)
			{
				DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
				change_mode(listen_mode); /* just close and reopen */
			}
			if (len < 0) continue;

			if (packet.xid != xid)
			{
			/* 32 bit change to 64 bit dennis 20080311 start */
             DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)", (uint32_t) packet.xid,xid);
			/* 32 bit change to 64 bit dennis 20080311 end */
			continue;
			}

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
			{
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

#ifdef SIX_RD_OPTION
			//6rd DHCPv4 option, build dummy option for testing (tom, 20110322)
			//unsigned char six_rd_dummy_option[] = 
			//{
			//	212 , 22 , 
			//	16 , //ipv4 mask length
			//	32 , //6rd prefix length
			//	0x20 , 0x01 , 0xca , 0xfe , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , //6rd prefix
			//	1 , 2 , 3 , 4 // 6rd BR ipv4 address
			//};

			//add_option_string(packet.options , six_rd_dummy_option);
#endif //SIX_RD_OPTION

			switch (state)
			{
			case INIT_SELECTING:
				DEBUG(LOG_INFO, ">>> INIT_SELECTING ....");
				/* Must be a DHCPOFFER to one of our xid's */
				if (*message == DHCPOFFER)
				{
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS

					got_ACs_IP = 0;//clear the flag
#endif
					if ((temp = get_option(&packet, DHCP_SERVER_ID)))
					{
						/* for query server */
						if (isQuerySrv)
						{
							printf("yes\n");
                                                        /*sandy 2010_6_24   get response ip*/			
							if (isDetectSrv)
							{								
								int i =0;
								char ipbuf[16], maskbuf[16], buf1[16]="0.0.0.250" ;				
								static uint32_t response_ip = 0;
								struct	in_addr   tempmask ,tempbuf,buf2;
						
								
								memcpy(&server_addr, temp, 4);
								
								/*sandy 2010_6_24   get response mask*/
								for (i = 0; options[i].code; i++)
								{
									if (packet.options[i + OPT_CODE] == DHCP_SUBNET && packet.options[i + OPT_LEN] ==4)
									{
										/*mask address*/
										sprintf(maskbuf, "%d.%d.%d.%d", packet.options[i + OPT_DATA],packet.options[i+1 + OPT_DATA],packet.options[i+2 + OPT_DATA],packet.options[i+3 + OPT_DATA]);
										if (packet.options[i + OPT_DATA] !=0 && packet.options[i+1 + OPT_DATA] ==0 && packet.options[i+2 + OPT_DATA] ==0 && packet.options[i+3 + OPT_DATA]==0)
										{
											printf("%d\n",8);
										}
										else if (packet.options[i + OPT_DATA] !=0 && packet.options[i+1 + OPT_DATA] !=0 && packet.options[i+2 + OPT_DATA] ==0 && packet.options[i+3 + OPT_DATA]==0)
										{
											printf("%d\n",16);
										}
										else if (packet.options[i + OPT_DATA] !=0 && packet.options[i+1 + OPT_DATA] !=0 && packet.options[i+2 + OPT_DATA] !=0 && packet.options[i+3 + OPT_DATA]==0)
										{
											printf("%d\n",24);
										}
										else if (packet.options[i + OPT_DATA] !=0 && packet.options[i+1 + OPT_DATA] !=0 && packet.options[i+2 + OPT_DATA] !=0 && packet.options[i+3 + OPT_DATA]!=0)
										{
											printf("%d\n",32);
										}
									//	printf("%s\n",maskbuf);
										 memset(&tempmask, 0x0, sizeof (tempmask));
								 		inet_aton(maskbuf,&tempmask);	
										/*0.0.0.250*/
										 memset(&buf2, 0x0, sizeof (buf2));
								 		inet_aton(buf1,&buf2);	
							
										/*get response IP address*/
										xid = packet.xid;					
										response_ip = packet.yiaddr;
										temp_addr.s_addr = response_ip;
										
										/*do And mask */
										tempbuf.s_addr = temp_addr.s_addr & tempmask.s_addr ;
										/*do Or  0.0.0.250*/
										tempbuf.s_addr =  tempbuf.s_addr |buf2.s_addr ;
										sprintf(ipbuf, "%s",inet_ntoa(tempbuf));								
										printf("%s\n",ipbuf);
									}	
								}
							}
							
							/*sandy 2010_6_24----------------*/
							exit_client(0);
						}
						memcpy(&server_addr, temp, 4);
						xid = packet.xid;
						requested_ip = packet.yiaddr;
						temp_addr.s_addr = server_addr;
						DEBUG(LOG_INFO, "DHCP: Client receive OFFER from %s.", inet_ntoa(temp_addr));
#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
						syslog(ALOG_DEBUG|LOG_NOTICE, "DHCP: Client receive OFFER from %s.", inet_ntoa(temp_addr));
#else
						syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Client receive OFFER from %s.", inet_ntoa(temp_addr));
#endif
#else
						syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:011[%s]", inet_ntoa(temp_addr));
#endif
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
						get_ACs_IP_addr(&packet);
#endif
						/* enter requesting state */
						state = REQUESTING;
						timeout = now;
						packet_num = 0;
					}
					else
					{
						DEBUG(LOG_ERR, "No server ID in message");
					}
				}
				break;

			case RENEW_REQUESTED:
			case REQUESTING:
			case RENEWING:
			case REBINDING:
				DEBUG(LOG_INFO, ">>> REQUESTING ....");
				if (*message == DHCPACK)
				{
					LOG(LOG_INFO, "Received DHCP ACK");
					temp_addr.s_addr = packet.yiaddr;
					if (state == REQUESTING)
					{
						//+++ Mark
						// After aquiring the ip from dhcp server, we send out an arp to check if the ip is already in use
						// rather than following unix convention and fill the source ip address as zero in the arp packet
						// we changed it to follow windows xp conveition and fill the source ip address with the newly aquired ip
						// on ethereal this will become a special arp packet that is distictively used for ip crash checking.
						//if (!arpping(packet.yiaddr, 0, client_config.arp, client_config.interface))
						if (!arpping(packet.yiaddr, packet.yiaddr, client_config.arp, client_config.interface))
						//--- Mark
						{
							LOG(LOG_INFO, "the address (%s) is used by another host", inet_ntoa(temp_addr));
#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
							syslog(ALOG_DEBUG|LOG_NOTICE,"DHCP: Client receive ACK, but the address (%s) is used by another host.", inet_ntoa(temp_addr));
#else
							syslog(ALOG_NOTICE|LOG_NOTICE,"DHCP: Client receive ACK, but the address (%s) is used by another host.", inet_ntoa(temp_addr));
#endif
#else
							syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:014[%s]", inet_ntoa(temp_addr));
#endif
                                                        /*Erick DHCP Decline 20110415*/
							send_decline(xid, server_addr, temp_addr.s_addr);
							state = INIT_SELECTING;
							timeout = now;
							requested_ip = 0;
							packet_num = 0;
							sleep(3); /* avoid excessive network traffic */
							break;
						}
					}

					if (!(temp = get_option(&packet, DHCP_LEASE_TIME)))
					{
						LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
						lease = 60 * 60;
					}
					else
					{
						memcpy(&lease, temp, 4);
						lease = ntohl(lease);
					}
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
					get_ACs_IP_addr(&packet);
#endif
					/* enter bound state */
					t1 = lease / 2;

					/* little fixed point for n * .875 */
					t2 = (lease * 0x7) >> 3;
					temp_addr.s_addr = packet.yiaddr;
					LOG(LOG_INFO, "Lease of %s obtained, lease time %ld", inet_ntoa(temp_addr), lease);

					/* Add logs ... */
					temp_addr.s_addr = server_addr;
					ipstr1 = strdup(inet_ntoa(temp_addr));
					temp_addr.s_addr = packet.yiaddr;
					ipstr2 = strdup(inet_ntoa(temp_addr));
#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
					syslog(ALOG_DEBUG|LOG_NOTICE,"DHCP: Client receive ACK from %s, IP=%s, Lease time=%d.",ipstr1, ipstr2, lease);
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"DHCP: Client receive ACK from %s, IP=%s, Lease time=%d.",ipstr1, ipstr2, lease);

#endif
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:013[%s][%s][%d]", ipstr1, ipstr2, lease);
#endif
					free(ipstr1);
					free(ipstr2);

					start = now;
					timeout = t1 + start;
					requested_ip = packet.yiaddr;
#ifndef DHCPPLUS
					run_script(&packet, ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
#ifdef CLASSLESS_STATIC_ROUTE_OPTION					
					/* We process dhcp option 121 right after renew or bound */
					{
						unsigned int index=0, remain=0;
						do {
							run_script_opt121(&packet, &index, &remain );
							index++;
						} while(remain);

					}
#endif	
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
					/*
					 * Support DHCP option 249 which has the same function as option 121,
					 * but it is documented in Microsoft MSDN not RFC.
					 */   
					{
						unsigned int index=0, remain=0;
						do {
							run_script_opt249(&packet, &index, &remain );
							index++;
						} while(remain);
					}
#endif
#ifdef STATIC_ROUTE_OPTION					
					/* We process dhcp option 33 right after renew or bound */
					{
						unsigned int index=0, remain=0;
						do {
							run_script_opt33(&packet, &index, &remain );
							index++;
						} while(remain);

					}
#endif	
#else
					if(dhcpplus==1)
					{	
							dhcpplus_check_status();
							if(dhcpplus_status != 1){
								run_script(&packet,"dhcpplus");
							}else{
								run_script(&packet, ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
							}
							dhcpplus_update_address();

					}else{
						run_script(&packet, ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
#ifdef CLASSLESS_STATIC_ROUTE_OPTION						   
						/* We process dhcp option 121 right after renew or bound */   
						{
						unsigned int index=0, remain=0;
							do {
								printf("Perform run_script_opt121!!\n");
								run_script_opt121(&packet, &index, &remain );
								index++;
							} while(remain);
						}
#endif 
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
						/* We process dhcp option 249 right after renew or bound */   
						{
						unsigned int index=0, remain=0;
							do {
								printf("Perform run_script_opt249!!\n");
								run_script_opt249(&packet, &index, &remain );
								index++;
							} while(remain);
						}
#endif
#ifdef STATIC_ROUTE_OPTION						   
						/* We process dhcp option 33 right after renew or bound */   
						{
						unsigned int index=0, remain=0;
							do {
								printf("Perform run_script_opt33!!\n");
								run_script_opt33(&packet, &index, &remain );
								index++;
							} while(remain);
						}
#endif	
					}
#endif // !DHCPPLUS	
					state = BOUND;
					change_mode(LISTEN_NONE);
					
					if (client_config.quit_after_lease) exit_client(0);
					if (!client_config.foreground) background();
				}
				else if (*message == DHCPNAK)
				{
					/* return to init state */
					LOG(LOG_INFO, "Received DHCP NAK");

					uint32_t server_id;
					/* RFC 2131 - DHCP NAK MUST have server ID option */
					if (!(temp = get_option(&packet, DHCP_SERVER_ID)))
					{
						DEBUG(LOG_ERR, "couldnt get server ID option from packet -- ignoring");
						break;
					}
					else
					{
						memcpy(&server_id, temp, 4);
					}
					/* check if DHCP NAK is for us */
					if (server_id != server_addr)
					{
						DEBUG(LOG_ERR, "DHCP NAK is not for us -- ignoring");
						break;
					}
#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
					syslog(ALOG_DEBUG|LOG_NOTICE,"DHCP: Client receive NAK.");
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"DHCP: Client receive NAK.");
#endif
#else
					syslog(ALOG_NOTICE|LOG_NOTICE,"NTC:015");
#endif
					run_script(&packet, "nak");
					if (state != REQUESTING) run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now;
					requested_ip = 0;
					packet_num = 0;
					change_mode(LISTEN_RAW);
					sleep(3); /* avoid excessive network traffic */
				}
				else
				{
					LOG(LOG_INFO, "Received ????  %d", *message);
				}
				break;
				/* case BOUND, RELEASED: - ignore all packets */
			}
		}
		else if (retval > 0 && FD_ISSET(signal_pipe[0], &rfds))
		{
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0)
			{
				DEBUG(LOG_ERR, "Could not read signal: %s", strerror(errno));
				continue; /* probably just EINTR */
			}
			DEBUG(LOG_INFO,"Get Signal -- [%s]\n",(sig==SIGUSR1)?"SIGUSR1":((sig==SIGUSR2)?"SIGUSR2":"SIGTERM"));
			switch (sig)
			{
			case SIGUSR1:	perform_renew();	break;
			case SIGUSR2:	perform_release();	break;
			case SIGTERM:	LOG(LOG_INFO, "Received SIGTERM");
							DEBUG(LOG_INFO, "Received SIGTERM - %d(%d,%d,%d)\n", sig,SIGUSR1,SIGUSR2,SIGTERM);
							perform_release();
							exit_client(0);
			}
		}
		else if (retval == -1 && errno == EINTR)
		{
#if 1 //Kloat add for killing udhcpc immediately. 2008.1.30
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0)
			{
				DEBUG(LOG_ERR, "Could not read signal: %s", strerror(errno));
				continue; /* probably just EINTR */
			}
			DEBUG(LOG_INFO,"Get Signal -- [%s]\n",(sig==SIGUSR1)?"SIGUSR1":((sig==SIGUSR2)?"SIGUSR2":"SIGTERM"));
			switch (sig)
			{
			case SIGUSR1:	perform_renew();	break;
			case SIGUSR2:	perform_release();	break;
			case SIGTERM:	LOG(LOG_INFO, "Received SIGTERM");
							DEBUG(LOG_INFO, "Received SIGTERM - %d(%d,%d,%d)\n", sig,SIGUSR1,SIGUSR2,SIGTERM);
							perform_release();
							exit_client(0);
			}
#endif  //Kloat end
			/* a signal was caught */
		}
		else
		{
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}
	}
	return 0;
}

