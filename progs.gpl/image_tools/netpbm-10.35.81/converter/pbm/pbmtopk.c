/*
  pbmtopk, adapted from "pxtopk.c by tomas rokicki" by AJCD 1/8/90
  
  compile with: cc -o pbmtopk pbmtopk.c -lm -lpbm
*/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "pm_c_util.h"
#include "pbm.h"
#include "nstring.h"
#include "mallocvar.h"

#define MAXPKCHAR 256
#define MAXOPTLINE 200
#define MAXWIDTHTAB 256
#define MAXHEIGHTTAB 16
#define MAXDEPTHTAB 16
#define MAXITALICTAB 64
#define MAXPARAMS 30
#define NAMELENGTH 80

#define fixword(d) ((int)((double)(d)*1048576))
#define unfixword(f) ((double)(f) / 1048576)
#define fixrange(f) ((f) < 16777216 && (f) > -16777216)
#define designunits(p) ((p)*72.27/(double)resolution/unfixword(designsize))

/* character flags: in order of appearance in option files. */
#define XOFFSET     1
#define YOFFSET     2
#define HORZESC     4
#define VERTESC     8
#define TFMWIDTH   16
#define TFMHEIGHT  32
#define TFMDEPTH   64
#define TFMITALIC 128

typedef int integer ;
typedef char quarterword ;
typedef char boolean ;
typedef quarterword ASCIIcode ;
typedef quarterword eightbits ;
typedef unsigned char byte ;

static integer resolution, designsize ;
static char *filename[MAXPKCHAR] ;

static integer xoffset[MAXPKCHAR] ;
static integer yoffset[MAXPKCHAR] ;
static integer horzesc[MAXPKCHAR] ;
static integer vertesc[MAXPKCHAR] ;

static byte tfmindex[MAXPKCHAR] ;
static byte hgtindex[MAXPKCHAR] ;
static byte depindex[MAXPKCHAR] ;
static byte italindex[MAXPKCHAR] ;
static byte charflags[MAXPKCHAR] ;

static bit **bitmap ;
static integer smallestch = MAXPKCHAR ;
static integer largestch = -1;
static integer emwidth  = 0;
static integer checksum ;
static const char *codingscheme = "GRAPHIC" ;
static const char *familyname = "PBM" ;

static integer widthtab[MAXWIDTHTAB] = {0}; /* TFM widths */
static integer numwidth = 1;      /* number of entries in width table */
static integer heighttab[MAXHEIGHTTAB]  = {0};
static integer numheight = 1;
static integer depthtab[MAXDEPTHTAB] = {0};
static integer numdepth = 1;
static integer italictab[MAXITALICTAB] = {0};
static integer numitalic = 1;
static integer parameters[MAXPARAMS] = {0};
static integer numparam = 0;

static ASCIIcode xord[128] ;
static char xchr[256] = {
    '?', '?', '?', '?', '?', '?', '?', '?',
    '?', '?', '?', '?', '?', '?', '?', '?',
    '?', '?', '?', '?', '?', '?', '?', '?',
    '?', '?', '?', '?', '?', '?', '?', '?',
    ' ', '!', '"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', '{', '|', '}', '~', '?' };

static FILE *tfmfile, *pkfile ;
static char tfmname[NAMELENGTH+1], pkname[NAMELENGTH+1] ;
static integer pbmtopk_pkloc = 0 ;
static integer bitweight ;
static integer outputbyte ;
static integer car ;
static integer hppp ;
static integer width ;
static integer height ;

/* check sum algorithm (in Pascal):

compute_checksum()
    begin
        c0:=bc; c1:=ec; c2:=bc; c3:=ec;
        for c:=bc to ec do if char_wd[c]>0 then begin
            temp_width:=memory[char_wd[c]];
            if design_units<>unity then
               temp_width:=round((temp_width/design_units)*1048576.0);
            temp_width:=temp_width + (c+4)*@'20000000; 
                {this should be positive}
            c0:=(c0+c0+temp_width) mod 255;
            c1:=(c1+c1+temp_width) mod 253;
            c2:=(c2+c2+temp_width) mod 251;
            c3:=(c3+c3+temp_width) mod 247;
            end;
        header_bytes[check_sum_loc]:=c0;
        header_bytes[check_sum_loc+1]:=c1;
        header_bytes[check_sum_loc+2]:=c2;
        header_bytes[check_sum_loc+3]:=c3;
        end
*/

