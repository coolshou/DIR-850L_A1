/*
  pktopbm, adapted from "pktopx in C by Tomas Rokicki" by AJCD 1/8/90
  1998-09-22: jcn <janneke@gnu.org>
     - lots of bugfixes:
     * always read x/y offset bytes (3x)
     * reset bmx, bmy to defaults for each char
     * fix bitmap y placement of dynamically packed char
     * skip char early if no output file allocated
     - added debug output
  
  compile with: cc -lpbm -o pktopbm pktopbm.c
  */

#include <stdio.h>
#include <string.h>

#include "pm_c_util.h"
#include "nstring.h"
#include "pbm.h"

#define NAMELENGTH 80
#define MAXROWWIDTH 3200
#define MAXPKCHAR 256

typedef int integer ;
typedef unsigned char quarterword ;
typedef char boolean ;
typedef quarterword eightbits ;

static FILE *pkfile ;
static char pkname[NAMELENGTH+1] ;
static integer pktopbm_pkloc = 0;
static char *filename[MAXPKCHAR] ;
static bit **bitmap = NULL ;
static integer dynf ;
static eightbits inputbyte ;
static eightbits bitweight ;
static integer repeatcount ;
static integer flagbyte ;
static integer debug=0;

#define dprintf(s,d) if (debug) printf(s,d)
#define dprintf0(s) if (debug) printf(s)

/* add a suffix to a filename in an allocated space */
static void
pktopbm_add_suffix(char *       const name, 
                   const char * const suffix) {

    char * const slash = strrchr(name, '/');
    char * const dot   = strrchr(name, '.');
    
    if ((dot && slash ? dot < slash : !dot) && !STREQ(name, "-"))
        strcat(name, suffix);
}



/* get a byte from the PK file */
static eightbits 
pktopbm_pkbyte(void) {
   pktopbm_pkloc++ ;
   return(getc(pkfile)) ;
}



/* get a 16-bit half word from the PK file */
static integer 
get16(void) {
   integer const a = pktopbm_pkbyte() ;
   return((a<<8) + pktopbm_pkbyte()) ;
}



/* get a 32-bit word from the PK file */
static integer get32(void) {
    integer a;
    a = get16() ;
    if (a > 32767) a -= 65536 ;
    return((a<<16) + get16()) ;
}



/* get a nibble from current input byte, or new byte if no current byte */
static integer 
getnyb(void) {
    eightbits temp;
    if (bitweight == 0) {
        inputbyte = pktopbm_pkbyte() ;
        bitweight = 16 ;
    }
    temp = inputbyte / bitweight ;
    inputbyte -= temp * bitweight ;
    bitweight >>= 4 ;
    return(temp) ;
}



/* get a bit from the current input byte, or a new byte if no current byte */
static bool
getbit(void) {
    bool temp ;
    bitweight >>= 1 ;
    if (bitweight == 0) {
        inputbyte = pktopbm_pkbyte() ;
        bitweight = 128 ;
    }
    temp = (inputbyte >= bitweight) ;
    if (temp) inputbyte -= bitweight ;
    return(temp) ;
}



/* unpack a dynamically packed number. dynf is dynamic packing threshold  */
static integer 
pkpackednum(void) {
    integer i, j ;
    i = getnyb() ;
    if (i == 0) {           /* large run count, >= 3 nibbles */
        do {
            j = getnyb() ;          /* count extra nibbles */
            i++ ;
        } while (j == 0) ;
        while (i > 0) {
            j = (j<<4) + getnyb() ; /* add extra nibbles */
            i-- ;
        }
        return (j - 15 +((13 - dynf)<<4) + dynf) ;
    } else if (i <= dynf) return (i) ;  /* number > 0 and <= dynf */
    else if (i < 14) return (((i - dynf - 1)<<4) + getnyb() + dynf + 1) ;
    else {
        if (i == 14) repeatcount = pkpackednum() ;  /* get repeat count */
        else repeatcount = 1 ;      /* value 15 indicates repeat count 1 */
        return(pkpackednum()) ;
    }
}



