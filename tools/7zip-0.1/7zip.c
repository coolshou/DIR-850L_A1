#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// +++ Add by shiang for endian conversion!  2004/08/12
#include <netinet/in.h>    // For htonl() / htons()
// --- Add by shiang for endian conversion!  2004/08/12

#include "lzma_encoder.h"

#define TARGET_IS_BIGENDIAN 1


unsigned char signature[6] = {'7', 'z', 0xbc, 0xaf, 0x27, 0x1c};

unsigned int GetDecompressAddress(char *ifname);

typedef struct {
  unsigned char kSignature[6];
  unsigned char Major;
  unsigned char Minor;
  unsigned int  uiCompr_Size;
  unsigned int  uiCompr_CRC;
  unsigned int  uiCompr_Addr;
  unsigned int  uiDecompr_Size;
  unsigned int  uiDecompr_CRC;
  unsigned int  uiDecompr_Addr;
} LZMA_Header;


void print_usage(void)
{
	printf("Usage: 7zip input-file output-file kernel/loader big/little\n");
	printf("           -k(K): self-decompress in kernel\n");
	printf("           -l(L): decompress in loader\n");
}


unsigned int imgCRC(unsigned char *ptr, unsigned int len)
{
  unsigned int i;
  unsigned int crc = 0;

  for(i=0; i < len; i++)
    crc += ptr[i];

  return crc;
}


int main(int argc, char **argv)
{
  char ifname[256];
  char ofname[256];
  char compress_mode[2];

#if 0
// +++ Add by shiang 2004/08/13
 	//We get the decompress address by grep the symbol name in symbol file.
  char entrypoint[256];  //the symbol name of entrypoint. 
  char symbolfile[256];  //The file records the symbol tables
// --- Add by shiang 2004/08/13
#endif
  
  int ifd,ofd;
  struct stat istat;
  unsigned int inSize, outSize;
  unsigned char *srcPtr, *dstPtr;
  int rc, count;
  LZMA_Header lzma_header;
  int hdrSize = sizeof(LZMA_Header);
  char endian;

  if (argc < 5) {
	  printf("Error: Invalid number of arguments\n");
	  print_usage();
	  return(-1);
  }
  
  // Copy the file name from command line argument to local variable
  strcpy(ifname, argv[1]);
  strcpy(ofname, argv[2]);
  
  strncpy(compress_mode, argv[3], 2);

  switch (compress_mode[1]) {
  	case 'k':
	case 'K':
		  lzma_header.Minor = 'k';
		  //lzma_header.uiDecompr_Addr = htonl(GetDecompressAddress(ifname));
		  break;
	case 'l':
	case 'L':
		  lzma_header.Minor = 'l';
		  break;
	default:
		  printf("Error: wrong argument %s, please use -k/-K/-l/-L\n", compress_mode);
		  return -1;
  }

  if (strcmp(argv[4], "big")==0) endian = 'b';
  else if (strcmp(argv[4], "little")==0) endian = 'l';
  else
  {
	  printf("known endian : [%s]\n", argv[4]);
	  print_usage();
	  return (-1);
  }
  
  // Open the input file for reading
  ifd = open(ifname,O_RDONLY,S_IREAD);
  if( ifd < 0 ) {
	  printf("Error: Unable to open %s\n", ifname);
	  return(-1);
  }
  
  // Get the input file properties
  fstat(ifd,&istat);

  // Create the input buffer
  inSize = istat.st_size;
  srcPtr = (unsigned char *)malloc(inSize);
  if( srcPtr==NULL ) {
	  printf("Error: Unable to create input buffer size of %d\n", inSize);
	  return(-1);
  }

  // Allocate memory for output buffer
  // TODO: How big to create?  Can we assume for now it will be smaller?
  dstPtr = (unsigned char *)malloc(inSize);
  if( dstPtr==NULL ) {
	  printf("Error: Unable to create output buffer size of %d\n", inSize);
	  return(-1);
  }


  // Read the buffer from file to local buffer
  count=read(ifd, (void *)srcPtr, istat.st_size);
  if( count!= istat.st_size ) {
	  printf("Debug: Need to handle multi-reads/error to buffer\n");
	  printf("Debug: Read returned 0x%0x\n", (unsigned int) count);
	  return(-1);
  }
  
  // Create a new Output File.
  ofd = creat(ofname, S_IRUSR | S_IWUSR);
  if(ofd<0) {
	  printf("Error: Unable to create %s\n", ofname );
	  return(-1);
  }
  

  // Encode the data
  rc = EncodeLZMA(dstPtr+hdrSize, &outSize, srcPtr, &inSize);
  if (rc != 0)
  {
	  printf("Compression Error.\n");
	  return (-1);
  }
  
  printf("File compression: %d to %d\n", inSize, outSize);

  // Assign LZMA Header values
  memcpy(lzma_header.kSignature, signature, 6);
  lzma_header.Major = '0';
  
#if 0  //Shiang: Currently we use this byte to separate the kerenl/loader decompress method!
  lzma_header.Minor = '2';
#endif

// +++ Modify by shiang for big endian system. 2004/08/12
  if (endian == 'b')
  {
    lzma_header.uiCompr_Size = htonl(outSize);  
    lzma_header.uiCompr_CRC = htonl(imgCRC(dstPtr+hdrSize, outSize));  
    lzma_header.uiCompr_Addr = 0;  
    lzma_header.uiDecompr_Size = htonl(inSize);  
    lzma_header.uiDecompr_CRC = htonl(imgCRC(srcPtr,inSize));  
    lzma_header.uiDecompr_Addr = 0;  
  }
  else  //little endian
  {
    lzma_header.uiCompr_Size = outSize;  
    lzma_header.uiCompr_CRC = imgCRC(dstPtr+hdrSize, outSize);  
    lzma_header.uiCompr_Addr = 0;  
    lzma_header.uiDecompr_Size = inSize;  
    lzma_header.uiDecompr_CRC = imgCRC(srcPtr,inSize);  
    lzma_header.uiDecompr_Addr = 0;  
  }
// +++ Modify by shiang for big endian system. 2004/08/12

  memcpy(dstPtr,&lzma_header, hdrSize);


  // Write buffer contents to new Compressed file.
  count = write(ofd, dstPtr, outSize+hdrSize);
  if( count!= (outSize+hdrSize) ) {
	  printf("Debug: Need to handle mult-writes/errors\n");
	  printf("Debug: Read returned 0x%0x\n", (unsigned int) count);
	  return(-1);
  }
  return (0);
}

