/* tga.h - header file for Targa files
*/
 
/* Header definition. */
struct ImageHeader {
    unsigned char IdLength;     /* length of Identifier String */
    unsigned char CoMapType;        /* 0 = no map */
    unsigned char ImgType;      /* image type (see below for values) */
    unsigned char Index_lo, Index_hi;   /* index of first color map entry */
    unsigned char Length_lo, Length_hi; /* number of entries in color map */
    unsigned char CoSize;       /* size of color map entry (15,16,24,32) */
    unsigned char X_org_lo, X_org_hi;   /* x origin of image */
    unsigned char Y_org_lo, Y_org_hi;   /* y origin of image */
    unsigned char Width_lo, Width_hi;   /* width of image */
    unsigned char Height_lo, Height_hi; /* height of image */
    unsigned char PixelSize;        /* pixel size (8,16,24,32) */
    unsigned char AttBits;  /* 4 bits, number of attribute bits per pixel */
    unsigned char Rsrvd;    /* 1 bit, reserved */
    unsigned char OrgBit;   /* 1 bit, origin: 0=lower left, 1=upper left */
    unsigned char IntrLve;  /* 2 bits, interleaving flag */
    const char * Id;     /* Identifier String.  Length is IDLength, above */
};

#define IMAGEIDFIELDMAXSIZE 256

typedef char ImageIDField[IMAGEIDFIELDMAXSIZE];

#define TGA_MAXVAL 255

/* Definitions for image types. */
#define TGA_Null 0
#define TGA_Map 1
#define TGA_RGB 2
#define TGA_Mono 3
#define TGA_RLEMap 9
#define TGA_RLERGB 10
#define TGA_RLEMono 11
#define TGA_CompMap 32
#define TGA_CompMap4 33

/* Definitions for interleave flag. */
#define TGA_IL_None 0
#define TGA_IL_Two 1
#define TGA_IL_Four 2

enum TGAbaseImageType {TGA_MAP_TYPE, TGA_MONO_TYPE, TGA_RGB_TYPE};
