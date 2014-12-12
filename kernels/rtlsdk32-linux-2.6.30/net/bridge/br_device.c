/*
 *	Device handling code
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>

#include <asm/uaccess.h>
#include "br_private.h"
//Alpha
#include <net/udp.h>
#if defined (CONFIG_RTL_IGMP_SNOOPING)
/*2008-01-15,for porting igmp snooping to linux kernel 2.6*/
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/igmp.h>
#include <net/checksum.h>
#include <net/rtl/rtl865x_igmpsnooping_glue.h>
#include <net/rtl/rtl865x_igmpsnooping.h>
extern int igmpsnoopenabled;
#if defined (CONFIG_RTL_MLD_SNOOPING)
#include <linux/ipv6.h>
#include <linux/in6.h>
extern int mldSnoopEnabled;
#endif
#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
#include <net/rtl/rtl865x_multicast.h>
#include <net/rtl/rtl865x_netif.h>
#include <net/rtl/rtl_nic.h>
#endif
extern unsigned int br0SwFwdPortMask;
extern unsigned int brIgmpModuleIndex;
extern unsigned int nicIgmpModuleIndex;
#endif

#if defined(CONFIG_RTL_HW_VLAN_SUPPORT)
extern uint32 rtl_hw_vlan_get_tagged_portmask(void);
#endif

#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
extern unsigned int br1SwFwdPortMask;
extern unsigned int nicIgmpModuleIndex_2;
#endif
#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
int rtl865x_ipMulticastHardwareAccelerate(struct net_bridge *br, unsigned int brFwdPortMask,
												unsigned int srcPort,unsigned int srcVlanId, 
												unsigned int srcIpAddr, unsigned int destIpAddr);
#endif
#if defined (CONFIG_RTL_MLD_SNOOPING)
extern int re865x_getIpv6TransportProtocol(struct ipv6hdr* ipv6h);
#endif

#ifdef CONFIG_RTK_INBAND_HOST_HACK
#define ETHER_HDR_LEN 14
#define ARP_HRD_LEN 8
extern unsigned char inband_Hostmac[];
extern int br_hackMac_enable;
extern void check_listen_info(struct sk_buff *skb);
#endif

