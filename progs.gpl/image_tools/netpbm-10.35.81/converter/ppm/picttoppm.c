/*
 * picttoppm.c -- convert a MacIntosh PICT file to PPM format.
 *
 * Copyright 1989,1992,1993 George Phillips
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 *
 * George Phillips <phillips@cs.ubc.ca>
 * Department of Computer Science
 * University of British Columbia
 *
 *
 * 2003-02:    Handling for DirectBitsRgn opcode (0x9b) added by 
 *             kabe@sra-tohoku.co.jp.
 *
 * 2004-03-27: Several bugs fixed by Steve Summit, scs@eskimo.com.
 *
 */

#define _XOPEN_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "pm_c_util.h"
#include "ppm.h"
#include "pbmfont.h"
#include "mallocvar.h"
#include "nstring.h"


/*
 * Typical byte, 2 byte and 4 byte integers.
 */
typedef unsigned char byte;
typedef char signed_byte;
typedef unsigned short word;
typedef unsigned long longword;


/*
 * Data structures for QuickDraw (and hence PICT) stuff.
 */

struct Rect {
    word top;
    word left;
    word bottom;
    word right;
};

struct pixMap {
    struct Rect Bounds;
    word version;
    word packType;
    longword packSize;
    longword hRes;
    longword vRes;
    word pixelType;
    word pixelSize;
    word cmpCount;
    word cmpSize;
    longword planeBytes;
    longword pmTable;
    longword pmReserved;
};

struct RGBColor {
    word red;
    word grn;
    word blu;
};

struct Point {
    word x;
    word y;
};

struct Pattern {
    byte pix[64];
};

struct rgbPlanes {
    word * red;
    word * grn;
    word * blu;
};

typedef void (*transfer_func) (struct RGBColor* src, struct RGBColor* dst);

static const char* stage;
static struct Rect picFrame;
static word* red;
static word* green;
static word* blue;
static word rowlen;
static word collen;
static longword planelen;
static int verbose;
static int fullres;
static int recognize_comment;

static struct RGBColor black = { 0, 0, 0 };
static struct RGBColor white = { 0xffff, 0xffff, 0xffff };

/* various bits of drawing state */
static struct RGBColor foreground = { 0, 0, 0 };
static struct RGBColor background = { 0xffff, 0xffff, 0xffff };
static struct RGBColor op_color;
static struct Pattern bkpat;
static struct Pattern fillpat;
static struct Rect clip_rect;
static struct Rect cur_rect;
static struct Point current;
static struct Pattern pen_pat;
static word pen_width;
static word pen_height;
static word pen_mode;
static transfer_func pen_trf;
static word text_font;
static byte text_face;
static word text_mode;
static transfer_func text_trf;
static word text_size;
static struct font* tfont;

/* state for magic printer comments */
static int ps_text;
static byte ps_just;
static byte ps_flip;
static word ps_rotation;
static byte ps_linespace;
static int ps_cent_x;
static int ps_cent_y;
static int ps_cent_set;

struct opdef {
    const char* name;
    int len;
    void (*impl) (int);
    const char* description;
};

struct raster {
/*----------------------------------------------------------------------------
   An image raster.  May be either truecolor or paletted.

   This is an array of pixels in row-major order, with 'rowSize'
   bytes per row, 'rowCount' high.

   Within a row, pixels go left to right.  The rows go top to bottom.

   Each pixel is either a palette index or an RGB triple, depending on
   the format of the associated PICT.
   
   Each pixel is one byte if the associated PICT has 8 or fewer bits
   per pixel.  If the associated PICT has 16 or 32 bits per pixel, an
   element herein is 2 or 4 bytes, respectively.

   For 16 bits per pixel, the two bytes for each pixel encode RGB values
   as described in decode16().

   For 32 bits per pixel, each row is divided into 4 planes.  Red,
   green, blue, and something else, in that order.  The format of a
   plane is one byte per pixel, left to right.
-----------------------------------------------------------------------------*/
    unsigned char * bytes;  /* malloc'ed */
    unsigned int rowSize;
    unsigned int rowCount;
};


static void
allocateRaster(struct raster * const rasterP,
               unsigned int    const width,
               unsigned int    const height,
               unsigned int    const bitsPerPixel) {

    if (width > UINT_MAX/4)
        pm_error("Width %u pixels too large for arithmetic", width);

    rasterP->rowCount = height;

    switch (bitsPerPixel) {
    case 32:
        /* TODO: I'm still trying to figure out this format.

           My theory today:
           The row data is in plane order (a row consists of red
           plane, then, green, then blue, then some 4th plane).

           The old hack code said 3 bytes per pixel here, and could get
           away with it because it never got to decoding the 4th plane.

           But the new clean code needs to tell it like it is and allocate
           4 bytes per pixel.  If we say 3 bytes per pixel here, we get an
           "invalid PICT" error because the image actually contains 4
           bytes per pixel and as we decompress it, we run out of place
           to put the data.

           We have yet to see if we can properly interpret the data.
        */

        rasterP->rowSize = width * 4;
        break;
    case 16:
        rasterP->rowSize = width * 2;
        break;
    case 8:
    case 4:
    case 2:
    case 1:
        rasterP->rowSize = width * 1;
        break;
    default:
        pm_error("INTERNAL ERROR: impossible bitsPerPixel value in "
                 "unpackbits(): %u", bitsPerPixel);
    }    
    if (UINT_MAX / rasterP->rowSize < rasterP->rowCount)
        pm_error("Arithmetic overflow computing size of %u x %u pixel "
                 "array.", rasterP->rowSize, rasterP->rowCount);

    MALLOCARRAY(rasterP->bytes, rasterP->rowSize * rasterP->rowCount);
    if (rasterP->bytes == NULL)
        pm_error("unable to get memory for %u x %u pixel packbits rectangle",
                 width, height);
}


static void
freeRaster(struct raster const raster) {

    free(raster.bytes);
}


struct blit_info {
    struct Rect        srcRect;
    struct Rect        srcBounds;
    struct raster      srcplane;
    int                pixSize;
    struct Rect        dstRect;
    struct RGBColor *  color_map;
    int                mode;
    struct blit_info * next;
};

static struct blit_info* blit_list = 0;
static struct blit_info** last_bl = &blit_list;

#define WORD_LEN (-1)

/*
 * a table of the first 194(?) opcodes.  The table is too empty.
 *
 * Probably could use an entry specifying if the opcode is valid in version
 * 1, etc.
 */

/* for reserved opcodes of known length */
#define res(length) \
{ "reserved", (length), NULL, "reserved for Apple use" }

/* for reserved opcodes of length determined by a function */
#define resf(skipfunction) \
{ "reserved", NA, (skipfunction), "reserved for Apple use" }

/* seems like RGB colors are 6 bytes, but Apple says they're variable */
/* I'll use 6 for now as I don't care that much. */
#define RGB_LEN (6)


static FILE* ifp;
static int align = 0;



static byte
read_byte(void) {
    int c;

    if ((c = fgetc(ifp)) == EOF)
        pm_error("EOF / read error while %s", stage);

    ++align;
    return c & 255;
}



static word
read_word(void) {
    byte b;

    b = read_byte();

    return (b << 8) | read_byte();
}



static void read_point(struct Point * const p) {
    p->y = read_word();
    p->x = read_word();
}



static longword
read_long(void) {
    word i;

    i = read_word();
    return (i << 16) | read_word();
}



static signed_byte
read_signed_byte(void) {
    return (signed_byte)read_byte();
}



static void 
read_short_point(struct Point * const p) {
    p->x = read_signed_byte();
    p->y = read_signed_byte();
}



static void
skip(int const byteCount) {
    static byte buf[1024];
    int n;

    align += byteCount;

    for (n = byteCount; n > 0; n -= 1024)
        if (fread(buf, n > 1024 ? 1024 : n, 1, ifp) != 1)
            pm_error("EOF / read error while %s", stage);
}



struct const_name {
    int value;
    const char * name;
};

struct const_name const transfer_name[] = {
    { 0,    "srcCopy" },
    { 1,    "srcOr" },
    { 2,    "srcXor" },
    { 3,    "srcBic" },
    { 4,    "notSrcCopy" },
    { 5,    "notSrcOr" },
    { 6,    "notSrcXor" },
    { 7,    "notSrcBic" },
    { 32,   "blend" },
    { 33,   "addPin" },
    { 34,   "addOver" },
    { 35,   "subPin" },
    { 36,   "transparent" },
    { 37,   "adMax" },
    { 38,   "subOver" },
    { 39,   "adMin" },
    { -1,   0 }
};

struct const_name font_name[] = {
    { 0,    "systemFont" },
    { 1,    "applFont" },
    { 2,    "newYork" },
    { 3,    "geneva" },
    { 4,    "monaco" },
    { 5,    "venice" },
    { 6,    "london" },
    { 7,    "athens" },
    { 8,    "sanFran" },
    { 9,    "toronto" },
    { 11,   "cairo" },
    { 12,   "losAngeles" },
    { 20,   "times" },
    { 21,   "helvetica" },
    { 22,   "courier" },
    { 23,   "symbol" },
    { 24,   "taliesin" },
    { -1,   0 }
};

struct const_name ps_just_name[] = {
    { 0,    "no" },
    { 1,    "left" },
    { 2,    "center" },
    { 3,    "right" },
    { 4,    "full" },
    { -1,   0 }
};

struct const_name ps_flip_name[] = {
    { 0,    "no" },
    { 1,    "horizontal" },
    { 2,    "vertical" },
    { -1,   0 }
};



static const char*
const_name(const struct const_name * const table,
           unsigned int              const ct) {

    static char numbuf[32];

    unsigned int i;

    for (i = 0; table[i].name; ++i)
        if (table[i].value == ct)
            return table[i].name;
    
    sprintf(numbuf, "? (%u)", ct);
    return numbuf;
}



static void 
picComment(word const type, 
           int const length) {

    unsigned int remainingLength;

    switch (type) {
    case 150:
        if (verbose) pm_message("TextBegin");
        if (length >= 6) {
            ps_just = read_byte();
            ps_flip = read_byte();
            ps_rotation = read_word();
            ps_linespace = read_byte();
            remainingLength = length - 5;
            if (recognize_comment)
                ps_text = 1;
            ps_cent_set = 0;
            if (verbose) {
                pm_message("%s justification, %s flip, %d degree rotation, "
                           "%d/2 linespacing",
                           const_name(ps_just_name, ps_just),
                           const_name(ps_flip_name, ps_flip),
                           ps_rotation, ps_linespace);
            }
        } else
            remainingLength = length;
        break;
    case 151:
        if (verbose) pm_message("TextEnd");
        ps_text = 0;
        remainingLength = length;
        break;
    case 152:
        if (verbose) pm_message("StringBegin");
        remainingLength = length;
        break;
    case 153:
        if (verbose) pm_message("StringEnd");
        remainingLength = length;
        break;
    case 154:
        if (verbose) pm_message("TextCenter");
        if (length < 8)
            remainingLength = length;
        else {
            ps_cent_y = read_word();
            if (ps_cent_y > 32767)
                ps_cent_y -= 65536;
            skip(2); /* ignore fractional part */
            ps_cent_x = read_word();
            if (ps_cent_x > 32767)
                ps_cent_x -= 65536;
            skip(2); /* ignore fractional part */
            remainingLength = length - 8;
            if (verbose)
                pm_message("offset %d %d", ps_cent_x, ps_cent_y);
        }
        break;
    case 155:
        if (verbose) pm_message("LineLayoutOff");
        remainingLength = length;
        break;
    case 156:
        if (verbose) pm_message("LineLayoutOn");
        remainingLength = length;
        break;
    case 160:
        if (verbose) pm_message("PolyBegin");
        remainingLength = length;
        break;
    case 161:
        if (verbose) pm_message("PolyEnd");
        remainingLength = length;
        break;
    case 163:
        if (verbose) pm_message("PolyIgnore");
        remainingLength = length;
        break;
    case 164:
        if (verbose) pm_message("PolySmooth");
        remainingLength = length;
        break;
    case 165:
        if (verbose) pm_message("picPlyClo");
        remainingLength = length;
        break;
    case 180:
        if (verbose) pm_message("DashedLine");
        remainingLength = length;
        break;
    case 181:
        if (verbose) pm_message("DashedStop");
        remainingLength = length;
        break;
    case 182:
        if (verbose) pm_message("SetLineWidth");
        remainingLength = length;
        break;
    case 190:
        if (verbose) pm_message("PostScriptBegin");
        remainingLength = length;
        break;
    case 191:
        if (verbose) pm_message("PostScriptEnd");
        remainingLength = length;
        break;
    case 192:
        if (verbose) pm_message("PostScriptHandle");
        remainingLength = length;
        break;
    case 193:
        if (verbose) pm_message("PostScriptFile");
        remainingLength = length;
        break;
    case 194:
        if (verbose) pm_message("TextIsPostScript");
        remainingLength = length;
        break;
    case 195:
        if (verbose) pm_message("ResourcePS");
        remainingLength = length;
        break;
    case 200:
        if (verbose) pm_message("RotateBegin");
        remainingLength = length;
        break;
    case 201:
        if (verbose) pm_message("RotateEnd");
        remainingLength = length;
        break;
    case 202:
        if (verbose) pm_message("RotateCenter");
        remainingLength = length;
        break;
    case 210:
        if (verbose) pm_message("FormsPrinting");
        remainingLength = length;
        break;
    case 211:
        if (verbose) pm_message("EndFormsPrinting");
        remainingLength = length;
        break;
    default:
        if (verbose) pm_message("%d", type);
        remainingLength = length;
        break;
    }
    if (remainingLength > 0)
        skip(remainingLength);
}



