/*===========================================================================*
 * input.c              
 *                      
 *  Stuff for getting the raw frame input for the program
 *                                    
 *===========================================================================*/

/* COPYRIGHT INFORMATION IS AT THE END OF THIS FILE */

#define _XOPEN_SOURCE 1
    /* This makes sure popen() is in stdio.h.  In GNU libc 2.1.3, 
     _POSIX_C_SOURCE = 2 is sufficient, but on AIX 4.3, the higher level
     _XOPEN_SOURCE is required.  2000.09.09 
    */


/*==============*
 * HEADER FILES *
 *==============*/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "nstring.h"
#include "mallocvar.h"
#include "parallel.h"
#include "readframe.h"
#include "param.h"
#include "jpeg.h"
#include "input.h"

extern boolean realQuiet;	/* TRUE = no messages to stdout */

struct InputFileEntry {
    char    left[256];
    char    right[256];
    bool    glob;       /* if FALSE, left is complete name */
    int     startID;
    int     endID;
    int     skip;
    int     numPadding;     /* -1 if there is none */
    int     numFiles;
    bool    repeat;
};


struct inputSource inputSource;



void
GetNthInputFileName(struct inputSource * const inputSourceP,
                    unsigned int         const n,
                    const char **        const fileNameP) {
/*----------------------------------------------------------------------------
   Return the file name of the Nth input file.
-----------------------------------------------------------------------------*/
    static int	lastN = 0, lastMapN = 0, lastSoFar = 0;
    int	    mapN;
    int     index;
    int	    soFar;
    int	    numPadding;
    struct InputFileEntry * mapNEntryP;

    assert(!inputSourceP->stdinUsed);
    assert(n < inputSourceP->numInputFiles);

    if (n >= lastN) {
        soFar = lastSoFar;
        index = lastMapN;
    } else {
        soFar = 0;
        index = 0;
    }

    assert(index < inputSourceP->numInputFileEntries);

    while (soFar + inputSourceP->inputFileEntries[index]->numFiles <= n) {
        soFar +=  inputSourceP->inputFileEntries[index]->numFiles;
        ++index;
        assert(index < inputSourceP->numInputFileEntries);
    }

    mapN = index;
    
    mapNEntryP = inputSourceP->inputFileEntries[mapN];

    index = mapNEntryP->startID + mapNEntryP->skip*(n - soFar);

    numPadding = mapNEntryP->numPadding;

    if (numPadding != -1) {
        char    numBuffer[33];
        int	    loop;

        sprintf(numBuffer, "%32d", index);
        for (loop = 32-numPadding; loop < 32; ++loop) {
            if (numBuffer[loop] != ' ')
                break;
            else
                numBuffer[loop] = '0';
        }

        if (mapNEntryP->repeat != TRUE)
            asprintfN(fileNameP, "%s%s%s",
                      mapNEntryP->left, &numBuffer[32-numPadding],
                      mapNEntryP->right);
        else
            asprintfN(fileNameP, "%s", mapNEntryP->left);
    } else {
        if (mapNEntryP->repeat != TRUE)
            asprintfN(fileNameP, "%s%d%s",
                      mapNEntryP->left, index, mapNEntryP->right);
        else
            asprintfN(fileNameP, "%s", mapNEntryP->left);
    }

    lastN = n;
    lastMapN = mapN;
    lastSoFar = soFar;
}



void
ReadNthFrame(struct inputSource * const inputSourceP,
             unsigned int         const frameNumber,
             boolean              const remoteIO,
             boolean              const childProcess,
             boolean              const separateConversion,
             const char *         const slaveConversion,
             const char *         const inputConversion,
             MpegFrame *          const frameP,
             bool *               const endOfStreamP) {

    if (remoteIO)
        /* Get the frame from the remote I/O server */
        GetRemoteFrame(frameP, frameNumber);
    else {
        /* Get the frame out of the file in which it lives */

        const char * conversion;

        if (childProcess && separateConversion)
            conversion = slaveConversion;
        else
            conversion = inputConversion;

        ReadFrame(frameP, inputSourceP, frameNumber, conversion, endOfStreamP);
    }
}



/* Jim Boucher's code */

