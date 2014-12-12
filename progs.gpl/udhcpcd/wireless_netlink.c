#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "debug.h"
#include "wireless.h"

#define MACSTR      "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a)  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

extern int binding_ifindex;
extern char blpath[32];

// start of mac blacklist
#ifndef BUFSIZ
	#define BUFSIZ 256
#endif

typedef struct _macblacklist_t_
{
	char *mac;
	struct _macblacklist_t_ *next;
} macblacklist_t;

typedef enum _bl_handle_type_t_
{
	BL_ADD,
	BL_DEL
} bl_handle_t;

static char *bl_msg[] = {
	"add",
	"del"
};

static void free_blacklist(macblacklist_t *entry);
static void print_blacklist(macblacklist_t *entry);
static int read_blacklist(macblacklist_t *entry, FILE *stream);
static int write_blacklist(macblacklist_t *entry, FILE *stream);
static int handle_blacklist(const char *entry, const int len, const char *path, const bl_handle_t type);
int isInBlacklist(const char *entry, const int len, const char *path);
// end of mac blacklist

int get_mi_status(void);

int wireless_netlink_init(void);
void wireless_netlink_wireless_data(char *data, int len, char *addr, int addr_len, int ifindex);
int wireless_newlink_rtm(struct nlmsghdr *h, int len, char *addr, int addr_len);
int wireless_netlink_receive(int sock, char *addr, int addr_len);
void wireless_netlink_deinit(int *sock);
int get_wireless_interface_index(const char *wlanif);

int get_mi_status(void)
{
	FILE *f = NULL;
	char buf[2];

	if((f = popen("rgdb -g /lan/dhcp/server/multiinstances", "r")) == NULL)
	{
		return -1;
	}

	fscanf(f, "%s", buf);
	fclose(f);

	return atoi(buf) > 0 ? 1 : 0;
}

int wireless_netlink_init(void)
{
	int sock;
	struct sockaddr_nl local;

	if((sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0)
	{
		LOG(LOG_ERR, "wireless: socket(PF_NETLINK,SOCK_RAW,NETLINK_ROUTE)");
		return -1;
	}

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = RTMGRP_LINK;

	if(bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0)
	{
		LOG(LOG_ERR, "wireless: bind(netlink)");
		close(sock);
		return -1;
	}

	return sock;
}

void wireless_netlink_wireless_data(char *data, int len, char *addr, int addr_len, int ifindex)
{
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end;

	pos = data;
	end = data + len;
	uprintf("addr_len = %d", addr_len);
	while (pos + IW_EV_LCP_LEN <= end)
	{
		memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);

		if (iwe->len <= IW_EV_LCP_LEN) return;

		if ((iwe->cmd == IWEVMICHAELMICFAILURE || iwe->cmd == IWEVCUSTOM))
		{
			// No necessary to handle it.
			;
		}
		else
		{
			memcpy(&iwe_buf, pos, sizeof(struct iw_event));
		}

		memset(addr, 0, addr_len);
		sprintf(addr,"%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned char)iwe->u.addr.sa_data[0],
			(unsigned char)iwe->u.addr.sa_data[1],
			(unsigned char)iwe->u.addr.sa_data[2],
			(unsigned char)iwe->u.addr.sa_data[3],
			(unsigned char)iwe->u.addr.sa_data[4],
			(unsigned char)iwe->u.addr.sa_data[5]);

		uprintf("binding_ifindex = %d, ifindex = %d\n", binding_ifindex, ifindex);
		switch (iwe->cmd)
		{
			case IWEVEXPIRED:
				//madwifi_del_sta(drv, (u8 *) iwe->u.addr.sa_data);
				//uprintf("del: " MACSTR, MAC2STR(iwe->u.addr.sa_data));
				handle_blacklist(addr, strlen(addr), blpath, BL_DEL);
				uprintf("disconnected(del): %s\n", addr);
				break;

			case IWEVREGISTERED:
				//madwifi_new_sta(drv, (u8 *) iwe->u.addr.sa_data);
				//uprintf("new: " MACSTR, MAC2STR(iwe->u.addr.sa_data));
				if(ifindex != binding_ifindex)
				{
					handle_blacklist(addr, strlen(addr), blpath, BL_ADD);
					uprintf("connected(new): %s\n", addr);
				}
				else
				{
					handle_blacklist(addr, strlen(addr), blpath, BL_DEL);
					uprintf("connected(do del): %s\n", addr);
				}
				break;

			default:
				;
				break;
		}
		pos += iwe->len;
	}
}

