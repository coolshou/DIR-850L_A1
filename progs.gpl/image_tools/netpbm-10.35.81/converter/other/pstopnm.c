/*----------------------------------------------------------------------------
                                 pstopnm
------------------------------------------------------------------------------
  Use Ghostscript to convert a Postscript file into a PBM, PGM, or PNM
  file.

  Implementation note: This program feeds the input file to Ghostcript
  directly (with possible statements preceding it), and uses
  Ghostscript's PNM output device drivers.  As an alternative,
  Ghostscript also comes with the Postscript program pstoppm.ps which
  we could run and it would read the input file and produce PNM
  output.  It isn't clear to me what pstoppm.ps adds to what you get
  from just feeding your input directly to Ghostscript as the main program.

-----------------------------------------------------------------------------*/

#define _BSD_SOURCE 1   /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  
    /* Make sure fdopen() is in stdio.h and strdup() is in string.h */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>  
#include <sys/stat.h>

#include "pnm.h"
#include "shhopt.h"
#include "nstring.h"

enum orientation {PORTRAIT, LANDSCAPE, UNSPECIFIED};
struct box {
    /* Description of a rectangle within an image; all coordinates 
       measured in points (1/72") with lower left corner of page being the 
       origin.
    */
    int llx;  /* lower left X coord */
        /* -1 for llx means whole box is undefined. */
    int lly;  /* lower left Y coord */
    int urx;  /* upper right X coord */
    int ury;  /* upper right Y coord */
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    unsigned int forceplain;
    struct box extract_box;
    unsigned int nocrop;
    unsigned int format_type;
    unsigned int verbose;
    float xborder;
    unsigned int xmax;
    unsigned int xsize;  /* zero means unspecified */
    float yborder;
    unsigned int ymax;
    unsigned int ysize;  /* zero means unspecified */
    unsigned int dpi;    /* zero means unspecified */
    enum orientation orientation;
    unsigned int goto_stdout;
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc( 100*sizeof( optEntry ) );
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int pbm_opt, pgm_opt, ppm_opt;
    unsigned int portrait_opt, landscape_opt;
    float llx, lly, urx, ury;
    unsigned int llxSpec, llySpec, urxSpec, urySpec;
    unsigned int xmaxSpec, ymaxSpec, xsizeSpec, ysizeSpec, dpiSpec;
    
    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "forceplain", OPT_FLAG,  NULL, &cmdlineP->forceplain,     0);
    OPTENT3(0, "llx",        OPT_FLOAT, &llx, &llxSpec,                  0);
    OPTENT3(0, "lly",        OPT_FLOAT, &lly, &llySpec,                  0);
    OPTENT3(0, "urx",        OPT_FLOAT, &urx, &urxSpec,                  0);
    OPTENT3(0, "ury",        OPT_FLOAT, &ury, &urySpec,                  0);
    OPTENT3(0, "nocrop",     OPT_FLAG,  NULL, &cmdlineP->nocrop,         0);
    OPTENT3(0, "pbm",        OPT_FLAG,  NULL, &pbm_opt,                  0);
    OPTENT3(0, "pgm",        OPT_FLAG,  NULL, &pgm_opt,                  0);
    OPTENT3(0, "ppm",        OPT_FLAG,  NULL, &ppm_opt,                  0);
    OPTENT3(0, "verbose",    OPT_FLAG,  NULL, &cmdlineP->verbose,        0);
    OPTENT3(0, "xborder",    OPT_FLOAT, &cmdlineP->xborder, NULL,        0);
    OPTENT3(0, "xmax",       OPT_UINT,  &cmdlineP->xmax, &xmaxSpec,      0);
    OPTENT3(0, "xsize",      OPT_UINT,  &cmdlineP->xsize, &xsizeSpec,    0);
    OPTENT3(0, "yborder",    OPT_FLOAT, &cmdlineP->yborder, NULL,        0);
    OPTENT3(0, "ymax",       OPT_UINT,  &cmdlineP->ymax, &ymaxSpec,      0);
    OPTENT3(0, "ysize",      OPT_UINT,  &cmdlineP->ysize, &ysizeSpec,    0);
    OPTENT3(0, "dpi",        OPT_UINT,  &cmdlineP->dpi, &dpiSpec,        0);
    OPTENT3(0, "portrait",   OPT_FLAG,  NULL, &portrait_opt,             0);
    OPTENT3(0, "landscape",  OPT_FLAG,  NULL, &landscape_opt,            0);
    OPTENT3(0, "stdout",     OPT_FLAG,  NULL, &cmdlineP->goto_stdout,    0);

    /* Set the defaults */
    cmdlineP->xborder = cmdlineP->yborder = 0.1;

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (xmaxSpec) {
        if (cmdlineP->xmax == 0)
            pm_error("zero is not a valid value for -xmax");
    } else
        cmdlineP->xmax = 612;

    if (ymaxSpec) {
        if (cmdlineP->ymax == 0)
            pm_error("zero is not a valid value for -ymax");
    } else 
        cmdlineP->ymax = 792;

    if (xsizeSpec) {
        if (cmdlineP->xsize == 0)
            pm_error("zero is not a valid value for -xsize");
    } else
        cmdlineP->xsize = 0;

    if (ysizeSpec) {
        if (cmdlineP->ysize == 0)
            pm_error("zero is not a valid value for -ysize");
    } else 
        cmdlineP->ysize = 0;

    if (portrait_opt & !landscape_opt)
        cmdlineP->orientation = PORTRAIT;
    else if (!portrait_opt & landscape_opt)
        cmdlineP->orientation = LANDSCAPE;
    else if (!portrait_opt & !landscape_opt)
        cmdlineP->orientation = UNSPECIFIED;
    else
        pm_error("Cannot specify both -portrait and -landscape options");

    if (pbm_opt)
        cmdlineP->format_type = PBM_TYPE;
    else if (pgm_opt)
        cmdlineP->format_type = PGM_TYPE;
    else if (ppm_opt)
        cmdlineP->format_type = PPM_TYPE;
    else
        cmdlineP->format_type = PPM_TYPE;

    /* If any one of the 4 bounding box coordinates is given on the
       command line, we default any of the 4 that aren't.  
    */
    if (llxSpec || llySpec || urxSpec || urySpec) {
        if (!llxSpec) cmdlineP->extract_box.llx = 72;
        else cmdlineP->extract_box.llx = llx * 72;
        if (!llySpec) cmdlineP->extract_box.lly = 72;
        else cmdlineP->extract_box.lly = lly * 72;
        if (!urxSpec) cmdlineP->extract_box.urx = 540;
        else cmdlineP->extract_box.urx = urx * 72;
        if (!urySpec) cmdlineP->extract_box.ury = 720;
        else cmdlineP->extract_box.ury = ury * 72;
    } else {
        cmdlineP->extract_box.llx = -1;
    }

    if (dpiSpec) {
        if (cmdlineP->dpi == 0)
            pm_error("Zero is not a valid value for -dpi");
    } else
        cmdlineP->dpi = 0;

    if (dpiSpec && xsizeSpec + ysizeSpec + xmaxSpec + ymaxSpec > 0)
        pm_error("You may not specify both size options and -dpi");

    if (argc-1 == 0)
        cmdlineP->input_filespec = "-";  /* stdin */
    else if (argc-1 == 1)
        cmdlineP->input_filespec = argv[1];
    else 
        pm_error("Too many arguments (%d).  "
                 "Only need one: the Postscript filespec", argc-1);
}



