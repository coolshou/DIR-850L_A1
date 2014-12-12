/* vi: set sw=4 ts=4: */
/* tunnel.c */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "l2tp.h"
#include "dtrace.h"
#include "eloop.h"

#ifdef DDEBUG
static char * state_name[] =
{
	"idle", "wait-ctl-reply", "wait-ctl-conn", "established",
	"received-stop-cnn", "sent-stop-cnn"
};
#endif

#define VENDOR_STR	"Alpha Networks Inc."

/* Comparison of serial numbers according to RFC 1982 */
#define SERIAL_GT(a, b) \
(((a) > (b) && (a) - (b) < 32768) || ((a) < (b) && (b) - (a) > 32768))

#define SERIAL_LT(a, b) \
(((a) < (b) && (b) - (a) < 32768) || ((a) > (b) && (a) - (b) > 32768))

l2tp_tunnel * the_tunnel = NULL;

/* static function prototypes */
static void tunnel_free(l2tp_tunnel * tunnel);
static void tunnel_xmit_queued_messages(l2tp_tunnel * tunnel);
static void tunnel_setup_hello(l2tp_tunnel * tunnel);

extern void modem_hungup(void);

/***************************************************************************/


static void tunnel_set_state(l2tp_tunnel * tunnel, int state)
{
	if (state == tunnel->state) return;
	d_dbg("l2tp: tunnel_set_state(): tunnel(%s) %s -> %s\n",
			l2tp_debug_tunnel_to_str(tunnel), state_name[tunnel->state], state_name[state]);
	tunnel->state = state;
}

static int tunnel_outstanding_frames(l2tp_tunnel * tunnel)
{
	int Ns = (int)tunnel->Ns_on_wire;
	int Nr = (int)tunnel->peer_Nr;
	if (Ns >= Nr) return Ns - Nr;
	return 65535 - Nr + Ns;
}

#ifdef DDEBUG
static char const * tunnel_flow_control_stats(l2tp_tunnel * tunnel)
{
	static char buf[256];
	snprintf(buf, sizeof(buf), "rws=%d cwnd=%d ssthresh=%d outstanding=%d",
			(int)tunnel->peer_rws,
			tunnel->cwnd,
			tunnel->ssthresh,
			tunnel_outstanding_frames(tunnel));
	return buf;
}
#endif

static uint16_t tunnel_make_tid(void)
{
	uint16_t id;
	for (;;)
	{
		L2TP_RANDOM_FILL(id);
		if (!id) continue;
		return id;
	}
}

static void tunnel_enqueue_dgram(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	dgram->next = NULL;
	if (tunnel->xmit_queue_tail)
	{
		tunnel->xmit_queue_tail->next = dgram;
		tunnel->xmit_queue_tail = dgram;
	}
	else
	{
		tunnel->xmit_queue_head = dgram;
		tunnel->xmit_queue_tail = dgram;
	}
	if (!tunnel->xmit_new_dgrams)
	{
		tunnel->xmit_new_dgrams = dgram;
	}
	dgram->Ns = tunnel->Ns;
	tunnel->Ns++;
	d_dbg("l2tp: tunnel_enqueue_dgram(%s, %s) %s\n",
			l2tp_debug_tunnel_to_str(tunnel),
			l2tp_debug_message_type_to_str(dgram->msg_type),
			tunnel_flow_control_stats(tunnel));
}

static void tunnel_dequeue_head(l2tp_tunnel * tunnel)
{
	l2tp_dgram * dgram = tunnel->xmit_queue_head;
	if (dgram)
	{
		tunnel->xmit_queue_head = dgram->next;
		if (tunnel->xmit_new_dgrams == dgram)
		{
			tunnel->xmit_new_dgrams = dgram->next;
		}
		if (tunnel->xmit_queue_tail == dgram)
		{
			tunnel->xmit_queue_tail = NULL;
		}
		l2tp_dgram_free(dgram);
	}
}

/****************************************************************************
 * FUNCTION: tunnel_do_ack
 * ARGUMENT:
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   If there is a frame on our queue, update it's Nr and run queue; if not,
 *   send a ZLB immediately.
 ***************************************************************************/
static void tunnel_do_ack(void * eloop_ctx, void * timeout_ctx)
{
	l2tp_tunnel * tunnel = (l2tp_tunnel *)timeout_ctx;

	/* Ack handler has fired */
	tunnel->ack_scheduled = 0;

	d_dbg("l2tp: tunnel_do_ack(%s)\n", l2tp_debug_tunnel_to_str(tunnel));

	/* If nothing is on the queue, add a ZLB
	 * or, if we cannot transmit because of flow-control, send ZLB */
	if (!tunnel->xmit_new_dgrams || tunnel_outstanding_frames(tunnel) >= tunnel->cwnd)
	{
		l2tp_tunnel_send_ZLB(tunnel);
		return;
	}
	/* Run the queue */
	tunnel_xmit_queued_messages(tunnel);
}

/****************************************************************************
 * FUNCTION: tunnel_do_hello
 * ARGUMENTS:
 * RETURNS:
 * DESCRIPTIOIN:
 *   Deallocates all tunnel state
 ***************************************************************************/
