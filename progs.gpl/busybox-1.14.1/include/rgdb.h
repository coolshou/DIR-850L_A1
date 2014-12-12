/* vi: set sw=4 ts=4: */

#ifndef __RGDB_HEADER_FILE__
#define __RGDB_HEADER_FILE__

#define RGDB_DEFAULT_UNIXSOCK	"/var/run/xmldb_sock"

/* action type */
typedef unsigned short action_t;
#define RGDB_NONE		0	/* no action */
#define RGDB_GET		1	/* get value from node */
#define RGDB_SET		2	/* set value to node */
#define RGDB_DEL		3	/* delete node */
#define RGDB_RELOAD		4	/* reload database from file. */
#define RGDB_DUMP		5	/* dump database to file. */
#define RGDB_PATCH		6	/* patch template file. */
#define RGDB_PATCH_BUFF	7	/* patch buffer */
#define RGDB_SET_NOTIFY	8	/* set notify message */
#define RGDB_EPHP		10	/* embeded php */

/* command struct */
typedef struct _rgdb_ipc_t rgdb_ipc_t;
struct _rgdb_ipc_t
{
	action_t		action;
	unsigned short	length;
	unsigned long	flags;
	int				retcode;
} __attribute__ ((packed));

/* RGDB flags */
#define RGDB_ESCAPE_JS	0x00000001

/* skip node 'proc' when dump, and keep 'proc' when load xml. */
#define RGDB_SKIP_PROC	0x00010000
extern int _cli_rgdb_get_(char *buff, int size, const char *format, ...);
extern int _cli_rgdb_set_(const char *node, const char *value);
extern int _cli_rgdb_del_(const char *node);
#define RGDBGET(x, y, args...) _cli_rgdb_get_(x, y, ##args)/*ftp_tftp_FW_CG_20100109 log_luo*/
#define RGDBSET(x, y) _cli_rgdb_set_(x, y)
#define RGDBDEL(x) _cli_rgdb_del_(x)
#endif
