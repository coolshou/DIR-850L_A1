/* Alpha syslog */

#ifndef __ASYSLOG_HEADER__
#define __ASYSLOG_HEADER__

/* Option flags for Alpha Networks Log Message*/
#define ALOG_SYSACT	(24<<3)	/* system activity (192) */
#define ALOG_DEBUG	(25<<3)	/* debug information (200) */
#define ALOG_ATTACK	(26<<3)	/* attacks (208) */
#define ALOG_DROP	(27<<3)	/* dropped packet (216) */
#define ALOG_NOTICE	(28<<3)	/* notice (224) */
#define ALOG_WIRELESS 	(29<<3)	/* wireless information (232) *//*syslogd_2007_01_23 , Jordan*/
#define ALOG_AP_SYSACT 	(30<<3)	/* system activity for ap (apsysact)(240) *//*syslogd_2007_02_02 , Jordan*/
#define ALOG_AP_NOTICE 	(31<<3)	/* notice for ap (apnotice)(248) *//*syslogd_2007_02_02 , Jordan*/
#define ALOG_TIME 	(32<<3)	/* time activity for ap (256) *//*syslogd_2007_04_13 , Jordan*/
/* redefine LOG_NFACILITIES */
#ifdef LOG_NFACILITIES
#undef LOG_NFACILITIES
#endif
#define	LOG_NFACILITIES	32	/* current number of facilities */ /*syslogd_2007_01_23 , Jordan*/


#ifdef ASYSLOG_NAMES
CODE afacilitynames[] =
{
	{"sysact", ALOG_SYSACT},
	{"debug", ALOG_DEBUG},
	{"attack", ALOG_ATTACK},
	{"drop", ALOG_DROP},
	{"notice", ALOG_NOTICE},
	{"wireless", ALOG_WIRELESS},/*syslogd_2007_01_23 , Jordan*/
	{"apsysact", ALOG_AP_SYSACT},/*syslogd_2007_02_02 , Jordan*/
	{"apnotice", ALOG_AP_NOTICE},/*syslogd_2007_02_02 , Jordan*/
	{"time", ALOG_TIME},/*syslogd_2007_04_13 , Jordan*/
	{ NULL, -1 }
};
#endif

#endif
