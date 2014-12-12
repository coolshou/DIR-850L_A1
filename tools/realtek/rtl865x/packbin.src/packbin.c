/*
 * Copyright c                Realtek Semiconductor Corporation, 2002
 * All rights reserved.                                                    
 * 
 * $Header: /home/cvsroot/uClinux-dist/tools/packbin.src/packbin.c,v 1.3 2005/01/10 05:26:56 rupert Exp $
 *
 * $Author: rupert $
 *
 * Abstract:
 *
 *   Append image header, calculate checksum and padding.
 *
 * $Log: packbin.c,v $
 * Revision 1.3  2005/01/10 05:26:56  rupert
 * * :  rename the tmpname path  to the current directory
 *
 * Revision 1.2  2005/01/06 12:22:47  yjlou
 * *: fixed the bug of temp file naming
 *    tmpFilename[50] = "/tmp/packbin.XXXXXX";
 *
 * Revision 1.1  2004/12/01 07:34:21  yjlou
 * *** empty log message ***
 *
 * Revision 1.1  2002/07/19 05:50:00  danwu
 * Create file.
 *
 *
 * 
 */

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <time.h>
/* integration with Loader. */
#include    "../../loader_srcroot/inc/rtl_types.h"
#include    "../../loader_srcroot/inc/rtl_image.h"

const char logo_msg[] = {
	"(c)Copyright Realtek, Inc. 2002\n" 
	"Project ROME\n\n"
};
#define ENDIAN_SWITCH32(x) (((x) >> 24) | (((x) >> 8) & 0xFF00) | \
                            (((x) << 8) & 0xFF0000) | (((x) << 24) &0xFF000000))
#define ENDIAN_SWITCH16(x) ((((x) >> 8) & 0xFF) | (((x) << 8) & 0xFF00))

void
main(int argc, char *argv[])
{
    FILE *              fp1;
    FILE *              fp2;
    char                tmpFilename[50] = "packbin.XXXXXX";
    unsigned char       imgType;
    unsigned long       imgSize,dateofday,timeofday;
    unsigned long       padTo;
    struct stat         fileStat;
    unsigned long       i;
    unsigned char       ch;
    unsigned char *     pch;
    unsigned char       chksum;
    
    fileImageHeader_t   imghdr;
	struct timeval timeval;
	struct tm *ut;
	time_t tt;

	tt = time(NULL);
	gettimeofday(&timeval,NULL);
	//gmtime_r(&tt,&ut);
	ut=localtime(&tt);
	printf("date %04d/%02d/%02d %02d:%02d:%02d \n",
			1900+ut->tm_year,
			ut->tm_mon+1,
			ut->tm_mday,
			ut->tm_hour,
			ut->tm_min,
			ut->tm_sec
			);
    	dateofday=((ut->tm_year+1900)<<16)+((ut->tm_mon+1)<<8)+(ut->tm_mday);
    	timeofday=((ut->tm_hour)<<24)+((ut->tm_min)<<16)+((ut->tm_sec)<<8);
	printf("%08x %08x\n",dateofday,timeofday);
    /* Check arguments */
    if (argc < 5)
    {
        printf("Usage: PACKBIN input_file image_type pad_to output_file\n");
        printf("    input_file    Name of the input binary image file.\n");
        printf("    image_type    [r|b|k] for [runtime|boot|kernel+fs].\n");
        printf("    pad_to        Size of the output binary image. (0 means no padding)\n");
        printf("    output_file   Name of the output binary image file.\n");
        
        return;
    }
    
    /* Open file */
     if ((fp1=fopen(argv[1], "r+b")) == NULL)
    {
        printf("Cannot open %s !!\n", argv[1]);
        return;
    }
    
    /* Get image type */
    imgType = (unsigned char) *argv[2];
    bzero((void *) &imghdr, sizeof(fileImageHeader_t));
    switch (imgType)
    {
        case 'r':
            imghdr.imageType = ENDIAN_SWITCH16(RTL_IMAGE_TYPE_RUN);
            break;
        case 'b':
            imghdr.imageType = ENDIAN_SWITCH16(RTL_IMAGE_TYPE_BOOT);
            break;
        case 'k':
            imghdr.imageType = ENDIAN_SWITCH16(RTL_IMAGE_TYPE_KFS);
            break;
        
        default:
            printf("Image type error !!\n");
    }
    
    /* Get padding size */
    padTo = strtol(argv[3], NULL, 0);
    
    /* Get file size */
    if (stat(argv[1], &fileStat) != 0)
    {
        printf("Cannot get file statistics !!\n");
        fclose(fp1);
        exit(-1);
    }
    imgSize = fileStat.st_size;
    
    /* Check padding size */
    if ( (padTo < imgSize + sizeof(fileImageHeader_t)) && (padTo != 0) )
    {
        printf("Padding size error !!\n");
        fclose(fp1);
        exit(-1);
    }
    if (!padTo)
    {
	    if (imgSize%2)
		    padTo=(imgSize+sizeof(fileImageHeader_t))+1;
    }    
    printf("Image Original Size = 0x%lx\n", imgSize);
    
    /* Temparay file */
    //tmpnam(tmpFilename);
    mkstemp(tmpFilename);
    if ((fp2=fopen(tmpFilename, "w+b")) == NULL)
    {
        printf("Cannot open temprary file !!\n");
        return;
    }
    
    /* Copy image */
    fseek(fp1, 0L, SEEK_SET);
    fseek(fp2, sizeof(fileImageHeader_t), SEEK_SET);
    chksum = 0;
    for (i=0; i<imgSize; i++)
    {
        ch = fgetc(fp1);
        fputc(ch, fp2);
        chksum ^= ch;
    }
    
    /* Padding */
    if (padTo)
        while (i++ < (padTo - sizeof(fileImageHeader_t)))
            fputc(0, fp2);
    
    /* Forge image header */
    printf("      Magic Number = 0x%lx\n", RTL_PRODUCT_MAGIC);
    imghdr.productMagic = ENDIAN_SWITCH32(RTL_PRODUCT_MAGIC);
    imghdr.imageHdrVer = RTL_IMAGE_HDR_VER_1_0;
    /* date and time to be implemented */
    imghdr.date=ENDIAN_SWITCH32(dateofday);
    imghdr.time=ENDIAN_SWITCH32(timeofday);
    if (padTo)
        imghdr.imageLen = ENDIAN_SWITCH32(padTo - sizeof(fileImageHeader_t));
    else
        imghdr.imageLen = ENDIAN_SWITCH32(imgSize);
    printf("      Body Checksum = 0x%x\n", chksum);
    imghdr.imageBdyCksm = chksum;
    /* Calculate header checksum */
    pch = (unsigned char *) &imghdr;
    chksum = 0;
    for (i=0; i<(sizeof(fileImageHeader_t)-1); i++)
        chksum ^= *pch++;
    imghdr.imageHdrCksm = chksum;
    printf("      Header Checksum = 0x%x\n", chksum);
    
    /* Write header */
    pch = (unsigned char *) &imghdr;
    fseek(fp2, 0L, SEEK_SET);
    for (i=0; i<sizeof(fileImageHeader_t); i++)
        fputc(*pch++, fp2);
    
    /* Close file and exit */
    fclose(fp1);
    fclose(fp2);
    
    printf("Binary image %s generated!\n", argv[4]);
    remove(argv[4]);
    rename(tmpFilename, argv[4]);
}

