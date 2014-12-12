#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>

#include "nstring.h"
#include "pngtxt.h"
#include "pm.h"
#include "mallocvar.h"

#define MAXCOMMENTS 256



static void
readOffKey(char           const textline[],
           unsigned int   const lineLength,
           unsigned int * const cursorP,
           char **        const keyP) {

    /* Get the comment key */
    char * cp;
    unsigned int cursor;

    cursor = *cursorP;
    
    MALLOCARRAY(cp, lineLength + 1);  /* leave room for terminating NUL */
    if (cp == NULL) 
        pm_error("Unable to allocate memory for text chunks");
    
    *keyP = cp;
    
    if (textline[0] == '"') {
        ++cursor;  /* skip past opening " */
        while (textline[cursor] != '"') {
            if (textline[cursor] == '\0') {
                *cp = '\0';
                pm_error("Invalid comment file format:  keyword contains "
                         "a NUL character.  Text leading up to the NUL "
                         "character is '%s'", *keyP);
            }
            *(cp++) = textline[cursor++];
        }
        ++cursor;  /* skip past closing " */
    } else {
        while (cursor < lineLength && 
               textline[cursor] != ' '  && textline[cursor] != '\t' &&
               textline[cursor] != '\0')
            *(cp++) = textline[cursor++];
    }
    *cp++ = '\0';

    *cursorP = cursor;
}




static void
startComment(struct png_text_struct * const commentP, 
             char                     const textline[],
             unsigned int             const lineLength,
             bool                     const compressed) {
/*----------------------------------------------------------------------------
   Assuming 'textline' is the first line of a comment in a comment file,
   put the information from it in the comment record *commentP.
   Use the text on this line as the comment text, even though the true
   comment text may include text from subsequent continuation lines as
   well.

   'textline' is not NUL-terminated.  Its length is 'lineLength', and
   it is at least one character long.  'textline' does not contain a
   newline character.

   'compressed' means the comment text is compressed.
-----------------------------------------------------------------------------*/
    unsigned int cursor;

    /* the following is a not that accurate check on Author or Title */
    if ((!compressed) || (textline[0] == 'A') || (textline[0] == 'T'))
        commentP->compression = -1;
    else
        commentP->compression = 0;

    cursor = 0;

    readOffKey(textline, lineLength, &cursor, &commentP->key);

    /* skip over delimiters between key and comment text */
    while (cursor < lineLength && 
           (textline[cursor] == ' ' || textline[cursor] == '\t' ||
           textline[cursor] == '\0'))
        ++cursor;
    
    {
        /* Get the first line of the comment text */
        unsigned int const startPos = cursor;
        char *cp;

        MALLOCARRAY(cp, lineLength+1);  /* leave room for safety NUL */
        if (!cp) 
            pm_error("Unable to allocate memory for text chunks");

        memcpy(cp, textline + startPos, lineLength - startPos);
        cp[lineLength - startPos] = '\0';  /* for safety - not part of text */
        commentP->text = cp;
        commentP->text_length = lineLength - startPos;
    }
}



static void
continueComment(struct png_text_struct * const commentP, 
                char                     const textline[],
                unsigned int             const lineLength) {
/*----------------------------------------------------------------------------
   Update the comment record *commentP by adding to it the text
   from textline[], which is a continuation line from a comment file.

   'textline' is not NUL-terminated.  Its length is 'lineLength', and
   it is at least one character long.  'textline' does not contain a
   newline character.
-----------------------------------------------------------------------------*/
    unsigned int cursor;  /* cursor into textline[] */

    unsigned int const newTextLength =
        commentP->text_length + lineLength + 1 + 1;

    REALLOCARRAY(commentP->text, newTextLength);

    if (commentP->text == NULL)
        pm_error("Unable to allocate %u bytes of memory for comment chunk",
                 newTextLength);

    commentP->text[commentP->text_length++] = '\n';

    /* Skip past leading delimiter characters in file line */
    cursor = 0;
    while (textline[cursor] == ' ' || textline[cursor] == '\t' ||
           textline[cursor] == '\0')
        ++cursor;

    memcpy(commentP->text + commentP->text_length,
           textline + cursor,
           lineLength - cursor);

    commentP->text_length += lineLength - cursor;

    commentP->text[commentP->text_length] = '\0';  /* for safety */
}



