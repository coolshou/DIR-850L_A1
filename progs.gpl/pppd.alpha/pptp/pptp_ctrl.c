/* vi: set sw=4 ts=4: */
/* pptp_ctrl.c ... handle PPTP control connection.
 *                 C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_ctrl.c,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include "pptp_msg.h"
#include "pptp_ctrl.h"
#include "pptp_options.h"
#include "eloop.h"
#include "dtrace.h"

/* PPTP error codes: ----------------------------------------------*/

/* (General Error Codes) */
static const struct
{
	const char *name, *desc;
}
pptp_general_errors[] =
{
#define PPTP_GENERAL_ERROR_NONE                 0
	{ "(None)",			"No general error" },
#define PPTP_GENERAL_ERROR_NOT_CONNECTED        1
	{ "(Not-Connected)","No control connection exists yet for this PAC-PNS pair"},
#define PPTP_GENERAL_ERROR_BAD_FORMAT           2
	{ "(Bad-Format)",	"Length is wrong or Magic Cookie value is incorrect"},
#define PPTP_GENERAL_ERROR_BAD_VALUE            3
	{ "(Bad-Value)",	"One of the field values was out of range or reserved field was non-zero"},
#define PPTP_GENERAL_ERROR_NO_RESOURCE          4
	{ "(No-Resource)",	"Insufficient resources to handle this command now"},
#define PPTP_GENERAL_ERROR_BAD_CALLID           5
	{ "(Bad-Call ID)",	"The Call ID is invalid in this context"},
#define PPTP_GENERAL_ERROR_PAC_ERROR            6
	{ "(PAC-Error)",	"A generic vendor-specific error occured in the PAC"}
};

/*
 * BECAUSE OF SIGNAL LIMITATIONS, EACH PROCESS CAN ONLY MANAGE ONE
 * CONNECTION.  SO THIS 'PPTP_CONN' STRUCTURE IS A BIT MISLEADING.
 * WE'LL KEEP CONNECTION-SPECIFIC INFORMATION IN THERE ANYWAY (AS
 * OPPOSED TO USING GLOBAL VARIABLES), BUT BEWARE THAT THE ENTIRE
 * UNIX SIGNAL-HANDLING SEMANTICS WOULD HAVE TO CHANGE (OR THE
 * TIME-OUT CODE DRASTICALLY REWRITTEN) BEFORE YOU COULD DO A 
 * PPTP_CONN_OPEN MORE THAN ONCE PER PROCESS AND GET AWAY WITH IT.
 */

/* This structure contains connection-specific information that the
 * signal handler needs to see.  Thus, it needs to be in a global
 * variable.  If you end up using pthreads or something (why not
 * just processes?), this would have to be placed in a thread-specific
 * data area, using pthread_get|set_specific, etc., so I've 
 * conveniently encapsulated it for you.
 * [linux threads will have to support thread-specific signals
 *  before this would work at all, which, as of this writing
 *  (linux-threads v0.6, linux kernel 2.1.72), it does not.]
 */

#define INITIAL_BUFSIZE 512	/* initial i/o buffer size. */

struct PPTP_CALL
{
	/* Call properties */
	enum { PPTP_CALL_PAC, PPTP_CALL_PNS } call_type;
	union
	{
		enum pptp_pac_state { PAC_IDLE, PAC_WAIT_REPLY, PAC_ESTABLISHED, PAC_WAIT_CS_ANS } pac;
		enum pptp_pns_state { PNS_IDLE, PNS_WAIT_REPLY, PNS_ESTABLISHED, PNS_WAIT_DISCONNECT } pns;
	} state;
	u_int16_t call_id, peer_call_id;
	u_int16_t sernum;
	u_int32_t speed;

	/* For user data: */
	pptp_call_cb callback;
	void *closure;
};

struct PPTP_CONN
{
	int inet_sock;

	/* Connection States: on startup: CONN_IDLE */
	enum { CONN_IDLE, CONN_WAIT_CTL_REPLY, CONN_WAIT_STOP_REPLY, CONN_ESTABLISHED } conn_state;

	/* Keep-alive states: on startup: KA_NONE */
	enum { KA_NONE, KA_OUTSTANDING } ka_state;

	/* Keep-alive ID; monotonically increasing (watch wrap-around!): on startup: 1 */
	u_int32_t ka_id;

	/* Other properties. */
	u_int16_t version;
	u_int16_t firmware_rev;
	u_int8_t hostname[64], vendor[64];
	/* XXX these are only PNS properties, currently XXX */

	/* Call assignment information. */
	u_int16_t call_serial_number;

	struct PPTP_CALL * call;

	void *closure;
	pptp_conn_cb callback;

	/******* IO buffers ******/
	char *read_buffer, *write_buffer;
	size_t read_alloc, write_alloc;
	size_t read_size, write_size;
};

