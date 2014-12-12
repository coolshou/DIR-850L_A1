/* vi: set sw=4 ts=4: */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../include/elbox_config.h"
#include <dtrace.h>
#include <unistd.h>
#include <postman.h>

#include <libxmldbc.h>
#define IS_WHITE(x) ((x) == ' ' || (x)=='\t' || (x) == '\n' || (x) == '\r')
#define ERRBREAK(R)		{ reason = (R); ret = -1; d_error("%s: ret = %d, reason = %s\n",__func__, ret, reason); break; }
const char * reatwhite(char * ptr)
{
	int i;

	if (ptr==NULL) return NULL;
	i = strlen(ptr)-1;
	while (i >= 0 && IS_WHITE(ptr[i])) ptr[i--] = '\0';
	return ptr;
}

int postman(int setcfg,int activate,int save, char * reason,const char * service,const char * realnode,const char * value)
{
	int session;
	int ret=0;
	FILE * fh;
	char buff[BUFF_MAX_LEN] = {0};
	char *node;
	char cmd[BUFF_MAX_LEN] = {0};
	char retphpbuff[BUFF_MAX_LEN] = {0};
	char signature[128] = {0};
	pid_t pid;
	pid = getpid();
	srand((unsigned) time(NULL));/*for get random session*/
	session=random()%10000;

	do
	{
		if(setcfg==DONT_SETCFG)
		{
			strcpy(retphpbuff,"1");
			break;	
		}
		/* read signature */
		fh = fopen(SIGNATURE_FILE, "r");
		if (fgets(signature, sizeof(signature), fh)==NULL)
			ERRBREAK("unable to read signature!");
		fclose(fh);
		fh = NULL;
		reatwhite(signature);
		d_dbg("%s: signature=[%s]\n", __func__, signature);
		/* We need a string object to construct the php command. */
		/* Remove the old postxml data of runtime node */
		sprintf(buff, "%s/%d/module", ELBOX_ARIES_SESSION_BASE,session);
		xmldbc_del(NULL, 0, buff);
		node = strcat(buff,realnode);
		/* Save received xml into the runtime node */
		fh = fopen(XML_TEMP_FILE, "w");
		if (!fh)							
			ERRBREAK("unable to open temp file.");
		/* Lock file to avoid accessing at the same time */
		do
		{
			ret = lockf(fileno(fh), F_TEST, 0);
			d_dbg("%s: lockf = [%d]\n", __func__, ret);
		}
		while(ret < 0);
	//{
			/* unable to lockf, another instance should running already. */
			//strcpy(reason,"BUSY");
			//return ret;
			//break;
		//}
		/* only one instance can run at the same time. */
		lockf(fileno(fh), F_LOCK, 0);
		lxmldbc_system("xmldbc -P /etc/scripts/phpcontrol/getcfg.php -V GETCFG_SVC=%s -V SESSION=%d -V SIGNATURE=%s > %s ",service,session,signature,XML_TEMP_FILE);
		xmldbc_read(NULL, 2, XML_TEMP_FILE);
		lockf(fileno(fh), F_ULOCK, 0);
		fclose(fh);
		remove(XML_TEMP_FILE);
		if(setcfg==DO_SETCFG)
		{
			xmldbc_set(NULL,0,node,value);
		}
		else if(setcfg==DO_DELCFG)
		{
			xmldbc_del(NULL, 0, node);
		}
		
		
		/* Calling fatlady to check the settings in the runtime node */
		sprintf(cmd,"/etc/scripts/php_cmd/fatlady.php\nprefix=%s/%d",ELBOX_ARIES_SESSION_BASE,session);
		xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff,sizeof(retphpbuff));
	} while(0);
	if(ret==0)
	{
		if(strcmp(retphpbuff,"1")!=0)
		{
			ret=-1;
			strcpy(reason,retphpbuff);
		}
		else
		{
			if (setcfg==DO_SETCFG || setcfg==DO_DELCFG)
			{
				/* Create php command. */
				sprintf(cmd, "%s\nACTION=SETCFG\nPREFIX=%s/%d",
					WAND_SCRIPT, ELBOX_ARIES_SESSION_BASE, session);
				d_dbg("%s: dophp(%s)\n",__func__,cmd);
				/* Execute php
				 * Pass a buffer to catch any output from the php.
				 * There will be no output from the php and we will ignore
				 * the output. */
				xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff, sizeof(retphpbuff));
			}
			if (activate==DO_ACTIVATE)
			{
				/* Create php command. */
				sprintf(cmd, "%s\nACTION=ACTIVATE\nPREFIX=%s/%d\nSHELL=/var/run/wand_activate_%d.sh",
					WAND_SCRIPT, ELBOX_ARIES_SESSION_BASE, session, pid);
				d_dbg("%s: dophp(%s)\n",__func__,cmd);
				/* Execute php
				 * Pass a buffer to catch any output from the php.
				 * There will be no output from the php and we will ignore
				 * the output. */
				xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff, sizeof(retphpbuff));
				sprintf(cmd, "sh /var/run/wand_activate_%d.sh", pid);
				lxmldbc_system(cmd);
			}
			if (save==DO_SAVE)
			{
				/* Save cofiguration to flash */
				d_dbg("%s: run /etc/scripts/dbsave.sh\n",__func__);
				lxmldbc_system("/etc/scripts/dbsave.sh");
			}
		}
	}
	/* Contruct the result. */
	return ret;
}
/*void GetPath( char * reason,int reasonsize,const char * key)
{
	char cmd[BUFF_MAX_LEN] = {0};
	sprintf(cmd,"/etc/scripts/phpcontrol/getpath/%s.php",key);
	xmldbc_ephp_wb(NULL, 0, cmd, reason,reasonsize);
}*/
void GetPathByTaget( char * reason,int reasonsize,const char * base,const char * node,const char * target,const char * value,int create)
{
	char cmd[BUFF_MAX_LEN] = {0};
	sprintf(cmd,"/etc/scripts/php_cmd/GETPATH.php\nBASE=%s\nNODE=%s\nTARGET=%s\nVALUE=%s\nCREATE=%d",base,node,target,value,create);
	xmldbc_ephp_wb(NULL, 0, cmd, reason,reasonsize);
}
int GetCfg(char * header,const char * service,char * reason)
{
	int session;
	int ret=0;
	FILE * fh;
	char signature[128] = {0};
	srand((unsigned) time(NULL));/*for get random session*/
	session=random()%10000;
	/* read signature */
	do
	{
		fh = fopen(SIGNATURE_FILE, "r");
		if (fgets(signature, sizeof(signature), fh)==NULL)
			ERRBREAK("unable to read signature!");
		fclose(fh);
		fh = NULL;
		reatwhite(signature);
		d_dbg("%s: signature=[%s]\n", __func__, signature);
		/* We need a string object to construct the php command. */
		/* Remove the old postxml data of runtime node */
		sprintf(header, "%s/%d", ELBOX_ARIES_SESSION_BASE,session);
		xmldbc_del(NULL, 0, header);
		/* Save received xml into the runtime node */
		fh = fopen(XML_TEMP_FILE, "w");
		if(!fh)							
			ERRBREAK("unable to open temp file.");
		/* Lock file to avoid accessing at the same time */
		do
		{
			ret = lockf(fileno(fh), F_TEST, 0);
			d_dbg("%s: lockf = [%d]\n", __func__, ret);
		}
		while(ret < 0);
		//{
			/* unable to lockf, another instance should running already. */
		//	ERRBREAK("BUSY");
		//}
		/* only one instance can run at the same time. */
		lockf(fileno(fh), F_LOCK, 0);
		lxmldbc_system("xmldbc -P /etc/scripts/php_cmd/getcfg.php -V GETCFG_SVC=%s -V SESSION=%d -V SIGNATURE=%s > %s ",service,session,signature,XML_TEMP_FILE);
		xmldbc_read(NULL, 2, XML_TEMP_FILE);
		lockf(fileno(fh), F_ULOCK, 0);
		fclose(fh);
		remove(XML_TEMP_FILE);	
	}while(0);	
	return ret;
}
void SetValue(int setcfg,const char * header,const char * realnode,const char * value)
{
	char node[BUFF_MAX_LEN] = {0};
	sprintf(node,"%s/module%s",header,realnode);
	if(setcfg==DO_SETCFG)
	{
		xmldbc_set(NULL,0,node,value);
	}
	else if(setcfg==DO_DELCFG)
	{
		xmldbc_del(NULL, 0, node);
	}
}
int PassValue(int setcfg,int activate,int save, const char * header, char * reason)
{
	pid_t pid;
	pid = getpid();
	int ret=0;
	char cmd[BUFF_MAX_LEN] = {0};
	char retphpbuff[BUFF_MAX_LEN] = {0};
	sprintf(cmd,"/etc/scripts/php_cmd/fatlady.php\nprefix=%s",header);
	xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff,sizeof(retphpbuff));
	if(strcmp(retphpbuff,"1")!=0)
	{
		ret=-1;
		strcpy(reason,retphpbuff);
	}
	else
	{
		if (setcfg==DO_SETCFG)
		{
			/* Create php command. */
			sprintf(cmd, "%s\nACTION=SETCFG\nPREFIX=%s",WAND_SCRIPT, header);
			d_dbg("%s: dophp(%s)\n",__func__,cmd);
			/* Execute php
			 * Pass a buffer to catch any output from the php.
			 * There will be no output from the php and we will ignore
			 * the output. */
			xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff, sizeof(retphpbuff));
		}
		if (activate==DO_ACTIVATE)
		{
			/* Create php command. */
			sprintf(cmd, "%s\nACTION=ACTIVATE\nPREFIX=%s\nSHELL=/var/run/wand_activate_%d.sh",
				WAND_SCRIPT, header, pid);
			d_dbg("%s: dophp(%s)\n",__func__,cmd);
			/* Execute php
			 * Pass a buffer to catch any output from the php.
			 * There will be no output from the php and we will ignore
			 * the output. */
			xmldbc_ephp_wb(NULL, 0, cmd, retphpbuff, sizeof(retphpbuff));
			sprintf(cmd, "sh /var/run/wand_activate_%d.sh", pid);
			lxmldbc_system(cmd);
		}
		if (save==DO_SAVE)
		{
			/* Save cofiguration to flash */
			d_dbg("%s: run /etc/scripts/dbsave.sh\n",__func__);
			lxmldbc_system("/etc/scripts/dbsave.sh");
		}
	}
	/* Contruct the result. */
	return ret;
}
