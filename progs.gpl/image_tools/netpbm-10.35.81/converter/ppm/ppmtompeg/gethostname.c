#define _BSD_SOURCE   /* Make sure strdup() is in string.h */

#include <string.h>
#include <errno.h>
#include <sys/utsname.h>

#include "pm.h"

#include "gethostname.h"

const char *
GetHostName(void) {
/*----------------------------------------------------------------------------
   Return the host name of this system.
-----------------------------------------------------------------------------*/
    struct utsname utsname;
    int rc;

    rc = uname(&utsname);

    if (rc < 0)
        pm_error("Unable to find out host name.  "
                 "uname() failed with errno %d (%s)", errno, strerror(errno));

    return strdup(utsname.nodename);
}
