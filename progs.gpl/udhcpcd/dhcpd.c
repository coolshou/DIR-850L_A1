/* vi: set sw=4 ts=4: */
/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <linux/types.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"

#include <elbox_config.h>
#include <syslog.h>
#include <asyslog.h>

/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
/* 32 bit change to 64 bit dennis 20080311 end */

#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
extern int get_mi_status(void);
extern int wireless_netlink_init(void);
extern int wireless_netlink_receive(int sock, char *addr, int addr_len);
extern void wireless_netlink_deinit(int *sock);
extern int get_wireless_interface_index(const char *wlanif);
extern int isInBlacklist(const char *entry, const int len, const char *path);

int isMI = 0;
int binding_ifindex, incoming_ifindex;
int wnetlink_socket = -1;
char blpath[32] = {0};
#endif

/* globals */
struct dhcpOfferedAddr *leases;
/* +++ Joy added static leases */
struct dhcpOfferedAddr *static_leases;
/* --- Joy added static leases */
struct server_config_t server_config;
static int signal_pipe[2];

/* prototype */
int valid_ip(u_int32_t ipaddr);
#ifdef COMBINED_BINARY
int udhcpd_main(int argc, char *argv[]);
#endif

/* Exit and cleanup */
static void exit_server(int retval)
{
	pidfile_delete(server_config.pidfile);
	CLOSE_LOG();
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	if(isMI) wireless_netlink_deinit(&wnetlink_socket);
#endif
	exit(retval);
}