#ifdef DDEBUG
/* Outgoing Call Reply Result Codes */
static const char *pptp_out_call_reply_result[] =
{
/* 0 */ "Unknown Result Code",
/* 1 */ "Connected",
/* 2 */ "General Error",
/* 3 */ "No Carrier Detected",
/* 4 */ "Busy Signal",
/* 5 */ "No Dial Tone",
/* 6 */ "Time Out",
/* 7 */ "Not Accepted, Call is administratively prohibited"
};
#endif

#define MAX_OUT_CALL_REPLY_RESULT 7

#ifdef DDEBUG
/* Call Disconnect Notify  Result Codes */
static const char *pptp_call_disc_ntfy[] =
{
/* 0 */ "Unknown Result Code",
/* 1 */ "Lost Carrier",
/* 2 */ "General Error",
/* 3 */ "Administrative Shutdown",
/* 4 */ "(your) Request"
};
#endif

#define MAX_CALL_DISC_NTFY 4

/* Local prototypes */
static void pptp_reset_timer(PPTP_CONN * conn);
static ssize_t pptp_read_some(PPTP_CONN * conn);
static int pptp_make_packet(PPTP_CONN * conn, void **buf, size_t * size);
static int pptp_send_ctrl_packet(PPTP_CONN * conn, void *buffer, size_t size);
static void pptp_dispatch_packet(PPTP_CONN * conn, void *buffer, size_t size);
static void pptp_dispatch_ctrl_packet(PPTP_CONN * conn, void *buffer, size_t size);
static void pptp_handle_timer(void * eloop_ctx, void * timeout_ctx);
//static void pptp_set_link(PPTP_CONN * conn, int peer_call_id);

/* non-zero if called from pptp_conn_destroy().
 * This flag is set at the beginning of pptp_conn_destroy(), and clear at the end.
 * if we are in pptp_conn_destroy(), don't send anything via control socket. */
int pptp_destroy_conn = 0;

/*----------------------------------------------------------------------*/
/* Constructors and Destructors.                                        */

/* Open new pptp_connection.  Returns NULL on failure. */
PPTP_CONN * pptp_conn_open(int inet_sock, int isclient, pptp_conn_cb callback)
{
	/* struct sigaction	sigact; */
	PPTP_CONN *	conn;

	d_dbg("pptp: >>> pptp_conn_open()\n");
	
	/* Allocate structure */
	if ((conn = malloc(sizeof (*conn))) == NULL) return NULL;
	/* Initialize */
	conn->call = NULL;
	conn->inet_sock = inet_sock;
	conn->conn_state = CONN_IDLE;
	conn->ka_state = KA_NONE;
	conn->ka_id = 1;
	conn->call_serial_number = 0;
	conn->callback = callback;
	/* Create I/O buffers */
	conn->read_size = conn->write_size = 0;
	conn->read_alloc = conn->write_alloc = INITIAL_BUFSIZE;
	conn->read_buffer = malloc(sizeof(*(conn->read_buffer))*conn->read_alloc);
	conn->write_buffer = malloc(sizeof(*(conn->write_buffer))*conn->write_alloc);
	if (conn->read_buffer == NULL || conn->write_buffer == NULL)
	{
		if (conn->read_buffer != NULL) free(conn->read_buffer);
		if (conn->write_buffer != NULL) free(conn->write_buffer);
		free(conn);
		return NULL;
	}

	/* Make this socket non-blocking. */
	fcntl(conn->inet_sock, F_SETFL, O_NONBLOCK);

	/* Request connection from server, if this is a client */
	if (isclient)
	{
		struct pptp_start_ctrl_conn packet =
		{
			PPTP_HEADER_CTRL(PPTP_START_CTRL_CONN_RQST),
			hton16(PPTP_VERSION), 0, 0,
			hton32(PPTP_FRAME_CAP), hton32(PPTP_BEARER_CAP),
			hton16(PPTP_MAX_CHANNELS),
			hton16(PPTP_FIRMWARE_VERSION),
			PPTP_HOSTNAME, PPTP_VENDOR
		};

		if (pptp_send_ctrl_packet(conn, &packet, sizeof (packet)))
			conn->conn_state = CONN_WAIT_CTL_REPLY;
		else
			return NULL;	/* could not send initial start request. */
	}

	/* Reset event timer */
	pptp_reset_timer(conn);
	return conn;
}

/* This currently *only* works for client call requests.
 * We need to do something else to allocate calls for incoming requests. */