void
JM2JPEG(struct inputSource * const inputSourceP) {
    char full_path[1024];
    char inter_file[1024]; 
    int ci;
    
    for(ci = 0; ci < inputSourceP->numInputFileEntries; ci++) {
        inter_file[0] = '\0';
        full_path[0] = '\0';
        strcpy(full_path, currentPath);
    
        if (!inputSource.stdinUsed) {
            strcat(full_path, "/");
            strcat(full_path, inputSourceP->inputFileEntries[ci]->left);
            strcpy(inter_file,full_path);
    
            if (!realQuiet)
                fprintf(stdout, "Extracting JPEG's in the JMOVIE from %s\n",
                        full_path);
    
            JMovie2JPEG(full_path,
                        inter_file,
                        inputSourceP->inputFileEntries[ci]->startID, 
                        inputSourceP->inputFileEntries[ci]->endID);
        } else
            pm_error("ERROR: Cannot use JMovie format on Standard Input");
    }
}



static const char *
SkipSpacesTabs(const char * const start) {

    const char * p;

    for (p = start; *p == ' ' || *p == '\t'; ++p);

    return p;
}



static void
processGlob(const char *            const input, 
            struct InputFileEntry * const inputFileEntryP) {

    const char *globPtr;
    char *  charPtr;
    char    left[256], right[256];
    char    leftNumText[256], rightNumText[256];
    char    skipNumText[256];
    int	    leftNum, rightNum;
    int	    skipNum;
    boolean padding;
    int	    numPadding = 0;

    inputFileEntryP->glob = TRUE;
    inputFileEntryP->repeat = FALSE;

    /* star expand */

    globPtr = input;
    charPtr = left;
    /* copy left of '*' */
    while ( (*globPtr != '\0') && (*globPtr != '*') ) {
        *charPtr = *globPtr;
        charPtr++;
        globPtr++;
    }
    *charPtr = '\0';

    if (*globPtr == '\0') {
        fprintf(stderr, 
                "WARNING: expanding non-star regular expression\n");
        inputFileEntryP->repeat = TRUE;
        globPtr = input;
        charPtr = left;
        /* recopy left of whitespace */
        while ( (*globPtr != '\0') && (*globPtr != '*') && 
                (*globPtr != ' ')  && (*globPtr != '\t')) {
            *charPtr = *globPtr;
            charPtr++;
            globPtr++;
        }
        *charPtr = '\0';
        *right = '\0';
    } else {

        globPtr++;
        charPtr = right;
        /* copy right of '*' */
        while ( (*globPtr != '\0') && (*globPtr != ' ') &&
                (*globPtr != '\t') ) {
            *charPtr = *globPtr;
            charPtr++;
            globPtr++;
        }
        *charPtr = '\0';
    }
      
    globPtr = SkipSpacesTabs(globPtr);

    if ( *globPtr != '[' ) {
        fprintf(stderr, 
                "ERROR:  "
                "Invalid input file expansion expression (no '[')\n");
        exit(1);
    }

    globPtr++;
    charPtr = leftNumText;
    /* copy left number */
    while ( ISDIGIT(*globPtr) ) {
        *charPtr = *globPtr;
        charPtr++;
        globPtr++;
    }
    *charPtr = '\0';

    if ( *globPtr != '-' ) {
        fprintf(stderr, 
                "ERROR:  "
                "Invalid input file expansion expression (no '-')\n");
        exit(1);
    }

    globPtr++;
    charPtr = rightNumText;
    /* copy right number */
    while ( ISDIGIT(*globPtr) ) {
        *charPtr = *globPtr;
        charPtr++;
        globPtr++;
    }
    *charPtr = '\0';
    if ( atoi(rightNumText) < atoi(leftNumText) ) {
        fprintf(stderr, 
                "ERROR:  "
                "Beginning of input range is higher than end.\n");
        exit(1);
    }


    if ( *globPtr != ']' ) {
        if ( *globPtr != '+' ) {
            fprintf(stderr, 
                    "ERROR:  "
                    "Invalid input file expansion expression "
                    "(no ']')\n");
            exit(1);
        }

        globPtr++;
        charPtr = skipNumText;
        /* copy skip number */
        while ( ISDIGIT(*globPtr) ) {
            *charPtr = *globPtr;
            charPtr++;
            globPtr++;
        }
        *charPtr = '\0';

        if ( *globPtr != ']' ) {
            fprintf(stderr, 
                    "ERROR:  Invalid input file expansion expression "
                    "(no ']')\n");
            exit(1);
        }

        skipNum = atoi(skipNumText);
    } else {
        skipNum = 1;
    }

    leftNum = atoi(leftNumText);
    rightNum = atoi(rightNumText);

    if ( (leftNumText[0] == '0') && (leftNumText[1] != '\0') ) {
        padding = TRUE;
        numPadding = strlen(leftNumText);
    } else {
        padding = FALSE;
    }

    inputFileEntryP->startID = leftNum;
    inputFileEntryP->endID = rightNum;
    inputFileEntryP->skip = skipNum;
    inputFileEntryP->numFiles = 
        (rightNum-leftNum+1)/skipNum;
    strcpy(inputFileEntryP->left, left);
    strcpy(inputFileEntryP->right, right);
    if ( padding ) {
        inputFileEntryP->numPadding = numPadding;
    } else {
        inputFileEntryP->numPadding = -1;
    }
}



