/* vi: set sw=4 ts=4: */
/* dtrace.h */

#ifndef __DTRACE_HEADER_FILE__
#define __DTRACE_HEADER_FILE__

#define DBG_ALL		0
#define DBG_DEBUG	10
#define DBG_INFO	20
#define DBG_WARN	30
#define DBG_ERROR	40
#define DBG_FATAL	50
#define DBG_NONE	100

#define DBG_DEFAULT	DBG_ALL

extern void __dtrace(int level, const char * format, ...);
extern void __dassert(char * exp, char * file, int line);

#ifdef DDEBUG

#define d_dbg(x, args...)   __dtrace(DBG_DEBUG, x, ##args)
#define d_info(x, args...)  __dtrace(DBG_INFO, x, ##args)
#define d_warn(x, args...)  __dtrace(DBG_WARN, x, ##args)
#define d_error(x, args...) __dtrace(DBG_ERROR, x, ##args)
#define d_fatal(x, args...) __dtrace(DBG_FATAL, x, ##args)
#define dtrace(x, args...)  __dtrace(DBG_ALL, x, ##args)
#define dassert(exp) (void)((exp) || (__dassert(#exp,__FILE__,__LINE__),0))

#else

#define d_dbg(x, args...)
#define d_info(x, args...)
#define d_warn(x, args...)
#define d_error(x, args...)
#define d_fatal(x, args...)
#define dtrace(x, args...)
#define dassert(x)


#endif /* end of #ifdef DDEBUG */

#endif /* end of #ifndef __DTRACE_HEADER_FILE__ */
