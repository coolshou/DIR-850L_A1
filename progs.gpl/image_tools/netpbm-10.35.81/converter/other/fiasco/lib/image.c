/*
 *  image.c:        Input and output of PNM images.
 *
 *  Written by:     Ullrich Hafner
 *      
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/15 17:21:30 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#include "pnm.h"

#include <string.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "fiasco.h"
#include "misc.h"
#include "image.h"

/*****************************************************************************

                prototypes
  
*****************************************************************************/

static void
init_chroma_tables (void);

/*****************************************************************************

                local variables
  
*****************************************************************************/
static int *Cr_r_tab = NULL;
static int *Cr_g_tab = NULL;
static int *Cb_g_tab = NULL;
static int *Cb_b_tab = NULL;

/*****************************************************************************

                public code
  
*****************************************************************************/

static fiasco_image_t *
make_image_base(void)
{
    fiasco_image_t * const imageP = Calloc (1, sizeof (fiasco_image_t));

    if (imageP == NULL)
        pm_error("Failed to allocate memory for image object");
    else {
        imageP->delete     = fiasco_image_delete;
        imageP->get_width  = fiasco_image_get_width;
        imageP->get_height = fiasco_image_get_height;
        imageP->is_color   = fiasco_image_is_color;
    }
    return imageP;
}



fiasco_image_t *
fiasco_image_new_file(const char * const filename)
/*
 *  FIASCO image constructor.
 *  Allocate memory for the FIASCO image structure and
 *  load the image from PNM file `filename'.
 *
 *  Return value:
 *  pointer to the new image structure
 *  or NULL in case of an error
 */
{
    fiasco_image_t * imageP;

    imageP = make_image_base();

    imageP->private = read_image_file(filename);

    return imageP;
}



fiasco_image_t *
fiasco_image_new_stream(FILE *       const ifP,
                        unsigned int const width,
                        unsigned int const height,
                        xelval       const maxval,
                        int          const format)
/*
 *  FIASCO image constructor.
 *  Allocate memory for the FIASCO image structure and
 *  load the image from the PNM stream in *ifP, which is positioned just
 *  after the header.  'width', 'height', 'maxval', and 'format' are the
 *  parameters of the image, i.e. the contents of that header.
 *
 *  Return value:
 *  pointer to the new image structure
 *  or NULL in case of an error
 */
{
    fiasco_image_t * imageP;

    imageP = make_image_base();

    imageP->private = read_image_stream(ifP, width, height, maxval, format);

    return imageP;
}



void
fiasco_image_delete (fiasco_image_t *image)
/*
 *  FIASCO image destructor.
 *  Free memory of FIASCO image struct.
 *
 *  No return value.
 *
 *  Side effects:
 *  structure 'image' is discarded.
 */
{
   image_t *this = cast_image (image);

   if (!this)
      return;

   try
   {
      free_image (this);
   }
   catch
   {
      return;
   }
}

unsigned
fiasco_image_get_width (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->width;
}

unsigned
fiasco_image_get_height (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->width;
}

int
fiasco_image_is_color (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->color;
}

image_t *
cast_image (fiasco_image_t *image)
/*
 *  Cast pointer `image' to type image_t.
 *  Check whether `image' is a valid object of type image_t.
 *
 *  Return value:
 *  pointer to dfiasco_t struct on success
 *      NULL otherwise
 */
{
   image_t *this = (image_t *) image->private;
   if (this)
   {
      if (!streq (this->id, "IFIASCO"))
      {
     set_error (_("Parameter `image' doesn't match required type."));
     return NULL;
      }
   }
   else
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "image");
   }

   return this;
}

image_t *
alloc_image (unsigned width, unsigned height, bool_t color, format_e format)
/*
 *  Image constructor:
 *  Allocate memory for the image_t structure.
 *  Image size is given by 'width' and 'height'.
 *  If 'color' == YES then allocate memory for three color bands (Y, Cb, Cr).
 *  otherwise just allocate memory for a grayscale image.
 *  'format' specifies whether image pixels of color images
 *  are stored in 4:4:4 or 4:2:0 format.
 *
 *  Return value:
 *  pointer to the new image structure.
 */
{
   image_t *image;
   color_e band;

   if ((width & 1) || (height & 1))
      error ("Width and height of images must be even numbers.");
   if (!color)
      format = FORMAT_4_4_4;

   image              = Calloc (1, sizeof (image_t));
   image->width       = width;
   image->height      = height;
   image->color       = color;
   image->format      = format;
   image->reference_count = 1;
   
   strcpy (image->id, "IFIASCO");

   for (band = first_band (color); band <= last_band (color); band++)
      if (format == FORMAT_4_2_0 && band != Y)
     image->pixels [band] = Calloc ((width * height) >> 2,
                    sizeof (word_t));
      else
     image->pixels [band] = Calloc (width * height, sizeof (word_t));
   
   return image;
}