static void tunnel_do_hello(void * eloop_ctx, void * timeout_ctx)
{
	l2tp_tunnel * tunnel = (l2tp_tunnel *)timeout_ctx;
	l2tp_dgram * dgram;

	/* Hello handler has fired */
	tunnel->hello_scheduled = 0;

	/* Reschedule HELLO timer */
	tunnel_setup_hello(tunnel);

	/* Send a HELLO message */
	dgram = l2tp_dgram_new_control(MESSAGE_HELLO, tunnel->assigned_id, 0);
	if (dgram) l2tp_tunnel_xmit_control_message(tunnel, dgram);
}

/****************************************************************************
 * FUNCTION: tunnel_handle_timeout
 * ARGUMENT:
 * RETURNS:
 * DESCRIPTION:
 *   Called when retransmission timer fires.
 ***************************************************************************/
static void tunnel_handle_timeout(void * eloop_ctx, void * timeout_ctx)
{
	l2tp_tunnel * tunnel = (l2tp_tunnel *)timeout_ctx;

	/* Reset xmit_new_dgrams */
	tunnel->xmit_new_dgrams = tunnel->xmit_queue_head;

	/* Set Ns on wire to Ns of first frame in queue */
	if (tunnel->xmit_queue_head) tunnel->Ns_on_wire = tunnel->xmit_queue_head->Ns;

	/* Go back to slow-start phase */
	tunnel->ssthresh = tunnel->cwnd / 2;
	if (!tunnel->ssthresh) tunnel->ssthresh = 1;
	tunnel->cwnd = 1;
	tunnel->cwnd_counter = 0;

	tunnel->retransmissions++;
	if (tunnel->retransmissions >= MAX_RETRANSMISSIONS)
	{
		d_error("l2tp: Too many retransmission on tunnel (%s); closing down\n", l2tp_debug_tunnel_to_str(tunnel));
		tunnel_free(tunnel);
		return;
	}

	/* Double timeout, capping at 8 seconds */
	if (tunnel->timeout < 8) tunnel->timeout *= 2;

	/* Run the queue */
	tunnel_xmit_queued_messages(tunnel);
}

/****************************************************************************
 * FUNCTION: tunnel_schedule_ack
 * ARGUMENT:
 *   tunnel -- the tunnel
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Schedules an ack for 100ms from now. The hope is we'll be able to
 *   piggy-back the ack on a packet in the queue; if not, we'll send a ZLB.
 ***************************************************************************/
static void tunnel_schedule_ack(l2tp_tunnel * tunnel)
{
	d_dbg("l2tp: tunnel_schedule_ack(%s)\n", l2tp_debug_tunnel_to_str(tunnel));

	/* Already scheduled ? Do nothing */
	if (tunnel->ack_scheduled) return;
	tunnel->ack_scheduled = 1;
	eloop_register_timeout(0, 100000, tunnel_do_ack, NULL, tunnel);
}

/****************************************************************************
 * FUNCTION: tunnel_setup_hello
 * ARGUMENT:
 *   tunnel -- the tunnel
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Sets up timer for sending HELLO messages
 ***************************************************************************/
static void tunnel_setup_hello(l2tp_tunnel * tunnel)
{
	if (tunnel->hello_scheduled) eloop_cancel_timeout(tunnel_do_hello, NULL, tunnel);
	eloop_register_timeout(60,0, tunnel_do_hello, NULL, tunnel);
	tunnel->hello_scheduled = 1;
}

/****************************************************************************
 * FUNCTION: tunnel_dequeue_acked_packets
 * ARGUMENTS:
 *   tunnel -- the tunnel
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Discards all acknowledged packets from our xmit queue.
 ***************************************************************************/
static void tunnel_dequeue_acked_packets(l2tp_tunnel * tunnel)
{
	l2tp_dgram * dgram = tunnel->xmit_queue_head;

	d_dbg("l2tp: tunnel_dequeue_acked_packets(%s) %s\n", l2tp_debug_tunnel_to_str(tunnel), tunnel_flow_control_stats(tunnel));
	while (dgram)
	{
		if (SERIAL_LT(dgram->Ns, tunnel->peer_Nr))
		{
			tunnel_dequeue_head(tunnel);
			if (tunnel->cwnd < tunnel->ssthresh)
			{
				/* Slow start: increment CWND for each packet dequeued */
				tunnel->cwnd++;
				if (tunnel->cwnd > tunnel->peer_rws) tunnel->cwnd = tunnel->peer_rws;
			}
			else
			{
				/* Congestion avoidance: increment CWND for each CWND packets dequeued */
				tunnel->cwnd_counter++;
				if (tunnel->cwnd_counter >= tunnel->cwnd)
				{
					tunnel->cwnd_counter = 0;
					tunnel->cwnd++;
					if (tunnel->cwnd > tunnel->peer_rws) tunnel->cwnd = tunnel->peer_rws;
				}
			}
		}
		else
		{
			break;
		}
		dgram = tunnel->xmit_queue_head;
	}
}

/****************************************************************************
 * FUNCTION: tunnel_xmit_queue_messages
 * ARGUMENTS:
 *   tunnel -- the tunnel
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Transmit as many control messages as possible from the queue.
 ***************************************************************************/