#define add_tfmwidth(v) (add_tfmtable(widthtab, &numwidth, v, MAXWIDTHTAB,\
                                      "TFM width"))
#define add_tfmheight(v) (add_tfmtable(heighttab, &numheight, v, MAXHEIGHTTAB,\
                                       "TFM height"))
#define add_tfmdepth(v) (add_tfmtable(depthtab, &numdepth, v, MAXDEPTHTAB,\
                                      "TFM depth"))
#define add_tfmitalic(v) (add_tfmtable(italictab, &numitalic, v, MAXITALICTAB,\
                                       "Italic correction"))


static byte
add_tfmtable(int *        const table, 
             int *        const count, 
             int          const value, 
             int          const max_count, 
             const char * const name) {
    
    integer i;
    for (i = 0; i < *count; i++) /* search for value in tfm table */
        if (table[i] == value) return (byte)i;
    if (*count >= max_count)
        pm_error("too many values in %s table", name) ;
    if (!fixrange(value))
        pm_error("%s %f for char %d out of range",
                 name, unfixword(value), car);
    table[*count] = value ;
    return (*count)++ ;
}



/* add a suffix to a filename in an allocated space */
static void 
pbmtopk_add_suffix(char * const name, 
                   const char * const suffix) {

    char *slash = strrchr(name, '/');
    char *dot = strrchr(name, '.');

    if ((dot && slash ? dot < slash : !dot) && !STREQ(name, "-"))
        strcat(name, suffix);
}



/* initialize the PK parameters */
static void 
initialize_pk(void) {
    integer i ;
    pm_message("This is PBMtoPK, version 2.4") ;
    for (i = 127 ; i <= 255 ; i ++) xchr[i] = '?' ;
    for (i = 0 ; i <= 127 ; i ++) xord[i] = 32 ;
    for (i = 32 ; i < 127 ; i ++) xord[(int)xchr[i]] = i ;
    for (i = 0; i < MAXPKCHAR; i++) {
        filename[i] = NULL;
        charflags[i] = 0;
    }
    designsize = fixword(1.0) ;
}



/* write a single byte to the PK file */
static void 
pbmtopk_pkbyte(integer const b_in) {
    integer b;

    b = b_in;  /* initial value */

    if (b < 0) 
        b += 256 ;
    putc(b, pkfile) ;
    pbmtopk_pkloc++ ;
}



/* write two bytes to the PK file */
static void 
pkhalfword(integer const a_in) {
    integer a;

    a = a_in;

    if (a < 0) 
        a += 65536 ;
    pbmtopk_pkbyte(a >> 8) ;
    pbmtopk_pkbyte(a & 255) ;
}



/* write three bytes to the PK file */
static void 
pkthreebytes(integer const a) {

    pbmtopk_pkbyte((a>>16) & 255) ;
    pbmtopk_pkbyte((a>>8) & 255) ;
    pbmtopk_pkbyte(a & 255) ;
}



/* write four bytes to the PK file */
static void 
pkword(integer const a) {
    pbmtopk_pkbyte((a>>24) & 255) ;
    pbmtopk_pkbyte((a>>16) & 255) ;
    pbmtopk_pkbyte((a>>8) & 255) ;
    pbmtopk_pkbyte(a & 255) ;
}



/* write a nibble to the PK file */
static void 
pknyb(integer const a) {

    if (bitweight == 16) {
        outputbyte = (a<<4) ;
        bitweight = 1 ;
    } else {
        pbmtopk_pkbyte(outputbyte + a) ;
        bitweight = 16 ;
    }
}