//Alpha
#if defined (CONFIG_BRIDGE_MDNSFILTER_BWCTRL)
#define mdns_dbg 0
//#define Mdns_CheckString "shareport"
#define MdnsPort  5353
#define dev_for_shareport_pkg "wlan0"
#define dev_for_NoShareport_pkg "eth1"
struct  MdnsDomain
{
unsigned short id;
unsigned short flag;
unsigned short questions;
unsigned short answer;
unsigned short authority;
unsigned short additional;
};
struct  Mdns_CheckString_
{
unsigned short len;
unsigned char *checkstring;
};
struct MdnsStaus
{
	unsigned char delflag;
	unsigned short addnum;// 0:answer  1: additional
	unsigned short len;//record len
	unsigned short point; //  09,shareport,0c,XX  .  point = 0cXX
	unsigned short new_point;
	unsigned int point_offset; //  09,shareport,0c,XX  . point_offset is the offset of the 0x xx
	unsigned int offset; //record offset 
};
static struct  Mdns_CheckString_  Mdns_CheckString[]={ //define the check string
{
.len = 9,
.checkstring = "shareport",
},
{
.len = 27,
.checkstring = "D-Link SharePort Web Access",
},//add the check string here,the last one must be NULL
{
.len = 0,
.checkstring = NULL
}
};
static int check_address(char *start,int maxoffset,char *pointe)
{
	if(pointe<start || pointe >(start + maxoffset))
	{
		if(mdns_dbg)panic_printk("<3>""-----check_address-- failed--start=%p--maxoffset=%x--point=%p\n",start,maxoffset,pointe);
		return -1;
	}
	else
		return 0;
}
static int check_string(unsigned char * str)
{	
	int i;
	unsigned int check_string_num = sizeof(Mdns_CheckString)/sizeof(Mdns_CheckString[0]);
	for(i=0;i<(check_string_num-1);i++)
	{
		if(*str == Mdns_CheckString[i].len){
			if(memcmp(str+1,Mdns_CheckString[i].checkstring,Mdns_CheckString[i].len)==0){
				return 0;
			}
		}
	}
return -1;
}
static __sum16 Udp_CheckSum(const struct sk_buff *skb)
{
	unsigned int udphoff;
	struct iphdr *iphd=(struct iphdr *)skb_network_header(skb);
	struct ipv6hdr *iphd6= (struct iphdr *)skb_network_header(skb);
	struct udphdr  *udphd=skb_transport_header(skb);
	__wsum udp_data_sum ;

	udphd->check =0;
	udp_data_sum = csum_partial((unsigned char *)udphd, udphd->len,0);
	if(iphd->version ==4){
		udphd->check = csum_tcpudp_magic(iphd->saddr,iphd->daddr,udphd->len,17,udp_data_sum);
	}
	else if(iphd->version ==6){
		udphd->check = csum_ipv6_magic(&iphd6->saddr,&iphd6->daddr,udphd->len,17,udp_data_sum);
	}
	else{
		return 0;
	}
}
static unsigned short  Mdns_CheckName(char *name,struct MdnsStaus *Mdns_Staus)
{
		unsigned char *name_tmp= name;
		unsigned short *offset = name;
		unsigned int name_len=0;
		
		unsigned int check_string_num = sizeof(Mdns_CheckString)/sizeof(Mdns_CheckString[0]);
		Mdns_Staus->delflag = -1;
		Mdns_Staus->point = 0;
		Mdns_Staus->addnum= 0;
		if(mdns_dbg)panic_printk("<3>""-----1--start checkname--------\n");
		if( name_tmp==NULL || Mdns_Staus==NULL)return -1;
		if(mdns_dbg)panic_printk("<3>""%x,",*name_tmp);
		if((*name_tmp & 0xc0)==0xc0)
		{
			if(mdns_dbg)panic_printk("<3>""-----2--end checkname--%x------\n",*offset);
			return *offset;
		}
		else
		{
				while(*name_tmp != 0)
				{	if(mdns_dbg)panic_printk("<3>""%x,",*name_tmp);
					if(Mdns_Staus->delflag != 0)
							Mdns_Staus->delflag = check_string(name_tmp);
					name_len = name_len + *name_tmp +1;
					name_tmp = name_tmp + *name_tmp +1;
					if(*name_tmp == 0xc0) // .loacl
					{	
						Mdns_Staus->point_offset = name_len;
						Mdns_Staus->point  =  (*((unsigned short *)name_tmp))  ;
						name_len ++;
						break;
					}
				}
				if(mdns_dbg)panic_printk("<3>""\n-----3--end checkname---name_len+1=%d-----\n",name_len+1);
			return  (name_len+1);
		}
}
static int Mdns_FindSharePortPkg(const struct sk_buff *old_skb,const char* data_point,const struct MdnsDomain *old_Mdns_Domain,struct sk_buff *new_skb)
{
		int i,j;
		
		struct MdnsDomain *new_Mdns_Domain;
		char* new_data_point;
		struct MdnsStaus Mdns_Staus[50];
		struct MdnsStaus Mdns_name_point_status;
		unsigned int name_point_len;
		unsigned int name_point_tmp;
		unsigned int record_len=0;
		unsigned short record_len_tmp=0;
		unsigned short new_name_ponint;
		char * old_data_point =  data_point;
		unsigned short *data_len=NULL;
		
		if(mdns_dbg)panic_printk("<3>""-----1--Buildpkg--------\n");
		if(old_skb==NULL ||  data_point==NULL || old_Mdns_Domain==NULL ||new_skb==NULL)
				return -1;
		if(old_Mdns_Domain->questions>0 || old_Mdns_Domain->authority > 0)
				return -2;

		struct udphdr  *udphd_old =(struct udphdr*)skb_transport_header(old_skb);
		struct udphdr  *udphd_new=skb_transport_header(new_skb);
	 	 new_data_point = (char*)udphd_new + sizeof(struct udphdr);
	  	 new_Mdns_Domain = (struct MdnsDomain *)new_data_point;
	  	new_data_point = new_data_point + sizeof(struct MdnsDomain);
	  	Mdns_Staus[0].offset = 0xc00c;
		new_Mdns_Domain->answer =0;
		new_Mdns_Domain->additional=0;
		
		for(i=0;i<old_Mdns_Domain->answer;i++)
		{			
			record_len_tmp = Mdns_CheckName(old_data_point,&Mdns_Staus[i]); //check the name ,and if the check name is include set delflag
			if((record_len_tmp & 0xc000)== 0xc000)
			{
				data_len = old_data_point + 10; 
				record_len =  *data_len  +12;  // 2 + 2 + 2+ 4 + 2+datalen
				new_name_ponint = record_len_tmp ;
				for(j=0;j<i;j++)
				{	if(mdns_dbg)panic_printk("<3>""-----1.1--Buildpkg-anwser num=%d--j=%d--data_len=%x---record_len=%x--record_len_tmp =%x,--[j].offset=%x,[j+1].offset=%x \n",i,j,*data_len,record_len,record_len_tmp,Mdns_Staus[j].offset,Mdns_Staus[j+1].offset);
					if(record_len_tmp >= Mdns_Staus[j].offset && record_len_tmp < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) 
							return -1; // the point offset is del
					}

				}
			}
			else if(record_len_tmp >0)
			{	
				data_len = old_data_point + record_len_tmp + 8; 
				record_len = record_len_tmp + *data_len  +10;//name + 2 + 2+ 4 + 2+datalen
				if(Mdns_Staus[i].delflag == 0)return -1; //find share port ,return
				if(Mdns_Staus[i].point !=0 && Mdns_Staus[i].delflag != 0){
					Mdns_Staus[i].new_point = Mdns_Staus[i].point;
					for(j=0;j<i;j++)
					{
					if(Mdns_Staus[i].point >= Mdns_Staus[j].offset && Mdns_Staus[i].point < Mdns_Staus[j+1].offset)
					{	if(Mdns_Staus[j].delflag ==0) 
							return -1; // the point offset is contain shareport
					}
					}
				}
			}
			else
			{
				return -3;//error
			}
			
			Mdns_Staus[i].len = record_len;
			old_data_point += record_len;
			if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
			Mdns_Staus[i+1].offset = Mdns_Staus[i].offset + record_len ;
			if(mdns_dbg)panic_printk("<3>""-----2--Buildpkg-answer num=%d--offset=%x---record_len=%x--delflag=%d--\n",i,Mdns_Staus[i].offset,record_len,Mdns_Staus[i].delflag);
		}
		
		for(i=old_Mdns_Domain->answer;i< old_Mdns_Domain->additional+old_Mdns_Domain->answer;i++)  // for additional 
		{		
			record_len_tmp = Mdns_CheckName(old_data_point,&Mdns_Staus[i]); //check the name ,and if the check name is include set delflag
			if((record_len_tmp & 0xc000)== 0xc000)
			{
				new_name_ponint = record_len_tmp ;
				data_len = old_data_point + 10; // old_data_point + 2 +2++2+4
				record_len =  *data_len  +12;  // 2 + 2 + 2+ 4 + 2+datalen
				for(j=0;j<i;j++)
				{	
					if(mdns_dbg)panic_printk("<3>""-----1.1--Buildpkg-anwser num=%d--j=%d--data_len=%x---record_len=%x--record_len_tmp =%x,--[j].offset=%x,[j+1].offset=%x \n",i,j,*data_len,record_len,record_len_tmp,Mdns_Staus[j].offset,Mdns_Staus[j+1].offset);
					if(record_len_tmp >= Mdns_Staus[j].offset && record_len_tmp < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) //del the record
							return -1;
					}
				}
			}
			else if(record_len_tmp >0)
			{
				data_len = old_data_point + record_len_tmp + 8; 
				record_len = record_len_tmp + *data_len  +10;//name + 2 + 2+ 4 + 2+datalen
				if(Mdns_Staus[i].delflag == 0)return -1; //find share port ,return
				if(Mdns_Staus[i].point !=0 && Mdns_Staus[i].delflag != 0){
					Mdns_Staus[i].new_point = Mdns_Staus[i].point;
					for(j=0;j<i;j++){
					if(Mdns_Staus[i].point >= Mdns_Staus[j].offset && Mdns_Staus[i].point < Mdns_Staus[j+1].offset){
						if(Mdns_Staus[j].delflag ==0) //
							return -1;
					}
					}
				}
			}
			else
			{
				return -4;
			}
			
			Mdns_Staus[i].len = record_len;
			old_data_point += record_len;	
			if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
			Mdns_Staus[i+1].offset = Mdns_Staus[i].offset + record_len ;
			if(mdns_dbg)panic_printk("<3>""-----4--Buildpkg-addtion num=%d--offset=%x---record_len=%x--old_data_point=%x,delflag=%d--\n",i,Mdns_Staus[i].offset,record_len,(char *)old_data_point,Mdns_Staus[i].delflag );
		}
		
