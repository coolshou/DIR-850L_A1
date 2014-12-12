/* dirutil.h ... directory utilities.
 *               C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: dirutil.h,v 1.1.1.1 2005/05/19 10:53:07 r01122 Exp $
 */

#ifndef __DIRUTILS_HEADER_FILE__
#define __DIRUTILS_HEADER_FILE__

#ifdef __cplusplus
extern "C" {
#endif

/* Returned malloc'ed string representing basename */
char *basenamex(char *pathname);
/* Return malloc'ed string representing directory name (no trailing slash) */
char *dirname(const char *pathname);
/* In-place modify a string to remove trailing slashes.  Returns arg. */
char *stripslash(char *pathname);
/* ensure dirname exists, creating it if necessary. */
int make_valid_path(char *dirname, mode_t mode);

#ifdef __cplusplus
}
#endif
#endif
