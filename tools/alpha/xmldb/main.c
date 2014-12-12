/* vi: set sw=4 ts=4: */
/*
 * xmldb: A XML database.
 * 
 *	xmldb main routine.
 *
 *	Created by David Hsieh, Alphanetworks, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#include "rgdb.h"
#include "xmldb.h"
#include "ephp.h"
#include "dtrace.h"
#include "dirutil.h"

#define PROGNAME			"xmldb"
#define VERSION				"0.2.0"

typedef int (*read_handler)(int index);

/****************************************************************************/

static int o_verbose=0;				/* verbose mode */
static char * o_unixsock = NULL;	/* unixsock name */
char * o_rootname = NULL;	        /* name of the XML root node */
static char pidfile[256]={0};       /* name of the PID file */
static char * o_notifysock = NULL;	/* notify socket */
int o_tlogs = 0;					/* use tlog to translate log message */

static void cleanup_exit(int exit_code)
{
	if (o_unixsock) free(o_unixsock);
	if (o_rootname) free(o_rootname);
	if (o_notifysock) free(o_notifysock);
	exit(exit_code);
}

static void show_usage(int exit_code)
{
	printf("%s version %s\n", PROGNAME, VERSION);
	printf("Usage: %s [OPTIOINS]\n", PROGNAME);
	printf("  -h                    show this help message.\n");
    printf("  -p {file name}        specify the pid file name of xmldb, default is xmldb_{unixsocket}.pid.\n");
	printf("  -v                    verbose mode.\n");
	printf("  -s {unix socket}      specify unixsocket to bind, default is %s .\n", RGDB_DEFAULT_UNIXSOCK);
	printf("  -n {root name}        specify the name of XML root node.\n");
	printf("  -N {notify socket}	specify unixsocket to send notify, default is %s .\n", XMLDB_NOTIFY_SOCK);
	printf("  -t                    use tlogs to translate log message.\n");

	cleanup_exit(exit_code);
}

void verbose(const char * format, ...)
{
	va_list marker;
	if (o_verbose)
	{
		va_start(marker, format);
		vfprintf(stdout, format, marker);
		va_end(marker);
	}
}

extern int optind;
extern char * optarg;

static void parse_args(int argc, char * argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "hvts:p:n:N:")) > 0)
	{
		switch (opt)
		{
		case 'h':
			show_usage(0);
			break;
		case 'v':
			o_verbose++;
			break;
		case 't':
			o_tlogs++;
			break;
		case 's':
			if (o_unixsock) free(o_unixsock);
			o_unixsock = strdup(optarg);
			break;
        case 'p':
            strcpy(pidfile,optarg);
            break;
		case 'n':
			if (o_rootname) free(o_rootname);
			o_rootname = strdup(optarg);
			break;
		case 'N':
			if (o_notifysock) free(o_notifysock);
			o_notifysock = strdup(optarg);
			break;
		default:
			show_usage(-1);
			break;
		}
	}
	if (!o_rootname) show_usage(-1);
	if (!o_unixsock) o_unixsock = strdup(RGDB_DEFAULT_UNIXSOCK);
	if (!o_notifysock) o_notifysock = strdup(XMLDB_NOTIFY_SOCK);
	verbose("%s version %s\n", PROGNAME, VERSION);
	verbose("Root Name    : %s\n", o_rootname);
	verbose("Unix Socket  : %s\n", o_unixsock);
	verbose("Notify Socket: %s\n", o_notifysock);
}

