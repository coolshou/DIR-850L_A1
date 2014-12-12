/*===========================================================================*
 * readframe.c                    
 *                                
 *  procedures to read in frames  
 *                                
 * EXPORTED PROCEDURES:           
 *  ReadFrame                     
 *  SetFileType                   
 *  SetFileFormat                 
 *                                
 *===========================================================================*/

/* COPYRIGHT INFORMATION IS AT THE END OF THIS FILE */


/*==============*
 * HEADER FILES *
 *==============*/

#define _BSD_SOURCE   /* Make sure popen() is in stdio.h */
#include "all.h"
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "ppm.h"
#include "nstring.h"

#include "mtypes.h"
#include "frames.h"
#include "prototypes.h"
#include "parallel.h"
#include "param.h"
#include "input.h"
#include "fsize.h"
#include "rgbtoycc.h"
#include "jpeg.h"
#include "opts.h"
#include "readframe.h"


/*==================*
 * STATIC VARIABLES *
 *==================*/

static int  fileType = BASE_FILE_TYPE;
struct YuvLine {
    uint8   data[3072];
    uint8   y[1024];
    int8    cr[1024];
    int8    cb[1024];
};


/*==================*
 * Portability      *
 *==================*/
#ifdef __OS2__
  #define popen _popen
#endif
   

/*==================*
 * Global VARIABLES *
 *==================*/

extern boolean GammaCorrection;
extern float GammaValue;
extern int outputWidth,outputHeight;
boolean resizeFrame;
const char *CurrFile;

/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static void ReadEYUV _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
                 int width, int height));
static void ReadAYUV _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
                 int width, int height));
static void SeparateLine _ANSI_ARGS_((FILE *fpointer, struct YuvLine *lineptr,
                     int width));
static void ReadY _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
                 int width, int height));
static void ReadSub4 _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
                  int width, int height));
static void DoGamma  _ANSI_ARGS_((MpegFrame *mf, int width, int height));

static void DoKillDim _ANSI_ARGS_((MpegFrame *mf, int w, int h));

#define safe_fread(ptr,sz,len,fileptr)                           \
    if ((safe_read_count=fread(ptr,sz,len,fileptr))!=sz*len) {   \
      fprintf(stderr,"Input file too small! (%s)\n",CurrFile);   \
      exit(1);}                                                  \

/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/



void    
SetResize(bool const set) {
    resizeFrame = set;
}



static void
ReadPNM(MpegFrame * const mpegFrameP,
        FILE *      const fp) {
/*----------------------------------------------------------------------------
   Read a PPM file containing one movie frame.
-----------------------------------------------------------------------------*/
    int pnmCols, pnmRows;
    xelval maxval;
    xel ** xels;

    xels = ppm_readppm(fp, &pnmCols, &pnmRows, &maxval);
    ERRCHK(mpegFrameP, "ppm_readppm");

    /*
     * if this is the first frame read, set the global frame size
     */
    Fsize_Note(mpegFrameP->id, pnmCols, pnmRows);

    PNMtoYUV(mpegFrameP, xels, Fsize_x, Fsize_y, maxval);
    ppm_freearray(xels, pnmRows);
}