static void
add_ps_to_filespec(const char orig_filespec[], char ** const new_filespec_p,
                   const int verbose) {
/*----------------------------------------------------------------------------
   If orig_filespec[] does not name an existing file, but the same
   name with ".ps" added to the end does, return the name with the .ps
   attached.  Otherwise, just return orig_filespec[].

   Return the name in newly malloc'ed storage, pointed to by
   *new_filespec_p.
-----------------------------------------------------------------------------*/
    struct stat statbuf;
    int stat_rc;

    stat_rc = lstat(orig_filespec, &statbuf);
    
    if (stat_rc == 0)
        *new_filespec_p = strdup(orig_filespec);
    else {
        const char *filespec_plus_ps;

        asprintfN(&filespec_plus_ps, "%s.ps", orig_filespec);

        stat_rc = lstat(filespec_plus_ps, &statbuf);
        if (stat_rc == 0)
            *new_filespec_p = strdup(filespec_plus_ps);
        else
            *new_filespec_p = strdup(orig_filespec);
        strfree(filespec_plus_ps);
    }
    if (verbose)
        pm_message("Input file is %s", *new_filespec_p);
}



static void
computeSizeResFromSizeSpec(unsigned int   const requestedXsize,
                           unsigned int   const requestedYsize,
                           unsigned int   const imageWidth,
                           unsigned int   const imageHeight,
                           unsigned int * const xsizeP,
                           unsigned int * const ysizeP,
                           unsigned int * const xresP,
                           unsigned int * const yresP) {

    if (requestedXsize) {
        *xsizeP = requestedXsize;
        *xresP = (unsigned int) (requestedXsize * 72 / imageWidth + 0.5);
        if (!requestedYsize) {
            *yresP = *xresP;
            *ysizeP = (unsigned int) (imageHeight * (float)*yresP/72 + 0.5);
            }
        }

    if (requestedYsize) {
        *ysizeP = requestedYsize;
        *yresP = (unsigned int) (requestedYsize * 72 / imageHeight + 0.5);
        if (!requestedXsize) {
            *xresP = *yresP;
            *xsizeP = (unsigned int) (imageWidth * (float)*xresP/72 + 0.5);
        }
    } 
}



