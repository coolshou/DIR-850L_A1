/* ppmtoarbtxt.c - convert portable pixmap to cleartext
**
** Renamed from ppmtotxt.c by Bryan Henderson in January 2003.
**
** Copyright (C) 1995 by Peter Kirchgessner
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <string.h>

#include "ppm.h"
#include "mallocvar.h"
#include "nstring.h"

typedef enum {
/* The types of object we handle */
    BDATA, IRED, IGREEN, IBLUE, ILUM, FRED, FGREEN, FBLUE, FLUM,
    WIDTH, HEIGHT, POSX, POSY
} SKL_OBJ_TYP;

/* Maximum size for a format string ("%d" etc.) */
#define MAXFORMAT 16

/* The data we keep for each object */
typedef union
 {
  struct BNDAT { char *bdat;   /* Binary data (text with newlines etc.) */
                 int ndat;
               } bin_data;

  struct ICDAT { char icformat[MAXFORMAT];  /* Integer colors */
                 int icolmin, icolmax;
               } icol_data;

  struct FCDAT { char fcformat[MAXFORMAT];  /* Float colors */
                 double fcolmin, fcolmax;
               } fcol_data;

  struct IDAT  { char iformat[MAXFORMAT];   /* Integer data */
               } i_data;
 } SKL_OBJ_DATA;


/* Each object has a type and some data */
typedef struct
 { 
   SKL_OBJ_TYP otyp;
   SKL_OBJ_DATA odata;
 } SKL_OBJ;


#define MAX_SKL_HEAD_OBJ 64
#define MAX_SKL_BODY_OBJ 256
#define MAX_SKL_TAIL_OBJ 64
#define MAX_LINE_BUF 1024
#define MAX_OBJ_BUF 80


static void write_txt (fout, nobj, obj, width, height, x, y, red, green, blue)
FILE *fout;
int nobj;
SKL_OBJ *obj[];
int width, height, x, y;
double red, green, blue;

{register int count;

#define WRITE_BNDAT(fd,theobj) \
 {struct BNDAT *bdata = &((theobj)->odata.bin_data); \
       fwrite (bdata->bdat,bdata->ndat,1,fd); }

#define WRITE_ICOL(fd,theobj,thecol) \
 {struct ICDAT *icdata = &((theobj)->odata.icol_data); \
  fprintf (fd,icdata->icformat,(int)(icdata->icolmin \
                + (icdata->icolmax - icdata->icolmin)*(thecol))); }

#define WRITE_FCOL(fd,theobj,thecol) \
 {struct FCDAT *fcdata = &((theobj)->odata.fcol_data); \
  fprintf (fd,fcdata->fcformat,(double)(fcdata->fcolmin \
                + (fcdata->fcolmax - fcdata->fcolmin)*(thecol))); }

#define WRITE_IDAT(fd,theobj,thedat) \
 {struct IDAT *idata = &((theobj)->odata.i_data); \
  fprintf (fd,idata->iformat,thedat); }

 for (count = 0; count < nobj; count++)
 {
   switch (obj[count]->otyp)
   {
     case BDATA:
       WRITE_BNDAT (fout,obj[count]);
       break;
     case IRED:
       WRITE_ICOL (fout,obj[count],red);
       break;
     case IGREEN:
       WRITE_ICOL (fout,obj[count],green);
       break;
     case IBLUE:
       WRITE_ICOL (fout,obj[count],blue);
       break;
     case ILUM:
       WRITE_ICOL (fout,obj[count],0.299*red+0.587*green+0.114*blue);
       break;
     case FRED:
       WRITE_FCOL (fout,obj[count],red);
       break;
     case FGREEN:
       WRITE_FCOL (fout,obj[count],green);
       break;
     case FBLUE:
       WRITE_FCOL (fout,obj[count],blue);
       break;
     case FLUM:
       WRITE_FCOL (fout,obj[count],0.299*red+0.587*green+0.114*blue);
       break;
     case WIDTH:
       WRITE_IDAT (fout,obj[count],width);
       break;
     case HEIGHT:
       WRITE_IDAT (fout,obj[count],height);
       break;
     case POSX:
       WRITE_IDAT (fout,obj[count],x);
       break;
     case POSY:
       WRITE_IDAT (fout,obj[count],y);
       break;
   }
 }
}


