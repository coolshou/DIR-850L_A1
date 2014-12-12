/* vi: set sw=4 ts=4: */
/*******************************************************************
 *
 * l2tp.h
 *
 * Header file for L2TP definitions.
 *
 *******************************************************************/

#ifndef __L2TP_HEADER_FILE__
#define __L2TP_HEADER_FILE__

#include <stdint.h>
#include <netinet/in.h>

#define MD5LEN	16	/* Length of MD5 hash */

/* Maximum size of L2TP datagram we accept */
#define MAX_PACKET_LEN		4096
#define MAX_SECRET_LEN		96
#define MAX_HOSTNAME		128
#define MAX_RETRANSMISSIONS	5
#define EXTRA_HEADER_ROOM	32

/* Bit definitions */
#define TYPE_BIT			0x80
#define LENGTH_BIT			0x40
#define SEQUENCE_BIT		0x08
#define OFFSET_BIT			0x02
#define PRIORITY_BIT		0x01
#define RESERVED_BITS		0x34
#define VERSION_MASK		0x0f
#define VERSION_RESERVED	0xf0

#define AVP_MANDATORY_BIT	0x80
#define AVP_HIDDEN_BIT		0x40
#define AVP_RESERVED_BITS	0x3c

#define MANDATORY			1
#define NOT_MANDATORY		0
#define HIDDEN				1
#define NOT_HIDDEN			0
#define VENDOR_IETF			0

#define AVP_MESSAGE_TYPE				0
#define AVP_RESULT_CODE					1
#define AVP_PROTOCOL_VERSION			2
#define AVP_FRAMING_CAPABILITIES		3
#define AVP_BEARER_CAPABILITIES			4
#define AVP_TIE_BREAKER					5
#define AVP_FIRMWARE_REVISION			6
#define AVP_HOST_NAME					7
#define AVP_VENDOR_NAME					8
#define AVP_ASSIGNED_TUNNEL_ID			9
#define AVP_RECEIVE_WINDOW_SIZE			10
#define AVP_CHALLENGE					11
#define AVP_Q931_CAUSE_CODE				12
#define AVP_CHALLENGE_RESPONSE			13
#define AVP_ASSIGNED_SESSION_ID			14
#define AVP_CALL_SERIAL_NUMBER			15
#define AVP_MINIMUM_BPS					16
#define AVP_MAXIMUM_BPS					17
#define	AVP_BEARER_TYPE					18
#define AVP_FRAMING_TYPE				19
#define AVP_CALLED_NUMBER				21
#define AVP_CALLING_NUMBER				22
#define AVP_SUB_ADDRESS					23
#define AVP_TX_CONNECT_SPEED			24
#define AVP_PHYSICAL_CHANNEL_ID			25
#define AVP_INITIAL_RECEIVED_CONFREQ	26
#define AVP_LAST_SENT_CONFREQ			27
#define AVP_LAST_RECEIVED_CONFREQ		28
#define AVP_PROXY_AUTHEN_TYPE			29
#define AVP_PROXY_AUTHEN_NAME			30
#define AVP_PROXY_AUTHEN_CHALLENGE		31
#define AVP_PROXY_AUTHEN_ID				32
#define AVP_PROXY_AUTHEN_RESPONSE		33
#define AVP_CALL_ERRORS					34
#define AVP_ACCM						35
#define AVP_RANDOM_VECTOR				36
#define AVP_PRIVATE_GROUP_ID			37
#define AVP_RX_CONNECT_SPEED			38
#define AVP_SEQUENCING_REQUIRED			39

#define HIGHEST_AVP						39

#define MESSAGE_SCCRQ					1
#define MESSAGE_SCCRP					2
#define MESSAGE_SCCCN					3
#define MESSAGE_StopCCN					4
#define MESSAGE_HELLO					6

#define MESSAGE_OCRQ					7
#define MESSAGE_OCRP					8
#define MESSAGE_OCCN					9