static void
openFile(struct inputSource * const inputSourceP,
         unsigned int         const frameNumber,
         const char *         const conversion, 
         FILE **              const ifPP) {

    if (inputSourceP->stdinUsed) {
        if (fileType == ANY_FILE_TYPE)
            pm_error(
                "ERROR : You cannot use a converter on frames when "
                "you supply them as Standard Input.  Either specify "
                "INPUT_CONVERTER * in the parameter file or supply the "
                "frames in files by specifying a directory with "
                "INPUT_DIRECTORY in the parameter file.");
        
        *ifPP = stdin;
    } else {
        const char * fileName;
        const char * fullFileName;
        
        GetNthInputFileName(inputSourceP, frameNumber, &fileName);
        
        asprintfN(&fullFileName, "%s/%s", currentPath, fileName);
        
        CurrFile = fullFileName;
        
        if (fileType == ANY_FILE_TYPE) {
            char command[1024];
            const char * convertPtr;
            char * commandPtr;
            const char * charPtr;
            
            /* replace every occurrence of '*' with fullFileName */
            convertPtr = conversion;
            commandPtr = command;
            while (*convertPtr != '\0') {
                while ((*convertPtr != '\0') && (*convertPtr != '*')) {
                    *commandPtr = *convertPtr;
                    ++commandPtr;
                    ++convertPtr;
                }
                
                if (*convertPtr == '*') {
                    /* copy fullFileName */
                    charPtr = fullFileName;
                    while (*charPtr != '\0') {
                        *commandPtr = *charPtr;
                        ++commandPtr;
                        ++charPtr;
                    }
                    ++convertPtr;   /* go past '*' */
                }
            }
            *commandPtr = '\0';
            
            *ifPP = popen(command, "r");
            if (*ifPP == NULL) {
                pm_message(
                    "ERROR:  Couldn't execute input conversion command "
                    "'%s'.  errno=%d (%s)",
                    command, errno, strerror(errno));
                if (ioServer)
                    pm_error("IO SERVER:  EXITING!!!");
                else
                    pm_error("SLAVE EXITING!!!");
            }
        } else {
            *ifPP = fopen(fullFileName, "rb");
            if (*ifPP == NULL)
                pm_error("Couldn't open input file '%s'", fullFileName);
            
            if (baseFormat == JMOVIE_FILE_TYPE)
                unlink(fullFileName);
        }
        strfree(fullFileName);
        strfree(fileName);
    }
}



static void
closeFile(struct inputSource * const inputSourceP,
          FILE *               const ifP) {

    if (!inputSourceP->stdinUsed) {
        if (fileType == ANY_FILE_TYPE) {
            int rc;
            rc = pclose(ifP);
            if (rc != 0)
                pm_message("WARNING:  pclose() failed with errno %d (%s)",
                           errno, strerror(errno));
        } else
            fclose(ifP);
    }
}



static bool
fileIsAtEnd(FILE * const ifP) {

    int c;
    bool eof;

    c = getc(ifP);
    if (c == EOF) {
        if (feof(ifP)) 
            eof = TRUE;
        else
            pm_error("File error on getc() to position to image");
    } else {
        int rc;

        eof = FALSE;

        rc = ungetc(c, ifP);
        if (rc == EOF) 
            pm_error("File error doing ungetc() to position to image.");
    }
    return eof;
}



void
ReadFrameFile(MpegFrame *  const frameP,
              FILE *       const ifP,
              const char * const conversion,
              bool *       const eofP) {
/*----------------------------------------------------------------------------
   Read a frame from the file 'ifP'.

   Return *eofP == TRUE iff we encounter EOF before we can get the
   frame.
-----------------------------------------------------------------------------*/
    MpegFrame   tempFrame;
    MpegFrame * framePtr;

    /* To make this code fit Netpbm properly, we should remove handling
       of all types except PNM and use pm_nextimage() to handle sensing
       of end of stream.
    */

    if (fileIsAtEnd(ifP))
        *eofP = TRUE;
    else {
        *eofP = FALSE;

        if (resizeFrame) {
            tempFrame.inUse = FALSE;
            tempFrame.orig_y = NULL;
            tempFrame.y_blocks = NULL;
            tempFrame.decoded_y = NULL;
            tempFrame.halfX = NULL;
            framePtr = &tempFrame;
        } else
            framePtr = frameP;

        switch(baseFormat) {
        case YUV_FILE_TYPE:

            /* Encoder YUV */
            if ((strncmp (yuvConversion, "EYUV", 4) == 0) ||
                (strncmp (yuvConversion, "UCB", 3) == 0)) 

                ReadEYUV(framePtr, ifP, realWidth, realHeight);

            else
                /* Abekas-type (interlaced) YUV */
                ReadAYUV(framePtr, ifP, realWidth, realHeight);

            break;
        case Y_FILE_TYPE:
            ReadY(framePtr, ifP, realWidth, realHeight);
            break;
        case PNM_FILE_TYPE:
            ReadPNM(framePtr, ifP);
            break;
        case SUB4_FILE_TYPE:
            ReadSub4(framePtr, ifP, yuvWidth, yuvHeight);
            break;
        case JPEG_FILE_TYPE:
        case JMOVIE_FILE_TYPE:
            ReadJPEG(framePtr, ifP);
            break;
        default:
            break;
        }

        if (resizeFrame)
            Frame_Resize(frameP, &tempFrame, Fsize_x, Fsize_y, 
                         outputWidth, outputHeight);
    
        if (GammaCorrection)
            DoGamma(frameP, Fsize_x, Fsize_y);

        if (kill_dim)
            DoKillDim(frameP, Fsize_x, Fsize_y);

        MotionSearchPreComputation(frameP);
    }
}