static void tunnel_xmit_queued_messages(l2tp_tunnel * tunnel)
{
	l2tp_dgram * dgram;

	dgram = tunnel->xmit_new_dgrams;
	if (!dgram) return;

	d_dbg("l2tp: xmit_queued(%s): %s\n", l2tp_debug_tunnel_to_str(tunnel), tunnel_flow_control_stats(tunnel));

	while (dgram)
	{
		/* If window is closed, we can't transmit anything */
		if (tunnel_outstanding_frames(tunnel) >= tunnel->cwnd) break;

		/* Update Nr */
		dgram->Nr = tunnel->Nr;

		/* Tid might have changed if call was initialized before
		 * tunnel establishment was complete */
		dgram->tid = tunnel->assigned_id;

		if (l2tp_dgram_send_to_wire(dgram, &tunnel->peer_addr) < 0) break;

		/* Cancel any pending ack */

		tunnel->xmit_new_dgrams = dgram->next;
		tunnel->Ns_on_wire = dgram->Ns + 1;
		d_dbg("l2tp: loop in xmit_queued(%s): %s\n", l2tp_debug_tunnel_to_str(tunnel), tunnel_flow_control_stats(tunnel));
		dgram = dgram->next;
	}

	eloop_cancel_timeout(tunnel_handle_timeout, NULL, tunnel);
	eloop_register_timeout(tunnel->timeout, 0, tunnel_handle_timeout, NULL, tunnel);
}

/****************************************************************************
 * FUNCTION: tunnel_send_ZLB
 * ARGUMENTS:
 *   tunnel -- the tunnel
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Sends a ZLB ack packet.
 ***************************************************************************/
void l2tp_tunnel_send_ZLB(l2tp_tunnel * tunnel)
{
	l2tp_dgram * dgram;

	dgram = l2tp_dgram_new_control(MESSAGE_ZLB, tunnel->assigned_id, 0);
	if (!dgram)
	{
		d_error("l2tp: l2tp_tunnel_send_ZLB: Out of memory\n");
		return;
	}
	dgram->Nr = tunnel->Nr;
	dgram->Ns = tunnel->Ns;
	l2tp_dgram_send_to_wire(dgram, &tunnel->peer_addr);
	l2tp_dgram_free(dgram);
}

/****************************************************************************
 * FUNCTION: tunnel_xmit_control_message
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- the datagram to transmit. After return from this
 *   		  function, the tunnel "owns" the dgram and the caller should
 *   		  no longer use it.
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Transmits a control message. If there is no room in the transmit
 *   window, queues the message.
 ***************************************************************************/
void l2tp_tunnel_xmit_control_message(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	/* Queue the datagram in case we need to retransmit it */
	tunnel_enqueue_dgram(tunnel, dgram);

	/* Run the queue */
	tunnel_xmit_queued_messages(tunnel);
}

/****************************************************************************
 * FUNCTION: tunnel_cancel_ack_handler
 * 
 ***************************************************************************/
void l2tp_tunnel_cancel_ack_handler(l2tp_tunnel * tunnel)
{
	if (tunnel->ack_scheduled)
	{
		eloop_cancel_timeout(tunnel_do_ack, NULL, tunnel);
		tunnel->ack_scheduled = 0;
	}
}

/****************************************************************************
 * FUNCTION: tunnel_new
 * ARGUMENTS:
 *   None
 * RETURNS:
 *   A newly-allocated and initialized l2tp_tunnel object
 ***************************************************************************/
static l2tp_tunnel * tunnel_new(void)
{
	l2tp_tunnel * tunnel;

	tunnel = malloc(sizeof(l2tp_tunnel));
	if (!tunnel) return NULL;

	memset(tunnel, 0, sizeof(l2tp_tunnel));
	tunnel->rws = 4;
	tunnel->peer_rws = 1;
	tunnel->timeout = 1;
	tunnel->my_id = tunnel_make_tid();
	tunnel->ssthresh = 1;
	tunnel->cwnd = 1;
	tunnel->pppox_fd = -1;

	d_dbg("l2tp: tunnel_new() -> %s\n", l2tp_debug_tunnel_to_str(tunnel));
	return tunnel;
}

/****************************************************************************
 * FUNCTION: tunnel_free
 * ARGUMENTS:
 *   tunnel -- tunnel to free
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Free all memory used by tunnel
 ***************************************************************************/
static void tunnel_free(l2tp_tunnel * tunnel)
{
	dassert(tunnel);

	d_dbg("tunnel_free() >>>\n");

	while (tunnel->xmit_queue_head) tunnel_dequeue_head(tunnel);
	if (tunnel->pppox_fd > 0) close(tunnel->pppox_fd);
	free(tunnel);
	the_tunnel = NULL;
	d_dbg("Sock is %d, closing ...!\n", Sock);
	if (pppol2tp_udp_fd > 0) close(pppol2tp_udp_fd);
	pppol2tp_udp_fd = -1;
	if (Sock > 0) close(Sock);
	Sock = -1;
	eloop_terminate();
	d_dbg("tunnel_free() <<<\n");
}

/****************************************************************************
 * FUNCTION: tunnel_send_SCCRQ
 * ARGUMENTS:
 *   tunnel -- the tunnel we wish to establish
 * RETURNS:
 *   0 if we handed the datagram off to control transport, -1 otherwise.
 * DESCRIPTION:
 *   Sends SCCRQ to establish a tunnel.
 ***************************************************************************/
