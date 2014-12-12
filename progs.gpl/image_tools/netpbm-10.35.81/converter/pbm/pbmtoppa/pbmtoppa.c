/* pbmtoppa.c
 * program to print a 600 dpi PBM file on the HP DJ820Cse or the HP720 series.
 * Copyright (c) 1998 Tim Norman.  See LICENSE for details
 * 2-24-98
 */

#define _BSD_SOURCE
    /* This makes sure strcasecmp() is in string.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pbm.h"
#include "ppa.h"
#include "ppapbm.h"
#include "cutswath.h"

#include "defaults.h"

/* Paper sizes in 600ths of an inch. */

/* US is 8.5 in by 11 in */

#define USWIDTH  (5100)
#define USHEIGHT (6600)

/* A4 is 210 mm by 297 mm == 8.27 in by 11.69 in */

#define A4WIDTH  (4960)
#define A4HEIGHT (7016)

int Width;      /* width and height in 600ths of an inch */
int Height;
int Pwidth;     /* width in bytes */

#define MAX_LINES 300

ppa_stat printer;

static int 
print_pbm(FILE * const in) {

    char line[1024];
    pbm_stat pbm;
    int done_page, numpages = 0;
    ppa_sweep_data sweeps[2];
    int current_sweep, previous_sweep;

    ppa_init_job(&printer);

    while(make_pbm_stat(&pbm, in)) {
        if (pbm.width != Width || pbm.height != Height) 
            pm_error("print_pbm(): Input image is not the size "
                    "of a page for Page %d.  "
                    "The input is %dW x %dH, "
                     "while a page is %dW x %dH pixels.\n"
                    "Page size is controlled by options and the configuration "
                    "file.",
                     numpages+1, pbm.width, pbm.height, Width, Height);

        ppa_init_page(&printer);
        ppa_load_page(&printer);

        sweeps[0].direction = right_to_left;
        sweeps[0].next=&sweeps[1];
        sweeps[1].direction = left_to_right;
        sweeps[1].next=&sweeps[0];

        current_sweep=0;
        previous_sweep=-1;

        done_page=0;
        while(!done_page) {
            int rc;
            rc = cut_pbm_swath(&pbm, &printer, MAX_LINES,
                               &sweeps[current_sweep]);
            switch(rc) {
            case 0:
                pm_error("print_pbm(): cut_pbm_swath() failed.");
                break;
            case 1:
                done_page=1;
                break;
            case 2:
                if (previous_sweep >= 0) {
                    ppa_print_sweep(&printer, &sweeps[previous_sweep]);
                    free(sweeps[previous_sweep].image_data);
                    free(sweeps[previous_sweep].nozzle_data);
                }
                previous_sweep=current_sweep;
                current_sweep= current_sweep==0 ? 1 : 0;
                break;
            default:
                pm_error("print_pbm(): unknown return code from "
                         "cut_pbm_swath()");
            }
        }
        if (previous_sweep >= 0) {
            sweeps[previous_sweep].next=NULL;
            ppa_print_sweep(&printer,&sweeps[previous_sweep]);
        }

        free(sweeps[0].image_data);
        free(sweeps[0].nozzle_data);
        free(sweeps[1].image_data);
        free(sweeps[1].nozzle_data);

        ppa_eject_page(&printer);

        /* eat any remaining whitespace */
        if(pbm.version==P1)
            fgets (line, 1024, in);

        ++numpages;
    }

    if (numpages == 0)
        pm_error("No pages printed!");

    ppa_end_print(&printer);

    fclose (pbm.fptr);
    fclose (printer.fptr);

    return 0;
}



static void 
set_printer_specific_defaults()
{
    switch(printer.version)
    {
    case HP720:
        printer.marg_diff     = HP720_MARG_DIFF;
        printer.bufsize       = HP720_BUFSIZE;

        printer.x_offset      = HP720_X_OFFSET;
        printer.y_offset      = HP720_Y_OFFSET;
        printer.top_margin    = HP720_TOP_MARGIN;
        printer.left_margin   = HP720_LEFT_MARGIN;
        printer.right_margin  = HP720_RIGHT_MARGIN;
        printer.bottom_margin = HP720_BOTTOM_MARGIN;
        break;
    case HP820:
        printer.marg_diff     = HP820_MARG_DIFF;
        printer.bufsize       = HP820_BUFSIZE;

        printer.x_offset      = HP820_X_OFFSET;
        printer.y_offset      = HP820_Y_OFFSET;
        printer.top_margin    = HP820_TOP_MARGIN;
        printer.left_margin   = HP820_LEFT_MARGIN;
        printer.right_margin  = HP820_RIGHT_MARGIN;
        printer.bottom_margin = HP820_BOTTOM_MARGIN;
        break;
    case HP1000:
        printer.marg_diff     = HP1000_MARG_DIFF;
        printer.bufsize       = HP1000_BUFSIZE;

        printer.x_offset      = HP1000_X_OFFSET;
        printer.y_offset      = HP1000_Y_OFFSET;
        printer.top_margin    = HP1000_TOP_MARGIN;
        printer.left_margin   = HP1000_LEFT_MARGIN;
        printer.right_margin  = HP1000_RIGHT_MARGIN;
        printer.bottom_margin = HP1000_BOTTOM_MARGIN;
        break;
    default:
        pm_error("set_printer_defaults(): unknown printer version");
    }
}