static void
computeSizeResBlind(unsigned int   const xmax,
                    unsigned int   const ymax,
                    unsigned int   const imageWidth,
                    unsigned int   const imageHeight,
                    bool           const nocrop,
                    unsigned int * const xsizeP,
                    unsigned int * const ysizeP,
                    unsigned int * const xresP,
                    unsigned int * const yresP) {

    *xresP = *yresP = MIN(xmax * 72 / imageWidth, 
                          ymax * 72 / imageHeight);
    
    if (nocrop) {
        *xsizeP = xmax;
        *ysizeP = ymax;
    } else {
        *xsizeP = (unsigned int) (imageWidth * (float)*xresP / 72 + 0.5);
        *ysizeP = (unsigned int) (imageHeight * (float)*yresP / 72 + 0.5);
    }
}



static void
compute_size_res(struct cmdlineInfo const cmdline, 
                 enum orientation   const orientation, 
                 struct box         const bordered_box,
                 unsigned int *     const xsizeP, 
                 unsigned int *     const ysizeP,
                 unsigned int *     const xresP, 
                 unsigned int *     const yresP) {
/*----------------------------------------------------------------------------
  Figure out how big the output image should be (return as
  *xsizeP and *ysizeP) and what output device resolution Ghostscript
  should assume (return as *xresP, *yresP).

  A resolution number is the number of pixels per inch that the a
  printer prints.  Since we're emulating a printed page with a PNM
  image, and a PNM image has no spatial dimension (you can't say how
  many inches wide a PNM image is), it's kind of confusing.  

  If the user doesn't select a resolution, we choose the resolution
  that causes the image to be a certain number of pixels, knowing how
  big (in inches) Ghostscript wants the printed picture to be.  For
  example, the part of the Postscript image we are going to print is 2
  inches wide.  We want the PNM image to be 1000 pixels wide.  So we
  tell Ghostscript that our horizontal output device resolution is 500
  pixels per inch.
  
  *xresP and *yresP are in dots per inch.
-----------------------------------------------------------------------------*/
    unsigned int sx, sy;
        /* The horizontal and vertical sizes of the input image, in points
           (1/72 inch)
        */

    if (orientation == LANDSCAPE) {
        sx = bordered_box.ury - bordered_box.lly;
        sy = bordered_box.urx - bordered_box.llx;
    } else {
        sx = bordered_box.urx - bordered_box.llx;
        sy = bordered_box.ury - bordered_box.lly;
    }

    if (cmdline.dpi) {
        /* User gave resolution; we figure out output image size */
        *xresP = *yresP = cmdline.dpi;
        *xsizeP = (int) (cmdline.dpi * sx / 72 + 0.5);
        *ysizeP = (int) (cmdline.dpi * sy / 72 + 0.5);
    } else  if (cmdline.xsize || cmdline.ysize)
        computeSizeResFromSizeSpec(cmdline.xsize, cmdline.ysize, sx, sy,
                                   xsizeP, ysizeP, xresP, yresP);
    else 
        computeSizeResBlind(cmdline.xmax, cmdline.ymax, sx, sy, cmdline.nocrop,
                            xsizeP, ysizeP, xresP, yresP);

    if (cmdline.verbose) {
        pm_message("output is %u pixels wide X %u pixels high",
                   *xsizeP, *ysizeP);
        pm_message("output device resolution is %u dpi horiz, %u dpi vert",
                   *xresP, *yresP);
    }
}



