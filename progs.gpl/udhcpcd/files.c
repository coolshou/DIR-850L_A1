/* vi: set sw=4 ts=4: */
/*
 * files.c -- DHCP server file manipulation *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>
#include <elbox_config.h>
/* 32 bit change to 64 bit dennis 20080311 end */
/* on these functions, make sure you datatype matches */
static int read_ip(char *line, void *arg)
{
	struct in_addr *addr = arg;
	struct hostent *host;
	int retval = 1;

	if (!inet_aton(line, addr))
	{
		if ((host = gethostbyname(line)))
		/* 32 bit change to 64 bit dennis 20080311 start */
		 	addr->s_addr = *((uint32_t *)host->h_addr_list[0]);
		/* 32 bit change to 64 bit dennis 20080311 end */

		else
			retval = 0;
	}
	return retval;
}


static int read_str(char *line, void *arg)
{
	char **dest = arg;

	if (*dest) free(*dest);
	*dest = strdup(line);

	return 1;
}


static int read_u32(char *line, void *arg)
{
	u_int32_t *dest = arg;
	char *endptr;
	*dest = strtoul(line, &endptr, 0);
	return endptr[0] == '\0';
}


static int read_yn(char *line, void *arg)
{
	char *dest = arg;
	int retval = 1;

	if		(!strcasecmp("yes", line))	*dest = 1;
	else if (!strcasecmp("no", line))	*dest = 0;
	else retval = 0;

	return retval;
}


/* read a dhcp option and add it to opt_list */
static int read_opt(char *line, void *arg)
{
	struct option_set **opt_list = arg;
	char *opt, *val, *endptr;
	struct dhcp_option *option = NULL;
	int retval = 0, length = 0;
	char buffer[255];
	u_int16_t result_u16;
	u_int32_t result_u32;
	int i;

	if (!(opt = strtok(line, " \t="))) return 0;

	for (i = 0; options[i].code; i++)
	{
		if (!strcmp(options[i].name, opt))
			option = &(options[i]);
	}
	if (!option) return 0;

	do {
		//-----Modify by Paul for Domain option which allow ",", " "
		//-----20050628
		length = option_lengths[option->flags & TYPE_MASK];
		if (length>1)	val = strtok(NULL, ", \t");
		else			val = line+strlen(opt)+1;

		if (val)
		{
			retval = 0;
			switch (option->flags & TYPE_MASK)
			{
			case OPTION_IP:
				retval = read_ip(val, buffer);
				break;

			case OPTION_IP_PAIR:
				retval = read_ip(val, buffer);
				if (!(val = strtok(NULL, ", \t/-"))) retval = 0;
				if (retval) retval = read_ip(val, buffer + 4);
				break;

			case OPTION_STRING:
				length = strlen(val);
				if (length > 0)
				{
					if (length > 254) length = 254;
					memcpy(buffer, val, length);
					retval = 1;
				}
				break;

			case OPTION_BOOLEAN:
				retval = read_yn(val, buffer);
				break;

			case OPTION_U8:
				buffer[0] = strtoul(val, &endptr, 0);
				retval = (endptr[0] == '\0');
				break;

			case OPTION_U16:
				result_u16 = htons(strtoul(val, &endptr, 0));
				memcpy(buffer, &result_u16, 2);
				retval = (endptr[0] == '\0');
				break;

			case OPTION_S16:
				result_u16 = htons(strtol(val, &endptr, 0));
				memcpy(buffer, &result_u16, 2);
				retval = (endptr[0] == '\0');
				break;

			case OPTION_U32:
				result_u32 = htonl(strtoul(val, &endptr, 0));
				memcpy(buffer, &result_u32, 4);
				retval = (endptr[0] == '\0');
				break;

			case OPTION_S32:
				result_u32 = htonl(strtol(val, &endptr, 0));
				memcpy(buffer, &result_u32, 4);
				retval = (endptr[0] == '\0');
				break;

			default:
				break;
			}
			if (retval) attach_option(opt_list, option, buffer, length);
		};
	} while (val && retval && option->flags & OPTION_LIST);
	return retval;
}

