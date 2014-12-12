/* script.c
 *
 * Functions to call the DHCP client notification scripts
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//+++ Ivor
#include <math.h>
//--- Ivor
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "options.h"
#include "dhcpd.h"
#include "dhcpc.h"
#include "packet.h"
#include "options.h"
#include "debug.h"
/* 32 bit change to 64 bit dennis 20080311 start */
#include <stdint.h>

#ifdef SIX_RD_OPTION
#include <arpa/inet.h>
#endif

/* 32 bit change to 64 bit dennis 20080311 end */
/* get a rough idea of how long an option will be (rounding up...) */
static int max_option_length[] = {
	[OPTION_IP] =		sizeof("255.255.255.255 "),
	[OPTION_IP_PAIR] =	sizeof("255.255.255.255 ") * 2,
	[OPTION_STRING] =	1,
	[OPTION_BOOLEAN] =	sizeof("yes "),
	[OPTION_U8] =		sizeof("255 "),
	[OPTION_U16] =		sizeof("65535 "),
	[OPTION_S16] =		sizeof("-32768 "),
	[OPTION_U32] =		sizeof("4294967295 "),
	[OPTION_S32] =		sizeof("-2147483684 "),
	//+++ Ivor
	[OPTION_33] =	sizeof("255.255.255.255 ") * 3 + sizeof("32 "),
	//--- Ivor
};

/* prototype */
void run_script(struct dhcpMessage *packet, const char *name);

//+++ Ivor
#if !defined(CLASSLESS_STATIC_ROUTE_OPTION) || !defined(MSCLSLESS_STATIC_ROUTE_OPTION)
static int get_opt121_dotted_dec_len(unsigned char * optionptr);
#endif
//--- Ivor

static int upper_length(int length, struct dhcp_option *option)
{
	return max_option_length[option->flags & TYPE_MASK] *
	       (length / option_lengths[option->flags & TYPE_MASK]);
}


static int sprintip(char *dest, char *pre, char *pos, unsigned char *ip) {
	return sprintf(dest, "%s%d.%d.%d.%d%s", pre, ip[0], ip[1], ip[2], ip[3], pos);
}