static int tunnel_send_SCCRQ(l2tp_tunnel * tunnel)
{
	uint16_t u16;
	uint32_t u32;
	unsigned char tie_breaker[8];
	unsigned char challenge[16];
	int old_hide;
	l2tp_dgram * dgram;

	dgram = l2tp_dgram_new_control(MESSAGE_SCCRQ, 0, 0);
	dassert(dgram);
	if (!dgram) return -1;

	/* Add the AVPs */
	/* HACK! Cisco equipment cannot handle hidden AVP's in SCCRQ.
	 * So we temporarily disable AVP hiding */
	old_hide = tunnel->peer->hide_avps;
	tunnel->peer->hide_avps = 0;

	/* Protocol version */
	u16 = htons(0x0100);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_PROTOCOL_VERSION, &u16);

	/* Framing capabilities - hard-coded as sync and async */
	u32 = htonl(3);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u32), VENDOR_IETF, AVP_FRAMING_CAPABILITIES, &u32);

	/* Host name */
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, strlen(Hostname), VENDOR_IETF, AVP_HOST_NAME, Hostname);

	/* Assigned ID */
	u16 = htons(tunnel->my_id);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);

	/* Receive window size */
	u16 = htons(tunnel->rws);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_RECEIVE_WINDOW_SIZE, &u16);

	/* Tie-breaker */
	l2tp_random_fill(tie_breaker, sizeof(tie_breaker));
	l2tp_dgram_add_avp(dgram, tunnel, NOT_MANDATORY, sizeof(tie_breaker), VENDOR_IETF, AVP_TIE_BREAKER, tie_breaker);

	/* Vendor name */
	l2tp_dgram_add_avp(dgram, tunnel, NOT_MANDATORY, strlen(VENDOR_STR), VENDOR_IETF, AVP_VENDOR_NAME, VENDOR_STR);

	/* Challenge */
	if (tunnel->peer->secret_len)
	{
		l2tp_random_fill(challenge, sizeof(challenge));
		l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(challenge), VENDOR_IETF, AVP_CHALLENGE, challenge);

		/* Compute and save expected response */
		l2tp_generate_response(MESSAGE_SCCRP, tunnel->peer->secret, challenge, sizeof(challenge), tunnel->expected_response);
	}

	tunnel->peer->hide_avps = old_hide;

	/* And ship it out */
	l2tp_tunnel_xmit_control_message(tunnel, dgram);
	return 1;
}

/*****************************************************************************
 * FUNCTION: tunnel_send_StopCCN
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   result_code -- what to put in result-code AVP
 *   error_code -- what to put in error-code field
 *   fmt -- format string for error message
 *   ... -- args for formatting error message
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Sends a StopCCN control message.
 ****************************************************************************/
static void tunnel_send_StopCCN(l2tp_tunnel * tunnel, int result_code, int error_code, char const * fmt, ...)
{
	char buf[256];
	va_list ap;
	uint16_t u16;
	uint16_t len;
	l2tp_dgram * dgram;

	/* Build the buffer for the result-code AVP */
	buf[0] = result_code / 256;
	buf[1] = result_code & 255;
	buf[2] = error_code / 256;
	buf[3] = error_code & 255;

	va_start(ap, fmt);
	vsnprintf(buf+4, 256-4, fmt, ap);
	buf[255] = 0;
	va_end(ap);

	d_dbg("l2tp: tunnel_send_StopCCN(%s, %d, %d, %s)\n",
			l2tp_debug_tunnel_to_str(tunnel), result_code, error_code, buf+4);

	len = 4 + strlen(buf+4);
	/* Build the datagram */
	dgram = l2tp_dgram_new_control(MESSAGE_StopCCN, tunnel->assigned_id, 0);
	if (!dgram) return;

	/* Add assigned tunnel ID */
	u16 = htons(tunnel->my_id);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);

	/* Add result code */
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, len, VENDOR_IETF, AVP_RESULT_CODE, buf);

	/* TODO: Clean up */
	tunnel_set_state(tunnel, TUNNEL_SENT_STOP_CCN);

	/* Ship it out */
	l2tp_tunnel_xmit_control_message(tunnel, dgram);
}

/*****************************************************************************
 * FUNCTION: tunnel_set_params
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- incoming SCCRQ or SCCRP datagram
 * RETURNS:
 *   0 if OK, non-zero on error
 * DESCRIPTION:
 *   Sets up initial tunnel parameters (assigned ID, etc.)
 ****************************************************************************/
