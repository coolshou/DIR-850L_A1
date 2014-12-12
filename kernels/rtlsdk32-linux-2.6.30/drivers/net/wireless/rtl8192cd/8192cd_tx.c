/*
 *  TX handle routines
 *
 *  $Id: 8192cd_tx.c,v 1.39.2.31 2011/01/19 15:20:34 victoryman Exp $
 *
 *  Copyright (c) 2009 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_TX_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/netdevice.h>
#endif

#ifdef __DRAYTEK_OS__
#include <draytek/wl_dev.h>
#endif

#include "./8192cd_cfg.h"
#include "./8192cd.h"
#include "./8192cd_hw.h"
#include "./8192cd_headers.h"
#include "./8192cd_debug.h"

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#ifdef RTL8192CD_VARIABLE_USED_DMEM
#include "./8192cd_dmem.h"
#endif

#if defined(CONFIG_RTL_WAPI_SUPPORT)
#include "wapiCrypto.h"
#endif

#define AMSDU_TX_DESC_TH		2	// A-MSDU tx desc threshold, A-MSDU will be
									// triggered when more than this threshold packet in hw queue

#define RET_AGGRE_BYPASS		0
#define RET_AGGRE_ENQUE			1
#define RET_AGGRE_DESC_FULL		2

#define TX_NORMAL				0
#define TX_NO_MUL2UNI			1
#define TX_AMPDU_BUFFER_SIG		2
#define TX_AMPDU_BUFFER_FIRST	3
#define TX_AMPDU_BUFFER_MID		4
#define TX_AMPDU_BUFFER_LAST	5

#ifdef CONFIG_RTL_STP
extern unsigned char STPmac[6];
#endif

#ifdef RTL_MANUAL_EDCA
unsigned int PRI_TO_QNUM(struct rtl8192cd_priv *priv, int priority)
{
	if (priv->pmib->dot11QosEntry.ManualEDCA) {
		return priv->pmib->dot11QosEntry.TID_mapping[priority];
	}
	else {
		if ((priority == 0) || (priority == 3)) {
			if (!((OPMODE & WIFI_STATION_STATE) && GET_STA_AC_BE_PARA.ACM))
				return BE_QUEUE;
			else
				return BK_QUEUE;
		} else if ((priority == 7) || (priority == 6)) {
			if (!((OPMODE & WIFI_STATION_STATE) && GET_STA_AC_VO_PARA.ACM)) {
				return VO_QUEUE;
			} else {
				if (!GET_STA_AC_VI_PARA.ACM)
					return VI_QUEUE;
				else if (!GET_STA_AC_BE_PARA.ACM)
					return BE_QUEUE;
				else
					return BK_QUEUE;
			}
		} else if ((priority == 5) || (priority == 4)) {
			if (!((OPMODE & WIFI_STATION_STATE) && GET_STA_AC_VI_PARA.ACM)) {
				return VI_QUEUE;
			} else {
				if (!GET_STA_AC_BE_PARA.ACM) 
					return BE_QUEUE;
				else
					return BK_QUEUE;
			}
		} else {
			return BK_QUEUE;
		}
	}
}
#else
#define PRI_TO_QNUM(priority, q_num, wifi_specific) { \
		if ((priority == 0) || (priority == 3)) \
			q_num = BE_QUEUE; \
		else if ((priority == 7) || (priority == 6)) \
			q_num = VO_QUEUE; \
		else if ((priority == 5) || (priority == 4)) \
			q_num = VI_QUEUE; \
		else  \
			q_num = BK_QUEUE; \
}
#endif


#if !defined(CONFIG_RTL_8676HWNAT) && defined(CONFIG_RTL8672) && !defined(CONFIG_RTL8686)
extern int check_IGMP_report(struct sk_buff *skb);
extern int check_wlan_mcast_tx(struct sk_buff *skb);
#endif
// MBSSID Port Mapping
#ifdef CONFIG_RTL8672
extern struct port_map wlanDev[];
extern int g_port_mapping;
#endif


#ifdef CONFIG_RTL_WAPI_SUPPORT
extern void SecSWSMS4Encryption(struct rtl8192cd_priv  *priv, struct tx_insn* txcfg);
#endif

#ifdef STA_EXT
extern unsigned short MCS_DATA_RATE[2][2][16];

unsigned short BG_TABLE[2][21] = {{73,68,63,57,52,47,42,37,31,30,27,25,23,22,20,18,16,14,11,9,8},
				{108,108,108,108,108,108,108,108,108,108,108,108,72,72,48,48,36,12,11,11,4}};
unsigned short MCS_40M_TABLE[2][18] = {{73,68,63,57,53,45,39,37,31,29,28,26,25,23,21,19,17,12},
					{7,7,7,7,7,7,7,7,5,4,4,4,2,2,2,0,0,0}};
unsigned short MCS_20M_TABLE[2][22]={{73,68,65,60,55,50,45,40,31,32,27,26,24,22,21,19,18,16,14,11,9,8},
					{7,7,7,7,7,7,7,7,7,6,5,5,5,4,3,3,3,2,3,1,0,0}};

#define BG_TABLE_SIZE 21
#define MCS_40M_TABLE_SIZE 18
#define MCS_20M_TABLE_SIZE 22
#endif

static int tkip_mic_padding(struct rtl8192cd_priv *priv,
			unsigned char *da, unsigned char *sa, unsigned char priority,
			unsigned char *llc,struct sk_buff *pskb, struct tx_insn* txcfg);

static void wep_fill_iv(struct rtl8192cd_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid);

static void tkip_fill_encheader(struct rtl8192cd_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid_out);

__MIPS16
__IRAM_IN_865X
static void aes_fill_encheader(struct rtl8192cd_priv *priv,
			unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid);

static int rtl8192cd_tx_queueDsr(struct rtl8192cd_priv *priv, unsigned int txRingIdx);

static void rtl8192cd_tx_restartQueue(struct rtl8192cd_priv *priv);



unsigned int get_tx_rate(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
#ifdef STA_EXT
	if (priv->pmib->dot11StationConfigEntry.autoRate
		&& (pstat->remapped_aid < 
#ifdef CONFIG_RTL_88E_SUPPORT
		(GET_CHIP_VER(priv)==VERSION_8188E)?(RTL8188E_NUM_STAT-1):
#endif
#ifdef CONFIG_RTL_8812_SUPPORT
		(GET_CHIP_VER(priv)==VERSION_8812E)?(RTL8812_NUM_STAT - 1):
#endif // CONFIG_RTL_8812_SUPPORT
		(FW_NUM_STAT-1)/*!(priv->STA_map & BIT(pstat->aid)*/
	)) {

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
		if (CHIP_VER_92X_SERIES(priv) && (priv->pmib->dot11RFEntry.txbf == 1))
			check_txrate_by_reg(priv, pstat);
#endif		

		return pstat->current_tx_rate;
		
	} else {
	
		if (
#ifdef CONFIG_RTL_88E_SUPPORT
			(GET_CHIP_VER(priv)==VERSION_8188E)?(pstat->remapped_aid == RTL8188E_NUM_STAT-1):
#endif
#ifdef CONFIG_RTL_8812_SUPPORT
			(GET_CHIP_VER(priv)==VERSION_8812E)?(pstat->remapped_aid == RTL8812_NUM_STAT - 1):
#endif // CONFIG_RTL_8812_SUPPORT

			(pstat->remapped_aid == FW_NUM_STAT-1/*(priv->STA_map & BIT(pstat->aid)) != 0*/)) {
			// firmware does not keep the aid ...
			//use default rate instead
			if (pstat->ht_cap_len) {	// is N client
				if (pstat->tx_bw == HT_CHANNEL_WIDTH_20_40) {//(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))){ //40Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_40M_TABLE[1][MCS_40M_TABLE_SIZE-1] | 0x80;
					for (i=1; i < MCS_40M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_40M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_40M_TABLE[1][i-1] | 0x80;
							break;
						}
					}
					return pstat->current_tx_rate;
				} else { // 20Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_20M_TABLE[1][MCS_20M_TABLE_SIZE-1] | 0x80;
					for (i=1; i < MCS_20M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_20M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_20M_TABLE[1][i-1] | 0x80;
							break;
						}
					}
					return pstat->current_tx_rate;
				}
				return pstat->current_tx_rate;

			} 
#ifdef CONFIG_RTL_8812_SUPPORT
			else if (pstat->vht_cap_len) {	// is N client
				if (pstat->tx_bw == HT_CHANNEL_WIDTH_80) {//(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))){ //40Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_40M_TABLE[1][MCS_40M_TABLE_SIZE-1] | 0x90;
					for (i=1; i < MCS_40M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_40M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_40M_TABLE[1][i-1] | 0x90;
							break;
						}
					}
					return pstat->current_tx_rate;
				} else { // 20Mhz
					int i = 0;
					pstat->current_tx_rate = MCS_20M_TABLE[1][MCS_20M_TABLE_SIZE-1] | 0x90;
					for (i=1; i < MCS_20M_TABLE_SIZE; i++) {
						if (pstat->rssi > MCS_20M_TABLE[0][i]){
							pstat->current_tx_rate = MCS_20M_TABLE[1][i-1] | 0x90;
							break;
						}
					}
					return pstat->current_tx_rate;
				}
				return pstat->current_tx_rate;

			} 
#endif
			else { // is BG client
				int i = 0;
				pstat->current_tx_rate = BG_TABLE[1][BG_TABLE_SIZE-1] | 0x80;
				for (i = 0; i < BG_TABLE_SIZE; i++) {
					if (pstat->rssi > BG_TABLE[0][i]){
						pstat->current_tx_rate = BG_TABLE[1][i-1];
						break;
				    }
				}
				return pstat->current_tx_rate;
			}
		} else {
			return pstat->current_tx_rate;
		}
	}
#else 

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	if (CHIP_VER_92X_SERIES(priv) && (priv->pmib->dot11RFEntry.txbf == 1))
		check_txrate_by_reg(priv, pstat);
#endif
	return pstat->current_tx_rate;
	
#endif
}


unsigned int get_lowest_tx_rate(struct rtl8192cd_priv *priv, struct stat_info *pstat,
				unsigned int tx_rate)
{
	unsigned int lowest_tx_rate;

#ifdef WDS
	if (((pstat->state & WIFI_WDS) && (priv->pmib->dot11WdsInfo.entry[pstat->wds_idx].txRate == 0)) ||
		(!(pstat->state & WIFI_WDS) && (priv->pmib->dot11StationConfigEntry.autoRate)))
#else
	if (priv->pmib->dot11StationConfigEntry.autoRate)
#endif
	{
			lowest_tx_rate = find_rate(priv, pstat, 0, 0);
	}
	else
		lowest_tx_rate = tx_rate;

	return lowest_tx_rate;
}


__MIPS16
__IRAM_IN_865X
void assign_wlanseq(struct rtl8192cd_hw *phw, unsigned char *pframe, struct stat_info *pstat, struct wifi_mib *pmib
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
	, unsigned char is_11s
#endif
	)
{
#ifdef WIFI_WMM
	unsigned char qosControl[2];
	int tid;

	if (is_qos_data(pframe)) {
		memcpy(qosControl, GetQosControl(pframe), 2);
		tid = qosControl[0] & 0x07;

		if (pstat) {
			SetSeqNum(pframe, pstat->AC_seq[tid]);
			pstat->AC_seq[tid] = (pstat->AC_seq[tid] + 1) & 0xfff;
		}
		else {
//			SetSeqNum(pframe, phw->AC_seq[tid]);
//			phw->AC_seq[tid] = (phw->AC_seq[tid] + 1) & 0xfff;
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
			if (is_11s)
			{
				SetSeqNum(pframe, phw->seq);
				phw->seq = (phw->seq + 1) & 0xfff;
			}
			else
#endif
				printk("Invalid seq num setting for Multicast or Broadcast pkt!!\n");
		}

		{
			if ((tid == 7) || (tid == 6))
				phw->VO_pkt_count++;
			else if ((tid == 5) || (tid == 4))
				phw->VI_pkt_count++;
			else if ((tid == 2) || (tid == 1))
				phw->BK_pkt_count++;
#ifdef WMM_VIBE_PRI
			else
				phw->BE_pkt_count++;
#endif
		}
	}
	else
#endif
	{
		SetSeqNum(pframe, phw->seq);
		phw->seq = (phw->seq + 1) & 0xfff;
	}
}


#ifdef CONFIG_RTK_MESH
static unsigned int get_skb_priority3(struct rtl8192cd_priv *priv, struct sk_buff *skb, int is_11s)
#else
__MIPS16 __IRAM_IN_865X static unsigned int get_skb_priority(struct rtl8192cd_priv *priv, struct sk_buff *skb)
#endif
{
	unsigned int pri=0, parsing=0;
	unsigned char protocol[2];

#ifdef WIFI_WMM
	if (QOS_ENABLE)
		parsing = 1;
#endif

	if (parsing) {
#ifdef CONFIG_RTK_VLAN_SUPPORT
		if (skb->cb[0])
			pri =  skb->cb[0];
		else
#endif
		{
			protocol[0] = skb->data[12];
			protocol[1] = skb->data[13];

			if ((protocol[0] == 0x08) && (protocol[1] == 0x00))
			{
#ifdef CONFIG_RTK_MESH
				if(is_11s & RELAY_11S)
				{
					if(skb->data[14]&1) // 6 addr
						pri = (skb->data[31] & 0xe0) >> 5;
					else
						pri = (skb->data[19] & 0xe0) >> 5;
				}
				else
#endif
				{
					pri = (skb->data[15] & 0xe0) >> 5;
				}
			}
			else if ((skb->cb[0]>0) && (skb->cb[0]<8))	// Ethernet driver will parse priority and put in cb[0]
				pri = skb->cb[0];
			else
				pri = 0;
		}

#ifdef CLIENT_MODE
		if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE)) {
			if (GET_STA_AC_VO_PARA.ACM) {
				if (!GET_STA_AC_VI_PARA.ACM) 
					pri = 5;
				else if (!GET_STA_AC_BE_PARA.ACM)
					pri = 0;
				else
					pri = 1;
			} else if (GET_STA_AC_VI_PARA.ACM) {
				if (!GET_STA_AC_BE_PARA.ACM)
					pri = 0;
				else
					pri = 1;
			} else if (GET_STA_AC_BE_PARA.ACM) {
				pri = 1;	// DSCP_BK tag = 1;
			}
		}
#endif	
		skb->cb[1] = pri;

		return pri;
	}
	else {
		// default is no priority
		skb->cb[1] = 0;
		return 0;
	}
}


#ifdef CONFIG_RTK_MESH
#define get_skb_priority(priv, skb)  get_skb_priority3(priv, skb, 0)
#endif


unsigned int isDHCPpkt(struct sk_buff *pskb)
{
#define DHCP_MAGIC 0x63825363

	struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	        __u8    ihl:4,
	                version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
	        __u8    version:4,
	                ihl:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	        __u8    tos;
	        __u16   tot_len;
	        __u16   id;
	        __u16   frag_off;
	        __u8    ttl;
	        __u8    protocol;
#if 0
	        __u16   check;
	        __u32   saddr;
	        __u32   daddr;
#endif
	};

	struct udphdr {
	        __u16   source;
	        __u16   dest;
	        __u16   len;
	        __u16   check;
	};

	struct dhcpMessage {
		u_int8_t op;
		u_int8_t htype;
		u_int8_t hlen;
		u_int8_t hops;
		u_int32_t xid;
		u_int16_t secs;
		u_int16_t flags;
		u_int32_t ciaddr;
		u_int32_t yiaddr;
		u_int32_t siaddr;
		u_int32_t giaddr;
		u_int8_t chaddr[16];
		u_int8_t sname[64];
		u_int8_t file[128];
		u_int32_t cookie;
#if 0
		u_int8_t options[308]; /* 312 - cookie */
#endif
	};

	unsigned short protocol = 0;
	struct iphdr* iph;
	struct udphdr *udph;
	struct dhcpMessage *dhcph;

	protocol = *((unsigned short *)(pskb->data + 2 * ETH_ALEN));

	if(protocol == __constant_htons(ETH_P_IP)) { /* IP */
		iph = (struct iphdr *)(pskb->data + ETH_HLEN);

		if(iph->protocol == 17) { /* UDP */
			udph = (struct udphdr *)((unsigned int)iph + (iph->ihl << 2));
			dhcph = (struct dhcpMessage *)((unsigned int)udph + sizeof(struct udphdr));

			if ((unsigned long)dhcph & 0x03) { //not 4-byte alignment
				u_int32_t cookie;
				char *pdhcphcookie;
				char *pcookie = (char *)&cookie;

				pdhcphcookie = (char *)&dhcph->cookie;
				pcookie[0] = pdhcphcookie[0];
				pcookie[1] = pdhcphcookie[1];
				pcookie[2] = pdhcphcookie[2];
				pcookie[3] = pdhcphcookie[3];
				if(cookie == htonl(DHCP_MAGIC))
					return TRUE;
			}
			else {
				if(dhcph->cookie == htonl(DHCP_MAGIC))
					return TRUE;
			}
		}
	}

	return FALSE;
}


static int dz_queue(struct rtl8192cd_priv *priv, struct stat_info *pstat, struct sk_buff *pskb)
{
	unsigned int ret;

	if (pstat)
	{
		if(0 == pstat->expire_to)
			return FALSE;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		if ((QOS_ENABLE) && (APSD_ENABLE) && (pstat->QosEnabled) && (pstat->apsd_bitmap & 0x0f)) {
			int pri = 0;

			pri = get_skb_priority(priv, pskb);

			if (((pri == 7) || (pri == 6)) && (pstat->apsd_bitmap & 0x01)) {
				ret = enque(priv, &(pstat->VO_dz_queue->head), &(pstat->VO_dz_queue->tail),
					(unsigned int)(pstat->VO_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if (ret)
					DEBUG_INFO("enque VO pkt\n");
			}
			else if (((pri == 5) || (pri == 4)) && (pstat->apsd_bitmap & 0x02)) {
				ret = enque(priv, &(pstat->VI_dz_queue->head), &(pstat->VI_dz_queue->tail),
					(unsigned int)(pstat->VI_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if (ret)
					DEBUG_INFO("enque VI pkt\n");
			}
			else if (((pri == 0) || (pri == 3)) && (pstat->apsd_bitmap & 0x08)) {
				ret = enque(priv, &(pstat->BE_dz_queue->head), &(pstat->BE_dz_queue->tail),
					(unsigned int)(pstat->BE_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if (ret)
					DEBUG_INFO("enque BE pkt\n");
			}
			else if (pstat->apsd_bitmap & 0x04) {
				ret = enque(priv, &(pstat->BK_dz_queue->head), &(pstat->BK_dz_queue->tail),
					(unsigned int)(pstat->BK_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE, (void *)pskb);
				if (ret)
					DEBUG_INFO("enque BK pkt\n");
			}
			else
				goto legacy_ps;

			if (!pstat->apsd_pkt_buffering)
				pstat->apsd_pkt_buffering = 1;

			if (ret == FALSE) {
				DEBUG_ERR("sleep Q full for priority = %d!\n", pri);
				return CONGESTED;
			}
			return TRUE;
		}
		else
legacy_ps:
#endif
		if (pstat->dz_queue.qlen<NUM_TXPKT_QUEUE){
			skb_queue_tail(&pstat->dz_queue, pskb);
			return TRUE;
		} else {
			return FALSE;
		}
	}
	else {	// Multicast or Broadcast
		ret = enque(priv, &(priv->dz_queue.head), &(priv->dz_queue.tail),
			(unsigned int)(priv->dz_queue.pSkb), NUM_TXPKT_QUEUE, (void *)pskb);
		if (ret == TRUE) {
		        if (!priv->pkt_in_dtimQ)
			        priv->pkt_in_dtimQ = 1;
		        return TRUE;
                }
	}

	return FALSE;
}


/*        Function to process different situations in TX flow             */
/* ====================================================================== */
#define TX_PROCEDURE_CTRL_STOP			0
#define TX_PROCEDURE_CTRL_CONTINUE		1
#define TX_PROCEDURE_CTRL_SUCCESS		2

#ifdef WDS
static int rtl8192cd_tx_wdsDevProc(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct net_device **dev_p,
				struct net_device **wdsDev_p, struct tx_insn *txcfg)
{
	struct stat_info *pstat;

	txcfg->wdsIdx = getWdsIdxByDev(priv, *dev_p);
	if (txcfg->wdsIdx < 0) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: getWdsIdxByDev() fail!\n");
		goto free_and_stop;
	}

	if (!netif_running(priv->dev)) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Can't send WDS packet due to wlan interface is down!\n");
		goto free_and_stop;
	}
	pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
	if (pstat && pstat->current_tx_rate==0) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Can't send packet due to tx rate is not supported in peer WDS AP!\n");
		goto free_and_stop;
	}
	*wdsDev_p = *dev_p;
	*dev_p = priv->dev;

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

free_and_stop:		/* Free current packet and stop TX process */

	dev_kfree_skb_any(skb);

	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}
#endif


#ifdef CLIENT_MODE
static int rtl8192cd_tx_clientMode(struct rtl8192cd_priv *priv, struct sk_buff **pskb)
{
	struct sk_buff *skb=*pskb;
	int DontEnterNat25=0;

	{
#ifdef RTK_BR_EXT
		int res, is_vlan_tag=0, i, do_nat25=1;
		unsigned short vlan_hdr=0;
		int lltd_flag=0;

		if (!priv->pmib->wscEntry.wsc_enable)
			mac_clone_handle_frame(priv, skb);
		if(priv->pmib->ethBrExtInfo.macclone_enable && priv->macclone_completed){
			if(!memcmp(skb->data+ETH_ALEN, GET_MY_HWADDR, ETH_ALEN))	{
				DontEnterNat25=1;
			}
		}
		if (!priv->pmib->ethBrExtInfo.nat25_disable && DontEnterNat25==0) 
		{
			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_8021Q)) {
				is_vlan_tag = 1;
				vlan_hdr = *((unsigned short *)(skb->data+MACADDRLEN*2+2));
				for (i=0; i<6; i++)
					*((unsigned short *)(skb->data+MACADDRLEN*2+2-i*2)) = *((unsigned short *)(skb->data+MACADDRLEN*2-2-i*2));
				skb_pull(skb, 4);
			}

			if (!memcmp(skb->data+MACADDRLEN, priv->br_mac, MACADDRLEN) &&
				(*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP)))
				memcpy(priv->br_ip, skb->data+WLAN_ETHHDR_LEN+12, 4);

			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP)) {
				if (memcmp(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN)) {
					if ((priv->scdb_entry = (struct nat25_network_db_entry *)scdb_findEntry(priv,
								skb->data+MACADDRLEN, skb->data+WLAN_ETHHDR_LEN+12)) != NULL) {
						memcpy(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN);
						memcpy(priv->scdb_ip, skb->data+WLAN_ETHHDR_LEN+12, 4);
						priv->scdb_entry->ageing_timer = jiffies;
						do_nat25 = 0;
					}
				}
				else {
					if (priv->scdb_entry) {
						priv->scdb_entry->ageing_timer = jiffies;
						do_nat25 = 0;
					}
					else {
						memset(priv->scdb_mac, 0, MACADDRLEN);
						memset(priv->scdb_ip, 0, 4);
					}
				}
			}

			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(0x88d9)) {
				if(skb->data[0] & 0x1)
				{
					do_nat25=0;
					lltd_flag=1;
				}
			}

			if (do_nat25)
			{
				if (nat25_db_handle(priv, skb, NAT25_CHECK) == 0) {
					struct sk_buff *newskb;

					if (is_vlan_tag) {
						skb_push(skb, 4);
						for (i=0; i<6; i++)
							*((unsigned short *)(skb->data+i*2)) = *((unsigned short *)(skb->data+4+i*2));
						*((unsigned short *)(skb->data+MACADDRLEN*2)) = __constant_htons(ETH_P_8021Q);
						*((unsigned short *)(skb->data+MACADDRLEN*2+2)) = vlan_hdr;
					}

					newskb = skb_copy(skb, GFP_ATOMIC);
					dev_kfree_skb_any(skb);
					if (newskb == NULL) {
						priv->ext_stats.tx_drops++;
						DEBUG_ERR("TX DROP: skb_copy fail!\n");
						goto stop_proc;
					}
					*pskb = skb = newskb;
					if (is_vlan_tag) {
						vlan_hdr = *((unsigned short *)(skb->data+MACADDRLEN*2+2));
						for (i=0; i<6; i++)
							*((unsigned short *)(skb->data+MACADDRLEN*2+2-i*2)) = *((unsigned short *)(skb->data+MACADDRLEN*2-2-i*2));
						skb_pull(skb, 4);
					}
				}

				res = nat25_db_handle(priv, skb, NAT25_INSERT);
				if (res < 0) {
					if (res == -2) {
						priv->ext_stats.tx_drops++;
						DEBUG_ERR("TX DROP: nat25_db_handle fail!\n");
						goto free_and_stop;
					}
					// we just print warning message and let it go
					DEBUG_WARN("nat25_db_handle INSERT fail!\n");
				}
			}

			if(lltd_flag != 1)
			{
				memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);
			}

			if (is_vlan_tag) {
				skb_push(skb, 4);
				for (i=0; i<6; i++)
					*((unsigned short *)(skb->data+i*2)) = *((unsigned short *)(skb->data+4+i*2));
				*((unsigned short *)(skb->data+MACADDRLEN*2)) = __constant_htons(ETH_P_8021Q);
				*((unsigned short *)(skb->data+MACADDRLEN*2+2)) = vlan_hdr;
			}
		}
		else{
			if (*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_8021Q)) {
				is_vlan_tag = 1;
			}

			if(is_vlan_tag){
				if(ICMPV6_MCAST_MAC(skb->data) && ICMPV6_PROTO1A_VALN(skb->data)){
					memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);
				}
			}else
			{
				if(ICMPV6_MCAST_MAC(skb->data) && ICMPV6_PROTO1A(skb->data)){
					memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);
				}
			}
		}




		// check if SA is equal to our MAC
		if (memcmp(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN)) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: untransformed frame SA:%02X%02X%02X%02X%02X%02X!\n",
				skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]);
			goto free_and_stop;
		}
#endif // RTK_BR_EXT
	}

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

#ifdef RTK_BR_EXT
free_and_stop:		/* Free current packet and stop TX process */

	dev_kfree_skb_any(skb);

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
#endif

}
#endif // CLIENT_MODE


#ifdef GBWC
static int rtl8192cd_tx_gbwc(struct rtl8192cd_priv *priv, struct stat_info	*pstat, struct sk_buff *skb)
{
	if (((priv->pmib->gbwcEntry.GBWCMode == GBWC_MODE_LIMIT_MAC_INNER) && (pstat->GBWC_in_group)) ||
		((priv->pmib->gbwcEntry.GBWCMode == GBWC_MODE_LIMIT_MAC_OUTTER) && !(pstat->GBWC_in_group)) ||
		(priv->pmib->gbwcEntry.GBWCMode == GBWC_MODE_LIMIT_IF_TX) ||
		(priv->pmib->gbwcEntry.GBWCMode == GBWC_MODE_LIMIT_IF_TRX)) {
		if ((priv->GBWC_tx_count + skb->len) > ((priv->pmib->gbwcEntry.GBWCThrd_tx * 1024 / 8) / (100 / GBWC_TO))) {
			// over the bandwidth
			if (priv->GBWC_consuming_Q) {
				// in rtl8192cd_GBWC_timer context
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: BWC bandwidth over!\n");
				rtl_kfree_skb(priv, skb, _SKB_TX_);
			}
			else {
				// normal Tx path
				int ret = enque(priv, &(priv->GBWC_tx_queue.head), &(priv->GBWC_tx_queue.tail),
						(unsigned int)(priv->GBWC_tx_queue.pSkb), NUM_TXPKT_QUEUE, (void *)skb);
				if (ret == FALSE) {
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: BWC tx queue full!\n");
					rtl_kfree_skb(priv, skb, _SKB_TX_);
				}
			}
			goto stop_proc;
		}
		else {
			// not over the bandwidth
			if (CIRC_CNT(priv->GBWC_tx_queue.head, priv->GBWC_tx_queue.tail, NUM_TXPKT_QUEUE) &&
					!priv->GBWC_consuming_Q) {
				// there are already packets in queue, put in queue too for order
				int ret = enque(priv, &(priv->GBWC_tx_queue.head), &(priv->GBWC_tx_queue.tail),
						(unsigned int)(priv->GBWC_tx_queue.pSkb), NUM_TXPKT_QUEUE, (void *)skb);
				if (ret == FALSE) {
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: BWC tx queue full!\n");
					rtl_kfree_skb(priv, skb, _SKB_TX_);
				}
				goto stop_proc;
			}
			else {
				// can transmit directly
				priv->GBWC_tx_count += skb->len;
			}
		}
	}

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}
#endif


#ifdef TX_SHORTCUT
static int rtl8192cd_tx_tkip(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct stat_info*pstat, struct tx_insn *txcfg)
{
	struct wlan_ethhdr_t *pethhdr;
	struct llc_snap	*pllc_snap;

	pethhdr = (struct wlan_ethhdr_t *)(skb->data - WLAN_ETHHDR_LEN);

#ifdef MCAST2UI_REFINE
        memcpy(pethhdr->daddr, &skb->cb[10], 6);
#endif

	pllc_snap = (struct llc_snap *)((UINT8 *)(txcfg->phdr) + txcfg->hdr_len + txcfg->iv);

#ifdef WIFI_WMM
	if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, ((QOS_ENABLE) && (pstat) && (pstat->QosEnabled))?skb->cb[1]:0, (UINT8 *)pllc_snap,
				skb, txcfg)) == FALSE)
#else
	if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, 0, (UINT8 *)pllc_snap,
				skb, txcfg)) == FALSE)
#endif
	{
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: Tkip mic padding fail!\n");
		rtl_kfree_skb(priv, skb, _SKB_TX_);
		release_wlanllchdr_to_poll(priv, txcfg->phdr);
		goto stop_proc;
	}
	skb_put((struct sk_buff *)txcfg->pframe, 8);
	txcfg->fr_len += 8;	// for Michael padding.

	/* Reply caller function : Continue process */
	return TX_PROCEDURE_CTRL_CONTINUE;

stop_proc:
	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}


__MIPS16
__IRAM_IN_865X
int get_tx_sc_index(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *hdr)
{
	int i;

	for (i=0; i<TX_SC_ENTRY_NUM; i++) {
#ifdef MCAST2UI_REFINE          
		if  ((OPMODE & WIFI_AP_STATE)
#ifdef WDS			
				&& !(pstat->state & WIFI_WDS)
#endif				
			) {		
			if (!memcmp(hdr+6, &pstat->tx_sc_ent[i].ethhdr.saddr, sizeof(struct wlan_ethhdr_t)-6)) 
				return i;							
		}
		else				
#endif
		{
			unsigned char *ptr;

			ptr = (unsigned char *)&pstat->tx_sc_ent[i].ethhdr;
			if (!memcmp(hdr, &pstat->tx_sc_ent[i].ethhdr, sizeof(struct wlan_ethhdr_t)))
				return i;
		}
	}

	return -1;
}


#ifdef CONFIG_RTL8672
__MIPS16
__IRAM_IN_865X
#endif
int get_tx_sc_free_entry(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *hdr)
{
	int i;

	i = get_tx_sc_index(priv, pstat, hdr);
	if (i >= 0)
		return i;
	else {
		for (i=0; i<TX_SC_ENTRY_NUM; i++) {
			if (pstat->tx_sc_ent[i].txcfg.fr_len == 0)
				return i;
		}
	}
	// no free entry
	i = pstat->tx_sc_replace_idx;
	pstat->tx_sc_replace_idx = (++pstat->tx_sc_replace_idx) % TX_SC_ENTRY_NUM;
	return i;
}
#endif


/*
	Always STOP process after calling this Procedure.
*/
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static void rtl8192cd_tx_xmitSkbFail(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct net_device *dev,
				struct net_device *wdsDev, struct tx_insn *txcfg)
{
/*
 * Comment-out the following lines to disable flow-control in any case to fix
 * WDS interface may be blocked sometimes during root interface is up/down
 * continously.
 */
#if 0
#ifdef WDS
	if (wdsDev)
		netif_stop_queue(wdsDev);
	else
#endif
	{
#ifdef WIFI_WMM
		if (!QOS_ENABLE)
#endif
			netif_stop_queue(dev);
	}
#endif

	priv->ext_stats.tx_drops++;
	DEBUG_WARN("TX DROP: Congested!\n");
	if (txcfg->phdr)
		release_wlanllchdr_to_poll(priv, txcfg->phdr);
	if (skb)
		rtl_kfree_skb(priv, skb, _SKB_TX_);

	return;
}


static int rtl8192cd_tx_slowPath(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct stat_info *pstat,
				struct net_device *dev, struct net_device *wdsDev, struct tx_insn *txcfg)
{

#ifdef CONFIG_RTL_WAPI_SUPPORT
	if ((pstat && pstat->wapiInfo
		&& (pstat->wapiInfo->wapiType!=wapiDisable)
		&& skb->protocol!=ETH_P_WAPI
		&& (!pstat->wapiInfo->wapiUCastTxEnable)))
	{
		{
			rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
			goto stop_proc;
		}
	}
#endif

	if (IEEE8021X_FUN && pstat && (pstat->ieee8021x_ctrlport == DOT11_PortStatus_Unauthorized) &&
#ifdef WDS
		(! (pstat->state & WIFI_WDS)) &&
#endif		
		(cpu_to_be16(*(unsigned short *)(skb->data + 12)) != 0x888e)&&(!(OPMODE & WIFI_ADHOC_STATE))) {
		DEBUG_ERR("TX DROP: control port not authorized!\n");
		rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
		goto stop_proc;
	}

	if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
		txcfg->q_num = BE_QUEUE;	// using low queue for data queue
		skb->cb[1] = 0;
	}
	txcfg->fr_type = _SKB_FRAME_TYPE_;
	txcfg->pframe = skb;

	if ((txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
		txcfg->phdr = NULL;
	else
	{
		txcfg->phdr = (UINT8 *)get_wlanllchdr_from_poll(priv);

		if (txcfg->phdr == NULL) {
			DEBUG_ERR("Can't alloc wlan header!\n");
			rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
			goto stop_proc;
		}

		if (skb->len > priv->pmib->dot11OperationEntry.dot11RTSThreshold)
			txcfg->retry = priv->pmib->dot11OperationEntry.dot11LongRetryLimit;
		else
			txcfg->retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

		memset((void *)txcfg->phdr, 0, sizeof(struct wlanllc_hdr));

#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
		{
			SetFrameSubType(txcfg->phdr, WIFI_11S_MESH);
			SetToDs(txcfg->phdr);
		}
		else
#endif
#ifdef WIFI_WMM
		if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled))
			SetFrameSubType(txcfg->phdr, WIFI_QOS_DATA);
		else
#endif
		SetFrameType(txcfg->phdr, WIFI_DATA);

		if (OPMODE & WIFI_AP_STATE) {
			SetFrDs(txcfg->phdr);
#ifdef WDS
			if (wdsDev)
				SetToDs(txcfg->phdr);
#endif
#ifdef A4_STA
			if (pstat && (pstat->state & WIFI_A4_STA)) 
				SetToDs(txcfg->phdr);			
#endif
		}
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE)
			SetToDs(txcfg->phdr);
		else if (OPMODE & WIFI_ADHOC_STATE)
			/* toDS=0, frDS=0 */;
#endif
		else
			DEBUG_WARN("non supported mode yet!\n");
	}

#ifdef USE_TXQUEUE
	if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable)
		rtl8192cd_tx_dsr((unsigned long)priv);
#endif

	if (rtl8192cd_wlantx(priv, txcfg) == CONGESTED)
	{
		rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
		goto stop_proc;
	}

	/* Reply caller function : process done successfully */
	return TX_PROCEDURE_CTRL_SUCCESS;

stop_proc:

	/* Reply caller function : STOP process */
	return TX_PROCEDURE_CTRL_STOP;
}

__MIPS16
__IRAM_IN_865X
int __rtl8192cd_start_xmit(struct sk_buff*skb, struct net_device *dev, int tx_fg);

__IRAM_IN_865X
int rtl8192cd_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
#ifdef NETDEV_NO_PRIV
	struct rtl8192cd_priv *priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;
#endif
	unsigned long x;
	int ret;

	SAVE_INT_AND_CLI(x);
	SMP_LOCK_XMIT(x);

#ifdef MCAST2UI_REFINE
	memcpy(&skb->cb[10], skb->data, 6);
#endif

	ret = __rtl8192cd_start_xmit(skb, dev, TX_NORMAL);

	RESTORE_INT(x);
	SMP_UNLOCK_XMIT(x);
	return ret;
}


#ifdef SUPPORT_TX_MCAST2UNI

__IRAM_IN_865X
int rtl8192cd_start_xmit_noM2U(struct sk_buff *skb, struct net_device *dev)
{
#ifdef NETDEV_NO_PRIV
	struct rtl8192cd_priv *priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;
#endif
	unsigned long x;
	int ret;

	SAVE_INT_AND_CLI(x);
	SMP_LOCK_XMIT(x);

	ret = __rtl8192cd_start_xmit(skb, dev, TX_NO_MUL2UNI);

	RESTORE_INT(x);
	SMP_UNLOCK_XMIT(x);
	return ret;
}

#endif

#ifdef SUPPORT_TX_AMSDU
static int amsdu_xmit(struct rtl8192cd_priv *priv, struct stat_info *pstat, struct tx_insn *txcfg, int tid,
				int from_isr, struct net_device *wdsDev, struct net_device *dev)
{
	int q_num, max_size, is_first=1, total_len=0, total_num=0;
	struct sk_buff *pskb;
	unsigned long	flags;
	int *tx_head, *tx_tail, space=0;
	struct rtl8192cd_hw	*phw = GET_HW(priv);

	txcfg->pstat = pstat;
	q_num = txcfg->q_num;

	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);

	max_size = pstat->amsdu_level;

	// start to transmit queued skb
	SAVE_INT_AND_CLI(flags);
	while (skb_queue_len(&pstat->amsdu_tx_que[tid]) > 0) {
		pskb = __skb_dequeue(&pstat->amsdu_tx_que[tid]);
		if (pskb == NULL)
			break;
		total_len += (pskb->len + sizeof(struct llc_snap) + 3);

		if (is_first) {
			if (skb_queue_len(&pstat->amsdu_tx_que[tid]) > 0) {
				space = CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC);
				if (space < 10) {
#ifdef SMP_SYNC
					if (!priv->pshare->has_triggered_tx_tasklet) {
						tasklet_schedule(&priv->pshare->tx_tasklet);
						priv->pshare->has_triggered_tx_tasklet = 1;
					}
#else
					rtl8192cd_tx_dsr((unsigned long)priv);
#endif
				
					space = CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC);
					if (space < 10) {
						// printk("Tx desc not enough for A-MSDU!\n");
						__skb_queue_head(&pstat->amsdu_tx_que[tid], pskb);
						RESTORE_INT(flags);
						return 0;
					}
				}
				txcfg->aggre_en = FG_AGGRE_MSDU_FIRST;
				is_first = 0;
				total_num++;
			}
			else {
				if (!from_isr) {
					__skb_queue_head(&pstat->amsdu_tx_que[tid], pskb);
					RESTORE_INT(flags);
					return 0;
				}
				txcfg->aggre_en = 0;
			}
		}
		else if ((skb_queue_len(&pstat->amsdu_tx_que[tid]) == 0) ||
				((total_len + pstat->amsdu_tx_que[tid].next->len + sizeof(struct llc_snap) + 3) > max_size) ||
				(total_num >= (space - 4)) || // 1 for header, 1 for ICV when sw encrypt, 2 for spare
				(!pstat->is_realtek_sta && (total_num >= (priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum-1)))) {
			txcfg->aggre_en = FG_AGGRE_MSDU_LAST;
			total_len = 0;
			is_first = 1;
			total_num = 0;
		}
		else {
			txcfg->aggre_en = FG_AGGRE_MSDU_MIDDLE;
			total_num++;
		}

		pstat->amsdu_size[tid] -= (pskb->len + sizeof(struct llc_snap));