/***************************************************************************/
static int open_unixsock(void)
{
	struct sockaddr_un where;
	struct stat st;
	char * dir;
	int s;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "socket: %s", strerror(errno));
		return s;
	}

	/* Check if unixsocket is already created. */
	if (stat(o_unixsock, &st) >= 0)
	{
		fprintf(stderr, "socket: %s for %s is already created!\n", o_unixsock, PROGNAME);
		close(s);
		return -1;
	}

	/* Make sure path is valid. */
	dir = dirname(o_unixsock);
	if (!make_valid_path(dir, 0770)) fprintf(stderr, "Could not make path to %s: %s\n", o_unixsock, strerror(errno));
	free(dir);

	where.sun_family = AF_UNIX;
	snprintf(where.sun_path, sizeof(where.sun_path), "%s", o_unixsock);

	if (bind(s, (struct sockaddr *)&where, sizeof(where)) < 0)
	{
		fprintf(stderr, "bind: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	chmod(where.sun_path, 0777);
	listen(s, 127);
	return s;
}

static void close_unixsock(int fd)
{
	close(fd);
	unlink(o_unixsock);
	verbose("Unix Socket (%s) closed\n", o_unixsock);
}

/***************************************************************************/
#define MAX_CLIENT_LIST	128
struct xmldb_client
{
	int			fd;
	rgdb_ipc_t	ipc;
};

static struct xmldb_client clients[MAX_CLIENT_LIST];

static void init_clients(void)
{
	int i;
	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		clients[i].fd = -1;
		clients[i].ipc.action = RGDB_NONE;
		clients[i].ipc.length = 0;
	}
}
static int add_client(int s)
{
	int i;
	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		if (clients[i].fd < 0)
		{
			verbose("add %d in clients[%d]\n", s, i);
			clients[i].fd = s;
			return 0;
		}
	}
	verbose("!!! client list is full, help !!!\n");
	return -1;
}
static int remove_client(int s)
{
	int i;
	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		if (clients[i].fd == s)
		{
			clients[i].fd = -1;
			clients[i].ipc.action = RGDB_NONE;
			clients[i].ipc.length = 0;
			return 0;
		}
	}
	return -1;
}
static int set_fd(fd_set * set)
{
	int i, max_fd = 0;

	FD_ZERO(set);
	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		if (clients[i].fd > 2)
		{
			verbose("set_fd(%d)\n", clients[i].fd);
			FD_SET(clients[i].fd, set);
			if (max_fd < clients[i].fd) max_fd = clients[i].fd;
		}
	}
	return max_fd;
}
static int dispatch_fd(fd_set * set, read_handler handler)
{
	int i, ret;
	int fd;

	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		fd = clients[i].fd;
		if (fd > 2 && FD_ISSET(fd, set))
		{
			verbose("clients[%d]: fd(%d) is set!\n", i, fd);
			if (handler) ret = handler(i);
			else ret = 0;
			FD_CLR(fd, set);
			if (ret <= 0)
			{
				verbose("ret = %d, closing fd[%d]: %d\n", ret, i, fd);
				close(fd);
				remove_client(fd);
			}
		}
	}
	return 0;
}
static void close_clients(void)
{
	int i;
	
	for (i=0; i<MAX_CLIENT_LIST; i++)
	{
		if (clients[i].fd > 0)
		{
			verbose("closing fd[%d]: %d\n", i, clients[i].fd);
			close(clients[i].fd);
			clients[i].fd = -1;
			clients[i].ipc.action = RGDB_NONE;
			clients[i].ipc.length = 0;
		}
	}
}

/***************************************************************************/

static void str_tolower(char * string)
{
	while (*string)
	{
		*string = (char)tolower(*string);
		string++;
	}
}

