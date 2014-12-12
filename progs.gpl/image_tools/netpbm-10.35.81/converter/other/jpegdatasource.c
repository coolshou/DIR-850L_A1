/*****************************************************************************
                           jpegdatasource.c
******************************************************************************
   This is the data source manager that Jpegtopnm supplies to the JPEG
   library.  The Jpeg library uses this object to get JPEG input.

   This data source manager is the same as the Jpeg library's built in
   "stdio" one, except that it looks ahead and one can query it to see
   if there is any data in the stream that the Jpeg library hasn't seen
   yet.  Thus, you can use it in a program that reads multiple JPEG 
   images and know when to stop.  You can also nicely handle completely
   empty input files more gracefully than just crying input error.

   This data source manager does 4K fread() reads and passes 4K buffers 
   to the Jpeg library.  It reads one 4K block ahead, so there is up to
   8K of image buffered at any time.

   By Bryan Henderson, San Jose CA 2002.10.13
*****************************************************************************/

#include <ctype.h>	   
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
/* Note: jpeglib.h prerequires stdlib.h and ctype.h.  It should include them
   itself, but doesn't.
*/
#include <jpeglib.h>

#include "pm_c_util.h"
#include "mallocvar.h"
#include "pnm.h"

#include "jpegdatasource.h"

#define BUFFER_SIZE 4096

struct sourceManager {
    /* The following structure _must_ be the first thing in this structure,
       because the methods below assume the address of the struct
       jpeg_source_mgr is the same as the address of the struct sourceManager.
       The former address is the only information the Jpeg library gives
       these methods to tell them on what source manager object they are
       supposed to operate.
    */
    struct jpeg_source_mgr jpegSourceMgr;
    FILE * ifP;
    JOCTET * currentBuffer;
    JOCTET * nextBuffer;
    unsigned int bytesInNextBuffer;
    JOCTET buffer1[BUFFER_SIZE];
    JOCTET buffer2[BUFFER_SIZE];
};


static void
dsInitSource(j_decompress_ptr const cinfoP) {
/*----------------------------------------------------------------------------
   This is the init_source method for the data source manager the Jpeg
   library uses.
-----------------------------------------------------------------------------*/
    /* The object was created ready to go and remains ready from one image
       to the next.  No per-image initialization is necessary.
    */
}



static boolean
dsFillInputBuffer(j_decompress_ptr const cinfoP) {
/*----------------------------------------------------------------------------
   This is the fill_input_buffer method for the data source manager the Jpeg
   library uses.
-----------------------------------------------------------------------------*/
    struct sourceManager * const srcP = (struct sourceManager *) cinfoP->src;

    if (srcP->bytesInNextBuffer == 0) 
        pm_error("End-of-file encountered in the middle of JPEG image.");
    else {
        /* Rotate the buffers */
        srcP->jpegSourceMgr.next_input_byte = srcP->nextBuffer;
        srcP->jpegSourceMgr.bytes_in_buffer = srcP->bytesInNextBuffer;
        {
            JOCTET * tmp;
            tmp = srcP->nextBuffer;
            srcP->nextBuffer = srcP->currentBuffer;
            srcP->currentBuffer = tmp;
        }

        /* Fill the new 'next' buffer */
        srcP->bytesInNextBuffer = 
            fread(srcP->nextBuffer, 1, BUFFER_SIZE, srcP->ifP);
    }
    return TRUE;
}



static void
dsSkipInputData(j_decompress_ptr const cinfoP, long const num_bytes) {
/*----------------------------------------------------------------------------
   This is the skip_input_data method for the data source manager the Jpeg
   library uses.
-----------------------------------------------------------------------------*/
    struct jpeg_source_mgr * const jpegSourceMgrP = cinfoP->src;

    long i;

    for (i = 0; i < num_bytes; ++i) {
        if (jpegSourceMgrP->bytes_in_buffer == 0)
            dsFillInputBuffer(cinfoP);
        ++jpegSourceMgrP->next_input_byte;
        --jpegSourceMgrP->bytes_in_buffer;
    }
}



static void
dsTermSource(j_decompress_ptr const cinfoP) {
/*----------------------------------------------------------------------------
   This is the term_source method for the data source manager the Jpeg
   library uses.
-----------------------------------------------------------------------------*/
    /* We couldn't care less that the Jpeg library is done reading an
       image.  The source manager object remains active and ready for the
       Jpeg library to read the next image.
    */
}



bool
dsDataLeft(struct sourceManager * const srcP) {

    return((srcP->jpegSourceMgr.bytes_in_buffer > 0 ||
            srcP->bytesInNextBuffer > 0));
}



struct sourceManager * 
dsCreateSource(const char * const fileName) {

    struct sourceManager * srcP;

    MALLOCVAR(srcP);
    if (srcP == NULL)
        pm_error("Unable to get memory for the Jpeg library source manager.");

    srcP->ifP = pm_openr(fileName);

    srcP->jpegSourceMgr.init_source = dsInitSource;
    srcP->jpegSourceMgr.fill_input_buffer = dsFillInputBuffer;
    srcP->jpegSourceMgr.skip_input_data = dsSkipInputData;
    srcP->jpegSourceMgr.resync_to_restart = jpeg_resync_to_restart;
    srcP->jpegSourceMgr.term_source = dsTermSource;
    
    srcP->currentBuffer = srcP->buffer1;
    srcP->nextBuffer = srcP->buffer2;
    srcP->jpegSourceMgr.bytes_in_buffer = 
        fread(srcP->currentBuffer, 1, BUFFER_SIZE, srcP->ifP);
    srcP->jpegSourceMgr.next_input_byte = srcP->currentBuffer;
    srcP->bytesInNextBuffer = 
        fread(srcP->nextBuffer, 1, BUFFER_SIZE, srcP->ifP);

    return srcP;
}

void
dsDestroySource(struct sourceManager * const srcP) {
    
    pm_close(srcP->ifP);
    free(srcP);

}



struct jpeg_source_mgr *
dsJpegSourceMgr(struct sourceManager * const srcP) {
    return &srcP->jpegSourceMgr;
}
