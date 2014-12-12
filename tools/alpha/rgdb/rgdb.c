/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "lrgbin.h"
#include "rgdb.h"

extern int optind;
extern char * optarg;

/****************************************************************************/

static int o_verbose = 0;			/* verbose mode */
static int o_ignore = 0;            /* ignore external function */
static int o_skip_proc = 0;			/* skip node 'proc'. */
static char * o_node_path = NULL;	/* node path */
static char * o_node_value = NULL;	/* node value (used for set only) */
static char * o_unixsock = NULL;	/* unix socket to communicate with xmldb. */
static char * o_reloadfile = NULL;	/* XML file to reload */
static char * o_dumpfile = NULL;	/* XML file the data base to dump to. */
static char * o_tempfile = NULL;	/* template file */
static char * o_ephpfile = NULL;	/* ephp file */
static char * o_escape = NULL;
static char * o_patchmsg = NULL;	/* template message */
static action_t o_action = RGDB_NONE;

static void verbose(const char * format, ...)
{
	va_list marker;
	if (o_verbose)
	{
		va_start(marker, format);
		vfprintf(stdout, format, marker);
		va_end(marker);
	}
}

static void cleanup_exit(int exit_code)
{
	if (o_node_path)	free(o_node_path);
	if (o_node_value)	free(o_node_value);
	if (o_unixsock)		free(o_unixsock);
	if (o_reloadfile)	free(o_reloadfile);
	if (o_dumpfile)		free(o_dumpfile);
	if (o_tempfile)		free(o_tempfile);
	if (o_escape)		free(o_escape);
	if (o_patchmsg)		free(o_patchmsg);
	if (o_ephpfile)		free(o_ephpfile);
	verbose("rgdb exit with code %d\n", exit_code);
	exit(exit_code);
}

static void show_usage(int exit_code)
{
	printf("Usage: rgdb [OPTIONS]\n");
	printf("  -h                     show this help message.\n");
	printf("  -v                     verbose mode.\n");
	printf("  -i                     ignore external function(like runtime).\n");
	printf("  -a                     skip node 'proc'.\n");
	printf("  -A {ephp file}         embeded php parse.\n");
	printf("  -g {node path}         get value from {node path}.\n");
	printf("  -s {node path} {value} set {value} in {node path}.\n");
	printf("  -n {node path} {msg}   set notify message in {node path}.\n");
	printf("  -d {node path}         delete {node path}.\n");
	printf("  -p {template file}     patch the specified template file.\n");
	printf("  -P {template message}  patch the specified template message.\n");
	printf("  -l {XML file}          reload XML file to database.\n");
	printf("  -e {type}              insert escape character in output.\n");
	printf("  -D {XML file}          dump database to XML file.\n");
	printf("  -S {unix socket}       specify unix socket name, default is %s.\n\n", RGDB_DEFAULT_UNIXSOCK);
	printf(" For type of escape character.\n");
	printf("  -e js                  javascript.\n");
	printf("\n");
	cleanup_exit(exit_code);
}