/* +++ Joy added */
/* read the hardware address; modify from in_ether() in busybox/interface.c */
static int read_haddr(char *line, void *arg)
{
	u_int8_t *chaddr = (u_int8_t *)arg;
	char *bufp;
	unsigned val;
	char c;
	int i;

	bufp = strtok(line, ", \t");

	i = 0;
	while ((*bufp != '\0') && (i < 6)) {
		val = 0;
		c = *bufp++;
		if (isdigit(c))
			val = c - '0';
		else if (c >= 'a' && c <= 'f')
			val = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			val = c - 'A' + 10;
		else {
			return 0;
		}
		val <<= 4;
		c = *bufp;
		if (isdigit(c))
			val |= c - '0';
		else if (c >= 'a' && c <= 'f')
			val |= c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			val |= c - 'A' + 10;
		else if (c == ':' || c == '-' || c == 0)
			val >>= 4;
		else {
			return 0;
		}
		if (c != 0)
			bufp++;
		*chaddr++ = (unsigned char) (val & 0xFF);
		i++;

		/* We might get a colon or dash here - not required. */
		if (*bufp == ':' || *bufp == '-') {
			bufp++;
		}
	}

	/* That's it. Any trailing junk? */
	if ((i == 6) && (*bufp == '\0'))
		return 1;

	return 0;
}

void add_opt(char *val,struct dhcp_option *option,void *arg)
{
	int length,retval;
	char buffer[255];
	char *endptr;
	u_int16_t result_u16;
	u_int32_t result_u32;
	struct option_set **opt_list = arg;
	length = option_lengths[option->flags & TYPE_MASK];
	switch (option->flags & TYPE_MASK)
			{
		case OPTION_IP:
			retval = read_ip(val, buffer);
			break;

		case OPTION_IP_PAIR:
			retval = read_ip(val, buffer);
			if (!(val = strtok(NULL, ", \t/-"))) retval = 0;
			if (retval) retval = read_ip(val, buffer + 4);
			break;
		case OPTION_STRING:
			length = strlen(val);
			if (length > 0)
			{
				if (length > 254) length = 254;
				memcpy(buffer, val, length);
				retval = 1;
			}
			break;

		case OPTION_BOOLEAN:
			retval = read_yn(val, buffer);
			break;

		case OPTION_U8:
			buffer[0] = strtoul(val, &endptr, 0);
			retval = (endptr[0] == '\0');
			break;

		case OPTION_U16:
			result_u16 = htons(strtoul(val, &endptr, 0));
			memcpy(buffer, &result_u16, 2);
			retval = (endptr[0] == '\0');
			break;

		case OPTION_S16:
			result_u16 = htons(strtol(val, &endptr, 0));
			memcpy(buffer, &result_u16, 2);
			retval = (endptr[0] == '\0');
			break;

		case OPTION_U32:
			result_u32 = htonl(strtoul(val, &endptr, 0));
			memcpy(buffer, &result_u32, 4);
			retval = (endptr[0] == '\0');
			break;
		case OPTION_S32:
			result_u32 = htonl(strtol(val, &endptr, 0));
			memcpy(buffer, &result_u32, 4);
			retval = (endptr[0] == '\0');
			break;

		default:
			break;
		}
		if (retval) attach_option(opt_list, option, buffer, length);
}
void cpyact(struct option_set *existing,char index)
{
	struct option_set *new,**curr;
	new = malloc(sizeof(struct option_set));
	new->data = malloc((existing->data[OPT_LEN] )+ 2);
	new->data[OPT_CODE] = existing->data[OPT_CODE];
	new->data[OPT_LEN] = existing->data[OPT_LEN];
	memcpy(new->data + 2, existing->data+2, existing->data[OPT_LEN]);
	curr = &(static_leases[index].options);
	while (*curr && (*curr)->data[OPT_CODE] < existing->data[OPT_CODE]) curr = &(*curr)->next;
	new->next = *curr;
	*curr = new;	
}
void cpyopttostatic_leases(int index)
{
	struct option_set *existing;
	if ((existing = find_option(static_leases[index].options, options[0].code))==NULL)
	{
		if(existing = find_option(server_config.options, options[0].code))
		{
			cpyact(existing,index);
		}
	}
	if ((existing = find_option(static_leases[index].options, options[2].code))==NULL)
	{
		if(existing = find_option(server_config.options, options[2].code))
		{
			cpyact(existing,index);
		}
	}
	if ((existing = find_option(static_leases[index].options, options[5].code))==NULL)
	{
		if(existing = find_option(server_config.options, options[5].code))
		{
			cpyact(existing,index);
		}
	}
	if ((existing = find_option(static_leases[index].options, options[11].code))==NULL)
	{
		if(existing = find_option(server_config.options, options[11].code))
		{
			cpyact(existing,index);
		}
	}
	if ((existing = find_option(static_leases[index].options, options[19].code))==NULL)
	{
		if(existing = find_option(server_config.options, options[19].code))
		{
			cpyact(existing,index);
		}
	}
	
}

