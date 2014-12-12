/* vi: set sw=4 ts=4: */
/*
 *	libxmldbc.c
 *
 *	common library for xmldb client.
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2004-2009 by Alpha Networks, Inc.
 *
 *	This file is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either'
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	The GNU C Library is distributed in the hope that it will be useful,'
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with the GNU C Library; if not, write to the Free
 *	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *	02111-1307 USA.
 */

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

#include <dtrace.h>
#include <xmldb.h>
#include <libxmldbc.h>

#ifdef DEBUG_LIBXMLDBC
#define XMLDBCDBG(x)	x
#else
#define XMLDBCDBG(x)
#endif

int lxmldbc_run_shell(char * buf, int size, const char * format, ...)
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
int lxmldbc_system(const char * format, ...)
{
	char cmd[MAX_CMD_LEN];
	va_list marker;

	va_start(marker, format);
	vsnprintf(cmd, sizeof(cmd), format, marker);
	va_end(marker);
	return system(cmd);
}

#define IS_WHITE(x)	((x) == ' ' || (x)=='\t' || (x) == '\n' || (x) == '\r')

char * lxmldbc_eatwhite(char * string)
{
	if (string==NULL) return NULL;
	while (*string)
	{
		if (!IS_WHITE(*string)) break;
		string++;
	}
	return string;
}

char * lxmldbc_reatwhite(char * ptr)
{
	int i;

	if (ptr==NULL) return NULL;
	i = strlen(ptr)-1;
	while (i >= 0 && IS_WHITE(ptr[i])) ptr[i--] = '\0';
	return ptr;
}

/**************************************************************************/

static int __open_socket(const char * sockname)
{
	struct sockaddr_un where;
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		d_error("%s: Counld not create unix domain socket: %s.\n",__FUNCTION__, strerror(errno));
		return -1;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);

	where.sun_family = AF_UNIX;
	if (sockname == NULL) sockname = XMLDB_DEFAULT_UNIXSOCK;
	snprintf(where.sun_path, sizeof(where.sun_path), "%s", sockname);

	if (connect(fd, (struct sockaddr *)&where, sizeof(where)) < 0)
	{
		d_error("%s: Cound not connect to unix socket: %s.\n",__FUNCTION__, sockname);
		close(fd);
		return -1;
	}
	return fd;
}

static int send_xmldb_cmd(int fd, action_t action, unsigned long flags, const char * data, unsigned short length)
{
	rgdb_ipc_t ipc;
	ssize_t size;

	ipc.action = action;
	ipc.flags = flags;
	ipc.length = length;
	size = send(fd, &ipc, sizeof(ipc), MSG_NOSIGNAL);
	if (size <= 0) return -1;
	size = send(fd, data, length, MSG_NOSIGNAL);
	if (size <= 0) return -1;
	return 0;
}

static void redirect_output(int fd, FILE * out)
{
	fd_set read_set;
	ssize_t size;
	char buff[1024];

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
	//joel add for bcm platform.the web server will lost some data without flush.
	fflush(out);
}

static size_t redirect_to_buffer(int fd, char * buff, size_t buff_size)
{
	fd_set read_set;
	ssize_t size;
	size_t written = 0;

	dassert(buff && buff_size);

	for (;;)
	{
		FD_ZERO(&read_set);
		FD_SET(fd, &read_set);
		if (select(fd+1, &read_set, NULL, NULL, NULL) < 0) continue;
		if (FD_ISSET(fd, &read_set))
		{
			size = read(fd, buff+written, buff_size - written);
			if (size <= 0) break;
			written += size;
			if (buff[written - 1] == '\0') break;
			if (buff_size >= written)
			{
				d_error("%s: no more buffer space for read !!\n", __FUNCTION__);
				break;
			}
		}
	}
	return written;
}

/* command with output */
static int _cmd_w_out(sock_t sn, action_t a, flag_t f, const void * param, size_t size, FILE * out)
{
	int sock, ret = -1;
	if ((sock = __open_socket(sn)) >= 0)
	{
		if (send_xmldb_cmd(sock, a, f, param, size) >= 0)
		{
			redirect_output(sock, out ? out : stdout);
			ret = 0;
		}
		close(sock);
	}
	return ret;
}