static void
ShortComment(int const version) {
    picComment(read_word(), 0);
}



static void
LongComment(int const version) {
    word type;

    type = read_word();
    picComment(type, read_word());
}



static void
skip_poly_or_region(int const version) {
    stage = "skipping polygon or region";
    skip(read_word() - 2);
}


#define NA (0)

#define FNT_BOLD    (1)
#define FNT_ITALIC  (2)
#define FNT_ULINE   (4)
#define FNT_OUTLINE (8)
#define FNT_SHADOW  (16)
#define FNT_CONDENSE    (32)
#define FNT_EXTEND  (64)

/* Some font searching routines */

struct fontinfo {
    int font;
    int size;
    int style;
    char* filename;
    struct font* loaded;
    struct fontinfo* next;
};

static struct fontinfo* fontlist = 0;
static struct fontinfo** fontlist_ins = &fontlist;



static int 
load_fontdir(const char * const dirfile) {
/*----------------------------------------------------------------------------
   Load the font directory from file named 'dirfile'.  Add its contents
   to the global list of fonts 'fontlist'.
-----------------------------------------------------------------------------*/
    FILE* fp;
    int n, nfont;
    char* arg[5], line[1024];
    struct fontinfo* fontinfo;

    if (!(fp = fopen(dirfile, "rb")))
        return -1;
    
    nfont = 0;
    while (fgets(line, 1024, fp)) {
        if ((n = mk_argvn(line, arg, 5)) == 0 || arg[0][0] == '#')
            continue;
        if (n != 4)
            continue;
        MALLOCVAR(fontinfo);
        if (fontinfo == NULL)
            pm_error("out of memory for font information");
        MALLOCARRAY(fontinfo->filename, strlen(arg[3] + 1));
        if (fontinfo->filename == NULL)
            pm_error("out of memory for font information file name");

        fontinfo->font = atoi(arg[0]);
        fontinfo->size = atoi(arg[1]);
        fontinfo->style = atoi(arg[2]);
        strcpy(fontinfo->filename, arg[3]);
        fontinfo->loaded = 0;

        fontinfo->next = 0;
        *fontlist_ins = fontinfo;
        fontlist_ins = &fontinfo->next;
        nfont++;
    }

    return nfont;
}



static void
read_rect(struct Rect * const r) {
    r->top = read_word();
    r->left = read_word();
    r->bottom = read_word();
    r->right = read_word();
}



static void
dumpRect(const char * const label,
         struct Rect  const rectangle) {

    pm_message("%s (%u,%u) (%u,%u)",
               label,
               rectangle.left,  rectangle.top,
               rectangle.right, rectangle.bottom);
}



static int
rectwidth(const struct Rect * const r) {
    return r->right - r->left;
}



static int
rectheight(const struct Rect * const r) {
    return r->bottom - r->top;
}



static bool
rectsamesize(const struct Rect * const r1, 
             const struct Rect * const r2) {
    return r1->right - r1->left == r2->right - r2->left &&
           r1->bottom - r1->top == r2->bottom - r2->top ;
}



static void
rectinter(struct Rect   const r1, 
          struct Rect   const r2, 
          struct Rect * const intersectionP) {

    intersectionP->left   = MAX(r1.left,   r2.left);
    intersectionP->top    = MAX(r1.top,    r2.top);
    intersectionP->right  = MIN(r1.right,  r2.right);
    intersectionP->bottom = MIN(r1.bottom, r2.bottom);
}



static void
rectscale(struct Rect * const r, 
          double        const xscale, 
          double        const yscale) {
    r->left *= xscale;
    r->right *= xscale;
    r->top *= yscale;
    r->bottom *= yscale;
}



static struct blit_info* 
add_blit_list(void) {

    struct blit_info * bi;
    
    MALLOCVAR(bi);
    if (bi == NULL)
        pm_error("out of memory for blit list");
    
    bi->next = 0;
    *last_bl = bi;
    last_bl = &bi->next;
    
    return bi;
}



/* Various transfer functions for blits.
 *
 * Note src[Not]{Or,Xor,Copy} only work if the source pixmap was originally
 * a bitmap.
 * There's also a small bug that the foreground and background colors
 * are not used in a srcCopy; this wouldn't be hard to fix.
 * It IS a problem since the foreground and background colors CAN be changed.
 */

static bool
rgbAllSame(const struct RGBColor * const colorP,
           unsigned int            const value) {

    return (colorP->red == value &&
            colorP->grn == value &&
            colorP->blu == value);
}


static bool
rgbIsWhite(const struct RGBColor * const colorP) {

    return rgbAllSame(colorP, 0xffff);
}

static bool
rgbIsBlack(const struct RGBColor * const colorP) {

    return rgbAllSame(colorP, 0);
}


static void 
srcCopy(struct RGBColor * const src, 
        struct RGBColor * const dst) {

    if (rgbIsBlack(src))
        *dst = foreground;
    else
        *dst = background;
}



static void 
srcOr(struct RGBColor * const src, 
      struct RGBColor * const dst) {
    if (rgbIsBlack(src))
        *dst = foreground;
}



static void 
srcXor(struct RGBColor * const src, 
       struct RGBColor * const dst) {
    dst->red ^= ~src->red;
    dst->grn ^= ~src->grn;
    dst->blu ^= ~src->blu;
}



static void 
srcBic(struct RGBColor * const src, 
       struct RGBColor * const dst) {
    if (rgbIsBlack(src))
        *dst = background;
}



static void 
notSrcCopy(struct RGBColor * const src, 
           struct RGBColor * const dst) {
    if (rgbIsWhite(src))
        *dst = foreground;
    else if (rgbIsBlack(src))
        *dst = background;
}



static void 
notSrcOr(struct RGBColor * const src, 
         struct RGBColor * const dst) {
    if (rgbIsWhite(src))
        *dst = foreground;
}



static void 
notSrcBic(struct RGBColor * const src, 
          struct RGBColor * const dst) {
    if (rgbIsWhite(src))
        *dst = background;
}



static void 
notSrcXor(struct RGBColor * const src, 
          struct RGBColor * const dst) {
    dst->red ^= src->red;
    dst->grn ^= src->grn;
    dst->blu ^= src->blu;
}



static void 
addOver(struct RGBColor * const src, 
        struct RGBColor * const dst) {
    dst->red += src->red;
    dst->grn += src->grn;
    dst->blu += src->blu;
}



static void 
addPin(struct RGBColor * const src, 
       struct RGBColor * const dst) {
    if ((long)dst->red + (long)src->red > (long)op_color.red)
        dst->red = op_color.red;
    else
        dst->red = dst->red + src->red;

    if ((long)dst->grn + (long)src->grn > (long)op_color.grn)
        dst->grn = op_color.grn;
    else
        dst->grn = dst->grn + src->grn;

    if ((long)dst->blu + (long)src->blu > (long)op_color.blu)
        dst->blu = op_color.blu;
    else
        dst->blu = dst->blu + src->blu;
}



static void 
subOver(struct RGBColor * const src, 
        struct RGBColor * const dst) {
    dst->red -= src->red;
    dst->grn -= src->grn;
    dst->blu -= src->blu;
}



/* or maybe its src - dst; my copy of Inside Mac is unclear */


static void 
subPin(struct RGBColor * const src, 
       struct RGBColor * const dst) {
    if ((long)dst->red - (long)src->red < (long)op_color.red)
        dst->red = op_color.red;
    else
        dst->red = dst->red - src->red;

    if ((long)dst->grn - (long)src->grn < (long)op_color.grn)
        dst->grn = op_color.grn;
    else
        dst->grn = dst->grn - src->grn;

    if ((long)dst->blu - (long)src->blu < (long)op_color.blu)
        dst->blu = op_color.blu;
    else
        dst->blu = dst->blu - src->blu;
}



static void 
adMax(struct RGBColor * const src, 
      struct RGBColor * const dst) {
    if (src->red > dst->red) dst->red = src->red;
    if (src->grn > dst->grn) dst->grn = src->grn;
    if (src->blu > dst->blu) dst->blu = src->blu;
}



static void 
adMin(struct RGBColor * const src, 
      struct RGBColor * const dst) {
    if (src->red < dst->red) dst->red = src->red;
    if (src->grn < dst->grn) dst->grn = src->grn;
    if (src->blu < dst->blu) dst->blu = src->blu;
}



static void 
blend(struct RGBColor * const src, 
      struct RGBColor * const dst) {
#define blend_component(cmp)    \
    ((long)src->cmp * (long)op_color.cmp) / 65536 +    \
    ((long)dst->cmp * (long)(65536 - op_color.cmp) / 65536)

    dst->red = blend_component(red);
    dst->grn = blend_component(grn);
    dst->blu = blend_component(blu);
}



static void 
transparent(struct RGBColor * const src, 
            struct RGBColor * const dst) {
    if (src->red != background.red ||
        src->grn != background.grn ||
        src->blu != background.blu) {
        *dst = *src;
    }
}



static transfer_func 
transfer(int const mode) {
    switch (mode) {
    case  0: return srcCopy;
    case  1: return srcOr;
    case  2: return srcXor;
    case  3: return srcBic;
    case  4: return notSrcCopy;
    case  5: return notSrcOr;
    case  6: return notSrcXor;
    case  7: return notSrcBic;
    case 32: return blend;
    case 33: return addPin;
    case 34: return addOver;
    case 35: return subPin;
    case 36: return transparent;
    case 37: return adMax;
    case 38: return subOver;
    case 39: return adMin;
    default:
        if (mode != 0)
            pm_message("no transfer function for code %s, using srcCopy",
                const_name(transfer_name, mode));
        return srcCopy;
    }
}



static pixval
redepth(pixval const c,
        pixval const oldMaxval) {
    
    return (c * PPM_MAXMAXVAL + oldMaxval / 2) / oldMaxval;
}



static struct RGBColor
decode16(unsigned char * const sixteen) {
/*----------------------------------------------------------------------------
   Decode a 16 bit PICT encoding of RGB:

      Bit   0:    nothing
      Bits  1- 5: red
      Bits  6-10: green
      Bits 11-15: blue

   'sixteen' is a two byte array.
-----------------------------------------------------------------------------*/
    struct RGBColor retval;

    retval.red = (sixteen[0] & 0x7c) >> 2;
    retval.grn = (sixteen[0] & 0x03) << 3 | (sixteen[1] & 0xe0) >> 5;
    retval.blu = (sixteen[1] & 0x1f) >> 0;
                
    return retval;
}