return 1 ;	
}
/*
static int Mdns_BuildNoShareportPkg(const struct sk_buff *old_skb,const char* data_point,const struct MdnsDomain *old_Mdns_Domain,struct sk_buff *new_skb)
{
		int i,j;
		
		struct MdnsDomain *new_Mdns_Domain;
		char* new_data_point;
		struct MdnsStaus Mdns_Staus[50];
		struct MdnsStaus Mdns_name_point_status;
		unsigned int name_point_len;
		unsigned int name_point_tmp;
		unsigned int record_len=0;
		unsigned short record_len_tmp=0;
		unsigned int new_total_data_len=0;
		unsigned short new_name_ponint;
		char * old_data_point =  data_point;
		unsigned short *data_len=NULL;
		
		if(mdns_dbg)panic_printk("<3>""-----1--Buildpkg--------\n");
		if(old_skb==NULL ||  data_point==NULL || old_Mdns_Domain==NULL ||new_skb==NULL)
				return -1;
		if(old_Mdns_Domain->questions>0 || old_Mdns_Domain->authority > 0)
				return -2;

		struct udphdr  *udphd_old =(struct udphdr*)skb_transport_header(old_skb);
		struct udphdr  *udphd_new=skb_transport_header(new_skb);
	 	 new_data_point = (char*)udphd_new + sizeof(struct udphdr);
	  	 new_Mdns_Domain = (struct MdnsDomain *)new_data_point;
	  	new_data_point = new_data_point + sizeof(struct MdnsDomain);
	  	Mdns_Staus[0].offset = 0xc00c;
		new_Mdns_Domain->answer =0;
		new_Mdns_Domain->additional=0;
		
		for(i=0;i<old_Mdns_Domain->answer;i++)
		{			
			record_len_tmp = Mdns_CheckName(old_data_point,&Mdns_Staus[i]); //check the name ,and if the check name is include set delflag
			if((record_len_tmp & 0xc000)== 0xc000)
			{
				data_len = old_data_point + 10; 
				record_len =  *data_len  +12;  // 2 + 2 + 2+ 4 + 2+datalen
				new_name_ponint = record_len_tmp ;
				for(j=0;j<i;j++)
				{	if(mdns_dbg)panic_printk("<3>""-----1.1--Buildpkg-anwser num=%d--j=%d--data_len=%x---record_len=%x--record_len_tmp =%x,--[j].offset=%x,[j+1].offset=%x \n",i,j,*data_len,record_len,record_len_tmp,Mdns_Staus[j].offset,Mdns_Staus[j+1].offset);
					if(record_len_tmp >= Mdns_Staus[j].offset && record_len_tmp < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) //del the record
						{
							Mdns_Staus[i].delflag = 0;
							//data_len = old_data_point + 2 + 8; 
							//record_len =  *data_len  +12;  // 2 + 2 + 2+ 4 + 2+datalen
						}
						else
						{
							if(check_address(new_Mdns_Domain,(udphd_old->len-8),(new_data_point +2)) <0) return -10;
							if(check_address(data_point,(udphd_old->len-20),(old_data_point+2)) <0) return -10;
							memcpy(new_data_point +2,old_data_point+2,record_len-2);
							*(unsigned short *)new_data_point = new_name_ponint;//set new name point
							new_Mdns_Domain->answer ++;
							new_total_data_len = new_total_data_len + record_len;
							new_data_point  += record_len;							
						}
						break;
					}
					else
					{
						if(Mdns_Staus[j].delflag ==0)
					 		new_name_ponint  -=  Mdns_Staus[j].len;
						else if(Mdns_Staus[j].addnum !=0)
							new_name_ponint   +=  Mdns_Staus[j].addnum;
						else{//do nothing
						}						
					}
				}
				if(j==i)return -3;//error
			}
			else if(record_len_tmp >0)
			{	
				data_len = old_data_point + record_len_tmp + 8; 
				record_len = record_len_tmp + *data_len  +10;//name + 2 + 2+ 4 + 2+datalen
				if(Mdns_Staus[i].point !=0 && Mdns_Staus[i].delflag != 0){
					Mdns_Staus[i].new_point = Mdns_Staus[i].point;
					for(j=0;j<i;j++){
					if(Mdns_Staus[i].point >= Mdns_Staus[j].offset && Mdns_Staus[i].point < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) //copy and add the name
						{
							//Mdns_Staus[i].delflag == 0; //change here
							name_point_tmp = Mdns_Staus[i].point & 0x3f;
							name_point_tmp  = name_point_tmp -0x0c;
							name_point_len = Mdns_CheckName((data_point+name_point_tmp),&Mdns_name_point_status);
							if(name_point_len == 0xc000||Mdns_name_point_status.delflag==0){
								Mdns_Staus[i].delflag = 0;
							}else{
							memcpy(new_data_point ,old_data_point,Mdns_Staus[i].point_offset);
							new_data_point += Mdns_Staus[i].point_offset;	
							new_total_data_len += Mdns_Staus[i].point_offset;
							memcpy(new_data_point ,(data_point+name_point_tmp),name_point_len);
							new_data_point += name_point_len;	
							new_total_data_len += name_point_len;
							memcpy(new_data_point ,(old_data_point+Mdns_Staus[i].point_offset+2),(record_len-Mdns_Staus[i].point_offset-2));
							new_data_point = new_data_point+ (record_len-Mdns_Staus[i].point_offset-2);	
							new_total_data_len += name_point_len;	
							Mdns_Staus[i].addnum = name_point_len -2;
							new_Mdns_Domain->answer ++;
							}
						}
						else  //just change the name point
						{
			
							if(check_address(new_Mdns_Domain,(udphd_old->len-8),new_data_point ) <0) return -10;
							if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
							memcpy(new_data_point ,old_data_point,record_len);
							new_Mdns_Domain->answer ++;
							*(unsigned short *)(new_data_point + Mdns_Staus[i].point_offset) = Mdns_Staus[i].new_point ;
							new_total_data_len = new_total_data_len + record_len;
							new_data_point += record_len;
											
						}
						break;
					}
					else
					{
						if(Mdns_Staus[j].delflag ==0)
					 		Mdns_Staus[i].new_point  -=  Mdns_Staus[j].len;
						else if(Mdns_Staus[j].addnum !=0)
							Mdns_Staus[i].new_point  +=  Mdns_Staus[j].addnum;
						else{//do nothing
						}
					}
						
					}
				}else{
				if(Mdns_Staus[i].delflag != 0){
					if(check_address(new_Mdns_Domain,(udphd_old->len-8),new_data_point ) <0) return -10;
					if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
					memcpy(new_data_point ,old_data_point,record_len);
					new_Mdns_Domain->answer ++;
					new_total_data_len = new_total_data_len + record_len;
					new_data_point += record_len;
				}
				}
			}
			else
			{
				return -3;//error
			}
			
			Mdns_Staus[i].len = record_len;
			old_data_point += record_len;
			Mdns_Staus[i+1].offset = Mdns_Staus[i].offset + record_len ;
			if(mdns_dbg)panic_printk("<3>""-----2--Buildpkg-answer num=%d--offset=%x---record_len=%x--delflag=%d--\n",i,Mdns_Staus[i].offset,record_len,Mdns_Staus[i].delflag);
		}
		
		for(i=old_Mdns_Domain->answer;i< old_Mdns_Domain->additional+old_Mdns_Domain->answer;i++)  // for additional 
		{		
			record_len_tmp = Mdns_CheckName(old_data_point,&Mdns_Staus[i]); //check the name ,and if the check name is include set delflag
			if((record_len_tmp & 0xc000)== 0xc000)
			{
				new_name_ponint = record_len_tmp ;
				data_len = old_data_point + 10; // old_data_point + 2 +2++2+4
				record_len =  *data_len  +12;  // 2 + 2 + 2+ 4 + 2+datalen
				for(j=0;j<i;j++)
				{	
					if(mdns_dbg)panic_printk("<3>""-----1.1--Buildpkg-anwser num=%d--j=%d--data_len=%x---record_len=%x--record_len_tmp =%x,--[j].offset=%x,[j+1].offset=%x \n",i,j,*data_len,record_len,record_len_tmp,Mdns_Staus[j].offset,Mdns_Staus[j+1].offset);
					if(record_len_tmp >= Mdns_Staus[j].offset && record_len_tmp < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) //del the record
						{
							Mdns_Staus[i].delflag = 0;
						}
						else
						{
							if(check_address(new_Mdns_Domain,(udphd_old->len-8),(new_data_point +2)) <0) return -10;
							if(check_address(data_point,(udphd_old->len-20),(old_data_point+2)) <0) return -10;
							*(unsigned short *)new_data_point = new_name_ponint;//set new name point
							memcpy(new_data_point+2,old_data_point+2,record_len-2);
							new_Mdns_Domain->additional++;
							new_total_data_len = new_total_data_len + record_len;
							new_data_point += record_len;							
						}
						break;
					}
					else
					{
						if(Mdns_Staus[j].delflag ==0)
					 		new_name_ponint  -=  Mdns_Staus[j].len;
						else if(Mdns_Staus[j].addnum !=0)
							new_name_ponint   +=  Mdns_Staus[j].addnum;
						else{//do nothing
						}		
					}
				}
				if(j==i)return -3;//error ,can not find the namepoint
			}
			else if(record_len_tmp >0)
			{
				data_len = old_data_point + record_len_tmp + 8; 
				record_len = record_len_tmp + *data_len  +10;//name + 2 + 2+ 4 + 2+datalen
				if(Mdns_Staus[i].point !=0 && Mdns_Staus[i].delflag != 0){
					Mdns_Staus[i].new_point = Mdns_Staus[i].point;
					for(j=0;j<i;j++){
					if(Mdns_Staus[i].point >= Mdns_Staus[j].offset && Mdns_Staus[i].point < Mdns_Staus[j+1].offset)
					{
						if(Mdns_Staus[j].delflag ==0) //
						{
							//Mdns_Staus[i].delflag == 0; //change here
							name_point_tmp = Mdns_Staus[i].point & 0x3f;
							name_point_tmp  = name_point_tmp -0x0c;
							name_point_len = Mdns_CheckName((data_point+name_point_tmp),&Mdns_name_point_status);
							if(name_point_len == 0xc000 ||Mdns_name_point_status.delflag==0){
								Mdns_Staus[i].delflag = 0;
							}else{
							memcpy(new_data_point ,old_data_point,Mdns_Staus[i].point_offset);
							new_data_point += Mdns_Staus[i].point_offset;	
							new_total_data_len += Mdns_Staus[i].point_offset;
							if(check_address(data_point,(udphd_old->len-20),(data_point+name_point_tmp)) <0) return -10;
							memcpy(new_data_point ,(data_point+name_point_tmp),name_point_len);
							new_data_point += name_point_len;	
							new_total_data_len += name_point_len;
							memcpy(new_data_point ,(old_data_point+Mdns_Staus[i].point_offset+2),(record_len-Mdns_Staus[i].point_offset-2));
							new_data_point = new_data_point+ (record_len-Mdns_Staus[i].point_offset-2);	
							new_total_data_len += name_point_len;	
							Mdns_Staus[i].addnum = name_point_len -2;
							new_Mdns_Domain->additional++;
							}
						}
						else
						{
							if(Mdns_Staus[i].delflag != 0){
							if(check_address(new_Mdns_Domain,(udphd_old->len-8),new_data_point ) <0) return -10;
							if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
							memcpy(new_data_point ,old_data_point,record_len);
							new_Mdns_Domain->additional++;
							*(unsigned short *)(new_data_point + Mdns_Staus[i].point_offset) = Mdns_Staus[i].new_point ;
							new_total_data_len = new_total_data_len + record_len;
							new_data_point += record_len;
						}					
						}
						break;
					}
					else
					{
						if(Mdns_Staus[j].delflag ==0)
					 		Mdns_Staus[i].new_point  -=  Mdns_Staus[j].len;
						else if(Mdns_Staus[j].addnum !=0)
							Mdns_Staus[i].new_point  +=  Mdns_Staus[j].addnum;
						else{//do nothing
						}
					}
						
					}
				}else{
				if(Mdns_Staus[i].delflag != 0){
					if(check_address(new_Mdns_Domain,(udphd_old->len-8),new_data_point ) <0) return -10;
					if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
					memcpy(new_data_point,old_data_point,record_len);
					new_Mdns_Domain->additional++;
					new_total_data_len = new_total_data_len + record_len;
					new_data_point += record_len;
				}
				}
			}
			else
			{
				return -4;
			}
			
			Mdns_Staus[i].len = record_len;
			old_data_point += record_len;	
			if(check_address(data_point,(udphd_old->len-20),old_data_point) <0) return -10;
			Mdns_Staus[i+1].offset = Mdns_Staus[i].offset + record_len ;
			if(mdns_dbg)panic_printk("<3>""-----4--Buildpkg-addtion num=%d--offset=%x---record_len=%x--old_data_point=%x,delflag=%d--\n",i,Mdns_Staus[i].offset,record_len,(char *)old_data_point,Mdns_Staus[i].delflag );
		}

udphd_new->len= sizeof(struct udphdr) + sizeof(struct MdnsDomain) + new_total_data_len ;
new_skb->tail = (char *)udphd_new + udphd_new->len ;
Udp_CheckSum(new_skb);
return new_total_data_len ;	
}
*/
static int Mdns_Filter(struct sk_buff *skb,struct net_device *dev)
{
struct net_bridge *br = netdev_priv(dev);
const unsigned char *dest = skb_mac_header(skb);
unsigned char Mdns_pkg_protocol = 0;
unsigned char *Mdns_data_point;
struct MdnsDomain *Mdns_Domain;
unsigned int new_skb_data_len =0; // domain len
struct iphdr *iphd=NULL;
struct udphdr  *udphd=NULL;
static int debug_one =0;

		if(MULTICAST_MAC(dest) ||IPV6_MULTICAST_MAC(dest))
		{
			iphd=(struct iphdr *)skb_network_header(skb);	
			if(iphd!=NULL&&iphd ->version== 4){
				Mdns_pkg_protocol = iphd->protocol;			
				udphd=(struct udphdr*)skb_transport_header(skb);
				
			}
			else if(iphd!=NULL&&iphd ->version== 6){//ipv6
				Mdns_pkg_protocol = *(((char *)iphd) +6);
				udphd=(struct udphdr*)skb_transport_header(skb);
			}
			else{
				//do nothing ,reserve
			}
			
			if( Mdns_pkg_protocol ==IPPROTO_UDP &&  udphd!=NULL && udphd->dest==MdnsPort ) // here we find out the mdns pkg.
			{
				Mdns_data_point= ((unsigned char *)udphd) + sizeof(struct udphdr) ;
				struct MdnsDomain  *Mdns_Domain = (struct MdnsDomain *)Mdns_data_point;
				Mdns_data_point = Mdns_data_point + sizeof(struct MdnsDomain);
				if(Mdns_Domain->flag | 0x8000)// mdns response
				{
					if(mdns_dbg)panic_printk("<3>""--------3---we find mdns pkg---flags=%x---answer=%x--additional=%x---\n",Mdns_Domain->flag,Mdns_Domain->answer,Mdns_Domain->additional);
               			 struct sk_buff *skb_noshareport;
               			 if ((skb_noshareport = skb_copy(skb, GFP_ATOMIC)) == NULL) {
						LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
						panic_printk("<3>""---%s(%d) skb clone failed, drop it----\n", __FUNCTION__, __LINE__);
						kfree_skb(skb_noshareport);
                			 }
					else{
						//new_skb_data_len=Mdns_BuildNoShareportPkg(skb,Mdns_data_point,Mdns_Domain,skb_noshareport);
						new_skb_data_len=Mdns_FindSharePortPkg(skb,Mdns_data_point,Mdns_Domain,skb_noshareport);
						br_flood_to_dev(br,skb,dev_for_shareport_pkg);
					}
					if(mdns_dbg)panic_printk("<3>""--------3---buildpkg end-new_skb_data_len=%d--udphd->len=%d--\n",new_skb_data_len,udphd->len);
					if(skb_noshareport != NULL && new_skb_data_len>0 && (new_skb_data_len +8)<= udphd->len)
					{
						br_flood_to_dev(br, skb_noshareport,dev_for_NoShareport_pkg);
					}
					else{
						kfree_skb(skb_noshareport);
					}				
					return 1;
				}
			}
		}

return 0;
}

