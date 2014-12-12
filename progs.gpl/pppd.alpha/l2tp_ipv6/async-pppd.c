/* vi: set sw=4 ts=4: */
/* async-pppd.c */

#include "l2tp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ppp_fcs.h"
#include "dtrace.h"


#define FRAME_ESC	0x7d
#define FRAME_FLAG	0x7e
#define FRAME_ADDR	0xff
#define FRAME_CTRL	0x03
#define FRAME_ENC	0x20

#define STATE_WAITFOR_FRAME_ADDR	0
#define STATE_DROP_PROTO			1
#define STATE_BUILDING_PACKET		2

static int PPPState = STATE_WAITFOR_FRAME_ADDR;
static int PPPPacketSize = 0;
static unsigned char PPPXorValue = 0;

#define PPP_BUF_SIZE	4096

static unsigned char pppBuf[PPP_BUF_SIZE + EXTRA_HEADER_ROOM];

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
void l2tp_async_ppp_handle_frame_from_tunnel(l2tp_session *ses, unsigned char *buf, size_t len)
{
	int n;
	unsigned char header[2] = {FRAME_ADDR, FRAME_CTRL};
	unsigned char tail[2];
	unsigned short fcs;
	unsigned char *ptr = pppBuf;
	unsigned char c;
	int i;

	/* compute FCS */
	fcs = pppfcs16(PPPINITFCS16, header, 2);
	fcs = pppfcs16(fcs, buf, len) ^ 0xffff;
	tail[0] = fcs & 0x00ff;
	tail[1] = (fcs >> 8) & 0x00ff;

	/* build a buffer to send to PPP */
	*ptr++ = FRAME_FLAG;
	*ptr++ = FRAME_ADDR;
	*ptr++ = FRAME_ESC;
	*ptr++ = FRAME_CTRL ^ FRAME_ENC;

	for (i=0; i<len; i++)
	{
		c = buf[i];
		if (c == FRAME_FLAG || c==FRAME_ADDR || c==FRAME_ESC || c < 0x20)
		{
			*ptr++ = FRAME_ESC;
			*ptr++ = c ^ FRAME_ENC;
		}
		else
		{
			*ptr++ = c;
		}
	}
	for (i=0; i<2; i++)
	{
		c = tail[i];
		if (c==FRAME_FLAG || c==FRAME_ADDR || c==FRAME_ESC || c < 0x20)
		{
			*ptr++ = FRAME_ESC;
			*ptr++ = c ^ FRAME_ENC;
		}
		else
		{
			*ptr++ = c;
		}
	}
	*ptr++ = FRAME_FLAG;

	/* TODO: Add error checking */
	n = write(ses->pty_fd, pppBuf, (ptr-pppBuf));
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
void l2tp_async_ppp_handle_frame_to_tunnel(l2tp_session * ses)
{
	int r;
	unsigned char * ptr = pppBuf;
	unsigned char c;
	static unsigned char packet[1536];

	/* EXTRA_HEADER_ROOM bytes extra space for l2tp header */
	r = read(ses->pty_fd, ptr, PPP_BUF_SIZE);
	if (r < 0) return;
	if (r==0) return;

	while (r)
	{
		if (PPPState == STATE_WAITFOR_FRAME_ADDR)
		{
			while (r)
			{
				--r;
				if (*ptr++ == FRAME_ADDR)
				{
					PPPState = STATE_DROP_PROTO;
					break;
				}
			}
		}
		/* Still waiting ... */
		if (PPPState == STATE_WAITFOR_FRAME_ADDR) return;
		while (r && PPPState == STATE_DROP_PROTO)
		{
			--r;
			c = *ptr++;
			if (c == (FRAME_CTRL ^ FRAME_ENC))
			{
				PPPState = STATE_BUILDING_PACKET;
			}
			else if (c == FRAME_CTRL)
			{
				PPPState = STATE_BUILDING_PACKET;
			}
		}
		if (PPPState == STATE_DROP_PROTO) return;

		/* Start building frame */
		while (r && PPPState == STATE_BUILDING_PACKET)
		{
			--r;
			c = *ptr++;
			switch (c)
			{
			case FRAME_ESC:
				PPPXorValue = FRAME_ENC;
				break;
			case FRAME_FLAG:
				if (PPPPacketSize < 2) return;
				l2tp_dgram_send_ppp_frame(ses, packet, PPPPacketSize - 2);
				PPPPacketSize = 0;
				PPPXorValue = 0;
				PPPState = STATE_WAITFOR_FRAME_ADDR;
				break;
			default:
				if (PPPPacketSize >= 1536)
				{
					PPPPacketSize = 0;
					PPPXorValue = 0;
					PPPState = STATE_WAITFOR_FRAME_ADDR;
					break;
				}
				else
				{
					packet[PPPPacketSize++] = c ^ PPPXorValue;
					PPPXorValue = 0;
				}
			}
		}
	}
	return;
}