image_t *
clone_image (image_t *image)
/*
 *  Copy constructor:
 *  Construct new image by copying the given `image'.
 *
 *  Return value:
 *  pointer to the new image structure.
 */
{
   image_t *new = alloc_image (image->width, image->height, image->color,
                   image->format);
   color_e band;
   
   for (band = first_band (new->color); band <= last_band (new->color); band++)
      if (new->format == FORMAT_4_2_0 && band != Y)
      {
     memcpy (new->pixels [band], image->pixels [band],
         ((new->width * new->height) >> 2) * sizeof (word_t));
      }
      else
      {
     memcpy (new->pixels [band], image->pixels [band],
         new->width * new->height * sizeof (word_t));
      }

   return new;
}

void
free_image (image_t *image)
/*
 *  Image destructor:
 *  Free memory of 'image' struct and pixel data.
 *
 *  No return value.
 *
 *  Side effects:
 *  structure 'image' is discarded.
 */
{
   if (image != NULL)
   {
      if (--image->reference_count)
     return;            /* image is still referenced */
      else
      {
     color_e band;

     for (band  = first_band (image->color);
          band <= last_band (image->color); band++)
        if (image->pixels [band])
           Free (image->pixels [band]);
     Free (image);
      }
   }
   else
      warning ("Can't free image <NULL>.");
}


static void 
read_image_data(image_t * const image, FILE *input, const bool_t color,
                const int width, const int height, const xelval maxval,
                const int format) {
   int row;
   int i;      /* Cursor into image->pixels arrays */
   xel * xelrow;
   /* The following are just the normal rgb -> YCbCr conversion matrix,
      except normalization to maxval 4095 (12 bit color) is built in
      */
   const double coeff_lu_r = +0.2989 / maxval * 4095;
   const double coeff_lu_g = +0.5866 / maxval * 4095;
   const double coeff_lu_b = +0.1145 / maxval * 4095;
   const double coeff_cb_r = -0.1687 / maxval * 4095;
   const double coeff_cb_g = -0.3312 / maxval * 4095;
   const double coeff_cb_b = +0.5000 / maxval * 4095;
   const double coeff_cr_r = +0.5000 / maxval * 4095;
   const double coeff_cr_g = -0.4183 / maxval * 4095;
   const double coeff_cr_b = -0.0816 / maxval * 4095;

   xelrow = pnm_allocrow(width);

   i = 0; 
   for (row = 0; row < height; row++) {
       int col;
       pnm_readpnmrow(input, xelrow, width, maxval, format);
       for (col = 0; col < width; col++) {
           if (color) {
               image->pixels[Y][i] = 
                   coeff_lu_r * PPM_GETR(xelrow[col]) 
                   + coeff_lu_g * PPM_GETG(xelrow[col])
                   + coeff_lu_b * PPM_GETB(xelrow[col]) - 2048;
               image->pixels[Cb][i] = 
                   coeff_cb_r * PPM_GETR(xelrow[col]) 
                   + coeff_cb_g * PPM_GETG(xelrow[col])
                   + coeff_cb_b * PPM_GETB(xelrow[col]);
               image->pixels[Cr][i] = 
                   coeff_cr_r * PPM_GETR(xelrow[col]) 
                   + coeff_cr_g * PPM_GETG(xelrow[col])
                   + coeff_cr_b * PPM_GETB(xelrow[col]);

               i++;
           } else 
               image->pixels[GRAY][i++] =
                   PNM_GET1(xelrow[col]) * 4095 / maxval - 2048;
       }
   }

   free(xelrow);
}



image_t *
read_image_stream(FILE *       const ifP,
                  unsigned int const width,
                  unsigned int const height,
                  xelval       const maxval,
                  int          const format)
/*
 * Read one PNM image from stream *ifP, which is positioned just after the
 *  header.  'width', 'height', 'maxval', and 'format' are the parameters of
 *  the image (i.e. the contents of that header).
 */
{
   image_t  *image;         /* pointer to new image structure */
   bool_t    color;         /* color image ? (YES/NO) */

   if (PNM_FORMAT_TYPE(format) == PPM_FORMAT)
       color = YES;
   else
       color = NO;

   if (width < 32)
       pm_error("Image must have a width of at least 32 pixels.");

   if (height < 32)
       pm_error("Image must have a height of at least 32 pixels.");

   image = alloc_image (width, height, color, FORMAT_4_4_4);

   read_image_data(image, ifP, color, width, height, maxval, format);

   return image;
}