#endif


/* net device transmit always called with no BH (preempt_disabled) */
int br_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);
	const unsigned char *dest = skb->data;
	struct net_bridge_fdb_entry *dst;

#if defined (CONFIG_RTL_IGMP_SNOOPING)	
	struct iphdr *iph=NULL;
	unsigned char proto=0;
	unsigned char reserved=0;
#if defined (CONFIG_RTL_MLD_SNOOPING) 	
	struct ipv6hdr *ipv6h=NULL;
#endif
	struct rtl_multicastDataInfo multicastDataInfo;
	struct rtl_multicastFwdInfo multicastFwdInfo;
	int ret=FAILED;
#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
	unsigned int srcPort=skb->srcPort;
	unsigned int srcVlanId=skb->srcVlanId;
#endif
#endif	

#ifdef CONFIG_RTK_INBAND_HOST_HACK
// send all paket that from local with hostmac (after bridge mac learning)	
	//hex_dump(skb->data, 48);
	if(br_hackMac_enable){			
		if(memcmp(skb->data,inband_Hostmac,6)) //if destmac is not to host
			memcpy(skb->data+6, inband_Hostmac,6); //then modify source mac to hostmac
		else
			goto ap_hcm_out;
		// if it is arp then sender mac also need modify to hostmac
		if( (skb->data[12] == 0x08) && (skb->data[13] == 0x06) ) //0806 = ARP
		{
			memcpy(skb->data+ETHER_HDR_LEN+ARP_HRD_LEN, inband_Hostmac,6); //modify arp sender mac			
		}
		else if((skb->data[12] == 0x08) && (skb->data[13] == 0x00)) //IP
		{
			check_listen_info(skb);
		}
	}