static void
doDiffSize(struct Rect       const clipsrc,
           struct Rect       const clipdst,
           int               const pixSize,
           int               const xsize,
           int               const ysize,
           transfer_func     const trf,
           struct RGBColor * const color_map, 
           unsigned char *   const src,
           int               const srcwid, 
           struct rgbPlanes  const dst,
           unsigned int      const dstwid) {

    unsigned int const dstadd = dstwid - xsize;

    FILE * pnmscalePipeP;
    const char * command;
    FILE * scaled;
    int cols, rows, format;
    pixval maxval;
    pixel * row;
    pixel * rowp;
    FILE * tempFileP;
    const char * tempFilename;
    word * reddst;
    word * grndst;
    word * bludst;

    reddst = dst.red;  /* initial value */
    grndst = dst.grn;  /* initial value */
    bludst = dst.blu;  /* initial value */

    pm_make_tmpfile(&tempFileP, &tempFilename);

    pm_close(tempFileP);

    asprintfN(&command, "pnmscale -xsize %d -ysize %d > %s",
              rectwidth(&clipdst), rectheight(&clipdst), tempFilename);

    pm_message("running command '%s'", command);

    pnmscalePipeP = popen(command, "w");
    if (pnmscalePipeP == NULL)
        pm_error("cannot execute command '%s'  popen() errno = %s (%d)",
                 command, strerror(errno), errno);

    strfree(command);

    fprintf(pnmscalePipeP, "P6\n%d %d\n%d\n",
            rectwidth(&clipsrc), rectheight(&clipsrc), PPM_MAXMAXVAL);

    switch (pixSize) {
    case 8: {
        unsigned int rowNumber;
        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const row = &src[rowNumber * srcwid];
            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const colorIndex = row[colNumber];
                struct RGBColor * const ct = &color_map[colorIndex];
                fputc(redepth(ct->red, 65535L), pnmscalePipeP);
                fputc(redepth(ct->grn, 65535L), pnmscalePipeP);
                fputc(redepth(ct->blu, 65535L), pnmscalePipeP);
            }
        }
    }
    break;
    case 16: {
        unsigned int rowNumber;
        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const row = &src[rowNumber * srcwid];
            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                struct RGBColor const color = decode16(&row[colNumber * 2]);
                fputc(redepth(color.red, 32), pnmscalePipeP);
                fputc(redepth(color.grn, 32), pnmscalePipeP);
                fputc(redepth(color.blu, 32), pnmscalePipeP);
            }
        }
    }
    break;
    case 32: {
        unsigned int const planeSize = srcwid / 4;
        unsigned int rowNumber;
        
        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const row = &src[rowNumber * srcwid];
            unsigned char * const redPlane = &row[planeSize * 0];
            unsigned char * const grnPlane = &row[planeSize * 1];
            unsigned char * const bluPlane = &row[planeSize * 2];

            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                fputc(redepth(redPlane[colNumber], 256), pnmscalePipeP);
                fputc(redepth(grnPlane[colNumber], 256), pnmscalePipeP);
                fputc(redepth(bluPlane[colNumber], 256), pnmscalePipeP);
            }
        }
    }
    break;
    }

    if (pclose(pnmscalePipeP))
        pm_error("pnmscale failed.  pclose() returned Errno %s (%d)",
                 strerror(errno), errno);

    ppm_readppminit(scaled = pm_openr(tempFilename), &cols, &rows,
                    &maxval, &format);
    row = ppm_allocrow(cols);
    /* couldn't hurt to assert cols, rows and maxval... */  

    if (trf == NULL) {
        while (rows-- > 0) {
            unsigned int i;
            ppm_readppmrow(scaled, row, cols, maxval, format);
            for (i = 0, rowp = row; i < cols; ++i, ++rowp) {
                *reddst++ = PPM_GETR(*rowp) * 65536L / (maxval + 1); 
                *grndst++ = PPM_GETG(*rowp) * 65536L / (maxval + 1); 
                *bludst++ = PPM_GETB(*rowp) * 65536L / (maxval + 1); 
            }
            reddst += dstadd;
            grndst += dstadd;
            bludst += dstadd;
        }
    }
    else {
        while (rows-- > 0) {
            unsigned int i;
            ppm_readppmrow(scaled, row, cols, maxval, format);
            for (i = 0, rowp = row; i < cols; i++, rowp++) {
                struct RGBColor dst_c, src_c;
                dst_c.red = *reddst;
                dst_c.grn = *grndst;
                dst_c.blu = *bludst;
                src_c.red = PPM_GETR(*rowp) * 65536L / (maxval + 1); 
                src_c.grn = PPM_GETG(*rowp) * 65536L / (maxval + 1); 
                src_c.blu = PPM_GETB(*rowp) * 65536L / (maxval + 1); 
                (*trf)(&src_c, &dst_c);
                *reddst++ = dst_c.red;
                *grndst++ = dst_c.grn;
                *bludst++ = dst_c.blu;
            }
            reddst += dstadd;
            grndst += dstadd;
            bludst += dstadd;
        }
    }

    pm_close(scaled);
    ppm_freerow(row);
    strfree(tempFilename);
    unlink(tempFilename);
}



static void
getRgb(struct rgbPlanes  const planes,
       unsigned int      const index,
       struct RGBColor * const rgbP) {

    rgbP->red = planes.red[index];
    rgbP->grn = planes.grn[index];
    rgbP->blu = planes.blu[index];
}



static void
putRgb(struct RGBColor  const rgb,
       unsigned int     const index,
       struct rgbPlanes const planes) {

    planes.red[index] = rgb.red;
    planes.grn[index] = rgb.grn;
    planes.blu[index] = rgb.blu;
}



static void
doSameSize(transfer_func           trf,
           int               const pixSize,
           int               const xsize,
           int               const ysize,
           unsigned char *   const src,
           unsigned int      const srcwid,
           struct RGBColor * const color_map,
           struct rgbPlanes  const dst,
           unsigned int      const dstwid) {
/*----------------------------------------------------------------------------
   Generalized (but slow) blit.

   Transfer pixels from 'src' to 'dst', applying the transfer function
   'trf'.

   'src' has the same format as the 'bytes' member of struct raster.
   'srcwid' is the size in bytes of each row, like raster.rowSize.

   We use only the first 'ysize' rows and only the first 'xsize'
   pixels of each row.

   We really should clean this up so that we can take pixels out of
   the middle of a row and rows out of the middle of the raster.  As
   it stands, Caller achieves the same result by passing as 'src'
   a pointer into the middle of a raster -- the upper left corner of
   the rectangle he wants.  But that is messy and nonobvious.

   Each plane of 'dst' is one word per pixel and contains actual
   colors, never a palette index.  It is an array in row-major order
   with 'dstwid' words per row.
-----------------------------------------------------------------------------*/
    switch (pixSize) {
    case 8: {
        unsigned int rowNumber;

        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const srcrow = &src[rowNumber * srcwid];
            unsigned int const dstRowCurs = rowNumber * dstwid;

            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                unsigned int const colorIndex = srcrow[colNumber];
                struct RGBColor dstColor;
                getRgb(dst, dstCursor, &dstColor);
                (*trf)(&color_map[colorIndex], &dstColor);
                putRgb(dstColor, dstCursor, dst);
            }
        }
    } break;
    case 16: {
        unsigned int rowNumber;

        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const row = &src[rowNumber * srcwid];
            unsigned int const dstRowCurs = rowNumber * dstwid;

            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                struct RGBColor const srcColor = decode16(&row[colNumber*2]);
                struct RGBColor dstColor;
                struct RGBColor scaledSrcColor;
                scaledSrcColor.red = srcColor.red << 11;
                scaledSrcColor.grn = srcColor.grn << 11;
                scaledSrcColor.blu = srcColor.blu << 11;
                getRgb(dst, dstCursor, &dstColor);
                (*trf)(&scaledSrcColor, &dstColor);
                putRgb(dstColor, dstCursor, dst);
            }
        }
    } break;
    case 32: {
        unsigned int const planeSize = srcwid / 4;
        unsigned int rowNumber;

        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const row = &src[rowNumber * srcwid];
            unsigned char * const redPlane = &row[planeSize * 0];
            unsigned char * const grnPlane = &row[planeSize * 1];
            unsigned char * const bluPlane = &row[planeSize * 2];
            unsigned int const dstRowCurs = rowNumber * dstwid;

            unsigned int colNumber;

            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                struct RGBColor srcColor, dstColor;
                getRgb(dst, dstCursor, &dstColor);
                srcColor.red = redPlane[colNumber] << 8;
                srcColor.grn = grnPlane[colNumber] << 8;
                srcColor.blu = bluPlane[colNumber] << 8;
                (*trf)(&srcColor, &dstColor);
                putRgb(dstColor, dstCursor, dst);
            }
        }
    } break;
    default:
        pm_error("Impossible value of pixSize: %u", pixSize);
    }
}



static void
blitIdempotent(unsigned int          const pixSize,
               unsigned int          const xsize,
               unsigned int          const ysize,
               unsigned char *       const src,
               unsigned int          const srcwid,
               struct RGBColor *     const colorMap, 
               struct rgbPlanes      const dst,
               unsigned int          const dstwid) {
/*----------------------------------------------------------------------------
  This is the same as doSameSize(), except optimized for the case that
  the transfer function is idempotent (i.e. it's just a straight copy).
  The original author's comments suggest that this optimization isn't
  all that important -- that he just wrote this first and instead of
  expanding it to handle arbitrary transfer functions, added functions
  for that.
-----------------------------------------------------------------------------*/
    switch (pixSize) {
    case 8: {
        unsigned int rowNumber;
        
        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const srcrow = &src[rowNumber * srcwid];
            unsigned int const dstRowCurs = rowNumber * dstwid;
            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                struct RGBColor * const ct = colorMap + srcrow[colNumber];
                dst.red[dstCursor] = ct->red;
                dst.grn[dstCursor] = ct->grn;
                dst.blu[dstCursor] = ct->blu;
            }
        }
    } break;
    case 16: {
        unsigned int rowNumber;

        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const srcrow = &src[rowNumber * srcwid];
            unsigned int const dstRowCurs = rowNumber * dstwid;
            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                struct RGBColor const srcColor =
                    decode16(&srcrow[colNumber * 2]);
                dst.red[dstCursor] = srcColor.red << 11;
                dst.grn[dstCursor] = srcColor.grn << 11;
                dst.blu[dstCursor] = srcColor.blu << 11;
            }
        }
    } break;
    case 32: {
        unsigned int const planeSize = srcwid / 4;
        unsigned int rowNumber;

        for (rowNumber = 0; rowNumber < ysize; ++rowNumber) {
            unsigned char * const srcrow = &src[rowNumber * srcwid];
            unsigned char * const redPlane = &srcrow[planeSize * 0];
            unsigned char * const grnPlane = &srcrow[planeSize * 1];
            unsigned char * const bluPlane = &srcrow[planeSize * 2];
            unsigned int const dstRowCurs = rowNumber * dstwid;

            unsigned int colNumber;
            for (colNumber = 0; colNumber < xsize; ++colNumber) {
                unsigned int const dstCursor = dstRowCurs + colNumber;
                dst.red[dstCursor] = redPlane[colNumber] << 8;
                dst.grn[dstCursor] = grnPlane[colNumber] << 8;
                dst.blu[dstCursor] = bluPlane[colNumber] << 8;
            }
        }
    } break;
    default:
        pm_error("INTERNAL ERROR: invalid bits per pixel (%u) in "
                 "blitIdempotent()", pixSize);
    }
}



static void
generalBlit(struct Rect       const srcRect, 
            struct Rect       const srcBounds, 
            struct raster     const srcplane,
            int               const pixSize, 
            struct Rect       const dstRect, 
            struct Rect       const dstBounds, 
            int               const dstwid, 
            struct RGBColor * const color_map, 
            int               const mode,
            struct Rect       const clipsrc,
            struct Rect       const clipdst) {
    
    unsigned char * src;
    struct rgbPlanes dst;
    int dstoff;
    int xsize;
    int ysize;
    int srcadd;
    transfer_func trf;

    if (verbose) {
        dumpRect("copying from:", clipsrc);
        dumpRect("to:          ", clipdst);
        pm_message("a %u x %u area to a %u x %u area",
                   rectwidth(&clipsrc), rectheight(&clipsrc),
                   rectwidth(&clipdst), rectheight(&clipdst));
    }

    {
        unsigned int const pkpixsize = pixSize == 16 ? 2 : 1;
        unsigned int const srcRowNumber = clipsrc.top - srcBounds.top;
        unsigned int const srcRowOffset =
            (clipsrc.left - srcBounds.left) * pkpixsize;
        assert(srcRowNumber < srcplane.rowCount);
        assert(srcRowOffset < srcplane.rowSize);
        src = srcplane.bytes + srcRowNumber * srcplane.rowSize + srcRowOffset;
        xsize = clipsrc.right - clipsrc.left;
        ysize = clipsrc.bottom - clipsrc.top;
        srcadd = srcplane.rowSize - xsize * pkpixsize;
    }

    dstoff = (clipdst.top - dstBounds.top) * dstwid +
        (clipdst.left - dstBounds.left);
    dst.red = red + dstoff;
    dst.grn = green + dstoff;
    dst.blu = blue + dstoff;

    /* get rid of Text mask mode bit, if (erroneously) set */
    if ((mode & ~64) == 0)
        trf = NULL;    /* optimized srcCopy */
    else
        trf = transfer(mode & ~64);

    if (!rectsamesize(&clipsrc, &clipdst))
        doDiffSize(clipsrc, clipdst, pixSize, xsize, ysize,
                   trf, color_map, src, srcplane.rowSize, dst, dstwid);
    else {
        if (trf == NULL)
            blitIdempotent(pixSize, xsize, ysize, src, srcplane.rowSize,
                           color_map, dst, dstwid);
        else
            doSameSize(trf, pixSize, xsize, ysize, src, srcplane.rowSize,
                       color_map, dst, dstwid);
    }
}



static int
blit(struct Rect       const srcRect, 
     struct Rect       const srcBounds, 
     struct raster     const srcplane,
     int               const pixSize, 
     struct Rect       const dstRect, 
     struct Rect       const dstBounds, 
     int               const dstwid, 
     struct RGBColor * const color_map, 
     int               const mode) {

    /* I can't tell what the result value of this function is supposed to mean,
       but I found several return statements that did not set it to anything,
       and several calls that examine it.  I'm guessing that "1" is the 
       appropriate thing to return in those cases, so I made it so.
       -Bryan 00.03.02
    */

    int retval;

    if (ps_text)
        retval = 1;
    else {
        /* Almost got it.  Clip source rect with source bounds.
           clip dest rect with dest bounds.  If source and
           destination are not the same size, use Pnmscale
           to get a nicely sized rectangle.
        */
        struct Rect clipsrc;
        struct Rect clipdst;

        rectinter(srcBounds, srcRect, &clipsrc);
        rectinter(dstBounds, dstRect, &clipdst);

        if (fullres) {
            struct blit_info * bi;
            bi = add_blit_list();
            bi->srcRect   = clipsrc;
            bi->srcBounds = srcBounds;
            bi->srcplane  = srcplane;
            bi->pixSize   = pixSize;
            bi->dstRect   = clipdst;
            bi->color_map = color_map;
            bi->mode      = mode;

            retval = 0;
        } else {
            generalBlit(srcRect, srcBounds, srcplane, pixSize,
                        dstRect, dstBounds, dstwid, color_map, mode,
                        clipsrc, clipdst);

            retval = 1;
        }
    }
    return retval;
}