static void
getFileLine(FILE *         const fileP, 
            const char **  const textP, 
            unsigned int * const lengthP) {
/*----------------------------------------------------------------------------
   Read the next line (characters from current position through the first
   newline character) and return it.  Put the text in newly malloc'ed 
   storage.

   Do not include the newline.

   Add a terminating NUL for safety, but note that you can't rely on this
   as the end of line marker because the line may contain a NUL.  *lengthP
   does not include the NUL that we add.

   If there are no more characters in the file, return NULL.
-----------------------------------------------------------------------------*/
    char * textline;  /* malloc'ed */
    unsigned int cursor;  /* cursor into textline[] */
    unsigned int allocated;
        /* The number of characters of space that are allocated for
           'textline' 
        */
    bool eol;
    bool gotSomething;

    allocated = 128;  /* initial value */

    MALLOCARRAY(textline, allocated);
    if (textline == NULL)
        pm_error("Unable to allocate buffer to read a line of a file.");
    
    cursor = 0;
    eol = FALSE;
    gotSomething = FALSE;

    while (!eol) {
        int const c = getc(fileP);
        
        if (c != EOF)
            gotSomething = TRUE;

        if (c == '\n' || c == EOF)
            eol = TRUE;
        else {
            if (cursor > allocated - 1 - 1) { /* leave space for safety NUL */
                allocated *= 2;
                REALLOCARRAY(textline, allocated);
                if (textline == NULL)
                    pm_error("Unable to allocate buffer to read a line of "
                             "a file.");
            }
            textline[cursor++] = c;
        }
    }
    textline[cursor] = '\0';  /* For safety; not part of line */

    if (gotSomething) {
        *textP = textline;
        *lengthP = cursor;
    } else {
        free(textline);
        *textP = NULL;
    }
}



static void
handleArrayAllocation(png_text **    const arrayP,
                      unsigned int * const allocatedCommentsP,
                      unsigned int   const commentIdx) {

    if (commentIdx >= *allocatedCommentsP) {
        *allocatedCommentsP *= 2;
        REALLOCARRAY(*arrayP, *allocatedCommentsP);
        if (*arrayP == NULL) 
            pm_error("unable to allocate memory for comment array");
    }
}


/******************************************************************************
                            EXTERNAL SUBROUTINES
******************************************************************************/


void 
pnmpng_read_text (png_info * const info_ptr, 
                  FILE *     const tfp, 
                  bool       const ztxt,
                  bool       const verbose) {

    const char * textline;
    unsigned int lineLength;
    unsigned int commentIdx;
    bool noCommentsYet;
    bool eof;
    unsigned int allocatedComments;
        /* Number of entries currently allocated for the info_ptr->text
           array 
        */

    allocatedComments = 256;  /* initial value */

    MALLOCARRAY(info_ptr->text, allocatedComments);
    if (info_ptr->text == NULL) 
        pm_error("unable to allocate memory for comment array");

    commentIdx = 0;
    noCommentsYet = TRUE;
   
    eof = FALSE;
    while (!eof) {
        getFileLine(tfp, &textline, &lineLength);
        if (textline == NULL)
            eof = TRUE;
        else {
            if (lineLength == 0) {
                /* skip this empty line */
            } else {
                handleArrayAllocation(&info_ptr->text, &allocatedComments,
                                      commentIdx);
                if ((textline[0] != ' ') && (textline[0] != '\t')) {
                    /* Line doesn't start with white space, which
                       means it starts a new comment.  
                    */
                    if (noCommentsYet) {
                        /* No previous comment to move past */
                    } else
                        ++commentIdx;
                    noCommentsYet = FALSE;

                    startComment(&info_ptr->text[commentIdx], 
                                 textline, lineLength, ztxt);
                } else {
                    /* Line starts with whitespace, which means it is
                       a continuation of the current comment.
                    */
                    if (noCommentsYet)
                        pm_error("Invalid comment file format: "
                                 "first line is a continuation line! "
                                 "(It starts with whitespace)");
                    continueComment(&info_ptr->text[commentIdx], 
                                    textline, lineLength);
                }
            }
            strfree(textline);
        }
    } 
    if (noCommentsYet)
        info_ptr->num_text = 0;
    else
        info_ptr->num_text = commentIdx + 1;

    if (verbose)
        pm_message("%d comments placed in text chunk", info_ptr->num_text);
}