void
ReadFrame(MpegFrame *          const frameP, 
          struct inputSource * const inputSourceP,
          unsigned int         const frameNumber,
          const char *         const conversion,
          bool *               const endOfStreamP) {
/*----------------------------------------------------------------------------
   Read the given frame, performing conversion as necessary.
-----------------------------------------------------------------------------*/
    FILE * ifP;

    openFile(inputSourceP, frameNumber, conversion, &ifP);

    ReadFrameFile(frameP, ifP, conversion, endOfStreamP);

    if (*endOfStreamP && !inputSourceP->stdinUsed)
        pm_error("Premature EOF on file containing Frame %u", frameNumber);

    closeFile(inputSourceP, ifP);
}



/*===========================================================================*
 *
 * SetFileType
 *
 *  set the file type to be either a base type (no conversion), or
 *  any type (conversion required)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    fileType
 *
 *===========================================================================*/
void
SetFileType(const char * const conversion)
{
    if ( strcmp(conversion, "*") == 0 ) {
    fileType = BASE_FILE_TYPE;
    } else {
    fileType = ANY_FILE_TYPE;
    }
}


/*===========================================================================*
 *
 * SetFileFormat
 *
 *  set the file format (PNM, YUV, JPEG)
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    baseFormat
 *
 *===========================================================================*/
void
SetFileFormat(const char * const format)
{
    if ( strcmp(format, "PPM") == 0 ) {
    baseFormat = PNM_FILE_TYPE;
    } else if ( strcmp(format, "YUV") == 0 ) {
    baseFormat = YUV_FILE_TYPE;
    } else if ( strcmp(format, "Y") == 0 ) {
    baseFormat = Y_FILE_TYPE;
    } else if ( strcmp(format, "PNM") == 0 ) {
    baseFormat = PNM_FILE_TYPE;
    } else if (( strcmp(format, "JPEG") == 0 ) || ( strcmp(format, "JPG") == 0 )) {
    baseFormat = JPEG_FILE_TYPE;
    } else if ( strcmp(format, "JMOVIE") == 0 ) {
    baseFormat = JMOVIE_FILE_TYPE;
    } else if ( strcmp(format, "SUB4") == 0 ) {
    baseFormat = SUB4_FILE_TYPE;
    } else {
    fprintf(stderr, "ERROR:  Invalid file format:  %s\n", format);
    exit(1);
    }
}