int wireless_newlink_rtm(struct nlmsghdr *h, int len, char *addr, int addr_len)
{
	struct ifinfomsg *ifi;
	int attrlen, nlmsg_len, rta_len;
	struct rtattr * attr;

	if((unsigned int)len < sizeof(*ifi)) return -1;

	ifi = NLMSG_DATA(h);

	nlmsg_len = NLMSG_ALIGN(sizeof(struct ifinfomsg));

	attrlen = h->nlmsg_len - nlmsg_len;
	if(attrlen < 0) return -1;

	attr = (struct rtattr *) (((char *) ifi) + nlmsg_len);

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while(RTA_OK(attr, attrlen))
	{
		if(attr->rta_type == IFLA_WIRELESS)
		{
			wireless_netlink_wireless_data(((char *) attr) + rta_len, attr->rta_len - rta_len, addr, addr_len, ifi->ifi_index);
		}
		attr = RTA_NEXT(attr, attrlen);
	}

	return ifi->ifi_index;
}

int wireless_netlink_receive(int sock, char *addr, int addr_len)
{
	char buf[256];
	int left;
	struct sockaddr_nl from;
	socklen_t fromlen;
	struct nlmsghdr *h;

	fromlen = sizeof(from);

	left = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &from, &fromlen);
	if(left < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
		{
			LOG(LOG_ERR,"wireless: recvfrom(netlink)");
		}
		return -1;
	}

	h = (struct nlmsghdr *) buf;
	while((unsigned int)left >= sizeof(*h))
	{
		int len, plen;

		len = h->nlmsg_len;
		plen = len - sizeof(*h);

		if(len > left || plen < 0)
		{
			LOG(LOG_ERR, "wireless: netlink message: len=%d left=%d plen=%d\n", len, left, plen);
			break;
		}

		switch(h->nlmsg_type)
		{
			case RTM_NEWLINK:
				return wireless_newlink_rtm(h, plen, addr, addr_len);
		}

		len = NLMSG_ALIGN(len);
		left -= len;
		h = (struct nlmsghdr *) ((char *) h + len);
	}

	if(left > 0)
	{
		LOG(LOG_ERR,"wireless: %d extra bytes in the end of netlink message\n", left);
	}

	return -1;
}

void wireless_netlink_deinit(int *sock)
{
	if(*sock) close(*sock);
	*sock = -1;
	return;
}

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
int get_wireless_interface_index(const char *wlanif)
{
	int fd;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0)
	{
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, wlanif);

		if(ioctl(fd, SIOCGIFINDEX, &ifr) == 0)
		{
			DEBUG(LOG_INFO, "wireless adapter index %d", ifr.ifr_ifindex);
		}
		else
		{
			close(fd);
			LOG(LOG_ERR, "wireless SIOCGIFINDEX failed!: %s", strerror(errno));
			return -1;
		}
	}
	else
	{
		LOG(LOG_ERR, "wireless socket failed!: %s", strerror(errno));
		return -1;
	}

	close(fd);

	return ifr.ifr_ifindex;
}

static void free_blacklist(macblacklist_t *entry)
{
	if(entry->next != NULL)
	{
		free_blacklist(entry->next);
	}

	free(entry->mac);
	free(entry);
}

static void print_blacklist(macblacklist_t *entry)
{
	macblacklist_t *ptr;

	ptr = entry;
	while(ptr->next != NULL)
	{
		fprintf(stderr, "mac: %s\n", ptr->mac);
		ptr = ptr->next;
	}
}

static int read_blacklist(macblacklist_t *entry, FILE *stream)
{
	macblacklist_t *ptr;
	char *buf = calloc(1, BUFSIZ);
	int buflen;

	if(buf == NULL)
	{
		entry = NULL;
		return -1;
	}

	ptr = entry;
	while(fgets(buf, BUFSIZ, stream) != NULL)
	{
		buflen = strlen(buf) - 1;
		buflen = buflen > 0 ? buflen : 0;
		buf[buflen] = '\0';

		if(buflen == 0) continue;

		ptr->mac = strdup(buf);

		macblacklist_t *newentry = calloc(1, sizeof(macblacklist_t));
		if(newentry == NULL)
		{
			ptr->next = NULL;
			if(buf) free(buf);
			return -1;
		}

		ptr->next = newentry;
		ptr = newentry;
	}

	if(buf) free(buf);
	return 0;
}

static int write_blacklist(macblacklist_t *entry, FILE *stream)
{
	macblacklist_t *ptr;
	int n = -1;

	ptr = (macblacklist_t *)entry;
	while(ptr->next != NULL)
	{
		n = fwrite(ptr->mac, strlen(ptr->mac), 1, stream);
		n = fwrite("\n", 1, 1, stream);
		ptr = ptr->next;
	}

	return n;
}