#ifdef MESH_AMSDU
		txcfg->llc = 0;
		if( isMeshPoint(pstat))
		{
			txcfg->is_11s = 8;
			dev = priv->mesh_dev;
			memcpy(txcfg->nhop_11s, pstat->hwaddr, MACADDRLEN);
		}
		else
			txcfg->is_11s = 0;

#endif
		rtl8192cd_tx_slowPath(priv, pskb, pstat, dev, wdsDev, txcfg);

	}
	RESTORE_INT(flags);

	return 1;
}


static int amsdu_timer_add(struct rtl8192cd_priv *priv, struct stat_info *pstat, int tid, int from_timeout)
{
	unsigned int now, timeout, new_timer=0;
	int setup_timer;
	int current_idx, next_idx;

	current_idx = priv->pshare->amsdu_timer_head;

	while (CIRC_CNT(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM)) {
		if (priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat == NULL) {
			priv->pshare->amsdu_timer_tail = (priv->pshare->amsdu_timer_tail + 1) & (AMSDU_TIMER_NUM - 1);
			new_timer = 1;
		}
		else
			break;
	}

	if (CIRC_CNT(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM) == 0) {
		cancel_timer2(priv);
		setup_timer = 1;
	}
	else if (CIRC_SPACE(current_idx, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM) == 0) {
		printk("%s: %s, amsdu timer overflow!\n", priv->dev->name, __FUNCTION__ );
		return -1;
	}
	else {	// some items in timer queue
		setup_timer = 0;
		if (new_timer)
			new_timer = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout;
	}

	next_idx = (current_idx + 1) & (AMSDU_TIMER_NUM - 1);

	priv->pshare->amsdu_timer[current_idx].priv = priv;
	priv->pshare->amsdu_timer[current_idx].pstat = pstat;
	priv->pshare->amsdu_timer[current_idx].tid = (unsigned char)tid;
	priv->pshare->amsdu_timer_head = next_idx;
	now = RTL_R32(TSFTR);
	timeout = now + priv->pmib->dot11nConfigEntry.dot11nAMSDUSendTimeout;
	priv->pshare->amsdu_timer[current_idx].timeout = timeout;

	if (!from_timeout) {
		if (setup_timer)
			setup_timer2(priv, timeout);
		else if (new_timer) {
			if (TSF_LESS(new_timer, now))
				setup_timer2(priv, timeout);
			else
				setup_timer2(priv, new_timer);
		}
	}

	return current_idx;
}


void amsdu_timeout(struct rtl8192cd_priv *priv, unsigned int current_time)
{
	struct tx_insn tx_insn;
	struct stat_info *pstat;
	struct net_device *wdsDev=NULL;
	struct rtl8192cd_priv *priv_this=NULL;
	int tid=0, head;
	//DECLARE_TXCFG(txcfg, tx_insn);

	head = priv->pshare->amsdu_timer_head;
	while (CIRC_CNT(head, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM))
	{
		DECLARE_TXCFG(txcfg, tx_insn);	// will be reused in this while loop

		pstat = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat;
		if (pstat) {
			tid = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].tid;
			priv_this = priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].priv;
			priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].pstat = NULL;
		}

		priv->pshare->amsdu_timer_tail = (priv->pshare->amsdu_timer_tail + 1) & (AMSDU_TIMER_NUM - 1);

		if (pstat) {
#ifdef WDS
			wdsDev = NULL;
			if (pstat->state & WIFI_WDS) {
				wdsDev = getWdsDevByAddr(priv, pstat->hwaddr);
				txcfg->wdsIdx = getWdsIdxByDev(priv, wdsDev);
			}
#endif
#ifdef RTL_MANUAL_EDCA
			txcfg->q_num = PRI_TO_QNUM(priv, tid);
#else
			PRI_TO_QNUM(tid, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif

			if (pstat->state & WIFI_SLEEP_STATE)
				pstat->amsdu_timer_id[tid] = amsdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else if (amsdu_xmit(priv_this, pstat, txcfg, tid, 1, wdsDev, priv->dev) == 0) // not finish
				pstat->amsdu_timer_id[tid] = amsdu_timer_add(priv_this, pstat, tid, 1) + 1;
			else
				pstat->amsdu_timer_id[tid] = 0;
		}
	}

	if (CIRC_CNT(priv->pshare->amsdu_timer_head, priv->pshare->amsdu_timer_tail, AMSDU_TIMER_NUM)) {
		setup_timer2(priv, priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout);
		if (TSF_LESS(priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout, current_time))
			printk("Setup timer2 %d too late (now %d)\n", priv->pshare->amsdu_timer[priv->pshare->amsdu_timer_tail].timeout, current_time);
	}
}


 int amsdu_check(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct stat_info *pstat, struct tx_insn *txcfg)
{
	int q_num;
	unsigned int priority;
	unsigned short protocol;
	int *tx_head, *tx_tail, cnt, add_timer=1;
	struct rtl8192cd_hw	*phw;
	unsigned long flags;
	struct net_device *wdsDev=NULL;

	protocol = ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2)));
#ifdef CONFIG_RTL_WAPI_SUPPORT
	if ((protocol == 0x888e)||(protocol == ETH_P_WAPI))
#else
	if (protocol == 0x888e)
#endif
	{
		return RET_AGGRE_BYPASS;
	}

	if (((protocol + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) &&
				 (skb_headroom(skb) < sizeof(struct llc_snap))) {
		return RET_AGGRE_BYPASS;
	}
//----------------------

#ifdef CONFIG_RTK_MESH
	priority = get_skb_priority3(priv, skb, txcfg->is_11s);
#else
	priority = get_skb_priority(priv, skb);
#endif
#ifdef RTL_MANUAL_EDCA
	q_num = PRI_TO_QNUM(priv, priority);
#else
	PRI_TO_QNUM(priority, q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif

	phw = GET_HW(priv);
	tx_head = get_txhead_addr(phw, q_num);
	tx_tail = get_txtail_addr(phw, q_num);

	cnt = CIRC_CNT_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC);

#if 0
	if (cnt <= AMSDU_TX_DESC_TH)
		return RET_AGGRE_BYPASS;
#endif

	if (cnt == (CURRENT_NUM_TX_DESC - 1))
		return RET_AGGRE_DESC_FULL;

#ifdef MESH_AMSDU
	if(txcfg->is_11s==0 && isMeshPoint(pstat))
	{
		return RET_AGGRE_DESC_FULL;
	}
	if (txcfg->is_11s&1)
	{
		short j, popen =  ((txcfg->mesh_header.mesh_flag &1) ? 16 : 4);
		if (skb_headroom(skb) < popen || skb_cloned(skb)) {
			struct sk_buff *skb2 = dev_alloc_skb(skb->len);
			if (skb2 == NULL) {
				printk("%s: %s, dev_alloc_skb() failed!\n", priv->mesh_dev->name, __FUNCTION__);
				return RET_AGGRE_BYPASS;
			}
         	memcpy(skb_put(skb2, skb->len), skb->data, skb->len);
			dev_kfree_skb_any(skb);
			skb = skb2;
			txcfg->pframe = (void *)skb;
		}
		skb_push(skb, popen);
		for(j=0; j<sizeof(struct wlan_ethhdr_t); j++)
			skb->data[j]= skb->data[j+popen];
		memcpy(skb->data+j, &(txcfg->mesh_header), popen);
	}
#endif // MESH_AMSDU


	SAVE_INT_AND_CLI(flags);
	__skb_queue_tail(&pstat->amsdu_tx_que[priority], skb);
	pstat->amsdu_size[priority] += (skb->len + sizeof(struct llc_snap));

	if ((pstat->amsdu_size[priority] >= pstat->amsdu_level) ||
			(!pstat->is_realtek_sta && (skb_queue_len(&pstat->amsdu_tx_que[priority]) >= priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum)))
	{
#ifdef WDS
		wdsDev = NULL;
		if (pstat->state & WIFI_WDS) {
			wdsDev = getWdsDevByAddr(priv, pstat->hwaddr);
			txcfg->wdsIdx = getWdsIdxByDev(priv, wdsDev);
		}
#endif
		// delete timer entry
		if (pstat->amsdu_timer_id[priority] > 0) {
			priv->pshare->amsdu_timer[pstat->amsdu_timer_id[priority] - 1].pstat = NULL;
			pstat->amsdu_timer_id[priority] = 0;
		}
		txcfg->q_num = q_num;
		if (amsdu_xmit(priv, pstat, txcfg, priority, 0, wdsDev, priv->dev) == 0) // not finish
			pstat->amsdu_timer_id[priority] = amsdu_timer_add(priv, pstat, priority, 0) + 1;
		else
			add_timer = 0;
	}

	if (add_timer) {
		if (pstat->amsdu_timer_id[priority] == 0)
			pstat->amsdu_timer_id[priority] = amsdu_timer_add(priv, pstat, priority, 0) + 1;
	}

	RESTORE_INT(flags);

	return RET_AGGRE_ENQUE;
}
#endif // SUPPORT_TX_AMSDU

static int isICMPv6Mng(struct sk_buff *skb)
{
	if((skb->protocol == 0x86dd) &&
		((skb->data[20] == 0x3a) || (skb->data[54] == 0x3a)) //next header is icmpv6
		//&& skb->data[54] == 0x86 //RA
		/*128,129,133,.....255 SHOULD BE MANAGMENT PACKET
		 REF:http://en.wikipedia.org/wiki/ICMPv6 */
	)
	{
		return 1;		
	}
	else
		return 0;	
}
static inline int isMDNS(unsigned char *data)
{
	if((data[3] == 0x00) && (data[4] == 0x00) && (data[5] == 0xfb) &&
			(((data[12] == 0x08) && (data[13] == 0x00) // IPv4
			&& (data[23] == 0x11) // UDP
			//&& (data[30] == 0xe0) // 224.0.0.251
			&& (data[36] == 0x14) && (data[37] == 0xe9)) // port 5353
			||
			((data[12] == 0x86) && (data[13] == 0xdd) // IPv6
			&& (data[20] == 0x11) // next header is UDP
			&& (data[56] == 0x14) && (data[57] == 0xe9)) // port 5353
			)) {
		return 1;
	}
		
	return 0;
}

#ifdef SUPPORT_TX_MCAST2UNI
static int rtl8192cd_tx_recycle(struct rtl8192cd_priv *priv, unsigned int txRingIdx, int *recycleCnt_p);
static inline void check_tx_queue(struct rtl8192cd_priv *priv)
{
		int *tx_head, *tx_tail;
		struct rtl8192cd_hw	*phw = GET_HW(priv);

		tx_head = get_txhead_addr(phw, BE_QUEUE);// use BE queue to send multicast
		tx_tail = get_txtail_addr(phw, BE_QUEUE);
		if(CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) <(CURRENT_NUM_TX_DESC/4))
		{
			rtl8192cd_tx_queueDsr(priv, BE_QUEUE);
			//int recycleCnt;
			//rtl8192cd_tx_recycle(priv, BE_QUEUE, &recycleCnt);
		}
		
		return;
}


#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL_ETH_PRIV_SKB)
	extern struct sk_buff *priv_skb_copy(struct sk_buff *skb);
	extern int eth_skb_free_num;
#endif
#ifdef CONFIG_RTL8196B_GW_8M
#define ETH_SKB_FREE_TH 50
#else
#define ETH_SKB_FREE_TH 100
#endif

int isSpecialFloodMac(struct rtl8192cd_priv *priv, struct sk_buff *skb)
{
	int i;
	if(priv->pshare->rf_ft_var.mc2u_flood_ctrl==0)
	{
		return 0;
	}
	
	for(i=0; i< priv->pshare->rf_ft_var.mc2u_flood_mac_num; i++)
	{
		if(memcmp(skb->data, priv->pshare->rf_ft_var.mc2u_flood_mac[i].macAddr,MACADDRLEN)==0)
		{
			return 1;
		}

	}
	return 0;
}

int mlcst2unicst(struct rtl8192cd_priv *priv, struct sk_buff *skb)
{
	struct stat_info *pstat;
	struct list_head *phead, *plist;
	struct sk_buff *newskb;
	unsigned char origDest[6];
	char skbCloned=0;
	int i= 0;
	int m2uCnt =0;
	int fwdCnt=0;
	struct stat_info *pstat_found = NULL;
#ifdef MCAST2UI_REFINE
	unsigned int privacy;
#endif

#ifdef CONFIG_MAXED_MULTICAST	
	int M2Uanyway=0;
#endif
	
	skbCloned = skb_cloned(skb);
	memcpy(origDest, skb->data, 6);

	// all multicast managment packet try do m2u
	if( isSpecialFloodMac(priv,skb) || IS_MDNSV4_MAC(skb->data)||IS_MDNSV6_MAC(skb->data)||IS_IGMP_PROTO(skb->data) || isICMPv6Mng(skb) || IS_ICMPV6_PROTO(skb->data)|| isMDNS(skb->data))
	{
		/*added by qinjunjie,do multicast to unicast conversion, and send to every associated station */
		phead = &priv->asoc_list;
		plist = phead->next;
		while (phead && (plist != phead)) {
			pstat = list_entry(plist, struct stat_info, asoc_list);
			plist = plist->next;

			/* avoid   come from STA1 and send back STA1 */ 
			if (!memcmp(pstat->hwaddr, &skb->data[6], 6)){		
				continue; 
                                        }
#ifdef MCAST2UI_REFINE
			privacy = get_sta_encrypt_algthm(priv, pstat);
					if ((privacy == _NO_PRIVACY_ || privacy == _CCMP_PRIVACY_) &&
							!UseSwCrypto(priv, pstat, (pstat ? FALSE : TRUE)) && (newskb = skb_clone(skb, GFP_ATOMIC))) {
						memcpy(&newskb->cb[10], pstat->hwaddr, 6);
						newskb->cb[2] = (char)0xff;
						__rtl8192cd_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
					}
					else
#endif
					{				
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL_ETH_PRIV_SKB)
						newskb = priv_skb_copy(skb);
#else
						newskb = skb_copy(skb, GFP_ATOMIC);
#endif
						if (newskb) {
#ifdef MCAST2UI_REFINE                                                  
							memcpy(&newskb->cb[10], pstat->hwaddr, 6);
#else
							memcpy(newskb->data, pstat->hwaddr, 6);
#endif
							newskb->cb[2] = (char)0xff;			// not do aggregation
							__rtl8192cd_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
						}
						else {
							DEBUG_ERR("%s: muti2unit skb_copy() failed!\n", priv->dev->name);
							priv->stop_tx_mcast2uni = 2;
							priv->ext_stats.tx_drops++;
							DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
							dev_kfree_skb_any(skb);
							return TRUE;
						}
			
					}
		}
		dev_kfree_skb_any(skb);
		return TRUE;
	}

//#ifdef VIDEO_STREAMING_REFINE
	// for video streaming refine 
	phead = &priv->asoc_list;
	plist = phead->next;
	while (phead && (plist != phead)) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		/* avoid   come from STA1 and send back STA1 */ 
		if (!memcmp(pstat->hwaddr, &skb->data[6], 6)){		
			continue; 
		}		

		for (i=0; i<MAX_IP_MC_ENTRY; i++) {
			if (pstat->ipmc[i].used && !memcmp(&pstat->ipmc[i].mcmac[0], origDest, 6)) {
				pstat_found = pstat;
				m2uCnt++;
				break;
			}
		}
	}

	if (m2uCnt == 1 && !skb_cloned(skb)) {
#ifdef MCAST2UI_REFINE                                                  
            memcpy(&skb->cb[10], pstat_found->hwaddr, 6);
#else
			memcpy(skb->data, pstat_found->hwaddr, 6);
#endif
			__rtl8192cd_start_xmit(skb, priv->dev, TX_NO_MUL2UNI);		
			m2uCnt = 0;	
			return TRUE;	
		}

	fwdCnt = m2uCnt;
	
	if(m2uCnt == 0){

#ifdef CONFIG_MAXED_MULTICAST	
		/*case: when STA <=3 do M2U anyway ; 
		if STA number > 3 by orig method(multicast);*/
		if(priv->assoc_num <=3){
			M2Uanyway=1;
			fwdCnt = priv->assoc_num;			
		}else{
			if(!priv->pshare->rf_ft_var.mc2u_drop_unknown) 
			{
				return FALSE;
			}
			else
			{
				priv->ext_stats.tx_drops++;
				dev_kfree_skb_any(skb);
				return TRUE;

			}
		}
#else
		if(!priv->pshare->rf_ft_var.mc2u_drop_unknown) 
		{
			/*case: if M2U can't success then 
			  forward by multicast(orig method),
			  defect: may affect system performance
		    	 advantage:better compatibility*/ 
			return FALSE;		
		}
		else
		{

		/*case: if M2U can't success then drop this packet ;
		    defect:maybe some management packet will lose
		    advantage:better performance*/ 
	   	
			priv->ext_stats.tx_drops++;
			dev_kfree_skb_any(skb);
			return TRUE;
			
		}

#endif


	}
//#endif


	
	// Do multicast to unicast conversion
	phead = &priv->asoc_list;
	plist = phead->next;
	while (phead && (plist != phead)) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if (!memcmp(pstat->hwaddr, &skb->data[6], 6)){		
			continue; /* avoid come from STA1 and send back STA1*/ 
		}		


		{
			int *tx_head, *tx_tail, q_num;
			struct rtl8192cd_hw	*phw = GET_HW(priv);
			q_num = BE_QUEUE;	// use BE queue to send multicast
			tx_head = get_txhead_addr(phw, q_num);
			tx_tail = get_txtail_addr(phw, q_num);
			if (priv->stop_tx_mcast2uni) {
				rtl8192cd_tx_queueDsr(priv, q_num);

				if (priv->stop_tx_mcast2uni  == 1 &&
						CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) > (CURRENT_NUM_TX_DESC*1)/4) {
					priv->stop_tx_mcast2uni = 0;
				}
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL_ETH_PRIV_SKB)
				else if ((priv->stop_tx_mcast2uni == 2) && (eth_skb_free_num > ETH_SKB_FREE_TH))
					{
					priv->stop_tx_mcast2uni = 0;
				}
#endif
				else {
					dev_kfree_skb_any(skb);
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
						return TRUE;
					}
                                                }
			else {
				if (CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) < 20) {
					priv->stop_tx_mcast2uni = 1;
					dev_kfree_skb_any(skb);
					priv->ext_stats.tx_drops++;
					DEBUG_ERR("TX DROP: %s: txdesc full!\n", __FUNCTION__);
					return TRUE;
				}
			}
		}
#ifdef CONFIG_MAXED_MULTICAST

		if(M2Uanyway){
				if((fwdCnt== 1) && !skb_cloned(skb))
					{
#ifdef MCAST2UI_REFINE                                                  
						memcpy(&skb->cb[10], pstat->hwaddr, 6);
#else
						memcpy(skb->data, pstat->hwaddr, 6);
#endif
						skb->cb[2] = (char)0xff;			// not do aggregation
						__rtl8192cd_start_xmit(skb, priv->dev, TX_NO_MUL2UNI);
						return TRUE;
					}
				else {
						#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL_ETH_PRIV_SKB)
						newskb = priv_skb_copy(skb);
						#else
						newskb = skb_copy(skb, GFP_ATOMIC);
						#endif
						if (newskb) {
#ifdef MCAST2UI_REFINE                                                  
							memcpy(&newskb->cb[10], pstat->hwaddr, 6);
#else
							memcpy(newskb->data, pstat->hwaddr, 6);
#endif
							newskb->cb[2] = (char)0xff;			// not do aggregation
                                                        __rtl8192cd_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
                                                }
						else {
							DEBUG_ERR("%s: muti2unit skb_copy() failed!\n", priv->dev->name);
							priv->stop_tx_mcast2uni = 2;
							priv->ext_stats.tx_drops++;
							DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
							dev_kfree_skb_any(skb);
							return TRUE;
						}
					
					fwdCnt--;

						}
					}
#endif
		for (i=0; i<MAX_IP_MC_ENTRY; i++) {
			if (pstat->ipmc[i].used && !memcmp(&pstat->ipmc[i].mcmac[0], origDest, 6)) {


				if((fwdCnt== 1) && !skb_cloned(skb))
						{
#ifdef MCAST2UI_REFINE                                                  
					memcpy(&skb->cb[10], pstat->hwaddr, 6);
#else
					memcpy(skb->data, pstat->hwaddr, 6);
#endif
					skb->cb[2] = (char)0xff; 		// not do aggregation
					__rtl8192cd_start_xmit(skb, priv->dev, TX_NO_MUL2UNI);
					return TRUE;
				}
				else {
#ifdef MCAST2UI_REFINE
						unsigned int privacy = get_sta_encrypt_algthm(priv, pstat);
						if ((privacy == _NO_PRIVACY_ || privacy == _CCMP_PRIVACY_) &&
								!skbCloned && (newskb = skb_clone(skb, GFP_ATOMIC))) {
							memcpy(&newskb->cb[10], pstat->hwaddr, 6);
							newskb->cb[2] = (char)0xff;
							__rtl8192cd_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
						}
						else
#endif
						{	
						#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL_ETH_PRIV_SKB)

						newskb = priv_skb_copy(skb);
						#else
						newskb = skb_copy(skb, GFP_ATOMIC);
						#endif
						if (newskb) {
#ifdef MCAST2UI_REFINE                                                  
							memcpy(&newskb->cb[10], pstat->hwaddr, 6);
#else
							memcpy(newskb->data, pstat->hwaddr, 6);
#endif
							newskb->cb[2] = (char)0xff;			// not do aggregation
							__rtl8192cd_start_xmit(newskb, priv->dev, TX_NO_MUL2UNI);
						}
						else {
							DEBUG_ERR("%s: muti2unit skb_copy() failed!\n", priv->dev->name);
							priv->stop_tx_mcast2uni = 2;
							priv->ext_stats.tx_drops++;
							DEBUG_ERR("TX DROP: %s: run out ether buffer!\n", __FUNCTION__);
							dev_kfree_skb_any(skb);
							return TRUE;
						}
						}
					fwdCnt--;
				}
			}
		}
	}
	
	/*
	 *	Device interested in this MC IP cannot be found, drop packet.
	 */
	priv->ext_stats.tx_drops++;
	dev_kfree_skb_any(skb);
	return TRUE;
}

#endif // TX_SUPPORT_MCAST2U


#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
int check_txdesc_dynamic_mechanism(struct rtl8192cd_priv *priv, int q_num, int txdesc_need)
{
	struct rtl8192cd_priv *root_priv = NULL;
	unsigned int lower_limit = priv->pshare->num_txdesc_lower_limit;
	unsigned int avail_cnt = priv->pshare->num_txdesc_cnt;
	unsigned int used = priv->use_txdesc_cnt[q_num];
	unsigned int accu = 0, i;

	if (IS_ROOT_INTERFACE(priv))
		root_priv = priv;
	else
		root_priv = GET_ROOT_PRIV(priv);
	
	if (IS_ROOT_INTERFACE(priv)) {
		if (IS_DRV_OPEN(priv))
			accu += used;
	} else {
		if (IS_DRV_OPEN(root_priv))
			accu += MAX_NUM(root_priv->use_txdesc_cnt[q_num], lower_limit);
	}
	
#ifdef UNIVERSAL_REPEATER
	if (IS_VXD_INTERFACE(priv)) {
		if (IS_DRV_OPEN(priv))
			accu += used;
	} else {
		if (IS_DRV_OPEN(root_priv->pvxd_priv))
			accu += MAX_NUM(root_priv->pvxd_priv->use_txdesc_cnt[q_num], lower_limit);
	}
#endif

#ifdef MBSSID
	for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
		if (IS_DRV_OPEN(root_priv->pvap_priv[i])) {
			if (root_priv->pvap_priv[i] == priv)
				accu += used;
	else
				accu += MAX_NUM(root_priv->pvap_priv[i]->use_txdesc_cnt[q_num], lower_limit);
		}
	}
#endif
	
	if (accu + txdesc_need <= avail_cnt)
		return 0;
	
	return -1;
}

#ifdef USE_TXQUEUE
int check_txq_dynamic_mechanism(struct rtl8192cd_priv *priv, int q_num)
{
	struct rtl8192cd_priv *root_priv = NULL;
	unsigned int lower_limit = priv->pshare->num_txq_lower_limit;
	unsigned int avail_cnt = priv->pshare->num_txq_cnt;
	unsigned int used = priv->use_txq_cnt[q_num];
	unsigned int accu = 0, i;
	
	if (IS_ROOT_INTERFACE(priv))
		root_priv = priv;
	else
		root_priv = GET_ROOT_PRIV(priv);
		
	if (IS_ROOT_INTERFACE(priv))
	{
		if ( IS_DRV_OPEN(priv) )
			accu += used;
	}
	else
	{
		if ( IS_DRV_OPEN(root_priv) )
			accu += MAX_NUM(root_priv->use_txq_cnt[q_num], lower_limit);
	}
		
#ifdef UNIVERSAL_REPEATER
	if (IS_VXD_INTERFACE(priv))
	{
		if ( IS_DRV_OPEN(priv) )
			accu += used;
	}
	else
	{
		if ( IS_DRV_OPEN(root_priv->pvxd_priv) )
			accu += MAX_NUM(root_priv->pvxd_priv->use_txq_cnt[q_num], lower_limit);
	}
#endif
	
#ifdef MBSSID
	for (i=0; i<RTL8192CD_NUM_VWLAN; i++)
	{
		if ( IS_DRV_OPEN(root_priv->pvap_priv[i]) )
		{
			if (root_priv->pvap_priv[i] == priv)
				accu += used;
			else
				accu += MAX_NUM(root_priv->pvap_priv[i]->use_txq_cnt[q_num], lower_limit);
		}
	}
#endif
		
	if (accu < avail_cnt)
		return 0;

	return -1;

}
#endif
#endif


#ifdef CONFIG_RTL_8812_SUPPORT
#ifdef BEAMFORMING_SUPPORT
BOOLEAN
IsMgntNDPA(
	pu1Byte		pdu
)
{
	BOOLEAN ret = 0;
	if(IsMgntActionNoAck(pdu) && GET_80211_HDR_ORDER(pdu))
	{
		if(GET_HT_CTRL_NDP_ANNOUNCEMENT(pdu+sMacHdrLng) == 1)
			ret = 1;
	}
	return ret;
}
#endif
static void rtl8192cd_fill_fwinfo_8812(struct rtl8192cd_priv *priv, struct tx_insn* txcfg, struct tx_desc  *pdesc, unsigned int frag_idx)
{
	char n_txshort = 0, bg_txshort = 0;
	int erp_protection = 0, n_protection = 0;
	unsigned char rate;
	unsigned char txRate = 0;
#ifdef DRVMAC_LB
	static unsigned int rate_select = 0;
#endif

#ifdef MP_TEST
	if (OPMODE & WIFI_MP_STATE) {
		if (is_VHT_rate(txcfg->tx_rate)) {
			txRate = txcfg->tx_rate -0x90;
			txRate += 44;
		} else if (is_MCS_rate(txcfg->tx_rate)) {	// HT rates
			txRate = txcfg->tx_rate & 0x7f;
			txRate += 12;
		} else{
			txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		}

		if (priv->pshare->is_40m_bw == 2) {
			pdesc->Dword5 |= set_desc((0 << TXdesc_92E_DataScSHIFT) | (0 << TXdesc_92E_RtsScSHIFT));
			pdesc->Dword5 |= set_desc(0x2 << TXdesc_92E_DataBwSHIFT);

			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M)
				n_txshort = 1;
		} else if (priv->pshare->is_40m_bw == 1) {	
			pdesc->Dword5 |= set_desc((0 << TXdesc_92E_DataScSHIFT) | (0 << TXdesc_92E_RtsScSHIFT));
			pdesc->Dword5 |= set_desc(0x1 << TXdesc_92E_DataBwSHIFT);

			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M)
				n_txshort = 1;
		} else {
			pdesc->Dword5 |= set_desc((0 << TXdesc_92E_DataScSHIFT) | (0 << TXdesc_92E_RtsScSHIFT));
			pdesc->Dword5 |= set_desc(0 << TXdesc_92E_DataBwSHIFT);
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M)
				n_txshort = 1;
		}

		if (txcfg->retry) {	
			pdesc->Dword4 |= set_desc(TXdesc_92E_RtyLmtEn);
            		pdesc->Dword4 |= set_desc((txcfg->retry  & TXdesc_92E_DataRtyLmtMask) << TXdesc_92E_DataRtyLmtSHIFT);
		}

		pdesc->Dword4 |= set_desc((txRate & TXdesc_92E_DataRateMask) << TXdesc_92E_DataRateSHIFT);
		
		
		if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G)
			pdesc->Dword4 |= set_desc((4 & TX_RtsRateMask_8812) << TX_RtsRateSHIFT_8812);

		return;
	}
#endif


	if (is_MCS_rate(txcfg->tx_rate))	// HT rates
	{
		if( txcfg->tx_rate >= 0x90)
		{
			txRate = (txcfg->tx_rate - 0x90) + 44;
		}
		else
		{
			txRate = txcfg->tx_rate & 0x7f;
			txRate += 12;
		}

		if (priv->pshare->is_40m_bw==2)
		{
//			get_txsc_8812(priv, priv->pmib->dot11RFEntry.dot11channel);
			if(txcfg->pstat && is_VHT_rate(txcfg->tx_rate) && (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_80 
				&& (priv->pmib->dot11StationConfigEntry.autoRate || is_fixedVHTTxRate(priv))
			) )
			{
				pdesc->Dword5 |= set_desc((0 << TXdesc_92E_DataScSHIFT) | (0 << TXdesc_92E_RtsScSHIFT));
				pdesc->Dword5 |= set_desc(0x2 << TXdesc_92E_DataBwSHIFT);

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M && //todo, add shortGI 80M option
					txcfg->pstat && (txcfg->pstat->vht_cap_buf.vht_cap_info & cpu_to_le32(BIT(5))))
						n_txshort = 1;
			}
			else if(txcfg->pstat && is_MCS_rate(txcfg->tx_rate) && (txcfg->pstat->tx_bw >= HT_CHANNEL_WIDTH_20_40
					&& (priv->pmib->dot11StationConfigEntry.autoRate || is_fixedMCSTxRate(priv))		
				) )
			{
				pdesc->Dword5 |= set_desc((priv->pshare->txsc_40 << TXdesc_92E_DataScSHIFT) | (priv->pshare->txsc_40 << TXdesc_92E_RtsScSHIFT));
				pdesc->Dword5 |= set_desc(0x1 << TXdesc_92E_DataBwSHIFT);

				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_)))
						n_txshort = 1;
			}
			else
			{
				pdesc->Dword5 |= set_desc((priv->pshare->txsc_20 << TXdesc_92E_DataScSHIFT) | (priv->pshare->txsc_20 << TXdesc_92E_RtsScSHIFT));
				
				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
						n_txshort = 1;
			}
		}
		else
		if (priv->pshare->is_40m_bw) {
			if (txcfg->pstat && (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
#ifdef WIFI_11N_2040_COEXIST
				&& !(priv->pmib->dot11nConfigEntry.dot11nCoexist && (((OPMODE & WIFI_AP_STATE) && 
				(priv->bg_ap_timeout ||priv->force_20_sta || priv->switch_20_sta
#ifdef STA_EXT
				|| priv->force_20_sta_ext || priv->switch_20_sta_ext
#endif
				))
#ifdef CLIENT_MODE
				|| ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection && 
				(txcfg->pstat->ht_ie_len) && !(txcfg->pstat->ht_ie_buf.info0 & _HTIE_STA_CH_WDTH_))
#endif
				))
#endif

				) {
					
				pdesc->Dword5 |= set_desc((1 << TXdesc_92E_DataBwSHIFT) | (3 << TXdesc_92E_DataScSHIFT));

				{
					if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
						txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_)))
						n_txshort = 1;
				}
			}
			else {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
					pdesc->Dword5 |= set_desc((2 << TXdesc_92E_DataScSHIFT) | (2 << TXdesc_92E_RtsScSHIFT));
				else
					pdesc->Dword5 |= set_desc((1 << TXdesc_92E_DataScSHIFT) | (1 << TXdesc_92E_RtsScSHIFT));

				{
					if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
						txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
						n_txshort = 1;
				}
			}
		} else {
			{
				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
					n_txshort = 1;
			}
		}

		
		if (
			( txcfg->pstat && (txcfg->pstat->aggre_mthd == AGGRE_MTHD_MPDU_AMSDU) && txcfg->aggre_en )	|| 
			((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) 
		){
			int TID = ((struct sk_buff *)txcfg->pframe)->cb[1];
			if (txcfg->pstat->ADDBA_ready[TID] && !txcfg->pstat->low_tp_disable_ampdu) {


					pdesc->Dword2 |= set_desc(TXdesc_92E_AggEn);


				/*
				 * assign aggr size
				 */

				// assign aggr density
				if (txcfg->privacy) {
					//8812_11n_iot, set TxAmpduDsty=7 for 20M WPA2
					if ((priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) && (!priv->pshare->is_40m_bw))
						pdesc->Dword2 |= set_desc(7 << TX_AmpduDstySHIFT);	// according to DWA-160 A2
					else
						pdesc->Dword2 |= set_desc(5 << TX_AmpduDstySHIFT);	// according to WN111v2
				}
				else {
					pdesc->Dword2 |= set_desc(((txcfg->pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_) << TX_AmpduDstySHIFT);
				}
			}
			//set Break
			if((txcfg->q_num >=1 && txcfg->q_num <=4)){
				if((txcfg->pstat != priv->pshare->CurPstat[txcfg->q_num-1])) {

						pdesc->Dword2 |= set_desc(TXdesc_92E_BK);
					priv->pshare->CurPstat[txcfg->q_num-1] = txcfg->pstat;
				}				
			} else {

					pdesc->Dword2 |= set_desc(TXdesc_92E_BK);
			}
		}

		// for STBC
		if (priv->pmib->dot11nConfigEntry.dot11nSTBC &&	txcfg->pstat )	// 2012 10 31  for test
		{
			if((txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_RX_STBC_CAP_)) 
				|| (txcfg->pstat->vht_cap_buf.vht_cap_info & cpu_to_le16(_VHTCAP_RX_STBC_CAP_))){

				pdesc->Dword5 |= set_desc(1 << TXdesc_92E_DataStbcSHIFT);

			}

		}
	}
	else	// legacy rate
	{
		txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		if (is_CCK_rate(txcfg->tx_rate) && (txcfg->tx_rate != 2)) {
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
				; // txfw->txshort = 0
			else {
				if (txcfg->pstat)
					bg_txshort = (priv->pmib->dot11RFEntry.shortpreamble) &&
									(txcfg->pstat->useShortPreamble);
				else
					bg_txshort = priv->pmib->dot11RFEntry.shortpreamble;
			}
		}

		if (priv->pshare->is_40m_bw==2) {
//			get_txsc_8812(priv, priv->pmib->dot11RFEntry.dot11channel);
			pdesc->Dword5 |= set_desc((priv->pshare->txsc_20 << TXdesc_92E_DataScSHIFT) | (priv->pshare->txsc_20  << TXdesc_92E_RtsScSHIFT));
		}
		else
		if (priv->pshare->is_40m_bw) {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
				pdesc->Dword5 |= set_desc((2 << TXdesc_92E_DataScSHIFT) | (2 << TXdesc_92E_RtsScSHIFT));
			else
				pdesc->Dword5 |= set_desc((1 << TXdesc_92E_DataScSHIFT) | (1 << TXdesc_92E_RtsScSHIFT));	
		}

		if (bg_txshort)
			pdesc->Dword5 |= set_desc(TXdesc_92E_DataShort);
			
	}

	if (txcfg->need_ack) { // unicast
		if (frag_idx == 0) {
			if ((txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len)) ||
				(txcfg->pstat && txcfg->pstat->is_forced_rts))
				pdesc->Dword3 |=set_desc(TX_RtsEn);
			else {
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					is_MCS_rate(txcfg->tx_rate) &&
					(priv->ht_protection /*|| txcfg->pstat->is_rtl8190_sta*/))
				{
					n_protection = 1;
					if (priv->pmib->dot11ErpInfo.protection)
						erp_protection = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						pdesc->Dword3 |= set_desc(TX_CTS2Self);
					else					
						pdesc->Dword3 |=set_desc(TX_RtsEn);
						
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(!is_CCK_rate(txcfg->tx_rate)) && // OFDM mode
					priv->pmib->dot11ErpInfo.protection)
				{
					erp_protection = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						pdesc->Dword3 |= set_desc(TX_CTS2Self);
					else						
						pdesc->Dword3 |=set_desc(TX_RtsEn);
					
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					(txcfg->pstat) && (txcfg->pstat->MIMO_ps & _HT_MIMO_PS_DYNAMIC_) &&
//					is_MCS_rate(txcfg->tx_rate) && ((txcfg->tx_rate & 0x7f)>7))
					is_2T_rate(txcfg->tx_rate))
				{	// when HT MIMO Dynamic power save is set and rate > MCS7, RTS is needed
					pdesc->Dword3 |=set_desc(TX_RtsEn);
				
				} else {
					/*
					 * Auto rts mode, use rts depends on packet length and packet tx time
					 */
#ifdef _OUTSRC_COEXIST
					if((IS_OUTSRC_CHIP(priv)&&(is_MCS_rate(txcfg->tx_rate) && (/*(txcfg->pstat->IOTPeer!=HT_IOT_PEER_INTEL) ||*/ !txcfg->pstat->no_rts))) 
						|| (!IS_OUTSRC_CHIP(priv) && (is_MCS_rate(txcfg->tx_rate) && (/*!txcfg->pstat->is_intel_sta ||*/ !txcfg->pstat->no_rts))))
#else
#ifdef USE_OUT_SRC	
					if (is_MCS_rate(txcfg->tx_rate) && (/*(txcfg->pstat->IOTPeer!=HT_IOT_PEER_INTEL) ||*/ !txcfg->pstat->no_rts))
#else
					if (is_MCS_rate(txcfg->tx_rate) && (/*!txcfg->pstat->is_intel_sta ||*/ !txcfg->pstat->no_rts))
#endif
#endif
						pdesc->Dword3 |=set_desc(TX_HwRtsEn | TX_RtsEn);
				}
			}

		}
	}


	if (get_desc(pdesc->Dword3 ) & (TX_CTS2Self|TX_RtsEn)) {
		if (erp_protection)
			rate = (unsigned char)find_rate(priv, NULL, 1, 3);
		else
			rate = (unsigned char)find_rate(priv, NULL, 1, 1);

		if (is_MCS_rate(rate)) {	// HT rates
			// can we use HT rates for RTS?
		} else {
			unsigned int rtsTxRate  = 0;
			rtsTxRate = get_rate_index_from_ieee_value(rate);
			if (erp_protection) {
				unsigned char  rtsShort = 0;
				if (is_CCK_rate(rate) && (rate != 2)) {
					if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
							(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
						rtsShort = 0; // do nothing
					else {
						if (txcfg->pstat)
							rtsShort = (priv->pmib->dot11RFEntry.shortpreamble) &&
											(txcfg->pstat->useShortPreamble);
						else
							rtsShort = priv->pmib->dot11RFEntry.shortpreamble;
					}
				}

				pdesc->Dword5 |= (rtsShort == 1)? set_desc(TXdesc_92E_RtsShort): 0;
				
			} 
			else if (n_protection) {
				rtsTxRate = get_rate_index_from_ieee_value(48);
			}
			else {	// > RTS threshold
			}

			pdesc->Dword4 |= set_desc((rtsTxRate & TXdesc_92E_RtsRateMask) << TXdesc_92E_RtsRateSHIFT);
			pdesc->Dword4 |= set_desc((0xf & TXdesc_92E_RtsRateFBLmtMask) << TXdesc_92E_RtsRateFBLmtSHIFT);
				
		}
	}

	if(n_txshort == 1
#ifdef STA_EXT
		&& txcfg->pstat && 
		(txcfg->pstat->remapped_aid < RTL8812_NUM_STAT -1)
#endif
	)
		pdesc->Dword5 |= set_desc(TXdesc_92E_DataShort);		
	

#ifdef DRVMAC_LB
	if (priv->pmib->miscEntry.drvmac_lb && (priv->pmib->miscEntry.lb_mlmp == 4)) {
		txRate = rate_select;
		if (rate_select++ > 0x1b)
			rate_select = 0;

		pdesc->Dword3 |= set_desc(TXdesc_92E_DisDataFB|TXdesc_92E_DisRtsFB|TXdesc_92E_UseRate);
	}
#endif


	if(priv->pshare->rf_ft_var.txforce != 0xff)
	{
				pdesc->Dword3 |= set_desc(TXdesc_92E_DisDataFB|TXdesc_92E_DisRtsFB|TXdesc_92E_UseRate);
				pdesc->Dword4 |= set_desc((priv->pshare->rf_ft_var.txforce & TXdesc_92E_DataRateMask) << TXdesc_92E_DataRateSHIFT);
	}
	else
	pdesc->Dword4 |= set_desc((txRate & TXdesc_92E_DataRateMask) << TXdesc_92E_DataRateSHIFT);


#if 1
	if (priv->pshare->rf_ft_var.rts_init_rate) {
		pdesc->Dword4 &= set_desc(~(TX_RtsRateMask_8812 << TX_RtsRateSHIFT_8812));
		pdesc->Dword4 |= set_desc(((priv->pshare->rf_ft_var.rts_init_rate) & TX_RtsRateMask_8812) << TX_RtsRateSHIFT_8812);
	}		
	if ((priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) && 
		(TX_RtsRateMask_8812&(get_desc(pdesc->Dword4)>>TX_RtsRateSHIFT_8812)) <4	)
		pdesc->Dword4 |= set_desc((4 & TX_RtsRateMask_8812) << TX_RtsRateSHIFT_8812);
#else		
	if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G)
		pdesc->Dword4 |= set_desc((4 & TX_RtsRateMask_8812) << TX_RtsRateSHIFT_8812);