#define MESSAGE_ICRQ					10
#define MESSAGE_ICRP					11
#define MESSAGE_ICCN					12

#define MESSAGE_CDN						14
#define MESSAGE_WEN						15
#define MESSAGE_SLI						16

/* a fake type for our own consumption */
#define MESSAGE_ZLB						32767

/* Result and error codes  */
#define RESULT_GENERAL_REQUEST			1
#define RESULT_GENERAL_ERROR			2
#define RESULT_CHANNEL_EXISTS			3
#define RESULT_NOAUTH					4
#define RESULT_UNSUPPORTED_VERSION		5
#define RESULT_SHUTTING_DOWN			6
#define RESULT_FSM_ERROR				7

#define ERROR_OK						0
#define ERROR_NO_CONTROL_CONNECTION		1
#define ERROR_BAD_LENGTH				2
#define ERROR_BAD_VALUE					3
#define ERROR_OUT_OF_RESOURCES			4
#define ERROR_INVALID_SESSION_ID		5
#define ERROR_VENDOR_SPECIFIC			6
#define ERROR_TRY_ANOTHER				7
#define ERROR_UNKNOWN_AVP_WITH_M_BIT	8

/* Tunnel states */
enum
{
	TUNNEL_IDLE,
	TUNNEL_WAIT_CTL_REPLY,
	TUNNEL_WAIT_CTL_CONN,
	TUNNEL_ESTABLISHED,
	TUNNEL_RECEIVED_STOP_CCN,
	TUNNEL_SENT_STOP_CCN
};

/* Session states */
enum
{
	SESSION_IDLE,
	SESSION_WAIT_TUNNEL,
	SESSION_WAIT_REPLY,
	SESSION_WAIT_CONNECT,
	SESSION_ESTABLISHED
};

/* an L2TP datagram */
typedef struct l2tp_dgram_t
{
	uint16_t msg_type;					/* Message type */
	uint8_t bits;						/* Options bits */
	uint8_t version;					/* Version */
	uint16_t length;					/* Length (opt) */
	uint16_t tid;						/* Tunnel ID */
	uint16_t sid;						/* Session ID */
	uint16_t Ns;						/* Ns (opt) */
	uint16_t Nr;						/* Nr (opt) */
	uint16_t off_size;					/* Offset size (opt) */
	unsigned char data[MAX_PACKET_LEN];	/* Data */
	size_t last_random;					/* Offset of last random vector AVP */
	size_t payload_len;					/* Payload len (not including L2TP header) */
	size_t cursor;						/* Cursor for adding/stripping AVP's */
	size_t alloc_len;					/* Length allocated for data */
	struct l2tp_dgram_t * next;			/* Link to next packet in xmit queue */
}
l2tp_dgram;

/* An L2TP peer */
typedef struct l2tp_peer_t
{
	struct sockaddr_in addr;			/* Peer's address */
	char secret[MAX_SECRET_LEN];		/* Secret for this peer */
	size_t secret_len;					/* Length of secret */
	//struct l2tp_call_ops_t * lac_ops;	/* Call ops if we act as LAC */
	//struct l2tp_call_ops_t * lns_ops;	/* Call ops if we act as LNS */
	int hide_avps;						/* If true, hide AVPs to this peer */
	int validate_peer_ip;				/* If true, do not accept datagrams except
										   from initial peer IP address */
}
l2tp_peer;

