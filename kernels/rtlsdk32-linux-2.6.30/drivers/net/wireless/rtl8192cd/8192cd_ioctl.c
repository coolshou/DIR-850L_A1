/*
 *  io-control handling routines
 *
 *  $Id: 8192cd_ioctl.c,v 1.36.2.14 2011/01/06 07:50:09 button Exp $
 *
 *  Copyright (c) 2009 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_IOCTL_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/delay.h>
#endif

#include "./8192cd_cfg.h"

#ifdef __LINUX_2_6__
#include <linux/initrd.h>
#include <linux/syscalls.h>
#endif

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#include "./8192cd_headers.h"
#include "./8192cd_debug.h"

#ifdef CONFIG_RTL_COMAPI_WLTOOLS
#include <linux/if_arp.h>
#include <net/iw_handler.h>
#include "./8192cd_comapi.h"
#endif
#ifdef CONFIG_RTK_MESH
// for commu with  802.11s path selection deamon;plus note
#include "../mesh_ext/mesh_route.h"
#endif

#ifdef WIFI_HAPD
#include <linux/wireless.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_crypto.h>
#include <net80211/ieee80211_ioctl.h>
#include "./8192cd_net80211.h"

#define HAPD_IOCTL_SETCONFIG	SIOCIWLASTPRIV //0x8BFF
#endif

#ifdef WIFI_WPAS
#define WPAS_IOCTL_CUSTOM		SIOCIWLASTPRIV //0x8BFF
#endif


#define RTL8192CD_IOCTL_SET_MIB				(SIOCDEVPRIVATE + 0x1)	// 0x89f1
#define RTL8192CD_IOCTL_GET_MIB				(SIOCDEVPRIVATE + 0x2)	// 0x89f2
#define RTL8192CD_IOCTL_WRITE_REG				(SIOCDEVPRIVATE + 0x3)	// 0x89f3
#define RTL8192CD_IOCTL_READ_REG				(SIOCDEVPRIVATE + 0x4)	// 0x89f4
#define RTL8192CD_IOCTL_WRITE_MEM				(SIOCDEVPRIVATE + 0x5)	// 0x89f5
#define RTL8192CD_IOCTL_READ_MEM				(SIOCDEVPRIVATE + 0x6)	// 0x89f6
#define RTL8192CD_IOCTL_DEL_STA				(SIOCDEVPRIVATE + 0x7)	// 0x89f7
#define RTL8192CD_IOCTL_WRITE_EEPROM			(SIOCDEVPRIVATE + 0x8)	// 0x89f8
#define RTL8192CD_IOCTL_READ_EEPROM			(SIOCDEVPRIVATE + 0x9)	// 0x89f9
#define RTL8192CD_IOCTL_WRITE_BB_REG			(SIOCDEVPRIVATE + 0xa)	// 0x89fa
#define RTL8192CD_IOCTL_READ_BB_REG			(SIOCDEVPRIVATE + 0xb)	// 0x89fb
#define RTL8192CD_IOCTL_WRITE_RF_REG			(SIOCDEVPRIVATE + 0xc)	// 0x89fc
#define RTL8192CD_IOCTL_READ_RF_REG			(SIOCDEVPRIVATE + 0xd)	// 0x89fd
#define RTL8192CD_IOCTL_USER_DAEMON_REQUEST	(SIOCDEVPRIVATE + 0xf)	// 0x89ff

#ifdef	CONFIG_RTK_MESH
#define RTL8192CD_IOCTL_STATIC_ROUTE			(SIOCDEVPRIVATE + 0xe)
#endif

#define SIOCGIWRTLSTAINFO		0x8B30
#define SIOCGIWRTLSTANUM		0x8B31
#define SIOCGIWRTLDRVVERSION	0x8B32
#define SIOCGIWRTLSCANREQ		0x8B33
#define SIOCGIWRTLGETBSSDB		0x8B34
#define SIOCGIWRTLJOINREQ		0x8B35
#define SIOCGIWRTLJOINREQSTATUS	0x8B36
#define SIOCGIWRTLGETBSSINFO	0x8B37
#ifdef WDS
#define SIOCGIWRTLGETWDSINFO	0x8B38
#endif
#define SIOCSIWRTLSTATXRATE		0x8B39
#ifdef MICERR_TEST
#define SIOCSIWRTLMICERROR		0x8B3A
#define SIOCSIWRTLMICREPORT		0x8B3B
#endif
#ifdef SUPPORT_SNMP_MIB
#define SIOCGSNMPMIB			0x8B3D
#endif
#ifdef USE_PID_NOTIFY
#define SIOCSIWRTLSETPID		0x8B3E
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
#define SIOCSIWRTLSETWAPIPID		0x8B3F
#endif
#define SIOCSMIBDATA	0x8B41
#define SIOCMIBINIT		0x8B42
#define SIOCMIBSYNC		0x8B43
#define SIOCGMIBDATA	0x8B44
#define SIOCSACLADD		0x8B45
#define SIOCSACLDEL		0x8B46
#define SIOCSACLQUERY	0x8B47

#define SIOCGMISCDATA	0x8B48

#ifdef RTK_WOW
#define SIOCGRTKWOW		0x8B49
#define SIOCGRTKWOWSTAINFO		0x8B5A
#endif

#define SIOCSRFPWRADJ	0x8B5B
#ifdef AUTO_TEST_SUPPORT
#define SIOCSSREQ		0x8B5C
#define SIOCJOINREQ		0x8B5D
#endif

#ifdef MP_TEST
#define MP_START_TEST	0x8B61
#define MP_STOP_TEST	0x8B62
#define MP_SET_RATE		0x8B63
#define MP_SET_CHANNEL	0x8B64
#define MP_SET_TXPOWER	0x8B65
#define MP_CONTIOUS_TX	0x8B66
#define MP_ARX			0x8B67
#define MP_SET_BSSID	0x8B68
#define MP_ANTENNA_TX	0x8B69
#define MP_ANTENNA_RX	0x8B6A
#define MP_SET_BANDWIDTH 0x8B6B
#define MP_SET_PHYPARA	0x8B6C
#define MP_QUERY_STATS 	0x8B6D
#define MP_TXPWR_TRACK	0x8B6E
#define MP_QUERY_TSSI	0x8B6F
#define MP_QUERY_THER	0x8B77
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
#define MP_SET_BAND		0x8B85
#endif
#define MP_RESET_STATS	0x8B86
#define MP_GET_TXPOWER	0x8B87
#define MP_DIG			0x8B88

#ifdef B2B_TEST
// set/get convention: set(even number) get (odd number)
#define MP_TX_PACKET	0x8B71
#define MP_RX_PACKET	0x8B70
#define MP_BRX_PACKET	0x8B73
#endif

#endif // MP_TEST

#if defined(SW_ANT_SWITCH) || defined(HW_ANT_SWITCH)
#define SIOCANTSELECT	0x8b9d
#endif

#define SIOCGIWRTLREGDUMP		0x8B78

#if defined(MBSSID) || defined(UNIVERSAL_REPEATER)
#define SIOCSICOPYMIB			0x8B79
#endif

#ifdef SUPPORT_TX_MCAST2UNI
#define SIOCGIMCAST_ADD			0x8B80
#define SIOCGIMCAST_DEL			0x8B81
#endif

#ifdef CONFIG_RTL8186_KB
#define SIOCGIREADGUESTMAC		0x8B82
#define SIOCSIWRTGUESTMAC		0x8B83
#endif

#if defined(CONFIG_RTL8186_KB_N)
#define SIOCGIWRTLAUTH			0x8B84//To get wireless auth result
#endif

#ifdef CONFIG_RTL8672
// MBSSID Port Mapping
#define SIOSIWRTLITFGROUP		0x8B90

extern int bitmap_virt2phy(int mbr);
extern struct port_map wlanDev[];
int g_port_mapping=FALSE;
#endif


#define SIOCRADIOOFF		0x8B8E

#ifdef PCIE_POWER_SAVING
#define SIOCEPDN		0x8B8F
#endif

#ifdef EN_EFUSE
#define SIOCEFUSE_GET 		0x8b9b
#define SIOCEFUSE_SET 		0x8b9a
#define SIOCEFUSE_SYNC 		0x8b9c
#endif

#ifdef P2P_SUPPORT
#define SIOCP2PCMD			0x8BD1  // command for p2p 
#define SIOCP2PSCANREQ		0x8BD2	// issue p2p discovery request
#define SIOCP2PGETRESULT	0x8BD3	// get p2p discovery result
#define SIOCP2PPROVREQ		0x8BD4	// issue provision discovery request
#define SIOCP2WSCMETHODCONF	0x8BD5	// report event and state
#define SIOCP2PPGETEVNIND	0x8BD6	// get event and state

#define SIOCP2P_WSC_REPORT_STATE			0x8BD7
#define SIOCP2P_REPORT_CLIENT_STATE			0x8BD8	// report client connect state
#endif

#ifdef BR_SHORTCUT
#define SIOCLEARBRSC		0x8B91
#endif

#ifdef	CONFIG_RTK_MESH

#ifdef _11s_TEST_MODE_
#define SAVE_RECEIBVER_PID 			0x8B92 //PID ioctl
#define DEQUEUE_RECEIBVER_IOCTL 	0x8B93 //DEQUEUE ioctl
#endif
// ==== inserted by GANTOE for manual site survey 2008/12/25 ====
#define SIOCJOINMESH 				0x8B94
#define SIOCCHECKMESHLINK			0x8B95	// This OID might be removed when the mesh peerlink precedure has been completed
// GANTOE
#ifdef D_ACL//tsananiu
#define RTL8192CD_IOCTL_ADD_ACL_TABLE		0x8B96
#define RTL8192CD_IOCTL_REMOVE_ACL_TABLE	0x8B97
#define RTL8192CD_IOCTL_GET_ACL_TABLE		0x8B98
#define ACL_allow 1
#define ACL_deny 2
#endif//tsananiu//

#define SIOCQPATHTABLE  0x8BA0  // query pathselection table
#define SIOCUPATHTABLE  0x8BA1  // update  existing entry's date in pathselection table
#define SIOCAPATHTABLE  0x8BA2  // add a new entry into pathselection table

#ifdef FREDDY	//mesh related
#define RTL8190_IOCTL_DEL_STA_ENC 0x8BA5 //for kick STA function by freddy 2008/12/2
#endif

#define GET_STA_LIST 			0x8BA6
#define SET_PORTAL_POOL 		0x8BA8
#define SIOC_NOTIFY_PATH_CREATE 0x8BA9 // path selection daemon notify dirver that the path to des mac has created
#define SIOC_UPDATE_ROOT_INFO 	0x8BAA // update root mac into driver
#define SIOC_GET_ROUTING_INFO	0x8BAB // send routing info to user space
#define REMOVE_PATH_ENTRY		0x8BAC // remove specified path entry
#define SIOC_SET_ROUTING_INFO	0x8BAD // set MESH routing info from user space

#define SAVEPID_IOCTL			0x8BB0   //PID ioctl
#define DEQUEUEDATA_IOCTL		0x8BB1   //DEQUEUE ioctl

#ifdef _MESH_ACL_ENABLE_
#define SIOCSMESHACLADD		0x8BB5
#define SIOCSMESHACLDEL		0x8BB6
#define SIOCSMESHACLQUERY	0x8BB7
#endif

#endif // CONFIG_RTK_MESH

#define SIOC92DAUTOCH	0x8BC5 // manual auto channel


#ifdef CONFIG_RTL_COMAPI_CFGFILE
#define SIOCCOMAPIFILE		0x8BC0
#endif
#ifdef CONFIG_RTL_92D_SUPPORT
#define SIOC92DIQK		0x8BC1
#define SIOC92DTEMPMP	0x8BC2
#ifdef CONFIG_RTL_92D_DMDP
#define SIOC92DSMCC		0x8BC3 //smart concurrent
#endif
#ifdef EN_EFUSE
#define SIOC92DSBANDADDR 0x8BC4 // set hwaddr by band
#endif
#ifdef NON_INTR_ANTDIV
#define SIOC92DATNDIV	0x8BC6 // set hwaddr by band
#endif
#ifdef DPK_92D
#define SIOC92DDPK	0x8BC7 // dpk
#endif
#endif

#ifdef MP_PSD_SUPPORT
#define MP_QUERY_PSD  		0x8BC9
#endif

#ifdef FOR_VHT5G_PF
#define SIOC8812SIGMA 0x8BCB //8812_PF4
#endif

#define _OFFSET(field)	((int)(long *)&(((struct wifi_mib *)0)->field))
#define _SIZE(field)	sizeof(((struct wifi_mib *)0)->field)

#define _OFFSET_RFFT(field)	((int)(long *)&(((struct rf_finetune_var *)0)->field))
#define _SIZE_RFFT(field)	sizeof(((struct rf_finetune_var *)0)->field)


typedef enum {BYTE_T, INT_T, SSID_STRING_T, BYTE_ARRAY_T, ACL_T, IDX_BYTE_ARRAY_T, MULTI_BYTE_T,
#ifdef _DEBUG_RTL8192CD_
		DEBUG_T,
#endif
	DEF_SSID_STRING_T, STRING_T, RFFT_T, VARLEN_BYTE_T,
#ifdef WIFI_SIMPLE_CONFIG
	PIN_IND_T,
	/* WPS2DOTX   */
	WSC_SELF_PIN_IND_T,
	WSC_SEPC_SSID_CONN_IND_T,	
	WSC_SEPC_MAC_CONN_IND_T,	
	/* WPS2DOTX   */
#ifdef INCLUDE_WPS
#ifndef CONFIG_MSC
	WSC_IND_T,
#endif
	FLASH_RESTORE_T,
#endif

#ifdef CONFIG_RTL_COMAPI_CFGFILE
	WSC_START_IND_T,
	//EV_MODE, EV_STATUS, EV_MEHOD, EV_STEP, EV_OOB
	WSC_MODE_IND_T,
	WSC_STATUS_IND_T,
	WSC_METHOD_IND_T,
	WSC_STEP_IND_T,
	WSC_OOB_IND_T,
#endif  //ifdef CONFIG_RTL_COMAPI_CFGFILE
#endif
#ifdef 	CONFIG_RTK_MESH
	WORD_T,
#endif
	ACL_INT_T,	// mac address + 1 int
#ifdef CONFIG_RTL_WAPI_SUPPORT
	INT_ARRAY_T,
	WAPI_KEY_T,
#endif
#ifdef CONFIG_RTL_COMAPI_CFGFILE
	SSID2SCAN_STRING_T,
#endif
	RFFT_ACL_T,
#ifdef SUPPORT_MULTI_PROFILE
	AP_PROFILE_T,
#endif
} TYPE_T;


#ifdef CONFIG_RTL_COMAPI_WLTOOLS

struct iw_priv_args privtab[] = {
	{ RTL8192CD_IOCTL_SET_MIB, IW_PRIV_TYPE_CHAR | 450, 0, "set_mib" },
	{ RTL8192CD_IOCTL_GET_MIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_mib" },
#ifdef _IOCTL_DEBUG_CMD_
	{ RTL8192CD_IOCTL_WRITE_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_reg" },
	{ RTL8192CD_IOCTL_READ_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_reg" },
	{ RTL8192CD_IOCTL_WRITE_MEM, IW_PRIV_TYPE_CHAR | 128, 0, "write_mem" },
	{ RTL8192CD_IOCTL_READ_MEM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_mem" },
	{ RTL8192CD_IOCTL_WRITE_BB_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_bb" },
	{ RTL8192CD_IOCTL_READ_BB_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_bb" },
	{ RTL8192CD_IOCTL_WRITE_RF_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_rf" },
	{ RTL8192CD_IOCTL_READ_RF_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_rf" },
#endif
	{ RTL8192CD_IOCTL_DEL_STA, IW_PRIV_TYPE_CHAR | 128, 0, "del_sta" },
	{ RTL8192CD_IOCTL_WRITE_EEPROM, IW_PRIV_TYPE_CHAR | 128, 0, "write_eeprom" },
	{ RTL8192CD_IOCTL_READ_EEPROM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_eeprom" },

#ifdef SUPPORT_SNMP_MIB
	{ SIOCGSNMPMIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_snmp_mib" },
#endif

	{ SIOCSRFPWRADJ, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "rf_pwr" },
#ifdef AUTO_TEST_SUPPORT
	{ SIOCSSREQ, IW_PRIV_TYPE_NONE,0,"at_ss" },
	{ SIOCJOINREQ, IW_PRIV_TYPE_CHAR|40,0,"at_join" },
#endif

#ifdef CONFIG_RTL_COMAPI_CFGFILE
	{ SIOCCOMAPIFILE, IW_PRIV_TYPE_NONE, 0, "cfgfile" },
#endif

#ifdef MP_TEST
	{ MP_START_TEST, IW_PRIV_TYPE_NONE, 0, "mp_start" },
	{ MP_STOP_TEST, IW_PRIV_TYPE_NONE, 0, "mp_stop" },
	{ MP_SET_RATE, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rate" },
	{ MP_SET_CHANNEL, IW_PRIV_TYPE_CHAR | 40, 0, "mp_channel" },
	{ MP_SET_TXPOWER, IW_PRIV_TYPE_CHAR | 40, 0, "mp_txpower" },
	{ MP_CONTIOUS_TX, IW_PRIV_TYPE_CHAR | 128, 0, "mp_ctx" },
	{ MP_ARX, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_arx" },
	{ MP_SET_BSSID, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bssid" },
	{ MP_ANTENNA_TX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_tx" },
	{ MP_ANTENNA_RX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_rx" },
	{ MP_SET_BANDWIDTH, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bandwidth" },
	{ MP_SET_PHYPARA, IW_PRIV_TYPE_CHAR | 40, 0, "mp_phypara" },
#ifdef B2B_TEST
	{ MP_TX_PACKET, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "mp_tx" },
	{ MP_BRX_PACKET, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_brx" },
#if 0
	{ MP_RX_PACKET, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rx" },
#endif
#endif
	{ MP_QUERY_STATS, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_query" },
	{ MP_TXPWR_TRACK, IW_PRIV_TYPE_CHAR | 40, 0, "mp_pwrtrk" },
	{ MP_QUERY_TSSI, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_tssi" },
	
	{ MP_QUERY_THER, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_ther" },
#ifdef CONFIG_RTL_92D_SUPPORT
	{ MP_SET_BAND, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_phyband" },
#endif
	{ MP_RESET_STATS, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_reset_stats" },
	{ MP_GET_TXPOWER, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_get_pwr" },
	{ MP_DIG, IW_PRIV_TYPE_CHAR | 40, 0, "mp_dig" },
#endif // MP_TEST
#ifdef MICERR_TEST
	{ SIOCSIWRTLMICERROR, IW_PRIV_TYPE_CHAR | 40, 0, "mic_error" },
	{ SIOCSIWRTLMICREPORT, IW_PRIV_TYPE_CHAR | 40, 0, "mic_report" },
#endif
	{ SIOCGIWRTLREGDUMP, IW_PRIV_TYPE_CHAR | 40, 0, "reg_dump" },

#if defined(MBSSID) || defined(UNIVERSAL_REPEATER)
	{ SIOCSICOPYMIB, IW_PRIV_TYPE_CHAR | 40, 0, "copy_mib" },
#endif

#ifdef CONFIG_RTL8186_KB
	{ SIOCGIREADGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "read_guestmac" },
	{ SIOCSIWRTGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "write_guestmac" },
#endif

#ifdef	CONFIG_RTK_MESH
	{ RTL8192CD_IOCTL_STATIC_ROUTE, IW_PRIV_TYPE_CHAR | 40, 0, "strt" },
#ifdef D_ACL//tsananiu
    { RTL8192CD_IOCTL_ADD_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "add_acl_table" },
    { RTL8192CD_IOCTL_REMOVE_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "remove_acl_table" },
    { RTL8192CD_IOCTL_GET_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_acl_table" },
#endif//tsananiu//
#endif
#ifdef BR_SHORTCUT
	{ SIOCLEARBRSC, IW_PRIV_TYPE_CHAR | 40, 0, "clear_brsc" },
#endif


	{ SIOCRADIOOFF, IW_PRIV_TYPE_CHAR | 128, 0, "radio_off" },

#ifdef PCIE_POWER_SAVING
#ifdef PCIE_POWER_SAVING_DEBUG
	{ SIOCEPDN, IW_PRIV_TYPE_CHAR | 128, 128, "epdn" },
#else
	{ SIOCEPDN, IW_PRIV_TYPE_CHAR | 128, 128, "stopps" },
#endif
#endif

#ifdef EN_EFUSE
	{ SIOCEFUSE_GET, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 512, "efuse_get" },
	{ SIOCEFUSE_SET, IW_PRIV_TYPE_CHAR | 512, IW_PRIV_TYPE_CHAR | 128, "efuse_set" },
	{ SIOCEFUSE_SYNC, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR |  128, "efuse_sync" },
#endif
	#ifdef MP_PSD_SUPPORT
	{ MP_QUERY_PSD, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_psd" }, 
	#endif
	{ SIOC92DAUTOCH, IW_PRIV_TYPE_CHAR | 128, 0, "autoch" },


};

static const iw_handler rtl_iwhandler[] =
{
	(iw_handler) NULL,						/* SIOCSIWCOMMIT */
	(iw_handler) NULL /* supported */,		/* SIOCGIWNAME	 */
	(iw_handler) NULL,						/* SIOCSIWNWID	 */
	(iw_handler) NULL,						/* SIOCGIWNWID	 */
	(iw_handler) rtl_siwfreq,				/* SIOCSIWFREQ	 */
	(iw_handler) rtl_giwfreq,				/* SIOCGIWFREQ	 */
	(iw_handler) rtl_siwmode,				/* SIOCSIWMODE	 */
	(iw_handler) rtl_giwmode,				/* SIOCGIWMODE	 */
	(iw_handler) NULL,						/* SIOCSIWSENS	 */
	(iw_handler) NULL,						/* SIOCGIWSENS	 */
	(iw_handler) NULL /* not used */,			/* SIOCSIWRANGE  */
	(iw_handler) rtl_giwrange,				/* SIOCGIWRANGE  */
	(iw_handler) NULL /* not used */,			/* SIOCSIWPRIV	 */
	(iw_handler) NULL /* kernel code */,		/* SIOCGIWPRIV	 */
	(iw_handler) NULL /* not used */,			/* SIOCSIWSTATS  */
	(iw_handler) NULL /*rtl8192cd_get_wireless_stats*//* kernel code */,	 /* SIOCGIWSTATS  */
	(iw_handler) NULL,						/* SIOCSIWSPY	 */
	(iw_handler) NULL,						/* SIOCGIWSPY	 */
	(iw_handler) NULL,						/* SIOCSIWTHRSPY */
	(iw_handler) NULL,						/* SIOCGIWTHRSPY */
	(iw_handler) rtl_siwap,					/* SIOCSIWAP	 */
	(iw_handler) rtl_giwap,					/* SIOCGIWAP	 */
#ifdef SIOCSIWMLME
	(iw_handler) NULL, //  rt_ioctl_siwmlme,	        /* SIOCSIWMLME   */	//chris: deauth, disassoc for client mode
#else
	(iw_handler) NULL,				        /* SIOCSIWMLME */
#endif // SIOCSIWMLME //
	(iw_handler) rtl_iwaplist,				/* SIOCGIWAPLIST */
#ifdef SIOCGIWSCAN
	(iw_handler) rtl_siwscan,				/* SIOCSIWSCAN	 */
	(iw_handler) rtl_giwscan,				/* SIOCGIWSCAN	 */
#else
	(iw_handler) NULL,				        /* SIOCSIWSCAN   */
	(iw_handler) NULL,				        /* SIOCGIWSCAN   */
#endif /* SIOCGIWSCAN */
	(iw_handler) rtl_siwessid, 				/* SIOCSIWESSID  */
	(iw_handler) rtl_giwessid, 				/* SIOCGIWESSID  */
//Alpha
//polo add for encrypt 7-24
	(iw_handler) NULL,				/* SIOCGIWENCRYPT	 */
	(iw_handler) NULL,				/* SIOCGIAUTHTYPE	 */	
//polo
	(iw_handler) NULL, //  rt_ioctl_siwnickn, 		/* SIOCSIWNICKN  */
	(iw_handler) NULL, //  rt_ioctl_giwnickn, 		/* SIOCGIWNICKN  */
	(iw_handler) NULL,						/* -- hole --	 */
	(iw_handler) NULL,						/* -- hole --	 */
	(iw_handler) rtl_siwrate,			/* SIOCSIWRATE	 */
	(iw_handler) rtl_giwrate,				/* SIOCGIWRATE	 */
	(iw_handler) rtl_siwrts,				/* SIOCSIWRTS	 */
	(iw_handler) rtl_giwrts,				/* SIOCGIWRTS	 */
	(iw_handler) rtl_siwfrag,				/* SIOCSIWFRAG	 */
	(iw_handler) rtl_giwfrag,				/* SIOCGIWFRAG	 */
	(iw_handler) NULL,						/* SIOCSIWTXPOW  */
	(iw_handler) NULL,						/* SIOCGIWTXPOW  */
	(iw_handler) rtl_siwretry,				/* SIOCSIWRETRY  */
	(iw_handler) rtl_giwretry,				/* SIOCGIWRETRY  */
	(iw_handler) rtl_siwencode,				/* SIOCSIWENCODE */
	(iw_handler) rtl_giwencode,				/* SIOCGIWENCODE */
	(iw_handler) NULL,						/* SIOCSIWPOWER  */
	(iw_handler) rtl_giwpower,				/* SIOCGIWPOWER  */
	(iw_handler) NULL,						/* -- hole -- */
	(iw_handler) NULL,						/* -- hole -- */
#if WIRELESS_EXT > 17	// for wpa_supplicant
	(iw_handler) NULL, //rt_ioctl_siwgenie, 		/* SIOCSIWGENIE  */
	(iw_handler) NULL, //rt_ioctl_giwgenie, 		/* SIOCGIWGENIE  */
	(iw_handler) NULL, //rt_ioctl_siwauth,			/* SIOCSIWAUTH	 */
	(iw_handler) NULL, //rt_ioctl_giwauth,			/* SIOCGIWAUTH	 */
	(iw_handler) NULL, //rt_ioctl_siwencodeext, 	/* SIOCSIWENCODEEXT */
	(iw_handler) NULL, //rt_ioctl_giwencodeext, 	/* SIOCGIWENCODEEXT */
	(iw_handler) NULL, //rt_ioctl_siwpmksa, 		/* SIOCSIWPMKSA  */
#endif
};

static iw_handler rtl_private_handler[] =
{
#if 0
	NULL, //set_mib,
	NULL, //get_mib,
#ifdef _IOCTL_DEBUG_CMD_
	NULL, //read_reg,
	NULL, //read_mem,
	NULL, //read_bb,
	NULL, //read_rf,
#endif
	NULL, //del_sta,
	NULL, //write_eeprom,
	NULL, //read_eeprom,
#ifdef SUPPORT_SNMP_MIB
	NULL, //get_snmp_mib,
#endif
	NULL, //rf_pwr,
#ifdef AUTO_TEST_SUPPORT
	NULL, //at_ss,
	NULL, //at_join,
#endif
#ifdef CONFIG_RTL_COMAPI_CFGFILE
	NULL, //cfgfile,
#endif
#ifdef MP_TEST
	NULL, //mp_start,
	NULL, //mp_stop,
	NULL, //mp_rate,
	NULL, //mp_channel,
	NULL, //mp_txpower,
	NULL, //mp_ctx,
	NULL, //mp_arx,
	NULL, //mp_bssid,
	NULL, //mp_ant_tx,
	NULL, //mp_ant_rx,
	NULL, //mp_bandwidth,
	NULL, //mp_phypara,

#ifdef B2B_TEST
	NULL, //mp_tx,
	NULL, //mp_brx,
#if 0
	mp_rx,
#endif
#endif
	NULL, //mp_query,
	NULL, //mp_tssi,
#ifdef RTL8192SE
	NULL, //mp_ther,
#endif
#endif // MP_TEST
#if (defined(CONFIG_RTL865X) && defined(CONFIG_RTL865X_CLE) && defined(MP_TEST)) || defined(MP_TEST_CFG)
	NULL, //mp_cfg,
#endif
#ifdef MICERR_TEST
	NULL, //mic_error,
	NULL, //mic_report,
#endif
#ifdef DEBUG_8190
	NULL, //reg_dump,
#endif

#if defined(MBSSID) || defined(UNIVERSAL_REPEATER)
	NULL, //copy_mib,
#endif
#ifdef CONFIG_RTL8186_KB
	NULL, //read_guestmac,
	NULL, //write_guestmac,
#endif

#ifdef	CONFIG_RTK_MESH
	NULL, //strt,
#ifdef D_ACL //tsananiu
	NULL, //add_acl_table,
	NULL, //remove_acl_table,
	NULL, //get_acl_table,
#endif
#endif
#ifdef BR_SHORTCUT
	NULL, //clear_brsc,
#endif
#else
	NULL, // return NULL to redirect to dev->ioctl
#endif
};

const struct iw_handler_def rtl8192cd_iw_handler_def =
{
#define	N(a)	(sizeof (a) / sizeof (a[0]))
	.standard	= (iw_handler *) rtl_iwhandler,
	.num_standard	= sizeof(rtl_iwhandler) / sizeof(iw_handler),
	.private = rtl_private_handler,
	.private_args = (struct iw_priv_args *)privtab,
	.num_private = sizeof(rtl_private_handler) / sizeof(iw_handler),
 	.num_private_args = sizeof(privtab) / sizeof(struct iw_priv_args),
#if 0 // IW_HANDLER_VERSION >= 6
    .get_wireless_stats = rtl8192cd_get_wireless_stats,
#endif
};

#endif


struct iwpriv_arg {
	char name[32];	/* mib name */
	TYPE_T type;	/* Type and number of args */
	int offset;		/* mib offset */
	int len;		/* mib byte len */
	int Default;	/* mib default value */
};

/* Bit mask value for flags, compatiable with old driver */
#define STA_INFO_FLAG_AUTH_OPEN     	0x01
#define STA_INFO_FLAG_AUTH_WEP      	0x02
#define STA_INFO_FLAG_ASOC          	0x04
#define STA_INFO_FLAG_ASLEEP        	0x08

/* BSS info, reported to web server */
typedef struct _bss_info_2_web {
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;
    unsigned char ssid[33];
} bss_info_2_web;

typedef enum _wlan_mac_state {
    STATE_DISABLED=0, STATE_IDLE, STATE_SCANNING, STATE_STARTED, STATE_CONNECTED, STATE_WAITFORKEY
} wlan_mac_state;

#ifdef WDS
typedef enum _wlan_wds_state {
	STATE_WDS_EMPTY=0, STATE_WDS_DISABLED, STATE_WDS_ACTIVE
} wlan_wds_state;

typedef struct _wds_info {
	unsigned char	state;
	unsigned char	addr[6];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	tx_errors;
	unsigned char	TxOperaRate;
} web_wds_info;
#endif

struct _wlan_sta_rateset {
	unsigned char	mac[6];
	unsigned char	txrate;
};

struct _misc_data_ {
	unsigned char	mimo_tr_hw_support;
	unsigned char	mimo_tr_used;
	unsigned char	resv[30];
};


/* MIB table */
static struct iwpriv_arg mib_table[] = {
	// struct Dot11RFEntry
	{"channel",		BYTE_T,		_OFFSET(dot11RFEntry.dot11channel), _SIZE(dot11RFEntry.dot11channel), 0},
	{"ch_low",		INT_T,		_OFFSET(dot11RFEntry.dot11ch_low), _SIZE(dot11RFEntry.dot11ch_low), 0},
	{"ch_hi",		INT_T,		_OFFSET(dot11RFEntry.dot11ch_hi), _SIZE(dot11RFEntry.dot11ch_hi), 0},
	{"pwrlevelCCK_A",		BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelCCK_A), _SIZE(dot11RFEntry.pwrlevelCCK_A), 0},
	{"pwrlevelCCK_B",		BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelCCK_B), _SIZE(dot11RFEntry.pwrlevelCCK_B), 0},
	{"pwrlevelHT40_1S_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelHT40_1S_A), _SIZE(dot11RFEntry.pwrlevelHT40_1S_A), 0},
	{"pwrlevelHT40_1S_B",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevelHT40_1S_B), _SIZE(dot11RFEntry.pwrlevelHT40_1S_B), 0},
	{"pwrdiffHT40_2S",		BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiffHT40_2S), _SIZE(dot11RFEntry.pwrdiffHT40_2S), 0},
	{"pwrdiffHT20",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiffHT20), _SIZE(dot11RFEntry.pwrdiffHT20), 0},
	{"pwrdiffOFDM",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiffOFDM), _SIZE(dot11RFEntry.pwrdiffOFDM), 0},
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
	{"pwrlevel5GHT40_1S_A",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevel5GHT40_1S_A), _SIZE(dot11RFEntry.pwrlevel5GHT40_1S_A), 0},
	{"pwrlevel5GHT40_1S_B",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrlevel5GHT40_1S_B), _SIZE(dot11RFEntry.pwrlevel5GHT40_1S_B), 0},
#endif
#ifdef CONFIG_RTL_92D_SUPPORT
	{"pwrdiff5GHT40_2S",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff5GHT40_2S), _SIZE(dot11RFEntry.pwrdiff5GHT40_2S), 0},
	{"pwrdiff5GHT20",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff5GHT20), _SIZE(dot11RFEntry.pwrdiff5GHT20), 0},
	{"pwrdiff5GOFDM",			BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff5GOFDM), _SIZE(dot11RFEntry.pwrdiff5GOFDM), 0},
#endif
#ifdef POWER_PERCENT_ADJUSTMENT
	{"powerpercent",	BYTE_T,	_OFFSET(dot11RFEntry.power_percent), _SIZE(dot11RFEntry.power_percent), 100},
#endif
	{"preamble",	INT_T,		_OFFSET(dot11RFEntry.shortpreamble), _SIZE(dot11RFEntry.shortpreamble), 0},
	{"trswitch",	INT_T,		_OFFSET(dot11RFEntry.trswitch), _SIZE(dot11RFEntry.trswitch), 0},
	{"disable_ch14_ofdm",	INT_T,	_OFFSET(dot11RFEntry.disable_ch14_ofdm), _SIZE(dot11RFEntry.disable_ch14_ofdm), 0},
	{"disable_ch1213",	INT_T,	_OFFSET(dot11RFEntry.disable_ch1213), _SIZE(dot11RFEntry.disable_ch1213), 1},
	{"xcap",		INT_T,	_OFFSET(dot11RFEntry.xcap), _SIZE(dot11RFEntry.xcap), 0},
	{"tssi1",		INT_T,	_OFFSET(dot11RFEntry.tssi1), _SIZE(dot11RFEntry.tssi1), 0},
	{"tssi2",		INT_T,	_OFFSET(dot11RFEntry.tssi2), _SIZE(dot11RFEntry.tssi2), 0},
	{"ther",		INT_T,	_OFFSET(dot11RFEntry.ther), _SIZE(dot11RFEntry.ther), 0},
	{"deltaIQK",		INT_T,	_OFFSET(dot11RFEntry.deltaIQK), _SIZE(dot11RFEntry.deltaIQK), 0},
	{"deltaLCK",		INT_T,	_OFFSET(dot11RFEntry.deltaLCK), _SIZE(dot11RFEntry.deltaLCK), 0},
	{"MIMO_TR_mode",	INT_T,	_OFFSET(dot11RFEntry.MIMO_TR_mode), _SIZE(dot11RFEntry.MIMO_TR_mode), MIMO_2T2R},
	{"phyBandSelect",	BYTE_T,	_OFFSET(dot11RFEntry.phyBandSelect), _SIZE(dot11RFEntry.phyBandSelect), PHY_BAND_2G},
	{"macPhyMode",	BYTE_T,	_OFFSET(dot11RFEntry.macPhyMode), _SIZE(dot11RFEntry.macPhyMode), SINGLEMAC_SINGLEPHY},
	{"smcc",	INT_T,	_OFFSET(dot11RFEntry.smcc), _SIZE(dot11RFEntry.smcc), 1},
	{"smcc_t",	INT_T,	_OFFSET(dot11RFEntry.smcc_t), _SIZE(dot11RFEntry.smcc_t), 6},
#ifdef PHASE2_TEST
	{"smcc_p",	INT_T,	_OFFSET(dot11RFEntry.smcc_p), _SIZE(dot11RFEntry.smcc_p), 18},
#endif
	{"trsw_pape_C9",		BYTE_T,	_OFFSET(dot11RFEntry.trsw_pape_C9), _SIZE(dot11RFEntry.trsw_pape_C9), 0x0},
	{"trsw_pape_CC",		BYTE_T,	_OFFSET(dot11RFEntry.trsw_pape_CC), _SIZE(dot11RFEntry.trsw_pape_CC), 0xFF},
	{"tx2path",				INT_T,	_OFFSET(dot11RFEntry.tx2path), _SIZE(dot11RFEntry.tx2path), 1},
	{"txbf",				INT_T,	_OFFSET(dot11RFEntry.txbf), _SIZE(dot11RFEntry.txbf), 0},
	{"target_pwr",			INT_T,	_OFFSET(dot11RFEntry.target_pwr), _SIZE(dot11RFEntry.target_pwr), 0},
	{"pa_type",			INT_T,	_OFFSET(dot11RFEntry.pa_type), _SIZE(dot11RFEntry.pa_type), 0},	

	// struct Dot11StationConfigEntry
	{"ssid",		SSID_STRING_T,	_OFFSET(dot11StationConfigEntry.dot11DesiredSSID), _SIZE(dot11StationConfigEntry.dot11DesiredSSID), 0},
	{"defssid",		DEF_SSID_STRING_T,	_OFFSET(dot11StationConfigEntry.dot11DefaultSSID), _SIZE(dot11StationConfigEntry.dot11DefaultSSID), 0},
	{"bssid2join",	BYTE_ARRAY_T,	_OFFSET(dot11StationConfigEntry.dot11DesiredBssid), _SIZE(dot11StationConfigEntry.dot11DesiredBssid), 0},
	{"bcnint",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11BeaconPeriod), _SIZE(dot11StationConfigEntry.dot11BeaconPeriod), 100},
	{"dtimperiod",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11DTIMPeriod), _SIZE(dot11StationConfigEntry.dot11DTIMPeriod), 1},
	{"swcrypto",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11swcrypto), _SIZE(dot11StationConfigEntry.dot11swcrypto), 0},
	{"aclmode",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11AclMode), _SIZE(dot11StationConfigEntry.dot11AclMode), 0},
	{"aclnum",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11AclNum), _SIZE(dot11StationConfigEntry.dot11AclNum), 0},
	{"acladdr",		ACL_T,		_OFFSET(dot11StationConfigEntry.dot11AclAddr), _SIZE(dot11StationConfigEntry.dot11AclAddr), 0},
	{"oprates",		INT_T,		_OFFSET(dot11StationConfigEntry.dot11SupportedRates), _SIZE(dot11StationConfigEntry.dot11SupportedRates), 0xfff},
	{"basicrates",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11BasicRates), _SIZE(dot11StationConfigEntry.dot11BasicRates), 0xf},
	{"regdomain",	INT_T,		_OFFSET(dot11StationConfigEntry.dot11RegDomain), _SIZE(dot11StationConfigEntry.dot11RegDomain), 1},
	{"txpwr_lmt_index",	INT_T,		_OFFSET(dot11StationConfigEntry.txpwr_lmt_index), _SIZE(dot11StationConfigEntry.txpwr_lmt_index), 0},		
	{"autorate",	INT_T,		_OFFSET(dot11StationConfigEntry.autoRate), _SIZE(dot11StationConfigEntry.autoRate), 1},
	{"fixrate",		INT_T,		_OFFSET(dot11StationConfigEntry.fixedTxRate), _SIZE(dot11StationConfigEntry.fixedTxRate), 0},
	{"swTkipMic",	INT_T,		_OFFSET(dot11StationConfigEntry.swTkipMic), _SIZE(dot11StationConfigEntry.swTkipMic), 1},
	{"disable_protection", INT_T,	_OFFSET(dot11StationConfigEntry.protectionDisabled), _SIZE(dot11StationConfigEntry.protectionDisabled), 0},
	{"disable_olbc", INT_T,		_OFFSET(dot11StationConfigEntry.olbcDetectDisabled), _SIZE(dot11StationConfigEntry.olbcDetectDisabled), 0},
	{"disable_nmlsc", INT_T,		_OFFSET(dot11StationConfigEntry.nmlscDetectDisabled), _SIZE(dot11StationConfigEntry.nmlscDetectDisabled), 0},
	{"deny_legacy",	INT_T,		_OFFSET(dot11StationConfigEntry.legacySTADeny), _SIZE(dot11StationConfigEntry.legacySTADeny), 0},
#ifdef CLIENT_MODE
	{"fast_roaming", INT_T,		_OFFSET(dot11StationConfigEntry.fastRoaming), _SIZE(dot11StationConfigEntry.fastRoaming), 0},
#endif
	{"lowestMlcstRate", INT_T,	_OFFSET(dot11StationConfigEntry.lowestMlcstRate), _SIZE(dot11StationConfigEntry.lowestMlcstRate), 0},
	{"stanum",		INT_T,		_OFFSET(dot11StationConfigEntry.supportedStaNum), _SIZE(dot11StationConfigEntry.supportedStaNum), 0},

#ifdef	CONFIG_RTK_MESH
	{"mesh_enable",			BYTE_T,			_OFFSET(dot1180211sInfo.mesh_enable),			_SIZE(dot1180211sInfo.mesh_enable),			0},
	{"mesh_root_enable",	BYTE_T,			_OFFSET(dot1180211sInfo.mesh_root_enable),		_SIZE(dot1180211sInfo.mesh_root_enable),	0},
	{"mesh_ap_enable",		BYTE_T,			_OFFSET(dot1180211sInfo.mesh_ap_enable),		_SIZE(dot1180211sInfo.mesh_ap_enable),		0},
	{"mesh_portal_enable",	BYTE_T,			_OFFSET(dot1180211sInfo.mesh_portal_enable),	_SIZE(dot1180211sInfo.mesh_portal_enable),	0},
	{"mesh_id",				STRING_T,		_OFFSET(dot1180211sInfo.mesh_id),				_SIZE(dot1180211sInfo.mesh_id),				0},
	{"mesh_max_neightbor",	WORD_T,			_OFFSET(dot1180211sInfo.mesh_max_neightbor),	_SIZE(dot1180211sInfo.mesh_max_neightbor),	0},

	{"log_enabled",			BYTE_T,			_OFFSET(dot1180211sInfo.log_enabled),		_SIZE(dot1180211sInfo.log_enabled),		0},
	{"mesh_privacy",		INT_T,		_OFFSET(dot11sKeysTable.dot11Privacy),	_SIZE(dot11sKeysTable.dot11Privacy),		0},

#ifdef _MESH_ACL_ENABLE_
	{"meshaclmode", 		INT_T,			_OFFSET(dot1180211sInfo.mesh_acl_mode), 		_SIZE(dot1180211sInfo.mesh_acl_mode),		0},
	{"meshaclnum",			INT_T,			_OFFSET(dot1180211sInfo.mesh_acl_num), 			_SIZE(dot1180211sInfo.mesh_acl_num),		0},
	{"meshacladdr", 		ACL_T,			_OFFSET(dot1180211sInfo.mesh_acl_addr), 		_SIZE(dot1180211sInfo.mesh_acl_addr),		0},
#endif

#ifdef 	_11s_TEST_MODE_
	{"mesh_reserved1",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved1),	_SIZE(dot1180211sInfo.mesh_reserved1),		0},
	{"mesh_reserved2",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved2),	_SIZE(dot1180211sInfo.mesh_reserved2),		0},
	{"mesh_reserved3",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved3),	_SIZE(dot1180211sInfo.mesh_reserved3),		0},
	{"mesh_reserved4",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved4),	_SIZE(dot1180211sInfo.mesh_reserved4),		0},
	{"mesh_reserved5",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved5),	_SIZE(dot1180211sInfo.mesh_reserved5),		0},
	{"mesh_reserved6",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved6),	_SIZE(dot1180211sInfo.mesh_reserved6),		0},
	{"mesh_reserved7",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved7),	_SIZE(dot1180211sInfo.mesh_reserved7),		0},
	{"mesh_reserved8",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved8),	_SIZE(dot1180211sInfo.mesh_reserved8),		0},
	{"mesh_reserved9",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserved9),	_SIZE(dot1180211sInfo.mesh_reserved9),		0},
	{"mesh_reserveda",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reserveda),	_SIZE(dot1180211sInfo.mesh_reserveda),		0},
	{"mesh_reservedb",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedb),	_SIZE(dot1180211sInfo.mesh_reservedb),		0},
	{"mesh_reservedc",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedc),	_SIZE(dot1180211sInfo.mesh_reservedc),		0},
	{"mesh_reservedd",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedd),	_SIZE(dot1180211sInfo.mesh_reservedd),		0},
	{"mesh_reservede",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservede),	_SIZE(dot1180211sInfo.mesh_reservede),		0},
	{"mesh_reservedf",			WORD_T,			_OFFSET(dot1180211sInfo.mesh_reservedf),	_SIZE(dot1180211sInfo.mesh_reservedf),		0},
	{"mesh_reservedstr1",		STRING_T,		_OFFSET(dot1180211sInfo.mesh_reservedstr1),	_SIZE(dot1180211sInfo.mesh_reservedstr1),	0},
#endif

#endif

	// struct Dot1180211AuthEntry
	{"authtype",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11AuthAlgrthm), _SIZE(dot1180211AuthEntry.dot11AuthAlgrthm), 0},
	{"encmode",		BYTE_T,		_OFFSET(dot1180211AuthEntry.dot11PrivacyAlgrthm), _SIZE(dot1180211AuthEntry.dot11PrivacyAlgrthm), 0},
	{"wepdkeyid",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11PrivacyKeyIndex), _SIZE(dot1180211AuthEntry.dot11PrivacyKeyIndex), 0},
#if defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD)
	{"psk_enable",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11EnablePSK), _SIZE(dot1180211AuthEntry.dot11EnablePSK), 0},
	{"wpa_cipher",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11WPACipher), _SIZE(dot1180211AuthEntry.dot11WPACipher), 0},
#ifdef RTL_WPA2
	{"wpa2_cipher",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11WPA2Cipher), _SIZE(dot1180211AuthEntry.dot11WPA2Cipher), 0},
#endif
	{"passphrase",	STRING_T,	_OFFSET(dot1180211AuthEntry.dot11PassPhrase), _SIZE(dot1180211AuthEntry.dot11PassPhrase), 0},
#ifdef CONFIG_RTL8186_KB
	{"passphrase_guest", STRING_T, _OFFSET(dot1180211AuthEntry.dot11PassPhraseGuest), _SIZE(dot1180211AuthEntry.dot11PassPhraseGuest), 0},
#endif
	{"gk_rekey",	INT_T,		_OFFSET(dot1180211AuthEntry.dot11GKRekeyTime), _SIZE(dot1180211AuthEntry.dot11GKRekeyTime), 0},
#endif

	// struct Dot118021xAuthEntry
	{"802_1x",		INT_T,		_OFFSET(dot118021xAuthEntry.dot118021xAlgrthm), _SIZE(dot118021xAuthEntry.dot118021xAlgrthm), 0},
	{"default_port",INT_T,		_OFFSET(dot118021xAuthEntry.dot118021xDefaultPort), _SIZE(dot118021xAuthEntry.dot118021xDefaultPort), 0},
	{"acct_enabled",INT_T,		_OFFSET(dot118021xAuthEntry.acct_enabled), _SIZE(dot118021xAuthEntry.acct_enabled), 0},
	{"acct_timeout_INT",INT_T,_OFFSET(dot118021xAuthEntry.acct_timeout_period), _SIZE(dot118021xAuthEntry.acct_timeout_period), 0},
	{"acct_timeout_TP",INT_T,_OFFSET(dot118021xAuthEntry.acct_timeout_throughput), _SIZE(dot118021xAuthEntry.acct_timeout_throughput), 0},

	// struct Dot11DefaultKeysTable
	{"wepkey1",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[0]), _SIZE(dot11DefaultKeysTable.keytype[0]), 0},
	{"wepkey2",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[1]), _SIZE(dot11DefaultKeysTable.keytype[1]), 0},
	{"wepkey3",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[2]), _SIZE(dot11DefaultKeysTable.keytype[2]), 0},
	{"wepkey4",		BYTE_ARRAY_T,	_OFFSET(dot11DefaultKeysTable.keytype[3]), _SIZE(dot11DefaultKeysTable.keytype[3]), 0},

	// struct Dot11OperationEntry
	{"opmode",		INT_T,		_OFFSET(dot11OperationEntry.opmode), _SIZE(dot11OperationEntry.opmode), 0x10},
	{"hiddenAP",	INT_T,		_OFFSET(dot11OperationEntry.hiddenAP), _SIZE(dot11OperationEntry.hiddenAP), 0},
	{"rtsthres",	INT_T,		_OFFSET(dot11OperationEntry.dot11RTSThreshold), _SIZE(dot11OperationEntry.dot11RTSThreshold), 2347},
	{"fragthres",	INT_T,		_OFFSET(dot11OperationEntry.dot11FragmentationThreshold), _SIZE(dot11OperationEntry.dot11FragmentationThreshold), 2347},
	{"shortretry",	INT_T,		_OFFSET(dot11OperationEntry.dot11ShortRetryLimit), _SIZE(dot11OperationEntry.dot11ShortRetryLimit), 0},
	{"longretry",	INT_T,		_OFFSET(dot11OperationEntry.dot11LongRetryLimit), _SIZE(dot11OperationEntry.dot11LongRetryLimit), 0},
	{"expired_time",INT_T,		_OFFSET(dot11OperationEntry.expiretime), _SIZE(dot11OperationEntry.expiretime), 30000}, /*in 10ms*/
#if defined(CONFIG_RTL8672)
	{"led_type",	INT_T,		_OFFSET(dot11OperationEntry.ledtype), _SIZE(dot11OperationEntry.ledtype), LEDTYPE_SW_LED2_GPIO8_LINKTXRX},
#else
	{"led_type",	INT_T,		_OFFSET(dot11OperationEntry.ledtype), _SIZE(dot11OperationEntry.ledtype), 0},
#endif
#ifdef RTL8190_SWGPIO_LED
	{"led_route",	INT_T,		_OFFSET(dot11OperationEntry.ledroute), _SIZE(dot11OperationEntry.ledroute), 0},
#endif
	{"iapp_enable",	INT_T,		_OFFSET(dot11OperationEntry.iapp_enable), _SIZE(dot11OperationEntry.iapp_enable), 0},
	{"block_relay",	INT_T,		_OFFSET(dot11OperationEntry.block_relay), _SIZE(dot11OperationEntry.block_relay), 0},
	{"deny_any",	INT_T,		_OFFSET(dot11OperationEntry.deny_any), _SIZE(dot11OperationEntry.deny_any), 0},
	{"crc_log",		INT_T,		_OFFSET(dot11OperationEntry.crc_log), _SIZE(dot11OperationEntry.crc_log), 0},
	{"wifi_specific",INT_T,		_OFFSET(dot11OperationEntry.wifi_specific), _SIZE(dot11OperationEntry.wifi_specific), 2},
#ifdef TX_SHORTCUT
	{"disable_txsc",INT_T,		_OFFSET(dot11OperationEntry.disable_txsc), _SIZE(dot11OperationEntry.disable_txsc), 0},
#endif
#ifdef RX_SHORTCUT
	{"disable_rxsc",INT_T,		_OFFSET(dot11OperationEntry.disable_rxsc), _SIZE(dot11OperationEntry.disable_rxsc), 0},
#endif
#ifdef BR_SHORTCUT
	{"disable_brsc",INT_T,		_OFFSET(dot11OperationEntry.disable_brsc), _SIZE(dot11OperationEntry.disable_brsc), 0},
#endif
	{"keep_rsnie",	INT_T,		_OFFSET(dot11OperationEntry.keep_rsnie), _SIZE(dot11OperationEntry.keep_rsnie), 0},
	{"guest_access",INT_T,		_OFFSET(dot11OperationEntry.guest_access), _SIZE(dot11OperationEntry.guest_access), 0},

	// struct bss_type
	{"band",		BYTE_T,		_OFFSET(dot11BssType.net_work_type), _SIZE(dot11BssType.net_work_type), 3},

	// struct erp_mib
	{"cts2self",	INT_T,		_OFFSET(dot11ErpInfo.ctsToSelf), _SIZE(dot11ErpInfo.ctsToSelf), 1},

#ifdef WDS
	// struct wds_info
	{"wds_enable",	INT_T,		_OFFSET(dot11WdsInfo.wdsEnabled), _SIZE(dot11WdsInfo.wdsEnabled), 0},
	{"wds_pure",	INT_T,		_OFFSET(dot11WdsInfo.wdsPure), _SIZE(dot11WdsInfo.wdsPure), 0},
	{"wds_priority",INT_T,		_OFFSET(dot11WdsInfo.wdsPriority), _SIZE(dot11WdsInfo.wdsPriority), 0},
	{"wds_num",		INT_T,		_OFFSET(dot11WdsInfo.wdsNum), _SIZE(dot11WdsInfo.wdsNum), 0},
	{"wds_add",		ACL_INT_T,		_OFFSET(dot11WdsInfo.entry), _SIZE(dot11WdsInfo.entry), 0},
	{"wds_encrypt",	INT_T,		_OFFSET(dot11WdsInfo.wdsPrivacy), _SIZE(dot11WdsInfo.wdsPrivacy), 0},
	{"wds_wepkey",	BYTE_ARRAY_T, _OFFSET(dot11WdsInfo.wdsWepKey), _SIZE(dot11WdsInfo.wdsWepKey), 0},
	{"wds_keyid",	INT_T, _OFFSET(dot11WdsInfo.wdsKeyId), _SIZE(dot11WdsInfo.wdsKeyId), 0},
#if defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD)
	{"wds_passphrase",	STRING_T, _OFFSET(dot11WdsInfo.wdsPskPassPhrase), _SIZE(dot11WdsInfo.wdsPskPassPhrase), 0},
#endif
#endif

#ifdef RTK_BR_EXT
	// struct br_ext_info
	{"nat25_disable",		INT_T,	_OFFSET(ethBrExtInfo.nat25_disable), _SIZE(ethBrExtInfo.nat25_disable), 0},
	{"macclone_enable",		INT_T,	_OFFSET(ethBrExtInfo.macclone_enable), _SIZE(ethBrExtInfo.macclone_enable), 0},
	{"dhcp_bcst_disable",	INT_T,	_OFFSET(ethBrExtInfo.dhcp_bcst_disable), _SIZE(ethBrExtInfo.dhcp_bcst_disable), 0},
	{"add_pppoe_tag",		INT_T,	_OFFSET(ethBrExtInfo.addPPPoETag), _SIZE(ethBrExtInfo.addPPPoETag), 1},
	{"clone_mac_addr",		BYTE_ARRAY_T,	_OFFSET(ethBrExtInfo.nat25_dmzMac), _SIZE(ethBrExtInfo.nat25_dmzMac), 0},
	{"nat25sc_disable",		INT_T,	_OFFSET(ethBrExtInfo.nat25sc_disable), _SIZE(ethBrExtInfo.nat25sc_disable), 0},
#endif

#ifdef DFS
	//struct Dot11DFSEntry
	{"disable_DFS",	INT_T,		_OFFSET(dot11DFSEntry.disable_DFS), _SIZE(dot11DFSEntry.disable_DFS), 0},
	{"disable_tx",	INT_T,		_OFFSET(dot11DFSEntry.disable_tx), _SIZE(dot11DFSEntry.disable_tx), 0},
	{"DFS_timeout",	INT_T,		_OFFSET(dot11DFSEntry.DFS_timeout), _SIZE(dot11DFSEntry.DFS_timeout), 10},	/*in 10ms*/
	{"DFS_detected",INT_T,		_OFFSET(dot11DFSEntry.DFS_detected), _SIZE(dot11DFSEntry.DFS_detected), 0},
	{"NOP_timeout",	INT_T,		_OFFSET(dot11DFSEntry.NOP_timeout), _SIZE(dot11DFSEntry.NOP_timeout), 180500}, /*in 10ms*/
#endif

	//struct miscEntry
	{"show_hidden_bss",INT_T,	_OFFSET(miscEntry.show_hidden_bss), _SIZE(miscEntry.show_hidden_bss), 0},
	{"ack_timeout",	INT_T,		_OFFSET(miscEntry.ack_timeout), _SIZE(miscEntry.ack_timeout), 0},
	{"private_ie",	VARLEN_BYTE_T,	_OFFSET(miscEntry.private_ie), _SIZE(miscEntry.private_ie), 0},
	{"rxInt",		INT_T,		_OFFSET(miscEntry.rxInt_thrd), _SIZE(miscEntry.rxInt_thrd), 300},
#ifdef DRVMAC_LB
	{"dmlb",		INT_T,		_OFFSET(miscEntry.drvmac_lb), _SIZE(miscEntry.drvmac_lb), 1},
	{"lb_da",		BYTE_ARRAY_T,	_OFFSET(miscEntry.lb_da), _SIZE(miscEntry.lb_da), 0},
	{"lb_tps",		INT_T,	_OFFSET(miscEntry.lb_tps), _SIZE(miscEntry.lb_tps), 0},
	{"lb_mlmp",		INT_T,	_OFFSET(miscEntry.lb_mlmp), _SIZE(miscEntry.lb_mlmp), 0},
#endif
	{"groupID",		INT_T,		_OFFSET(miscEntry.groupID), _SIZE(miscEntry.groupID), 0},
#ifdef MBSSID
	{"vap_enable",	INT_T,		_OFFSET(miscEntry.vap_enable), _SIZE(miscEntry.vap_enable), 0},
#endif
#if defined(RESERVE_TXDESC_FOR_EACH_IF) && (defined(UNIVERSAL_REPEATER) || defined(MBSSID))
	{"rsv_txdesc",		INT_T,		_OFFSET(miscEntry.rsv_txdesc), _SIZE(miscEntry.rsv_txdesc), 1},
#endif
#ifdef USE_TXQUEUE
	{"use_txq",		INT_T,		_OFFSET(miscEntry.use_txq), _SIZE(miscEntry.use_txq), 1},
#endif
	{"func_off",	INT_T,		_OFFSET(miscEntry.func_off), _SIZE(miscEntry.func_off), 0},

	//struct Dot11QosEntry
#ifdef WIFI_WMM
	{"qos_enable",	INT_T,		_OFFSET(dot11QosEntry.dot11QosEnable), _SIZE(dot11QosEntry.dot11QosEnable), 0},
#ifdef WMM_APSD
	{"apsd_enable",	INT_T,		_OFFSET(dot11QosEntry.dot11QosAPSD), _SIZE(dot11QosEntry.dot11QosAPSD), 0},
#ifdef CLIENT_MODE
	{"apsd_sta_be",	INT_T,		_OFFSET(dot11QosEntry.UAPSD_AC_BE), _SIZE(dot11QosEntry.UAPSD_AC_BE), 0},
	{"apsd_sta_bk",	INT_T,		_OFFSET(dot11QosEntry.UAPSD_AC_BK), _SIZE(dot11QosEntry.UAPSD_AC_BK), 0},
	{"apsd_sta_vi",	INT_T,		_OFFSET(dot11QosEntry.UAPSD_AC_VI), _SIZE(dot11QosEntry.UAPSD_AC_VI), 0},
	{"apsd_sta_vo",	INT_T,		_OFFSET(dot11QosEntry.UAPSD_AC_VO), _SIZE(dot11QosEntry.UAPSD_AC_VO), 0},
#endif
#endif

#ifdef RTL_MANUAL_EDCA
	{"manual_edca", 		INT_T,			_OFFSET(dot11QosEntry.ManualEDCA),		_SIZE(dot11QosEntry.ManualEDCA),		0},
	{"sta_bkq_acm", 		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BK].ACM), _SIZE(dot11QosEntry.STA_manualEDCA[BK].ACM), 0},
	{"sta_bkq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BK].AIFSN), _SIZE(dot11QosEntry.STA_manualEDCA[BK].AIFSN), 7},
	{"sta_bkq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BK].ECWmin), _SIZE(dot11QosEntry.STA_manualEDCA[BK].ECWmin), 4},
	{"sta_bkq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BK].ECWmax), _SIZE(dot11QosEntry.STA_manualEDCA[BK].ECWmax), 10},
	{"sta_bkq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BK].TXOPlimit), _SIZE(dot11QosEntry.STA_manualEDCA[BK].TXOPlimit), 0},
	{"sta_beq_acm", 		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BE].ACM), _SIZE(dot11QosEntry.STA_manualEDCA[BE].ACM), 0},
	{"sta_beq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BE].AIFSN), _SIZE(dot11QosEntry.STA_manualEDCA[BE].AIFSN), 3},
	{"sta_beq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BE].ECWmin), _SIZE(dot11QosEntry.STA_manualEDCA[BE].ECWmin), 4},
	{"sta_beq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BE].ECWmax), _SIZE(dot11QosEntry.STA_manualEDCA[BE].ECWmax), 10},
	{"sta_beq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[BE].TXOPlimit), _SIZE(dot11QosEntry.STA_manualEDCA[BE].TXOPlimit), 0},
	{"sta_viq_acm", 		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VI].ACM), _SIZE(dot11QosEntry.STA_manualEDCA[VI].ACM), 0},
	{"sta_viq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VI].AIFSN), _SIZE(dot11QosEntry.STA_manualEDCA[VI].AIFSN), 2},
	{"sta_viq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VI].ECWmin), _SIZE(dot11QosEntry.STA_manualEDCA[VI].ECWmin), 3},
	{"sta_viq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VI].ECWmax), _SIZE(dot11QosEntry.STA_manualEDCA[VI].ECWmax), 4},
	{"sta_viq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VI].TXOPlimit), _SIZE(dot11QosEntry.STA_manualEDCA[VI].TXOPlimit), 188},
	//{"ap_viq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), (B band)?188:94},
	{"sta_voq_acm", 		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VO].ACM), _SIZE(dot11QosEntry.STA_manualEDCA[VO].ACM), 0},
	{"sta_voq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VO].AIFSN), _SIZE(dot11QosEntry.STA_manualEDCA[VO].AIFSN), 2},
	{"sta_voq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VO].ECWmin), _SIZE(dot11QosEntry.STA_manualEDCA[VO].ECWmin), 2},
	{"sta_voq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VO].ECWmax), _SIZE(dot11QosEntry.STA_manualEDCA[VO].ECWmax), 3},
	{"sta_voq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.STA_manualEDCA[VO].TXOPlimit), _SIZE(dot11QosEntry.STA_manualEDCA[VO].TXOPlimit), 102},
	//{"ap_voq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), (B band)?102:47},
	//{"ap_beq_acm",				INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BE].ACM), _SIZE(dot11QosEntry.AP_manualEDCA[BE].ACM), 0},
	{"ap_beq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BE].AIFSN), _SIZE(dot11QosEntry.AP_manualEDCA[BE].AIFSN), 7},
	{"ap_beq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BE].ECWmin), _SIZE(dot11QosEntry.AP_manualEDCA[BE].ECWmin), 4},
	{"ap_beq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BE].ECWmax), _SIZE(dot11QosEntry.AP_manualEDCA[BE].ECWmax), 10},
	{"ap_beq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BE].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[BE].TXOPlimit), 0},
	//{"ap_bkq_acm",				INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BK].ACM), _SIZE(dot11QosEntry.AP_manualEDCA[BK].ACM), 0},
	{"ap_bkq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BK].AIFSN), _SIZE(dot11QosEntry.AP_manualEDCA[BK].AIFSN), 3},
	{"ap_bkq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BK].ECWmin), _SIZE(dot11QosEntry.AP_manualEDCA[BK].ECWmin), 4},
	{"ap_bkq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BK].ECWmax), _SIZE(dot11QosEntry.AP_manualEDCA[BK].ECWmax), 6},
	{"ap_bkq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[BK].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[BK].TXOPlimit), 0},
	//{"ap_viq_acm",				INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].ACM), _SIZE(dot11QosEntry.AP_manualEDCA[VI].ACM), 0},
	{"ap_viq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].AIFSN), _SIZE(dot11QosEntry.AP_manualEDCA[VI].AIFSN), 1},
	{"ap_viq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].ECWmin), _SIZE(dot11QosEntry.AP_manualEDCA[VI].ECWmin), 3},
	{"ap_viq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].ECWmax), _SIZE(dot11QosEntry.AP_manualEDCA[VI].ECWmax), 4},
	{"ap_viq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), 188},
	//{"ap_viq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VI].TXOPlimit), (B band)?188:94},
	//{"ap_voq_acm",				INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].ACM), _SIZE(dot11QosEntry.AP_manualEDCA[VO].ACM), 0},
	{"ap_voq_aifsn",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].AIFSN), _SIZE(dot11QosEntry.AP_manualEDCA[VO].AIFSN), 1},
	{"ap_voq_cwmin",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].ECWmin), _SIZE(dot11QosEntry.AP_manualEDCA[VO].ECWmin), 2},
	{"ap_voq_cwmax",		INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].ECWmax), _SIZE(dot11QosEntry.AP_manualEDCA[VO].ECWmax), 3},
	{"ap_voq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), 102},
	//{"ap_voq_txoplimit",	INT_T,			_OFFSET(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), _SIZE(dot11QosEntry.AP_manualEDCA[VO].TXOPlimit), (B band)?102:47},

	{"tid0_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[0]), _SIZE(dot11QosEntry.TID_mapping[0]), BE_QUEUE},
	{"tid1_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[1]), _SIZE(dot11QosEntry.TID_mapping[1]), BK_QUEUE},
	{"tid2_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[2]), _SIZE(dot11QosEntry.TID_mapping[2]), BK_QUEUE},
	{"tid3_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[3]), _SIZE(dot11QosEntry.TID_mapping[3]), BE_QUEUE},
	{"tid4_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[4]), _SIZE(dot11QosEntry.TID_mapping[4]), VI_QUEUE},
	{"tid5_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[5]), _SIZE(dot11QosEntry.TID_mapping[5]), VI_QUEUE},
	{"tid6_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[6]), _SIZE(dot11QosEntry.TID_mapping[6]), VO_QUEUE},
	{"tid7_mapping",		BYTE_T,	_OFFSET(dot11QosEntry.TID_mapping[7]), _SIZE(dot11QosEntry.TID_mapping[7]), VO_QUEUE},
#endif //RTL_MANUAL_EDCA

#endif //WIFI_WMM

#ifdef WIFI_SIMPLE_CONFIG
	// struct WifiSimpleConfigEntry
	{"wsc_enable",	INT_T,	_OFFSET(wscEntry.wsc_enable), _SIZE(wscEntry.wsc_enable), 0},
	{"pin",			PIN_IND_T, 0, 0},
	/* WPS2DOTX   */
	/* support  Assigned MAC Addr,Assigned SSID,dymanic change STA's PIN code, 2011-0505 */	
	{"wsc_mypin",	WSC_SELF_PIN_IND_T, 0, 0},
	{"wsc_specssid",WSC_SEPC_SSID_CONN_IND_T, 0, 0},	
	{"wsc_specmac",	WSC_SEPC_MAC_CONN_IND_T, 0, 0},	
	/* WPS2DOTX   */
#ifdef INCLUDE_WPS
#ifndef CONFIG_MSC
	{"wsc_start2",   WSC_IND_T, 0, 0},
	{"wsc_end",   WSC_IND_T, 0, 0},
	{"wsc_soap_action",   WSC_IND_T, 0, 0},
	{"wps_led_control",   WSC_IND_T, 0, 0},
	{"wps_get_config",   WSC_IND_T, 0, 0},
	{"wps_debug",   WSC_IND_T, 0, 0},
	{"wps_reinit",   WSC_IND_T, 0, 0},
#endif
#endif
#ifdef CONFIG_RTL_COMAPI_CFGFILE
	{"wsc_start",		WSC_START_IND_T, 0, 0},
	{"wsc_mode",		WSC_MODE_IND_T, 0, 0},
	{"wsc_status",		WSC_STATUS_IND_T, 0, 0},
	{"wsc_method",		WSC_METHOD_IND_T, 0, 0},
	{"wsc_step",		WSC_STEP_IND_T, 0, 0},
	{"wsc_oob",		WSC_OOB_IND_T, 0, 0},
#endif //CONFIG_RTL_COMAPI_CFGFILE
#endif

#ifdef GBWC
	// struct GroupBandWidthControl
	{"gbwcmode",	INT_T,		_OFFSET(gbwcEntry.GBWCMode), _SIZE(gbwcEntry.GBWCMode), 0},
	{"gbwcnum",		INT_T,		_OFFSET(gbwcEntry.GBWCNum), _SIZE(gbwcEntry.GBWCNum), 0},
	{"gbwcaddr",	ACL_T,		_OFFSET(gbwcEntry.GBWCAddr), _SIZE(gbwcEntry.GBWCAddr), 0},
	{"gbwcthrd_tx",	INT_T,		_OFFSET(gbwcEntry.GBWCThrd_tx), _SIZE(gbwcEntry.GBWCThrd_tx), 30000},
	{"gbwcthrd_rx",	INT_T,		_OFFSET(gbwcEntry.GBWCThrd_rx), _SIZE(gbwcEntry.GBWCThrd_rx), 30000},	
#endif

	// struct Dot11nConfigEntry
	{"supportedmcs",INT_T,		_OFFSET(dot11nConfigEntry.dot11nSupportedMCS), _SIZE(dot11nConfigEntry.dot11nSupportedMCS), 0xffff},
	{"basicmcs",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nBasicMCS), _SIZE(dot11nConfigEntry.dot11nBasicMCS), 0},
	{"use40M",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nUse40M), _SIZE(dot11nConfigEntry.dot11nUse40M), 0},
	{"2ndchoffset",	INT_T,		_OFFSET(dot11nConfigEntry.dot11n2ndChOffset), _SIZE(dot11nConfigEntry.dot11n2ndChOffset), 1},
	{"shortGI20M",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nShortGIfor20M), _SIZE(dot11nConfigEntry.dot11nShortGIfor20M), 0},
	{"shortGI40M",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nShortGIfor40M), _SIZE(dot11nConfigEntry.dot11nShortGIfor40M), 0},
	{"stbc",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nSTBC), _SIZE(dot11nConfigEntry.dot11nSTBC), 0},
	{"ldpc",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nLDPC), _SIZE(dot11nConfigEntry.dot11nLDPC), 0},
	{"ampdu",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMPDU), _SIZE(dot11nConfigEntry.dot11nAMPDU), 0},
	{"amsdu",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDU), _SIZE(dot11nConfigEntry.dot11nAMSDU), 0},
	// for support SIGMA_TEST
	{"addba_reject",INT_T,		_OFFSET(dot11nConfigEntry.dot11nAddBAreject), _SIZE(dot11nConfigEntry.dot11nAddBAreject), 0},	
	{"ampduSndSz",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMPDUSendSz), _SIZE(dot11nConfigEntry.dot11nAMPDUSendSz), 0},
	{"supportedvht",INT_T,		_OFFSET(dot11acConfigEntry.dot11SupportedVHT), _SIZE(dot11acConfigEntry.dot11SupportedVHT), 0xfffa},	
	{"vht_txmap",INT_T,		_OFFSET(dot11acConfigEntry.dot11VHT_TxMap), _SIZE(dot11acConfigEntry.dot11VHT_TxMap), 0xfffff},	
	
#ifdef RX_BUFFER_GATHER
	{"amsduMax",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDURecvMax), _SIZE(dot11nConfigEntry.dot11nAMSDURecvMax), 1},
#else
#if defined(CONFIG_RTL8196B_GW_8M) || defined(CONFIG_RTL8196C_AP_ROOT) || defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL8196C_CLIENT_ONLY) || defined(__ECOS)
	{"amsduMax",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDURecvMax), _SIZE(dot11nConfigEntry.dot11nAMSDURecvMax), 0},
#else
	{"amsduMax",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDURecvMax), _SIZE(dot11nConfigEntry.dot11nAMSDURecvMax), 0},
#endif
#endif
	{"amsduTimeout",INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDUSendTimeout), _SIZE(dot11nConfigEntry.dot11nAMSDUSendTimeout), 400}, /*in us*/
	{"amsduNum",	INT_T,		_OFFSET(dot11nConfigEntry.dot11nAMSDUSendNum), _SIZE(dot11nConfigEntry.dot11nAMSDUSendNum), 15},
	{"lgyEncRstrct",INT_T,		_OFFSET(dot11nConfigEntry.dot11nLgyEncRstrct), _SIZE(dot11nConfigEntry.dot11nLgyEncRstrct), 15},
#ifdef WIFI_11N_2040_COEXIST
	{"coexist",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nCoexist), _SIZE(dot11nConfigEntry.dot11nCoexist), 0},
#endif
	{"txnoack",		INT_T,		_OFFSET(dot11nConfigEntry.dot11nTxNoAck), _SIZE(dot11nConfigEntry.dot11nTxNoAck), 0},

	// struct ReorderControlEntry
	{"rc_enable",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlEnable), _SIZE(reorderCtrlEntry.ReorderCtrlEnable), 1},
	{"rc_winsz",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlWinSz), _SIZE(reorderCtrlEntry.ReorderCtrlWinSz), RC_ENTRY_NUM},
	{"rc_timeout",	INT_T,		_OFFSET(reorderCtrlEntry.ReorderCtrlTimeout), _SIZE(reorderCtrlEntry.ReorderCtrlTimeout), 30000}, /*in us*/

#ifdef CONFIG_RTK_VLAN_SUPPORT
	// struct VlanConfig
	{"global_vlan",	INT_T,		_OFFSET(vlan.global_vlan), _SIZE(vlan.global_vlan), 0},
	{"is_lan",	INT_T,		_OFFSET(vlan.is_lan), _SIZE(vlan.is_lan), 1},
	{"vlan_enable",	INT_T,		_OFFSET(vlan.vlan_enable), _SIZE(vlan.vlan_enable), 0},
	{"vlan_tag",	INT_T,		_OFFSET(vlan.vlan_tag), _SIZE(vlan.vlan_tag), 0},
	{"vlan_id",	INT_T,		_OFFSET(vlan.vlan_id), _SIZE(vlan.vlan_id), 0},
	{"vlan_pri",	INT_T,		_OFFSET(vlan.vlan_pri), _SIZE(vlan.vlan_pri), 0},
	{"vlan_cfi",	INT_T,		_OFFSET(vlan.vlan_cfi), _SIZE(vlan.vlan_cfi), 0},
#endif

#ifdef  CONFIG_RTL_WAPI_SUPPORT
	{"wapiType",		INT_T,	_OFFSET(wapiInfo.wapiType), _SIZE(wapiInfo.wapiType), 0},
#ifdef  WAPI_SUPPORT_MULTI_ENCRYPT
	{"wapiUCastEncodeType",		INT_T,	_OFFSET(wapiInfo.wapiUCastEncodeType), _SIZE(wapiInfo.wapiUCastEncodeType), 0},
	{"wapiMCastEncodeType",		INT_T,	_OFFSET(wapiInfo.wapiMCastEncodeType), _SIZE(wapiInfo.wapiMCastEncodeType), 0},
#endif
	{"wapiPsk",	WAPI_KEY_T,	_OFFSET(wapiInfo.wapiPsk), _SIZE(wapiInfo.wapiPsk), 0},
	{"wapiPsklen",	INT_T,	_OFFSET(wapiInfo.wapiPsk.len), _SIZE(wapiInfo.wapiPsk.len), 0},
	{"wapiUCastKeyType",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyType), _SIZE(wapiInfo.wapiUpdateUCastKeyType), 0},
	{"wapiUCastKeyTimeout",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyTimeout), _SIZE(wapiInfo.wapiUpdateUCastKeyTimeout), 0},
	{"wapiUCastKeyPktNum",		INT_T,	_OFFSET(wapiInfo.wapiUpdateUCastKeyPktNum), _SIZE(wapiInfo.wapiUpdateUCastKeyPktNum), 0},
	{"wapiMCastKeyType",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyType), _SIZE(wapiInfo.wapiUpdateMCastKeyType), 0},
	{"wapiMCastKeyTimeout",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyTimeout), _SIZE(wapiInfo.wapiUpdateMCastKeyTimeout), 0},
	{"wapiMCastKeyPktNum",		INT_T,	_OFFSET(wapiInfo.wapiUpdateMCastKeyPktNum), _SIZE(wapiInfo.wapiUpdateMCastKeyPktNum), 0},
	{"wapiTimeout",		INT_ARRAY_T,	_OFFSET(wapiInfo.wapiTimeout), _SIZE(wapiInfo.wapiTimeout), 0},
#endif

#ifdef DOT11D
	{"countrycode",		INT_T,	_OFFSET(dot11dCountry.dot11CountryCodeSwitch), _SIZE(dot11dCountry.dot11CountryCodeSwitch), 1},
	{"countrystr",	STRING_T,	_OFFSET(dot11dCountry.dot11CountryString), _SIZE(dot11dCountry.dot11CountryString), 0},
#endif

#ifdef SUPPORT_MULTI_PROFILE
	{"ap_profile_enable",		INT_T,	_OFFSET(ap_profile.enable_profile), _SIZE(ap_profile.enable_profile), 0},
	{"ap_profile_num",		INT_T,	_OFFSET(ap_profile.profile_num), _SIZE(ap_profile.profile_num), 0},	
	{"ap_profile_add",		AP_PROFILE_T,	_OFFSET(ap_profile), _SIZE(ap_profile), 0},
#endif

#ifdef _DEBUG_RTL8192CD_
	// debug flag
	{"debug_err",	DEBUG_T,	1, sizeof(rtl8192cd_debug_err), 0},
	{"debug_info",	DEBUG_T,	2, sizeof(rtl8192cd_debug_info), 0},
	{"debug_trace",	DEBUG_T,	3, sizeof(rtl8192cd_debug_trace), 0},
	{"debug_warn",	DEBUG_T,	4, sizeof(rtl8192cd_debug_warn), 0},
#endif

	// for RF debug
	{"ofdm_1ss_oneAnt",	RFFT_T,	_OFFSET_RFFT(ofdm_1ss_oneAnt), _SIZE_RFFT(ofdm_1ss_oneAnt), 0},// 1ss and ofdm rate using one ant
	{"pathB_1T",		RFFT_T,	_OFFSET_RFFT(pathB_1T), _SIZE_RFFT(pathB_1T), 0},// using pathB as 1T2R/1T1R tx path
	{"rssi_dump",		RFFT_T,	_OFFSET_RFFT(rssi_dump), _SIZE_RFFT(rssi_dump), 0},
	{"rxfifoO",			RFFT_T,	_OFFSET_RFFT(rxfifoO), _SIZE_RFFT(rxfifoO), 0},
	{"raGoDownUpper",	RFFT_T,	_OFFSET_RFFT(raGoDownUpper), _SIZE_RFFT(raGoDownUpper), 50},
	{"raGoDown20MLower",RFFT_T,	_OFFSET_RFFT(raGoDown20MLower), _SIZE_RFFT(raGoDown20MLower), 18},
	{"raGoDown40MLower",RFFT_T,	_OFFSET_RFFT(raGoDown40MLower), _SIZE_RFFT(raGoDown40MLower), 15},
	{"raGoUpUpper",		RFFT_T,	_OFFSET_RFFT(raGoUpUpper), _SIZE_RFFT(raGoUpUpper), 55},
	{"raGoUp20MLower",	RFFT_T,	_OFFSET_RFFT(raGoUp20MLower), _SIZE_RFFT(raGoUp20MLower), 23},
	{"raGoUp40MLower",	RFFT_T,	_OFFSET_RFFT(raGoUp40MLower), _SIZE_RFFT(raGoUp40MLower), 20},
	{"dig_enable",		RFFT_T,	_OFFSET_RFFT(dig_enable), _SIZE_RFFT(dig_enable), 1},
#ifdef INTERFERENCE_CONTROL
	{"digGoLowerLevel", RFFT_T, _OFFSET_RFFT(digGoLowerLevel), _SIZE_RFFT(digGoLowerLevel), 30},
    {"digGoUpperLevel", RFFT_T, _OFFSET_RFFT(digGoUpperLevel), _SIZE_RFFT(digGoUpperLevel), 35},
	{"nbi_filter_enable",	RFFT_T,	_OFFSET_RFFT(nbi_filter_enable), _SIZE_RFFT(nbi_filter_enable), 0},
#else
	{"digGoLowerLevel",	RFFT_T,	_OFFSET_RFFT(digGoLowerLevel), _SIZE_RFFT(digGoLowerLevel), 35},
	{"digGoUpperLevel",	RFFT_T,	_OFFSET_RFFT(digGoUpperLevel), _SIZE_RFFT(digGoUpperLevel), 40},
#endif
	{"dcThUpper",		RFFT_T,	_OFFSET_RFFT(dcThUpper), _SIZE_RFFT(dcThUpper), 30},
	{"dcThLower",		RFFT_T,	_OFFSET_RFFT(dcThLower), _SIZE_RFFT(dcThLower), 25},
	{"rssiTx20MUpper",	RFFT_T,	_OFFSET_RFFT(rssiTx20MUpper), _SIZE_RFFT(rssiTx20MUpper), 20},
	{"rssiTx20MLower",	RFFT_T,	_OFFSET_RFFT(rssiTx20MLower), _SIZE_RFFT(rssiTx20MLower), 15},
	{"rssi_expire_to",	RFFT_T,	_OFFSET_RFFT(rssi_expire_to), _SIZE_RFFT(rssi_expire_to), 60},
	{"rts_init_rate",	RFFT_T,	_OFFSET_RFFT(rts_init_rate), _SIZE_RFFT(rts_init_rate), 0},

	{"cck_pwr_max",		RFFT_T,	_OFFSET_RFFT(cck_pwr_max), _SIZE_RFFT(cck_pwr_max), 0},
	{"cck_tx_pathB",	RFFT_T,	_OFFSET_RFFT(cck_tx_pathB), _SIZE_RFFT(cck_tx_pathB), 0},

	{"tx_pwr_ctrl",		RFFT_T,	_OFFSET_RFFT(tx_pwr_ctrl), _SIZE_RFFT(tx_pwr_ctrl), 0},

	// 11n ap AES debug
	{"aes_check_th",	RFFT_T,	_OFFSET_RFFT(aes_check_th), _SIZE_RFFT(aes_check_th), 2},

	// Tx power tracking
	{"tpt_period",		RFFT_T,	_OFFSET_RFFT(tpt_period), _SIZE_RFFT(tpt_period), 30},

	// TXOP enlarge
	{"txop_enlarge_upper",		RFFT_T,	_OFFSET_RFFT(txop_enlarge_upper), _SIZE_RFFT(txop_enlarge_upper), 20},
	{"txop_enlarge_lower",		RFFT_T,	_OFFSET_RFFT(txop_enlarge_lower), _SIZE_RFFT(txop_enlarge_lower), 15},

#ifdef LOW_TP_TXOP
	{"low_tp_txop",				RFFT_T,	_OFFSET_RFFT(low_tp_txop),	_SIZE_RFFT(low_tp_txop), 1},
	{"low_tp_txop_thd_n",		RFFT_T,	_OFFSET_RFFT(low_tp_txop_thd_n),	_SIZE_RFFT(low_tp_txop_thd_n), 22},
	{"low_tp_txop_thd_g",		RFFT_T,	_OFFSET_RFFT(low_tp_txop_thd_g),	_SIZE_RFFT(low_tp_txop_thd_g), 17},
	{"low_tp_txop_thd_low",		RFFT_T,	_OFFSET_RFFT(low_tp_txop_thd_low),	_SIZE_RFFT(low_tp_txop_thd_low), 0},
	{"low_tp_txop_delay",		RFFT_T,	_OFFSET_RFFT(low_tp_txop_delay),	_SIZE_RFFT(low_tp_txop_delay), 1},
	{"cwmax_enhance_thd",		RFFT_T,	_OFFSET_RFFT(cwmax_enhance_thd), _SIZE_RFFT(cwmax_enhance_thd), 2000},
#endif

	// 2.3G support
	{"frq_2_3G",		RFFT_T,	_OFFSET_RFFT(use_frq_2_3G), _SIZE_RFFT(use_frq_2_3G), 0},

	// for mp test
#ifdef MP_TEST
	{"mp_specific",		RFFT_T,	_OFFSET_RFFT(mp_specific), _SIZE_RFFT(mp_specific), 0},
#endif

#ifdef IGMP_FILTER_CMO
	{"igmp_deny",		RFFT_T,	_OFFSET_RFFT(igmp_deny), _SIZE_RFFT(igmp_deny), 0},
#endif
	//Support IP multicast->unicast
#ifdef SUPPORT_TX_MCAST2UNI
	{"mc2u_disable",	RFFT_T,	_OFFSET_RFFT(mc2u_disable), _SIZE_RFFT(mc2u_disable), 0},
	{"mc2u_drop_unknown",	RFFT_T,	_OFFSET_RFFT(mc2u_drop_unknown), _SIZE_RFFT(mc2u_drop_unknown), 0},
	{"mc2u_flood_ctrl",	RFFT_T,	_OFFSET_RFFT(mc2u_flood_ctrl), _SIZE_RFFT(mc2u_flood_ctrl), 0},
	{"mc2u_flood_mac_num",	RFFT_T,		_OFFSET_RFFT(mc2u_flood_mac_num), _SIZE_RFFT(mc2u_flood_mac_num), 0},
	{"mc2u_flood_mac",		RFFT_ACL_T,		_OFFSET_RFFT(mc2u_flood_mac), _SIZE_RFFT(mc2u_flood_mac), 0},
#endif

#ifdef HIGH_POWER_EXT_PA
	{"use_ext_pa",		RFFT_T,	_OFFSET_RFFT(use_ext_pa), _SIZE_RFFT(use_ext_pa), 0},
	{"hp_ofdm_max",		RFFT_T,	_OFFSET_RFFT(hp_ofdm_pwr_max), _SIZE_RFFT(hp_ofdm_pwr_max), 63},
	{"hp_cck_max",		RFFT_T,	_OFFSET_RFFT(hp_cck_pwr_max), _SIZE_RFFT(hp_cck_pwr_max), 63},
#endif

#if defined(CONFIG_RTL_NOISE_CONTROL) || defined(CONFIG_RTL_NOISE_CONTROL_92C)
	{"dnc_enable",		RFFT_T,	_OFFSET_RFFT(dnc_enable), _SIZE_RFFT(dnc_enable), 1},
#endif

#ifdef CONFIG_HIGH_POWER_EXT_LNA
	{"use_ext_lna",		RFFT_T, _OFFSET_RFFT(use_ext_lna), _SIZE_RFFT(use_ext_lna), 1},
#else
	{"use_ext_lna",		RFFT_T, _OFFSET_RFFT(use_ext_lna), _SIZE_RFFT(use_ext_lna), 0},
#endif
	{"ndsi_support",	RFFT_T, _OFFSET_RFFT(NDSi_support), _SIZE_RFFT(NDSi_support), 0},
	{"edcca_thd",		RFFT_T, _OFFSET_RFFT(edcca_thd), _SIZE_RFFT(edcca_thd), 0},

#ifdef EN_EFUSE
	{"use_efuse",		INT_T,		_OFFSET(efuseEntry.enable_efuse), _SIZE(efuseEntry.enable_efuse), 1},
#endif

#ifdef PCIE_POWER_SAVING
	{"ps",		RFFT_T, _OFFSET_RFFT(power_save), _SIZE_RFFT(power_save), (/*L2_en|*/ L1_en|_1x1_en|offload_en|stop_dma_en)},
#ifdef RTL8676_WAKE_GPIO
#ifdef WIFI_SIMPLE_CONFIG
		{"wps_led_active",	RFFT_T, _OFFSET_RFFT(wps_led_active), _SIZE_RFFT(wps_led_active), 0},
#endif
#endif
#endif


#ifdef SW_ANT_SWITCH
	{"antSw_enable",		RFFT_T, _OFFSET_RFFT(antSw_enable), _SIZE_RFFT(antSw_enable), 1},
//	{"antSw_dump",			RFFT_T, _OFFSET_RFFT(antSw_dump), _SIZE_RFFT(antSw_dump), 0},
#endif
#ifdef HW_ANT_SWITCH
	{"antHw_enable",		RFFT_T, _OFFSET_RFFT(antHw_enable), _SIZE_RFFT(antHw_enable), 1},
#endif
#if defined(HW_ANT_SWITCH) || defined(SW_ANT_SWITCH)
	{"antdump",				RFFT_T, _OFFSET_RFFT(ant_dump), _SIZE_RFFT(ant_dump), 0},
	{"antSw_select",		RFFT_T, _OFFSET_RFFT(antSw_select), _SIZE_RFFT(antSw_select), 1},
#endif
#ifdef ADD_TX_POWER_BY_CMD
	{"txPowerPlus_cck_1",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_cck_1), _SIZE_RFFT(txPowerPlus_cck_1), 0x7f},
	{"txPowerPlus_cck_2",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_cck_2), _SIZE_RFFT(txPowerPlus_cck_2), 0x7f},
	{"txPowerPlus_cck_5",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_cck_5), _SIZE_RFFT(txPowerPlus_cck_5), 0x7f},
	{"txPowerPlus_cck_11",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_cck_11), _SIZE_RFFT(txPowerPlus_cck_11), 0x7f},
	{"txPowerPlus_ofdm_6",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_6), _SIZE_RFFT(txPowerPlus_ofdm_6), 0x7f},
	{"txPowerPlus_ofdm_9",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_9), _SIZE_RFFT(txPowerPlus_ofdm_9), 0x7f},
	{"txPowerPlus_ofdm_12",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_12), _SIZE_RFFT(txPowerPlus_ofdm_12), 0x7f},
	{"txPowerPlus_ofdm_18",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_18), _SIZE_RFFT(txPowerPlus_ofdm_18), 0x7f},
	{"txPowerPlus_ofdm_24",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_24), _SIZE_RFFT(txPowerPlus_ofdm_24), 0x7f},
	{"txPowerPlus_ofdm_36",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_36), _SIZE_RFFT(txPowerPlus_ofdm_36), 0x7f},
	{"txPowerPlus_ofdm_48",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_48), _SIZE_RFFT(txPowerPlus_ofdm_48), 0x7f},
	{"txPowerPlus_ofdm_54",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_ofdm_54), _SIZE_RFFT(txPowerPlus_ofdm_54), 0x7f},
	{"txPowerPlus_mcs_0",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_0), _SIZE_RFFT(txPowerPlus_mcs_0), 0x7f},
	{"txPowerPlus_mcs_1",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_1), _SIZE_RFFT(txPowerPlus_mcs_1), 0x7f},
	{"txPowerPlus_mcs_2",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_2), _SIZE_RFFT(txPowerPlus_mcs_2), 0x7f},
	{"txPowerPlus_mcs_3",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_3), _SIZE_RFFT(txPowerPlus_mcs_3), 0x7f},
	{"txPowerPlus_mcs_4",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_4), _SIZE_RFFT(txPowerPlus_mcs_4), 0x7f},
	{"txPowerPlus_mcs_5",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_5), _SIZE_RFFT(txPowerPlus_mcs_5), 0x7f},
	{"txPowerPlus_mcs_6",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_6), _SIZE_RFFT(txPowerPlus_mcs_6), 0x7f},
	{"txPowerPlus_mcs_7",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_7), _SIZE_RFFT(txPowerPlus_mcs_7), 0x7f},
	{"txPowerPlus_mcs_8",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_8), _SIZE_RFFT(txPowerPlus_mcs_8), 0x7f},
	{"txPowerPlus_mcs_9",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_9), _SIZE_RFFT(txPowerPlus_mcs_9), 0x7f},
	{"txPowerPlus_mcs_10",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_10), _SIZE_RFFT(txPowerPlus_mcs_10), 0x7f},
	{"txPowerPlus_mcs_11",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_11), _SIZE_RFFT(txPowerPlus_mcs_11), 0x7f},
	{"txPowerPlus_mcs_12",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_12), _SIZE_RFFT(txPowerPlus_mcs_12), 0x7f},
	{"txPowerPlus_mcs_13",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_13), _SIZE_RFFT(txPowerPlus_mcs_13), 0x7f},
	{"txPowerPlus_mcs_14",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_14), _SIZE_RFFT(txPowerPlus_mcs_14), 0x7f},
	{"txPowerPlus_mcs_15",	RFFT_T,	_OFFSET_RFFT(txPowerPlus_mcs_15), _SIZE_RFFT(txPowerPlus_mcs_15), 0x7f},
#endif

	{"rootFwBeacon",		RFFT_T,	_OFFSET_RFFT(rootFwBeacon), _SIZE_RFFT(rootFwBeacon), 1},
	{"ledBlinkingFreq",		RFFT_T,	_OFFSET_RFFT(ledBlinkingFreq), _SIZE_RFFT(ledBlinkingFreq), 1},

	{"diffAmpduSz",		RFFT_T,	_OFFSET_RFFT(diffAmpduSz), _SIZE_RFFT(diffAmpduSz), 1},
#ifdef CONFIG_RTL8672
	{"1rcca",				RFFT_T,	_OFFSET_RFFT(one_path_cca), _SIZE_RFFT(one_path_cca), 1},
#else
	{"1rcca",				RFFT_T,	_OFFSET_RFFT(one_path_cca), _SIZE_RFFT(one_path_cca), 0},
#endif

#ifdef CONFIG_RTL_COMAPI_CFGFILE
	{"hwaddr",				BYTE_ARRAY_T,	_OFFSET(dot11OperationEntry.hwaddr), _SIZE(dot11OperationEntry.hwaddr), 0},
	{"ssid2scan",			SSID2SCAN_STRING_T, _OFFSET(dot11StationConfigEntry.dot11SSIDtoScan), _SIZE(dot11StationConfigEntry.dot11SSIDtoScan), 0},
#ifdef 	CONFIG_RTK_MESH
	{"meshSilence", 		BYTE_T, _OFFSET(dot1180211sInfo.meshSilence), _SIZE(dot1180211sInfo.meshSilence), 0},
#endif
#endif

#ifdef DFS
	{"dfsdbgmode",		RFFT_T,	_OFFSET_RFFT(dfsdbgmode), _SIZE_RFFT(dfsdbgmode), 0},
	{"dfsdelayiqk",         RFFT_T, _OFFSET_RFFT(dfsdelayiqk), _SIZE_RFFT(dfsdelayiqk), 1},
	{"det_off",			RFFT_T,	_OFFSET_RFFT(dfs_det_off), _SIZE_RFFT(dfs_det_off), 1},
	{"det_reset",		RFFT_T,	_OFFSET_RFFT(dfs_det_reset), _SIZE_RFFT(dfs_det_reset), 0},
	{"fa_lower",		RFFT_T,	_OFFSET_RFFT(dfs_fa_cnt_lower), _SIZE_RFFT(dfs_fa_cnt_lower), 40},
	{"fa_mid",			RFFT_T,	_OFFSET_RFFT(dfs_fa_cnt_mid), _SIZE_RFFT(dfs_fa_cnt_mid), 120},
	{"fa_upper",		RFFT_T,	_OFFSET_RFFT(dfs_fa_cnt_upper), _SIZE_RFFT(dfs_fa_cnt_upper), 150},
	{"crc32_lower",		RFFT_T,	_OFFSET_RFFT(dfs_crc32_cnt_lower), _SIZE_RFFT(dfs_crc32_cnt_lower), 10},
	{"ratio_th",		RFFT_T,	_OFFSET_RFFT(dfs_fa_ratio_th), _SIZE_RFFT(dfs_fa_ratio_th), 15},
	{"det_period",		RFFT_T,	_OFFSET_RFFT(dfs_det_period), _SIZE_RFFT(dfs_det_period), 20},
	{"det_print",		RFFT_T,	_OFFSET_RFFT(dfs_det_print), _SIZE_RFFT(dfs_det_print), 0},
	{"det_print1",		RFFT_T,	_OFFSET_RFFT(dfs_det_print1), _SIZE_RFFT(dfs_det_print1), 1},
	{"det_print2",		RFFT_T,	_OFFSET_RFFT(dfs_det_print2), _SIZE_RFFT(dfs_det_print2), 0},
	{"det_print3",		RFFT_T,	_OFFSET_RFFT(dfs_det_print3), _SIZE_RFFT(dfs_det_print3), 0},
	{"det_print_time",	RFFT_T,	_OFFSET_RFFT(dfs_det_print_time), _SIZE_RFFT(dfs_det_print_time), 0},
	{"hist_len",		RFFT_T,	_OFFSET_RFFT(dfs_det_hist_len), _SIZE_RFFT(dfs_det_hist_len), 5},
	{"sum_th",			RFFT_T,	_OFFSET_RFFT(dfs_det_sum_th), _SIZE_RFFT(dfs_det_sum_th), 2},
	{"flag_offset",		RFFT_T,	_OFFSET_RFFT(dfs_det_flag_offset), _SIZE_RFFT(dfs_det_flag_offset), 2},
	{"DPT_FA_TH_upper",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_fa_th_upper), _SIZE_RFFT(dfs_dpt_fa_th_upper), 1000},
	{"DPT_FA_TH_lower",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_fa_th_lower), _SIZE_RFFT(dfs_dpt_fa_th_lower), 80},
	{"DPT_Pulse_TH_mid",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_pulse_th_mid), _SIZE_RFFT(dfs_dpt_pulse_th_mid), 5},
	{"DPT_Pulse_TH_lower",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_pulse_th_lower), _SIZE_RFFT(dfs_dpt_pulse_th_lower), 1},
	{"DPT_ST_L2H_max",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_st_l2h_max), _SIZE_RFFT(dfs_dpt_st_l2h_max), 0x4e},
	{"DPT_ST_L2H_min",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_st_l2h_min), _SIZE_RFFT(dfs_dpt_st_l2h_min), 0x22},
	{"DPT_ST_L2H_add",	RFFT_T,	_OFFSET_RFFT(dfs_dpt_st_l2h_add), _SIZE_RFFT(dfs_dpt_st_l2h_add), 1},
	{"DPT_ini_gain_th",	RFFT_T,	_OFFSET_RFFT(dpt_ini_gain_th), _SIZE_RFFT(dpt_ini_gain_th), 0x30},
	{"pwdb_th",			RFFT_T,	_OFFSET_RFFT(dfs_pwdb_th), _SIZE_RFFT(dfs_pwdb_th), 0x0d},
	{"psd_pw_th",		RFFT_T,	_OFFSET_RFFT(dfs_psd_pw_th), _SIZE_RFFT(dfs_psd_pw_th), 65},
	{"psd_fir_decay",	RFFT_T,	_OFFSET_RFFT(dfs_psd_fir_decay), _SIZE_RFFT(dfs_psd_fir_decay), 23},
	{"skip_iqk",		RFFT_T,	_OFFSET_RFFT(dfs_skip_iqk), _SIZE_RFFT(dfs_skip_iqk), 0},
	{"scan_inband",		RFFT_T,	_OFFSET_RFFT(dfs_scan_inband), _SIZE_RFFT(dfs_scan_inband), 0},
	{"psd_op",			RFFT_T,	_OFFSET_RFFT(dfs_psd_op), _SIZE_RFFT(dfs_psd_op), 1},
	{"psd_tp_th",		RFFT_T,	_OFFSET_RFFT(dfs_psd_tp_th), _SIZE_RFFT(dfs_psd_tp_th), 2},
#endif

#ifdef SW_TX_QUEUE
	{"swqh",          RFFT_T, _OFFSET_RFFT(swq_en_highthd), _SIZE_RFFT(swq_en_highthd), 400},
	{"swql",          RFFT_T, _OFFSET_RFFT(swq_dis_lowthd), _SIZE_RFFT(swq_dis_lowthd), 80},
	{"swqen",         RFFT_T, _OFFSET_RFFT(swq_enable), _SIZE_RFFT(swq_enable), 1},
	{"swqdbg",        RFFT_T, _OFFSET_RFFT(swq_dbg), _SIZE_RFFT(swq_dbg), 0},
#if defined(CONFIG_RTL_819XD)
	{"swqaggnum",		RFFT_T, _OFFSET_RFFT(swq_aggnum), _SIZE_RFFT(swq_aggnum), 4},
#else
	{"swqaggnum",		RFFT_T, _OFFSET_RFFT(swq_aggnum), _SIZE_RFFT(swq_aggnum), 8},
#endif
	{"thd1",			RFFT_T, _OFFSET_RFFT(timeout_thd), _SIZE_RFFT(timeout_thd), 60},
	{"thd2",            RFFT_T, _OFFSET_RFFT(timeout_thd2), _SIZE_RFFT(timeout_thd2), 150},
	{"thd3",		  	RFFT_T, _OFFSET_RFFT(timeout_thd3), _SIZE_RFFT(timeout_thd3), 300},
#endif

#ifdef A4_STA
	{"a4_enable",			RFFT_T,	_OFFSET_RFFT(a4_enable), _SIZE_RFFT(a4_enable), 1},
#endif

#ifdef RTL8192D_INT_PA
	{"use_intpa92d",		RFFT_T,	_OFFSET_RFFT(use_intpa92d), _SIZE_RFFT(use_intpa92d), 0},
#endif
	{"pwr_by_rate",        RFFT_T, _OFFSET_RFFT(pwr_by_rate), _SIZE_RFFT(pwr_by_rate), 0},
#ifdef _TRACKING_TABLE_FILE
	{"pwr_track_file",        RFFT_T, _OFFSET_RFFT(pwr_track_file), _SIZE_RFFT(pwr_track_file), 0},
#endif
#ifdef TXPWR_LMT
	{"disable_txpwrlmt",	RFFT_T, _OFFSET_RFFT(disable_txpwrlmt), _SIZE_RFFT(disable_txpwrlmt), 0},
#endif
#ifdef CONFIG_RTL_92D_DMDP
	{"peerReinit",	RFFT_T, _OFFSET_RFFT(peerReinit), _SIZE_RFFT(peerReinit), 0},
#endif
#ifdef WIFI_WMM
	{"wifi_beq_iot",	RFFT_T, _OFFSET_RFFT(wifi_beq_iot), _SIZE_RFFT(wifi_beq_iot), 0},
#endif
	{"bcast_to_dzq",	RFFT_T, _OFFSET_RFFT(bcast_to_dzq), _SIZE_RFFT(bcast_to_dzq), 0},
#ifdef TLN_STATS
	{"stats_time_interval",        RFFT_T, _OFFSET_RFFT(stats_time_interval), _SIZE_RFFT(stats_time_interval), 86400},
#endif
#ifdef TX_EARLY_MODE
	{"em_enable",	RFFT_T, _OFFSET_RFFT(em_enable), _SIZE_RFFT(em_enable), 1},
#endif
#ifdef CLIENT_MODE
	{"sta_mode_ps",	RFFT_T, _OFFSET_RFFT(sta_mode_ps), _SIZE_RFFT(sta_mode_ps), 0},
#endif
#ifdef CONFIG_RTL_WLAN_DOS_FILTER
	{"dos_block_time",	RFFT_T, _OFFSET_RFFT(dos_block_time), _SIZE_RFFT(dos_block_time), 20},
#endif

	{"intel_tp",	RFFT_T, _OFFSET_RFFT(intel_rtylmt_tp_margin), _SIZE_RFFT(intel_rtylmt_tp_margin), 125*1024}, /* unit: byte */

#ifdef CONFIG_RTL_88E_SUPPORT
	{"disable_pkt_pause",	RFFT_T, _OFFSET_RFFT(disable_pkt_pause), _SIZE_RFFT(disable_pkt_pause), 0},
	{"disable_pkt_nolink",	RFFT_T, _OFFSET_RFFT(disable_pkt_nolink), _SIZE_RFFT(disable_pkt_nolink), 0},
        {"maxpktfail",  RFFT_T, _OFFSET_RFFT(max_pkt_fail), _SIZE_RFFT(max_pkt_fail), 50},
	{"minpktfail",  RFFT_T, _OFFSET_RFFT(min_pkt_fail), _SIZE_RFFT(min_pkt_fail), 30},
#endif
	{"low_tp_no_aggr",	RFFT_T, _OFFSET_RFFT(low_tp_no_aggr), _SIZE_RFFT(low_tp_no_aggr), 0},
#ifdef CONFIG_RTL_8812_SUPPORT  //vht rate 
//	{"supported_vht_rates",	RFFT_T, _OFFSET_RFFT(dot11SupportedVHT), _SIZE_RFFT(dot11SupportedVHT), 0xfffa},
//	{"vht_txrate_map",	RFFT_T, _OFFSET_RFFT(dot11VHT_TxMap), _SIZE_RFFT(dot11VHT_TxMap), 0xfffff},
//	{"update_rainfo",	RFFT_T, _OFFSET_RFFT(update_rainfo), _SIZE_RFFT(update_rainfo), 0x1},
#ifdef FOR_VHT5G_PF
	{"cca_test",	RFFT_T, _OFFSET_RFFT(use_cca), _SIZE_RFFT(use_cca), 0x0},
// operating mode notification
	{"opm",	RFFT_T, _OFFSET_RFFT(oper_mode_field), _SIZE_RFFT(oper_mode_field), 0x00},
	{"opmtest", RFFT_T, _OFFSET_RFFT(opmtest), _SIZE_RFFT(opmtest), 0x00},	

// local power constraint
	{"lpwrc", RFFT_T, _OFFSET_RFFT(lpwrc), _SIZE_RFFT(lpwrc), 0}, 
	
// channel switch 
	{"csa",	RFFT_T, _OFFSET_RFFT(csa), _SIZE_RFFT(csa), 0x00},
	{"lgirate", RFFT_T, _OFFSET_RFFT(lgirate), _SIZE_RFFT(lgirate), 0xffff}, 
#endif
	{"cca_rts",	RFFT_T, _OFFSET_RFFT(cca_rts), _SIZE_RFFT(cca_rts), 0x00},	// 1: static, 2: dynamic

// Tx Power Diff
	{"pwrdiff_20BW1S_OFDM1T_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_20BW1S_OFDM1T_A), _SIZE(dot11RFEntry.pwrdiff_20BW1S_OFDM1T_A), 0},
	{"pwrdiff_40BW2S_20BW2S_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_40BW2S_20BW2S_A), _SIZE(dot11RFEntry.pwrdiff_40BW2S_20BW2S_A), 0},
	{"pwrdiff_5G_20BW1S_OFDM1T_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_A), _SIZE(dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_A), 0},
	{"pwrdiff_5G_40BW2S_20BW2S_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_A), _SIZE(dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_A), 0},
	{"pwrdiff_5G_80BW1S_160BW1S_A",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_A), _SIZE(dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_A), 0},
	{"pwrdiff_5G_80BW2S_160BW2S_A", BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_A), _SIZE(dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_A), 0},
	{"pwrdiff_20BW1S_OFDM1T_B", BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_20BW1S_OFDM1T_B), _SIZE(dot11RFEntry.pwrdiff_20BW1S_OFDM1T_B), 0},
	{"pwrdiff_40BW2S_20BW2S_B", BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_40BW2S_20BW2S_B), _SIZE(dot11RFEntry.pwrdiff_40BW2S_20BW2S_B), 0},
	{"pwrdiff_5G_20BW1S_OFDM1T_B",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_B), _SIZE(dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_B), 0},
	{"pwrdiff_5G_40BW2S_20BW2S_B",	BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_B), _SIZE(dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_B), 0},
	{"pwrdiff_5G_80BW1S_160BW1S_B", BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_B), _SIZE(dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_B), 0},
	{"pwrdiff_5G_80BW2S_160BW2S_B", BYTE_ARRAY_T, _OFFSET(dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_B), _SIZE(dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_B), 0},
#endif
	{"txforce", RFFT_T, _OFFSET_RFFT(txforce), _SIZE_RFFT(txforce), 0xff},
};

#ifdef _DEBUG_RTL8192CD_
unsigned long rtl8192cd_debug_err=0xffffffff;
unsigned long rtl8192cd_debug_info=0;
unsigned long rtl8192cd_debug_trace=0;
unsigned long rtl8192cd_debug_warn=0;
#endif

void MDL_DEVINIT set_mib_default_tbl(struct rtl8192cd_priv *priv)
{
	int i;
	int arg_num = sizeof(mib_table)/sizeof(struct iwpriv_arg);

	for (i=0; i<arg_num; i++) {
		if (mib_table[i].Default) {
			if (mib_table[i].type == BYTE_T)
				*(((unsigned char *)priv->pmib)+mib_table[i].offset) = (unsigned char)mib_table[i].Default;
			else if (mib_table[i].type == INT_T)
				memcpy(((unsigned char *)priv->pmib)+mib_table[i].offset, (unsigned char *)&mib_table[i].Default, sizeof(int));
			else if (mib_table[i].type == RFFT_T && mib_table[i].len == 1)
				*(((unsigned char *)&(priv->pshare->rf_ft_var))+mib_table[i].offset) = (unsigned char)mib_table[i].Default;
			else if (mib_table[i].type == RFFT_T && mib_table[i].len == 4)
				memcpy(((unsigned char *)&(priv->pshare->rf_ft_var))+mib_table[i].offset, (unsigned char *)&mib_table[i].Default, sizeof(int));
			else {
				// We only give default value of types of BYTE_T and INT_T here.
				// Some others are gave in set_mib_default().
			}
		}
	}
}


#ifdef TXPWR_LMT

int _convert_2_pwr_dot(char *s, int base)
{
	int k = 0;
	int flag = 0;

	k = 0;
	if (base == 10) {
		while (*s >= '0' && *s <= '9') {
			flag = 1;
			k = 10 * k + (*s - '0');
			s++;
		}

		k = k*2;

		if(*s == '.'){
			flag = 1;
			s++;
			
			if(*s >= '5' && *s <= '9')
				k++;				
		}
	}
	else
		return 0;

	if (!flag)
		return -1;
	return k;
}


#endif


int _atoi(char *s, int base)
{
	int k = 0;
	int sign = 1;

	k = 0;
	if (base == 10) {
		if(*s== '-') {
			sign = -1;
			s++;
		}
		while (*s != '\0' && *s >= '0' && *s <= '9') {
			k = 10 * k + (*s - '0');
			s++;
		}
		k *= sign;
	}
	else {
		while (*s != '\0') {
			int v;
			if ( *s >= '0' && *s <= '9')
				v = *s - '0';
			else if ( *s >= 'a' && *s <= 'f')
				v = *s - 'a' + 10;
			else if ( *s >= 'A' && *s <= 'F')
				v = *s - 'A' + 10;
			else {
				_DEBUG_ERR("error hex format!\n");
#ifdef TXPWR_LMT_8812
				return k;
#else
				return 0;
#endif
			}
			k = 16 * k + v;
			s++;
		}
	}
	return k;
}


static struct iwpriv_arg *get_tbl_entry(char *pstr)
{
	int i=0;
	int arg_num = sizeof(mib_table)/sizeof(struct iwpriv_arg);
	volatile char name[128];

	while (*pstr && *pstr != '=')
		name[i++] = *pstr++;
	name[i] = '\0';

	for (i=0; i<arg_num; i++) {
		if (!strcmp((char *)name, mib_table[i].name)) {
			return &mib_table[i];
		}
	}
	return NULL;
}


int get_array_val(unsigned char *dst, char *src, int len)
{
	char tmpbuf[4];
	int num=0;

	while (len > 0) {
		memcpy(tmpbuf, src, 2);
		tmpbuf[2]='\0';
		*dst++ = (unsigned char)_atoi(tmpbuf, 16);
		len-=2;
		src+=2;
		num++;
	}
	return num;
}


char *get_arg(char *src, char *val)
{
	int len=0;

	while (*src && *src!=',') {
		*val++ = *src++;
		len++;
	}
	if (len == 0)
		return NULL;

	*val = '\0';

	if (*src==',')
		src++;

	return src;
}


#ifdef SUPPORT_MULTI_PROFILE
static int  add_ap_profile(struct rtl8192cd_priv *priv, unsigned char *data)
{
	char  tmpbuf[100], *ptr;
	struct ap_profile profile;	

	if (priv->pmib->ap_profile.profile_num >= PROFILE_NUM) {
		panic_printk("Can't add new one because profile table is full!\n");
		return -1;		
	}

	if (data == NULL) {
		panic_printk("invalid ap_profile_add value [%s] !\n", data);
		return -1;
	}
	ptr  = get_arg((char *)data, tmpbuf);
	if (ptr == NULL || strlen(tmpbuf) == 0) {
		panic_printk("SSID must be set!\n");
		return -1;
	}		
	strcpy(profile.ssid, tmpbuf);

	ptr  = get_arg(ptr, tmpbuf);
	if (ptr == NULL) {
		panic_printk("encryption must be set!\n");
		return -1;
	}				
	profile.encryption =  _atoi(tmpbuf, 10);		
	if (profile.encryption > 4) {
		panic_printk("Invalid encryption value!\n");
		return -1;
	}					

	ptr  = get_arg(ptr, tmpbuf);
	if (ptr == NULL) {
		panic_printk("auth_type must be set!\n");
		return -1;
	}				
	profile.auth_type =  _atoi(tmpbuf, 10);
	if (profile.encryption == 0)
		goto copy_profile;
	
	if (profile.encryption == 1 || profile.encryption == 2) {
		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("default wep tx key must be set!\n");
			return -1;
		}
		profile.wep_default_key =  _atoi(tmpbuf, 10);

		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("wep key1 must be set!\n");
			return -1;			
		}		
		if (profile.encryption == 1 && strlen(tmpbuf) != 10) {
			panic_printk("Invalid wep64 key1 value!\n");
			return -1;
		}		
		if (profile.encryption == 2 && strlen(tmpbuf) != 26) {
			panic_printk("Invalid wep128 key1 value!\n");
			return -1;
		}
		get_array_val(profile.wep_key1, tmpbuf, strlen(tmpbuf));
		
		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("wep key2 must be set!\n");
			return -1;			
		}		
		if (profile.encryption == 1 && strlen(tmpbuf) != 10) {
			panic_printk("Invalid wep64 key2 value!\n");
			return -1;
		}
		if (profile.encryption == 2 && strlen(tmpbuf) != 26) {
			panic_printk("Invalid wep128 key2 value!\n");
			return -1;
		}
		get_array_val(profile.wep_key2, tmpbuf, strlen(tmpbuf));
		
		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("wep key3 must be set!\n");
			return -1;			
		}		
		if (profile.encryption == 1 && strlen(tmpbuf) != 10) {
			panic_printk("Invalid wep64 key3 value!\n");
			return -1;
		}
		if (profile.encryption == 2 && strlen(tmpbuf) != 26) {
			panic_printk("Invalid wep128 key3 value!\n");
			return -1;
		}
		get_array_val(profile.wep_key3, tmpbuf, strlen(tmpbuf));
		
		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("wep key4 must be set!\n");
			return -1;			
		}		
		if (profile.encryption == 1 && strlen(tmpbuf) != 10) {
			panic_printk("Invalid wep64 key4 value!\n");
			return -1;
		}
		if (profile.encryption == 2 && strlen(tmpbuf) != 26) {
			panic_printk("Invalid wep128 key4 value!\n");
			return -1;
		}		
		get_array_val(profile.wep_key4, tmpbuf, strlen(tmpbuf));		
	}
	else {
		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL) {
			panic_printk("wpa cipher must be set!\n");
			return -1;
		}		
		profile.wpa_cipher =  _atoi(tmpbuf, 10);

		ptr  = get_arg(ptr, tmpbuf);
		if (ptr == NULL || strlen(tmpbuf) == 0 || strlen(tmpbuf) < 8 || strlen(tmpbuf) > 64) {
			panic_printk("Invalid wpa psk!\n");
			return -1;
		}						
		strcpy(profile.wpa_psk, tmpbuf);	
	}
	
copy_profile:	
	memset(&priv->pmib->ap_profile.profile[priv->pmib->ap_profile.profile_num], '\0', sizeof(profile));	
	memcpy(&priv->pmib->ap_profile.profile[priv->pmib->ap_profile.profile_num], &profile, sizeof(profile));	
	priv->pmib->ap_profile.profile_num++;
	return 0;
}
#endif	// SUPPORT_MULTI_PROFILE


#if (!defined(CONFIG_RTL_COMAPI_CFGFILE) && !defined(CONFIG_RTL_COMAPI_WLTOOLS) && !defined(INCLUDE_WPS))
static
#endif
int set_mib(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct iwpriv_arg *entry;
	int int_val, int_idx, len, *int_ptr;
	int is_hex_type=0;
	unsigned char byte_val;
	char *arg_val, tmpbuf[100];
#ifdef 	CONFIG_RTK_MESH
	unsigned short word;
#endif
	DEBUG_TRACE;

	DEBUG_INFO("set_mib %s\n", data);

	entry = get_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid mib name [%s] !\n", data);
		return -1;
	}

	// search value
	arg_val = (char *)data;
	while (*arg_val && *arg_val != '='){
		arg_val++;
	}

	if (!*arg_val) {
		DEBUG_ERR("mib value empty [%s] !\n", data);
		return -1;
	}
	
	
	//printk("[%s %d] %c \n",__FUNCTION__,__LINE__ , *arg_val);
	arg_val++;
	
	// skip space
	while (*arg_val && *arg_val == 0x7f)
	arg_val++;

	if(*arg_val=='0' && (*(arg_val+1)== 'x' || *(arg_val+1)== 'X')){
		is_hex_type=1;
		arg_val+=2;
		printk("[%s %d]hex format\n",__FUNCTION__,__LINE__);
	}


	switch (entry->type) {
	case BYTE_T:
		byte_val = (unsigned char)_atoi(arg_val, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset, &byte_val,  1);
		break;
#ifdef 	CONFIG_RTK_MESH
	case WORD_T:
		word = (unsigned short)_atoi(arg_val, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset, &word,  2);
		break;
#endif
	case INT_T:
		if(is_hex_type)
			int_val = _atoi(arg_val, 16);
		else
		int_val = _atoi(arg_val, 10);
		
#ifdef WIFI_SIMPLE_CONFIG
		if (strcmp(entry->name, "wsc_enable") == 0) {
			if (int_val == 4) { // disable hidden AP
				if (HIDDEN_AP && priv->pbeacon_ssid) {
					memcpy(priv->pbeacon_ssid+2, SSID, SSID_LEN);
					priv->hidden_ap_mib_backup = HIDDEN_AP;
					HIDDEN_AP = 0;
				}
				break;
			}
			if (int_val == 5) { // restore hidden AP
				if (priv->pbeacon_ssid && !HIDDEN_AP && priv->hidden_ap_mib_backup) {
					memset(priv->pbeacon_ssid+2, '\0', SSID_LEN);
					HIDDEN_AP = priv->hidden_ap_mib_backup;
				}
				break;
			}
#ifdef CLIENT_MODE
			if ((priv->pmib->wscEntry.wsc_enable == 1) && (int_val == 0)) { 
				/*handle for WPS client mode fail or timeout*/ 

				if (priv->recover_join_req) {
					priv->recover_join_req = 0;
					priv->pmib->wscEntry.wsc_enable = 0;

					memcpy(&priv->pmib->dot11Bss, &priv->dot11Bss_original, sizeof(struct bss_desc));

					SSID_LEN = priv->orig_SSID_LEN ;	
					memset(SSID,'\0',sizeof(SSID));					
					memcpy(SSID , priv->orig_SSID , SSID_LEN);

					SSID2SCAN_LEN = priv->orig_SSID_LEN;
					memset(SSID2SCAN,'\0',sizeof(SSID2SCAN));
					memcpy(SSID2SCAN ,priv->orig_SSID , SSID2SCAN_LEN);															
					//memset(BSSID, 0, MACADDRLEN);
					rtl8192cd_close(priv->dev);
					rtl8192cd_open(priv->dev);
					break;
				}
			}
			else if ((priv->pmib->wscEntry.wsc_enable == 6) && (int_val == 0)) {
				/*handle for WPS client mode success;(don't do wlan driver close open)*/ 			

				priv->pmib->wscEntry.wsc_enable = 0;
				
			}
			else if ((priv->pmib->wscEntry.wsc_enable == 0) && (int_val == 1)){
				/*before start of client WPS backup some info for later restroe*/ 

				memcpy(&priv->dot11Bss_original, &priv->pmib->dot11Bss, sizeof(struct bss_desc));
				memset(priv->orig_SSID,'\0',sizeof(priv->orig_SSID));				
				memcpy(priv->orig_SSID , SSID , SSID_LEN);
				priv->orig_SSID_LEN = SSID_LEN;	
 				/*fixed for IOT issue */
				if((OPMODE&(WIFI_STATION_STATE | WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE))
						==(WIFI_STATION_STATE | WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE))
				{
					/*if client mode is associated set recover_join_req;
					then when wsc immediately be cancelled client will recover orig assoc*/ 
					priv->recover_join_req = 1;	
					
					issue_disassoc(priv, BSSID, _RSON_DEAUTH_STA_LEAVING_);					
					OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);					
				} 
 

				
			}

#endif
		}
#endif
		memcpy(((unsigned char *)priv->pmib)+entry->offset, (unsigned char *)&int_val, sizeof(int));
		break;

	case SSID_STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		memset(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, sizeof(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID));
		memcpy(priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, arg_val, strlen(arg_val));
		priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = strlen(arg_val);
		if ((SSID_LEN == 3) &&
			((SSID[0] == 'A') || (SSID[0] == 'a')) &&
			((SSID[1] == 'N') || (SSID[1] == 'n')) &&
			((SSID[2] == 'Y') || (SSID[2] == 'y'))) {
			SSID2SCAN_LEN = 0;
			memset(SSID2SCAN, 0, 32);
		}
		else {
			SSID2SCAN_LEN = SSID_LEN;
			memcpy(SSID2SCAN, SSID, SSID_LEN);
		}
		break;

	case BYTE_ARRAY_T:
		len = strlen(arg_val);
		if (len/2 > entry->len) {
			DEBUG_ERR("invalid len of BYTE_ARRAY_T mib [%s] !\n", entry->name);
			return -1;
		}
		if (len%2) {
			DEBUG_ERR("invalid len of BYTE_ARRAY_T mib [%s] !\n", entry->name);
			return -1;
		}
		get_array_val(((unsigned char *)priv->pmib)+entry->offset, arg_val, strlen(arg_val));
		break;

	case ACL_T:
	case ACL_INT_T:
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid ACL_T addr [%s] !\n", entry->name);
			return -1;
		}
		if (entry->type == ACL_T && strlen(tmpbuf)!=12) {
			DEBUG_ERR("invalid len of ACL_T mib [%s] !\n", entry->name);
			return -1;
		}
		int_ptr = (int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		int_idx = *int_ptr;
		if (entry->type == ACL_T)
			get_array_val(((unsigned char *)priv->pmib)+entry->offset+int_idx*6, tmpbuf, 12);
		else {
			get_array_val(((unsigned char *)priv->pmib)+entry->offset+int_idx*(6+4), tmpbuf, 12);
			if (strlen(arg_val) > 0) {
				int_val = _atoi(arg_val, 10);
				memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx*(6+4)+6, &int_val, 4);
			}
		}
		*int_ptr = *int_ptr + 1;
		break;

	case IDX_BYTE_ARRAY_T:
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY mib [%s] !\n", entry->name);
			return -1;
		}
		int_idx = _atoi(tmpbuf, 10);
		if (int_idx+1 > entry->len) {
			DEBUG_ERR("invalid BYTE_ARRAY mib index [%s, %d] !\n", entry->name, int_idx);
			return -1;
		}
		arg_val = get_arg(arg_val, tmpbuf);
		if (arg_val == NULL) {
			DEBUG_ERR("invalid BYTE_ARRAY mib [%s] !\n", entry->name);
			return -1;
		}
		byte_val = (unsigned char)_atoi(tmpbuf, 10);
		memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx, (unsigned char *)&byte_val, sizeof(byte_val));
		break;

	case MULTI_BYTE_T:
		int_idx=0;
		while (1) {
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			if (int_idx+1 > entry->len) {
				DEBUG_ERR("invalid MULTI_BYTE_T mib index [%s, %d] !\n", entry->name, int_idx);
				return -1;
			}
			byte_val = (unsigned char)_atoi(tmpbuf, 16);
			memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx++, (unsigned char *)&byte_val, sizeof(byte_val));
		}
		// copy length to next parameter
		memcpy( ((unsigned char *)priv->pmib)+entry->offset+entry->len, (unsigned char *)&int_idx, sizeof(int));
		break;

#ifdef _DEBUG_RTL8192CD_
	case DEBUG_T:
		int_val = _atoi(arg_val, 16);
		if (entry->offset==1)
			rtl8192cd_debug_err = int_val;
		else if (entry->offset==2)
			rtl8192cd_debug_info = int_val;
		else if (entry->offset==3)
			rtl8192cd_debug_trace = int_val;
		else if (entry->offset==4)
			rtl8192cd_debug_warn = int_val;
		else {
			DEBUG_ERR("invalid debug index\n");
		}
		break;
#endif // _DEBUG_RTL8192CD_

	case DEF_SSID_STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		memset(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, 0, sizeof(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID));
		memcpy(priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, arg_val, strlen(arg_val));
		priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen = strlen(arg_val);
		break;
#ifdef CONFIG_RTL_COMAPI_CFGFILE
	case SSID2SCAN_STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		memset(priv->pmib->dot11StationConfigEntry.dot11SSIDtoScan, 0, sizeof(priv->pmib->dot11StationConfigEntry.dot11SSIDtoScan));
		memcpy(priv->pmib->dot11StationConfigEntry.dot11SSIDtoScan, arg_val, strlen(arg_val));
		priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen = strlen(arg_val);
		break;
#endif

	case STRING_T:
		if (strlen(arg_val) > entry->len)
			arg_val[entry->len] = '\0';
		strcpy(((unsigned char *)priv->pmib)+entry->offset, arg_val);
		break;

	case RFFT_T:
		if (entry->len == 1) {
			byte_val = _atoi(arg_val, 10);
			memcpy(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, (unsigned char *)&byte_val, entry->len);
#ifdef DFS
			if ((strcmp(entry->name, "dfsdbgmode") == 0) && (byte_val)){
				if (priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_ETSI) {
					if (GET_CHIP_VER(priv) == VERSION_8192D) {
					PHY_SetBBReg(priv, 0x90c, bMaskDWord, 0x83321333);
					PHY_SetBBReg(priv, 0xe10, bMaskDWord, 0x30303030);
					PHY_SetBBReg(priv, 0x83c, bMaskDWord, 0x30303030);
				}
					else if (GET_CHIP_VER(priv) == VERSION_8812E)
						PHY_SetBBReg(priv, 0x80c, BIT(28), 1);
				}
				if (GET_CHIP_VER(priv) == VERSION_8192D) {
				PHY_SetBBReg(priv, 0xc7c, BIT(28), 0); // ynlin dbg
				PHY_SetBBReg(priv, 0xcdc, BIT(8)|BIT(9), 1);
			}
			}
			else if (strcmp(entry->name, "det_off") == 0) {
				if (timer_pending(&priv->dfs_det_chk_timer))
					del_timer_sync(&priv->dfs_det_chk_timer);
				if (priv->pshare->rf_ft_var.dfs_det_off == 0) {
					init_timer(&priv->dfs_det_chk_timer);
					priv->dfs_det_chk_timer.data = (unsigned long) priv;
					priv->dfs_det_chk_timer.function = rtl8192cd_dfs_det_chk_timer;
					mod_timer(&priv->dfs_det_chk_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pshare->rf_ft_var.dfs_det_period*10));
				}
			}
			else if ((strcmp(entry->name, "det_reset") == 0) && (byte_val)) {
				priv->pmib->dot11DFSEntry.DFS_detected = 0;
				priv->FA_count_pre = 0;
				priv->VHT_CRC_ok_cnt_pre = 0;
				priv->HT_CRC_ok_cnt_pre = 0;
				priv->LEG_CRC_ok_cnt_pre = 0;
				priv->mask_idx = 0;
				priv->mask_hist_checked = 0;
				memset(priv->radar_det_mask_hist, 0, sizeof(priv->radar_det_mask_hist));
				memset(priv->pulse_flag_hist, 0, sizeof(priv->pulse_flag_hist));
			}
#endif
		} else if (entry->len == 4) {
			int_val = _atoi(arg_val, 10);
			memcpy(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, (unsigned char *)&int_val, entry->len);
		}
		break;

	case RFFT_ACL_T:
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL) {
				DEBUG_ERR("invalid RFFT_ACL_T addr [%s] !\n", entry->name);
				return -1;
			}
			if (strlen(tmpbuf)!=12) {
				DEBUG_ERR("invalid len of RFFT_ACL_T mib [%s] !\n", entry->name);
				return -1;
			}
			
			int_ptr = (int *)(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset+entry->len);
			int_idx = *int_ptr;
			get_array_val(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset+int_idx*6, tmpbuf, 12);
			*int_ptr = *int_ptr + 1;
			break;

	case VARLEN_BYTE_T:
		len = strlen(arg_val);
		if (len/2 > entry->len) {
			DEBUG_ERR("invalid len of VARLEN_BYTE_T mib [%s] !\n", entry->name);
			return -1;
		}
		if (len%2) {
			DEBUG_ERR("invalid len of VARLEN_BYTE_T mib [%s] !\n", entry->name);
			return -1;
		}
		memset(((unsigned char *)priv->pmib)+entry->offset, 0, entry->len);
		len = get_array_val(((unsigned char *)priv->pmib)+entry->offset, arg_val, strlen(arg_val));
		*(unsigned int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len) = len;
		break;

#ifdef WIFI_SIMPLE_CONFIG
	case PIN_IND_T:
		if (strlen(arg_val) > entry->len) {

#ifdef INCLUDE_WPS
			//include-wps case
			//upnp will direct function call to wps_pin(arg_val);
			//printk("set_mib:PIN=%s\n",arg_val);
			wps_pin(arg_val);
#else
			DOT11_WSC_PIN_IND wsc_ind;

			wsc_ind.EventId = DOT11_EVENT_WSC_PIN_IND;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code, arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
#endif
		}
		break;
/* WPS2DOTX   */
	/* support  Assigned MAC Addr,Assigned SSID,dymanic change STA's PIN code, 2011-0505 */	
	case WSC_SELF_PIN_IND_T:
		if (strlen(arg_val) > entry->len) {
			DOT11_WSC_PIN_IND wsc_ind;
			wsc_ind.EventId = DOT11_EVENT_WSC_SET_MY_PIN;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code, arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
		}
		break;
	case WSC_SEPC_SSID_CONN_IND_T:
		if (strlen(arg_val) > entry->len) {
			DOT11_WSC_PIN_IND wsc_ind;

			wsc_ind.EventId = DOT11_EVENT_WSC_SPEC_SSID;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code, arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
		}
		break;
	case WSC_SEPC_MAC_CONN_IND_T:
		if (strlen(arg_val) > entry->len) {
			DOT11_WSC_PIN_IND wsc_ind;
			wsc_ind.EventId = DOT11_EVENT_WSC_SPEC_MAC_IND;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code, arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
		}
		break;
	/* support  Assigned MAC Addr,Assigned SSID,dymanic change STA's PIN code, 2011-0505 */			
/* WPS2DOTX   */	
#ifdef INCLUDE_WPS
#ifndef CONFIG_MSC
	case WSC_IND_T:
		/*event notify user space upnp ,call by wps "*/
	if (strcmp(entry->name, "wps_get_config") == 0)  {
			printk("sme rx wps_get_config cmd\n");
			DOT11_WSC_PIN_IND wsc_ind;
			wsc_ind.EventId = DOT11_EVENT_WSC_GETCONF_IND;
			wsc_ind.IsMoreEvent = 0;
			strcpy(wsc_ind.code,arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
			event_indicate(priv, NULL, -1);
		}
		else if (strcmp(entry->name, "wsc_soap_action") == 0)  {
			DOT11_WSC_SOAP soap;
	        	struct iw_point wrq;

        		wrq.pointer = (caddr_t)&soap;
        		wrq.length = sizeof(DOT11_WSC_SOAP);

	       		soap.EventId = DOT11_EVENT_WSC_SOAP;
        		soap.IsMoreEvent = FALSE;
	        	strcpy(soap.action, arg_val);
				printk("ioctl soap name:%s\n",arg_val);
	        	//rtl8192cd_ioctl_priv_daemonreq(priv->dev, &wrq);
		}

		else if (strcmp(entry->name, "wsc_event_callback") == 0)  {
			DOT11_WSC_PIN_IND wsc_ind;

			wsc_ind.EventId = DOT11_EVENT_WSC_GETCONF_IND;
			wsc_ind.IsMoreEvent = 0;
            DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
            event_indicate(priv, NULL, -1);

        }
		else if (strcmp(entry->name, "wps_led_control") == 0) {	/*event to upnp*/

            DOT11_WPS_LEDCRTL wsc_ind;
                        short flag = 0;

                        wsc_ind.EventId = DOT11_EVENT_WSC_LEDCONTROL_IND;

                        if(strcmp("WSC_START",arg_val) == 0)
                            flag = -1;
                        else if(strcmp("WSC_END",arg_val) == 0)
                            flag = -2;
                        else if(strcmp("PBC_OVERLAPPED",arg_val) == 0)
                            flag = -3;
                        else if(strcmp("WSC_ERROR",arg_val) == 0)
                            flag = -4;
                        else if(strcmp("WSC_SUCCESS",arg_val) == 0)
                            flag = -5;
                        else if(strcmp("WSC_NOP",arg_val) == 0)
                            flag = -6;
                        else
                            flag = -7;

            wsc_ind.flag = flag;
            DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WPS_LEDCRTL));
            event_indicate(priv, NULL, -1);

        }
		else if (strcmp(entry->name, "wps_debug") == 0) {	/*event to wps*/
	     	priv->pshare->WSC_CONT_S.debug = _atoi(arg_val,10);
  	    }
		else if (strcmp(entry->name, "wps_reinit") == 0) {	/*event to wps*/
			printk("WPS module reinit from set_mib\n");
	     	priv->pshare->WSC_CONT_S.wait_reinit = 1 ;
			wps_init(priv);
  	    }
        break;

	// from wps call event to user space upnp
	case FLASH_RESTORE_T:
		if (strlen(arg_val) > entry->len) {
			DOT11_WSC_RESTORE2FLASH_IND	restore2flash;
			memset(&restore2flash , 0 ,sizeof(struct _DOT11_WSC_RESTORE2FLASH_IND));
			restore2flash.EventId = DOT11_EVENT_WSC_PUTCONF_IND;

			strcpy(restore2flash.flashcmd[0], arg_val);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&restore2flash,
				sizeof(DOT11_WSC_RESTORE2FLASH_IND));
			event_indicate(priv, NULL, -1);
		}
		break;

#endif	// end of CONFIG_MSC
#endif	// end of INCLUDE_WPS
#ifdef CONFIG_RTL_COMAPI_CFGFILE
        case WSC_START_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_START_IND;
                        wsc_ind.IsMoreEvent = 0;
                        //wsc_ind.value = arg_val;
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
        //EV_MODE, EV_STATUS, EV_MEHOD, EV_STEP, EV_OOB
        case WSC_MODE_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_MODE_IND;
                        wsc_ind.IsMoreEvent = 0;
                        wsc_ind.value = _atoi(arg_val,10);
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
        case WSC_STATUS_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_STATUS_IND;
                        wsc_ind.IsMoreEvent = 0;
                        wsc_ind.value = _atoi(arg_val,10);
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
        case WSC_METHOD_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_METHOD_IND;
                        wsc_ind.IsMoreEvent = 0;
                        wsc_ind.value = _atoi(arg_val,10);
                        if( wsc_ind.value > 3 || wsc_ind.value ==0 )
                                wsc_ind.value = 1;      //default set to pin method
                        printk("iwpriv set method = %d\n",wsc_ind.value);
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
        case WSC_STEP_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_STEP_IND;
                        wsc_ind.IsMoreEvent = 0;
                        //wsc_ind.value = arg_val;
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
        case WSC_OOB_IND_T:
                if( strlen(arg_val) > 0 ) {
                        DOT11_WSC_IND wsc_ind;
                        wsc_ind.EventId = DOT11_EVENT_WSC_OOB_IND;
                        wsc_ind.IsMoreEvent = 0;
                        //wsc_ind.value = arg_val;
                        DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_IND));
                        event_indicate(priv, NULL, -1);
                }
                break;
#endif  //ifdef CONFIG_RTL_COMAPI_CFGFILE
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case INT_ARRAY_T:
		int_idx=0;
		while (1) {
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			if (int_idx+1 > (entry->len)/sizeof(int)) {
				DEBUG_ERR("invalid MULTI_BYTE_T mib index [%s, %d] !\n", entry->name, int_idx);
				return -1;
			}
			int_val = _atoi(tmpbuf, 16);
			memcpy(((unsigned char *)priv->pmib)+entry->offset+int_idx++, (void *)&int_val, sizeof(int_val));
		}
		break;
	case WAPI_KEY_T:
		{
			char tmppasswd[100]={0};
			wapiMibPSK *wapipsk=NULL;
			int_idx=0;

			/*Get Password*/
			arg_val = get_arg(arg_val, tmpbuf);
			if (arg_val == NULL)
				break;
			memcpy(tmppasswd, tmpbuf, strlen(tmpbuf));

			/*Get Password length*/
			arg_val=get_arg(arg_val, tmpbuf);
			int_val = _atoi(tmpbuf, 16);

			wapipsk=(wapiMibPSK *)((unsigned char *)(priv->pmib)+entry->offset);

			/*Hex or passthru*/
			if((0==(strlen(tmppasswd) % 2))  && (int_val < strlen(tmppasswd)) &&
				(int_val == (strlen(tmppasswd)/2)))
			{
				/*Hex mode*/
				string_to_hex(tmppasswd,wapipsk->octet,strlen(tmppasswd));
			}
			else
			{
				strncpy(wapipsk->octet,tmppasswd,strlen(tmppasswd));
			}
			wapipsk->len = int_val;
			break;
	      }
#endif

#ifdef SUPPORT_MULTI_PROFILE
	case AP_PROFILE_T:
		add_ap_profile(priv, arg_val);
		break;
#endif

	default:
		DEBUG_ERR("invalid mib type!\n");
		break;
	}

	return 0;
}
#ifndef INCLUDE_WPS
static
#endif
int get_mib(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct iwpriv_arg *entry;
	int i, len, *int_ptr, copy_len;
	char tmpbuf[40];

	DEBUG_TRACE;

	DEBUG_INFO("get_mib %s\n", data);

	entry = get_tbl_entry((char *)data);
	if (entry == NULL) {
		DEBUG_ERR("invalid mib name [%s] !\n", data);
		return -1;
	}
	copy_len = entry->len;

	switch (entry->type) {
	case BYTE_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset,  1);
		PRINT_INFO("byte data: %d\n", *data);
		break;
#ifdef 	CONFIG_RTK_MESH
	case WORD_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset,  2);
		PRINT_INFO("word data: %d\n", *data);
		break;
#endif
	case INT_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, sizeof(int));
		PRINT_INFO("int data: %d\n", *((int *)data));
		break;

	case SSID_STRING_T:
		memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen);
		tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen] = '\0';
		strcpy((char *)data, tmpbuf);
		PRINT_INFO("ssid: %s\n", tmpbuf);
		break;

	case BYTE_ARRAY_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, entry->len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<entry->len; i++)
			PRINT_INFO("%02x", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	case ACL_T:
		int_ptr = (int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		PRINT_INFO("ACL table (%d):\n", *int_ptr);
		copy_len = 0;
		for (i=0; i<*int_ptr; i++) {
			memcpy(data, ((unsigned char *)priv->pmib)+entry->offset+i*6, 6);
			PRINT_INFO("mac-addr: %02x-%02x-%02x-%02x-%02x-%02x\n",
				data[0],data[1],data[2],data[3],data[4],data[5]);
			data += 6;
			copy_len += 6;
		}
		DEBUG_INFO("\n");
		break;

	case IDX_BYTE_ARRAY_T:
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, entry->len);
		PRINT_INFO("data (dec): ");
		for (i=0; i<entry->len; i++)
			PRINT_INFO("%d ", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	case MULTI_BYTE_T:
		memcpy(&len, ((unsigned char *)priv->pmib)+entry->offset+entry->len, sizeof(int));
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<len; i++)
			PRINT_INFO("%02x ", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

#ifdef _DEBUG_RTL8192CD_
	case DEBUG_T:
		if (entry->offset==1)
			memcpy(data, (unsigned char *)&rtl8192cd_debug_err, sizeof(rtl8192cd_debug_err));
		else if (entry->offset==2)
			memcpy(data, (unsigned char *)&rtl8192cd_debug_info, sizeof(rtl8192cd_debug_info));
		else if (entry->offset==3)
			memcpy(data, (unsigned char *)&rtl8192cd_debug_trace, sizeof(rtl8192cd_debug_trace));
		else if (entry->offset==4)
			memcpy(data, (unsigned char *)&rtl8192cd_debug_warn, sizeof(rtl8192cd_debug_warn));
		else {
			DEBUG_ERR("invalid debug index\n");
		}
		PRINT_INFO("debug flag(hex): %08lx\n", *((unsigned long *)data));
		break;
#endif // _DEBUG_RTL8192CD_

	case DEF_SSID_STRING_T:
		memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen);
		tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen] = '\0';
		strcpy((char *)data, tmpbuf);
		PRINT_INFO("defssid: %s\n", tmpbuf);
		break;

	case STRING_T:
		strcpy((char *)data, (char *)(((unsigned char *)priv->pmib)+entry->offset));
		PRINT_INFO("string data: %s\n", data);
		break;

	case RFFT_T:
		memcpy(data, ((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset, sizeof(int));
		PRINT_INFO("int data: %d\n", *((int *)data));
		break;
	case RFFT_ACL_T:
		int_ptr = (int *)(((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset+entry->len);
		copy_len = 0;
		for (i=0; i<*int_ptr; i++) 
		{
			memcpy(data, ((unsigned char *)&priv->pshare->rf_ft_var)+entry->offset+i*6, 6);
			PRINT_INFO("mac-addr: %02x-%02x-%02x-%02x-%02x-%02x\n",
				data[0],data[1],data[2],data[3],data[4],data[5]);
			data += 6;
			copy_len += 6;
		}
		DEBUG_INFO("\n");
		break;

	case VARLEN_BYTE_T:
		copy_len = *(unsigned int *)(((unsigned char *)priv->pmib)+entry->offset+entry->len);
		memcpy(data, ((unsigned char *)priv->pmib)+entry->offset, copy_len);
		PRINT_INFO("data (hex): ");
		for (i=0; i<copy_len; i++)
			PRINT_INFO("%02x", *((unsigned char *)((unsigned char *)priv->pmib)+entry->offset+i));
		PRINT_INFO("\n");
		break;

	default:
		DEBUG_ERR("invalid mib type!\n");
		return 0;
	}

	return copy_len;
}


#ifdef _IOCTL_DEBUG_CMD_
/*
 * Write register, command: "iwpriv wlanX write_reg,type,offset,value"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw" (based on wlan register offset)
 *			    "_b" - byte, "_w" - word, "_dw" - "dw" (based on register offset 0)
 *		offset and value should be input in hex
 */
static int write_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char name[100];
	int i=0, op=0, offset;
	unsigned long ioaddr, val;

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';

	if (!strcmp((char *)name, "b"))
		op = 1;
	else if (!strcmp((char *)name, "w"))
		op = 2;
	else if (!strcmp((char *)name, "dw"))
		op = 3;
	else if (!strcmp((char *)name, "_b"))
		op = 0x81;
	else if (!strcmp((char *)name, "_w"))
		op = 0x82;
	else if (!strcmp((char *)name, "_dw"))
		op = 0x83;

	if (op == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		return -1;
	}

	if ( !(op&0x80))  // wlan register
		ioaddr = priv->pshare->ioaddr;
	else
		ioaddr = 0;

	// get offset and value
	i=0;
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';
	if (!*data++) {
		DEBUG_ERR("invalid offset!\n");
		return -1;
	}
	offset = _atoi((char *)name, 16);
	val = (unsigned long)_atoi((char *)data, 16);

	DEBUG_INFO("write reg in %s: addr=%08x, val=0x%x\n",
			(op == 1 ? "byte" : (op == 2 ? "word" : "dword")),
			offset, (int)val);

	switch (op&0x7f) {
	case 1:
		RTL_W8(offset, ((unsigned char)val));
		break;
	case 2:
		RTL_W16(offset, ((unsigned short)val));
		break;
	case 3:
		RTL_W32(offset, ((unsigned long)val));
		break;
	}
	return 0;
}


/*
 * Read register, command: "iwpriv wlanX read_reg,type,offset"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw" (based on wlan register offset)
 *			    "_b" - byte, "_w" - word, "_dw" - "dw" (based on register offset 0)
 *		offset should be input in hex
 */
static int read_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char name[100];
	int i=0, op=0, offset, len=0;
	unsigned long ioaddr, dw_val;
	unsigned char *org_ptr=data, b_val;
	unsigned short w_val;

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		name[i++] = *data++;
	name[i] = '\0';

	if (!strcmp((char *)name, "b"))
		op = 1;
	else if (!strcmp((char *)name, "w"))
		op = 2;
	else if (!strcmp((char *)name, "dw"))
		op = 3;
	else if (!strcmp((char *)name, "_b"))
		op = 0x81;
	else if (!strcmp((char *)name, "_w"))
		op = 0x82;
	else if (!strcmp((char *)name, "_dw"))
		op = 0x83;

	if (op == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		return -1;
	}

	if ( !(op&0x80))  // wlan register
		ioaddr = priv->pshare->ioaddr;
	else
		ioaddr = 0;

	// get offset
	offset = _atoi((char *)data, 16);

	switch (op&0x7f) {
	case 1:
		b_val = (unsigned char)RTL_R8(offset);
		panic_printk("read byte reg %x=0x%02x\n", offset, b_val);
		len = 1;
		memcpy(org_ptr, &b_val, len);
		break;
	case 2:
		w_val = (unsigned short)RTL_R16(offset);
		panic_printk("read word reg %x=0x%04x\n", offset, w_val);
		len = 2;
		memcpy(org_ptr, (char *)&w_val, len);
		break;
	case 3:
		dw_val = (unsigned long)RTL_R32(offset);
		panic_printk("read dword reg %x=0x%08lx\n", offset, dw_val);
		len = 4;
		memcpy(org_ptr, (char *)&dw_val, len);
		break;
	}

	return len;
}


/*
 * Write memory, command: "iwpriv wlanX write_mem,type,start,len,value"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw"
 *		start, len and value should be input in hex
 */
static int write_mem(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char tmpbuf[100];
	int i=0, size=0, len;
	unsigned long val, start;

	DEBUG_TRACE;

	// get access type
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';

	if (!strcmp((char *)tmpbuf, "b"))
		size = 1;
	else if (!strcmp((char *)tmpbuf, "w"))
		size = 2;
	else if (!strcmp((char *)tmpbuf, "dw"))
		size = 4;

	if (size == 0 || !*data++) {
		DEBUG_ERR("invalid command!\n");
		return -1;
	}

	// get start, len, and value
	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid start!\n");
		return -1;
	}
	start = (unsigned long)_atoi((char *)tmpbuf, 16);

	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid len!\n");
		return -1;
	}
	len = _atoi((char *)tmpbuf, 16);
	val = (unsigned long)_atoi((char *)data, 16);

	DEBUG_INFO("write memory: start=%08lx, len=%x, data=0x%x (%s)\n",
		start,	len, (int)val,
		(size == 1 ? "byte" : (size == 2 ? "word" : "dword")));

	for (i=0; i<len; i++) {
		memcpy((char *)start, (char *)&val, size);
		start += size;
	}
	return 0;
}


/*
 * Read memory, command: "iwpriv wlanX read_mem,type,start,len"
 * 	where: type may be: "b" - byte, "w" - word, "dw" - "dw"
 *		start, and len should be input in hex
 */
static int read_mem(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char tmpbuf[100];
//#ifndef CONFIG_RTL8186_TR	 //brad add for tr 11n
#if !(defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL8196C_KLD))
//#if !defined(__LINUX_2_6__) || defined(CONFIG_PANIC_PRINTK)
//#ifdef _DEBUG_RTL8192CD_
	char *tmp1;
//#endif
#endif// !define CONFIG_RTL8186_TR
	int i=0, size=0, len, copy_len;
	unsigned long start, dw_val;
	unsigned short w_val;
	unsigned char b_val, *pVal=NULL, *org_ptr=data;

	DEBUG_TRACE;

	tmp1 = (char *)kmalloc(2048, GFP_ATOMIC);
	if (tmp1 == NULL) {
		panic_printk("Not enough memory\n");
		return -1;
	}

	// get access type
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';

	if (!strcmp((char *)tmpbuf, "b")) {
		size = 1;
		pVal = &b_val;
	}
	else if (!strcmp((char *)tmpbuf, "w")) {
		size = 2;
		pVal = (unsigned char *)&w_val;
	}
	else if (!strcmp((char *)tmpbuf, "dw")) {
		size = 4;
		pVal = (unsigned char *)&dw_val;
	}

	if (size == 0 || !*data++) {
		DEBUG_ERR("invalid type!\n");
		kfree(tmp1);
		return -1;
	}

	// get start and len
	i=0;
	while (*data && *data != ',')
		tmpbuf[i++] = *data++;
	tmpbuf[i] = '\0';
	if (i==0 || !*data++) {
		DEBUG_ERR("invalid start!\n");
		kfree(tmp1);
		return -1;
	}
	start = (unsigned long)_atoi((char *)tmpbuf, 16);
	len = _atoi((char *)data, 16);
#if !(defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL8196C_KLD))
//#if !defined(__LINUX_2_6__) || defined(CONFIG_PANIC_PRINTK)
//#ifdef _DEBUG_RTL8192CD_
	sprintf(tmp1, "read memory: from=%lx, len=0x%x (%s)\n",
		start, len, (size == 1 ? "byte" : (size == 2 ? "word" : "dword")));

	for (i=0; i<len; i++) {
		char tmp2[10];
		memcpy(pVal, (char *)start+i*size, size);
		if (size == 1) {
			sprintf(tmp2, "%02x ", b_val);
			if ((i>0) && ((i%16)==0))
				strcat(tmp1, "\n");
		}
		else if (size == 2) {
			sprintf(tmp2, "%04x ", w_val);
			if ((i>0) && ((i%8)==0))
				strcat(tmp1, "\n");
		}
		else if (size == 4) {
			sprintf(tmp2, "%08lx ", dw_val);
			if ((i>0) && ((i%8)==0))
				strcat(tmp1, "\n");
		}
		strcat(tmp1, tmp2);
	}
	strcat(tmp1, "\n");

	panic_printk("%s", tmp1);
//#endif // _DEBUG_RTL8192CD_
#endif // !define CONFIG_RTL8186_TR
	if (size*len > 128)
		copy_len = 128;
	else
		copy_len = size*len;
	memcpy(org_ptr,  (char *)start, copy_len);

	kfree(tmp1);
	return copy_len;
}


static int write_bb_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	return 0;
}


static int read_bb_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	return 0;
}


static int write_rf_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char tmpbuf[32];
	unsigned int path, offset, val, val_read;
	int i;

	DEBUG_TRACE;

	if (strlen((char *)data) != 0) {
		i = 0;
		while (*data && *data != ',')
			tmpbuf[i++] = *data++;
		tmpbuf[i] = '\0';
		if (i==0 || !*data++) {
			DEBUG_ERR("invalid path!\n");
			return -1;
		}
		path = _atoi((char *)tmpbuf, 16);

		i = 0;
		while (*data && *data != ',')
			tmpbuf[i++] = *data++;
		tmpbuf[i] = '\0';
		if (i==0 || !*data++) {
			DEBUG_ERR("invalid offset!\n");
			return -1;
		}
		offset = _atoi((char *)tmpbuf, 16);

		val = (unsigned long)_atoi((char *)data, 16);

		PHY_SetRFReg(priv, path, offset, bMask20Bits, val);
		val_read = PHY_QueryRFReg(priv, path, offset, bMask20Bits, 1);
		printk("write RF %d offset 0x%02x val [0x%05x],  read back [0x%05x]\n",
			path, offset, val&0xfffff, val_read&0xfffff);
	}

	return 0;
}


static int read_rf_reg(struct rtl8192cd_priv *priv, unsigned char *data)
{
	volatile char tmpbuf[32];
	unsigned char *arg = data;
	unsigned int path, offset, val;
	int i;

	DEBUG_TRACE;

	if (strlen((char *)arg) != 0) {
		i = 0;
		while (*arg && *arg != ',')
			tmpbuf[i++] = *arg++;
		tmpbuf[i] = '\0';
		if (i==0 || !*arg++) {
			DEBUG_ERR("invalid path!\n");
			return -1;
		}
		path = _atoi((char *)tmpbuf, 16);

		offset = (unsigned char)_atoi((char *)arg, 16);
		val = PHY_QueryRFReg(priv, path, offset, bMask20Bits, 1);
		panic_printk("read RF %d reg %02x=0x%08x\n", path, offset, val);
		memcpy(data, (char *)&val, 4);
		return 4;
	}
	return 1;
}


#ifdef CONFIG_RTL8186_KB
int get_guestmac(struct rtl8192cd_priv *priv, GUESTMAC_T *macdata)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid)
		{
			memcpy(macdata->macaddr, priv->guestMac[i].macaddr, 6);
			macdata->valid = priv->guestMac[i].valid;
		}
		else
			break;
		macdata++;
	}
	return sizeof(GUESTMAC_T)*i;
}


int set_guestmacvalid(struct rtl8192cd_priv *priv, char *buf)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid)
		{
			continue;
		}
		memcpy(priv->guestMac[i].macaddr, buf, 6);
		priv->guestMac[i].valid = 1;
		return 0;
	}
	/*No slot avaible*/
	return -1;
}


int set_guestmacinvalid(struct rtl8192cd_priv *priv, char *buf)
{
	int i=0;

	for (i=0; i<MAX_GUEST_NUM; i++)
	{
		if (priv->guestMac[i].valid && !memcmp(priv->guestMac[i].macaddr, buf, 6))
		{
			priv->guestMac[i].valid = 0;
			return 0;
		}
	}
	/*No such slot*/
	return -1;
}
#endif // CONFIG_RTL8186_KB


#ifdef _DEBUG_RTL8192CD_

//_TXPWR_REDEFINE

#define POWER_MIN_CHECK(a,b)            (((a) > (b)) ? (b) : (a))
#define POWER_RANGE_CHECK(val)		(((val) > 0x3f)? 0x3f : ((val < 0) ? 0 : val))
#define COUNT_SIGN_OFFSET(val, oft)	(((oft & 0x08) == 0x08)? (val - (0x10 - oft)) : (val + oft))

#define ASSIGN_TX_POWER_OFFSET(offset, setting) { \
	if (setting != 0x7f) \
		offset = setting; \
}


static int ch2idx(int ch)
{
	int val=-1;
	// |1~14|36, 38, 40, ..., 64|100, 102, ..., 140|149, 151, ..., 165|
	if (ch<=14)
		val = ch-1;
	else if (ch<=64)
		val = ((ch-36)>>1)+14;
	else if (ch<=140)
		val = ((ch-100)>>1)+29;
	else if (ch<=165)
		val = ((ch-149)>>1)+50;

	return val;
}



#ifdef ADD_TX_POWER_BY_CMD
static void check_txpwr_by_cmd(struct rtl8192cd_priv *priv, 
	char *MCSTxAgcOffset_A, char *MCSTxAgcOffset_B,
	char *OFDMTxAgcOffset_A, char *OFDMTxAgcOffset_B,
	char *CCKTxAgc_A, char *CCKTxAgc_B)
{
//	char is_by_cmd = 0;	

	if( (priv->pshare->rf_ft_var.txPowerPlus_cck_11 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_cck_5 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_cck_2 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_cck_1 != 0x7f))
	{		
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_A[0], priv->pshare->rf_ft_var.txPowerPlus_cck_11);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_A[1], priv->pshare->rf_ft_var.txPowerPlus_cck_5);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_A[2], priv->pshare->rf_ft_var.txPowerPlus_cck_2);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_A[3], priv->pshare->rf_ft_var.txPowerPlus_cck_1);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_B[0], priv->pshare->rf_ft_var.txPowerPlus_cck_11);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_B[1], priv->pshare->rf_ft_var.txPowerPlus_cck_5);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_B[2], priv->pshare->rf_ft_var.txPowerPlus_cck_2);
		ASSIGN_TX_POWER_OFFSET(CCKTxAgc_B[3], priv->pshare->rf_ft_var.txPowerPlus_cck_1);

		printk("TXPWR_BY_CMD: CCK = %02x %02x %02x %02x \n", 
			priv->pshare->rf_ft_var.txPowerPlus_cck_11,
			priv->pshare->rf_ft_var.txPowerPlus_cck_5,
			priv->pshare->rf_ft_var.txPowerPlus_cck_2,
			priv->pshare->rf_ft_var.txPowerPlus_cck_1);
	}


	if( (priv->pshare->rf_ft_var.txPowerPlus_ofdm_18 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_12!= 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_9 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_6 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_54 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_48 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_36 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_ofdm_24 != 0x7f))
	{
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[0], priv->pshare->rf_ft_var.txPowerPlus_ofdm_18);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[1], priv->pshare->rf_ft_var.txPowerPlus_ofdm_12);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[2], priv->pshare->rf_ft_var.txPowerPlus_ofdm_9);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[3], priv->pshare->rf_ft_var.txPowerPlus_ofdm_6);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[0], priv->pshare->rf_ft_var.txPowerPlus_ofdm_18);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[1], priv->pshare->rf_ft_var.txPowerPlus_ofdm_12);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[2], priv->pshare->rf_ft_var.txPowerPlus_ofdm_9);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[3], priv->pshare->rf_ft_var.txPowerPlus_ofdm_6);

		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[4], priv->pshare->rf_ft_var.txPowerPlus_ofdm_54);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[5], priv->pshare->rf_ft_var.txPowerPlus_ofdm_48);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[6], priv->pshare->rf_ft_var.txPowerPlus_ofdm_36);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_A[7], priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[4], priv->pshare->rf_ft_var.txPowerPlus_ofdm_54);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[5], priv->pshare->rf_ft_var.txPowerPlus_ofdm_48);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[6], priv->pshare->rf_ft_var.txPowerPlus_ofdm_36);
		ASSIGN_TX_POWER_OFFSET(OFDMTxAgcOffset_B[7], priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);

		printk("TXPWR_BY_CMD: OFDM = %02x %02x %02x %02x %02x %02x %02x %02x \n", 
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_18,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_12,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_9,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_6, 
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_54,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_48,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_36,
			priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);
	}


	if( (priv->pshare->rf_ft_var.txPowerPlus_mcs_3 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_2 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_1 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_0 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_7 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_6 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_5 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_4 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_11 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_10 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_9 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_8 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_15 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_14 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_13 != 0x7f)
		|| (priv->pshare->rf_ft_var.txPowerPlus_mcs_12 != 0x7f))
	{
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[0], priv->pshare->rf_ft_var.txPowerPlus_mcs_3);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[1], priv->pshare->rf_ft_var.txPowerPlus_mcs_2);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[2], priv->pshare->rf_ft_var.txPowerPlus_mcs_1);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[3], priv->pshare->rf_ft_var.txPowerPlus_mcs_0);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[0], priv->pshare->rf_ft_var.txPowerPlus_mcs_3);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[1], priv->pshare->rf_ft_var.txPowerPlus_mcs_2);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[2], priv->pshare->rf_ft_var.txPowerPlus_mcs_1);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[3], priv->pshare->rf_ft_var.txPowerPlus_mcs_0);

		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[4], priv->pshare->rf_ft_var.txPowerPlus_mcs_7);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[5], priv->pshare->rf_ft_var.txPowerPlus_mcs_6);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[6], priv->pshare->rf_ft_var.txPowerPlus_mcs_5);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[7], priv->pshare->rf_ft_var.txPowerPlus_mcs_4);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[4], priv->pshare->rf_ft_var.txPowerPlus_mcs_7);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[5], priv->pshare->rf_ft_var.txPowerPlus_mcs_6);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[6], priv->pshare->rf_ft_var.txPowerPlus_mcs_5);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[7], priv->pshare->rf_ft_var.txPowerPlus_mcs_4);

		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[8], priv->pshare->rf_ft_var.txPowerPlus_mcs_11);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[9], priv->pshare->rf_ft_var.txPowerPlus_mcs_10);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[10], priv->pshare->rf_ft_var.txPowerPlus_mcs_9);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[11], priv->pshare->rf_ft_var.txPowerPlus_mcs_8);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[8], priv->pshare->rf_ft_var.txPowerPlus_mcs_11);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[9], priv->pshare->rf_ft_var.txPowerPlus_mcs_10);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[10], priv->pshare->rf_ft_var.txPowerPlus_mcs_9);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[11], priv->pshare->rf_ft_var.txPowerPlus_mcs_8);

		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[12], priv->pshare->rf_ft_var.txPowerPlus_mcs_15);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[13], priv->pshare->rf_ft_var.txPowerPlus_mcs_14);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[14], priv->pshare->rf_ft_var.txPowerPlus_mcs_13);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_A[15], priv->pshare->rf_ft_var.txPowerPlus_mcs_12);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[12], priv->pshare->rf_ft_var.txPowerPlus_mcs_15);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[13], priv->pshare->rf_ft_var.txPowerPlus_mcs_14);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[14], priv->pshare->rf_ft_var.txPowerPlus_mcs_13);
		ASSIGN_TX_POWER_OFFSET(MCSTxAgcOffset_B[15], priv->pshare->rf_ft_var.txPowerPlus_mcs_12);

		printk("TXPWR_BY_CMD: OFDM = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n", 
			priv->pshare->rf_ft_var.txPowerPlus_mcs_3,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_2,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_1,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_0, 
			priv->pshare->rf_ft_var.txPowerPlus_mcs_7,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_6,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_5,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_4, 
			priv->pshare->rf_ft_var.txPowerPlus_mcs_11,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_10,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_9,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_8, 
			priv->pshare->rf_ft_var.txPowerPlus_mcs_15,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_14,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_13,
			priv->pshare->rf_ft_var.txPowerPlus_mcs_12);
	}
	
}
#endif


static void dump_cck(struct rtl8192cd_priv *priv, 
	unsigned char pwrlevelCCK_A, unsigned char pwrlevelCCK_B, 
	char *CCKTxAgc_A, char *CCKTxAgc_B
	)
{
	char byte, byte1, byte2, byte3;

	if (priv->pshare->rf_ft_var.cck_pwr_max) 
	{
		 printk("Use cck_pwr_max = %d\n", priv->pshare->rf_ft_var.cck_pwr_max);
		 
		 byte = POWER_RANGE_CHECK(priv->pshare->rf_ft_var.cck_pwr_max);	 
		 printk("A_CCK1_Mcs32(0xe08): 0x----%02x--\n", byte);
		 printk("B_CCK5_1_Mcs32(0x838): 0x%02x%02x%02x--\n", byte, byte, byte);
		 printk("A_CCK11_2_B_CCK11(0x86c): 0x%02x%02x%02x%02x\n", byte, byte, byte, byte);
		 
		 return;
	}

	if (pwrlevelCCK_A == 0) {	// use default value
#ifdef HIGH_POWER_EXT_PA
		if (priv->pshare->rf_ft_var.use_ext_pa)
			byte = HP_CCK_POWER_DEFAULT;
		else
#endif
			byte = 0x24;

		printk("Use default cck value = %d\n", byte);

		pwrlevelCCK_A = pwrlevelCCK_B = byte;
		byte = 0;
		ASSIGN_TX_POWER_OFFSET(byte, priv->pshare->rf_ft_var.txPowerPlus_cck_1);
		byte = POWER_RANGE_CHECK(pwrlevelCCK_A + byte);
		printk("A_CCK1_Mcs32(0xe08): 0x----%02x--\n", byte);
		
		byte = byte1 = byte2 = 0;
		ASSIGN_TX_POWER_OFFSET(byte, priv->pshare->rf_ft_var.txPowerPlus_cck_1);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_cck_2);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_cck_5);
		byte  = POWER_RANGE_CHECK(pwrlevelCCK_B + byte);
		byte1 = POWER_RANGE_CHECK(pwrlevelCCK_B + byte1);
		byte2 = POWER_RANGE_CHECK(pwrlevelCCK_B + byte2);
		printk("B_CCK5_1_Mcs32(0x838): 0x%02x%02x%02x--\n", byte2, byte1, byte);

		byte = byte1 = byte2 = 0;
		ASSIGN_TX_POWER_OFFSET(byte, priv->pshare->rf_ft_var.txPowerPlus_cck_2);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_cck_5);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_cck_11);
		byte  = POWER_RANGE_CHECK(pwrlevelCCK_A + byte);
		byte1 = POWER_RANGE_CHECK(pwrlevelCCK_A + byte1);
		byte2 = POWER_RANGE_CHECK(pwrlevelCCK_A + byte2);
		printk("A_CCK11_2_B_CCK11(0x86c): 0x%02x%02x%02x%02x\n", byte2, byte1, byte, byte2);
		
		return; // use default
	}

	byte = POWER_RANGE_CHECK(pwrlevelCCK_A + CCKTxAgc_A[3]);
	printk("A_CCK1_Mcs32(0xe08): 0x----%02x--\n", byte);

	byte = POWER_RANGE_CHECK(pwrlevelCCK_B + CCKTxAgc_B[1]); 
	byte1 = POWER_RANGE_CHECK(pwrlevelCCK_B + CCKTxAgc_B[2]);
	byte2 = POWER_RANGE_CHECK(pwrlevelCCK_B + CCKTxAgc_B[3]);
	printk("B_CCK5_1_Mcs32(0x838): 0x%02x%02x%02x--\n", byte, byte1, byte2);
	
	byte = POWER_RANGE_CHECK(pwrlevelCCK_A + CCKTxAgc_A[0]);
	byte1 = POWER_RANGE_CHECK(pwrlevelCCK_A + CCKTxAgc_A[1]);
	byte2 = POWER_RANGE_CHECK(pwrlevelCCK_A + CCKTxAgc_A[2]);
	byte3 = POWER_RANGE_CHECK(pwrlevelCCK_B + CCKTxAgc_B[0]);
	printk("A_CCK11_2_B_CCK11(0x86c): 0x%02x%02x%02x%02x\n", byte, byte1, byte2, byte3);

	return;

}


static void dump_ofdm_mcs0(struct rtl8192cd_priv *priv, int defValue, 
	unsigned char pwrlevelHT40_1S_A, unsigned char pwrlevelHT40_1S_B, 
	unsigned char pwrdiffHT40_2S, unsigned char pwrdiffHT20, unsigned char pwrdiffOFDM, 
	unsigned char pwrlevelHT40_1S_A_6dB, unsigned char pwrlevelHT40_1S_B_6dB,
	unsigned char pwrdiffHT40_2S_6dB, unsigned char pwrdiffHT20_6dB,
	char *MCSTxAgcOffset_A, char *MCSTxAgcOffset_B, char *OFDMTxAgcOffset_A, char *OFDMTxAgcOffset_B,
	int phyBandSelect
	)
{

	char base, byte0, byte1, byte2, byte3;
	unsigned char  offset;
	
#ifdef USB_POWER_SUPPORT
	char base_6dBm;
	unsigned char offset_6dBm;
#endif
	
	if (pwrlevelHT40_1S_A == 0)
	{
		
#ifdef HIGH_POWER_EXT_PA
		if (priv->pshare->rf_ft_var.use_ext_pa)
			defValue = HP_OFDM_POWER_DEFAULT ;
#endif
		printk("pwrlevelHT40_1S_A = 0, Use Default Value = %d\n", defValue);

		base = defValue;
		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_ofdm_18);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_ofdm_12);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_ofdm_9);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_ofdm_6);

		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		
		printk("A_Rate18_06(0xe00): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);
		printk("B_Rate18_06(0x830): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_ofdm_54);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_ofdm_48);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_ofdm_36);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_ofdm_24);
		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		printk("A_Rate54_24(0xe04): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3); 
		printk("B_Rate54_24(0x834): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	 


		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_3);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_2);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_1);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_0);
		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		printk("A_Mcs03_Mcs00(0xe10): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);
		printk("B_Mcs03_Mcs00(0x83c): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);

		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_7);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_6);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_5);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_4);
		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		printk("A_Mcs07_Mcs04(0xe14): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);
		printk("B_Mcs07_Mcs04(0x848): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);


//_TXPWR_REDEFINE
#ifdef USB_POWER_SUPPORT
		byte0 = byte1 = byte2 = byte3 = -USB_HT_2S_DIFF;
#else
		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_11);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_10);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_9);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_8);
#endif

		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		printk("A_Mcs11_Mcs08(0xe18): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);
		printk("B_Mcs11_Mcs08(0x84c): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

//_TXPWR_REDEFINE
#ifdef USB_POWER_SUPPORT
		byte0 = byte1 = byte2 = byte3 = -USB_HT_2S_DIFF;
#else
		byte0 = byte1 = byte2 = byte3 = 0;
		ASSIGN_TX_POWER_OFFSET(byte0, priv->pshare->rf_ft_var.txPowerPlus_mcs_15);
		ASSIGN_TX_POWER_OFFSET(byte1, priv->pshare->rf_ft_var.txPowerPlus_mcs_14);
		ASSIGN_TX_POWER_OFFSET(byte2, priv->pshare->rf_ft_var.txPowerPlus_mcs_13);
		ASSIGN_TX_POWER_OFFSET(byte3, priv->pshare->rf_ft_var.txPowerPlus_mcs_12);
#endif

		byte0 = POWER_RANGE_CHECK(base + byte0);
		byte1 = POWER_RANGE_CHECK(base + byte1);
		byte2 = POWER_RANGE_CHECK(base + byte2);
		byte3 = POWER_RANGE_CHECK(base + byte3);
		printk("A_Mcs15_Mcs12(0xe1c): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);
		printk("B_Mcs15_Mcs12(0x868): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

		return; // use default
	}
	else
	{
		//===PATH A===
		//OFDM
		base = pwrlevelHT40_1S_A;
		offset = (pwrdiffOFDM & 0x0f);
		
#if defined(CONFIG_RTL_92D_SUPPORT)&& defined(CONFIG_RTL_92D_DMDP)
		//_TXPWR_REDEFINE??
		if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
			offset = ((pwrdiffOFDM & 0xf0) >> 4);
		}
#endif	
		base = COUNT_SIGN_OFFSET(base, offset);
		
		byte0 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[0]);
		byte1 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[1]);
		byte2 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[2]);
		byte3 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[3]);
		printk("A_Rate18_06(0xe00): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);		

		
		byte0 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[4]);
		byte1 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[5]);
		byte2 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[6]);
		byte3 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_A[7]);
		printk("A_Rate54_24(0xe04): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	 

		//MCS 0 - 7
		base = pwrlevelHT40_1S_A;
		if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20) {
			offset = (pwrdiffHT20 & 0x0f);
#if defined(CONFIG_RTL_92D_SUPPORT)&& defined(CONFIG_RTL_92D_DMDP)
			//_TXPWR_REDEFINE??
			if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
				offset = ((pwrdiffHT20 & 0xf0) >> 4);
			}
#endif
			base = COUNT_SIGN_OFFSET(base, offset);
		}

		byte0 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[0]);
		byte1 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[1]);
		byte2 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[2]);
		byte3 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[3]);
		printk("A_Mcs03_Mcs00(0xe10): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	
		
		byte0 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[4]);
		byte1 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[5]);
		byte2 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[6]);
		byte3 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_A[7]);	  
		printk("A_Mcs07_Mcs04(0xe14): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);

		offset = (pwrdiffHT40_2S & 0x0f);
#if defined(CONFIG_RTL_92D_SUPPORT)&& defined(CONFIG_RTL_92D_DMDP)
		//_TXPWR_REDEFINE??
		if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
			offset = ((pwrdiffHT40_2S & 0xf0) >> 4);
		}
#endif	
		base = COUNT_SIGN_OFFSET(base, offset);

		//MCS 8 -12
#ifdef USB_POWER_SUPPORT

		base_6dBm = pwrlevelHT40_1S_A_6dB;
		
		if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20) 
		{
			offset_6dBm = (pwrdiffHT20_6dB & 0x0f);
				
#if defined(CONFIG_RTL_92D_SUPPORT)&& defined(CONFIG_RTL_92D_DMDP)
			if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
				offset_6dBm = ((pwrdiffHT20_6dB & 0xf0) >> 4);
			}
#endif	
			base_6dBm = COUNT_SIGN_OFFSET(base_6dBm, offset_6dBm);
		}
			
		offset_6dBm = (pwrdiffHT40_2S_6dB & 0x0f);
			
#if defined(CONFIG_RTL_92D_SUPPORT)&& defined(CONFIG_RTL_92D_DMDP)
		if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
			offset_6dBm = ((pwrdiffHT40_2S_6dB & 0xf0) >> 4);
		}
#endif	
		
		base_6dBm = COUNT_SIGN_OFFSET(base_6dBm, offset_6dBm);
		
		if ((pwrlevelHT40_1S_A_6dB!= 0) && (pwrlevelHT40_1S_A_6dB!= pwrlevelHT40_1S_A))
			byte0 = byte1 = byte2 = byte3 = base_6dBm;
		else if((base - USB_HT_2S_DIFF) > 0)
			byte0 = byte1 = byte2 = byte3 = POWER_RANGE_CHECK(base - USB_HT_2S_DIFF);
		else
			byte0 = byte1 = byte2 = byte3 = POWER_RANGE_CHECK(defValue - USB_HT_2S_DIFF);

		printk("A_Mcs11_Mcs08(0xe18): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);

		if ((pwrlevelHT40_1S_A_6dB != 0) && (pwrlevelHT40_1S_A_6dB != pwrlevelHT40_1S_A))
			byte0 = byte1 = byte2 = byte3 =	base_6dBm;
		else if((base - USB_HT_2S_DIFF) > 0)
			byte0 = byte1 = byte2 = byte3 =	POWER_RANGE_CHECK(base - USB_HT_2S_DIFF);
		else
			byte0 = byte1 = byte2 = byte3 =	POWER_RANGE_CHECK(defValue - USB_HT_2S_DIFF);

		printk("A_Mcs15_Mcs12(0xe1c): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);

#else			
		byte0 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[8]);
		byte1 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[9]);
		byte2 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[10]);
		byte3 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[11]);
		printk("A_Mcs11_Mcs08(0xe18): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

		byte0 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[12]);
		byte1 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[13]);
		byte2 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[14]);
		byte3 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_A[15]);
		printk("A_Mcs15_Mcs12(0xe1c): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);
#endif
		//===PATH B===
		if (pwrlevelHT40_1S_B == 0)
			pwrlevelHT40_1S_B = defValue;
		
		//OFDM
		base = pwrlevelHT40_1S_B;
		offset = ((pwrdiffOFDM & 0xf0) >> 4);
		base = COUNT_SIGN_OFFSET(base, offset);
		
		byte0 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[0]);
		byte1 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[1]);
		byte2 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[2]);
		byte3 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[3]);	
		printk("B_Rate18_06(0x830): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);		

		
		byte0 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[4]);
		byte1 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[5]);
		byte2 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[6]);
		byte3 = POWER_RANGE_CHECK(base + OFDMTxAgcOffset_B[7]);
		printk("B_Rate54_24(0x834): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	 

		//MCS 0 - 7
		base = pwrlevelHT40_1S_B;
		if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20) {
			offset = ((pwrdiffHT20 & 0xf0) >> 4);
			base = COUNT_SIGN_OFFSET(base, offset);
		}

		byte0 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[0]);
		byte1 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[1]);
		byte2 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[2]);
		byte3 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[3]);
		printk("B_Mcs03_Mcs00(0x83c): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);	
		
		byte0 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[4]);
		byte1 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[5]);
		byte2 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[6]);
		byte3 = POWER_RANGE_CHECK(base + MCSTxAgcOffset_B[7]);	  
		printk("B_Mcs07_Mcs04(0x848): 0x%02x%02x%02x%02x\n", byte0, byte1, byte2, byte3);

		offset = ((pwrdiffHT40_2S & 0xf0) >> 4);
		base = COUNT_SIGN_OFFSET(base, offset);

		//MCS 8 -12
#ifdef USB_POWER_SUPPORT

		base_6dBm = pwrlevelHT40_1S_B_6dB;
		if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20) {
			offset_6dBm = ((pwrdiffHT20_6dB & 0xf0) >> 4);
			base_6dBm = COUNT_SIGN_OFFSET(base_6dBm, offset_6dBm);
		}
		
		offset_6dBm = ((pwrdiffHT40_2S_6dB& 0xf0) >> 4);
		base_6dBm = COUNT_SIGN_OFFSET(base_6dBm, offset_6dBm);
		
		if ((pwrlevelHT40_1S_B_6dB!= 0) && (pwrlevelHT40_1S_B_6dB!= pwrlevelHT40_1S_B))
			byte0 = byte1 = byte2 = byte3 = base_6dBm;
		else if((base - USB_HT_2S_DIFF) > 0)
			byte0 = byte1 = byte2 = byte3 = POWER_RANGE_CHECK(base - USB_HT_2S_DIFF);
		else
			byte0 = byte1 = byte2 = byte3 = POWER_RANGE_CHECK(defValue - USB_HT_2S_DIFF);

		printk("B_Mcs11_Mcs08(0x84c): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

		if ((pwrlevelHT40_1S_B_6dB != 0) && (pwrlevelHT40_1S_B_6dB != pwrlevelHT40_1S_B))
			byte0 = byte1 = byte2 = byte3 =	base_6dBm;
		else if((base - USB_HT_2S_DIFF) > 0)
			byte0 = byte1 = byte2 = byte3 =	POWER_RANGE_CHECK(base - USB_HT_2S_DIFF);
		else
			byte0 = byte1 = byte2 = byte3 =	POWER_RANGE_CHECK(defValue - USB_HT_2S_DIFF);

		printk("B_Mcs15_Mcs12(0x868): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

#else			
		byte0 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[8]);
		byte1 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[9]);
		byte2 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[10]);
		byte3 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[11]);
		printk("B_Mcs11_Mcs08(0x84c): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

		byte0 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[12]);
		byte1 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[13]);
		byte2 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[14]);
		byte3 = POWER_RANGE_CHECK(base + priv->pshare->phw->MCSTxAgcOffset_B[15]);
		printk("B_Mcs15_Mcs12(0x868): 0x%02x%02x%02x%02x\n",byte0, byte1, byte2, byte3);

		return;
#endif
	}

}

static void txpwr_dump(struct rtl8192cd_priv *priv, int start, int end)
{

	int channel = 0;
	int tmp = 0;
	int defValue = 0x28;
//	char base, byte0, byte1, byte2, byte3;
//	unsigned char  offset;
	unsigned int phyBandSelect;
	
	char pwrlevelCCK_A = 0;
	char pwrlevelCCK_B = 0;
	
	unsigned char pwrlevelHT40_1S_A = 0;
	unsigned char pwrlevelHT40_1S_B = 0;
	unsigned char pwrdiffHT40_2S = 0;
	unsigned char pwrdiffHT20 = 0;
	unsigned char pwrdiffOFDM = 0;

#if 1 // USB_POWER_SUPPORT
	unsigned char pwrlevelHT40_1S_A_6dB = 0;
	unsigned char pwrlevelHT40_1S_B_6dB = 0;
	unsigned char pwrdiffHT40_2S_6dB = 0;
	unsigned char pwrdiffHT20_6dB = 0;
//	char base_6dBm;
//	unsigned char offset_6dBm;
#endif

	int pg_tbl_idx = 0;
	int PHYREG_PG = 4;
	char MCSTxAgcOffset_A[16];
	char MCSTxAgcOffset_B[16];
	char OFDMTxAgcOffset_A[8];
	char OFDMTxAgcOffset_B[8];

	//_TXPWR_REDEFINE ?? int or char ??
	char CCKTxAgc_A[4];
	char CCKTxAgc_B[4];

#ifdef TXPWR_LMT
	unsigned int tgpwr_CCK = 0;
	unsigned int tgpwr_OFDM = 0;
	unsigned int txpwr_lmt_CCK = 0;
	unsigned int txpwr_lmt_OFDM = 0;

	unsigned int tgpwr_HT1S = 0;
	unsigned int tgpwr_HT2S = 0;
	unsigned int txpwr_lmt_HT1S = 0;
	unsigned int txpwr_lmt_HT2S = 0;

	int i;
	int max_idx;
#endif

	if(end <= 14)
		phyBandSelect = PHY_BAND_2G;
	else
		phyBandSelect = PHY_BAND_5G;

	
#ifdef CONFIG_RTL_92D_SUPPORT
			if (GET_CHIP_VER(priv)==VERSION_8192D) {
#if defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E)
				if (phyBandSelect & PHY_BAND_5G)
					defValue=0x28;
				else
					defValue=0x2d;
#else
				if (phyBandSelect & PHY_BAND_5G)
					defValue=0x26;
				else
					defValue=0x30;
#endif
			}
#endif

	
	if(start > end)
	{
		printk("Error! start = %d < end = %d\n", start, end);
	}
	else if (end <= 14)
	{
		for(channel = start; channel <= end; channel++ ) //_TXPWR_REDEFINE ?? DO NOT PRINT TOO MUCH
		{
			
			printk("\n[CHANNEL%03d]", channel);
			printk("\n");

			//===GET FROM FLASH===
			
			pwrlevelCCK_A = priv->pmib->dot11RFEntry.pwrlevelCCK_A[channel-1];
			pwrlevelCCK_B = priv->pmib->dot11RFEntry.pwrlevelCCK_B[channel-1];
			pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevelHT40_1S_A[channel-1];
			pwrlevelHT40_1S_B = priv->pmib->dot11RFEntry.pwrlevelHT40_1S_B[channel-1];
			pwrdiffHT40_2S = priv->pmib->dot11RFEntry.pwrdiffHT40_2S[channel-1];
			pwrdiffHT20 = priv->pmib->dot11RFEntry.pwrdiffHT20[channel-1];
			pwrdiffOFDM = priv->pmib->dot11RFEntry.pwrdiffOFDM[channel-1];
			
#ifdef USB_POWER_SUPPORT
			printk(">>FLASH - 13dBm<<\n");
#endif
			printk("pwrlevelCCK_A = %d, pwrlevelCCK_B = %d\n", pwrlevelCCK_A, pwrlevelCCK_B);
			printk("pwrlevelHT40_1S_A = %d, pwrlevelHT40_1S_B = %d\n", pwrlevelHT40_1S_A, pwrlevelHT40_1S_B);
			printk("pwrdiffHT40_2S = %d(0x%02x), pwrdiffHT20 = %d(0x%02x), pwrdiffOFDM = %d(0x%02x)\n", 
						pwrdiffHT40_2S, pwrdiffHT40_2S,
						pwrdiffHT20, pwrdiffHT20,
						pwrdiffOFDM, pwrdiffOFDM);

#ifdef USB_POWER_SUPPORT
			printk(">>FLASH - 6dBm<<\n");
			pwrlevelHT40_1S_A_6dB = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[channel-1];
			pwrlevelHT40_1S_B_6dB = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
			pwrdiffHT40_2S_6dB = priv->pmib->dot11RFEntry.pwrdiff5GHT40_2S[channel-1];
			pwrdiffHT20_6dB = priv->pmib->dot11RFEntry.pwrdiff5GHT20[channel-1];
			
			printk("pwrlevelHT40_1S_A = %d, pwrlevelHT40_1S_B = %d\n", pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB);
			printk("pwrdiffHT40_2S = %d(0x%02x), pwrdiffHT20 = %d(0x%02x)\n", 
						pwrdiffHT40_2S_6dB, pwrdiffHT40_2S_6dB,
						pwrdiffHT20_6dB, pwrdiffHT20_6dB);
#endif


#if defined(CONFIG_RTL_92D_SUPPORT) && defined(CONFIG_RTL_92D_DMDP)
			if (GET_CHIP_VER(priv)==VERSION_8192D) {
				if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) {
					printk("92D-DMDP WLAN1 , Set pwrlevelCCK_A = pwrlevelCCK_B\n");
					if (phyBandSelect & PHY_BAND_2G)
						pwrlevelCCK_A = priv->pmib->dot11RFEntry.pwrlevelCCK_B[channel-1];
				}
			}
#endif


#ifdef CONFIG_RTL_92D_DMDP//_Eric ?? Get chip ??
			if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) 
			{
				printk("92D-DMDP WLAN1, Set pwrlevelHT40_1S_A = pwrlevelHT40_1S_B\n");
				if (phyBandSelect & PHY_BAND_5G)
				{
					pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
				}
				else 
				{
					pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevelHT40_1S_B[channel-1];
				}
#ifdef USB_POWER_SUPPORT
				if (phyBandSelect & PHY_BAND_5G) 
				{
					pwrlevelHT40_1S_A_6dB= priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel];
				} 
				else 
				{
					pwrlevelHT40_1S_A_6dB= priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
				}	
#endif
			}
#endif


			//===POWER BY RATE===
			printk(">>Power By Rate Table<<\n");
			pg_tbl_idx = 0;

#ifdef CONFIG_RTL_92D_SUPPORT
			if (GET_CHIP_VER(priv) == VERSION_8192D) {
				if (priv->pshare->is_40m_bw == 0) {
					if (channel<=3)
						pg_tbl_idx = BGN_20_CH1_3;
					else if (channel<=9)
						pg_tbl_idx = BGN_20_CH4_9;
					else
						pg_tbl_idx = BGN_20_CH10_14;
				} else {
					if (channel<=3)
						pg_tbl_idx = BGN_40_CH1_3;
					else if (channel<=9)
						pg_tbl_idx = BGN_40_CH4_9;
					else
						pg_tbl_idx = BGN_40_CH10_14;
				}
			}
#ifdef MP_TEST
			//In Noraml Driver mode, and if mib 'pwr_by_rate' = 0 >> Use default power by rate table 
			if( (priv->pshare->rf_ft_var.mp_specific == 0) && (priv->pshare->rf_ft_var.pwr_by_rate == 0) )
				pg_tbl_idx = BGN_2040_ALL;
#endif
#endif

			printk("pg_tbl_idx = %d\n", pg_tbl_idx);

			Read_PG_File(priv, PHYREG_PG, pg_tbl_idx, 
					MCSTxAgcOffset_A, MCSTxAgcOffset_B, 
					OFDMTxAgcOffset_A, OFDMTxAgcOffset_B,
					CCKTxAgc_A, CCKTxAgc_B);

			printk("MCSTxAgcOffset_A - ");
			for(tmp = 0; tmp <16; tmp ++)
				printk("%02x ", MCSTxAgcOffset_A[tmp]);
			printk("\n");

			printk("MCSTxAgcOffset_B - ");
			for(tmp = 0; tmp <16; tmp ++)
				printk("%02x ", MCSTxAgcOffset_B[tmp]);
			printk("\n");

			printk("OFDMTxAgcOffset_A - ");
			for(tmp = 0; tmp <8; tmp ++)
				printk("%02x ", OFDMTxAgcOffset_A[tmp]);
			printk("\n");

			printk("OFDMTxAgcOffset_B - ");
			for(tmp = 0; tmp <8; tmp ++)
				printk("%02x ", OFDMTxAgcOffset_B[tmp]);
			printk("\n");

			printk("CCKTxAgc_A - ");
			for(tmp = 0; tmp <4; tmp ++)
				printk("%02x ", CCKTxAgc_A[tmp]);
			printk("\n");

			printk("CCKTxAgc_B - ");
			for(tmp = 0; tmp <4; tmp ++)
				printk("%02x ", CCKTxAgc_B[tmp]);
			printk("\n");

#ifdef ADD_TX_POWER_BY_CMD
			check_txpwr_by_cmd(priv, MCSTxAgcOffset_A, MCSTxAgcOffset_B, 
			 	OFDMTxAgcOffset_A, OFDMTxAgcOffset_B, CCKTxAgc_A, CCKTxAgc_B);
#endif


			//===Count FLASH + POWER BY RATE===

			printk(">>Tx Power - Power By Rate<<\n");
	
			dump_cck(priv, pwrlevelCCK_A, pwrlevelCCK_B, CCKTxAgc_A, CCKTxAgc_B);
			
			dump_ofdm_mcs0(priv, defValue, pwrlevelHT40_1S_A, pwrlevelHT40_1S_B, 
				pwrdiffHT40_2S, pwrdiffHT20, pwrdiffOFDM, 
				pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB,
				pwrdiffHT40_2S_6dB, pwrdiffHT20_6dB,
				MCSTxAgcOffset_A, MCSTxAgcOffset_B, OFDMTxAgcOffset_A, OFDMTxAgcOffset_B,phyBandSelect);
				
			
#ifdef TXPWR_LMT
			//===BAND EDGE LIMIT===
			if (priv->pshare->rf_ft_var.disable_txpwrlmt)
				continue;
			
			printk(">>Band Edge Limit Table<<\n");
			tmp = ch2idx(channel);
			
			txpwr_lmt_CCK = priv->pshare->ch_pwr_lmtCCK[tmp];
			txpwr_lmt_OFDM = priv->pshare->ch_pwr_lmtOFDM[tmp];
			tgpwr_CCK = priv->pshare->ch_tgpwr_CCK[tmp];
			tgpwr_OFDM = priv->pshare->ch_tgpwr_OFDM[tmp];

			printk("txpwr_lmt_CCK = %d tgpwr_CCK = %d\n", txpwr_lmt_CCK,  tgpwr_CCK);
			printk("txpwr_lmt_OFDM = %d tgpwr_OFDM = %d\n", txpwr_lmt_OFDM,  tgpwr_OFDM);

			if (priv->pshare->is_40m_bw == 0)
			{
				txpwr_lmt_HT1S = priv->pshare->ch_pwr_lmtHT20_1S[tmp];
				txpwr_lmt_HT2S = priv->pshare->ch_pwr_lmtHT20_2S[tmp];
				tgpwr_HT1S = priv->pshare->ch_tgpwr_HT20_1S[tmp];
				tgpwr_HT2S = priv->pshare->ch_tgpwr_HT20_2S[tmp];

				printk("txpwr_lmt_HT1S_20M = %d tgpwr_HT1S_20M = %d\n", txpwr_lmt_HT1S,  tgpwr_HT1S);
				printk("txpwr_lmt_HT2S_20M = %d tgpwr_HT2S_20M = %d\n", txpwr_lmt_HT2S,  tgpwr_HT2S);
			}
			else
			{
				txpwr_lmt_HT1S = priv->pshare->ch_pwr_lmtHT40_1S[tmp];
				txpwr_lmt_HT2S = priv->pshare->ch_pwr_lmtHT40_2S[tmp];
				tgpwr_HT1S = priv->pshare->ch_tgpwr_HT40_1S[tmp];
				tgpwr_HT2S = priv->pshare->ch_tgpwr_HT40_2S[tmp];

				printk("txpwr_lmt_HT1S_40M = %d tgpwr_HT1S_40M = %d\n", txpwr_lmt_HT1S,  tgpwr_HT1S);
				printk("txpwr_lmt_HT2S_40M = %d tgpwr_HT2S_40M = %d\n", txpwr_lmt_HT2S,  tgpwr_HT2S);
			}

			//===Count FLASH + min{POWER BY RATE, LIMIT}===
			if((txpwr_lmt_CCK == 0) && (txpwr_lmt_OFDM == 0) 
				&& (txpwr_lmt_HT1S == 0) && (txpwr_lmt_HT1S == 0))
			{
				printk("No Band Edge Limit for this channel=%d\n", channel);
				continue;
			}
			
			if (txpwr_lmt_CCK || tgpwr_CCK){
				max_idx=255;
			}else{
				max_idx = (txpwr_lmt_CCK - tgpwr_CCK);
			}

			for (i=0; i<=3; i++) {
				CCKTxAgc_A[i] = POWER_MIN_CHECK(CCKTxAgc_A[i], max_idx);
				CCKTxAgc_A[i] = POWER_MIN_CHECK(CCKTxAgc_A[i], max_idx);
			}

			dump_cck(priv, pwrlevelCCK_A, pwrlevelCCK_B, CCKTxAgc_A, CCKTxAgc_B);
		
			if (!txpwr_lmt_OFDM || !tgpwr_OFDM){
				max_idx=255;
			}else{
				max_idx = (txpwr_lmt_OFDM - tgpwr_OFDM);
			}

			for (i=0; i<=7; i++) {
				OFDMTxAgcOffset_A[i] = POWER_MIN_CHECK(OFDMTxAgcOffset_A[i], max_idx);
				OFDMTxAgcOffset_B[i] = POWER_MIN_CHECK(OFDMTxAgcOffset_B[i], max_idx);
			}
	
			if (!txpwr_lmt_HT1S || !tgpwr_HT1S){
				max_idx = 255;
			}else{
				max_idx = (txpwr_lmt_HT1S - tgpwr_HT1S);
			}

			for (i=0; i<=7; i++) {
				MCSTxAgcOffset_A[i] = POWER_MIN_CHECK(MCSTxAgcOffset_A[i], max_idx);
				MCSTxAgcOffset_B[i] = POWER_MIN_CHECK(MCSTxAgcOffset_B[i], max_idx);
			}

			if (!txpwr_lmt_HT2S || !tgpwr_HT2S){
				max_idx = 255;
			}else{
				max_idx = (txpwr_lmt_HT2S - tgpwr_HT2S);
			}

			for (i=8; i<=15; i++) {
				MCSTxAgcOffset_A[i] = POWER_MIN_CHECK(MCSTxAgcOffset_A[i], max_idx);
				MCSTxAgcOffset_B[i] = POWER_MIN_CHECK(MCSTxAgcOffset_B[i], max_idx);
			}

			dump_ofdm_mcs0(priv, defValue, pwrlevelHT40_1S_A, pwrlevelHT40_1S_B, 
				pwrdiffHT40_2S, pwrdiffHT20, pwrdiffOFDM, 
				pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB,
				pwrdiffHT40_2S_6dB, pwrdiffHT20_6dB,
				MCSTxAgcOffset_A, MCSTxAgcOffset_B, OFDMTxAgcOffset_A, OFDMTxAgcOffset_B,phyBandSelect);
#endif
	
		}
	}
	else if (end <= 199)
	{
		for(channel = start; channel <= end; channel+=2 )
		{
			int ori_channel = channel;
		
			printk("\n[CHANNEL%03d]", channel);
			printk("\n");

			//TXPWR_REDEFINE
			//FLASH GROUP [36-99] [100-148] [149-165] 
			//Special Cases: [34-2, 34, 34+2,  36-2, 165+2]:No DATA , [149-2]:FLASH DATA OF Channel-146-6dBm
			//Use Flash data of channel 36 & 140 & 165 for these special cases.
			if((channel > 30) && (channel < 36))
				channel = 36;
			else if (channel == (149-2))
				channel = 140;
			else if(channel > 165)
				channel = 165;

			//===GET FROM FLASH===
			
			pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[channel-1];
			pwrlevelHT40_1S_B = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
			pwrdiffHT40_2S = priv->pmib->dot11RFEntry.pwrdiff5GHT40_2S[channel-1];
			pwrdiffHT20 = priv->pmib->dot11RFEntry.pwrdiff5GHT20[channel-1];
			pwrdiffOFDM = priv->pmib->dot11RFEntry.pwrdiff5GOFDM[channel-1];
		
#ifdef USB_POWER_SUPPORT
			printk(">>Flash - 13dBm<<\n");
#endif

			printk("pwrlevelHT40_1S_A = %d, pwrlevelHT40_1S_B = %d\n", pwrlevelHT40_1S_A, pwrlevelHT40_1S_B);
			printk("pwrdiffHT40_2S = %d(0x%02x), pwrdiffHT20 = %d(0x%02x), pwrdiffOFDM = %d(0x%02x)\n", 
						pwrdiffHT40_2S, pwrdiffHT40_2S,
						pwrdiffHT20, pwrdiffHT20,
						pwrdiffOFDM, pwrdiffOFDM);

#ifdef USB_POWER_SUPPORT
			printk(">>Flash - 6dBm<<\n");
			pwrlevelHT40_1S_A_6dB = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[channel];
			pwrlevelHT40_1S_B_6dB = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel];
			pwrdiffHT40_2S_6dB = priv->pmib->dot11RFEntry.pwrdiff5GHT40_2S[channel];
			pwrdiffHT20_6dB = priv->pmib->dot11RFEntry.pwrdiff5GHT20[channel];

			printk("pwrlevelHT40_1S_A = %d, pwrlevelHT40_1S_B = %d\n", pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB);
			printk("pwrdiffHT40_2S = %d(0x%02x), pwrdiffHT20 = %d(0x%02x)\n", 
						pwrdiffHT40_2S_6dB, pwrdiffHT40_2S_6dB,
						pwrdiffHT20_6dB, pwrdiffHT20_6dB);
#endif

#ifdef CONFIG_RTL_92D_DMDP
			if (priv->pmib->dot11RFEntry.macPhyMode==DUALMAC_DUALPHY && priv->pshare->wlandev_idx == 1) 
			{
				printk("92D-DMDP WLAN1, Set pwrlevelHT40_1S_A = pwrlevelHT40_1S_B\n");
				if (phyBandSelect & PHY_BAND_5G)
				{
					pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
				}
				else 
				{
					pwrlevelHT40_1S_A = priv->pmib->dot11RFEntry.pwrlevelHT40_1S_B[channel-1];
				}
#ifdef USB_POWER_SUPPORT
				if (phyBandSelect & PHY_BAND_5G) 
				{
					pwrlevelHT40_1S_A_6dB= priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel];
				} 
				else 
				{
					pwrlevelHT40_1S_A_6dB= priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[channel-1];
				}	
#endif
			}
#endif

			channel = ori_channel;


			//===POWER BY RATE===
			printk(">>Power By Rate Table<<\n");

			pg_tbl_idx = 0;

#ifdef CONFIG_RTL_92D_SUPPORT
			if (GET_CHIP_VER(priv) == VERSION_8192D) {
				if (priv->pshare->is_40m_bw == 0) {
					if (channel<=99)
						pg_tbl_idx = AN_20_CH_36_64;
					else if (channel<=148)
						pg_tbl_idx = AN_20_CH_100_140;
					else
						pg_tbl_idx = AN_20_CH_149_165;
				} else {
					if (channel<=99)
						pg_tbl_idx = AN_40_CH_36_64;
					else if (channel<=148)
						pg_tbl_idx = AN_40_CH_100_140;
					else
						pg_tbl_idx = AN_40_CH_149_165;
				}
			}
#ifdef MP_TEST
			//In Noraml Driver mode, and if mib 'pwr_by_rate' = 0 >> Use default power by rate table 
			if( (priv->pshare->rf_ft_var.mp_specific == 0) && (priv->pshare->rf_ft_var.pwr_by_rate == 0) )
				pg_tbl_idx = BGN_2040_ALL;
#endif
#endif

			printk("pg_tbl_idx = %d\n", pg_tbl_idx);

			Read_PG_File(priv, PHYREG_PG, pg_tbl_idx, 
					MCSTxAgcOffset_A, MCSTxAgcOffset_B, 
					OFDMTxAgcOffset_A, OFDMTxAgcOffset_B,
					CCKTxAgc_A, CCKTxAgc_B);

			printk("MCSTxAgcOffset_A - ");
			for(tmp = 0; tmp <16; tmp ++)
				printk("%02x ", MCSTxAgcOffset_A[tmp]);
			printk("\n");

			printk("MCSTxAgcOffset_B - ");
			for(tmp = 0; tmp <16; tmp ++)
				printk("%02x ", MCSTxAgcOffset_B[tmp]);
			printk("\n");

			printk("OFDMTxAgcOffset_A - ");
			for(tmp = 0; tmp <8; tmp ++)
				printk("%02x ", OFDMTxAgcOffset_A[tmp]);
			printk("\n");

			printk("OFDMTxAgcOffset_B - ");
			for(tmp = 0; tmp <8; tmp ++)
				printk("%02x ", OFDMTxAgcOffset_B[tmp]);
			printk("\n");

			printk("CCKTxAgc_A - ");
			for(tmp = 0; tmp <4; tmp ++)
				printk("%02x ", CCKTxAgc_A[tmp]);
			printk("\n");

			printk("CCKTxAgc_B - ");
			for(tmp = 0; tmp <4; tmp ++)
				printk("%02x ", CCKTxAgc_B[tmp]);
			printk("\n");

#ifdef ADD_TX_POWER_BY_CMD
			check_txpwr_by_cmd(priv, MCSTxAgcOffset_A, MCSTxAgcOffset_B, 
			 	OFDMTxAgcOffset_A, OFDMTxAgcOffset_B, CCKTxAgc_A, CCKTxAgc_B);
#endif

			//===Count FLASH + POWER BY RATE===

			printk(">>Tx Power - Power By Rate<<\n");
			
			dump_ofdm_mcs0(priv, defValue, pwrlevelHT40_1S_A, pwrlevelHT40_1S_B, 
				pwrdiffHT40_2S, pwrdiffHT20, pwrdiffOFDM, 
				pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB,
				pwrdiffHT40_2S_6dB, pwrdiffHT20_6dB,
				MCSTxAgcOffset_A, MCSTxAgcOffset_B, OFDMTxAgcOffset_A, OFDMTxAgcOffset_B, phyBandSelect);


#ifdef TXPWR_LMT
			//===BAND EDGE LIMIT===
			if (priv->pshare->rf_ft_var.disable_txpwrlmt)
				continue;

			printk(">>Band Edge Limit Table<<\n");

			tmp = ch2idx(channel);

			txpwr_lmt_CCK = priv->pshare->ch_pwr_lmtCCK[tmp];
			txpwr_lmt_OFDM = priv->pshare->ch_pwr_lmtOFDM[tmp];
			tgpwr_CCK = priv->pshare->ch_tgpwr_CCK[tmp];
			tgpwr_OFDM = priv->pshare->ch_tgpwr_OFDM[tmp];

			printk("txpwr_lmt_CCK = %d tgpwr_CCK = %d\n", txpwr_lmt_CCK,  tgpwr_CCK);
			printk("txpwr_lmt_OFDM = %d tgpwr_OFDM = %d\n", txpwr_lmt_OFDM,  tgpwr_OFDM);

			if (priv->pshare->is_40m_bw == 0)
			{
				txpwr_lmt_HT1S = priv->pshare->ch_pwr_lmtHT20_1S[tmp];
				txpwr_lmt_HT2S = priv->pshare->ch_pwr_lmtHT20_2S[tmp];
				tgpwr_HT1S = priv->pshare->ch_tgpwr_HT20_1S[tmp];
				tgpwr_HT2S = priv->pshare->ch_tgpwr_HT20_2S[tmp];

				printk("txpwr_lmt_HT1S_20M = %d tgpwr_HT1S_20M = %d\n", txpwr_lmt_HT1S,  tgpwr_HT1S);
				printk("txpwr_lmt_HT2S_20M = %d tgpwr_HT2S_20M = %d\n", txpwr_lmt_HT2S,  tgpwr_HT2S);
			}
			else
			{
				txpwr_lmt_HT1S = priv->pshare->ch_pwr_lmtHT40_1S[tmp];
				txpwr_lmt_HT2S = priv->pshare->ch_pwr_lmtHT40_2S[tmp];
				tgpwr_HT1S = priv->pshare->ch_tgpwr_HT40_1S[tmp];
				tgpwr_HT2S = priv->pshare->ch_tgpwr_HT40_2S[tmp];

				printk("txpwr_lmt_HT1S_40M = %d tgpwr_HT1S_40M = %d\n", txpwr_lmt_HT1S,  tgpwr_HT1S);
				printk("txpwr_lmt_HT2S_40M = %d tgpwr_HT2S_40M = %d\n", txpwr_lmt_HT2S,  tgpwr_HT2S);
			}

			//===Count FLASH + min{POWER BY RATE, LIMIT}===
			if((txpwr_lmt_OFDM == 0) && (txpwr_lmt_HT1S == 0) && (txpwr_lmt_HT1S == 0))
			{
				printk("No Band Edge Limit for this channel=%d\n", channel);
				continue;
			}

			if (!txpwr_lmt_OFDM || !tgpwr_OFDM){
				max_idx=255;
			}else{
				max_idx = (txpwr_lmt_OFDM - tgpwr_OFDM);
			}

			for (i=0; i<=7; i++) {
				OFDMTxAgcOffset_A[i] = POWER_MIN_CHECK(OFDMTxAgcOffset_A[i], max_idx);
				OFDMTxAgcOffset_B[i] = POWER_MIN_CHECK(OFDMTxAgcOffset_B[i], max_idx);
			}

			if (!txpwr_lmt_HT1S || !tgpwr_HT1S){
				max_idx = 255;
			}else{
				max_idx = (txpwr_lmt_HT1S - tgpwr_HT1S);
			}
			
			for (i=0; i<=7; i++) {
				MCSTxAgcOffset_A[i] = POWER_MIN_CHECK(MCSTxAgcOffset_A[i], max_idx);
				MCSTxAgcOffset_B[i] = POWER_MIN_CHECK(MCSTxAgcOffset_B[i], max_idx);
			}
			
			if (!txpwr_lmt_HT2S || !tgpwr_HT2S){
				max_idx = 255;
			}else{
				max_idx = (txpwr_lmt_HT2S - tgpwr_HT2S);
			}

			for (i=8; i<=15; i++) {
				MCSTxAgcOffset_A[i] = POWER_MIN_CHECK(MCSTxAgcOffset_A[i], max_idx);
				MCSTxAgcOffset_B[i] = POWER_MIN_CHECK(MCSTxAgcOffset_B[i], max_idx);
			}

			dump_ofdm_mcs0(priv, defValue, pwrlevelHT40_1S_A, pwrlevelHT40_1S_B, 
				pwrdiffHT40_2S, pwrdiffHT20, pwrdiffOFDM, 
				pwrlevelHT40_1S_A_6dB, pwrlevelHT40_1S_B_6dB,
				pwrdiffHT40_2S_6dB, pwrdiffHT20_6dB,
				MCSTxAgcOffset_A, MCSTxAgcOffset_B, OFDMTxAgcOffset_A, OFDMTxAgcOffset_B, phyBandSelect);

#endif
			
		}
	}

}

#endif

#ifdef CONFIG_RTL_8812_SUPPORT
void reg_dump_8812(struct rtl8192cd_priv *priv)
{
		panic_printk("Initial Gain, Sensitivity:\n");
		panic_printk(" 0xC50: 0x%02x\n", RTL_R8(0xc50));
		panic_printk(" 0xC58: 0x%02x\n", RTL_R8(0xc58));
		panic_printk(" 0xC30: 0x%02x\n", RTL_R8(0xc30));
		panic_printk(" 0xC87: 0x%02x\n", RTL_R8(0xc87));
		panic_printk(" 0xA0A: 0x%02x\n", RTL_R8(0xa0a));
		
		panic_printk("EDCA para:\n");
		panic_printk(" VO(0x%03x): 0x%08x\n", EDCA_VO_PARA, RTL_R32(EDCA_VO_PARA));
		panic_printk(" VI(0x%03x): 0x%08x\n", EDCA_VI_PARA, RTL_R32(EDCA_VI_PARA));
		panic_printk(" BE(0x%03x): 0x%08x\n", EDCA_BE_PARA, RTL_R32(EDCA_BE_PARA));
		panic_printk(" BK(0x%03x): 0x%08x\n", EDCA_BK_PARA, RTL_R32(EDCA_BK_PARA));
		
		panic_printk("Tx power:\n");
		panic_printk(" A_CCK11_CCK1(0x%03x):      0x%08x\n", rTxAGC_A_CCK11_CCK1_JAguar, RTL_R32(rTxAGC_A_CCK11_CCK1_JAguar));
		panic_printk(" A_Rate18_06(0x%03x):       0x%08x\n", rTxAGC_A_Ofdm18_Ofdm6_JAguar, RTL_R32(rTxAGC_A_Ofdm18_Ofdm6_JAguar));
		panic_printk(" A_Rate54_24(0x%03x):       0x%08x\n", rTxAGC_A_Ofdm54_Ofdm24_JAguar, RTL_R32(rTxAGC_A_Ofdm54_Ofdm24_JAguar));
		panic_printk(" A_Mcs03_Mcs00(0x%03x):     0x%08x\n", rTxAGC_A_MCS3_MCS0_JAguar, RTL_R32(rTxAGC_A_MCS3_MCS0_JAguar));
		panic_printk(" A_Mcs07_Mcs04(0x%03x):     0x%08x\n", rTxAGC_A_MCS7_MCS4_JAguar, RTL_R32(rTxAGC_A_MCS7_MCS4_JAguar));
		panic_printk(" A_Mcs11_Mcs08(0x%03x):     0x%08x\n", rTxAGC_A_MCS11_MCS8_JAguar, RTL_R32(rTxAGC_A_MCS11_MCS8_JAguar));
		panic_printk(" A_Mcs15_Mcs12(0x%03x):     0x%08x\n", rTxAGC_A_MCS15_MCS12_JAguar, RTL_R32(rTxAGC_A_MCS15_MCS12_JAguar));
		panic_printk(" A_Nss13_Nss10(0x%03x):     0x%08x\n", rTxAGC_A_Nss1Index3_Nss1Index0_JAguar, RTL_R32(rTxAGC_A_Nss1Index3_Nss1Index0_JAguar));
		panic_printk(" A_Nss17_Nss14(0x%03x):     0x%08x\n", rTxAGC_A_Nss1Index7_Nss1Index4_JAguar, RTL_R32(rTxAGC_A_Nss1Index7_Nss1Index4_JAguar));
		panic_printk(" A_Nss21_Nss18(0x%03x):     0x%08x\n", rTxAGC_A_Nss2Index1_Nss1Index8_JAguar, RTL_R32(rTxAGC_A_Nss2Index1_Nss1Index8_JAguar));
		panic_printk(" A_Nss25_Nss22(0x%03x):     0x%08x\n", rTxAGC_A_Nss2Index5_Nss2Index2_JAguar, RTL_R32(rTxAGC_A_Nss2Index5_Nss2Index2_JAguar));
		panic_printk(" A_Nss29_Nss26(0x%03x):     0x%08x\n", rTxAGC_A_Nss2Index9_Nss2Index6_JAguar, RTL_R32(rTxAGC_A_Nss2Index9_Nss2Index6_JAguar));
		panic_printk(" B_CCK11_CCK1(0x%03x):      0x%08x\n", rTxAGC_B_CCK11_CCK1_JAguar, RTL_R32(rTxAGC_B_CCK11_CCK1_JAguar));
		panic_printk(" B_Rate18_06(0x%03x):       0x%08x\n", rTxAGC_B_Ofdm18_Ofdm6_JAguar, RTL_R32(rTxAGC_B_Ofdm18_Ofdm6_JAguar));
		panic_printk(" B_Rate54_24(0x%03x):       0x%08x\n", rTxAGC_B_Ofdm54_Ofdm24_JAguar, RTL_R32(rTxAGC_B_Ofdm54_Ofdm24_JAguar));
		panic_printk(" B_Mcs03_Mcs00(0x%03x):     0x%08x\n", rTxAGC_B_MCS3_MCS0_JAguar, RTL_R32(rTxAGC_B_MCS3_MCS0_JAguar));
		panic_printk(" B_Mcs07_Mcs04(0x%03x):     0x%08x\n", rTxAGC_B_MCS7_MCS4_JAguar, RTL_R32(rTxAGC_B_MCS7_MCS4_JAguar));
		panic_printk(" B_Mcs11_Mcs08(0x%03x):     0x%08x\n", rTxAGC_B_MCS11_MCS8_JAguar, RTL_R32(rTxAGC_B_MCS11_MCS8_JAguar));
		panic_printk(" B_Mcs15_Mcs12(0x%03x):     0x%08x\n", rTxAGC_B_MCS15_MCS12_JAguar, RTL_R32(rTxAGC_B_MCS15_MCS12_JAguar));
		panic_printk(" B_Nss13_Nss10(0x%03x):     0x%08x\n", rTxAGC_B_Nss1Index3_Nss1Index0_JAguar, RTL_R32(rTxAGC_B_Nss1Index3_Nss1Index0_JAguar));
		panic_printk(" B_Nss17_Nss14(0x%03x):     0x%08x\n", rTxAGC_B_Nss1Index7_Nss1Index4_JAguar, RTL_R32(rTxAGC_B_Nss1Index7_Nss1Index4_JAguar));
		panic_printk(" B_Nss21_Nss18(0x%03x):     0x%08x\n", rTxAGC_B_Nss2Index1_Nss1Index8_JAguar, RTL_R32(rTxAGC_B_Nss2Index1_Nss1Index8_JAguar));
		panic_printk(" B_Nss25_Nss22(0x%03x):     0x%08x\n", rTxAGC_B_Nss2Index5_Nss2Index2_JAguar, RTL_R32(rTxAGC_B_Nss2Index5_Nss2Index2_JAguar));
		panic_printk(" B_Nss29_Nss26(0x%03x):     0x%08x\n", rTxAGC_B_Nss2Index9_Nss2Index6_JAguar, RTL_R32(rTxAGC_B_Nss2Index9_Nss2Index6_JAguar));
		return;
}
#endif

static void reg_dump(struct rtl8192cd_priv *priv, char *str)
{
	int i, j, len;
	unsigned char tmpbuf[100];

	panic_printk("[Channel-%03d]", priv->pmib->dot11RFEntry.dot11channel);

			if (priv->pshare->is_40m_bw == 0)
			{
		panic_printk(" - 20M BW");
			}
			else
			{
		if (priv->pshare->is_40m_bw == 1)
			panic_printk(" - 40M BW ");
		else
			panic_printk(" - 80M BW ");
				
				if (priv->pshare->offset_2nd_chan == 1)
			panic_printk("BELOW");
				else if (priv->pshare->offset_2nd_chan == 2)
			panic_printk("ABOVE");
				else if (priv->pshare->offset_2nd_chan == 0)
			panic_printk("DONT CARE");
			}

	panic_printk("\n");

#ifdef _DEBUG_RTL8192CD_

	if (strcmp(str, "tx") == 0)
	{
		printk("\n==2G L 1-3==\n");
		txpwr_dump(priv, 1, 1);
		printk("\n==2G M 4-9==\n");
		txpwr_dump(priv, 4, 4);
		printk("\n==2G H 10-14==\n");
		txpwr_dump(priv, 10, 10);


#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8192D) {
			printk("\n==5G G1_L 36-45==\n");
			txpwr_dump(priv, 36, 36);
			printk("\n==5G G1_M 46-55==\n");
			txpwr_dump(priv, 46, 46);
			printk("\n==5G G1_H 56-99==\n");
			txpwr_dump(priv, 56, 56);
			printk("\n==5G G2_L 100-113==\n");
			txpwr_dump(priv, 100, 100);
			printk("\n==5G G2_M 114-127==\n");
			txpwr_dump(priv, 114, 114);
			printk("\n==5G G2_H 128-148==\n");
			txpwr_dump(priv, 128, 128);
			printk("\n==5G G3_L 149-154==\n");
			txpwr_dump(priv, 149, 149);
			printk("\n==5G G3_M 155-160==\n");
			txpwr_dump(priv, 155, 155);
			printk("\n==5G G3_H 161-165==\n");
			txpwr_dump(priv, 161, 161);
		}
#endif

		return;
	}


	if (strcmp(str, "tx-2g") == 0)
	{
			printk("\n==2G L LIST==\n");
			txpwr_dump(priv, 1, 3);
			printk("\n==2G M LIST==\n");
			txpwr_dump(priv, 4, 9);
			printk("\n==2G H LIST==\n");
			txpwr_dump(priv, 10, 14);

		return;
	}

	if (strcmp(str, "tx-5gl") == 0)
	{
#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8192D) {	
			printk("\n==5G G1_L LIST==\n");
			txpwr_dump(priv, 36, 45);
			printk("\n==5G G1_M LIST==\n");
			txpwr_dump(priv, 46, 55);
			printk("\n==5G G1_H LIST==\n");
			txpwr_dump(priv, 56, 66); //99 > 66, Because dump too much will cause reboot
		} else
#endif
		{
			printk("NOT 92D, NOT support 5G\n");
		}

		return;
	}

	if (strcmp(str, "tx-5gm") == 0)
	{
#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8192D) {
			printk("\n==5G G2_L LIST==\n");
			txpwr_dump(priv, 100, 113);
			printk("\n==5G G2_M LIST==\n");
			txpwr_dump(priv, 114, 127);
			printk("\n==5G G2_H LIST==\n");
			txpwr_dump(priv, 128, 148);
		} else
#endif
		{
			printk("NOT 92D, NOT support 5G\n");
		}

		return;
	}

	if (strcmp(str, "tx-5gh") == 0)
	{

#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8192D) {
			printk("\n==5G G3_L LIST==\n");
			txpwr_dump(priv, 149, 154);
			printk("\n==5G G3_M LIST==\n");
			txpwr_dump(priv, 155, 160);
			printk("\n==5G G3_H LIST==\n");
			txpwr_dump(priv, 161, 165);
		} else
#endif
		{
			printk("NOT 92D, NOT support 5G\n");
		}
			
		return;
	}
#endif


#ifdef CONFIG_RTL_8812_SUPPORT
	if (strcmp(str, "check") == 0)
	{
			return;
	}

#endif

	if (strcmp(str, "all") != 0) {

#ifdef CONFIG_RTL_8812_SUPPORT
		if (GET_CHIP_VER(priv) == VERSION_8812E) {
			reg_dump_8812(priv);
			return;
		}
#endif
		panic_printk("Initial Gain, Sensitivity:\n");
		panic_printk(" 0xC50: 0x%02x\n", RTL_R8(0xc50));
#if defined(CONFIG_RTL_92C_SUPPORT) || defined(CONFIG_RTL_92D_SUPPORT)
		if (
#ifdef CONFIG_RTL_92C_SUPPORT
			(GET_CHIP_VER(priv)==VERSION_8192C) || (GET_CHIP_VER(priv)==VERSION_8188C)
#endif
#ifdef CONFIG_RTL_92D_SUPPORT
#ifdef CONFIG_RTL_92C_SUPPORT
			|| 
#endif
			(GET_CHIP_VER(priv)==VERSION_8192D)
#endif
			)
			panic_printk(" 0xC58: 0x%02x\n", RTL_R8(0xc58));
#endif
		panic_printk(" 0xC30: 0x%02x\n", RTL_R8(0xc30));
		panic_printk(" 0xC87: 0x%02x\n", RTL_R8(0xc87));
		panic_printk(" 0xA0A: 0x%02x\n", RTL_R8(0xa0a));
		panic_printk("EDCA para:\n");
		panic_printk(" VO(0x%03x): 0x%08x\n", EDCA_VO_PARA, RTL_R32(EDCA_VO_PARA));
		panic_printk(" VI(0x%03x): 0x%08x\n", EDCA_VI_PARA, RTL_R32(EDCA_VI_PARA));
		panic_printk(" BE(0x%03x): 0x%08x\n", EDCA_BE_PARA, RTL_R32(EDCA_BE_PARA));
		panic_printk(" BK(0x%03x): 0x%08x\n", EDCA_BK_PARA, RTL_R32(EDCA_BK_PARA));
		panic_printk("Tx power:\n");
		panic_printk(" A_CCK1_Mcs32(0x%03x):      0x%08x\n", rTxAGC_A_CCK1_Mcs32, RTL_R32(rTxAGC_A_CCK1_Mcs32));
		panic_printk(" B_CCK5_1_Mcs32(0x%03x):    0x%08x\n", rTxAGC_B_CCK5_1_Mcs32, RTL_R32(rTxAGC_B_CCK5_1_Mcs32));
		panic_printk(" A_CCK11_2_B_CCK11(0x%03x): 0x%08x\n", rTxAGC_A_CCK11_2_B_CCK11, RTL_R32(rTxAGC_A_CCK11_2_B_CCK11));
		panic_printk(" A_Rate18_06(0x%03x):       0x%08x\n", rTxAGC_A_Rate18_06, RTL_R32(rTxAGC_A_Rate18_06));
		panic_printk(" A_Rate54_24(0x%03x):       0x%08x\n", rTxAGC_A_Rate54_24, RTL_R32(rTxAGC_A_Rate54_24));
		panic_printk(" A_Mcs03_Mcs00(0x%03x):     0x%08x\n", rTxAGC_A_Mcs03_Mcs00, RTL_R32(rTxAGC_A_Mcs03_Mcs00));
		panic_printk(" A_Mcs07_Mcs04(0x%03x):     0x%08x\n", rTxAGC_A_Mcs07_Mcs04, RTL_R32(rTxAGC_A_Mcs07_Mcs04));
		panic_printk(" A_Mcs11_Mcs08(0x%03x):     0x%08x\n", rTxAGC_A_Mcs11_Mcs08, RTL_R32(rTxAGC_A_Mcs11_Mcs08));
		panic_printk(" A_Mcs15_Mcs12(0x%03x):     0x%08x\n", rTxAGC_A_Mcs15_Mcs12, RTL_R32(rTxAGC_A_Mcs15_Mcs12));
		panic_printk(" B_Rate18_06(0x%03x):       0x%08x\n", rTxAGC_B_Rate18_06, RTL_R32(rTxAGC_B_Rate18_06));
		panic_printk(" B_Rate54_24(0x%03x):       0x%08x\n", rTxAGC_B_Rate54_24, RTL_R32(rTxAGC_B_Rate54_24));
		panic_printk(" B_Mcs03_Mcs00(0x%03x):     0x%08x\n", rTxAGC_B_Mcs03_Mcs00, RTL_R32(rTxAGC_B_Mcs03_Mcs00));
		panic_printk(" B_Mcs07_Mcs04(0x%03x):     0x%08x\n", rTxAGC_B_Mcs07_Mcs04, RTL_R32(rTxAGC_B_Mcs07_Mcs04));
		panic_printk(" B_Mcs11_Mcs08(0x%03x):     0x%08x\n", rTxAGC_B_Mcs11_Mcs08, RTL_R32(rTxAGC_B_Mcs11_Mcs08));
		panic_printk(" B_Mcs15_Mcs12(0x%03x):     0x%08x\n", rTxAGC_B_Mcs15_Mcs12, RTL_R32(rTxAGC_B_Mcs15_Mcs12));
		return;
	}

#if 0 //  defined(CONFIG_RTL865X_WTDOG) || (defined(CONFIG_RTL_WTDOG) && defined(CONFIG_RTL_92D_SUPPORT))
	static unsigned long wtval;
	wtval = *((volatile unsigned long *)0xB800311C);
	*((volatile unsigned long *)0xB800311C) = 0xA5000000;	// disabe watchdog
#endif

	panic_printk("\nMAC Registers:\n");
	for (i=0; i<0x1000; i+=0x10) {
		len = sprintf((char *)tmpbuf, "%03X\t", i);
		for (j=i; j<i+0x10; j+=4)
			len += sprintf((char *)(tmpbuf+len), "%08X ", (unsigned int)RTL_R32(j));
		len += sprintf((char *)(tmpbuf+len), "\n");
		panic_printk((char *)tmpbuf);
	}
	panic_printk("\n");

	panic_printk("\nRF Registers:\n");
	len = 0;

	{
		unsigned int rf_reg_offset = 0x34; /* RTL8192C/RTL8188R */
#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv) == VERSION_8192D)
			rf_reg_offset = 0x50;
#endif
#ifdef CONFIG_RTL_88E_SUPPORT
		if (GET_CHIP_VER(priv) == VERSION_8188E)
			rf_reg_offset = 0xff;
#endif
#ifdef CONFIG_RTL_8812_SUPPORT
		if (GET_CHIP_VER(priv) == VERSION_8812E)
			rf_reg_offset = 0xff;
#endif


		for (i = RF92CD_PATH_A; i < priv->pshare->phw->NumTotalRFPath; i++) {
			for (j = 0; j <= rf_reg_offset; j++) {
				len += sprintf((char *)(tmpbuf+len), "%d%02x  %05x",
					i, j, PHY_QueryRFReg(priv, i, j, bMask20Bits, 1));

				if (j && !((j+1)%4))
					len += sprintf((char *)(tmpbuf+len), "\n");
				else
					len += sprintf((char *)(tmpbuf+len), "     ");
				panic_printk((char *)tmpbuf);
				len = 0;
			}
			panic_printk("\n");
		}
	}

#if 0 //defined(CONFIG_RTL865X_WTDOG) || (defined(CONFIG_RTL_WTDOG) && defined(CONFIG_RTL_92D_SUPPORT))
	*((volatile unsigned long *)0xB800311C) |=  1 << 23;
	*((volatile unsigned long *)0xB800311C) = wtval;
#endif
}
#endif // _IOCTL_DEBUG_CMD_


/*
 * data: byte 0 ~ byte 11 is mac addr
 *       if byte 12 & 13 is "no", will NOT send disasoc request
 *       else will send disasoc request
 */
int del_sta(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct stat_info *pstat;
	unsigned char macaddr[MACADDRLEN], tmpbuf[3];
	unsigned long flags;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	int i, send_disasoc=1;

	if (!netif_running(priv->dev))
		return 0;

	for(i=0; i<MACADDRLEN; i++)
	{
		tmpbuf[0] = data[2*i];
		tmpbuf[1] = data[2*i+1];
		tmpbuf[2] = 0;
		macaddr[i] = (unsigned char)_atoi((char *)tmpbuf, 16);
	}

	if ((data[12] == 'n') && (data[13] == 'o'))
		send_disasoc = 0;

#ifdef CONFIG_RTK_MESH
	// following add by chuangch 2007/08/30 (IAPP update proxy table)
	//by pepsi 08/03/12 for using PU
	if (( GET_MIB(priv)->dot1180211sInfo.mesh_enable == 1)	// fix: 0000107 2008/10/13
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID) // Spare for Mesh work with Multiple AP (Please see Mantis 0000107 for detail)
			&& IS_ROOT_INTERFACE(priv)
#endif
	)
		HASH_DELETE(priv->proxy_table, macaddr);

#endif	// CONFIG_RTK_MESH

	DEBUG_INFO("del_sta %02X%02X%02X%02X%02X%02X\n",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

	pstat = get_stainfo(priv, macaddr);

	if (pstat == NULL)
		return 0;

	if (!list_empty(&pstat->asoc_list))
	{
		if (IEEE8021X_FUN)
		{
#ifndef WITHOUT_ENQUEUE
			memcpy((void *)Disassociation_Ind.MACAddr, (void *)macaddr, MACADDRLEN);
			Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
			Disassociation_Ind.IsMoreEvent = 0;
			Disassociation_Ind.Reason = _STATS_OTHER_;
			Disassociation_Ind.tx_packets = pstat->tx_pkts;
			Disassociation_Ind.rx_packets = pstat->rx_pkts;
			Disassociation_Ind.tx_bytes   = pstat->tx_bytes;
			Disassociation_Ind.rx_bytes   = pstat->rx_bytes;
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
						sizeof(DOT11_DISASSOCIATION_IND));
#endif
#if defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD)
			psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, macaddr, NULL, 0);
#endif

#ifdef WIFI_HAPD
			event_indicate_hapd(priv, macaddr, HAPD_EXIRED, NULL);
#ifdef HAPD_DRV_PSK_WPS
			event_indicate(priv, macaddr, 2);
#endif
#else
			event_indicate(priv, macaddr, 2);
#endif
		}

		if (send_disasoc)
			issue_disassoc(priv, macaddr, _RSON_UNSPECIFIED_);

		if (pstat->expire_to > 0)
		{
			SAVE_INT_AND_CLI(flags);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);

			LOG_MSG("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",
				macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
		}
	}

	free_stainfo(priv, pstat);

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
		start_clnt_lookup(priv, 0);
	}
#endif

	return 1;
}

#ifdef FREDDY	//mesh related
int del_sta_enc(struct rtl8192cd_priv *priv, unsigned char *data,int iStatusCode)
{
	struct stat_info *pstat;
	unsigned char macaddr[MACADDRLEN], tmpbuf[3];
	unsigned long flags;
	DOT11_DEAUTHENTICATION_IND Deauthentication_Ind;
	int i;

	if (!netif_running(priv->dev))
		return 0;

	for(i=0; i<MACADDRLEN; i++)
	{
		tmpbuf[0] = data[2*i];
		tmpbuf[1] = data[2*i+1];
		tmpbuf[2] = 0;
		macaddr[i] = (unsigned char)_atoi((char *)tmpbuf, 16);
	}

#ifdef CONFIG_RTK_MESH
	// following add by chuangch 2007/08/30 (IAPP update proxy table)
	//by pepsi 08/03/12 for using PU
	if ((1 == GET_MIB(priv)->dot1180211sInfo.mesh_enable)	// fix: 0000107 2008/10/13
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID) // Spare for Mesh work with Multiple AP (Please see Mantis 0000107 for detail)
			&& IS_ROOT_INTERFACE(priv)
#endif
	)
		HASH_DELETE(priv->proxy_table, macaddr);

#endif	// CONFIG_RTK_MESH

	DEBUG_INFO("del_sta %02X%02X%02X%02X%02X%02X\n",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	pstat = get_stainfo(priv, macaddr);
	if (pstat == NULL)
		return 0;

	if (!list_empty(&pstat->asoc_list))
	{
		if (IEEE8021X_FUN)
		{
#ifndef WITHOUT_ENQUEUE
			memcpy((void *)Deauthentication_Ind.MACAddr, (void *)macaddr, MACADDRLEN);
			Deauthentication_Ind.EventId = DOT11_EVENT_DEAUTHENTICATION_IND;
			Deauthentication_Ind.IsMoreEvent = 0;
			Deauthentication_Ind.Reason = iStatusCode;
			Deauthentication_Ind.tx_packets = pstat->tx_pkts;
			Deauthentication_Ind.rx_packets = pstat->rx_pkts;
			Deauthentication_Ind.tx_bytes   = pstat->tx_bytes;
			Deauthentication_Ind.rx_bytes   = pstat->rx_bytes;
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Deauthentication_Ind,
						sizeof(DOT11_DEAUTHENTICATION_IND));
#endif
#if defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD)
			psk_indicate_evt(priv, DOT11_EVENT_DEAUTHENTICATION_IND, macaddr, NULL, 0);
#endif

#ifdef WIFI_HAPD
			event_indicate_hapd(priv, macaddr, HAPD_EXIRED, NULL);
#ifdef HAPD_DRV_PSK_WPS
			event_indicate(priv, macaddr, 2);
#endif
#else
			event_indicate(priv, macaddr, 2);
#endif
		}

		issue_deauth(priv, macaddr, iStatusCode);
		if (pstat->expire_to > 0)
		{
			printk("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
			SAVE_INT_AND_CLI(flags);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);

			LOG_MSG("A STA is deleted by application program - %02X:%02X:%02X:%02X:%02X:%02X\n",
				macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
		}
	}
	free_stainfo(priv, pstat);

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE) {
		OPMODE &= ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
		start_clnt_lookup(priv, 0);
	}
#endif

	return 1;
}
#endif

static int write_eeprom(struct rtl8192cd_priv *priv, unsigned char *data)
{
	return -1;
}


static int read_eeprom(struct rtl8192cd_priv *priv, unsigned char *data)
{
	return -1;
}


static void get_sta_info(struct rtl8192cd_priv *priv, sta_info_2_web *pInfo, int size
#ifdef RTK_WOW
				,unsigned int wakeup_on_wlan
#endif
				)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	struct net_device *dev = priv->dev;
	int i;

	memset((char *)pInfo, '\0', sizeof(sta_info_2_web)*size);

	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead))
		return;

	plist = phead->next;
	while ((plist != phead) && (size > 0))
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;
		if ((pstat->state & WIFI_ASOC_STATE) &&
			((pstat->expire_to > 0)
#ifdef RTK_WOW
			|| wakeup_on_wlan
#endif
			)
#ifdef CONFIG_RTK_MESH
			&& (!(GET_MIB(priv)->dot1180211sInfo.mesh_enable) || isSTA(pstat))	// STA ONLY
#endif
#ifdef RTK_WOW
			&& (!wakeup_on_wlan || pstat->is_rtk_wow_sta)
#endif
#ifdef WDS
			&& !(pstat->state & WIFI_WDS)
#endif
		) {
			pInfo->aid = pstat->aid;
			memcpy(pInfo->addr, pstat->hwaddr, 6);
			pInfo->tx_packets = pstat->tx_pkts;
			pInfo->rx_packets = pstat->rx_pkts;
			pInfo->expired_time = pstat->expire_to * 100; /*1s to 10ms unit*/
			pInfo->flags = STA_INFO_FLAG_ASOC;
			if (!list_empty(&pstat->sleep_list))
				pInfo->flags |= STA_INFO_FLAG_ASLEEP;
			pInfo->TxOperaRate = pstat->current_tx_rate;
			pInfo->RxOperaRate = pstat->rx_rate;
#ifdef TLN_STATS
			pInfo->enc_type = pstat->dot11KeyMapping.dot11Privacy;

			if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK) {
				if (pstat->wpa_sta_info->RSNEnabled & PSK_WPA2)
					pInfo->auth_type = STATS_PSK_WPA2;
				else 
					pInfo->auth_type = STATS_PSK_WPA;
			} else if (IEEE8021X_FUN && ((pstat->dot11KeyMapping.dot11Privacy != _WEP_40_PRIVACY_) && 
				(pstat->dot11KeyMapping.dot11Privacy != _WEP_104_PRIVACY_))) {
				if (pstat->enterpise_wpa_info == STATS_ETP_WPA2)
					pInfo->auth_type = STATS_ETP_WPA2;
				else 
					pInfo->auth_type = STATS_ETP_WPA;
			} else {
				 if (pstat->AuthAlgrthm)
				 	pInfo->auth_type = STATS_AUTH_SHARE;
				 else
				 	pInfo->auth_type = STATS_AUTH_OPEN;
			}
#endif
			pInfo->rssi = pstat->rssi;
			pInfo->sq = pstat->sq; // Alpha add
			pInfo->link_time = pstat->link_time;
			pInfo->tx_fail = pstat->tx_fail;
			pInfo->tx_bytes = pstat->tx_bytes;
			pInfo->rx_bytes = pstat->rx_bytes;

			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
				pInfo->network = WIRELESS_11A;
			else if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
				if (!isErpSta(pstat))
					pInfo->network = WIRELESS_11B;
				else {
					pInfo->network = WIRELESS_11G;
					for (i=0; i<STAT_OPRATE_LEN; i++) {
						if (is_CCK_rate(STAT_OPRATE[i])) {
							pInfo->network |= WIRELESS_11B;
							break;
						}
					}
				}
			}
			else // 11B only
				pInfo->network = WIRELESS_11B;
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				if (pstat->ht_cap_len) {
					pInfo->network |= WIRELESS_11N;
					if (pstat->ht_current_tx_info & TX_USE_40M_MODE)
						pInfo->ht_info |= TX_USE_40M_MODE;
					if (pstat->ht_current_tx_info & TX_USE_SHORT_GI)
						pInfo->ht_info |= TX_USE_SHORT_GI;
				}
			}
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)
			{
#ifdef CONFIG_RTL_8812_SUPPORT	
				if(pstat ->vht_cap_len) {
					if(pstat->tx_bw == HT_CHANNEL_WIDTH_80)
						pInfo->ht_info |= TX_USE_80M_MODE;
 					if (pstat->ht_current_tx_info & TX_USE_40M_MODE)
						pInfo->ht_info |= TX_USE_40M_MODE;
					if (pstat->ht_current_tx_info & TX_USE_SHORT_GI)
						pInfo->ht_info |= TX_USE_SHORT_GI;

					pInfo->acTxOperaRate = query_vht_rate(pstat);
					pInfo->network |= WIRELESS_11AC;
				}
#endif
			}
			pInfo++;
			size--;
		}
	}
}

#ifdef _SINUX_
/*
 *  return 1: if mac is wlan client MAC, 0: if not.
 */
int rtl8192cd_check_wlan_mac(char *wlan_ifname, unsigned char *mac)
{
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;

	struct list_head *phead, *plist;
	struct stat_info *pstat;
	int ret = 0;

    if (mac == NULL)
        return 0;

    net_dev = dev_get_by_name(wlan_ifname);
    if (net_dev == NULL)
    {
        printk("rtl8192cd_check_wlan_mac(): can not get dev %s\n", wlan_ifname);
        return 0;
    }
    priv = (struct rtl8192cd_priv *)net_dev -> priv;


	phead = &priv->asoc_list;
	if (!netif_running(net_dev) || list_empty(phead)){
		dev_put(net_dev);
		return 0;
	}

	plist = phead->next;
	while (plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;
		if ((pstat->state & WIFI_ASOC_STATE) && (pstat->expire_to > 0)
#ifdef WDS
			&& !(pstat->state & WIFI_WDS)
#endif
		) {
            if (memcmp(mac, pstat->hwaddr,6) == 0) {
                ret = 1;
                break;
            }
        }
	}

    dev_put(net_dev);

    return ret;
}

typedef struct tag_ASSOCIATE_TABLE{
    char mac[6];
    /* err: 0: no error 1: error */
    char err;
}ASSOCIATE_TABLE, *PASSOCIATE_TABLE;

int rtl8192cd_getMacTable(char *wlan_ifname,  ASSOCIATE_TABLE *mac_table, int table_num)
{
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;

	struct list_head *phead, *plist;
	struct stat_info *pstat;
	int ret = 0;


    net_dev = dev_get_by_name(wlan_ifname);
    if (net_dev == NULL)
    {
        printk("rtl8192cd_check_wlan_mac(): can not get dev %s\n", wlan_ifname);
        return -1;
    }
    priv = (struct rtl8192cd_priv *)net_dev -> priv;


	phead = &priv->asoc_list;
	if (!netif_running(net_dev) || list_empty(phead))
	{
		dev_put(net_dev);
		return -1;
	}

	plist = phead->next;
	while (plist != phead && table_num > 0 )
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;
		if ((pstat->state & WIFI_ASOC_STATE) && (pstat->expire_to > 0)
#ifdef WDS
			&& !(pstat->state & WIFI_WDS)
#endif
		) {
            memcpy(mac_table->mac, pstat->hwaddr, 6);
            mac_table ++;
            table_num --;
            ret ++;
        }
	}

    dev_put(net_dev);

    return ret;
}

#endif

static void get_bss_info(struct rtl8192cd_priv *priv, bss_info_2_web *pBss)
{
	struct net_device *dev = priv->dev;
#ifdef CLIENT_MODE
	struct stat_info *pstat;
#endif

	memset(pBss, '\0', sizeof(bss_info_2_web));

	if (!netif_running(dev)) {
		pBss->state = STATE_DISABLED;
		return;
	}

	if (priv->pmib->miscEntry.func_off) {
		pBss->state = STATE_DISABLED;
		return;
	}

	if (OPMODE & WIFI_AP_STATE)
		pBss->state = STATE_STARTED;
#ifdef CLIENT_MODE
	else {
		switch (priv->join_res) {
		case STATE_Sta_No_Bss:
			pBss->state = STATE_SCANNING;
			break;
		case STATE_Sta_Bss:
			if (IEEE8021X_FUN) {
				pstat = get_stainfo(priv, BSSID);
				if (pstat == NULL)
					return;
				if (pstat->ieee8021x_ctrlport)
					pBss->state = STATE_CONNECTED;
				else
					pBss->state = STATE_WAITFORKEY;
			}
			else
				pBss->state = STATE_CONNECTED;
			break;
		case STATE_Sta_Ibss_Active:
			pBss->state = STATE_CONNECTED;
			break;
		case STATE_Sta_Ibss_Idle:
			pBss->state = STATE_STARTED;
			break;
		default:
			pBss->state = STATE_SCANNING;
			break;
		}
	}
#endif

	if (priv->pmib->dot11StationConfigEntry.autoRate)
		pBss->txRate = find_rate(priv, NULL, 1, 0);
	else
		pBss->txRate = get_rate_from_bit_value(priv->pmib->dot11StationConfigEntry.fixedTxRate);
	memcpy(pBss->ssid, SSID, SSID_LEN);
	pBss->ssid[SSID_LEN] = '\0';
	if (OPMODE & WIFI_SITE_MONITOR)
		pBss->channel = priv->site_survey.ss_channel;
	else
		pBss->channel = priv->pmib->dot11RFEntry.dot11channel;

	if (pBss->state == STATE_STARTED || pBss->state == STATE_CONNECTED) {
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE))
			if (IS_DRV_OPEN(priv))
				memcpy(pBss->bssid, priv->pmib->dot11Bss.bssid, MACADDRLEN);
			else
				memset(pBss->bssid, '\0', MACADDRLEN);
		else
#endif
		memcpy(pBss->bssid, BSSID, MACADDRLEN);
#ifdef CLIENT_MODE
		if (priv->join_res == STATE_Sta_Bss) {
			pstat = get_stainfo(priv, BSSID);
			if (pstat) {
				pBss->rssi = pstat->rssi;
				pBss->sq = pstat->sq;
			}
		}
#endif
	}
	else {
		memset(pBss->bssid, '\0', MACADDRLEN);
		if (pBss->state == STATE_DISABLED)
			pBss->channel = 0;
	}
}


#ifdef WDS
static int get_wds_info(struct rtl8192cd_priv *priv, web_wds_info *pWds)
{
	int i, j=0;
	struct stat_info *pstat;

	memset(pWds, '\0', NUM_WDS*sizeof(web_wds_info));

	for (j=0, i=0; i<NUM_WDS && i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
		if (!netif_running(priv->wds_dev[i]))
			continue;

		memcpy(pWds[j].addr, priv->pmib->dot11WdsInfo.entry[i].macAddr, 6);

		pstat = get_stainfo(priv, pWds[j].addr);
		if(NULL == pstat)
			continue;

		pWds[j].state = STATE_WDS_ACTIVE;
		pWds[j].tx_packets = pstat->tx_pkts;
		pWds[j].rx_packets = pstat->rx_pkts;
		pWds[j].tx_errors = pstat->tx_fail;
		pWds[j].TxOperaRate = pstat->current_tx_rate;
		j++;
	}

	return (sizeof(web_wds_info)*j);
}
#endif // WDS


static int set_sta_txrate(struct rtl8192cd_priv *priv, struct _wlan_sta_rateset *rate_set)
{
	struct stat_info *pstat;

	if (!netif_running(priv->dev))
		return 0;

	pstat = get_stainfo(priv, rate_set->mac);
	if (pstat == NULL)
		return 0;
	if (!(pstat->state & WIFI_ASOC_STATE))
		return 0;
	if (priv->pmib->dot11StationConfigEntry.autoRate) {
		DEBUG_INFO("Auto rate turned on. Can't set rate\n");
		return 0;
	}

	pstat->current_tx_rate = rate_set->txrate;
	return 1;
}


#ifdef MICERR_TEST
static int issue_mic_err_pkt(struct rtl8192cd_priv *priv, unsigned char *cli_mac)
{
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;

	skb = dev_alloc_skb(64);
	if (skb != NULL) {
		skb->dev = priv->dev;
		pethhdr = (struct wlan_ethhdr_t *)(skb->data);
		if (!(OPMODE & WIFI_STATION_STATE)) {
			unsigned char null_mac[]={0,0,0,0,0,0};
			if (!memcmp(cli_mac, null_mac, MACADDRLEN)) {
				printk("Usage: iwpriv wlanx mic_error [cli_mac_addr]\n");
				return 0;
			}
			printk("%s() Send MIC error packet to %02X:%02X:%02X:%02X:%02X:%02X\n",__func__,cli_mac[0],cli_mac[1],cli_mac[2],cli_mac[3],cli_mac[4],cli_mac[5]);
			memcpy(pethhdr->daddr, cli_mac, MACADDRLEN);
		} else {
			printk("Send MIC error packet to AP...\n");
			memcpy(pethhdr->daddr, BSSID, MACADDRLEN);
		}
		memcpy(pethhdr->saddr, GET_MY_HWADDR, MACADDRLEN);
		pethhdr->type = 0x888e;

		memset(skb->data+WLAN_ETHHDR_LEN, 0xa5, 32);
		skb_put(skb, WLAN_ETHHDR_LEN+32);

		priv->micerr_flag = 1;
		if (rtl8192cd_start_xmit(skb, priv->dev))
			rtl_kfree_skb(priv, skb, _SKB_TX_);
	} else {
		printk("Can't allocate sk_buff\n");
	}
	return 0;
}


static int issue_mic_rpt_pkt(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;
	int format;
	unsigned char pattern[] = {0x01, 0x03, 0x00, 0x5f, 0xfe, 0x0d, 0x00, 0x00};

	if (!(OPMODE & WIFI_STATION_STATE)) {
		printk("Fail: not in client mode\n");
		return 0;
	}

	if (!strcmp(data, "xp"))
		format = 1;
	else if (!strcmp(data, "funk"))
		format = 2;
	else {
		printk("Usage: iwpriv wlanx mic_report {xp | funk}\n");
		return 0;
	}

	printk("Send MIC report (%s format) to AP...\n", (format==1)? "XP":"Funk");
	skb = dev_alloc_skb(64);
	if (skb != NULL)
	{
		skb->dev = priv->dev;
		pethhdr = (struct wlan_ethhdr_t *)(skb->data);
		memcpy(pethhdr->daddr, BSSID, MACADDRLEN);
		memcpy(pethhdr->saddr, GET_MY_HWADDR, MACADDRLEN);
		pethhdr->type = 0x888e;

		if (format == 2)
			pattern[5] = 0x0f;
		memcpy(skb->data+WLAN_ETHHDR_LEN, pattern, sizeof(pattern));
		skb_put(skb, WLAN_ETHHDR_LEN+sizeof(pattern));

		if (rtl8192cd_start_xmit(skb, priv->dev))
			rtl_kfree_skb(priv, skb, _SKB_TX_);
	}
	else
	{
		printk("Can't allocate sk_buff\n");
	}
	return 0;
}
#endif // MICERR_TEST


#if defined(D_ACL) || defined(MICERR_TEST) 	//mesh related

static int iwpriv_atoi(unsigned char *data, unsigned char *buf, int len)
{
	unsigned char tmpbuf[20] = {'\0'};
	unsigned char ascii_addr[12] = {'\0'};
	unsigned char hex_addr[6] = {'\0'};
	int i=0;

	if( (len - 1) == MACADDRLEN ){	//user send 6 byte mac address
		if(copy_from_user(buf, (void *)data, len))
			return -1;
		return 0;
	}
	else if( (len - 1) == MACADDRLEN*2 ){ //user send 12 byte mac string
		if(copy_from_user(tmpbuf, (void *)data, len))
			return -1;

		strcpy(ascii_addr, tmpbuf);
		strcpy(buf+MACADDRLEN,tmpbuf);

	  	for(i = 0; i < MACADDRLEN*2; i++){
        	   	if( '0' <= ascii_addr[i]  && ascii_addr[i] <= '9')
        			ascii_addr[i] -= 48;
               		else if( 'A' <= ascii_addr[i] && ascii_addr[i] <= 'F' )
                		ascii_addr[i] -= 55;
			else if( 'a' <= ascii_addr[i] && ascii_addr[i] <= 'f' )
				ascii_addr[i] -= 87;
	                printk("%d", ascii_addr[i]);
		}

                for(i = 0; i < MACADDRLEN*2; i+=2)
                	hex_addr[i>>1] = (ascii_addr[i] << 4) | (ascii_addr[i+1]);

		memcpy(buf,hex_addr,MACADDRLEN);
		DEBUG_INFO("in iwpriv_atoi function\n");
		return 0;
	}
	else{
		DEBUG_ERR("Wrong input format\n");
		return -1;
	}

}
#endif

static int acl_add_cmd(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	struct list_head		*phead, *plist, *pnewlist;
	struct wlan_acl_node	*paclnode;
	unsigned char macaddr[6];

	if (copy_from_user((void *)macaddr, (void *)data, 6))
		return -1;

#ifdef D_ACL	//tsananiu	(mesh related)
	unsigned char tmpbuf[20] = {'\0'};
	unsigned char tmp_add[12] = {'\0'};
	int i;
	if(copy_from_user(tmpbuf, (void *)data, len))
		return -1;

	if( (len - 1) == 6 ){	//user send 6 byte mac address
		for(i = 0; i < 6; i++)
			macaddr[i] = tmpbuf[i];
	}
	else if( (len - 1) == 12 ){ //user send 12 byte mac string

		strcpy(tmp_add, tmpbuf);

	  	for(i = 0; i < 12; i++){
        	   	if( '0' <= tmp_add[i]  && tmp_add[i] <= '9')
        			tmp_add[i] -= 48;
               		else if( 'A' <= tmp_add[i] && tmp_add[i] <= 'F' )
                		tmp_add[i] -= 55;
			else if( 'a' <= tmp_add[i] && tmp_add[i] <= 'f' )
				tmp_add[i] -= 87;
	                printk("%d", tmp_add[i]);
		}

                for(i = 0; i < 12; i+=2){
                	macaddr[i>>1] = (tmp_add[i] << 4) | (tmp_add[i+1]);
                }
	}

	else{
		printk("Wrong input format\n");
	}
	DEBUG_INFO("in add function\n");
	//len = 6;
#else
	if (copy_from_user((void *)macaddr, (void *)data, 6))
		return -1;
#endif//tsananiu//

	// first of all, check if this address has been in acl_list;
	phead = &priv->wlan_acl_list;
	plist = phead->next;

	DEBUG_INFO("Adding %X:%X:%X:%X:%X:%X to acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, 6)))
		{
			DEBUG_INFO("mac-addr %02X%02X%02X%02X%02X%02X has been in acl_list\n",
					macaddr[0], macaddr[1], macaddr[2],
					macaddr[3], macaddr[4], macaddr[5]);
			return 0;
		}
	}

	if (list_empty(&priv->wlan_aclpolllist))
	{
		DEBUG_INFO("acl_poll is full!\n");
		return 0;
	}

	pnewlist = (priv->wlan_aclpolllist.next);
	list_del_init(pnewlist);

	paclnode = list_entry(pnewlist, struct wlan_acl_node, list);

	memcpy((void *)paclnode->addr, macaddr, 6);

	if (len == 6)
		paclnode->mode = (unsigned char)priv->pmib->dot11StationConfigEntry.dot11AclMode;
	else
		paclnode->mode = data[6];

	list_add_tail(pnewlist, phead);

	return 0;
}


static int acl_remove_cmd(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	struct list_head *phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned char macaddr[6];

	if (data) {
#ifdef D_ACL	//tsananiu	(mesh related)
		int i;
		unsigned char tmpbuf[20] = {'\0'};
		unsigned char tmp_add[12] = {'\0'};

		if(copy_from_user(tmpbuf, (void *)data, len))
			return -1;

		if( (len - 1) == 6 ){	//user send 6 byte mac address
			for(i = 0; i < 6; i++)
				macaddr[i] = tmpbuf[i];
		}
		else if( (len - 1) == 12 ){ //user send 12 byte mac string

			strcpy(tmp_add, tmpbuf);

		  	for(i = 0; i < 12; i++){
	        	   	if( '0' <= tmp_add[i]  && tmp_add[i] <= '9')
	        			tmp_add[i] -= 48;
	               		else if( 'A' <= tmp_add[i] && tmp_add[i] <= 'F' )
	                		tmp_add[i] -= 55;
				else if( 'a' <= tmp_add[i] && tmp_add[i] <= 'f' )
					tmp_add[i] -= 87;
		                DEBUG_INFO("%d", tmp_add[i]);
			}

	                for(i = 0; i < 12; i+=2){
	                	macaddr[i>>1] = (tmp_add[i] << 4) | (tmp_add[i+1]);
	                }
		}
		else{
			DEBUG_ERR("Wrong input format\n");
			return -1;
		}
		DEBUG_INFO("in remove function\n");
		DEBUG_INFO("%X:%X:%X:%X:%X:%X\n",
	                macaddr[0],macaddr[1],macaddr[2],
	                macaddr[3],macaddr[4],macaddr[5]);
	//len = 6;
#else
		if (copy_from_user((void *)macaddr, (void *)data, 6))
			return -1;

		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X to acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);
#endif//tsananiu//
	}

	phead = &priv->wlan_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, 6)))
		{
			list_del_init(&paclnode->list);
			list_add_tail(&paclnode->list, &priv->wlan_aclpolllist);
			return 0;
		}
	}

	if (data) {
		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X is not in acl_table\n",
                macaddr[0],macaddr[1],macaddr[2],
                macaddr[3],macaddr[4],macaddr[5]);
	}
	return 0;
}


static int acl_query_cmd(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct list_head	*phead, *plist;
	struct wlan_acl_node	*paclnode;

	phead = &priv->wlan_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (copy_to_user((void *)data, paclnode->addr, 6))
			return -1;
		data += 6;
	}
	return 0;
}


#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
// Copy from acl_add_cmd
static int mesh_acl_add_cmd(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	struct list_head		*phead, *plist, *pnewlist;
	struct wlan_acl_node	*paclnode;
	unsigned char macaddr[MACADDRLEN];

	if (copy_from_user((void *)macaddr, (void *)data, MACADDRLEN))
		return -1;

	// first of all, check if this address has been in acl_list;
	phead = &priv->mesh_acl_list;
	plist = phead->next;

	DEBUG_INFO("Adding %X:%X:%X:%X:%X:%X to mesh_acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, MACADDRLEN)))
		{
			DEBUG_INFO("mac-addr %02X%02X%02X%02X%02X%02X has been in mesh_acl_list\n",
					macaddr[0], macaddr[1], macaddr[2],
					macaddr[3], macaddr[4], macaddr[5]);
			return 0;
		}
	}

	if (list_empty(&priv->mesh_aclpolllist))
	{
		DEBUG_INFO("mesh_acl_poll is full!\n");
		return 0;
	}

	pnewlist = (priv->mesh_aclpolllist.next);
	list_del_init(pnewlist);

	paclnode = list_entry(pnewlist, struct wlan_acl_node, list);

	memcpy((void *)paclnode->addr, macaddr, MACADDRLEN);

	if (len == 6)
		paclnode->mode = (unsigned char)priv->pmib->dot1180211sInfo.mesh_acl_mode;
	else
		paclnode->mode = data[6];

	list_add_tail(pnewlist, phead);

	return 0;
}


// Copy from acl_remove_cmd
static int mesh_acl_remove_cmd(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	struct list_head *phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned char macaddr[MACADDRLEN];

	if (data) {
		if (copy_from_user((void *)macaddr, (void *)data, 6))
			return -1;

		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X to mesh_acl_table\n",
				macaddr[0],macaddr[1],macaddr[2],
				macaddr[3],macaddr[4],macaddr[5]);
	}

	phead = &priv->mesh_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (!(memcmp((void *)macaddr, paclnode->addr, MACADDRLEN)))
		{
			list_del_init(&paclnode->list);
			list_add_tail(&paclnode->list, &priv->mesh_aclpolllist);
			return 0;
		}
	}

	if (data) {
		DEBUG_INFO("Delete %X:%X:%X:%X:%X:%X is not in mesh_acl_table\n",
                macaddr[0],macaddr[1],macaddr[2],
                macaddr[3],macaddr[4],macaddr[5]);
	}
	return 0;
}


// Copy from acl_query_cmd
static int mesh_acl_query_cmd(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct list_head	*phead, *plist;
	struct wlan_acl_node	*paclnode;

	phead = &priv->mesh_acl_list;

	if (list_empty(phead)) // nothing to remove
		return 0;

	plist = phead->next;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;

		if (copy_to_user((void *)data, paclnode->addr, 6))
			return -1;
		data += 6;
	}
	return 0;
}
#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_


static void get_misc_data(struct rtl8192cd_priv *priv, struct _misc_data_ *pdata)
{
	memset(pdata, '\0', sizeof(struct _misc_data_));

	pdata->mimo_tr_hw_support = GET_HW(priv)->MIMO_TR_hw_support;

	// get number of tx path
	if (get_rf_mimo_mode(priv) == MIMO_1T2R)
		pdata->mimo_tr_used = 1;
	else if (get_rf_mimo_mode(priv) == MIMO_1T1R)
		pdata->mimo_tr_used = 1;
	else if (get_rf_mimo_mode(priv) == MIMO_2T2R)
		pdata->mimo_tr_used = 2;
	else	// MIMO_2T4R
		pdata->mimo_tr_used = 2;

	return;
}


#ifdef AUTO_TEST_SUPPORT
static void rtl8192cd_SSReq_AutoTest(struct rtl8192cd_priv *priv)
{
	INT8 ret = 0;
	//int i1;
	//static int timerbeinit = 0;

	#ifdef CONFIG_RTK_MESH
	if((priv->auto_channel &0x30) && timer_pending(&priv->ss_timer))
		ret = -2;
	else
	#endif
	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
		ret = -1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey
	{
		priv->ss_ssidlen = 0;
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
		priv->ss_req_ongoing = 1;
		start_clnt_ss(priv);
	}else{
		return ;
	}
}


#ifdef CLIENT_MODE

#ifndef WIFI_WPAS
			static int check_bss_encrypt(struct rtl8192cd_priv *priv);
#endif

static int rtl8192cd_join_AutoTest(struct rtl8192cd_priv *priv, unsigned char *data)
{
	INT8 ret = 0;

	char tmpbuf[33];
	char SSID123[34];
	int ix  = 0;
	int found = 0 ;


	if (!netif_running(priv->dev))
		ret = 2;
	else if (priv->ss_req_ongoing)
		ret = 1;
	else
		ret = 0;

	if (copy_from_user((void *)SSID123, (void *)data, 33) ){
		panic_printk("copy SSID fail!!\n");
		return -1;
	}

	for(ix = 0 ; ix < priv->site_survey.count_backup ; ix++){
		if(!strcmp(priv->site_survey.bss_backup[ix].ssid , SSID123 )){
				found = 1;
				break;
		}
	}

	if(found == 0){
		ret = 3;
		panic_printk("SSID not found!!\n");
	}else{
		memcpy((void *)&(priv->pmib->dot11Bss) ,
		(void *)&priv->site_survey.bss_backup[ix] , sizeof(struct bss_desc));
	}




	if (!ret)	// now, let's start site survey and join
	{


#ifdef WIFI_SIMPLE_CONFIG
		if (priv->pmib->wscEntry.wsc_enable && (priv->pmib->dot11Bss.bsstype&WIFI_WPS)) {
			priv->pmib->dot11Bss.bsstype &= ~WIFI_WPS;
			priv->wps_issue_join_req = 1;
		}
		else
#endif
		{
			if (check_bss_encrypt(priv) == FAIL) {
				DEBUG_INFO("Encryption mismatch!\n");
				ret = 2;
				if (copy_to_user((void *)data, (void *)&ret, 1))
					return -1;
				else
					return 0;
			}
		}

		if ((priv->pmib->dot11Bss.ssidlen == 0) || (priv->pmib->dot11Bss.ssid[0] == '\0')) {
			DEBUG_INFO("Join to a hidden AP!\n");
			ret = 2;
			if (copy_to_user((void *)data, (void *)&ret, 1))
				return -1;
			else
				return 0;
		}

#ifdef UNIVERSAL_REPEATER
		disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

		memcpy(tmpbuf, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		tmpbuf[priv->pmib->dot11Bss.ssidlen] = '\0';
		DEBUG_INFO("going to join bss: %s\n", tmpbuf);

		panic_printk("going to join bss: %s\n", tmpbuf);

		memcpy(SSID2SCAN, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		SSID2SCAN_LEN = priv->pmib->dot11Bss.ssidlen;

		SSID_LEN = SSID2SCAN_LEN;
		memcpy(SSID, SSID2SCAN, SSID_LEN);
		memset(BSSID, 0, MACADDRLEN);

// button 2009.05.21
// derive  PSK with slelected SSID
#ifdef INCLUDE_WPA_PSK
		if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
			derivePSK(priv);
#endif
		priv->join_req_ongoing = 1;
		priv->authModeRetry = 0;
		start_clnt_join(priv);
	}

	return 0;
}
#endif
#endif


#ifndef CONFIG_RTL_COMAPI_WLTOOLS
static
#endif
int rtl8192cd_ss_req(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	INT8 ret = 0;
#ifdef CONFIG_RTK_MESH
	// by GANTOE for manual site survey 2008/12/25
	// inserted by Joule for simple channel unification protocol 2009/01/06
	if((priv->auto_channel &0x30) && timer_pending(&priv->ss_timer))
		ret = -2;
	else
#endif
	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
		ret = -1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey
	{
		priv->ss_ssidlen = 0;
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);

#ifdef WIFI_SIMPLE_CONFIG
		if (len == 2)
			priv->ss_req_ongoing = 2;	// WiFi-Simple-Config scan-req
		else
#endif
			priv->ss_req_ongoing = 1;
		start_clnt_ss(priv);
	}

#ifdef WIFI_WPAS //_Eric ??
	if (copy_to_user((void *)data, (void *)&ret, 1))
		memcpy(data, &ret, 1);
#else
	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;
#endif

	return 0;
}

static int rtl8192cd_autochannel_sel(struct rtl8192cd_priv *priv)
{
	INT8 ret = 0;

	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
		ret = -1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey
	{
		priv->ss_ssidlen = 0;
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
		priv->ss_req_ongoing = 1;
		priv->auto_channel = 1;
		start_clnt_ss(priv);
	}
	return ret;
}

static int rtl8192cd_get_ss_status(struct rtl8192cd_priv *priv, unsigned char *data)
{
	UINT8 flags;
	INT8 ret = 0;

	if (copy_from_user((void *)&flags, (void *)(data), 1))
		return -1;

	if (!netif_running(priv->dev) || priv->ss_req_ongoing)
	{
		ret = -1;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 1)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 0)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)data, (void *)&ret, 1))
			return -1;
		// now we should report data base.
		if (copy_to_user((void *)(data+4), priv->site_survey.bss_backup,
				sizeof(struct bss_desc)*priv->site_survey.count_backup))
			return -1;
	}

#ifdef WIFI_SIMPLE_CONFIG
	else if (flags == 2) { // get simple-config scan result, append WSC IE
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)data, (void *)&ret, 1))
			return -1;
		// now we should report data base.
		if (copy_to_user((void *)(data+4), priv->site_survey.ie_backup,
				sizeof(struct wps_ie_info)*priv->site_survey.count_backup))
			return -1;
	}
#endif

	return 0;
}
#ifdef P2P_SUPPORT

int p2p_get_p2pconnect_state(struct rtl8192cd_priv *priv, unsigned char *data)
{
	struct p2p_state_event	p2p_state_event_t;
	memset(&p2p_state_event_t , 0 ,sizeof(struct p2p_state_event));	
	
	if(priv->p2pPtr==NULL)
			return -1;	
		
	p2p_state_event_t.p2p_status = P2P_STATE;

	if(P2P_STATE == P2P_S_CLIENT_CONNECTED_DHCPC){

		P2P_DEBUG("Wlan driver report client is connected\n");
		P2P_DEBUG("Indicate web server to start udhcpc \n\n");		
		
		P2P_STATE = P2P_S_CLIENT_CONNECTED_DHCPC_done ;	// after web rdy get this state ; change it
		
	}else if(P2P_STATE == P2P_S_preGO2GO_DHCPD){

		P2P_DEBUG("now is GO mode\n");
		P2P_DEBUG("Indicate web server to start udhcpd ...\n\n");		
		P2P_STATE = P2P_S_preGO2GO_DHCPD_done ;	// after web rdy get this state ; change it
		
	}else if(P2P_STATE == P2P_S_back2dev){
		// indicate web server to reset to p2p device mode
		P2P_DEBUG("reinit by web server\n");
		P2P_STATE = P2P_S_IDLE;

	}	
	
	if (copy_to_user((void *)(data), (void *)&p2p_state_event_t, sizeof(struct p2p_state_event)))
		return -1;	

	return 0;

}

int p2p_get_event_state(struct rtl8192cd_priv *priv, unsigned char *data)
{
	int MethodCase=0;
	struct p2p_state_event	p2p_state_event_t;
	memset(&p2p_state_event_t , 0 ,sizeof(struct p2p_state_event));
	
	if(priv->p2pPtr==NULL)
			return -1;	
		
	if(P2P_STATE>=P2P_S_PROVI_TX_REQ && P2P_STATE<=P2P_S_NEGO_WAIT_CONF )
		p2p_state_event_t.p2p_status = 4;
	else if(P2P_STATE ==P2P_S_CLIENT_CONNECTED_DHCPC ||  P2P_STATE ==P2P_S_CLIENT_CONNECTED_DHCPC_done)
		p2p_state_event_t.p2p_status = 5;
	else if(P2P_STATE == P2P_S_preGO2GO_DHCPD ||  P2P_STATE ==P2P_S_preGO2GO_DHCPD_done)
		p2p_state_event_t.p2p_status = 6;	
	else if(P2P_STATE == P2P_S_back2dev )
		p2p_state_event_t.p2p_status = 7;
	else
		p2p_state_event_t.p2p_status = P2P_STATE;

	
	p2p_state_event_t.p2p_role = P2PMODE;	

	if(P2P_EVENT_INDICATE){
		
		p2p_state_event_t.p2p_event = P2P_EVENT_INDICATE;	
		
		if(P2P_EVENT_INDICATE == P2P_EVENT_RX_PROVI_REQ)
		{
			MethodCase = priv->p2pPtr->wsc_method_from_target_dev;

			switch(MethodCase){
				case CONFIG_METHOD_PIN:
				case CONFIG_METHOD_DISPLAY:  
					p2p_state_event_t.p2p_wsc_method = CONFIG_METHOD_DISPLAY;
					break;
	 	 	    case CONFIG_METHOD_PBC:
					p2p_state_event_t.p2p_wsc_method = CONFIG_METHOD_PBC;
					break;
			    case CONFIG_METHOD_KEYPAD:
					p2p_state_event_t.p2p_wsc_method = CONFIG_METHOD_KEYPAD;
					break;
			}
			
		}
		P2P_EVENT_INDICATE = 0 ;		
	}

	if (copy_to_user((void *)(data), (void *)&p2p_state_event_t, sizeof(struct p2p_state_event)))
		return -1;	

	return 0;
}


int p2p_wps_indicate_state(struct rtl8192cd_priv *priv, unsigned char *data)
{

	unsigned char flags;


	printk("(%s %d)\n\n\n",__FUNCTION__,__LINE__);	

	
  	if (copy_from_user((void *)&flags, (void *)(data), 1)){
		return -1;
  	}

	P2P_DEBUG("Report from wscd , WPS is %s\n\n\n", (flags==1?"success":"fail"));

	if(flags == GO_WPS_SUCCESS){
		if(P2PMODE == P2P_PRE_GO)
		{
			P2P_DEBUG("pre-GO mdoe ;rx Report from wscd ; WPS is done \n\n");	
			// mode change
			P2PMODE = P2P_TMP_GO ;

			// state change
			P2P_STATE = P2P_S_preGO2GO_DHCPD;

			/*build beacon P2P IE  ; 
			 when from (Pre go) switch to GO only need change beacon P2P IE*/ 			
			if(P2PMODE == P2P_TMP_GO || P2PMODE == P2P_PRE_GO )
				priv->p2pPtr->p2p_beacon_ie_len = 
					p2p_build_beacon_ie(priv,priv->p2pPtr->p2p_beacon_ie);
	
		}else if(P2PMODE == P2P_PRE_CLIENT){
			P2P_DEBUG("Pre-Client mdoe ;Report from wscd ; WPS is  done \n\n");			
			P2PMODE = P2P_CLIENT ;
		}
	}
	return 0;
	
}

int rtl8192cd_p2p_ss_req(struct rtl8192cd_priv *priv, unsigned char *data, int len)
{
	INT8 ret = 0;
	if (!netif_running(priv->dev)  || P2P_DISCOVERY	)
		ret = -1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey
	{
		printk("\n\n trigger P2P_discovery from UI\n");	
			
		P2P_DISCOVERY = 1;
		priv->site_survey.count = 0;	
		P2P_scan(priv,NULL);	
	}

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}



static int rtl8192cd_p2p_get_ss_status(struct rtl8192cd_priv *priv, unsigned char *data)
{
	UINT8 flags;
	INT8 ret = 0;
	//int idx ;
	if (copy_from_user((void *)&flags, (void *)(data), 1))
		return -1;

	if (!netif_running(priv->dev) || P2P_DISCOVERY)
	{
		ret = -1;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 1)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)(data), (void *)&ret, 1))
			return -1;
	}
	else if (flags == 0)
	{
		ret = priv->site_survey.count_backup;
		if (copy_to_user((void *)data, (void *)&ret, 1))
			return -1;
		// now we should report data base.
				
		if (copy_to_user((void *)(data+4), priv->site_survey.bss_backup,
				sizeof(struct bss_desc)*priv->site_survey.count_backup))
			return -1;
	}


	return 0;
}


#endif

#ifdef CLIENT_MODE
#ifdef WIFI_WPAS
int check_bss_encrypt(struct rtl8192cd_priv *priv)
#else
static int check_bss_encrypt(struct rtl8192cd_priv *priv)
#endif
{

#if defined(CONFIG_RTL_WAPI_SUPPORT)
	//	WAPI
	if (priv->pmib->wapiInfo.wapiType!=wapiDisable
		|| priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_WAPI_SMS4_)
	{
		if ((priv->pmib->dot11Bss.capability & BIT(4)) == 0)
			return FAIL;
		else if (priv->pmib->dot11Bss.t_stamp[0] != SECURITY_INFO_WAPI)
			return FAIL;
		else
			return SUCCESS;
	} else
#endif
	// no encryption
	if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == 0)
	{
		if (priv->pmib->dot11Bss.capability & BIT(4))
			return FAIL;
		else
			return SUCCESS;
	}
	// legacy encryption
	else if (!IEEE8021X_FUN &&
		((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ||
		 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)))
	{
		if ((priv->pmib->dot11Bss.capability & BIT(4)) == 0)
			return FAIL;
		else if (priv->pmib->dot11Bss.t_stamp[0] != 0)
			return FAIL;
		else
			return SUCCESS;
	}
	// WPA/WPA2
	else
	{
		if ((priv->pmib->dot11Bss.capability & BIT(4)) == 0)
			return FAIL;
		else if (priv->pmib->dot11Bss.t_stamp[0] == 0)
			return FAIL;
		else if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_1_) &&
			((priv->pmib->dot11Bss.t_stamp[0] & 0x0000ffff) == 0))
			return FAIL;
		else if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) &&
			((priv->pmib->dot11Bss.t_stamp[0] & 0xffff0000) == 0))
			return FAIL;
		else
			return SUCCESS;
	}
}


static int	rtl8192cd_join(struct rtl8192cd_priv *priv, unsigned char *data)
{
	INT8 ret = 0;
	char tmpbuf[33];

	if (!netif_running(priv->dev))
		ret = 2;
	else if (priv->ss_req_ongoing)
		ret = 1;
	else
		ret = 0;

	if (!ret)	// now, let's start site survey and join
	{
		if (copy_from_user((void *)&(priv->pmib->dot11Bss), (void *)data, sizeof(struct bss_desc)))
			return -1;

#ifdef WIFI_SIMPLE_CONFIG
		if (priv->pmib->wscEntry.wsc_enable && (priv->pmib->dot11Bss.bsstype&WIFI_WPS)) {
			priv->pmib->dot11Bss.bsstype &= ~WIFI_WPS;
			priv->wps_issue_join_req = 1;
		}
		else
#endif
		{
			if (check_bss_encrypt(priv) == FAIL) {
				DEBUG_INFO("Encryption mismatch!\n");
				ret = 2;
				if (copy_to_user((void *)data, (void *)&ret, 1))
					return -1;
				else
					return 0;
			}
		}

		if ((priv->pmib->dot11Bss.ssidlen == 0) || (priv->pmib->dot11Bss.ssid[0] == '\0')) {
			DEBUG_INFO("Join to a hidden AP!\n");
			ret = 2;
			if (copy_to_user((void *)data, (void *)&ret, 1))
				return -1;
			else
				return 0;
		}

#ifdef UNIVERSAL_REPEATER
		disable_vxd_ap(GET_VXD_PRIV(priv));
#endif

		memcpy(tmpbuf, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		tmpbuf[priv->pmib->dot11Bss.ssidlen] = '\0';
		DEBUG_INFO("going to join bss: %s\n", tmpbuf);

		memcpy(SSID2SCAN, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
		SSID2SCAN_LEN = priv->pmib->dot11Bss.ssidlen;

		SSID_LEN = SSID2SCAN_LEN;
		memcpy(SSID, SSID2SCAN, SSID_LEN);
		memset(BSSID, 0, MACADDRLEN);

// button 2009.05.21
// derive  PSK with slelected SSID
#ifdef INCLUDE_WPA_PSK
		if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
			derivePSK(priv);
#endif
		priv->join_req_ongoing = 1;
		priv->authModeRetry = 0;
		start_clnt_join(priv);
	}

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}

#ifdef CONFIG_RTK_MESH
// ==== inserted by GANTOE for site survey 2008/12/25 ====
// This function might be modifed when the mesh peerlink precedure has been completed
static int rtl8192cd_join_mesh (struct rtl8192cd_priv *priv, unsigned char* meshid, int meshid_len, int channel, int reset)
{
	int i, ret = -1;
	unsigned long flags;

	SAVE_INT_AND_CLI(flags);
	for(i = 0; i < priv->site_survey.count; i++)
	{
		if(reset || (!memcmp(meshid, priv->site_survey.bss[i].meshid, meshid_len) && priv->site_survey.bss[i].channel == channel))
		{
			SwChnl(priv, priv->site_survey.bss[i].channel, 0); // in this version, automatically establishing link
			priv->pmib->dot11RFEntry.dot11channel = priv->site_survey.bss[i].channel;
			memcpy(GET_MIB(priv)->dot1180211sInfo.mesh_id, meshid, meshid_len);
// fixed by Joule 2009.01.10
			GET_MIB(priv)->dot1180211sInfo.mesh_id[meshid_len]=0;
			update_beacon(priv);
			ret = 0;
		}
	}
	RESTORE_INT(flags);
	return ret;
}

// This function might be removed when the mesh peerlink precedure has been completed
static int rtl8192cd_check_mesh_link (struct rtl8192cd_priv *priv, unsigned char* macaddr)
{
	int ret = -1;
	unsigned long flags;
	struct stat_info *pstat;
	struct list_head *phead, *plist, *pprevlist;

	SAVE_INT_AND_CLI(flags);
	phead= &priv->mesh_mp_hdr;
	plist = phead->next;
	pprevlist = phead;

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, mesh_mp_ptr);
		if(!memcmp(pstat->hwaddr, macaddr, 6))
		{
			ret = 0;
			break;
		}
		plist = plist->next;
		pprevlist = plist->prev;
	}
	RESTORE_INT(flags);
	return ret;
}
#endif

static int rtl8192cd_join_status(struct rtl8192cd_priv *priv, unsigned char *data)
{
	INT8 ret = 0;

	if (!netif_running(priv->dev) || priv->join_req_ongoing)
		ret = -1;	// pending
	else
		ret = priv->join_res;

	if (copy_to_user((void *)data, (void *)&ret, 1))
		return -1;

	return 0;
}
#endif // CLIENT_MODE

#ifdef	SUPPORT_TX_MCAST2UNI
#ifndef CONFIG_MSC
static
#endif
void AddDelMCASTGroup2STA(struct rtl8192cd_priv *priv, unsigned char *mac2addr, int add)
{
	int i, free=-1, found=0;
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;
	phead = &priv->asoc_list;
	plist = phead->next;

	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		// Search from SA stat list. If found check if mc entry is existed in table
		if (((OPMODE & WIFI_AP_STATE) && !memcmp(pstat->hwaddr, mac2addr+6 , 6))
#ifdef CLIENT_MODE
				|| !(OPMODE & WIFI_AP_STATE)
#endif
			) {
#ifdef WDS
			if ((OPMODE & WIFI_AP_STATE) && (pstat->state & WIFI_WDS))
				continue;	// Do not need to mc2uni coNversion in WDS
#endif
			for (i=0; i<MAX_IP_MC_ENTRY; i++) {
				if (pstat->ipmc[i].used && !memcmp(pstat->ipmc[i].mcmac, mac2addr, 6)) {
					found = 1;
					break;
				}
				if (free == -1 && !pstat->ipmc[i].used)
					free = i;
			}

			if (found) {
				if (!add) { // delete entry
					pstat->ipmc[i].used = 0;
					pstat->ipmc_num--;
				}
			}
			else { // not found
				if (!add) {
					printk("%s: Delete MC entry not found!\n", priv->dev->name);
				}
				else { // add entry
					if (free == -1)  { // no free entry
						printk("%s: MC entry full!\n", priv->dev->name);
					}
					else {
						memcpy(pstat->ipmc[free].mcmac, mac2addr, 6);
						pstat->ipmc[free].used	= 1;
						pstat->ipmc_num++;
					}
				}
			}
		}
	}
}
#endif // SUPPORT_TX_MCAST2UNI


#ifdef DRVMAC_LB
void drvmac_loopback(struct rtl8192cd_priv *priv)
{
	struct stat_info *pstat;
	unsigned char *da = priv->pmib->miscEntry.lb_da;

	// prepare station info
	if (memcmp(da, "\x0\x0\x0\x0\x0\x0", 6) && !IS_MCAST(da))
	{
		pstat = get_stainfo(priv, da);
		if (pstat == NULL)
		{
			pstat = alloc_stainfo(priv, da, -1);
			pstat->state = WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE;
			memcpy(pstat->bssrateset, AP_BSSRATE, AP_BSSRATE_LEN);
			pstat->bssratelen = AP_BSSRATE_LEN;
			pstat->expire_to = 30000;
			list_add_tail(&pstat->asoc_list, &priv->asoc_list);
			cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
			if (QOS_ENABLE)
				pstat->QosEnabled = 1;
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				pstat->ht_cap_len = priv->ht_cap_len;
				memcpy(&pstat->ht_cap_buf, &priv->ht_cap_buf, priv->ht_cap_len);
			}
			pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
			update_fwtbl_asoclst(priv, pstat);
//			add_update_RATid(priv, pstat);
		}
	}

	// accept all packets
//	RTL_W32(_RCR_, RTL_R32(_RCR_) | _AAP_);
	RTL_W32(RCR, RTL_R32(RCR) | RCR_AAP);


	// enable MAC loopback
//	RTL_W32(_CPURST_, RTL_R32(_CPURST_) | BIT(16) | BIT(17));
	RTL_W32(CR, RTL_R32(CR) | (LB_MAC_DLY&LBMODE_Mask)<<LBMODE_SHIFT);
}
#endif // DRVMAC_LB


int dynamic_RF_pwr_adj(struct rtl8192cd_priv *priv, unsigned char *data)
{
	unsigned char *ptr = data;
	int index, minus_sign, adj_value;
	unsigned int writeVal, readVal;
	unsigned char byte0, byte1, byte2, byte3;
	unsigned char RF6052_MAX_TX_PWR = 0x3F;

	if (*ptr == '-') {
		minus_sign = 1;
		ptr++;
	}
	else if (*ptr == '+') {
		minus_sign = 0;
		ptr++;
	}
	else {
		sprintf((char *)data, "[FAIL] No sign to know to add or subtract\n");
		return strlen((char *)data)+1;
	}

	adj_value = _atoi((char *)ptr, 10);
	if (adj_value >= 64) {
		sprintf((char *)data, "[FAIL] Adjust value too large\n");
		return strlen((char *)data)+1;
	}

	for (index=0; index<8; index++)
	{
		if ((index == 2) || (index == 3))
			continue;

		readVal = PHY_QueryBBReg(priv, rTxAGC_A_Rate18_06+index*4, 0x7f7f7f7f);
		byte0 = (readVal & 0xff000000) >> 24;
		byte1 = (readVal & 0x00ff0000) >> 16;
		byte2 = (readVal & 0x0000ff00) >> 8;
		byte3 = (readVal & 0x000000ff);

		if (minus_sign) {
			if (byte0 >= adj_value)
				byte0 -= adj_value;
			else
				byte0 = 0;
			if (byte1 >= adj_value)
				byte1 -= adj_value;
			else
				byte1 = 0;
			if (byte2 >= adj_value)
				byte2 -= adj_value;
			else
				byte2 = 0;
			if (byte3 >= adj_value)
				byte3 -= adj_value;
			else
				byte3 = 0;
		}
		else {
			byte0 += adj_value;
			byte1 += adj_value;
			byte2 += adj_value;
			byte3 += adj_value;
		}

		// Max power index = 0x3F Range = 0-0x3F
		if (byte0 > RF6052_MAX_TX_PWR)
			byte0 = RF6052_MAX_TX_PWR;
		if (byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if (byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if (byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;

		writeVal = (byte0<<24) | (byte1<<16) |(byte2<<8) | byte3;
		PHY_SetBBReg(priv, rTxAGC_A_Rate18_06+index*4, 0x7f7f7f7f, writeVal);
	}

	byte0 = PHY_QueryBBReg(priv, rTxAGC_A_CCK1_Mcs32, bTxAGCRateCCK);
	if (minus_sign)
		byte0 -= adj_value;
	else
		byte0 += adj_value;
	if (byte0 > RF6052_MAX_TX_PWR)
		byte0 = RF6052_MAX_TX_PWR;
	PHY_SetBBReg(priv, rTxAGC_A_CCK1_Mcs32, bTxAGCRateCCK, byte0);

	sprintf((char *)data, "[SUCCESS] %s %d level RF power\n", minus_sign?"Subtract":"Add", adj_value);
	return strlen((char *)data)+1;
}

extern void clear_shortcut_cache(void);

#ifdef WIFI_HAPD

int rtl8192cd_net80211_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
#ifdef NETDEV_NO_PRIV
	struct rtl8192cd_priv *priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	struct rtl8192cd_priv	*priv = (struct rtl8192cd_priv *)dev->priv;
#endif
	struct wifi_mib *pmib = priv->pmib;
	unsigned long flags;
	struct iwreq *wrq = (struct iwreq *) ifr;
	unsigned char *tmpbuf, *tmp1;
	UINT16 sta_num;
	int i = 0, ret = 0, sizeof_tmpbuf;
	static unsigned char tmpbuf1[1024];

	DEBUG_TRACE;

	sizeof_tmpbuf = sizeof(tmpbuf1);
	tmpbuf = tmpbuf1;
	memset(tmpbuf, '\0', sizeof_tmpbuf);

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);

	//printk("rtl8192cd_net80211_ioctl, name = %s, cmd =0x%x\n", wrq->ifr_name, cmd);

	switch ( cmd )
	{

		case IEEE80211_IOCTL_SETPARAM:
			ret = rtl_net80211_setparam(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_GETPARAM:
			break;
		case IEEE80211_IOCTL_SETMODE:
			break;
		case IEEE80211_IOCTL_GETMODE:
			break;
		case IEEE80211_IOCTL_SETWMMPARAMS:
			break;
		case IEEE80211_IOCTL_GETWMMPARAMS:
			break;
		case IEEE80211_IOCTL_SETCHANLIST:
			break;
		case IEEE80211_IOCTL_GETCHANLIST:
			break;
		case IEEE80211_IOCTL_CHANSWITCH:
			break;
		case IEEE80211_IOCTL_GET_APPIEBUF:
			ret = rtl_net80211_getwpaie(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_SET_APPIEBUF:
			ret = rtl_net80211_setappiebuf(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_FILTERFRAME:
			break;
		case IEEE80211_IOCTL_GETCHANINFO:
			break;
		case IEEE80211_IOCTL_SETOPTIE:
#ifdef WIFI_WPAS
			ret = rtl_net80211_setoptie(dev, NULL, &wrq->u, NULL);
#endif
			break;
		case IEEE80211_IOCTL_GETOPTIE:
			break;
		case IEEE80211_IOCTL_SETMLME:
			ret = rtl_net80211_setmlme(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_SETKEY:
			ret = rtl_net80211_setkey(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_DELKEY:
			ret = rtl_net80211_delkey(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_ADDMAC:
			break;
		case IEEE80211_IOCTL_DELMAC:
			break;
#if	(defined(WIFI_HAPD) && defined(WDS)) && !defined(HAPD_DRV_PSK_WPS)
		case IEEE80211_IOCTL_WDSADDMAC:
			ret = rtl_net80211_wdsaddmac(dev, NULL, &wrq->u, NULL);
			break;
		case IEEE80211_IOCTL_WDSDELMAC:
			ret = rtl_net80211_wdsdelmac(dev, NULL, &wrq->u, NULL);
			break;
#endif
		case IEEE80211_IOCTL_KICKMAC:
			break;

#ifdef WIFI_WPAS
		case WPAS_IOCTL_CUSTOM: //_Eric ?? No need to define ??
			{
				unsigned char *is_hapd = (unsigned char *)(wrq->u.data.pointer);

				if(*is_hapd == 0)
					ret = rtl_wpas_custom(dev, NULL, &wrq->u, NULL);
				else
					ret = rtl_hapd_config(dev, NULL, &wrq->u, NULL);
			}

			break;
#else
		case HAPD_IOCTL_SETCONFIG:
			ret = rtl_hapd_config(dev, NULL, &wrq->u, NULL);
			break;
#endif

		default:
			break;

	}

	RESTORE_INT(flags);
	SMP_UNLOCK(flags);

	return ret;

}
#endif

#ifdef FOR_VHT5G_PF //8812_PF4
#if 0
# map of the commands for realtek11n ap 
array set realtekapcommands { \
	    MISC "" \
        CHANNEL "iwpriv interface set_mib channel=" \
        SSID "iwpriv interface set_mib ssid=" \
        MPDUMINSPACE "" \
        FRGMNT "iwpriv interface set_mib fragthres=" \
        BCNINT "iwpriv interface set_mib bcnint=" \
        WIDTH "iwpriv interface set_mib use40M=" \
        RTS "iwpriv interface set_mib rtsthres=" \
        EXCHANNELOFFSET "iwpriv interface set_mib 2ndchoffset=" \
	    OPMODE "iwpriv interface set_mib opmode=" \
        SHORTGI20 "iwpriv interface set_mib shortGI20M=" \
        SHORTGI40 "iwpriv interface set_mib shortGI40M=" \
        SHORTGI80 "iwpriv interface set_mib shortGI40M=" \
        DUMMY "led flash 400" \
        AMPDU "iwpriv interface set_mib ampdu=" \
	    AMSDU "iwpriv interface set_mib amsdu=" \
	    ADDBAREJECT "iwpriv interface set_mib addba_reject=" \
		PHYSEL "iwpriv interface set_mib phyBandSelect=" \
		BAND "iwpriv interface set_mib band=" \
		DENYLEGACY "iwpriv interface set_mib deny_legacy=" \
		PRIVACY_TYPE "iwpriv interface set_mib encmode=" \
		PRESHAREKEY "iwpriv interface set_mib passphrase=" \
	    QOS "iwpriv interface set_mib qos_enable=" \
		MIMO_TR "iwpriv interface set_mib MIMO_TR_mode=" \
	    STBC_TX "iwpriv wlan0 set_mib stbc=" \
	    SEC "flash set interface " \
	    RADIO "flash set interface WLAN_DISABLED " \
	    WME "flash set interface WMM_ENABLED " \
        SECURITY "" \
	    RIFS "" \
	    DBWSIG40 "iwpriv interface set_mib cca_rts=" \
	    DBWSIG80 "iwpriv interface set_mib cca_rts=" \
	    FIXRATE_AC "iwpriv wlan0 set_mib fixrate=" \
	    AUTORATE "iwpriv wlan0 set_mib autorate=" \
	    NOACK "iwpriv wlan0 set_mib txnoack=" \
    }
#endif
void reset_default_sigma(struct rtl8192cd_priv *priv)
{

	if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
		printk("reset_default_sigma for 8812 (11AC, 80M) +++\n");
	else
	{
		printk("No need to reset for 92C\n");
		return;
	}
	
	//No Need to reset ssid, channel, band, rts , frag, security, edcu	  

	//priv->pmib->dot11RFEntry.dot11channel	
	//priv->pmib->dot11StationConfigEntry.dot11DesiredSSID
	//priv->pmib->dot11OperationEntry.dot11FragmentationThreshold
	priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod = 100;
	priv->pmib->dot11nConfigEntry.dot11nUse40M = 2;
	//priv->pmib->dot11OperationEntry.dot11RTSThreshold
	//priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;
	//priv->pmib->dot11OperationEntry.opmode
	priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M = 1;	  
	priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M = 1;
	priv->pmib->dot11nConfigEntry.dot11nAMPDU = 1;
	priv->pmib->dot11nConfigEntry.dot11nAMSDU = 1;
	priv->pmib->dot11nConfigEntry.dot11nAddBAreject = 0;
	//priv->pmib->dot11RFEntry.phyBandSelect
	priv->pmib->dot11BssType.net_work_type = (WIRELESS_11A |WIRELESS_11N |WIRELESS_11AC);
	priv->pmib->dot11StationConfigEntry.legacySTADeny = 0;
	//priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm
	//priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest
	priv->pmib->dot11QosEntry.dot11QosEnable = 1;
#if 1
	priv->pmib->dot11RFEntry.MIMO_TR_mode = MIMO_2T2R;
//
	priv->pmib->dot11acConfigEntry.dot11SupportedVHT = 0xfffa;
	priv->pmib->dot11acConfigEntry.dot11VHT_TxMap = 0xfffff;
//	
#endif
	priv->pmib->dot11nConfigEntry.dot11nSTBC = 0;
	priv->pshare->rf_ft_var.cca_rts = 0;
	priv->pmib->dot11StationConfigEntry.autoRate = 1;
	priv->pmib->dot11StationConfigEntry.fixedTxRate	= 0;
	priv->pmib->dot11nConfigEntry.dot11nTxNoAck = 0;
	priv->pmib->dot11QosEntry.ManualEDCA = 0;
	priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct=15;

	priv->pshare->rf_ft_var.opmtest = 0;
	priv->pmib->dot11RFEntry.txbf = 0;
	priv->pmib->dot11RFEntry.txldpc =0;
	priv->pmib->dot11RFEntry.rxldpc =0;

	priv->pshare->rf_ft_var.lgirate = 0xffff;
	priv->pshare->rf_ft_var.lpwrc = 0;
}
#endif

int rtl8192cd_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
#ifdef NETDEV_NO_PRIV
	struct rtl8192cd_priv *priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
#else
	struct rtl8192cd_priv	*priv = (struct rtl8192cd_priv *)dev->priv;
#endif
	struct wifi_mib *pmib = priv->pmib;
	unsigned long flags;
	struct iwreq *wrq = (struct iwreq *) ifr;
	unsigned char *tmpbuf, *tmp1;
	UINT16 sta_num;
	int	i = 0, ret = -1, sizeof_tmpbuf;
	//Alpha
	int max_user_sta_num=0;
#ifdef CONFIG_RTL8672
	// MBSSID Port Mapping
	int ifgrp_member_tmp;
#endif
	static unsigned char tmpbuf1[1024];
#ifdef RTK_WOW
	unsigned int wakeup_on_wlan = 0;
#endif

#ifdef CONFIG_RTK_MESH
	unsigned char strPID[10];
	int len;
	static UINT8 QueueData[MAXDATALEN2];
	int		QueueDataLen;
	// UINT8	val8;

	// int j;
	#define DATAQUEUE_EMPTY "Queue is empty"
#endif

	DEBUG_TRACE;


#ifdef WIFI_HAPD
		if((IEEE80211_IOCTL_SETPARAM <= cmd) && (cmd <= IEEE80211_IOCTL_KICKMAC))
			return rtl8192cd_net80211_ioctl(dev, ifr, cmd);

		if(cmd == HAPD_IOCTL_SETCONFIG)
			return rtl8192cd_net80211_ioctl(dev, ifr, cmd);
#endif


	sizeof_tmpbuf = sizeof(tmpbuf1);
	tmpbuf = tmpbuf1;
	memset(tmpbuf, '\0', sizeof_tmpbuf);

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);

	switch ( cmd )
	{

	case SIOCGIWNAME:
		strcpy(wrq->u.name, "IEEE 802.11-DS");
		ret = 0;
		break;
#ifdef CONFIG_RTL_COMAPI_WLTOOLS
	case SIOCGIFHWADDR:

#ifdef WIFI_HAPD //_Eric ??
		memcpy(ifr->ifr_hwaddr.sa_data, pmib->dot11OperationEntry.hwaddr, MACADDRLEN);
#endif

		memcpy(wrq->u.name, pmib->dot11OperationEntry.hwaddr, MACADDRLEN);
		ret = 0;
		break;
	case SIOCSIWFREQ:	//set channel/frequency (Hz)
	{
		ret = rtl_siwfreq(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCGIWFREQ:	// get channel/frequency (Hz)
	{
		ret = rtl_giwfreq(dev, NULL, &wrq->u, NULL);
		break;
	}
#ifdef WIFI_WPAS
	case SIOCSIWMODE:	//set operation mode
	{
		ret = rtl_siwmode(dev, NULL, &wrq->u, NULL);
		break;
	}
#endif
	case SIOCGIWMODE:	//get operation mode
	{
		u32 *mode=&wrq->u.mode;
		ret = rtl_giwmode(dev, NULL, mode, NULL);
		break;
	}
	case SIOCSIWAP:  //set access point MAC addresses
	{
		struct sockaddr *ap_addr=&wrq->u.ap_addr;
		ret = rtl_siwap(dev, NULL, &wrq->u, ap_addr->sa_data);
		break;
	}
	case SIOCGIWAP: 	//get access point MAC addresses
	{
		struct sockaddr *ap_addr=&wrq->u.ap_addr;
		ret = rtl_giwap(dev, NULL, &wrq->u, ap_addr->sa_data);
		break;
	}
	case SIOCGIWESSID:	//Get ESSID
	{
		struct iw_point *essid=&wrq->u.essid;
		ret = rtl_giwessid(dev, NULL, &wrq->u, essid->pointer);
		break;
	}
	case SIOCSIWESSID:	//Set ESSID
	{
		struct iw_point *essid=&wrq->u.essid;
		ret = rtl_siwessid(dev, NULL, &wrq->u, essid->pointer);
		break;
	}
//Alpha
//polo add 7-24	
	case SIOCGIWENCRYPT: //Get ENCRYPT
	{
		break;
	}
	case SIOCGIAUTHTYPE: //Get AUTHTYPE
	{
		break;
	}	
//polo end	
	case SIOCGIWRATE: //get default bit rate (bps)
		ret = rtl_giwrate(dev, NULL, &wrq->u, NULL);
		break;
	case SIOCSIWRATE:  //set default bit rate (bps)
		ret = rtl_siwrate(dev, NULL, &wrq->u, NULL);
		break;
	case SIOCGIWRANGE: //Range of Parameters
	{
		ret = rtl_giwrange(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCSIWSCAN:
	{
		ret = rtl_siwscan(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCGIWSCAN:
		{
		ret = rtl_giwscan(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCGIWRTS:  // get RTS/CTS threshold (bytes)
	{
		ret = rtl_giwrts(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCSIWRTS:  //set RTS/CTS threshold (bytes)
	{
		ret = rtl_siwrts(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCGIWFRAG:  //get fragmentation thr (bytes)
	{
		ret = rtl_giwfrag(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCSIWFRAG:  //set fragmentation thr (bytes)
	{
		ret = rtl_siwfrag(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCSIWRETRY:	//set retry limit
	{
		ret = rtl_siwretry(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCGIWRETRY:	//get retry limit
	{
		ret = rtl_giwretry(dev, NULL, &wrq->u, NULL);
		break;
	}
	case SIOCSIWENCODE:  //get encoding token & mode
	{
		struct iw_point *erq=&wrq->u.encoding;
		if(erq)
			ret = rtl_siwencode(dev, NULL, &wrq->u, erq->pointer);
		break;
	}
	case SIOCGIWENCODE:  //get encoding token & mode
	{
		struct iw_point *erq=&wrq->u.encoding;
		if(erq)
			ret = rtl_giwencode(dev, NULL, &wrq->u, erq->pointer);
		break;
	}
	case SIOCGIWPOWER:
	{
		ret = rtl_giwpower(dev, NULL, &wrq->u, NULL);
		break;
	}
#endif

	case SIOCMIBINIT:	//-- copy kernel data to user data --//
		if (wrq->u.data.length != sizeof(struct wifi_mib)) {
			panic_printk("IOCTL: mib size mismatch!\n");
			ret = -1;
			break;
		}
		if (copy_to_user((void *)wrq->u.data.pointer, (void *)pmib, wrq->u.data.length) == 0)
			ret = 0;
		break;

	case SIOCMIBSYNC:	//-- sync user data to kernel data  --//
		if (wrq->u.data.length != sizeof(struct wifi_mib)) {
			panic_printk("IOCTL: mib size mismatch!\n");
			ret = -1;
			break;
		}
		if (copy_from_user((void *)pmib, (void *)wrq->u.data.pointer, wrq->u.data.length) == 0)
			ret = 0;
		break;

	case SIOCGIWPRIV:	//-- get private ioctls for iwpriv --//
		if (wrq->u.data.pointer) {
#ifndef CONFIG_RTL_COMAPI_WLTOOLS
			static struct iw_priv_args privtab[] = {
				{ RTL8192CD_IOCTL_SET_MIB, IW_PRIV_TYPE_CHAR | 450, 0, "set_mib" },
				{ RTL8192CD_IOCTL_GET_MIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_mib" },
#ifdef _IOCTL_DEBUG_CMD_
				{ RTL8192CD_IOCTL_WRITE_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_reg" },
				{ RTL8192CD_IOCTL_READ_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_reg" },
				{ RTL8192CD_IOCTL_WRITE_MEM, IW_PRIV_TYPE_CHAR | 128, 0, "write_mem" },
				{ RTL8192CD_IOCTL_READ_MEM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_mem" },
				{ RTL8192CD_IOCTL_WRITE_BB_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_bb" },
				{ RTL8192CD_IOCTL_READ_BB_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_bb" },
				{ RTL8192CD_IOCTL_WRITE_RF_REG, IW_PRIV_TYPE_CHAR | 128, 0, "write_rf" },
				{ RTL8192CD_IOCTL_READ_RF_REG, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_rf" },
#endif
				{ RTL8192CD_IOCTL_DEL_STA, IW_PRIV_TYPE_CHAR | 128, 0, "del_sta" },
				{ RTL8192CD_IOCTL_WRITE_EEPROM, IW_PRIV_TYPE_CHAR | 128, 0, "write_eeprom" },
				{ RTL8192CD_IOCTL_READ_EEPROM, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_BYTE | 128, "read_eeprom" },

#ifdef SUPPORT_SNMP_MIB
				{ SIOCGSNMPMIB, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_snmp_mib" },
#endif

				{ SIOCSRFPWRADJ, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "rf_pwr" },

#ifdef AUTO_TEST_SUPPORT
				{ SIOCSSREQ, IW_PRIV_TYPE_NONE, 0, "at_ss" },
				{ SIOCJOINREQ, IW_PRIV_TYPE_CHAR | 40, 0, "at_join" },
#endif

#ifdef MP_TEST
				{ MP_START_TEST, IW_PRIV_TYPE_NONE, 0, "mp_start" },
				{ MP_STOP_TEST, IW_PRIV_TYPE_NONE, 0, "mp_stop" },
				{ MP_SET_RATE, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rate" },
				{ MP_SET_CHANNEL, IW_PRIV_TYPE_CHAR | 40, 0, "mp_channel" },
				{ MP_SET_TXPOWER, IW_PRIV_TYPE_CHAR | 40, 0, "mp_txpower" },
				{ MP_CONTIOUS_TX, IW_PRIV_TYPE_CHAR | 128, 0, "mp_ctx" },
				{ MP_ARX, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_arx" },
				{ MP_SET_BSSID, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bssid" },
				{ MP_ANTENNA_TX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_tx" },
				{ MP_ANTENNA_RX, IW_PRIV_TYPE_CHAR | 40, 0, "mp_ant_rx" },
				{ MP_SET_BANDWIDTH, IW_PRIV_TYPE_CHAR | 40, 0, "mp_bandwidth" },
				{ MP_SET_PHYPARA, IW_PRIV_TYPE_CHAR | 40, 0, "mp_phypara" },
#ifdef B2B_TEST
				{ MP_TX_PACKET, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "mp_tx" },
				{ MP_BRX_PACKET, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_brx" },
#if 0
				{ MP_RX_PACKET, IW_PRIV_TYPE_CHAR | 40, 0, "mp_rx" },
#endif
#endif
				{ MP_QUERY_STATS, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_query" },
				{ MP_TXPWR_TRACK, IW_PRIV_TYPE_CHAR | 40, 0, "mp_pwrtrk" },
				{ MP_QUERY_TSSI, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_tssi" },
				#ifdef MP_PSD_SUPPORT
				{ MP_QUERY_PSD, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_psd" }, 
				#endif
				{ MP_QUERY_THER, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_ther" },
#ifdef CONFIG_RTL_92D_SUPPORT
				{ MP_SET_BAND, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_phyband" },
#endif
				{ MP_RESET_STATS, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_reset_stats" },
				{ MP_GET_TXPOWER, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_CHAR | 128, "mp_get_pwr" },
				{ MP_DIG, IW_PRIV_TYPE_CHAR | 40, 0, "mp_dig" },	
#endif // MP_TEST
#if defined(SW_ANT_SWITCH) || defined(HW_ANT_SWITCH)
				{ SIOCANTSELECT, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "dvyAnt_set" },
#endif

#ifdef MICERR_TEST
				{ SIOCSIWRTLMICERROR, IW_PRIV_TYPE_CHAR | 40, 0, "mic_error" },
				{ SIOCSIWRTLMICREPORT, IW_PRIV_TYPE_CHAR | 40, 0, "mic_report" },
#endif
				{ SIOCGIWRTLREGDUMP, IW_PRIV_TYPE_CHAR | 40, 0, "reg_dump" },

#if defined(MBSSID) || defined(UNIVERSAL_REPEATER)
				{ SIOCSICOPYMIB, IW_PRIV_TYPE_CHAR | 40, 0, "copy_mib" },
#endif

#ifdef CONFIG_RTL8186_KB
				{ SIOCGIREADGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "read_guestmac" },
				{ SIOCSIWRTGUESTMAC, IW_PRIV_TYPE_CHAR | 40, 0, "write_guestmac" },
#endif

#ifdef	CONFIG_RTK_MESH
				{ RTL8192CD_IOCTL_STATIC_ROUTE, IW_PRIV_TYPE_CHAR | 40, 0, "strt" },
#ifdef D_ACL//tsananiu
                { RTL8192CD_IOCTL_ADD_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "add_acl_table" },
                { RTL8192CD_IOCTL_REMOVE_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "remove_acl_table" },
                { RTL8192CD_IOCTL_GET_ACL_TABLE, IW_PRIV_TYPE_CHAR | 40, IW_PRIV_TYPE_BYTE | 128, "get_acl_table" },
#endif//tsananiu//
#endif
#ifdef BR_SHORTCUT
				{ SIOCLEARBRSC, IW_PRIV_TYPE_CHAR | 40, 0, "clear_brsc" },
#endif


				{ SIOCRADIOOFF, IW_PRIV_TYPE_CHAR | 128, 0, "radio_off" },

#ifdef PCIE_POWER_SAVING
#ifdef PCIE_POWER_SAVING_DEBUG
				{ SIOCEPDN, IW_PRIV_TYPE_CHAR | 128, 128, "epdn" },
#else
				{ SIOCEPDN, IW_PRIV_TYPE_CHAR | 128, 128, "stopps" },
#endif
#endif

#ifdef 	EN_EFUSE
				{ SIOCEFUSE_GET, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 512, "efuse_get" },
				{ SIOCEFUSE_SET, IW_PRIV_TYPE_CHAR | 512, IW_PRIV_TYPE_CHAR | 128, "efuse_set" },
				{ SIOCEFUSE_SYNC, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "efuse_sync" },
#endif
#ifdef 	P2P_SUPPORT
				{ SIOCP2PCMD, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "p2pcmd" },
#endif
#ifdef CONFIG_RTL_92D_SUPPORT
				{ SIOC92DIQK, IW_PRIV_TYPE_CHAR | 128, 0, "iqk" },
				{ SIOC92DTEMPMP, IW_PRIV_TYPE_CHAR | 128, 0, "mprt" },
#ifdef CONFIG_RTL_92D_DMDP
				{ SIOC92DSMCC, IW_PRIV_TYPE_CHAR | 128, 0, "smcc" },
#endif
#ifdef EN_EFUSE
				{ SIOC92DSBANDADDR, IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | 128, "bandadd" },
#endif
#ifdef NON_INTR_ANTDIV
				{ SIOC92DATNDIV, IW_PRIV_TYPE_CHAR | 128, 0 | 128, "antdiv" },
#endif
#ifdef DPK_92D
				{ SIOC92DDPK, IW_PRIV_TYPE_CHAR | 128, 0 | 128, "dpk" },
#endif
#endif // CONFIG_RTL_92D_SUPPORT
#ifdef FOR_VHT5G_PF
				{ SIOC8812SIGMA, IW_PRIV_TYPE_CHAR | 128, 0 | 128, "sigma_default" }, //8812_PF4
#endif
				{ SIOC92DAUTOCH, IW_PRIV_TYPE_CHAR | 128, 0, "autoch" }
			};
#endif
#ifdef __LINUX_2_6__
			ret = access_ok(VERIFY_WRITE, (const void *)wrq->u.data.pointer, sizeof(privtab));
			if (!ret) {
				ret = -EFAULT;
				DEBUG_ERR("user space valid check error!\n");
				break;
			}
#else
			ret = verify_area(VERIFY_WRITE, (const void *)wrq->u.data.pointer, sizeof(privtab));
			if (ret) {
				DEBUG_ERR("verify_area() error!\n");
				break;
			}
#endif

			wrq->u.data.length = sizeof(privtab) / sizeof(privtab[0]);
			if (copy_to_user((void *)wrq->u.data.pointer, privtab, sizeof(privtab)))
				ret = -EFAULT;
		}
		break;


#ifdef D_ACL	//tsananiu ; mesh related
	case RTL8192CD_IOCTL_ADD_ACL_TABLE:
		if( (ret = iwpriv_atoi((unsigned char *)(wrq->u.data.pointer),tmpbuf,wrq->u.data.length)) )
		{
			DEBUG_ERR("Trasnslate MAC address from user space error\n");
			break;
		}
		ret = acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
                pstat = get_stainfo(priv, tmpbuf);


                DEBUG_INFO("mode %d\n", priv->pmib->dot11StationConfigEntry.dot11AclMode);

		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_allow)
        		memset(priv->aclCache, 0, sizeof(priv->aclCache));

		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_deny){
                	if(NULL != pstat){
	                	if(isSTA(pstat)){	//if station
        	              		DEBUG_INFO("I am a STA\n");
                	      		ret = del_sta(priv, tmpbuf+MACADDRLEN);
                	      	}

                      		else{
                      			DEBUG_INFO("I am a mesh node\n");
                      			issue_disassoc_MP(priv, pstat, 0, 0);
				}
                 	}
                 }

                break;

	case RTL8192CD_IOCTL_REMOVE_ACL_TABLE:
		if( (ret = iwpriv_atoi((unsigned char *)(wrq->u.data.pointer),tmpbuf,wrq->u.data.length)) )
		{
			DEBUG_ERR("Trasnslate MAC address from user space error\n");
			break;
		}
		ret = acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		pstat = get_stainfo(priv, tmpbuf);


		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_deny)
        		memset(priv->aclCache, 0, sizeof(priv->aclCache));


		if(priv->pmib->dot11StationConfigEntry.dot11AclMode == ACL_allow){
			DEBUG_INFO("in allow mode\n");
			if(NULL != pstat){
				DEBUG_INFO("pstat != NULL\n");
				if(isSTA(pstat)){	//if station
        	              		DEBUG_INFO("I am a STA\n");
                	      		ret = del_sta(priv, tmpbuf+MACADDRLEN);
                	      	}
				else{
                      			DEBUG_INFO("I am a mesh node\n");
                      			issue_disassoc_MP(priv, pstat, 0, 0);
                      		}
                      		memset(priv->aclCache, 0, sizeof(priv->aclCache));
                      	}
		}

		break;

        case RTL8192CD_IOCTL_GET_ACL_TABLE:

        	ret = acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));

        	break;
#endif//tsananiu//

	case RTL8192CD_IOCTL_SET_MIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = set_mib(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_GET_MIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = get_mib(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

#ifdef _IOCTL_DEBUG_CMD_
	case RTL8192CD_IOCTL_WRITE_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_reg(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_READ_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8192CD_IOCTL_WRITE_MEM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_mem(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_READ_MEM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;
		i = read_mem(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8192CD_IOCTL_WRITE_BB_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_bb_reg(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_READ_BB_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_bb_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case RTL8192CD_IOCTL_WRITE_RF_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_rf_reg(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_READ_RF_REG:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_rf_reg(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
#endif // _IOCTL_DEBUG_CMD_


	case RTL8192CD_IOCTL_DEL_STA:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = del_sta(priv, tmpbuf);
		break;


	case RTL8192CD_IOCTL_WRITE_EEPROM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		ret = write_eeprom(priv, tmpbuf);
		break;

	case RTL8192CD_IOCTL_READ_EEPROM:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = read_eeprom(priv, tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

#ifdef RTK_WOW
	case SIOCGRTKWOWSTAINFO:	//-- get station info for Realtek proprietary wake up on wlan mode--//
		wakeup_on_wlan = 1;
#endif
	case SIOCGIWRTLSTAINFO:	//-- get station table information --//
		//Alpha
		//sizeof_tmpbuf = sizeof(sta_info_2_web) * (NUM_STAT + 1); // for the max of all sta info
		max_user_sta_num = (wrq->u.data.length/sizeof(sta_info_2_web))-1; // max number of sta info is depend on user space buff size
		sizeof_tmpbuf = sizeof(sta_info_2_web) * (max_user_sta_num + 1); // for the max of all sta info
		//Alpha end
		tmp1 = (unsigned char *)kmalloc(sizeof_tmpbuf, GFP_KERNEL);
		if (!tmp1) {
			printk("Unable to allocate temp buffer for ioctl (SIOCGIWRTLSTAINFO)!\n");
			return -1;
		}
		memset(tmp1, '\0', sizeof(sta_info_2_web));

		if (copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, 1))
			break;
		//Alpha
		//if ((tmpbuf[0] == 0) || (tmpbuf[0] > NUM_STAT))
		//	sta_num = NUM_STAT;
		if ((tmpbuf[0] == 0) || (tmpbuf[0] > max_user_sta_num))
			sta_num = max_user_sta_num;
		//Alpha end
		else
			sta_num = tmpbuf[0];
#ifdef RTK_WOW
		get_sta_info(priv, (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)), sta_num, wakeup_on_wlan);
#else
		get_sta_info(priv, (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)), sta_num);
#endif
		if (copy_to_user((void *)wrq->u.data.pointer, tmp1, sizeof(sta_info_2_web)*(sta_num+1)))
			break;
		wrq->u.data.length = sizeof(sta_info_2_web)*(sta_num+1);
		ret = 0;

		kfree(tmp1);
		break;

	case SIOCGIWRTLSTANUM:	//-- get the number of stations in table --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE) &&
				!IS_DRV_OPEN(priv))
			sta_num = 0;
		else
#endif
		//sta_num = get_assoc_sta_num(priv);	// this will count expired sta
		sta_num = priv->assoc_num;
		if (copy_to_user((void *)wrq->u.data.pointer, &sta_num, sizeof(sta_num)))
			break;
		wrq->u.data.length = sizeof(sta_num);
		ret = 0;
		break;

	case SIOCGIWRTLDRVVERSION:
		tmpbuf[0] = DRV_VERSION_H;
		tmpbuf[1] = DRV_VERSION_L;
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, 2))
			break;
		wrq->u.data.length = 2;
		ret = 0;
		break;

	case SIOCGIWRTLGETBSSINFO: //-- get BSS info --//
		get_bss_info(priv, (bss_info_2_web *)tmpbuf);
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, sizeof(bss_info_2_web)))
			break;
		wrq->u.data.length = sizeof(bss_info_2_web);
		ret = 0;
		break;

#if defined(CONFIG_RTL8186_KB_N)//To get auth result
	case SIOCGIWRTLAUTH:
		if (copy_to_user((void *)wrq->u.data.pointer, &authRes, sizeof(authRes)))
			break;
		wrq->u.data.length = sizeof(authRes);
		ret = 0;
		authRes = 0;//To init authRes
		break;
#endif

#ifdef WDS
	case SIOCGIWRTLGETWDSINFO: //-- get WDS table information --//
		ret = get_wds_info(priv, (web_wds_info *)tmpbuf);
		if ((ret > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, ret))
			break;
		wrq->u.data.length = ret;
		break;
#endif

	case SIOCSIWRTLSTATXRATE:	//-- set station tx rate --//
		if (wrq->u.data.length != sizeof(struct _wlan_sta_rateset) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, sizeof(struct _wlan_sta_rateset)))
			break;
		ret = set_sta_txrate(priv, (struct _wlan_sta_rateset *)tmpbuf);
		break;


#ifdef MICERR_TEST
	case SIOCSIWRTLMICERROR:
		ret = iwpriv_atoi((unsigned char *)(wrq->u.data.pointer),tmpbuf,wrq->u.data.length);
		ret = issue_mic_err_pkt(priv, tmpbuf);
		break;
#ifdef CLIENT_MODE
	case SIOCSIWRTLMICREPORT:
	{
		struct sta_info *pstat;
		if ((pstat = get_stainfo(priv, BSSID)) != NULL)
			ClientSendEAPOL(priv, pstat, 0);
	}
		ret = 0;
		break;
#endif
#endif


	case SIOCSACLADD:
		ret = acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSACLDEL:
		ret = acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSACLQUERY:
		ret = acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
	case SIOCSMESHACLADD:
		ret = mesh_acl_add_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSMESHACLDEL:
		ret = mesh_acl_remove_cmd(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCSMESHACLQUERY:
		ret = mesh_acl_query_cmd(priv, (unsigned char *)(wrq->u.data.pointer));
		break;
#endif

	case SIOCGMISCDATA:
		get_misc_data(priv, (struct _misc_data_ *)tmpbuf);
		if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, sizeof(struct _misc_data_)))
			break;
		wrq->u.data.length = sizeof(struct _misc_data_);
		ret = 0;
		break;


	case RTL8192CD_IOCTL_USER_DAEMON_REQUEST:
#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
#endif
		ret = rtl8192cd_ioctl_priv_daemonreq(dev, &wrq->u.data);
		break;


#ifdef USE_PID_NOTIFY
	case SIOCSIWRTLSETPID:
		priv->pshare->wlanapp_pid = -1;
		if (wrq->u.data.length != sizeof(pid_t) ||
			copy_from_user(&priv->pshare->wlanapp_pid, (void *)wrq->u.data.pointer, sizeof(pid_t))) {
			//break;
		} else {
			ret = 0;
		}

	#if defined(LINUX_2_6_27_)
		if (priv->pshare->wlanapp_pid != -1)
		{
			rcu_read_lock();
			_wlanapp_pid = get_pid(find_vpid(priv->pshare->wlanapp_pid));
			rcu_read_unlock();
		}
	#endif
		break;
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case SIOCSIWRTLSETWAPIPID:
		priv->pshare->wlanwapi_pid= -1;
		if (wrq->u.data.length != sizeof(pid_t) ||
			copy_from_user(&priv->pshare->wlanwapi_pid, (void *)wrq->u.data.pointer, sizeof(pid_t)))
			break;
		ret = 0;
		break;
#endif


#ifdef	CONFIG_RTK_MESH
	case RTL8192CD_IOCTL_STATIC_ROUTE:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		if( memcmp(tmpbuf, "del", 3)==0 )
		{
			mac12_to_6(tmpbuf+4, tmpbuf+0x100);
			ret = remove_path_entry(priv, tmpbuf+0x100);
		}
		else if((memcmp(tmpbuf, "add", 3)==0) && (wrq->u.data.length>28) )
		{
			struct path_sel_entry Entry;
			memset((void*)&Entry, 0, sizeof(struct path_sel_entry));
			mac12_to_6(tmpbuf+4, Entry.destMAC);
			mac12_to_6(tmpbuf+4+13, Entry.nexthopMAC);
			Entry.flag=1;
			ret = pathsel_table_entry_insert_tail( priv, &Entry);
		}else
			ret =0;
		break;
// ==== inserted by GANTOE for manual site survey 2008/12/25 ====
	case SIOCJOINMESH:
		{
			struct
			{
				unsigned char *meshid;
				int meshid_len, channel, reset;
			}mesh_identifier;
			if(wrq->u.data.length > 0)
			{
				memcpy(&mesh_identifier, wrq->u.data.pointer, wrq->u.data.length);
				ret = rtl8192cd_join_mesh(priv, mesh_identifier.meshid, mesh_identifier.meshid_len, mesh_identifier.channel, mesh_identifier.reset);
			}
			else
				ret = -1;
		}
		break;
	case SIOCCHECKMESHLINK:	// This case might be removed when the mesh peerlink precedure has been completed
		{
			if(wrq->u.data.length == 6)
				ret = rtl8192cd_check_mesh_link(priv, wrq->u.data.pointer);
			else
				ret = -1;
		}
		break;
// ==== GANTOE ====
#endif
	case SIOCGIWRTLSCANREQ:		//-- Issue SS request --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && !priv->pmib->wscEntry.wsc_enable) {
			DEBUG_ERR("can't do site-survey for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
			GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't do site-survey for vap!\n");
			break;
		}
#endif

#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
#endif
		ret = rtl8192cd_ss_req(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;

	case SIOCGIWRTLGETBSSDB:	//-- Get SS Status --//
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv) && !priv->pmib->wscEntry.wsc_enable) {
			DEBUG_ERR("can't get site-survey status for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
			GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't get site-survey status for vap!\n");
			break;
		}
#endif
		ret	= rtl8192cd_get_ss_status(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

/*--------------P2P related ioctl----------------------------------start*/
#ifdef P2P_SUPPORT
	/*P2P UI request do p2p discovery */
	case SIOCP2PSCANREQ:		
		if(!(OPMODE&WIFI_P2P_SUPPORT))
			return -1;

		if((P2PMODE  != P2P_DEVICE) && (P2PMODE  != P2P_CLIENT))
			return -1;		


#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
#endif
		ret = rtl8192cd_p2p_ss_req(priv, (unsigned char *)(wrq->u.data.pointer), wrq->u.data.length);
		break;
	/*P2P UI get P2P SS Status and Result*/
	case SIOCP2PGETRESULT:	

		if(!(OPMODE&WIFI_P2P_SUPPORT))
			return -1;
		if((P2PMODE  != P2P_DEVICE) && (P2PMODE  != P2P_CLIENT))
			return -1;
		
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv)) {
			DEBUG_ERR("can't get site-survey status for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
			GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't get site-survey status for vap!\n");
			break;
		}
#endif
		ret	= rtl8192cd_p2p_get_ss_status(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	//-- issue provision discovery request , need device address from P2P UI --//
	case SIOCP2PPROVREQ:	

		#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
		#endif
		ret	= req_p2p_provision_req(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	/*P2P UI confirm wsc method,pincode,Target device to wlan driver, 
	  if we active send provision req  before ,then will send nego req here */
	case SIOCP2WSCMETHODCONF:	

		#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
		#endif
		ret	= req_p2p_wsc_confirm(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	//-- report event and state to P2P UI--//		
	case SIOCP2PPGETEVNIND:	
		ret	= p2p_get_event_state(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	//-- wscd(GO mode) report WPS success or fail --//
	case SIOCP2P_WSC_REPORT_STATE:	
		ret	= p2p_wps_indicate_state(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	/*Report 1.p2p client connect state to web server ; for process start udhcpc
			 2.p2p pre-GO change to GO (WPS is done and success) indicate web server need start udhcpd*/		
	case SIOCP2P_REPORT_CLIENT_STATE:	
		ret	= p2p_get_p2pconnect_state(priv, (unsigned char *)(wrq->u.data.pointer));
		break;
	
#endif	// end of P2P_SUPPORT
/*--------------P2P related ioctl----------------------------------end*/

#ifdef AUTO_TEST_SUPPORT
	case SIOCSSREQ:
		rtl8192cd_SSReq_AutoTest(priv);
		ret = 0;
		break;

	case SIOCJOINREQ:
#ifdef CLIENT_MODE
		ret = rtl8192cd_join_AutoTest(priv ,  (unsigned char *)(wrq->u.data.pointer));
#endif
		break;

#endif

#ifdef CLIENT_MODE
	case SIOCGIWRTLJOINREQ:		//-- Issue Join Request --//
		ret = rtl8192cd_join(priv, (unsigned char *)(wrq->u.data.pointer));
		break;

	case SIOCGIWRTLJOINREQSTATUS:	//-- Get Join Status --//
		ret = rtl8192cd_join_status(priv, (unsigned char *)(wrq->u.data.pointer));
		break;
#endif


#ifdef RTK_WOW
	case SIOCGRTKWOW:	//-- issue Realtek proprietary wake up on wlan mode --//
		ret = 5;
		do {
			issue_rtk_wow(priv,  (unsigned char *)(wrq->u.data.pointer));
		} while(--ret > 0);
		break;
#endif

	case SIOCSRFPWRADJ:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = dynamic_RF_pwr_adj(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#ifdef MP_TEST
	case MP_START_TEST:
		mp_start_test(priv);
		ret = 0;
		break;

	case MP_STOP_TEST:
		mp_stop_test(priv);
		ret = 0;
		break;

	case MP_SET_RATE:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_datarate(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_CHANNEL:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_channel(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_BANDWIDTH:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_bandwidth(priv, tmpbuf);
		ret = 0;
		break;

	case MP_SET_TXPOWER:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_tx_power(priv, tmpbuf);
		ret = 0;
		break;

	case MP_CONTIOUS_TX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
		mp_ctx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		SMP_LOCK(flags);
		ret = 0;
		break;

	case MP_ARX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = mp_arx(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

	case MP_SET_BSSID:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_bssid(priv, tmpbuf);
		ret = 0;
		break;

	case MP_ANTENNA_TX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_ant_tx(priv, tmpbuf);
		ret = 0;
		break;

	case MP_ANTENNA_RX:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_ant_rx(priv, tmpbuf);
		ret = 0;
		break;

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
	case MP_SET_BAND:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_phyBand(priv, tmpbuf);
		ret = 0;
		break;
#endif

	case MP_RESET_STATS:
		mp_reset_stats(priv);
		ret = 0;
		break;

	case MP_SET_PHYPARA:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		mp_set_phypara(priv, tmpbuf);
		ret = 0;
		break;

#ifdef B2B_TEST
	case MP_TX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_tx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#if 0
	case MP_RX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
#ifndef __LINUX_2_6__
		RESTORE_INT(flags);
#endif
		mp_rx(priv, tmpbuf);
#ifndef __LINUX_2_6__
		SAVE_INT_AND_CLI(flags);
#endif
		ret = 0;
		break;
#endif

	case MP_BRX_PACKET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_brx(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;
#endif // B2B_TEST

	case MP_QUERY_STATS:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = mp_query_stats(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

	case MP_TXPWR_TRACK:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		mp_txpower_tracking(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;

	case MP_QUERY_TSSI:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_query_tssi(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case MP_QUERY_THER:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_query_ther(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
	#ifdef MP_PSD_SUPPORT
	case MP_QUERY_PSD:	
		if (copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;		
		RESTORE_INT(flags);
		i = mp_query_psd(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0 ) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i )) 
				break;
			wrq->u.data.length = i;
			ret = 0;
			//printk("The address of DA is 0x%p\n",(void *)wrq->u.data.pointer);
		}
		break;
	#endif
	case MP_GET_TXPOWER:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		i = mp_get_txpwr(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case MP_DIG:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		mp_dig(priv, tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;			
#endif	// MP_TEST

#if defined(SW_ANT_SWITCH) || defined(HW_ANT_SWITCH)
	case SIOCANTSELECT:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		diversity_antenna_select(priv, tmpbuf);
		ret = 0;
		break;
#endif

	case SIOCGIWRTLREGDUMP:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		RESTORE_INT(flags);
		reg_dump(priv, (char *)tmpbuf);
		SAVE_INT_AND_CLI(flags);
		ret = 0;
		break;

#ifdef BR_SHORTCUT
	case SIOCLEARBRSC:
		clear_shortcut_cache();
		ret = 0;
		break;
#endif

	case SIOCRADIOOFF:
#ifdef PCIE_POWER_SAVING
		radio_off(priv);
#endif
		ret = 0;
		break;


#ifdef PCIE_POWER_SAVING
	case SIOCEPDN:
#ifdef PCIE_POWER_SAVING_DEBUG
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = PCIE_PowerDown(priv, tmpbuf);

		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
#else
		priv->pshare->rf_ft_var.power_save &=0xf0;
		PCIeWakeUp(priv, (POWER_DOWN_T0));
#endif
		ret = 0;
		break;
#endif
#ifdef EN_EFUSE
	case SIOCEFUSE_GET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = efuse_get(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

	case SIOCEFUSE_SET:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = efuse_set(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

	case SIOCEFUSE_SYNC:
		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		i = efuse_sync(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;

#endif


#ifdef P2P_SUPPORT
	case SIOCP2PCMD:

		if(copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		
		//printk("ioctl-->process_p2p_cmd\n");
		
		i = process_p2p_cmd(priv, tmpbuf);
		if (i > 0) {
			if (copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
		}
		wrq->u.data.length = i;
		ret = 0;
		break;
		
#endif

#if defined(MBSSID) || defined(UNIVERSAL_REPEATER)
	case SIOCSICOPYMIB:
		memcpy(priv->pmib, GET_ROOT_PRIV(priv)->pmib, sizeof(struct wifi_mib));
		ret = 0;
		break;
#endif

#ifdef CONFIG_RTL8186_KB
	case SIOCGIREADGUESTMAC:
		i = get_guestmac(priv, (GUESTMAC_T *)tmpbuf);
		if (i >= 0) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;

	case SIOCSIWRTGUESTMAC:
		if (copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;
		set_guestmacvalid(priv, tmpbuf);
		ret = 0;
		break;
#endif

#ifdef CONFIG_RTL8672
	// MBSSID Port Mapping
	case SIOSIWRTLITFGROUP:
	{
		if (copy_from_user((char *)&ifgrp_member_tmp, wrq->u.data.pointer, 4))
			break;

		for (i=0; i<5; i++) {
			if ( wrq->u.data.flags == i ) {
				wlanDev[i].dev_ifgrp_member = bitmap_virt2phy(ifgrp_member_tmp);
				g_port_mapping = TRUE;
				break;
			}
		}
		break;
	}
#endif

#ifdef SUPPORT_SNMP_MIB
	case SIOCGSNMPMIB:
		if ((wrq->u.data.length > sizeof_tmpbuf) ||
			copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
			break;

		if (mib_get(priv, tmpbuf, tmpbuf, &i)) {
			if ((i > 0) && copy_to_user((void *)wrq->u.data.pointer, tmpbuf, i))
				break;
			wrq->u.data.length = i;
			ret = 0;
		}
		break;
#endif

#ifdef	SUPPORT_TX_MCAST2UNI
	case SIOCGIMCAST_ADD:
	case SIOCGIMCAST_DEL:

		if (!priv->pshare->rf_ft_var.mc2u_disable) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
			struct net_device *dev_vap;
			struct rtl8192cd_priv	*priv_vap;

			if (IS_ROOT_INTERFACE(priv))
#endif
			{
#ifdef MBSSID
				if (priv->pmib->miscEntry.vap_enable)
				{
					for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
						if (IS_DRV_OPEN(priv->pvap_priv[i])) {
//							rtl8192cd_ioctl(priv->pvap_priv[i]->dev, ifr, cmd);
							dev_vap = priv->pvap_priv[i]->dev;
							priv_vap = priv->pvap_priv[i];
							if (netif_running(dev_vap)) {
								if ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & WIFI_AP_STATE)
#ifdef CLIENT_MODE
									|| ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
									)
									AddDelMCASTGroup2STA(priv_vap,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
							}
						}
					}
				}
#endif
#ifdef UNIVERSAL_REPEATER
				if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
					dev_vap = (GET_VXD_PRIV(priv))->dev;
					priv_vap = (struct rtl8192cd_priv *)dev_vap->priv;
//					rtl8192cd_ioctl((GET_VXD_PRIV(priv))->dev, ifr, cmd);
					if (netif_running(dev_vap)) {
						if ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & WIFI_AP_STATE)
#ifdef CLIENT_MODE
							|| ((((GET_MIB(priv_vap))->dot11OperationEntry.opmode) & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
							)
							AddDelMCASTGroup2STA(priv_vap,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
					}
				}
#endif
			}
			if (netif_running(priv->dev)) {
				DEBUG_INFO("%s: %s MCAST Group mac %02x%02x%02x%02x%02x%02x\n",  priv->dev->name,
					((cmd == SIOCGIMCAST_ADD) ? "Add" : "Del"),
					((unsigned char *)ifr)[0],((unsigned char *)ifr)[1],
					((unsigned char *)ifr)[2],((unsigned char *)ifr)[3],((unsigned char *)ifr)[4],((unsigned char *)ifr)[5]);
				DEBUG_INFO("STA mac %02x%02x%02x%02x%02x%02x\n", ((unsigned char *)ifr)[6],((unsigned char *)ifr)[7],
					((unsigned char *)ifr)[8],((unsigned char *)ifr)[9],((unsigned char *)ifr)[10],((unsigned char *)ifr)[11]);

				if ((OPMODE & WIFI_AP_STATE)
#ifdef CLIENT_MODE
						|| ((OPMODE & (WIFI_STATION_STATE|WIFI_ASOC_STATE))==(WIFI_STATION_STATE|WIFI_ASOC_STATE))
#endif
					)
					AddDelMCASTGroup2STA(priv,(unsigned char *)ifr, (cmd == SIOCGIMCAST_ADD) ? 1 : 0);
			}
		}
		ret = 0;
		break;
#endif	// SUPPORT_TX_MCAST2UNI
#ifdef	CONFIG_RTK_MESH
	case SIOCQPATHTABLE:
   	{
		unsigned char destaddr[MACADDRLEN] = {0};

		copy_from_user(destaddr, (void *)(wrq->u.data.pointer), MACADDRLEN);
		//MESH_DEBUG_MSG("kernel destaddr = %s\n",destaddr);
		struct path_sel_entry *pEntry = pathsel_query_table( priv, destaddr); // modified by chuangch 2007.09.14
		ret = -1;
		if(pEntry!= (struct path_sel_entry *)-1)
		{
			if (copy_to_user((void *)wrq->u.data.pointer, (void *)pEntry, (int)&((struct path_sel_entry*)0)->start) == 0)
			{
				ret = 0;
				wrq->u.data.length = sizeof(struct path_sel_entry);
			}
		}
		break;
	}

	case SIOCUPATHTABLE:
	{
		struct path_sel_entry Entry;
		copy_from_user((struct path_sel_entry *)&Entry, (void *)(wrq->u.data.pointer), (int)&((struct path_sel_entry*)0)->start);
		ret = pathsel_modify_table_entry(priv, &Entry); // chuangch 2007.09.14
		break;
	}
	case SIOCAPATHTABLE:
	{
		struct path_sel_entry Entry, *pEntry;
		memset((void*)&Entry, 0, sizeof(Entry));
		copy_from_user((struct path_sel_entry *)&Entry, (void *)(wrq->u.data.pointer), (int)&((struct path_sel_entry*)0)->start);
		ret = 0;
		pEntry = pathsel_query_table( priv, Entry.destMAC );

		if( pEntry == (struct path_sel_entry *)-1 || pEntry->flag==0)
		{
			Entry.update_time = xtime; // chuangch 2007.09.19
			Entry.routeMaintain = xtime; // chuangch 10.19
			ret = pathsel_table_entry_insert_tail( priv, &Entry); //chuangch 2007.09.14

			MESH_DEBUG_MSG("create path to:%02X:%02X:%02X:%02X:%02X:%02X, Nexthop=%02X:%02X:%02X:%02X:%02X:%02X, Hop count=%d\n",
				Entry.destMAC[0], Entry.destMAC[1], Entry.destMAC[2], Entry.destMAC[3], Entry.destMAC[4], Entry.destMAC[5],
				Entry.nexthopMAC[0],  Entry.nexthopMAC[1], Entry.nexthopMAC[2], Entry.nexthopMAC[3], Entry.nexthopMAC[4], Entry.nexthopMAC[5],
				Entry.hopcount);
		}

		break;
	}

	//modify by Jason 2007.11.26
	case REMOVE_PATH_ENTRY:
	{
		unsigned char invalid_node_addr[MACADDRLEN] = {0};
		unsigned long flags;
		struct path_sel_entry *pEntry;

		if ( copy_from_user((void *)invalid_node_addr, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}

		MESH_DEBUG_MSG("REMOVE_PATH_ENTRY\n");
		MESH_DEBUG_MSG("invalid_node_addr =%2X-%2X-%2X-%2X-%2X-%2X-\n",invalid_node_addr[0],invalid_node_addr[1],invalid_node_addr[2],invalid_node_addr[3],invalid_node_addr[4],invalid_node_addr[5]);

		pEntry = pathsel_query_table( priv, invalid_node_addr );
		if(pEntry != (struct path_sel_entry *)-1 && pEntry->flag==0)
		{
#ifdef __LINUX_2_6__
/*by qjj_qin and hf_shi*/
#else
			SAVE_INT_AND_CLI(flags);
#endif
			ret = remove_path_entry(priv,invalid_node_addr);
#ifdef __LINUX_2_6__
/*by qjj_qin and hf_shi*/
#else
			RESTORE_INT(flags);
#endif
		}
		break;
	}

#ifdef FREDDY	//mesh related
	//modified for kick STA function by freddy 2008/12/2
	case RTL8190_IOCTL_DEL_STA_ENC:
		{
			int iReasonCode = _STATS_FAILURE_;
			char MacBuf[256];
			char *pStr = NULL;
			int iArg = 0;
			if ((wrq->u.data.length > sizeof_tmpbuf) ||
				copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;

			pStr = strtok(tmpbuf,",");
			while(pStr != 0)
			{
			   switch(iArg)
			   {
			   		case 0://MAC address

						strncpy(MacBuf,pStr,256);

			   		break;

					case 1://Reason code

						sscanf(pStr,"%d",&iReasonCode);

			   		break;
			   }
			   iArg++;
			   pStr = strtok(NULL,",");
			}

			ret = del_sta_enc(priv, MacBuf,iReasonCode);
		}
		break;
#endif
	case GET_STA_LIST:
	{
		struct stat_info	*pstat;

		struct proxy_table_entry *pEntry=NULL;
		static unsigned char node[MACADDRLEN] = {0};

		if ( copy_from_user((void *)node, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}

		// my station
		pstat = get_stainfo(priv, node);

		if(pstat != 0)
		{
			if(isSTA(pstat) && (pstat->state & WIFI_ASOC_STATE)) {
				ret = 0;
				break;
			}
			else // a neighbor
				goto ret_nothing;
		}

		pEntry = HASH_SEARCH(priv->proxy_table,node);

		// my proxied entry
		if(pEntry && (memcmp(GET_MY_HWADDR, pEntry->owner, MACADDRLEN)==0))
		{
			ret = 0;
			break;
		}

ret_nothing:
		// if not my station or not my proxied entry, then just fill garbage(0x0b) and return normally
		memset(node, 0x0b, sizeof(node));
	       	if (copy_to_user((void *)wrq->u.data.pointer, (void *)node, MACADDRLEN) != 0)
		{
			ret = -1;
			break;
		}
		ret = 0;
		break;
	}

	case SET_PORTAL_POOL:
	{
		if ( copy_from_user( (priv->pann_mpp_tb->pann_mpp_pool), (void *)(wrq->u.data.pointer), sizeof(struct pann_mpp_tb_entry) * MAX_MPP_NUM ) ) {
			ret = -1;
			break;
		}
		ret = 0;
  		break;
	}

	case SIOC_NOTIFY_PATH_CREATE:
	{
		unsigned char destaddr[MACADDRLEN] = {0};

		if ( copy_from_user((void *)destaddr, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}
		notify_path_found(destaddr,priv);
		// MESH_DEBUG_MSG("destaddr =%2X-%2X-%2X-%2X-%2X-%2X-\n",destaddr[0],destaddr[1],destaddr[2],destaddr[3],destaddr[4],destaddr[5]);
		ret = 0;
  		break;
	}

	case SIOC_UPDATE_ROOT_INFO:
	{
		if ( copy_from_user((void *)priv->root_mac, (void *)(wrq->u.data.pointer), MACADDRLEN) ) {
			ret = -1;
			break;
		}
		ret = 0;
#if 0
		LOG_MESH_MSG("Root MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
			priv->root_mac[0],priv->root_mac[1],priv->root_mac[2],priv->root_mac[3],priv->root_mac[4],priv->root_mac[5]);
#endif
  		break;
	}

	case SIOC_GET_ROUTING_INFO:
	{
		unsigned char buffer[128] = {0};
		buffer[0] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_root_enable);
		buffer[1] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_portal_enable);
#ifdef	_11s_TEST_MODE_
		buffer[2] = (unsigned short)(priv->pmib->dot1180211sInfo.mesh_reserved1);
#endif
		if (copy_to_user((void *)wrq->u.data.pointer, (void *)buffer, 128) == 0)
			ret = 0;
		else
			ret = -1;

  		break;
	}

case SIOC_SET_ROUTING_INFO:
	{
		unsigned char buffer[128] = {0};

		if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
		}

		if (copy_from_user((void *)buffer, (void *)wrq->u.data.pointer, 128) == 0)
			ret = 0;
		else
			ret = -1;

		priv->pmib->dot1180211sInfo.mesh_root_enable = buffer[0];
		priv->pmib->dot1180211sInfo.mesh_portal_enable = buffer[1];
  		break;
	}


#ifdef  _11s_TEST_MODE_
	case SAVE_RECEIBVER_PID:
	{
		if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
		}

		len = wrq->u.data.length;
		memset(strPID, 0, sizeof(strPID));
		copy_from_user(strPID, (void *)wrq->u.data.pointer, len);

		pid_receiver = 0;
		for(i = 0; i < len; i++) //char -> int
		{
			pid_receiver = pid_receiver * 10 + (strPID[i] - 48);
		}
		ret = 0;
		break;
	}

	case DEQUEUE_RECEIBVER_IOCTL:
	{
		if((ret = DOT11_DeQueue2((unsigned long)priv, priv->receiver_queue, QueueData, &QueueDataLen)) != 0) {
			copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY));
			wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
		} else {
			copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen);
			wrq->u.data.length = QueueDataLen;
		}
		break;
	}
#endif

	case SAVEPID_IOCTL:
	{
		if ( !wrq->u.data.pointer ){
			ret = -1;
			break;
		}

		len = wrq->u.data.length;
		memset(strPID, 0, sizeof(strPID));
		copy_from_user(strPID, (void *)wrq->u.data.pointer, len);

		pid_pathsel = 0;
		for(i = 0; i < len; i++) //char -> int
		{
			pid_pathsel = pid_pathsel * 10 + (strPID[i] - 48);
		}

		ret = 0;
		break;
	}

	case DEQUEUEDATA_IOCTL:
	{
		if((ret = DOT11_DeQueue2((unsigned long)priv, priv->pathsel_queue, QueueData, &QueueDataLen)) != 0) {
			copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY));
			wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
		} else {
			copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen);
			wrq->u.data.length = QueueDataLen;
		}

		break;
	}
#endif // CONFIG_RTK_MESH

#ifdef CONFIG_RTL_COMAPI_CFGFILE
	case SIOCCOMAPIFILE:
	{
		ret = CfgFileProc(dev);
		break;
	}
#endif

	case SIOC92DAUTOCH:
	{
		if (!(OPMODE & WIFI_AP_STATE)){
			DEBUG_ERR("can't do auto-channel select for non-AP mode!\n");
			break;
		}
#ifdef UNIVERSAL_REPEATER
		if (IS_VXD_INTERFACE(priv)) {
			DEBUG_ERR("can't do auto-channel select for vxd!\n");
			break;
		}
#endif
#ifdef MBSSID
		if (
			GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
			IS_VAP_INTERFACE(priv)) {
			DEBUG_ERR("can't do auto-channel select for vap!\n");
			break;
		}
#endif

#ifdef PCIE_POWER_SAVING
		PCIeWakeUp(priv, POWER_DOWN_T0);
#endif
		ret = rtl8192cd_autochannel_sel(priv);
		break;
	}

#ifdef CONFIG_RTL_92D_SUPPORT
	case SIOC92DIQK:
	{
		PHY_IQCalibrate(priv);
		ret = 0;
		break;
	}

#ifdef TEMP_MP_92D
	case SIOC92DTEMPMP:
	{
		if (wrq->u.data.pointer) {
			if ((wrq->u.data.length > sizeof_tmpbuf) ||
				copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;

			priv->mp_txrate = _atoi(tmpbuf, 16);
			printk("MP TX rate = %d \n", priv->mp_txrate);
			ret = 0;
		} else {
			ret = -1;
		}
		break;
	}
#endif

#ifdef SMART_CONCURRENT_92D
	case SIOC92DSMCC:
	{
		#if 0
		extern int smcc_92D_enable2x2_5G(struct rtl8192cd_priv * priv);
		extern int smcc_92D_enable2x2_2G(struct rtl8192cd_priv * priv);

		if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) {
			ret = smcc_92D_enable2x2_5G(priv);
			printk("switch to 2x2 5G (ret=%d)\n", ret);
		} else {
			ret = smcc_92D_enable2x2_2G(priv);
			printk("switch to 2x2 2G (ret=%d)\n", ret);
		}
		#else
		if (priv->pmib->dot11RFEntry.smcc==1 && priv->pmib->dot11RFEntry.macPhyMode != SINGLEMAC_SINGLEPHY){
			if (priv->smcc_state)
				priv->smcc_state = 0;
			else
				priv->smcc_state = 1;

			smcc_signin_linkstate(priv, 1, priv->pmib->dot11RFEntry.smcc_t, priv->smcc_state);
			ret = 0;
		}
		#endif
		break;
	}
#endif

#ifdef EN_EFUSE
	case SIOC92DSBANDADDR:
	{
		unsigned int phyband;
		u8 efuse_MAC=0;
		if (wrq->u.data.pointer) {
			if ((wrq->u.data.length > sizeof_tmpbuf) ||
				copy_from_user(tmpbuf, (void *)wrq->u.data.pointer, wrq->u.data.length))
				break;

			phyband = _atoi(tmpbuf, 16);
			printk("get phyband = %d \n",phyband);
			if (phyband==2)
				efuse_MAC = EEPROM_MAC0_MACADDRESS;
			else
				efuse_MAC = EEPROM_MAC1_MACADDRESS;

			if (/*priv->AutoloadFailFlag==FALSE &&*/ priv->pmib->efuseEntry.enable_efuse==1) {
#ifdef __KERNEL__
				struct sockaddr addr;
#endif
				unsigned char *hwinfo = &(priv->EfuseMap[EFUSE_INIT_MAP][0]);
				unsigned char *mac = hwinfo + efuse_MAC;
				unsigned char zero[] = {0, 0, 0, 0, 0, 0};
				/* printk("wlan%d EFUSE MAC [%02x:%02x:%02x:%02x:%02x:%02x]\n", priv->pshare->wlandev_idx,
						*mac, *(mac+1), *(mac+2), *(mac+3), *(mac+4), *(mac+5)); */
				if(memcmp(mac, zero, MACADDRLEN) && !IS_MCAST(mac)) {
#ifdef __KERNEL__
					memcpy(addr.sa_data, mac, MACADDRLEN);
					rtl8192cd_set_hwaddr(priv->dev, (void *)&addr);
#else
					rtl8192cd_set_hwaddr(priv->dev, (void *)mac);
#endif
				}
			}

			ret = 0;
		} else {
			ret = -1;
		}
		break;
	}
#endif // EN_EFUSE

#ifdef NON_INTR_ANTDIV
	case SIOC92DATNDIV:
	{
		if (priv->pmib->dot11RFEntry.macPhyMode == DUALMAC_DUALPHY){
			extern u32 if_priv[];
			unsigned long temp_18[2], temp_28[2], temp_0b[2];
			int i, ch[2];

			// Backup RF 18, 28, 0B
			for (i=0;i<2;i++) {
				temp_18[i] = DMDP_PHY_QueryRFReg(i, RF92CD_PATH_A, 0x18, bMask20Bits, 1);
				ch[i] = temp_18[i] & 0xff;
				temp_28[i] = DMDP_PHY_QueryRFReg(i, RF92CD_PATH_A, 0x28, bMask20Bits, 1);
				temp_0b[i] = DMDP_PHY_QueryRFReg(i, RF92CD_PATH_A, 0x0b, bMask20Bits, 1);
				printk("RF[%d] 18=0x%05x 28=0x%05x 0B=0x%05x\n",i, temp_18[i], temp_28[i], temp_0b[i]);
			}


			PHY_SetBBReg(priv, 0xb30, BIT(27), 1);

			// Restore RF 18, 28, 0B
			for (i=0;i<2;i++) {
				DMDP_PHY_SetRFReg(i, RF92CD_PATH_A, 0x18, bMask20Bits, temp_18[i]);
				//DMDP_PHY_SetRFReg(i, RF92CD_PATH_A, 0x28, BIT(7)|BIT(6), (temp_28[i]&(BIT(7)|BIT(6)))>>6);
				DMDP_PHY_SetRFReg(i, RF92CD_PATH_A, 0x0b, bMask20Bits, temp_0b[i]);
			}
			for (i=0;i<2;i++)
				SetIMR_n((struct rtl8192cd_priv *)if_priv[i], ch[i]);
			for (i=0;i<2;i++)
				PHY_IQCalibrate((struct rtl8192cd_priv *)if_priv[i]);

			printk("Non-interrupt antenna switched!\n");
			ret = 0;
		}else {
			printk("NOT DMDP, cannot support antenna switch\n");
			ret = -1;
		}
		break;
	}
#endif // EN_EFUSE
#ifdef DPK_92D
	case SIOC92DDPK:
	{
		if (priv->pmib->dot11RFEntry.phyBandSelect==PHY_BAND_5G){
			if (priv->pshare->rf_ft_var.dpk_on){
				int ch = PHY_QueryRFReg(priv,RF92CD_PATH_A,0x18,0xff,1);
				unsigned int curMaxRFPath, eRFPath;
				if (priv->pmib->dot11RFEntry.macPhyMode == DUALMAC_DUALPHY)
					curMaxRFPath = RF92CD_PATH_B;
				else
					curMaxRFPath = RF92CD_PATH_MAX;

				for(eRFPath = RF92CD_PATH_A; eRFPath < curMaxRFPath; eRFPath++){
					if (eRFPath == RF92CD_PATH_A)
						PHY_SetBBReg(priv, 0xb68, bMaskDWord, 0x28080000);
					else
						PHY_SetBBReg(priv, 0xb6c, bMaskDWord, 0x28080000);
						
					if (ch<=64){
						PHY_SetRFReg(priv,eRFPath,0x03,bMask20Bits,0x94a12);
						PHY_SetRFReg(priv,eRFPath,0x04,bMask20Bits,0x94a12);
						PHY_SetRFReg(priv,eRFPath,0x0e,bMask20Bits,0x94a12);
					}else if (ch<=140){
						PHY_SetRFReg(priv,eRFPath,0x03,bMask20Bits,0x94a52);
						PHY_SetRFReg(priv,eRFPath,0x04,bMask20Bits,0x94a52);
						PHY_SetRFReg(priv,eRFPath,0x0e,bMask20Bits,0x94a52);
					}else{
						PHY_SetRFReg(priv,eRFPath,0x03,bMask20Bits,0x94a12);
						PHY_SetRFReg(priv,eRFPath,0x04,bMask20Bits,0x94a12);
						PHY_SetRFReg(priv,eRFPath,0x0e,bMask20Bits,0x94a12);
					}

					PHY_SetRFReg(priv,eRFPath,0x16,bMask20Bits,0xe1874);
					PHY_SetRFReg(priv,eRFPath,0x16,bMask20Bits,0xa1874);
					PHY_SetRFReg(priv,eRFPath,0x16,bMask20Bits,0x61874);
					PHY_SetRFReg(priv,eRFPath,0x16,bMask20Bits,0x21874);

				}
				priv->pshare->rf_ft_var.dpk_on = 0;
				panic_printk("DPK OFF!\n");
			}else{
				priv->pshare->rf_ft_var.dpk_on = 1;
				panic_printk("DPK ON!\n");
				//PHY_DPCalibrate(priv);
			}
			ret = 0;
		}else {
			panic_printk("NO DPK for 2G!\n");
			ret = -1;
		}
		break;
	}
#endif
#endif // CONFIG_RTL_92D_SUPPORT
#ifdef FOR_VHT5G_PF
	//8812_PF4
	case SIOC8812SIGMA:
	{
		panic_printk("Reset Default for SIGMA!!\n");
		reset_default_sigma(priv);
		ret=0;
		break;
	}
#endif
	}

	RESTORE_INT(flags);
	SMP_UNLOCK(flags);
	return ret;
}


void delay_us(unsigned int t)
{
#ifdef __LINUX_2_6__
#ifdef CONFIG_RTL8672
	udelay(t);
#else
	__udelay(t);
#endif
#else
	__udelay(t, __udelay_val);
#endif
}


void delay_ms(unsigned int t)
{
	mdelay(t);
}

#ifdef _SINUX_
enum iwpriv_type {
    IW_NA,
    IW_RD,
    IW_WR,
    IW_DP
};

typedef int cmd_rw_handler(struct rtl8192cd_priv *, unsigned char *);
typedef void cmd_dump_handler(struct rtl8192cd_priv *, unsigned char *);

struct cmd_map_stru {
    char *cmd_str;
    void *cmd_handler;
    int  type;
};

static struct cmd_map_stru iwpriv_cmds[] = {
    {"set_mib",     set_mib,    IW_WR},
    {"get_mib",     get_mib,    IW_RD},
    {"write_mem",   write_mem,  IW_WR},
    {"read_mem",    read_mem,   IW_RD},
    {"read_reg",    read_reg,   IW_RD},
    {"write_reg",   write_reg,  IW_WR},
    {"read_rf",     read_rf_reg,   IW_RD},
    {"write_rf",    write_rf_reg,  IW_WR},
    {"reg_dump",    reg_dump,      IW_DP},
    {NULL,          NULL,       IW_NA}
};


int rtl8192cd_iwpriv_cmd_process(unsigned char *ifname, unsigned char *data)
{
    static unsigned char buf[1024]={0};
    struct cmd_map_stru * p_cmd;
    unsigned char *para;
    int ret;
    struct net_device *net_dev;
    struct rtl8192cd_priv *priv;
    cmd_rw_handler *cmd_rw;
    cmd_dump_handler *cmd_dump;


    struct cmd_map_stru * cmds = iwpriv_cmds;
    int i = 0 ;

    printk("\n");
    printk("rtl8192cd_iwpriv_cmd_process: cmd=%s\n", data);

    while (cmds[i].cmd_str != NULL ) {
        if ( memcmp(data, cmds[i].cmd_str, strlen(cmds[i].cmd_str)) == 0 )
           break;
        else
            i++;
    }

    if (cmds[i].cmd_str == NULL || cmds[i].cmd_handler == NULL) {
       printk(" wireless ioctl command '%s' invalid !\n", data);
        return -1;
    }

    // get priv
    net_dev = dev_get_by_name(ifname);
    if (net_dev == NULL)
    {
        printk("rtl8192cd_iwpriv_cmd_process: can not get dev %s\n", data);
        return -ENETDOWN;
    }
    priv = (struct rtl8192cd_priv *)net_dev -> priv;

    // put command parameter into buf
    para = data + strlen(cmds[i].cmd_str);
    para += strspn(para, " \t");

    strncpy(buf, para, 1024);
    buf[1023] = '\0';

    // run command
    printk("wireless ioctl: cmd=%s, para=%s\n", cmds[i].cmd_str, buf);

    if (cmds[i].type == IW_DP) { // dump command
        cmd_dump = (cmd_dump_handler *)cmds[i].cmd_handler;
        cmd_dump(priv, buf);
        dev_put(net_dev);
        return 0;
    }
    else {  // read/write command
        cmd_rw = (cmd_rw_handler *)cmds[i].cmd_handler;
        ret = cmd_rw(priv, buf);
	  dev_put(net_dev);
        if (ret < 0) {
            printk("run fail!\n");
            return ret;
        }
    }

    printk("run successful !\n");

    if (cmds[i].type == IW_RD) {
        buf[ret] = '\0'; // add '\0' end for string
        printk("    result length: %d\n", ret);
        printk("    result text  : %s\n", buf);
        for (i=0; i<ret; i++)
            printk(" %02X", buf[i]);
        printk("\n");
    }

    return 0;
}


int sos_ioctl_priv_get(char *ifname, char *cmd, char* data)
{
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;
    int retlen;


    net_dev = dev_get_by_name(ifname);

    if (net_dev == NULL)
    {
        printk("sos_ioctl_priv_stat: can not get dev %s\n", data);
        return -ENETDOWN;
    }

    priv = (struct rtl8192cd_priv *)net_dev -> priv;

    retlen = get_mib(priv, data);

    dev_put(net_dev);

    if (retlen > 0)
        return retlen;
    else
        return -1;
}


int sos_ioctl_priv_set(
    char *netName,
    char	*name,
    void	*value)
{
    int ret = 0;
	unsigned long flags;
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;
    int sizeof_tmpbuf;
    static unsigned char tmpbuf[1024];

    char *this_char = name;
    char *this_value = (char *)value;

    sizeof_tmpbuf = sizeof(tmpbuf);
    memset(tmpbuf, '\0', sizeof_tmpbuf);

    net_dev = dev_get_by_name(netName);

    if (net_dev == NULL)
    {
        printk("sos_ioctl_priv_set: %scan not get dev\n", netName);
        return -ENETDOWN;
    }

	priv = (struct rtl8192cd_priv *)net_dev->priv;

	SAVE_INT_AND_CLI(flags);

    memcpy(tmpbuf, this_char, strlen(this_char));
    ret = set_mib(priv, tmpbuf);
    dev_put(net_dev);
    if (ret == 0)
        strcpy(this_value, tmpbuf);

    printk("sos_ioctl_priv_set: set_mib return %d\n", ret);

	RESTORE_INT(flags);

    return ret;
}

struct wifi_mib * get_wlandev_mib(struct net_device *dev)
{
 	struct rtl8192cd_priv * priv;

    if (dev == NULL)
       return NULL;

    priv  = (struct rtl8192cd_priv *) (dev -> priv);

    return priv -> pmib;
}

struct spinlock_t * get_wlandev_lock (struct net_device *dev)
{
 	struct rtl8192cd_priv * priv;

    if (dev == NULL)
        return NULL;

    priv  = (struct rtl8192cd_priv *) (dev -> priv);

    return & (priv -> pshare -> lock);

}

#if 0
/* rtl8192cd_getMacTable return mac num, if fail return -1 */
int rtl8192cd_get_staInfo(char *wlan_ifname, sta_info_2_web **ppsta_info, int *sta_len)
{
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;
    int sizeof_tmpbuf;
    unsigned char *tmp1;
    sta_info_2_web *psta_info;
    int i,j=0;

    net_dev = dev_get_by_name(wlan_ifname);

    if (net_dev == NULL)
    {
        printk("rtl8192cd_get_staInfo(): can not get dev %s\n", wlan_ifname);
        return -1;
    }

    priv = (struct rtl8192cd_priv *)net_dev -> priv;

    dev_put(net_dev);


	sizeof_tmpbuf = sizeof(sta_info_2_web) * (NUM_STAT + 1); // for the max of all sta info
    tmp1 = (unsigned char *)kmalloc(sizeof_tmpbuf, GFP_ATOMIC);

    if (!tmp1) {
		printk("Unable to allocate temp buffer for rtl8192cd_get_staInfo()!\n");
		return -1;
	}
	memset(tmp1, '\0', sizeof(sta_info_2_web));

	get_sta_info(priv, (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)), NUM_STAT);

/*
    for (i=0,j=0; i< maxLen; i++) {
        psta_info = (sta_info_2_web *)(tmp1 + sizeof(sta_info_2_web)*(i+1));
        if (memcmp(psta_info->addr, "\x0\x0\x0\x0\x0\x0", 6)!=0) {
            memcpy(macTable+j*6, psta_info->addr, 6);
            j++;
        }
    }
*/
    *ppsta_info = tmp1;
    *sta_len = NUM_STAT+1;

    return 0;
}


void rtl8192cd_put_staInfo(sta_info_2_web *psta_info)
{
    if (psta_info != NULL)
        kfree(psta_info);

    return;
}

#endif

int rtl8192cd_getAutoChannel(char *wlan_ifname, int *channel)
{
    struct net_device	*net_dev;
	struct rtl8192cd_priv	*priv;
	unsigned int ret;

    net_dev = dev_get_by_name(wlan_ifname);

    if (net_dev == NULL)
    {
        printk("sos_ioctl_priv_stat: can not get dev %s\n", wlan_ifname);
        return -1;
    }

    priv = (struct rtl8192cd_priv *)net_dev -> priv;

    *channel = priv->pmib->dot11RFEntry.dot11channel;

    ret=priv->pmib->dot11nConfigEntry.dot11n2ndChOffset;

    dev_put(net_dev);

    return ret;
}

int rtl8192cd_get_sta_num(char *wlan_ifname)
{
    struct net_device	*net_dev=NULL;
	struct rtl8192cd_priv	*priv=NULL;
	int number=0;

    net_dev = dev_get_by_name(wlan_ifname);
    if (net_dev )
    {
        priv = (struct rtl8192cd_priv *)net_dev -> priv;
        number = priv->assoc_num;
        dev_put(net_dev);
    }

	return number;
}

#ifdef _SINUX_
/*   Dynamic adjust RF power
 *
 *   para values is char array, length must be 28.
 *
 */
int rtl8192cd_adjust_rf_power(struct net_device *wdev, char *values)
{
	struct rtl8192cd_priv *priv         = wdev->priv;
    struct rf_finetune_var	* pvar   = &(priv->pshare->rf_ft_var);

    pvar->txPowerPlus_cck_1     = values[0];
	pvar->txPowerPlus_cck_2     = values[1];
	pvar->txPowerPlus_cck_5     = values[2];
	pvar->txPowerPlus_cck_11    = values[3];
	pvar->txPowerPlus_ofdm_6    = values[4];
	pvar->txPowerPlus_ofdm_9    = values[5];
	pvar->txPowerPlus_ofdm_12   = values[6];
	pvar->txPowerPlus_ofdm_18   = values[7];
	pvar->txPowerPlus_ofdm_24   = values[8];
	pvar->txPowerPlus_ofdm_36   = values[9];
	pvar->txPowerPlus_ofdm_48   = values[10];
	pvar->txPowerPlus_ofdm_54   = values[11];
	pvar->txPowerPlus_mcs_0     = values[12];
	pvar->txPowerPlus_mcs_1     = values[13];
	pvar->txPowerPlus_mcs_2     = values[14];
	pvar->txPowerPlus_mcs_3     = values[15];
	pvar->txPowerPlus_mcs_4     = values[16];
	pvar->txPowerPlus_mcs_5     = values[17];
	pvar->txPowerPlus_mcs_6     = values[18];
	pvar->txPowerPlus_mcs_7     = values[19];
	pvar->txPowerPlus_mcs_8     = values[20];
	pvar->txPowerPlus_mcs_9     = values[21];
	pvar->txPowerPlus_mcs_10    = values[22];
	pvar->txPowerPlus_mcs_11    = values[23];
	pvar->txPowerPlus_mcs_12    = values[24];
	pvar->txPowerPlus_mcs_13    = values[25];
	pvar->txPowerPlus_mcs_14    = values[26];
	pvar->txPowerPlus_mcs_15    = values[27];

    return 0;
}

EXPORT_SYMBOL(rtl8192cd_adjust_rf_power);
#endif

EXPORT_SYMBOL(AddDelMCASTGroup2STA);
EXPORT_SYMBOL(rtl8192cd_get_sta_num);
EXPORT_SYMBOL(get_wlandev_lock);
EXPORT_SYMBOL(get_wlandev_mib);
EXPORT_SYMBOL(sos_ioctl_priv_set);
EXPORT_SYMBOL(sos_ioctl_priv_get);
EXPORT_SYMBOL(rtl8192cd_getMacTable);
EXPORT_SYMBOL(rtl8192cd_check_wlan_mac);
EXPORT_SYMBOL(rtl8192cd_getAutoChannel);
EXPORT_SYMBOL(rtl8192cd_iwpriv_cmd_process);

#endif