/* write preamble to PK file */
static void 
writepreamble(void) {
    integer i ;
    const char * const comment = "PBMtoPK 2.4 output" ;
   
    pbmtopk_pkbyte(247) ;                /* PRE command */
    pbmtopk_pkbyte(89) ;             /* PK file type */
    pbmtopk_pkbyte(strlen(comment)) ;            /* output comment */
    for (i = 0 ; i < strlen(comment); i++) 
        pbmtopk_pkbyte(xord[(int)comment[i]]) ;
    pkword(designsize) ;             /* write designsize */
    pkword(checksum) ;       /* write checksum; calculate if possible */
    pkword(hppp) ;               /* write H pixels per point */
    pkword(hppp) ;               /* write V pixels per point */
}



/* write postamble to PK file, padded to word length */
static void 
writepostamble(void) {
    pbmtopk_pkbyte(245) ;                /* POST command */
    while (pbmtopk_pkloc % 4)
        pbmtopk_pkbyte(246) ;             /* pad with no-ops */
    pm_message("%d bytes written to packed file.", pbmtopk_pkloc) ;
}



/* write a byte to the TFM file */
static void 
tfmbyte(integer const b_in) {
    integer b;

    b = b_in;

    if (b < 0) b += 256 ;
    putc(b, tfmfile) ;
}



/* write a half word to the TFM file */
static void 
tfmhalfword(integer const a_in) {
    
    integer a;

    a = a_in;

    if (a < 0) a += 65536 ;
    tfmbyte(a >> 8) ;
    tfmbyte(a & 255) ;
}



/* write a word to the TFM file */
static void 
tfmword(integer const a) {
    tfmbyte((a>>24) & 255) ;
    tfmbyte((a>>16) & 255) ;
    tfmbyte((a>>8) & 255) ;
    tfmbyte(a & 255) ;
}



/* write the whole TFM file for the font */
static void 
writetfmfile(void) {
    integer totallength ;
    integer headersize = 17;
    integer i ;
   
    if (largestch - smallestch < 0) {
        largestch = 0;
        smallestch = 1;
    }
    if (numparam < 7) /* set default parameters */
        switch (numparam) {
        case 0: /* slant */
            parameters[numparam++] = 0 ;
        case 1: /* space */
            parameters[numparam++] = fixword(designunits(emwidth/3.0));
        case 2: /* space_stretch */
            parameters[numparam++] = fixword(unfixword(parameters[1])/2.0) ;
        case 3: /* space_shrink */
            parameters[numparam++] = fixword(unfixword(parameters[1])/3.0) ;
        case 4: /* x_height */
            parameters[numparam++] = fixword(0.45);
        case 5: /* quad */
            parameters[numparam++] = fixword(designunits(emwidth)) ;
        case 6: /* extra_space */
            parameters[numparam++] = fixword(unfixword(parameters[1])/3.0) ;
        }
    totallength = 6 + headersize + (largestch+1-smallestch) +
        numwidth + numheight + numdepth + numitalic + numparam ;
    /* lengths */
    tfmhalfword(totallength) ;           /* write file TFM length */
    tfmhalfword(headersize) ;            /* write TFM header length */
    tfmhalfword(smallestch) ;            /* write lowest char index */
    tfmhalfword(largestch) ;         /* write highest char index */
    tfmhalfword(numwidth) ;          /* write number of widths */
    tfmhalfword(numheight) ;         /* write number of heights */
    tfmhalfword(numdepth) ;          /* write number of depths */
    tfmhalfword(numitalic) ;         /* write number of italcorrs */
    tfmhalfword(0) ;             /* lig/kern table */
    tfmhalfword(0) ;             /* kern table */
    tfmhalfword(0) ;             /* extensible char table */
    tfmhalfword(numparam) ;          /* number of fontdimens */
    /* header */
    tfmword(checksum) ;              /* write checksum */
    tfmword(designsize) ;            /* write designsize */
    if (strlen(codingscheme) > 39) 
        tfmbyte(39) ; /* write coding scheme len */
    else 
        tfmbyte(strlen(codingscheme)) ;
    for (i = 0; i < 39; i++)         /* write coding scheme */
        if 
            (*codingscheme) tfmbyte(xord[(int)(*codingscheme++)]) ;
        else
            tfmbyte(0) ;
    if (strlen(familyname) > 19) 
        tfmbyte(19) ;   /* write family length */
    else 
        tfmbyte(strlen(familyname)) ;
    for (i = 0; i < 19; i++)         /* write family */
        if (*familyname) 
            tfmbyte(xord[(int)(*familyname++)]) ;
        else 
            tfmbyte(0) ;
    /* char_info */
    for (car = smallestch; car <= largestch; car++)
        if (filename[car]) {          /* write character info */
            tfmbyte(tfmindex[car]) ;
            tfmbyte((hgtindex[car]<<4) + depindex[car]) ;
            tfmbyte(italindex[car]<<2) ;
            tfmbyte(0) ;
        } else
            tfmword(0) ;
    /* width table */
    for (i = 0; i < numwidth; i++) tfmword(widthtab[i]) ;
    /* height table */
    for (i = 0; i < numheight; i++) tfmword(heighttab[i]) ;
    /* depth table */
    for (i = 0; i < numdepth; i++) tfmword(depthtab[i]) ;
    /* italic correction table */
    for (i = 0; i < numitalic; i++) tfmword(italictab[i]) ;
    /* no lig_kern, kern, or exten tables */
    /* fontdimen table */
    for (i = 0; i < numparam; i++)
        if (i && !fixrange(parameters[i]))
            pm_error("parameter %d out of range (-p)", i);
        else
            tfmword(parameters[i]) ;
    pm_message("%d bytes written to tfm file.", totallength*4) ;
}



