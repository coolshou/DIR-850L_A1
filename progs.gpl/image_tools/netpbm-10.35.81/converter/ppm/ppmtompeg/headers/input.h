#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "pm_c_util.h"
#include "ppm.h"
#include "frame.h"

struct InputFileEntry;

struct inputSource {
/*----------------------------------------------------------------------------
   This describes the source of data for the program.
   Typically, the source data is a bunch of raw frames in PPM format.
   But sometimes, it is a bunch of already encoded frames or GOPs.
-----------------------------------------------------------------------------*/
    bool stdinUsed;
    int numInputFiles;
        /* This is the maximum number of input files available.  If
           we're reading from explicitly named files, it is exactly
           the number available.  If we're reading from a stream, it's
           infinity.  (At the moment, "reading from a stream" is
           equivalent to "reading from Standard Input").
        */

    /* Members below here defined only if 'stdinUsed' is false */

    struct InputFileEntry ** inputFileEntries;
        /* Each element of this array describes a set of input files.
           Valid elements are consecutive starting at index 0.
        */
    unsigned int             numInputFileEntries;
        /* Number of valid entries in array inputFileEntries[] */
    unsigned int             ifArraySize;
        /* Number of allocated entries in the array inputFileEntries[] */
};


void
GetNthInputFileName(struct inputSource * const inputSourceP,
                    unsigned int         const n,
                    const char **        const fileName);

void
ReadNthFrame(struct inputSource * const inputSourceP,
             unsigned int         const frameNumber,
             boolean              const remoteIO,
             boolean              const childProcess,
             boolean              const separateConversion,
             const char *         const slaveConversion,
             const char *         const inputConversion,
             MpegFrame *          const frameP,
             bool *               const endOfStreamP);

void
JM2JPEG(struct inputSource * const inputSourceP);

void
AddInputFiles(struct inputSource * const inputSourceP,
              const char *         const input);

void
SetStdinInput(struct inputSource * const inputSourceP);

void
CreateInputSource(struct inputSource ** const inputSourcePP);

void
DestroyInputSource(struct inputSource * const inputSourceP);

#endif
