#include "libbb.h"
#include "imghdr.h"
#include "lrgbin.h"
#include "rgdb.h"
#ifdef CONFIG_FEATURE_HTTPD_AUTH_MD5
#include "md5.h"
#else
#include "../../../include/md5.h"
#endif
#include <syslog.h>
#include "../../../include/asyslog.h"

char g_signature[50];

static unsigned long _cpu_to_le(unsigned long value)
{
	static int swap = -1;
	static unsigned long patt = 0x01020304;
	static unsigned char * p = (unsigned char *)&patt;
	
	if (swap == -1)
	{
		if (p[0] == 0x04 && p[1] == 0x03 && p[2] == 0x02 && p[3] == 0x01) swap=0;
		else swap = 1;
	}
	if (swap)
	{
		return (((value & 0x000000ff) << 24) |
				((value & 0x0000ff00) << 8) |
				((value & 0x00ff0000) >> 8) |
				((value & 0xff000000) >> 24));
	}
	return value;
}

int v2_check_image_block(const char * image, int size)
{
	int offset = sizeof(imghdr2_t);
	imgblock_t * block = NULL;
	MD5_CTX ctx;
	unsigned char digest[16];

	while (offset < size)
	{
		block = (imgblock_t *)&image[offset];

		//printf("Image header (0x%08x):\n", offset);
		//printf("  magic  : 0x%08x\n", block->magic);
		//printf("  size   : %d (0x%x)\n", block->size, block->size);
		//printf("  offset : 0x%08x\n", block->offset);
		//printf("  devname: \'%s\'\n", block->devname);
		//printf("  digest : "); DBYTES(block->digest, 16);
		
		if (block->magic != _cpu_to_le(IMG_V2_MAGIC_NO))
		{
			//printf("Wrong Magic in header !\n");
			break;
		}
		if (offset + sizeof(imgblock_t) + block->size > size)
		{
			//printf("Size out of boundary !\n");
			break;
		}

		/* check MD5 digest */
		MD5Init(&ctx);
		MD5Update(&ctx, (unsigned char *)&block->offset, sizeof(block->offset));
		MD5Update(&ctx, (unsigned char *)block->devname, sizeof(block->devname));
		MD5Update(&ctx, (unsigned char *)&block[1], block->size);
		MD5Final(digest, &ctx);

		if (memcmp(digest, block->digest, 16)!=0)
		{
			//printf("MD5 digest mismatch !\n");
//			printf("digest caculated : "); DBYTES(digest, 16);
//			printf("digest in header : "); DBYTES(block->digest, 16);
			break;
		}


		/* move to next block */
		offset += sizeof(imgblock_t);
		offset += block->size;

		//printf("Advance to next block : offset=%d(0x%x), size=%d(0x%x)\n", offset, offset, size, size);
	}
	if (offset == size) return 0;


//	printf("illegal block found at offset %d (0x%x)!\n", offset, offset);
//	printf("  offset (%d) : \n", offset); DBYTES((unsigned char *)block, 16);
/*FW_log_20100119, start {, Added by log_luo*/                        
   bb_error_msg("Image file is not acceptable. Please check download file is right.");
#if ELBOX_PROGS_GPL_SYSLOGD 
   syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Image file is not acceptable. Please check download file is right.");
#endif 
/*FW_log_20100119, End }, Added by log_luo*/
	return -1;
}