/* read a character from a PBM file */
static void readcharacter(void) {
    FILE *fp;
   
    fp = pm_openr(filename[car]);
    bitmap = pbm_readpbm(fp, &width, &height) ;
    pm_close(fp) ;
   
    if ((charflags[car] & HORZESC) == 0) horzesc[car] = width ;
    if ((charflags[car] & VERTESC) == 0) vertesc[car] = 0;
    if ((charflags[car] & XOFFSET) == 0) xoffset[car] = 0;
    if ((charflags[car] & YOFFSET) == 0) yoffset[car] = height-1;
    if ((charflags[car] & TFMWIDTH) == 0)
        tfmindex[car] = add_tfmwidth(fixword(designunits(width)));
    if ((charflags[car] & TFMHEIGHT) == 0)
        hgtindex[car] = add_tfmheight(fixword(designunits(yoffset[car]+1)));
    if ((charflags[car] & TFMDEPTH) == 0)
        depindex[car] = 
            add_tfmdepth(fixword(designunits(height-1-yoffset[car])));
    if ((charflags[car] & TFMITALIC) == 0) italindex[car] = 0;
   
    if (car < smallestch) smallestch = car;
    if (car > largestch) largestch = car;
    if (width > emwidth) emwidth = width ;
}



/* test if two rows of the PBM are the same */
static int 
equal(const bit * const row1, 
      const bit * const row2) {

    integer i ;
   
    for (i = 0; i < width; i++)
        if (row1[i] != row2[i]) 
            return (0) ;

    return(1) ;
}



