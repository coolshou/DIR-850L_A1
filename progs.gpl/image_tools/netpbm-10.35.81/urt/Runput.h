void 
RunSetup(rle_hdr * the_hdr);

void
RunSkipBlankLines(int nblank, rle_hdr * the_hdr);

void
RunSetColor(int c, rle_hdr * the_hdr);

void
RunSkipPixels(int nskip, int last, int wasrun, rle_hdr * the_hdr);

void
RunNewScanLine(int flag, rle_hdr * the_hdr);

void
Runputdata(rle_pixel * buf, int n, rle_hdr * the_hdr);

void
Runputrun(int color, int n, int last, rle_hdr * the_hdr);

void
RunputEof(rle_hdr * the_hdr);
