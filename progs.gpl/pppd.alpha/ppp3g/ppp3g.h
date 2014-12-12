/* vi: set sw=4 ts=4: */
/*******************************************************************
 *
 * ppp3g.h
 *
 * Header file for PPP3G definitions.
 *
 *******************************************************************/

#ifndef __PPP3G_HEADER_FILE__
#define __PPP3G_HEADER_FILE__

/* #include <stdint.h>
#include <netinet/in.h> */


/* in pppoe_options.c */
extern int  ppp3g_debug;			
extern char *ppp3g_chat_file;
extern char *ppp3g_dc_chat_file;
extern char speed_str[];		


extern int 	real_tty_fd;


#endif	/* end of #ifndef __PPP3G_HEADER_FILE__ */
