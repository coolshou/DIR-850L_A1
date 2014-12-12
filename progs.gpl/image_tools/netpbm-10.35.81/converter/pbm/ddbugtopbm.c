/* ddbugtopbm - convert Diddle/DiddleBug sketch DB to PBM files.
 * Copyright (c) 2002 Russell Marks. See COPYING for licence details.
 *
 * The decompression code (uncompress_sketch() is directly from
 * DiddleBug itself, which like ddbugtopbm is distributed under the
 * terms of the GNU GPL. As such, the following copyright applies to
 * uncompress_sketch():
 *
 * Copyright (c) 1999,2000 Mitch Blevins <mblevin@debian.org>.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 *
 * Adapted to Netpbm by Bryan Henderson 2003.08.09.  Bryan got his copy
 * from ftp:ibiblio.com/pub/linux/apps/graphics/convert, dated 2002.08.21.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pm_c_util.h"
#include "pbm.h"
#include "mallocvar.h"

/* this is basically UncompressSketch() from DiddleBug 2.50's diddlebug.c */
static void 
uncompress_sketch(unsigned char * const cPtr,
                  unsigned char * const uPtr,
                  int             const size) {
    int i, j;
    unsigned char num_bytes;

    j = 0;
    for (i=0; i<size; ++i) {
        if (cPtr[i]&0x80) {
            num_bytes = cPtr[i]&0x3f;  /* ~(0x40|0x80) */
            ++num_bytes;
            if (cPtr[i]&0x40) {
                /* Mixed */
                memmove(uPtr+j, cPtr+i+1, num_bytes);
                i += num_bytes;
            } else {
                /* Black */
                memset(uPtr+j, 0xff, num_bytes);
            }
        } else {
            /* White */
            num_bytes = cPtr[i]&0x7f; /* ~0x80 */
            ++num_bytes;
            memset(uPtr+j, 0x00, num_bytes);
        }
        j += num_bytes;
    }
}



static const char *
make_noname(void) {
    static char name[128];
    static int num=0;
    FILE *out;

    out = NULL;

    do {
        num++;
        if (out != NULL) 
            fclose(out);
        sprintf(name, "sketch-%04d.pbm", num);
    } while (num<10000 && (out = fopen(name, "rb")) != NULL);

    if (num>=10000)
        pm_error("too many unnamed sketches!");

    return(name);
}



int 
main(int argc, char ** argv) {
    FILE * const in=stdin;

    static unsigned char buf[64*1024];
    int *recoffsets;
    int f;
    int numrecs;
    bool is_diddle;

    pbm_init(&argc, argv);

    if (argc-1 > 0)
        pm_error("Program takes no arguments.");

    /* main DB header */
    fread(buf, 1, 64, in);

    fread(buf, 1, 14, in);
    if (memcmp(buf, "DIDL", 4) != 0 && memcmp(buf, "DIDB", 4) !=0 )
        pm_error("not a Diddle or DiddleBug DB.");
    is_diddle = (memcmp(buf, "DIDL", 4) == 0);
    numrecs = buf[12]*256+buf[13];

    MALLOCARRAY_NOFAIL(recoffsets, numrecs);

    for (f = 0; f < numrecs; ++f) {
        fread(buf, 1, 8, in);
        recoffsets[f] = ((buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3]);
    }

    for (f = 0; f < numrecs; ++f) {
        static unsigned char obuf[64*1024];
        char * nameptr;
        char * ptr;
        const char * outfilename;
        FILE *out;

        fseek(in, recoffsets[f], SEEK_SET);
        fread(buf, 1, 16*1024, in);  /* XXX crappy! */

        if (is_diddle) {
            /* Diddle */
            memcpy(obuf, buf, 160*160/8);
            nameptr=(char*)(buf+(160*160/8));
        } else {
            /* DiddleBug */
            int const sketchlen = buf[8]*256+buf[9];
            uncompress_sketch(buf+26+80+1, obuf, sketchlen-80-1);
            nameptr = (char*)(buf+26+sketchlen);
        }

        for (ptr = nameptr; *ptr; ++ptr) {
            if (!isalnum(*ptr) && strchr("()-_+=[]:;,.<>?",*ptr) == NULL)
                *ptr='_';
            if (isupper(*ptr)) 
                *ptr = tolower(*ptr);
        }

        if (*nameptr==0)
            outfilename = make_noname();
        else {
            strcat(nameptr, ".pbm");
            outfilename = nameptr;
        }


        pm_message("extracting sketch %2d as `%s'", f, outfilename);
        if((out=fopen(outfilename,"wb"))==NULL)
            pm_message("WARNING: couldn't open file '%s'.  Carrying on...", 
                       outfilename);
        else {
            pbm_writepbminit(out, 160, 160, FALSE);
            fwrite(obuf,1,160*160/8,out);
            fclose(out);
        }
    }
    return 0;
}
