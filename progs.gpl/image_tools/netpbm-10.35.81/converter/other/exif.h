#ifndef EXIF_H_INCLUDED
#define EXIF_H_INCLUDED

#define MAX_COMMENT 2000

#ifdef _WIN32
    #define PATH_MAX _MAX_PATH
#endif

/*--------------------------------------------------------------------------
  This structure stores Exif header image elements in a simple manner
  Used to store camera data as extracted from the various ways that it can be
  stored in an exif header
--------------------------------------------------------------------------*/
typedef struct {
    char  CameraMake   [32];
    char  CameraModel  [40];
    char  DateTime     [20];
    int   Height, Width;
    int   Orientation;
    int   IsColor;
    int   FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    int   HaveCCDWidth;  /* boolean */
    float CCDWidth;
    float ExposureBias;
    int   Whitebalance;
    int   MeteringMode;
    int   ExposureProgram;
    int   ISOequivalent;
    int   CompressionLevel;
    char  Comments[MAX_COMMENT];

    unsigned char * ThumbnailPointer;  /* Pointer at the thumbnail */
    unsigned ThumbnailSize;     /* Size of thumbnail. */

    char * DatePointer;
}ImageInfo_t;


/* Prototypes for exif.c functions. */

void 
process_EXIF(unsigned char * const ExifSection, 
             unsigned int    const length,
             ImageInfo_t *   const ImageInfoP, 
             int             const ShowTags,
             const char **   const errorP);

void 
ShowImageInfo(ImageInfo_t * const ImageInfoP);

#endif