static int handle_blacklist(const char *entry, const int len, const char *path, const bl_handle_t type)
{
	FILE *file = NULL;
	char *buf = calloc(1, BUFSIZ);
	int ecode = 0;
	macblacklist_t *list = calloc(1, sizeof(macblacklist_t)), *ptr = NULL, *previous = NULL;
	int status = 0, n = 0;

	if(buf == NULL || list == NULL)
	{
		perror("Too low memory.");
		ecode = -1;
		goto handle_end;
	}

	if((file = fopen(path, "r")) == NULL)
	{
		sprintf(buf, "%s(%s)", __FUNCTION__, bl_msg[type]);
		perror(buf);
		ecode = -1;
		goto handle_end;
	}

	read_blacklist(list, file);

	if(file)
	{
		fclose(file);
		file = NULL;
	}

	//print_blacklist(list);

	ptr = list;
	while(ptr->next != NULL)
	{
		n++;
		//uprintf("(%s)ptr->mac: %s\n", bl_msg[type], ptr->mac);
		if(strncmp(ptr->mac, entry, len) == 0)
		{
			status = 1;
			break;
		}
		else
		{
			status = 0;
		}
		previous = ptr;
		ptr = ptr->next;
	}
	uprintf("status = %d", status);

	switch(status)
	{
		case 0:
			if(type == BL_ADD)
			{
				uprintf("%s: The entry %s is not in the blacklist and %s it.\n", bl_msg[type], entry, bl_msg[type]);
				macblacklist_t *newentry = calloc(1, sizeof(macblacklist_t));
				if(newentry == NULL)
				{
					ecode = -1;
					goto handle_end;
				}
				ptr->mac = strdup(entry);
				newentry->next = NULL;
				ptr->next = newentry;

				if((file = fopen(path, "w")) == NULL)
				{
					sprintf(buf, "%s(%s)", __FUNCTION__, bl_msg[type]);
					perror(buf);
					ecode = -1;
					goto handle_end;
				}

				write_blacklist(list, file);
			}
			else if(type == BL_DEL)
			{
				uprintf("%s: The entry %s is not in the blacklist. No necessary to handle.", bl_msg[type], entry);
			}
			break;

		case 1:
			if(type == BL_ADD)
			{
				uprintf("%s: The entry %s is in the blacklist. No necessary to handle.", bl_msg[type], entry);
			}
			else if(type == BL_DEL)
			{
				uprintf("%s: The entry %s is in the blacklist and %s it.\n",  bl_msg[type], entry, bl_msg[type]);
				if(n == 1)
				{
					if(ptr->next->mac) free(ptr->next->mac);
					if(ptr->next)      free(ptr->next);
					ptr->next = NULL;
					ptr->mac  = NULL;
				}
				else
				{
					previous->next = ptr->next;
					if(ptr->mac) free(ptr->mac);
					if(ptr)      free(ptr);
				}

				if((file = fopen(path, "w")) == NULL)
				{
					sprintf(buf, "%s(%s)", __FUNCTION__, bl_msg[type]);
					perror(buf);
					ecode = -1;
					goto handle_end;
				}

				write_blacklist(list, file);
			}
			break;

		default:
			uprintf("It's a tricky status.\n");
			break;
	}

	print_blacklist(list);

handle_end:
	if(file) fclose(file);
	if(buf)  free(buf);
	free_blacklist(list);
	uprintf("ecode = %d", ecode);
	return ecode;
}

int isInBlacklist(const char *entry, const int len, const char *path)
{
	FILE *file = NULL;
	char *buf = calloc(1, BUFSIZ);
	int buflen;
	/*
	ecode :
		-1 -> error
		 0 -> not in blacklist
		 1 -> in blacklist
	*/
	int ecode = 0;

	if(buf == NULL)
	{
		perror("Too low memory.");
		ecode = -1;
		goto end;
	}

	if((file = fopen(path, "r")) == NULL)
	{
		perror("check_file");
		ecode = -1;
		goto end;
	}

	while(fgets(buf, BUFSIZ, file) != NULL)
	{
		buflen = strlen(buf) - 1;
		buflen = buflen > 0 ? buflen : 0;
		buf[buflen] = '\0';

		if(!buflen || buflen != len) continue;

		if(strncmp(buf, entry, len) == 0)
		{
			ecode = 1;
			//uprintf("The MAC %s is in the blacklist\n", entry);
			goto end;
		}
		else
		{
			ecode = 0;
			//uprintf("The MAC %s is not in the blacklist\n", entry);
		}
	}

end:
	if(file) fclose(file);
	if(buf)  free(buf);
	uprintf("ecode = %d\n", ecode);
	return ecode;
}