int v3_check_image_block(const char * image, int size)
{
	int offset = sizeof(imghdr2_t);
	imgblock_tv3 * block = NULL;
	MD5_CTX ctx;
	unsigned char digest[16];
#ifdef ELBOX_FIRMWARE_FLASH_SP_MX
	unsigned char xmldb_buff[10];
	int           bcflag0, imgflag0;
#endif /*ELBOX_FIRMWARE_FLASH_SP_MX*/

	while (offset < size)
	{
		block = (imgblock_tv3 *)&image[offset];

		//printf("Image header (0x%08x):\n", offset);
		//printf("  magic  : 0x%08x\n", block->magic);
		//printf("  size   : %d (0x%x)\n", block->size, block->size);
		//printf("  offset : 0x%08x\n", block->offset);
		//printf("  devname: \'%s\'\n", block->devname);
		//printf("  digest : "); DBYTES(block->digest, 16);

		if (block->magic != _cpu_to_le(IMG_V3_MAGIC_NO))
		{
			//printf("Wrong Magic in header !\n");
			break;
		}

#ifdef ELBOX_FIRMWARE_FLASH_SP_MX
		memset(xmldb_buff, 0x0, sizeof(xmldb_buff));
		RGDBGET(xmldb_buff, sizeof(xmldb_buff)-1, "/runtime/nvram/flash");
		if(strlen(xmldb_buff)!=0)
		{
			bcflag0 = atoi(xmldb_buff);
#ifdef B_ENDIAN
		imgflag0 = ((block->flag[0]&0xff000000)>>24)|
		           ((block->flag[0]&0x00ff0000)>>8)|
		           ((block->flag[0]&0x0000ff00)<<8)|
		           ((block->flag[0]&0x000000ff)<<24);
#else
		imgflag0 = block->flag[0];
#endif /*B_ENDIAN*/
			if(bcflag0 > imgflag0)
			{
				printf("New board can't support old firmware\n");
				break;
			}
		}
#endif /*ELBOX_FIRMWARE_FLASH_SP_MX*/

		if (offset + sizeof(imgblock_tv3) + block->size > size)
		{
			//printf("Size out of boundary !\n");
			break;
		}

		/* check MD5 digest */
		MD5Init(&ctx);
		MD5Update(&ctx, (unsigned char *)&block->offset, sizeof(block->offset));
		MD5Update(&ctx, (unsigned char *)block->devname, sizeof(block->devname));
		MD5Update(&ctx, (unsigned char *)&block[1], block->size);
		MD5Final(digest, &ctx);

		if (memcmp(digest, block->digest, 16)!=0)
		{
			//printf("MD5 digest mismatch !\n");
			//printf("digest caculated : "); DBYTES(digest, 16);
			//printf("digest in header : "); DBYTES(block->digest, 16);
			break;
		}


		/* move to next block */
		offset += sizeof(imgblock_tv3);
		offset += block->size;

		//printf("Advance to next block : offset=%d(0x%x), size=%d(0x%x)\n", offset, offset, size, size);
	}
	if (offset == size) return 0;


	//printf("illegal block found at offset %d (0x%x)!\n", offset, offset);
	//printf("  offset (%d) : \n", offset); DBYTES((unsigned char *)block, 16);

	return -1;
}

int v2_image_check(const char * image, int size)
{
	imghdr2_t * v2hdr = (imghdr2_t *)image;
	unsigned char signature[MAX_SIGNATURE];
	
	int i;
	if (v2hdr && size > sizeof(imghdr2_t) && v2hdr->magic == _cpu_to_le(IMG_V2_MAGIC_NO))
	{
		/* check if the signature match */
		//printf("check image signature !\n");
		memset(g_signature,0,50);/*ftp_tftp_FW_CG_20100119 log_luo*/
        RGDBGET(g_signature, 50, "/runtime/layout/image_sign");/*ftp_tftp_FW_CG_20100119 log_luo*/
		
		memset(signature, 0, sizeof(signature));
		strncpy(signature, g_signature, sizeof(signature));
	

		//printf("  expected signature : [%s]\n", signature);
		//printf("  image signature    : [%s]\n", v2hdr->signature);

		if (strncmp(signature, v2hdr->signature, MAX_SIGNATURE)==0)
			return v2_check_image_block(image, size);
		/* check if the signature is {boardtype}_aLpHa (ex: wrgg02_aLpHa, wrgg03_aLpHa */
		for (i=0; signature[i]!='_' && i<MAX_SIGNATURE; i++);
		if (signature[i] == '_')
		{
			signature[i+1] = 'a';
			signature[i+2] = 'L';
			signature[i+3] = 'p';
			signature[i+4] = 'H';
			signature[i+5] = 'a';
			signature[i+6] = '\0';

			//printf("  try this signature : [%s]\n", signature);

			if (strcmp(signature, v2hdr->signature) == 0)
				return v2_check_image_block(image, size);
		}
	}
/*FW_log_20070412, start {, Added by log_luo*/                        
                        bb_error_msg("Image file is not acceptable. Please check download file is right.");
#if ELBOX_PROGS_GPL_SYSLOGD 
                      	syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Image file is not acceptable. Please check download file is right.");
#endif 
/*FW_log_20070412, End }, Added by log_luo*/	
	
	return -1;
}

