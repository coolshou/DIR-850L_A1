#define _XOPEN_SOURCE    /* Make sure M_PI is in math.h */
#define _BSD_SOURCE      /* Make sure strdup is in string.h */

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "pm_c_util.h"
#include "shhopt.h"
#include "mallocvar.h"
#include "nstring.h"
#include "ppm.h"
#include "ppmdraw.h"
#include "ppmdfont.h"

static bool verbose;


static double
sindeg(double const angle) {

    return sin((double)angle / 360 * 2 * M_PI);
}


static double
cosdeg(double const angle) {

    return cos((double)angle / 360 * 2 * M_PI);
}


struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilename;  /* '-' if stdin */
    const char * scriptfile;     /* NULL means none. '-' means stdin */
    const char * script;         /* NULL means none */
    unsigned int verbose;
};



static void
parseCommandLine (int argc, char ** argv,
                  struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int scriptSpec, scriptfileSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "script",      OPT_STRING,    &cmdlineP->script,
            &scriptSpec,      0);
    OPTENT3(0, "scriptfile",  OPT_STRING,    &cmdlineP->scriptfile,
            &scriptfileSpec,  0);
    OPTENT3(0, "verbose",     OPT_FLAG,      NULL,
            &cmdlineP->verbose, 0);


    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */
    
    if (!scriptSpec && !scriptfileSpec)
        pm_error("You must specify either -script or -scriptfile");

    if (scriptSpec && scriptfileSpec)
        pm_error("You may not specify both -script and -scriptfile");

    if (!scriptSpec)
        cmdlineP->script = NULL;
    if (!scriptfileSpec)
        cmdlineP->scriptfile = NULL;

    if (argc-1 < 1) {
        if (cmdlineP->scriptfile && strcmp(cmdlineP->scriptfile, "-") == 0)
            pm_error("You can't specify Standard Input for both the "
                     "input image and the script file");
        else
            cmdlineP->inputFilename = "-";
    }
    else if (argc-1 == 1)
        cmdlineP->inputFilename = argv[1];
    else
        pm_error("Program takes at most one argument:  input file name");
}


struct pos {
    unsigned int x;
    unsigned int y;
};

struct drawState {
    struct pos currentPos;
    pixel color;
};



static void
initDrawState(struct drawState * const drawStateP,
              pixval             const maxval) {

    drawStateP->currentPos.x = 0;
    drawStateP->currentPos.y = 0;
    PPM_ASSIGN(drawStateP->color, maxval, maxval, maxval);
}



static void
readScriptFile(const char *  const scriptFileName,
               const char ** const scriptP) {

    FILE * scriptFileP;
    char * script;
    size_t scriptAllocation;
    size_t bytesReadSoFar;

    scriptAllocation = 4096;

    MALLOCARRAY(script, scriptAllocation);

    if (script == NULL)
        pm_error("out of memory reading script from file");

    scriptFileP = pm_openr(scriptFileName);

    bytesReadSoFar = 0;
    while (!feof(scriptFileP)) {
        size_t bytesRead;

        if (scriptAllocation - bytesReadSoFar < 2) {
            scriptAllocation += 4096;
            REALLOCARRAY(script, scriptAllocation);
            if (script == NULL)
                pm_error("out of memory reading script from file");
        }
        bytesRead = fread(script + bytesReadSoFar, 1,
                          scriptAllocation - bytesReadSoFar - 1, scriptFileP);
        bytesReadSoFar += bytesRead;
    }
    pm_close(scriptFileP);

    {
        unsigned int i;
        for (i = 0; i < bytesReadSoFar; ++i)
            if (!isprint(script[i]) && !isspace(script[i]))
                pm_error("Script contains byte that is not printable ASCII "
                         "character: 0x%02x", script[i]);
    }
    script[bytesReadSoFar] = '\0';  /* terminating NUL */

    *scriptP = script;
}



enum drawVerb {
    VERB_SETPOS,
    VERB_SETLINETYPE,
    VERB_SETLINECLIP,
    VERB_SETCOLOR,
    VERB_SETFONT,
    VERB_LINE,
    VERB_LINE_HERE,
    VERB_SPLINE3,
    VERB_CIRCLE,
    VERB_FILLEDRECTANGLE,
    VERB_TEXT,
    VERB_TEXT_HERE
};

