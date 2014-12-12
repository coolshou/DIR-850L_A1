/* vi: set sw=4 ts=4: */
/* imghdr.h
 *
 *	This file defines the image header format for web upgrade.
 *	Currently the image format is used by WRGG02/WRGG03.
 *
 *	Copyright (C) 2003-2004, Alpha Networks, Inc.
 *
 *	2004/2/20 by David Hsieh
 *
 */

#ifndef _IMGHDR_HEADER_
#define _IMGHDR_HEADER_
#include <stdint.h>

#define MAX_SIGNATURE	32

/* Image header for WRGG02. */
typedef struct _imghdr
{
	char			signature[MAX_SIGNATURE];
	unsigned long	image_offset1;
	unsigned long	flash_offset1;
	unsigned long	size1;
	unsigned char	check1[16];
	unsigned long	image_offset2;
	unsigned long	flash_offset2;
	unsigned long	size2;
	unsigned char	check2[16];
}
imghdr;


/* Version 2 of image header. */
/*
 * Version 2 image will look like ...
 *
 * +--------------------------------------------+
 * | signature: 32 bytes                        |
 * +--------------------------------------------+
 * | image block 1 (imgblock_t)                 |
 * +--------------------------------------------+
 * | image 1                                    |
 * |                                            |
 * +--------------------------------------------+
 * | image block 2 (imgblock_t)                 |
 * +--------------------------------------------+
 * | image 2                                    |
 * |                                            |
 * +--------------------------------------------+
 */

#define IMG_MAX_DEVNAME		32
#define IMG_V2_MAGIC_NO		0x20040220	/* version 2 magic number */
#define IMG_V3_MAGIC_NO		0x20080321	/* version 3 magic number */

typedef struct _imgblock imgblock_t;
struct _imgblock
{
	uint32_t	magic;		/* image magic number (should be IMG_V2_MAGIC_NO in little endian). */
	uint32_t	size;		/* size of the image. */
	uint32_t	offset;		/* offset from the beginning of the storage device. */
	char			devname[IMG_MAX_DEVNAME];	/* null termiated string of the storage device name. ex. "/dev/mtd6" */
	unsigned char	digest[16];	/* MD5 digest of the image */
} __attribute__ ((packed));

typedef struct _imgblockv3 imgblock_tv3;
struct _imgblockv3
{
	uint32_t	magic;		/* image magic number (should be IMG_V3_MAGIC_NO in little endian). */
	char        version[16];/* firmware version ex: v1.00 */
	char        modle[16];  /* Modle name ex:DAP-2553 */
	uint32_t	flag[2];	/* control flag */
	uint32_t	reserve[2];	/* control flag */
	char    	buildno[16];/* build number */
	uint32_t	size;		/* size of the image. */
	uint32_t	offset;		/* offset from the beginning of the storage device. */
	char			devname[IMG_MAX_DEVNAME];	/* null termiated string of the storage device name. ex. "/dev/mtd6" */
	unsigned char	digest[16];	/* MD5 digest of the image */
} __attribute__ ((packed));

typedef struct _imghdr2 imghdr2_t;
struct _imghdr2
{
	char			signature[MAX_SIGNATURE];
	uint32_t	magic;	/* should be IMG_V2_MAGIC_NO in little endian. */
} __attribute__ ((packed));
static unsigned long _cpu_to_le(unsigned long value);
extern int v2_check_image_block(const char * image, int size);
extern int v3_check_image_block(const char * image, int size);
extern int v2_image_check(const char * image, int size);
extern int v3_image_check(const char * image, int size);
extern int v2_burn_image(const char * image, int size);
extern int v3_burn_image(const char * image, int size);
extern int burn_image(const char * image, int size);
extern void ftpSaveacl(char *bufout);
int IsValidMac(char* buff,int linesize);
int IsExist(char * mac,char * macList,int aclnum);
int ReadACLFromFile(char * fileName,char *buffer);
int UpdateAclList(char * buffer,int aclnum);
extern int ftpUpload_acl(char *input,int length); 
#endif
