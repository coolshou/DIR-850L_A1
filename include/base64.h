/* vi: set sw=4 ts=4: */
#ifndef __BASE64_HEADER__
#define __BASE64_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

int base64encode(unsigned char* input, const int inputlen, unsigned char** output);
int base64decode(unsigned char* input, const int inputlen, unsigned char** output);

#ifdef __cplusplus
}
#endif

#endif