static int tunnel_set_params(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	unsigned char * val;
	int mandatory, hidden;
	uint16_t len, vendor, type;
	int err = 0;
	int found_response =0;
	uint16_t u16;

	/* Find peer */
	/* We now support only one peer at a time, so we don't need to find peer */

	/* Get assigned tunnel ID */
	val = l2tp_dgram_search_avp(dgram, tunnel, &mandatory, &hidden, &len, VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID);
	if (!val)
	{
		d_error("l2tp: No assinged tunnel ID AVP in SCCRQ/SCCRP\n");
		tunnel_free(tunnel);
		return -1;
	}

	if (!l2tp_dgram_validate_avp(VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, len, mandatory))
	{
		tunnel_free(tunnel);
		return -1;
	}

	/* Set tid */
	tunnel->assigned_id = ((unsigned int)val[0]) * 256 + (unsigned int)val[1];
	if (!tunnel->assigned_id)
	{
		d_error("l2tp: Invalid assigned-tunnel-ID of zero\n");
		tunnel_free(tunnel);
		return -1;
	}

	/* Validate peer */
	if (!tunnel->peer)
	{
		d_error("l2tp: Peer %s is not authorized to create a tunnel\n", inet_ntoa(tunnel->peer_addr.sin_addr));
		tunnel_send_StopCCN(tunnel, RESULT_NOAUTH, ERROR_OK, "%s", l2tp_get_errmsg());
		return -1;
	}

	/* Pull out and examine AVP's */
	while (1)
	{
		val = l2tp_dgram_pull_avp(dgram, tunnel, &mandatory, &hidden, &len, &vendor, &type, &err);
		if (!val)
		{
			if (err)
			{
				tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_BAD_VALUE, "%s", l2tp_get_errmsg());
				return -1;
			}
			break;
		}

		/* Unknown vendor ? Ignore, unless mandatory */
		if (vendor != VENDOR_IETF)
		{
			if (!mandatory) continue;
			tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_UNKNOWN_AVP_WITH_M_BIT,
					"Unknown mandatory attribute (vendor=%d, type=%d) in %s",
					(int)vendor, (int)type, l2tp_debug_avp_type_to_str(dgram->msg_type));
			return -1;
		}

		/* Validate AVP, ignore invalid AVP's without M bit set */
		if (!l2tp_dgram_validate_avp(vendor, type, len, mandatory))
		{
			if (!mandatory) continue;
			tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_BAD_LENGTH, "%s", l2tp_get_errmsg());
			return -1;
		}

		switch (type)
		{
		case AVP_PROTOCOL_VERSION:
			u16 = ((uint16_t)val[0]) * 256 + val[1];
			if (u16 != 0x100)
			{
				tunnel_send_StopCCN(tunnel, RESULT_UNSUPPORTED_VERSION, 0x100, "Unsupported protocol version");
				return -1;
			}
			break;

		case AVP_HOST_NAME:
			if (len >= MAX_HOSTNAME) len = MAX_HOSTNAME-1;
			memcpy(tunnel->peer_hostname, val, len);
			tunnel->peer_hostname[len+1] = 0;
			d_dbg("l2tp: %s Peer host name is '%s'\n", l2tp_debug_tunnel_to_str(tunnel), tunnel->peer_hostname);
			break;

		case AVP_FRAMING_CAPABILITIES:
			/* TODO: Do we care about framing capabilities? */
			break;

		case AVP_ASSIGNED_TUNNEL_ID:
			/* Already been handled */
			break;

		case AVP_BEARER_CAPABILITIES:
			/* TODO: Do we care ? */
			break;

		case AVP_RECEIVE_WINDOW_SIZE:
			u16 = ((uint16_t)val[0]) * 256 + val[1];
			/* Silently correct bogus rwin */
			if (u16 < 1) u16 = 1;
			tunnel->peer_rws = u16;
			tunnel->ssthresh = u16;
			break;

		case AVP_CHALLENGE:
			if (tunnel->peer->secret_len)
			{
				l2tp_generate_response(
					((dgram->msg_type == MESSAGE_SCCRQ) ? MESSAGE_SCCRP : MESSAGE_SCCCN),
					tunnel->peer->secret,
					val, len, tunnel->response);
			}
			break;

		case AVP_CHALLENGE_RESPONSE:
			/* Length has been validated by l2tp_dgram_validate_avp */
			if (tunnel->peer->secret_len)
			{
				if (memcmp(val, tunnel->expected_response, MD5LEN))
				{
					tunnel_send_StopCCN(tunnel, RESULT_NOAUTH, ERROR_BAD_VALUE, "Incorret challenge response");
					return -1;
				}
			}
			found_response = 1;
			break;

		case AVP_TIE_BREAKER:
			/* TODO: Handle tie-breaker */
			break;

		case AVP_FIRMWARE_REVISION:
			/* TODO: Do we care ? */
			break;

		case AVP_VENDOR_NAME:
			/* TODO: Do we care ? */
			break;

		default:
			/* TODO: Maybe print an error ? */
			break;
		}
	}

	if (tunnel->peer->secret_len && !found_response && dgram->msg_type != MESSAGE_SCCRQ)
	{
		tunnel_send_StopCCN(tunnel, RESULT_NOAUTH, 0, "Missing challenge-response");
		return -1;
	}
	return 0;
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_StopCCN
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- received datagram
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Processes a received StopCCN frame (shuts tunnel down)
 ****************************************************************************/