/* allocation is same for version 1 or version 2.  We are super-duper
 * wasteful of memory for version 1 picts.  Someday, we'll separate
 * things and only allocate a byte per pixel for version 1 (or heck,
 * even only a bit, but that would require even more extra work).
 */

static void 
allocPlanes(struct rgbPlanes * const planesP) {

    struct rgbPlanes planes;

    rowlen = picFrame.right - picFrame.left;
    collen = picFrame.bottom - picFrame.top;

    clip_rect = picFrame;

    planelen = rowlen * collen;
    MALLOCARRAY(planes.red,  planelen);
    MALLOCARRAY(planes.grn, planelen);
    MALLOCARRAY(planes.blu, planelen);
    if (planes.red == NULL || planes.grn == NULL || planes.blu == NULL)
        pm_error("not enough memory to hold picture");

    /* initialize background to white */
    memset(planes.red, 255, planelen * sizeof(word));
    memset(planes.grn, 255, planelen * sizeof(word));
    memset(planes.blu, 255, planelen * sizeof(word));

    *planesP = planes;

    /* Until we wean this program off of global variables, we have to
       set these:
    */

    red   = planes.red;
    green = planes.grn;
    blue  = planes.blu;
}



static void
freePlanes(struct rgbPlanes const planes) {

    free(planes.red);
    free(planes.grn);
    free(planes.blu);
}



static unsigned char
compact(word const input) {

    return (input >> 8) & 0xff;
}



static void
do_blits(struct rgbPlanes * const planesP) {

    struct blit_info* bi;
    int srcwidth, dstwidth, srcheight, dstheight;
    double  scale, scalelow, scalehigh;
    double  xscale = 1.0;
    double  yscale = 1.0;
    double  lowxscale, highxscale, lowyscale, highyscale;
    int     xscalecalc = 0, yscalecalc = 0;

    fullres = 0;

    for (bi = blit_list; bi; bi = bi->next) {
        srcwidth = rectwidth(&bi->srcRect);
        dstwidth = rectwidth(&bi->dstRect);
        srcheight = rectheight(&bi->srcRect);
        dstheight = rectheight(&bi->dstRect);
        if (srcwidth > dstwidth) {
            scalelow  = (double)(srcwidth      ) / (double)dstwidth;
            scalehigh = (double)(srcwidth + 1.0) / (double)dstwidth;
            switch (xscalecalc) {
            case 0:
                lowxscale = scalelow;
                highxscale = scalehigh;
                xscalecalc = 1;
                break;
            case 1:
                if (scalelow < highxscale && scalehigh > lowxscale) {
                    if (scalelow > lowxscale) lowxscale = scalelow;
                    if (scalehigh < highxscale) highxscale = scalehigh;
                }
                else {
                    scale = (lowxscale + highxscale) / 2.0;
                    xscale = (double)srcwidth / (double)dstwidth;
                    if (scale > xscale) xscale = scale;
                    xscalecalc = 2;
                }
                break;
            case 2:
                scale = (double)srcwidth / (double)dstwidth;
                if (scale > xscale) xscale = scale;
                break;
            }
        }

        if (srcheight > dstheight) {
            scalelow =  (double)(srcheight      ) / (double)dstheight;
            scalehigh = (double)(srcheight + 1.0) / (double)dstheight;
            switch (yscalecalc) {
            case 0:
                lowyscale = scalelow;
                highyscale = scalehigh;
                yscalecalc = 1;
                break;
            case 1:
                if (scalelow < highyscale && scalehigh > lowyscale) {
                    if (scalelow > lowyscale) lowyscale = scalelow;
                    if (scalehigh < highyscale) highyscale = scalehigh;
                }
                else {
                    scale = (lowyscale + highyscale) / 2.0;
                    yscale = (double)srcheight / (double)dstheight;
                    if (scale > yscale) yscale = scale;
                    yscalecalc = 2;
                }
                break;
            case 2:
                scale = (double)srcheight / (double)dstheight;
                if (scale > yscale) yscale = scale;
                break;
            }
        }
    }

    if (xscalecalc == 1) {
        if (1.0 >= lowxscale && 1.0 <= highxscale)
            xscale = 1.0;
        else
            xscale = lowxscale;
    }
    if (yscalecalc == 1) {
        if (1.0 >= lowyscale && 1.0 <= lowyscale)
            yscale = 1.0;
        else
            yscale = lowyscale;
    }

    if (xscale != 1.0 || yscale != 1.0) {
        for (bi = blit_list; bi; bi = bi->next)
            rectscale(&bi->dstRect, xscale, yscale);

        pm_message("Scaling output by %f in X and %f in Y",
                   xscale, yscale);
        rectscale(&picFrame, xscale, yscale);
    }

    allocPlanes(planesP);

    for (bi = blit_list; bi; bi = bi->next) {
        blit(bi->srcRect, bi->srcBounds, bi->srcplane,
             bi->pixSize,
             bi->dstRect, picFrame, rowlen,
             bi->color_map,
             bi->mode);
    }
}



static void
outputPpm(struct rgbPlanes const planes) {

    unsigned int width;
    unsigned int height;
    pixel * pixelrow;
    unsigned int row;
    unsigned int srcCursor;

    stage = "writing PPM";

    assert(picFrame.right  > picFrame.left);
    assert(picFrame.bottom > picFrame.top);

    width  = picFrame.right  - picFrame.left;
    height = picFrame.bottom - picFrame.top;

    ppm_writeppminit(stdout, width, height, PPM_MAXMAXVAL, 0);
    pixelrow = ppm_allocrow(width);
    srcCursor = 0;
    for (row = 0; row < height; ++row) {
        unsigned int col;
        for (col = 0; col < width; ++col) {
            PPM_ASSIGN(pixelrow[col],
                       compact(planes.red[srcCursor]),
                       compact(planes.grn[srcCursor]),
                       compact(planes.blu[srcCursor])
                );
            ++srcCursor;
        }
        ppm_writeppmrow(stdout, pixelrow, width, PPM_MAXMAXVAL, 0);
    }
    pm_close(stdout);
}



/*
 * All data in version 2 is 2-byte word aligned.  Odd size data
 * is padded with a null.
 */
static word
get_op(int const version) {
    if ((align & 1) && version == 2) {
        stage = "aligning for opcode";
        read_byte();
    }

    stage = "reading opcode";

    if (version == 1)
        return read_byte();
    else
        return read_word();
}



static void
Clip(int const version) {
    word len;

    len = read_word();

    if (len == 0x000a) {    /* null rgn */
        read_rect(&clip_rect);
        /* XXX should clip this by picFrame */
        if (verbose)
            dumpRect("clipping to", clip_rect);
    }
    else
        skip(len - 2);
}



static void
OpColor(int const version) {
    op_color.red = read_word();
    op_color.grn = read_word();
    op_color.blu = read_word();
}



static void
read_pixmap(struct pixMap * const p) {

    stage = "getting pixMap header";

    read_rect(&p->Bounds);
    p->version = read_word();
    p->packType = read_word();
    p->packSize = read_long();
    p->hRes = read_long();
    p->vRes = read_long();
    p->pixelType = read_word();
    p->pixelSize = read_word();
    p->cmpCount = read_word();
    p->cmpSize = read_word();
    p->planeBytes = read_long();
    p->pmTable = read_long();
    p->pmReserved = read_long();

    if (verbose) {
        pm_message("pixelType: %d", p->pixelType);
        pm_message("pixelSize: %d", p->pixelSize);
        pm_message("cmpCount:  %d", p->cmpCount);
        pm_message("cmpSize:   %d", p->cmpSize);
    }

    if (p->pixelType != 0)
        pm_error("sorry, I do only chunky format.  "
                 "This image has pixel type %hu", p->pixelType);
    if (p->cmpCount != 1)
        pm_error("sorry, cmpCount != 1");
    if (p->pixelSize != p->cmpSize)
        pm_error("oops, pixelSize != cmpSize");
}



static struct RGBColor*
read_color_table(void) {
    longword ctSeed;
    word ctFlags;
    word ctSize;
    word val;
    int i;
    struct RGBColor* color_table;

    stage = "getting color table info";

    ctSeed = read_long();
    ctFlags = read_word();
    ctSize = read_word();

    if (verbose) {
        pm_message("ctSeed:  %ld", ctSeed);
        pm_message("ctFlags: %d", ctFlags);
        pm_message("ctSize:  %d", ctSize);
    }

    stage = "reading color table";

    MALLOCARRAY(color_table, ctSize + 1);
    if (color_table == NULL)
        pm_error("no memory for color table");

    for (i = 0; i <= ctSize; i++) {
        val = read_word();
        /* The indices in a device color table are bogus and usually == 0.
         * so I assume we allocate up the list of colors in order.
         */
        if (ctFlags & 0x8000)
            val = i;
        if (val > ctSize)
            pm_error("pixel value greater than color table size");
        color_table[val].red = read_word();
        color_table[val].grn = read_word();
        color_table[val].blu = read_word();

        if (verbose > 1)
            pm_message("%d: [%d,%d,%d]", val,
                color_table[val].red,
                color_table[val].grn,
                color_table[val].blu);
    }

    return color_table;
}



static void
readBytes(FILE *          const ifP,
          unsigned int    const n, 
          unsigned char * const buf) {

    align += n;

    if (fread(buf, n, 1, ifP) != 1)
        pm_error("EOF / read error while %s", stage);
}



static void
copyFullBytes(unsigned char * const packed,
              unsigned char * const expanded,
              unsigned int    const packedLen) {

    unsigned int i;

    for (i = 0; i < packedLen; ++i)
        expanded[i] = packed[i];
}



static void
expand4Bits(unsigned char * const packed,
            unsigned char * const expanded,
            unsigned int    const packedLen) {

    unsigned int i;
    unsigned char * dst;

    dst = &expanded[0];
    
    for (i = 0; i < packedLen; ++i) {
        *dst++ = (packed[i] >> 4) & 0x0f;
        *dst++ = (packed[i] >> 0) & 0x0f;
    }
}



static void
expand2Bits(unsigned char * const packed,
            unsigned char * const expanded,
            unsigned int    const packedLen) {
        
    unsigned int i;
    unsigned char * dst;

    dst = &expanded[0];
    
    for (i = 0; i < packedLen; ++i) {
        *dst++ = (packed[i] >> 6) & 0x03;
        *dst++ = (packed[i] >> 4) & 0x03;
        *dst++ = (packed[i] >> 2) & 0x03;
        *dst++ = (packed[i] >> 0) & 0x03;
    }
}



static void
expand1Bit(unsigned char * const packed,
           unsigned char * const expanded,
           unsigned int    const packedLen) {

    unsigned int i;
    unsigned char * dst;

    dst = &expanded[0];
    
    for (i = 0; i < packedLen; ++i) {
        *dst++ = (packed[i] >> 7) & 0x01;
        *dst++ = (packed[i] >> 6) & 0x01;
        *dst++ = (packed[i] >> 5) & 0x01;
        *dst++ = (packed[i] >> 4) & 0x01;
        *dst++ = (packed[i] >> 3) & 0x01;
        *dst++ = (packed[i] >> 2) & 0x01;
        *dst++ = (packed[i] >> 1) & 0x01;
        *dst++ = (packed[i] >> 0) & 0x01;
    }
}



static void
unpackBuf(unsigned char *  const packed, 
          unsigned int     const packedLen,
          int              const bitsPerPixel,
          unsigned char ** const expandedP,
          unsigned int *   const expandedLenP) {
/*----------------------------------------------------------------------------
   Expand the bit string 'packed', which is 'packedLen' bytes long
   into an array of bytes, with one byte per pixel.  Each 'bitsPerPixel'
   of 'packed' is a pixel.

   So e.g. if it's 4 bits per pixel and 'packed' is 0xabcdef01, we
   return 0x0a0b0c0d0e0f0001 as *expandedP.

   As a special case, if there are multiple bytes per pixel, we just
   return the exact same bit string.

   *expandedP is static storage.

   'packedLen' must not be greater than 256.
-----------------------------------------------------------------------------*/
    static unsigned char expanded[256 * 8];
    unsigned char * src;
    unsigned char * dst;

    assert(packedLen <= 256);

    src = &packed[0];
    dst = &expanded[0];

    switch (bitsPerPixel) {
    case 8:
    case 16:
    case 32:
        assert(packedLen <= sizeof(expanded));
        copyFullBytes(packed, expanded, packedLen);
        *expandedLenP = packedLen;
        break;
    case 4:
        assert(packedLen * 2 <= sizeof(expanded));
        expand4Bits(packed, expanded, packedLen);
        *expandedLenP = packedLen * 2;
        break;
    case 2:
        assert(packedLen * 4 <= sizeof(expanded));
        expand2Bits(packed, expanded, packedLen);
        *expandedLenP = packedLen * 4;
        break;
    case 1:
        assert(packedLen * 8 <= sizeof(expanded));
        expand1Bit(packed, expanded, packedLen);
        *expandedLenP = packedLen * 8;
        break;
    default:
        pm_error("INTERNAL ERROR: bitsPerPixel = %u in unpackBuf",
                 bitsPerPixel);
    }
    *expandedP = expanded;
}