/* Fill dest with the text of option 'option'. */
static void fill_options(char *dest, unsigned char *option, struct dhcp_option *type_p)
{
	int type, optlen;
	u_int16_t val_u16;
	int16_t val_s16;
	u_int32_t val_u32;
	int32_t val_s32;
	int len = option[OPT_LEN - 2];
	//+++ Ivor
	int i, nos = 0;
	uint32_t opt121_netmask;
	unsigned char opt121_dst[4] = {0};
	//--- Ivor

	dest += sprintf(dest, "%s=", type_p->name);

	type = type_p->flags & TYPE_MASK;
	optlen = option_lengths[type];
	for(;;) {
		switch (type) {
		//+++ Ivor
		case OPTION_121:
		case OPTION_249:
			/* env[j] output format: (separated by single white space)
			    clsstrout ="netid         netmask         Number of CIDR    gw "
			ex: clsstrout ="192.168.2.128 255.255.255.128 25 192.168.1.102 "
			ex: clsstrout ="192.168.2.128 255.255.255.128 25 192.168.1.102 0.0.0.0 0.0.0.0 0 192.168.1.252 "
			*/
			nos = (int)ceil( ((double)*(option))/8 );
			optlen  = nos + 4 + 1;
			memset(opt121_dst, 0, sizeof(opt121_dst));

			if (nos !=0)
				opt121_netmask = 0xFFFFFFFF;
			else // nos=0 ===> default gw
				opt121_netmask = 0x00000000;

			opt121_netmask <<= (32 - *(option));	
			opt121_netmask = ntohl(opt121_netmask);

			for( i=0; i<nos; i++)
				*(opt121_dst + i) = *(option+ i + 1) ;
			*((uint32_t *)opt121_dst) = *((uint32_t *)opt121_dst) & opt121_netmask;
			dest += sprintip(dest, "", ",", opt121_dst);
			dest += sprintip(dest, "", ",", (unsigned char *)&opt121_netmask);
			dest += sprintf(dest, "%u,", *option);
			dest += sprintip(dest, "", " ", option+nos+1);
			break;
		case OPTION_33:
			/* env[j] output format: (separated by single white space)
			    sstrout ="netid       netmask       Number of CIDR    gw "
			ex: sstrout ="192.168.2.0 255.255.255.0 24 192.168.1.202 "
			ex: sstrout ="5.0.0.0 255.0.0.0 8 192.168.1.205 172.6.6.6 255.255.0.0 16 192.168.1.206 "
			*/
			dest += sprintip(dest, "", ",", option);
			
			/* Bouble - 20121022 - treat all option33 static route as host route(mask=255.255.255.255).
			   Older implement already treat it as host route. Reference fill_envp_opt33(), there are comments from Leon. 
			   Here, why netmask will depend on addredd class? 
			   RFC do not difine clearly,but the window's action  is set host as 32 netmask.We do same action as windows.
			 */
			#if 0 
			if ( (*option & 0x80)  == 0x00) // class A
				dest += sprintf(dest, "255.0.0.0,8,");
			else if ( (*option & 0xC0) == 0x80) // class B
				dest += sprintf(dest, "255.255.0.0,16,");
			else if ( (*option & 0xE0) == 0xC0) // class C
				dest += sprintf(dest, "255.255.255.0,24,");
			else // others, i use 255.255.255.255 as netmask, but it's not good.
			#endif
				dest += sprintf(dest, "255.255.255.255,32,");
			option += 4;
			optlen = 4;
			len -= optlen;
			dest += sprintip(dest, "", " ", option);
			break;
		//--- Ivor
		case OPTION_IP_PAIR:
			dest += sprintip(dest, "", " ", option);
			*(dest++) = '/';
			option += 4;
			optlen = 4;
		case OPTION_IP:	/* Works regardless of host byte order. */
			dest += sprintip(dest, "", " ", option);
 			break;
		case OPTION_BOOLEAN:
			dest += sprintf(dest, *option ? "yes " : "no ");
			break;
		case OPTION_U8:
			dest += sprintf(dest, "%u ", *option);
			break;
		case OPTION_U16:
			memcpy(&val_u16, option, 2);
			dest += sprintf(dest, "%u ", ntohs(val_u16));
			break;
		case OPTION_S16:
			memcpy(&val_s16, option, 2);
			dest += sprintf(dest, "%d ", ntohs(val_s16));
			break;
		case OPTION_U32:
			memcpy(&val_u32, option, 4);
			/* 32 bit change to 64 bit dennis 20080311 start */
			dest += sprintf(dest, "%u ", (uint32_t) ntohl(val_u32));
     		 /* 32 bit change to 64 bit dennis 20080311 end */
			break;
		case OPTION_S32:
			memcpy(&val_s32, option, 4);
			dest += sprintf(dest, "%ld ", (long) ntohl(val_s32));
			break;
		case OPTION_STRING:
			memcpy(dest, option, len);
			dest[len] = '\0';
			return;	 /* Short circuit this case */
		}
		option += optlen;
		len -= optlen;
		if (len <= 0) break;
	}
}


static char *find_env(const char *prefix, char *defaultstr)
{
	extern char **environ;
	char **ptr;
	const int len = strlen(prefix);

	for (ptr = environ; *ptr != NULL; ptr++) {
		if (strncmp(prefix, *ptr, len) == 0)
			return *ptr;
	}
	return defaultstr;
}

#ifdef SIX_RD_OPTION
typedef struct six_rd_option_t
{
	//general option header
	unsigned char code;
	unsigned char option_len;

	//six rd option data
	unsigned char mask_len;
	unsigned char prefix_len;
	struct in6_addr prefix;
	struct in_addr rbip_addr[1];
}__attribute__ ((packed)) six_rd_option;
#endif //SIX_RD_OPTION