enum postscript_language {COMMON_POSTSCRIPT, ENCAPSULATED_POSTSCRIPT};

static enum postscript_language
language_declaration(const char input_filespec[], int const verbose) {
/*----------------------------------------------------------------------------
  Return the Postscript language in which the file declares it is written.
  (Except that if the file is on Standard Input or doesn't validly declare
  a languages, just say it is Common Postscript).
-----------------------------------------------------------------------------*/
    enum postscript_language language;

    if (STREQ(input_filespec, "-"))
        /* Can't read stdin, because we need it to remain positioned for the 
           Ghostscript interpreter to read it.
        */
        language = COMMON_POSTSCRIPT;
    else {
        FILE *infile;
        char line[80];

        infile = pm_openr(input_filespec);

        if (fgets(line, sizeof(line), infile) == NULL)
            language = COMMON_POSTSCRIPT;
        else {
            const char eps_header[] = " EPSF-";

            if (strstr(line, eps_header))
                language = ENCAPSULATED_POSTSCRIPT;
            else
                language = COMMON_POSTSCRIPT;
        }
        fclose(infile);
    }
    if (verbose)
        pm_message("language is %s",
                   language == ENCAPSULATED_POSTSCRIPT ?
                   "encapsulated postscript" :
                   "not encapsulated postscript");
    return language;
}



static struct box
compute_box_to_extract(struct box const cmdline_extract_box,
                       char       const input_filespec[],
                       bool       const verbose) {

    struct box retval;

    if (cmdline_extract_box.llx != -1)
        /* User told us what box to extract, so that's what we'll do */
        retval = cmdline_extract_box;
    else {
        /* Try to get the bounding box from the DSC %%BoundingBox
           statement (A Postscript comment) in the input.
        */
        struct box ps_bb;  /* Box described by %%BoundingBox stmt in input */

        if (STREQ(input_filespec, "-"))
            /* Can't read stdin, because we need it to remain
               positioned for the Ghostscript interpreter to read it.  
            */
            ps_bb.llx = -1;
        else {
            FILE *infile;
            int found_BB, eof;  /* logical */
            infile = pm_openr(input_filespec);
            
            found_BB = FALSE;
            eof = FALSE;
            while (!eof && !found_BB) {
                char line[200];
                
                if (fgets(line, sizeof(line), infile) == NULL)
                    eof = TRUE;
                else {
                    int rc;
                    rc = sscanf(line, "%%%%BoundingBox: %d %d %d %d",
                                &ps_bb.llx, &ps_bb.lly, 
                                &ps_bb.urx, &ps_bb.ury);
                    if (rc == 4) 
                        found_BB = TRUE;
                }
            }
            fclose(infile);

            if (!found_BB) {
                ps_bb.llx = -1;
                pm_message("Warning: no %%%%BoundingBox statement "
                           "in the input or command line.\n"
                           "Will use defaults");
            }
        }
        if (ps_bb.llx != -1) {
            if (verbose)
                pm_message("Using %%%%BoundingBox statement from input.");
            retval = ps_bb;
        } else { 
            /* Use the center of an 8.5" x 11" page with 1" border all around*/
            retval.llx = 72;
            retval.lly = 72;
            retval.urx = 540;
            retval.ury = 720;
        }
    }
    if (verbose)
        pm_message("Extracting the box ((%d,%d),(%d,%d))",
                   retval.llx, retval.lly, retval.urx, retval.ury);
    return retval;
}



static enum orientation
compute_orientation(struct cmdlineInfo const cmdline, 
                    struct box         const extract_box) {

    enum orientation retval;
    unsigned int const input_width  = extract_box.urx - extract_box.llx;
    unsigned int const input_height = extract_box.ury - extract_box.lly;

    if (cmdline.orientation != UNSPECIFIED)
        retval = cmdline.orientation;
    else {
        if ((!cmdline.xsize || !cmdline.ysize) &
            (cmdline.xsize || cmdline.ysize)) {
            /* User specified one output dimension, but not the other,
               so we can't use output dimensions to make the decision.  So
               just use the input dimensions.
            */
            if (input_height > input_width) retval = PORTRAIT;
            else retval = LANDSCAPE;
        } else {
            int output_width, output_height;
            if (cmdline.xsize) {
                /* He gave xsize and ysize, so that's the output size */
                output_width = cmdline.xsize;
                output_height = cmdline.ysize;
            } else {
                /* Well then we'll just use his (or default) xmax, ymax */
                output_width = cmdline.xmax;
                output_height = cmdline.ymax;
            }

            if (input_height > input_width && output_height > output_width)
                retval = PORTRAIT;
            else if (input_height < input_width && 
                     output_height < output_width)
                retval = PORTRAIT;
            else 
                retval = LANDSCAPE;
        }
    }
    return retval;
}