#endif

	if (txcfg->need_ack) {
		// give retry limit to management frame
		if (txcfg->q_num == MANAGE_QUE_NUM) {
			pdesc->Dword4 |= set_desc(TXdesc_92E_RtyLmtEn);
			
			if (GetFrameSubType(txcfg->phdr) == WIFI_PROBERSP) {
				;	// 0 no need to set
			}
#ifdef WDS
			else if ((GetFrameSubType(txcfg->phdr) == WIFI_PROBEREQ) && (txcfg->pstat->state & WIFI_WDS)) {
				pdesc->Dword4 |= set_desc((2 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
			}
#endif
			else {
				pdesc->Dword4 |= set_desc((6 & TXdesc_92E_DataRtyLmtMask) << TXdesc_92E_DataRtyLmtSHIFT);
			}
		}
#ifdef WDS
		else if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat->rx_avarage == 0) {
				pdesc->Dword4 |= set_desc(TX_RtyLmtEn);
				pdesc->Dword4 |= set_desc((3 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
			}
		}
#endif
#ifdef _OUTSRC_COEXIST
		else if(((IS_OUTSRC_CHIP(priv)&&(is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->IOTPeer==HT_IOT_PEER_INTEL))) 
					|| (!IS_OUTSRC_CHIP(priv) && (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->is_intel_sta))))
#else
#ifdef USE_OUT_SRC	
		else if (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->IOTPeer==HT_IOT_PEER_INTEL) 
#else
		else if (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->is_intel_sta) 
#endif
#endif
			&& !(txcfg->pstat->leave) && priv->pshare->intel_rty_lmt) {			
			pdesc->Dword4 |= set_desc(TXdesc_92E_RtyLmtEn);
			pdesc->Dword4 |= set_desc((priv->pshare->intel_rty_lmt & TXdesc_92E_DataRtyLmtMask) << TXdesc_92E_DataRtyLmtSHIFT);
		}
#ifdef _OUTSRC_COEXIST
		else if((IS_OUTSRC_CHIP(priv)&&((txcfg->pstat->IOTPeer==HT_IOT_PEER_BROADCOM) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave)))
			|| (!IS_OUTSRC_CHIP(priv) && ((txcfg->pstat->is_broadcom_sta) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))))
#else
#ifdef USE_OUT_SRC	
		else if ((txcfg->pstat->IOTPeer==HT_IOT_PEER_BROADCOM) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))
#else
		else if ((txcfg->pstat->is_broadcom_sta) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))
#endif			
#endif
                {
			pdesc->Dword4 |= set_desc(TXdesc_92E_RtyLmtEn);
            		pdesc->Dword4 |= set_desc((0x20 & TXdesc_92E_DataRtyLmtMask) << TXdesc_92E_DataRtyLmtSHIFT);
                }

		if (priv->pshare->rf_ft_var.tx_pwr_ctrl && txcfg->pstat && (txcfg->fr_type == _SKB_FRAME_TYPE_)) {
			if (txcfg->pstat->hp_level == 1) {
				pdesc->Dword5 |= set_desc((2 & TXdesc_8812_TxPwrOffetMask) << TXdesc_8812_TxPwrOffetSHIFT);
			}
		}	

#ifdef BEAMFORMING_SUPPORT
	if(txcfg->ndpa) {
		unsigned char *pFrame = (unsigned char*)txcfg->phdr;
		if(IsCtrlNDPA(pFrame) || IsMgntNDPA(pFrame))
		{
			//SET_TX_DESC_DATA_RETRY_LIMIT_8812(pDesc, 5);
			//SET_TX_DESC_RETRY_LIMIT_ENABLE_8812(pDesc, 1);
			pdesc->Dword4 &= set_desc(~(TXdesc_92E_DataRtyLmtMask << TXdesc_92E_DataRtyLmtSHIFT));									
            pdesc->Dword4 |= set_desc(TXdesc_92E_RtyLmtEn|((0x05 & TXdesc_92E_DataRtyLmtMask) << TXdesc_92E_DataRtyLmtSHIFT));	

			if(IsMgntNDPA(pFrame))		//0xe0
			{
				pdesc->Dword3 |= set_desc((1 &TXdesc_92E_NDPAMASK)<<TXdesc_92E_NDPASHIFT);
			}	
			else		// 0x54
			{
				if(!IS_TEST_CHIP(priv))
				{
					pdesc->Dword3 |= set_desc((2 &TXdesc_92E_NDPAMASK)<<TXdesc_92E_NDPASHIFT);

				}
				else
				{
					pdesc->Dword3 |= set_desc((1 &TXdesc_92E_NDPAMASK)<<TXdesc_92E_NDPASHIFT);				
				}
			}	
			panic_printk("LINE:%d, %x\n", __LINE__, get_desc(pdesc->Dword3));
		}
	}
#endif

		if ((priv->pmib->dot11nConfigEntry.dot11nLDPC == 1) && (txcfg->pstat) && 
	((txcfg->pstat->ht_cap_len && cpu_to_le16(txcfg->pstat->ht_cap_buf.ht_cap_info) & _HTCAP_SUPPORT_RX_LDPC_) ||
			(txcfg->pstat->vht_cap_len && (cpu_to_le32(txcfg->pstat->vht_cap_buf.vht_cap_info) & BIT(RX_LDPC_E))))) {
		pdesc->Dword5 |= set_desc(TXdesc_92E_DataLDPC);	
	}
	}
}

#endif



static void rtl8192cd_fill_fwinfo(struct rtl8192cd_priv *priv, struct tx_insn* txcfg, struct tx_desc  *pdesc, unsigned int frag_idx)
{
	char n_txshort = 0, bg_txshort = 0;
	int erp_protection = 0, n_protection = 0;
	unsigned char rate;
	unsigned char txRate = 0;
#ifdef DRVMAC_LB
	static unsigned int rate_select = 0;
#endif

#ifdef MP_TEST
	if (OPMODE & WIFI_MP_STATE) {
		if (is_MCS_rate(txcfg->tx_rate)) {	// HT rates
			txRate = txcfg->tx_rate & 0x7f;
			txRate += 12;
			//pdesc->Dword4 |= set_desc(TX_TXHT);
		}
		else{
			txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		}

		if (priv->pshare->CurrentChannelBW) {
			pdesc->Dword4 |= set_desc(TX_DataBw | (3&TX_DataScMask) << TX_DataScSHIFT);
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M)
				n_txshort = 1;
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M)
				n_txshort = 1;
		}

		if (txcfg->retry)
			pdesc->Dword5 |= set_desc((txcfg->retry & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT | TX_RtyLmtEn);

		if(n_txshort == 1)
			pdesc->Dword5 |= set_desc(TX_SGI);

		pdesc->Dword5 |= set_desc((txRate & TX_DataRateMask) << TX_DataRateSHIFT);
		
#ifdef CONFIG_RTL_92D_SUPPORT
		if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) {
			pdesc->Dword4 |= set_desc((4 & TX_RtsRateMask) << TX_RtsRateSHIFT);
			//if (is_CCK_rate(txRate))
				//pdesc->Dword5 |= set_desc((4 & TX_DataRateMask) << TX_DataRateSHIFT);
		}
#endif

		return;
	}
#endif

	if (priv->pmib->dot11RFEntry.txbf == 1) {
		pdesc->Dword2 &= set_desc(0x03ffffff); // clear related bits
		pdesc->Dword2 |= set_desc(1 << TX_TxAntCckSHIFT); 	// Set Default CCK rate with 1T
		pdesc->Dword2 |= set_desc(1 << TX_TxAntlSHIFT); 	// Set Default Legacy rate with 1T
		pdesc->Dword2 |= set_desc(1 << TX_TxAntHtSHIFT); 	// Set Default Ht rate		
	}

	if (is_MCS_rate(txcfg->tx_rate))	// HT rates
	{
		txRate = txcfg->tx_rate & 0x7f;
		txRate += 12;
		
		if (priv->pmib->dot11RFEntry.txbf == 1) {
			if (txRate <= 0x12) {
				pdesc->Dword2 |= set_desc(3 << TX_TxAntHtSHIFT); // Set Ht rate < MCS6 with 2T
			}
		}

		if (priv->pshare->is_40m_bw) {
			if (txcfg->pstat && (txcfg->pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)
#ifdef WIFI_11N_2040_COEXIST
				&& !(priv->pmib->dot11nConfigEntry.dot11nCoexist && (((OPMODE & WIFI_AP_STATE) && 
				(priv->bg_ap_timeout ||priv->force_20_sta || priv->switch_20_sta
#ifdef CONFIG_RTL_88E_SUPPORT
				|| ((GET_CHIP_VER(priv) == VERSION_8188E)?(priv->force_20_sta_88e_hw_ext || priv->switch_20_sta_88e_hw_ext):0)
#endif
#ifdef STA_EXT
				|| priv->force_20_sta_ext || priv->switch_20_sta_ext
#endif
				))
#ifdef CLIENT_MODE
				|| ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection && 
				(txcfg->pstat->ht_ie_len) && !(txcfg->pstat->ht_ie_buf.info0 & _HTIE_STA_CH_WDTH_))
#endif
				))
#endif

				) {
				pdesc->Dword4 |= set_desc(TX_DataBw | (3 << TX_DataScSHIFT));

#if defined(CONFIG_RTL_88E_SUPPORT) && defined(TXREPORT)
				if ((txcfg->fixed_rate) || (GET_CHIP_VER(priv)!=VERSION_8188E))
#endif
				{
					if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
						txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_)))
						n_txshort = 1;
				}
			}
			else {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
					pdesc->Dword4 |= set_desc((2 << TX_DataScSHIFT) | (2 << TX_RtsScSHIFT));
				else
					pdesc->Dword4 |= set_desc((1 << TX_DataScSHIFT) | (1 << TX_RtsScSHIFT));

#if defined(CONFIG_RTL_88E_SUPPORT) && defined(TXREPORT)
				if ((txcfg->fixed_rate) || (GET_CHIP_VER(priv)!=VERSION_8188E))
#endif
				{
					if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
						txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
						n_txshort = 1;
				}
			}
		} else {
#if defined(CONFIG_RTL_88E_SUPPORT) && defined(TXREPORT)
			if ((txcfg->fixed_rate) || (GET_CHIP_VER(priv)!=VERSION_8188E))
#endif
			{
				if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
					txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
					n_txshort = 1;
			}
		}

#if defined(CONFIG_RTL_88E_SUPPORT) && defined(TXREPORT)
		if ((GET_CHIP_VER(priv)==VERSION_8188E) && !(txcfg->fixed_rate)) {
			if (txcfg->pstat->ht_current_tx_info & TX_USE_SHORT_GI)
				n_txshort = 1;
		}
#endif

		if ((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST)) {
			int TID = ((struct sk_buff *)txcfg->pframe)->cb[1];
			if (txcfg->pstat->ADDBA_ready[TID] && !txcfg->pstat->low_tp_disable_ampdu) {
#ifdef CONFIG_RTL_88E_SUPPORT
				if (GET_CHIP_VER(priv)==VERSION_8188E)
					pdesc->Dword2 |= set_desc(TXdesc_88E_AggEn);
				else
#endif
					pdesc->Dword1 |= set_desc(TX_AggEn);

				/*
				 * assign aggr size
				 */
				 if (priv->pshare->rf_ft_var.diffAmpduSz) {
					pdesc->Dword6 |= set_desc((txcfg->pstat->diffAmpduSz & 0xffff) << TX_MCS1gMaxSHIFT | TX_UseMaxLen);
					
#ifdef CONFIG_RTL_88E_SUPPORT
					if (GET_CHIP_VER(priv)!=VERSION_8188E)
#endif						
					pdesc->Dword7 |= set_desc(txcfg->pstat->diffAmpduSz & 0xffff0000);
				 }	
				// assign aggr density
				if (txcfg->privacy) {
#ifdef CONFIG_RTL_92D_SUPPORT
					if ((priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) && (!priv->pshare->is_40m_bw))
						pdesc->Dword2 |= set_desc(7 << TX_AmpduDstySHIFT);	// according to DWA-160 A2
					else
#endif
						pdesc->Dword2 |= set_desc(5 << TX_AmpduDstySHIFT);	// according to WN111v2
				}
				else {
					pdesc->Dword2 |= set_desc(((txcfg->pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_) << TX_AmpduDstySHIFT);
			}
			}
			//set Break
			if((txcfg->q_num >=1 && txcfg->q_num <=4)){
				if((txcfg->pstat != priv->pshare->CurPstat[txcfg->q_num-1])) {
#ifdef CONFIG_RTL_88E_SUPPORT
					if (GET_CHIP_VER(priv)==VERSION_8188E)
						pdesc->Dword2 |= set_desc(TXdesc_88E_BK);
					else
#endif
						pdesc->Dword1 |= set_desc(TX_BK);
					priv->pshare->CurPstat[txcfg->q_num-1] = txcfg->pstat;
				}				
			} else {
#ifdef CONFIG_RTL_88E_SUPPORT
				if (GET_CHIP_VER(priv)==VERSION_8188E)
					pdesc->Dword2 |= set_desc(TXdesc_88E_BK);
				else
#endif
					pdesc->Dword1 |= set_desc(TX_BK);
			}
		}

		// for STBC
		if (priv->pmib->dot11nConfigEntry.dot11nSTBC &&
			txcfg->pstat && (txcfg->pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_RX_STBC_CAP_)))
			pdesc->Dword4 |= set_desc(1 << TX_DataStbcSHIFT);
	}
	else	// legacy rate
	{
		txRate = get_rate_index_from_ieee_value((UINT8)txcfg->tx_rate);
		if (is_CCK_rate(txcfg->tx_rate) && (txcfg->tx_rate != 2)) {
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
				; // txfw->txshort = 0
			else {
				if (txcfg->pstat)
					bg_txshort = (priv->pmib->dot11RFEntry.shortpreamble) &&
									(txcfg->pstat->useShortPreamble);
				else
					bg_txshort = priv->pmib->dot11RFEntry.shortpreamble;
			}
		}

		if (priv->pshare->is_40m_bw) {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
				pdesc->Dword4 |= set_desc((2 << TX_DataScSHIFT) | (2 << TX_RtsScSHIFT));
			else
				pdesc->Dword4 |= set_desc((1 << TX_DataScSHIFT) | (1 << TX_RtsScSHIFT));	
		}

		if (bg_txshort)
			pdesc->Dword4 |= set_desc(TX_DataShort);
	}

	if (txcfg->need_ack) { // unicast
		if (frag_idx == 0) {
			if ((txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len)) ||
				(txcfg->pstat && txcfg->pstat->is_forced_rts))
				pdesc->Dword4 |= set_desc(TX_RtsEn);
			else {
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					is_MCS_rate(txcfg->tx_rate) &&
					(priv->ht_protection /*|| txcfg->pstat->is_rtl8190_sta*/))
				{
					n_protection = 1;
					if (priv->pmib->dot11ErpInfo.protection)
						erp_protection = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						pdesc->Dword4 |= set_desc(TX_CTS2Self);
					else					
						pdesc->Dword4 |= set_desc(TX_RtsEn);
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
					(!is_CCK_rate(txcfg->tx_rate)) && // OFDM mode
					priv->pmib->dot11ErpInfo.protection)
				{
					erp_protection = 1;
					if (priv->pmib->dot11ErpInfo.ctsToSelf)
						pdesc->Dword4 |= set_desc(TX_CTS2Self);
					else						
						pdesc->Dword4 |= set_desc(TX_RtsEn);
				}
				else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
					(txcfg->pstat) && (txcfg->pstat->MIMO_ps & _HT_MIMO_PS_DYNAMIC_) &&
//					is_MCS_rate(txcfg->tx_rate) && ((txcfg->tx_rate & 0x7f)>7))
					is_2T_rate(txcfg->tx_rate))
				{	// when HT MIMO Dynamic power save is set and rate > MCS7, RTS is needed
					//Realtek for Intel 6205 IOT, Builder, 2013/01/29.
					//+++
					//pdesc->Dword4 |= set_desc(TX_RtsEn);
					//+++
				} else {
					/*
					 * Auto rts mode, use rts depends on packet length and packet tx time
					 */
#ifdef _OUTSRC_COEXIST
					if((IS_OUTSRC_CHIP(priv)&&(is_MCS_rate(txcfg->tx_rate) && (/*(txcfg->pstat->IOTPeer!=HT_IOT_PEER_INTEL) ||*/ !txcfg->pstat->no_rts))) 
						|| (!IS_OUTSRC_CHIP(priv) && (is_MCS_rate(txcfg->tx_rate) && (/*!txcfg->pstat->is_intel_sta ||*/ !txcfg->pstat->no_rts))))
#else
#ifdef USE_OUT_SRC	
					if (is_MCS_rate(txcfg->tx_rate) && (/*(txcfg->pstat->IOTPeer!=HT_IOT_PEER_INTEL) ||*/ !txcfg->pstat->no_rts))
#else
					if (is_MCS_rate(txcfg->tx_rate) && (/*!txcfg->pstat->is_intel_sta ||*/ !txcfg->pstat->no_rts))
#endif						
#endif
						pdesc->Dword4 |= set_desc(TX_HwRtsEn | TX_RtsEn);
				}
			}
		}
	}

	if (get_desc(pdesc->Dword4) & (TX_RtsEn | TX_CTS2Self)) {
		if (erp_protection)
			rate = (unsigned char)find_rate(priv, NULL, 1, 3);
		else
			rate = (unsigned char)find_rate(priv, NULL, 1, 1);

		if (is_MCS_rate(rate)) {	// HT rates
			// can we use HT rates for RTS?
		} else {
			unsigned int rtsTxRate  = 0;
			rtsTxRate = get_rate_index_from_ieee_value(rate);
			if (erp_protection) {
				unsigned char  rtsShort = 0;
				if (is_CCK_rate(rate) && (rate != 2)) {
					if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
							(priv->pmib->dot11ErpInfo.longPreambleStaNum > 0))
						rtsShort = 0; // do nothing
					else {
						if (txcfg->pstat)
							rtsShort = (priv->pmib->dot11RFEntry.shortpreamble) &&
											(txcfg->pstat->useShortPreamble);
						else
							rtsShort = priv->pmib->dot11RFEntry.shortpreamble;
					}
				}
				pdesc->Dword4 |= (rtsShort == 1)? set_desc(TX_RtsShort): 0;
			} 
#ifdef CONFIG_RTL_88E_SUPPORT
			else if ((GET_CHIP_VER(priv)==VERSION_8188E) && is_MCS_rate(txcfg->tx_rate)) {
				switch (txcfg->tx_rate & 0x7f) {
				case 0:
					rtsTxRate = get_rate_index_from_ieee_value(12);	// 6 Mbps
					break;
				case 1:
					rtsTxRate = get_rate_index_from_ieee_value(24);	// 12 Mbis
					break;
				case 2:
					rtsTxRate = get_rate_index_from_ieee_value(36);	// 18 Mbps
					break;
				default:
					rtsTxRate = get_rate_index_from_ieee_value(48);
					break;
				}
			} 
#endif
			else if (n_protection) {
				rtsTxRate = get_rate_index_from_ieee_value(48);
			}
			else {	// > RTS threshold
			}
			pdesc->Dword4 |= set_desc((rtsTxRate & TX_RtsRateMask) << TX_RtsRateSHIFT);
			pdesc->Dword5 |= set_desc((0xf & TX_RtsRateFBLmtMask) << TX_RtsRateFBLmtSHIFT);
			//8192SE Must specified BW mode while sending RTS ...
			if (priv->pshare->is_40m_bw)
				pdesc->Dword4 |= set_desc(TX_RtsBw);
		}
	}

	if(n_txshort == 1
#ifdef STA_EXT
		&& txcfg->pstat && (
#ifdef CONFIG_RTL_88E_SUPPORT
		(GET_CHIP_VER(priv)==VERSION_8188E)?(txcfg->pstat->remapped_aid < RTL8188E_NUM_STAT -1):
#endif
		(txcfg->pstat->remapped_aid < FW_NUM_STAT -1))
#endif
	)
		pdesc->Dword5 |= set_desc(TX_SGI);

#ifdef DRVMAC_LB
	if (priv->pmib->miscEntry.drvmac_lb && (priv->pmib->miscEntry.lb_mlmp == 4)) {
		txRate = rate_select;
		if (rate_select++ > 0x1b)
			rate_select = 0;

		pdesc->Dword4 |= set_desc(TX_UseRate);
		pdesc->Dword4 |= set_desc(TX_DisDataFB);
		pdesc->Dword4 |= set_desc(TX_DisRtsFB);// disable RTS fall back
	}
#endif

	if(priv->pshare->rf_ft_var.txforce != 0xff)
	{
		pdesc->Dword4 |= set_desc(TX_UseRate);
		pdesc->Dword5 |= set_desc((priv->pshare->rf_ft_var.txforce & TX_DataRateMask) << TX_DataRateSHIFT);
	}
	else
	pdesc->Dword5 |= set_desc((txRate & TX_DataRateMask) << TX_DataRateSHIFT);

#ifdef CONFIG_RTL_92D_SUPPORT
	if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) {
		pdesc->Dword4 |= set_desc((4 & TX_RtsRateMask) << TX_RtsRateSHIFT);
		//if (is_CCK_rate(txRate))
			//pdesc->Dword5 |= set_desc((4 & TX_DataRateMask) << TX_DataRateSHIFT);
	}
#endif

	if (txcfg->need_ack) {
		// give retry limit to management frame
		if (txcfg->q_num == MANAGE_QUE_NUM) {
			pdesc->Dword5 |= set_desc(TX_RtyLmtEn);
			if (GetFrameSubType(txcfg->phdr) == WIFI_PROBERSP) {
				;	// 0 no need to set
			}
#ifdef WDS
			else if ((GetFrameSubType(txcfg->phdr) == WIFI_PROBEREQ) && (txcfg->pstat->state & WIFI_WDS)) {
				pdesc->Dword5 |= set_desc((2 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
			}
#endif
			else {
				pdesc->Dword5 |= set_desc((6 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
			}
		}
#ifdef WDS
		else if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat->rx_avarage == 0) {
				pdesc->Dword5 |= set_desc(TX_RtyLmtEn);
				pdesc->Dword5 |= set_desc((3 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
			}
		}
#endif
#ifdef _OUTSRC_COEXIST
		else if(((IS_OUTSRC_CHIP(priv)&&(is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->IOTPeer==HT_IOT_PEER_INTEL))) 
					|| (!IS_OUTSRC_CHIP(priv) && (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->is_intel_sta) )))
#else
#ifdef USE_OUT_SRC	
		else if (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->IOTPeer==HT_IOT_PEER_INTEL) 
#else
		else if (is_MCS_rate(txcfg->pstat->current_tx_rate) && (txcfg->pstat->is_intel_sta) 
#endif			
#endif
			&& !(txcfg->pstat->leave) && priv->pshare->intel_rty_lmt) {
			pdesc->Dword5 |= set_desc(TX_RtyLmtEn);
			pdesc->Dword5 |= set_desc((priv->pshare->intel_rty_lmt & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
		}
#ifdef _OUTSRC_COEXIST
		else if((IS_OUTSRC_CHIP(priv)&&((txcfg->pstat->IOTPeer==HT_IOT_PEER_BROADCOM) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave)))
			|| (!IS_OUTSRC_CHIP(priv) && ((txcfg->pstat->is_broadcom_sta) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))))
#else
#ifdef USE_OUT_SRC	
		else if ((txcfg->pstat->IOTPeer==HT_IOT_PEER_BROADCOM) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))
#else
		else if ((txcfg->pstat->is_broadcom_sta) && (txcfg->pstat->retry_inc) && !(txcfg->pstat->leave))
#endif			
#endif
                {
                        pdesc->Dword5 |= set_desc(TX_RtyLmtEn);
                        pdesc->Dword5 |= set_desc((0x20 & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
                }

		// High power mechanism
		//if ((GET_CHIP_VER(priv) == VERSION_8192C)||(GET_CHIP_VER(priv) == VERSION_8188C))
		{
			if (priv->pshare->rf_ft_var.tx_pwr_ctrl && txcfg->pstat && (txcfg->fr_type == _SKB_FRAME_TYPE_)) {
				if ((txcfg->pstat->hp_level == 1)
#if defined(CONFIG_RTL_92D_SUPPORT) && defined(CONFIG_RTL_NOISE_CONTROL)
					|| ((priv->pshare->DNC_on) && (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)) 
#endif
					) {
						int pwr = (priv->pshare->rf_ft_var.min_pwr_idex > 16) ? 16: priv->pshare->rf_ft_var.min_pwr_idex;
						pwr &= 0x1e;
						pdesc->Dword6 |= set_desc(((-pwr) & TX_TxAgcAMask) << TX_TxAgcASHIFT);
						pdesc->Dword6 |= set_desc(((-pwr) & TX_TxAgcBMask) << TX_TxAgcBSHIFT);
					}
			}
		}
	}
}


#ifdef TX_EARLY_MODE
__MIPS16
__IRAM_IN_865X
static void insert_emcontent(struct rtl8192cd_priv *priv, struct tx_insn* txcfg, unsigned char *buf)
{
	struct stat_info *pstat = txcfg->pstat;
	unsigned int dw[2];

	dw[0] = set_desc((pstat->empkt_num & 0xf) |
							(((pstat->empkt_len[0]+pstat->emextra_len) << 4) & 0xfff0) |
							(((pstat->empkt_len[1]+pstat->emextra_len) << 16) & 0xfff0000) |
							(((pstat->empkt_len[2]+pstat->emextra_len) << 28) & 0xf0000000) 	
							);
	dw[1] = set_desc((((pstat->empkt_len[2]+pstat->emextra_len) >> 4) & 0xff) |
							(((pstat->empkt_len[3]+pstat->emextra_len) << 8) & 0xfff00) |							
							(((pstat->empkt_len[4]+pstat->emextra_len) << 20) & 0xfff00000) 
							);
	memcpy(buf, dw, 8);
}
#endif


#ifdef CONFIG_RTL_8812_SUPPORT

#ifdef CONFIG_RTK_MESH
int rtl8192cd_signin_txdesc_8812(struct rtl8192cd_priv *priv, struct tx_insn* txcfg, struct wlan_ethhdr_t *wlan_ethhdr_p)
#else
int rtl8192cd_signin_txdesc_8812(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
#endif
{
	struct tx_desc		*phdesc, *pdesc, *pndesc, *picvdesc, *pmicdesc, *pfrstdesc;
	struct tx_desc_info	*pswdescinfo, *pdescinfo, *pndescinfo, *picvdescinfo, *pmicdescinfo;
	unsigned int 		fr_len, tx_len, i, keyid;
	int					*tx_head, q_num;
	unsigned long		tmpphyaddr;
	unsigned char		*da, *pbuf, *pwlhdr, *pmic, *picv;
	struct rtl8192cd_hw	*phw;
	unsigned char		 q_select;
#ifdef TX_SHORTCUT
	int					fit_shortcut=0, idx=0;
#endif
	unsigned int		pfrst_dma_desc=0;
	unsigned int		*dma_txhead;

	unsigned long		flush_addr[20];
	int					flush_len[20];
	int					flush_num=0;

	picvdesc=NULL;
	keyid=0;
	pmic=NULL;
	picv=NULL;
	q_select=0;

	if (txcfg->tx_rate == 0) {
		DEBUG_ERR("tx_rate=0!\n");
		txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
	}

	q_num = txcfg->q_num;

	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);

	tx_len = txcfg->fr_len;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	else
		pbuf = (unsigned char*)txcfg->pframe;

	da = get_da((unsigned char *)txcfg->phdr);

	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);

	// in case of default key, then find the key id
	if (GetPrivacy((txcfg->phdr)))
	{
#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat)
				keyid = txcfg->pstat->keyid;
			else
				keyid = 0;
		}
		else
#endif

#ifdef __DRAYTEK_OS__
		if (!IEEE8021X_FUN)
			keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		else {
			if (IS_MCAST(GetAddr1Ptr ((unsigned char *)txcfg->phdr)) || !txcfg->pstat)
				keyid = priv->pmib->dot11GroupKeysTable.keyid;
			else
				keyid = txcfg->pstat->keyid;
		}
#else

		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_40_PRIVACY_ ||
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_104_PRIVACY_) {
			if(IEEE8021X_FUN && txcfg->pstat) {
#ifdef A4_STA
				if (IS_MCAST(da) && !(txcfg->pstat->state & WIFI_A4_STA))
#else
				if(IS_MCAST(da))
#endif					
					keyid = 0;
				else
					keyid = txcfg->pstat->keyid;
			}
			else
				keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		}
#endif


	}

	for(i=0, pfrstdesc= phdesc + (*tx_head); i < txcfg->frg_num; i++)
	{
		/*------------------------------------------------------------*/
		/*           fill descriptor of header + iv + llc             */
		/*------------------------------------------------------------*/
		pdesc     = phdesc + (*tx_head);
		pdescinfo = pswdescinfo + *tx_head;

		//clear all bits

		memset(pdesc, 0, 40);

		if (i != 0)
		{
			// we have to allocate wlan_hdr
			pwlhdr = (UINT8 *)get_wlanhdr_from_poll(priv);
			if (pwlhdr == (UINT8 *)NULL)
			{
				DEBUG_ERR("System-bug... should have enough wlan_hdr\n");
				return (txcfg->frg_num - i);
			}
			// other MPDU will share the same seq with the first MPDU
			memcpy((void *)pwlhdr, (void *)(txcfg->phdr), txcfg->hdr_len); // data pkt has 24 bytes wlan_hdr
			pdesc->Dword9 |= set_desc((GetSequence(txcfg->phdr) & TXdesc_92E_TX_SeqMask)  << TXdesc_92E_TX_SeqSHIFT);
		}
		else
		{
#ifdef WIFI_WMM
			if (txcfg->pstat && (is_qos_data(txcfg->phdr))) {
				if ((GetFrameSubType(txcfg->phdr) & (WIFI_DATA_TYPE | BIT(6) | BIT(7))) == (WIFI_DATA_TYPE | BIT(7))) {
					unsigned char tempQosControl[2];
					memset(tempQosControl, 0, 2);
					tempQosControl[0] = ((struct sk_buff *)txcfg->pframe)->cb[1];
#ifdef WMM_APSD
					if (
#ifdef CLIENT_MODE
						(OPMODE & WIFI_AP_STATE) &&
#endif
						(APSD_ENABLE) && (txcfg->pstat) && (txcfg->pstat->state & WIFI_SLEEP_STATE) &&
						(!GetMData(txcfg->phdr)) &&
						((((tempQosControl[0] == 7) || (tempQosControl[0] == 6)) && (txcfg->pstat->apsd_bitmap & 0x01)) ||
						 (((tempQosControl[0] == 5) || (tempQosControl[0] == 4)) && (txcfg->pstat->apsd_bitmap & 0x02)) ||
						 (((tempQosControl[0] == 3) || (tempQosControl[0] == 0)) && (txcfg->pstat->apsd_bitmap & 0x08)) ||
						 (((tempQosControl[0] == 2) || (tempQosControl[0] == 1)) && (txcfg->pstat->apsd_bitmap & 0x04))))
						tempQosControl[0] |= BIT(4);
#endif
					if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
						tempQosControl[0] |= BIT(7);

					if (priv->pmib->dot11nConfigEntry.dot11nTxNoAck)
						tempQosControl[0] |= BIT(5);

					memcpy((void *)GetQosControl((txcfg->phdr)), tempQosControl, 2);
				}
			}
#endif

#ifdef BEAMFORMING_SUPPORT
			if(!txcfg->ndpa)
#endif	
			assign_wlanseq(GET_HW(priv), txcfg->phdr, txcfg->pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
				, txcfg->is_11s
#endif
				);

			pdesc->Dword9 |= set_desc((GetSequence(txcfg->phdr) & TXdesc_92E_TX_SeqMask)  << TXdesc_92E_TX_SeqSHIFT);
			pwlhdr = txcfg->phdr;
		}
#ifdef BEAMFORMING_SUPPORT
		if(!txcfg->ndpa)
#endif			
		SetDuration(pwlhdr, 0);

		rtl8192cd_fill_fwinfo_8812(priv, txcfg, pdesc, i);
#if 1
//#ifdef FOR_VHT5G_PF //for 8812 tx no ack
		if (priv->pmib->dot11nConfigEntry.dot11nTxNoAck){
			pdesc->Dword0 |= set_desc(TX_BMC);
			pdesc->Dword2 &= set_desc(~(TXdesc_92E_AggEn));
		}
#endif

#if 1
//#ifdef FOR_VHT5G_PF //for 8812 cca
		if(txcfg->pstat) //8812_11n_iot, only vht clnt support cca_rts
		if((priv->pshare->rf_ft_var.cca_rts) && (txcfg->pstat->vht_cap_len > 0))
		pdesc->Dword2 |= set_desc((priv->pshare->rf_ft_var.cca_rts & 0x03) << TX_8812_CcaRtsSHIFT);	//10b:  RTS support dynamic mode CCA secondary
#endif


#ifdef HW_ANT_SWITCH
		pdesc->Dword2 &= set_desc(~ BIT(24));
		pdesc->Dword2 &= set_desc(~ BIT(25));
		if(!(priv->pshare->rf_ft_var.CurAntenna & 0x80) && (txcfg->pstat)) {
			pdesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<24);
			pdesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<25);
		}
#endif


		pdesc->Dword0 |= set_desc(40 << TX_OffsetSHIFT); // tx_desc size
		
		if (IS_MCAST(GetAddr1Ptr((unsigned char *)txcfg->phdr)))
			pdesc->Dword0 |= set_desc(TX_BMC);

#ifdef CLIENT_MODE
		if (OPMODE & WIFI_STATION_STATE) {
			if (GetFrameSubType(txcfg->phdr) == WIFI_PSPOLL)
				pdesc->Dword3 |= set_desc(TXdesc_92E_NAVUSEHDR);


			if (priv->ps_state)
				SetPwrMgt(pwlhdr);
			else
				ClearPwrMgt(pwlhdr);
		}
#endif
#ifdef BEAMFORMING_SUPPORT
		if(txcfg->ndpa)
			pdesc->Dword3 |= set_desc(TXdesc_92E_NAVUSEHDR);