/* command without output */
static int _cmd_wo_out(sock_t sn, action_t a, flag_t f, const void * param, size_t size)
{
	rgdb_ipc_t ipc;
	ssize_t rsize;
	int sock;
	int ret = -1;

	if ((sock = __open_socket(sn)) >= 0)
	{
		if (send_xmldb_cmd(sock, a, f, param, size) >= 0)
		{
			rsize = read(sock, &ipc, sizeof(ipc));
			ret = ipc.retcode;
		}
		close(sock);
	}
	return ret;
}

/***************************************************************************/
/* export functions */

ssize_t xmldbc_get_wb(sock_t sn, flag_t f, const char * node, char * buff, size_t size)
{
	int sock;
	ssize_t ret = -1;

	if ((sock = __open_socket(sn)) >= 0)
	{
		if (send_xmldb_cmd(sock, XMLDB_GET, f, node, strlen(node)+1) >= 0)
		{
			redirect_to_buffer(sock, buff, size);
			ret = 0;
		}
		close(sock);
	}
	return ret;
}

int xmldbc_get(sock_t sn, flag_t f, const char * node, FILE * out)
{
	return _cmd_w_out(sn, XMLDB_GET, f, node, strlen(node)+1, out);
}

ssize_t xmldbc_ephp_wb(sock_t sn, flag_t f, const char * file, char * buff, size_t size)
{
	int sock;
	ssize_t ret = -1;

	if ((sock = __open_socket(sn)) >= 0)
	{
		if (send_xmldb_cmd(sock, XMLDB_EPHP, f, file, strlen(file)+1) >= 0)
		{
			redirect_to_buffer(sock, buff, size);
			ret = 0;
		}
		close(sock);
	}
	return ret;
}

int xmldbc_ephp(sock_t sn, flag_t f, const char * file, FILE * out)
{
	return _cmd_w_out(sn, XMLDB_EPHP, f, file, strlen(file)+1, out);
}

int xmldbc_set(sock_t sn, flag_t f, const char * node, const char * value)
{
	char buff[512];

	snprintf(buff, sizeof(buff)-1, "%s %s", node, value);
	buff[511] = '\0';
	return _cmd_wo_out(sn, XMLDB_SET, f, buff, strlen(buff)+1);
}

int xmldbc_setext(sock_t sn, flag_t f, const char * node, const char * cmd)
{
	char buff[512];

	snprintf(buff, sizeof(buff)-1, "%s %s", node, cmd);
	buff[511] = '\0';
	return _cmd_wo_out(sn, XMLDB_SETEXT, f, buff, strlen(buff)+1);
}

int xmldbc_timer(sock_t sn, flag_t f, const char * cmd)
{
	return _cmd_wo_out(sn, XMLDB_TIMER, f, cmd, strlen(cmd)+1);
}

int xmldbc_killtimer(sock_t sn, flag_t f, const char * tag)
{
	return _cmd_wo_out(sn, XMLDB_KILLTIMER, f, tag, strlen(tag)+1);
}

int xmldbc_del(sock_t sn, flag_t f, const char * node)
{
	return _cmd_wo_out(sn, XMLDB_DEL, f, node, strlen(node)+1);
}

int xmldbc_reload(sock_t sn, flag_t f, const char * file)
{
	return _cmd_wo_out(sn, XMLDB_RELOAD, f, file, strlen(file)+1);
}

int xmldbc_patch(sock_t sn, flag_t f, const char * file)
{
	return _cmd_wo_out(sn, XMLDB_PATCH, f, file, strlen(file)+1);
}

int xmldbc_read(sock_t sn, flag_t f, const char * file)
{
	return _cmd_wo_out(sn, XMLDB_READ, f, file, strlen(file)+1);
}

int xmldbc_write(sock_t sn, flag_t f, const char * node, FILE * out)
{
	return _cmd_w_out(sn, XMLDB_WRITE, f, node, strlen(node)+1, out);
}

int xmldbc_dump(sock_t sn, flag_t f, const char * file)
{
	return _cmd_wo_out(sn, XMLDB_DUMP, f, file, strlen(file)+1);
}
