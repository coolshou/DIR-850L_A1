/* See LICENSE file for licensing information.
*/

#include "pm_c_util.h"
#include "mallocvar.h"
#include "pnm.h"

#include "palm.h"

int
palmcolor_compare_indices(const void * const p1,
                          const void * const p2) {

    if ((*((Color) p1) & 0xFF000000) < (*((Color) p2) & 0xFF000000))
        return -1;
    else if ((*((Color) p1) & 0xFF000000) > (*((Color) p2) & 0xFF000000))
        return 1;
    else
        return 0;
}



int
palmcolor_compare_colors(const void * const p1,
                         const void * const p2) {

    unsigned long const val1 = *((const unsigned long *) p1) & 0xFFFFFF;
    unsigned long const val2 = *((const unsigned long *) p2) & 0xFFFFFF;

    if (val1 < val2)
        return -1;
    else if (val1 > val2)
        return 1;
    else
        return 0;
}

/***********************************************************************
 ***********************************************************************
 ***********************************************************************
 ******* colortables from pilrc-2.6/bitmap.c ***************************
 ***********************************************************************
 ***********************************************************************
 ***********************************************************************/

#if 0

/*
 * The 1bit-2 color system palette for Palm Computing Devices.
 */
static int PalmPalette1bpp[2][3] = 
{
  { 255, 255, 255}, {   0,   0,   0 }
};

/*
 * The 2bit-4 color system palette for Palm Computing Devices.
 */
static int PalmPalette2bpp[4][3] = 
{
  { 255, 255, 255}, { 192, 192, 192}, { 128, 128, 128 }, {   0,   0,   0 }
};

/*
 * The 4bit-16 color system palette for Palm Computing Devices.
 */
static int PalmPalette4bpp[16][3] = 
{
  { 255, 255, 255}, { 238, 238, 238 }, { 221, 221, 221 }, { 204, 204, 204 },
  { 187, 187, 187}, { 170, 170, 170 }, { 153, 153, 153 }, { 136, 136, 136 },
  { 119, 119, 119}, { 102, 102, 102 }, {  85,  85,  85 }, {  68,  68,  68 },
  {  51,  51,  51}, {  34,  34,  34 }, {  17,  17,  17 }, {   0,   0,   0 }
};

/*
 * The 4bit-16 color system palette for Palm Computing Devices.
 */
static int PalmPalette4bppColor[16][3] = 
{
  { 255, 255, 255}, { 128, 128, 128 }, { 128,   0,   0 }, { 128, 128,   0 },
  {   0, 128,   0}, {   0, 128, 128 }, {   0,   0, 128 }, { 128,   0, 128 },
  { 255,   0, 255}, { 192, 192, 192 }, { 255,   0,   0 }, { 255, 255,   0 },
  {   0, 255,   0}, {   0, 255, 255 }, {   0,   0, 255 }, {   0,   0,   0 }
};

#endif  /* 0 */

/*
 * The 8bit-256 color system palette for Palm Computing Devices.
 *
 * NOTE:  only the first 231, plus the last one, are valid.
 */