static void
unpackUncompressedBits(FILE *          const ifP,
                       struct raster   const raster,
                       unsigned int    const rowBytes,
                       unsigned int    const bitsPerPixel) {
/*----------------------------------------------------------------------------
   Read the raster from the file into 'raster'.  The data in the file is not
   compressed (but may still be packed multiple pixels per byte).

   In PICT terminology, it appears that compression is called
   "packing" and I don't know what packing is called.  But we don't
   use that confusing terminology in this program, except when talking
   to the user.
-----------------------------------------------------------------------------*/
    unsigned int rowOfRect;
    unsigned char * linebuf;

    MALLOCARRAY(linebuf, rowBytes + 100);
    if (linebuf == NULL)
        pm_error("can't allocate memory for line buffer");

    for (rowOfRect = 0; rowOfRect < raster.rowCount; ++rowOfRect) { 
        unsigned char * bytePixels;
        unsigned int expandedByteCount;
        unsigned char * rasterRow;
        unsigned int i;

        rasterRow = raster.bytes + rowOfRect * raster.rowSize;

        readBytes(ifP, rowBytes, linebuf);

        unpackBuf(linebuf, rowBytes, bitsPerPixel,
                  &bytePixels, &expandedByteCount);

        assert(expandedByteCount <= raster.rowSize);

        for (i = 0; i < expandedByteCount; ++i)
            rasterRow[i] = bytePixels[i];
    }
    free(linebuf);
}



static void
expandRun(unsigned char * const block,
          unsigned int    const blockLimit,
          unsigned int    const bitsPerPixel,
          unsigned char * const expanded,
          unsigned int    const expandedSize,
          unsigned int *  const blockLengthP,
          unsigned int *  const expandedByteCountP) {
/*----------------------------------------------------------------------------
   Expand a run (the data says, "repeat the next pixel N times").

   Return the expanded run as expanded[], which has room for 'expandedSize'
   elements.  Return as *expandedByteCountP the number of elements actually
   returned.
-----------------------------------------------------------------------------*/
    unsigned int const pkpixsize = bitsPerPixel == 16 ? 2 : 1;

    if (1 + pkpixsize > blockLimit)
        pm_error("PICT run block runs off the end of its line.  "
                 "Invalid PICT file.");
    else {
        unsigned int const runLength = (block[0] ^ 0xff) + 2;
        
        unsigned int i;
        unsigned char * bytePixels;  /* Points to static storage */
        unsigned int expandedByteCount;
        unsigned int outputCursor;
        
        assert(block[0] & 0x80);  /* It's a run */

        if (verbose > 1)
            pm_message("Block: run of %u pixels or plane samples", runLength);
        
        unpackBuf(&block[1], pkpixsize, bitsPerPixel,
                  &bytePixels, &expandedByteCount);
        
        if (expandedByteCount > expandedSize)
            pm_error("Invalid PICT image.  It contains a row with more pixels "
                     "than the width of the image");
        
        outputCursor = 0;
        for (i = 0; i < runLength; ++i) {
            unsigned int j;
            for (j = 0; j < expandedByteCount; ++j)
                expanded[outputCursor++] = bytePixels[j];
        }
        *blockLengthP = 1 + pkpixsize;
        *expandedByteCountP = expandedByteCount * runLength;
    }
}



static void
copyPixelGroup(unsigned char * const block,
               unsigned int    const blockLimit,
               unsigned int    const bitsPerPixel,
               unsigned char * const dest,
               unsigned int    const destSize,
               unsigned int *  const blockLengthP,
               unsigned int *  const rasterBytesGeneratedP) {
/*----------------------------------------------------------------------------
   Copy a group of pixels (the data says, "take the following N pixels").
-----------------------------------------------------------------------------*/
    unsigned int const pkpixsize = bitsPerPixel == 16 ? 2 : 1;
    unsigned int const groupLen = block[0] + 1;
    unsigned int const blockLength = 1 + groupLen * pkpixsize;

    if (blockLength > blockLimit)
        pm_error("PICT non-run block (length %u) "
                 "runs off the end of its line.  "
                 "Invalid PICT file.", blockLength);
    else {
        unsigned int i;
        unsigned char * bytePixels;  /* Points to static storage */
        unsigned int bytePixelLen;
        
        assert(blockLimit >= 1);  /* block[0] exists */
        assert((block[0] & 0x80) == 0);  /* It's not a run */
        
        if (verbose > 1)
            pm_message("Block: %u individual pixels or plane samples",
                       groupLen);
        
        unpackBuf(&block[1], groupLen * pkpixsize, bitsPerPixel,
                  &bytePixels, &bytePixelLen);
        
        for (i = 0; i < MIN(bytePixelLen, destSize); ++i)
            dest[i] = bytePixels[i];
        
        *blockLengthP = blockLength;
        *rasterBytesGeneratedP = MIN(bytePixelLen, destSize);
    }
}



static void
interpretOneRasterBlock(unsigned char * const block,
                        unsigned int    const blockLimit,
                        unsigned int    const bitsPerPixel,
                        unsigned char * const raster,
                        unsigned int    const rasterSize,
                        unsigned int *  const blockLengthP,
                        unsigned int *  const rasterBytesGeneratedP) {
/*----------------------------------------------------------------------------
   Return the pixels described by the PICT block block[], assuming
   the PICT format is 'bitsPerPixel' bits per pixel.

   Return them as raster[], which has 'rasterSize' elements of space.
   Return as *rasterBytesGeneratedP the number of elements actually
   returned.

   block[] self-describes its length, and we return that as
   *blockLengthP.  But there are at most 'blockLimit' bytes there, so
   if block[] claims to be longer than that, some of the block is
   missing (i.e. corrupt PICT).
-----------------------------------------------------------------------------*/
    if (block[0] & 0x80)
        expandRun(block, blockLimit, bitsPerPixel, raster, rasterSize,
                  blockLengthP, rasterBytesGeneratedP);
    else
        copyPixelGroup(block, blockLimit, bitsPerPixel, raster, rasterSize,
                       blockLengthP, rasterBytesGeneratedP);

    assert(*rasterBytesGeneratedP <= rasterSize);
}



static unsigned int const maxPixelBytesPerBlock = 1024;



static void
unpackCompressedBits(FILE *          const ifP,
                     struct raster   const raster,
                     unsigned int    const rowBytes,
                     unsigned int    const bitsPerPixel) {
/*----------------------------------------------------------------------------
   Read the raster of on file *ifP and place it in 'raster'.

   The data in the file is compressed with run length encoding and
   possibly packed multiple pixels per byte as well.

   In PICT terminology, it appears that compression is called
   "packing" and I don't know what packing is called.  But we don't
   use that confusing terminology in this program, except when talking
   to the user.

   *boundsP describes the rectangle.
-----------------------------------------------------------------------------*/
    unsigned int const llsize = rowBytes > 200 ? 2 : 1;
    unsigned int rowOfRect;
    unsigned char * linebuf;
    unsigned int linebufSize;

    linebufSize = rowBytes;
    MALLOCARRAY(linebuf, linebufSize);
    if (linebuf == NULL)
        pm_error("can't allocate memory for line buffer");

    for (rowOfRect = 0; rowOfRect < raster.rowCount; ++rowOfRect) {
        unsigned char * const rowRaster =
            &raster.bytes[rowOfRect * raster.rowSize];
        unsigned int linelen;
        unsigned int lineCursor;
        unsigned int rasterCursor;

        if (llsize == 2)
            linelen = read_word();
        else
            linelen = read_byte();

        if (verbose > 1)
            pm_message("linelen: %u", linelen);

        if (linelen > linebufSize) {
            linebufSize = linelen;
            REALLOCARRAY(linebuf, linebufSize);
            if (linebuf == NULL)
                pm_error("can't allocate memory for line buffer");
        }
        readBytes(ifP, linelen, linebuf);

        for (lineCursor = 0, rasterCursor = 0; lineCursor < linelen; ) {
            unsigned int blockLength, rasterBytesGenerated;

            assert(lineCursor <= linelen);
            
            interpretOneRasterBlock(
                &linebuf[lineCursor], linelen - lineCursor,
                bitsPerPixel,
                &rowRaster[rasterCursor], raster.rowSize - rasterCursor,
                &blockLength, &rasterBytesGenerated);

            lineCursor += blockLength;
            rasterCursor += rasterBytesGenerated;
            assert(rasterCursor <= raster.rowSize);
        }
        if (verbose > 1)
            pm_message("row %u: got %u", rowOfRect, rasterCursor);
    }
    free(linebuf);
}



static void
unpackbits(FILE *           const ifP,
           struct Rect *    const boundsP,
           word             const rowBytesArg, 
           int              const bitsPerPixel,
           struct raster *  const rasterP) {

    unsigned int const rectHeight = boundsP->bottom - boundsP->top;
    unsigned int const rectWidth  = boundsP->right  - boundsP->left;
    
    struct raster raster;
    unsigned int rowBytes;

    stage = "unpacking packbits";

    if (verbose)
        pm_message("rowBytes = %u, bitsPerPixel = %d",
                   rowBytesArg, bitsPerPixel);
        
    allocateRaster(&raster, rectWidth, rectHeight, bitsPerPixel);

    rowBytes = rowBytesArg ? rowBytesArg : raster.rowSize;

    if (verbose)
        pm_message("raster.rowSize: %u bytes; file row = %u bytes",
                   raster.rowSize, rowBytes);

    if (rowBytes < 8)
        unpackUncompressedBits(ifP, raster, rowBytes, bitsPerPixel);
    else
        unpackCompressedBits(ifP, raster, rowBytes, bitsPerPixel);

    *rasterP = raster;
}



static void
interpretRowBytesWord(word           const rowBytesWord,
                      bool *         const pixMapP,
                      unsigned int * const rowBytesP) {

    *pixMapP   = !!(rowBytesWord & 0x8000);
    *rowBytesP = rowBytesWord & 0x7fff;

    if (verbose)
        pm_message("PCT says %s, %u bytes per row",
                   *pixMapP ? "pixmap" : "bitmap", *rowBytesP);
}



/* this just skips over a version 2 pattern.  Probably will return
 * a pattern in the fabled complete version.
 */
static void
read_pattern(void) {

    word PatType;

    stage = "Reading a pattern";

    PatType = read_word();

    switch (PatType) {
    case 2:
        skip(8); /* old pattern data */
        skip(5); /* RGB for pattern */
        break;
    case 1: {
        word rowBytesWord;
        bool pixMap;
        unsigned int rowBytes;
        struct pixMap p;
        struct raster raster;
        struct RGBColor * ct;

        skip(8); /* old pattern data */
        rowBytesWord = read_word();
        interpretRowBytesWord(rowBytesWord, &pixMap, &rowBytes);
        read_pixmap(&p);
        ct = read_color_table();
        unpackbits(ifp, &p.Bounds, rowBytes, p.pixelSize, &raster);
        freeRaster(raster);
        free(ct);
    } break;
    default:
        pm_error("unknown pattern type in read_pattern");
    }
}



/* these 3 do nothing but skip over their data! */

static void
BkPixPat(int const version) {
    read_pattern();
}

static void
PnPixPat(int const version) {
    read_pattern();
}

static void
FillPixPat(int const version) {
    read_pattern();
}

static void
read_8x8_pattern(struct Pattern * const pat) {
    unsigned char buf[8];
    unsigned char * exp;
    unsigned int len;
    unsigned int expandedLen;
    unsigned int i;

    len = 8;  /* initial value */
    readBytes(ifp, len, buf);
    if (verbose) {
        pm_message("pattern: %02x%02x%02x%02x",
                   buf[0], buf[1], buf[2], buf[3]);
        pm_message("pattern: %02x%02x%02x%02x",
            buf[4], buf[5], buf[6], buf[7]);
    }
    unpackBuf(buf, len, 1, &exp, &expandedLen);
    for (i = 0; i < 64; ++i)
        pat->pix[i] = exp[i];
}



static void 
BkPat(int const version) {
    read_8x8_pattern(&bkpat);
}



static void 
PnPat(int const version) {
    read_8x8_pattern(&pen_pat);
}