PPTP_CALL * pptp_call_open(PPTP_CONN * conn, pptp_call_cb callback, char *phonenr)
{
	PPTP_CALL *	call;
	int			i;


	d_dbg("pptp: >>> pptp_call_open()\n");
	
	/* Assign call id */
	srand(time(NULL));
	i=rand()%65536;

	/* allocate structure. */
	if ((call = malloc(sizeof (*call))) == NULL) return NULL;
	/* Initialize call structure */
	call->call_type = PPTP_CALL_PNS;
	call->state.pns = PNS_IDLE;
	call->call_id = (u_int16_t)i;
	call->sernum = conn->call_serial_number++;
	call->callback = callback;
	call->closure = NULL;
	/* Send off the call request */
	{
		struct pptp_out_call_rqst packet =
		{
			PPTP_HEADER_CTRL(PPTP_OUT_CALL_RQST),
			hton16(call->call_id), hton16(call->sernum),
			hton32(PPTP_BPS_MIN), hton32(PPTP_BPS_MAX),
			hton32(PPTP_BEARER_CAP), hton32(PPTP_FRAME_CAP),
			hton16(PPTP_WINDOW), 0, 0, 0, {0}, {0}
		};

		/* fill in the phone number if it was specified */
		if (phonenr)
		{
			strncpy((char *)packet.phone_num, phonenr, sizeof(packet.phone_num));
			packet.phone_len = strlen(phonenr);
			if (packet.phone_len > sizeof (packet.phone_num))
				packet.phone_len = sizeof (packet.phone_num);
			packet.phone_len = hton16(packet.phone_len);
		}

		if (pptp_send_ctrl_packet(conn, &packet, sizeof (packet)))
		{
			pptp_reset_timer(conn);
			call->state.pns = PNS_WAIT_REPLY;
			conn->call = call;
			return call;
		}
		else
		{	/* oops, unsuccessful. Deallocate. */
			free(call);
			return NULL;
		}
	}
}

void pptp_call_close(PPTP_CONN * conn, PPTP_CALL * call)
{
	struct pptp_call_clear_rqst rqst =
	{
		PPTP_HEADER_CTRL(PPTP_CALL_CLEAR_RQST), 0, 0
	};

	dassert(conn && conn->call);
	dassert(call);
	dassert(call->call_type == PPTP_CALL_PNS);	/* haven't thought about PAC yet */
	dassert(call->state.pns != PNS_IDLE);
	rqst.call_id = hton16(call->call_id);

	/* don't check state against WAIT_DISCONNECT... allow multiple disconnect
	 * requests to be made. */
	pptp_send_ctrl_packet(conn, &rqst, sizeof (rqst));
	pptp_reset_timer(conn);
	call->state.pns = PNS_WAIT_DISCONNECT;

	/* call structure will be freed when we have confirmation of disconnect. */
}

/* hard close. */
void pptp_call_destroy(PPTP_CONN * conn, PPTP_CALL * call)
{
	dassert(conn && conn->call);
	dassert(call);

	/* notify */
	if (call->callback != NULL) call->callback(conn, call, CALL_CLOSE_DONE);
	/* deallocate */
	conn->call=NULL;
	free(call);
}

/* this is a soft close */
void pptp_conn_close(PPTP_CONN * conn, u_int8_t close_reason)
{
	struct pptp_stop_ctrl_conn rqst =
	{
		PPTP_HEADER_CTRL(PPTP_STOP_CTRL_CONN_RQST),
		hton8(close_reason), 0, 0
	};

	dassert(conn && conn->call);
	/* avoid repeated close attempts */
	if (conn->conn_state == CONN_IDLE || conn->conn_state == CONN_WAIT_STOP_REPLY)
		return;

	/* close open calls, if any */
	pptp_call_close(conn, conn->call);

	/* now close connection */
	d_info("pptp: Closing PPTP connection %d\n", close_reason);
	pptp_send_ctrl_packet(conn, &rqst, sizeof (rqst));
	pptp_reset_timer(conn);	/* wait 60 seconds for reply */
	conn->conn_state = CONN_WAIT_STOP_REPLY;

	return;
}

/* this is a hard close */
void pptp_conn_destroy(PPTP_CONN * conn)
{
	dassert(conn != NULL);
	pptp_destroy_conn = 1;

	/* destroy all open calls */
	if (conn->call) pptp_call_destroy(conn, conn->call);
	/* notify */
	if (conn->callback != NULL) conn->callback(conn, CONN_CLOSE_DONE);
	/* deallocate */
	free(conn);
	pptp_destroy_conn = 0;
}

/************** Deal with messages, in a non-blocking manner *************/

/* handle any pptp file descriptors set in fd_set, and clear them */
void pptp_handler(int sock, void * eloop_ctx, void * sock_ctx)
{
	PPTP_CONN * conn = (PPTP_CONN *)sock_ctx;
	ssize_t read;
	void *buffer;
	size_t size;

	d_dbg("pptp: >>> pptp_handler()\n");
	read = pptp_read_some(conn);	/* read as much as we can without blocking */
	/* if read is zero, EOF, socket was closed by peer. */
	d_dbg("pptp: pptp_read_some return %d\n", read);
	if (read <= 0)
	{
		pptp_conn_destroy(conn);
		return;
	}

	/* make packets of the buffer, while we can. */
	while (pptp_make_packet(conn, &buffer, &size))
	{
		pptp_dispatch_packet(conn, buffer, size);
		free(buffer);
	}

	/* That's all, folks.  Simple, eh? */
}