ap_hcm_out:		
#endif

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	skb_reset_mac_header(skb);
	skb_pull(skb, ETH_HLEN);

	if (dest[0] & 1)
	{
//Alpha
#if  defined (CONFIG_BRIDGE_MDNSFILTER_BWCTRL)	// shareport filter
if(br->mdns_filter == 1){
if(Mdns_Filter(skb,dev))return 0;
}
/*
#define MdnsPort  5353
unsigned char Mdns_pkg_protocol = 0;
unsigned char *Mdns_data_point;
struct MdnsDomain *Mdns_Domain;
unsigned int new_skb_data_len =0; // domain len
struct iphdr *iphd=NULL;
struct udphdr  *udphd=NULL;
static int debug_one =0;
if(br->mdns_filter == 1){

		if(MULTICAST_MAC(dest) ||IPV6_MULTICAST_MAC(dest))
		{
			panic_printk("<3>""------------0.1-----------\n");
			iphd=(struct iphdr *)skb_network_header(skb);	
			if(iphd!=NULL&&iphd ->version== 4){
				Mdns_pkg_protocol = iphd->protocol;			
				udphd=(struct udphdr*)skb_transport_header(skb);
				
			}
			else if(iphd!=NULL&&iphd ->version== 6){//ipv6
				Mdns_pkg_protocol = *(((char *)iphd) +6);
				udphd=(struct udphdr*)skb_transport_header(skb);
			}
			else{
				//do nothing ,reserve
			}
			panic_printk("<3>""------------0.2-----------\n");
			if( Mdns_pkg_protocol ==IPPROTO_UDP &&  udphd!=NULL && udphd->dest==MdnsPort ) // here we find out the mdns pkg.
			{
				panic_printk("<3>""---------------------1----------protocol=%x----port=%x--------\n",Mdns_pkg_protocol, udphd->dest);
				Mdns_data_point= ((unsigned char *)udphd) + sizeof(struct udphdr) ;
				struct MdnsDomain  *Mdns_Domain = (struct MdnsDomain *)Mdns_data_point;
				Mdns_data_point = Mdns_data_point + sizeof(struct MdnsDomain);
				panic_printk("<3>""--------2---we find mdns pkg---flags=%x---answer=%x--additional=%x---\n",Mdns_Domain->flag,Mdns_Domain->answer,Mdns_Domain->additional);
				if(Mdns_Domain->flag | 0x8000)// mdns response
				{
				//debug_one ++;
				//if(debug_one % 2 !=0 ){kfree_skb(skb);return 0;}

				panic_printk("<3>""--------3---we find mdns pkg---flags=%x---answer=%x--additional=%x---\n",Mdns_Domain->flag,Mdns_Domain->answer,Mdns_Domain->additional);
               			 struct sk_buff *skb_noshareport;
               			 if ((skb_noshareport = skb_copy(skb, GFP_ATOMIC)) == NULL) {
						LOG_MEM_ERROR("%s(%d) skb clone failed, drop it\n", __FUNCTION__, __LINE__);
						panic_printk("<3>""---%s(%d) skb clone failed, drop it----\n", __FUNCTION__, __LINE__);
                			 }
					else
						br_flood_to_dev(br,skb,"wlan0");
						new_skb_data_len=Mdns_BuildNoShareportPkg(skb,Mdns_data_point,Mdns_Domain,skb_noshareport);
					panic_printk("<3>""--------3---buildpkg end-new_skb_data_len=%d--udphd->len=%d--\n",new_skb_data_len,udphd->len);
					if(skb_noshareport != NULL && new_skb_data_len>0 && (new_skb_data_len +8)<= udphd->len)
					{
						br_flood_to_dev(br, skb_noshareport,"eth1");
					}
					else{
						kfree_skb(skb_noshareport);
					}
					//panic_printk("<3>""------------4-----------\n");
					//if(skb_noshareport !=NULL)br_flood_to_dev(br, skb_noshareport,"eth1");
					
					panic_printk("<3>""-----------5-----------\n");
					return 0;
				}
				panic_printk("<3>""-----------6-----------\n");
				//br_flood_to_dev(br,skb,"wlan0");
				//kfree_skb(skb);
				//return 0;
				// br_flood_to_dev(br, skb,"eth1");
				//br_flood_deliver(br, skb);
			}
		}
}
*/
#endif
#if defined (CONFIG_RTL_IGMP_SNOOPING)	
		if(igmpsnoopenabled) 
		{	
			if(MULTICAST_MAC(dest))
			{
			
				iph=(struct iphdr *)skb_network_header(skb);
				proto =  iph->protocol;
				#if 0
				if(( iph->daddr&0xFFFFFF00)==0xE0000000)
				{
				        reserved=1;
				}
				#endif

				#if defined(CONFIG_USB_UWIFI_HOST)
				if(iph->daddr == 0xEFFFFFFA || iph->daddr == 0xE1010101)
				#else
				if(iph->daddr == 0xEFFFFFFA)
				#endif
				{
					/*for microsoft upnp*/
					reserved=1;
				}
				
				if(((proto ==IPPROTO_UDP) ||(proto ==IPPROTO_TCP))  && (reserved ==0))
				{
					multicastDataInfo.ipVersion=4;
					multicastDataInfo.sourceIp[0]=  (uint32)(iph->saddr);
					multicastDataInfo.groupAddr[0]=  (uint32)(iph->daddr);
					ret= rtl_getMulticastDataFwdInfo(brIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);
			
					br_multicast_deliver(br, multicastFwdInfo.fwdPortMask, skb, 0);
					if((ret==SUCCESS) && (multicastFwdInfo.cpuFlag==0))
					{
						#if defined (CONFIG_RTL_HARDWARE_MULTICAST)
						if((srcVlanId!=0) && (srcPort!=0xFFFF))
						{
							#if defined(CONFIG_RTK_VLAN_SUPPORT)
							if(rtk_vlan_support_enable == 0)
							{
								rtl865x_ipMulticastHardwareAccelerate(br, multicastFwdInfo.fwdPortMask,srcPort,srcVlanId, multicastDataInfo.sourceIp[0], multicastDataInfo.groupAddr[0]);
							}
							#else
							rtl865x_ipMulticastHardwareAccelerate(br, multicastFwdInfo.fwdPortMask,srcPort,srcVlanId, multicastDataInfo.sourceIp[0], multicastDataInfo.groupAddr[0]);
							#endif
						}
						#endif		
					}
				
				}
				else
				{
					br_flood_deliver(br, skb);
				}

				
			}
#if defined(CONFIG_RTL_MLD_SNOOPING)	
			else if(mldSnoopEnabled && IPV6_MULTICAST_MAC(dest))
			{
				ipv6h=(struct ipv6hdr *)skb_network_header(skb);
				proto=re865x_getIpv6TransportProtocol(ipv6h);
				if ((proto ==IPPROTO_UDP) ||(proto ==IPPROTO_TCP))
				{
					multicastDataInfo.ipVersion=6;
					memcpy(&multicastDataInfo.sourceIp, &ipv6h->saddr, sizeof(struct in6_addr));
					memcpy(&multicastDataInfo.groupAddr, &ipv6h->daddr, sizeof(struct in6_addr));
					ret= rtl_getMulticastDataFwdInfo(brIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);
					br_multicast_deliver(br, multicastFwdInfo.fwdPortMask, skb, 0);
				}
				else
				{
					br_flood_deliver(br, skb);
				}
			}
#endif		
			else
			{
				br_flood_deliver(br, skb);
			}
		
		}
		else
		{ 
			br_flood_deliver(br, skb);
		}	
#else
		br_flood_deliver(br, skb);
#endif
	}
	else if ((dst = __br_fdb_get(br, dest)) != NULL)
		br_deliver(dst->dst, skb);
	else
		br_flood_deliver(br, skb);

	return 0;
}