static SKL_OBJ *
save_bin_data(int    const ndat, 
              char * const bdat) {

    /* Save binary data in Object */

    SKL_OBJ *obj;

    obj = (SKL_OBJ *)malloc (sizeof (SKL_OBJ) + ndat);
    if (obj != NULL)
    {
        obj->otyp = BDATA;
        obj->odata.bin_data.ndat = ndat;
        obj->odata.bin_data.bdat = ((char *)(obj))+sizeof (SKL_OBJ);
        memcpy (obj->odata.bin_data.bdat,bdat,ndat);
    }
    return (obj);
}



/* Save integer color data in Object */
static SKL_OBJ *save_icol_data (ctyp,format,icolmin,icolmax)
SKL_OBJ_TYP ctyp;
char *format;
int icolmin, icolmax;

{SKL_OBJ *obj;

 obj = (SKL_OBJ *)malloc (sizeof (SKL_OBJ));
 if (obj != NULL)
 {
   obj->otyp = ctyp;
   strcpy (obj->odata.icol_data.icformat,format);
   obj->odata.icol_data.icolmin = icolmin;
   obj->odata.icol_data.icolmax = icolmax;
 }
 return (obj);
}


/* Save float color data in Object */
static SKL_OBJ *save_fcol_data (ctyp,format,fcolmin,fcolmax)
SKL_OBJ_TYP ctyp;
char *format;
double fcolmin, fcolmax;

{SKL_OBJ *obj;

 obj = (SKL_OBJ *)malloc (sizeof (SKL_OBJ));
 if (obj != NULL)
 {
   obj->otyp = ctyp;
   strcpy (obj->odata.fcol_data.fcformat,format);
   obj->odata.fcol_data.fcolmin = fcolmin;
   obj->odata.fcol_data.fcolmax = fcolmax;
 }
 return (obj);
}


/* Save universal data in Object */
static SKL_OBJ *save_i_data (ctyp,format)
SKL_OBJ_TYP ctyp;
char *format;

{SKL_OBJ *obj;

 obj = (SKL_OBJ *)malloc (sizeof (SKL_OBJ));
 if (obj != NULL)
 {
   obj->otyp = ctyp;
   strcpy (obj->odata.i_data.iformat,format);
 }
 return (obj);
}


/* Read skeleton file */
static int read_skeleton (filename,maxskl,nskl,skl)
char *filename;
int maxskl,*nskl;
SKL_OBJ **skl;

