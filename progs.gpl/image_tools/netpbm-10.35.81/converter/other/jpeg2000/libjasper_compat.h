/* Here's some stuff to create backward compatibility with older Jasper
   libraries.  Unfortunately, new versions of the Jasper library are not
   backward compatible with old applications.
*/
/* The color space thing got more complex between Version 1.600 and
   1.701.  For example, it now allows for multiple kinds of RGB, whereas
   in 1.600 RGB meant SRGB.  As part of that change, names changed
   from "colorspace" to "clrspc".
*/
#if defined(jas_image_setcolorspace)
/* Old style color space */
#define jas_image_setclrspc jas_image_setcolorspace
#define JAS_CLRSPC_GENRGB JAS_IMAGE_CS_RGB
#define JAS_CLRSPC_GENGRAY JAS_IMAGE_CS_GRAY
#define JAS_CLRSPC_UNKNOWN JAS_IMAGE_CS_UNKNOWN

#define jas_clrspc_fam(clrspc) (clrspc)
#define jas_image_clrspc jas_image_colorspace

#define JAS_CLRSPC_FAM_RGB JAS_IMAGE_CS_RGB
#define JAS_CLRSPC_FAM_GRAY JAS_IMAGE_CS_GRAY
#define JAS_CLRSPC_FAM_UNKNOWN JAS_IMAGE_CS_UNKNOWN

#endif