static int br_dev_open(struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);

	br_features_recompute(br);
	netif_start_queue(dev);
	br_stp_enable_bridge(br);

	return 0;
}

static void br_dev_set_multicast_list(struct net_device *dev)
{
}

static int br_dev_stop(struct net_device *dev)
{
	br_stp_disable_bridge(netdev_priv(dev));

	netif_stop_queue(dev);

	return 0;
}

static int br_change_mtu(struct net_device *dev, int new_mtu)
{
	struct net_bridge *br = netdev_priv(dev);
	if (new_mtu < 68 || new_mtu > br_min_mtu(br))
		return -EINVAL;

	dev->mtu = new_mtu;

#ifdef CONFIG_BRIDGE_NETFILTER
	/* remember the MTU in the rtable for PMTU */
	br->fake_rtable.u.dst.metrics[RTAX_MTU - 1] = new_mtu;
#endif

	return 0;
}

/* Allow setting mac address to any valid ethernet address. */
static int br_set_mac_address(struct net_device *dev, void *p)
{
	struct net_bridge *br = netdev_priv(dev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EINVAL;

	spin_lock_bh(&br->lock);
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
	br_stp_change_bridge_id(br, addr->sa_data);
	br->flags |= BR_SET_MAC_ADDR;
	spin_unlock_bh(&br->lock);

	return 0;
}

static void br_getinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "bridge");
	strcpy(info->version, BR_VERSION);
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "N/A");
}

