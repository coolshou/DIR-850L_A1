
/*
 * Flash mapping for rtl8196 board
 *
 * Copyright (C) 2008 Realtek Corporation
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/root_dev.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <linux/delay.h>
//#include <linux/squashfs_fs.h>
#include "../../../fs/squashfs/squashfs_fs.h"
#include <linux/autoconf.h>

#ifdef CONFIG_MTD_CONCAT
#include <linux/mtd/concat.h>
#endif

#if 0 /* 0, Alpha. */
//#define WINDOW_ADDR 0xbfe00000
#else
#define WINDOW_ADDR 0xbd000000
#endif /* 0, Alpha. */
#ifdef CONFIG_SPANSION_16M_FLASH	
#define WINDOW_SIZE 0x1000000
#define FLASH_BANK_SIZE 0x400000
#else 
#define WINDOW_SIZE 0x400000
#endif 
#define BUSWIDTH 2

#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
#define MAX_SPI_CS 2		/* Number of CS we are going to test */
#else
#define MAX_SPI_CS 1
#endif

#define SQUASHFS_MAGIC_LZMA 0x68737173

//static struct mtd_info *rtl8196_mtd;

__u8 rtl8196_map_read8(struct map_info *map, unsigned long ofs)
{
	//printk("enter %s %d\n",__FILE__,__LINE__);
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 rtl8196_map_read16(struct map_info *map, unsigned long ofs)
{
	//printk("enter %s %d\n",__FILE__,__LINE__);
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 rtl8196_map_read32(struct map_info *map, unsigned long ofs)
{
	//printk("enter %s %d\n",__FILE__,__LINE__);
	return __raw_readl(map->map_priv_1 + ofs);
}

void rtl8196_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	//printk("enter to %x from  %x len %d\n",to, map->map_priv_1+from , len);
	//11/15/05' hrchen, change the size to fit file systems block size if use different fs
	//4096 for cramfs, 1024 for squashfs
	if (from>0x10000)
	    memcpy(to, map->map_priv_1 + from, (len<=1024)?len:1024);//len);
	else
	    memcpy(to, map->map_priv_1 + from, (len<=4096)?len:4096);//len);
	//printk("enter %s %d\n", __FILE__,__LINE__);

}

void rtl8196_map_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

void rtl8196_map_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

void rtl8196_map_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

void rtl8196_map_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	//printk("enter %s %d\n",__FILE__,__LINE__);
	memcpy_toio(map->map_priv_1 + to, from, len);
}

static struct map_info spi_map[MAX_SPI_CS] = {
        {
			name: 		"flash_bank_1",
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
			size: 		CONFIG_RTL_SPI_FLASH1_SIZE,
#else
			size:			WINDOW_SIZE,
#endif
			phys: 		0xbd000000,
			virt: 			0xbd000000,
			bankwidth: 	BUSWIDTH

        },
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
        {
			name: 		"flash_bank_2",
			size: 		CONFIG_RTL_SPI_FLASH2_SIZE,
			phys: 		0xbe000000,
			virt: 			0xbe000000,
			bankwidth: 	BUSWIDTH
        }
#endif
};

static struct mtd_info *my_sub_mtd[MAX_SPI_CS] = {
	NULL,
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
	NULL
#endif
};

static struct mtd_info *mymtd;