static int PalmPalette8bpp[256][3] = 
{
  { 255, 255, 255 }, { 255, 204, 255 }, { 255, 153, 255 }, { 255, 102, 255 }, 
  { 255,  51, 255 }, { 255,   0, 255 }, { 255, 255, 204 }, { 255, 204, 204 }, 
  { 255, 153, 204 }, { 255, 102, 204 }, { 255,  51, 204 }, { 255,   0, 204 }, 
  { 255, 255, 153 }, { 255, 204, 153 }, { 255, 153, 153 }, { 255, 102, 153 }, 
  { 255,  51, 153 }, { 255,   0, 153 }, { 204, 255, 255 }, { 204, 204, 255 },
  { 204, 153, 255 }, { 204, 102, 255 }, { 204,  51, 255 }, { 204,   0, 255 },
  { 204, 255, 204 }, { 204, 204, 204 }, { 204, 153, 204 }, { 204, 102, 204 },
  { 204,  51, 204 }, { 204,   0, 204 }, { 204, 255, 153 }, { 204, 204, 153 },
  { 204, 153, 153 }, { 204, 102, 153 }, { 204,  51, 153 }, { 204,   0, 153 },
  { 153, 255, 255 }, { 153, 204, 255 }, { 153, 153, 255 }, { 153, 102, 255 },
  { 153,  51, 255 }, { 153,   0, 255 }, { 153, 255, 204 }, { 153, 204, 204 },
  { 153, 153, 204 }, { 153, 102, 204 }, { 153,  51, 204 }, { 153,   0, 204 },
  { 153, 255, 153 }, { 153, 204, 153 }, { 153, 153, 153 }, { 153, 102, 153 },
  { 153,  51, 153 }, { 153,   0, 153 }, { 102, 255, 255 }, { 102, 204, 255 },
  { 102, 153, 255 }, { 102, 102, 255 }, { 102,  51, 255 }, { 102,   0, 255 },
  { 102, 255, 204 }, { 102, 204, 204 }, { 102, 153, 204 }, { 102, 102, 204 },
  { 102,  51, 204 }, { 102,   0, 204 }, { 102, 255, 153 }, { 102, 204, 153 },
  { 102, 153, 153 }, { 102, 102, 153 }, { 102,  51, 153 }, { 102,   0, 153 },
  {  51, 255, 255 }, {  51, 204, 255 }, {  51, 153, 255 }, {  51, 102, 255 },
  {  51,  51, 255 }, {  51,   0, 255 }, {  51, 255, 204 }, {  51, 204, 204 },
  {  51, 153, 204 }, {  51, 102, 204 }, {  51,  51, 204 }, {  51,   0, 204 },
  {  51, 255, 153 }, {  51, 204, 153 }, {  51, 153, 153 }, {  51, 102, 153 },
  {  51,  51, 153 }, {  51,   0, 153 }, {   0, 255, 255 }, {   0, 204, 255 },
  {   0, 153, 255 }, {   0, 102, 255 }, {   0,  51, 255 }, {   0,   0, 255 },
  {   0, 255, 204 }, {   0, 204, 204 }, {   0, 153, 204 }, {   0, 102, 204 },
  {   0,  51, 204 }, {   0,   0, 204 }, {   0, 255, 153 }, {   0, 204, 153 },
  {   0, 153, 153 }, {   0, 102, 153 }, {   0,  51, 153 }, {   0,   0, 153 },
  { 255, 255, 102 }, { 255, 204, 102 }, { 255, 153, 102 }, { 255, 102, 102 },
  { 255,  51, 102 }, { 255,   0, 102 }, { 255, 255,  51 }, { 255, 204,  51 },
  { 255, 153,  51 }, { 255, 102,  51 }, { 255,  51,  51 }, { 255,   0,  51 },
  { 255, 255,   0 }, { 255, 204,   0 }, { 255, 153,   0 }, { 255, 102,   0 },
  { 255,  51,   0 }, { 255,   0,   0 }, { 204, 255, 102 }, { 204, 204, 102 },
  { 204, 153, 102 }, { 204, 102, 102 }, { 204,  51, 102 }, { 204,   0, 102 },
  { 204, 255,  51 }, { 204, 204,  51 }, { 204, 153,  51 }, { 204, 102,  51 },
  { 204,  51,  51 }, { 204,   0,  51 }, { 204, 255,   0 }, { 204, 204,   0 },
  { 204, 153,   0 }, { 204, 102,   0 }, { 204,  51,   0 }, { 204,   0,   0 },
  { 153, 255, 102 }, { 153, 204, 102 }, { 153, 153, 102 }, { 153, 102, 102 },
  { 153,  51, 102 }, { 153,   0, 102 }, { 153, 255,  51 }, { 153, 204,  51 },
  { 153, 153,  51 }, { 153, 102,  51 }, { 153,  51,  51 }, { 153,   0,  51 },
  { 153, 255,   0 }, { 153, 204,   0 }, { 153, 153,   0 }, { 153, 102,   0 },
  { 153,  51,   0 }, { 153,   0,   0 }, { 102, 255, 102 }, { 102, 204, 102 },
  { 102, 153, 102 }, { 102, 102, 102 }, { 102,  51, 102 }, { 102,   0, 102 },
  { 102, 255,  51 }, { 102, 204,  51 }, { 102, 153,  51 }, { 102, 102,  51 },
  { 102,  51,  51 }, { 102,   0,  51 }, { 102, 255,   0 }, { 102, 204,   0 },
  { 102, 153,   0 }, { 102, 102,   0 }, { 102,  51,   0 }, { 102,   0,   0 },
  {  51, 255, 102 }, {  51, 204, 102 }, {  51, 153, 102 }, {  51, 102, 102 },
  {  51,  51, 102 }, {  51,   0, 102 }, {  51, 255,  51 }, {  51, 204,  51 },
  {  51, 153,  51 }, {  51, 102,  51 }, {  51,  51,  51 }, {  51,   0,  51 },
  {  51, 255,   0 }, {  51, 204,   0 }, {  51, 153,   0 }, {  51, 102,   0 },
  {  51,  51,   0 }, {  51,   0,   0 }, {   0, 255, 102 }, {   0, 204, 102 },
  {   0, 153, 102 }, {   0, 102, 102 }, {   0,  51, 102 }, {   0,   0, 102 },
  {   0, 255,  51 }, {   0, 204,  51 }, {   0, 153,  51 }, {   0, 102,  51 },
  {   0,  51,  51 }, {   0,   0,  51 }, {   0, 255,   0 }, {   0, 204,   0 },
  {   0, 153,   0 }, {   0, 102,   0 }, {   0,  51,   0 }, {  17,  17,  17 },
  {  34,  34,  34 }, {  68,  68,  68 }, {  85,  85,  85 }, { 119, 119, 119 },
  { 136, 136, 136 }, { 170, 170, 170 }, { 187, 187, 187 }, { 221, 221, 221 },
  { 238, 238, 238 }, { 192, 192, 192 }, { 128,   0,   0 }, { 128,   0, 128 },
  {   0, 128,   0 }, {   0, 128, 128 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }
};