int v3_image_check(const char * image, int size)
{
	imghdr2_t * v2hdr = (imghdr2_t *)image;
	unsigned char signature[MAX_SIGNATURE];
	int i;
	if (v2hdr && size > sizeof(imghdr2_t) && v2hdr->magic == _cpu_to_le(IMG_V3_MAGIC_NO))
	{
		/* check if the signature match */
		//printf("check image signature !\n");
		
		memset(g_signature,0,50);/*ftp_tftp_FW_CG_20100119 log_luo*/
        RGDBGET(g_signature, 50, "/runtime/layout/image_sign");/*ftp_tftp_FW_CG_20100119 log_luo*/

		memset(signature, 0, sizeof(signature));
		strncpy(signature, g_signature, sizeof(signature));

		//printf("  expected signature : [%s]\n", signature);
		//printf("  image signature    : [%s]\n", v2hdr->signature);

		if (strncmp(signature, v2hdr->signature, MAX_SIGNATURE)==0)
			return v3_check_image_block(image, size);

		/* check if the signature is {boardtype}_aLpHa (ex: wrgg02_aLpHa, wrgg03_aLpHa */
		for (i=0; signature[i]!='_' && i<MAX_SIGNATURE; i++);
		if (signature[i] == '_')
		{
			signature[i+1] = 'a';
			signature[i+2] = 'L';
			signature[i+3] = 'p';
			signature[i+4] = 'H';
			signature[i+5] = 'a';
			signature[i+6] = '\0';

			//printf("  try this signature : [%s]\n", signature);

			if (strcmp(signature, v2hdr->signature) == 0)
				return v3_check_image_block(image, size);
		}
	}
	return -1;
}

int v2_burn_image(const char * image, int size)
{
	int offset = sizeof(imghdr2_t);
	imgblock_t * block;
	imghdr2_t headcheck;
	FILE * fh;

	printf("v2_burn_image >>>>\n");

	while (offset < size)
	{
		block = (imgblock_t *)&image[offset];

		printf("burning image block.\n");
		printf("  size    : %d (0x%x)\n", (unsigned int)block->size, (unsigned int)block->size);
		printf("  devname : %s\n", block->devname);
		printf("  offset  : %d (0x%x)\n", (unsigned int)block->offset, (unsigned int)block->offset);

	    //joel add ,before we write the mtd block ,umout first,some kernel can not write when device busy
		{
			char umount_buff[256];
			sprintf(umount_buff,"umount %s ",block->devname);
			system(umount_buff);
		}
#if 1
		fh = fopen(block->devname, "w+");
		if (fh == NULL)
		{
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
			syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Fail to write file!");
#endif		
 /* papa add end for syslog 2007/03/30 */
			printf("Failed to open device %s\n", block->devname);
			return -1;
		}
#if 1//joel auto check the header in flash,we can not handle the offset non zero....
		if(fread(&headcheck,sizeof(imghdr2_t),1,fh) && headcheck.magic == _cpu_to_le(IMG_V2_MAGIC_NO) && block->offset==0)
		{
		fseek(fh, block->offset, SEEK_SET);
			printf("head in flash\n");
			//write header 1
			fwrite((const void *)image, 1, sizeof(imghdr2_t), fh);
			//write header 2
			fwrite((const void *)&block[0], 1, sizeof(imgblock_t), fh);
		}
		else
#endif
		{
		   printf("No header in images\n");
		   fseek(fh, block->offset, SEEK_SET);
		}
		fwrite((const void *)&block[1], 1, block->size, fh);
		fflush(fh);	//log_200910,fix "SQUASHFS error: lzma returned unexpected result 0x1" when reboot.
		fclose(fh);
#endif
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
		syslog(ALOG_AP_SYSACT|LOG_NOTICE,"[SYSACT]Firmware update success");	
#endif 
 /* papa add end for syslog 2007/03/30 */
#ifdef ELBOX_PROGS_GPL_NET_SNMP
//sendtrap("[SNMP-TRAP][Specific=12]");
#endif
		printf("burning done!\n");

		/* move to next block */
		offset += sizeof(imgblock_t);
		offset += block->size;
	}
	//upload_file.flag = 0;

	return 0;
}

