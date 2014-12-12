#include "libbb.h"
#include "rgdb.h"
#include "lrgbin.h"

#define BUFSIZ 1024
#define Config_Path "/var/config.bin"
#define Acl_Path "/var/acl.tem"
#define CLI_OK 0
#define CLI_ERROR -1

/*		The rgdb tool function		*/
int _cli_rgdb_get_(char *buff, int size, const char *format, ...)
{
	int fd;
	char node[BUFSIZ];
	va_list marker;
	va_start(marker, format);
	vsnprintf(node, BUFSIZ, format, marker);
	va_end(marker);
	if((fd = lrgdb_open(NULL)) == -1)
	{
		perror("Cannot open database.");
		return -1;
	}
	if(lrgdb_get(fd, 0, node, NULL) == -1)
	{
		perror("Get ERROR!");
		lrgdb_close(fd);
		return -1;
	}
	if(read(fd, buff, size) == -1)
	{
		perror("Read database failed.");
		lrgdb_close(fd);
		return -1;
	}
	lrgdb_close(fd);
	return 0;
}
int _cli_rgdb_set_(const char *node, const char *value)
{
	int fd;
	if((fd = lrgdb_open(NULL)) == CLI_ERROR)
	{
		perror("Cannot open database.");
		return CLI_ERROR;
	}
	if(lrgdb_set(fd, 0, node, value) == CLI_ERROR)
	{
		lrgdb_close(fd);
		perror("Set ERROR!");
		return CLI_ERROR;
	}
	lrgdb_close(fd);
	return CLI_OK;
}
int _cli_rgdb_del_(const char *node)
{
	int fd;
	if((fd = lrgdb_open(NULL)) == CLI_ERROR)
	{
		perror("Cannot open database.");
		return CLI_ERROR;
	}
	if(lrgdb_del(fd, 0, node) == CLI_ERROR)
	{
		lrgdb_close(fd);
		perror("Del ERROR!");
		return CLI_ERROR;
	}
	lrgdb_close(fd);
	return CLI_OK;
}