Colormap
palmcolor_build_default_8bit_colormap(void) {

    unsigned int i;

    Colormap cm;

    MALLOCVAR_NOFAIL(cm);
    cm->nentries = 232;
    MALLOCARRAY_NOFAIL(cm->color_entries, cm->nentries);

    /* Fill in the colors */
    for (i = 0; i < 231; ++i) {
        cm->color_entries[i] = ((i << 24) |
                                (PalmPalette8bpp[i][0] << 16) |
                                (PalmPalette8bpp[i][1] << 8) |
                                (PalmPalette8bpp[i][2]));
    }
    cm->color_entries[231] = 0xFF000000;
    cm->ncolors = 232;

    /* now sort the table */
    qsort (cm->color_entries, cm->ncolors, sizeof(Color_s), 
           palmcolor_compare_colors);
    return cm;
}



Colormap
palmcolor_build_custom_8bit_colormap(unsigned int const rows,
                                     unsigned int const cols,
                                     pixel **     const pixels) {
    unsigned int row;
    Colormap colormap;

    MALLOCVAR_NOFAIL(colormap);
    colormap->nentries = 256;
    MALLOCARRAY_NOFAIL(colormap->color_entries, colormap->nentries);
    colormap->ncolors = 0;  /* initial value */
    
    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            Color found;
            Color_s temp;

            temp = ((PPM_GETR(pixels[row][col]) << 16) |
                    (PPM_GETG(pixels[row][col]) << 8) |
                    PPM_GETB(pixels[row][col]));
            found = (bsearch (&temp,
                              colormap->color_entries, colormap->ncolors,
                              sizeof(Color_s), palmcolor_compare_colors));
            if (!found) {
                if (colormap->ncolors >= colormap->nentries)
                    pm_error("Too many colors for custom colormap "
                             "(max 256).  "
                             "Try using pnmquant to reduce the number "
                             "of colors.");
                else {
                    /* add the new color, and re-sort */
                    temp |= ((colormap->ncolors) << 24);
                    colormap->color_entries[colormap->ncolors] = temp;
                    colormap->ncolors += 1;
                    qsort(colormap->color_entries, colormap->ncolors, 
                          sizeof(Color_s), palmcolor_compare_colors);
                }
            }
        }
    }
    return colormap;
}



Colormap
palmcolor_read_colormap (FILE * const ifP) {

    unsigned short ncolors;
    Colormap retval;
    int rc;
    
    rc = pm_readbigshort(ifP, (short *) &ncolors);
    if (rc != 0)
        retval = NULL;
    else {
        long colorentry;
        Colormap colormap;
        unsigned int i;
        bool error;

        MALLOCVAR_NOFAIL(colormap);
        colormap->nentries = ncolors;
        MALLOCARRAY_NOFAIL(colormap->color_entries, colormap->nentries);
        
        for (i = 0, error = FALSE;  i < ncolors && !error;  ++i) {
            int rc;
            rc = pm_readbiglong(ifP, &colorentry);
            if (rc != 0)
                error = TRUE;
            else
                colormap->color_entries[i] = (colorentry & 0xFFFFFFFF);
        }
        if (error) {
            free (colormap->color_entries);
            free (colormap);
            retval = NULL;
        } else {
            colormap->ncolors = ncolors;
            retval = colormap;
        }
    }
    return retval;
}