#ifdef CONFIG_RTL_FLASH_MAPPING_ENABLE
#if 0 /* 0, Alpha. */
#if defined( CONFIG_ROOTFS_JFFS2 )
static struct mtd_partition rtl8196_parts1[] = {
        {
                name:           "boot+cfg",
                size:           (CONFIG_RTL_LINUX_IMAGE_OFFSET-0),
                offset:         0x00000000,
        },
        {
                name:           "jffs2(linux+root fs)",                
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
#ifdef CONFIG_MTD_CONCAT
                size:        (CONFIG_RTL_SPI_FLASH1_SIZE+CONFIG_RTL_SPI_FLASH2_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
#else
                size:        (CONFIG_RTL_SPI_FLASH1_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
#endif
#else
                size:        (WINDOW_SIZE - CONFIG_RTL_ROOT_IMAGE_OFFSET),
#endif
                offset:      (CONFIG_RTL_ROOT_IMAGE_OFFSET),
        }
};
#elif defined( CONFIG_ROOTFS_RAMFS )
static struct mtd_partition rtl8196_parts1[] = {
        {
                name:        "boot+cfg+linux+rootfs",
                size:        (CONFIG_RTL_FLASH_SIZE-0),
                offset:      0x00000000,
        },
};

#elif defined( CONFIG_ROOTFS_SQUASH )
#ifndef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
static struct mtd_partition rtl8196_parts1[] = {
        {
                name: "boot+cfg+linux",
                size:           (CONFIG_RTL_ROOT_IMAGE_OFFSET-0),
                offset:         0x00000000,
        },
        {
                name:           "root fs",  
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
#ifdef CONFIG_MTD_CONCAT
                size:        (CONFIG_RTL_SPI_FLASH1_SIZE+CONFIG_RTL_SPI_FLASH2_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
#else
		  size:        (CONFIG_RTL_SPI_FLASH1_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
#endif
#else
                size:        (CONFIG_RTL_FLASH_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
#endif
                offset:         (CONFIG_RTL_ROOT_IMAGE_OFFSET),
        }
};

#else //!CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
static struct mtd_partition rtl8196_parts1[] = {
        {
                name: "boot+cfg+linux(bank1)",
                size:           (CONFIG_RTL_ROOT_IMAGE_OFFSET-0),
                offset:         0x00000000,
        },
        {
                name:           "root fs(bank1)",                
                size:        (CONFIG_RTL_FLASH_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
                offset:         (CONFIG_RTL_ROOT_IMAGE_OFFSET),
        },
        {
                name: "inux(bank2)",
                size:           (CONFIG_RTL_ROOT_IMAGE_OFFSET-0),
                offset:         CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET,
        },
        {
                name:           "root fs(bank2)",                
                size:        (CONFIG_RTL_FLASH_SIZE-CONFIG_RTL_ROOT_IMAGE_OFFSET),
                offset:         CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET+(CONFIG_RTL_ROOT_IMAGE_OFFSET),
        }

};
#endif //CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE

#else
#error "unknow flash filesystem type"
#endif
#endif /* 0, Alpha. */
	/*
	 *  Flash layout
	 *
	 *  |      16K      |      16K      |      16K      |      16K      |
	 *  +---------------------------------------------------------------+ 0x00000000
	 *  | Boot (64K)                                                    |
	 *  +---------------------------------------------------------------+ 0x00010000
	 *  | RF params(64K)                                                |
	 *  +---------------------------------------------------------------+ 0x00020000
	 *  | devdata(64K)                                                  |
	 *  +---------------------------------------------------------------+ 0x00030000
	 *  | devconf(64k)                                                  |
	 *  +---------------------------------------------------------------+ 0x00040000
	 *  | retain(64k)                                                   |
	 *  +---------------------------------------------------------------+ 0x00050000
	 *  | Upgrade (linux kernel/rootfs)                                 |
	 *  ~                                                               ~
	 *  |                                                               |
	 *  +---------------------------------------------------------------+ FLASH_SIZE-LANGPACK_SIZE-MYDLINK_SIZE
	 *  | mydlink block(512k)                                           |
	 *  +---------------------------------------------------------------+ FLASH_SIZE-LANGPACK_SIZE
	 *  | language pack(64k)                                            |
	 *  +---------------------------------------------------------------+ FLASH_SIZE
	 */
#define FLASH_START			0x00000000
#define FLASH_SIZE			CONFIG_RTL_FLASH_SIZE
#define BOOTCODE_OFFSET		0x00000000
#define BOOTCODE_SIZE		(HWSETTING_OFFSET-BOOTCODE_OFFSET)
#define HWSETTING_OFFSET	CONFIG_RTL_HW_SETTING_OFFSET
#define HWSETTING_SIZE		(DEVDATA_OFFSET-HWSETTING_OFFSET)
#define DEVDATA_OFFSET		CONFIG_RTL_DEVDATA_OFFSET
#define DEVDATA_SIZE		(DEVCONF_OFFSET-DEVDATA_OFFSET)
#define DEVCONF_OFFSET		CONFIG_RTL_DEVCONF_OFFSET
#define DEVCONF_SIZE		(RETAIN_OFFSET-DEVCONF_OFFSET)
#define RETAIN_OFFSET		CONFIG_RTL_RETAIN_OFFSET
#define RETAIN_SIZE			(UPGRADE_OFFSET-RETAIN_OFFSET)
#define UPGRADE_OFFSET		CONFIG_RTL_LINUX_IMAGE_OFFSET

#ifdef CONFIG_MTD_ELBOX_MYDLINK
	#ifdef CONFIG_MTD_ELBOX_LANGPACK
		#define UPGRADE_SIZE		(MYDLINK_OFFSET-UPGRADE_OFFSET)
		#define MYDLINK_OFFSET		(FLASH_SIZE-LANGPACK_SIZE-MYDLINK_SIZE)
		#define MYDLINK_SIZE		0x80000
	#else /* Nodef CONFIG_MTD_ELBOX_LANGPACK*/
		#define UPGRADE_SIZE		(MYDLINK_OFFSET-UPGRADE_OFFSET)
		#define MYDLINK_OFFSET		(FLASH_SIZE-MYDLINK_SIZE)
		#define MYDLINK_SIZE		0x80000
	#endif /*CONFIG_MTD_ELBOX_LANGPACK*/
#else /* Nodef CONFIG_MTD_ELBOX_MYDLINK*/
	#ifdef CONFIG_MTD_ELBOX_LANGPACK
		#define UPGRADE_SIZE		(LANGPACK_OFFSET-UPGRADE_OFFSET)
		#define MYDLINK_SIZE		0
	#else /* Nodef CONFIG_MTD_ELBOX_LANGPACK*/
		#define UPGRADE_SIZE		(FLASH_SIZE-UPGRADE_OFFSET)
		#define MYDLINK_SIZE		0
	#endif /*CONFIG_MTD_ELBOX_LANGPACK*/
#endif

#ifdef CONFIG_MTD_ELBOX_LANGPACK
	#define LANGPACK_OFFSET		(FLASH_SIZE-LANGPACK_SIZE)
	#define LANGPACK_SIZE		0x30000
#else /* Nodef CONFIG_MTD_ELBOX_LANGPACK*/
	#define LANGPACK_SIZE		0
#endif /*CONFIG_MTD_ELBOX_LANGPACK*/
/* +++, Alpha. */
static struct mtd_partition rtl8196_parts1[] = {
	/* The following partitions are the "MUST" in ELBOX. */
	{name:"rootfs",		offset:0,               size:0,             mask_flags:MTD_WRITEABLE,	},
	{name:"upgrade",	offset:UPGRADE_OFFSET,  size:UPGRADE_SIZE,								},
	{name:"retain",		offset:RETAIN_OFFSET,	size:RETAIN_SIZE,								},
	{name:"devconf",	offset:DEVCONF_OFFSET,  size:DEVCONF_SIZE,								},
	{name:"devdata",	offset:DEVDATA_OFFSET,  size:DEVDATA_SIZE,								},
	{name:"HW setting",	offset:HWSETTING_OFFSET,size:HWSETTING_SIZE,							},
#ifdef CONFIG_MTD_ELBOX_LANGPACK
	{name:"langpack",	offset:LANGPACK_OFFSET, size:LANGPACK_SIZE,								},
#endif /*CONFIG_MTD_ELBOX_LANGPACK*/
	{name:"flash",		offset:0,               size:FLASH_SIZE,	mask_flags:MTD_WRITEABLE,	},
	/* The following partitions are board dependent. */
	{name:"rtlboot",	offset:0,				size:BOOTCODE_SIZE,	mask_flags:MTD_WRITEABLE,	},
#ifdef CONFIG_MTD_ELBOX_MYDLINK
    {name:"mydlink",	offset:MYDLINK_OFFSET,	size:MYDLINK_SIZE,								}
#endif
};
/* +++, Alpha. */

#else // !CONFIG_RTL_FLASH_MAPPING_ENABLE
static struct mtd_partition rtl8196_parts1[] = {
        {
                name: "boot+cfg+linux",
                size:  0x00130000,
                offset:0x00000000,
        },
        {
                name:           "root fs",                
		   		size:        	0x002D0000,
                offset:         0x00130000,
        }
};
#endif

#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE
static struct mtd_partition rtl8196_parts2[] = {
        {
                name: 		"data",
                size:  		CONFIG_RTL_SPI_FLASH2_SIZE,
                offset:		0x00000000,
        }
};
#endif

/*********************************************************************/
/* SEAMA */
#define SEAMA_MAGIC 0x5EA3A417
typedef struct seama_hdr seamahdr_t;
struct seama_hdr
{
	uint32_t	magic;		/* should always be SEAMA_MAGIC. */
	uint16_t	reserved;	/* reserved for  */
	uint16_t	metasize;	/* size of the META data */
	uint32_t	size;		/* size of the image */
} __attribute__ ((packed));

/* the tag is 32 bytes octet,
 * first part is the tag string,
 * and the second half is reserved for future used. */
#define PACKIMG_TAG "--PaCkImGs--"
struct packtag
{
	char tag[16];
	unsigned long size;
	char reserved[12];
};
typedef struct realtek_hdr realtekhdr_t;
struct realtek_hdr{
	unsigned char signature[4];
	unsigned long startAddr;
	unsigned long burnAddr;
	unsigned long len;
} __attribute__ ((packed));

static struct mtd_partition * init_mtd_partitions(struct mtd_info * mtd, size_t size)
{
	struct squashfs_super_block * squashfsb;
	struct packtag * ptag = NULL;
	unsigned char buf[64];
	int off = rtl8196_parts1[1].offset;
	size_t len;
	seamahdr_t * seama;
	//int i;
	//int *pp;
	/* Try to read the SEAMA header */
	memset(buf, 0xa5, sizeof(buf));
	if ((mtd->read(mtd, off, sizeof(seamahdr_t), &len, buf) == 0)
			&& (len == sizeof(seamahdr_t)))
	{
		seama = (seamahdr_t *)buf;
		if (ntohl(seama->magic) == SEAMA_MAGIC)
		{
			/* We got SEAMA, the offset should be shift. */
			off += sizeof(seamahdr_t);
			if (ntohl(seama->size) > 0) off += 16;
			off += ntohs(seama->metasize);
		}else 
			printk("%s:------- the flash image can't  SEAMA header----------\n",mtd->name);
	}
	//printk("---------off1==0x%x---------\n",off);
	/* Looking for PACKIMG_TAG in the 64K boundary. */
	//size=0x400000

	//		for(i=0;i<1200;i+=4){
	//		pp=0xbd050000+0x000d0000+i;
	//		printk("\n------memory address =%x------\n",*pp);
	//		}

	for (off += (0x000d0000)/*CONFIG_MTD_ELBOX_KERNEL_SKIP*/; off < size; off += (64*1024))
	{
		/* Find the tag. */

		memset(buf, 0xa5, sizeof(buf));
		if (mtd->read(mtd, off, sizeof(buf), &len, buf) || len != sizeof(buf)) {
			continue;
		}
		if (memcmp(buf, PACKIMG_TAG, 12))
			continue;
		else{
			//			printk("------- the flash image has PACKIMG_TAG ----------\n");
		}
		/* We found the tag, check for the supported file system. */
		squashfsb = (struct squashfs_super_block *)(buf + sizeof(struct packtag));
		if (squashfsb->s_magic == SQUASHFS_MAGIC_LZMA)
		{
			//		printk("------ squashfs filesystem found at offset %x----------\n", off);
			ptag = (struct packtag *)buf;
			rtl8196_parts1[0].offset = off + 32;
			rtl8196_parts1[0].size = ntohl(ptag->size);		
			//		printk("--------ptag->size==%x---------\n", ntohl(ptag->size));
			return  rtl8196_parts1;
		}
	}

	printk(KERN_NOTICE "%s: Couldn't find valid rootfs image!\n", mtd->name);
	return NULL;
}

#if LINUX_VERSION_CODE < 0x20212 && defined(MODULE)
#define init_rtl8196_map init_module
#define cleanup_rtl8196_map cleanup_module
#endif

#define mod_init_t  static int __init
#define mod_exit_t  static void __exit

mod_init_t init_rtl8196_map(void)
{
        int i,chips;
#ifdef SIZE_REMAINING
	struct mtd_partition *last_partition;
#endif
   	chips = 0;
	for (i=0;i<MAX_SPI_CS;i++) {
		simple_map_init(&spi_map[i]);
		my_sub_mtd[i] = do_map_probe(spi_map[i].name, &spi_map[i]);

		if (my_sub_mtd[i]) {
			my_sub_mtd[i]->owner = THIS_MODULE;
			chips++;
			//printk("%s, %d, i=%d, chips=%d\n", __FUNCTION__, __LINE__, i, chips);
		}
	}
	
	#ifdef CONFIG_MTD_CONCAT
	if (chips == 1) 
		mymtd = my_sub_mtd[0];
	else 		
		{
			//printk("%s, %d\n, size=0x%x\n", __FUNCTION__, __LINE__, my_sub_mtd[0]->size);
			mymtd = mtd_concat_create(&my_sub_mtd[0], chips,"flash_concat");
			//printk("%s, %d, size=0x%x\n", __FUNCTION__, __LINE__, (mymtd->size));
		}
	
	if (!mymtd) {
		printk("Cannot create flash concat device\n");
		return -ENXIO;
	}
	#endif
	
#ifdef SIZE_REMAINING
#ifdef CONFIG_MTD_CONCAT
		last_partition = &rtl8196_parts1[ ARRAY_SIZE(rtl8196_parts1) - 1 ];
		if( last_partition->size == SIZE_REMAINING ) {
			if( last_partition->offset > mymtd->size ) {
				printk( "Warning: partition offset larger than mtd size\n" );
			}else{
				last_partition->size = mymtd->size - last_partition->offset;
			}
#ifdef DEBUG_MAP
			printk(KERN_NOTICE "last_partition size: 0x%x\n",last_partition->size );
#endif
		}
#else
		//for (i=0;i<chips;i++) 
		{
				last_partition = &rtl8196_parts1[ ARRAY_SIZE(rtl8196_parts1) - 1 ];
				if( last_partition->size == SIZE_REMAINING ) {
					if( last_partition->offset > my_sub_mtd[0]->size ) {
						printk( "Warning: partition offset larger than mtd size\n" );
					}else{
						last_partition->size = my_sub_mtd[0]->size - last_partition->offset;
					}
#ifdef DEBUG_MAP
					printk(KERN_NOTICE "last_partition size: 0x%x\n",last_partition->size );
#endif
				}
				last_partition = &rtl8196_parts2[ ARRAY_SIZE(rtl8196_parts2) - 1 ];
				if( last_partition->size == SIZE_REMAINING ) {
					if( last_partition->offset > my_sub_mtd[1]->size ) {
						printk( "Warning: partition offset larger than mtd size\n" );
					}else{
						last_partition->size = my_sub_mtd[1]->size - last_partition->offset;
					}
		#ifdef DEBUG_MAP
					printk(KERN_NOTICE "last_partition size: 0x%x\n",last_partition->size );
		#endif
				}
		}
#endif
#endif

	
	#ifdef CONFIG_MTD_CONCAT
	add_mtd_partitions(mymtd, rtl8196_parts1, ARRAY_SIZE(rtl8196_parts1));
	#ifdef DEBUG_MAP
    	printk(KERN_NOTICE "name=%s, size=0x%x\n", mymtd->name, mymtd->size);
	#endif
	#else
	if (my_sub_mtd[0]) {
		if (init_mtd_partitions(my_sub_mtd[0], WINDOW_SIZE) && ARRAY_SIZE(rtl8196_parts1)){
			add_mtd_partitions(my_sub_mtd[0], rtl8196_parts1, ARRAY_SIZE(rtl8196_parts1));
		}
	#ifdef DEBUG_MAP
    	printk(KERN_NOTICE "name=%s, size=0x%x\n", my_sub_mtd[0]->name,
					 my_sub_mtd[0]->size);
	#endif
	}
#ifdef CONFIG_RTL_TWO_SPI_FLASH_ENABLE	
	if (my_sub_mtd[1]) {
		add_mtd_partitions(my_sub_mtd[1], rtl8196_parts2, 
					ARRAY_SIZE(rtl8196_parts2));
	#ifdef DEBUG_MAP
    	printk(KERN_NOTICE "name=%s, size=0x%x\n", my_sub_mtd[1]->name,
					 my_sub_mtd[1]->size);
	#endif
	}
#endif
	#endif

	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, 0);
	return 0;
}

mod_exit_t cleanup_rtl8196_map(void)
{
	int i;

	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
	}

	for(i=0; i<MAX_SPI_CS; i++)
	{
		if (my_sub_mtd[i])
		{
			del_mtd_partitions(my_sub_mtd[i]);
			map_destroy(my_sub_mtd[i]);
		}
	}
}

MODULE_LICENSE("GPL");
module_init(init_rtl8196_map);
module_exit(cleanup_rtl8196_map);

