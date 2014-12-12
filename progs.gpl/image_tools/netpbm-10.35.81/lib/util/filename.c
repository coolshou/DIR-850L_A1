#include "nstring.h"

#include "filename.h"

const char *
pm_basename(const char * const fileName) {
/*----------------------------------------------------------------------------
   Return the filename portion of a file name, e.g. "foo.ppm" from
   "/home/bryanh/foo.ppm".

   Return it as a malloc'ed string.
-----------------------------------------------------------------------------*/
    unsigned int basenameStart;
    unsigned int i;
    const char * retval;

    basenameStart = 0;  /* initial assumption */

    for (i = 0; fileName[i]; ++i) {
        if (fileName[i] == '/')
            basenameStart = i+1;
    }
    asprintfN(&retval, "%s", &fileName[basenameStart]);

    return retval;
}
