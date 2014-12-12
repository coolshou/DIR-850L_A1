/* vi: set sw=4 ts=4: */
/*
 *	hexstring.c
 *
 *	This file contains some useful function to manipulate HEX-string.
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 *	Copyright (C) 2007-2009 by Alpha Networks, Inc.
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
#include <string.h>

/* convert the HEX char. */
static int hex_char(char c)
{
	if (c>='0' && c<='9') return (int)(c - '0');
	if (c>='a' && c<='f') return (int)(c - 'a' + 10);
	if (c>='A' && c<='F') return (int)(c - 'A' + 10);
	return -1;
}

/*
 * Parse the HEX string to buffer.
 * 'buf'	- the buffer to save the result.
 * 'size'	- the size of the buffer.
 * 'string'	- the HEX string.
 * On return, the byte count of the saved data in buffer.
 */
size_t read_hexstring(unsigned char * buf, size_t size, const char * string)
{
	size_t i = 0;
	int c;

	while (*string && i < size)
	{
		buf[i] = 0;

		do
		{
			/* first digit */
			c = hex_char(*string++);
			if (c < 0) break;
			buf[i] = (char)c;

			/* second digit */
			if (!(*string)) break;
			c = hex_char(*string++);
			if (c < 0) break;
			buf[i] = buf[i] * 0x10 + (char)c;

			/* If next  */
			if (!(*string)) break;
			if (hex_char(*string) < 0) string++;
		} while (0);
		i++;
	}
	return i;
}

/* convert the binary values to MAC Address string. */
const char * print_macaddr(const unsigned char * macaddr)
{
	static char buf[32];
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr[0], macaddr[1],
		macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	return buf;
}

/* convert the binary values to UUID string. */
const char * print_uuid(const unsigned char * uuid)
{
	static char buf[40];
	printf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
		uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9],
		uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
	return buf;
}

void dump_hex(FILE * out, const char * start, const char * end, char delimiter,
				 const unsigned char * bin, size_t size)
{
	size_t i;

	if (!out) out = stdout;
	if (start) fprintf(out, start);
	if (size > 0)
	{
		if (delimiter)
		{
			fprintf(out, "%02x", bin[0]);
			for (i=1; i < size; i++) fprintf(out, "%c%02x", delimiter, bin[i]);
		}
		else
		{
			for (i=1; i < size; i++) fprintf(out, "%02x", bin[i]);
		}
	}
	if (end) fprintf(out, end);
}

#ifdef TEST_CASE
struct testcase
{
	size_t size;
	const char * pattern;
};

struct testcase cases[] =
{
	{ 6, "00:11:22:33:44:55" },
	{ 6, "AABBCCDDEEFF" },
	{ 6, "0,1,2,3,4,5,6,7,8,9,0" },
	{ 6, "00:80:c8:123456" },
	{ 16, "4acb2345-0987-1234-7654-00aacc1122bb" },
	{ 0, NULL }
};

int main(int argc, char * argv[])
{
	unsigned char buffer[512];
	int i;

	for (i=0; cases[i].size; i++)
	{
		memset(buffer, 0xcc, sizeof(buffer));
		read_hexstring(buffer, cases[i].size, cases[i].pattern);
		printf("case %d: pattern=[%s], result=", i, cases[i].pattern);
		dump_hex(stdout, "[", "]\n", '-', buffer, cases[i].size);
	}
}
#endif
