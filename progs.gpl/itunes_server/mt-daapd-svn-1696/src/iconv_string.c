/* Copyright (C) 1999-2001, 2003 Bruno Haible.
   This file is not part of the GNU LIBICONV Library.
   This file is put into the public domain.  */

#include "iconv_string.h"
#include <iconv.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define tmpbufsize 4096

int iconv_string (const char* tocode, const char* fromcode,
                  const char* start, const char* end,
                  char** resultp, size_t* lengthp)
{
	
	iconv_t cd = 0;
	size_t length = 0;
  char* result = NULL;
  
	cd = iconv_open(tocode,fromcode);
  DBG_ICONV(printf("iconv_string.c: iconv_open=%d\n",cd);)
  if (cd == (iconv_t)(-1)) {
    if (errno != EINVAL)
      return -1;
      
     /* prevent detector error */
     int ret;
      
     ret = iconv_string(tocode,"GB18030",start,end,resultp,lengthp);

		 if (!(ret < 0 && errno == EILSEQ)){
     	return ret;
		 ret = iconv_string(tocode,"BIG5",start,end,resultp,lengthp);  
		 	return ret;
    }
    
    errno = EINVAL;
    return -1;
  }
  /* Determine the length we need. */
  {
    size_t count = 0;
    char tmpbuf[tmpbufsize];
    const char* inptr = start;
    size_t insize = end-start;
    while (insize > 0) {
      char* outptr = tmpbuf;
      size_t outsize = tmpbufsize;
      size_t res = iconv(cd,&inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1) && errno != E2BIG) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
      count += outptr-tmpbuf;
    }
    {
      char* outptr = tmpbuf;
      size_t outsize = tmpbufsize;
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
      count += outptr-tmpbuf;
    }
    length = count;
  }
  if (lengthp != NULL)
    *lengthp = length;
  if (resultp == NULL) {
    iconv_close(cd);
    return 0;
  }
  result = (*resultp == NULL ? malloc(length) : realloc(*resultp,length));
  *resultp = result;
  if (length == 0) {
    iconv_close(cd);
    return 0;
  }
  if (result == NULL) {
    iconv_close(cd);
    errno = ENOMEM;
    return -1;
  }
  iconv(cd,NULL,NULL,NULL,NULL); /* return to the initial state */
  /* Do the conversion for real. */
  {
    const char* inptr = start;
    size_t insize = end-start;
    char* outptr = result;
    size_t outsize = length;
    while (insize > 0) {
      size_t res = iconv(cd,&inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
    }
    {
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
    }
    if (outsize != 0) abort();
  }
  iconv_close(cd);
  return 0;
}