static void 
FillPat(int const version) {
    read_8x8_pattern(&fillpat);
}



static void 
PnSize(int const version) {
    pen_height = read_word();
    pen_width = read_word();
    if (verbose)
        pm_message("pen size %d x %d", pen_width, pen_height);
}



static void 
PnMode(int const version) {

    pen_mode = read_word();

    if (pen_mode >= 8 && pen_mode < 15)
        pen_mode -= 8;
    if (verbose)
        pm_message("pen transfer mode = %s",
            const_name(transfer_name, pen_mode));
    
    pen_trf = transfer(pen_mode);
}



static void 
read_rgb(struct RGBColor * const rgb) {
    rgb->red = read_word();
    rgb->grn = read_word();
    rgb->blu = read_word();
}



static void 
RGBFgCol(int const v) {
    read_rgb(&foreground);
    if (verbose)
        pm_message("foreground now [%d,%d,%d]", 
            foreground.red, foreground.grn, foreground.blu);
}



static void 
RGBBkCol(int const v) {
    read_rgb(&background);
    if (verbose)
        pm_message("background now [%d,%d,%d]", 
            background.red, background.grn, background.blu);
}



#define PIXEL_INDEX(x,y) ((y) - picFrame.top) * rowlen + (x) - picFrame.left

static void 
draw_pixel(int                const x, 
           int                const y, 
           struct RGBColor * const clr, 
           transfer_func            trf) {

    int i;
    struct RGBColor dst;

    if (x < clip_rect.left || x >= clip_rect.right ||
        y < clip_rect.top || y >= clip_rect.bottom)
    {
        return;
    }

    i = PIXEL_INDEX(x, y);
    dst.red = red[i];
    dst.grn = green[i];
    dst.blu = blue[i];
    (*trf)(clr, &dst);
    red[i] = dst.red;
    green[i] = dst.grn;
    blue[i] = dst.blu;
}



static void 
draw_pen_rect(struct Rect * const r) {
    int const rowadd = rowlen - (r->right - r->left);

    int i;
    int x, y;
    struct RGBColor dst;

    i = PIXEL_INDEX(r->left, r->top);  /* initial value */
    
    for (y = r->top; y < r->bottom; y++) {
        for (x = r->left; x < r->right; x++) {
            dst.red = red[i];
            dst.grn = green[i];
            dst.blu = blue[i];
            if (pen_pat.pix[(x & 7) + (y & 7) * 8])
                (*pen_trf)(&black, &dst);
            else
                (*pen_trf)(&white, &dst);
            red[i] = dst.red;
            green[i] = dst.grn;
            blue[i] = dst.blu;

            i++;
        }
        i += rowadd;
    }
}



static void 
draw_pen(int const x, 
         int const y) {
    struct Rect penrect;

    penrect.left = x;
    penrect.right = x + pen_width;
    penrect.top = y;
    penrect.bottom = y + pen_height;

    rectinter(penrect, clip_rect, &penrect);

    draw_pen_rect(&penrect);
}

/*
 * Digital Line Drawing
 * by Paul Heckbert
 * from "Graphics Gems", Academic Press, 1990
 */

/*
 * digline: draw digital line from (x1,y1) to (x2,y2),
 * calling a user-supplied procedure at each pixel.
 * Does no clipping.  Uses Bresenham's algorithm.
 *
 * Paul Heckbert    3 Sep 85
 */
static void 
scan_line(short const x1, 
          short const y1, 
          short const x2, 
          short const y2) {
    int d, x, y, ax, ay, sx, sy, dx, dy;

    if (!(pen_width == 0 && pen_height == 0)) {

        dx = x2-x1;  ax = ABS(dx)<<1;  sx = SGN(dx);
        dy = y2-y1;  ay = ABS(dy)<<1;  sy = SGN(dy);

        x = x1;
        y = y1;
        if (ax>ay) {        /* x dominant */
            d = ay-(ax>>1);
            for (;;) {
                draw_pen(x, y);
                if (x==x2) return;
                if ((x > rowlen) && (sx > 0)) return;
                if (d>=0) {
                    y += sy;
                    d -= ax;
                }
                x += sx;
                d += ay;
            }
        }
        else {          /* y dominant */
            d = ax-(ay>>1);
            for (;;) {
                draw_pen(x, y);
                if (y==y2) return;
                if ((y > collen) && (sy > 0)) return;
                if (d>=0) {
                    x += sx;
                    d -= ay;
                }
                y += sy;
                d += ax;
            }
        }
    }
}



static void 
Line(int const v) {
  struct Point p1;
  read_point(&p1);
  read_point(&current);
  if (verbose)
    pm_message("(%d,%d) to (%d, %d)",
           p1.x,p1.y,current.x,current.y);
  scan_line(p1.x,p1.y,current.x,current.y);
}



static void 
LineFrom(int const v) {
  struct Point p1;
  read_point(&p1);
  if (verbose)
    pm_message("(%d,%d) to (%d, %d)",
           current.x,current.y,p1.x,p1.y);

  if (!fullres)
      scan_line(current.x,current.y,p1.x,p1.y);

  current.x = p1.x;
  current.y = p1.y;
}



static void 
ShortLine(int const v) {
  struct Point p1;
  read_point(&p1);
  read_short_point(&current);
  if (verbose)
    pm_message("(%d,%d) delta (%d, %d)",
           p1.x,p1.y,current.x,current.y);
  current.x += p1.x;
  current.y += p1.y;

  if (!fullres)
      scan_line(p1.x,p1.y,current.x,current.y);
}



static void 
ShortLineFrom(int const v) {
  struct Point p1;
  read_short_point(&p1);
  if (verbose)
    pm_message("(%d,%d) delta (%d, %d)",
               current.x,current.y,p1.x,p1.y);
  p1.x += current.x;
  p1.y += current.y;
  if (!fullres)
      scan_line(current.x,current.y,p1.x,p1.y);
  current.x = p1.x;
  current.y = p1.y;
}

static void 
do_paintRect(struct Rect const prect) {
    struct Rect rect;
  
    if (fullres)
        return;

    if (verbose)
        dumpRect("painting", prect);

    rectinter(clip_rect, prect, &rect);

    draw_pen_rect(&rect);
}



static void 
paintRect(int const v) {
    read_rect(&cur_rect);
    do_paintRect(cur_rect);
}



static void 
paintSameRect(int const v) {
    do_paintRect(cur_rect);
}



static void 
do_frameRect(struct Rect const rect) {
    int x, y;

    if (fullres)
        return;
  
    if (verbose)
        dumpRect("framing", rect);

    if (pen_width == 0 || pen_height == 0)
        return;

    for (x = rect.left; x <= rect.right - pen_width; x += pen_width) {
        draw_pen(x, rect.top);
        draw_pen(x, rect.bottom - pen_height);
    }

    for (y = rect.top; y <= rect.bottom - pen_height ; y += pen_height) {
        draw_pen(rect.left, y);
        draw_pen(rect.right - pen_width, y);
    }
}



static void 
frameRect(int const v) {
    read_rect(&cur_rect);
    do_frameRect(cur_rect);
}



static void 
frameSameRect(int const v) {
    do_frameRect(cur_rect);
}



/* a stupid shell sort - I'm so embarassed  */

static void 
poly_sort(int const sort_index, struct Point points[]) {
  int d, i, j, temp;

  /* initialize and set up sort interval */
  d = 4;
  while (d<=sort_index) d <<= 1;
  d -= 1;

  while (d > 1) {
    d >>= 1;
    for (j = 0; j <= (sort_index-d); j++) {
      for(i = j; i >= 0; i -= d) {
    if ((points[i+d].y < points[i].y) ||
        ((points[i+d].y == points[i].y) &&
         (points[i+d].x <= points[i].x))) {
      /* swap x1,y1 with x2,y2 */
      temp = points[i].y;
      points[i].y = points[i+d].y;
      points[i+d].y = temp;
      temp = points[i].x;
      points[i].x = points[i+d].x;
      points[i+d].x = temp;
    }
      }
    }
  }
}



/* Watch out for the lack of error checking in the next two functions ... */

static void 
scan_poly(int          const np, 
          struct Point       pts[]) {
  int dx,dy,dxabs,dyabs,i,scan_index,j,k,px,py;
  int sdx,sdy,x,y,toggle,old_sdy,sy0;

  /* This array needs to be at least as large as the largest dimension of
     the bounding box of the poly (but I don't check for overflows ...) */
  struct Point coord[5000];

  scan_index = 0;

  /* close polygon */
  px = pts[np].x = pts[0].x;
  py = pts[np].y = pts[0].y;

  /*  This section draws the polygon and stores all the line points
   *  in an array. This doesn't work for concave or non-simple polys.
   */
  /* are y levels same for first and second points? */
  if (pts[1].y == pts[0].y) {
    coord[scan_index].x = px;
    coord[scan_index].y = py;
    scan_index++;
  }

#define sign(x) ((x) > 0 ? 1 : ((x)==0 ? 0:(-1)) )   

  old_sdy = sy0 = sign(pts[1].y - pts[0].y);
  for (j=0; j<np; j++) {
    /* x,y difference between consecutive points and their signs  */
    dx = pts[j+1].x - pts[j].x;
    dy = pts[j+1].y - pts[j].y;
    sdx = SGN(dx);
    sdy = SGN(dy);
    dxabs = abs(dx);
    dyabs = abs(dy);
    x = y = 0;

    if (dxabs >= dyabs)
      {
    for (k=0; k < dxabs; k++) {
      y += dyabs;
      if (y >= dxabs) {
        y -= dxabs;
        py += sdy;
        if (old_sdy != sdy) {
          old_sdy = sdy;
          scan_index--;
        }
        coord[scan_index].x = px+sdx;
        coord[scan_index].y = py;
        scan_index++;
      }
      px += sdx;
      draw_pen(px, py);
    }
      }
    else
      {
    for (k=0; k < dyabs; k++) {
      x += dxabs;
      if (x >= dyabs) {
        x -= dyabs;
        px += sdx;
      }
      py += sdy;
      if (old_sdy != sdy) {
        old_sdy = sdy;
        if (sdy != 0) scan_index--;
      }
      draw_pen(px,py);
      coord[scan_index].x = px;
      coord[scan_index].y = py;
      scan_index++;
    }
      }
  }

  /* after polygon has been drawn now fill it */

  scan_index--;
  if (sy0 + sdy == 0) scan_index--;

  poly_sort(scan_index, coord);
  
  toggle = 0;
  for (i = 0; i < scan_index; i++) {
    if ((coord[i].y == coord[i+1].y) && (toggle == 0))
      {
    for (j = coord[i].x; j <= coord[i+1].x; j++)
      draw_pen(j, coord[i].y);
    toggle = 1;
      }
    else
      toggle = 0;
  }
}
  

  
static void 
paintPoly(int const v) {
  struct Rect bb;
  struct Point pts[100];
  int i, np = (read_word() - 10) >> 2;

  read_rect(&bb);
  for (i=0; i<np; ++i)
    read_point(&pts[i]);

  /* scan convert poly ... */
  if (!fullres)
      scan_poly(np, pts);
}



static void 
PnLocHFrac(int const version) {
    word frac = read_word();

    if (verbose)
        pm_message("PnLocHFrac = %d", frac);
}



static void 
TxMode(int const version) {
    text_mode = read_word();

    if (text_mode >= 8 && text_mode < 15)
        text_mode -= 8;
    if (verbose)
        pm_message("text transfer mode = %s",
            const_name(transfer_name, text_mode));
    
    /* ignore the text mask bit 'cause we don't handle it yet */
    text_trf = transfer(text_mode & ~64);
}



static void 
TxFont(int const version) {
    text_font = read_word();
    if (verbose)
        pm_message("text font %s", const_name(font_name, text_font));
}



static void 
TxFace(int const version) {
    text_face = read_byte();
    if (verbose)
        pm_message("text face %d", text_face);
}



static void 
TxSize(int const version) {
    text_size = read_word();
    if (verbose)
        pm_message("text size %d", text_size);
}



static void
skip_text(void) {
    skip(read_byte());
}



static int 
abs_value(int const x) {
    if (x < 0)
        return -x;
    else
        return x;
}



static struct font* 
get_font(int const font, 
         int const size, 
         int const style) {
    int closeness, bestcloseness;
    struct fontinfo* fi, *best;

    best = 0;
    for (fi = fontlist; fi; fi = fi->next) {
        closeness = abs_value(fi->font - font) * 10000 +
            abs_value(fi->size - size) * 100 +
            abs_value(fi->style - style);
        if (!best || closeness < bestcloseness) {
            best = fi;
            bestcloseness = closeness;
        }
    }

    if (best) {
        if (best->loaded)
            return best->loaded;

        if ((best->loaded = pbm_loadbdffont(best->filename)))
            return best->loaded;
    }

    /* It would be better to go looking for the nth best font, really */
    return 0;
}



/* This only does 0, 90, 180 and 270 degree rotations */

