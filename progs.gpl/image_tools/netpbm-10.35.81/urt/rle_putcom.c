/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may not include this software in a program or other software product
 * without supplying the source, or without informing the end-user that the 
 * source is available for no extra charge.
 *
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 */
/* 
 * rle_putcom.c - Add a picture comment to the header struct.
 * 
 * Author:  Spencer W. Thomas
 *      Computer Science Dept.
 *      University of Utah
 * Date:    Mon Feb  2 1987
 * Copyright (c) 1987, University of Utah
 */

#include <stdio.h>

#include "mallocvar.h"
#include "pm.h"
#include "rle.h"

/*****************************************************************
 * TAG( match )
 * 
 * Match a name against a test string for "name=value" or "name".
 * If it matches name=value, return pointer to value part, if just
 * name, return pointer to NUL at end of string.  If no match, return NULL.
 *
 * Inputs:
 *  n:  Name to match.  May also be "name=value" to make it easier
 *      to replace comments.
 *  v:  Test string.
 * Outputs:
 *  Returns pointer as above.
 * Assumptions:
 *  [None]
 * Algorithm:
 *  [None]
 */
static const char *
match(const char * const nArg,
      const char * const vArg) {

    const char * n;
    const char * v;

    for (n = nArg, v = vArg; *n != '\0' && *n != '=' && *n == *v; ++n, ++v)
        ;
    if (*n == '\0' || *n == '=') {
        if (*v == '\0')
            return v;
        else if (*v == '=')
            return ++v;
    }

    return NULL;
}



/*****************************************************************
 * TAG( rle_putcom )
 * 
 * Put a comment into the header struct.
 * Inputs:
 *  value:      Value to add to comments.
 *  the_hdr:    Header struct to add to.
 * Outputs:
 *  the_hdr:    Modified header struct.
 *  Returns previous value;
 * Assumptions:
 *  value pointer can be used as is (data is NOT copied).
 * Algorithm:
 *  Find match if any, else add at end (realloc to make bigger).
 */
const char *
rle_putcom(const char * const value,
           rle_hdr *    const the_hdr) {

    if ( the_hdr->comments == NULL) {
        MALLOCARRAY_NOFAIL(the_hdr->comments, 2);
        the_hdr->comments[0] = value;
        the_hdr->comments[1] = NULL;
    } else {
        const char ** cp;
        const char * v;
        const char ** old_comments;
        int i;
        for (i = 2, cp = the_hdr->comments; *cp != NULL; ++i, ++cp)
            if (match(value, *cp) != NULL) {
                v = *cp;
                *cp = value;
                return v;
            }
        /* Not found */
        /* Can't realloc because somebody else might be pointing to this
         * comments block.  Of course, if this were true, then the
         * assignment above would change the comments for two headers.
         * But at least, that won't crash the program.  Realloc will.
         * This would work a lot better in C++, where hdr1 = hdr2
         * could copy the pointers, too.
         */
        old_comments = the_hdr->comments;
        MALLOCARRAY(the_hdr->comments, i);
        if (the_hdr->comments == NULL)
            pm_error("Unable to allocate memory for comments");
        the_hdr->comments[--i] = NULL;
        the_hdr->comments[--i] = value;
        for (--i; i >= 0; --i)
            the_hdr->comments[i] = old_comments[i];
    }

    return NULL;
}



/*****************************************************************
 * TAG( rle_delcom )
 * 
 * Delete a comment from header struct.
 * Inputs:
 *  name:       Name of comment to delete.
 *  the_hdr:    Header to delete comment from.
 * Outputs:
 *  the_hdr:    Modified header struct.
 *  Returns original comment value.
 * Assumptions:
 *  [None]
 * Algorithm:
 *  [None]
 */
const char *
rle_delcom(const char * const name,
           rle_hdr *    const the_hdr) {

    const char * v = NULL;

    if (the_hdr->comments == NULL)
        v = NULL;
    else {
        const char ** cp;

        for (cp = the_hdr->comments; *cp != NULL; ++cp)
            if (match(name, *cp) != NULL) {
                v = *cp;
                for ( ; *cp != NULL; ++cp)
                    *cp = cp[1];
                break;
            }
        /* Not found */
        if (*the_hdr->comments == NULL)
            the_hdr->comments = NULL;
    }

    return v;
}