image_t *
read_image_file(const char * const filename)
/*
 *  Read the PNM image from the file named 'filename'.
 *
 *  Return value:
 *  pointer to the image structure.
 */
{
    FILE * ifP;
    int    width, height;    /* image dimensions */
    xelval   maxval;         /* Maxval of image */
    int format;              /* Image's format code */
    image_t * imageP;        /* pointer to new image structure */

    ifP = pm_openr(filename);

    pnm_readpnminit(ifP, &width, &height, &maxval, &format);

    imageP = read_image_stream(ifP, width, height, maxval, format);

    pm_close(ifP);

    return imageP;
}



void
write_image (const char *image_name, const image_t *image)
/*
 *  Write given 'image' data to the file 'image_name'.
 *  
 *  No return value.
 */
{
   FILE *output;            /* output stream */
   int format;
   int row;
   int i;     /* Cursor into image->pixel arrays */
   xel * xelrow;
   unsigned *gray_clip;         /* clipping table */

   assert (image && image_name);
   
   if (image->format == FORMAT_4_2_0)
   {
      warning ("Writing of images in 4:2:0 format not supported.");
      return;
   }
   
   if (image_name == NULL)
       output = stdout;
   else if (streq(image_name, "-"))
       output = stdout;
   else
       output = pm_openw((char*)image_name);

   gray_clip  = init_clipping ();   /* mapping of int -> unsigned */
   if (!gray_clip)
      error (fiasco_get_error_message ());
   init_chroma_tables ();

   format = image->color ? PPM_TYPE : PGM_TYPE;
   
   pnm_writepnminit(output, image->width, image->height, 255, format, 0);

   xelrow = pnm_allocrow(image->width);
   i = 0;
   for (row = 0; row < image->height; row++) {
       int col;
       for (col = 0; col < image->width; col++) {
           if (image->color) {
               word_t yval, cbval, crval;

               yval  = image->pixels[Y][i]  / 16 + 128;
               cbval = image->pixels[Cb][i] / 16;
               crval = image->pixels[Cr][i] / 16;

               PPM_ASSIGN(xelrow[col], 
                          gray_clip[yval + Cr_r_tab[crval]],
                          gray_clip[yval + Cr_g_tab[crval] + Cb_g_tab [cbval]],
                          gray_clip[yval + Cb_b_tab[cbval]]);

           } else
               /* The 16 below should be 4095/255 = 16.0588 */
               PNM_ASSIGN1(xelrow[col], 
                           gray_clip[image->pixels[GRAY][i]/16+128]);
           i++;
       }
       pnm_writepnmrow(output, xelrow, 
                       image->width, 255, format, 0);
   }
   pnm_freerow(xelrow);

   pm_close(output);
}

bool_t
same_image_type (const image_t *img1, const image_t *img2)
/*
 *  Check whether the given images 'img1' and `img2' are of the same type.
 *
 *  Return value:
 *  YES if images 'img1' and `img2' are of the same type
 *  NO  otherwise.
 */
{
   assert (img1 && img2);
   
   return ((img1->width == img2->width)
       && (img1->height == img2->height)
       && (img1->color == img2->color)
       && (img1->format == img2->format));
}

/*****************************************************************************

                private code
  
*****************************************************************************/

static void
init_chroma_tables (void)
/*
 *  Chroma tables are used to perform fast YCbCr->RGB color space conversion.
 */
{
   int crval, cbval, i;

   if (Cr_r_tab != NULL || Cr_g_tab != NULL ||
       Cb_g_tab != NULL || Cb_b_tab != NULL)
      return;

   Cr_r_tab = Calloc (768, sizeof (int));
   Cr_g_tab = Calloc (768, sizeof (int));
   Cb_g_tab = Calloc (768, sizeof (int));
   Cb_b_tab = Calloc (768, sizeof (int));

   for (i = 256; i < 512; i++)
   {
      cbval = crval  = i - 128 - 256;

      Cr_r_tab[i] =  1.4022 * crval + 0.5;
      Cr_g_tab[i] = -0.7145 * crval + 0.5;
      Cb_g_tab[i] = -0.3456 * cbval + 0.5; 
      Cb_b_tab[i] =  1.7710 * cbval + 0.5;
   }
   for (i = 0; i < 256; i++)
   {
      Cr_r_tab[i] = Cr_r_tab[256];
      Cr_g_tab[i] = Cr_g_tab[256];
      Cb_g_tab[i] = Cb_g_tab[256]; 
      Cb_b_tab[i] = Cb_b_tab[256];
   }
   for (i = 512; i < 768; i++)
   {
      Cr_r_tab[i] = Cr_r_tab[511];
      Cr_g_tab[i] = Cr_g_tab[511];
      Cb_g_tab[i] = Cb_g_tab[511]; 
      Cb_b_tab[i] = Cb_b_tab[511];
   }

   Cr_r_tab += 256 + 128;
   Cr_g_tab += 256 + 128;
   Cb_g_tab += 256 + 128;
   Cb_b_tab += 256 + 128;
}