static struct box
add_borders(const struct box input_box, 
            const float xborder_scale, float yborder_scale,
            const int verbose) {
/*----------------------------------------------------------------------------
   Return a box which is 'input_box' plus some borders.

   Add left and right borders that are the fraction 'xborder_scale' of the
   width of the input box; likewise for top and bottom borders with 
   'yborder_scale'.
-----------------------------------------------------------------------------*/
    struct box retval;

    const int left_right_border_size = 
        (int) ((input_box.urx - input_box.llx) * xborder_scale + 0.5);
    const int top_bottom_border_size = 
        (int) ((input_box.ury - input_box.lly) * yborder_scale + 0.5);

    retval.llx = input_box.llx - left_right_border_size;
    retval.lly = input_box.lly - top_bottom_border_size;
    retval.urx = input_box.urx + left_right_border_size;
    retval.ury = input_box.ury + top_bottom_border_size;

    if (verbose)
        pm_message("With borders, extracted box is ((%d,%d),(%d,%d))",
                   retval.llx, retval.lly, retval.urx, retval.ury);

    return retval;
}



static const char *
compute_pstrans(const struct box box, const enum orientation orientation,
                const int xsize, const int ysize, 
                const int xres, const int yres) {

    const char * retval;

    if (orientation == PORTRAIT) {
        int llx, lly;
        llx = box.llx - (xsize * 72 / xres - (box.urx - box.llx)) / 2;
        lly = box.lly - (ysize * 72 / yres - (box.ury - box.lly)) / 2;
        asprintfN(&retval, "%d neg %d neg translate", llx, lly);
    } else {
        int llx, ury;
        llx = box.llx - (ysize * 72 / yres - (box.urx - box.llx)) / 2;
        ury = box.ury + (xsize * 72 / xres - (box.ury - box.lly)) / 2;
        asprintfN(&retval, "90 rotate %d neg %d neg translate", llx, ury);
    }

    if (retval == NULL)
        pm_error("Unable to allocate memory for pstrans");

    return retval;
}



static const char *
compute_outfile_arg(const struct cmdlineInfo cmdline) {

    const char *retval;  /* malloc'ed */

    if (cmdline.goto_stdout)
        retval = strdup("-");
    else if (STREQ(cmdline.input_filespec, "-"))
        retval = strdup("-");
    else {
        char * basename;
        const char * suffix;
        
        basename  = strdup(cmdline.input_filespec);
        if (strlen(basename) > 3 && 
            STREQ(basename+strlen(basename)-3, ".ps")) 
            /* The input filespec ends in ".ps".  Chop it off. */
            basename[strlen(basename)-3] = '\0';

        switch (cmdline.format_type) {
        case PBM_TYPE: suffix = "pbm"; break;
        case PGM_TYPE: suffix = "pgm"; break;
        case PPM_TYPE: suffix = "ppm"; break;
        default: pm_error("Internal error: invalid value for format_type: %d",
                          cmdline.format_type);
        }
        asprintfN(&retval, "%s%%03d.%s", basename, suffix);

        strfree(basename);
    }
    return(retval);
}



static const char *
compute_gs_device(const int format_type, const int forceplain) {

    const char * basetype;
    const char * retval;

    switch (format_type) {
    case PBM_TYPE: basetype = "pbm"; break;
    case PGM_TYPE: basetype = "pgm"; break;
    case PPM_TYPE: basetype = "ppm"; break;
    default: pm_error("Internal error: invalid value format_type");
    }
    if (forceplain)
        retval = strdup(basetype);
    else
        asprintfN(&retval, "%sraw", basetype);

    if (retval == NULL)
        pm_error("Unable to allocate memory for gs device");

    return(retval);
}



