struct inputSource;

void
GOPsToMPEG(struct inputSource * const inputSourceP,
           const char *         const outputFileName, 
           FILE *               const outputFilePtr);

typedef void (*fileAcquisitionFn)(void *       const handle,
                                  unsigned int const frameNumber,
                                  FILE **      const ifPP);


typedef void (*fileDispositionFn)(void *       const handle,
                                  unsigned int const frameNumber);

void
FramesToMPEG(FILE *               const outputFile, 
             void *               const inputHandle,
             fileAcquisitionFn          acquireInputFile,
             fileDispositionFn          disposeInputFile);