/* An L2TP tunnel */
typedef struct l2tp_tunnel_t
{
	uint16_t my_id;						/* My tunnel ID */
	uint16_t assigned_id;				/* ID assigned by peer */
	l2tp_peer * peer;					/* The L2TP peer */
	struct sockaddr_in peer_addr;		/* Peer's address */
	uint16_t Ns;						/* Sequence of next packet to queue */
	uint16_t Ns_on_wire;				/* Sequence of next packet to be sent on wire */
	uint16_t Nr;						/* Expected sequence of next received packet */
	uint16_t peer_Nr;					/* Last packet ack'd by peer */
	int ssthresh;						/* Slow-start threshold */
	int cwnd;							/* Congestion window */
	int cwnd_counter;					/* Counter for incrementing cwnd in congestion-avoidance phase */
	int timeout;						/* Retransmission timeout (seconds) */
	int retransmissions;				/* Number of retransmissions */
	int rws;							/* Our receive window size */
	int peer_rws;						/* Peer receive window size */
	l2tp_dgram * xmit_queue_head;		/* Head of control transmit queue */
	l2tp_dgram * xmit_queue_tail;		/* Tail of control transmit queue */
	l2tp_dgram * xmit_new_dgrams;		/* dgrams which have not been transmitted */
	char peer_hostname[MAX_HOSTNAME];	/* Peer's host name */
	unsigned char response[MD5LEN];		/* Our response to challenge */
	unsigned char expected_response[MD5LEN]; /* Expected resp. to challenge */
	int state;							/* Tunnel state */
	int pppox_fd;                       /* PPPOX tunnel fd */

	unsigned int ack_scheduled:1;		/* set at scheduling ack, reset when fired. */
	unsigned int hello_scheduled:1;		/* set at scheduling hello, reset when fired. */
}
l2tp_tunnel;

typedef struct l2tp_session_t l2tp_session;
typedef void (*frame_from_tunnel)(l2tp_session * ses, unsigned char * buf, size_t len);
typedef void (*frame_to_tunnel)(l2tp_session * ses);

/* A session within a tunnel */
struct l2tp_session_t
{
	l2tp_tunnel * tunnel;				/* Tunnel we belong to */
	uint16_t my_id;						/* My ID */
	uint16_t assigned_id;				/* Assigned ID */
	int state;							/* Session state */

	/* Some flags */
	unsigned int snooping:1;			/* Are we snooping in on LCP ? */
	unsigned int got_send_accm:1;		/* Do we have send_accm ? */
	unsigned int got_recv_accm:1;		/* Do we have recv_accm ? */
	unsigned int we_are_lac:1;			/* Are we a LAC ? */
	unsigned int sequencing_required:1;	/* Sequencing required ? */
	unsigned int sent_sli:1;			/* Did we send SLI yet ? */

	uint32_t send_accm;					/* Negotiated send accm */
	uint32_t recv_accm;					/* Negotiated receive accm */
	uint16_t Nr;						/* Data sequence number */
	uint16_t Ns;						/* Data sequence number */

	int pty_fd;							/* master pty fd */
	frame_from_tunnel handle_frame_from_tunnel;
	frame_to_tunnel handle_frame_to_tunnel;

	char calling_number[MAX_HOSTNAME];	/* Calling number */
	void * private;						/* Private data for call-op's use */
};


/* sync_pppd.c */
void l2tp_sync_ppp_handle_frame_from_tunnel(l2tp_session * ses, unsigned char * buf, size_t size);
void l2tp_sync_ppp_handle_frame_to_tunnel(l2tp_session * ses);

/* async_pppd.c */
void l2tp_async_ppp_handle_frame_from_tunnel(l2tp_session * ses, unsigned char * buf, size_t size);
void l2tp_async_ppp_handle_frame_to_tunnel(l2tp_session * ses);

/* l2tp_options.c */
extern char l2tp_peer_addr[];
extern char l2tp_secret[];
extern int l2tp_sync;
extern int l2tp_port;
extern int l2tp_lns;
extern int l2tp_hide_avps;
extern int l2tp_kernel_mode;
extern int pppol2tp_fd;

/* l2tp.c */
extern int Sock;
extern int pppol2tp_udp_fd;
extern char Hostname[];
void l2tp_generate_response(uint16_t msg_type, char const * secret, unsigned char const * challenge, size_t chal_len, unsigned char buf[16]);
void pppol2tp_init_network();
int pppol2tp_tunnel(l2tp_tunnel *t);

