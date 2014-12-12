/* vi: set sw=4 ts=4: */
/* pptp_gre.c  -- encapsulate PPP in PPTP-GRE.
 *                Handle the IP Protocol 47 portion of PPTP.
 *                C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_gre.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "ppp_fcs.h"
#include "pptp_msg.h"
#include "pptp_gre.h"
#include "pptp_ctrl.h"
//#include "util.h"
#include "pqueue.h"
#include "eloop.h"
#include "dtrace.h"

#ifndef N_HDLC
#include <linux/termios.h>
#endif

#define PACKET_MAX 8196
/* test for a 32 bit counter overflow */
#define WRAPPED( curseq, lastseq) \
    ((((curseq) & 0xffffff00)==0) && (((lastseq) & 0xffffff00 )==0xffffff00))
#define REORDER_LOGGING 1


static PPTP_CONN * g_conn = NULL;
struct in_addr localbind = { INADDR_NONE };
static int pptp_fd = -1;
static int gre_fd = -1;
static u_int32_t ack_sent, ack_recv;
static u_int32_t seq_sent, seq_recv;
static u_int16_t pptp_gre_call_id, pptp_gre_peer_call_id;
gre_stats_t stats;

static unsigned char hdlc_buff[PACKET_MAX * 2 + 2];
static unsigned char copy[PACKET_MAX];
static unsigned char gre_buff[PACKET_MAX+64];
static unsigned int len = 0, escape = 0;
static int checkedsync = 0;
static int first = 1;
static u_int32_t seq = 1;	/* first sequence number sent must be 1 */
static int pptp_ready = 0;
static int pptp_exit = 0;


typedef int (*callback_t) (int cl, void *pack, unsigned int len);

/* init_vars */
static void init_vars(void)
{
	g_conn = NULL;
	pptp_fd = -1;
	gre_fd = -1;
	memset(&stats, 0, sizeof(stats));
	ack_sent = ack_recv = seq_sent = seq_recv = 0;

	len = escape = 0;
	checkedsync = 0;
	first = 1;
	seq = 1;
	pptp_ready = pptp_exit = 0;
}


/* decaps_hdlc gets all the packets possible with ONE blocking read */
/* returns <0 if read() call fails */
static int decaps_hdlc(int fd, callback_t callback, int cl);
static int encaps_hdlc(int fd, void *pack, unsigned int len);
static int decaps_gre(int fd, callback_t callback, int cl);
static int encaps_gre(int fd, void *pack, unsigned int len);
static int dequeue_gre(callback_t callback, int cl);

extern char pptp_server_ip[];
extern int pptp_sync;
extern char pptp_localbind[];
extern void pptp_handler(int sock, void * eloop_ctx, void * sock_ctx);
extern void modem_hungup(void);
extern int hungup;
extern int pptp_destroy_conn;

extern u_int32_t exclude_peer;	/* defined in IPCP. */

#if 0
#include <stdio.h>
void print_packet(int fd, void *pack, unsigned int len, const char * msg)
{
	unsigned char *b = (unsigned char *) pack;
	unsigned int i, j;
	FILE *out = fdopen(fd, "w");

	if (msg)
		fprintf(out, "-- begin packet (%u) %s --\n", len, msg);
	else
		fprintf(out, "-- begin packet (%u) --\n", len);
	for (i = 0; i < len; i += 16)
	{
		for (j = 0; j < 8; j++)
		{
			if (i+2*j+1 < len)
				fprintf(out, "%02x%02x ", (unsigned int)b[i+2*j], (unsigned int)b[i+2*j+1]);
			else if (i+2*j < len)
				fprintf(out, "%02x ", (unsigned int)b[i+2*j]);
		}
		fprintf(out, "\n");
	}
	fprintf(out, "-- end packet --\n");
	fflush(out);
}
#endif

//#define __WRGN15_RALINK__
#ifdef __WRGN15_RALINK__ //kloat add for gre header set and get
static u_int8_t get_gre_flags(unsigned char *buff)
{
	return buff[0];
}
static u_int8_t get_gre_ver(unsigned char *buff)
{
	return buff[1];
}
static u_int16_t get_gre_protocol(unsigned char *buff)
{
	return (buff[3]<<8)|buff[2];
}
static u_int16_t get_gre_payload_len(unsigned char *buff)
{
	return (buff[5]<<8)|buff[4];
}
static u_int16_t get_gre_call_id(unsigned char *buff)
{
	return (buff[7]<<8)|buff[6];
}
static u_int32_t get_gre_seq(unsigned char *buff)
{
	return (buff[11]<<24)|(buff[10]<<16)|(buff[9]<<8)|buff[8];
}
static u_int32_t get_gre_ack(unsigned char *buff)
{
	return (buff[15]<<24)|(buff[14]<<16)|(buff[13]<<8)|buff[12];
}

