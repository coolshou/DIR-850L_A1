/* vi: set sw=4 ts=4: */

#ifndef __XMLD_HEADER_FILE__
#define __XMLD_HEADER_FILE__

struct xml_node
{
	struct xml_node * child;
	struct xml_node * next;

	int id;
	char * name;
	char * value;
	char * notify;
};

#define MAX_TAG_LEN			256
#define MAX_VAL_LEN			256
#define MAX_PATCH_MSG_LEN	256
#define MAX_MAP_SECTION		32


#define MAX_PATCH_BUFFER	1024
#define XMLDB_PIDFILE       "/var/run/xmldb_%s.pid"

#define XMLDB_NOTIFY_SOCK	"/var/run/xmldb_notify"

typedef struct patch_info PINFO;
struct patch_info
{
	unsigned long flags;
	struct xml_node * current;
	int outfd;	/* output fd */

	/* array */
	int max;

	/* if 'fh' is not NULL, read data from 'fh' */
	FILE * fh;

	/* if 'fh' is NULL, read data from the following buffer */
	char * buff;
	size_t size;
	long offset;
	size_t allocsize;
};


/* prototyps & global variables */
#define IS_WHITE(c)	(( (c)==' ' || (c)=='\t' || (c)=='\r' || (c)=='\n' ) ? 1 : 0 )

extern struct xml_node g_xml_root;
extern char * o_rootname;

/* patch buffer */
#if 0
extern char patch_buff[];
extern size_t pb_size;
extern long pb_offset;
#endif

long pb_tell(FILE * stream);
int pb_seek(FILE * stream, long offset, int whence);
int pb_getc(FILE * stream);

const char * xmldb_eatwhite(const char * string);
char * xmldb_reatwhite(char * string);

int		xmldb_read_xml(const char * file, unsigned long flags);
void	xmldb_dump_nodes(FILE * fh, struct xml_node * node, int depth, unsigned long flags);

struct xml_node *	xmldb_find_sibling(struct xml_node * start, const char * name, int id, int create);
struct xml_node *	xmldb_find_node(struct xml_node * root, const char * path, int create);
struct xml_node *	xmldb_set_value(struct xml_node * root, const char * path, const char * value);
struct xml_node *	xmldb_set_notify(struct xml_node * root, const char * path, const char * message);
const char *		xmldb_get_value(struct xml_node * root, const char * path);
int					xmldb_del_node(struct xml_node * root, const char * path);

int	xmldb_patch_message(int fd, struct xml_node * root, const char * msg, unsigned long flags);
int	xmldb_patch_file(int fd, const char * file, unsigned long flags);
int	xmldb_patch_buffer(int fd, unsigned short length, unsigned long flags);

#endif