/* Read as much as we can without blocking. */
static ssize_t pptp_read_some(PPTP_CONN * conn)
{
	ssize_t retval;

	if (conn->read_size == conn->read_alloc)
	{	/* need to alloc more memory */
		char *new_buffer = realloc(conn->read_buffer, sizeof(*(conn->read_buffer))*conn->read_alloc*2);
		if (new_buffer == NULL)
		{
			d_error("pptp: pptp_read_some(): Out of memory\n");
			return -1;
		}
		conn->read_alloc *= 2;
		conn->read_buffer = new_buffer;
	}
	retval = read(conn->inet_sock, conn->read_buffer + conn->read_size, conn->read_alloc - conn->read_size);
	if (retval < 0)
	{
		if (errno == EINTR || errno == EAGAIN)
		{
			/* ignore */ ;
		}
		else
		{	/* a real error */
			d_error("pptp: pptp_read_some(): read error: %s\n", strerror(errno));
			//pptp_conn_destroy(conn);	/* shut down fast. */
		}
		return retval;
	}
	conn->read_size += retval;
	assert(conn->read_size <= conn->read_alloc);
	return retval;
}

/********************* Packet formation *******************************/
/* Make valid packets from read_buffer */
static int pptp_make_packet(PPTP_CONN * conn, void **buf, size_t * size)
{
	struct pptp_header *header;
	size_t bad_bytes = 0;
	assert(buf != NULL);
	assert(size != NULL);

	/* Give up unless there are at least sizeof(pptp_header) bytes */
	while ((conn->read_size - bad_bytes) >= sizeof (struct pptp_header))
	{
		/* Throw out bytes until we have a valid header. */
		header = (struct pptp_header *)(conn->read_buffer + bad_bytes);
		if (ntoh32(header->magic) != PPTP_MAGIC)
			goto throwitout;
		if (ntoh16(header->reserved0) != 0)
			d_warn("pptp: reserved0 field is not zero! (0x%x) Cisco feature?\n", ntoh16(header->reserved0));
		if (ntoh16(header->length) < sizeof (struct pptp_header))
			goto throwitout;
		if (ntoh16(header->length) > PPTP_CTRL_SIZE_MAX)
			goto throwitout;
		/* well.  I guess it's good. Let's see if we've got it all. */
		if (ntoh16(header->length) > (conn->read_size - bad_bytes))
			/* nope.  Let's wait until we've got it, then. */
			goto flushbadbytes;
		/* One last check: */
		if ((ntoh16(header->pptp_type) == PPTP_MESSAGE_CONTROL) &&
		    (ntoh16(header->length) != PPTP_CTRL_SIZE(ntoh16(header->ctrl_type))))
			goto throwitout;

		/* well, I guess we've got it. */
		*size = ntoh16(header->length);
		*buf = malloc(*size);
		if (*buf == NULL)
		{
			d_warn("pptp: Out of memory.\n");
			return 0;	/* ack! */
		}
		memcpy(*buf, conn->read_buffer + bad_bytes, *size);
		/* Delete this packet from the read_buffer. */
		conn->read_size -= (bad_bytes + *size);
		memmove(conn->read_buffer, conn->read_buffer + bad_bytes + *size, conn->read_size);
		if (bad_bytes > 0)
		{
			d_info("%lu bad bytes thrown away.", (unsigned long) bad_bytes);
		}
		return 1;

throwitout:
		bad_bytes++;
	}

flushbadbytes:
	/* no more packets.  Let's get rid of those bad bytes */
	conn->read_size -= bad_bytes;
	memmove(conn->read_buffer, conn->read_buffer + bad_bytes, conn->read_size);
	if (bad_bytes > 0)
	{
		d_info("pptp: %lu bad bytes thrown away.\n", (unsigned long) bad_bytes);
	}
	return 0;
}

/* Add packet to write_buffer */
static int pptp_send_ctrl_packet(PPTP_CONN * conn, void *buffer, size_t size)
{
	ssize_t retval;

	retval = write(conn->inet_sock, buffer, size);
	if (retval < 0)
	{
		if (errno == EAGAIN || errno == EINTR)
		{
			/* ignore */
		}
		else
		{
			/* a real error */
			d_error("pptp: pptp_send_ctrl_packet(): write error: %s\n", strerror(errno));
			pptp_conn_destroy(conn);
		}
		return 0;
	}
	assert(retval == (ssize_t)size);
	return 1;
}

/********************** Packet Dispatch ****************************/
/* Dispatch packets (general) */
static void pptp_dispatch_packet(PPTP_CONN * conn, void *buffer, size_t size)
{
	struct pptp_header *header = (struct pptp_header *) buffer;
	dassert(buffer);
	dassert(ntoh32(header->magic) == PPTP_MAGIC);
	dassert(ntoh16(header->length) == size);

	switch (ntoh16(header->pptp_type))
	{
	case PPTP_MESSAGE_CONTROL:
		pptp_dispatch_ctrl_packet(conn, buffer, size);
		break;
	case PPTP_MESSAGE_MANAGE:
		/* MANAGEMENT messages aren't even part of the spec right now. */
		d_info("pptp: PPTP management message received, but not understood.\n");
		break;
	default:
		d_info("pptp: Unknown PPTP control message type received: %u\n", (unsigned int) ntoh16(header->pptp_type));
		break;
	}
}

