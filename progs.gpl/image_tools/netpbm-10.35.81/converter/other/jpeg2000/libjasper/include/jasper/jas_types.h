/* In the original Jasper library, this contains definitions of the int_fast32,
   etc. types and bool.

   In Netpbm, we do that with pm_config.h, and the original Jasper
   method doesn't work.
*/
#include "pm_config.h"


/* The below macro is intended to be used for type casts.  By using this
  macro, type casts can be easily located in the source code with
  tools like "grep". */
#define	JAS_CAST(t, e) \
	((t) (e))