struct setposArg {
    int x;
    int y;
};

struct setlinetypeArg {
    int type;
};

struct setlineclipArg {
    unsigned int clip;
};

struct setcolorArg {
    const char * colorName;
};

struct setfontArg {
    const char * fontFileName;
};

struct lineArg {
    int x0;
    int y0;
    int x1;
    int y1;
};

struct lineHereArg {
    int right;
    int down;
};

struct spline3Arg {
    int x0;
    int y0;
    int x1;
    int y1;
    int x2;
    int y2;
};

struct circleArg {
    int cx;
    int cy;
    unsigned int radius;
};

struct filledrectangleArg {
    int x;
    int y;
    unsigned int width;
    unsigned int height;
};

struct textArg {
    int xpos;
    int ypos;
    unsigned int height;
    int angle;
    const char * text;
};


struct drawCommand {
    enum drawVerb verb;
    union {
        struct setposArg          setposArg;
        struct setlinetypeArg     setlinetypeArg;
        struct setlineclipArg     setlineclipArg;
        struct setcolorArg        setcolorArg;
        struct setfontArg         setfontArg;
        struct lineArg            lineArg;
        struct lineHereArg        lineHereArg;
        struct spline3Arg         spline3Arg;
        struct circleArg          circleArg;
        struct filledrectangleArg filledrectangleArg;
        struct textArg            textArg;
    } u;
};



static void
freeDrawCommand(const struct drawCommand * const commandP) {
    
    switch (commandP->verb) {
    case VERB_SETPOS:
        break;
    case VERB_SETLINETYPE:
        break;
    case VERB_SETLINECLIP:
        break;
    case VERB_SETCOLOR:
        strfree(commandP->u.setcolorArg.colorName);
        break;
    case VERB_SETFONT:
        strfree(commandP->u.setfontArg.fontFileName);
        break;
    case VERB_LINE:
        break;
    case VERB_LINE_HERE:
        break;
    case VERB_SPLINE3:
        break;
    case VERB_CIRCLE:
        break;
    case VERB_FILLEDRECTANGLE:
        break;
    case VERB_TEXT:
    case VERB_TEXT_HERE:
        strfree(commandP->u.textArg.text);
        break;
    }
    

    free((void *) commandP);
}



struct commandListElt {
    struct commandListElt * nextP;
    const struct drawCommand * commandP;
};


struct script {
    struct commandListElt * commandListHeadP;
    struct commandListElt * commandListTailP;
};



static void
freeScript(struct script * const scriptP) {

    struct commandListElt * p;
    struct commandListElt * nextP;

    for (p = scriptP->commandListHeadP; p; p = nextP) {
        freeDrawCommand(p->commandP);
        nextP = p->nextP;
        free(p);
    }

    free(scriptP);
}



static void
doTextHere(pixel **                   const pixels,
           unsigned int               const cols,
           unsigned int               const rows,
           pixval                     const maxval,
           const struct drawCommand * const commandP,
           struct drawState *         const drawStateP) {
    
    ppmd_text(pixels, cols, rows, maxval,
              drawStateP->currentPos.x,
              drawStateP->currentPos.y,
              commandP->u.textArg.height,
              commandP->u.textArg.angle,
              commandP->u.textArg.text,
              PPMD_NULLDRAWPROC,
              &drawStateP->color);
    
    {
        int left, top, right, bottom;
        
        ppmd_text_box(commandP->u.textArg.height, 0,
                      commandP->u.textArg.text,
                      &left, &top, &right, &bottom);
        

        drawStateP->currentPos.x +=
            ROUND((right-left) * cosdeg(commandP->u.textArg.angle));
        drawStateP->currentPos.y -=
            ROUND((right-left) * sindeg(commandP->u.textArg.angle));
    }
}