static void processJmovie(const char *            const input, 
                          struct InputFileEntry * const inputFileEntryP) {
    FILE    *jmovie;
    char    full_path[1024];

    inputFileEntryP->glob = TRUE;
    full_path[0] = '\0';
    strcpy(full_path, currentPath);
    
    strcat(full_path, "/");
    strcat(full_path, input);
    jmovie = fopen(input, "rb"); 
    
    if (jmovie == NULL) {
        perror (input); 
        exit (1);
    }
    
    fseek (jmovie, (8*sizeof(char)), 0);
    fseek (jmovie, (2*sizeof(int)), 1);
    
    if (fread(&(inputFileEntryP->numFiles),
              sizeof(int), 1, jmovie) != 1) {
        perror ("Error in reading number of frames in JMOVIE");
        exit(1);
    }
    fclose (jmovie);

    strcpy(inputFileEntryP->right,".jpg");
    inputFileEntryP->numPadding = -1;
    inputFileEntryP->startID = 1;
    inputFileEntryP->endID = (inputFileEntryP->numFiles-1);
    inputFileEntryP->skip = 1;
    if (! realQuiet) {
        fprintf (stdout, 
                 "Encoding all %d frames from JMOVIE.\n", 
                 inputFileEntryP->endID);
    }
} 



static void
processSimpleFileName(const char *            const input,
                      struct InputFileEntry * const inputFileEntryP) {

    inputFileEntryP->glob = FALSE;
    inputFileEntryP->numFiles = 1;
    /* fixes a bug from version 1.3: */
    inputFileEntryP->numPadding = 0;
    /* fixes a bug from version 1.4 */
    strcpy(inputFileEntryP->right,"\0");
    inputFileEntryP->startID = 0;
    inputFileEntryP->endID = 0;
    inputFileEntryP->skip = 0;
}


#define INPUT_ENTRY_BLOCK_SIZE   128

void
AddInputFiles(struct inputSource * const inputSourceP,
              const char *         const input) {

    unsigned int const currentIndex = inputSourceP->numInputFileEntries;

    struct InputFileEntry * currentEntryP;

    if (currentIndex >= inputSourceP->ifArraySize) {
        /* Get more space */
        inputSourceP->ifArraySize += INPUT_ENTRY_BLOCK_SIZE;
        REALLOCARRAY_NOFAIL(inputSourceP->inputFileEntries, 
                            inputSourceP->ifArraySize);
    }
    MALLOCVAR_NOFAIL(inputSourceP->inputFileEntries[currentIndex]);
    currentEntryP = inputSourceP->inputFileEntries[currentIndex];

    if (input[strlen(input)-1] == ']') 
        processGlob(input, currentEntryP);
    else {
        strcpy(currentEntryP->left, input);
        if (baseFormat == JMOVIE_FILE_TYPE) 
            processJmovie(input, currentEntryP);
        else 
            processSimpleFileName(input, currentEntryP);
    }
    inputSourceP->numInputFiles += currentEntryP->numFiles;
    ++inputSourceP->numInputFileEntries;
}



void
SetStdinInput(struct inputSource * const inputSourceP) {

    assert(inputSourceP->numInputFileEntries == 0);

    inputSourceP->stdinUsed = TRUE;
    inputSourceP->numInputFiles = INT_MAX;
}



void
CreateInputSource(struct inputSource ** const inputSourcePP) {

    struct inputSource * inputSourceP;

    MALLOCVAR_NOFAIL(inputSourceP);

    inputSourceP->stdinUsed = FALSE;
    inputSourceP->numInputFileEntries = 0;
    inputSourceP->ifArraySize = 1;
    inputSourceP->numInputFiles = 0;
    MALLOCARRAY_NOFAIL(inputSourceP->inputFileEntries, 1);
    
    *inputSourcePP = inputSourceP;
}



void
DestroyInputSource(struct inputSource * const inputSourceP) {

    unsigned int i;

    for (i = 0; i < inputSourceP->numInputFileEntries; ++i)
        free(inputSourceP->inputFileEntries[i]);

    free(inputSourceP->inputFileEntries);

    free(inputSourceP);
}



/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