ssize_t do_get(int index)
{
	ssize_t size;
	char * buff;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_get(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		str_tolower(buff);
		verbose("do_get[%s], %d\n", buff, size);
		xmldb_patch_message(clients[index].fd, NULL, buff, clients[index].ipc.flags);
	}
	else
	{
		verbose("do_get(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send value */
	free(buff);
	return size;
}
ssize_t do_patch(int index)
{
	ssize_t size;
	char * buff;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_path(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		verbose("do_patch[%s], %d\n", buff, size);
		xmldb_patch_file(clients[index].fd, buff, clients[index].ipc.flags);
	}
	else
	{
		verbose("do_patch(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send value */
	free(buff);
	return size;
}
ssize_t do_ephp(int index)
{
	ssize_t size;
	char * buff;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_ephp(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);
	if (size > 0)
	{
		verbose("do_ephp[%s], %d\n", buff, size);
		xmldb_ephp(clients[index].fd, buff, clients[index].ipc.flags);
	}
	else
	{
		verbose("do_ephp(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send value */
	free(buff);
	return size;
}
ssize_t do_patch_buffer(int index)
{
	ssize_t size;
	unsigned char dummy;

	/* read parameter */
	if (clients[index].ipc.length < MAX_PATCH_BUFFER)
	{
		size = xmldb_patch_buffer(clients[index].fd, clients[index].ipc.length, clients[index].ipc.flags);
	}
	else
	{
		dummy = 0;
		write(clients[index].fd, &dummy, sizeof(dummy));
		size = 0;
	}

	/* works done, send value */
	return size;
}
ssize_t do_set(int index)
{
	ssize_t size;
	char * buff;
	char * value;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_set(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		verbose("do_set[%s], %d\n", buff, size);
		value = strchr((const char *)buff, ' ');
		if (value)
		{
			*value++='\0';
			verbose("set %s = %s\n", buff, value);
			str_tolower(buff);
			xmldb_set_value(NULL, buff, value);
		}
		clients[index].ipc.action = RGDB_NONE;
		clients[index].ipc.length = 0;
		clients[index].ipc.retcode = 0;
		write(clients[index].fd, &clients[index].ipc, sizeof(clients[index].ipc));
	}
	else
	{
		verbose("do_set(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send ack */
	free(buff);
	return size;
}
ssize_t do_set_notify(int index)
{
	ssize_t size;
	char * buff;
	char * value;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_set_notify(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		verbose("do_set_notify[%s], %d\n", buff, size);
		value = strchr((const char *)buff, ' ');
		if (value)
		{
			*value++='\0';
			verbose("set notify %s : %s\n", buff, value);
			str_tolower(buff);
			xmldb_set_notify(NULL, buff, value);
		}
		clients[index].ipc.action = RGDB_SET_NOTIFY;
		clients[index].ipc.length = 0;
		clients[index].ipc.retcode = 0;
		write(clients[index].fd, &clients[index].ipc, sizeof(clients[index].ipc));
	}
	else
	{
		verbose("do_set_notify(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send ack */
	free(buff);
	return size;
}
ssize_t do_del(int index)
{
	ssize_t size;
	char * buff;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_del(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		/* do delete node */
		str_tolower(buff);
		verbose("do_del[%s], %d\n", buff, size);
		xmldb_del_node(NULL, buff);
		clients[index].ipc.action = RGDB_NONE;
		clients[index].ipc.length = 0;
		clients[index].ipc.retcode = 0;
		write(clients[index].fd, &clients[index].ipc, sizeof(clients[index].ipc));
	}
	else
	{
		verbose("do_del(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send ack */
	free(buff);
	return size;
}
ssize_t do_reload(int index)
{
	ssize_t size;
	char * buff;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_reload(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		/* do reload */
		verbose("do_reload[%s], %d\n", buff, size);
		clients[index].ipc.action = RGDB_NONE;
		clients[index].ipc.length = 0;
		clients[index].ipc.retcode = xmldb_read_xml(buff, clients[index].ipc.flags);
		write(clients[index].fd, &clients[index].ipc, sizeof(clients[index].ipc));
	}
	else
	{
		verbose("do_reload(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send ack */
	free(buff);
	return size;
}
ssize_t do_dump(int index)
{
	ssize_t size;
	char * buff;
	FILE * fh;

	/* prepare buffer for parameter */
	buff = (char *)malloc(clients[index].ipc.length);
	if (!buff)
	{
		verbose("do_dump(): internal error, malloc return NULL !!!!\n");
		return 0;
	}

	/* read parameter */
	size = read(clients[index].fd, buff, clients[index].ipc.length);

	if (size > 0)
	{
		/* do dump */
		verbose("do_dump[%s], %d\n", buff, size);
		if (strcmp(buff, "stdout")==0) fh = stdout;
		else fh = fopen(buff, "w");
		if (fh)
		{
			xmldb_dump_nodes(fh, &g_xml_root, 0, clients[index].ipc.flags);
			if (fh != stdout) fclose(fh);
		}
		clients[index].ipc.action = RGDB_NONE;
		clients[index].ipc.length = 0;
		clients[index].ipc.retcode = 0;
		write(clients[index].fd, &clients[index].ipc, sizeof(clients[index].ipc));
	}
	else
	{
		verbose("do_dump(): internal error, read() return %d !!!!\n", size);
	}

	/* works done, send ack */
	free(buff);
	return size;
}

/***************************************************************************/

static int terminate = 0;

static void sighandler(int sig)
{
	verbose("sighandler (%d)\n", sig);
	terminate = 1;
}

static int rhandler(int index)
{
	ssize_t size;
	int fd = clients[index].fd;

	verbose("rhandler(%d): action = %d, length = %d\n", index, clients[index].ipc.action, clients[index].ipc.length);

	switch (clients[index].ipc.action)
	{
	case RGDB_NONE:
		size = read(fd, &clients[index].ipc, sizeof(clients[index].ipc));
		if (size) verbose("action = %d, length = %d\n", clients[index].ipc.action, clients[index].ipc.length);
		break;

	case RGDB_GET:
		size = do_get(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_SET:
		size = do_set(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_SET_NOTIFY:
		size = do_set_notify(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_DEL:
		size = do_del(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_RELOAD:
		size = do_reload(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_DUMP:
		size = do_dump(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_PATCH:
		size = do_patch(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_EPHP:
		size = do_ephp(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	case RGDB_PATCH_BUFF:
		size = do_patch_buffer(index);
		clients[index].ipc.action = RGDB_NONE;
		break;
	default:
		verbose("client %d, unknown action (%d), ignore it !!!!!\n", index, clients[index].ipc.action);
		size = 0;
		clients[index].ipc.action = RGDB_NONE;
		break;
	}
	return size;
}

/***************************************************************************/

void send_notify(const char * message)
{
	int ret, sock, flags;
	static struct sockaddr_un sunix;
	static int done=0;

	d_dbg("send_notify(%s) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", message);
	
	if (!done)
	{
		memset(&sunix, 0, sizeof(struct sockaddr_un));
		sunix.sun_family = AF_UNIX;
		strncpy(sunix.sun_path, o_notifysock, sizeof(sunix.sun_path));
		done++;
	}

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		d_error("send_notify(): unable to open socket!!\n");
		return;
	}
	flags = fcntl(sock, F_GETFL);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	ret = sendto(sock, (const void *)message, strlen(message)+1, 0,
			(struct sockaddr *)&sunix, sizeof(sunix.sun_family) + strlen(sunix.sun_path));
	close(sock);
	return;
}

/***************************************************************************/

int main(int argc, char * argv[])
{
	int usock, s;
	fd_set read_set;
	int max_fd;
	struct sockaddr_un from;
	int len;
    char *pStr=NULL;
    FILE *pFile;

	/* parse arguments */
	parse_args(argc, argv);

    /* Record PID */
    if((pStr=strrchr(o_unixsock, '/')))
      pStr++;
    else
      pStr=o_unixsock;
    if(0==pidfile[0])
      sprintf(pidfile, XMLDB_PIDFILE, pStr);
    if((pFile=fopen(pidfile, "w")))
      {
       fprintf(pFile, "%d", getpid());
       fclose(pFile);
      }

	g_xml_root.name = o_rootname;

	/* open and listen on unix socket */
	usock = open_unixsock();
	if (usock < 0) cleanup_exit(-1);

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGKILL, sighandler);
	init_clients();
	
	do
	{
		max_fd = set_fd(&read_set);
		FD_SET(usock, &read_set);
		if (max_fd < usock) max_fd = usock;
		verbose("select waiting ...\n");
		if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0)
		{
			verbose("select: %s\n", strerror(errno));
			continue;
		}

		if (FD_ISSET(usock, &read_set))
		{
			len = sizeof(from);
			if ((s = accept(usock, (struct sockaddr *)&from, &len)) < 0)
			{
				fprintf(stderr, "socket not accepted: %s\n", strerror(errno));
				break;
			}
			verbose("new connection acceptd (fd=%d)\n", s);
			add_client(s);
		}
		dispatch_fd(&read_set, rhandler);

	} while (!terminate);

	close_clients();
	close_unixsock(usock);
	
	cleanup_exit(0);
	return 0;
}