static void
findGhostscriptProg(const char ** const retvalP) {
    
    *retvalP = NULL;  /* initial assumption */
    if (getenv("GHOSTSCRIPT"))
        *retvalP = strdup(getenv("GHOSTSCRIPT"));
    if (*retvalP == NULL) {
        if (getenv("PATH") != NULL) {
            char *pathwork;  /* malloc'ed */
            const char * candidate;

            pathwork = strdup(getenv("PATH"));
            
            candidate = strtok(pathwork, ":");

            *retvalP = NULL;
            while (!*retvalP && candidate) {
                struct stat statbuf;
                const char * filename;
                int rc;

                asprintfN(&filename, "%s/gs", candidate);
                rc = stat(filename, &statbuf);
                if (rc == 0) {
                    if (S_ISREG(statbuf.st_mode))
                        *retvalP = strdup(filename);
                } else if (errno != ENOENT)
                    pm_error("Error looking for Ghostscript program.  "
                             "stat(\"%s\") returns errno %d (%s)",
                             filename, errno, strerror(errno));
                strfree(filename);

                candidate = strtok(NULL, ":");
            }
            free(pathwork);
        }
    }
    if (*retvalP == NULL)
        *retvalP = strdup("/usr/bin/gs");
}



static void
execGhostscript(int const inputPipeFd,
                const char ghostscript_device[],
                const char outfile_arg[], 
                int const xsize, int const ysize, 
                int const xres, int const yres,
                const char input_filespec[], int const verbose) {
    
    const char *arg0;
    const char *ghostscriptProg;
    const char *deviceopt;
    const char *outfileopt;
    const char *gopt;
    const char *ropt;
    int rc;

    findGhostscriptProg(&ghostscriptProg);

    /* Put the input pipe on Standard Input */
    rc = dup2(inputPipeFd, STDIN_FILENO);
    close(inputPipeFd);

    asprintfN(&arg0, "gs");
    asprintfN(&deviceopt, "-sDEVICE=%s", ghostscript_device);
    asprintfN(&outfileopt, "-sOutputFile=%s", outfile_arg);
    asprintfN(&gopt, "-g%dx%d", xsize, ysize);
    asprintfN(&ropt, "-r%dx%d", xres, yres);

    /* -dSAFER causes Postscript to disable %pipe and file operations,
       which are almost certainly not needed here.  This prevents our
       Postscript program from doing crazy unexpected things, possibly
       as a result of a malicious booby trapping of our Postscript file.
    */

    if (verbose) {
        pm_message("execing '%s' with args '%s' (arg 0), "
                   "'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s'",
                   ghostscriptProg, arg0,
                   deviceopt, outfileopt, gopt, ropt, "-q", "-dNOPAUSE", 
                   "-dSAFER", "-");
    }

    execl(ghostscriptProg, arg0, deviceopt, outfileopt, gopt, ropt, "-q",
          "-dNOPAUSE", "-dSAFER", "-", NULL);
    
    pm_error("execl() of Ghostscript ('%s') failed, errno=%d (%s)",
             ghostscriptProg, errno, strerror(errno));
}