static void 
shipcharacter(void) {

    integer compsize ;
    integer i, j, k ;
    bit *zerorow, *onesrow ;
    integer *repeatptr, *bitcounts ;
    integer count ;
    integer test ;
    integer curptr, rowptr ;
    integer bitval ;
    integer repeatflag ;
    integer colptr ;
    integer currepeat ;
    integer dynf ;
    integer deriv[14] ;
    integer bcompsize ;
    boolean firston ;
    integer flagbyte ;
    boolean state ;
    boolean on ;
    integer hbit ;
    integer pbit ;
    boolean ron, son ;
    integer rcount, scount ;
    integer ri, si ;
    integer max2 ;
    integer predpkloc ;
    integer buff ;
   
    integer tfwid = widthtab[tfmindex[car]] ;
    integer hesc = horzesc[car] ;
    integer vesc = vertesc[car] ;
    integer xoff = xoffset[car] ;
    integer yoff = yoffset[car] ;
   
    MALLOCARRAY(repeatptr, height + 1);
    MALLOCARRAY(bitcounts, height * width);
    if (repeatptr == NULL || bitcounts == NULL)
        pm_error("out of memory while allocating bit counts");
    zerorow = pbm_allocrow(width) ;      /* initialize plain rows */
    onesrow = pbm_allocrow(width) ;
    for (i = 0 ; i < width ; i++) {
        zerorow[i] = PBM_WHITE ;
        onesrow[i] = PBM_BLACK ;
    }
    for (i=0; i < height; i = k) {       /* set repeat pointers */
        k = i + 1;
        if (!equal(bitmap[i], zerorow) && !equal(bitmap[i], onesrow)) {
            while (k < height && equal(bitmap[i], bitmap[k]))
                k++;
            repeatptr[i] = k - i - 1;
        } else {
            repeatptr[i] = 0;
        }
    }
    repeatptr[height] = 0 ;
    colptr = width - 1 ;
    repeatflag = currepeat = curptr = count = rowptr = 0 ;
    test = PBM_WHITE ;
    do {
        colptr++ ;
        if (colptr == width) {            /* end of row, get next row */
            colptr = 0 ;
            rowptr = currepeat ;
            if (repeatptr[currepeat] > 0) {
                repeatflag = repeatptr[currepeat] ;
                currepeat += repeatflag ;
                rowptr += repeatflag ;
            }
            currepeat++ ;
        }
        if (rowptr >= height) bitval = -1 ;
        else bitval = bitmap[rowptr][colptr] ;
        if (bitval == test) count++ ;     /* count repeated pixels */
        else {                    /* end of pixel run */
            bitcounts[curptr++] = count ;
            if (curptr+3 >= height*width)
                pm_error("out of memory while saving character counts");
            count = 1 ;
            test = bitval ;
            if (repeatflag > 0) {
                bitcounts[curptr++] = -repeatflag ;
                repeatflag = 0 ;
            }
        }
    } while (test != -1) ;
    bitcounts[curptr] = 0 ;
    bitcounts[curptr + 1] = 0 ;
    for (i = 1 ; i <= 13 ; i ++) deriv[i] = 0 ;
    i = firston = (bitcounts[0] == 0) ;
    compsize = 0 ;
    while (bitcounts[i] != 0) {          /* calculate dyn_f */
        j = bitcounts[i] ;
        if (j == -1) compsize++ ;
        else {
            if (j < 0) {
                compsize++ ;
                j = -j ;
            }
            if (j < 209) compsize += 2 ;
            else {
                k = j - 193 ;
                while (k >= 16) {
                    k >>= 4 ;
                    compsize += 2 ;
                }
                compsize++ ;
            }
            if (j < 14) (deriv[j])-- ;
            else if (j < 209) (deriv[(223 - j) / 15])++ ;
            else {
                k = 16 ;
                while (((k<<4) < j + 3)) k <<= 4 ;
                if (j - k <= 192)
                    deriv[(207 - j + k) / 15] += 2 ;
            }
        }
        i++ ;
    }
    bcompsize = compsize ;
    dynf = 0 ;
    for (i = 1 ; i <= 13 ; i ++) {
        compsize += deriv[i] ;
        if (compsize <= bcompsize) {
            bcompsize = compsize ;
            dynf = i ;
        }
    }
    compsize = ((bcompsize + 1)>>1) ;
    if ((compsize > ((height*width+7)>>3)) || (height*width == 0)) {
        compsize = ((height*width+7)>>3) ;
        dynf = 14 ;
    }
    flagbyte = (dynf<<4) ;
    if (firston) flagbyte |= 8 ;
    if (tfwid > 16777215 || tfwid < 0 || hesc < 0 || vesc != 0 ||
        compsize > 196579 || width > 65535 || height > 65535 ||
        xoff > 32767 || yoff > 32767 || xoff < -32768 || yoff < -32768) {
        flagbyte |= 7 ;               /* long form preamble */
        pbmtopk_pkbyte(flagbyte) ;
        compsize += 28 ;
        pkword(compsize) ;            /* char packet size */
        pkword(car) ;             /* character number */
        predpkloc = pbmtopk_pkloc + compsize ;
        pkword(tfwid) ;               /* TFM width */
        pkword(hesc<<16) ;            /* horiz escapement */
        pkword(vesc<<16) ;            /* vert escapement */
        pkword(width) ;               /* bounding box width */
        pkword(height) ;              /* bounding box height */
        pkword(xoff) ;                /* horiz offset */
        pkword(yoff) ;                /* vert offset */
    } else if (hesc > 255 || width > 255 || height > 255 ||
               xoff > 127 || yoff > 127 || xoff < -128 ||
               yoff < -128 || compsize > 1016) {
        compsize += 13 ;              /* extended short preamble */
        flagbyte += (compsize>>16) + 4 ;
        pbmtopk_pkbyte(flagbyte) ;
        pkhalfword(compsize & 65535) ;        /* char packet size */
        pbmtopk_pkbyte(car) ;             /* character number */
        predpkloc = pbmtopk_pkloc + compsize ;
        pkthreebytes(tfwid) ;         /* TFM width */
        pkhalfword(hesc) ;            /* horiz escapement */
        pkhalfword(width) ;           /* bounding box width */
        pkhalfword(height) ;          /* bounding box height */
        pkhalfword(xoff) ;            /* horiz offset */
        pkhalfword(yoff) ;            /* vert offset */
    } else {
        compsize += 8 ;               /* short form preamble */
        flagbyte = flagbyte + (compsize>>8) ;
        pbmtopk_pkbyte(flagbyte) ;
        pbmtopk_pkbyte(compsize & 255) ;          /* char packet size */
        pbmtopk_pkbyte(car) ;             /* character number */
        predpkloc = pbmtopk_pkloc + compsize ;
        pkthreebytes(tfwid) ;         /* TFM width */
        pbmtopk_pkbyte(hesc) ;                /* horiz escapement */
        pbmtopk_pkbyte(width) ;               /* bounding box width */
        pbmtopk_pkbyte(height) ;              /* bounding box height */
        pbmtopk_pkbyte(xoff) ;                /* horiz offset */
        pbmtopk_pkbyte(yoff) ;                /* vert offset */
    }
    if (dynf != 14) {                /* write packed character */
        bitweight = 16 ;
        max2 = 208 - 15 * dynf ;
        i = firston ;
        while (bitcounts[i] != 0) {
            j = bitcounts[i] ;
            if (j == - 1) pknyb(15) ;
            else {
                if (j < 0) {
                    pknyb(14) ;
                    j = -j ;
                }
                if (j <= dynf) pknyb(j) ;
                else if (j <= max2) {
                    j -= dynf + 1 ;
                    pknyb((j >> 4) + dynf + 1) ;
                    pknyb((j & 15)) ;
                } else {
                    j -= max2 - 15 ;
                    k = 16 ;
                    while (k <= j) {
                        k <<= 4 ;
                        pknyb(0) ;
                    }
                    while (k > 1) {
                        k >>= 4 ;
                        pknyb(j / k) ;
                        j = j % k ;
                    }
                }
            }
            i++ ;
        }
        if (bitweight != 16) pbmtopk_pkbyte(outputbyte) ;
    } else {                 /* write bitmap character */
        buff = 0 ;
        pbit = 8 ;
        i = firston ;
        hbit = width ;
        on = ! firston ;
        state = 0 ;
        count = repeatflag = 0 ;
        while ((bitcounts[i] != 0) || state || (count > 0)) {
            if (state) {
                count = rcount ;
                i = ri ;
                on = ron ;
                repeatflag-- ;
            } else {
                rcount = count ;
                ri = i ;
                ron = on ;
            }
            do {
                if (count == 0) {
                    if (bitcounts[i] < 0) {
                        if (! state) repeatflag = -bitcounts[i] ;
                        i++ ;
                    }
                    count = bitcounts[i] ;
                    i++ ;
                    on = !on ;
                }
                if ((count >= pbit) && (pbit < hbit)) {
                    if (on) buff += (1 << pbit) - 1 ;
                    pbmtopk_pkbyte(buff) ;
                    buff = 0 ;
                    hbit -= pbit ;
                    count -= pbit ;
                    pbit = 8 ;
                } else if ((count < pbit) && (count < hbit)) {
                    if (on) buff += (1 << pbit) - (1 << (pbit - count)) ;
                    pbit -=  count ;
                    hbit -= count ;
                    count = 0 ;
                } else {
                    if (on) buff += (1 << pbit) - (1 << (pbit - hbit)) ;
                    count -= hbit ;
                    pbit -= hbit ;
                    hbit = width ;
                    if (pbit == 0) {
                        pbmtopk_pkbyte(buff) ;
                        buff = 0 ;
                        pbit = 8 ;
                    }
                }
            } while (hbit != width) ;
            if (state && (repeatflag == 0)) {
                count = scount ;
                i = si ;
                on = son ;
                state = 0 ;
            } else if (! state && (repeatflag > 0)) {
                scount = count ;
                si = i ;
                son = on ;
                state = 1 ;
            }
        }
        if (pbit != 8) pbmtopk_pkbyte(buff) ;
    }
    if (predpkloc != pbmtopk_pkloc)
        pm_error("bad predicted character length: character %d", car);
    pbm_freerow(zerorow); 
    pbm_freerow(onesrow); 
    free((char *)repeatptr);
    free((char *)bitcounts);
}