#endif		

		if (i != (txcfg->frg_num - 1))
		{
			SetMFrag(pwlhdr);
			if (i == 0) {
				fr_len = (txcfg->frag_thrshld - txcfg->llc);
				tx_len -= (txcfg->frag_thrshld - txcfg->llc);
			}
			else {
				fr_len = txcfg->frag_thrshld;
				tx_len -= txcfg->frag_thrshld;
			}
		}
		else	// last seg, or the only seg (frg_num == 1)
		{
			fr_len = tx_len;
			ClearMFrag(pwlhdr);
		}
		SetFragNum((pwlhdr), i);

		if ((i == 0) && (txcfg->fr_type == _SKB_FRAME_TYPE_))
		{
			pdesc->Dword7 |= set_desc((txcfg->hdr_len + txcfg->llc) << TX_TxBufSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
			pdescinfo->type = _PRE_ALLOCLLCHDR_;
		}
		else
		{
			pdesc->Dword7 |= set_desc(txcfg->hdr_len << TX_TxBufSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
			pdescinfo->type = _PRE_ALLOCHDR_;
		}

#ifdef _11s_TEST_MODE_
		mesh_debug_tx9(txcfg, pdescinfo);
#endif

		switch (q_num) {
		case HIGH_QUEUE:
			q_select = 0x11;// High Queue
			break;
		case MGNT_QUEUE:
			q_select = 0x12;
			break;
#if defined(DRVMAC_LB) && defined(WIFI_WMM)
		case BE_QUEUE:
			q_select = 0;
			break;
#endif
		default:
			// data packet
#ifdef RTL_MANUAL_EDCA
			if (priv->pmib->dot11QosEntry.ManualEDCA) {
				switch (q_num) {
				case VO_QUEUE:
					q_select = 6;
					break;
				case VI_QUEUE:
					q_select = 4;
			break;
				case BE_QUEUE:
					q_select = 0;
				break;
			default:
					q_select = 1;
					break;
				}
			}
			else
#endif
			q_select = ((struct sk_buff *)txcfg->pframe)->cb[1];
				break;
		}
		pdesc->Dword1 |= set_desc((q_select & TX_QSelMask)<< TX_QSelSHIFT);

		if (i != (txcfg->frg_num - 1))
			pdesc->Dword2 |= set_desc(TX_MoreFrag);

// Set RateID
		if (txcfg->pstat) {
			if (txcfg->pstat->aid != MANAGEMENT_AID) {
				u8 ratid;

				ratid = txcfg->pstat->ratr_idx;
								
				pdesc->Dword1 |= set_desc((ratid & TXdesc_92E_RateIDMask) << TX_RateIDSHIFT);
				
	// Set MacID
				pdesc->Dword1 |= set_desc(REMAP_AID(txcfg->pstat) & TXdesc_92E_MacIdMask);

#ifdef BEAMFORMING_SUPPORT
				if (priv->pmib->dot11RFEntry.txbf == 1) {
					pdesc->Dword2 |= set_desc((txcfg->pstat->p_aid & TX_8812_PAIDMask) << TX_8812_PAIDSHIFT);
					pdesc->Dword2 |= set_desc((txcfg->pstat->g_id & TX_8812_GIDMask) << TX_8812_GIDSHIFT);
				}
#endif
			}
		} else {
			pdesc->Dword1 |= set_desc((ARFR_BMC &TXdesc_92E_RateIDMask)<<TX_RateIDSHIFT);
	}
	
		pdesc->Dword4 |= set_desc((0x1f & TXdesc_92E_DataRateFBLmtMask) << TXdesc_92E_DataRateFBLmtSHIFT);
		
		if (txcfg->fixed_rate) {
			pdesc->Dword3 |= set_desc(TXdesc_92E_DisDataFB|TXdesc_92E_DisRtsFB|TXdesc_92E_UseRate);
		}

#ifdef STA_EXT
		if(txcfg->pstat && 
			(txcfg->pstat->remapped_aid == RTL8812_NUM_STAT-1/*(priv->pshare->STA_map & BIT(txcfg->pstat->aid)*/))
			{
			pdesc->Dword3 |= set_desc(TXdesc_92E_UseRate);
			}
#endif

		if (!txcfg->need_ack && txcfg->privacy && UseSwCrypto(priv, NULL, TRUE))
			pdesc->Dword1 &= set_desc( ~(TX_SecTypeMask<< TX_SecTypeSHIFT));

		if (txcfg->privacy) {
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE))) {
				pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0)+ txcfg->icv + txcfg->mic + txcfg->iv);
				pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7)+ txcfg->iv);
			} else {
				// hw encrypt
				switch(txcfg->privacy) {
				case _WEP_104_PRIVACY_:
				case _WEP_40_PRIVACY_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					wep_fill_iv(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x1 << TX_SecTypeSHIFT);
					break;

				case _TKIP_PRIVACY_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv + txcfg->mic);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					tkip_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x1 << TX_SecTypeSHIFT);
					break;
				#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
				case _WAPI_SMS4_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					
					pdesc->Dword1 |= set_desc(0x2 << TX_SecTypeSHIFT);
					break;
				#endif
				case _CCMP_PRIVACY_:
					//michal also hardware...
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					aes_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x3 << TX_SecTypeSHIFT);
					break;

				default:
					DEBUG_ERR("Unknow privacy\n");
					break;
				}
			}
		}

#ifdef TX_EARLY_MODE
		if (GET_TX_EARLY_MODE && pwlhdr && i == 0) {
			pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xff00ffff) | (0x28 << TX_OffsetSHIFT));
			pdesc->Dword1 = set_desc(get_desc(pdesc->Dword1) | (1 << TX_PktOffsetSHIFT));
			pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + 8);

			memset(pwlhdr-8, '\0', 8);			
			if (txcfg->pstat && txcfg->pstat->empkt_num > 0) 			
				insert_emcontent(priv, txcfg, pwlhdr-8);

			pdesc->Dword10 = set_desc(get_physical_addr(priv, pwlhdr-8,
				get_desc(pdesc->Dword7)&TX_TxBufSizeMask, PCI_DMA_TODEVICE));	
		}
		else
#endif
		{
			pdesc->Dword10 = set_desc(get_physical_addr(priv, pwlhdr,
				(get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE));

		}

		// below is for sw desc info
		pdescinfo->paddr  = get_desc(pdesc->Dword10);
		pdescinfo->pframe = pwlhdr;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = txcfg->pstat;
#endif

#ifdef TX_SHORTCUT
		if (!priv->pmib->dot11OperationEntry.disable_txsc && txcfg->pstat &&
				(txcfg->fr_type == _SKB_FRAME_TYPE_) &&
				(txcfg->frg_num == 1) &&
				((txcfg->privacy == 0)
#ifdef CONFIG_RTL_WAPI_SUPPORT
				|| (txcfg->privacy == _WAPI_SMS4_)
#endif
				|| (txcfg->privacy && !UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))) &&
				!GetMData(txcfg->phdr) ) {

#ifdef CONFIG_RTK_MESH
			if( txcfg->is_11s&0x1) {
				if( wlan_ethhdr_p ) {
					idx = get_tx_sc_free_entry(priv, txcfg->pstat, wlan_ethhdr_p);
				} else {
					txsc_debug("rtl8192cd_signin_txdesc invoked with NULL pointer\n");
					idx = get_tx_sc_free_entry(priv, txcfg->pstat, pbuf - sizeof(struct wlan_ethhdr_t));
				}
			} else
#endif
				idx = get_tx_sc_free_entry(priv, txcfg->pstat, pbuf - sizeof(struct wlan_ethhdr_t));

#ifdef CONFIG_RTK_MESH
			if( txcfg->is_11s&0x1) {
				if( wlan_ethhdr_p ) {
					memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, wlan_ethhdr_p, sizeof(struct wlan_ethhdr_t));
				} else {
					memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, pbuf - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));
				}
			} else
#endif
				memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, pbuf - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));

			memcpy(&txcfg->pstat->tx_sc_ent[idx].hwdesc1, pdesc, 40);
			descinfo_copy(&txcfg->pstat->tx_sc_ent[idx].swdesc1, pdescinfo);
			txcfg->pstat->protection = priv->pmib->dot11ErpInfo.protection;
			txcfg->pstat->ht_protection = priv->ht_protection;
			txcfg->pstat->tx_sc_ent[idx].sc_keyid = keyid;
			txcfg->pstat->tx_sc_ent[idx].pktpri = ((struct sk_buff *)txcfg->pframe)->cb[1];
			fit_shortcut = 1;
		}
		else {
			if (txcfg->pstat) {
				for (idx=0; idx<TX_SC_ENTRY_NUM; idx++)
					txcfg->pstat->tx_sc_ent[idx].hwdesc1.Dword7 &= ~TX_TxBufSizeMask;
			}
		}
#endif

		pfrst_dma_desc = dma_txhead[*tx_head];

		if (i != 0) {
			pdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
			rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		}

#ifdef USE_RTL8186_SDK
		flush_addr[flush_num]  = (unsigned long)bus_to_virt(get_desc(pdesc->Dword10));
		flush_len[flush_num++]= (get_desc(pdesc->Dword7) & TX_TxBufSizeMask);
#endif

/*
		//printk desc content
		{
			unsigned int *ppdesc = (unsigned int *)pdesc;
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc)), get_desc(*(ppdesc+1)), get_desc(*(ppdesc+2)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+3)), get_desc(*(ppdesc+4)), get_desc(*(ppdesc+5)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+6)), get_desc(*(ppdesc+7)), get_desc(*(ppdesc+8)));
			printk("%08x\n", *(ppdesc+9));
			printk("===================================================\n");
		}
*/

		txdesc_rollover(pdesc, (unsigned int *)tx_head);

		if (txcfg->fr_len == 0)
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				pdesc->Dword0 |= set_desc(TX_LastSeg);
			goto init_deschead;
		}

		/*------------------------------------------------------------*/
		/*              fill descriptor of frame body                 */
		/*------------------------------------------------------------*/
		pndesc     = phdesc + *tx_head;
		pndescinfo = pswdescinfo + *tx_head;
		//clear all bits

		memset(pndesc, 0,40);
		pndesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | (TX_OWN));


#ifdef HW_ANT_SWITCH
		pndesc->Dword2 &= set_desc(~ BIT(24));
		pndesc->Dword2 &= set_desc(~ BIT(25));
		if(!(priv->pshare->rf_ft_var.CurAntenna & 0x80) && (txcfg->pstat)) {
			pndesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<24);
			pndesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<25);
		}
#endif


		if (txcfg->privacy)
		{
			if (txcfg->privacy == _WAPI_SMS4_)
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
					pndesc->Dword0 |= set_desc(TX_LastSeg);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
			else if (!UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
					pndesc->Dword0 |= set_desc(TX_LastSeg);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
		}
		else
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				pndesc->Dword0 |= set_desc(TX_LastSeg);
			pndescinfo->pstat = txcfg->pstat;
			pndescinfo->rate = txcfg->tx_rate;
		}

#ifdef CONFIG_RTL_WAPI_SUPPORT
#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
		if ((txcfg->privacy == _WAPI_SMS4_)&&(UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE))))
#else
		if (txcfg->privacy == _WAPI_SMS4_)
#endif
		{
			pndesc->Dword7 |= set_desc( (fr_len+SMS4_MIC_LEN) & TX_TxBufSizeMask);
		}
		else
#endif
		pndesc->Dword7 |= set_desc(fr_len & TX_TxBufSizeMask);

		if (i == 0)
			pndescinfo->type = txcfg->fr_type;
		else
			pndescinfo->type = _RESERVED_FRAME_TYPE_;

#if defined(CONFIG_RTK_MESH) && defined(MESH_USE_METRICOP)
		if( (txcfg->fr_type == _PRE_ALLOCMEM_) && (txcfg->is_11s & 128)) // for 11s link measurement frame
			pndescinfo->type =_RESERVED_FRAME_TYPE_;
#endif

#ifdef _11s_TEST_MODE_
		mesh_debug_tx10(txcfg, pndescinfo);
#endif


		pndesc->Dword10 = set_desc(tmpphyaddr); //TxBufferAddr
		pndescinfo->paddr = get_desc(pndesc->Dword10);
		pndescinfo->pframe = txcfg->pframe;
		pndescinfo->len = txcfg->fr_len;	// for pci_unmap_single
		pndescinfo->priv = priv;

		pbuf += fr_len;
		tmpphyaddr += fr_len;

#ifdef TX_SHORTCUT
		if (fit_shortcut) {
			memcpy(&txcfg->pstat->tx_sc_ent[idx].hwdesc2, pndesc, 40);
			descinfo_copy(&txcfg->pstat->tx_sc_ent[idx].swdesc2, pndescinfo);
		}
#endif

#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pndesc->Dword10));
		flush_len[flush_num++] = get_desc(pndesc->Dword7) & TX_TxBufSizeMask;

		txdesc_rollover(pndesc, (unsigned int *)tx_head);

		// retrieve H/W MIC and put in payload
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if (txcfg->privacy == _WAPI_SMS4_)
		{
			SecSWSMS4Encryption(priv, txcfg);
#if 0
			if (txcfg->mic>0)
			{
				pndesc->Dword7 &= (~TX_TxBufSizeMask);
				pndesc->Dword7 |= set_desc((fr_len+txcfg->mic)& TX_TxBufSizeMask);
				flush_len[flush_num-1]= (get_desc(pndesc->Dword7) & TX_TxBufSizeMask);
			}
			else
			{
				txcfg->mic = SMS4_MIC_LEN;
			}
#endif
		
		} else
#endif

		if ((txcfg->privacy == _TKIP_PRIVACY_) &&
			(priv->pshare->have_hw_mic) &&
			!(priv->pmib->dot11StationConfigEntry.swTkipMic) &&
			(i == (txcfg->frg_num-1)) )	// last segment
		{
			register unsigned long int l,r;
			unsigned char *mic;
			volatile int i;

			while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0)
				for (i=0; i<10; i++)
					;

			l = *(volatile unsigned int *)GDMAICVL;
			r = *(volatile unsigned int *)GDMAICVR;

			mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
			mic[0] = (unsigned char)(l & 0xff);
			mic[1] = (unsigned char)((l >> 8) & 0xff);
			mic[2] = (unsigned char)((l >> 16) & 0xff);
			mic[3] = (unsigned char)((l >> 24) & 0xff);
			mic[4] = (unsigned char)(r & 0xff);
			mic[5] = (unsigned char)((r >> 8) & 0xff);
			mic[6] = (unsigned char)((r >> 16) & 0xff);
			mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
			if (priv->micerr_flag) {
				mic[7] ^= mic[7];
				priv->micerr_flag = 0;
			}
#endif
		}


		/*------------------------------------------------------------*/
		/*                insert sw encrypt here!                     */
		/*------------------------------------------------------------*/
		if (txcfg->privacy && UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
		{
			if (txcfg->privacy == _TKIP_PRIVACY_ ||
				txcfg->privacy == _WEP_40_PRIVACY_ ||
				txcfg->privacy == _WEP_104_PRIVACY_)
			{
				picvdesc     = phdesc + *tx_head;
				picvdescinfo = pswdescinfo + *tx_head;
				//clear all bits
				memset(picvdesc, 0,32);

				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST){
					picvdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
				}
				else{
					picvdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);
				}

				picvdesc->Dword7 |= (set_desc(txcfg->icv & TX_TxBufSizeMask)); //TxBufferSize

				// append ICV first...
				picv = get_icv_from_poll(priv);
				if (picv == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc picv\n");
					BUG();
				}

				picvdescinfo->type = _PRE_ALLOCICVHDR_;
				picvdescinfo->pframe = picv;
				picvdescinfo->pstat = txcfg->pstat;
				picvdescinfo->rate = txcfg->tx_rate;
				picvdescinfo->priv = priv;
				//TxBufferAddr

				picvdesc->Dword10 = set_desc(get_physical_addr(priv, picv, txcfg->icv, PCI_DMA_TODEVICE));

				if (i == 0)
					tkip_icv(picv,
						pwlhdr + txcfg->hdr_len + txcfg->iv, txcfg->llc,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask));
				else
					tkip_icv(picv,
						NULL, 0,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask));

				if ((i == 0) && (txcfg->llc != 0)) {
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 8, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 4, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv,
							txcfg->privacy);
				}
				else { // not first segment or no snap header
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv,
							txcfg->privacy);
				}
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

				flush_addr[flush_num]  = (unsigned long)bus_to_virt(get_desc(picvdesc->Dword10));
				flush_len[flush_num++]=(get_desc(picvdesc->Dword7) & TX_TxBufSizeMask);

				txdesc_rollover(picvdesc, (unsigned int *)tx_head);
			}

			else if (txcfg->privacy == _CCMP_PRIVACY_)
			{
				pmicdesc = phdesc + *tx_head;
				pmicdescinfo = pswdescinfo + *tx_head;

				//clear all bits
				memset(pmicdesc, 0,32);
				
				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
					pmicdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
				else
				  pmicdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);

				// set TxBufferSize
				pmicdesc->Dword7 = set_desc(txcfg->mic & TX_TxBufSizeMask);

				// append MIC first...
				pmic = get_mic_from_poll(priv);
				if (pmic == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc pmic\n");
					BUG();
				}

				pmicdescinfo->type = _PRE_ALLOCMICHDR_;
				pmicdescinfo->pframe = pmic;
				pmicdescinfo->pstat = txcfg->pstat;
				pmicdescinfo->rate = txcfg->tx_rate;
				pmicdescinfo->priv = priv;
				// set TxBufferAddr
				pmicdesc->Dword10= set_desc(get_physical_addr(priv, pmic, txcfg->mic, PCI_DMA_TODEVICE));

				// then encrypt all (including ICV) by AES
				if ((i == 0)&&(txcfg->llc != 0)) // encrypt 3 segments ==> llc, mpdu, and mic
				{
					aesccmp_encrypt(priv, pwlhdr, pwlhdr + txcfg->hdr_len + 8,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), pmic);
				}
				else // encrypt 2 segments ==> mpdu and mic
					aesccmp_encrypt(priv, pwlhdr, NULL,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), pmic);
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
				rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pmicdesc->Dword10));
				flush_len[flush_num++]= (get_desc(pmicdesc->Dword7) & TX_TxBufSizeMask);

				txdesc_rollover(pmicdesc, (unsigned int *)tx_head);
			}
		}
	}


init_deschead:
#if 0
	switch (q_select) {
	case 0:
	case 3:
	   if (q_num != BE_QUEUE)
    		printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 1:
	case 2:
		if (q_num != BK_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 4:
	case 5:
		if (q_num != VI_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 6:
	case 7:
		if (q_num != VO_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x11 :
		 if (q_num != HIGH_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x12 :
		if (q_num != MGNT_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	default :
		printk("%s %d warning : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	break;
	}
#endif

	for (i=0; i<flush_num; i++)
		rtl_cache_sync_wback(priv, flush_addr[i], flush_len[i], PCI_DMA_TODEVICE);

	if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
		priv->amsdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
		priv->amsdu_first_dma_desc = pfrst_dma_desc;
#endif
		priv->amsdu_len = get_desc(pfrstdesc->Dword0) & 0xffff; // get pktSize
		return 0;
	}

	pfrstdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

	if (q_num == HIGH_QUEUE) {
//		priv->pshare->pkt_in_hiQ = 1;
		priv->pkt_in_hiQ = 1;

		return 0;
	} else {
		tx_poll(priv, q_num);
	}

	return 0;
}
#endif

// I AM not sure that if our Buffersize and PKTSize is right,
// If there are any problem, fix this first
#ifdef CONFIG_RTK_MESH
int rtl8192cd_signin_txdesc(struct rtl8192cd_priv *priv, struct tx_insn* txcfg, struct wlan_ethhdr_t *wlan_ethhdr_p)
#else
int rtl8192cd_signin_txdesc(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
#endif
{
	struct tx_desc		*phdesc, *pdesc, *pndesc, *picvdesc, *pmicdesc, *pfrstdesc;
	struct tx_desc_info	*pswdescinfo, *pdescinfo, *pndescinfo, *picvdescinfo, *pmicdescinfo;
	unsigned int 		fr_len, tx_len, i, keyid;
	int					*tx_head, q_num;
	unsigned long		tmpphyaddr;
	unsigned char		*da, *pbuf, *pwlhdr, *pmic, *picv;
	struct rtl8192cd_hw	*phw;
	unsigned char		 q_select;
#ifdef TX_SHORTCUT
	int					fit_shortcut=0, idx=0;
#endif
	unsigned int		pfrst_dma_desc=0;
	unsigned int		*dma_txhead;

	unsigned long		flush_addr[20];
	int					flush_len[20];
	int					flush_num=0;


#ifdef CONFIG_RTL_8812_SUPPORT
	if(GET_CHIP_VER(priv)== VERSION_8812E){
#ifdef CONFIG_RTK_MESH
		return rtl8192cd_signin_txdesc_8812(priv, txcfg, wlan_ethhdr_p);
#else
		return rtl8192cd_signin_txdesc_8812(priv, txcfg);
#endif
	}
#endif

	picvdesc=NULL;
	keyid=0;
	pmic=NULL;
	picv=NULL;
	q_select=0;

	if (txcfg->tx_rate == 0) {
		DEBUG_ERR("tx_rate=0!\n");
		txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
	}

	q_num = txcfg->q_num;

	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);

	tx_len = txcfg->fr_len;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
		pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	else
		pbuf = (unsigned char*)txcfg->pframe;

	da = get_da((unsigned char *)txcfg->phdr);

	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);

	// in case of default key, then find the key id
	if (GetPrivacy((txcfg->phdr)))
	{
#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			if (txcfg->pstat)
				keyid = txcfg->pstat->keyid;
			else
				keyid = 0;
		}
		else
#endif

#ifdef __DRAYTEK_OS__
		if (!IEEE8021X_FUN)
			keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		else {
			if (IS_MCAST(GetAddr1Ptr ((unsigned char *)txcfg->phdr)) || !txcfg->pstat)
				keyid = priv->pmib->dot11GroupKeysTable.keyid;
			else
				keyid = txcfg->pstat->keyid;
		}
#else

		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_40_PRIVACY_ ||
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WEP_104_PRIVACY_) {
			if(IEEE8021X_FUN && txcfg->pstat) {
#ifdef A4_STA
				if (IS_MCAST(da) && !(txcfg->pstat->state & WIFI_A4_STA))
#else
				if(IS_MCAST(da))
#endif					
					keyid = 0;
				else
					keyid = txcfg->pstat->keyid;
			}
			else
				keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		}
#endif


	}

	for(i=0, pfrstdesc= phdesc + (*tx_head); i < txcfg->frg_num; i++)
	{
		/*------------------------------------------------------------*/
		/*           fill descriptor of header + iv + llc             */
		/*------------------------------------------------------------*/
		pdesc     = phdesc + (*tx_head);
		pdescinfo = pswdescinfo + *tx_head;

		//clear all bits
		memset(pdesc, 0, 32);

		if (i != 0)
		{
			// we have to allocate wlan_hdr
			pwlhdr = (UINT8 *)get_wlanhdr_from_poll(priv);
			if (pwlhdr == (UINT8 *)NULL)
			{
				DEBUG_ERR("System-bug... should have enough wlan_hdr\n");
				return (txcfg->frg_num - i);
			}
			// other MPDU will share the same seq with the first MPDU
			memcpy((void *)pwlhdr, (void *)(txcfg->phdr), txcfg->hdr_len); // data pkt has 24 bytes wlan_hdr
			pdesc->Dword3 |= set_desc((GetSequence(txcfg->phdr) & TX_SeqMask)  << TX_SeqSHIFT);
		}
		else
		{
#ifdef WIFI_WMM
			if (txcfg->pstat && (is_qos_data(txcfg->phdr))) {
				if ((GetFrameSubType(txcfg->phdr) & (WIFI_DATA_TYPE | BIT(6) | BIT(7))) == (WIFI_DATA_TYPE | BIT(7))) {
					unsigned char tempQosControl[2];
					memset(tempQosControl, 0, 2);
					tempQosControl[0] = ((struct sk_buff *)txcfg->pframe)->cb[1];
#ifdef WMM_APSD
					if (
#ifdef CLIENT_MODE
						(OPMODE & WIFI_AP_STATE) &&
#endif
						(APSD_ENABLE) && (txcfg->pstat) && (txcfg->pstat->state & WIFI_SLEEP_STATE) &&
						(!GetMData(txcfg->phdr)) &&
						((((tempQosControl[0] == 7) || (tempQosControl[0] == 6)) && (txcfg->pstat->apsd_bitmap & 0x01)) ||
						 (((tempQosControl[0] == 5) || (tempQosControl[0] == 4)) && (txcfg->pstat->apsd_bitmap & 0x02)) ||
						 (((tempQosControl[0] == 3) || (tempQosControl[0] == 0)) && (txcfg->pstat->apsd_bitmap & 0x08)) ||
						 (((tempQosControl[0] == 2) || (tempQosControl[0] == 1)) && (txcfg->pstat->apsd_bitmap & 0x04))))
						tempQosControl[0] |= BIT(4);
#endif
					if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
						tempQosControl[0] |= BIT(7);

					if (priv->pmib->dot11nConfigEntry.dot11nTxNoAck)
						tempQosControl[0] |= BIT(5);

					memcpy((void *)GetQosControl((txcfg->phdr)), tempQosControl, 2);
				}
			}
#endif

			assign_wlanseq(GET_HW(priv), txcfg->phdr, txcfg->pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
				, txcfg->is_11s
#endif
				);
			pdesc->Dword3 |= set_desc((GetSequence(txcfg->phdr) & TX_SeqMask)  << TX_SeqSHIFT);
			pwlhdr = txcfg->phdr;
		}
		SetDuration(pwlhdr, 0);

		rtl8192cd_fill_fwinfo(priv, txcfg, pdesc, i);

#ifdef HW_ANT_SWITCH
		pdesc->Dword2 &= set_desc(~ BIT(24));
		pdesc->Dword2 &= set_desc(~ BIT(25));
		if(!(priv->pshare->rf_ft_var.CurAntenna & 0x80) && (txcfg->pstat)) {
			pdesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<24);
			pdesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<25);
		}
#endif

		pdesc->Dword0 |= set_desc(32 << TX_OffsetSHIFT); // tx_desc size

		if (IS_MCAST(GetAddr1Ptr((unsigned char *)txcfg->phdr)))
			pdesc->Dword0 |= set_desc(TX_BMC);

#ifdef CLIENT_MODE
		if (OPMODE & WIFI_STATION_STATE) {
			if (GetFrameSubType(txcfg->phdr) == WIFI_PSPOLL)
				pdesc->Dword1 |= set_desc(TX_NAVUSEHDR);

			if (priv->ps_state)
				SetPwrMgt(pwlhdr);
			else
				ClearPwrMgt(pwlhdr);
		}
#endif

		if (i != (txcfg->frg_num - 1))
		{
			SetMFrag(pwlhdr);
			if (i == 0) {
				fr_len = (txcfg->frag_thrshld - txcfg->llc);
				tx_len -= (txcfg->frag_thrshld - txcfg->llc);
			}
			else {
				fr_len = txcfg->frag_thrshld;
				tx_len -= txcfg->frag_thrshld;
			}
		}
		else	// last seg, or the only seg (frg_num == 1)
		{
			fr_len = tx_len;
			ClearMFrag(pwlhdr);
		}
		SetFragNum((pwlhdr), i);

		if ((i == 0) && (txcfg->fr_type == _SKB_FRAME_TYPE_))
		{
			pdesc->Dword7 |= set_desc((txcfg->hdr_len + txcfg->llc) << TX_TxBufSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
			pdescinfo->type = _PRE_ALLOCLLCHDR_;
		}
		else
		{
			pdesc->Dword7 |= set_desc(txcfg->hdr_len << TX_TxBufSizeSHIFT);
			pdesc->Dword0 |= set_desc((fr_len + (get_desc(pdesc->Dword7) & TX_TxBufSizeMask)) << TX_PktSizeSHIFT);
			pdesc->Dword0 |= set_desc(TX_FirstSeg);
			pdescinfo->type = _PRE_ALLOCHDR_;
		}

#ifdef _11s_TEST_MODE_
		mesh_debug_tx9(txcfg, pdescinfo);
#endif

		switch (q_num) {
		case HIGH_QUEUE:
			q_select = 0x11;// High Queue
			break;
		case MGNT_QUEUE:
			q_select = 0x12;
			break;
#if defined(DRVMAC_LB) && defined(WIFI_WMM)
		case BE_QUEUE:
			q_select = 0;
			break;
#endif
		default:
			// data packet
#ifdef RTL_MANUAL_EDCA
			if (priv->pmib->dot11QosEntry.ManualEDCA) {
				switch (q_num) {
				case VO_QUEUE:
					q_select = 6;
					break;
				case VI_QUEUE:
					q_select = 4;
			break;
				case BE_QUEUE:
					q_select = 0;
				break;
			default:
					q_select = 1;
					break;
				}
			}
			else
#endif
			q_select = ((struct sk_buff *)txcfg->pframe)->cb[1];
				break;
		}
		pdesc->Dword1 |= set_desc((q_select & TX_QSelMask)<< TX_QSelSHIFT);

		if (i != (txcfg->frg_num - 1))
			pdesc->Dword2 |= set_desc(TX_MoreFrag);

// Set RateID
		if (txcfg->pstat) {
			if (txcfg->pstat->aid != MANAGEMENT_AID) {
				u8 ratid;

#ifdef CONFIG_RTL_92D_SUPPORT
				if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G){
					if (txcfg->pstat->tx_ra_bitmap & 0xffff000) {
						if (priv->pshare->is_40m_bw)
							ratid = ARFR_2T_Band_A_40M;
						else
							ratid = ARFR_2T_Band_A_20M;
					} else {
						ratid = ARFR_G_ONLY;
					}
				} else 
#endif
				{			
					if (txcfg->pstat->tx_ra_bitmap & 0xff00000) {
						if (priv->pshare->is_40m_bw)
							ratid = ARFR_2T_40M;
						else
							ratid = ARFR_2T_20M;
					} else if (txcfg->pstat->tx_ra_bitmap & 0xff000) {
						if (priv->pshare->is_40m_bw)
							ratid = ARFR_1T_40M;
						else
							ratid = ARFR_1T_20M;
					} else if (txcfg->pstat->tx_ra_bitmap & 0xff0) {
						ratid = ARFR_BG_MIX;
					} else {
						ratid = ARFR_B_ONLY;
					}


#ifdef P2P_SUPPORT	/*tx to GC no user B rate*/
					if(txcfg->pstat->is_p2p_client){
						switch(ratid) {
							case  ARFR_BG_MIX :
								ratid = ARFR_G_ONLY;
								break;
							default:
								ratid = ARFR_2T_Band_A_40M;		
						}
					}
#endif				
				}
				
				pdesc->Dword1 |= set_desc((ratid & TX_RateIDMask) << TX_RateIDSHIFT);
	// Set MacID
#ifdef CONFIG_RTL_88E_SUPPORT
				if (GET_CHIP_VER(priv)==VERSION_8188E)
					pdesc->Dword1 |= set_desc(REMAP_AID(txcfg->pstat) & TXdesc_88E_MacIdMask);
				else
#endif
					pdesc->Dword1 |= set_desc(REMAP_AID(txcfg->pstat) & TX_MacIdMask);
			}
		} else {
	
#ifdef CONFIG_RTL_92D_SUPPORT
		if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G)
			pdesc->Dword1 |= set_desc((ARFR_Band_A_BMC &TX_RateIDMask)<<TX_RateIDSHIFT);
		else
#endif
			pdesc->Dword1 |= set_desc((ARFR_BMC &TX_RateIDMask)<<TX_RateIDSHIFT);
	}

		pdesc->Dword5 |= set_desc((0x1f & TX_DataRateFBLmtMask) << TX_DataRateFBLmtSHIFT);
		if (txcfg->fixed_rate)
			pdesc->Dword4 |= set_desc(TX_DisDataFB|TX_DisRtsFB|TX_UseRate);
#ifdef CONFIG_RTL_88E_SUPPORT
		else if (GET_CHIP_VER(priv)==VERSION_8188E)
			pdesc->Dword4 |= set_desc(TX_UseRate);
#endif

#ifdef STA_EXT
		if(txcfg->pstat && (
#ifdef CONFIG_RTL_88E_SUPPORT
			(GET_CHIP_VER(priv)==VERSION_8188E)?(txcfg->pstat->remapped_aid == RTL8188E_NUM_STAT-1):
#endif
			(txcfg->pstat->remapped_aid == FW_NUM_STAT-1/*(priv->pshare->STA_map & BIT(txcfg->pstat->aid)*/)))
			pdesc->Dword4 |= set_desc(TX_UseRate);
#endif

		if (!txcfg->need_ack && txcfg->privacy && UseSwCrypto(priv, NULL, TRUE))
			pdesc->Dword1 &= set_desc( ~(TX_SecTypeMask<< TX_SecTypeSHIFT));

		if (txcfg->privacy) {
			if (UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE))) {
				pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0)+ txcfg->icv + txcfg->mic + txcfg->iv);
				pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7)+ txcfg->iv);
			} else {
				// hw encrypt
				switch(txcfg->privacy) {
				case _WEP_104_PRIVACY_:
				case _WEP_40_PRIVACY_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					wep_fill_iv(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x1 << TX_SecTypeSHIFT);
					break;

				case _TKIP_PRIVACY_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv + txcfg->mic);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					tkip_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x1 << TX_SecTypeSHIFT);
					break;
				#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
				case _WAPI_SMS4_:
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					
					pdesc->Dword1 |= set_desc(0x2 << TX_SecTypeSHIFT);
					break;
				#endif
				case _CCMP_PRIVACY_:
					//michal also hardware...
					pdesc->Dword0 = set_desc(get_desc(pdesc->Dword0) + txcfg->iv);
					pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + txcfg->iv);
					aes_fill_encheader(priv, pwlhdr, txcfg->hdr_len, keyid);
					pdesc->Dword1 |= set_desc(0x3 << TX_SecTypeSHIFT);
					break;

				default:
					DEBUG_ERR("Unknow privacy\n");
					break;
				}
			}
		}

#ifdef TX_EARLY_MODE
		if (GET_TX_EARLY_MODE && pwlhdr && i == 0) {
			pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xff00ffff) | (0x28 << TX_OffsetSHIFT));
			pdesc->Dword1 = set_desc(get_desc(pdesc->Dword1) | (1 << TX_PktOffsetSHIFT));
#ifdef CONFIG_RTL_88E_SUPPORT
			if (GET_CHIP_VER(priv) == VERSION_8188E) {
				pdesc->Dword6 &= set_desc(~ (0xf << TX_MaxAggNumSHIFT));
				pdesc->Dword6 |= set_desc(0xf << TX_MaxAggNumSHIFT);
			}
#endif
			pdesc->Dword7 = set_desc(get_desc(pdesc->Dword7) + 8);

			memset(pwlhdr-8, '\0', 8);			
			if (txcfg->pstat && txcfg->pstat->empkt_num > 0) 			
				insert_emcontent(priv, txcfg, pwlhdr-8);
			
			pdesc->Dword8 = set_desc(get_physical_addr(priv, pwlhdr-8,
				get_desc(pdesc->Dword7)&TX_TxBufSizeMask, PCI_DMA_TODEVICE));	
		}
		else
#endif
		{
#if defined(TX_EARLY_MODE) && defined(CONFIG_RTL_88E_SUPPORT)
			if (GET_CHIP_VER(priv) == VERSION_8188E)
				pdesc->Dword6 &= set_desc(~ (0xf << TX_MaxAggNumSHIFT));
#endif
			pdesc->Dword8 = set_desc(get_physical_addr(priv, pwlhdr,
				(get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE));
		}

		// below is for sw desc info
		pdescinfo->paddr  = get_desc(pdesc->Dword8);
		pdescinfo->pframe = pwlhdr;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = txcfg->pstat;
#endif

#ifdef TX_SHORTCUT
		if (!priv->pmib->dot11OperationEntry.disable_txsc && txcfg->pstat &&
				(txcfg->fr_type == _SKB_FRAME_TYPE_) &&
				(txcfg->frg_num == 1) &&
				((txcfg->privacy == 0)
#ifdef CONFIG_RTL_WAPI_SUPPORT
				|| (txcfg->privacy == _WAPI_SMS4_)
#endif
				|| (txcfg->privacy && !UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))) &&
				!GetMData(txcfg->phdr) ) {

#ifdef CONFIG_RTK_MESH
			if( txcfg->is_11s&0x1) {
				if( wlan_ethhdr_p ) {
					idx = get_tx_sc_free_entry(priv, txcfg->pstat, wlan_ethhdr_p);
				} else {
					txsc_debug("rtl8192cd_signin_txdesc invoked with NULL pointer\n");
					idx = get_tx_sc_free_entry(priv, txcfg->pstat, pbuf - sizeof(struct wlan_ethhdr_t));
				}
			} else
#endif
				idx = get_tx_sc_free_entry(priv, txcfg->pstat, pbuf - sizeof(struct wlan_ethhdr_t));

#ifdef CONFIG_RTK_MESH
			if( txcfg->is_11s&0x1) {
				if( wlan_ethhdr_p ) {
					memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, wlan_ethhdr_p, sizeof(struct wlan_ethhdr_t));
				} else {
					memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, pbuf - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));
				}
			} else
#endif
				memcpy((void *)&txcfg->pstat->tx_sc_ent[idx].ethhdr, pbuf - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));

			desc_copy(&txcfg->pstat->tx_sc_ent[idx].hwdesc1, pdesc);
			descinfo_copy(&txcfg->pstat->tx_sc_ent[idx].swdesc1, pdescinfo);
			txcfg->pstat->protection = priv->pmib->dot11ErpInfo.protection;
			txcfg->pstat->ht_protection = priv->ht_protection;
			txcfg->pstat->tx_sc_ent[idx].sc_keyid = keyid;
			txcfg->pstat->tx_sc_ent[idx].pktpri = ((struct sk_buff *)txcfg->pframe)->cb[1];
			fit_shortcut = 1;
		}
		else {
			if (txcfg->pstat) {
				for (idx=0; idx<TX_SC_ENTRY_NUM; idx++)
					txcfg->pstat->tx_sc_ent[idx].hwdesc1.Dword7 &= ~TX_TxBufSizeMask;
			}
		}
#endif

		pfrst_dma_desc = dma_txhead[*tx_head];

		if (i != 0) {
			pdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
			rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif
		}

#ifdef USE_RTL8186_SDK
		flush_addr[flush_num]  = (unsigned long)bus_to_virt(get_desc(pdesc->Dword8));
		flush_len[flush_num++]= (get_desc(pdesc->Dword7) & TX_TxBufSizeMask);
#endif

/*
		//printk desc content
		{
			unsigned int *ppdesc = (unsigned int *)pdesc;
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc)), get_desc(*(ppdesc+1)), get_desc(*(ppdesc+2)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+3)), get_desc(*(ppdesc+4)), get_desc(*(ppdesc+5)));
			printk("%08x    %08x    %08x \n", get_desc(*(ppdesc+6)), get_desc(*(ppdesc+7)), get_desc(*(ppdesc+8)));
			printk("%08x\n", *(ppdesc+9));
			printk("===================================================\n");
		}
*/

		txdesc_rollover(pdesc, (unsigned int *)tx_head);

		if (txcfg->fr_len == 0)
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				pdesc->Dword0 |= set_desc(TX_LastSeg);
			goto init_deschead;
		}

		/*------------------------------------------------------------*/
		/*              fill descriptor of frame body                 */
		/*------------------------------------------------------------*/
		pndesc     = phdesc + *tx_head;
		pndescinfo = pswdescinfo + *tx_head;
		//clear all bits
		memset(pndesc, 0,32);
		pndesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | (TX_OWN));