static void 
rotate(int * const x, 
       int * const y) {
    int tmp;

    if (ps_rotation >= 315 || ps_rotation <= 45)
        return;

    *x -= ps_cent_x;
    *y -= ps_cent_y;

    if (ps_rotation > 45 && ps_rotation < 135) {
        tmp = *x;
        *x = *y;
        *y = tmp;
    }
    else if (ps_rotation >= 135 && ps_rotation < 225) {
        *x = -*x;
    }
    else if (ps_rotation >= 225 && ps_rotation < 315) {
        tmp = *x;
        *x = *y;
        *y = -tmp;
    }
    *x += ps_cent_x;
    *y += ps_cent_y;
}



static void
do_ps_text(word const tx, 
           word const ty) {
    int len, width, i, w, h, x, y, rx, ry, o;
    byte str[256], ch;
    struct glyph* glyph;

    current.x = tx;
    current.y = ty;

    if (!ps_cent_set) {
        ps_cent_x += tx;
        ps_cent_y += ty;
        ps_cent_set = 1;
    }

    len = read_byte();

    /* XXX this width calculation is not completely correct */
    width = 0;
    for (i = 0; i < len; i++) {
        ch = str[i] = read_byte();
        if (tfont->glyph[ch])
            width += tfont->glyph[ch]->xadd;
    }

    if (verbose) {
        str[len] = '\0';
        pm_message("ps text: %s", str);
    }

    /* XXX The width is calculated in order to do different justifications.
     * However, I need the width of original text to finish the job.
     * In other words, font metrics for Quickdraw fonts
     */

    x = tx;

    for (i = 0; i < len; i++) {
        if (!(glyph = tfont->glyph[str[i]]))
            continue;
        
        y = ty - glyph->height - glyph->y;
        for (h = 0; h < glyph->height; h++) {
            for (w = 0; w < glyph->width; w++) {
                rx = x + glyph->x + w;
                ry = y;
                rotate(&rx, &ry);
                if ((rx >= picFrame.left) && (rx < picFrame.right) &&
                    (ry >= picFrame.top) && (ry < picFrame.bottom))
                {
                    o = PIXEL_INDEX(rx, ry);
                    if (glyph->bmap[h * glyph->width + w]) {
                        red[o] = foreground.red;
                        green[o] = foreground.grn;
                        blue[o] = foreground.blu;
                    }
                }
            }
            y++;
        }
        x += glyph->xadd;
    }
}



static void
do_text(word const startx, 
        word const starty) {
    if (fullres)
        skip_text();
    else {
        if (!(tfont = get_font(text_font, text_size, text_face)))
            tfont = pbm_defaultfont("bdf");

        if (ps_text)
            do_ps_text(startx, starty);
        else {
            int len;
            word x, y;

            x = startx;
            y = starty;
            for (len = read_byte(); len > 0; --len) {
                struct glyph* const glyph = tfont->glyph[read_byte()];
                if (glyph) {
                    int dy;
                    int h;
                    for (h = 0, dy = y - glyph->height - glyph->y;
                         h < glyph->height; 
                         ++h, ++dy) {
                        int w;
                        for (w = 0; w < glyph->width; ++w) {
                            struct RGBColor * const colorP = 
                                glyph->bmap[h * glyph->width + w] ?
                                &black : &white;
                            draw_pixel(x + w + glyph->x, dy, colorP, text_trf);
                        }
                    }
                    x += glyph->xadd;
                }
            }
            current.x = x;
            current.y = y;
        }
    }
}



static void
LongText(int const version) {
    struct Point p;

    read_point(&p);
    do_text(p.x, p.y);
}



static void
DHText(int const version) {
    current.x += read_byte();
    do_text(current.x, current.y);
}



static void
DVText(int const version) {
    current.y += read_byte();
    do_text(current.x, current.y);
}



static void
DHDVText(int const version) {
    byte dh, dv;

    dh = read_byte();
    dv = read_byte();

    if (verbose)
        pm_message("dh, dv = %d, %d", dh, dv);

    current.x += dh;
    current.y += dv;
    do_text(current.x, current.y);
}



/*
 * This could use read_pixmap, but I'm too lazy to hack read_pixmap.
 */

static void
directBits(unsigned int const pictVersion, 
           bool         const skipRegion) {

    struct pixMap   p;
    struct Rect     srcRect;
    struct Rect     dstRect;
    struct raster   raster;
    word            mode;
    unsigned int    rectWidth;

    /* skip fake len, and fake EOF */
    skip(4);    /* Ptr baseAddr == 0x000000ff */
    read_word();    /* version */
    read_rect(&p.Bounds);
    rectWidth = p.Bounds.right - p.Bounds.left;
    p.packType = read_word();
    p.packSize = read_long();
    p.hRes = read_long();
    p.vRes = read_long();
    p.pixelType = read_word();
    p.pixelSize = read_word();
    p.pixelSize = read_word();    /* XXX twice??? */
    p.cmpCount = read_word();
    p.cmpSize = read_word();
    p.planeBytes = read_long();
    p.pmTable = read_long();
    p.pmReserved = read_long();

    read_rect(&srcRect);
    if (verbose)
        dumpRect("source rectangle:", srcRect);

    read_rect(&dstRect);
    if (verbose)
        dumpRect("destination rectangle:", dstRect);

    mode = read_word();
    if (verbose)
        pm_message("transfer mode = %s", const_name(transfer_name, mode));

    if (skipRegion) 
        skip_poly_or_region(pictVersion);

    unpackbits(ifp, &p.Bounds, 0, p.pixelSize, &raster);

    blit(srcRect, p.Bounds, raster, p.pixelSize,
         dstRect, picFrame, rowlen, NULL, mode);

    freeRaster(raster);
}



#define SKIP_REGION_TRUE TRUE
#define SKIP_REGION_FALSE FALSE

static void
DirectBitsRect(int const version) {

    directBits(version, SKIP_REGION_FALSE);
}



static void
DirectBitsRgn(int const version) {

    directBits(version, SKIP_REGION_TRUE);
}



static void
do_pixmap(int  const version, 
          word const rowBytes, 
          int  const is_region) {
/*----------------------------------------------------------------------------
   Do a paletted image.
-----------------------------------------------------------------------------*/
    word mode;
    struct pixMap p;
    struct raster raster;
    struct RGBColor * color_table;
    struct Rect srcRect;
    struct Rect dstRect;

    read_pixmap(&p);

    if (verbose)
        pm_message("%u x %u paletted image",
                   p.Bounds.right - p.Bounds.left,
                   p.Bounds.bottom - p.Bounds.top);

    color_table = read_color_table();

    read_rect(&srcRect);

    if (verbose)
        dumpRect("source rectangle:", srcRect);

    read_rect(&dstRect);

    if (verbose)
        dumpRect("destination rectangle:", dstRect);

    mode = read_word();

    if (verbose)
        pm_message("transfer mode = %s", const_name(transfer_name, mode));

    if (is_region)
        skip_poly_or_region(version);

    stage = "unpacking rectangle";

    unpackbits(ifp, &p.Bounds, rowBytes, p.pixelSize, &raster);

    blit(srcRect, p.Bounds, raster, 8,
         dstRect, picFrame, rowlen, color_table, mode);

    free(color_table);
    freeRaster(raster);
}



static void
do_bitmap(int const version, 
          int const rowBytes, 
          int const is_region) {
/*----------------------------------------------------------------------------
   Do a bitmap.  That's one bit per pixel, 0 is white, 1 is black.
-----------------------------------------------------------------------------*/
    struct Rect Bounds;
    struct Rect srcRect;
    struct Rect dstRect;
    word mode;
    struct raster raster;
    static struct RGBColor color_table[] = { 
        {65535L, 65535L, 65535L}, {0, 0, 0} };

    read_rect(&Bounds);
    read_rect(&srcRect);
    read_rect(&dstRect);
    mode = read_word();
    if (verbose)
        pm_message("transfer mode = %s", const_name(transfer_name, mode));

    if (is_region)
        skip_poly_or_region(version);

    stage = "unpacking rectangle";

    unpackbits(ifp, &Bounds, rowBytes, 1, &raster);

    blit(srcRect, Bounds, raster, 8,
         dstRect, picFrame, rowlen, color_table, mode);

    freeRaster(raster);
}



static void
BitsRect(int const version) {

    word rowBytesWord;
    bool pixMap;
    unsigned int rowBytes;

    stage = "Reading rowBytes word for bitsrect";
    rowBytesWord = read_word();

    interpretRowBytesWord(rowBytesWord, &pixMap, &rowBytes);

    if (pixMap)
        do_pixmap(version, rowBytes, 0);
    else
        do_bitmap(version, rowBytes, 0);
}



static void
BitsRegion(int const version) {

    word rowBytesWord;
    bool pixMap;
    unsigned int rowBytes;

    stage = "Reading rowBytes for bitsregion";
    rowBytesWord = read_word();

    interpretRowBytesWord(rowBytesWord, &pixMap, &rowBytes);

    if (pixMap)
        do_pixmap(version, rowBytes, 1);
    else
        do_bitmap(version, rowBytes, 1);
}



 /*
  * See http://developer.apple.com/techpubs/mac/QuickDraw/QuickDraw-461.html
  * for opcode description
  */