/* check that character is in valid range */
static void 
checkchar(void) {
    if (car < 0 || car >= MAXPKCHAR)
        pm_error("character must be in range 0 to %d", MAXPKCHAR-1) ;
}



/* read character information from an option file */
static void 
optionfile(const char * const name) {

    FILE *fp ;
    char buffer[MAXOPTLINE] ;
   
    fp = pm_openr(name);
    while (!feof(fp)) {
        char *here = buffer;
      
        if (fgets(buffer, MAXOPTLINE, fp) == NULL) break ;
        while (ISSPACE(*here)) here++ ;
        if (*here && *here == '=') {
            if (sscanf(here+1, "%d", &car) != 1)
                pm_error("bad option file line %s", buffer) ;
        } else if (*here && *here != '%' && *here != '#') {
            char str[NAMELENGTH] ;
            integer i, n;
     
            checkchar() ;
            if (sscanf(here, "%s%n", str, &n) != 1)
                pm_error("bad option file line %s", buffer) ;
            filename[car] = strdup(str);
            if (filename[car] == NULL)
                pm_error("out of memory allocating filename %s", str);
            for (i = 1; i < 256; i<<=1) {
                here += n;
                if (sscanf(here, "%s%n", str, &n) != 1) break ;
                if (strcmp(str, "*")) {
                    charflags[car] |= i ;
                    switch (i) {
                    case XOFFSET:
                        xoffset[car] = atoi(str) ;
                        break ;
                    case YOFFSET:
                        yoffset[car] = atoi(str) ;
                        break ;
                    case HORZESC:
                        horzesc[car] = atoi(str) ;
                        break ;
                    case VERTESC:
                        vertesc[car] = atoi(str) ;
                        break ;
                    case TFMWIDTH:
                        tfmindex[car] = add_tfmwidth(fixword(atof(str))) ;
                        break ;
                    case TFMHEIGHT:
                        hgtindex[car] = add_tfmheight(fixword(atof(str))) ;
                        break ;
                    case TFMDEPTH:
                        depindex[car] = add_tfmdepth(fixword(atof(str))) ;
                        break ;
                    case TFMITALIC:
                        italindex[car] = add_tfmitalic(fixword(atof(str))) ;
                        break ;
                    }
                }
            }
            car++ ;
        }
    }
    pm_close(fp) ;
}



