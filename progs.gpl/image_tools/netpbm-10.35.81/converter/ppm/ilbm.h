#ifndef ILBM_H_INCLUDED
#define ILBM_H_INCLUDED

/* ilbm.h - definitions for IFF ILBM files */

#define RowBytes(cols)          ((((cols) + 15) / 16) * 2)


/* definitions for BMHD */

typedef struct {
    unsigned short w, h;
    short x, y;
    unsigned char nPlanes, masking, compression, flags;
    unsigned short transparentColor;
    unsigned char xAspect, yAspect;
    short pageWidth, pageHeight;
} BitMapHeader;
#define BitMapHeaderSize    20

#define BMHD_FLAGS_CMAPOK       (1<<7)      /* valid 8bit colormap */

#define mskNone                 0
#define mskHasMask              1
#define mskHasTransparentColor  2
#define mskLasso                3       /* not supported */
#define mskMAXKNOWN             mskLasso
static const char * mskNAME[] = { 
    "none", "mask plane", "transparent color", "lasso" 
};

#define cmpNone                 0
#define cmpByteRun1             1
#define cmpMAXKNOWN             cmpByteRun1
static const char * cmpNAME[] = { "none", "byterun1" };


/* definitions for CMAP */

#if 0   /* not used */
typedef struct {
    unsigned char r, g, b;
} ColorRegister;
#endif


/* definitions for CAMG */

#define CAMGChunkSize       4

#define vmLACE              0x0004
#define vmEXTRA_HALFBRITE   0x0080
#define vmHAM               0x0800
#define vmHIRES             0x8000

#define HAMCODE_CMAP      0     /* look up color in colormap */
#define HAMCODE_BLUE      1     /* new blue component */
#define HAMCODE_RED       2     /* new red component */
#define HAMCODE_GREEN     3     /* new green component */


/* multipalette PCHG chunk definitions */

/* get number of longwords in line mask from PCHG.LineCount */
#define MaskLongWords(x)    (((x) + 31) / 32)

typedef struct {
    unsigned short  Compression;
    unsigned short  Flags;
    short           StartLine;      /* may be negative */
    unsigned short  LineCount;
    unsigned short  ChangedLines;
    unsigned short  MinReg;
    unsigned short  MaxReg;
    unsigned short  MaxChanges;
    unsigned long   TotalChanges;
} PCHGHeader;
#define PCHGHeaderSize      20

/* Compression modes */
#define PCHG_COMP_NONE      0
#define PCHG_COMP_HUFFMAN   1

/* Flags */
#define PCHGF_12BIT         (1 << 0)    /* use SmallLineChanges */
#define PCHGF_32BIT         (1 << 1)    /* use BigLineChanges */
#define PCHGF_USE_ALPHA     (1 << 2)    /* meaningful only if PCHG_32BIT is on:
                                           use the Alpha channel info */
typedef struct {
    unsigned long   CompInfoSize;
    unsigned long   OriginalDataSize;
} PCHGCompHeader;
#define PCHGCompHeaderSize  8

#if 0   /* not used */
typedef struct {
    unsigned char   ChangeCount16;
    unsigned char   ChangeCount32;
    unsigned short  *PaletteChange;
} SmallLineChanges;

typedef struct {
    unsigned short  Register;
    unsigned char   Alpha, Red, Blue, Green;    /* ARBG, not ARGB */
} BigPaletteChange;

typedef struct {
    unsigned short      ChangeCount;
    BigPaletteChange    *PaletteChange;
} BigLineChanges;
#endif /* 0 */


/* definitions for CLUT */

#if 0 /* not used */
typedef struct {
    unsigned long type;
    unsigned long reserved0;
    unsigned char lut[256];
} ColorLUT;
#endif /* 0 */
#define CLUTSize    (256+4+4)

/* types */
#define CLUT_MONO   0
#define CLUT_RED    1
#define CLUT_GREEN  2
#define CLUT_BLUE   3
#define CLUT_HUE    4   /* not supported */
#define CLUT_SAT    5   /* not supported */


/* unofficial DCOL chunk for direct-color */

typedef struct {
    unsigned char r, g, b, pad1;
} DirectColor;
#define DirectColorSize     4



/* IFF chunk IDs */

typedef unsigned long   IFF_ID;

#define MAKE_ID(a, b, c, d) \
    ((IFF_ID)(a)<<24 | (IFF_ID)(b)<<16 | (IFF_ID)(c)<<8 | (IFF_ID)(d))

#define ID_FORM     MAKE_ID('F', 'O', 'R', 'M')     
    /* EA IFF 85 group identifier */
#define ID_CAT      MAKE_ID('C', 'A', 'T', ' ')     
    /* EA IFF 85 group identifier */
#define ID_LIST     MAKE_ID('L', 'I', 'S', 'T')     
    /* EA IFF 85 group identifier */
#define ID_PROP     MAKE_ID('P', 'R', 'O', 'P')     
    /* EA IFF 85 group identifier */
#define ID_END      MAKE_ID('E', 'N', 'D', ' ')     
    /* unofficial END-of-FORM identifier (see Amiga RKM Devices Ed.3
       page 376) */
#define ID_ILBM     MAKE_ID('I', 'L', 'B', 'M')     
    /* EA IFF 85 raster bitmap form */
#define ID_DEEP     MAKE_ID('D', 'E', 'E', 'P')     
    /* Chunky pixel image files (Used in TV Paint) */
