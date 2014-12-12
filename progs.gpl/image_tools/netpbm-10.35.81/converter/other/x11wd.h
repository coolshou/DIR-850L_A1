/* x11wd.h - the following defs are taken from various X.V11R2 header files
*/

#ifndef X11WD_H_INCLUDED
#define X11WD_H_INCLUDED

enum byteorder {LSBFirst=0, MSBFirst=1};
/* They used to be defined wrongly as macros:  2000.05.08 -Bryan

#define LSBFirst    0
#define MSBFirst    1
*/

#define XYBitmap    0
#define XYPixmap    1
#define ZPixmap     2

enum visualclass {StaticGray=0,GrayScale=1,StaticColor=2,PseudoColor=3,
                  TrueColor=4, DirectColor=5};

/* They used to be defined wrongly as macros:  2000.05.08 -Bryan
#define StaticGray  0
#define GrayScale   1
#define StaticColor 2
#define PseudoColor 3
#define TrueColor   4
#define DirectColor 5
*/

typedef uint32n xwdval;
#define XWDVAL_MAX ((xwdval)(-1))
#define X11WD_FILE_VERSION 7
typedef struct {
    xwdval header_size;     /* Size of the entire file header (bytes). */
    xwdval file_version;    /* X11WD_FILE_VERSION */
    xwdval pixmap_format;   /* Pixmap format */
    xwdval pixmap_depth;    /* Pixmap depth */
    xwdval pixmap_width;    /* Pixmap width */
    xwdval pixmap_height;   /* Pixmap height */
    xwdval xoffset;     /* Bitmap x offset */
    xwdval byte_order;      /* MSBFirst, LSBFirst */
    xwdval bitmap_unit;     /* Bitmap unit */
    xwdval bitmap_bit_order;    /* MSBFirst, LSBFirst */
    xwdval bitmap_pad;      /* Bitmap scanline pad */
    xwdval bits_per_pixel;  /* Bits per pixel */
    xwdval bytes_per_line;  /* Bytes per scanline */
    xwdval visual_class;    /* Class of colormap */
    xwdval red_mask;        /* Z red mask */
    xwdval green_mask;      /* Z green mask */
    xwdval blue_mask;       /* Z blue mask */
    xwdval bits_per_rgb;    /* Log base 2 of distinct color values */
    xwdval colormap_entries;
        /* I have no idea what this is.  An old comment says "number of
           entries in colormap," but readers seem to use 'ncolors' for that
           instead.  That's how Pnmtoxwd sets ncolors, and is how Xwdtopnm
           interprets it.  Xwdtopnm doesn't even look at 'colormap_entries'.

           This could be an old mistake; maybe colormap_entries was 
           originally the number of entries in the colormap, and ncolors
           was the number of distinct colors in the image (which might be
           less than colormap_entries or, for direct color, could be much
           larger).
        */
    xwdval ncolors;
        /* Number of entries in the color map (for direct color, it's the
           number of entries in each of them).  See 'colormap_entries'. 
        */
    xwdval window_width;    /* Window width */
    xwdval window_height;   /* Window height */
    int32n window_x;        /* Window upper left X coordinate */
    int32n window_y;        /* Window upper left Y coordinate */
    xwdval window_bdrwidth; /* Window border width */
    } X11WDFileHeader;

typedef struct {
    uint32n num;
    unsigned short red, green, blue;
    char flags;         /* do_red, do_green, do_blue */
    char pad;
    } X11XColor;

#endif