#ifdef HW_ANT_SWITCH
		pndesc->Dword2 &= set_desc(~ BIT(24));
		pndesc->Dword2 &= set_desc(~ BIT(25));
		if(!(priv->pshare->rf_ft_var.CurAntenna & 0x80) && (txcfg->pstat)) {
			pndesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<24);
			pndesc->Dword2 |= set_desc(((txcfg->pstat->CurAntenna^priv->pshare->rf_ft_var.CurAntenna)&1)<<25);
		}
#endif


		if (txcfg->privacy)
		{
			if (txcfg->privacy == _WAPI_SMS4_)
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
					pndesc->Dword0 |= set_desc(TX_LastSeg);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
			else if (!UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
			{
				if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
					pndesc->Dword0 |= set_desc(TX_LastSeg);
				pndescinfo->pstat = txcfg->pstat;
				pndescinfo->rate = txcfg->tx_rate;
			}
		}
		else
		{
			if (txcfg->aggre_en != FG_AGGRE_MSDU_FIRST)
				pndesc->Dword0 |= set_desc(TX_LastSeg);
			pndescinfo->pstat = txcfg->pstat;
			pndescinfo->rate = txcfg->tx_rate;
		}

#ifdef CONFIG_RTL_WAPI_SUPPORT
#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
		if ((txcfg->privacy == _WAPI_SMS4_)&&(UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE))))
#else
		if (txcfg->privacy == _WAPI_SMS4_)
#endif
		{
			pndesc->Dword7 |= set_desc( (fr_len+SMS4_MIC_LEN) & TX_TxBufSizeMask);
		}
		else
#endif
		pndesc->Dword7 |= set_desc(fr_len & TX_TxBufSizeMask);

		if (i == 0)
			pndescinfo->type = txcfg->fr_type;
		else
			pndescinfo->type = _RESERVED_FRAME_TYPE_;

#if defined(CONFIG_RTK_MESH) && defined(MESH_USE_METRICOP)
		if( (txcfg->fr_type == _PRE_ALLOCMEM_) && (txcfg->is_11s & 128)) // for 11s link measurement frame
			pndescinfo->type =_RESERVED_FRAME_TYPE_;
#endif

#ifdef _11s_TEST_MODE_
		mesh_debug_tx10(txcfg, pndescinfo);
#endif

		pndesc->Dword8 = set_desc(tmpphyaddr); //TxBufferAddr
		pndescinfo->paddr = get_desc(pndesc->Dword8);
		pndescinfo->pframe = txcfg->pframe;
		pndescinfo->len = txcfg->fr_len;	// for pci_unmap_single
		pndescinfo->priv = priv;

		pbuf += fr_len;
		tmpphyaddr += fr_len;

#ifdef TX_SHORTCUT
		if (fit_shortcut) {
			desc_copy(&txcfg->pstat->tx_sc_ent[idx].hwdesc2, pndesc);
			descinfo_copy(&txcfg->pstat->tx_sc_ent[idx].swdesc2, pndescinfo);
		}
#endif

#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
		rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

		flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pndesc->Dword8));
		flush_len[flush_num++] = get_desc(pndesc->Dword7) & TX_TxBufSizeMask;

		txdesc_rollover(pndesc, (unsigned int *)tx_head);

		// retrieve H/W MIC and put in payload
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if (txcfg->privacy == _WAPI_SMS4_)
		{
			SecSWSMS4Encryption(priv, txcfg);
#if 0
			if (txcfg->mic>0)
			{
				pndesc->Dword7 &= (~TX_TxBufSizeMask);
				pndesc->Dword7 |= set_desc((fr_len+txcfg->mic)& TX_TxBufSizeMask);
				flush_len[flush_num-1]= (get_desc(pndesc->Dword7) & TX_TxBufSizeMask);
			}
			else
			{
				txcfg->mic = SMS4_MIC_LEN;
			}
#endif
		
		} else
#endif

		if ((txcfg->privacy == _TKIP_PRIVACY_) &&
			(priv->pshare->have_hw_mic) &&
			!(priv->pmib->dot11StationConfigEntry.swTkipMic) &&
			(i == (txcfg->frg_num-1)) )	// last segment
		{
			register unsigned long int l,r;
			unsigned char *mic;
			volatile int i;

			while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0)
				for (i=0; i<10; i++)
					;

			l = *(volatile unsigned int *)GDMAICVL;
			r = *(volatile unsigned int *)GDMAICVR;

			mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
			mic[0] = (unsigned char)(l & 0xff);
			mic[1] = (unsigned char)((l >> 8) & 0xff);
			mic[2] = (unsigned char)((l >> 16) & 0xff);
			mic[3] = (unsigned char)((l >> 24) & 0xff);
			mic[4] = (unsigned char)(r & 0xff);
			mic[5] = (unsigned char)((r >> 8) & 0xff);
			mic[6] = (unsigned char)((r >> 16) & 0xff);
			mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
			if (priv->micerr_flag) {
				mic[7] ^= mic[7];
				priv->micerr_flag = 0;
			}
#endif
		}


		/*------------------------------------------------------------*/
		/*                insert sw encrypt here!                     */
		/*------------------------------------------------------------*/
		if (txcfg->privacy && UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
		{
			if (txcfg->privacy == _TKIP_PRIVACY_ ||
				txcfg->privacy == _WEP_40_PRIVACY_ ||
				txcfg->privacy == _WEP_104_PRIVACY_)
			{
				picvdesc     = phdesc + *tx_head;
				picvdescinfo = pswdescinfo + *tx_head;
				//clear all bits
				memset(picvdesc, 0,32);

				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST){
					picvdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
				}
				else{
					picvdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);
				}

				picvdesc->Dword7 |= (set_desc(txcfg->icv & TX_TxBufSizeMask)); //TxBufferSize

				// append ICV first...
				picv = get_icv_from_poll(priv);
				if (picv == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc picv\n");
					BUG();
				}

				picvdescinfo->type = _PRE_ALLOCICVHDR_;
				picvdescinfo->pframe = picv;
				picvdescinfo->pstat = txcfg->pstat;
				picvdescinfo->rate = txcfg->tx_rate;
				picvdescinfo->priv = priv;
				//TxBufferAddr
				picvdesc->Dword8 = set_desc(get_physical_addr(priv, picv, txcfg->icv, PCI_DMA_TODEVICE));

				if (i == 0)
					tkip_icv(picv,
						pwlhdr + txcfg->hdr_len + txcfg->iv, txcfg->llc,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask));
				else
					tkip_icv(picv,
						NULL, 0,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask));

				if ((i == 0) && (txcfg->llc != 0)) {
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 8, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len,
							pwlhdr + txcfg->hdr_len + 4, sizeof(struct llc_snap),
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv,
							txcfg->privacy);
				}
				else { // not first segment or no snap header
					if (txcfg->privacy == _TKIP_PRIVACY_)
						tkip_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv);
					else
						wep_encrypt(priv, pwlhdr, txcfg->hdr_len, NULL, 0,
							pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), picv, txcfg->icv,
							txcfg->privacy);
				}
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
				rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

				flush_addr[flush_num]  = (unsigned long)bus_to_virt(get_desc(picvdesc->Dword8));
				flush_len[flush_num++]=(get_desc(picvdesc->Dword7) & TX_TxBufSizeMask);

				txdesc_rollover(picvdesc, (unsigned int *)tx_head);
			}

			else if (txcfg->privacy == _CCMP_PRIVACY_)
			{
				pmicdesc = phdesc + *tx_head;
				pmicdescinfo = pswdescinfo + *tx_head;

				//clear all bits
				memset(pmicdesc, 0,32);
				
				if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
					pmicdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN);
				else
				  pmicdesc->Dword0   = set_desc((get_desc(pdesc->Dword0) & (~TX_FirstSeg)) | TX_OWN | TX_LastSeg);

				// set TxBufferSize
				pmicdesc->Dword7 = set_desc(txcfg->mic & TX_TxBufSizeMask);

				// append MIC first...
				pmic = get_mic_from_poll(priv);
				if (pmic == NULL)
				{
					DEBUG_ERR("System-Buf! can't alloc pmic\n");
					BUG();
				}

				pmicdescinfo->type = _PRE_ALLOCMICHDR_;
				pmicdescinfo->pframe = pmic;
				pmicdescinfo->pstat = txcfg->pstat;
				pmicdescinfo->rate = txcfg->tx_rate;
				pmicdescinfo->priv = priv;
				// set TxBufferAddr
				pmicdesc->Dword8= set_desc(get_physical_addr(priv, pmic, txcfg->mic, PCI_DMA_TODEVICE));

				// then encrypt all (including ICV) by AES
				if ((i == 0)&&(txcfg->llc != 0)) // encrypt 3 segments ==> llc, mpdu, and mic
				{
					aesccmp_encrypt(priv, pwlhdr, pwlhdr + txcfg->hdr_len + 8,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), pmic);
				}
				else // encrypt 2 segments ==> mpdu and mic
					aesccmp_encrypt(priv, pwlhdr, NULL,
						pbuf - (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), (get_desc(pndesc->Dword7) & TX_TxBufSizeMask), pmic);
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
				rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif
				flush_addr[flush_num]=(unsigned long)bus_to_virt(get_desc(pmicdesc->Dword8));
				flush_len[flush_num++]= (get_desc(pmicdesc->Dword7) & TX_TxBufSizeMask);

				txdesc_rollover(pmicdesc, (unsigned int *)tx_head);
			}
		}
	}


init_deschead:
#if 0
	switch (q_select) {
	case 0:
	case 3:
	   if (q_num != BE_QUEUE)
    		printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 1:
	case 2:
		if (q_num != BK_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	   break;
	case 4:
	case 5:
		if (q_num != VI_QUEUE)
		    printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 6:
	case 7:
		if (q_num != VO_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x11 :
		 if (q_num != HIGH_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	case 0x12 :
		if (q_num != MGNT_QUEUE)
			printk("%s %d error : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
		break;
	default :
		printk("%s %d warning : q_select[%d], q_num[%d]\n", __FUNCTION__, __LINE__, q_select, q_num);
	break;
	}
#endif

	for (i=0; i<flush_num; i++)
		rtl_cache_sync_wback(priv, flush_addr[i], flush_len[i], PCI_DMA_TODEVICE);

	if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
		priv->amsdu_first_desc = pfrstdesc;
#ifndef USE_RTL8186_SDK
		priv->amsdu_first_dma_desc = pfrst_dma_desc;
#endif
		priv->amsdu_len = get_desc(pfrstdesc->Dword0) & 0xffff; // get pktSize
		return 0;
	}

	pfrstdesc->Dword0 |= set_desc(TX_OWN);
#ifndef USE_RTL8186_SDK
#if defined(NOT_RTK_BSP)
#else
	rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(pfrst_dma_desc), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
#endif

	if (q_num == HIGH_QUEUE) {
//		priv->pshare->pkt_in_hiQ = 1;
		priv->pkt_in_hiQ = 1;

		return 0;
	} else {
		tx_poll(priv, q_num);
	}

	return 0;
}


#ifdef SUPPORT_TX_AMSDU
int rtl8192cd_signin_txdesc_amsdu(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
{
	struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info *pswdescinfo, *pdescinfo;
	unsigned int  tx_len;
	int *tx_head, q_num;
	unsigned long	tmpphyaddr;
	unsigned char *pbuf;
	struct rtl8192cd_hw *phw;
	unsigned int *dma_txhead;

	q_num = txcfg->q_num;
	phw	= GET_HW(priv);

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc   	= get_txdesc(phw, q_num);
	pswdescinfo = get_txdesc_info(priv->pshare->pdesc_info, q_num);

	pbuf = ((struct sk_buff *)txcfg->pframe)->data;
	tx_len = ((struct sk_buff *)txcfg->pframe)->len;
	tmpphyaddr = get_physical_addr(priv, pbuf, tx_len, PCI_DMA_TODEVICE);
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)pbuf, tx_len, PCI_DMA_TODEVICE);
#endif

	pdesc     = phdesc + (*tx_head);
	pdescinfo = pswdescinfo + *tx_head;

	//clear all bits
#ifdef CONFIG_RTL_8812_SUPPORT
	if(GET_CHIP_VER(priv)== VERSION_8812E)
	{
		memset(pdesc, 0, 40);					//clear all bits
		pdesc->Dword10 = set_desc(tmpphyaddr); // TXBufferAddr
		pdesc->Dword7 |= (set_desc(tx_len & TX_TxBufSizeMask));
		pdesc->Dword0 |= set_desc(40 << TX_OffsetSHIFT); // tx_desc size
	}
	else
#endif
	{
	memset(pdesc, 0, 32);
	pdesc->Dword8 = set_desc(tmpphyaddr); // TXBufferAddr
		pdesc->Dword7 |= (set_desc(tx_len & TX_TxBufSizeMask));
		pdesc->Dword0 |= set_desc(32 << TX_OffsetSHIFT); // tx_desc size
	}

	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST){
		pdesc->Dword0 = set_desc(TX_OWN | TX_LastSeg);
	}
	else
		pdesc->Dword0 = set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	pdescinfo->type = _SKB_FRAME_TYPE_;

#ifdef CONFIG_RTL_8812_SUPPORT
	if(GET_CHIP_VER(priv)== VERSION_8812E)
	pdescinfo->paddr = get_desc(pdesc->Dword10); // TXBufferAddr
	else
#endif
	pdescinfo->paddr = get_desc(pdesc->Dword8); // TXBufferAddr

	pdescinfo->pframe = txcfg->pframe;
	pdescinfo->len = txcfg->fr_len;
	pdescinfo->priv = priv;

	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	priv->amsdu_len += tx_len;

	if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
		pfrstdesc = priv->amsdu_first_desc;
		pfrstdesc->Dword0 = set_desc((get_desc(pfrstdesc->Dword0) &0xff0000) | priv->amsdu_len | TX_FirstSeg | TX_OWN);

#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, bus_to_virt(priv->amsdu_first_dma_desc), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		tx_poll(priv, q_num);
	}

	return 0;
}
#endif // SUPPORT_TX_AMSDU


#ifdef FW_SW_BEACON
int rtl8192cd_SendBeaconByCmdQ(struct rtl8192cd_priv *priv, unsigned char *dat_content, unsigned short txLength)
{
	struct tx_desc *pdesc;
	struct tx_desc *phdesc;
	int	*tx_head;
	struct rtl8192cd_hw *phw;

	phw = GET_HW(priv);
	tx_head	= &phw->txcmdhead;
	phdesc = phw->txcmd_desc;
	pdesc = phdesc + (*tx_head);

	if (get_desc(pdesc->Dword0) & TX_OWN) {
		return FALSE;
	}

	// Clear all status
	memset(pdesc, 0, 32);

	pdesc->Dword0 |= set_desc(TX_BMC|TX_FirstSeg | TX_LastSeg | ((32)<<TX_OffsetSHIFT));
	pdesc->Dword0 |= set_desc((unsigned short)(txLength) << TX_PktSizeSHIFT);
	pdesc->Dword2 |= set_desc((GetSequence(dat_content) & TX_SeqMask) << TX_SeqSHIFT);
//	pdesc->Dword4 |= set_desc((0x08 << TX_RtsRateSHIFT) | (0x7 << TX_RaBRSRIDSHIFT) | TX_UseRate);	//	need to confirm
	pdesc->Dword4 |= set_desc((0x08 << TX_RtsRateSHIFT) | TX_UseRate);
	pdesc->Dword4 |= set_desc(TX_DisDataFB);
	pdesc->Dword7 |= set_desc((unsigned short)(txLength) << TX_TxBufSizeSHIFT);
#ifdef CONFIG_RTL_8812_SUPPORT
	if(GET_CHIP_VER(priv)== VERSION_8812E)
	pdesc->Dword10 = set_desc(get_physical_addr(priv, dat_content, txLength, PCI_DMA_TODEVICE));
	else
#endif
	pdesc->Dword8 = set_desc(get_physical_addr(priv, dat_content, txLength, PCI_DMA_TODEVICE));

	rtl_cache_sync_wback(priv, (unsigned int)dat_content, txLength, PCI_DMA_TODEVICE);
	pdesc->Dword1 |= set_desc(0x13 << TX_QSelSHIFT);	// sw beacon
	pdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, bus_to_virt(phw->txcmd_desc_dma_addr[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	*tx_head = (*tx_head + 1) & (NUM_CMD_DESC - 1);

	return TRUE;
}
#endif


//-----------------------------------------------------------------------------
// Procedure:    Fill Tx Command Packet Descriptor
//
// Description:   This routine fill command packet descriptor. We assum that command packet
//				require only one descriptor.
//
// Arguments:   This function is only for Firmware downloading in current stage
//
// Returns:
//-----------------------------------------------------------------------------
int rtl8192cd_SetupOneCmdPacket(struct rtl8192cd_priv *priv, unsigned char *dat_content, unsigned short txLength, unsigned char LastPkt)
/*
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb,
	IN	u1Byte			QueueIndex,
	IN	u2Byte			index,
	IN	BOOLEAN			bFirstSeg,
	IN	BOOLEAN			bLastSeg,
	IN	pu1Byte			VirtualAddress,
	IN	u4Byte			PhyAddressLow,
	IN	u4Byte			BufferLen,
	IN	BOOLEAN     		bSetOwnBit,
	IN	BOOLEAN			bLastInitPacket,
	IN    u4Byte			DescPacketType,
	IN	u4Byte			PktLen
	)
*/
{

	unsigned char	ih=0;
	unsigned char	DescNum;
	unsigned short	DebugTimerCount;

	struct tx_desc	*pdesc;
	struct tx_desc	*phdesc;
	volatile unsigned int *ppdesc  ; //= (unsigned int *)pdesc;
	int	*tx_head, *tx_tail;
	struct rtl8192cd_hw	*phw = GET_HW(priv);

	tx_head	= (int *)&phw->txcmdhead;
	tx_tail = (int *)&phw->txcmdtail;
	phdesc = phw->txcmd_desc;

	DebugTimerCount = 0; // initialize debug counter to exit loop
	DescNum = 1;

//TODO: Fill the dma check here

//	printk("data lens: %d\n", txLength );

	for (ih=0; ih<DescNum; ih++) {
		pdesc      = (phdesc + (*tx_head));
		ppdesc = (unsigned int *)pdesc;
		// Clear all status
		memset(pdesc, 0, 36);
//		rtl_cache_sync_wback(priv, phw->txcmd_desc_dma_addr[*tx_head], 32, PCI_DMA_TODEVICE);
		// For firmware downlaod we only need to set LINIP
		if (LastPkt)
			pdesc->Dword0 |= set_desc(TX_LINIP);

		// From Scott --> 92SE must set as 1 for firmware download HW DMA error
		pdesc->Dword0 |= set_desc(TX_FirstSeg);;//bFirstSeg;
		pdesc->Dword0 |= set_desc(TX_LastSeg);;//bLastSeg;

		// 92SE need not to set TX packet size when firmware download
		pdesc->Dword7 |=  (set_desc((unsigned short)(txLength) << TX_TxBufSizeSHIFT));

		memcpy(priv->pshare->txcmd_buf, dat_content, txLength);

		rtl_cache_sync_wback(priv, (unsigned int)priv->pshare->txcmd_buf, txLength, PCI_DMA_TODEVICE);

#ifdef CONFIG_RTL_8812_SUPPORT
		if(GET_CHIP_VER(priv)== VERSION_8812E)
			pdesc->Dword10 =  set_desc(priv->pshare->cmdbuf_phyaddr);
		else
#endif
		pdesc->Dword8 =  set_desc(priv->pshare->cmdbuf_phyaddr);


//		pdesc->Dword0	|= set_desc((unsigned short)(txLength) << TX_PktIDSHIFT);
		pdesc->Dword0	|= set_desc((unsigned short)(txLength) << TX_PktSizeSHIFT);
		//if (bSetOwnBit)
		{
			pdesc->Dword0 |= set_desc(TX_OWN);
//			*(ppdesc) |= set_desc(BIT(31));
		}

#ifndef USE_RTL8186_SDK
		rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(phw->txcmd_desc_dma_addr[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif
		*tx_head = (*tx_head + 1) & (NUM_CMD_DESC - 1);
	}

	return TRUE;
}


#ifdef CONFIG_RTK_MESH
#ifndef __LINUX_2_6__
__MIPS16
#endif
int reuse_meshhdr(struct rtl8192cd_priv *priv, struct tx_insn *txcfg)
{
	const short meshhdrlen= (txcfg->mesh_header.mesh_flag & 0x01) ? 16 : 4;
	struct sk_buff *pskb = (struct sk_buff *)txcfg->pframe;
	if (skb_cloned(pskb))
	{
		struct sk_buff	*newskb = skb_copy(pskb, GFP_ATOMIC);
		rtl_kfree_skb(priv, pskb, _SKB_TX_);
		if (newskb == NULL) {
			priv->ext_stats.tx_drops++;
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
			DEBUG_ERR("TX DROP: Can't copy the skb!\n");
			return 0;
		}
		txcfg->pframe = pskb = newskb;
#ifdef ENABLE_RTL_SKB_STATS
		rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif
	}
	memcpy(skb_push(pskb,meshhdrlen), &(txcfg->mesh_header), meshhdrlen);
	txcfg->fr_len += meshhdrlen;
	return 1;
}


#ifndef __LINUX_2_6__
__MIPS16
#endif
int start_xmit_mesh(struct sk_buff *skb, struct net_device *dev, struct tx_insn *txcfg)
{
	struct stat_info	*pstat=NULL;
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;

#ifdef	_11s_TEST_MODE_
	if(mesh_debug_tx1(dev, priv, skb)<0)
		return 0;
#endif

	// 11s action, send by pathsel daemon
	unsigned char zero14[14] = {0}; // the first 14 bytes is zero: 802.3: 6 bytes (src) + 6 bytes (dst) + 2 bytes (protocol)
#ifdef	_11s_TEST_MODE_
	if(mesh_debug_tx2( priv, skb)<0)
		return 0;
#endif

	if(memcmp(skb->data, zero14, sizeof(zero14))==0)
	{
		if(issue_11s_mesh_action(skb, dev)==0)
			dev_kfree_skb_any(skb);
		return 0;
	}

	// drop any packet which has dest to STA but go throu MSH0
	{
		pstat = get_stainfo(priv, skb->data);
		if(pstat!=0 && isSTA(pstat))
		{
			dev_kfree_skb_any(skb);
			return 0;
		}
	}

	if(!dot11s_datapath_decision(skb, txcfg, 1)) //the dest form bridge need be update to proxy table
		return 0;

	return 1;
}
#endif


#if defined(WIFI_WMM) && defined(DRVMAC_LB)
void SendLbQosNullData(struct rtl8192cd_priv *priv)
{
	struct wifi_mib *pmib;
	unsigned char *hwaddr;
	unsigned char tempQosControl[2];
	DECLARE_TXINSN(txinsn);


	pmib = GET_MIB(priv);
	hwaddr = pmib->dot11OperationEntry.hwaddr;

	if(!pmib->miscEntry.drvmac_lb) {
		printk("LB mode disabled, cannot SendLbQosNullData!!!\n");
		return;
	}

	if(!memcmp(pmib->miscEntry.lb_da, "\x0\x0\x0\x0\x0\x0", 6)) {
		printk("LB addr is NULL, cannot SendLbQosNullData!!!\n");
		return;
	}

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;
	txinsn.q_num = BE_QUEUE;
//	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	txinsn.phdr = get_wlanhdr_from_poll(priv);
	txinsn.pframe = NULL;

	if (txinsn.phdr == NULL)
		goto send_qos_null_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, BIT(7) | WIFI_DATA_NULL);
	SetFrDs(txinsn.phdr);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pmib->miscEntry.lb_da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	txinsn.hdr_len = WLAN_HDR_A3_QOS_LEN;

	memset(tempQosControl, 0, 2);
//	tempQosControl[0] = 0x07;		//set priority to VO
//	tempQosControl[0] |= BIT(4);	//set EOSP
	memcpy((void *)GetQosControl((txinsn.phdr)), tempQosControl, 2);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

send_qos_null_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
}


void SendLbQosData(struct rtl8192cd_priv *priv)
{
	struct wifi_mib *pmib;
	unsigned char *hwaddr;
	unsigned char tempQosControl[2];
	unsigned char	*pbuf;
	static unsigned int pkt_length = 1;
	DECLARE_TXINSN(txinsn);

	pmib = GET_MIB(priv);
	hwaddr = pmib->dot11OperationEntry.hwaddr;

	if(!pmib->miscEntry.drvmac_lb) {
		printk("LB mode disabled, cannot SendLbQosData!!!\n");
		return;
	}

	if(!memcmp(pmib->miscEntry.lb_da, "\x0\x0\x0\x0\x0\x0", 6)) {
		printk("LB addr is NULL, cannot SendLbQosData!!!\n");
		return;
	}

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;
	txinsn.q_num = BE_QUEUE;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
//	txinsn.pframe = NULL;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto send_qos_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto send_qos_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));
	memset((void *)pbuf, 0, PRE_ALLOCATED_BUFSIZE);

	SetFrameSubType(txinsn.phdr, WIFI_QOS_DATA);
	SetFrDs(txinsn.phdr);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pmib->miscEntry.lb_da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	txinsn.hdr_len = WLAN_HDR_A3_QOS_LEN;

	memset(tempQosControl, 0, 2);
	memcpy((void *)GetQosControl((txinsn.phdr)), tempQosControl, 2);

//	printk("--0x%02x%02x%02x%02x 0x%02x%02x%02x%02x--\n", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), *(pbuf+4), *(pbuf+5), *(pbuf+6), *(pbuf+7));

	if (pmib->miscEntry.lb_mlmp == 1) {
		// all zero in payload
		memset((void *)pbuf, 0x00, pkt_length);
		pbuf += pkt_length;
		txinsn.fr_len += pkt_length;
	}
	else if (pmib->miscEntry.lb_mlmp == 2) {
		// all 0xff in payload
		memset((void *)pbuf, 0xff, pkt_length);
//		printk("~~0x%02x%02x%02x%02x 0x%02x%02x%02x%02x~~\n", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), *(pbuf+4), *(pbuf+5), *(pbuf+6), *(pbuf+7));
		pbuf += pkt_length;
		txinsn.fr_len += pkt_length;
	}
	else if ((pmib->miscEntry.lb_mlmp == 3) || (pmib->miscEntry.lb_mlmp == 4)) {
		// all different value in payload, 0x00~0xff
		unsigned int i = 0;
//		printk("~~");
		for (i = 0; i <pkt_length; i++) {
			memset((void *)pbuf, i%0x100, 1);
//			printk("%02x", *pbuf);
			pbuf += 1;
			txinsn.fr_len += 1;
		}
//		printk("~~\n");
	}
	else {
		printk("wrong LB muli-length-multi-packet setting!!\n");
		goto send_qos_fail;
	}

//	if (pkt_length++ >= 600)
	if (pkt_length++ >= 2048)
		pkt_length = 1;

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

send_qos_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}
#endif

#ifdef TX_EARLY_MODE
__MIPS16
__IRAM_IN_865X
static void get_tx_early_info(struct rtl8192cd_priv *priv, struct stat_info  *pstat,  struct sk_buff_head *pqueue)
{	
	struct sk_buff *next_skb;

	pstat->empkt_num = 0;
	memset(pstat->empkt_len, '\0', sizeof(pstat->empkt_len));
	
#ifdef CONFIG_RTL_88E_SUPPORT
	if (GET_CHIP_VER(priv) == VERSION_8188E) {
		int tmplen = 0, extra_len = 0;
		
		if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_) {
			extra_len = 16;
		} else if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) {
			extra_len = 20;
		} else if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) {
			extra_len = 8;
		}
		
		skb_queue_walk(pqueue, next_skb) {
			if ((pstat->empkt_num %2) == 0) {
				pstat->empkt_len[pstat->empkt_num/2] = next_skb->len+WLAN_HDR_A3_LEN+WLAN_CRC_LEN+extra_len;
			} else {
				tmplen = pstat->empkt_len[(pstat->empkt_num-1)/2];
				tmplen += ((tmplen%4)?(4-tmplen%4):0)+4;
				tmplen += next_skb->len+WLAN_HDR_A3_LEN+WLAN_CRC_LEN+extra_len;	
				pstat->empkt_len[(pstat->empkt_num-1)/2] = tmplen;
			}
		
			pstat->empkt_num++;
			if (skb_queue_is_last(pqueue, next_skb))
				break;

			if (pstat->empkt_num >= 10)
				break;
		}	
	} else 
#endif	
	skb_queue_walk(pqueue, next_skb) {
		pstat->empkt_len[pstat->empkt_num++] = next_skb->len;

		if (skb_queue_is_last(pqueue, next_skb))
			break;

		if (pstat->empkt_num >= 5)
			break;
	}	
}
#endif

#ifdef SW_TX_QUEUE
__MIPS16
__IRAM_IN_865X
int __rtl8192cd_start_xmit_out(struct sk_buff *skb, struct stat_info	*pstat);

void rtl8192cd_beq_timer(unsigned long task_priv)
{
    struct stat_info        *pstat = (struct stat_info *)task_priv;
    struct sk_buff *skb = NULL;
	struct rtl8192cd_priv *priv = NULL;
	unsigned long x;

    while(1) {
    	skb = skb_dequeue(&pstat->swq.be_queue);

        if (skb == NULL)
        	break;
		
#ifdef NETDEV_NO_PRIV
		priv = ((struct rtl8192cd_priv *)netdev_priv(skb->dev))->wlan_priv;
#else		
		priv = (struct rtl8192cd_priv *)skb->dev->priv;
#endif

#ifdef TX_EARLY_MODE
	if (priv && GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) 
		get_tx_early_info(priv, pstat, &pstat->swq.be_queue);		
#endif		
		
		SAVE_INT_AND_CLI(x);
        __rtl8192cd_start_xmit_out(skb, pstat);
		RESTORE_INT(x);
		
		pstat->swq.q_TOCount[BE_QUEUE]++;
    }
	pstat->swq.beq_empty = 0;
	
	SAVE_INT_AND_CLI(x);
	adjust_swq_setting(priv, pstat, BE_QUEUE, CHECK_DEC_AGGN);
	RESTORE_INT(x);
}

void rtl8192cd_bkq_timer(unsigned long task_priv)
{
    struct stat_info        *pstat = (struct stat_info *)task_priv;
    struct sk_buff *skb = NULL;
	struct rtl8192cd_priv *priv = NULL;
	unsigned long x;

    while(1) {
    	skb = skb_dequeue(&pstat->swq.bk_queue);

        if (skb == NULL)
        	break;

#ifdef NETDEV_NO_PRIV
		priv = ((struct rtl8192cd_priv *)netdev_priv(skb->dev))->wlan_priv;
#else
		priv = (struct rtl8192cd_priv *)skb->dev->priv;
#endif

#ifdef TX_EARLY_MODE
	if (priv && GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) 
		get_tx_early_info(priv, pstat, &pstat->swq.bk_queue);		
#endif		
		
		SAVE_INT_AND_CLI(x);
        __rtl8192cd_start_xmit_out(skb, pstat);
		RESTORE_INT(x);
		
		pstat->swq.q_TOCount[BK_QUEUE]++;
    }
    pstat->swq.bkq_empty = 0;
    
    SAVE_INT_AND_CLI(x);
	adjust_swq_setting(priv, pstat, BK_QUEUE, CHECK_DEC_AGGN);
	RESTORE_INT(x);
}

void rtl8192cd_viq_timer(unsigned long task_priv)
{
    struct stat_info        *pstat = (struct stat_info *)task_priv;
    struct sk_buff *skb = NULL;
	struct rtl8192cd_priv *priv = NULL;
	unsigned long x;

    while(1) {
    	skb = skb_dequeue(&pstat->swq.vi_queue);

        if (skb == NULL)
        	break;
		
#ifdef NETDEV_NO_PRIV
		priv = ((struct rtl8192cd_priv *)netdev_priv(skb->dev))->wlan_priv;
#else		
		priv = (struct rtl8192cd_priv *)skb->dev->priv;
#endif

#ifdef TX_EARLY_MODE
	if (priv && GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) 
		get_tx_early_info(priv, pstat, &pstat->swq.vi_queue);		
#endif		
		
		SAVE_INT_AND_CLI(x);
        __rtl8192cd_start_xmit_out(skb, pstat);
		RESTORE_INT(x);
		
		pstat->swq.q_TOCount[VI_QUEUE]++;
    }
    pstat->swq.viq_empty = 0;
    
    SAVE_INT_AND_CLI(x);
	adjust_swq_setting(priv, pstat, VI_QUEUE, CHECK_DEC_AGGN);
	RESTORE_INT(x);
}

void rtl8192cd_voq_timer(unsigned long task_priv)
{
    struct stat_info        *pstat = (struct stat_info *)task_priv;
    struct sk_buff *skb = NULL;
	struct rtl8192cd_priv *priv = NULL;
	unsigned long x;

    while(1) {
    	skb = skb_dequeue(&pstat->swq.vo_queue);

        if (skb == NULL)
        	break;

#ifdef NETDEV_NO_PRIV
		priv = ((struct rtl8192cd_priv *)netdev_priv(skb->dev))->wlan_priv;
#else
		priv = (struct rtl8192cd_priv *)skb->dev->priv;
#endif

#ifdef TX_EARLY_MODE
	if (priv && GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) 
		get_tx_early_info(priv, pstat, &pstat->swq.vo_queue);		
#endif		
		
		SAVE_INT_AND_CLI(x);
        __rtl8192cd_start_xmit_out(skb, pstat);
		RESTORE_INT(x);
		
		pstat->swq.q_TOCount[VO_QUEUE]++;
    }
    pstat->swq.voq_empty = 0;
    
    SAVE_INT_AND_CLI(x);
	adjust_swq_setting(priv, pstat, VO_QUEUE, CHECK_DEC_AGGN);
	RESTORE_INT(x);
}

__MIPS16
__IRAM_IN_865X
int sw_dequeue(struct rtl8192cd_priv *priv, struct sk_buff_head *pqueue,  struct timer_list *ptimer)//, int *pempty)
{
	struct stat_info *pstat = (struct stat_info *)ptimer->data;
    if (timer_pending(ptimer))
        del_timer_sync(ptimer);

    while(1)
    {
        struct sk_buff *tmpskb;
    	tmpskb = skb_dequeue(pqueue);
        if (tmpskb == NULL)
           	break;

#ifdef TX_EARLY_MODE
	if (GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) 
		get_tx_early_info(priv, pstat, pqueue);
#endif
		
        __rtl8192cd_start_xmit_out(tmpskb, pstat);
	}
	return 0;
}