static void tunnel_handle_StopCCN(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	unsigned char * val;
	int mandatory, hidden;
	uint16_t len, vendor, type;
	int err;

	/* Shut down all the sessions */
	if (the_session)
	{
		l2tp_session_free(the_session, "Tunnel closing down");
	}

	tunnel_set_state(tunnel, TUNNEL_RECEIVED_STOP_CCN);

	/* If there are any queued datagrams waiting for transmission,
	 * nuke them and adjust tunnel's Ns to whatever peer has ack'd */
	/* TODO: Is this correct ? */
	if (tunnel->xmit_queue_head)
	{
		tunnel->Ns = tunnel->peer_Nr;
		while (tunnel->xmit_queue_head) tunnel_dequeue_head(tunnel);
	}

	/* Parse the AVP's */
	while (1)
	{
		val = l2tp_dgram_pull_avp(dgram, tunnel, &mandatory, &hidden, &len, &vendor, &type, &err);
		if (!val) break;

		/* Only care about assigned tunnel ID. TODO: Fix this! */
		if (vendor != VENDOR_IETF || type != AVP_ASSIGNED_TUNNEL_ID) continue;

		if (len == 2)
		{
			tunnel->assigned_id = ((unsigned int)val[0]) * 256 + (unsigned int)val[1];
		}
	}

	/* No point in delaying ack; there will never be a datagram for
	 * piggy-back. So cancel ack timer and shoot out a ZLB now */
	l2tp_tunnel_cancel_ack_handler(tunnel);
	l2tp_tunnel_send_ZLB(tunnel);
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_SCCRQ
 * ARGUMENTS:
 *   dgram -- the received datagram
 *   from -- address of sender
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Handles an incoming SCCRQ
 ****************************************************************************/
static void tunnel_handle_SCCRQ(l2tp_dgram * dgram, struct sockaddr_in * from)
{
	l2tp_tunnel * tunnel = NULL;
	uint16_t u16;
	uint32_t u32;
	unsigned char challenge[16];

	/* TODO: Check if this is a retransmitted SCCRQ */
	/* Allocate a tunnel */
	tunnel = tunnel_new();
	if (!tunnel)
	{
		d_error("l2tp: Unable to allocate new tunnel\n");
		return;
	}

	tunnel->peer_addr = *from;

	/* We've received out first control datagram (SCCRQ) */
	tunnel->Nr = 1;

	if (tunnel_set_params(tunnel, dgram) < 0) return;

	/* Hunky-dory. Send SCCRP */
	dgram = l2tp_dgram_new_control(MESSAGE_SCCRP, tunnel->assigned_id, 0);
	if (!dgram)
	{	/* Doh! Out of resources. Not much change of StopCCN working... */
		tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES, "Out of memory");
		return;
	}

	/* Protocol version */
	u16 = htons(0x0100);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_PROTOCOL_VERSION, &u16);

	/* Framing capabilities -- hard-coded as sync and async */
	u32 = htonl(3);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u32), VENDOR_IETF, AVP_FRAMING_CAPABILITIES, &u32);

	/* Host name */
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, strlen(Hostname), VENDOR_IETF, AVP_HOST_NAME, Hostname);

	/* Assigned ID */
	u16 = htons(tunnel->my_id);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_TUNNEL_ID, &u16);

	/* Receive window size */
	u16 = htons(tunnel->rws);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_RECEIVE_WINDOW_SIZE, &u16);

	/* Vendor name */
	l2tp_dgram_add_avp(dgram, tunnel, NOT_MANDATORY, strlen(VENDOR_STR), VENDOR_IETF, AVP_VENDOR_NAME, VENDOR_STR);

	if (tunnel->peer->secret_len)
	{
		/* Response */
		l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, MD5LEN, VENDOR_IETF, AVP_CHALLENGE_RESPONSE, tunnel->response);

		/* Challenge */
		l2tp_random_fill(challenge, sizeof(challenge));
		l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(challenge), VENDOR_IETF, AVP_CHALLENGE, challenge);

		/* Compute and save expected response */
		l2tp_generate_response(MESSAGE_SCCCN, tunnel->peer->secret, challenge, sizeof(challenge), tunnel->expected_response);
	}

	tunnel_set_state(tunnel, TUNNEL_WAIT_CTL_CONN);

	/* And ship it out */
	l2tp_tunnel_xmit_control_message(tunnel, dgram);
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_SCCRP
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- the incoming datagram
 * RETURNS:
 *   Nothing:
 * DESCRIPTION:
 *   Handles an incoming SCCRP
 ****************************************************************************/
static void tunnel_handle_SCCRP(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	/* TODO: If we don't get challenge response at all, barf
	 * Are we expecting SCCRP ? */
	if (tunnel->state != TUNNEL_WAIT_CTL_REPLY)
	{
		tunnel_send_StopCCN(tunnel, RESULT_FSM_ERROR, 0, "Not expecting SCCRP");
		return;
	}

	/* Extract tunnel params */
	if (tunnel_set_params(tunnel, dgram) < 0) return;

	tunnel_set_state(tunnel, TUNNEL_ESTABLISHED);
	tunnel_setup_hello(tunnel);

	/* Hunky-dory. Send SCCCN */
	dgram = l2tp_dgram_new_control(MESSAGE_SCCCN, tunnel->assigned_id, 0);
	if (!dgram)
	{
		/* Doh! Out of resources. Not much chance of StopCCN working... */
		tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES, "Out of memory");
		return;
	}

	/* Add response */
	if (tunnel->peer->secret_len)
	{
		l2tp_dgram_add_avp(dgram, tunnel, MANDATORY,
			MD5LEN, VENDOR_IETF, AVP_CHALLENGE_RESPONSE, tunnel->response);
	}

	/* Ship it out */
	l2tp_tunnel_xmit_control_message(tunnel, dgram);

	pppol2tp_tunnel(tunnel);

	/* Tell sessions tunnel has been established */
	dassert(the_session!=NULL);
	l2tp_session_notify_tunnel_open(the_session);
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_SCCCN
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- the incoming datagram
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Handles an incoming SCCCN
 ****************************************************************************/