static int br_set_sg(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_SG;
	else
		br->feature_mask &= ~NETIF_F_SG;

	br_features_recompute(br);
	return 0;
}

static int br_set_tso(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_TSO;
	else
		br->feature_mask &= ~NETIF_F_TSO;

	br_features_recompute(br);
	return 0;
}

static int br_set_tx_csum(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_NO_CSUM;
	else
		br->feature_mask &= ~NETIF_F_ALL_CSUM;

	br_features_recompute(br);
	return 0;
}

static const struct ethtool_ops br_ethtool_ops = {
	.get_drvinfo    = br_getinfo,
	.get_link	= ethtool_op_get_link,
	.get_tx_csum	= ethtool_op_get_tx_csum,
	.set_tx_csum 	= br_set_tx_csum,
	.get_sg		= ethtool_op_get_sg,
	.set_sg		= br_set_sg,
	.get_tso	= ethtool_op_get_tso,
	.set_tso	= br_set_tso,
	.get_ufo	= ethtool_op_get_ufo,
	.get_flags	= ethtool_op_get_flags,
};

static const struct net_device_ops br_netdev_ops = {
	.ndo_open		 = br_dev_open,
	.ndo_stop		 = br_dev_stop,
	.ndo_start_xmit		 = br_dev_xmit,
	.ndo_set_mac_address	 = br_set_mac_address,
	.ndo_set_multicast_list	 = br_dev_set_multicast_list,
	.ndo_change_mtu		 = br_change_mtu,
	.ndo_do_ioctl		 = br_dev_ioctl,
};

