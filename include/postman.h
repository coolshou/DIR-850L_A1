
#ifndef __POSTMAN_HEADER__
#define __POSTMAN_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/**********************************************************************/
#define SIGNATURE_FILE		"/etc/config/image_sign"
#define XML_TEMP_FILE		"/var/tmp/fatladytemp.xml"
#define WAND_SCRIPT			"/etc/scripts/php_cmd/wand.php"
#define PHPPATH			    "/etc/scripts/php_cmd/cmd"
#define BUFF_MAX_LEN		1024
#define NODES_LEN			5
#define DO_SETCFG           1
#define DO_DELCFG           2
#define DO_ACTIVATE         1
#define DO_SAVE             1
#define DONT_SETCFG         0
#define DONT_ACTIVATE       0
#define DONT_SAVE           0
/* Form data, which have the name/value pair */
const char * reatwhite(char * ptr);
int postman(int setcfg,int activate,int save, char * reason,const char * service,const char * realnode,const char * value);
//void GetPath( char * reason,int reasonsize,const char * key);
void GetPathByTaget( char * reason,int reasonsize,const char * base,const char * node,const char * target,const char * value,int create);
int GetCfg(char * header,const char * service,char * reason);
void SetValue(int setcfg,const char * header,const char * realnode,const char * value);
int PassValue(int setcfg,int activate,int save, const char * header, char * reason);
#endif
