#ifndef PM_TIFF_H_INCLUDED
#define PM_TIFF_H_INCLUDED

typedef struct {
/*----------------------------------------------------------------------------
   This is an association between a tag value name and the integer that
   represents the tag value in the TIFF.

   E.g. for an ORIENTATION tag, the value named "TOPLEFT" is represented
   by the integer 1.
-----------------------------------------------------------------------------*/
    const char *  name;
    unsigned long value;
} tagvalmap;

typedef struct tagDefinition {
/*----------------------------------------------------------------------------
   This is the definition of a type of tag, e.g. ORIENTATION.
-----------------------------------------------------------------------------*/
    const char * name;
        /* The name by which our user knows the tag type, e.g. 
           "ORIENTATION"
        */
    unsigned int tagnum;
        /* The integer by which libtiff knows the tag type, e.g.
           TIFFTAG_ORIENTATION
        */
    void (*      put)(TIFF *, unsigned int, const char *, const tagvalmap *);
    const tagvalmap * choices;
        /* List of the possible values for the tag, if it is one with
           enumerated values.  e.g. for ORIENTATION, it's TOPLEFT,
           TOPRIGHT, etc.
        */
} tagDefinition;



const tagDefinition *
tagDefFind(const char * const name);

#endif