static void
executeScript(struct script * const scriptP,
              pixel **        const pixels,
              unsigned int    const cols,
              unsigned int    const rows,
              pixval          const maxval) {

    struct drawState drawState;
    unsigned int seq;
        /* Sequence number of current command (0 = first, etc.) */
    struct commandListElt * p;
        /* Pointer to current element in command list */

    initDrawState(&drawState, maxval);

    for (p = scriptP->commandListHeadP, seq = 0; p; p = p->nextP, ++seq) {
        const struct drawCommand * const commandP = p->commandP;

        if (verbose)
            pm_message("Command %u: %u", seq, commandP->verb);

        switch (commandP->verb) {
        case VERB_SETPOS:
            drawState.currentPos.x = commandP->u.setposArg.x;
            drawState.currentPos.y = commandP->u.setposArg.y;
            break;
        case VERB_SETLINETYPE:
            ppmd_setlinetype(commandP->u.setlinetypeArg.type);
            break;
        case VERB_SETLINECLIP:
            ppmd_setlineclip(commandP->u.setlineclipArg.clip);
            break;
        case VERB_SETCOLOR:
            drawState.color =
                ppm_parsecolor2(commandP->u.setcolorArg.colorName,
                                maxval, TRUE);
            break;
        case VERB_SETFONT: {
            FILE * ifP;
            const struct ppmd_font * fontP;
            ifP = pm_openr(commandP->u.setfontArg.fontFileName);
            ppmd_read_font(ifP, &fontP);
            ppmd_set_font(fontP);
            pm_close(ifP);
        } break;
        case VERB_LINE:
            ppmd_line(pixels, cols, rows, maxval,
                      commandP->u.lineArg.x0, commandP->u.lineArg.y0,
                      commandP->u.lineArg.x1, commandP->u.lineArg.y1,
                      PPMD_NULLDRAWPROC,
                      &drawState.color);
            break;
        case VERB_LINE_HERE: {
            struct pos endPos;

            endPos.x = drawState.currentPos.x + commandP->u.lineHereArg.right;
            endPos.y = drawState.currentPos.y + commandP->u.lineHereArg.down;

            ppmd_line(pixels, cols, rows, maxval,
                      drawState.currentPos.x, drawState.currentPos.y,
                      endPos.x, endPos.y,
                      PPMD_NULLDRAWPROC,
                      &drawState.color);
            drawState.currentPos = endPos;
        } break;
        case VERB_SPLINE3:
            ppmd_spline3(pixels, cols, rows, maxval,
                         commandP->u.spline3Arg.x0,
                         commandP->u.spline3Arg.y0,
                         commandP->u.spline3Arg.x1,
                         commandP->u.spline3Arg.y1,
                         commandP->u.spline3Arg.x2,
                         commandP->u.spline3Arg.y2,
                         PPMD_NULLDRAWPROC,
                         &drawState.color);
            break;
        case VERB_CIRCLE:
            ppmd_circle(pixels, cols, rows, maxval,
                        commandP->u.circleArg.cx,
                        commandP->u.circleArg.cy,
                        commandP->u.circleArg.radius,
                        PPMD_NULLDRAWPROC,
                        &drawState.color);
            break;
        case VERB_FILLEDRECTANGLE:
            ppmd_filledrectangle(pixels, cols, rows, maxval,
                                 commandP->u.filledrectangleArg.x,
                                 commandP->u.filledrectangleArg.y,
                                 commandP->u.filledrectangleArg.width,
                                 commandP->u.filledrectangleArg.height,
                                 PPMD_NULLDRAWPROC,
                                 &drawState.color);
            break;
        case VERB_TEXT:
            ppmd_text(pixels, cols, rows, maxval,
                      commandP->u.textArg.xpos,
                      commandP->u.textArg.ypos,
                      commandP->u.textArg.height,
                      commandP->u.textArg.angle,
                      commandP->u.textArg.text,
                      PPMD_NULLDRAWPROC,
                      &drawState.color);
            break;
        case VERB_TEXT_HERE:
            doTextHere(pixels, cols, rows, maxval, commandP, &drawState);
            break;
        }
    }
}



struct tokenSet {
    
    const char * token[10];
    unsigned int count;
    
};