FILE *
ReadIOConvert(struct inputSource * const inputSourceP,
              unsigned int         const frameNumber) {
/*----------------------------------------------------------------------------
   Do conversion; return handle to the appropriate file.
-----------------------------------------------------------------------------*/
    FILE    *ifp;
    char    command[1024];
    const char * fullFileName;
    char *convertPtr, *commandPtr;
    const char * fileName;

    GetNthInputFileName(inputSourceP, frameNumber, &fileName);

    asprintfN(&fullFileName, "%s/%s", currentPath, fileName);

    if ( strcmp(ioConversion, "*") == 0 ) {
        char buff[1024];
        ifp = fopen(fullFileName, "rb");
        sprintf(buff,"fopen \"%s\"",fullFileName);
        ERRCHK(ifp, buff);
        return ifp;
    }

    /* replace every occurrence of '*' with fullFileName */
    convertPtr = ioConversion;
    commandPtr = command;
    while ( *convertPtr != '\0' ) {
        while ( (*convertPtr != '\0') && (*convertPtr != '*') ) {
            *commandPtr = *convertPtr;
            commandPtr++;
            convertPtr++;
        }

        if ( *convertPtr == '*' ) {
            /* copy fullFileName */
            const char * charPtr;
            charPtr = fullFileName;
            while ( *charPtr != '\0' ) {
                *commandPtr = *charPtr;
                commandPtr++;
                charPtr++;
            }

            convertPtr++;   /* go past '*' */
        }
    }
    *commandPtr = '\0';

    if ( (ifp = popen(command, "r")) == NULL ) {
        fprintf(stderr, "ERROR:  "
                "Couldn't execute input conversion command:\n");
        fprintf(stderr, "\t%s\n", command);
        fprintf(stderr, "errno = %d\n", errno);
        if ( ioServer ) {
            fprintf(stderr, "IO SERVER:  EXITING!!!\n");
        } else {
            fprintf(stderr, "SLAVE EXITING!!!\n");
        }
        exit(1);
    }

    strfree(fullFileName);
    strfree(fileName);

    return ifp;
}



/*===========================================================================*
 *
 * ReadEYUV
 *
 *  read a Encoder-YUV file (concatenated Y, U, and V)
 *
 * RETURNS: mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadEYUV(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int y;
    uint8   junk[4096];
    int     safe_read_count;

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < Fsize_y; y++) {         /* Y */
    safe_fread(mf->orig_y[y], 1, Fsize_x, fpointer);

    /* read the leftover stuff on the right side */
    if ( width != Fsize_x ) {
        safe_fread(junk, 1, width-Fsize_x, fpointer);
    }
    }

    /* read the leftover stuff on the bottom */
    for (y = Fsize_y; y < height; y++) {
    safe_fread(junk, 1, width, fpointer);
    }

    for (y = 0; y < (Fsize_y >> 1); y++) {          /* U */
    safe_fread(mf->orig_cb[y], 1, Fsize_x >> 1, fpointer);

    /* read the leftover stuff on the right side */
    if ( width != Fsize_x ) {
        safe_fread(junk, 1, (width-Fsize_x)>>1, fpointer);
    }
    }

    /* read the leftover stuff on the bottom */
    for (y = (Fsize_y >> 1); y < (height >> 1); y++) {
    safe_fread(junk, 1, width>>1, fpointer);
    }

    for (y = 0; y < (Fsize_y >> 1); y++) {          /* V */
    safe_fread(mf->orig_cr[y], 1, Fsize_x >> 1, fpointer);

    /* read the leftover stuff on the right side */
    if ( width != Fsize_x ) {
        safe_fread(junk, 1, (width-Fsize_x)>>1, fpointer);
    }
    }

    /* ignore leftover stuff on the bottom */
}