static void
execute_ghostscript(const char pstrans[], const char ghostscript_device[],
                    const char outfile_arg[], 
                    const int xsize, const int ysize, 
                    const int xres, const int yres,
                    const char input_filespec[], 
                    const enum postscript_language language,
                    const int verbose) {

    int gs_exit;  /* wait4 exit code from Ghostscript */
    FILE *gs;  /* Pipe to Ghostscript's standard input */
    FILE *infile;
    int rc;
    int eof;  /* End of file on input */
    int pipefd[2];

    if (strlen(outfile_arg) > 80)
        pm_error("output file spec too long.");
    
    rc = pipe(pipefd);
    if (rc < 0)
        pm_error("Unable to create pipe to talk to Ghostscript process.  "
                 "errno = %d (%s)", errno, strerror(errno));
    
    rc = fork();
    if (rc < 0)
        pm_error("Unable to fork a Ghostscript process.  errno=%d (%s)",
                 errno, strerror(errno));
    else if (rc == 0) {
        /* Child process */
        close(pipefd[1]);
        execGhostscript(pipefd[0], ghostscript_device, outfile_arg,
                        xsize, ysize, xres, yres, input_filespec, verbose);
    } else {
        pid_t const ghostscriptPid = rc;
        int const pipeToGhostscriptFd = pipefd[1];
        /* parent process */
        close(pipefd[0]);

        gs = fdopen(pipeToGhostscriptFd, "w");
        if (gs == NULL) 
            pm_error("Unable to open stream on pipe to Ghostscript process.");
    
        infile = pm_openr(input_filespec);
        /*
          In encapsulated Postscript, we the encapsulator are supposed to
          handle showing the page (which we do by passing a showpage
          statement to Ghostscript).  Any showpage statement in the 
          input must be defined to have no effect.
          
          See "Enscapsulated PostScript Format File Specification",
          v. 3.0, 1 May 1992, in particular Example 2, p. 21.  I found
          it at 
          http://partners.adobe.com/asn/developer/pdfs/tn/5002.EPSF_Spec.pdf
          The example given is a much fancier solution than we need
          here, I think, so I boiled it down a bit.  JM 
        */
        if (language == ENCAPSULATED_POSTSCRIPT)
            fprintf(gs, "\n/b4_Inc_state save def /showpage { } def\n");
 
        if (verbose) 
            pm_message("Postscript prefix command: '%s'", pstrans);

        fprintf(gs, "%s\n", pstrans);

        /* If our child dies, it closes the pipe and when we next write to it,
           we get a SIGPIPE.  We must survive that signal in order to report
           on the fate of the child.  So we ignore SIGPIPE:
        */
        signal(SIGPIPE, SIG_IGN);

        eof = FALSE;
        while (!eof) {
            char buffer[4096];
            int bytes_read;
            
            bytes_read = fread(buffer, 1, sizeof(buffer), infile);
            if (bytes_read == 0) 
                eof = TRUE;
            else 
                fwrite(buffer, 1, bytes_read, gs);
        }
        pm_close(infile);

        if (language == ENCAPSULATED_POSTSCRIPT)
            fprintf(gs, "\nb4_Inc_state restore showpage\n");

        fclose(gs);
        
        waitpid(ghostscriptPid, &gs_exit, 0);
        if (rc < 0)
            pm_error("Wait for Ghostscript process to terminated failed.  "
                     "errno = %d (%s)", errno, strerror(errno));

        if (gs_exit != 0) {
            if (WIFEXITED(gs_exit))
                pm_error("Ghostscript failed.  Exit code=%d\n", 
                         WEXITSTATUS(gs_exit));
            else if (WIFSIGNALED(gs_exit))
                pm_error("Ghostscript process died due to a signal %d.",
                         WTERMSIG(gs_exit));
            else 
                pm_error("Ghostscript process died with exit code %d", 
                         gs_exit);
        }
    }
}



int
main(int argc, char **argv) {

    struct cmdlineInfo cmdline;
    char *input_filespec;  /* malloc'ed */
        /* The file specification of our Postscript input file */
    unsigned int xres, yres;    /* Resolution in pixels per inch */
    unsigned int xsize, ysize;  /* output image size in pixels */
    struct box extract_box;
        /* coordinates of the box within the input we are to extract; i.e.
           that will become the output. 
           */
    struct box bordered_box;
        /* Same as above, but expanded to include borders */

    enum postscript_language language;
    enum orientation orientation;
    const char *ghostscript_device;
    const char *outfile_arg;
    const char *pstrans;

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    add_ps_to_filespec(cmdline.input_filespec, &input_filespec,
                       cmdline.verbose);

    extract_box = compute_box_to_extract(cmdline.extract_box, input_filespec, 
                                         cmdline.verbose);

    language = language_declaration(input_filespec, cmdline.verbose);
    
    orientation = compute_orientation(cmdline, extract_box);

    bordered_box = add_borders(extract_box, cmdline.xborder, cmdline.yborder,
                               cmdline.verbose);

    compute_size_res(cmdline, orientation, bordered_box, 
                     &xsize, &ysize, &xres, &yres);
    
    pstrans = compute_pstrans(bordered_box, orientation,
                              xsize, ysize, xres, yres);

    outfile_arg = compute_outfile_arg(cmdline);

    ghostscript_device = 
        compute_gs_device(cmdline.format_type, cmdline.forceplain);
    
    pm_message("Writing %s file", ghostscript_device);
    
    execute_ghostscript(pstrans, ghostscript_device, outfile_arg, 
                        xsize, ysize, xres, yres, input_filespec,
                        language, cmdline.verbose);

    strfree(ghostscript_device);
    strfree(outfile_arg);
    strfree(pstrans);
    
    return 0;
}


