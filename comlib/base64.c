/* vi: set sw=4 ts=4: */
/*
 * b64 encode/decode
 */

#include <stdio.h>
#include <stdlib.h>

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
static void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
	out[0] = cb64[ in[0] >> 2 ];
	out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*! \fn ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
	\brief Base64 encode a stream adding padding and line breaks as per spec.
	\par
	\b Note: The encoded stream must be freed
	\param input The stream to encode
	\param inputlen The length of \a input
	\param output The encoded stream
	\returns The length of the encoded stream
*/
int base64encode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* out;
	unsigned char* in;

	*output = (unsigned char*)malloc(((inputlen * 4) / 3) + 5);
	out = *output;
	in  = input;

	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}

	while ((in+3) <= (input+inputlen))
	{
		encodeblock(in, out, 3);
		in += 3;
		out += 4;
	}
	if ((input+inputlen)-in == 1)
	{
		encodeblock(in, out, 1);
		out += 4;
	}
	else if ((input+inputlen)-in == 2)
	{
		encodeblock(in, out, 2);
		out += 4;
	}
	*out = 0;

	return (int)(out-*output);
}

/* Decode 4 '6-bit' characters into 3 8-bit binary bytes */
static void decodeblock( unsigned char in[4], unsigned char out[3] )
{
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*! \fn ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
	\brief Decode a base64 encoded stream discarding padding, line breaks and noise
	\par
	\b Note: The decoded stream must be freed
	\param input The stream to decode
	\param inputlen The length of \a input
	\param output The decoded stream
	\returns The length of the decoded stream
*/
int base64decode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* inptr;
	unsigned char* out;
	unsigned char v;
	unsigned char in[4];
	int i, len;

	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}

	*output = (unsigned char*)malloc(((inputlen * 3) / 4) + 4);
	out = *output;
	inptr = input;

	while( inptr <= (input+inputlen) )
	{
		for( len = 0, i = 0; i < 4 && inptr <= (input+inputlen); i++ )
		{
			v = 0;
			while( inptr <= (input+inputlen) && v == 0 ) {
				v = (unsigned char) *inptr;
				inptr++;
				v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if( v ) {
					v = (unsigned char) ((v == '$') ? 0 : v - 61);
				}
			}
			if( inptr <= (input+inputlen) ) {
				len++;
				if( v ) {
					in[ i ] = (unsigned char) (v - 1);
				}
			}
			else {
				in[i] = 0;
			}
		}
		if( len )
		{
			decodeblock( in, out );
			out += len-1;
		}
	}
	*out = 0;
	return (int)(out-*output);
}


#ifdef BASE64_TEST

#include <string.h>

int main(int argc, char argv[])
{
	unsigned char text[] = { 0x1, 0x2, 0x4, 0x0, 0xff, 0x45, 0xab, 0xfd, 0x56, 0x55, 0xaa, 0x5a };
	int size = sizeof(text);
	int i;
	unsigned char * out = NULL;
	unsigned char * out2 = NULL;

	printf("TEXT: ");
	for (i=0; i<size; i++)
	{
		if ((i%16)==0) printf("\n");
		printf("%02x ", text[i]);
	}
	printf("\n");

	base64encode(text, (const int)size, &out);
	if (out) printf("encoded = [%s]\n", out);

	else printf("Woops !!!! no output buffer !!!\n");

	size = base64decode(out, (const int)strlen(out), &out2);
	printf("decoded [size=%d]: ", size);
	for (i=0; out2 && i<size; i++)
	{
		if ((i%16)==0) printf("\n");
		printf("%02x ", out2[i]);
	}
	printf("\n");

	if (out) free(out);
	if (out2) free(out2);
}
#endif