void br_dev_setup(struct net_device *dev)
{
	random_ether_addr(dev->dev_addr);
	ether_setup(dev);

	dev->netdev_ops = &br_netdev_ops;
	dev->destructor = free_netdev;
	SET_ETHTOOL_OPS(dev, &br_ethtool_ops);
	dev->tx_queue_len = 0;
	dev->priv_flags = IFF_EBRIDGE;
#if defined(CONFIG_HTTP_FILE_SERVER_SUPPORT) || defined(CONFIG_RTL_USB_UWIFI_HOST_SPEEDUP)
	dev->features = NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HIGHDMA |
			NETIF_F_GSO_MASK | NETIF_F_NO_CSUM | NETIF_F_LLTX |
			NETIF_F_NETNS_LOCAL | NETIF_F_GSO|NETIF_F_GRO|NETIF_F_LRO;

#elif defined(CONFIG_RTL_USB_IP_HOST_SPEEDUP)
        dev->features = NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HIGHDMA |
                        NETIF_F_GSO_MASK | NETIF_F_NO_CSUM | NETIF_F_LLTX |
                        NETIF_F_NETNS_LOCAL | NETIF_F_GSO|NETIF_F_GRO;
#else
	dev->features = NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HIGHDMA |
			NETIF_F_GSO_MASK | NETIF_F_NO_CSUM | NETIF_F_LLTX |
			NETIF_F_NETNS_LOCAL | NETIF_F_GSO;
#endif
}

#if defined (CONFIG_RTL_HARDWARE_MULTICAST)

int rtl865x_ipMulticastHardwareAccelerate(struct net_bridge *br, unsigned int brFwdPortMask,
												unsigned int srcPort,unsigned int srcVlanId, 
												unsigned int srcIpAddr, unsigned int destIpAddr)
{
	int ret;
	//int fwdDescCnt;
	//unsigned short port_bitmask=0;

	unsigned int tagged_portmask=0;


	struct rtl_multicastDataInfo multicastDataInfo;
	struct rtl_multicastFwdInfo  multicastFwdInfo;
	
	rtl865x_tblDrv_mCast_t * existMulticastEntry;
	rtl865x_mcast_fwd_descriptor_t  fwdDescriptor;

	#if 0
	printk("%s:%d,srcPort is %d,srcVlanId is %d,srcIpAddr is 0x%x,destIpAddr is 0x%x\n",__FUNCTION__,__LINE__,srcPort,srcVlanId,srcIpAddr,destIpAddr);
	#endif


#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)!=0 &&strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)!=0 )
	{
		return -1;
	}
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)==0 && (brFwdPortMask & br0SwFwdPortMask))
	{
		return -1;
	}	
	
	if(strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)==0 && (brFwdPortMask & br1SwFwdPortMask))
	{
		return -1;
	}
#else
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)!=0)
	{
		return -1;
	}

	if(brFwdPortMask & br0SwFwdPortMask)
	{
		return -1;
	}
#endif
	//printk("%s:%d,destIpAddr is 0x%x, srcIpAddr is 0x%x, srcVlanId is %d, srcPort is %d\n",__FUNCTION__,__LINE__,destIpAddr, srcIpAddr, srcVlanId, srcPort);
	existMulticastEntry=rtl865x_findMCastEntry(destIpAddr, srcIpAddr, (unsigned short)srcVlanId, (unsigned short)srcPort);
	if(existMulticastEntry!=NULL)
	{
		/*it's already in cache */
		return 0;

	}

	if(brFwdPortMask==0)
	{
		rtl865x_blockMulticastFlow(srcVlanId, srcPort, srcIpAddr, destIpAddr);
		return 0;
	}
	
	multicastDataInfo.ipVersion=4;
	multicastDataInfo.sourceIp[0]=  srcIpAddr;
	multicastDataInfo.groupAddr[0]=  destIpAddr;

	/*add hardware multicast entry*/

	#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	if(strcmp(br->dev->name,RTL_PS_BR0_DEV_NAME)==0)
	{
		memset(&fwdDescriptor, 0, sizeof(rtl865x_mcast_fwd_descriptor_t ));
		strcpy(fwdDescriptor.netifName,"eth*");
		fwdDescriptor.fwdPortMask=0xFFFFFFFF;
		ret= rtl_getMulticastDataFwdInfo(nicIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);
	}
	else if(strcmp(br->dev->name,RTL_PS_BR1_DEV_NAME)==0)
	{
		memset(&fwdDescriptor, 0, sizeof(rtl865x_mcast_fwd_descriptor_t ));
		strcpy(fwdDescriptor.netifName,"eth2");
		fwdDescriptor.fwdPortMask=0xFFFFFFFF;
		ret= rtl_getMulticastDataFwdInfo(nicIgmpModuleIndex_2, &multicastDataInfo, &multicastFwdInfo);
	}
	#else
	memset(&fwdDescriptor, 0, sizeof(rtl865x_mcast_fwd_descriptor_t ));
	strcpy(fwdDescriptor.netifName,"eth*");
	fwdDescriptor.fwdPortMask=0xFFFFFFFF;
	
	ret= rtl_getMulticastDataFwdInfo(nicIgmpModuleIndex, &multicastDataInfo, &multicastFwdInfo);
	#endif
	if(ret!=0)
	{
		return -1;
	}
	else
	{
		if(multicastFwdInfo.cpuFlag)
		{
			fwdDescriptor.toCpu=1;
		}
		fwdDescriptor.fwdPortMask=multicastFwdInfo.fwdPortMask & (~(1<<srcPort));
	}

#if defined(CONFIG_RTL_HW_VLAN_SUPPORT)
	if(rtl_hw_vlan_ignore_tagged_mc == 1)
		tagged_portmask = rtl_hw_vlan_get_tagged_portmask();
#endif
	if((fwdDescriptor.fwdPortMask & tagged_portmask) == 0)
	{

		ret=rtl865x_addMulticastEntry(destIpAddr, srcIpAddr, (unsigned short)srcVlanId, (unsigned short)srcPort,
							&fwdDescriptor, 1, 0, 0, 0);
	}

	return 0;
}

#endif
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
int rtl865x_same_root(struct net_device *dev1,struct net_device *dev2){

	struct net_bridge_port *p = rcu_dereference(dev1->br_port);
	struct net_bridge_port *p2 = rcu_dereference(dev2->br_port);
	return !strncmp(p->br->dev->name,p2->br->dev->name,3);
}
#endif
