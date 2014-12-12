/* gen_palm_colormap.c - generate a ppm file containing the default Palm colormap
 *
 * Bill Janssen  <bill@janssen.org>
 */

#include "pnm.h"

#include "palm.h"

int main( int argc, char **argv ) {

  int i;
  Color_s current;
  Colormap default_map = palmcolor_build_default_8bit_colormap ();

  printf("P3\n%d 1\n255\n", default_map->ncolors);
  for (i = 0;  i < default_map->ncolors;  i++) {
    current = default_map->color_entries[i];
    printf ("%d %d %d\n", (current & 0xFF0000) >> 16, (current & 0xFF00) >> 8, (current & 0xFF));
    /* printf ("%x:  %d %d %d\n", (current & 0xFF000000) >> 24, (current & 0xFF0000) >> 16, (current & 0xFF00) >> 8, (current & 0xFF)); */
  };
  return 0;
}