void set_gre_flags( u_int8_t flags )
{
	gre_buff[0] = flags;
}
void set_gre_ver( u_int8_t ver )
{
	gre_buff[1] = ver;
}
void set_gre_protocol( u_int16_t protocol )
{
	gre_buff[3] = (protocol>>8)&0xFF;
	gre_buff[2] = protocol&0xFF;
}
void set_gre_payload_len( u_int16_t payload_len )
{
	gre_buff[5] = (payload_len>>8)&0xFF;
	gre_buff[4] = payload_len&0xFF;
}
void set_gre_call_id( u_int16_t call_id )
{
	gre_buff[7] = (call_id>>8)&0xFF;
	gre_buff[6] = call_id&0xFF;
}
void set_gre_seq( u_int32_t seq )
{
	gre_buff[11] = (seq>>24)&0xFF;
	gre_buff[10] = (seq>>16)&0xFF;
	gre_buff[9] = (seq>>8)&0xFF;
	gre_buff[8] = seq&0xFF;
}
void set_gre_ack( u_int32_t ack )
{
	gre_buff[15] = (ack>>24)&0xFF;
	gre_buff[14] = (ack>>16)&0xFF;
	gre_buff[13] = (ack>>8)&0xFF;
	gre_buff[12] = ack&0xFF;
}
#endif //kloat end get and set

static uint64_t time_now_usecs()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

