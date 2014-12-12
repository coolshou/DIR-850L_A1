#ifndef _DEBUG_H
#define _DEBUG_H

#include "libbb_udhcp.h"

#include <stdio.h>
#ifdef SYSLOG
	#include <syslog.h>
#endif

#ifdef SYSLOG
	#define LOG(level, str, args...) do { printf(str, ## args); \
				printf("\n"); \
				syslog(level, str, ## args); } while(0)
	#define OPEN_LOG(name) openlog(name, 0, 0)
	#define CLOSE_LOG() closelog()
#else
	#define LOG_EMERG	"EMERGENCY!"
	#define LOG_ALERT	"ALERT!"
	#define LOG_CRIT	"critical!"
	#define LOG_WARNING	"warning"
	#define LOG_ERR	"error"
	#define LOG_INFO	"info"
	#define LOG_DEBUG	"debug"

	#if 0
	/*
		# define LOG(level, str, args...) do { printf("%s, ", level); \
					printf(str, ## args); \
					printf("\n"); } while(0)
	*/
		#define LOG(level, str, args...) printf(str "\n" , ##args)
	#else
		#define LOG(level, str, args...)
	#endif

	#define OPEN_LOG(name) do {;} while(0)
	#define CLOSE_LOG() do {;} while(0)
#endif /* end of SYSLOG */

#ifdef DEBUG
	#define UDHCP_DEBUG
	#ifdef UDHCP_DEBUG
		extern void _uprintf_(const char *file,const char *func, int line, const char * format, ...);
		#define uprintf(args...)   _uprintf_(__FILE__,__FUNCTION__,__LINE__,##args)

		#ifdef DEBUG
			#undef DEBUG
		#endif
		#define DEBUG(level, args...)  _uprintf_(__FILE__,__FUNCTION__,__LINE__,##args)

		#ifdef LOG
			#undef LOG
		#endif
		#define LOG(level, args...)    _uprintf_(__FILE__,__FUNCTION__,__LINE__,##args)
	#else
		#define uprintf(args...)
		//# define DEBUG(level, str, args...) LOG(level, str, ## args)
		#define DEBUG(level, str, args...) printf(str "\n" , ##args)
		#define DEBUGGING
	#endif
#else
	#undef UDHCP_DEBUG
	#ifdef UDHCP_DEBUG
		extern void _uprintf_(const char *file,const char *func, int line, const char * format, ...);
		#define uprintf(args...)   _uprintf_(__FILE__,__FUNCTION__,__LINE__,##args)
	#else
		#define uprintf(args...)
	#endif

	//# define DEBUG(level, str, args...) do {;} while(0)
	#define DEBUG(level, str, args...)
#endif

// for _uprintf_
#ifdef ELBOX_PROGS_GPL_UDHCP_MULTI_INSTANCE
	#define _wlanif_ server_config.wlanif
#else
	#define _wlanif_ ""
#endif

#endif /* end of _DEBUG_H */