/* read ip/mac pair and add it to static DHCP leases */
static int read_static(char *line, void *arg)
{
	static int index = 0;
	char *val;
	int retval = 0,i;
	u_int32_t yiaddr;
	u_int8_t chaddr[16];
	char hostname[64];
	char strchaddr[64];
	struct dhcp_option *option = NULL;
	val = (char *)arg;	// just make compiler happy! by David Hsieh
	
	if (index >= MAX_STATIC_LEASES) return retval;
	val = strtok(line, ", \t");
//	printf("DHCPD: Get hostname [ %s ]\n",val);
	memset(hostname, 0, sizeof(hostname));
	if( strlen(val) > sizeof(hostname))
	{
		memcpy( hostname, val, sizeof(hostname) );
		hostname[ sizeof(hostname) - 1 ] = '\0';
	}
	else
		memcpy(hostname, val, strlen(val));
	if (!(val = strtok(NULL, ", \t/-"))) retval = 0;
	retval = read_ip(val, &yiaddr);
	if (!(val = strtok(NULL, ", \t/-"))) retval = 0;
	memset(strchaddr, 0, sizeof(strchaddr));
	memcpy(strchaddr,val,strlen(val));
	memset(chaddr,0,sizeof(chaddr));

	/*-------------------add by wenwen;2012/02/03------for Static IP assignment-------------------------*/
	//if (retval)	retval = read_haddr(val, chaddr);//add by wenwen;
	while (val = strtok(NULL, " \t")) 
	{
		for (i = 0; options[i].code; i++)
		{
			if (!strcmp(options[i].name, val))
				option = &(options[i]);
		}
		val = strtok(NULL, ", \t");
		add_opt(val,option,&(static_leases[index].options));
	}
	//if (retval)
	//{
		read_haddr(strchaddr, chaddr);
		strcpy(static_leases[index].hostname, hostname);
		static_leases[index].yiaddr = yiaddr;
		memcpy(static_leases[index].chaddr,chaddr,16);
		// Kloat Liu add for identify used static leases 
		static_leases[ index ].ACKed = 0;
		static_leases[ index ].expires = 0xFFFFFFFF;
	//	printf("DHCPD: Read Static [ MAC:%s ACK:%d expires:%08x ]\n", val, static_leases[index].ACKed, static_leases[index].expires);
		// Kloat Liu end
		cpyopttostatic_leases(index);
		
		index++;
	//}
	return retval;
}
/* --- Joy added */

