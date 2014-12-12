/* pbmtonokia.c - convert a portable bitmap to Nokia Smart Messaging
   Formats (NOL, NGG, HEX)

** Copyright (C)2001 OMS Open Media System GmbH, Tim Rühsen
** <tim.ruehsen@openmediasystem.de>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.

History
  07.06.2001 Created
  20.11.2001 Handle Picture Messages
             new option -txt to embed text into Picture Messages
             new option -net to specify operator network code for 
                Nokia Operator Logos

Notes:
  - limited to rows <= 255 and columns <= 255
  - limited to b/w graphics, not animated

Testing:
  Testing was done with SwissCom SMSC (Switzerland) and IC3S SMSC (Germany).
  The data was send with EMI/UCP protocol over TCP/IP.

  - 7.6.2001: tested with Nokia 3210: 72x14 Operator Logo
  - 7.6.2001: tested with Nokia 6210: 72x14 Operator Logo and 
              72x14 Group Graphic

Todo:
  - more testing
  - sendsms compatibility ?
  - are -fmt NOL and -fmt NGG working ok?  */

#define _BSD_SOURCE    /* Make sure strcasecmp() is in string.h */
#include <string.h>

#include "nstring.h"
#include "pbm.h"

#define FMT_HEX_NOL   1
#define FMT_HEX_NGG   2
#define FMT_HEX_NPM   3
#define FMT_NOL       4
#define FMT_NGG       5

static void 
usage(char *myname)
{
    pm_message("Copyright (C)2001 OMS GmbH");
    pm_message("Contact: Tim Ruehsen <tim.ruehsen@openmediasystem.de>\n");
    pm_usage("[options] [pbmfile]\n"
             "  Options:\n"
             "    -fmt <HEX_NOL|HEX_NGG|HEX_NPM|NOL|NGG>  "
             "Output format (default=HEX_NOL)\n"
             "    -net <network code>                     "
             "Network code for NOL operator logos\n"
             "    -txt <text message>                     "
             "Text for NPM picture messages\n");

    exit(1);
}

int 
main(int argc, char *argv[])
{
    FILE    *fp;
    bit    **image;
    unsigned int    c;
    int    argpos, output=FMT_HEX_NOL, rows, cols, row, col, p, it, len;
    char    header[32], *myname;
    char    network_code[6+1];
    char    *text=NULL;

    if ((myname=strrchr(argv[0],'/'))!=NULL) myname++; else myname=argv[0];

    pbm_init(&argc, argv);

    strcpy(network_code, "62F210"); /* default is German D1 net */

    for(argpos=1;argpos<argc;argpos++) {
        if (argv[argpos][0]=='-') {
            if (argv[argpos][1]=='-') {
                if (argc>argpos+1 && ISDIGIT(argv[argpos+1][0]))
                    {argpos++;break;}
            } else if (STREQ(argv[argpos],"-fmt") && argc>argpos+1) {
                ++argpos;
                if (!strcasecmp(argv[argpos],"HEX_NOL")) output=FMT_HEX_NOL;
                else if (!strcasecmp(argv[argpos],"HEX_NGG")) 
                    output=FMT_HEX_NGG;
                else if (!strcasecmp(argv[argpos],"HEX_NPM")) 
                    output=FMT_HEX_NPM;
                else if (!strcasecmp(argv[argpos],"NOL")) output=FMT_NOL;
                else if (!strcasecmp(argv[argpos],"NGG")) output=FMT_NGG;
                else usage(myname);
            } else if (STREQ(argv[argpos],"-net") && argc>argpos+1) {
                char * const network_code_arg=argv[++argpos];
                unsigned int it;
                len=strlen(network_code_arg);
                if (len!=6) 
                    pm_error("Network code must be 6 hex-digits long");
                for (it=0;it<strlen(network_code_arg);it++) {
                    if (!ISXDIGIT(network_code_arg[it])) 
                        pm_error("Network code must contain hex-digits only");
                    network_code[it]=TOUPPER(network_code_arg[it]);
                }
                network_code[it] = '\0';
            } else if (STREQ(argv[argpos],"-txt") && argc>argpos+1) {
                text=argv[++argpos];
            }
            else usage(myname);
        } else break;
    }

    if (argpos==argc) {
        image = pbm_readpbm(stdin, &cols, &rows);
    } else {
        fp=pm_openr(argv[argpos]);
        image = pbm_readpbm(fp, &cols, &rows);
        pm_close(fp);
    }

    memset(header,0,sizeof(header));

    switch (output) {
    case FMT_HEX_NOL:
        /* header */
        printf("06050415820000%s00%02X%02X01",network_code,cols,rows);

        /* image */
        for (row=0;row<rows;row++) {
            for (p=c=col=0;col<cols;col++) {
                if (image[row][col]==PBM_BLACK) c|=0x80>>p;
                if (++p==8) {
                    printf("%02X",c);
                    p=c=0;
                }
            }
            if (p) printf("%02X",c);
        }
        break;
    case FMT_HEX_NGG:
        /* header */
        printf("0605041583000000%02X%02X01",cols,rows);

        /* image */
        for (row=0;row<rows;row++) {
            for (p=c=col=0;col<cols;col++) {
                if (image[row][col]==PBM_BLACK) c|=0x80>>p;
                if (++p==8) {
                    printf("%02X",c);
                    p=c=0;
                }
            }
            if (p) printf("%02X",c);
        }
        break;
    case FMT_HEX_NPM:
        /* header */
        printf("060504158A0000");

        /* text */
        if (text!=NULL) {
            printf("00%04X",(len=strlen(text)));
            for (it=0;it<len;it++) printf("%02X",text[it]);
        }

        /* image */
        printf("02%04X00%02X%02X01",(cols*rows)/8+4,cols,rows);
        for (row=0;row<rows;row++) {
            for (p=c=col=0;col<cols;col++) {
                if (image[row][col]==PBM_BLACK) c|=0x80>>p;
                if (++p==8) {
                    printf("%02X",c);
                    p=c=0;
                }
            }
            if (p) printf("%02X",c);
        }
        break;
    case FMT_NOL:
        /* header - this is a hack */
        header[0]='N';
        header[1]='O';
        header[2]='L';
        header[4]=header[7]=header[8]=header[14]=header[16]=1;
        header[6]=4;
        header[10]=cols;
        header[12]=rows;
        header[18]=0x53;
        fwrite(header,20,1,stdout);

        /* image */
        for (row=0;row<rows;row++) {
            for (p=c=col=0;col<cols;col++) {
                if (image[row][col]==PBM_BLACK) putchar('1');
                else putchar('0');
            }
        }
        break;
    case FMT_NGG:
        /* header - this is a hack */
        header[0]='N';
        header[1]='G';
        header[2]='G';
        header[4]=header[10]=header[12]=1;
        header[6]=cols;
        header[8]=rows;
        header[14]=0x4a;
        fwrite(header,16,1,stdout);

        /* image */
        for (row=0;row<rows;row++) {
            for (p=c=col=0;col<cols;col++) {
                if (image[row][col]==PBM_BLACK) putchar('1');
                else putchar('0');
            }
        }
        break;
    default:
        pm_error("Output format %d not implemented!\n"
                 "Contact Tim Ruehsen <tim.ruehsen@openmediasystem.de>\n",
                 output);
        return 1;
    }
    return 0;
}