static void 
show_usage(const char* const prog)
{
    printf("usage: %s [ options ] [ <infile> [ <outfile> ] ]\n\n",prog);
    printf("  Prints a pbm- or pbmraw-format <infile> to HP720/820/1000-format <outfile>.\n\n");
    printf("    -v <version>    printer version (720, 820, or 1000)\n");
    printf("    -x <xoff>       vertical offset adjustment in 1\"/600\n");
    printf("    -y <yoff>       horizontal offset adjustment in 1\"/600\n");
    printf("    -t <topmarg>    top margin in 1\"/600    (default: 150 = 0.25\")\n");
    printf("    -l <leftmarg>   left margin in 1\"/600   (default: 150 = 0.25\")\n");
    printf("    -r <rightmarg>  right margin in 1\"/600  (default: 150 = 0.25\")\n");
    printf("    -b <botmarg>    bottom margin in 1\"/600 (default: 150 = 0.25\")\n");
    printf("    -s <paper>      paper size (us, a4, default: us)\n");
    printf("    -f <cfgfile>    read <cfgfile> as parameters\n\n");
    printf("  The -x and -y options accumulate.  The -v option resets the horizontal and\n");
    printf("  vertical adjustments to an internal default.  <infile> and <outfile> default\n");
    printf("  to stdin and stdout.  '-' is a synonym for stdin and stdout.\n\n");
    printf("  Configuration files specified with the '-f' parameter have the following\n  format:\n\n");
    printf("    # Comment\n");
    printf("    <key1> <value1>\n");
    printf("    <key2> <value2>\n");
    printf("    [etc.]\n\n");
    printf("  Valid keys are 'version', 'xoffset', 'yoffset', 'topmargin', 'leftmargin',\n");
    printf("  'rightmargin', 'bottommargin', 'papersize', or any non-null truncated\n");
    printf("  version of these words.  Valid values are the same as with the corresponding\n");
    printf("  command-line parameters.  Parameters in the configuration file act as though\n");
    printf("  the corresponding parameters were substituted, in order, for the '-f'\n");
    printf("  parameter which specified the file.\n\n");
    printf("  The file /etc/pbmtoppa.conf, if it exists, is processed as a configuration\n");
    printf("  file before any command-line parameters are processed.\n\n");
}



static void 
parm_version(char* arg)
{
    if(!strcasecmp(arg,"hp720") || !strcmp(arg,"720"))
        printer.version=HP720;
    else if(!strcasecmp(arg,"hp820") || !strcmp(arg,"820"))
        printer.version=HP820;
    else if(!strcasecmp(arg,"hp1000") || !strcmp(arg,"1000"))
        printer.version=HP1000;
    else
        pm_error("parm_version(): unknown printer version '%s'",arg);
    set_printer_specific_defaults();
}



static void 
parm_iversion(int arg)
{
    switch(arg)
    {
    case 720:
        printer.version=HP720;
        break;
    case 820:
        printer.version=HP820;
        break;
    case 1000:
        printer.version=HP1000;
        break;
    default:
        pm_error("parm_iversion(): unknown printer version '%d'", arg);
    }
    set_printer_specific_defaults();
}



static void 
dump_config()
{
    printf("version:  ");
    switch(printer.version)
    {
    case HP710:  printf("HP710\n");  break;
    case HP720:  printf("HP720\n");  break;
    case HP820:  printf("HP820\n");  break;
    case HP1000: printf("HP1000\n"); break;
    }
    printf("x-offset: %d\ny-offset: %d\nmargins:\n top:    %d\n"
           " left:   %d\n right:  %d\n bottom: %d\n",printer.x_offset,
           printer.y_offset,printer.top_margin,printer.left_margin,
           printer.right_margin,printer.bottom_margin);
    exit(0);
}



