void
ComputeDiffDCTs(MpegFrame * const current,
                MpegFrame * const prev,
                int         const by,
                int         const bx,
                vector      const m,
                int *       const pattern);

void
ComputeDiffDCTBlock(Block           current,
                    Block           dest,
                    Block           motionBlock,
                    boolean * const significantDifferenceP);

void
ComputeMotionBlock(uint8 ** const prev,
                   int      const by,
                   int      const bx,
                   vector   const m,
                   Block *  const motionBlockP);

void
ComputeMotionLumBlock(MpegFrame * const prevFrame,
                      int         const by,
                      int         const bx,
                      vector      const m,
                      LumBlock *  const motionBlockP);

void
BlockToData(uint8 ** const data,
            Block          block,
            int      const by,
            int      const bx);

void
AddMotionBlock(Block          block,
               uint8 ** const prev,
               int      const by,
               int      const bx,
               vector   const m);

void
AddBMotionBlock(Block          block,
                uint8 ** const prev,
                uint8 ** const next,
                int      const by,
                int      const bx,
                int      const mode,
                motion   const motion);

void
BlockifyFrame(MpegFrame * const frameP);