int v3_burn_image(const char * image, int size)
{
	int offset = sizeof(imghdr2_t);
	imgblock_tv3 * block;
	imghdr2_t headcheck;
	FILE * fh;

    RGDBSET("/runtime/update/status","IN_PROCESS");
	printf("v3_burn_image >>>>\n");

	while (offset < size)
	{
		block = (imgblock_tv3 *)&image[offset];

		printf("burning image block.\n");
		printf("  size    : %d (0x%x)\n", (unsigned int)block->size, (unsigned int)block->size);
		printf("  devname : %s\n", block->devname);
		printf("  offset  : %d (0x%x)\n", (unsigned int)block->offset, (unsigned int)block->offset);

		//joel add ,before we write the mtd block ,umout first,some kernel can not write when device busy
		{
			char umount_buff[256];
			sprintf(umount_buff,"umount %s  >/dev/null 2>&1",block->devname);
			system(umount_buff);
		}
#if 1
		fh = fopen(block->devname, "w+");
		if (fh == NULL)
		{
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
			syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Fail to write file!");
#endif
 /* papa add end for syslog 2007/03/30 */
			printf("Failed to open device %s\n", block->devname);
			return -1;
		}
#if 1//joel auto check the header in flash,we can not handle the offset non zero....
		if(fread(&headcheck,sizeof(imghdr2_t),1,fh) && headcheck.magic == _cpu_to_le(IMG_V3_MAGIC_NO) && block->offset==0)
		{
        fseek(fh, block->offset, SEEK_SET);
		printf("head in flash\n");
		//write header 1
		fwrite((const void *)image, 1, sizeof(imghdr2_t), fh);
		//write header 2
		fwrite((const void *)&block[0], 1, sizeof(imgblock_tv3), fh);
		}
		else
#endif
		{
			printf("No header in images\n");
			fseek(fh, block->offset, SEEK_SET);
		}
		fwrite((const void *)&block[1], 1, block->size, fh);
		fflush(fh);	//log_200910,fix "SQUASHFS error: lzma returned unexpected result 0x1" when reboot.
		fclose(fh);
#endif
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
		syslog(ALOG_AP_SYSACT|LOG_NOTICE,"[SYSACT]Firmware update success");
#endif
 /* papa add end for syslog 2007/03/30 */
#ifdef ELBOX_PROGS_GPL_NET_SNMP
//sendtrap("[SNMP-TRAP][Specific=12]");
#endif
		printf("burning done!\n");

#if ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
            system("xmldbc -t upfw:15:reboot &");
#endif
		/* move to next block */
		offset += sizeof(imgblock_tv3);
		offset += block->size;
	}
	//upload_file.flag = 0;
	/*phelpsll add update success message. 2009/08/11*/
    printf("Please reboot device!\n");
	return 0;
}

int burn_image(const char * image, int size)
{
	FILE * fh;

	//printf("burn_image >>>>\n");
	fh = fopen("/dev/mtdblock/1", "w+");
	if (fh == NULL)
	{
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
			syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Fail to write file!");
#endif
 /* papa add end for syslog 2007/03/30 */
		//printf("Failed to open device %s\n", "/dev/mtdblock/1");
		return -1;
	}
	fwrite((const void *)image, 1, size, fh);
	fclose(fh);
 /* papa add start for syslog 2007/03/30 */
#if ELBOX_PROGS_GPL_SYSLOGD_AP
	syslog(ALOG_AP_SYSACT|LOG_NOTICE,"[SYSACT]Firmware update success");
#endif
 /* papa add end for syslog 2007/03/30 */
	//printf("burning done!\n");

	//upload_file.flag = 0;

	return 0;
}

#define MAXACLNUM 256
#define MAXLINESIZ 18
static int aclmode=0; //o allow,1 reject

void ftpSaveacl(char *bufout)
{
	char buf1[128];
	char temp[128];
	int  i;

   //band selection for dual band ap 2690 start
	int band =1;
    	char path[100],buff[30];
       memset(path,0,100);
	memset(buff,0,30);
	sprintf(path,"%s","/runtime/wireless/bandselection");
      	RGDBGET(buff,1, path);
	if(strlen(buff)!=0){
	band = atol(buff);
	}
   //band selection for dual band ap 2690 end
   
       memset(buf1,0,128);
       memset(temp,0,128);
	RGDBGET(buf1,128,"/wlan/inf:%d/acl/mode",band);
	if(!strcasecmp(buf1,"0"))
	              sprintf(temp,"%s\n","OFF");
	else if(!strcasecmp(buf1,"1"))
	              sprintf(temp,"%s\n","ALLOW");
	else if(!strcasecmp(buf1,"2"))
	              sprintf(temp,"%s\n","DENY");
       else
	              sprintf(temp,"%s\n","OFF");

	strcat(bufout,temp);
        for(i=0;i<MAXACLNUM;i++)
      	{
         	memset(buf1,0,128);
		memset(temp,0x0,128);
	      sprintf(buf1,"/wlan/inf:%d/acl/mac:%d",band,(i+1));
	      RGDBGET(temp,128,buf1); 
		  strcat(bufout,temp);
            if(strlen(temp))
            	{
            	   strcat(bufout,"\n");

            	}else
            	{
            	    break;
            	}
      	}
	
}