static void 
read_config_file(const char* const fname)
{
    FILE* cfgfile=fopen(fname,"r");
    char line[1024],key[14],buf[10];
    int len,value,lineno=1;

    if(!cfgfile)
        pm_error("read_config_file(): couldn't open file '%s'", fname);

    while(fgets(line,1024,cfgfile))
    {
        if(strchr(line,'#'))
            *strchr(line,'#')=0;
        switch(sscanf(line,"%13s%9s",key,buf))
        {
        case 2:
            value=atoi(buf);
            len=strlen(key);
            if(!strncmp(key,"version",len))
                parm_iversion(value);
            else if(!strncmp(key,"xoffset",len))
                printer.x_offset=value;
            else if(!strncmp(key,"yoffset",len))
                printer.y_offset=value;
            else if(!strncmp(key,"topmargin",len))
                printer.top_margin=value;
            else if(!strncmp(key,"leftmargin",len))
                printer.left_margin=value;
            else if(!strncmp(key,"rightmargin",len))
                printer.right_margin=value;
            else if(!strncmp(key,"bottommargin",len))
                printer.bottom_margin=value;
            else if(!strncmp(key,"papersize",len))
            {
                if(!strcmp(buf,"us"))
                {
                    Width = USWIDTH;
                    Height = USHEIGHT;
                }
                else if(!strcmp(buf,"a4"))
                {
                    Width = A4WIDTH;
                    Height = A4HEIGHT;
                }
                else
                    pm_error("read_config_file(): unknown paper size %s", buf);
            }
            else if(!strcmp(key,"dump"))
                dump_config();
            else 
                pm_error("read_config_file(): unrecognized parameter '%s' "
                         "(line %d)", key, lineno);
        case EOF:
        case 0: 
            break;
        default:
            pm_error("read_config_file(): error parsing config file "
                     "(line %d)", lineno);
        }
        lineno++;
    }

    if(feof(cfgfile))
    {
        fclose(cfgfile);
        return;
    }

    pm_error("read_config_file(): error parsing config file");
}



const char* const defaultcfgfile="/etc/pbmtoppa.conf";



int 
main(int argc, char *argv[]) {

    int argn;
    int got_in=0, got_out=0, do_continue=1;
    FILE *in=stdin, *out=stdout;
    struct stat tmpstat;

    pbm_init(&argc, argv);

    printer.version       = DEFAULT_PRINTER;
    printer.x_offset      = DEFAULT_X_OFFSET;
    printer.y_offset      = DEFAULT_Y_OFFSET;
    printer.top_margin    = DEFAULT_TOP_MARGIN;
    printer.left_margin   = DEFAULT_LEFT_MARGIN;
    printer.right_margin  = DEFAULT_RIGHT_MARGIN;
    printer.bottom_margin = DEFAULT_BOTTOM_MARGIN;
    printer.DPI           = DEFAULT_DPI;
    Width = USWIDTH;
    Height = USHEIGHT;
    set_printer_specific_defaults();

    if(!stat(defaultcfgfile,&tmpstat))
        read_config_file(defaultcfgfile);

    for(argn=1; argn<argc; argn++)
    {
        if(!strcmp(argv[argn],"-h"))
        {
            show_usage(*argv);
            return 0;
        }
        else if(!strcmp(argv[argn],"-d"))
            dump_config();
        else if(argn+1<argc)
        {
            do_continue=1;
            if(!strcmp(argv[argn],"-v"))
                parm_version(argv[++argn]);
            else if(!strcmp(argv[argn],"-x"))
                printer.x_offset+=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-y"))
                printer.y_offset+=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-t"))
                printer.top_margin=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-l"))
                printer.left_margin=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-r"))
                printer.right_margin=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-b"))
                printer.bottom_margin=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-d"))
                printer.DPI=atoi(argv[++argn]);
            else if(!strcmp(argv[argn],"-s"))
            {
                argn++;
                if(!strcmp(argv[argn],"us"))
                {
                    Width = USWIDTH;
                    Height = USHEIGHT;
                }
                else if(!strcmp(argv[argn],"a4"))
                {
                    Width = A4WIDTH;
                    Height = A4HEIGHT;
                }
                else
                    pm_error("unknown paper size %s",argv[argn]);
            }
            else if(!strcmp(argv[argn],"-f"))
                read_config_file(argv[++argn]);
            else do_continue=0;
            if(do_continue) continue;
        }

        if(!got_in)
        {
            if (strcmp (argv[argn], "-") == 0)
                in = stdin;
            else if ((in = fopen (argv[argn], "rb")) == NULL)
                pm_error("main(): couldn't open file '%s'", 
                         argv[argn]);
            got_in=1;
        }
        else if(!got_out)
        {
            if (strcmp (argv[argn], "-") == 0)
                out = stdout;
            else if ((out = fopen (argv[argn], "wb")) == NULL)
                pm_error("main(): couldn't open file '%s'", 
                         argv[argn]);
            got_out=1;
        }
        else
            pm_error("main(): unrecognized parameter '%s'", argv[argn]);
    }

    Pwidth=(Width+7)/8;
    printer.fptr=out;

    return print_pbm (in);
}