static struct config_keyword keywords[] =
{
	/* keyword[14]	handler   variable address				default[20] */
	{"start",		read_ip,  &(server_config.start),		"192.168.0.20"},
	{"end",			read_ip,  &(server_config.end),			"192.168.0.254"},
#ifdef ELBOX_PROGS_GPL_UDHCP_HOLD_LAN_IP
	{"lan_ip",			read_ip,  &(server_config.lan_ip),			"192.168.0.50"},
#endif	
	{"interface",	read_str, &(server_config.interface),	"eth0"},
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	{"wlanif",      read_str, &(server_config.wlanif),      "ath0"},
#endif
	{"option",		read_opt, &(server_config.options),		""},
	{"opt",			read_opt, &(server_config.options),		""},
	{"max_leases",	read_u32, &(server_config.max_leases),	"254"},
	{"remaining",	read_yn,  &(server_config.remaining),	"yes"},
	{"auto_time",	read_u32, &(server_config.auto_time),	"7200"},
	{"decline_time",read_u32, &(server_config.decline_time),"3600"},
	{"conflict_time",read_u32,&(server_config.conflict_time),"3600"},
	{"offer_time",	read_u32, &(server_config.offer_time),	"60"},
	{"min_lease",	read_u32, &(server_config.min_lease),	"60"},
	{"lease_file",	read_str, &(server_config.lease_file),	"/var/lib/misc/udhcpd.leases"},
	{"pidfile",		read_str, &(server_config.pidfile),		"/var/run/udhcpd.pid"},
	{"notify_file", read_str, &(server_config.notify_file),	""},
	{"siaddr",		read_ip,  &(server_config.siaddr),		"0.0.0.0"},
	{"sname",		read_str, &(server_config.sname),		""},
	{"boot_file",	read_str, &(server_config.boot_file),	""},
	// Sam Chen add for project seattle to call dhcp_helper to process leases file
	{"dhcp_helper",	read_str, &(server_config.dhcp_helper),	""},
	// Sam Chen end
	/*ADDME: static lease */
	/* +++ Joy added static leases */
	{"static",		read_static, NULL,						""},
	/* --- Joy added static leases */
	//joel add for force broadcast.
	{"force_bcast",	read_yn,  &(server_config.force_bcast),	"no"},
	{"",			NULL, 	  NULL,							""}
};


int read_config(char *file)
{
	FILE *in;
	char buffer[153], orig[153], *token, *line;
	int i;

	for (i = 0; strlen(keywords[i].keyword); i++)
	{
		if (strlen(keywords[i].def))
			keywords[i].handler(keywords[i].def, keywords[i].var);
	}

	if (!(in = fopen(file, "r")))
	{
		LOG(LOG_ERR, "unable to open config file: %s", file);
		return 0;
	}

	while (fgets(buffer, 153, in))
	{

		if (strchr(buffer, '\n')) *(strchr(buffer, '\n')) = '\0';
		strncpy(orig, buffer, 153);//add by wenwen;2012/02/03;for Static IP assignment

		//-----Modify by Paul for Domain name which allow "#"
		//-----20050628
		//		if (strchr(buffer, '#')) *(strchr(buffer, '#')) = '\0';
		token = buffer + strspn(buffer, " \t");
		if (*token == '\0') continue;
		line = token + strcspn(token, " \t=");
		if (*line == '\0') continue;
		*line = '\0';
		line++;

		/* eat leading whitespace */
		line = line + strspn(line, " \t=");

		/* eat trailing whitespace */
		for (i = strlen(line); i > 0 && isspace(line[i - 1]); i--);
		line[i] = '\0';
		for (i = 0; strlen(keywords[i].keyword); i++)
		{
			if (!strcasecmp(token, keywords[i].keyword))
			{
				if (!keywords[i].handler(line, keywords[i].var))
				{
					LOG(LOG_ERR, "unable to parse '%s'", orig);
					/* reset back to the default value */
					keywords[i].handler(keywords[i].def, keywords[i].var);
				}
			}
		}
	}
	fclose(in);
	return 1;
}


