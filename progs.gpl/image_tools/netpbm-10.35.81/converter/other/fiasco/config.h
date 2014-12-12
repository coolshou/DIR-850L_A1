/* config.h.  Generated automatically by configure.  */
/* But then manually maintained as part of Netpbm! */

/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if the X Window System is missing or not being used.  */
#define X_DISPLAY_MISSING 1

/* Define if shifting of signed integers works */
#define HAVE_SIGNED_SHIFT 1

/* Define if you don't have the CLOCKS_PER_SEC define in <time.h>. */
/* #undef CLOCKS_PER_SEC */

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* Define if you have the log2 function.  */
/* #undef HAVE_LOG2 */
/* For Netpbm, we won't use log2() because it might not exist.  In
   Netpbm, misc.c contains Log2(), so we just define log2 as Log2 here.
   But first, we include <math.h> because if we don't it may get included
   after config.h, and then there could be a redefinition issue with log2.
*/
#include <math.h>
#undef log2
#define log2 Log2

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the <X11/extensions/XShm.h> header file.  */
/* #undef HAVE_X11_EXTENSIONS_XSHM_H */

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <features.h> header file.  */
#define HAVE_FEATURES_H 1

/* Define if you have the <setjmp.h> header file.  */
#define HAVE_SETJMP_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1

/* Name of package */
#define PACKAGE "fiasco"

/* Version number of package */
#define VERSION "1.0"

#define FIASCO_SHARE "/etc/"
