/* pnmmontage.c - build a montage of portable anymaps
 *
 * Copyright 2000 Ben Olmstead.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */

#include <limits.h>
#include <string.h>

#include "pam.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"

typedef struct { int f[sizeof(int) * 8 + 1]; } factorset;
typedef struct { int x; int y; } coord;

static int qfactor = 200;
static int quality = 5;

static factorset 
factor(int n)
{
  int i, j;
  factorset f;
  for (i = 0; i < sizeof(int) * 8 + 1; ++i)
    f.f[i] = 0;
  for (i = 2, j = 0; n > 1; ++i)
  {
    if (n % i == 0)
      f.f[j++] = i, n /= i, --i;
  }
  return (f);
}

static int 
gcd(int n, int m)
{
  factorset nf, mf;
  int i, j;
  int g;

  nf = factor(n);
  mf = factor(m);

  i = j = 0;
  g = 1;
  while (nf.f[i] && mf.f[j])
  {
    if (nf.f[i] == mf.f[j])
      g *= nf.f[i], ++i, ++j;
    else if (nf.f[i] < mf.f[j])
      ++i;
    else
      ++j;
  }
  return (g);
}

static __inline__ int imax(int n, int m) { return (n > m ? n : m); }

static int 
checkcollision(coord *locs, coord *szs, coord *cloc, coord *csz, int n)
{
  int i;
  for (i = 0; i < n; ++i)
  {
    if ((locs[i].x < cloc->x + csz->x) &&
        (locs[i].y < cloc->y + csz->y) &&
        (locs[i].x + szs[i].x > cloc->x) &&
        (locs[i].y + szs[i].y > cloc->y))
      return (1);
  }
  return (0);
}

static void 
recursefindpack(coord *current, coord currentsz, coord *set, 
                coord *best, int minarea, int *maxarea, 
                int depth, int n, int xinc, int yinc)
{
  coord c;
  if (depth == n)
  {
    if (currentsz.x * currentsz.y < *maxarea)
    {
      memcpy(best, current, sizeof(coord) * n);
      *maxarea = currentsz.x * currentsz.y;
    }
    return;
  }

  for (current[depth].x = 0; 
       imax(current[depth].x + set[depth].x, currentsz.x) * 
           imax(currentsz.y, set[depth].y) < *maxarea; 
       current[depth].x += xinc)
  {
    for (current[depth].y = 0; 
         imax(current[depth].x + set[depth].x, currentsz.x) * 
             imax(currentsz.y, current[depth].y + set[depth].y) < *maxarea; 
         current[depth].y += yinc)
    {
      c.x = imax(current[depth].x + set[depth].x, currentsz.x);
      c.y = imax(current[depth].y + set[depth].y, currentsz.y);
      if (!checkcollision(current, set, &current[depth], &set[depth], depth))
      {
        recursefindpack(current, c, set, best, minarea, maxarea, 
                        depth + 1, n, xinc, yinc);
        if (*maxarea <= minarea)
          return;
      }
    }
  }
}

static void 
findpack(struct pam *imgs, int n, coord *coords)
{
  int minarea;
  int i;
  int rdiv;
  int cdiv;
  int minx = -1;
  int miny = -1;
  coord *current;
  coord *set;
  int z = INT_MAX;
  coord c = { 0, 0 };

  if (quality > 1)
  {
    for (minarea = i = 0; i < n; ++i)
      minarea += imgs[i].height * imgs[i].width,
      minx = imax(minx, imgs[i].width),
      miny = imax(miny, imgs[i].height);

    minarea = minarea * qfactor / 100;
  }
  else
  {
    minarea = INT_MAX - 1;
  }

  /* It's relatively easy to show that, if all the images
   * are multiples of a particular size, then a best
   * packing will always align the images on a grid of
   * that size.
   *
   * This speeds computation immensely.
   */
  for (rdiv = imgs[0].height, i = 1; i < n; ++i)
    rdiv = gcd(imgs[i].height, rdiv);

  for (cdiv = imgs[0].width, i = 1; i < n; ++i)
    cdiv = gcd(imgs[i].width, cdiv);

  MALLOCARRAY(current, n);
  MALLOCARRAY(set, n);
  for (i = 0; i < n; ++i)
    set[i].x = imgs[i].width,
    set[i].y = imgs[i].height;
  recursefindpack(current, c, set, coords, minarea, &z, 0, n, cdiv, rdiv);
}



