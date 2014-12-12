/******************************************************************************
                              pnmendian
*******************************************************************************

  Reverse the endianness of multi-byte samples in a Netpbm stream.
  I.e. convert between the true format and the little endian variation of
  it.
******************************************************************************/
  
#include "pam.h"


static sample
reverseSample(sample const insample, unsigned int const bytesPerSample) {
/*----------------------------------------------------------------------------
  Return a sample whose value is the least significant
  'bytes_per_sample' bytes, in reverse order.
-----------------------------------------------------------------------------*/
    unsigned int bytePos;
    sample shiftedInsample;
    sample outsample;
    shiftedInsample = insample;  /* initial value */
    outsample = 0;  /* initial value */
    for (bytePos = 0; bytePos < bytesPerSample; ++bytePos) {
        outsample = outsample * 256 + (shiftedInsample & 0xff);
        shiftedInsample >>= 8;
    }
    return outsample;
}



int main(int argc, char *argv[]) {

    struct pam inpam, outpam;
    tuple * intuplerow;
    tuple * outtuplerow;
    unsigned int row;

    pnm_init(&argc, argv);

    pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));

    outpam = inpam;
    outpam.file = stdout;

    pnm_writepaminit(&outpam);

    intuplerow = pnm_allocpamrow(&inpam);      
    outtuplerow = pnm_allocpamrow(&outpam);

    for (row = 0; row < inpam.height; row++) {
        unsigned int col;
        pnm_readpamrow(&inpam, intuplerow);
        for (col = 0; col < inpam.width; col++) {
            unsigned int plane;
            for (plane = 0; plane < inpam.depth; plane++) 
                outtuplerow[col][plane] = 
                    reverseSample(intuplerow[col][plane], 
                                  inpam.bytes_per_sample);
        }
        pnm_writepamrow(&outpam, outtuplerow);
    }

    pnm_freepamrow(outtuplerow);        
    pnm_freepamrow(intuplerow);        

    exit(0);
}