__MIPS16
__IRAM_IN_865X
int sw_enqueue(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct stat_info	*pstat)
{
	int q_len, pri, q_num, tri_time, *pempty, aggnum;
	//unsigned char swq_out = 0;
	struct sk_buff_head *pqueue;
	struct timer_list *ptimer;
	void (*timer_hook)(unsigned long task_priv);

	pri = get_skb_priority(priv, skb);
#if defined(RTL_MANUAL_EDCA)
	q_num = PRI_TO_QNUM(priv, pri);
#else
	PRI_TO_QNUM(pri, q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif
	switch(q_num)
	{
		case BK_QUEUE:
			pqueue = &pstat->swq.bk_queue;
			ptimer = &pstat->swq.bkq_timer;
			timer_hook = rtl8192cd_bkq_timer;
			pempty = &pstat->swq.bkq_empty;
			pstat->swq.q_used[BK_QUEUE]=1;
			aggnum=pstat->swq.q_aggnum[BK_QUEUE];
			break;
		case VO_QUEUE:
			pqueue = &pstat->swq.vo_queue;
			ptimer = &pstat->swq.voq_timer;
			timer_hook = rtl8192cd_voq_timer;
			pempty = &pstat->swq.voq_empty;
			pstat->swq.q_used[VO_QUEUE]=1;
			aggnum=pstat->swq.q_aggnum[VO_QUEUE];
			break;
		case VI_QUEUE:
			pqueue = &pstat->swq.vi_queue;
			ptimer = &pstat->swq.viq_timer;
			timer_hook = rtl8192cd_viq_timer;
			pempty = &pstat->swq.viq_empty;
			pstat->swq.q_used[VI_QUEUE]=1;
			aggnum=pstat->swq.q_aggnum[VI_QUEUE];
			break;
		//case BE_QUEUE:
		default:
			pqueue = &pstat->swq.be_queue;
			ptimer = &pstat->swq.beq_timer;
			timer_hook = rtl8192cd_beq_timer;
			pempty = &pstat->swq.beq_empty;
			pstat->swq.q_used[BE_QUEUE]=1;
			aggnum=pstat->swq.q_aggnum[BE_QUEUE];
			break;
	}

	skb_queue_tail(pqueue, skb);
	ptimer->data = (unsigned long)pstat;

	if (!pstat->ADDBA_ready[pri])
	{
		sw_dequeue(priv, pqueue, ptimer);
		*pempty = 0;
		return 0;
	}

	q_len = skb_queue_len(pqueue);
	if(priv->assoc_num == 1)
		aggnum = priv->pshare->rf_ft_var.swq_aggnum;

#ifdef TX_EARLY_MODE
	if (q_len >= (GET_EM_SWQ_ENABLE ? MAX_EM_QUE_NUM : aggnum)) {
#else
	if (q_len >= aggnum) {
#endif		
		sw_dequeue(priv, pqueue, ptimer);//, pempty);
		*pempty = 0;
		return 0;
	} else if (q_len == 1) {
		if (!timer_pending(ptimer))
		{
			if ((pstat->tx_avarage > 1875000)) 					//15M~
				tri_time = 1;
			else if ((pstat->tx_avarage > 500000)) 					//4M~15M
				tri_time = 3;
			else if ((pstat->tx_avarage > 250000) && (pstat->tx_avarage <= 500000)) //2M~4M
				tri_time = 9;
			else if ((pstat->tx_avarage > 100000))					 //800K~2M
				tri_time = 12;
			else
				tri_time = 1;
			
			ptimer->data = (unsigned long) pstat;
			ptimer->function = timer_hook; //rtl8190_tmp_timer;
			mod_timer(ptimer, jiffies + tri_time);
		}
	}

	*pempty = 1;
	return 0;
}
#endif

#ifdef A4_STA
static void send_br_packet(struct rtl8192cd_priv *priv, struct sk_buff *skb, struct stat_info	*pstat)
{
	struct sk_buff *newskb=NULL;
	struct tx_insn tx_insn;
	DECLARE_TXCFG(txcfg, tx_insn);

#ifdef DETECT_STA_EXISTANCE
	if (pstat->leave) {
		priv->ext_stats.tx_drops++;
		DEBUG_WARN("TX DROP: sta may leave! %02x%02x%02x%02x%02x%02x\n", pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		return;
	}
#endif

	if (UseSwCrypto(priv, pstat, FALSE) ||
			(get_sta_encrypt_algthm(priv, pstat) == _TKIP_PRIVACY_)) {
		if (skb_cloned(skb)) {		
			newskb = skb_copy(skb, GFP_ATOMIC);
			if (newskb == NULL) {
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: Can't copy the skb!\n");
				return;
			}
		}
	}

	if (!newskb) {
		newskb = skb_clone(skb, GFP_ATOMIC);
		if (newskb == NULL) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Can't clone the skb!\n");
			return;
		}
	}

	if ((pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
							(WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) {
		if (dz_queue(priv, pstat, newskb) == TRUE) {
			DEBUG_INFO("queue up skb due to sleep mode\n");			
		}
		else {
			DEBUG_WARN("ucst sleep queue full!!\n");
			dev_kfree_skb_any(newskb);
		}
		return;	
	}

	txcfg->pstat = pstat;		
	
	rtl8192cd_tx_slowPath(priv, newskb, pstat, priv->dev, NULL, txcfg);
}
#endif  /* A4_STA */

#if defined(CONFIG_RTL_CUSTOM_PASSTHRU)
		#define	RTL_PASSTHRU_FREE_AND_STOP		free_and_stoptx

	#endif

#ifndef SW_TX_QUEUE
__MIPS16
__IRAM_IN_865X
int __rtl8192cd_start_xmit_out(struct sk_buff *skb, struct stat_info *pstat);
#endif

__MIPS16
__IRAM_IN_865X
int __rtl8192cd_start_xmit(struct sk_buff *skb, struct net_device *dev, int tx_flag)
{
	struct rtl8192cd_priv *priv;
	struct stat_info	*pstat=NULL;
	unsigned char		*da;
	//struct sk_buff *newskb = NULL;
	struct net_device *wdsDev = NULL;
#ifdef SW_TX_QUEUE
	int q_num, *pempty;
	int swq_out = 0;
    struct sk_buff_head *pqueue;
    struct timer_list *ptimer;
//    void (*timer_hook)(unsigned long task_priv);
#endif
	struct tx_insn tx_insn;
//	int real_len;

	DECLARE_TXCFG(txcfg, tx_insn);

	if (skb->len < 15)
    {
        _DEBUG_ERR("TX DROP: SKB len small:%d\n", skb->len);
        goto free_and_stoptx;
    }

#if defined(CONFIG_RTL_CUSTOM_PASSTHRU)
	if (dev==wlan_device[passThruWanIdx].priv->pWlanDev ||skb->dev==wlan_device[passThruWanIdx].priv->pWlanDev)
	{
		if (SUCCESS==rtl_isPassthruFrame(skb->data))
		{
#ifdef UNIVERSAL_REPEATER
			if(passThruStatusWlan & 0x8) //WISP Mode Enable. default is vxd
			{
				#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
				  unsigned int wispWlanIndex=(passThruStatusWlan&WISP_WLAN_IDX_MASK)>>WISP_WLAN_IDX_RIGHT_SHIFT;
				  if ((wlan_device[wispWlanIndex].priv->drv_state & DRV_STATE_OPEN )&&
				  	 ((GET_MIB(GET_VXD_PRIV((wlan_device[wispWlanIndex].priv)))->dot11OperationEntry.opmode) & WIFI_STATION_STATE))
				  {
					#ifdef NETDEV_NO_PRIV
				       dev=skb->dev=((struct rtl8192cd_priv *)(((struct rtl8192cd_priv *)netdev_priv(wlan_device[wispWlanIndex].priv->pWlanDev))->wlan_priv))->pvxd_priv->dev;
					#else	
					dev=skb->dev=GET_VXD_PRIV(wlan_device[wispWlanIndex].priv)->dev;
					#endif
				  } else {
					goto RTL_PASSTHRU_FREE_AND_STOP;
				  }
				#else		
					#ifdef NETDEV_NO_PRIV
					dev=skb->dev=((struct rtl8192cd_priv *)(((struct rtl8192cd_priv *)netdev_priv(wlan_device[passThruWanIdx].priv->pWlanDev))->wlan_priv))->pvxd_priv->dev;
					#else	
					dev=skb->dev= GET_VXD_PRIV(wlan_device[passThruWanIdx].priv)->dev;
					#endif
				#endif
			} else
#endif
			{
				#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
				  unsigned int wispWlanIndex=(passThruStatusWlan&WISP_WLAN_IDX_MASK)>>WISP_WLAN_IDX_RIGHT_SHIFT;
				  if ((wlan_device[wispWlanIndex].priv->drv_state & DRV_STATE_OPEN )&&
				  	 (((GET_MIB(wlan_device[wispWlanIndex].priv))->dot11OperationEntry.opmode) & WIFI_STATION_STATE))
				  {
					#ifdef NETDEV_NO_PRIV
				        dev=skb->dev=((struct rtl8192cd_priv *)netdev_priv(wlan_device[wispWlanIndex].priv->pWlanDev))->wlan_priv->dev;
					#else	
					dev=skb->dev=wlan_device[wispWlanIndex].priv->dev;
					#endif
				  } else {
					goto RTL_PASSTHRU_FREE_AND_STOP;
				  }
				#else		
					#ifdef NETDEV_NO_PRIV
					dev=skb->dev=((struct rtl8192cd_priv *)netdev_priv(wlan_device[passThruWanIdx].priv->pWlanDev))->wlan_priv->dev;
					#else	
					dev=skb->dev=((struct rtl8192cd_priv *)(wlan_device[passThruWanIdx].priv->pWlanDev->priv))->dev;
					#endif
				#endif
			}
		} else {
			goto RTL_PASSTHRU_FREE_AND_STOP;
		}
	}
#endif	/* defined(CONFIG_RTL_CUSTOM_PASSTHRU) */

#ifdef CONFIG_RTL_STP
	//port5: virtual device for wlan0 stp BPDU process
	if(memcmp((void *)(dev->name), "port5", 5)==0)
	{
		if ((skb->data[0]&0x01) && !memcmp(&(skb->data[0]), STPmac, 5) && !(skb->data[5] & 0xF0))
		{
			//To virtual device port5, tx bpdu.
		}
		else
		{
			//To virtual device port5, drop tx pkt becasue not bpdu!
			goto free_and_stoptx;

		}
	}
#endif

#ifdef CONFIG_RTL_EAP_RELAY
//mark_test , remove EAP padding by ethernet
	if (ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2))) == 0x888e)
	{			
		real_len = ETH_ALEN*2+2+4+ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2+2+2)));
		skb_trim(skb,real_len);
		//printk("2 wlan tx EAP ,skb->len=%d,skb->data=%d \n",skb->len,skb->data_len);		
	}
#endif

#ifdef NETDEV_NO_PRIV
	priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	priv = (struct rtl8192cd_priv *)dev->priv;
#endif

//#ifdef SW_TX_QUEUE
        skb->dev = dev;
//#endif

#ifdef CONFIG_RTK_MESH
//	skb->dev = dev;

	if( priv->pmib->dot1180211sInfo.mesh_enable && (dev == priv->mesh_dev)) {
		tx_flag = TX_NO_MUL2UNI;
		goto mesh_skip;
	}
#endif // CONFIG_RTK_MESH

#ifdef WDS
	if (!dev->base_addr && skb_cloned(skb)
		&& (priv->pmib->dot11WdsInfo.wdsPrivacy == _TKIP_PRIVACY_)
		) {
		struct sk_buff *mcast_skb = NULL;
		mcast_skb = skb_copy(skb, GFP_ATOMIC);
		rtl_kfree_skb(priv, skb, _SKB_TX_);
		if (mcast_skb == NULL) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Can't copy the skb!\n");
			return 0;
		}
		skb = mcast_skb;
	}
#endif

#ifdef	IGMP_FILTER_CMO
	if (priv->pshare->rf_ft_var.igmp_deny)
	{
		if ((OPMODE & WIFI_AP_STATE) &&	IS_MCAST(skb->data))
		{
			if (IP_MCAST_MAC(skb->data)
				#ifdef	TX_SUPPORT_IPV6_MCAST2UNI
				|| ICMPV6_MCAST_MAC(skb->data)
				#endif
				)
			{
				if(!IS_IGMP_PROTO(skb->data)){
					priv->ext_stats.tx_drops++;										
					DEBUG_ERR("TX DROP: Multicast packet filtered\n");
					dev_kfree_skb_any(skb);
					return 0;
				}
			}
		}
	}
#endif

#ifdef SUPPORT_TX_MCAST2UNI
#ifdef WDS			// when interface is WDS don't enter multi2uni path
	if (dev->base_addr)
#endif
	if (!priv->pshare->rf_ft_var.mc2u_disable) {
		if ((OPMODE & WIFI_AP_STATE) &&	IS_MCAST(skb->data))
		{
			if ((IP_MCAST_MAC(skb->data)
				#ifdef	TX_SUPPORT_IPV6_MCAST2UNI
				|| IPV6_MCAST_MAC(skb->data)
				#endif
				) && (tx_flag != TX_NO_MUL2UNI))
			{
				if (mlcst2unicst(priv, skb)){
					return 0;
				}
			}
		}

		skb->cb[2] = 0;	// allow aggregation
	}
#endif

#if !defined(CONFIG_RTL_8676HWNAT) && defined(CONFIG_RTL8672) && !defined(CONFIG_RTL8686)
	skb->cb[16] = tx_flag;


	//IGMP snooping
	if (check_wlan_mcast_tx(skb)==1) {
		goto free_and_stoptx;
	}

	// Mason Yu
	if ( g_port_mapping == TRUE ) {
		for (k=0; k<5; k++) {
			if (dev == wlanDev[k].dev_pointer) {
				if (wlanDev[k].dev_ifgrp_member!=0 && skb->vlan_member!=0 && skb->vlan_member!=wlanDev[k].dev_ifgrp_member) {
					goto free_and_stoptx;
				}
				break;
			}
		}
	}
#endif //!defined(CONFIG_RTL_8676HWNAT) && defined(CONFIG_RTL8672) && !defined(CONFIG_RTL8686)

#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		GET_ROOT(priv)->pmib->dot11DFSEntry.disable_tx) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: DFS probation period\n");
		goto free_and_stoptx;
	}
#endif

	if (!IS_DRV_OPEN(priv))
        goto free_and_stoptx;

#ifdef MP_TEST
	if (OPMODE & WIFI_MP_STATE) {
        goto free_and_stoptx;
	}
#endif

#ifdef CONFIG_RTL867X_VLAN_MAPPING
	if (re_vlan_loaded()) {
		struct ethhdr *eth = (struct ethhdr *)skb->data;
		unsigned short vid = 0;

		if (eth->h_proto != ETH_P_PAE) {
			if (re_vlan_skb_xmit(skb, &vid)) {
				priv->ext_stats.tx_drops++;
				return 0;
			}
			if (vid && re_vlan_addtag(skb, vid)) {
				priv->ext_stats.tx_drops++;
				return 0;
			}
		}
	}
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
	if (priv->pmib->vlan.global_vlan &&
			ntohs(*((UINT16 *)((UINT8 *)skb->data + ETH_ALEN*2))) != 0x888e) {
		int get_pri = 0;
#ifdef WIFI_WMM
		if (QOS_ENABLE) {
#ifdef CLIENT_MODE
			if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
				pstat = get_stainfo(priv, BSSID);
			else
#endif
			{
#ifdef MCAST2UI_REFINE
                                if (pstat == NULL && !IS_MCAST(&skb->cb[10]))
                                        pstat = get_stainfo(priv, &skb->cb[10]);
#else
				if (pstat == NULL && !IS_MCAST(skb->data))
					pstat = get_stainfo(priv, skb->data);
#endif
			}

			if (pstat && pstat->QosEnabled)
				get_pri = 1;
		}
#endif

		if (!get_pri)
			skb->cb[0] = '\0';

		if (tx_vlan_process(dev, &priv->pmib->vlan, skb, get_pri)) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: by vlan!\n");
			goto free_and_stoptx;
		}
	}
	else
		skb->cb[0] = '\0';
#endif

#ifdef WDS
	if (dev->base_addr) {
		// normal packets
		if (priv->pmib->dot11WdsInfo.wdsPure) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Sent normal pkt in Pure WDS mode!\n");
            goto free_and_stoptx;
		}
	}
	else {
		// WDS process
		if (rtl8192cd_tx_wdsDevProc(priv, skb, &dev, &wdsDev, txcfg) == TX_PROCEDURE_CTRL_STOP) {
            goto stop_proctx;
		}
	}
#endif // WDS

	if (priv->pmib->miscEntry.func_off)
        goto free_and_stoptx;

	// drop packets if link status is null
#ifdef WDS
	if (!wdsDev)
#endif
	{
		if (get_assoc_sta_num(priv) == 0) {
			priv->ext_stats.tx_drops++;
			DEBUG_WARN("TX DROP: Non asoc tx request!\n");
            goto free_and_stoptx;
		}
	}

#ifdef CLIENT_MODE
	// nat2.5 translation, mac clone, dhcp broadcast flag add.
	if (((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
			|| (OPMODE & WIFI_ADHOC_STATE)) {
#ifdef RTK_BR_EXT
		if (!priv->pmib->ethBrExtInfo.nat25_disable &&
				!(skb->data[0] & 1) &&
				priv->dev->br_port &&  memcmp(skb->data+MACADDRLEN, priv->br_mac, MACADDRLEN) &&
				*((unsigned short *)(skb->data+MACADDRLEN*2)) != __constant_htons(ETH_P_8021Q) &&
				*((unsigned short *)(skb->data+MACADDRLEN*2)) == __constant_htons(ETH_P_IP) &&
				!memcmp(priv->scdb_mac, skb->data+MACADDRLEN, MACADDRLEN) && priv->scdb_entry) {
			memcpy(skb->data+MACADDRLEN, GET_MY_HWADDR, MACADDRLEN);
			priv->scdb_entry->ageing_timer = jiffies;
		}
		else
#endif

		if (rtl8192cd_tx_clientMode(priv, &skb) == TX_PROCEDURE_CTRL_STOP) {
            goto stop_proctx;
		}
		
#ifdef RTK_BR_EXT //8812_client
		dhcp_flag_bcast(priv, skb);
#endif

	}
#endif // CLIENT_MODE

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable)
	{
		if ((OPMODE & WIFI_AP_STATE) && !wdsDev) {
			if ((*(unsigned int *)&(skb->cb[8]) == 0x86518190) &&						// come from wlan interface
				(*(unsigned int *)&(skb->cb[12]) != priv->pmib->miscEntry.groupID))		// check group ID
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: not the same group!\n");
                goto free_and_stoptx;
			}
		}
	}
#endif

#ifdef USE_TXQUEUE
	if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable) {
		int pri = 0, qnum = BE_QUEUE, qlen = 0, qidx = 0;
		pri = get_skb_priority(priv, skb);
#ifdef RTL_MANUAL_EDCA
        q_num = PRI_TO_QNUM(priv, pri);
#else
		PRI_TO_QNUM(pri, qnum, priv->pmib->dot11OperationEntry.wifi_specific);
#endif
		
		if (!priv->pshare->txq_isr)
		{
			if (txq_len(&priv->pshare->txq_list[qnum]) > 0)
			{
#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
				if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
					if (!check_txq_dynamic_mechanism(priv, qnum)) {
						append_skb_to_txq_tail(&priv->pshare->txq_list[qnum], priv, skb, dev, &priv->pshare->txq_pool);
						priv->use_txq_cnt[qnum]++;
						priv->pshare->txq_check = 1;
				        goto stop_proctx;
					} else {
						DEBUG_ERR("TX DROP: exceed the tx queue!\n");
						priv->ext_stats.tx_drops++;
			            goto free_and_stoptx;
					}
				}
				else
#endif
				{
					for (qidx=0; qidx<7; qidx++)
						qlen += txq_len(&priv->pshare->txq_list[qidx]);
					if (qlen < TXQUEUE_SIZE) {
						append_skb_to_txq_tail(&priv->pshare->txq_list[qnum], priv, skb, dev, &priv->pshare->txq_pool);
						priv->pshare->txq_check = 1;
			            goto stop_proctx;
					} else {
						DEBUG_ERR("TX DROP: exceed the tx queue!\n");
						priv->ext_stats.tx_drops++;
			            goto free_and_stoptx;
					}
				}
			}
		}
	}
#endif

#ifdef ENABLE_RTL_SKB_STATS
	rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif

#ifdef WDS
	if (wdsDev)
		da = priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr;
	else
#endif
#ifdef CONFIG_RTK_MESH
	if(dev == priv->mesh_dev) {
		dot11s_datapath_decision(skb, txcfg, 1);
		da = txcfg->nhop_11s;
	}
	else
#endif
	{

#ifdef MCAST2UI_REFINE
        da = &skb->cb[10];
#else
		da = skb->data;
#endif
	}

#ifdef CLIENT_MODE
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
		pstat = get_stainfo(priv, BSSID);
	else
#endif
	{
		pstat = get_stainfo(priv, da);
#ifdef A4_STA
		if (pstat == NULL && !IS_MCAST(skb->data) &&  priv->pshare->rf_ft_var.a4_enable) 
			pstat = a4_sta_lookup(priv, da);		
#endif		

#ifdef SW_TX_QUEUE
		if (pstat)
			swq_out = priv->swq_en | pstat->swq.beq_empty | pstat->swq.viq_empty | pstat->swq.voq_empty | pstat->swq.bkq_empty;
#endif
	}
#ifdef DETECT_STA_EXISTANCE
	if(pstat && pstat->leave)	{
		priv->ext_stats.tx_drops++;
		DEBUG_WARN("TX DROP: sta may leave! %02x%02x%02x%02x%02x%02x\n", pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
        goto free_and_stoptx;
	}
#endif

#ifdef SW_TX_QUEUE

	//if ((pstat == NULL) || (pstat && (!priv->swq_en) && (!(pstat->swq.beq_empty | pstat->swq.viq_empty | pstat->swq.voq_empty | pstat->swq.bkq_empty))))
	if (swq_out == 0
#ifdef GBWC
		|| priv->GBWC_consuming_Q
#endif		
		) {
#ifdef CONFIG_RTK_MESH
mesh_skip:
#endif
     		return __rtl8192cd_start_xmit_out(skb, pstat);
	} else if (priv->swq_en) {
		return sw_enqueue(priv,skb,pstat);
	} else {
		//if ((pstat != NULL) && (priv->pshare->rf_ft_var.swq_enable == 1) && (AMPDU_ENABLE != 0))
			//ptimer->data = (unsigned long)pstat;
		for(q_num = BK_QUEUE; q_num <= VO_QUEUE; q_num++)
		{
			switch(q_num)
			{
				case BK_QUEUE:
					pqueue = &pstat->swq.bk_queue;		
					ptimer = &pstat->swq.bkq_timer;
					pempty = &pstat->swq.bkq_empty;
					break;
				case VO_QUEUE:
	                pqueue = &pstat->swq.vo_queue;
					ptimer = &pstat->swq.voq_timer;
					pempty = &pstat->swq.voq_empty;
	                break;
				case VI_QUEUE:
	                pqueue = &pstat->swq.vi_queue;
					ptimer = &pstat->swq.viq_timer;
					pempty = &pstat->swq.viq_empty;
	                break;
					//case BE_QUEUE:
				default:
	                pqueue = &pstat->swq.be_queue;
					ptimer = &pstat->swq.beq_timer;
					pempty = &pstat->swq.beq_empty;
	                break;
			}				

			if (*pempty == 0)
				continue;

			ptimer->data = (unsigned long)pstat;
			sw_dequeue(priv, pqueue, ptimer);//, pempty, pstat);
			*pempty = 0;
		}

	    __rtl8192cd_start_xmit_out(skb, pstat);
	}
    goto stop_proctx;
#else
	return __rtl8192cd_start_xmit_out(skb, pstat);
#endif	

free_and_stoptx:          /* Free current packet and stop TX process */
    dev_kfree_skb_any(skb);

stop_proctx:                      /* Stop process and assume the TX-ed packet is already "processed" (freed or TXed) in previous code. */

    return 0;
}

__MIPS16
__IRAM_IN_865X
int __rtl8192cd_start_xmit_out(struct sk_buff *skb, struct stat_info *pstat) //struct net_device *dev)
{
    struct rtl8192cd_priv *priv;
    //struct stat_info        *pstat=NULL;
    struct net_device *dev = skb->dev;
    //unsigned char           *da;
    struct sk_buff *newskb = NULL;
    struct net_device *wdsDev = NULL;
#if defined(CONFIG_RTL8672) || defined(TX_SHORTCUT)
    int k;
#endif
    struct tx_insn tx_insn;
    //int real_len;
    DECLARE_TXCFG(txcfg, tx_insn);

#ifdef NETDEV_NO_PRIV
	priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	priv = (struct rtl8192cd_priv *)dev->priv;
#endif

#ifdef CONFIG_RTL8672
#ifdef SUPPORT_TX_MCAST2UNI
	if (skb->cb[16] == TX_NO_MUL2UNI)
		txcfg->isMC2UC = 1;
	else
		txcfg->isMC2UC = 0;
#endif
#endif

#ifdef WDS
	if (dev->base_addr) {
		// normal packets
		if (priv->pmib->dot11WdsInfo.wdsPure) {
			priv->ext_stats.tx_drops++;
			DEBUG_ERR("TX DROP: Sent normal pkt in Pure WDS mode!\n");
			goto free_and_stop;
		}
	}
	else {
		// WDS process
		if (rtl8192cd_tx_wdsDevProc(priv, skb, &dev, &wdsDev, txcfg) == TX_PROCEDURE_CTRL_STOP) {
			goto stop_proc;
		}
	}
#endif // WDS


#ifdef CONFIG_RTL_MESH_SUPPORT
	if( skb->dev == priv->mesh_dev )
		if(!start_xmit_mesh(skb, dev, txcfg))
			goto stop_proc;
#endif
/*
	if (UseSwCrypto(priv, pstat, (pstat ? FALSE : TRUE)) ||					// sw enc will modify content
		(pstat && (get_sta_encrypt_algthm(priv, pstat) == _TKIP_PRIVACY_)))	// need append MIC
*/
	{
		if (skb_cloned(skb)
#ifdef MCAST2UI_REFINE
                        && !memcmp(skb->data, &skb->cb[10], 6)
#endif
			)
		{
			newskb = skb_copy(skb, GFP_ATOMIC);
			rtl_kfree_skb(priv, skb, _SKB_TX_);
			if (newskb == NULL) {
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: Can't copy the skb!\n");
				return 0;
			}
			skb = newskb;

#ifdef ENABLE_RTL_SKB_STATS
			rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif
		}
	}

#ifdef SUPPORT_SNMP_MIB
	if (IS_MCAST(skb->data))
		SNMP_MIB_INC(dot11MulticastTransmittedFrameCount, 1);
#endif

	if ((OPMODE & WIFI_AP_STATE)
#ifdef WDS
			&& (!wdsDev)
#endif
#ifdef CONFIG_RTK_MESH
			&& (txcfg->is_11s == 0)
#endif
		) {
#ifdef MCAST2UI_REFINE
                if ((pstat && ((pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
                                (WIFI_SLEEP_STATE | WIFI_ASOC_STATE))) ||
                        (((IS_MCAST(&skb->cb[10]) && (priv->sleep_list.next != &priv->sleep_list)) ||
			((priv->pshare->rf_ft_var.bcast_to_dzq) && ((unsigned char)skb->cb[10] == 0xff))) &&
                                (!priv->release_mcast)))
#else
		if ((pstat && ((pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
				(WIFI_SLEEP_STATE | WIFI_ASOC_STATE))) ||
			(((IS_MCAST(skb->data) && (priv->sleep_list.next != &priv->sleep_list)) ||
			(priv->pshare->rf_ft_var.bcast_to_dzq && (*(skb->data) == 0xff))) &&
				(!priv->release_mcast)))
#endif
		{

#ifdef _11s_TEST_MODE_
			mesh_debug_tx3(dev, priv, skb);
#endif

			if (dz_queue(priv, pstat, skb) == TRUE) {
				DEBUG_INFO("queue up skb due to sleep mode\n");
				goto stop_proc;
			}
			else {
				if (pstat) {
					DEBUG_WARN("ucst sleep queue full!!\n");
				}
				else {
					DEBUG_WARN("mcst sleep queue full!!\n");
				}
				goto free_and_stop;
			}
		}
	}

#ifdef GBWC
	if (priv->pmib->gbwcEntry.GBWCMode && pstat) {
		if (rtl8192cd_tx_gbwc(priv, pstat, skb) == TX_PROCEDURE_CTRL_STOP) {
			goto stop_proc;
		}
	}
#endif

#if defined(CONFIG_RTK_MESH) && !defined(MESH_AMSDU) && !defined(MESH_TX_SHORTCUT)
	if(dev == priv->mesh_dev)
		goto just_skip;
#endif


#ifdef SUPPORT_TX_AMSDU
	if (pstat && (pstat->aggre_mthd & AGGRE_MTHD_MSDU) && (pstat->amsdu_level > 0)
#ifdef SUPPORT_TX_MCAST2UNI
		&& (priv->pshare->rf_ft_var.mc2u_disable || (skb->cb[2] != (char)0xff))
#endif
		) {
		int ret = amsdu_check(priv, skb, pstat, txcfg);

		if (ret == RET_AGGRE_ENQUE)
			goto stop_proc;

		if (ret == RET_AGGRE_DESC_FULL)
			goto free_and_stop;
	}
#endif
#ifdef MESH_AMSDU
	if(dev == priv->mesh_dev)
		goto just_skip;
#endif

#ifdef SW_TX_QUEUE
	if ((priv->assoc_num > 1) && pstat)
    {
#ifdef MCAST2UI_REFINE
        if (memcmp(&skb->cb[10], priv->record_mac, 6))
#else
        if (memcmp(skb->data, priv->record_mac, 6))
#endif
    	{
        	priv->swq_txmac_chg++;
#ifdef MCAST2UI_REFINE
            memcpy(priv->record_mac, &skb->cb[10], 6);
#else
            memcpy(priv->record_mac, skb->data, 6);
#endif
       	}
        else
        {
        	int pri, q_num;
            pri = get_skb_priority(priv, skb);
#ifdef RTL_MANUAL_EDCA
            q_num = PRI_TO_QNUM(priv, pri);
#else
            PRI_TO_QNUM(pri, q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif
            if (priv->record_qnum != q_num)
            {
            	priv->swq_txmac_chg++;
                priv->record_qnum = q_num;
            }
        }
    }
#endif

#ifdef TX_SHORTCUT
	if (!priv->pmib->dot11OperationEntry.disable_txsc && pstat &&
		((k = get_tx_sc_index(priv, pstat, skb->data)) >= 0) &&
		((pstat->tx_sc_ent[k].txcfg.privacy == 0) ||
#ifdef CONFIG_RTL_WAPI_SUPPORT
		  (pstat->tx_sc_ent[k].txcfg.privacy == _WAPI_SMS4_) ||
#endif
		  !UseSwCrypto(priv, pstat->tx_sc_ent[k].txcfg.pstat, (pstat->tx_sc_ent[k].txcfg.pstat ? FALSE : TRUE)) ) &&
		(pstat->tx_sc_ent[k].txcfg.fr_len > 0) &&
		(get_skb_priority(priv, skb) == pstat->tx_sc_ent[k].pktpri) &&
		(FRAGTHRSLD > 1500))
	{
			int						*tx_head, *tx_tail, q_num;
			struct rtl8192cd_hw		*phw = GET_HW(priv);

			q_num = pstat->tx_sc_ent[k].txcfg.q_num;
			tx_head = get_txhead_addr(phw, q_num);
			tx_tail = get_txtail_addr(phw, q_num);
			// check if we need active tx tasklet
			if (CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) < CURRENT_NUM_TX_DESC/2) {
				if (!priv->pshare->has_triggered_tx_tasklet) {
#ifdef __KERNEL__
					tasklet_schedule(&priv->pshare->tx_tasklet);
#endif
					priv->pshare->has_triggered_tx_tasklet = 1;
				}
			}
			// Check if we need to reclaim TX-ring before processing TX
			if (CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) < 10) {
				rtl8192cd_tx_queueDsr(priv, q_num);
			}

#if defined(RESERVE_TXDESC_FOR_EACH_IF)
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			if ( GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc ) {
				if( check_txdesc_dynamic_mechanism(priv, q_num, 2) ) {
#ifdef USE_TXQUEUE
					if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable) {
						if (priv->pshare->txq_isr) {
							append_skb_to_txq_head(&priv->pshare->txq_list[q_num], priv, skb, dev, &priv->pshare->txq_pool);
							priv->pshare->txq_stop = 1;
						} else {
							append_skb_to_txq_tail(&priv->pshare->txq_list[q_num], priv, skb, dev, &priv->pshare->txq_pool);
						}
						priv->use_txq_cnt[q_num]++;
						priv->pshare->txq_check = 1;
					}
					else
#endif
					{
						DEBUG_WARN("%d hw Queue desc exceed available count: used:%d\n", q_num, priv->use_txdesc_cnt[q_num]);
						rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
					}
					goto stop_proc;
				}
			} else
#endif
#endif
			{
#ifdef TX_EARLY_MODE
			if (((GET_TX_EARLY_MODE && GET_EM_SWQ_ENABLE) ? 14 : 4) >
					 CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC))
#else
			if ((4) > CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC)) //per mpdu, we need 2 desc...
#endif
			{
				// 2 is for spare...
#ifdef USE_TXQUEUE
				if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable) {
					if (priv->pshare->txq_isr) {
						append_skb_to_txq_head(&priv->pshare->txq_list[q_num], priv, skb, dev, &priv->pshare->txq_pool);
						priv->pshare->txq_stop = 1;
					} else {
						append_skb_to_txq_tail(&priv->pshare->txq_list[q_num], priv, skb, dev, &priv->pshare->txq_pool);
					}
					priv->pshare->txq_check = 1;
				}
				else
#endif
				{
					DEBUG_ERR("%d hw Queue desc not available! head=%d, tail=%d request %d\n",q_num,*tx_head,*tx_tail,2);
					rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
				}
				goto stop_proc;
			}
			}

#if defined(MESH_TX_SHORTCUT)
			if((txcfg->is_11s&1) && !mesh_txsc_decision(txcfg, &pstat->tx_sc_ent[k].txcfg)) {
					goto just_skip;
			}
#endif
			memcpy(txcfg, &pstat->tx_sc_ent[k].txcfg, sizeof(struct tx_insn));

#ifdef CONFIG_RTL8672
#ifdef SUPPORT_TX_MCAST2UNI
			if (skb->cb[16] == TX_NO_MUL2UNI)
				txcfg->isMC2UC = 1;
			else
				txcfg->isMC2UC = 0;
#endif
#endif

			txcfg->phdr = (UINT8 *)get_wlanllchdr_from_poll(priv);
			if (txcfg->phdr == NULL) {
				DEBUG_ERR("Can't alloc wlan header!\n");
				rtl8192cd_tx_xmitSkbFail(priv, skb, dev, wdsDev, txcfg);
				goto stop_proc;
			}
			memcpy(txcfg->phdr, (const void *)&pstat->tx_sc_ent[k].wlanhdr, sizeof(struct wlanllc_hdr));
			txcfg->pframe = skb;
			txcfg->fr_len = skb->len - WLAN_ETHHDR_LEN;
			skb_pull(skb, WLAN_ETHHDR_LEN);

#ifdef CONFIG_RTL_WAPI_SUPPORT
#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
			if((txcfg->privacy==_WAPI_SMS4_)&&(txcfg->llc>0)&&(UseSwCrypto(priv, pstat, (pstat ? FALSE : TRUE))))
#else
			if((txcfg->privacy==_WAPI_SMS4_)&&(txcfg->llc>0))
#endif
			{				
				//To restore not-encrypted llc in wlan hdr
				//because llc in wlan hdr has been sms4encrypted to deliver at SecSWSMS4Encryption()
				eth_2_llc(&pstat->tx_sc_ent[k].ethhdr, (struct llc_snap *)(txcfg->phdr+txcfg->hdr_len + txcfg->iv));
			}
#endif

			if (txcfg->privacy == _TKIP_PRIVACY_) {
				if (rtl8192cd_tx_tkip(priv, skb, pstat, txcfg) == TX_PROCEDURE_CTRL_STOP) {
					goto stop_proc;
				}
			}

#ifdef MESH_TX_SHORTCUT
			if ( (txcfg->is_11s&1) && (GetFrameSubType(txcfg->phdr) == WIFI_11S_MESH))
                    if( !reuse_meshhdr(priv, txcfg) ) {
							txsc_debug("Mesh reuse header\n");
					        goto stop_proc;
					}
#endif
			txcfg->tx_rate = get_tx_rate(priv, pstat);
			txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);
			// log tx statistics...

#ifdef CONFIG_RTL8672
			tx_sum_up(priv, pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv, txcfg);
#else
			tx_sum_up(priv, pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv);
#endif
			SNMP_MIB_INC(dot11TransmittedFragmentCount, 1);
			pstat->tx_sc_pkts_lv1++;

#ifdef PCIE_POWER_SAVING
			PCIeWakeUp(priv, (POWER_DOWN_T0));
#endif

			// for SW LED
			priv->pshare->LED_tx_cnt++;

			if (
#ifdef CONFIG_RTL_8812_SUPPORT
//			( (GET_CHIP_VER(priv)== VERSION_8812E) && ( AMSDU_ENABLE && AMPDU_ENABLE && txcfg->aggre_en )) || 
			( txcfg->pstat && (txcfg->pstat->aggre_mthd == AGGRE_MTHD_MPDU_AMSDU) && txcfg->aggre_en ) ||
#endif
			((txcfg->aggre_en >= FG_AGGRE_MPDU) && (txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST))
			) {
				if (!pstat->ADDBA_ready[(int)skb->cb[1]]) {
					if ((pstat->ADDBA_req_num[(int)skb->cb[1]] < 5) && !pstat->ADDBA_sent[(int)skb->cb[1]]) {
						pstat->ADDBA_req_num[(int)skb->cb[1]]++;
						issue_ADDBAreq(priv, pstat, (int)skb->cb[1]);
						pstat->ADDBA_sent[(int)skb->cb[1]]++;
					}
				}
			}

			// check if we could re-use tx descriptor
			if ((get_desc(pstat->tx_sc_ent[k].hwdesc1.Dword7)&TX_TxBufSizeMask) > 0 &&
				((
#ifdef CONFIG_RTL_WAPI_SUPPORT
				// Note: for sw wapi, txcfg->mic=16; for hw wapi, txcfg->mic=0.
				 (txcfg->privacy==_WAPI_SMS4_) ? ((skb->len+txcfg->mic)==(get_desc(pstat->tx_sc_ent[k].hwdesc2.Dword7)&TX_TxBufSizeMask)) :
#endif
				(skb->len == (get_desc(pstat->tx_sc_ent[k].hwdesc2.Dword7)&TX_TxBufSizeMask)))) &&
				(txcfg->tx_rate == pstat->tx_sc_ent[k].txcfg.tx_rate) &&
				(pstat->protection == priv->pmib->dot11ErpInfo.protection) &&
				(pstat->ht_protection == priv->ht_protection)
#if defined(WIFI_WMM) && defined(WMM_APSD)
				&& (!(
#ifdef CLIENT_MODE
				(OPMODE & WIFI_AP_STATE) &&
#endif
				(APSD_ENABLE) && (pstat->state & WIFI_SLEEP_STATE)))
#endif
				) {

				pstat->tx_sc_pkts_lv2++;

				if (txcfg->privacy) {
					switch (txcfg->privacy) {
					case _WEP_104_PRIVACY_:
					case _WEP_40_PRIVACY_:
						wep_fill_iv(priv, txcfg->phdr, txcfg->hdr_len, pstat->tx_sc_ent[k].sc_keyid);
						break;

					case _TKIP_PRIVACY_:
						tkip_fill_encheader(priv, txcfg->phdr, txcfg->hdr_len, pstat->tx_sc_ent[k].sc_keyid);
						break;

					case _CCMP_PRIVACY_:
						aes_fill_encheader(priv, txcfg->phdr, txcfg->hdr_len, pstat->tx_sc_ent[k].sc_keyid);
						break;
					}
				}


				{
			
					if ((skb_headroom(skb) >= (txcfg->hdr_len + txcfg->llc + txcfg->iv
#ifdef TX_EARLY_MODE
						+ (GET_TX_EARLY_MODE ? 8 : 0)
#endif
						)) &&
						!skb_cloned(skb) &&
						(txcfg->privacy != _TKIP_PRIVACY_)
#if defined(CONFIG_RTL_WAPI_SUPPORT)
						&& (txcfg->privacy != _WAPI_SMS4_)
#endif
// The patch for "Kernel unaligned instruction access" issue
#if 1
						&& ((((unsigned int)skb->data) % 2) == 0)
#endif
						)
					{
						memcpy((skb->data - (txcfg->hdr_len + txcfg->llc + txcfg->iv)), txcfg->phdr, (txcfg->hdr_len + txcfg->llc + txcfg->iv));
						release_wlanllchdr_to_poll(priv, txcfg->phdr);
						txcfg->phdr = skb->data - (txcfg->hdr_len + txcfg->llc + txcfg->iv);
						txcfg->one_txdesc = 1;
					}
				}

#if defined(RESERVE_TXDESC_FOR_EACH_IF)
#if (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
				if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
					priv->use_txdesc_cnt[q_num] += (txcfg->one_txdesc)? 1 : 2;
					if ( rtl8192cd_signin_txdesc_shortcut(priv, txcfg, k))
						priv->use_txdesc_cnt[q_num] -= (txcfg->one_txdesc)? 1 : 2;
				} else
#endif
#endif
				{
				rtl8192cd_signin_txdesc_shortcut(priv, txcfg, k);
				}
				goto stop_proc;
			}

#if defined(RESERVE_TXDESC_FOR_EACH_IF)
#if (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
			if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
				int desc_num=2;	
				if ( txcfg->privacy
#if defined(CONFIG_RTL_WAPI_SUPPORT)
						&& (_WAPI_SMS4_ != txcfg->privacy)
#endif
						&& UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)) ){
					desc_num=3;
				}
				priv->use_txdesc_cnt[q_num] += desc_num;
				if ( rtl8192cd_signin_txdesc(priv, txcfg)) {
					priv->use_txdesc_cnt[q_num] -= desc_num;
				}
			} else
#endif
#endif
			{
#ifdef CONFIG_RTK_MESH
				rtl8192cd_signin_txdesc(priv, txcfg, NULL);
#else
				rtl8192cd_signin_txdesc(priv, txcfg);
#endif
			}

			pstat->tx_sc_ent[k].txcfg.tx_rate = txcfg->tx_rate;
			goto stop_proc;
	}

	if (!priv->pmib->dot11OperationEntry.disable_txsc && pstat)
		pstat->tx_sc_pkts_slow++;
#endif // TX_SHORTCUT

#ifdef CONFIG_RTK_MESH
just_skip:
#endif
	/* ==================== Slow path of packet TX process ==================== */
	if (rtl8192cd_tx_slowPath(priv, skb, pstat, dev, wdsDev, txcfg) == TX_PROCEDURE_CTRL_STOP) {
		goto stop_proc;
	}

#ifdef __KERNEL__
	dev->trans_start = jiffies;
#endif

