/* vi: set sw=4 ts=4: */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "lrgbin.h"
#include "rgdb.h"

int lrgbin_run_shell(char * buf, int size, const char * format, ...)
{
	FILE * fp;
	int i, c;
	char cmd[MAX_CMD_LEN];
	va_list marker;

	va_start(marker, format);
	vsnprintf(cmd, sizeof(cmd), format, marker);
	va_end(marker);

	fp = popen(cmd, "r");
	if (fp)
	{
		for (i=0; i<size-1; i++)
		{
			c = fgetc(fp);
			if (c == EOF) break;
			buf[i] = (char)c;
		}
		buf[i] = '\0';
		pclose(fp);

		/* remove the last '\n' */
		i = strlen(buf);
		if (buf[i-1] == '\n') buf[i-1] = 0;
		return 0;
	}
	buf[0] = 0;
	return -1;
}

/* call system() in printf() format. */
int lrgbin_system(const char * format, ...)
{
	char cmd[MAX_CMD_LEN];
	va_list marker;

	va_start(marker, format);
	vsnprintf(cmd, sizeof(cmd), format, marker);
	va_end(marker);
	return system(cmd);
}

char * lrgbin_eatwhite(char * string)
{
	if (string==NULL) return NULL;
	while (*string)
	{
		if (!IS_WHITE(*string)) break;
		string++;
	}
	return string;
}

void lrgbin_reatwhite(char * ptr)
{
	int i;

	if (ptr==NULL) return;
	i = strlen(ptr)-1;
	while (i >= 0 && (ptr[i] == ' ' || ptr[i]=='\t')) ptr[i--] = '\0';
}

/***************************************************************************/
/* functions for pointer array. */

void lrgbin_pa_init(parray_t * pa, size_t growsby)
{
	pa->growsby = growsby ? growsby : 16;
	pa->size = 0;
	pa->array = NULL;
}

void lrgbin_pa_destroy(parray_t * pa, int do_free)
{
	size_t i;

	if (pa->size > 0 && pa->array)
	{
		if (do_free)
		{
			for (i=0; i<pa->size; i++) { if (pa->array[i]) free(pa->array[i]); }
		}
		free(pa->array);
	}
	lrgbin_pa_init(pa, 16);
}

int lrgbin_pa_grows(parray_t * pa, size_t size)
{
	void ** newarray;

	/* if size is enough, do nothing. */
	if (pa->size >= size) return 0;

	newarray = calloc(size, sizeof(void *));
	if (!newarray) return -1;

	if (pa->array)
	{
		memcpy(newarray, pa->array, pa->size * sizeof(void *));
		free(pa->array);
	}
	pa->array = newarray;
	pa->size = size;
	return 0;
}

void * lrgbin_pa_get_nth(parray_t * pa, size_t index)
{
	if (index >= pa->size) return NULL;
	return pa->array[index];
}

int lrgbin_pa_set_nth(parray_t * pa, size_t index, void * pointer, int do_free)
{
	int ret;

	if (index >= pa->size)
	{
		ret = lrgbin_pa_grows(pa, pa->size + pa->growsby);
		if (ret < 0) return ret;
	}
	if (do_free && pa->array[index]) free(pa->array[index]);
	pa->array[index] = pointer;
	return 0;
}
 
/***************************************************************************/

/***************************************************************************/

/* helper function to generate day strings for iptables */
static char __days[] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,Sun,Mon,Tue,Wed,Thu,Fri,Sat,";
static char _days[32];
const char * lrgbin_getdaystring(int start, int end)
{
	/* make sure start & end are 0 ~ 6 */
	start %= 7; end %= 7;

	if (start == end)
	{
		memcpy(_days, &__days[start*4], 3);
		_days[3] = '\0';
	}
	else if (start < end)
	{
		memcpy(_days, &__days[start*4], (end-start)*4+3);
		_days[(end-start)*4+3] = '\0';
	}
	else
	{
		end += 7;
		memcpy(_days, &__days[start*4], (end-start)*4+3);
		_days[(end-start)*4+3] = '\0';
	}

	return _days;
}

/***************************************************************************/

char * lrgbin_get_line_from_file(FILE * file, int strip_new_line)
{
	static const int GROWBY = 80;	/* how large we will grow strings by */
	int ch;
	int idx = 0;
	char * linebuf = NULL;
	int linebufsz = 0;

	while (1)
	{
		ch = fgetc(file);
		if (ch == EOF) break;

		/* grow the line buffer as necessary */
		while (idx > linebufsz - 2)
			linebuf = realloc(linebuf, linebufsz += GROWBY);

		/* strip the tailing newline character if necessary */
		if (strip_new_line && ch == '\n') ch = 0;
		linebuf[idx++] = (char)ch;
		if (ch == '\n' || ch == '\0') break;
	}

	/* if the length is zero, return NULL buffer. */
	if (idx == 0) return NULL;

	linebuf[idx] = '\0';
	return linebuf;
}

/***************************************************************************/