static void 
adjustDepth(tuple *            const tuplerow,
            const struct pam * const inpamP,
            const struct pam * const outpamP,
            coord              const coord) {

    if (inpamP->depth < outpamP->depth) {
        unsigned int i;
        for (i = coord.x; i < coord.x + inpamP->width; ++i) {
            int j;
            for (j = inpamP->depth; j < outpamP->depth; ++j)
                tuplerow[i][j] = tuplerow[i][inpamP->depth - 1];
        }
    }
}



static void 
adjustMaxval(tuple *            const tuplerow,
             const struct pam * const inpamP,
             const struct pam * const outpamP,
             coord              const coord) {

    if (inpamP->maxval < outpamP->maxval) {
        int i;
        for (i = coord.x; i < coord.x + inpamP->width; ++i) {
            int j;
            for (j = 0; j < outpamP->depth; ++j)
                tuplerow[i][j] *= outpamP->maxval / inpamP->maxval;
        }
    }
}



static void
writePam(struct pam *       const outpamP,
         unsigned int       const nfiles,
         const coord *      const coords,
         const struct pam * const imgs) {

    tuple *tuplerow;
    int i;
  
    pnm_writepaminit(outpamP);

    tuplerow = pnm_allocpamrow(outpamP);

    for (i = 0; i < outpamP->height; ++i) {
        int j;
        for (j = 0; j < nfiles; ++j) {
            if (coords[j].y <= i && i < coords[j].y + imgs[j].height) {
                pnm_readpamrow(&imgs[j], &tuplerow[coords[j].x]);
                adjustDepth(tuplerow, &imgs[j], outpamP, coords[j]);

                adjustMaxval(tuplerow, &imgs[j], outpamP, coords[j]);

            }
        }
        pnm_writepamrow(outpamP, tuplerow);
    }
    pnm_freepamrow(tuplerow);
}