/* Dispatch packets (control messages) */
static void pptp_dispatch_ctrl_packet(PPTP_CONN * conn, void *buffer, size_t size)
{
	struct pptp_header *	header = (struct pptp_header *) buffer;
	u_int8_t				close_reason = PPTP_STOP_NONE;

	dassert(buffer);
	dassert(ntoh32(header->magic) == PPTP_MAGIC);
	dassert(ntoh16(header->length) == size);
	dassert(ntoh16(header->pptp_type) == PPTP_MESSAGE_CONTROL);

	if (size < PPTP_CTRL_SIZE(ntoh16(header->ctrl_type)))
	{
		d_warn("pptp: Invalid packet received [type: %d; length: %d].\n", (int)ntoh16(header->ctrl_type), (int)size);
		return;
	}
	
	switch (ntoh16(header->ctrl_type))
	{
	/* ----------- STANDARD Start-Session MESSAGES ------------ */
	case PPTP_START_CTRL_CONN_RQST:
		d_dbg("pptp: rx PPTP_START_CTRL_CONN_RQST\n");
		{
			struct pptp_start_ctrl_conn *packet = (struct pptp_start_ctrl_conn *) buffer;
			struct pptp_start_ctrl_conn reply =
			{
				PPTP_HEADER_CTRL(PPTP_START_CTRL_CONN_RPLY),
				hton16(PPTP_VERSION), 0, 0,
				hton32(PPTP_FRAME_CAP), hton32(PPTP_BEARER_CAP),
				hton16(PPTP_MAX_CHANNELS),
				hton16(PPTP_FIRMWARE_VERSION),
				PPTP_HOSTNAME, PPTP_VENDOR
			};

			if (conn->conn_state == CONN_IDLE)
			{
				if (ntoh16(packet->version) < PPTP_VERSION)
				{	/* Can't support this (earlier) PPTP_VERSION */
					reply.version = packet->version;
					reply.result_code = hton8(5);	/* protocol version not supported */
					pptp_send_ctrl_packet(conn, &reply, sizeof(reply));
					pptp_reset_timer(conn);	/* give sender a chance for a retry */
				}
				else
				{	/* same or greater version */
					if (pptp_send_ctrl_packet(conn, &reply, sizeof(reply)))
					{
						conn->conn_state = CONN_ESTABLISHED;
						d_info("pptp: server connection ESTABLISHED.\n");
						pptp_reset_timer(conn);
					}
				}
			}
			break;
		}
	case PPTP_START_CTRL_CONN_RPLY:
		d_dbg("pptp: rx PPTP_START_CTRL_CONN_RPLY\n");
		{
			struct pptp_start_ctrl_conn *packet = (struct pptp_start_ctrl_conn *) buffer;
			if (conn->conn_state == CONN_WAIT_CTL_REPLY)
			{	/* XXX handle collision XXX [see rfc] */
				if (ntoh16(packet->version) != PPTP_VERSION)
				{
					if (conn->callback != NULL) conn->callback(conn, CONN_OPEN_FAIL);
					close_reason = PPTP_STOP_PROTOCOL;
					goto pptp_conn_close;
				}
				/* J'ai change le if () afin que la connection ne se ferme pas pour un "rien" :p
				 * adel@cybercable.fr - */
				if (ntoh8(packet->result_code) != 1 && ntoh8(packet->result_code) != 0)
				{
					if (conn->callback != NULL) conn->callback(conn, CONN_OPEN_FAIL);
					close_reason = PPTP_STOP_PROTOCOL;
					goto pptp_conn_close;
				}
				conn->conn_state = CONN_ESTABLISHED;

				/* log session properties */
				conn->version = ntoh16(packet->version);
				conn->firmware_rev = ntoh16(packet->firmware_rev);
				memcpy(conn->hostname, packet->hostname, sizeof(conn->hostname));
				memcpy(conn->vendor, packet->vendor, sizeof(conn->vendor));

				pptp_reset_timer(conn);	/* 60 seconds until keep-alive */
				d_info("pptp: Client connection established.\n");

				if (conn->callback != NULL) conn->callback(conn, CONN_OPEN_DONE);
			}	/* else goto pptp_conn_close; */
			break;
		}

	/* ----------- STANDARD Stop-Session MESSAGES ------------ */
	case PPTP_STOP_CTRL_CONN_RQST:
		d_dbg("pptp: rx PPTP_STOP_CTRL_CONN_RQST\n");
		{
#if 0
			/* make gcc happy about "unused variable 'packet'" here */
			struct pptp_stop_ctrl_conn *packet =	/* XXX do something with this XXX */
				(struct pptp_stop_ctrl_conn *) buffer;
#endif
			/* conn_state should be CONN_ESTABLISHED, but it could be something else */
			struct pptp_stop_ctrl_conn reply =
			{
				PPTP_HEADER_CTRL(PPTP_STOP_CTRL_CONN_RPLY),
				hton8(1), hton8(PPTP_GENERAL_ERROR_NONE), 0
			};

			if (conn->conn_state == CONN_IDLE) break;
			if (pptp_send_ctrl_packet(conn, &reply, sizeof (reply)))
			{
				if (conn->callback != NULL) conn->callback(conn, CONN_CLOSE_RQST);
				conn->conn_state = CONN_IDLE;
				pptp_conn_destroy(conn);
			}
			break;
		}
	case PPTP_STOP_CTRL_CONN_RPLY:
		d_dbg("pptp: rx PPTP_STOP_CTRL_CONN_RPLY\n");
		{
#if 0
			/* make gcc happy about "unused variable 'packet'" here */
			struct pptp_stop_ctrl_conn *packet =	/* XXX do something with this XXX */
			    (struct pptp_stop_ctrl_conn *) buffer;
#endif
			/* conn_state should be CONN_WAIT_STOP_REPLY, but it could be something else */
			if (conn->conn_state == CONN_IDLE) break;
			conn->conn_state = CONN_IDLE;
			pptp_conn_destroy(conn);
			break;
		}
	/* ----------- STANDARD Echo/Keepalive MESSAGES ------------ */
	case PPTP_ECHO_RPLY:
		d_dbg("pptp: rx PPTP_ECHO_RPLY\n");
		{
			struct pptp_echo_rply *packet = (struct pptp_echo_rply *)buffer;

			if ((conn->ka_state == KA_OUTSTANDING) &&
			    (ntoh32(packet->identifier) == conn->ka_id))
			{
				conn->ka_id++;
				conn->ka_state = KA_NONE;
				pptp_reset_timer(conn);
			}
			break;
		}
	case PPTP_ECHO_RQST:
		d_dbg("pptp: rx PPTP_ECHO_RQST\n");
		{
			struct pptp_echo_rqst *packet = (struct pptp_echo_rqst *)buffer;
			struct pptp_echo_rply reply =
			{
				PPTP_HEADER_CTRL(PPTP_ECHO_RPLY),
				packet->identifier,	/* skip hton32(ntoh32(id)) */
				hton8(1), hton8(PPTP_GENERAL_ERROR_NONE), 0
			};
			pptp_send_ctrl_packet(conn, &reply, sizeof (reply));
			pptp_reset_timer(conn);
			break;
		}
	/* ----------- OUTGOING CALL MESSAGES ------------ */
	case PPTP_OUT_CALL_RQST:
		d_dbg("pptp: rx PPTP_OUT_CALL_RQST\n");
		{
			struct pptp_out_call_rqst *packet = (struct pptp_out_call_rqst *)buffer;
			struct pptp_out_call_rply reply =
			{
				PPTP_HEADER_CTRL(PPTP_OUT_CALL_RPLY),
				0 /* callid */ , packet->call_id, 1,
				PPTP_GENERAL_ERROR_NONE, 0,
				hton32(PPTP_CONNECT_SPEED),
				hton16(PPTP_WINDOW), hton16(PPTP_DELAY), 0
			};
			/* XXX PAC: eventually this should make an outgoing call. XXX */
			reply.result_code = hton8(7);	/* outgoing calls verboten */
			pptp_send_ctrl_packet(conn, &reply, sizeof (reply));
			break;
		}
	case PPTP_OUT_CALL_RPLY:
		d_dbg("pptp: rx PPTP_OUT_CALL_RPLY\n");
		{
			struct pptp_out_call_rply *packet = (struct pptp_out_call_rply *)buffer;
			PPTP_CALL *call = conn->call;
			u_int16_t callid = ntoh16(packet->call_id_peer);
			if (callid != call->call_id)
			{
				d_warn("pptp: PPTP_OUT_CALL_RPLY received for non-existant call.\n");
				break;
			}
			if (call->call_type != PPTP_CALL_PNS)
			{
				d_warn("Ack!  How did this call_type get here?");	/* XXX? */
				break;
			}
			if (call->state.pns == PNS_WAIT_REPLY)
			{
				/* check for errors */
				if (packet->result_code != 1)
				{
					/* An error.  Log it verbosely. */
					unsigned int legal_error_value = sizeof(pptp_general_errors) / sizeof(pptp_general_errors[0]);
					int err = packet->error_code;
					d_error("Our outgoing call request [callid %d] has not been accepted.", (int) callid);
					d_error("Reply result code is %d '%s'. Error code is %d, Cause code is %d", packet->result_code,
						pptp_out_call_reply_result[packet->result_code <= MAX_OUT_CALL_REPLY_RESULT ? packet->result_code : 0],
						err, packet->cause_code);
					if ((err > 0) && (err < legal_error_value))
					{
						if (packet->result_code != PPTP_RESULT_GENERAL_ERROR)
							d_error("Result code is something else then \"general error\", so the following error is probably bogus.");
						d_error("Error is '%s', Error message: '%s'", pptp_general_errors[err].name, pptp_general_errors[err].desc);
					}
					call->state.pns = PNS_IDLE;
					if (call->callback != NULL) call->callback(conn, call, CALL_OPEN_FAIL);
					pptp_call_destroy(conn, call);
				}
				else
				{
					/* connection established */
					call->state.pns = PNS_ESTABLISHED;
					call->peer_call_id = ntoh16(packet->call_id);
					call->speed = ntoh32(packet->speed);
					pptp_reset_timer(conn);
					/* call pptp_set_link. unless the user specified a quirk
					   and this quirk has a set_link hook, this is a noop */
					/* pptp_set_link(conn, call->peer_call_id); */

					if (call->callback != NULL) call->callback(conn, call, CALL_OPEN_DONE);
					d_info("Outgoing call established (call ID %u, peer's call ID %u).\n", call->call_id, call->peer_call_id);
				}
			}
			break;
		}
	/* ----------- INCOMING CALL MESSAGES ------------ */
	/* XXX write me XXX */
	/* ----------- CALL CONTROL MESSAGES ------------ */
	case PPTP_CALL_CLEAR_RQST:
		d_dbg("pptp: rx PPTP_CALL_CLEAR_RQST\n");
		if (conn->call)
		{
			struct pptp_call_clear_rqst *packet = (struct pptp_call_clear_rqst *) buffer;
			struct pptp_call_clear_ntfy reply =
			{
				PPTP_HEADER_CTRL(PPTP_CALL_CLEAR_NTFY),
				packet->call_id,
				1, PPTP_GENERAL_ERROR_NONE, 0, 0, {0}
			};
			d_dbg("pptp: my call id = %d, rqst call id = %d\n", conn->call->call_id, ntoh16(packet->call_id));
			if (conn->call->peer_call_id == ntoh16(packet->call_id))
			{
				PPTP_CALL *call = conn->call;
				if (call->callback != NULL) call->callback(conn, call, CALL_CLOSE_RQST);
				pptp_send_ctrl_packet(conn, &reply, sizeof(reply));
				pptp_call_destroy(conn, call);
				d_info("pppt: Call closed (RQST) (call id %d)\n", (int) call->call_id);
			}
		}
		break;
	case PPTP_CALL_CLEAR_NTFY:
		d_dbg("pptp: rx PPTP_CALL_CLEAR_NTFY\n");
		if (conn->call)
		{
			struct pptp_call_clear_ntfy * packet = (struct pptp_call_clear_ntfy *) buffer;
//marco, should use callid or peer_call_id?
			if (conn->call->call_id == ntoh16(packet->call_id) || (conn->call->peer_call_id == ntoh16(packet->call_id)) )
			{
				PPTP_CALL *call = conn->call;
				int err = packet->error_code;
				unsigned int legal_error_value = sizeof(pptp_general_errors)/sizeof(pptp_general_errors[0]);

				d_info("pptp: Call disconnect notification received (call id %d)\n", (int) call->call_id);
				d_info("pptp: Result code is %d: '%s'. Error code is %d, Cause code is %d\n", packet->result_code,
						pptp_call_disc_ntfy[packet->result_code <= MAX_CALL_DISC_NTFY ? packet->result_code : 0],
						err, packet->cause_code);

				if ((err > 0) && (err < legal_error_value))
				{
					if (packet->result_code != PPTP_RESULT_GENERAL_ERROR)
						d_error("Result code is something else then \"general error\", so the following error is probably bogus.\n");
					d_error("Error is '%s', Error message: '%s'\n", pptp_general_errors[err].name, pptp_general_errors[err].desc);
				}					
				pptp_call_destroy(conn, call);
			}
			/* XXX we could log call stats here XXX */
			/* XXX not all servers send this XXX */
		}
		break;
	case PPTP_SET_LINK_INFO:
		d_dbg("pptp: rx PPTP_SET_LINK_INFO\n");
		{
			/* I HAVE NO CLUE WHAT TO DO IF send_accm IS NOT 0! */
			/* this is really dealt with in the HDLC deencapsulation, anyway. */
			struct pptp_set_link_info *packet = (struct pptp_set_link_info *) buffer;
			if (ntoh32(packet->send_accm) == 0 && ntoh32(packet->recv_accm) == 0)
				break;	/* this is what we expect. */
			/* log it, otherwise. */
			d_info("PPTP_SET_LINK_INFO received from peer_callid %u\n", (unsigned int) ntoh16(packet->call_id_peer));
			d_info("  send_accm is %08lX, recv_accm is %08lX\n", (unsigned long) ntoh32(packet->send_accm), (unsigned long) ntoh32(packet->recv_accm));
			break;
		}
	default:
		d_error("pptp: Unrecognized Packet %d received.\n", (int)ntoh16(((struct pptp_header *)buffer)->ctrl_type));
		/* goto pptp_conn_close; */
		break;
	}
	return;

pptp_conn_close:
	d_warn("pptp: pptp_conn_close(%d)\n", (int) close_reason);
	pptp_conn_close(conn, close_reason);
}