/* Signal handler */
static void signal_handler(int sig)
{
	if (send(signal_pipe[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0)
	{
		LOG(LOG_ERR, "Could not send signal: %s", strerror(errno));
	}
}

int valid_ip(u_int32_t ipaddr)
{
	u_int32_t addr;

	// 1.must in our pool
	addr = ntohl(ipaddr);
	if (addr < ntohl(server_config.start) || addr > ntohl(server_config.end))
	{
		DEBUG(LOG_INFO, "%s: out of range, addr=0x%08x, start=0x%08x, end=0x%08x",
			__FUNCTION__, addr, ntohl(server_config.start), ntohl(server_config.end));
		return 0;
	}

	// 2.is not server ip,broadcast,domain
	if((ipaddr & 0xFF)==0)
	{
		fprintf(stderr,"Error it's not a ip\n");
		return 0;
	}
	if((ipaddr & 0xFF) == 0xFF)
	{
		fprintf(stderr,"Error it's bcast \n");
		return 0;
	}
	if(ipaddr == server_config.server)
	{
		fprintf(stderr,"Error it's my ip \n");
		return 0;
	}

	// may other check
	return 1;
}

#ifdef COMBINED_BINARY
int udhcpd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	fd_set rfds;
	struct timeval tv;
	int server_socket = -1;
	int bytes, retval;
	struct dhcpMessage packet;
	unsigned char *state;
#if 0 /* Joy modified: hostname is moved to sendACK() */
	unsigned char *server_id, *requested, *hostname;
#else
	unsigned char *server_id, *requested;
#endif
	u_int32_t server_id_align, requested_align;
/* 32 bit change to 64 bit dennis 20080311 start */
	uint32_t timeout_end;
/* 32 bit change to 64 bit dennis 20080311 end */
	struct option_set *option;
	struct dhcpOfferedAddr *lease;
	struct dhcpOfferedAddr *static_lease;
	int pid_fd;
	int max_sock;
	int sig;
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP
	u_int32_t hold_ipaddr;
#endif	
	char mac[32];

	OPEN_LOG("udhcpd");
	LOG(LOG_INFO, "udhcp server (v%s) started", VERSION);

	/* +++ Joy added static leases */
	static_leases = malloc(sizeof(struct dhcpOfferedAddr) * MAX_STATIC_LEASES);
	memset(static_leases, 0, sizeof(struct dhcpOfferedAddr) * MAX_STATIC_LEASES);
	/* --- Joy added static leases */

	memset(&server_config, 0, sizeof(struct server_config_t));

	if (argc < 2) read_config(DHCPD_CONF_FILE);
	else read_config(argv[1]);

	pid_fd = pidfile_acquire(server_config.pidfile);
	pidfile_write_release(pid_fd);

	if ((option = find_option(server_config.options, DHCP_LEASE_TIME)))
	{
		memcpy(&server_config.lease, option->data + 2, 4);
		server_config.lease = ntohl(server_config.lease);
	}
	else
	{
		server_config.lease = LEASE_TIME;
	}
	// Sam Chen add for leasing the IP (ex. 192.168.1.255/16) to client
	if ((option = find_option(server_config.options, DHCP_SUBNET)))
	{
		memcpy(&server_config.mask, option->data + 2, 4);
		server_config.mask = ~ntohl(server_config.mask);
	}
	else
	{
		server_config.mask = 0xff;
	}
	// Sam Chen end

	leases = malloc(sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
	memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
	read_leases(server_config.lease_file);
	/* +++ Joy added */
	write_leases();
	/* --- Joy added */

	if (read_interface(server_config.interface, &server_config.ifindex,
				&server_config.server, server_config.arp) < 0)
	{
		exit_server(1);
	}

#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	isMI = get_mi_status();
	uprintf("isMI = %d\n", isMI);
	if(isMI)
	{
		if((binding_ifindex = get_wireless_interface_index(server_config.wlanif)) < 0)
		{
			exit_server(1);
		}
		uprintf("binding_ifindex = %d", binding_ifindex);

		sprintf(blpath, "/var/run/mudhcpd-%s.blacklist", server_config.wlanif);
		{
			FILE *blfile = NULL;

			if((blfile = fopen(blpath, "w+")) == NULL)
			{
				exit_server(1);
			}
			fclose(blfile);
		}
	}
#endif

#if 0
#ifndef DEBUGGING
	pid_fd = pidfile_acquire(server_config.pidfile); /* hold lock during fork. */
	if (daemon(0, 0) == -1)
	{
		perror("fork");
		exit_server(1);
	}
	pidfile_write_release(pid_fd);
#endif
#endif


	socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
	signal(SIGUSR1, signal_handler);
	signal(SIGTERM, signal_handler);

	//hendry
	//timeout_end = time(0) + server_config.auto_time;
	timeout_end = get_uptime() + server_config.auto_time;
	
	while(1)
	{	/* loop until universe collapses */
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
		if(isMI)
		{
			if(wnetlink_socket < 0)
			{
				if((wnetlink_socket = wireless_netlink_init()) < 0)
				{
					exit_server(1);
				}
			}
		}
#endif

		if (server_socket < 0)
		{
			if ((server_socket = listen_socket(INADDR_ANY, SERVER_PORT, server_config.interface)) < 0)
			{
				LOG(LOG_ERR, "FATAL: couldn't create server socket, %s", strerror(errno));
				exit_server(0);
			}
		}

		FD_ZERO(&rfds);
		FD_SET(server_socket, &rfds);
		FD_SET(signal_pipe[0], &rfds);
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
		if(isMI) FD_SET(wnetlink_socket, &rfds);
#endif
		if (server_config.auto_time)
		{
			//hendry
			//tv.tv_sec = timeout_end - time(0);
			tv.tv_sec = timeout_end - get_uptime();
			tv.tv_usec = 0;
		}

		if (!server_config.auto_time || tv.tv_sec > 0)
		{
			max_sock = server_socket > signal_pipe[0] ? server_socket : signal_pipe[0];
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
			if(isMI) max_sock = max_sock > wnetlink_socket ? max_sock : wnetlink_socket;
#endif
			retval = select(max_sock + 1, &rfds, NULL, NULL, server_config.auto_time ? &tv : NULL);
		}
		else
		{
			retval = 0; /* If we already timed out, fall through */
		}

		if (retval == 0)
		{
			write_leases();
			//timeout_end = time(0) + server_config.auto_time;
			//hendry
			timeout_end = get_uptime() + server_config.auto_time;
			continue;
		}
		else if (retval < 0 && errno != EINTR)
		{
			DEBUG(LOG_INFO, "error on select");
			continue;
		}

#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
		if(isMI)
		{
			if(FD_ISSET(wnetlink_socket, &rfds))
			{
				incoming_ifindex = wireless_netlink_receive(wnetlink_socket, &mac[0], sizeof(mac));
				//uprintf("--- incoming_ifindex = %d, binding_ifindex = %d ---", incoming_ifindex, binding_ifindex);
				if(incoming_ifindex != binding_ifindex)
				{
					continue;
				}
			}
		}
#endif

		if (FD_ISSET(signal_pipe[0], &rfds))
		{
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0) continue; /* probably just EINTR */
			switch (sig)
			{
			case SIGUSR1:
				LOG(LOG_INFO, "Received a SIGUSR1");
				write_leases();
				/* why not just reset the timeout, eh */
				timeout_end = time(0) + server_config.auto_time;
				continue;
			case SIGTERM:
				LOG(LOG_INFO, "Received a SIGTERM");
				exit_server(0);
			}
		}

		if ((bytes = get_packet(&packet, server_socket)) < 0)
		{	/* this waits for a packet - idle */
			if (bytes == -1 && errno != EINTR)
			{
				DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
				close(server_socket);
				server_socket = -1;
			}
			continue;
		}

#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
		if(isMI)
		{
			if(isInBlacklist(mac, strlen(mac), blpath))
			{
				//wireless_netlink_deinit(&wnetlink_socket);
				timeout_end = time(0) + server_config.auto_time;
				close(server_socket);
				server_socket = -1;
				continue;
			}
			uprintf("socket %d go to the next steps", wnetlink_socket);
		}
#endif

		if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
		{
			DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
			continue;
		}

		/* ADDME: look for a static lease */
		/* Kwest add static lease here */
		static_lease = find_static_lease_by_chaddr(packet.chaddr);

		if(static_lease)
			lease = static_lease;
		else
		lease = find_lease_by_chaddr(packet.chaddr);

		sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
				packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
				packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);

		switch (state[0])
		{
		case DHCPDISCOVER:
			DEBUG(LOG_INFO,"received DISCOVER and sent OFFER");

#ifndef LOGNUM
#ifdef ELBOX_PROGS_PRIV_LOGD_AP
			syslog(ALOG_DEBUG|LOG_NOTICE, "DHCP: Server receive DISCOVER from %s.", mac);
#else
			syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server receive DISCOVER from %s.", mac);
#endif
#else
			syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:018[%s]", mac);
#endif

			if (sendOffer(&packet) < 0)
			{
				LOG(LOG_ERR, "send OFFER failed");
			}
			break;

		case DHCPREQUEST:
			DEBUG(LOG_INFO, "%s: received REQUEST from %s", __FUNCTION__, mac);

			requested = get_option(&packet, DHCP_REQUESTED_IP);
			server_id = get_option(&packet, DHCP_SERVER_ID);

#ifndef LOGNUM
			syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server receive REQUEST from %s.", mac);
#else
			syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:022[%s]", mac);
#endif

			if (requested)
			{
				memcpy(&requested_align, requested, 4);
				DEBUG(LOG_INFO, "%s: DHCP_REQUESTED_IP = 0x%08x",__FUNCTION__, requested_align);
			}
			if (server_id)
			{
				memcpy(&server_id_align, server_id, 4);
				DEBUG(LOG_INFO, "%s: DHCP_SERVER_ID = 0x%08x",__FUNCTION__, server_id_align);
			}

			if (lease)	/*ADDME: or static lease */
			{
				DEBUG(LOG_INFO, "%s: found lease !",__FUNCTION__);
				/* RFC 2131, 4.3.2: */
				if (server_id)
				{
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP
                                        hold_ipaddr =ntohl(server_config.lan_ip);  
#endif
					/* SELECTING State */
					DEBUG(LOG_INFO, "%s: server_id = %08x",__FUNCTION__, ntohl(server_id_align));
					if (server_id_align == server_config.server &&
						requested && requested_align == lease->yiaddr){	
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP						
						if(hold_ipaddr==lease->yiaddr)
							sendNAK(&packet);
						else
#endif							
						sendACK(&packet, lease->yiaddr);
					}
				}
				else
				{
					if (requested)
					{
						/* INIT-REBOOT State */
						DEBUG(LOG_INFO, "%s: have requested!",__FUNCTION__);
						if (lease->yiaddr == requested_align)
							sendACK(&packet, lease->yiaddr);
						else
							sendNAK(&packet);
					}
					else
					{
						/* RENEWING or REBINDING State */
						if (lease->yiaddr == packet.ciaddr)
							sendACK(&packet, lease->yiaddr);
						else
							sendNAK(&packet); /* don't know what to do!!!! */
					}
				}
			}
			/* what to do if we have no record of the client */
			else if (server_id)
			{	/* SELECTING State */
				DEBUG(LOG_INFO, "%s: have server_id !",__FUNCTION__);
				/* For other server, keep silent. */
			}
			else if (requested)
			{
				DEBUG(LOG_INFO, "%s: have requested !",__FUNCTION__);
				/* INIT-REBOOT State */
				if ((lease = find_lease_by_yiaddr(requested_align)))
				{
					if (lease_expired(lease))	/* probably best if we drop this lease */
						memset(lease->chaddr, 0, 16);
					else /* make some contention for this address */
						sendNAK(&packet);
				}
				/* If request valid IP, send ACK.(happen when udhcpd restart) */
				else if (valid_ip(requested_align) && !check_ip(requested_align)) {
					sendACK(&packet, requested_align);
				}
				else /* else send NAK */
					sendNAK(&packet);
			}
			else {
				/* RENEWING or REBINDING State */
				if (packet.ciaddr && valid_ip(packet.ciaddr)) /* happen when udhcpd restart before T1 and T2 */
					sendACK(&packet, packet.ciaddr);
				else /* don't know what to do!!!! */
					sendNAK(&packet);
			}
			break;

		case DHCPDECLINE:
			DEBUG(LOG_INFO,"received DECLINE");
			if (lease && lease != static_lease)
			{
				memset(lease->chaddr, 0, 16);
				//lease->expires = /*time(0) + */server_config.decline_time;
				//hendry
				lease->expires = get_uptime() + server_config.decline_time;
			}
			break;

		case DHCPRELEASE:
			DEBUG(LOG_INFO,"received RELEASE");
			if (lease && lease != static_lease)
			{
				//lease->expires = /*time(0)*/0;
				//hendry 
				lease->expires = get_uptime();
				write_leases();
#ifndef LOGNUM
			syslog(ALOG_NOTICE|LOG_NOTICE, "DHCP: Server receive RELEASE from %s.", mac);
#else
			syslog(ALOG_NOTICE|LOG_NOTICE, "NTC:035[%s]", mac);
#endif
			}
			else if (static_lease)
			{
				static_lease->ACKed = 0;
				write_leases();
			}
			break;

		case DHCPINFORM:
			DEBUG(LOG_INFO,"received INFORM");
			send_inform(&packet);
			break;

		default:
			LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
		}
	}

	return 0;
}

/* added by Leo, 2008/05/12 11:21:49 */
#ifdef UDHCP_DEBUG
#include <stdarg.h>
void _uprintf_(const char *file, const char *func, int line, const char * format, ...)
{
	char *buf;
	int fd,n;
	va_list list;

	fd = open("/dev/console", O_RDWR);

	buf = (char *)calloc(1,1024);

	va_start(list, format);

#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	sprintf(buf,"[%s:%s:%d@%s]\n\t", file, func, line, _wlanif_);
#else
	sprintf(buf,"[%s:%s:%d]\n\t", file, func, line);
#endif

	vsprintf(buf+strlen(buf), format, list);

	if(*(buf+strlen(buf)-1) != '\n') *(buf+strlen(buf)) = '\n';

	n = write(fd, buf, strlen(buf));

	va_end(list);

	free(buf);

	close(fd);
}
#endif
/* ********************************* */