static void
parseDrawCommand(struct tokenSet             const commandTokens,
                 const struct drawCommand ** const drawCommandPP) {

    struct drawCommand * drawCommandP;

    if (commandTokens.count < 1)
        pm_error("No tokens in command.");
    else {
        const char * const verb = commandTokens.token[0];

        MALLOCVAR(drawCommandP);
        if (drawCommandP == NULL)
            pm_error("Out of memory to parse '%s' command", verb);

        if (STREQ(verb, "setpos")) {
            drawCommandP->verb = VERB_SETPOS;
            if (commandTokens.count < 3)
                pm_error("Not enough tokens for a 'setpos' command.  "
                         "Need %u.  Got %u", 3, commandTokens.count);
            else {
                drawCommandP->u.setposArg.x = atoi(commandTokens.token[1]);
                drawCommandP->u.setposArg.y = atoi(commandTokens.token[2]);
            }
        } else if (STREQ(verb, "setlinetype")) {
            drawCommandP->verb = VERB_SETLINETYPE;
            if (commandTokens.count < 2)
                pm_error("Not enough tokens for a 'setlinetype' command.  "
                         "Need %u.  Got %u", 2, commandTokens.count);
            else {
                const char * const typeArg = commandTokens.token[1];
                if (STREQ(typeArg, "normal"))
                    drawCommandP->u.setlinetypeArg.type = PPMD_LINETYPE_NORMAL;
                else if (STREQ(typeArg, "normal"))
                    drawCommandP->u.setlinetypeArg.type = 
                        PPMD_LINETYPE_NODIAGS;
                else
                    pm_error("Invalid type");
            }
        } else if (STREQ(verb, "setlineclip")) {
            drawCommandP->verb = VERB_SETLINECLIP;
            if (commandTokens.count < 2)
                pm_error("Not enough tokens for a 'setlineclip' command.  "
                         "Need %u.  Got %u", 2, commandTokens.count);
            else
                drawCommandP->u.setlineclipArg.clip =
                    atoi(commandTokens.token[1]);
        } else if (STREQ(verb, "setcolor")) {
            drawCommandP->verb = VERB_SETCOLOR;
            if (commandTokens.count < 2)
                pm_error("Not enough tokens for a 'setcolor' command.  "
                         "Need %u.  Got %u", 2, commandTokens.count);
            else
                drawCommandP->u.setcolorArg.colorName =
                    strdup(commandTokens.token[1]);
        } else if (STREQ(verb, "setfont")) {
            drawCommandP->verb = VERB_SETFONT;
            if (commandTokens.count < 2)
                pm_error("Not enough tokens for a 'setfont' command.  "
                         "Need %u.  Got %u", 2, commandTokens.count);
            else
                drawCommandP->u.setfontArg.fontFileName =
                    strdup(commandTokens.token[1]);
        } else if (STREQ(verb, "line")) {
            drawCommandP->verb = VERB_LINE;
            if (commandTokens.count < 5)
                pm_error("Not enough tokens for a 'line' command.  "
                         "Need %u.  Got %u", 5, commandTokens.count);
            else {
                drawCommandP->u.lineArg.x0 = atoi(commandTokens.token[1]);
                drawCommandP->u.lineArg.y0 = atoi(commandTokens.token[2]);
                drawCommandP->u.lineArg.x1 = atoi(commandTokens.token[3]);
                drawCommandP->u.lineArg.y1 = atoi(commandTokens.token[4]);
            } 
        } else if (STREQ(verb, "line_here")) {
            drawCommandP->verb = VERB_LINE_HERE;
            if (commandTokens.count < 3)
                pm_error("Not enough tokens for a 'line_here' command.  "
                         "Need %u.  Got %u", 3, commandTokens.count);
            else {
                struct lineHereArg * const argP =
                    &drawCommandP->u.lineHereArg;
                argP->right = atoi(commandTokens.token[1]);
                argP->down = atoi(commandTokens.token[2]);
            } 
       } else if (STREQ(verb, "spline3")) {
            drawCommandP->verb = VERB_SPLINE3;
            if (commandTokens.count < 7)
                pm_error("Not enough tokens for a 'spline3' command.  "
                         "Need %u.  Got %u", 7, commandTokens.count);
            else {
                struct spline3Arg * const argP =
                    &drawCommandP->u.spline3Arg;
                argP->x0 = atoi(commandTokens.token[1]);
                argP->y0 = atoi(commandTokens.token[2]);
                argP->x1 = atoi(commandTokens.token[3]);
                argP->y1 = atoi(commandTokens.token[4]);
                argP->x2 = atoi(commandTokens.token[5]);
                argP->y2 = atoi(commandTokens.token[6]);
            } 
        } else if (STREQ(verb, "circle")) {
            drawCommandP->verb = VERB_CIRCLE;
            if (commandTokens.count < 4)
                pm_error("Not enough tokens for a 'circle' command.  "
                         "Need %u.  Got %u", 4, commandTokens.count);
            else {
                struct circleArg * const argP = &drawCommandP->u.circleArg;
                argP->cx     = atoi(commandTokens.token[1]);
                argP->cy     = atoi(commandTokens.token[2]);
                argP->radius = atoi(commandTokens.token[3]);
            } 
        } else if (STREQ(verb, "filledrectangle")) {
            drawCommandP->verb = VERB_FILLEDRECTANGLE;
            if (commandTokens.count < 5)
                pm_error("Not enough tokens for a 'filledrectangle' command.  "
                         "Need %u.  Got %u", 4, commandTokens.count);
            else {
                struct filledrectangleArg * const argP =
                    &drawCommandP->u.filledrectangleArg;
                argP->x      = atoi(commandTokens.token[1]);
                argP->y      = atoi(commandTokens.token[2]);
                argP->width  = atoi(commandTokens.token[3]);
                argP->height = atoi(commandTokens.token[4]);
            } 
        } else if (STREQ(verb, "text")) {
            drawCommandP->verb = VERB_TEXT;
            if (commandTokens.count < 6)
                pm_error("Not enough tokens for a 'text' command.  "
                         "Need %u.  Got %u", 6, commandTokens.count);
            else {
                drawCommandP->u.textArg.xpos  = atoi(commandTokens.token[1]);
                drawCommandP->u.textArg.ypos  = atoi(commandTokens.token[2]);
                drawCommandP->u.textArg.height= atoi(commandTokens.token[3]);
                drawCommandP->u.textArg.angle = atoi(commandTokens.token[4]);
                drawCommandP->u.textArg.text  = strdup(commandTokens.token[5]);
                if (drawCommandP->u.textArg.text == NULL)
                    pm_error("Out of storage parsing 'text' command");
            }
        } else if (STREQ(verb, "text_here")) {
            drawCommandP->verb = VERB_TEXT_HERE;
            if (commandTokens.count < 4)
                pm_error("Not enough tokens for a 'text_here' command.  "
                         "Need %u.  Got %u", 4, commandTokens.count);
            else {
                drawCommandP->u.textArg.height= atoi(commandTokens.token[1]);
                drawCommandP->u.textArg.angle = atoi(commandTokens.token[2]);
                drawCommandP->u.textArg.text  = strdup(commandTokens.token[3]);
                if (drawCommandP->u.textArg.text == NULL)
                    pm_error("Out of storage parsing 'text_here' command");
            }
        } else
            pm_error("Unrecognized verb '%s'", verb);
    }
    *drawCommandPP = drawCommandP;
}