int 
main(int argc, char **argv)
{
  struct pam *imgs;
  struct pam outimg;
  struct pam p;
  int nfiles;
  int i, j;
  unsigned int q[10];
  coord *coords;
  const char *headfname = NULL;
  const char *datafname = NULL;
  const char *prefix = "";
  FILE *header;
  FILE *data;
  char **names;
  char *c;

  optEntry *option_def = malloc(100*sizeof(optEntry));
      /* Instructions to OptParseOptions3 on how to parse our options.
       */
  optStruct3 opt;

  unsigned int option_def_index;

  option_def_index = 0;   /* incremented by OPTENTRY */
  OPTENT3( 0,  "data",    OPT_STRING, &datafname, NULL, 0);
  OPTENT3( 0,  "header",  OPT_STRING, &headfname, NULL, 0);
  OPTENT3('q', "quality", OPT_UINT,   &qfactor,   NULL, 0);
  OPTENT3('p', "prefix",  OPT_STRING, &prefix,    NULL, 0);
  OPTENT3('0', "0",       OPT_FLAG,   NULL, &q[0],      0);
  OPTENT3('1', "1",       OPT_FLAG,   NULL, &q[1],      0);
  OPTENT3('2', "2",       OPT_FLAG,   NULL, &q[2],      0);
  OPTENT3('3', "3",       OPT_FLAG,   NULL, &q[3],      0);
  OPTENT3('4', "4",       OPT_FLAG,   NULL, &q[4],      0);
  OPTENT3('5', "5",       OPT_FLAG,   NULL, &q[5],      0);
  OPTENT3('6', "6",       OPT_FLAG,   NULL, &q[6],      0);
  OPTENT3('7', "7",       OPT_FLAG,   NULL, &q[7],      0);
  OPTENT3('8', "8",       OPT_FLAG,   NULL, &q[8],      0);
  OPTENT3('9', "9",       OPT_FLAG,   NULL, &q[9],      0);

  opt.opt_table = option_def;
  opt.short_allowed = FALSE;
  opt.allowNegNum = FALSE;

  pnm_init(&argc, argv);

  /* Check for flags. */
  optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

  if (headfname)
    header = pm_openw(headfname);

  if (datafname)
    data = pm_openw(datafname);

  for (i = 0; i < 10; ++i)
  {
    if (q[i])
    {
      quality = i;
      switch (quality)
      {
        case 0: case 1: break;
        case 2: case 3: case 4: case 5: case 6: 
            qfactor = 100 * (8 - quality); 
            break;
        case 7: qfactor = 150; break;
        case 8: qfactor = 125; break;
        case 9: qfactor = 100; break;
      }
    }
  }

  if (1 < argc)
    nfiles = argc - 1;
  else
    nfiles = 1;

  MALLOCARRAY(imgs, nfiles);
  MALLOCARRAY(coords, nfiles);
  MALLOCARRAY(names, nfiles);
  
  if (!imgs || !coords || !names)
    pm_error("out of memory");

  if (1 < argc)
  {
    for (i = 0; i < nfiles; ++i)
    {
      if (strchr(argv[i+1], ':'))
      {
        imgs[i].file = pm_openr(strchr(argv[i+1], ':') + 1);
        *strchr(argv[i+1], ':') = 0;
        names[i] = argv[i+1];
      }
      else
      {
        imgs[i].file = pm_openr(argv[i+1]);
        names[i] = argv[i+1];
      }
    }
  }
  else
  {
    imgs[0].file = stdin;
  }

  pnm_readpaminit(imgs[0].file, &imgs[0], PAM_STRUCT_SIZE(tuple_type));
  outimg.maxval = imgs[0].maxval;
  outimg.format = imgs[0].format;
  memcpy(outimg.tuple_type, imgs[0].tuple_type, sizeof(imgs[0].tuple_type));
  outimg.depth = imgs[0].depth;

  for (i = 1; i < nfiles; ++i)
  {
    pnm_readpaminit(imgs[i].file, &imgs[i], PAM_STRUCT_SIZE(tuple_type));
    if (PAM_FORMAT_TYPE(imgs[i].format) > PAM_FORMAT_TYPE(outimg.format))
      outimg.format = imgs[i].format,
      memcpy(outimg.tuple_type, imgs[i].tuple_type, 
             sizeof(imgs[i].tuple_type));
    outimg.maxval = imax(imgs[i].maxval, outimg.maxval);
    outimg.depth = imax(imgs[i].depth, outimg.depth);
  }

  for (i = 0; i < nfiles - 1; ++i)
    for (j = i + 1; j < nfiles; ++j)
      if (imgs[j].width * imgs[j].height > imgs[i].width * imgs[i].height)
        p = imgs[i], imgs[i] = imgs[j], imgs[j] = p,
        c = names[i], names[i] = names[j], names[j] = c;

  findpack(imgs, nfiles, coords);

  outimg.height = outimg.width = 0;
  for (i = 0; i < nfiles; ++i)
  {
    outimg.width = imax(outimg.width, imgs[i].width + coords[i].x);
    outimg.height = imax(outimg.height, imgs[i].height + coords[i].y);
  }

  outimg.size = sizeof(outimg);
  outimg.len = sizeof(outimg);
  outimg.file = stdout;
  outimg.bytes_per_sample = 0;
  for (i = outimg.maxval; i; i >>= 8)
    ++outimg.bytes_per_sample;

  writePam(&outimg, nfiles, coords, imgs);

  if (datafname)
  {
    fprintf(data, ":0:0:%u:%u\n", outimg.width, outimg.height);

    for (i = 0; i < nfiles; ++i)
    {
      fprintf(data, "%s:%u:%u:%u:%u\n", names[i], coords[i].x,
          coords[i].y, imgs[i].width, imgs[i].height);
    }
  }

  if (headfname)
  {
    fprintf(header, "#define %sOVERALLX %u\n"
                    "#define %sOVERALLY %u\n"
                    "\n",
                    prefix, outimg.width,
                    prefix, outimg.height);

    for (i = 0; i < nfiles; ++i)
    {
      *strchr(names[i], '.') = 0;
      for (j = 0; names[i][j]; ++j)
      {
        if (ISLOWER(names[i][j]))
          names[i][j] = TOUPPER(names[i][j]);
      }
      fprintf(header, "#define %s%sX %u\n"
                      "#define %s%sY %u\n"
                      "#define %s%sSZX %u\n"
                      "#define %s%sSZY %u\n"
                      "\n",
                      prefix, names[i], coords[i].x,
                      prefix, names[i], coords[i].y,
                      prefix, names[i], imgs[i].width,
                      prefix, names[i], imgs[i].height);
    }
  }

  for (i = 0; i < nfiles; ++i)
    pm_close(imgs[i].file);
  pm_close(stdout);

  if (headfname)
    pm_close(header);

  if (datafname)
    pm_close(data);

  return 0;
}