/*===========================================================================*
 *
 * ReadAYUV
 *
 *  read an Abekas-YUV file
 *
 * RETURNS: mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadAYUV(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int x, y;
    struct  YuvLine line1, line2;
    uint8   junk[4096];
    uint8    *cbptr, *crptr;
    int     safe_read_count;

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < Fsize_y; y += 2) {
    SeparateLine(fpointer, &line1, width);
    SeparateLine(fpointer, &line2, width);

    /* Copy the Y values for each line to the frame */
    for (x = 0; x < Fsize_x; x++) {
        mf->orig_y[y][x]   = line1.y[x];
        mf->orig_y[y+1][x] = line2.y[x];
    }

    cbptr = &(mf->orig_cb[y>>1][0]);
    crptr = &(mf->orig_cr[y>>1][0]);

    /* One U and one V for each two pixels horizontal as well */
    /* Toss the second line of Cr/Cb info, averaging was worse,
       so just subsample */
    for (x = 0; x < (Fsize_x >> 1); x ++) {
        cbptr[x] =  line1.cb[x];
        crptr[x] =  line1.cr[x];

    }
    }

    /* read the leftover stuff on the bottom */
    for (y = Fsize_y; y < height; y++) {
    safe_fread(junk, 1, width<<1, fpointer);
    }

}

/*===========================================================================*
 *
 * SeparateLine
 *
 *  Separates one line of pixels into Y, U, and V components
 *
 * RETURNS: lineptr modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
SeparateLine(fpointer, lineptr, width)
    FILE *fpointer;
    struct YuvLine *lineptr;
    int width;
{
    uint8   junk[4096];
    int8    *crptr, *cbptr;
    uint8   *yptr;
    int     num, length;
    int     safe_read_count;


    /* Sets the deinterlacing pattern */

    /* shorthand for UYVY */
    if (strncmp(yuvConversion, "ABEKAS", 6) == 0) {
    strcpy(yuvConversion, "UYVY");

    /* shorthand for YUYV */
    } else if (strncmp(yuvConversion, "PHILLIPS", 8) == 0) {
    strcpy(yuvConversion, "YUYV");
    }

    length = strlen (yuvConversion);

    if ((length % 2) != 0) {
    fprintf (stderr, "ERROR : YUV_FORMAT must represent two pixels, hence must be even in length.\n");
    exit(1);
    }

    /* each line in 4:2:2 chroma format takes 2X bytes to represent X pixels.
     * each line in 4:4:4 chroma format takes 3X bytes to represent X pixels.
     * Therefore, half of the length of the YUV_FORMAT represents 1 pixel.
     */
    safe_fread(lineptr->data, 1, Fsize_x*(length>>1), fpointer);

    /* read the leftover stuff on the right side */
    if ( width != Fsize_x ) {
    safe_fread(junk, 1, (width-Fsize_x)*(length>>1), fpointer);
    }

    crptr = &(lineptr->cr[0]);
    cbptr = &(lineptr->cb[0]);
    yptr = &(lineptr->y[0]);

    for (num = 0; num < (Fsize_x*(length>>1)); num++) {
    switch (yuvConversion[num % length]) {
    case 'U':
    case 'u':
        *(cbptr++) = (lineptr->data[num]);
        break;
    case 'V':
    case 'v':
        *(crptr++) = (lineptr->data[num]);
        break;
    case 'Y':
    case 'y':
        *(yptr++) = (lineptr->data[num]);
        break;
    default:
            fprintf(stderr, "ERROR: YUV_FORMAT must be one of the following:\n");
            fprintf(stderr, "       ABEKAS\n");
            fprintf(stderr, "       EYUV\n");
            fprintf(stderr, "       PHILLIPS\n");
            fprintf(stderr, "       UCB\n");
        fprintf(stderr, "       or any even-length string consisting of the letters U, V, and Y.\n");
            exit(1);
        }
    
    }

}


/*===========================================================================*
 *
 * ReadY
 *
 *  read a Y file
 *
 * RETURNS: mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadY(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int y;
    uint8   junk[4096];
    int     safe_read_count;

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < Fsize_y; y++) {         /* Y */
    safe_fread(mf->orig_y[y], 1, Fsize_x, fpointer);

    /* read the leftover stuff on the right side */
    if ( width != Fsize_x ) {
        safe_fread(junk, 1, width-Fsize_x, fpointer);
    }
    }

    /* read the leftover stuff on the bottom */
    for (y = Fsize_y; y < height; y++) {
    safe_fread(junk, 1, width, fpointer);
    }
    
    for (y = 0 ; y < (Fsize_y >> 1); y++) {
      memset(mf->orig_cb[y], 128, (Fsize_x>>1));
      memset(mf->orig_cr[y], 128, (Fsize_x>>1));
    }
}