/* Open IP protocol socket */
int pptp_gre_bind(struct in_addr inetaddr)
{
	struct sockaddr_in src_addr, loc_addr;
	extern struct in_addr localbind;

	int s = socket(AF_INET, SOCK_RAW, PPTP_PROTO);
	if (s < 0)
	{
		d_warn("pptp: pptp_gre_bind(): socket: %s\n", strerror(errno));
		return -1;
	}

	if (localbind.s_addr != INADDR_NONE)
	{
		memset(&loc_addr, 0, sizeof (loc_addr));
		loc_addr.sin_family = AF_INET;
		loc_addr.sin_addr = localbind;
		if (bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) != 0)
		{
			d_warn("pptp: pptp_gre_bind(): bind: %s\n", strerror(errno));
			close(s);
			return -1;
		}
	}

	src_addr.sin_family = AF_INET;
	src_addr.sin_addr = inetaddr;
	src_addr.sin_port = 0;

	if (connect(s, (struct sockaddr *) &src_addr, sizeof (src_addr)) < 0)
	{
		d_warn("pptp: pptp_gre_bind(): connect: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	d_dbg("pptp: gre socket is %d\n", s);
	return s;
}

static void gre_send_ack(void * eloop_ctx, void * timeout_ctx)
{
	int gre_fd = (int)timeout_ctx;
	if (ack_sent != seq_recv)
	{
		encaps_gre(gre_fd, NULL, 0);
	}
}

#define HDLC_FLAG			0x7E
#define HDLC_ESCAPE			0x7D
#define HDLC_TRANSPARENCY	0x20

/* ONE blocking read per call; dispatches all packets possible */
/* returns 0 on success, or <0 on read failure                 */
static int decaps_hdlc(int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl)
{
	//unsigned char buffer[PACKET_MAX];
	unsigned char * buffer = hdlc_buff;
	unsigned int start = 0;
	int end;
	int status;

	//d_dbg("pptp: >>> decaps_hdlc(fd=%d)\n", fd);
	
	/* start is start of packet.  end is end of buffer data */
	/*  this is the only blocking read we will allow */

	if ((end = read(fd, buffer, PACKET_MAX)) <= 0)
	{
		d_warn("pptp: decaps_hdlc(): short read (%d): %s\n", end, strerror(errno));
		if (errno == EIO) d_warn("pptp: decaps_hdlc(): pppd may have shutdown, see pppd log\n");
		return -1;
	}

#if 0
	print_packet(2, buffer, end, "decaps_hdlc:");
#endif

	/* FIXME: replace by warnings when the test is shown reliable */
	if (!checkedsync)
	{
		checkedsync = 1;
		d_info("pptp: PPP mode seems to be %s.\n", buffer[0] == HDLC_FLAG ? "Asynchronous" : "Synchronous");
	}
	if (pptp_sync)
	{
#if 0
		while (start + 8 < end)
		{
			len = ntoh16(*(short int *)(buffer + start + 6)) + 4;
			/* note: the buffer may contain an incomplete packet at the end
			 * this packet will be read again at the next read() */
			d_dbg("start=%d, len=%d\n", start, len);
			if (start + len <= end)
			{
				if ((status = cb(cl, buffer + start, len)) < 0)
				{
					d_dbg("pptp: decaps_hdlc(): callback function return %d\n", status);
					return status;	/* error-check */
				}
			}
			start += len;
		}
#else
		if ((status = cb(cl, buffer, end)) < 0)
		{
			d_dbg("pptp: decaps_hdlc(): callback function return %d\n", status);
			return status; /* error-check */
		}
#endif
		return 0;
	}

	while (start < end)
	{	/* Copy to 'copy' and un-escape as we go. */
		while (buffer[start] != HDLC_FLAG)
		{
			if ((escape == 0) && buffer[start] == HDLC_ESCAPE)
			{
				escape = HDLC_TRANSPARENCY;
			}
			else
			{
				if (len < PACKET_MAX) copy[len++] = buffer[start] ^ escape;
				escape = 0;
			}
			start++;

			if (start >= end)
			{
				d_info("pptp: decaps_hdlc(): No more data, but the frame is not complete yet.\n");
				return 0;	/* No more data, but the frame is not complete yet. */
			}
		}

		/* found flag.  skip past it */
		start++;

		/* check for over-short packets and silently discard, as per RFC1662 */
		if ((len < 4) || (escape != 0))
		{
			len = 0;
			escape = 0;
			continue;
		}
		/* check, then remove the 16-bit FCS checksum field */
		if (pppfcs16(PPPINITFCS16, copy, len) != PPPGOODFCS16)
		{
			d_warn("pptp: decaps_hdlc(): Bad Frame Check Sequence during PPP to GRE decapsulation\n");
		}
		len -= sizeof (u_int16_t);

		/* so now we have a packet of length 'len' in 'copy' */
		//d_dbg("pptp: decaps_hdlc(): put %d bytes to encaps_gre()\n", len);
		if ((status = cb(cl, copy, len)) < 0)
		{
			d_dbg("pptp: decaps_hdlc(): callback function return %d\n", status);
			return status;	/* error-check */
		}

		/* Great!  Let's do more! */
		len = 0;
		escape = 0;
	}

	return 0;
	/* No more data to process. */
}

/* Make stripped packet into HDLC packet */
static int encaps_hdlc(int fd, void *pack, unsigned int len)
{
	unsigned char *source = (unsigned char *)pack;
	//unsigned char dest[2 * PACKET_MAX + 2];	/* largest expansion possible */
	unsigned char * dest = hdlc_buff;
	unsigned int pos = 0, i;
	u_int16_t fcs;

	//d_dbg("pptp: >>> encaps_hdlc(fd=%d, len=%d)\n", fd, len);

	if (pptp_sync) 
	{
		//+++ marco's workarround has fixed by siyou. 2011/1/6 10:44am
		//no need to sleep anymore.
		//the fix is in kernel/drivers/char/pty.c pty_open() with tty->low_latency=1.
			//marco, add 20 ms sleep here, or in some situation, two packets will combine as one packet
			//which will affect the state machine and we will not reply chap response		
			//usleep(20000);
		//--- 
		
		return write(fd, source, len);
	}

	/* Compute the FCS */
	fcs = pppfcs16(PPPINITFCS16, source, len) ^ 0xFFFF;

	/* start character */
	dest[pos++] = HDLC_FLAG;
	/* escape the payload */
	for (i = 0; i < len + 2; i++)
	{
		/* wacked out assignment to add FCS to end of source buffer */
		unsigned char c = (i < len) ? source[i] : (i==len) ? (fcs & 0xFF) : ((fcs >> 8) & 0xFF);
		if (pos >= (2*PACKET_MAX+2)) break;	/* truncate on overflow */
		if ((c < 0x20) || (c == HDLC_FLAG) || (c == HDLC_ESCAPE))
		{
			dest[pos++] = HDLC_ESCAPE;
			if (pos < (2*PACKET_MAX+2)) dest[pos++] = c ^ 0x20;
		}
		else
		{
			dest[pos++] = c;
		}
	}
	/* tack on the end-flag */
	if (pos < (2*PACKET_MAX+2)) dest[pos++] = HDLC_FLAG;

	/* now write this packet */
	//d_dbg("pptp: encaps_hdlc(): write(fd=%d, len=%d)\n", fd, pos);
#if 0
	print_packet(2, dest, pos, "encaps_hdlc:");
#endif

	return write(fd, dest, pos);
}

static int decaps_gre(int fd, callback_t callback, int cl)
{
	//unsigned char buffer[PACKET_MAX + 64 /*ip header */ ];
	unsigned char * buffer = gre_buff;
	struct pptp_gre_header *header;
	int status, ip_len = 0;
	unsigned int headersize;
	unsigned int payload_len;
	u_int32_t seq;
#ifdef __WRGN15_RALINK__
	unsigned char * header_buff;
#endif

	//d_dbg("pptp: >>> decaps_gre(fd=%d)\n", fd);
	
	if ((status = read(fd, buffer, PACKET_MAX+64)) <= 0)
	{
		d_warn("pptp: decaps_gre(): short read (%d): %s\n", status, strerror(errno));
		stats.rx_errors++;
		return -1;
	}

#if 0
	print_packet(2, buffer, status, "decaps_gre:");
#endif

	/* strip off IP header, if present */
	if ((buffer[0] & 0xF0) == 0x40) ip_len = (buffer[0] & 0xF) * 4;
	header = (struct pptp_gre_header *)(buffer + ip_len);
#ifdef __WRGN15_RALINK__
	header_buff = buffer + ip_len;
#endif

	/* verify packet (else discard) */
#ifdef __WRGN15_RALINK__
	if (((ntoh8(get_gre_ver(header_buff)) & 0x7F) != PPTP_GRE_VER) ||	/* version should be 1		*/
	    (ntoh16(get_gre_protocol(header_buff)) != PPTP_GRE_PROTO) ||		/* GRE protocol for PPTP	*/
	    PPTP_GRE_IS_C(ntoh8(get_gre_flags(header_buff))) ||				/* flag C should be clear	*/
	    PPTP_GRE_IS_R(ntoh8(get_gre_flags(header_buff))) ||				/* flag R should be clear	*/
	    (!PPTP_GRE_IS_K(ntoh8(get_gre_flags(header_buff)))) ||			/* flag K should be set		*/
	    ((ntoh8(get_gre_flags(header_buff)) & 0xF) != 0))				/* routing and recursion ctrl = 0 */
#else
	if (((ntoh8(header->ver) & 0x7F) != PPTP_GRE_VER) ||	/* version should be 1		*/
	    (ntoh16(header->protocol) != PPTP_GRE_PROTO) ||		/* GRE protocol for PPTP	*/
	    PPTP_GRE_IS_C(ntoh8(header->flags)) ||				/* flag C should be clear	*/
	    PPTP_GRE_IS_R(ntoh8(header->flags)) ||				/* flag R should be clear	*/
	    (!PPTP_GRE_IS_K(ntoh8(header->flags))) ||			/* flag K should be set		*/
	    ((ntoh8(header->flags) & 0xF) != 0))				/* routing and recursion ctrl = 0 */
#endif
	{
		/* if invalid, discard this packet */
		d_warn("pptp: decaps_gre(): Discarding GRE: %X %X %X %X %X %X\n",
		     ntoh8(header->ver) & 0x7F, ntoh16(header->protocol),
		     PPTP_GRE_IS_C(ntoh8(header->flags)),
		     PPTP_GRE_IS_R(ntoh8(header->flags)),
		     PPTP_GRE_IS_K(ntoh8(header->flags)),
		     ntoh8(header->flags) & 0xF);
		stats.rx_invalid++;
		return 0;
	}
#ifdef __WRGN15_RALINK__
	if (PPTP_GRE_IS_A(ntoh8(get_gre_ver(header_buff))))
	{	/* acknowledgement present */
		u_int32_t ack = (PPTP_GRE_IS_S(ntoh8(get_gre_flags(header_buff)))) ? get_gre_ack(header_buff) : get_gre_seq(header_buff);	/* ack in different place if S=0 */
#else
	if (PPTP_GRE_IS_A(ntoh8(header->ver)))
	{	/* acknowledgement present */
		u_int32_t ack = (PPTP_GRE_IS_S(ntoh8(header->flags))) ? header->ack : header->seq;
		/* ack in different place if S=0 */
#endif
		ack = ntoh32(ack);
		if (ack > ack_recv) ack_recv = ack;
		/* also handle sequence number wrap-around  */
		if (WRAPPED(ack, ack_recv)) ack_recv = ack;
		if (ack_recv == stats.pt.seq)
		{
			int rtt = time_now_usecs() - stats.pt.time;
			stats.rtt = (stats.rtt + rtt) / 2;
		}
	}
#ifdef __WRGN15_RALINK__
	if (!PPTP_GRE_IS_S(ntoh8(get_gre_flags(header_buff)))) return 0;	/* ack, but no payload */
#else
	if (!PPTP_GRE_IS_S(ntoh8(header->flags))) return 0;	/* ack, but no payload */
#endif

	headersize = sizeof (*header);
#ifdef __WRGN15_RALINK__
	payload_len = ntoh16(get_gre_payload_len(header_buff));
	seq = ntoh32(get_gre_seq(header_buff));

	if (!PPTP_GRE_IS_A(ntoh8(get_gre_ver(header_buff)))) headersize -= sizeof (get_gre_ack(header_buff));
#else
	payload_len = ntoh16(header->payload_len);
	seq = ntoh32(header->seq);

	if (!PPTP_GRE_IS_A(ntoh8(header->ver))) headersize -= sizeof (header->ack);
#endif

	/* check for incomplete packet (length smaller than expected) */
	if (status - headersize < payload_len)
	{
		d_warn("pptp: decaps_gre(): discarding truncated packet (expected %d, got %d bytes)\n", payload_len, status - headersize);
		stats.rx_truncated++;
		return 0;
	}

	/* check for out-of-order sequence number */
	/* (handle sequence number wrap-around, and try to do it right) */
	if (first || (seq == seq_recv + 1) || WRAPPED(seq, seq_recv))
	{
#if REORDER_LOGGING_VERBOSE
		d_dbg("pptp: decaps_gre(): accepting packet %d\n", seq);
#endif
		stats.rx_accepted++;
		first = 0;
		/* update recv seq, and register a 0.5 seconds timeout so we have chance to send ack. */
		seq_recv = seq;
		eloop_register_timeout(0, 500000, gre_send_ack, NULL, (void *)fd);
		return callback(cl, buffer + ip_len + headersize, payload_len);
	}
	else if (seq < seq_recv + 1 || WRAPPED(seq_recv, seq))
	{
		d_warn("pptp: decaps_gre(): discarding duplicate or old packet %d (expecting %d)\n", seq, seq_recv + 1);
		stats.rx_underwin++;
	}
	else if (seq < seq_recv + MISSING_WINDOW || WRAPPED(seq, seq_recv + MISSING_WINDOW))
	{
#if 1	/* some logging of reordening is required, we might miss some "side effects" */
		d_dbg("pptp: decaps_gre(): buffering out-of-order packet %d (expecting %d)\n", seq, seq_recv + 1);
#endif
		pqueue_add(seq, buffer + ip_len + headersize, payload_len);
		stats.rx_buffered++;
	}
	else
	{
		d_warn("pptp: decaps_gre(): discarding bogus packet %d (expecting %d)\n", seq, seq_recv + 1);
		stats.rx_overwin++;
	}

	return 0;
}

static int dequeue_gre(callback_t callback, int cl)
{
	pqueue_t *head;
	int status;

	//d_dbg("pptp: >>> dequeue_gre()\n");
	
	head = pqueue_head();

	while (head != NULL &&
			((head->seq == seq_recv + 1) || (WRAPPED(head->seq, seq_recv)) || (pqueue_expiry_time(head) <= 0)))
	{
		if (head->seq != seq_recv + 1 && !WRAPPED(head->seq, seq_recv))
		{
			stats.rx_lost += head->seq - seq_recv - 1;
#ifdef REORDER_LOGGING
			d_warn("pptp: dequeue_gre(): timeout waiting for %d packets\n", head->seq - seq_recv - 1);
#endif
		}
#ifdef REORDER_LOGGING
		d_info("pptp: dequeue_gre(): accepting %d from queue\n", head->seq);
#endif

		seq_recv = head->seq;
		status = callback(cl, head->packet, head->packlen);
		pqueue_del(head);
		if (status < 0) return status;
		eloop_register_timeout(0, 500000, gre_send_ack, NULL, (void *)gre_fd);
		head = pqueue_head();
	}

	return 0;
}

static int encaps_gre(int fd, void *pack, unsigned int len)
{
	struct pptp_gre_header * header;
	unsigned int header_len;
	int rc;

	//d_dbg("pptp: >>> encaps_gre(fd=%d, len=%d)\n", fd, len);
	
	header = (struct pptp_gre_header *)gre_buff;

	/* package this up in a GRE shell. */
#ifdef __WRGN15_RALINK__
	set_gre_flags(hton8(PPTP_GRE_FLAG_K));
	set_gre_ver(hton8(PPTP_GRE_VER));
	set_gre_protocol(hton16(PPTP_GRE_PROTO));
	set_gre_payload_len(hton16(len));
	set_gre_call_id(hton16(pptp_gre_peer_call_id));
#else
	header->flags = hton8(PPTP_GRE_FLAG_K);
	header->ver = hton8(PPTP_GRE_VER);
	header->protocol = hton16(PPTP_GRE_PROTO);
	header->payload_len = hton16(len);
	header->call_id = hton16(pptp_gre_peer_call_id);
#endif

	/* special case ACK with no payload */
	if (pack == NULL)
	{
		if (ack_sent != seq_recv)
		{
#ifdef __WRGN15_RALINK__
			set_gre_ver(get_gre_ver(gre_buff)|hton8(PPTP_GRE_FLAG_A));
			set_gre_payload_len(hton16(0));
			set_gre_seq(hton32(seq_recv));
#else
			header->ver |= hton8(PPTP_GRE_FLAG_A);
			header->payload_len = hton16(0);
			header->seq = hton32(seq_recv);	/* ack is in odd place because S=0 */
#endif
			ack_sent = seq_recv;
			eloop_cancel_timeout(gre_send_ack, NULL, (void *)fd);
			rc = write(fd, header, sizeof(*header) - sizeof (header->seq));
			if (rc < 0)
			{
				d_dbg("rc = %d, errno=%d, %s\n", rc, errno, strerror(errno));

				stats.tx_failed++;
			}
			else if (rc < sizeof(*header) - sizeof(header->seq))
			{
				stats.tx_short++;
			}
			else
			{
				stats.tx_acks++;
			}
			//d_dbg("pptp: encaps_gre: rc = %d !!\n", rc);
			return rc;
		}
		else
		{
			return 0;	/* we don't need to send ACK */
		}
	}
	/* explicit brace to avoid ambiguous `else' warning */
	/* send packet with payload */
#ifdef __WRGN15_RALINK__
	set_gre_flags(get_gre_flags(gre_buff)|hton8(PPTP_GRE_FLAG_S));
	set_gre_seq(hton32(seq));
#else
	header->flags |= hton8(PPTP_GRE_FLAG_S);
	header->seq = hton32(seq);
#endif
	if (ack_sent != seq_recv)
	{	/* send ack with this message */
#ifdef __WRGN15_RALINK__
		set_gre_ver(get_gre_ver(gre_buff)|hton8(PPTP_GRE_FLAG_A));
		set_gre_ack(hton32(seq_recv));
#else
		header->ver |= hton8(PPTP_GRE_FLAG_A);
		header->ack = hton32(seq_recv);
#endif
		ack_sent = seq_recv;
		header_len = sizeof(*header);
		eloop_cancel_timeout(gre_send_ack, NULL, (void *)fd);
	}
	else
	{	/* don't send ack */
		header_len = sizeof(*header) - sizeof(header->ack);
	}
	if ((header_len + len) >= (PACKET_MAX + sizeof(struct pptp_gre_header)))
	{
		stats.tx_oversize++;
		return 0;	/* drop this, it's too big */
	}
	/* copy payload into buffer */
	memcpy(gre_buff + header_len, pack, len);
	/* record and increment sequence numbers */
	seq_sent = seq;
	seq++;
	/* write this baby out to the net */
#if 0
	print_packet(2, gre_buff, header_len+len, "encaps_gre:");
#endif
	rc = write(fd, gre_buff, header_len + len);
	if (rc < 0)
	{
		d_dbg("rc = %d, errno=%d, %s\n", rc, errno, strerror(errno));
		stats.tx_failed++;
	}
	else if (rc < header_len + len)
	{
		stats.tx_short++;
	}
	else
	{
		stats.tx_sent++;
		stats.pt.seq = seq_sent;
		stats.pt.time = time_now_usecs();
	}
	//d_dbg("pptp: encaps_gre: rc = %d!\n", rc);
	return rc;
}


static void call_callback(PPTP_CONN * conn, PPTP_CALL * call, enum call_state state)
{
	switch (state)
	{
	case CALL_OPEN_RQST:
		d_dbg("pptp: call_callback(): CALL_OPEN_RQST! (We should not see this packet!)\n");
		break;
	case CALL_OPEN_DONE:
		d_dbg("pptp: call_callback(): CALL_OPEN_DONE!\n");
		ack_sent = ack_recv = seq_sent = seq_recv = 0;
		pptp_call_get_ids(conn, call, &pptp_gre_call_id, &pptp_gre_peer_call_id);
		eloop_terminate();
		pptp_ready = 1;
		break;
	case CALL_OPEN_FAIL:
		d_dbg("pptp: call_callback(): CALL_OPEN_FAIL!\n");
		pptp_ready = 0;
		pptp_conn_close(conn, PPTP_STOP_NONE);
		break;
	case CALL_CLOSE_RQST:
		d_dbg("pptp: call_callback(): CALL_CLOSE_RQST!\n");
		break;
	case CALL_CLOSE_DONE:
		d_dbg("pptp: call_callback(): CALL_CLOSE_DONE!\n");
		pptp_ready = 0;
		if (!pptp_destroy_conn) pptp_conn_close(conn, PPTP_STOP_NONE);
		break;
	}
}

static void conn_callback(PPTP_CONN * conn, enum conn_state state)
{
	switch (state)
	{
	case CONN_OPEN_RQST:
		d_dbg("pptp: conn_callback(): CONN_OPEN_RQST!\n");
		break;
	case CONN_OPEN_DONE:
		d_dbg("pptp: conn_callback(): CONN_OPEN_DONE!\n");
		pptp_call_open(conn, call_callback, NULL);
		break;
	case CONN_OPEN_FAIL:
		d_dbg("pptp: conn_callback(): CONN_OPEN_FAIL!\n");
		eloop_terminate();
		break;
	case CONN_CLOSE_RQST:
		d_dbg("pptp: conn_callback(): CONN_CLOSE_RQST!\n");
		pptp_ready = 0;
		break;
	case CONN_CLOSE_DONE:
		d_dbg("pptp: conn_callback(): CONN_CLOSE_DONE!\n");
		modem_hungup();
		eloop_terminate();
		g_conn = NULL;
		if (pptp_fd > 0) close(pptp_fd);
		if (gre_fd > 0) close(gre_fd);
		pptp_fd = gre_fd = -1;
		pptp_ready = 0;
		pptp_exit = 1;
		break;
	}
}

//static void close_inetsock(int fd, struct in_addr inetaddr)
//{
//	close(fd);
//}

static int open_inetsock(struct in_addr inetaddr)
{
	struct sockaddr_in dest;
	int s;

	d_dbg("pptp: >>> openSocket()\n");
	
	dest.sin_family = AF_INET;
	dest.sin_port = htons(PPTP_PORT);
	dest.sin_addr = inetaddr;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		d_warn("pptp: open_inetsocket(): socket: %s\n", strerror(errno));
		return s;
	}

	if (connect(s, (struct sockaddr *)&dest, sizeof(dest)) < 0)
	{
		d_warn("pptp: open_inetsocket(): connect: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	d_dbg("pptp: openSocket(%d)\n", s);
	return s;
}


//static int master_pty_fd = -1;

static struct in_addr get_ip_address(char * name)
{
	struct in_addr retval;
	struct hostent * host = gethostbyname(name);
	retval.s_addr = -1;
	if (host == NULL)
	{
		if (h_errno == HOST_NOT_FOUND) d_error("pptp: gethostbyname '%s' : HOST NOT FOUND\n", name);
		else if (h_errno == NO_ADDRESS) d_error("pptp: gethostbyname '%s' : NO IP ADDRESS\n", name);
		else d_error("pptp: gethostbyname '%s' : name server error\n", name);
		return retval;
	}
	if (host->h_addrtype != AF_INET)
	{
		d_error("pptp: Host '%s' has non-internet address\n", name);
		return retval;
	}
	memcpy(&retval.s_addr, host->h_addr, sizeof(retval.s_addr));
	return retval;
}

static void gre_handler(int sock, void * eloop_ctx, void * sock_ctx)
{
	int pty_fd = (int)sock_ctx;

	//d_dbg("pptp: >>> gre_handler()\n");
	if (decaps_gre(sock, encaps_hdlc, pty_fd) < 0) goto close_conn;
	if (dequeue_gre(encaps_hdlc, pty_fd) < 0) goto close_conn;
	return;

close_conn:
	d_error("pptp: gre_handler(): something wrong, closing pptp connection now !!!!\n");
	pptp_conn_close((PPTP_CONN *)eloop_ctx, PPTP_STOP_NONE);
	return;
}

static void pty_handler(int sock, void * eloop_ctx, void * sock_ctx)
{
	int gre_fd = (int)sock_ctx;
	//d_dbg("pptp: >>> pty_handler()\n");
	if (decaps_hdlc(sock, encaps_gre, gre_fd) < 0)
	{
		d_info("pptp: decaps_hdlc() fail, close connection!\n");
		pptp_conn_close((PPTP_CONN *)eloop_ctx, PPTP_STOP_NONE);
	}
}


int pptp_module_connect(int pty_fd)
{
	int disc = N_HDLC;
	long flags;
	struct in_addr inetaddr;
	PPTP_CONN * conn = NULL;

	d_dbg("pptp: >>> pptp_module_connect()\n");

	/* init static variables */
	init_vars();

	/* setup parameters */
	if (strlen(pptp_localbind)) 
	{
		if (inet_pton(AF_INET, pptp_localbind, (void *)&localbind) < 1)
		{
			localbind.s_addr = INADDR_NONE;
		}
	}

#if 1
	if (pptp_sync)
	{
		if (ioctl(pty_fd, TIOCSETD, &disc) < 0)
		{
			d_error("pptp: unable to set link discipline to N_HDLC.\n"
					"      Make sure your kernel supports the N_HDLC line discipline,\n"
					"      or do not use the SYNCHOUNOUS option. Quitting.\n");
			return -1;
		}
		else
		{
			d_info("pptp: Changed pty line discipline to N_HDLC for synchrounous mode.\n");
		}
		/* There is a bug in Linux's select which returns a descriptor
		 * as readable if N_HDLC line discipline is on, even if
		 * it isn't really readable. This return happens only when
		 * select() times out. To avoid blocking forever in read(),
		 * make descriptor non-blocking. */
		flags = fcntl(pty_fd, F_GETFL);
		if (flags < 0)
		{
			d_error("pptp: error fcntl(F_GETFL)\n");
			return -1;
		}
		if (fcntl(pty_fd, F_SETFL, (long)flags | O_NONBLOCK) < 0)
		{
			d_error("pptp: error fcntl(F_SETFL)\n");
			return -1;
		}
	}
#endif
	
	/* Get server IP address */
	inetaddr = get_ip_address(pptp_server_ip);

	/* record the server ip in 'exclude_peer' (IPCP).
	 * This IP can not be used as peer IP.
	 * by David Hsieh <david_hsieh@alphanetworks.com> */
	exclude_peer = inetaddr.s_addr;
	d_dbg("%s: exclude_peer = %08x\n",__FUNCTION__, exclude_peer);

	/* Now we have the peer address, bind the GRE socket */
	gre_fd = pptp_gre_bind(inetaddr);
	if (gre_fd < 0)
	{
		d_error("pptp: Cannot bind GRE socket, aborting!\n");
		return -1;
	}

	if ((pptp_fd = open_inetsock(inetaddr)) < 0) return -1;
	if ((conn = pptp_conn_open(pptp_fd, 1, conn_callback)) == NULL)
	{
		d_error("pptp: pptp_conn_open failed!\n");
		close(pptp_fd);
		return -1;
	}

	g_conn = conn;

	pptp_ready = pptp_exit = 0;
	eloop_register_read_sock(pptp_fd, pptp_handler, NULL, conn);
	eloop_run();
	if (pptp_ready)
	{
		d_dbg("pptp: pptp connected!!\n");
		eloop_register_read_sock(gre_fd, gre_handler, conn, (void *)pty_fd);
		eloop_register_read_sock(pty_fd, pty_handler, conn, (void *)gre_fd);
		eloop_continue();
		return 0;
	}
	else
	{
		return -1;
	}
}

void pptp_module_disconnect(void)
{
	d_dbg("pptp: >>> pptp_module_disconnect()\n");
	hungup = 1;
	if (g_conn) pptp_conn_destroy(g_conn);

	exclude_peer = 0;
	d_dbg("%s: clear exlucde_peer!\n",__FUNCTION__);

	dassert(g_conn == NULL);
	dassert(pptp_fd < 0);
	dassert(gre_fd < 0);
}