static void
disposeOfCommandTokens(struct tokenSet * const tokenSetP,
                       struct script *   const scriptP) {

    /* We've got a whole command in 'tokenSet'.  Parse it into *scriptP
       and reset tokenSet to empty.
    */
    
    struct commandListElt * commandListEltP;
    
    MALLOCVAR(commandListEltP);
    if (commandListEltP == NULL)
        pm_error("Out of memory allocating command list element frame");

    parseDrawCommand(*tokenSetP, &commandListEltP->commandP);

    {
        unsigned int i;
        for (i = 0; i < tokenSetP->count; ++i)
            strfree(tokenSetP->token[i]);
        tokenSetP->count = 0;
    }
    /* Put the list element for this command at the tail of the list */
    commandListEltP->nextP = NULL;
    if (scriptP->commandListTailP)
        scriptP->commandListTailP->nextP = commandListEltP;
    else
        scriptP->commandListHeadP = commandListEltP;

    scriptP->commandListTailP = commandListEltP;
}



static void
processToken(const char *      const scriptText,
             unsigned int      const cursor,
             unsigned int      const tokenStart,
             struct script *   const scriptP,
             struct tokenSet * const tokenSetP) {

    char * token;
    unsigned int const tokenLength = cursor - tokenStart;
    MALLOCARRAY_NOFAIL(token, tokenLength + 1);
    memcpy(token, &scriptText[tokenStart], tokenLength);
    token[tokenLength] = '\0';
    
    if (STREQ(token, ";")) {
        disposeOfCommandTokens(tokenSetP, scriptP);
        free(token);
    } else {
        if (tokenSetP->count >= ARRAY_SIZE(tokenSetP->token))
            pm_error("too many tokens");
        else
            tokenSetP->token[tokenSetP->count++] = token;
    }
}