/* Set link info, for pptp servers that need it.
   this is a noop, unless the user specified a quirk and
   there's a set_link hook defined in the quirks table
   for that quirk */
#if 0
void pptp_set_link(PPTP_CONN * conn, int peer_call_id)
{
	int idx, rc;

	/* if we need to send a set_link packet because of buggy
	 * hardware or pptp server, do it now */
	if ((idx = get_quirk_index()) != -1 && pptp_fixups[idx].set_link_hook)
	{
		struct pptp_set_link_info packet;
		if ((rc = pptp_fixups[idx].set_link_hook(&packet, peer_call_id)))
			warn("calling the set_link hook failed (%d)", rc);

		if (pptp_send_ctrl_packet(conn, &packet, sizeof (packet)))
		{
			pptp_reset_timer();
		}
	}
}
#endif

/********************* Get info from call structure *******************/

/* NOTE: The peer_call_id is undefined until we get a server response. */
void pptp_call_get_ids(PPTP_CONN * conn, PPTP_CALL * call, u_int16_t * call_id, u_int16_t * peer_call_id)
{
	dassert(conn != NULL);
	dassert(call != NULL);
	*call_id = call->call_id;
	*peer_call_id = call->peer_call_id;
}

void pptp_call_closure_put(PPTP_CONN * conn, PPTP_CALL * call, void *cl)
{
	dassert(conn != NULL);
	dassert(call != NULL);
	call->closure = cl;
}