static void tunnel_handle_SCCCN(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	unsigned char * val;
	uint16_t len;
	int hidden, mandatory;

	/* Are we expecting SCCCN ? */
	if (tunnel->state != TUNNEL_WAIT_CTL_CONN)
	{
		tunnel_send_StopCCN(tunnel, RESULT_FSM_ERROR, 0, "Not expecting SCCCN");
		return;
	}

	/* Check challenge response */
	if (tunnel->peer->secret_len)
	{
		val = l2tp_dgram_search_avp(dgram, tunnel, &mandatory, &hidden, &len, VENDOR_IETF, AVP_CHALLENGE_RESPONSE);
		if (!val)
		{
			tunnel_send_StopCCN(tunnel, RESULT_NOAUTH, 0, "Missing challenge-response");
			return;
		}
		if (!l2tp_dgram_validate_avp(VENDOR_IETF, AVP_CHALLENGE_RESPONSE, len, mandatory))
		{
			tunnel_send_StopCCN(tunnel, RESULT_GENERAL_ERROR, ERROR_BAD_LENGTH, "Invalid challenge-response");
			return;
		}
		if (memcmp(val, tunnel->expected_response, MD5LEN))
		{
			tunnel_send_StopCCN(tunnel, RESULT_NOAUTH, ERROR_BAD_VALUE, "Incorrect challenge response");
			return;
		}
	}

	pppol2tp_tunnel(tunnel);

	tunnel_set_state(tunnel, TUNNEL_ESTABLISHED);
	tunnel_setup_hello(tunnel);

	/* Tell sessions tunnel has been established */
	dassert(the_session!=NULL);
	l2tp_session_notify_tunnel_open(the_session);
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_ICRQ
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- the datagram
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Handle ICRQ (Incoming Call ReQuest)
 ****************************************************************************/
static void tunnel_handle_ICRQ(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	uint16_t u16;
	unsigned char * val;
	uint16_t len;
	int mandatory, hidden;

	/* Get assigned session ID */
	val = l2tp_dgram_search_avp(dgram, tunnel, &mandatory, &hidden, &len, VENDOR_IETF, AVP_ASSIGNED_SESSION_ID);
	if (!val)
	{
		d_error("l2tp: No assigned tunnel ID AVP in ICRQ\n");
		return;
	}
	if (!l2tp_dgram_validate_avp(VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, len, mandatory))
	{
		/* TODO: send CDN */
		return;
	}

	/* Set assigned session ID */
	u16 = ((uint16_t)val[0]) * 256 + (uint16_t)val[1];
	if (!u16)
	{
		/* TODO: send CDN */
		return;
	}

	/* Tunnel in wrong state? */
	if (tunnel->state != TUNNEL_ESTABLISHED)
	{
		/* TODO: send CDN */
		return;
	}

	/* Set up new incoming call */
	/* TODO: include calling number */
	//l2tp_session_lns_handle_incoming_call(tunnel, u16, dgram, "");
}

/*****************************************************************************
 * FUNCTION: tunnel_process_received_datagram
 * ARGUMENTS:
 *   tunnel -- the tunnel
 *   dgram -- the received datagram
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Handles a received control message for this tunnel
 ****************************************************************************/
static void tunnel_process_received_datagram(l2tp_tunnel * tunnel, l2tp_dgram * dgram)
{
	l2tp_session * ses = NULL;

	/* If message is associated with existing session, find session */
	d_dbg("l2tp: tunnel_process_received_datagram(%s, %s)\n",
			l2tp_debug_tunnel_to_str(tunnel),
			l2tp_debug_message_type_to_str(dgram->msg_type));

	switch (dgram->msg_type)
	{
	case MESSAGE_OCRP:
	case MESSAGE_OCCN:
	case MESSAGE_ICRP:
	case MESSAGE_ICCN:
	case MESSAGE_CDN:
		ses = (the_session->my_id == dgram->sid) ? the_session : NULL;
		if (!ses)
		{
			d_error("l2tp: Session-related message for unknown session %d\n", (int) dgram->sid);
			return;
		}
		break;
	}

	switch (dgram->msg_type)
	{
	case MESSAGE_StopCCN:	tunnel_handle_StopCCN(tunnel, dgram);	break;
	case MESSAGE_SCCRP:		tunnel_handle_SCCRP(tunnel, dgram);		break;
	case MESSAGE_SCCCN:		tunnel_handle_SCCCN(tunnel, dgram);		break;
	case MESSAGE_ICRQ:		tunnel_handle_ICRQ(tunnel, dgram);		break;
	case MESSAGE_CDN:		l2tp_session_handle_CDN(ses, dgram);	break;
	case MESSAGE_ICRP:		l2tp_session_handle_ICRP(ses, dgram);	break;
	case MESSAGE_ICCN:		l2tp_session_handle_ICCN(ses, dgram);	break;
	}
}

/*****************************************************************************
 * FUNCTIONL tunnel_establish
 * ARGUMENTS:
 *   peer -- peer with which to establish tunnel
 * RETURNS:
 *   A newly-allocated tunnel, or NULL on error.
 * DESCRIPTION:
 *   Begins tunnel establishment to peer.
 ****************************************************************************/
l2tp_tunnel * l2tp_tunnel_establish(l2tp_peer * peer)
{
	l2tp_tunnel * tunnel;

	dassert(the_tunnel==NULL);
	tunnel = tunnel_new();
	if (!tunnel) return NULL;

	tunnel->peer = peer;
	tunnel->peer_addr = peer->addr;

	tunnel_send_SCCRQ(tunnel);
	tunnel_set_state(tunnel, TUNNEL_WAIT_CTL_REPLY);
	the_tunnel = tunnel;
	return tunnel;
}

/*****************************************************************************
 * FUNCTION: tunnel_handle_receive_control_datagram
 * ARGUMENTS:
 *   dgram -- received datagram
 *   from -- address of sender
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Handles a received control datagram
 ****************************************************************************/
void l2tp_tunnel_handle_received_control_datagram(l2tp_dgram * dgram, struct sockaddr_in * from)
{
	l2tp_tunnel * tunnel;

	/* If it's SCCRQ, then handle it */
	if (dgram->msg_type == MESSAGE_SCCRQ)
	{
		tunnel_handle_SCCRQ(dgram, from);
		return;
	}

	if (the_tunnel==NULL)
	{
		d_error("l2tp: tunnel not created yet !!\n");
		return;
	}
	if (the_tunnel->my_id != dgram->tid)
	{
		d_error("l2tp: Invalid control message - unknown tunnel ID %d, (expecting %d)\n", dgram->tid, the_tunnel->my_id);
		return;
	}
	tunnel = the_tunnel;

	/* Verify that source address is the tunnel's peer */
	if (tunnel->peer->validate_peer_ip)
	{
		if (from->sin_addr.s_addr != tunnel->peer_addr.sin_addr.s_addr)
		{
			d_error("l2tp: Invalid control message for tunnel %s - not sent from peer\n", l2tp_debug_tunnel_to_str(tunnel));
			return;
		}
	}

	/* Set port for replies */
	tunnel->peer_addr.sin_port = from->sin_port;

	/* Schedule an ACK for 100ms from now, but do not ack ZLB's */
	if (dgram->msg_type != MESSAGE_ZLB) tunnel_schedule_ack(tunnel);

	/* If it's an old datagram, ignore it */
	if (dgram->Ns != tunnel->Nr)
	{
		if (SERIAL_LT(dgram->Ns, tunnel->Nr))
		{
			/* Old packet: Drop it
			 * Throw away ack'd packets in our xmit queue */
			tunnel_dequeue_acked_packets(tunnel);
			return;
		}
		/* Out-of-order packet or intermediate dropped packets.
		 * TODO: Look into handling this better. */
		return;
	}

	/* Do not increment if we got ZLB */
	if (dgram->msg_type != MESSAGE_ZLB) tunnel->Nr++;

	/* Update peer_Nr */
	if (SERIAL_GT(dgram->Nr, tunnel->peer_Nr)) tunnel->peer_Nr = dgram->Nr;

	/* Reset retransmissions stuff */
	tunnel->retransmissions = 0;
	tunnel->timeout = 1;

	/* Throw away ack'd packets in our xmit queue */
	tunnel_dequeue_acked_packets(tunnel);

	/* Let the specific tunnel handle it */
	tunnel_process_received_datagram(tunnel, dgram);

	/* Run the xmit queue -- window may have opened */
	tunnel_xmit_queued_messages(tunnel);

	/* Reschedule HELLO handler for 60 seconds in future */
	if (tunnel->state != TUNNEL_RECEIVED_STOP_CCN &&
		tunnel->state != TUNNEL_SENT_STOP_CCN &&
		tunnel->hello_scheduled)
	{
		tunnel_setup_hello(tunnel);
	}
	
	/* Destroy tunnel if required and if xmit queue empty */
	if (!tunnel->xmit_queue_head)
	{
		eloop_cancel_timeout(tunnel_handle_timeout, NULL, tunnel);
		if ((tunnel->state == TUNNEL_RECEIVED_STOP_CCN) ||
			(tunnel->state == TUNNEL_SENT_STOP_CCN))
		{
			/* Our stop-CCN has be ack'd, destroy NOW */
			modem_hungup();
			tunnel_free(tunnel);
		}
	}
}

/****************************************************************************
 * FUNCTION: tunnel_delete_session
 * ARGUMENTS:
 *   ses -- session to delete
 * RETURNS:
 *   Nothing
 * DESCRIPTION:
 *   Deletes session from tunnel's hash table and frees it.
 ***************************************************************************/
void l2tp_tunnel_delete_session(l2tp_session * ses, char const * reason)
{
	l2tp_tunnel * tunnel = ses->tunnel;

	l2tp_session_free(ses, reason);

	/* Tear down tunnel */
	tunnel_send_StopCCN(tunnel, RESULT_GENERAL_REQUEST, 0, "Last session has closed");
}

void l2tp_tunnel_close(l2tp_tunnel * tunnel)
{
	tunnel_send_StopCCN(tunnel, RESULT_GENERAL_REQUEST, 0, "Last session has closed");
	tunnel_free(tunnel);
}