int IsValidMac(char* buff,int linesize)
{
	int i=0;
	if(!buff||linesize!=17)
		return -1;
    /*allenxiao add check is it a legal Mac 2012.5.24 */
   if((buff[1] < '9' && buff[1]%2 != 0) || (buff[1] > 'A' && buff[1]%2 == 0))
   	return -1;
   /*end allenxiao add 2011.1.7*/
   for(i=0;i<MAXLINESIZ-1;i++)
   {

	 if(!((buff[i]>='A'&&buff[i]<='F')||(buff[i]>='0'&&buff[i]<='9')||buff[i]==':'))
	  {
	 	return -1;
	  }
   	 if(i%3==2)
   	 {
   	       if(buff[i]!=':')
   	     	{
		   	return -1;
   	     	}
   	  }
	   else
	  {
	        if(!((buff[i]>='A'&&buff[i]<='F')||(buff[i]>='0'&&buff[i]<='9')))
	   	 {
		   	return -1;
	   	 }
	   }
	  }
   return 0;
}

int IsExist(char * mac,char * macList,int aclnum)
{
	int i=0;
	if(!mac||!macList)
		return 0;
	for(i=0;i<aclnum*MAXLINESIZ;i=i+MAXLINESIZ)
	{
		if(!memcmp(&mac[0],&macList[i],MAXLINESIZ))
		{
			return 1;
		}
	}
	return 0;
}

int ReadACLFromFile(char * fileName,char *buffer)
{
	FILE *pFile;
   int aclnum=0;
   int state=1;
   char linebuf[MAXLINESIZ];
   int linesize=0;
   int totalnum=0; 
   char **arrAcl;
   int ignore=0;
   state = 1;
   arrAcl=(char**)buffer;
   /*allenxiao add MIB info: set default value.2011.1.10*/
   RGDBSET("/runtime/update/status","CORRECT");
   if((pFile=fopen("/var/acl.tem","r")))
   	{
   	   while (state&&aclnum<MAXACLNUM&&totalnum<(MAXACLNUM*200)){
		int c;
		totalnum++;
		c = getc(pFile); 
		if (c== EOF) 
		{
		    ignore=0;
		    state = 0;
		    break;
		 }else if (c == 10||c ==  13)
		 {
		      if(linesize>0)
			{
			    linebuf[linesize]='\0';
			    if(linesize==3&&!memcmp(linebuf,"OFF",3))
			    {
			         aclmode=0;
		           }
			    else if (linesize==5&&!memcmp(linebuf,"ALLOW",5))
			    {
				  aclmode=1;
			    }
			     else if (linesize==4&&!memcmp(linebuf,"DENY",4))
			     {
			  	  aclmode=2;
			     }
			     else
			    {
			          if(IsValidMac(linebuf,linesize)!=0)
				   {
				        /*allenxiao add MIB info.2012.5.24*/
						RGDBSET("/runtime/update/status","WRONG_ACL");
				       fclose(pFile);
				 	return -1;
				    }else
				    {
				         if(!IsExist(linebuf,buffer,aclnum))
				 	  {
					       memcpy(&buffer[aclnum*MAXLINESIZ],&linebuf[0],MAXLINESIZ);
				 	       aclnum++;
				 	    }
				 	}
				   }
				 memset(linebuf,0,sizeof(linebuf));
				
			     }
			linesize=0;
		       ignore=0;
		}
		else
		{
		    if(c == ';') ignore=1;   /*treat ';' as a commentary splitter*/
		    if(c=='-')c=':';
		    if(c>=97&&c<=122)c=c-32; //UpperCase	
		    if(!ignore&&c!=32&&linesize<(MAXLINESIZ-1))//ignore both space and the char(s) after ';' 
		    {
		        linebuf[linesize]=c;
			 linesize++;
		     }
		  }
   	  }
	
       fclose(pFile);
	if(aclnum>MAXACLNUM)
	   aclnum=-1;

	return aclnum;
   }
}