void * pptp_call_closure_get(PPTP_CONN * conn, PPTP_CALL * call)
{
	dassert(conn != NULL);
	dassert(call != NULL);
	return call->closure;
}

void pptp_conn_closure_put(PPTP_CONN * conn, void *cl)
{
	dassert(conn != NULL);
	conn->closure = cl;
}

void * pptp_conn_closure_get(PPTP_CONN * conn)
{
	dassert(conn != NULL);
	return conn->closure;
}

/*********************** Handle keep-alive timer ***************/

static void pptp_handle_timer(void * eloop_ctx, void * timeout_ctx)
{
	PPTP_CONN * conn = (PPTP_CONN *)timeout_ctx;
	PPTP_CALL * call = NULL;

	d_dbg("pptp: >>> pptp_handle_timer()\n");
	
	/* "Keep Alives and Timers, 1": check connection state */
	if (conn->conn_state != CONN_ESTABLISHED)
	{
		if (conn->conn_state == CONN_WAIT_STOP_REPLY)
		{
			/* hard close. */
			pptp_conn_destroy(conn);
			return;
		}
		else
		{
			/* soft close */
			pptp_conn_close(conn, PPTP_STOP_NONE);
		}
	}
	else
	{
	}
	
	/* "Keep Alives and Timers, 2": check echo status */
	if (conn->ka_state == KA_OUTSTANDING)	/*no response to keep-alive */
	{
		d_info("pptp: no response to keep_alive!\n");
		//pptp_conn_close(conn, PPTP_STOP_NONE);
		pptp_conn_destroy(conn);
		return;
	}
	else
	{	/* ka_state == NONE *//* send keep-alive */
		struct pptp_echo_rqst rqst =
		{
			PPTP_HEADER_CTRL(PPTP_ECHO_RQST),
			hton32(conn->ka_id)
		};
		d_dbg("pptp: send echo request!\n");
		pptp_send_ctrl_packet(conn, &rqst, sizeof (rqst));
		conn->ka_state = KA_OUTSTANDING;
	}

	/* check incoming/outgoing call states for !IDLE && !ESTABLISHED */
	call = conn->call;
	if (call->call_type == PPTP_CALL_PNS)
	{
		if (call->state.pns == PNS_WAIT_REPLY)
		{	/* send close request */
			pptp_call_close(conn, call);
			dassert(call->state.pns == PNS_WAIT_DISCONNECT);
		}
		else if (call->state.pns == PNS_WAIT_DISCONNECT)
		{	/* hard-close the call */
			pptp_call_destroy(conn, call);
		}
	}
#if 0 /* PAC not implemented */
	else if (call->call_type == PPTP_CALL_PAC)
	{
		if (call->state.pac == PAC_WAIT_REPLY)
		{
			/* XXX FIXME -- drop the PAC connection XXX */
		}
		else if (call->state.pac == PAC_WAIT_CS_ANS)
		{
			/* XXX FIXME -- drop the PAC connection XXX */
		}
	}
#endif

	eloop_register_timeout(PPTP_TIMEOUT, 0, pptp_handle_timer, NULL, conn);
}

static void pptp_reset_timer(PPTP_CONN * conn)
{
	//pptp_time_out = time(NULL)+PPTP_TIMEOUT;
	eloop_cancel_timeout(pptp_handle_timer, NULL, conn);
	eloop_register_timeout(PPTP_TIMEOUT, 0, pptp_handle_timer, NULL, conn);
}
