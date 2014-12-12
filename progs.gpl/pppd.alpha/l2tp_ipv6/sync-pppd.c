/* vi: set sw=4 ts=4: */
/* sync-pppd.c */

#include "l2tp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dtrace.h"

#if 0
extern void print_packet(int fd, void *pack, unsigned int len, const char * msg);
#endif

/**********************************************************************
* %FUNCTION: handle_frame_from_tunnel
* %ARGUMENTS:
*  ses -- l2tp session
*  buf -- received PPP frame
*  len -- length of frame
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Shoots the frame to PPP's pty
***********************************************************************/
void l2tp_sync_ppp_handle_frame_from_tunnel(l2tp_session *ses, unsigned char *buf, size_t len)
{
    int n;

	d_dbg("l2tp_sync_ppp_handle_frame_from_tunnel >>>\n");
	
    /* Add framing bytes */
    *--buf = 0x03;
    *--buf = 0xFF;
    len += 2;
#if 0
	print_packet(2, buf, len, "l2tp_sync_ppp_handle_from_from_tunnel");
#endif
	
	//+++ fix by siyou. 2011/1/6 11:17am
	//marco, add 20 ms sleep here, or in some situation, two packets will combine as one packet
	//which will affect the state machine and we will not reply chap response		
	//usleep(20000);
	//---
	
    /* TODO: Add error checking */
    n = write(ses->pty_fd, buf, len);
	d_dbg("l2tp_sync_ppp_handle_frame_from_tunnel <<<\n");
}


/**********************************************************************
* %FUNCTION: handle_frame_to_tunnel
* %ARGUMENTS:
*  ses -- l2tp session
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles readability on PTY; shoots PPP frame over tunnel
***********************************************************************/
void l2tp_sync_ppp_handle_frame_to_tunnel(l2tp_session * ses)
{
	static unsigned char buf[4096+EXTRA_HEADER_ROOM];
	unsigned char * payload;
	int n;
	int iters = 5;

	d_dbg("%s >>>\n" , __FUNCTION__);
	
	/* It seems to be better to read in a loop than to go
	 * back to select loop.  However, don't loop forever, or
	 * we could have a DoS potential */
	payload = buf + EXTRA_HEADER_ROOM;

	while (iters--)
	{	/* EXTRA_HEADER_ROOM bytes extra space for l2tp header */
		n = read(ses->pty_fd, payload, sizeof(buf)-EXTRA_HEADER_ROOM);
		/* TODO: Check this.... */
		if (n <= 2) break;
		if (!ses) continue;

		/* Chop off framing bytes */
#if 0
		print_packet(2, payload, n, "l2tp_sync_ppp_handle_from_to_tunnel");
#endif
		if (payload[0] == 0xff && payload[1] == 0x03)
		{
			payload += 2;
			n -= 2;
		}
		l2tp_dgram_send_ppp_frame(ses, payload, n);
	}

	d_dbg("l2tp_sync_ppp_handle_from_to_tunnel <<<\n");
}