#ifdef A4_STA
	if ((OPMODE & WIFI_AP_STATE) && IS_MCAST(da) && 
			priv->pshare->rf_ft_var.a4_enable && !list_empty(&priv->a4_sta_list)) {
		struct list_head *phead, *plist;

		skb_push(skb, WLAN_ETHHDR_LEN);
		
		phead = &priv->a4_sta_list;
		plist = phead->next;

		while (plist != phead) {
			struct stat_info *dst_stat;
			
			pstat = list_entry(plist, struct stat_info, a4_sta_list);
			ASSERT(pstat->state & WIFI_A4_STA);

			dst_stat = a4_sta_lookup(priv, skb->data+MACADDRLEN);
			if (pstat != dst_stat)			
				send_br_packet(priv, skb, pstat);
			
			plist = plist->next;
		}
	}
#endif

	goto stop_proc;

free_and_stop:		/* Free current packet and stop TX process */

		dev_kfree_skb_any(skb);

stop_proc:			/* Stop process and assume the TX-ed packet is already "processed" (freed or TXed) in previous code. */
	return 0;
}


#ifndef CONFIG_RTK_MESH
int	rtl8192cd_wlantx(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
{
	return (rtl8192cd_firetx(priv, txcfg));
}
#endif


#ifdef TX_SHORTCUT


#ifdef CONFIG_RTL_8812_SUPPORT
__MIPS16
__IRAM_IN_865X
int rtl8192cd_signin_txdesc_shortcut_8812(struct rtl8192cd_priv *priv, struct tx_insn *txcfg, int idx)
{
	struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info *pswdescinfo, *pdescinfo;
	struct rtl8192cd_hw	*phw;
	int *tx_head, q_num;
	struct stat_info *pstat;
	struct sk_buff *pskb;
	unsigned int pfrst_dma_desc;
	unsigned int *dma_txhead;
/*
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	uint8				*wapiMic2;
	struct tx_desc		*pmicdesc;
	struct tx_desc_info	*pmicdescinfo;
#endif
*/
	pstat = txcfg->pstat;
	pskb = (struct sk_buff *)txcfg->pframe;
	pfrst_dma_desc=0;

	phw	= GET_HW(priv);
	q_num = txcfg->q_num;

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc		= get_txdesc(phw, q_num);
	pswdescinfo	= get_txdesc_info(priv->pshare->pdesc_info, q_num);

	/*------------------------------------------------------------*/
	/*           fill descriptor of header + iv + llc             */
	/*------------------------------------------------------------*/
	pfrstdesc = pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;

    memcpy(pdesc, &pstat->tx_sc_ent[idx].hwdesc1, 40);

	assign_wlanseq(GET_HW(priv), txcfg->phdr, pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
	, txcfg->is_11s
#endif
		);

	pdesc->Dword9 = 0;
	pdesc->Dword9 |= set_desc((GetSequence(txcfg->phdr) & TXdesc_92E_TX_SeqMask)  << TXdesc_92E_TX_SeqSHIFT);	

//	if (txcfg->pstat)
//		pdesc->Dword1 |= set_desc(txcfg->pstat->aid & TX_MACIDMask);

	//set Break
	if((txcfg->q_num >=1 && txcfg->q_num <=4)){
		if((pstat != priv->pshare->CurPstat[txcfg->q_num-1])) {

				pdesc->Dword2 |= set_desc(TXdesc_92E_BK);

			priv->pshare->CurPstat[txcfg->q_num-1] = pstat;
		} else
			pdesc->Dword2 &= set_desc(~TXdesc_92E_BK);

	} else {
			pdesc->Dword2 |= set_desc(TXdesc_92E_BK);
	}


	if(priv->pshare->rf_ft_var.txforce != 0xff)
	{
		pdesc->Dword4 &= set_desc(~(TXdesc_92E_DataRateMask << TXdesc_92E_DataRateSHIFT));
		pdesc->Dword3 |= set_desc(TXdesc_92E_DisDataFB|TXdesc_92E_DisRtsFB|TXdesc_92E_UseRate);
		pdesc->Dword4 |= set_desc((priv->pshare->rf_ft_var.txforce & TXdesc_92E_DataRateMask) << TXdesc_92E_DataRateSHIFT);
	}


	if (txcfg->one_txdesc) {
#ifdef TX_EARLY_MODE
		if (GET_TX_EARLY_MODE) {
			pdesc->Dword0 = set_desc(((get_desc(pdesc->Dword0) & 0xff00ffff) |(0x28 << TX_OffsetSHIFT)) |
									TX_LastSeg | 	(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
			pdesc->Dword1 = set_desc(get_desc(pdesc->Dword1) | (1 << TX_PktOffsetSHIFT) );
			pdesc->Dword7 = set_desc((get_desc(pdesc->Dword7) & 0xffff0000) |
						(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len+8));
			memset(txcfg->phdr-8, '\0', 8);			
			if (pstat->empkt_num > 0) 				
				insert_emcontent(priv, txcfg, txcfg->phdr-8);
			pdesc->Dword10 = set_desc(get_physical_addr(priv, txcfg->phdr-8,
				(get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE));
					
		}
		else
#endif
		{	
			pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xffff0000) |
				TX_LastSeg | (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
			pdesc->Dword7 = set_desc((get_desc(pdesc->Dword7) & 0xffff0000) |
				(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
		}
	}

#ifdef TX_EARLY_MODE
	if (GET_TX_EARLY_MODE) 
		pdesc->Dword10 = set_desc(get_physical_addr(priv, txcfg->phdr-8,
			(get_desc(pdesc->Dword7)& TX_TxBufSizeMask), PCI_DMA_TODEVICE));
	else
#endif
	pdesc->Dword10 = set_desc(get_physical_addr(priv, txcfg->phdr,
		(get_desc(pdesc->Dword7)& TX_TxBufSizeMask), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->tx_sc_ent[idx].swdesc1);
	pdescinfo->paddr  = get_desc(pdesc->Dword10); // buffer addr
	if (txcfg->one_txdesc) {
		pdescinfo->type = _SKB_FRAME_TYPE_;
		pdescinfo->pframe = pskb;
		pdescinfo->priv = priv;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->pstat = pstat;
#endif
	}
	else {
		pdescinfo->pframe = txcfg->phdr;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = pstat;
#endif
	}

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		if (GetFrameSubType(pdescinfo->pframe) == WIFI_PSPOLL)
			pdesc->Dword1 |= set_desc(TX_NAVUSEHDR);

		if (priv->ps_state)
			SetPwrMgt(pdescinfo->pframe);
		else
			ClearPwrMgt(pdescinfo->pframe);
	}
#endif

	pfrst_dma_desc = dma_txhead[*tx_head];
/*
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	if (txcfg->one_txdesc)
		goto one_txdesc;

	/*------------------------------------------------------------*/
	/*              fill descriptor of frame body                 */
	/*------------------------------------------------------------*/
	pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
    memcpy(pdesc, &pstat->tx_sc_ent[idx].hwdesc2, 40);

	pdesc->Dword10 = set_desc(get_physical_addr(priv, pskb->data,
		(get_desc(pdesc->Dword7)&0x0fff), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->tx_sc_ent[idx].swdesc2);

	pdescinfo->paddr  = get_desc(pdesc->Dword10);
	pdescinfo->pframe = pskb;
	pdescinfo->priv = priv;
/*
#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else			
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (txcfg->privacy == _WAPI_SMS4_)
	{
		SecSWSMS4Encryption(priv, txcfg);
	} else
#endif

	if ((txcfg->privacy == _TKIP_PRIVACY_) &&
		(priv->pshare->have_hw_mic) &&
		!(priv->pmib->dot11StationConfigEntry.swTkipMic))
	{
		register unsigned long int l,r;
		unsigned char *mic;
		int delay = 18;

		while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0) {
			delay_us(delay);
			delay = delay / 2;
		}

		l = *(volatile unsigned int *)GDMAICVL;
		r = *(volatile unsigned int *)GDMAICVR;

		mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
		mic[0] = (unsigned char)(l & 0xff);
		mic[1] = (unsigned char)((l >> 8) & 0xff);
		mic[2] = (unsigned char)((l >> 16) & 0xff);
		mic[3] = (unsigned char)((l >> 24) & 0xff);
		mic[4] = (unsigned char)(r & 0xff);
		mic[5] = (unsigned char)((r >> 8) & 0xff);
		mic[6] = (unsigned char)((r >> 16) & 0xff);
		mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			mic[7] ^= mic[7];
			priv->micerr_flag = 0;
		}
#endif
	}

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword10), (get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE);
#endif

one_txdesc:

#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pfrstdesc->Dword10), (get_desc(pfrstdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE);
#endif

#ifdef SUPPORT_SNMP_MIB
	if (txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len))
		SNMP_MIB_INC(dot11RTSSuccessCount, 1);
#endif

	pfrstdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, pfrst_dma_desc, sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	if (q_num == HIGH_QUEUE) {
		DEBUG_WARN("signin shortcut for DTIM pkt?\n");
		return 0;
	} else {
		tx_poll(priv, q_num);
	}

	return 0;
}
#endif

__MIPS16
__IRAM_IN_865X
int rtl8192cd_signin_txdesc_shortcut(struct rtl8192cd_priv *priv, struct tx_insn *txcfg, int idx)
{
	struct tx_desc *phdesc, *pdesc, *pfrstdesc;
	struct tx_desc_info *pswdescinfo, *pdescinfo;
	struct rtl8192cd_hw	*phw;
	int *tx_head, q_num;
	struct stat_info *pstat;
	struct sk_buff *pskb;
	unsigned int pfrst_dma_desc;
	unsigned int *dma_txhead;
/*
#if defined(CONFIG_RTL_WAPI_SUPPORT)
	uint8				*wapiMic2;
	struct tx_desc		*pmicdesc;
	struct tx_desc_info	*pmicdescinfo;
#endif
*/

#ifdef CONFIG_RTL_8812_SUPPORT
	if(GET_CHIP_VER(priv)== VERSION_8812E)
		return rtl8192cd_signin_txdesc_shortcut_8812(priv, txcfg, idx);
#endif

	pstat = txcfg->pstat;
	pskb = (struct sk_buff *)txcfg->pframe;
	pfrst_dma_desc=0;

	phw	= GET_HW(priv);
	q_num = txcfg->q_num;

	dma_txhead	= get_txdma_addr(phw, q_num);
	tx_head		= get_txhead_addr(phw, q_num);
	phdesc		= get_txdesc(phw, q_num);
	pswdescinfo	= get_txdesc_info(priv->pshare->pdesc_info, q_num);

	/*------------------------------------------------------------*/
	/*           fill descriptor of header + iv + llc             */
	/*------------------------------------------------------------*/
	pfrstdesc = pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	
	desc_copy(pdesc, &pstat->tx_sc_ent[idx].hwdesc1);
	assign_wlanseq(GET_HW(priv), txcfg->phdr, pstat, GET_MIB(priv)
#ifdef CONFIG_RTK_MESH	// For broadcast data frame via mesh (ex:ARP requst)
	, txcfg->is_11s
#endif
		);

			
	pdesc->Dword3 = 0;
	pdesc->Dword3 = set_desc((GetSequence(txcfg->phdr) & TX_SeqMask) << TX_SeqSHIFT);

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	if ((GET_CHIP_VER(priv) == VERSION_8192C) ||  (GET_CHIP_VER(priv) == VERSION_8192D)|| (GET_CHIP_VER(priv) == VERSION_8188C))
#endif
	if (priv->pmib->dot11RFEntry.txbf == 1) {
		pdesc->Dword2 &= set_desc(0x03ffffff); // clear related bits
		pdesc->Dword2 |= set_desc(1 << TX_TxAntCckSHIFT);	// Set Default CCK rate with 1T
		pdesc->Dword2 |= set_desc(1 << TX_TxAntlSHIFT); 	// Set Default Legacy rate with 1T
		pdesc->Dword2 |= set_desc(1 << TX_TxAntHtSHIFT);	// Set Default Ht rate

		if (is_MCS_rate(txcfg->tx_rate)) {
			if ((txcfg->tx_rate & 0x7f) <= 6){
					pdesc->Dword2 |= set_desc(3 << TX_TxAntHtSHIFT); // Set Ht rate < MCS6 with 2T
			}
		}
	}

//	if (txcfg->pstat)
//		pdesc->Dword1 |= set_desc(txcfg->pstat->aid & TX_MACIDMask);

	//set Break
	if((txcfg->q_num >=1 && txcfg->q_num <=4)){
		if((pstat != priv->pshare->CurPstat[txcfg->q_num-1])) {
#ifdef CONFIG_RTL_88E_SUPPORT
			if (GET_CHIP_VER(priv)==VERSION_8188E)
				pdesc->Dword2 |= set_desc(TXdesc_88E_BK);
			else
#endif
				pdesc->Dword1 |= set_desc(TX_BK);
			priv->pshare->CurPstat[txcfg->q_num-1] = pstat;
		} else
#ifdef CONFIG_RTL_88E_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8188E) {
			pdesc->Dword2 &= set_desc(~TXdesc_88E_BK);
		} else
#endif
		{
			pdesc->Dword1 &= set_desc(~TX_BK); // clear it
		}
	} else {
#ifdef CONFIG_RTL_88E_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8188E)
			pdesc->Dword2 |= set_desc(TXdesc_88E_BK);
		else
#endif
			pdesc->Dword1 |= set_desc(TX_BK);
	}


#ifdef _OUTSRC_COEXIST
	if((IS_OUTSRC_CHIP(priv)&&(pstat->IOTPeer==HT_IOT_PEER_INTEL)) || (!IS_OUTSRC_CHIP(priv) && (pstat->is_intel_sta)))
#else
#ifdef USE_OUT_SRC	
	if (pstat->IOTPeer==HT_IOT_PEER_INTEL)
#else
	if (pstat->is_intel_sta)
#endif
#endif
	{
		if (is_MCS_rate(pstat->current_tx_rate) && !(pstat->leave)
			&& priv->pshare->intel_rty_lmt) {
			pdesc->Dword5 |= set_desc(TX_RtyLmtEn);
			pdesc->Dword5 |= set_desc((priv->pshare->intel_rty_lmt & TX_DataRtyLmtMask) << TX_DataRtyLmtSHIFT);
		} else {
			pdesc->Dword5 &= set_desc(~TX_RtyLmtEn);
			pdesc->Dword5 &= set_desc(~(TX_DataRtyLmtMask << TX_DataRtyLmtSHIFT));
		}
	}

#if defined(CONFIG_RTL_88E_SUPPORT) && defined(TXREPORT)
	if ((GET_CHIP_VER(priv)==VERSION_8188E) && (!txcfg->fixed_rate)) {
		if (pstat->ht_current_tx_info & TX_USE_SHORT_GI)
			pdesc->Dword5 |= set_desc(TX_SGI);
		else
			pdesc->Dword5 &= set_desc(~TX_SGI);
	}
#endif

	if (txcfg->one_txdesc) {
#ifdef TX_EARLY_MODE
		if (GET_TX_EARLY_MODE) {
			pdesc->Dword0 = set_desc(((get_desc(pdesc->Dword0) & 0xff00ffff) |(0x28 << TX_OffsetSHIFT)) |
									TX_LastSeg | 	(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
			pdesc->Dword1 = set_desc(get_desc(pdesc->Dword1) | (1 << TX_PktOffsetSHIFT) );
			pdesc->Dword7 = set_desc((get_desc(pdesc->Dword7) & 0xffff0000) |
						(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len+8));
#ifdef CONFIG_RTL_88E_SUPPORT
			if (GET_CHIP_VER(priv) == VERSION_8188E) {
				pdesc->Dword6 &= set_desc(~ (0xf << TX_MaxAggNumSHIFT));
				pdesc->Dword6 |= set_desc(0xf << TX_MaxAggNumSHIFT);
			}
#endif
			memset(txcfg->phdr-8, '\0', 8);			
			if (pstat->empkt_num > 0) 				
				insert_emcontent(priv, txcfg, txcfg->phdr-8);
			pdesc->Dword8 = set_desc(get_physical_addr(priv, txcfg->phdr-8,
				(get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE));
					
		}
		else
#endif
		{	
#if defined(TX_EARLY_MODE) && defined(CONFIG_RTL_88E_SUPPORT)
			if (GET_CHIP_VER(priv) == VERSION_8188E)
				pdesc->Dword6 &= set_desc(~ (0x0f << TX_MaxAggNumSHIFT));
#endif
			pdesc->Dword0 = set_desc((get_desc(pdesc->Dword0) & 0xffff0000) |
				TX_LastSeg | (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
			pdesc->Dword7 = set_desc((get_desc(pdesc->Dword7) & 0xffff0000) |
				(txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->fr_len));
		}
	}

	if(priv->pshare->rf_ft_var.txforce != 0xff)
	{
		pdesc->Dword5 &= set_desc(~(TX_DataRateMask << TX_DataRateSHIFT));
		pdesc->Dword4 |= set_desc(TX_UseRate);
		pdesc->Dword5 |= set_desc((priv->pshare->rf_ft_var.txforce & TX_DataRateMask) << TX_DataRateSHIFT);
	}

#ifdef TX_EARLY_MODE
	if (GET_TX_EARLY_MODE) 
		pdesc->Dword8 = set_desc(get_physical_addr(priv, txcfg->phdr-8,
			(get_desc(pdesc->Dword7)& TX_TxBufSizeMask), PCI_DMA_TODEVICE));
	else
#endif
	pdesc->Dword8 = set_desc(get_physical_addr(priv, txcfg->phdr,
		(get_desc(pdesc->Dword7)& TX_TxBufSizeMask), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->tx_sc_ent[idx].swdesc1);
	pdescinfo->paddr  = get_desc(pdesc->Dword8); // buffer addr
	if (txcfg->one_txdesc) {
		pdescinfo->type = _SKB_FRAME_TYPE_;
		pdescinfo->pframe = pskb;
		pdescinfo->priv = priv;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->pstat = pstat;
#endif
	}
	else {
		pdescinfo->pframe = txcfg->phdr;
#if defined(WIFI_WMM) && defined(WMM_APSD)
		pdescinfo->priv = priv;
		pdescinfo->pstat = pstat;
#endif
	}

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		if (GetFrameSubType(pdescinfo->pframe) == WIFI_PSPOLL)
			pdesc->Dword1 |= set_desc(TX_NAVUSEHDR);

		if (priv->ps_state)
			SetPwrMgt(pdescinfo->pframe);
		else
			ClearPwrMgt(pdescinfo->pframe);
	}
#endif

	pfrst_dma_desc = dma_txhead[*tx_head];
/*
#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

	if (txcfg->one_txdesc)
		goto one_txdesc;

	/*------------------------------------------------------------*/
	/*              fill descriptor of frame body                 */
	/*------------------------------------------------------------*/
	pdesc = phdesc + *tx_head;
	pdescinfo = pswdescinfo + *tx_head;
	desc_copy(pdesc, &pstat->tx_sc_ent[idx].hwdesc2);

	pdesc->Dword8 = set_desc(get_physical_addr(priv, pskb->data,
		(get_desc(pdesc->Dword7)&0x0fff), PCI_DMA_TODEVICE));

	descinfo_copy(pdescinfo, &pstat->tx_sc_ent[idx].swdesc2);
	pdescinfo->paddr  = get_desc(pdesc->Dword8);
	pdescinfo->pframe = pskb;
	pdescinfo->priv = priv;
/*
#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, dma_txhead[*tx_head], sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), (get_desc(pdesc->Dword7)&TX_TxBufferSizeMask), PCI_DMA_TODEVICE);
#endif
*/
	txdesc_rollover(pdesc, (unsigned int *)tx_head);

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	if (txcfg->privacy == _WAPI_SMS4_)
	{
		SecSWSMS4Encryption(priv, txcfg);
	} else
#endif

	if ((txcfg->privacy == _TKIP_PRIVACY_) &&
		(priv->pshare->have_hw_mic) &&
		!(priv->pmib->dot11StationConfigEntry.swTkipMic))
	{
		register unsigned long int l,r;
		unsigned char *mic;
		int delay = 18;

		while ((*(volatile unsigned int *)GDMAISR & GDMA_COMPIP) == 0) {
			delay_us(delay);
			delay = delay / 2;
		}

		l = *(volatile unsigned int *)GDMAICVL;
		r = *(volatile unsigned int *)GDMAICVR;

		mic = ((struct sk_buff *)txcfg->pframe)->data + txcfg->fr_len - 8;
		mic[0] = (unsigned char)(l & 0xff);
		mic[1] = (unsigned char)((l >> 8) & 0xff);
		mic[2] = (unsigned char)((l >> 16) & 0xff);
		mic[3] = (unsigned char)((l >> 24) & 0xff);
		mic[4] = (unsigned char)(r & 0xff);
		mic[5] = (unsigned char)((r >> 8) & 0xff);
		mic[6] = (unsigned char)((r >> 16) & 0xff);
		mic[7] = (unsigned char)((r >> 24) & 0xff);

#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			mic[7] ^= mic[7];
			priv->micerr_flag = 0;
		}
#endif
	}

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(dma_txhead[*tx_head]), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#else
	rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(get_desc(pdesc->Dword8)), (get_desc(pdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE);
#endif

one_txdesc:

#ifdef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(get_desc(pfrstdesc->Dword8)), (get_desc(pfrstdesc->Dword7)&TX_TxBufSizeMask), PCI_DMA_TODEVICE);
#endif

#ifdef SUPPORT_SNMP_MIB
	if (txcfg->rts_thrshld <= get_mpdu_len(txcfg, txcfg->fr_len))
		SNMP_MIB_INC(dot11RTSSuccessCount, 1);
#endif

	pfrstdesc->Dword0 |= set_desc(TX_OWN);

#ifndef USE_RTL8186_SDK
	rtl_cache_sync_wback(priv, (unsigned int)bus_to_virt(pfrst_dma_desc), sizeof(struct tx_desc), PCI_DMA_TODEVICE);
#endif

	if (q_num == HIGH_QUEUE) {
		DEBUG_WARN("signin shortcut for DTIM pkt?\n");
		return 0;
	} else {
		tx_poll(priv, q_num);
	}

	return 0;
}
#endif // TX_SHORTCUT


/* This sub-routine is gonna to check how many tx desc we need */
static int check_txdesc(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
{
	struct sk_buff 	*pskb=NULL;
	unsigned short  protocol;
	unsigned char   *da=NULL;
	struct stat_info	*pstat=NULL;
	int priority=0;
	unsigned int is_dhcp = 0;

	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST)
		return TRUE;

	txcfg->privacy = txcfg->iv = txcfg->icv = txcfg->mic = 0;
	txcfg->frg_num = 0;
	txcfg->need_ack = 1;

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	{
		pskb = ((struct sk_buff *)txcfg->pframe);
		txcfg->fr_len = pskb->len - WLAN_ETHHDR_LEN;

#ifdef MP_TEST
		if (OPMODE & WIFI_MP_STATE) {
			txcfg->hdr_len = WLAN_HDR_A3_LEN;
			txcfg->frg_num = 1;
			if (IS_MCAST(pskb->data))
				txcfg->need_ack = 0;
			return TRUE;
		}
#endif

#ifdef WDS
		if (txcfg->wdsIdx >= 0) {
			txcfg->hdr_len = WLAN_HDR_A4_LEN;
			pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
			if (pstat == NULL) {
				DEBUG_ERR("TX DROP: %s: get_stainfo() for wds failed [%d]!\n", (char *)__FUNCTION__, txcfg->wdsIdx);
				return FALSE;
			}

			txcfg->privacy = priv->pmib->dot11WdsInfo.wdsPrivacy;
			switch (txcfg->privacy) {
				case _WEP_40_PRIVACY_:
				case _WEP_104_PRIVACY_:
					txcfg->iv = 4;
					txcfg->icv = 4;
					break;
				case _TKIP_PRIVACY_:
					txcfg->iv = 8;
					txcfg->icv = 4;
					txcfg->mic = 0;
					txcfg->fr_len += 8; // for Michael padding
					break;
				case _CCMP_PRIVACY_:
					txcfg->iv = 8;
					txcfg->icv = 0;
					txcfg->mic = 8;
					break;
			}
			txcfg->frg_num = 1;
			if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
				priority = get_skb_priority(priv, (struct sk_buff *)txcfg->pframe);
#ifdef RTL_MANUAL_EDCA
				txcfg->q_num = PRI_TO_QNUM(priv, priority);
#else
				PRI_TO_QNUM(priority, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif
			}

			txcfg->tx_rate = get_tx_rate(priv, pstat);
			txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);

			if (priv->pmib->dot11WdsInfo.entry[pstat->wds_idx].txRate)
				txcfg->fixed_rate = 1;
			txcfg->need_ack = 1;
			txcfg->pstat = pstat;
#ifdef WIFI_WMM
			if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled) && (is_qos_data(txcfg->phdr)))
				txcfg->hdr_len = WLAN_HDR_A4_QOS_LEN;
#endif

			if (txcfg->aggre_en == 0) {
				if ((pstat->aggre_mthd == AGGRE_MTHD_MPDU) && is_MCS_rate(txcfg->tx_rate))
					txcfg->aggre_en = FG_AGGRE_MPDU;
			}

			return TRUE;
		}
#endif

#ifdef WIFI_WMM
		if (OPMODE & WIFI_AP_STATE) {
#ifdef MCAST2UI_REFINE
			pstat = get_stainfo(priv, &pskb->cb[10]);
#else
			pstat = get_stainfo(priv, pskb->data);
#endif
#ifdef A4_STA
			if (pstat == NULL) {
				if (txcfg->pstat && (txcfg->pstat->state & WIFI_A4_STA)) 
					pstat = txcfg->pstat;
				else if (!IS_MCAST(pskb->data) &&  priv->pshare->rf_ft_var.a4_enable)
					pstat = a4_sta_lookup(priv, pskb->data);
				if (pstat) 
					da = pstat->hwaddr;
			}
#endif
		}
		else if (OPMODE & WIFI_STATION_STATE)
			pstat = get_stainfo(priv, BSSID);
		else if (OPMODE & WIFI_ADHOC_STATE)
				pstat = get_stainfo(priv, pskb->data);

		if ((pstat) && (QOS_ENABLE) && (pstat->QosEnabled) && (is_qos_data(txcfg->phdr))) {
			txcfg->hdr_len = WLAN_HDR_A3_QOS_LEN;
		}
		else
#endif
		{
			txcfg->hdr_len = WLAN_HDR_A3_LEN;
		}

#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
		{
			txcfg->hdr_len = WLAN_HDR_A4_QOS_LEN ;
			da = txcfg->nhop_11s;
		}
		else
#endif

#ifdef A4_STA
		if (!da)
#endif
#ifdef MCAST2UI_REFINE
			da = &pskb->cb[10];
#else
			da = pskb->data;
#endif

		//check if da is associated, if not, just drop and return false
		if (!IS_MCAST(da)
#ifdef CLIENT_MODE
			|| (OPMODE & WIFI_STATION_STATE)
#endif
#ifdef A4_STA
			|| (pstat && (pstat->state & WIFI_A4_STA))
#endif		
			)
		{
#ifdef A4_STA
			if (!(pstat && (pstat->state & WIFI_A4_STA)))
#endif
		{
#ifdef CLIENT_MODE
			if (OPMODE & WIFI_STATION_STATE)
				pstat = get_stainfo(priv, BSSID);
			else
#endif
			{
				pstat = get_stainfo(priv, da);
			}
			}

			if ((pstat == NULL) || (!(pstat->state & WIFI_ASOC_STATE)))
			{
				DEBUG_ERR("TX DROP: Non asoc tx request!\n");
				return FALSE;
			}
#ifdef A4_STA
			if (pstat->state & WIFI_A4_STA)
				txcfg->hdr_len += WLAN_ADDR_LEN;
#endif
			protocol = ntohs(*((UINT16 *)((UINT8 *)pskb->data + ETH_ALEN*2)));

			if ((((protocol == 0x888E) && ((GET_UNICAST_ENCRYP_KEYLEN == 0)
#ifdef WIFI_SIMPLE_CONFIG
				|| (pstat->state & WIFI_WPS_JOIN)
#endif
				))
#ifdef CONFIG_RTL_WAPI_SUPPORT
				 || (protocol == ETH_P_WAPI)
#endif

#ifdef BEAMFORMING_SUPPORT
				|| (txcfg->ndpa) 
#endif
				))
				txcfg->privacy = 0;
			else
				txcfg->privacy = get_privacy(priv, pstat, &txcfg->iv, &txcfg->icv, &txcfg->mic);

			if ((OPMODE & WIFI_AP_STATE) && !IS_MCAST(da) && pskb && (isDHCPpkt(pskb) == TRUE))
				is_dhcp++;

			if ((protocol == 0x888E)
#ifdef CONFIG_RTL_WAPI_SUPPORT
				||(protocol == ETH_P_WAPI)
#endif
			|| is_dhcp) {
				// use basic rate to send EAP packet for sure
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
				txcfg->lowest_tx_rate = txcfg->tx_rate;
				txcfg->fixed_rate = 1;
			} else {
				txcfg->tx_rate = get_tx_rate(priv, pstat);
				txcfg->lowest_tx_rate = get_lowest_tx_rate(priv, pstat, txcfg->tx_rate);
				if (!priv->pmib->dot11StationConfigEntry.autoRate &&
					!(should_restrict_Nrate(priv, pstat) && is_fixedMCSTxRate(priv)))
#ifdef CONFIG_RTL_8812_SUPPORT // ??
				if( (GET_CHIP_VER(priv)== VERSION_8812E) &&(! is_fixedVHTTxRate(priv) || (pstat->vht_cap_len)))
#endif

					txcfg->fixed_rate = 1;
			}

			if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
#ifdef CONFIG_RTK_MESH
				priority = get_skb_priority3(priv, (struct sk_buff *)txcfg->pframe, txcfg->is_11s);
#else
				priority = get_skb_priority(priv, (struct sk_buff *)txcfg->pframe);
#endif
#ifdef RTL_MANUAL_EDCA
				txcfg->q_num = PRI_TO_QNUM(priv, priority);
#else
				PRI_TO_QNUM(priority, txcfg->q_num, priv->pmib->dot11OperationEntry.wifi_specific);
#endif
			}

#ifdef CONFIG_RTL_WAPI_SUPPORT
			if (protocol==ETH_P_WAPI)
			{
				txcfg->q_num = MGNT_QUEUE;
			}
#endif

			if (txcfg->aggre_en == 0
#ifdef SUPPORT_TX_MCAST2UNI
					&& (priv->pshare->rf_ft_var.mc2u_disable || (pskb->cb[2] != (char)0xff))
#endif
				) {
				if ((pstat->aggre_mthd == AGGRE_MTHD_MPDU) &&
				/*	is_MCS_rate(txcfg->tx_rate) &&*/ (protocol != 0x888E)
#ifdef CONFIG_RTL_WAPI_SUPPORT
					&& (protocol != ETH_P_WAPI)
#endif
					&& !is_dhcp)
					txcfg->aggre_en = FG_AGGRE_MPDU;
			}

			if(
#ifdef CONFIG_RTL_8812_SUPPORT
//			( (GET_CHIP_VER(priv)== VERSION_8812E) && (AMPDU_ENABLE && AMSDU_ENABLE && txcfg->aggre_en) )	|| 
			( txcfg->pstat && (txcfg->pstat->aggre_mthd == AGGRE_MTHD_MPDU_AMSDU) && txcfg->aggre_en ) ||
#endif
			 (txcfg->aggre_en >= FG_AGGRE_MPDU && txcfg->aggre_en <= FG_AGGRE_MPDU_BUFFER_LAST) 
			)
			{
				//panic_printk("%s %d pstat->ADDBA_ready[priority]=%d, priority=%d\n",__func__,__LINE__,pstat->ADDBA_ready[priority],priority);
				if (!pstat->ADDBA_ready[priority]) {
					if ((pstat->ADDBA_req_num[priority] < 5) && !pstat->ADDBA_sent[priority]) {
						pstat->ADDBA_req_num[priority]++;
						issue_ADDBAreq(priv, pstat, priority);
						pstat->ADDBA_sent[priority]++;
					}
				}
			}
		}
#ifdef CONFIG_RTK_MESH
		else if(!txcfg->is_11s) // txcfg is not an 11s data frame, but it is multicast
#else
		else
#endif
		{
			// if group key not set yet, don't let unencrypted multicast go to air
			if (priv->pmib->dot11GroupKeysTable.dot11Privacy) {
				if (GET_GROUP_ENCRYP_KEYLEN == 0) {
					DEBUG_ERR("TX DROP: group key not set yet!\n");
					return FALSE;
				}
			}

			txcfg->privacy = get_mcast_privacy(priv, &txcfg->iv, &txcfg->icv, &txcfg->mic);

			if ((OPMODE & WIFI_AP_STATE) && priv->pkt_in_dtimQ) {
				txcfg->q_num = MCAST_QNUM;
				SetMData(txcfg->phdr);
			}
			else {
				txcfg->q_num = BE_QUEUE;
				pskb->cb[1] = 0;
			}

			if ((*da) == 0xff)	// broadcast
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
			else {				// multicast
				if (priv->pmib->dot11StationConfigEntry.lowestMlcstRate)
					txcfg->tx_rate = get_rate_from_bit_value(priv->pmib->dot11StationConfigEntry.lowestMlcstRate);
				else
					txcfg->tx_rate = find_rate(priv, NULL, 1, 1);
			}

#ifdef _11s_TEST_MODE_
			mesh_debug_tx4(priv, txcfg);
#endif

			txcfg->lowest_tx_rate = txcfg->tx_rate;
			txcfg->fixed_rate = 1;
		}
#ifdef CONFIG_RTK_MESH
		else // txcfg is 11s multicast
		{
			if ((*da) == 0xff)	// broadcast
				txcfg->tx_rate = find_rate(priv, NULL, 0, 1);
			else if (priv->pmib->dot11StationConfigEntry.lowestMlcstRate)
				txcfg->tx_rate = get_rate_from_bit_value(priv->pmib->dot11StationConfigEntry.lowestMlcstRate);
			else
				txcfg->tx_rate = find_rate(priv, NULL, 1, 1);
			txcfg->fixed_rate = 1;

#ifdef _11s_TEST_MODE_
			mesh_debug_tx5(priv, txcfg);
#endif

			txcfg->lowest_tx_rate = txcfg->tx_rate;
 			txcfg->privacy = _NO_PRIVACY_;

			if ((OPMODE & WIFI_AP_STATE) && priv->pkt_in_dtimQ) {
				txcfg->q_num = MCAST_QNUM;
				SetMData(txcfg->phdr);
			}
			else {
				txcfg->q_num = BE_QUEUE;
				pskb->cb[1] = 0;
				//txcfg->q_num = VI_QUEUE;
				//pskb->cb[1] = 5;
			}
		}
#endif // CONFIG_RTK_MESH
	}
#ifdef _11s_TEST_MODE_	/*---11s mgt frame---*/
	else if (txcfg->is_11s)
		mesh_debug_tx6(priv, txcfg);
#endif

	if (!da)
	{
		// This is non data frame, no need to frag.
#ifdef CONFIG_RTK_MESH
		if(txcfg->is_11s)
			da = GetAddr1Ptr(txcfg->phdr);
		else
#endif
			da = get_da((unsigned char *) (txcfg->phdr));

		txcfg->frg_num = 1;

		if (IS_MCAST(da))
			txcfg->need_ack = 0;
		else
			txcfg->need_ack = 1;

		if (GetPrivacy(txcfg->phdr))
		{
			// only auth with legacy wep...
			txcfg->iv = 4;
			txcfg->icv = 4;
			txcfg->privacy = priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		}

#ifdef DRVMAC_LB
		if (GetFrameType(txcfg->phdr) == WIFI_MGT_TYPE)
#endif
		if (txcfg->fr_len != 0)	//for mgt frame
			txcfg->hdr_len += WLAN_HDR_A3_LEN;
	}

#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE))
#else
	if (OPMODE & WIFI_AP_STATE)
#endif
	{
		if (IS_MCAST(da))
		{
			txcfg->frg_num = 1;
			txcfg->need_ack = 0;
			txcfg->rts_thrshld = 10000;
		}
		else if(!(txcfg->phdr && (GetFrameType(txcfg->phdr) == WIFI_MGT_TYPE) && (GetFrameSubType((unsigned char *) (txcfg->phdr))>>4 == 5)))	//  exclude probe rsp
		{
			pstat = get_stainfo(priv, da);
			txcfg->pstat = pstat;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		if ((txcfg->fr_type == _SKB_FRAME_TYPE_) ||							// skb frame
				!memcmp(GetAddr1Ptr(txcfg->phdr), BSSID, MACADDRLEN)) {		// mgt frame to AP
			pstat = get_stainfo(priv, BSSID);
			txcfg->pstat = pstat;
		}
	}
#endif

#ifdef BEAMFORMING_SUPPORT
	if((priv->pmib->dot11RFEntry.txbf == 1) && (pstat))
		Beamforming_GidPAid(priv, pstat);
#endif
	if (txcfg->privacy == _TKIP_PRIVACY_)
		txcfg->fr_len += 8;	// for Michael padding.

	txcfg->frag_thrshld -= (txcfg->mic + txcfg->iv + txcfg->icv + txcfg->hdr_len);

	if (txcfg->frg_num == 0)
	{
		if (txcfg->aggre_en > 0)
			txcfg->frg_num = 1;
		else {
			// how many mpdu we need...
			int llc;

			if ((ntohs(*((UINT16 *)((UINT8 *)pskb->data + ETH_ALEN*2))) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN)
				llc = sizeof(struct llc_snap);
			else
				llc = 0;

			txcfg->frg_num = (txcfg->fr_len + llc) / txcfg->frag_thrshld;
			if (((txcfg->fr_len + llc) % txcfg->frag_thrshld) != 0)
				txcfg->frg_num += 1;
		}
	}

	return TRUE;
}


/*-------------------------------------------------------------------------------
tx flow:
	Please refer to design spec for detail flow

rtl8192cd_firetx: check if hw desc is available for tx
				hdr_len, iv,icv, tx_rate info are available

signin_txdesc: fillin the desc and txpoll is necessary
--------------------------------------------------------------------------------*/
int __rtl8192cd_firetx(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
{
	int					*tx_head, *tx_tail, q_num;
	unsigned int		val32, is_dhcp=0;
	struct sk_buff		*pskb=NULL;
	struct llc_snap		*pllc_snap;
	struct wlan_ethhdr_t	*pethhdr=NULL;
#ifdef SUPPORT_TX_AMSDU
	struct wlan_ethhdr_t	*pmsdu_hdr;
	struct wlan_ethhdr_t  pethhdr_data;
#endif
	struct rtl8192cd_hw	*phw = GET_HW(priv);
#if defined(CONFIG_RTK_MESH) || defined(CONFIG_RTL_WAPI_SUPPORT)
	struct wlan_ethhdr_t	ethhdr;
	pethhdr = &ethhdr;
#endif

#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
	unsigned int		txdesc_need = 1, ret_txdesc = 0;
#endif
	unsigned long		x;

	/*---frag_thrshld setting---plus tune---0115*/
#ifdef	WDS
	if (txcfg->wdsIdx >= 0){
		txcfg->frag_thrshld = 2346; // if wds, disable fragment
	}else
#endif
#ifdef CONFIG_RTK_MESH
	if(txcfg->is_11s){
		txcfg->frag_thrshld = 2346; // if Mesh case, disable fragment
	}else
#endif
	{
		txcfg->frag_thrshld = FRAGTHRSLD - _CRCLNG_;
	}
	/*---frag_thrshld setting---end*/

	txcfg->rts_thrshld  = RTSTHRSLD;
	txcfg->frg_num = 0;

#ifdef DFS
	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		GET_ROOT(priv)->pmib->dot11DFSEntry.disable_tx) {
		priv->ext_stats.tx_drops++;
		DEBUG_ERR("TX DROP: DFS probation period\n");

		if (txcfg->fr_type == _SKB_FRAME_TYPE_) {
			rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
		} else if (txcfg->fr_type == _PRE_ALLOCMEM_) {
			release_mgtbuf_to_poll(priv, txcfg->pframe);
			release_wlanhdr_to_poll(priv, txcfg->phdr);
		}

		return SUCCESS;
	}
#endif

	if ((check_txdesc(priv, txcfg)) == FALSE) // this will only happen in errorous forwarding
	{
		priv->ext_stats.tx_drops++;
		// Don't need to print "TX DROP", already print in check_txdesc()
		if (txcfg->fr_type == _SKB_FRAME_TYPE_) {
			rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
			release_wlanllchdr_to_poll(priv, txcfg->phdr);
		}
		else if (txcfg->fr_type == _PRE_ALLOCMEM_) {
			release_mgtbuf_to_poll(priv, txcfg->pframe);
			release_wlanhdr_to_poll(priv, txcfg->phdr);
		}
		return SUCCESS;
	}

	if (txcfg->aggre_en > 0)
		txcfg->frag_thrshld = 2346;

	if (txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) {
		// now we are going to calculate how many hw desc we should have before tx...
		// wlan_hdr(including iv and llc) will occupy one desc, payload will occupy one, and
		// icv/mic will occupy the third desc if swcrypto is utilized.
		q_num = txcfg->q_num;
		tx_head = get_txhead_addr(phw, q_num);
		tx_tail = get_txtail_addr(phw, q_num);

		if (txcfg->privacy)
		{
			if (
#if defined(CONFIG_RTL_WAPI_SUPPORT)
					(_WAPI_SMS4_ != txcfg->privacy) &&
#endif
					UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
				val32 = txcfg->frg_num * 3;  //extra one is for ICV padding.
			else
				val32 = txcfg->frg_num * 2;
		}
		else
			val32 = txcfg->frg_num * 2;

#if defined(RESERVE_TXDESC_FOR_EACH_IF)
#if (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
		if(GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
			if (check_txdesc_dynamic_mechanism(priv, q_num, val32)) {
#ifdef USE_TXQUEUE
				if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable) {
					if (priv->pshare->txq_isr) {
						append_skb_to_txq_head(&priv->pshare->txq_list[q_num], priv, txcfg->pframe, priv->dev, &priv->pshare->txq_pool);
						priv->pshare->txq_stop = 1;
					} else {
						append_skb_to_txq_tail(&priv->pshare->txq_list[q_num], priv, txcfg->pframe, priv->dev, &priv->pshare->txq_pool);
					}
					if (txcfg->phdr)
						release_wlanllchdr_to_poll(priv, txcfg->phdr);
					priv->use_txq_cnt[q_num]++;
					priv->pshare->txq_check = 1;
					return SUCCESS;					
				}
				else
#endif
				{
					DEBUG_WARN("%d hw Queue desc exceed available count: used:%d\n", q_num,priv->use_txdesc_cnt[q_num]);
					return CONGESTED;
				}
			}
			txdesc_need = val32;
		} else
#endif
#endif
		{
		if ((val32 + 2) > CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC)) //per mpdu, we need 2 desc...
		{
			 // 2 is for spare...
#ifdef USE_TXQUEUE
			if (GET_ROOT(priv)->pmib->miscEntry.use_txq && priv->pshare->iot_mode_enable) {
				if (priv->pshare->txq_isr) {
					append_skb_to_txq_head(&priv->pshare->txq_list[q_num], priv, txcfg->pframe, priv->dev, &priv->pshare->txq_pool);
					priv->pshare->txq_stop = 1;
				} else {
					append_skb_to_txq_tail(&priv->pshare->txq_list[q_num], priv, txcfg->pframe, priv->dev, &priv->pshare->txq_pool);
				}
				if (txcfg->phdr)
					release_wlanllchdr_to_poll(priv, txcfg->phdr);
				priv->pshare->txq_check = 1;
				return SUCCESS;
			}
			else
#endif
			{
				DEBUG_ERR("%d hw Queue desc not available! head=%d, tail=%d request %d\n",q_num,*tx_head,*tx_tail,val32);
				return CONGESTED;
			}
		}
	}
	} else {
		  q_num = txcfg->q_num;
	}

	// then we have to check if wlan-hdr is available for usage...
	// actually, the checking can be void

	/* ----------
				Actually, I believe the check below is redundant.
				Since at this moment, desc is available, hdr/icv/
				should be enough.
													--------------*/
	val32 = txcfg->frg_num;

	if (val32 >= priv->pshare->pwlan_hdr_poll->count)
	{
		DEBUG_ERR("%d hw Queue tx without enough wlan_hdr\n", q_num);
		return CONGESTED;
	}

	// then we have to check if wlan_snapllc_hdrQ is enough for use
	// for each msdu, we need wlan_snapllc_hdrQ for maximum

	if (txcfg->fr_type == _SKB_FRAME_TYPE_)
	{
		pskb = (struct sk_buff *)txcfg->pframe;

		if ((OPMODE & WIFI_AP_STATE) && pskb && (isDHCPpkt(pskb) == TRUE))
			is_dhcp++;

#ifdef SUPPORT_TX_AMSDU
		// for AMSDU
		if (txcfg->aggre_en >= FG_AGGRE_MSDU_FIRST) {
			unsigned short usLen;
			int msdu_len;

			memcpy(&pethhdr_data, pskb->data, sizeof(struct wlan_ethhdr_t));
			pethhdr = &pethhdr_data;
			msdu_len = pskb->len - WLAN_ETHHDR_LEN;
			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				if (skb_headroom(pskb) < sizeof(struct llc_snap)) {
					struct sk_buff *skb2 = dev_alloc_skb(pskb->len);
					if (skb2 == NULL) {
						printk("%s: %s, dev_alloc_skb() failed!\n", priv->dev->name, __FUNCTION__);
						dev_kfree_skb_any(pskb);
						if(txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
							release_wlanllchdr_to_poll(priv, txcfg->phdr);
						return 0;
					}
					memcpy(skb_put(skb2, pskb->len), pskb->data, pskb->len);
					dev_kfree_skb_any(pskb);
					pskb = skb2;
					txcfg->pframe = (void *)pskb;
				}
				skb_push(pskb, sizeof(struct llc_snap));
			}
			pmsdu_hdr = (struct wlan_ethhdr_t *)pskb->data;

			memcpy(pmsdu_hdr, pethhdr, 12);
			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				usLen = (unsigned short)(msdu_len + sizeof(struct llc_snap));
				pmsdu_hdr->type = ntohs(usLen);
				eth_2_llc(pethhdr, (struct llc_snap *)(((unsigned long)pmsdu_hdr)+sizeof(struct wlan_ethhdr_t)));
			}
			else {
				usLen = (unsigned short)msdu_len;
				pmsdu_hdr->type = ntohs(usLen);
			}

			if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST)
				eth2_2_wlanhdr(priv, pethhdr, txcfg);

			txcfg->llc = 0;
			pllc_snap = NULL;

			if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST || txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) {
				if ((usLen+14) % 4)
					skb_put(pskb, 4-((usLen+14)%4));
			}
			txcfg->fr_len = pskb->len;
		}
		else 
#endif /* SUPPORT_TX_AMSDU */			
		{	// not A-MSDU
			// now, we should adjust the skb ...
			skb_pull(pskb, WLAN_ETHHDR_LEN);
#ifdef CONFIG_RTK_MESH
			memcpy(pethhdr, pskb->data - sizeof(struct wlan_ethhdr_t), sizeof(struct wlan_ethhdr_t));
#else
			pethhdr = (struct wlan_ethhdr_t *)(pskb->data - sizeof(struct wlan_ethhdr_t));
#endif
			pllc_snap = (struct llc_snap *)((UINT8 *)(txcfg->phdr) + txcfg->hdr_len + txcfg->iv);

			if ((ntohs(pethhdr->type) + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) {
				eth_2_llc(pethhdr, pllc_snap);
				txcfg->llc = sizeof(struct llc_snap);
			}
			else
			{
				pllc_snap = NULL;
			}

			eth2_2_wlanhdr(priv, pethhdr, txcfg);

#ifdef CONFIG_RTK_MESH
			if(txcfg->is_11s&1 && GetFrameSubType(txcfg->phdr) == WIFI_11S_MESH)
			{
				const short meshhdrlen= (txcfg->mesh_header.mesh_flag & 0x01) ? 16 : 4;
				if (skb_cloned(pskb))
				{
					struct sk_buff	*newskb = skb_copy(pskb, GFP_ATOMIC);
					rtl_kfree_skb(priv, pskb, _SKB_TX_);
					if (newskb == NULL) {
						priv->ext_stats.tx_drops++;
						release_wlanllchdr_to_poll(priv, txcfg->phdr);
						DEBUG_ERR("TX DROP: Can't copy the skb!\n");
						return SUCCESS;
					}
					txcfg->pframe = pskb = newskb;
#ifdef ENABLE_RTL_SKB_STATS
					rtl_atomic_inc(&priv->rtl_tx_skb_cnt);
#endif
				}
				memcpy(skb_push(pskb,meshhdrlen), &(txcfg->mesh_header), meshhdrlen);
				txcfg->fr_len += meshhdrlen;
			}
#endif // CONFIG_RTK_MESH
		}

		// TODO: modify as when skb is not bigger enough, take ICV from local pool
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if (txcfg->privacy == _WAPI_SMS4_)
		{
			if ((pskb->tail + SMS4_MIC_LEN) > pskb->end)
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: an over size skb!\n");
				rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
				wapiAssert(0);
				return SUCCESS;
			}
			memcpy(&ethhdr, pethhdr, sizeof(struct wlan_ethhdr_t));
#ifdef MCAST2UI_REFINE
                        memcpy(&ethhdr.daddr, &pskb->cb[10], 6);
#endif
			pethhdr = &ethhdr;
		} else
#endif

		if (txcfg->privacy == _TKIP_PRIVACY_)
		{
			if ((pskb->tail + 8) > pskb->end)
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: an over size skb!\n");
				rtl_kfree_skb(priv, (struct sk_buff *)txcfg->pframe, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
				return SUCCESS;
			}

#ifdef MCAST2UI_REFINE
                        memcpy(pethhdr->daddr, &pskb->cb[10], 6);
#endif

#ifdef WIFI_WMM
			if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, ((QOS_ENABLE) && (txcfg->pstat) && (txcfg->pstat->QosEnabled))?pskb->cb[1]:0, (UINT8 *)pllc_snap,
					pskb, txcfg)) == FALSE)
#else
			if ((tkip_mic_padding(priv, pethhdr->daddr, pethhdr->saddr, 0, (UINT8 *)pllc_snap,
					pskb, txcfg)) == FALSE)
#endif
			{
				priv->ext_stats.tx_drops++;
				DEBUG_ERR("TX DROP: Tkip mic padding fail!\n");
				rtl_kfree_skb(priv, pskb, _SKB_TX_);
				release_wlanllchdr_to_poll(priv, txcfg->phdr);
				return SUCCESS;
			}
			if ((txcfg->aggre_en < FG_AGGRE_MSDU_FIRST) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
				skb_put((struct sk_buff *)txcfg->pframe, 8);
		}
	}

	if (txcfg->privacy && txcfg->aggre_en <= FG_AGGRE_MSDU_FIRST)
		SetPrivacy(txcfg->phdr);

	// log tx statistics...
#ifdef CONFIG_RTL8672
	tx_sum_up(priv, txcfg->pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv, txcfg);
#else
	tx_sum_up(priv, txcfg->pstat, txcfg->fr_len+txcfg->hdr_len+txcfg->iv+txcfg->llc+txcfg->mic+txcfg->icv);
#endif
	SNMP_MIB_INC(dot11TransmittedFragmentCount, 1);

	// for SW LED
	if (txcfg->aggre_en > FG_AGGRE_MSDU_FIRST || GetFrameType(txcfg->phdr) == WIFI_DATA_TYPE) {
		priv->pshare->LED_tx_cnt++;
	} else {
		if (priv->pshare->LED_cnt_mgn_pkt)
			priv->pshare->LED_tx_cnt++;
	}

#ifdef PCIE_POWER_SAVING
	PCIeWakeUp(priv, (POWER_DOWN_T0));
#endif

	SAVE_INT_AND_CLI(x);
	
#ifdef SUPPORT_TX_AMSDU
	if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE || txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
		if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
			priv->use_txdesc_cnt[q_num] += 1;
			if (rtl8192cd_signin_txdesc_amsdu(priv, txcfg))
				priv->use_txdesc_cnt[q_num] -= 1;
		} else
#endif
		{
		rtl8192cd_signin_txdesc_amsdu(priv, txcfg);
		}
	} else
#endif
	{
#ifdef _11s_TEST_MODE_
		signin_txdesc_galileo(priv, txcfg);
#else
#if defined(RESERVE_TXDESC_FOR_EACH_IF)
#if (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
		if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc) {
			priv->use_txdesc_cnt[q_num] += txdesc_need;
			if ((ret_txdesc = rtl8192cd_signin_txdesc(priv, txcfg)) != 0 ) {
				if (
#if defined(CONFIG_RTL_WAPI_SUPPORT)
						(_WAPI_SMS4_ != txcfg->privacy) &&
#endif
						UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))
					priv->use_txdesc_cnt[q_num] -= 3 * ret_txdesc;
				else
					priv->use_txdesc_cnt[q_num] -= 2 * ret_txdesc;
			}
		} else
#endif
#endif
		{
#ifdef CONFIG_RTK_MESH
		rtl8192cd_signin_txdesc(priv, txcfg, pethhdr);
#else
		rtl8192cd_signin_txdesc(priv, txcfg);
#endif
		}
#endif
	}
	RESTORE_INT(x);

