/*
 * Copyright c                Realtek Semiconductor Corporation, 2002
 * All rights reserved.                                                    
 * 
 * $Header: /home/cvsroot/uClinux-dist/tools/packbin.src/kfspad.c,v 1.2 2005/03/11 04:25:10 rupert Exp $
 *
 * $Author: rupert $
 *
 * Abstract:
 *
 *   Append image header, calculate checksum and padding.
 *
 * $Log: kfspad.c,v $
 * Revision 1.2  2005/03/11 04:25:10  rupert
 * *:  give filename to mkstemp
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
    unsigned long       imgSize;
    unsigned long       padTo;
    struct stat         fileStat;
    unsigned long       i;
    unsigned char       ch;
    unsigned char *     pch;
    unsigned char       chksum;

    /* Check arguments */
    if (argc < 4)
    {
        printf("Usage: merge kernel_file size rootfs output_file\n");
        printf("    kernel_file    Name of the kernel binary image file.\n");
        printf("    size    size\n");
        printf("    rootfs        Size of the output binary image. (0 means no padding)\n");
        printf("    output_file   Name of the output binary image file.\n");
        
        return;
    }
    
    /* Open file */
     if ((fp1=fopen(argv[1], "r+b")) == NULL)
    {
        printf("Cannot open %s !!\n", argv[1]);
        return;
    }
    
    /* Get file size */
    if (stat(argv[1], &fileStat) != 0)
    {
        printf("Cannot get file statistics !!\n");
        fclose(fp1);
        exit(-1);
    }
    imgSize = fileStat.st_size;
    printf("Kernel Image Size = 0x%lx %d\n", imgSize,imgSize);
    
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
    chksum = 0;
    for (i=0; i<imgSize; i++)
    {
        ch = fgetc(fp1);
        fputc(ch, fp2);
    }
    close(fp1);
    
    padTo = strtol(argv[2], NULL, 0);
     while (i++ < (padTo - sizeof(fileImageHeader_t)))
            fputc(0, fp2);
 

     if ((fp1=fopen(argv[3], "r+b")) == NULL)
    {
        printf("Cannot open %s !!\n", argv[1]);
        return;
    }
    
    /* Get file size */
    if (stat(argv[3], &fileStat) != 0)
    {
        printf("Cannot get file statistics !!\n");
        fclose(fp1);
        exit(-1);
    }
    imgSize = fileStat.st_size;
    printf("Root File System Image Size = 0x%lx %d\n", imgSize,imgSize);
    for (i=0; i<imgSize; i++)
    {
        ch = fgetc(fp1);
        fputc(ch, fp2);
    }

    /* Padding */
    /* Close file and exit */
    fclose(fp1);
    fclose(fp2);
    
    printf("Binary image %s generated!\n", argv[4]);
    remove(argv[4]);
    rename(tmpFilename, argv[4]);
}

