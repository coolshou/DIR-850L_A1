/* vi: set sw=4 ts=4: */
/* dtrace.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "dtrace.h"

#define OUTPUT stderr
int ddbg_level = DBG_DEFAULT;

void __dtrace(int level, const char * format, ...)
{
	va_list marker;
	if (ddbg_level <= level)
	{
		va_start(marker, format);
		vfprintf(OUTPUT, format, marker);
		va_end(marker);
	}
}

void __dassert(char * exp, char * file, int line)
{
	__dtrace(DBG_ALL, "DASSERT: file: %s, line: %d\n", file, line);
	__dtrace(DBG_ALL, "\t%s\n", exp);
	abort();
}


#ifdef TEST_DTRACE

int main(int argc, char * argv[])
{
	dtrace("Hello world !\n");
	dtrace("Hello: %d %s 0x%x\n", 12, "test", 12);
	return 0;
}

#endif