/*===========================================================================*
 *
 * ReadSub4
 *
 *  read a YUV file (subsampled even further by 4:1 ratio)
 *
 * RETURNS: mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadSub4(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int y;
    register int x;
    uint8   buffer[1024];
    int     safe_read_count;

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < (height>>1); y++) {         /* Y */
    safe_fread(buffer, 1, width>>1, fpointer);
    for ( x = 0; x < (width>>1); x++ ) {
        mf->orig_y[2*y][2*x] = buffer[x];
        mf->orig_y[2*y][2*x+1] = buffer[x];
        mf->orig_y[2*y+1][2*x] = buffer[x];
        mf->orig_y[2*y+1][2*x+1] = buffer[x];
    }
    }

    for (y = 0; y < (height >> 2); y++) {           /* U */
    safe_fread(buffer, 1, width>>2, fpointer);
    for ( x = 0; x < (width>>2); x++ ) {
        mf->orig_cb[2*y][2*x] = buffer[x];
        mf->orig_cb[2*y][2*x+1] = buffer[x];
        mf->orig_cb[2*y+1][2*x] = buffer[x];
        mf->orig_cb[2*y+1][2*x+1] = buffer[x];
    }
    }

    for (y = 0; y < (height >> 2); y++) {           /* V */
    safe_fread(buffer, 1, width>>2, fpointer);
    for ( x = 0; x < (width>>2); x++ ) {
        mf->orig_cr[2*y][2*x] = buffer[x];
        mf->orig_cr[2*y][2*x+1] = buffer[x];
        mf->orig_cr[2*y+1][2*x] = buffer[x];
        mf->orig_cr[2*y+1][2*x+1] = buffer[x];
    }
    }
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * DoGamma
 *
 *  Gamma Correct the Lum values
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    Raises Y values to power gamma.
 *
 *===========================================================================*/
static void
DoGamma(mf, w, h)
MpegFrame *mf;
int w,h;
{
  static int GammaVal[256];
  static boolean init_done=FALSE;
  int i,j;

  if (!init_done) {
    for(i=0; i<256; i++) 
      GammaVal[i]=(unsigned char) (pow(((double) i)/255.0,GammaValue)*255.0+0.5);
    init_done=TRUE;
  }

  for (i=0; i< h; i++) {  /* For each line */
    for (j=0; j<w; j++) { /* For each Y value */
      mf->orig_y[i][j] = GammaVal[mf->orig_y[i][j]];
    }}
}




/*===========================================================================*
 *
 * DoKillDim
 *
 *  Applies an input filter to small Y values.
 *
 * RETURNS: nothing
 *
 * SIDE EFFECTS:    Changes Y values:
 *
 *  Output    |                 /
              |                /
              |               /
              |              !
              |             /
              |            !
              |           /
              |          -
              |        /
              |      --
              |     /
              |   --
              | /
              ------------------------
                        ^ kill_dim_break
                             ^kill_dim_end
              kill_dim_slope gives the slope (y = kill_dim_slope * x +0)
              from 0 to kill_dim_break                      
 *
 *===========================================================================*/