int
main(int argc, char *argv[]) {
    integer i, hesc, vesc, xoff, yoff, tfwid, tfdep, tfhgt, tfital ;
    byte flags ;
    const char * const usage = "pkfile[.pk] tfmfile[.tfm] dpi "
        "[-s designsize] [-p num param...]\n"
        "[-C codingscheme ] [-F family] [-c num | <char>]...\n"
        "<char> is:\n"
        "[-W tfmwidth] [-H tfmheight] [-D tfmdepth] [-I ital_corr] "
        "[-h horiz]\n"
        "[-v vert] [-x xoffset] [-y yoffset] file\n"
        "or:\n"
        "-f optfile\n" ;

    pbm_init(&argc, argv);
    initialize_pk() ;
   
    if (--argc < 1) pm_usage(usage) ;
    strcpy(pkname, *++argv) ;
    pbmtopk_add_suffix(pkname, ".pk") ;
   
    if (--argc < 1) pm_usage(usage) ;
    strcpy(tfmname, *++argv) ;
    pbmtopk_add_suffix(tfmname, ".tfm") ;
   
    if (--argc < 1) pm_usage(usage) ;
    resolution = atoi(*++argv) ;
    if (resolution < 1 || resolution > 32767)
        pm_error("unlikely resolution %d dpi", resolution);
   
    car = flags = hesc = vesc = xoff = yoff = tfwid = 0;
    while (++argv, --argc) {
        if (argv[0][0] == '-' && argv[0][1]) {
            char c, *p;
            c = argv[0][1] ;
            if (argv[0][2]) p = *argv + 2 ;    /* set argument pointer */
            else if (++argv, --argc) p = *argv ;
            else pm_usage(usage) ;
            switch (c) {
            case 'C':
                codingscheme = p;
                break ;
            case 'F':
                familyname = p;
                break ;
            case 'c':
                car = atoi(p) ;
                break ;
            case 's':
                designsize = fixword(atof(p));
                if (designsize < 1048576)
                    pm_error("design size %f out of range", 
                             unfixword(designsize));
            case 'h':
                hesc = atoi(p) ;
                flags |= HORZESC ;
                break ;
            case 'v':
                vesc = atoi(p) ;
                flags |= VERTESC ;
                break ;
            case 'x':
                xoff = atoi(p) ;
                flags |= XOFFSET ;
                break ;
            case 'y':
                yoff = atoi(p) ;
                flags |= YOFFSET ;
                break ;
            case 'W':
                tfwid = fixword(atof(p)) ;
                flags |= TFMWIDTH ;
                break ;
            case 'H':
                tfhgt = fixword(atof(p)) ;
                flags |= TFMHEIGHT ;
                break ;
            case 'D':
                tfdep = fixword(atof(p)) ;
                flags |= TFMDEPTH ;
                break ;
            case 'I':
                tfital = fixword(atof(p)) ;
                flags |= TFMITALIC ;
                break ;
            case 'f':
                optionfile(p) ;
                break ;
            case 'p':
                numparam = atoi(p);
                if (numparam < 1 || numparam > MAXPARAMS)
                    pm_error("parameter count %d out of range", numparam);
                for (i=0; i<numparam; i++)
                    if (++argv,--argc)
                        parameters[i] = fixword(atof(*argv)) ;
                    else
                        pm_error("not enough parameters (-p)");
                break ;
            default:
                pm_usage(usage) ;
            }
        } else  {
            checkchar() ;
            if (flags & TFMWIDTH)
                tfmindex[car] = add_tfmwidth(tfwid);
            if (flags & TFMDEPTH)
                depindex[car] = add_tfmdepth(tfdep);
            if (flags & TFMHEIGHT)
                hgtindex[car] = add_tfmheight(tfhgt);
            if (flags & TFMITALIC)
                italindex[car] = add_tfmitalic(tfital);
            horzesc[car] = hesc ;
            vertesc[car] = vesc ;
            xoffset[car] = xoff ;
            yoffset[car] = yoff ;
            filename[car] = *argv ;
            charflags[car] = flags ;
            car++ ;
            flags = 0;
        }
    }
    hppp = ROUND((resolution<<16) / 72.27) ;
    pkfile = pm_openw(pkname);
    tfmfile = pm_openw(tfmname);
    writepreamble() ;
    for (car = 0 ; car < MAXPKCHAR ; car++)
        if (filename[car]) {
            readcharacter() ;
            shipcharacter() ;
        }
    writepostamble() ;
    writetfmfile() ;
    pm_close(pkfile) ;
    pm_close(tfmfile) ;

    return 0;
}