/* skip specials in PK files, inserted by Metafont or some other program */
static void
skipspecials(void) {
    integer i, j;
    do {
        flagbyte = pktopbm_pkbyte() ;
        if (flagbyte >= 240)
            switch(flagbyte) {
            case 240:           /* specials of size 1-4 bytes */
            case 241:
            case 242:
            case 243:
                i = 0 ;
                for (j = 240 ; j <= flagbyte ; ++j) 
                    i = (i<<8) + pktopbm_pkbyte() ;
                for (j = 1 ; j <= i ; ++j) 
                    pktopbm_pkbyte() ;  /* ignore special */
                break ;
            case 244:           /* no-op, parameters to specials */
                get32() ;
            case 245:           /* start of postamble */
            case 246:           /* no-op */
                break ;
            case 247:           /* pre-amble in wrong place */
            case 248:
            case 249:
            case 250:
            case 251:
            case 252:
            case 253:
            case 254:
            case 255:
                pm_error("unexpected flag byte %d", flagbyte) ;
            }
    } while (!(flagbyte < 240 || flagbyte == 245)) ;
}



/* ignore character packet */
static void
ignorechar(integer const car, 
           integer const endofpacket) {

   while (pktopbm_pkloc != endofpacket) pktopbm_pkbyte() ;
   if (car < 0 || car >= MAXPKCHAR)
      pm_message("Character %d out of range", car) ;
   skipspecials() ;
}