#ifdef TX_SHORTCUT
	if ((!priv->pmib->dot11OperationEntry.disable_txsc) &&
		(txcfg->fr_type == _SKB_FRAME_TYPE_) &&
		(txcfg->pstat) &&
		(txcfg->frg_num == 1) &&
		((txcfg->privacy == 0)
#ifdef CONFIG_RTL_WAPI_SUPPORT
		|| (txcfg->privacy == _WAPI_SMS4_)
#endif
		|| (txcfg->privacy && !UseSwCrypto(priv, txcfg->pstat, (txcfg->pstat ? FALSE : TRUE)))) &&
		/*(!IEEE8021X_FUN ||
			(IEEE8021X_FUN && (txcfg->pstat->ieee8021x_ctrlport == 1) &&
			(cpu_to_be16(pethhdr->type)!=0x888e))) && */
		(cpu_to_be16(pethhdr->type) != 0x888e) &&
		!is_dhcp && 
#ifdef CONFIG_RTL_WAPI_SUPPORT
		 (cpu_to_be16(pethhdr->type)  != ETH_P_WAPI)&&
#endif
		!GetMData(txcfg->phdr) &&
#ifdef A4_STA		
		((txcfg->pstat && txcfg->pstat->state & WIFI_A4_STA) 
			||!IS_MCAST((unsigned char *)pethhdr)) &&
#else
#ifdef MCAST2UI_REFINE
                pskb && !IS_MCAST(&pskb->cb[10]) &&
#else

		!IS_MCAST((unsigned char *)pethhdr) &&
#endif
#endif
		(txcfg->aggre_en < FG_AGGRE_MSDU_FIRST)
		)
	{
		int i = get_tx_sc_index(priv, txcfg->pstat, (unsigned char *)pethhdr);
		memcpy(&txcfg->pstat->tx_sc_ent[i].txcfg, txcfg, sizeof(struct tx_insn));
		memcpy((void *)&txcfg->pstat->tx_sc_ent[i].wlanhdr, txcfg->phdr, sizeof(struct wlanllc_hdr));
/*
#if defined(MESH_TX_SHORTCUT)
		memcpy((void *)&txcfg->pstat->meshhdr, (void *)&txcfg->mesh_header, ((txcfg->mesh_header.mesh_flag & 0x01) ? 16 : 4));
#endif
*/
	}
	else {
		if (txcfg->pstat && pethhdr) {
			int i = get_tx_sc_index(priv, txcfg->pstat, (unsigned char *)pethhdr);
			if (i >= 0)
				txcfg->pstat->tx_sc_ent[i].txcfg.fr_len = 0;
		}
	}
#endif

	return SUCCESS;
}


int rtl8192cd_firetx(struct rtl8192cd_priv *priv, struct tx_insn* txcfg)
{
#ifdef RX_TASKLET
	unsigned long x;
	int ret;

	SAVE_INT_AND_CLI(x);

	ret = __rtl8192cd_firetx(priv, txcfg);

	RESTORE_INT(x);
	return ret;
#else
	return (__rtl8192cd_firetx(priv, txcfg));
#endif
}


/*
	Procedure to re-cycle TXed packet in Queue index "txRingIdx"

	=> Return value means if system need restart-TX-queue or not.

		1: Need Re-start Queue
		0: Don't Need Re-start Queue
*/

static int rtl8192cd_tx_recycle(struct rtl8192cd_priv *priv, unsigned int txRingIdx, int *recycleCnt_p)
{
	struct tx_desc	*pdescH, *pdesc;
	struct tx_desc_info *pdescinfoH, *pdescinfo;
	volatile int	head, tail;
	struct rtl8192cd_hw	*phw=GET_HW(priv);
	int				needRestartQueue=0;
	int				recycleCnt=0;

	head		= get_txhead(phw, txRingIdx);
	tail		= get_txtail(phw, txRingIdx);
	pdescH		= get_txdesc(phw, txRingIdx);
	pdescinfoH	= get_txdesc_info(priv->pshare->pdesc_info, txRingIdx);

	while (CIRC_CNT_RTK(head, tail, CURRENT_NUM_TX_DESC))
	{
		pdesc = pdescH + (tail);
		pdescinfo = pdescinfoH + (tail);

#ifdef __MIPSEB__
		pdesc = (struct tx_desc *)KSEG1ADDR(pdesc);
#endif

		if (!pdesc || (get_desc(pdesc->Dword0) & TX_OWN))
			break;

#if defined(CONFIG_NET_PCI) && !defined(USE_RTL8186_SDK)
		if (IS_PCIBIOS_TYPE)
			//use the paddr and flen of pdesc field for icv, mic case which doesn't fill the pdescinfo
#ifdef CONFIG_RTL_8812_SUPPORT
			if(GET_CHIP_VER(priv)== VERSION_8812E)	
			pci_unmap_single(priv->pshare->pdev,
							 get_desc(pdesc->Dword10),
							 (get_desc(pdesc->Dword7)&0xffff),
							 PCI_DMA_TODEVICE);
			else
#endif
			pci_unmap_single(priv->pshare->pdev,
							 get_desc(pdesc->Dword8),
							 (get_desc(pdesc->Dword7)&0xffff),
							 PCI_DMA_TODEVICE);

#endif

		if (pdescinfo->type == _SKB_FRAME_TYPE_)
		{
#ifdef MP_TEST
			if (OPMODE & WIFI_MP_CTX_BACKGROUND) {
				struct sk_buff *skb = (struct sk_buff *)(pdescinfo->pframe);
				skb->data = skb->head;
				skb->tail = skb->data;
				skb->len = 0;
				priv->pshare->skb_tail = (priv->pshare->skb_tail + 1) & (NUM_MP_SKB - 1);
			}
			else
#endif
			{
#ifdef __LINUX_2_6__
				rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
#else
// for debug ------------
//				rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
				if (pdescinfo->pframe) {
				if (((struct sk_buff *)pdescinfo->pframe)->list) {
					DEBUG_ERR("Free tx skb error, skip it!\n");
					priv->ext_stats.freeskb_err++;
				}
				else
					rtl_kfree_skb(pdescinfo->priv, (struct sk_buff *)(pdescinfo->pframe), _SKB_TX_IRQ_);
				}
#endif
				needRestartQueue = 1;
			}
		}
		else if (pdescinfo->type == _PRE_ALLOCMEM_)
		{
			release_mgtbuf_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCHDR_)
		{
			release_wlanhdr_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCLLCHDR_)
		{
			release_wlanllchdr_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCICVHDR_)
		{
			release_icv_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _PRE_ALLOCMICHDR_)
		{
			release_mic_to_poll(priv, (UINT8 *)(pdescinfo->pframe));
		}
		else if (pdescinfo->type == _RESERVED_FRAME_TYPE_)
		{
			// the chained skb, no need to release memory
		}
		else
		{
			DEBUG_ERR("Unknown tx frame type %d\n", pdescinfo->type);
		}

		// for skb buffer free
		pdescinfo->pframe = NULL;

		recycleCnt ++;

		tail = (tail + 1) % CURRENT_NUM_TX_DESC;

#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
		if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc)
			pdescinfo->priv->use_txdesc_cnt[txRingIdx]--;
#endif
	}

	*get_txtail_addr(phw, txRingIdx) = tail;

	if (recycleCnt_p)
		*recycleCnt_p = recycleCnt;

	return needRestartQueue;
}


/*-----------------------------------------------------------------------------
Purpose of tx_dsr:

	For ALL TX queues
		1. Free Allocated Buf
		2. Update tx_tail
		3. Update tx related counters
		4. Restart tx queue if needed
------------------------------------------------------------------------------*/
void rtl8192cd_tx_dsr(unsigned long task_priv)
{
	struct rtl8192cd_priv	*priv = (struct rtl8192cd_priv *)task_priv;
	unsigned int	j=0;
	unsigned int	restart_queue=0;
	struct rtl8192cd_hw	*phw=GET_HW(priv);
	int needRestartQueue;
	unsigned long flags;

	if (!phw)
		return;

#ifdef PCIE_POWER_SAVING
	if ((priv->pwr_state == L2) || (priv->pwr_state == L1)) {
		priv->pshare->has_triggered_tx_tasklet = 0;
		return;
	}
#endif

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK_XMIT(flags);
	for(j=0; j<=HIGH_QUEUE; j++)
	{
		needRestartQueue = rtl8192cd_tx_recycle(priv, j, NULL);
		/* If anyone of queue report the TX queue need to be restart : we would set "restart_queue" to process ALL queues */
		if (needRestartQueue == 1)
			restart_queue = 1;
	}

	if (restart_queue)
		rtl8192cd_tx_restartQueue(priv);

#ifdef MP_TEST
#if 1//def CONFIG_RTL8672
	if ((OPMODE & (WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND|WIFI_MP_CTX_BACKGROUND_STOPPING))
			==(WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND))
#else //CONFIG_RTL8672
	if ((OPMODE & (WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND))==(WIFI_MP_STATE|WIFI_MP_CTX_BACKGROUND)) 
#endif //CONFIG_RTL8672
	{
		int *tx_head, *tx_tail;
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
		tx_head = get_txhead_addr(phw, BE_QUEUE);
		tx_tail = get_txtail_addr(phw, BE_QUEUE);
		if (CIRC_SPACE_RTK(*tx_head, *tx_tail, CURRENT_NUM_TX_DESC) > (CURRENT_NUM_TX_DESC/2))
			mp_ctx(priv, (unsigned char *)"tx-isr");
		SAVE_INT_AND_CLI(flags);
		SMP_LOCK(flags);
	}
#endif

	refill_skb_queue(priv);

#ifdef USE_TXQUEUE
	if (GET_ROOT(priv)->pmib->miscEntry.use_txq && !priv->pshare->txq_isr && priv->pshare->txq_check)
	{
		int q_num, send_cnt = 0;
		priv->pshare->txq_isr = 1;
		
		for (q_num=6; q_num>=0; q_num--)
		{
			priv->pshare->txq_stop = 0;
			while ( txq_len(&priv->pshare->txq_list[q_num]) > 0 )
			{
				struct sk_buff *tmp_skb = NULL;
				struct net_device *dev = NULL;
				remove_skb_from_txq(&priv->pshare->txq_list[q_num], &tmp_skb, &dev, &priv->pshare->txq_pool);
				if (tmp_skb && dev) {
#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
					if (GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc)
						((struct rtl8192cd_priv*)(dev->priv))->use_txq_cnt[q_num]--;
#endif
					__rtl8192cd_start_xmit(tmp_skb, dev, TX_NORMAL);
					send_cnt++;
					if (priv->pshare->txq_stop) break;
				}
			}
		}
		
		priv->pshare->txq_isr = 0;

		if (send_cnt == 0)
			priv->pshare->txq_check = 0;
	}
#endif

	priv->pshare->has_triggered_tx_tasklet = 0;

	RESTORE_INT(flags);
	SMP_UNLOCK_XMIT(flags);
}


/*
	Try to do TX-DSR for only ONE TX-queue ( rtl8192cd_tx_dsr would check for ALL TX queue )
*/
int rtl8192cd_tx_queueDsr(struct rtl8192cd_priv *priv, unsigned int txRingIdx)
{
	int recycleCnt;
	unsigned long flags;
	SAVE_INT_AND_CLI(flags);

	if (rtl8192cd_tx_recycle(priv, txRingIdx, &recycleCnt) == 1)
		rtl8192cd_tx_restartQueue(priv);

	RESTORE_INT(flags);
	return recycleCnt;
}


/*
	Procedure to restart TX Queue
*/
static void rtl8192cd_tx_restartQueue(struct rtl8192cd_priv *priv)
{
#ifdef __KERNEL__
	if (netif_queue_stopped(priv->dev)) {
		DEBUG_INFO("wake-up queue\n");
		netif_wake_queue(priv->dev);
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) && netif_queue_stopped(GET_VXD_PRIV(priv)->dev)) {
		DEBUG_INFO("wake-up VXD queue\n");
		netif_wake_queue(GET_VXD_PRIV(priv)->dev);
	}
#endif

#ifdef MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
		int bssidIdx;
		for (bssidIdx=0; bssidIdx<RTL8192CD_NUM_VWLAN; bssidIdx++) {
			if (IS_DRV_OPEN(priv->pvap_priv[bssidIdx]) && netif_queue_stopped(priv->pvap_priv[bssidIdx]->dev)) {
				DEBUG_INFO("wake-up Vap%d queue\n", bssidIdx);
				netif_wake_queue(priv->pvap_priv[bssidIdx]->dev);
			}
		}
	}
#endif

#ifdef CONFIG_RTK_MESH
	if (1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)
	{
		if (netif_running(priv->mesh_dev) &&
				netif_queue_stopped(priv->mesh_dev) )
		{
			netif_wake_queue(priv->mesh_dev);
		}
	}
#endif
#ifdef WDS
	if (priv->pmib->dot11WdsInfo.wdsEnabled) {
		int i;
		for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
			if (netif_running(priv->wds_dev[i]) &&
				netif_queue_stopped(priv->wds_dev[i])) {
				DEBUG_INFO("wake-up wds[%d] queue\n", i);
				netif_wake_queue(priv->wds_dev[i]);
			}
		}
	}
#endif
#endif
}


static int tkip_mic_padding(struct rtl8192cd_priv *priv,
				unsigned char *da, unsigned char *sa, unsigned char priority,
				unsigned char *llc, struct sk_buff *pskb, struct tx_insn* txcfg)
{
	// now check what's the mic key we should apply...

	unsigned char	*mickey = NULL;
	unsigned int	keylen = 0;
	struct stat_info	*pstat = NULL;
	unsigned char	*hdr, hdr_buf[16];
	unsigned int	num_blocks;
	unsigned char	tkipmic[8];
	unsigned char	*pbuf=pskb->data;
	unsigned int	len=pskb->len;
	unsigned int	llc_len = 0;

	// check if the mic/tkip key is valid at this moment.

	if ((pskb->tail + 8) > (pskb->end))
	{
		DEBUG_ERR("pskb have no extra room for TKIP Michael padding\n");
		return FALSE;
	}

#ifdef WDS
	if (txcfg->wdsIdx >= 0) {
		pstat = get_stainfo(priv, priv->pmib->dot11WdsInfo.entry[txcfg->wdsIdx].macAddr);
		if (pstat) {
			keylen = GET_UNICAST_MIC_KEYLEN;
			mickey = GET_UNICAST_TKIP_MIC1_KEY;
		}
	}
	else
#endif
	if (OPMODE & WIFI_AP_STATE)
	{

#ifdef CONFIG_RTK_MESH

		if(txcfg->is_11s)

			da = txcfg->nhop_11s;

#endif
		if (IS_MCAST(da))
		{
			keylen = GET_GROUP_MIC_KEYLEN;
			mickey = GET_GROUP_TKIP_MIC1_KEY;
#ifdef A4_STA			
			if (txcfg->pstat && (txcfg->pstat->state & WIFI_A4_STA)) {
				pstat = txcfg->pstat;
				keylen = GET_UNICAST_MIC_KEYLEN;
				mickey = GET_UNICAST_TKIP_MIC1_KEY;
			}
#endif			
		}
		else
		{
			pstat = get_stainfo (priv, da);
#ifdef A4_STA			
			if (priv->pshare->rf_ft_var.a4_enable && (pstat == NULL)) 
				pstat = a4_sta_lookup(priv, da);
#endif		
			if (pstat == NULL) {
				DEBUG_ERR("tx mic pstat == NULL\n");
				return FALSE;
			}

			keylen = GET_UNICAST_MIC_KEYLEN;
			mickey = GET_UNICAST_TKIP_MIC1_KEY;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo (priv, BSSID);
		if (pstat == NULL) {
			DEBUG_ERR("tx mic pstat == NULL\n");
			return FALSE;
		}

		keylen = GET_UNICAST_MIC_KEYLEN;
		mickey = GET_UNICAST_TKIP_MIC2_KEY;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		keylen = GET_GROUP_MIC_KEYLEN;
		mickey = GET_GROUP_TKIP_MIC1_KEY;
	}
#endif

	if ((txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) || (txcfg->aggre_en == FG_AGGRE_MSDU_LAST))
		mickey = pstat->tmp_mic_key;

	if (keylen == 0)
	{
		DEBUG_ERR("no mic padding for TKIP due to keylen=0\n");
		return FALSE;
	}

	if (txcfg->aggre_en <= FG_AGGRE_MSDU_FIRST) {
		hdr = hdr_buf;
		memcpy((void *)hdr, (void *)da, WLAN_ADDR_LEN);
		memcpy((void *)(hdr + WLAN_ADDR_LEN), (void *)sa, WLAN_ADDR_LEN);
		hdr[12] = priority;
		hdr[13] = hdr[14] = hdr[15] = 0;
	}
	else
		hdr = NULL;

	pbuf[len] = 0x5a;   /* Insert padding */
	pbuf[len+1] = 0x00;
	pbuf[len+2] = 0x00;
	pbuf[len+3] = 0x00;
	pbuf[len+4] = 0x00;
	pbuf[len+5] = 0x00;
	pbuf[len+6] = 0x00;
	pbuf[len+7] = 0x00;

	if (llc)
		llc_len = 8;
	num_blocks = (16 + llc_len + len + 5) / 4;
	if ((16 + llc_len + len + 5) & (4-1))
		num_blocks++;

	if (txcfg->aggre_en >= FG_AGGRE_MSDU_FIRST) {
		if (txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) {
			num_blocks = (16 + len) / 4;
			if ((16 + len) & (4-1))
				num_blocks++;
		}
		else if (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE) {
			num_blocks = len / 4;
			if (len & (4-1))
				num_blocks++;
		}
		else if (txcfg->aggre_en == FG_AGGRE_MSDU_LAST) {
			num_blocks = (len + 5) / 4;
			if ((len + 5) & (4-1))
				num_blocks++;
		}
	}

	michael(priv, mickey, hdr, llc, pbuf, (num_blocks << 2), tkipmic, 1);

	//tkip mic is MSDU-based, before filled-in descriptor, already finished.
	if (!(priv->pshare->have_hw_mic) ||
		(priv->pmib->dot11StationConfigEntry.swTkipMic))
	{
#ifdef MICERR_TEST
		if (priv->micerr_flag) {
			tkipmic[7] ^= tkipmic[7];
			priv->micerr_flag = 0;
		}
#endif
		if ((txcfg->aggre_en == FG_AGGRE_MSDU_FIRST) || (txcfg->aggre_en == FG_AGGRE_MSDU_MIDDLE)) {
			memcpy((void *)pstat->tmp_mic_key, (void *)tkipmic, 8);
		}
		else
			memcpy((void *)(pbuf + len), (void *)tkipmic, 8);
	}

	return TRUE;
}


static void wep_fill_iv(struct rtl8192cd_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid)
{
	unsigned char *iv = pwlhdr + hdrlen;
	union PN48 *ptsc48 = NULL;
	union PN48 auth_pn48;
	unsigned char *da;
	struct stat_info *pstat = NULL;

	memset(&auth_pn48, 0, sizeof(union PN48));
	da = get_da(pwlhdr);

#if defined(WDS) || defined(A4_STA)
	if (get_tofr_ds(pwlhdr) == 3)
		da = GetAddr1Ptr(pwlhdr);
#endif

	if (OPMODE & WIFI_AP_STATE)
	{
		if (IS_MCAST(da))
		{
			ptsc48 = GET_GROUP_ENCRYP_PN;
		}
		else
		{
			pstat = get_stainfo(priv, da);
			ptsc48 = GET_UNICAST_ENCRYP_PN;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		if (pstat != NULL)
			ptsc48 = GET_UNICAST_ENCRYP_PN;
		else
			ptsc48 = &auth_pn48;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
		ptsc48 = GET_GROUP_ENCRYP_PN;
#endif

	if (ptsc48 == NULL)
	{
		DEBUG_ERR("no TSC for WEP due to ptsc48=NULL\n");
		return;
	}

	iv[0] = ptsc48->_byte_.TSC0;
	iv[1] = ptsc48->_byte_.TSC1;
	iv[2] = ptsc48->_byte_.TSC2;
	iv[3] = 0x0 | (keyid << 6);

	if (ptsc48->val48 == 0xffffffffffffULL)
		ptsc48->val48 = 0;
	else
		ptsc48->val48++;
}


static void tkip_fill_encheader(struct rtl8192cd_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid_out)
{
	unsigned char *iv = pwlhdr + hdrlen;
	union PN48 *ptsc48 = NULL;
	unsigned int keyid = 0;
	unsigned char *da;
	struct stat_info *pstat = NULL;

	da = get_da(pwlhdr);

	if (OPMODE & WIFI_AP_STATE)
	{
#if defined(WDS) || defined(CONFIG_RTK_MESH) || defined(A4_STA)
		unsigned int to_fr_ds = (GetToDs(pwlhdr) << 1) | GetFrDs(pwlhdr);
		if (to_fr_ds == 3)
			da = GetAddr1Ptr(pwlhdr);
#endif

		if (IS_MCAST(da))
		{
			ptsc48 = GET_GROUP_ENCRYP_PN;
			keyid = priv->pmib->dot11GroupKeysTable.keyid;
		}
		else
		{
			pstat = get_stainfo(priv, da);
			ptsc48 = GET_UNICAST_ENCRYP_PN;
			keyid = 0;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		ptsc48 = GET_UNICAST_ENCRYP_PN;
		keyid = 0;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		ptsc48 = GET_GROUP_ENCRYP_PN;
		keyid = 0;
	}
#endif

#ifdef __DRAYTEK_OS__
	keyid = keyid_out;
#endif

	if (ptsc48 == NULL)
	{
		DEBUG_ERR("no TSC for TKIP due to ptsc48=NULL\n");
		return;
	}

	iv[0] = ptsc48->_byte_.TSC1;
	iv[1] = (iv[0] | 0x20) & 0x7f;
	iv[2] = ptsc48->_byte_.TSC0;
	iv[3] = 0x20 | (keyid << 6);
	iv[4] = ptsc48->_byte_.TSC2;
	iv[5] = ptsc48->_byte_.TSC3;
	iv[6] = ptsc48->_byte_.TSC4;
	iv[7] = ptsc48->_byte_.TSC5;

	if (ptsc48->val48 == 0xffffffffffffULL)
		ptsc48->val48 = 0;
	else
		ptsc48->val48++;
}


__MIPS16
__IRAM_IN_865X
static void aes_fill_encheader(struct rtl8192cd_priv *priv,
				unsigned char *pwlhdr, unsigned int hdrlen, unsigned long keyid)
{
	unsigned char *da;
	struct stat_info *pstat = NULL;
	union PN48 *pn48 = NULL;
	UINT8 pn_vector[6];

#ifdef __DRAYTEK_OS__
	int	keyid_input = keyid;
#endif

	da = get_da(pwlhdr);

	if (OPMODE & WIFI_AP_STATE)
	{
#if defined(WDS) || defined(CONFIG_RTK_MESH) || defined(A4_STA)
		unsigned int to_fr_ds = (GetToDs(pwlhdr) << 1) | GetFrDs(pwlhdr);
		if (to_fr_ds == 3)
			da = GetAddr1Ptr(pwlhdr);
#endif


		if (IS_MCAST(da))
		{
			pn48 = GET_GROUP_ENCRYP_PN;
			keyid = priv->pmib->dot11GroupKeysTable.keyid;
		}
		else
		{
			pstat = get_stainfo(priv, da);
			pn48 = GET_UNICAST_ENCRYP_PN;
			keyid = 0;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		pn48 = GET_UNICAST_ENCRYP_PN;
		keyid = 0;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		pn48 = GET_GROUP_ENCRYP_PN;
		keyid = 0;
	}
#endif

	if (pn48 == NULL)
	{
		DEBUG_ERR("no TSC for AES due to pn48=NULL\n");
		return;
	}

#ifdef __DRAYTEK_OS__
	keyid = keyid_input;
#endif

	pn_vector[0] = pwlhdr[hdrlen]   = pn48->_byte_.TSC0;
	pn_vector[1] = pwlhdr[hdrlen+1] = pn48->_byte_.TSC1;
	pwlhdr[hdrlen+2] =  0x00;
	pwlhdr[hdrlen+3] = (0x20 | (keyid << 6));
	pn_vector[2] = pwlhdr[hdrlen+4] = pn48->_byte_.TSC2;
	pn_vector[3] = pwlhdr[hdrlen+5] = pn48->_byte_.TSC3;
	pn_vector[4] = pwlhdr[hdrlen+6] = pn48->_byte_.TSC4;
	pn_vector[5] = pwlhdr[hdrlen+7] = pn48->_byte_.TSC5;

   	if (pn48->val48 == 0xffffffffffffULL)
		pn48->val48 = 0;
	else
		pn48->val48++;
}