/* put all the paramaters into an environment */
static char **fill_envp(struct dhcpMessage *packet)
{
	int num_options = 0;
	int i, j;
	char **envp;
	unsigned char *temp;
	char over = 0;
#ifdef SIX_RD_OPTION
	const six_rd_option *six_rd_opt = NULL;
	int brip_addr_found = 0;
#endif

	if (packet == NULL)
		num_options = 0;
	else {
		for (i = 0; options[i].code; i++)
			if (get_option(packet, options[i].code))
				num_options++;
#ifdef CLASSLESS_STATIC_ROUTE_OPTION
	/* Got option 121, don't count */		
		if (get_option(packet, DHCP_CLSLESS_ROUTE)) {
				num_options--;
		}	
#endif
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
	/* Got option 249, don't count */		
		if (get_option(packet, DHCP_MS_ROUTE)) {
				num_options--;
		}
#endif
#ifdef STATIC_ROUTE_OPTION
	/* Got option 33, don't count */		
		if (get_option(packet, DHCP_STATIC_ROUTE)) {
				num_options--;
		}		
#endif	
#ifdef SIX_RD_OPTION
		if((temp = get_option(packet , DHCP_6RD)))
		{
			//i need whole dhcp option, roll back 2 bytes to option header
			six_rd_opt = (const six_rd_option*)(temp - 2);

			//check its size first
			if((six_rd_opt->option_len + 2) < (unsigned char)(sizeof(*six_rd_opt) - sizeof(struct in_addr)))
				six_rd_opt = NULL;
			else if((six_rd_opt->option_len + 2) >= (unsigned char)sizeof(*six_rd_opt))
			{
				brip_addr_found = 1;
				num_options += 4;
			}
			else
				num_options += 3;
		}
#endif //SIX_RD_OPTION
		if (packet->siaddr) num_options++;
		if ((temp = get_option(packet, DHCP_OPTION_OVER)))
			over = *temp;
		if (!(over & FILE_FIELD) && packet->file[0]) num_options++;
		if (!(over & SNAME_FIELD) && packet->sname[0]) num_options++;
	}
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	envp = xmalloc((num_options + 5+1) * sizeof(char *));
#else
       envp = xmalloc((num_options + 5) * sizeof(char *));
#endif
	envp[0] = xmalloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[1] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[2] = find_env("HOME", "HOME=/");

	if (packet == NULL) {
		envp[3] = NULL;
		return envp;
	}

	envp[3] = xmalloc(sizeof("ip=255.255.255.255"));
	sprintip(envp[3], "ip=", " ", (unsigned char *) &packet->yiaddr);
	for (i = 0, j = 4; options[i].code; i++) {
		if ((temp = get_option(packet, options[i].code))) {
			
			//+++ Ivor
			if (*(temp-OPT_DATA)== DHCP_CLSLESS_ROUTE) { /* option 121 */
			#ifdef CLASSLESS_STATIC_ROUTE_OPTION		
				/* for Russia ISP, don't fill in environment!! */	
				continue;
			#else
				/* calculate envp len */
				envp[j] = xmalloc( get_opt121_dotted_dec_len( temp-OPT_DATA ) + strlen(options[i].name) + 2 );
			#endif
			} else if (*(temp-OPT_DATA)== DHCP_MS_ROUTE) { /* option 249 */
			#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
				/* for Russia ISP, don't fill in environment!! */
				continue;
			#else
				/* calculate envp len */
				envp[j] = xmalloc( get_opt121_dotted_dec_len( temp-OPT_DATA ) + strlen(options[i].name) + 2 );
			#endif
			} else if (*(temp-OPT_DATA)== DHCP_STATIC_ROUTE) { /* option 33 */
			#ifdef STATIC_ROUTE_OPTION		
				/* for Russia ISP, don't fill in environment!! */	
				continue;
			#else
				/* calculate envp len */
				envp[j] = xmalloc(upper_length(temp[OPT_LEN - 2], &options[i]) + strlen(options[i].name) + 2);
			#endif	
			} else { /* other option */
				/* calculate envp len */
				envp[j] = xmalloc(upper_length(temp[OPT_LEN - 2], &options[i]) + strlen(options[i].name) + 2);
			}
			//--- Ivor

			fill_options(envp[j], temp, &options[i]);
			j++;
		}
	}
	if (packet->siaddr) {
		envp[j] = xmalloc(sizeof("siaddr=255.255.255.255"));
		sprintip(envp[j++], "siaddr=", " ", (unsigned char *) &packet->siaddr);
	}
	if (!(over & FILE_FIELD) && packet->file[0]) {
		/* watch out for invalid packets */
		packet->file[sizeof(packet->file) - 1] = '\0';
		envp[j] = xmalloc(sizeof("boot_file=") + strlen((const char *)packet->file));
		sprintf(envp[j++], "boot_file=%s", packet->file);
	}
	if (!(over & SNAME_FIELD) && packet->sname[0]) {
		/* watch out for invalid packets */
		packet->sname[sizeof(packet->sname) - 1] = '\0';
		envp[j] = xmalloc(sizeof("sname=") + strlen((const char *)packet->sname));
		sprintf(envp[j++], "sname=%s", packet->sname);
	}
#ifdef ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
	if (got_ACs_IP){
		envp[j]= xmalloc(sizeof("ac_ip=255.255.255.255"));
		sprintf(envp[j++],"ac_ip=%s",str_ACs_IP);
	}
#endif

#ifdef SIX_RD_OPTION
	if(six_rd_opt)
	{
		int len;
		envp[j] = xmalloc(sizeof("sixrd_msklen=0000000000"));
		sprintf(envp[j++] , "sixrd_msklen=%u" , six_rd_opt->mask_len);

		envp[j] = xmalloc(sizeof("sixrd_prefixlen=0000000000"));
		sprintf(envp[j++] , "sixrd_prefixlen=%u" , six_rd_opt->prefix_len);

		envp[j] = xmalloc(128);
		len = sprintf(envp[j] , "sixrd_prefix=");
		inet_ntop(AF_INET6 , &six_rd_opt->prefix , envp[j] + len , 128 - len);
		++j;

		if(brip_addr_found)
		{
			envp[j] = xmalloc(128);
			len = sprintf(envp[j] , "sixrd_bripaddr=");
			inet_ntop(AF_INET , &six_rd_opt->rbip_addr[0] , envp[j] + len , 128 - len);
			++j;
		}
	}
#endif //SIX_RD_OPTION

	envp[j] = NULL;

	//+++ Ivor
	#if 0
	{ // print out envp for debug
		int k;
		printf("#######################\n ");
		for ( k=0; k<j; k++ )
		{
			printf("envp[%d]=> \"%s\" ", k, envp[k]); 
		}
		printf("\n#######################\n ");
	}
	#endif
	//--- Ivor

	return envp;
}
#ifdef CLASSLESS_STATIC_ROUTE_OPTION
/* put Classess Static Route option paramaters into an environment */
static char **fill_envp_opt121(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain )
{
	unsigned int opt121_len = 0, opt121_mask_width=0;
	unsigned int i;
	char **envp;
	unsigned char *temp;
	unsigned char *opt121;
	unsigned char opt121_netmask[4] = {0};
	unsigned char opt121_dst[4] = {0};
	unsigned char opt121_gateway[4] = {0};
	unsigned int offset = 0;

	if (packet == NULL) {
		return NULL;
	}	
	else {
		/* There is no option 121 in the option list */
		if ((opt121 = get_option(packet, DHCP_CLSLESS_ROUTE)) == NULL) {
			return NULL;
		}	
			
		opt121_len = *(opt121 - OPT_LEN);

		for (i = 0; i < *index; i++) {
			offset += (*(opt121+offset) + 7)/8 + 4 +1;
		}	

		if (opt121_len - (offset + ((*(opt121+offset) + 7)/8 + 4 +1)) <= 0) {
			*remain = 0;
		}
		else {
			*remain = 1;	
		}	
		
		opt121+=offset;
		
		opt121_mask_width = *(opt121);
		
		/* If netmask width > 32, it means something wrong*/
		if (opt121_mask_width > 32) {
			*remain = 0;
			return NULL;
		}	
		
		for (i = 0; i < opt121_mask_width/8; i++) {
			opt121_netmask[i] = 255;
		}

		if (opt121_mask_width%8) {
			opt121_netmask[i] = (unsigned char)(-128 >> (opt121_mask_width%8-1));
		}	
		
		for (i = 0; i < (opt121_mask_width+7)/8; i++) {
			opt121_dst[i] = opt121[1 + i];
		}

		for (i =0; i < 4; i++) {
			opt121_dst[i] &= opt121_netmask[i];
		}	

		for (i = 0; i < 4; i++) {
			opt121_gateway[i] = opt121[1 + (opt121_mask_width+7)/8 + i];
		}
	}

	envp = xmalloc(9 * sizeof(char *));
	envp[0] = xmalloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[1] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[2] = find_env("HOME", "HOME=/");

	envp[3] = xmalloc(sizeof("sdest=255.255.255.255"));
	sprintip(envp[3], "sdest=", " ", opt121_dst);
	envp[4] = xmalloc(sizeof("ssubnet=255.255.255.255"));
	sprintip(envp[4], "ssubnet=", " ", opt121_netmask);
	envp[5] = xmalloc(sizeof("srouter=255.255.255.255"));
	sprintip(envp[5], "srouter=", " ", opt121_gateway);
	envp[6] = xmalloc(sizeof("sindex=255"));
	sprintf(envp[6], "sindex=%u", *index+1);
	temp = envp[7] = xmalloc(sizeof("snum=") + sizeof("99 ") * (*index+1));
	temp += sprintf(temp, "snum=");
	for (i=0; i<=*index; i++) {
		temp += sprintf(temp,"%u ", i+1);
	}	

	envp[8] = NULL;
	return envp;	
}
#endif
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
/* put Classess Static Route option paramaters into an environment */
static char **fill_envp_opt249(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain )
{
	unsigned int opt121_len = 0, opt121_mask_width=0;
	unsigned int i;
	char **envp;
	unsigned char *temp;
	unsigned char *opt121;
	unsigned char opt121_netmask[4] = {0};
	unsigned char opt121_dst[4] = {0};
	unsigned char opt121_gateway[4] = {0};
	unsigned int offset = 0;

	if (packet == NULL) {
		return NULL;
	}	
	else {
		/* There is no option 249 in the option list */
		if ((opt121 = get_option(packet, DHCP_MS_ROUTE)) == NULL) {
			return NULL;
		}	
			
		opt121_len = *(opt121 - OPT_LEN);

		for (i = 0; i < *index; i++) {
			offset += (*(opt121+offset) + 7)/8 + 4 +1;
		}	

		if (opt121_len - (offset + ((*(opt121+offset) + 7)/8 + 4 +1)) <= 0) {
			*remain = 0;
		}
		else {
			*remain = 1;	
		}	
		
		opt121+=offset;
		
		opt121_mask_width = *(opt121);
		
		/* If netmask width > 32, it means something wrong*/
		if (opt121_mask_width > 32) {
			*remain = 0;
			return NULL;
		}	
		
		for (i = 0; i < opt121_mask_width/8; i++) {
			opt121_netmask[i] = 255;
		}

		if (opt121_mask_width%8) {
			opt121_netmask[i] = (unsigned char)(-128 >> (opt121_mask_width%8-1));
		}	
		
		for (i = 0; i < (opt121_mask_width+7)/8; i++) {
			opt121_dst[i] = opt121[1 + i];
		}

		for (i =0; i < 4; i++) {
			opt121_dst[i] &= opt121_netmask[i];
		}	

		for (i = 0; i < 4; i++) {
			opt121_gateway[i] = opt121[1 + (opt121_mask_width+7)/8 + i];
		}
	}

	envp = xmalloc(9 * sizeof(char *));
	envp[0] = xmalloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[1] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[2] = find_env("HOME", "HOME=/");

	envp[3] = xmalloc(sizeof("sdest=255.255.255.255"));
	sprintip(envp[3], "sdest=", " ", opt121_dst);
	envp[4] = xmalloc(sizeof("ssubnet=255.255.255.255"));
	sprintip(envp[4], "ssubnet=", " ", opt121_netmask);
	envp[5] = xmalloc(sizeof("srouter=255.255.255.255"));
	sprintip(envp[5], "srouter=", " ", opt121_gateway);
	envp[6] = xmalloc(sizeof("sindex=255"));
	sprintf(envp[6], "sindex=%u", *index+1);
	temp = envp[7] = xmalloc(sizeof("snum=") + sizeof("99 ") * (*index+1));
	temp += sprintf(temp, "snum=");
	for (i=0; i<=*index; i++) {
		temp += sprintf(temp,"%u ", i+1);
	}	

	envp[8] = NULL;
	return envp;	
}
#endif
#ifdef STATIC_ROUTE_OPTION
/* put Classess Static Route option paramaters into an environment */
static char **fill_envp_opt33(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain )
{
	unsigned int opt33_len = 0; 
	unsigned int i;
	char **envp;
	unsigned char *temp;
	unsigned char *opt33;
	unsigned char opt33_netmask[4] = {0};
	unsigned char opt33_dst[4] = {0};
	unsigned char opt33_gateway[4] = {0};
	unsigned int offset = 0;

	if (packet == NULL) {
		return NULL;
	}	
	else {
		/* There is no option 33 in the option list */
		if ((opt33 = get_option(packet, DHCP_STATIC_ROUTE)) == NULL) {
			return NULL;
		}	
			
		opt33_len = *(opt33 - OPT_LEN);

		for (i = 0; i < *index; i++) {
			offset += 4 + 4;
		}	

		if (opt33_len - (offset + 4 +4) <= 0) {
			*remain = 0;
		}
		else {
			*remain = 1;	
		}	
		
		opt33+=offset;
		
		/*Here, I have a bit confusion. Option 33 doesn't contain netmask information so I should check the first byte of IP to determite
		   the netmask. i.e if the MSB of IP is 0xxxxxxxB then it's a class A IP, the netmask should be 255.0.0.0, if the MSB of IP is 10xxxxxxB 
		   then it's a class B IP, the netmask should be 255.255.0.0 and if the MSB of IP is 110xxxxxB then it's a class C IP, the netmask should be
		   255.255.255.0 .But here I just fill in netmask 255.255.255.255 to specify that the destination address is not a net but host to meet
		   customer's request. Leon,20090312*/						
		for (i = 0; i < 4; i++) {
			opt33_netmask[i] = 255;
		}
		
		for (i = 0; i < 4; i++) {
			opt33_dst[i] = opt33[i];
		}

		for (i = 0; i < 4; i++) {
			opt33_gateway[i] = opt33[4 + i];
		}
	}

	envp = xmalloc(9 * sizeof(char *));
	envp[0] = xmalloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[1] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[2] = find_env("HOME", "HOME=/");

	envp[3] = xmalloc(sizeof("sdest=255.255.255.255"));
	sprintip(envp[3], "sdest=", " ", opt33_dst);
	envp[4] = xmalloc(sizeof("ssubnet=255.255.255.255"));
	sprintip(envp[4], "ssubnet=", " ", opt33_netmask);
	envp[5] = xmalloc(sizeof("srouter=255.255.255.255"));
	sprintip(envp[5], "srouter=", " ", opt33_gateway);
	envp[6] = xmalloc(sizeof("sindex=255"));
	sprintf(envp[6], "sindex=%u", *index+1);
	temp = envp[7] = xmalloc(sizeof("snum=") + sizeof("99 ") * (*index+1));
	temp += sprintf(temp, "snum=");
	for (i=0; i<=*index; i++) {
		temp += sprintf(temp,"%u ", i+1);
	}	

	envp[8] = NULL;
	return envp;	
}
#endif
/* Call a script with a par file and env vars */
void run_script(struct dhcpMessage *packet, const char *name)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;

	/* call script */
	pid = vfork();
	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	} else if (pid == 0) {
		envp = fill_envp(packet);

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       name, NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
		exit(1);
	}
}
#ifdef CLASSLESS_STATIC_ROUTE_OPTION
/* Call a script with a par file and env vars */
void run_script_opt121(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;
	
	envp = fill_envp_opt121(packet,index,remain);
	if (envp == NULL)
		return;
	
	/* call script */
	pid = vfork();
	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	} else if (pid == 0) {

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       "classlessstaticroute", NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
				DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		exit(1);
	}
}
#endif
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
/* Call a script with a par file and env vars */
void run_script_opt249(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;
	
	envp = fill_envp_opt249(packet,index,remain);
	if (envp == NULL)
		return;
	
	/* call script */
	pid = vfork();
	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	} else if (pid == 0) {

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       "classlessstaticroute", NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
				DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		exit(1);
	}
}
#endif
#ifdef STATIC_ROUTE_OPTION
/* Call a script with a par file and env vars */
void run_script_opt33(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;
	
	envp = fill_envp_opt33(packet,index,remain);
	if (envp == NULL)
		return;
	
	/* call script */
	pid = vfork();
	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	} else if (pid == 0) {

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       "staticroute", NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
				DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
	
		exit(1);
	}
}
#endif
//+++ Ivor
#if !defined(CLASSLESS_STATIC_ROUTE_OPTION) || !defined(MSCLSLESS_STATIC_ROUTE_OPTION)
static int get_opt121_dotted_dec_len(unsigned char * optionptr)
{
	int len, i=0, nos=0;/* nos: number of significant octets(bytes), possible value: 0,1,2,3,4 */
	int dotted_dec_len = 0;
	unsigned char * opt_data = optionptr+OPT_DATA;
	len = *(optionptr+OPT_LEN);
	for(i=0; i<len; )
	{
		nos = (int)ceil( ((double)*(opt_data+i))/8 );
		dotted_dec_len += strlen("111.111.111.111 ")	/* netid			*/
						+ strlen("255.255.255.255 ")	/* netmask 			*/
						+ strlen("25 ")					/* cidr				*/	
						+ strlen("222.222.222.222 ");	/* gw				*/
		i += nos + 4 + 1;
	}
	return dotted_dec_len;		
}
#endif
//--- Ivor