#define ID_RGB8     MAKE_ID('R', 'G', 'B', '8')     
    /* RGB image forms, Turbo Silver (Impulse) */
#define ID_RGBN     MAKE_ID('R', 'G', 'B', 'N')     
    /* RGB image forms, Turbo Silver (Impulse) */
#define ID_PBM      MAKE_ID('P', 'B', 'M', ' ')     
    /* 256-color chunky format (DPaint 2 ?) */
#define ID_ACBM     MAKE_ID('A', 'C', 'B', 'M')     
    /* Amiga Contiguous Bitmap (AmigaBasic) */

/* generic */

#define ID_FVER     MAKE_ID('F', 'V', 'E', 'R')     
    /* AmigaOS version string */
#define ID_JUNK     MAKE_ID('J', 'U', 'N', 'K')     
    /* always ignore this chunk */
#define ID_ANNO     MAKE_ID('A', 'N', 'N', 'O')     
    /* EA IFF 85 Generic Annotation chunk */
#define ID_AUTH     MAKE_ID('A', 'U', 'T', 'H')     
    /* EA IFF 85 Generic Author chunk */
#define ID_CHRS     MAKE_ID('C', 'H', 'R', 'S')     
    /* EA IFF 85 Generic character string chunk */
#define ID_NAME     MAKE_ID('N', 'A', 'M', 'E')     
    /* EA IFF 85 Generic Name of art, music, etc. chunk */
#define ID_TEXT     MAKE_ID('T', 'E', 'X', 'T')     
    /* EA IFF 85 Generic unformatted ASCII text chunk */
#define ID_copy     MAKE_ID('(', 'c', ')', ' ')     
/* EA IFF 85 Generic Copyright text chunk */

/* ILBM chunks */

#define ID_BMHD     MAKE_ID('B', 'M', 'H', 'D')     
    /* ILBM BitmapHeader */
#define ID_CMAP     MAKE_ID('C', 'M', 'A', 'P')     
    /* ILBM 8bit RGB colormap */
#define ID_GRAB     MAKE_ID('G', 'R', 'A', 'B')     
    /* ILBM "hotspot" coordiantes */
#define ID_DEST     MAKE_ID('D', 'E', 'S', 'T')     
    /* ILBM destination image info */
#define ID_SPRT     MAKE_ID('S', 'P', 'R', 'T')     
    /* ILBM sprite identifier */
#define ID_CAMG     MAKE_ID('C', 'A', 'M', 'G')     
    /* Amiga viewportmodes */
#define ID_BODY     MAKE_ID('B', 'O', 'D', 'Y')     
    /* ILBM image data */
#define ID_CRNG     MAKE_ID('C', 'R', 'N', 'G')     
    /* color cycling */
#define ID_CCRT     MAKE_ID('C', 'C', 'R', 'T')     
    /* color cycling */
#define ID_CLUT     MAKE_ID('C', 'L', 'U', 'T')     
    /* Color Lookup Table chunk */
#define ID_DPI      MAKE_ID('D', 'P', 'I', ' ')     
    /* Dots per inch chunk */
#define ID_DPPV     MAKE_ID('D', 'P', 'P', 'V')     
    /* DPaint perspective chunk (EA) */
#define ID_DRNG     MAKE_ID('D', 'R', 'N', 'G')     
    /* DPaint IV enhanced color cycle chunk (EA) */
#define ID_EPSF     MAKE_ID('E', 'P', 'S', 'F')     
    /* Encapsulated Postscript chunk */
#define ID_CMYK     MAKE_ID('C', 'M', 'Y', 'K')     
    /* Cyan, Magenta, Yellow, & Black color map (Soft-Logik) */
#define ID_CNAM     MAKE_ID('C', 'N', 'A', 'M')     
    /* Color naming chunk (Soft-Logik) */
#define ID_PCHG     MAKE_ID('P', 'C', 'H', 'G')     
    /* Line by line palette control information (Sebastiano Vigna) */
#define ID_PRVW     MAKE_ID('P', 'R', 'V', 'W')     
    /* A mini duplicate ILBM used for preview (Gary Bonham) */
#define ID_XBMI     MAKE_ID('X', 'B', 'M', 'I')     
    /* eXtended BitMap Information (Soft-Logik) */
#define ID_CTBL     MAKE_ID('C', 'T', 'B', 'L')     
    /* Newtek Dynamic Ham color chunk */
#define ID_DYCP     MAKE_ID('D', 'Y', 'C', 'P')     
    /* Newtek Dynamic Ham chunk */
#define ID_SHAM     MAKE_ID('S', 'H', 'A', 'M')     
    /* Sliced HAM color chunk */
#define ID_ABIT     MAKE_ID('A', 'B', 'I', 'T')     
    /* ACBM body chunk */
#define ID_DCOL     MAKE_ID('D', 'C', 'O', 'L')     
    /* unofficial direct color */
#define ID_DPPS     MAKE_ID('D', 'P', 'P', 'S')
    /* ? */
#define ID_TINY     MAKE_ID('T', 'I', 'N', 'Y')
    /* ? */

/* other stuff */

#define MAXPLANES       16
typedef unsigned short  rawtype;

#define MAXCMAPCOLORS   (1 << MAXPLANES)
#define MAXCOLVAL       255     /* max value of color component */

#endif