void write_leases(void)
{
	FILE *fp;
	unsigned int i,j;
	char buf[255];
	//time_t curr = time(0);
	/* 32 bit change to 64 bit dennis 20080311 start */
	uint32_t lease_time;
	/* 32 bit change to 64 bit dennis 20080311 end */
	struct in_addr addr;

	if (!(fp = fopen(server_config.lease_file, "w")))
	{
		LOG(LOG_ERR, "Unable to open %s for writing", server_config.lease_file);
		return;
	}

	DEBUG(LOG_INFO, "writing leases file >>>>>>>>>>>");

	for (i = 0; i < server_config.max_leases; i++)
	{
		if (leases[i].yiaddr != 0)
		{
			if (server_config.remaining)
			{
				if (lease_expired(&(leases[i]))) lease_time = 0;
				else
				{
					#if 0
					// Kloat Liu modified to store relative lease interval instead of time point.
					lease_time = leases[i].expires-server_config.auto_time/* - curr*/;
					leases[i].expires -= server_config.auto_time;
					// Kloat Liu end
					#else
					//+++ hendry
					lease_time = leases[i].expires - get_uptime();
					//--- hendry
					#endif
				}
			}
			else
			{
				lease_time = leases[i].expires;
			}
			lease_time = htonl(lease_time);

			if (leases[i].ACKed)
			{
				fwrite(leases[i].chaddr, 16, 1, fp);
				fwrite(&(leases[i].yiaddr), 4, 1, fp);
				fwrite(&lease_time, 4, 1, fp);
				fwrite(&leases[i].ACKed, 4, 1, fp);
				fwrite(leases[i].hostname, 64, 1, fp);
				addr.s_addr = leases[i].yiaddr;
				DEBUG(LOG_INFO, "%02x:%02x:%02x:%02x:%02x:%02x %s %d",
						leases[i].chaddr[0], leases[i].chaddr[1], leases[i].chaddr[2],
						leases[i].chaddr[3], leases[i].chaddr[4], leases[i].chaddr[5],
						inet_ntoa(addr), (int)ntohl(lease_time));
			}
		}
	}
	// Kloat Liu add for identify used static leases.
	for (j = 0; (j < MAX_STATIC_LEASES) && (static_leases[j].yiaddr); j++)
	{
		lease_time = htonl( static_leases[ j ].expires );
	//	printf("DHCPD: To write static lease item- %s\n", static_leases[j].ACKed==1?"yes":"no");
		if( static_leases[ j ].ACKed )
		{
			fwrite(static_leases[ j ].chaddr, 16, 1, fp);
			fwrite(&(static_leases[ j ].yiaddr), 4, 1, fp);
			fwrite(&lease_time, 4, 1, fp);
			fwrite(&static_leases[ j ].ACKed, 4, 1, fp);
			fwrite(static_leases[ j ].hostname, 64, 1, fp);
			addr.s_addr = static_leases[j].yiaddr;
			DEBUG(LOG_INFO, "%02x:%02x:%02x:%02x:%02x:%02x %s %d",
					static_leases[j].chaddr[0], static_leases[j].chaddr[1], static_leases[j].chaddr[2],
					static_leases[j].chaddr[3], static_leases[j].chaddr[4], static_leases[j].chaddr[5],
					inet_ntoa(addr), (int)ntohl(lease_time));
		}
	}
	// Kloat Liu end
	DEBUG(LOG_INFO, "writing leases file <<<<<<<<<<<");
	fclose(fp);

	// Sam Chen add for project seattle to call dhcp_helper to process leases file
	if (server_config.dhcp_helper)
		system(server_config.dhcp_helper);
	// Sam Chen end

	if (server_config.notify_file)
	{
		sprintf(buf, "%s %s", server_config.notify_file, server_config.lease_file);
		system(buf);
	}
}