/* open socket connection to xmldb */
int lrgdb_open(const char * sockname)
{
	struct sockaddr_un where;
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "Cound not create unix domain socket: %s.\n", strerror(errno));
		return -1;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);

	where.sun_family = AF_UNIX;
	if (sockname)	snprintf(where.sun_path, sizeof(where.sun_path), "%s", sockname);
	else			snprintf(where.sun_path, sizeof(where.sun_path), RGDB_DEFAULT_UNIXSOCK);
	if (connect(fd, (struct sockaddr *)&where, sizeof(where)) < 0)
	{
		fprintf(stderr, "Cound not connect to unix socket: %s.\n", sockname);
		close(fd);
		return -1;
	}
	return fd;
}

void lrgdb_close(int fd)
{
	close(fd);
}

static int send_xmldb_cmd(int fd, action_t action, unsigned long flags, const char * data, unsigned short length)
{
	rgdb_ipc_t ipc;
	ssize_t size;

	ipc.action = action;
	ipc.flags = flags;
	ipc.length = length;
	size = write(fd, &ipc, sizeof(ipc));
	if (size <= 0) return -1;
	size = write(fd, data, length);
	if (size <= 0) return -1;
	return 0;
}

static void redirect_output(int fd, FILE * out)
{
	fd_set read_set;
	ssize_t size;
	static char buff[1024];

	for (;;)
	{
		FD_ZERO(&read_set);
		FD_SET(fd, &read_set);
		if (select(fd+1, &read_set, NULL, NULL, NULL) < 0) continue;
		if (FD_ISSET(fd, &read_set))
		{
			size = read(fd, buff, sizeof(buff));
			if (size <= 0) break;
			if (buff[size-1] == '\0')
			{
				fwrite(buff, 1, strlen(buff), out);
				break;
			}
			else
			{
				fwrite(buff, 1, size, out);
			}
		}
	}
}

int lrgdb_get(int fd, unsigned long flags, const char * node, FILE * out)
{
	if (send_xmldb_cmd(fd, RGDB_GET, flags, node, strlen(node)+1) < 0) return -1;
	if (out) redirect_output(fd, out);
	return 0;
}

/* get with buffer */
ssize_t lrgdb_getwb(int fd, char * buff, size_t size, const char * format, ...)
{
	char node[512];
	va_list marker;
	ssize_t ret;

	va_start(marker, format);
	vsnprintf(node, sizeof(node), format, marker);
	va_end(marker);

	if (lrgdb_get(fd, 0, node, NULL)<0) return -1;
	ret = read(fd, buff, size);
	return ret;
}

int lrgdb_patch(int fd, unsigned long flags, const char * tempfile, FILE * out)
{
	if (send_xmldb_cmd(fd, RGDB_PATCH, flags, tempfile, strlen(tempfile)+1) < 0) return -1;
	if (out) redirect_output(fd, out);
	return 0;
}

int lrgdb_ephp(int fd, unsigned long flags, const char * phpfile, FILE * out)
{
	if (send_xmldb_cmd(fd, RGDB_EPHP, flags, phpfile, strlen(phpfile)+1) < 0) return -1;
	if (out) redirect_output(fd, out);
	return 0;
}

int lrgdb_patch_buffer(int fd, unsigned long flags, const char * buffer, FILE * out)
{
	if (send_xmldb_cmd(fd, RGDB_PATCH_BUFF, flags, buffer, strlen(buffer)+1) < 0) return -1;
	if (out) redirect_output(fd, out);
	return 0;
}

int lrgdb_set(int fd, unsigned long flags, const char * node, const char * value)
{
	char buff[256];
	rgdb_ipc_t ipc;
	ssize_t size;

	snprintf(buff, sizeof(buff)-1, "%s %s", node, value);
	buff[255]='\0';
	if (send_xmldb_cmd(fd, RGDB_SET, flags, buff, strlen(buff)+1) < 0) return -1;
	size = read(fd, &ipc, sizeof(ipc));
	return ipc.retcode;
}

int lrgdb_set_notify(int fd, unsigned long flags, const char * node, const char * message)
{
	char buff[256];
	rgdb_ipc_t ipc;
	ssize_t size;

	snprintf(buff, sizeof(buff)-1, "%s %s", node, message);
	buff[255]='\0';
	if (send_xmldb_cmd(fd, RGDB_SET_NOTIFY, flags, buff, strlen(buff)+1) < 0) return -1;
	size = read(fd, &ipc, sizeof(ipc));
	return ipc.retcode;
}

int lrgdb_del(int fd, unsigned long flags, const char * node)
{
	rgdb_ipc_t ipc;
	ssize_t size;
	
	if (send_xmldb_cmd(fd, RGDB_DEL, flags, node, strlen(node)+1) < 0) return -1;
	size = read(fd, &ipc, sizeof(ipc));
	return ipc.retcode;
}

int lrgdb_reload(int fd, unsigned long flags, const char * file)
{
	rgdb_ipc_t ipc;
	ssize_t size;

	if (send_xmldb_cmd(fd, RGDB_RELOAD, flags, file, strlen(file)+1) < 0) return -1;
	size = read(fd, &ipc, sizeof(ipc));
	return ipc.retcode;
}

int lrgdb_dump(int fd, unsigned long flags, const char * file)
{
	rgdb_ipc_t ipc;
	ssize_t size;

	if (send_xmldb_cmd(fd, RGDB_DUMP, flags, file, strlen(file)+1) < 0) return -1;
	size = read(fd, &ipc, sizeof(ipc));
	return ipc.retcode;
}

/***************************************************************************/

