/* Copyright (C) 1999-2001 Bruno Haible.
   This file is not part of the GNU LIBICONV Library.
   This file is put into the public domain.  */

/*
 * This C function converts an entire string from one encoding to another,
 * using iconv. Easier to use than iconv() itself, and supports autodetect
 * encodings on input.
 *
 *   int iconv_string (const char* tocode, const char* fromcode,
 *                     const char* start, const char* end,
 *                     char** resultp, size_t* lengthp)
 *
 * Converts a memory region given in encoding FROMCODE to a new memory
 * region in encoding TOCODE. FROMCODE and TOCODE are as for iconv_open(3),
 * except that FROMCODE may be one of the values
 *    "autodetect_utf8"          supports ISO-8859-1 and UTF-8
 *    "autodetect_jp"            supports EUC-JP, ISO-2022-JP-2 and SHIFT_JIS
 *    "autodetect_kr"            supports EUC-KR and ISO-2022-KR
 * The input is in the memory region between start (inclusive) and end
 * (exclusive). If resultp is not NULL, the output string is stored in
 * *resultp; malloc/realloc is used to allocate the result.
 *
 * This function does not treat zero characters specially.
 *
 * Return value: 0 if successful, otherwise -1 and errno set. Particular
 * errno values: EILSEQ and ENOMEM.
 *
 * Example:
 *   const char* s = ...;
 *   char* result = NULL;
 *   if (iconv_string("UCS-4-INTERNAL", "autodetect_utf8",
 *                    s, s+strlen(s)+1, &result, NULL) < 0)
 *     perror("iconv_string");
 *
 */
#include <stddef.h>

/* save char set and analyze it curtis@alpha 04_01_2009 */
#define CHAR_SET_LIST_LEN 30
#define CHAR_SET_LEN 30

typedef struct character_set {
		char name[CHAR_SET_LEN];
		int counter;
}CHAR_SET;

typedef struct character_set_list {
		CHAR_SET char_set_list[CHAR_SET_LIST_LEN];
}CHAR_SET_LIST;

//#define DBG_ICONV
#ifdef DBG_ICONV
	#define	DBG_ICONV(x)	x
#else
	#define	DBG_ICONV(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int iconv_string (const char* tocode, const char* fromcode, const char* start, const char* end, char** resultp, size_t* lengthp);

#ifdef __cplusplus
}
#endif