/* debug.c */
char const * l2tp_debug_avp_type_to_str(uint16_t type);
char const * l2tp_debug_message_type_to_str(uint16_t type);
char const * l2tp_debug_tunnel_to_str(l2tp_tunnel * tunnel);
char const * l2tp_debug_session_to_str(l2tp_session * session);
char const * l2tp_debug_describe_dgram(l2tp_dgram const * dgram);

/* utils.c */
typedef void (*l2tp_shutdown_func)(void *);
int l2tp_register_shutdown_handler(l2tp_shutdown_func f, void * data);
void l2tp_cleanup(void);
char const * l2tp_get_errmsg(void);
void l2tp_set_errmsg(char const * fmt, ...);
void l2tp_die(void);
void l2tp_random_init(void);
void l2tp_random_fill(void * ptr, size_t size);
#define L2TP_RANDOM_FILL(x) l2tp_random_fill(&(x), sizeof(x))

/* dgram.c */
int l2tp_dgram_validate_avp(uint16_t vendor, uint16_t type, uint16_t len, int mandatory);
l2tp_dgram * l2tp_dgram_new(size_t len);
l2tp_dgram * l2tp_dgram_new_control(uint16_t msg_type, uint16_t tid, uint16_t sid);
void l2tp_dgram_free(l2tp_dgram * dgram);
l2tp_dgram * l2tp_dgram_take_from_wire(struct sockaddr_in * from);
int l2tp_dgram_send_to_wire(l2tp_dgram const * dgram, struct sockaddr_in const * to);
int l2tp_dgram_add_avp(l2tp_dgram * dgram, l2tp_tunnel * tunnel, int mandatory, uint16_t len,
						uint16_t vendor, uint16_t type, void * val);
unsigned char * l2tp_dgram_pull_avp(l2tp_dgram * dgram, l2tp_tunnel * tunnel,
						int * mandatory, int * hidden, uint16_t * len, uint16_t * vendor, uint16_t * type, int * err);
unsigned char * l2tp_dgram_search_avp(l2tp_dgram * dgram, l2tp_tunnel * tunnel,
						int * mandatory, int * hidden, uint16_t * len, uint16_t vendor, uint16_t type);
int l2tp_dgram_send_ppp_frame(l2tp_session *ses, unsigned char const *buf, int len);

/* peer.c */
extern l2tp_peer the_peer;
int l2tp_peer_init(void);

/* tunnel.c */
extern l2tp_tunnel * the_tunnel;
void l2tp_tunnel_send_ZLB(l2tp_tunnel * tunnel);
void l2tp_tunnel_xmit_control_message(l2tp_tunnel * tunnel, l2tp_dgram * dgram);
void l2tp_tunnel_cancel_ack_handler(l2tp_tunnel * tunnel);
l2tp_tunnel * l2tp_tunnel_establish(l2tp_peer * peer);
void l2tp_tunnel_delete_session(l2tp_session * ses, char const * reason);
void l2tp_tunnel_handle_received_control_datagram(l2tp_dgram * dgram, struct sockaddr_in * from);
void l2tp_tunnel_close(l2tp_tunnel * tunnel);

/* session.c */
extern l2tp_session * the_session;
l2tp_session * l2tp_session_call_lns(l2tp_peer * peer, char const * calling_number, void * private);
void l2tp_session_free(l2tp_session * ses, char const * reason);
void l2tp_session_notify_tunnel_open(l2tp_session * ses);
void l2tp_session_lcp_snoop(l2tp_session * ses, unsigned char const * buf, int len, int incoming);
void l2tp_session_handle_ICRP(l2tp_session * ses, l2tp_dgram * dgram);
void l2tp_session_handle_ICCN(l2tp_session * ses, l2tp_dgram * dgram);
void l2tp_session_handle_CDN(l2tp_session * ses, l2tp_dgram * dgram);

#endif /* endof #ifndef __L2TP_HEADER_FILE__ */