void read_leases(char *file)
{
	FILE *fp;
	unsigned int i = 0;
	struct dhcpOfferedAddr lease, *oldest;
	struct in_addr in;
	//struct dhcpOfferedAddr lease;

	if (!(fp = fopen(file, "r")))
	{
		LOG(LOG_ERR, "Unable to open %s for reading", file);
		return;
	}

	DEBUG(LOG_INFO, "reading leases file >>>>");
	while (i < server_config.max_leases && (fread(&lease, sizeof lease, 1, fp) == 1))
	{
#if 0 //Joy modified: exclude MACs already in static lease table
		/* ADDME: is it a static lease */
		if (lease.yiaddr >= server_config.start && lease.yiaddr <= server_config.end) {
			lease.expires = ntohl(lease.expires);
			if (!server_config.remaining) lease.expires -= time(0);
			if (!(oldest = add_lease(lease.chaddr, lease.yiaddr, lease.expires))) {
				LOG(LOG_WARNING, "Too many leases while loading %s\n", file);
				break;
			}
			strncpy(oldest->hostname, lease.hostname, sizeof(oldest->hostname) - 1);
			oldest->hostname[sizeof(oldest->hostname) - 1] = '\0';
			i++;
		}
#else
		if (lease.yiaddr >= server_config.start && lease.yiaddr <= server_config.end && lease.ACKed)
		{
			int matched = 0;
			int j;
			for (j = 0; (j < MAX_STATIC_LEASES) && (static_leases[j].yiaddr); j++)
			{
				in.s_addr = static_leases[j].yiaddr;
				DEBUG(LOG_INFO, "static[%d]: %s %02x:%02x:%02x:%02x:%02x:%02x", j, inet_ntoa(in),
						static_leases[j].chaddr[0], static_leases[j].chaddr[1], static_leases[j].chaddr[2],
						static_leases[j].chaddr[3], static_leases[j].chaddr[4], static_leases[j].chaddr[5]);

				if (!memcmp(lease.chaddr,static_leases[j].chaddr,sizeof(lease.chaddr)) && 
					static_leases[ j ].yiaddr == lease.yiaddr )
				{
					DEBUG(LOG_INFO, "Skip lease %02x:%02x:%02x:%02x:%02x:%02x",
							lease.chaddr[0], lease.chaddr[1], lease.chaddr[2],
							lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
					matched = 1;
					static_leases[ j ].ACKed = 1;
					break;
				}
				if (!memcmp(lease.chaddr,static_leases[j].chaddr,sizeof(lease.chaddr)) || 
					static_leases[ j ].yiaddr == lease.yiaddr )
				{
					DEBUG(LOG_INFO, "Skip lease %s", inet_ntoa(in));
					matched = 1;
					break;
				}
			}
			if (!matched)
			{
				lease.expires = ntohl(lease.expires);

				//hendry
				#if 0 
				// Kloat Liu modified to store relative lease interval instead of time point.
				// We do not need this state now.
				//if (!server_config.remaining) lease.expires -= time(0);
				// Kloat Liu end

				#else
				
				if (!server_config.remaining) lease.expires -= get_uptime();

				#endif 
				
				if( lease.expires == 0xFFFFFFFF )
					lease.expires = server_config.lease;
				if (!(oldest = add_lease(lease.chaddr,lease.yiaddr,lease.expires,lease.hostname)))
				{
					LOG(LOG_WARNING, "Too many leases while loading %s\n", file);
					break;
				}
				oldest->ACKed=1;
				i++;
			}
		}
#endif
	}
	DEBUG(LOG_INFO, "reading leases file <<<<");
	DEBUG(LOG_INFO, "Read %d leases", i);
	fclose(fp);
}

//hendry, please use this function for debug
#if 0
void dump_all_leases()
{
	int i = 0;
	struct dhcpOfferedAddr *curr_lease = NULL;
	struct in_addr addr;
	for(i=0;i<server_config.max_leases ;i++)
	{
		if(leases[i].yiaddr==0)
			continue;
		
		curr_lease = &(leases[i]);
		addr.s_addr = curr_lease->yiaddr;

		DEBUG(LOG_INFO, "  - lease  [%d] %02x:%02x:%02x:%02x:%02x:%02x %20s %10u",
				i,
				curr_lease->chaddr[0], curr_lease->chaddr[1], curr_lease->chaddr[2], curr_lease->chaddr[3], curr_lease->chaddr[4], curr_lease->chaddr[5],
				inet_ntoa(addr) , curr_lease->expires);
		
	}
}
#endif