int UpdateAclList(char * buffer,int aclnum)
{
   int i=0;
   char path[100],buff[30];
   int aclno=0;
   
   //band selection for dual band ap 2690 start
	int band =1;
       memset(path,0,100);
	memset(buff,0,30);
	sprintf(path,"%s","/runtime/wireless/bandselection");
      	RGDBGET(buff,1, path);
	if(strlen(buff)!=0){
	band = atol(buff);
	}
   //band selection for dual band ap 2690 end
 	
   if((!buffer)||aclnum<0||aclnum>MAXACLNUM){
   	return -1;
   	}
   memset(path,0,100);
   memset(buff,0,30);
   //clear the old acl list
      for(i=0;i<MAXACLNUM;i++)
      	{
      	     memset(path,0,100);
            memset(buff,0,30);
	     sprintf(path,"/wlan/inf:%d/acl/mac:%d",band,1);
	   // printf("path:%s\n",path);
      	     RGDBGET(buff,10, path);
	  //  printf("buff:%s\n",buff);
            if(strlen(buff))
            	{
            	  RGDBDEL(path);
            	}else
            	{
            		break;
            	}
      	}
   //set acl mode,0:off,1:allow,2:deny
   sprintf(path,"/wlan/inf:%d/acl/mode",band);
   sprintf(buff,"%d",aclmode);
   RGDBSET(path,buff);
 //set acl list
   for(i=0;i<aclnum*MAXLINESIZ;i=i+MAXLINESIZ)
   	{		
            memset(path,0,100);
            memset(buff,0,30);
            sprintf(path,"/wlan/inf:%d/acl/mac:%d",band,(aclno+1));
            sprintf(buff,"%s",(char*)&buffer[i]);
            RGDBSET(path,buff);
	     aclno++;
	}
		
 memset(path,0,100);
 memset(buff,0,30);
 sprintf(path,"/runtime/sys/fw_size");
 sprintf(buff,"%d",(aclno+1));
  RGDBSET(path,buff);
  return 0;
}


int ftpUpload_acl(char *input,int length)
{
	char *pInput=input;
	FILE *pFile;
	int filelen = length;
       char* aclFileName="/var/acl.tem";
       char* buffer=NULL;
	int aclnum=0;
//2009_07_02 sandy++++++
#ifdef CONFIG_AP_NEAP_AP_ARRAY	
	int cfg_version_buf;
	char temp_buf[128];
#endif	
//2009_07_02 sandy-----
       if((pFile=fopen(aclFileName,"w")))
	{
		fwrite(pInput, 1, filelen, pFile);
		fclose(pFile);
		//Read setting to buffer;
		buffer = malloc(MAXACLNUM*MAXLINESIZ);
              if(buffer == NULL)
                   goto error;
		if((aclnum=ReadACLFromFile(aclFileName,buffer))<0)
		     {
		      goto error;
		     }
		if(UpdateAclList(buffer,aclnum)!=0)
		    {
	              goto error;
		    }
		if(buffer) free(buffer);
		unlink(aclFileName);
		//rlt_page(p, "sys_stunnel_process.php");
		//RGDBSET( "/runtime/web/redirect_next_page","sys_stunnel_process.php");
		//redirect_page(p, "/www/sys/redirectlink.php");
		      
		//sync_time();
		//upload_file.flag=0;
		//upload_file.uptime=current_uptime;
//-------------------------------------------
//ap array config version conuter 2009_07_02 sandy+++++++		
#ifdef CONFIG_AP_NEAP_AP_ARRAY
//	status_buf =0; //2009_9_2 sandy
//        RGDBGET(temp_buf,sizeof(temp_buf), "/wlan/inf:1/APARRAY_enable");
//	status_buf = atol(temp_buf);
//	if (status_buf == 1)
//	{	
				memset(temp_buf, 0x0, sizeof(temp_buf));
				RGDBGET(temp_buf,sizeof(temp_buf), "/wlan/inf:1/aparray_cfg_version");
				cfg_version_buf =0; 
				cfg_version_buf = atol(temp_buf);
				cfg_version_buf++;
				memset(temp_buf, 0x0, sizeof(temp_buf));
				sprintf( temp_buf, "%d", cfg_version_buf );
				RGDBSET( "/wlan/inf:1/aparray_cfg_version",temp_buf);
//	}		
#endif	
//-------------------------------------------

		return 0;
	 }


	error:
	free(buffer);
	//unlink(aclFileName);
	//rlt_page(p, "sys_stunnel_error.php");
	//RGDBSET( "/runtime/web/redirect_next_page","sys_stunnel_error.php");
	//redirect_page(p, "/www/sys/redirectlink.php");
	return -1;
}