static struct opdef const optable[] = {
/* 0x00 */  { "NOP", 0, NULL, "nop" },
/* 0x01 */  { "Clip", NA, Clip, "clip" },
/* 0x02 */  { "BkPat", 8, BkPat, "background pattern" },
/* 0x03 */  { "TxFont", 2, TxFont, "text font (word)" },
/* 0x04 */  { "TxFace", 1, TxFace, "text face (byte)" },
/* 0x05 */  { "TxMode", 2, TxMode, "text mode (word)" },
/* 0x06 */  { "SpExtra", 4, NULL, "space extra (fixed point)" },
/* 0x07 */  { "PnSize", 4, PnSize, "pen size (point)" },
/* 0x08 */  { "PnMode", 2, PnMode, "pen mode (word)" },
/* 0x09 */  { "PnPat", 8, PnPat, "pen pattern" },
/* 0x0a */  { "FillPat", 8, FillPat, "fill pattern" },
/* 0x0b */  { "OvSize", 4, NULL, "oval size (point)" },
/* 0x0c */  { "Origin", 4, NULL, "dh, dv (word)" },
/* 0x0d */  { "TxSize", 2, TxSize, "text size (word)" },
/* 0x0e */  { "FgColor", 4, NULL, "foreground color (longword)" },
/* 0x0f */  { "BkColor", 4, NULL, "background color (longword)" },
/* 0x10 */  { "TxRatio", 8, NULL, "numerator (point), denominator (point)" },
/* 0x11 */  { "Version", 1, NULL, "version (byte)" },
/* 0x12 */  { "BkPixPat", NA, BkPixPat, "color background pattern" },
/* 0x13 */  { "PnPixPat", NA, PnPixPat, "color pen pattern" },
/* 0x14 */  { "FillPixPat", NA, FillPixPat, "color fill pattern" },
/* 0x15 */  { "PnLocHFrac", 2, PnLocHFrac, "fractional pen position" },
/* 0x16 */  { "ChExtra", 2, NULL, "extra for each character" },
/* 0x17 */  res(0),
/* 0x18 */  res(0),
/* 0x19 */  res(0),
/* 0x1a */  { "RGBFgCol", RGB_LEN, RGBFgCol, "RGB foreColor" },
/* 0x1b */  { "RGBBkCol", RGB_LEN, RGBBkCol, "RGB backColor" },
/* 0x1c */  { "HiliteMode", 0, NULL, "hilite mode flag" },
/* 0x1d */  { "HiliteColor", RGB_LEN, NULL, "RGB hilite color" },
/* 0x1e */  { "DefHilite", 0, NULL, "Use default hilite color" },
/* 0x1f */  { "OpColor", NA, OpColor, "RGB OpColor for arithmetic modes" },
/* 0x20 */  { "Line", 8, Line, "pnLoc (point), newPt (point)" },
/* 0x21 */  { "LineFrom", 4, LineFrom, "newPt (point)" },
/* 0x22 */  { "ShortLine", 6, ShortLine, 
              "pnLoc (point, dh, dv (-128 .. 127))" },
/* 0x23 */  { "ShortLineFrom", 2, ShortLineFrom, "dh, dv (-128 .. 127)" },
/* 0x24 */  res(WORD_LEN),
/* 0x25 */  res(WORD_LEN),
/* 0x26 */  res(WORD_LEN),
/* 0x27 */  res(WORD_LEN),
/* 0x28 */  { "LongText", NA, LongText, 
              "txLoc (point), count (0..255), text" },
/* 0x29 */  { "DHText", NA, DHText, "dh (0..255), count (0..255), text" },
/* 0x2a */  { "DVText", NA, DVText, "dv (0..255), count (0..255), text" },
/* 0x2b */  { "DHDVText", NA, DHDVText, 
              "dh, dv (0..255), count (0..255), text" },
/* 0x2c */  res(WORD_LEN),
/* 0x2d */  res(WORD_LEN),
/* 0x2e */  res(WORD_LEN),
/* 0x2f */  res(WORD_LEN),
/* 0x30 */  { "frameRect", 8, frameRect, "rect" },
/* 0x31 */  { "paintRect", 8, paintRect, "rect" },
/* 0x32 */  { "eraseRect", 8, NULL, "rect" },
/* 0x33 */  { "invertRect", 8, NULL, "rect" },
/* 0x34 */  { "fillRect", 8, NULL, "rect" },
/* 0x35 */  res(8),
/* 0x36 */  res(8),
/* 0x37 */  res(8),
/* 0x38 */  { "frameSameRect", 0, frameSameRect, "rect" },
/* 0x39 */  { "paintSameRect", 0, paintSameRect, "rect" },
/* 0x3a */  { "eraseSameRect", 0, NULL, "rect" },
/* 0x3b */  { "invertSameRect", 0, NULL, "rect" },
/* 0x3c */  { "fillSameRect", 0, NULL, "rect" },
/* 0x3d */  res(0),
/* 0x3e */  res(0),
/* 0x3f */  res(0),
/* 0x40 */  { "frameRRect", 8, NULL, "rect" },
/* 0x41 */  { "paintRRect", 8, NULL, "rect" },
/* 0x42 */  { "eraseRRect", 8, NULL, "rect" },
/* 0x43 */  { "invertRRect", 8, NULL, "rect" },
/* 0x44 */  { "fillRRrect", 8, NULL, "rect" },
/* 0x45 */  res(8),
/* 0x46 */  res(8),
/* 0x47 */  res(8),
/* 0x48 */  { "frameSameRRect", 0, NULL, "rect" },
/* 0x49 */  { "paintSameRRect", 0, NULL, "rect" },
/* 0x4a */  { "eraseSameRRect", 0, NULL, "rect" },
/* 0x4b */  { "invertSameRRect", 0, NULL, "rect" },
/* 0x4c */  { "fillSameRRect", 0, NULL, "rect" },
/* 0x4d */  res(0),
/* 0x4e */  res(0),
/* 0x4f */  res(0),
/* 0x50 */  { "frameOval", 8, NULL, "rect" },
/* 0x51 */  { "paintOval", 8, NULL, "rect" },
/* 0x52 */  { "eraseOval", 8, NULL, "rect" },
/* 0x53 */  { "invertOval", 8, NULL, "rect" },
/* 0x54 */  { "fillOval", 8, NULL, "rect" },
/* 0x55 */  res(8),
/* 0x56 */  res(8),
/* 0x57 */  res(8),
/* 0x58 */  { "frameSameOval", 0, NULL, "rect" },
/* 0x59 */  { "paintSameOval", 0, NULL, "rect" },
/* 0x5a */  { "eraseSameOval", 0, NULL, "rect" },
/* 0x5b */  { "invertSameOval", 0, NULL, "rect" },
/* 0x5c */  { "fillSameOval", 0, NULL, "rect" },
/* 0x5d */  res(0),
/* 0x5e */  res(0),
/* 0x5f */  res(0),
/* 0x60 */  { "frameArc", 12, NULL, "rect, startAngle, arcAngle" },
/* 0x61 */  { "paintArc", 12, NULL, "rect, startAngle, arcAngle" },
/* 0x62 */  { "eraseArc", 12, NULL, "rect, startAngle, arcAngle" },
/* 0x63 */  { "invertArc", 12, NULL, "rect, startAngle, arcAngle" },
/* 0x64 */  { "fillArc", 12, NULL, "rect, startAngle, arcAngle" },
/* 0x65 */  res(12),
/* 0x66 */  res(12),
/* 0x67 */  res(12),
/* 0x68 */  { "frameSameArc", 4, NULL, "rect, startAngle, arcAngle" },
/* 0x69 */  { "paintSameArc", 4, NULL, "rect, startAngle, arcAngle" },
/* 0x6a */  { "eraseSameArc", 4, NULL, "rect, startAngle, arcAngle" },
/* 0x6b */  { "invertSameArc", 4, NULL, "rect, startAngle, arcAngle" },
/* 0x6c */  { "fillSameArc", 4, NULL, "rect, startAngle, arcAngle" },
/* 0x6d */  res(4),
/* 0x6e */  res(4),
/* 0x6f */  res(4),
/* 0x70 */  { "framePoly", NA, skip_poly_or_region, "poly" },
/* 0x71 */  { "paintPoly", NA, paintPoly, "poly" },
/* 0x72 */  { "erasePoly", NA, skip_poly_or_region, "poly" },
/* 0x73 */  { "invertPoly", NA, skip_poly_or_region, "poly" },
/* 0x74 */  { "fillPoly", NA, skip_poly_or_region, "poly" },
/* 0x75 */  resf(skip_poly_or_region),
/* 0x76 */  resf(skip_poly_or_region),
/* 0x77 */  resf(skip_poly_or_region),
/* 0x78 */  { "frameSamePoly", 0, NULL, "poly (NYI)" },
/* 0x79 */  { "paintSamePoly", 0, NULL, "poly (NYI)" },
/* 0x7a */  { "eraseSamePoly", 0, NULL, "poly (NYI)" },
/* 0x7b */  { "invertSamePoly", 0, NULL, "poly (NYI)" },
/* 0x7c */  { "fillSamePoly", 0, NULL, "poly (NYI)" },
/* 0x7d */  res(0),
/* 0x7e */  res(0),
/* 0x7f */  res(0),
/* 0x80 */  { "frameRgn", NA, skip_poly_or_region, "region" },
/* 0x81 */  { "paintRgn", NA, skip_poly_or_region, "region" },
/* 0x82 */  { "eraseRgn", NA, skip_poly_or_region, "region" },
/* 0x83 */  { "invertRgn", NA, skip_poly_or_region, "region" },
/* 0x84 */  { "fillRgn", NA, skip_poly_or_region, "region" },
/* 0x85 */  resf(skip_poly_or_region),
/* 0x86 */  resf(skip_poly_or_region),
/* 0x87 */  resf(skip_poly_or_region),
/* 0x88 */  { "frameSameRgn", 0, NULL, "region (NYI)" },
/* 0x89 */  { "paintSameRgn", 0, NULL, "region (NYI)" },
/* 0x8a */  { "eraseSameRgn", 0, NULL, "region (NYI)" },
/* 0x8b */  { "invertSameRgn", 0, NULL, "region (NYI)" },
/* 0x8c */  { "fillSameRgn", 0, NULL, "region (NYI)" },
/* 0x8d */  res(0),
/* 0x8e */  res(0),
/* 0x8f */  res(0),
/* 0x90 */  { "BitsRect", NA, BitsRect, "copybits, rect clipped" },
/* 0x91 */  { "BitsRgn", NA, BitsRegion, "copybits, rgn clipped" },
/* 0x92 */  res(WORD_LEN),
/* 0x93 */  res(WORD_LEN),
/* 0x94 */  res(WORD_LEN),
/* 0x95 */  res(WORD_LEN),
/* 0x96 */  res(WORD_LEN),
/* 0x97 */  res(WORD_LEN),
/* 0x98 */  { "PackBitsRect", NA, BitsRect, "packed copybits, rect clipped" },
/* 0x99 */  { "PackBitsRgn", NA, BitsRegion, "packed copybits, rgn clipped" },
/* 0x9a */  { "DirectBitsRect", NA, DirectBitsRect, 
              "PixMap, srcRect, dstRect, int copymode, PixData" },
/* 0x9b */  { "DirectBitsRgn", NA, DirectBitsRgn, 
              "PixMap, srcRect, dstRect, int copymode, maskRgn, PixData" },
/* 0x9c */  res(WORD_LEN),
/* 0x9d */  res(WORD_LEN),
/* 0x9e */  res(WORD_LEN),
/* 0x9f */  res(WORD_LEN),
/* 0xa0 */  { "ShortComment", 2, ShortComment, "kind (word)" },
/* 0xa1 */  { "LongComment", NA, LongComment, 
              "kind (word), size (word), data" }
};



static void
interpret_pict(void) {
    byte ch;
    word picSize;
    word opcode;
    word len;
    unsigned int version;
    int i;
    struct rgbPlanes planes;

    for (i = 0; i < 64; i++)
        pen_pat.pix[i] = bkpat.pix[i] = fillpat.pix[i] = 1;
    pen_width = pen_height = 1;
    pen_mode = 0; /* srcCopy */
    pen_trf = transfer(pen_mode);
    text_mode = 0; /* srcCopy */
    text_trf = transfer(text_mode);

    stage = "Reading picture size";
    picSize = read_word();

    if (verbose)
        pm_message("picture size = %d (0x%x)", picSize, picSize);

    stage = "reading picture frame";
    read_rect(&picFrame);

    if (verbose) {
        dumpRect("Picture frame:", picFrame);
        pm_message("Picture size is %d x %d",
            picFrame.right - picFrame.left,
            picFrame.bottom - picFrame.top);
    }

    if (!fullres)
        allocPlanes(&planes);

    while ((ch = read_byte()) == 0)
        ;
    if (ch != 0x11)
        pm_error("No version number");

    version = read_byte();

    switch (version) {
    case 1:
        break;
    case 2: {
        unsigned char const subcode = read_byte();
        if (subcode != 0xff)
            pm_error("The only Version 2 PICT images this program "
                     "undertands are subcode 0xff.  This image has "
                     "subcode 0x%02x", subcode);
    } break;
    default:
        pm_error("Unrecognized PICT version %u", version);
    }

    if (verbose)
        pm_message("PICT version %u", version);

    while((opcode = get_op(version)) != 0xff) {
        if (opcode < 0xa2) {
            stage = optable[opcode].name;
            if (verbose) {
                if (STREQ(stage, "reserved"))
                    pm_message("reserved opcode=0x%x", opcode);
                else
                    pm_message("Opcode: %s", optable[opcode].name);
            }

            if (optable[opcode].impl != NULL)
                (*optable[opcode].impl)(version);
            else if (optable[opcode].len >= 0)
                skip(optable[opcode].len);
            else switch (optable[opcode].len) {
            case WORD_LEN:
                len = read_word();
                skip(len);
                break;
            default:
                pm_error("can't do length %u", optable[opcode].len);
            }
        } else if (opcode == 0xc00) {
            if (verbose)
                pm_message("HeaderOp");
            stage = "HeaderOp";
            skip(24);
        } else if (opcode >= 0xa2 && opcode <= 0xaf) {
            stage = "skipping reserved";
            if (verbose)
                pm_message("%s 0x%x", stage, opcode);
            skip(read_word());
        } else if (opcode >= 0xb0 && opcode <= 0xcf) {
            /* just a reserved opcode, no data */
            if (verbose)
                pm_message("reserved 0x%x", opcode);
        } else if (opcode >= 0xd0 && opcode <= 0xfe) {
            stage = "skipping reserved";
            if (verbose)
                pm_message("%s 0x%x", stage, opcode);
            skip(read_long());
        } else if (opcode >= 0x100 && opcode <= 0x7fff) {
            stage = "skipping reserved";
            if (verbose)
                pm_message("%s 0x%x", stage, opcode);
            skip((opcode >> 7) & 255);
        } else if (opcode >= 0x8000 && opcode <= 0x80ff) {
            /* just a reserved opcode */
            if (verbose)
                pm_message("reserved 0x%x", opcode);
        } else if (opcode >= 0x8100) {
            stage = "skipping reserved";
            if (verbose)
                pm_message("%s 0x%x", stage, opcode);
            skip(read_long());
        } else
            pm_error("This program does not understand opcode 0x%04x", opcode);
    }
    
    if (fullres)
        do_blits(&planes);

    outputPpm(planes);

    freePlanes(planes);
}



int
main(int argc, char * argv[]) {
    int argn;
    int header;
    const char* const usage =
"[-verbose] [-fullres] [-noheader] [-quickdraw] [-fontdir file] [pictfile]";

    ppm_init( &argc, argv );

    argn = 1;
    verbose = 0;
    fullres = 0;
    header = 1;
    recognize_comment = 1;

    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
        if (pm_keymatch(argv[argn], "-verbose", 2))
            verbose++;
        else if (pm_keymatch(argv[argn], "-fullres", 3))
            fullres = 1;
        else if (pm_keymatch(argv[argn], "-noheader", 2))
            header = 0;
        else if (pm_keymatch(argv[argn], "-quickdraw", 2))
            recognize_comment = 0;
        else if (pm_keymatch(argv[argn], "-fontdir", 3)) {
            argn++;
            if (!argv[argn])
                pm_usage(usage);
            else
                load_fontdir(argv[argn]);
        }
        else
            pm_usage(usage);
        ++argn;
    }

    load_fontdir("fontdir");

    if (argn < argc) {
        ifp = pm_openr(argv[argn]);
        ++argn;
    } else
        ifp = stdin;

    if (argn != argc)
        pm_usage(usage);

    if (header) {
        stage = "Reading 512 byte header";
        skip(512);
    }

    interpret_pict();

    return 0;
}
