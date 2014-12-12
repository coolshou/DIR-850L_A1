#include "ansi.h"


void
JMovie2JPEG(const char * const infilename,
            const char * const obase,
            int          const start,
            int          const end);

void
ReadJPEG(MpegFrame * const mf,
         FILE *      const fp);