{FILE *sklfile;
 int slen, objlen, chr, n_odata;
 int icolmin,icolmax;
 double fcolmin,fcolmax;
 SKL_OBJ_TYP otyp;
 char line[MAX_LINE_BUF+MAX_OBJ_BUF+16];
 char objstr[MAX_OBJ_BUF],typstr[MAX_OBJ_BUF];
 char formstr[MAX_OBJ_BUF];
 char meta1 = '#', meta2 = '(', meta3 = ')';

#define SAVE_BIN(slen,line) \
 { if ((skl[*nskl] = save_bin_data (slen,line)) != NULL) (*nskl)++; \
   slen = 0; }

#define ADD_STR(slen,line,addlen,addstr) \
 {int count=0; line[slen++] = meta1; line[slen++] = meta2; \
  while (count++ < addlen) line[slen++] = addstr[count]; }

 if ((sklfile = fopen (filename,"r")) == NULL)
   return (-1);

 /* Parse skeleton file */
 *nskl = 0;

 slen = 0;
 while ((chr = getc (sklfile)) != EOF)   /* Up to end of skeleton file */
 {
   if (*nskl >= maxskl) return (-1);

   if (slen+1 >= MAX_LINE_BUF)   /* Buffer finished ? Save as binary object */
   {
     SAVE_BIN (slen,line);
   }

   if (chr != meta1)      /* Look for start of replacement string */
   {
     line[slen++] = chr;
     continue;
   }

   if ((chr = getc (sklfile)) == EOF)
   {
     line[slen++] = meta1;
     break;
   }
   if (chr != meta2) /* '(' ? Otherwise no replacement */
   {
     line[slen++] = meta1;
     line[slen++] = chr;
     continue;
   }

   objlen = 0;
   for (;;)   /* Read replacement string up to ')' */
   {
     if (objlen == sizeof (objstr)-1) break; /* ')' not found */
     if ((chr = getc (sklfile)) == EOF) break;
     if (chr == meta3) break;
     objstr[objlen++] = chr;
   }
   objstr[objlen] = '\0'; /* Now objstr keeps data without metas */

   if (chr != meta3)    /* Object not found ? */
   {
     ADD_STR (slen,line,objlen,objstr);   /* Save what we already read */
     if (chr == EOF) break;
     continue;
   }

   typstr[0] = '\0';           /* Get typ of data */
   sscanf (objstr,"%s",typstr);

                   /* Check for integer colors */
   if      (STREQ(typstr,"ired")  ) otyp = IRED;
   else if (STREQ(typstr,"igreen")) otyp = IGREEN;
   else if (STREQ(typstr,"iblue") ) otyp = IBLUE;
   else if (STREQ(typstr,"ilum")  ) otyp = ILUM;
                   /* Check for real colors */
   else if (STREQ(typstr,"fred")  ) otyp = FRED;
   else if (STREQ(typstr,"fgreen")) otyp = FGREEN;
   else if (STREQ(typstr,"fblue") ) otyp = FBLUE;
   else if (STREQ(typstr,"flum")  ) otyp = FLUM;
                   /* Check for integer data */
   else if (STREQ(typstr,"width") ) otyp = WIDTH;
   else if (STREQ(typstr,"height")) otyp = HEIGHT;
   else if (STREQ(typstr,"posx")  ) otyp = POSX;
   else if (STREQ(typstr,"posy")  ) otyp = POSY;
   else                                    otyp = BDATA;

   if ((otyp == IRED) || (otyp == IGREEN) || (otyp == IBLUE) || (otyp == ILUM))
   {
     n_odata = sscanf (objstr,"%*s%s%d%d",formstr,&icolmin,&icolmax);

     if (n_odata == EOF)  /* No arguments specified ? Use defaults */
     {
       strcpy (formstr,"%d"); icolmin = 0; icolmax = 255;
     }
     else if (n_odata != 3)  /* Wrong specification */
     {
       otyp = BDATA;
     }
   }

   if ((otyp == FRED) || (otyp == FGREEN) || (otyp == FBLUE) || (otyp == FLUM))
   {
     n_odata = sscanf (objstr,"%*s%s%lf%lf",formstr,&fcolmin,&fcolmax);

     if (n_odata == EOF)  /* No arguments specified ? Use defaults */
     {
       strcpy (formstr,"%f"); fcolmin = 0.0; fcolmax = 1.0;
     }
     else if (n_odata != 3)  /* Wrong specification */
     {
       otyp = BDATA;
     }
   }

   if (   (otyp == WIDTH) || (otyp == HEIGHT)
       || (otyp == POSX) || (otyp == POSY))
   {
     n_odata = sscanf (objstr,"%*s%s",formstr);

     if (n_odata == EOF)  /* No arguments specified ? Use defaults */
     {
       strcpy (formstr,"%d");
     }
     else if (n_odata != 1)  /* Wrong specification */
     {
       otyp = BDATA;
     }
   }

   if (otyp != BDATA)   /* Got an object definition ? */
   {
     if (slen > 0)      /* Save what we already found */
     {
       SAVE_BIN (slen,line);
     }
   }

   /* Now process the object in objstr */
   switch (otyp)
   {
     case BDATA:   /* Bad object definition ? Save as text */
       ADD_STR (slen,line,objlen,objstr);
       break;

     case IRED:
     case IGREEN:
     case IBLUE:
     case ILUM:
       skl[*nskl] = save_icol_data (otyp,formstr,icolmin,icolmax);
       if (skl[*nskl] != NULL) (*nskl)++;
       break;

     case FRED:
     case FGREEN:
     case FBLUE:
     case FLUM:
       skl[*nskl] = save_fcol_data (otyp,formstr,fcolmin,fcolmax);
       if (skl[*nskl] != NULL) (*nskl)++;
       break;

     case WIDTH:
     case HEIGHT:
     case POSX:
     case POSY:
       skl[*nskl] = save_i_data (otyp,formstr);
       if (skl[*nskl] != NULL) (*nskl)++;
       break;
   }
 } /* EOF of skeleton file */

 if (slen > 0)      /* Drop finishing newline character */
 {
   if (line[slen-1] == '\n') slen--;
 }

 if (slen > 0)      /* Something left ? */
 {
   SAVE_BIN (slen,line);   /* Save it */
 }

 fclose (sklfile);
 return (0);
}