static void
parseScript(const char *     const scriptText,
            struct script ** const scriptPP) {

    struct script * scriptP;
    unsigned int cursor;  /* cursor in scriptText[] */
    bool intoken;      /* Cursor is inside token */
    unsigned int tokenStart;
        /* Position in 'scriptText' where current token starts.
           Meaningless if 'intoken' is false.
        */
    bool quotedToken;
        /* Current token is a quoted string.  Meaningless if 'intoken'
           is false 
        */
    struct tokenSet tokenSet;

    MALLOCVAR_NOFAIL(scriptP);

    scriptP->commandListHeadP = NULL;
    scriptP->commandListTailP = NULL;

    /* A token begins with a non-whitespace character.  A token ends before
       a whitespace character or semicolon or end of script, except that if
       the token starts with a double quote, whitespace and semicolon don't
       end it and another double quote does.

       Semicolon (unquoted) is a token by itself.
    */

    tokenSet.count = 0;
    intoken = FALSE;
    tokenStart = 0;
    cursor = 0;

    while (scriptText[cursor] != '\0') {
        char const scriptChar = scriptText[cursor];
        
        if (intoken) {
            if ((quotedToken && scriptChar == '"') ||
                (!quotedToken && (isspace(scriptChar) || scriptChar == ';'))) {
                /* We've passed a token. */

                processToken(scriptText, cursor, tokenStart, scriptP,
                             &tokenSet);

                intoken = FALSE;
                if (scriptChar != ';')
                    ++cursor;
            } else
                ++cursor;
        } else {
            if (!isspace(scriptChar)) {
                /* A token starts here */

                if (scriptChar == ';')
                    /* It ends here too -- semicolon is token by itself */
                    processToken(scriptText, cursor+1, cursor, scriptP,
                                 &tokenSet);
                else {
                    intoken = TRUE;
                    quotedToken = (scriptChar == '"');
                    if (quotedToken)
                        tokenStart = cursor + 1;
                    else
                        tokenStart = cursor;
                }
            }
            ++cursor;
        }            
    }

    if (intoken) {
        /* Parse the last token, which was terminated by end of string */
        if (quotedToken)
            pm_error("Script ends in the middle of a quoted string");
        processToken(scriptText, cursor, tokenStart, scriptP, &tokenSet);
    }

    if (tokenSet.count > 0) {
        /* Parse the last command, which was not terminated with a semicolon.
         */
        disposeOfCommandTokens(&tokenSet, scriptP);
    }

    *scriptPP = scriptP;
}



static void
getScript(struct cmdlineInfo const cmdline,
          struct script **   const scriptPP) {

    const char * scriptText;

    if (cmdline.script) {
        scriptText = strdup(cmdline.script);
        if (scriptText == NULL)
            pm_error("Out of memory creating script");
    } else if (cmdline.scriptfile)
        readScriptFile(cmdline.scriptfile, &scriptText);
    else
        pm_error("INTERNAL ERROR: no script");

    if (verbose)
        pm_message("Executing script '%s'", scriptText);

    parseScript(scriptText, scriptPP);

    strfree(scriptText);
}

          

static void
doOneImage(FILE *          const ifP,
           struct script * const scriptP)  {

    pixel ** pixels;
    pixval maxval;
    int rows, cols;
    
    pixels = ppm_readppm(ifP, &cols, &rows, &maxval);
    
    executeScript(scriptP, pixels, cols, rows, maxval);
    
    ppm_writeppm(stdout, pixels, cols, rows, maxval, 0);
    
    ppm_freearray(pixels, rows);
}



int
main(int argc, char * argv[]) {

    struct cmdlineInfo cmdline;
    FILE * ifP;
    struct script * scriptP;
    bool eof;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;

    ifP = pm_openr(cmdline.inputFilename);

    getScript(cmdline, &scriptP);

    eof = FALSE;
    while (!eof) {
        doOneImage(ifP, scriptP);
        ppm_nextimage(ifP, &eof);
    }

    freeScript(scriptP);

    pm_close(ifP);

    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0;
}