static void parse_args(int argc, char * argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "hviag:s:d:p:P:l:e:D:S:A:n:")) > 0)
	{
		switch (opt)
		{
		case 'h':
			show_usage(0);
			break;
		case 'v':
			o_verbose++;
			break;
		case 'i':
            o_ignore++;
			break;
		case 'a':
			o_skip_proc++;
			break;
		case 'e':
			if (o_escape) free(o_escape);
			o_escape = strdup(optarg);
			if (o_escape == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			break;
		case 'g':
			if (o_node_path) free(o_node_path);
			o_node_path = strdup(lrgbin_eatwhite(optarg));
			if (o_node_path == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_GET;
			break;
		case 'n':
		case 's':
			if (o_node_path) free(o_node_path);
			o_node_path = strdup(lrgbin_eatwhite(optarg));
			if (o_node_path == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			if (argc <= optind)
			{
				verbose("no value assigned for set action!\n");
				show_usage(-1);
			}
			if (o_node_value) free(o_node_value);
			o_node_value = strdup(lrgbin_eatwhite(argv[optind]));
			if (o_node_value==NULL)
			{
				verbose("memory allocateion fail!\n");
				cleanup_exit(-1);
			}
			if (opt == 'n') o_action = RGDB_SET_NOTIFY;
			else o_action = RGDB_SET;
			break;
		case 'd':
			if (o_node_path) free(o_node_path);
			o_node_path = strdup(lrgbin_eatwhite(optarg));
			if (o_node_path == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_DEL;
			break;
		case 'p':
			if (o_tempfile) free(o_tempfile);
			o_tempfile = strdup(lrgbin_eatwhite(optarg));
			if (o_tempfile == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_PATCH;
			break;
		case 'P':
			if (o_patchmsg) free(o_patchmsg);
			o_patchmsg = strdup(optarg);
			if (o_patchmsg == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_PATCH_BUFF;
			break;
		case 'A':
			if (o_ephpfile) free(o_ephpfile);
			o_ephpfile = strdup(optarg);
			if (o_ephpfile == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_EPHP;
			break;
		case 'l':
			if (o_reloadfile) free(o_reloadfile);
			o_reloadfile = strdup(optarg);
			if (o_reloadfile == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_RELOAD;
			break;
		case 'D':
			if (o_dumpfile) free(o_dumpfile);
			o_dumpfile = strdup(optarg);
			if (o_dumpfile == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			o_action = RGDB_DUMP;
			break;
		case 'S':
			if (o_unixsock) free(o_unixsock);
			o_unixsock = strdup(optarg);
			if (o_unixsock == NULL)
			{
				verbose("memory allocation fail!\n");
				cleanup_exit(-1);
			}
			break;
		default:
			show_usage(-1);
			break;
		}
	}
	if (o_action == RGDB_NONE)
	{
		verbose("no action specified!\n");
		cleanup_exit(0);
	}
	if (!o_unixsock) o_unixsock = strdup(RGDB_DEFAULT_UNIXSOCK);
	verbose("Unix socket: %s\n", o_unixsock);
}

/****************************************************************************/
/* call external application for runtime nodes. */

static void runtime(const char * node)
{
	int i;
	char cmd[64];

	for (i=0; node[i]!='/'; i++) cmd[i] = node[i];
	cmd[i] = '\0';
	switch (o_action)
	{
	case RGDB_GET:
		verbose("%s -g %s\n", cmd, &node[i+1]);
		lrgbin_system("%s -g %s", cmd, &node[i+1]);
		break;
	case RGDB_SET:
		verbose("%s -s %s \"%s\"\n", cmd, &node[i+1], o_node_value);
		lrgbin_system("%s -s %s \"%s\"\n", cmd, &node[i+1], o_node_value);
		break;
	}
}

/****************************************************************************/

static unsigned long get_flags(void)
{
	unsigned long flags=0;
	if (o_escape && strcmp(o_escape, "js")==0) flags |= RGDB_ESCAPE_JS;
	if (o_skip_proc) flags |= RGDB_SKIP_PROC;
	return flags;
}

static int action_get(int fd)
{
	return lrgdb_get(fd, get_flags(), o_node_path, stdout);
}

static int action_patch(int fd)
{
	return lrgdb_patch(fd, get_flags(), o_tempfile, stdout);
}

static int action_ephp(int fd)
{
	return lrgdb_ephp(fd, get_flags(), o_ephpfile, stdout);
}

static int action_patch_buffer(int fd)
{
	return lrgdb_patch_buffer(fd, get_flags(), o_patchmsg, stdout);
}

static int action_set(int fd)
{
	return lrgdb_set(fd, get_flags(), o_node_path, o_node_value);
}

static int action_set_notify(int fd)
{
	return lrgdb_set_notify(fd, get_flags(), o_node_path, o_node_value);
}

static int action_del(int fd)
{
	return lrgdb_del(fd, get_flags(), o_node_path);
}

static int action_reload(int fd)
{
	return lrgdb_reload(fd, get_flags(), o_reloadfile);
}

static int action_dump(int fd)
{
	return lrgdb_dump(fd, get_flags(), o_dumpfile);
}

/****************************************************************************/

#ifdef RGBIN_BOX
int rgdb_main(int argc, char * argv[])
#else
int main(int argc, char * argv[])
#endif
{
	int fd;
	int i=-1, l;

	parse_args(argc, argv);

	if (o_node_value) lrgbin_reatwhite(o_node_value);
	if (o_node_path)
	{
		lrgbin_reatwhite(o_node_path);
		l = strlen(o_node_path);
		for (i=0; i<l; i++) o_node_path[i] = tolower(o_node_path[i]);

		/* check to see if it's runtime node. */
		if (!o_ignore && strncmp(o_node_path, "/runtime/", 9)==0)
		{
			runtime(&o_node_path[9]);
			cleanup_exit(0);
		}
	}

	fd = lrgdb_open(o_unixsock);
	if (fd)
	{
		switch (o_action)
		{
		case RGDB_GET:			i = action_get(fd); printf("\n"); break;
		case RGDB_SET:			i = action_set(fd); break;
		case RGDB_SET_NOTIFY:	i = action_set_notify(fd); break;
		case RGDB_DEL:			i = action_del(fd); break;
		case RGDB_RELOAD:		i = action_reload(fd); break;
		case RGDB_DUMP:			i = action_dump(fd); break;
		case RGDB_PATCH:		i = action_patch(fd); break;
		case RGDB_EPHP:			i = action_ephp(fd); break;
		case RGDB_PATCH_BUFF:	i = action_patch_buffer(fd); break;
		}
		lrgdb_close(fd);
	}
	cleanup_exit(i);
	return i;
}