int main( argc, argv )
int argc;
char* argv[];

{register int col;
 register pixel* xP;
 pixel* pixelrow;
 pixval maxval,red,green,blue;
 double dmaxval;
 int argn, rows, cols, format, row;
 int head_nskl,body_nskl,tail_nskl;
 SKL_OBJ *head_skl[MAX_SKL_HEAD_OBJ];
 SKL_OBJ *body_skl[MAX_SKL_BODY_OBJ];
 SKL_OBJ *tail_skl[MAX_SKL_TAIL_OBJ];
 FILE *ifp;
 const char *usage = "bodyskl [ -hd headskl ] [ -tl tailskl ] [pnmfile]";

 ppm_init( &argc, argv );

 argn = 1;
 if (argn == argc)
   pm_usage( usage );
                          /* Read body skeleton file */
 if (read_skeleton (argv[argn],sizeof (body_skl)/sizeof (SKL_OBJ *),
                    &body_nskl,body_skl) < 0)
   pm_usage ( usage );
 ++argn;

 head_nskl = tail_nskl = 0;

 while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0')
 {
   if ( pm_keymatch ( argv[argn], "-hd", 1) && argn+1 < argc )
   {
     argn++;           /* Read header skeleton */
     if (read_skeleton (argv[argn],sizeof (head_skl)/sizeof (SKL_OBJ *),
                        &head_nskl,head_skl) < 0)
       pm_usage ( usage );
   }
   else if ( pm_keymatch ( argv[argn], "-tl", 1) && argn+1 < argc )
   {
     argn++;           /* Read tail skeleton */
     if (read_skeleton (argv[argn],sizeof (tail_skl)/sizeof (SKL_OBJ *),
                        &tail_nskl,tail_skl) < 0)
       pm_usage ( usage );
   }
   else
   {
     pm_usage ( usage );
   }
   argn++;
 }

 if ( argn != argc )
 {
   ifp = pm_openr( argv[argn] );
   ++argn;
 }
 else 
 {
   ifp = stdin;
 }

 if ( argn != argc )
   pm_usage( usage );

 ppm_readppminit( ifp, &cols, &rows, &maxval, &format );
 pixelrow = ppm_allocrow( cols );
 dmaxval = (double)maxval;

 if (head_nskl > 0)    /* Write header */
   write_txt (stdout,head_nskl,head_skl,cols,rows,0,0,0.0,0.0,0.0);

 for ( row = 0; row < rows; ++row )
 {
   ppm_readppmrow( ifp, pixelrow, cols, maxval, format );

   for ( col = 0, xP = pixelrow; col < cols; ++col, ++xP )
   {
     red = PPM_GETR( *xP );
     green = PPM_GETG( *xP );
     blue = PPM_GETB( *xP );
     write_txt (stdout,body_nskl,body_skl,cols,rows,col,row,
                red/dmaxval,green/dmaxval,blue/dmaxval);
   }
 }

 if (tail_nskl > 0)    /* Write trailer */
   write_txt (stdout,tail_nskl,tail_skl,cols,rows,0,0,0.0,0.0,0.0);

 pm_close( ifp );

 exit( 0 );
}