static void
DoKillDim(mf, w, h)
MpegFrame *mf;
int w,h;
{
  static boolean init_done=FALSE;
  static unsigned char mapper[256];
  register int i,j;
  double slope, intercept;

  slope = (kill_dim_end - kill_dim_break*kill_dim_slope)*1.0 /
    (kill_dim_end - kill_dim_break);
  intercept = kill_dim_end * (1.0-slope);

  if (!init_done) {
    for(i=0; i<256; i++) {
      if (i >= kill_dim_end) {
        mapper[i] = (char) i;
      } else if (i >= kill_dim_break) {
        mapper[i] = (char) (slope*i + intercept);
      } else { /* i <= kill_dim_break */
        mapper[i] = (char) floor(i*kill_dim_slope + 0.49999);
      }
    }
    init_done = TRUE;
  }

  for (i=0;  i < h;  i++) {  /* For each line */
    for (j=0;   j < w;   j++) { /* For each Y value */
      mf->orig_y[i][j] = mapper[mf->orig_y[i][j]];
    }}
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

/*  
 *  $Header: /n/picasso/project/mpeg/mpeg_dist/mpeg_encode/RCS/readframe.c,v 1.27 1995/08/14 22:31:40 smoot Exp $
 *  $Log: readframe.c,v $
 *  Revision 1.27  1995/08/14 22:31:40  smoot
 *  reads training info from PPms now (needed for piping reads)
 *
 *  Revision 1.26  1995/08/07 21:48:36  smoot
 *  better error reporting, JPG == JPEG now
 *
 *  Revision 1.25  1995/06/12 20:30:12  smoot
 *  added popen for OS2
 *
 * Revision 1.24  1995/06/08  20:34:36  smoot
 * added "b"'s to fopen calls to make MSDOS happy
 *
 * Revision 1.23  1995/05/03  10:16:01  smoot
 * minor compile bug with static f
 *
 * Revision 1.22  1995/05/02  22:00:12  smoot
 * added TUNEing, setting near-black values to black
 *
 * Revision 1.21  1995/03/27  21:00:01  eyhung
 * fixed bug with some long jpeg names
 *
 * Revision 1.20  1995/02/02  01:05:54  eyhung
 * Fixed aAdded error checking for stdin
 *
 * Revision 1.19  1995/02/01  05:01:12  eyhung
 * Removed troubleshooting printf
 *
 * Revision 1.18  1995/01/31  21:08:16  eyhung
 * Improved YUV_FORMAT strings with better algorithm
 *
 * Revision 1.17  1995/01/27  23:34:09  eyhung
 * Removed temporary JPEG files created by JMOVIE input
 *
 * Revision 1.16  1995/01/27  21:57:43  eyhung
 * Added case for reading original JMOVIES
 *
 * Revision 1.14  1995/01/24  23:47:51  eyhung
 * Confusion with Abekas format fixed : all other YUV revisions are wrong
 *
 * Revision 1.13  1995/01/20  00:02:30  smoot
 * added gamma correction
 *
 * Revision 1.12  1995/01/19  23:09:21  eyhung
 * Changed copyrights
 *
 * Revision 1.11  1995/01/17  22:23:07  aswan
 * AbekasYUV chrominance implementation fixed
 *
 * Revision 1.10  1995/01/17  21:26:25  smoot
 * Tore our average on Abekus/Phillips reconstruct
 *
 * Revision 1.9  1995/01/17  08:22:34  eyhung
 * Debugging of ReadAYUV
 *
 * Revision 1.8  1995/01/16  13:18:24  eyhung
 * Interlaced YUV format (e.g. Abekas) capability added (slightly buggy)
 *
 * Revision 1.7  1995/01/16  06:58:23  eyhung
 * Added skeleton of ReadAYUV (for Abekas YUV files)
 *
 * Revision 1.6  1995/01/13  23:22:23  smoot
 * Added ReadY, so we can make black&white movies (how artsy!)
 *
 * Revision 1.5  1994/12/16  00:20:40  smoot
 * Now errors out on too small an input file
 *
 * Revision 1.4  1994/11/12  02:11:59  keving
 * nothing
 *
 * Revision 1.3  1994/03/15  00:27:11  keving
 * nothing
 *
 * Revision 1.2  1993/12/22  19:19:01  keving
 * nothing
 *
 * Revision 1.1  1993/07/22  22:23:43  keving
 * nothing
 *
 */