int
main(int argc, char *argv[]) {
    integer x;
    integer endofpacket ;
    boolean turnon ;
    integer i, j;
    integer car ;
    integer bmx=0, bmy=0;
    integer set_bmx=0, set_bmy=0;
    bit row[MAXROWWIDTH+1] ;
    const char * const usage = 
        "pkfile[.pk] [-d] [[-x width] [-y height] [-c num] pbmfile]...";
   
    pbm_init(&argc, argv);
    for (i = 0 ; i < MAXPKCHAR ; i ++) filename[i] = NULL ;

    pm_message("This is PKtoPBM, version 2.5") ;

    if (--argc < 1) pm_usage(usage) ;

    strcpy(pkname, *++argv) ;
    pktopbm_add_suffix(pkname, ".pk") ;

    car = 0 ;
    /* urg: use getopt */
    while (++argv, --argc) {
        if (argv[0][0] == '-' && argv[0][1])
            switch (argv[0][1]) {
            case 'X':
            case 'x':
                if (argv[0][2]) bmx = atoi(*argv+2) ;
                else if (++argv, --argc) set_bmx = atoi(*argv) ;
                else pm_usage(usage) ;
                continue ;
            case 'Y':
            case 'y':
                if (argv[0][2]) bmy = atoi(*argv+2) ;
                else if (++argv, --argc) set_bmy = atoi(*argv) ;
                else pm_usage(usage) ;
                continue ;
            case 'C':
            case 'c':
                if (argv[0][2]) car = atoi(*argv+2) ;
                else if (++argv, --argc) car = atoi(*argv) ;
                else pm_usage(usage) ;
                break ;
            case 'd':
                debug=1;
                break ;
            default:
                pm_usage(usage) ;
            } else if (car < 0 || car >= MAXPKCHAR) {
                pm_error("character must be in range 0 to %d (-c)", 
                         MAXPKCHAR-1) ;
            } else filename[car++] = *argv ;
    }

    pkfile = pm_openr(pkname);
    if (pktopbm_pkbyte() != 247)
        pm_error("bad PK file (pre command missing)") ;
    if (pktopbm_pkbyte() != 89)
        pm_error("wrong version of packed file") ;
    j = pktopbm_pkbyte() ;              /* get header comment size */
    for (i = 1 ; i <= j ; i ++) pktopbm_pkbyte() ;  /* ignore header comment */
    get32() ;                   /* ignore designsize */
    get32() ;                   /* ignore checksum */
    if (get32() != get32())         /* h & v pixels per point */
        pm_message("Warning: aspect ratio not 1:1") ;
    skipspecials() ;
    while (flagbyte != 245) {           /* not at postamble */
        integer cheight, cwidth ;
        integer xoffs=0, yoffs=0;
        FILE *ofp;

        bmx=set_bmx;
        bmy=set_bmy;
        dynf = (flagbyte>>4) ;          /* get dynamic packing value */
        flagbyte &= 15 ;
        turnon = (flagbyte >= 8) ;      /* black or white initially? */
        if (turnon) flagbyte &= 7 ;     /* long or short form */
        if (flagbyte == 7) {            /* long form preamble */
            integer packetlength = get32() ;    /* character packet length */
            car = get32() ;         /* character number */
            endofpacket = packetlength + pktopbm_pkloc;
                /* calculate end of packet */
            if ((car >= MAXPKCHAR) || !filename[car]) {
                ignorechar(car, endofpacket);
                continue;
            }
            dprintf0 ("flagbyte7\n");
            dprintf ("car: %d\n", car);
            get32() ;               /* ignore tfmwidth */
            x=get32() ;             /* ignore horiz escapement */
            x=get32() ;             /* ignore vert escapement */
            dprintf ("horiz esc %d\n", x);
            dprintf ("vert esc %d\n", x);
            cwidth = get32() ;          /* bounding box width */
            cheight = get32() ;         /* bounding box height */
            dprintf ("cwidth %d\n", cwidth);
            dprintf ("cheight %d\n", cheight);
            if (cwidth < 0 || cheight < 0 || 
                cwidth > 65535 || cheight > 65535) {
                ignorechar(car, endofpacket);
                continue;
            }
            xoffs= get32() ;              /* horiz offset */
            yoffs= get32() ;              /* vert offset */
            dprintf ("xoffs %d\n", xoffs);
            dprintf ("yoffs %d\n", yoffs);
        } else if (flagbyte > 3) {      /* extended short form */
            integer packetlength = ((flagbyte - 4)<<16) + get16() ;
            /* packet length */
            car = pktopbm_pkbyte() ;            /* char number */
            endofpacket = packetlength + pktopbm_pkloc ; 
                /* calculate end of packet */
            if ((car >= MAXPKCHAR) || !filename[car]) {
                ignorechar(car, endofpacket);
                continue;
            }
            dprintf0 ("flagbyte>3\n");
            dprintf ("car: %d\n", car);
            pktopbm_pkbyte() ;              /* ignore tfmwidth (3 bytes) */
            get16() ;               /* ignore tfmwidth (3 bytes) */
            get16() ;               /* ignore horiz escapement */
            cwidth = get16() ;          /* bounding box width */
            cheight = get16() ;         /* bounding box height */
            dprintf ("cwidth %d\n", cwidth);
            dprintf ("cheight %d\n", cheight);
            xoffs=get16();                         /* horiz offset */
            if (xoffs >= 32768)
                xoffs-= 65536;
            yoffs=get16();                         /* vert offset */
            if (yoffs >= 32768)
                yoffs-= 65536;
            dprintf ("xoffs %d\n", xoffs);
            dprintf ("yoffs %d\n", yoffs);
        } else {                    /* short form preamble */
            integer packetlength = (flagbyte<<8) + pktopbm_pkbyte() ;
            /* packet length */
            car = pktopbm_pkbyte() ;            /* char number */
            endofpacket = packetlength + pktopbm_pkloc ;    
                /* calculate end of packet */
            if ((car >= MAXPKCHAR) || !filename[car]) {
                ignorechar(car, endofpacket);
                continue;
            }
            dprintf0 ("flagbyte<=3\n");
            dprintf ("car: %d\n", car);
            pktopbm_pkbyte() ;          /* ignore tfmwidth (3 bytes) */
            get16() ;               /* ignore tfmwidth (3 bytes) */
            x = pktopbm_pkbyte() ;  /* ignore horiz escapement */
            dprintf ("horiz esc %d\n", x);
            cwidth = pktopbm_pkbyte() ;            /* bounding box width */
            cheight = pktopbm_pkbyte() ;           /* bounding box height */
            dprintf ("cwidth %d\n", cwidth);
            dprintf ("cheight %d\n", cheight);
            xoffs=pktopbm_pkbyte ();               /* horiz offset */
            if (xoffs >= 128)
                xoffs-=256;
            yoffs=pktopbm_pkbyte ();               /* vert offset */
            if (yoffs >= 128)
                yoffs-=256;
            dprintf ("xoffs %d\n", xoffs);
            dprintf ("yoffs %d\n", yoffs);
        }
        if (filename[car]) {
            if (!bmx) bmx= cwidth;
            if (!bmy) bmy= cheight;
            bitmap = pbm_allocarray(bmx, bmy) ;
            if (bitmap == NULL)
                pm_error("out of memory allocating bitmap") ;
        } else {
            ignorechar(car, endofpacket);
            continue;
        }
        bitweight = 0 ;
        for (i = 0 ; i < bmy ; i ++)           /* make it blank */
            for (j = 0 ; j < bmx ; j ++)
                bitmap[i][j]= PBM_WHITE;
        if (dynf == 14) {               /* bitmapped character */
            dprintf ("bmy: %d\n ", bmy);
            dprintf ("y: %d\n ", bmy-yoffs-1);
            for (i = 0 ; i < cheight ; i ++) {
                int yi= i+(bmy-yoffs-1);
                for (j = 0 ; j < cwidth ; j ++) {
                    int xj= j-xoffs;
                    if (getbit() && 0<=xj && xj<bmx && 0<=yi && yi<bmy)
                        bitmap[yi][xj] = PBM_BLACK ;
                }
            }
        } else {                    /* dynamically packed char */
            integer rowsleft = cheight ;
            integer hbit = cwidth ;
            integer rp = 0;
            repeatcount = 0 ;
            dprintf ("bmy: %d\n ", bmy);
            dprintf ("y: %d\n", cheight-rowsleft+(bmy-2*yoffs-1));
            while (rowsleft > 0) {
                integer count = pkpackednum() ; /* get current color count */
                while (count > 0) {
                    if (count < hbit) {     /* doesn't extend past row */
                        hbit -= count ;
                        while (count--)
                            row[rp++] = turnon ? PBM_BLACK : PBM_WHITE;
                    } else {                /* reaches end of row */
                        count -= hbit ;
                        while (hbit--)
                            row[rp++] = turnon ? PBM_BLACK : PBM_WHITE;
                        for (i = 0; i <= repeatcount; i++) {  /* fill row */
                            int yi= i+cheight-rowsleft-1;
                            if (0<=yi && yi < bmy)
                                for (j = 0; j < cwidth; j++) {
                                    int xj= j-xoffs;
                                    if (0<=xj && xj<bmx)
                                        bitmap[yi][xj] = row[j] ;
                                }
                        }
                        rowsleft -= repeatcount + 1;
                        repeatcount = rp = 0 ;
                        hbit = cwidth ;
                    }
                }
                turnon = !turnon ;
            }
            if (rowsleft != 0 || hbit != cwidth)
                pm_error("bad pk file (more bits than required)") ;
        }
        if (endofpacket != pktopbm_pkloc)
            pm_error("bad pk file (bad packet length)") ;

        ofp = pm_openw(filename[car]);
        filename[car] = NULL;
        pbm_writepbm(ofp, bitmap, bmx, bmy, 0) ;
        pbm_freearray(bitmap, bmy) ;
        pm_close(ofp) ;
        skipspecials() ;
    }
    while (! feof(pkfile)) pktopbm_pkbyte() ;       /* skip trailing junk */
    pm_close(pkfile);
    for (car = 0; car < MAXPKCHAR; car++)
        if (filename[car])
            pm_message("Warning: No character in position %d (file %s).",
                       car, filename[car]) ;
    pm_message("%d bytes read from packed file.", pktopbm_pkloc-1) ;
    return 0;
}
