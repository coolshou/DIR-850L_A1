#include <stdio.h>
#include <assert.h>

#include "ppm.h"
#include "ppmdfont.h"



static int
untwos(unsigned char const arg) {

    if (arg >= 128)
        return arg - 256;
    else
        return arg;
}



static void
dumpHeader(struct ppmd_fontHeader const fontHeader) {
    
    pm_message("Font has %u characters", fontHeader.characterCount);
    pm_message("Font has code points %u through %u",
               fontHeader.firstCodePoint,
               fontHeader.firstCodePoint + fontHeader.characterCount - 1);
}



static void
dumpGlyph(struct ppmd_glyph const glyph) {

    unsigned int commandNum;

    pm_message("  skip before: %u pixels; skip after: %u pixels; "
               "%u commands:", 
               glyph.header.skipBefore,
               glyph.header.skipAfter,
               glyph.header.commandCount);

    for (commandNum = 0;
         commandNum < glyph.header.commandCount;
         ++commandNum) {
         
        struct ppmd_glyphCommand const glyphCommand =
            glyph.commandList[commandNum];
        
        const char * verbDisp;

        switch (glyphCommand.verb) {
        case CMD_NOOP:     verbDisp = "NOOP";     break;
        case CMD_DRAWLINE: verbDisp = "DRAWLINE"; break;
        case CMD_MOVEPEN:  verbDisp = "MOVEPEN";  break;
        }

        pm_message("    %s %d %d",
                   verbDisp, untwos(glyphCommand.x), untwos(glyphCommand.y));
    }
}



int
main(int argc, char **argv) {

    const struct ppmd_font * fontP;
    unsigned int relativeCodePoint;

    ppm_init(&argc, argv);

    ppmd_read_font(stdin, &fontP);

    dumpHeader(fontP->header);

    for (relativeCodePoint = 0;
         relativeCodePoint < fontP->header.characterCount;
         ++relativeCodePoint) {

        pm_message("Code point %u:",
                   fontP->header.firstCodePoint + relativeCodePoint);

        dumpGlyph(fontP->glyphTable[relativeCodePoint]);
    }

    ppmd_free_font(fontP);
    
    return 0;
}
