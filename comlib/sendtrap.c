#include <stdlib.h> /*definition of NULL*/
#include <stdio.h> /*memory and string manupulation*/
#include <string.h> /* strncpy() */
#include <sendtrap.h>
#include <libxmldbc.h>
#include <../include/elbox_config.h>


#if 1 //traveller add for encapsulate the rgdb funcation
#define XMLDEBUG   0x01
#define DEBUG           0x0

void xml_debug(int level,char *action,char *node,char *value){
	int xml_debug=0;
	char buf[256];
       memset(buf,0x0,sizeof(buf));
       xmldbc_get_wb(NULL,0,"/runtime/snmp/debug",buf,256);
	//printf("%s\n",buf);
      	xml_debug = atol(buf);
	//printf("xml_debug:%d\n",xml_debug);
	if(level & xml_debug){   //debug by set xmldb outside
                printf("action:%s       node:%s     value:%s\n",action,node,value);
                }
}

void rgdb_del(char *node){
        xml_debug(XMLDEBUG,"Del",node,0);
        xmldbc_del(NULL,0,node);
}

//string get
void rgdb_get(char *node,char *value,int size)
{
        memset(value,0x0,size);
        xmldbc_get_wb(NULL,0,node,value,size);
        xml_debug(XMLDEBUG,"Get",node,value);
}

void rgdb_get_max(char *node,char *value)
{
        memset(value,0x0,256);
        xmldbc_get_wb(NULL,0,node,value,256);
        xml_debug(XMLDEBUG,"Get",node,value);
}

void rgdb_default_get_max(char *node,char *value ,char *def){
        rgdb_get_max(node,value);
        if(strlen(value)==0){
            memset(value,0x0,256);
            memcpy(value,def,strlen(def));
            }
        
}

void rgdb_get_single_index(char *node ,char *value,int index){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index);
        rgdb_get_max(buf,value);
}

void rgdb_get_double_index(char *node ,char *value,int index1,int index2){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index1,index2);
        rgdb_get_max(buf,value);
}

//int get
int int_rgdb_get(char *node){
       char buf[256];
       memset(buf,0x0,sizeof(buf));
       rgdb_get_max(node,buf);
       return atol(buf);
}

int int_default_rgdb_get(char *node,int def){
       int buf;
       char buff[256];
       memset(buff,0x0,sizeof(buff));
       rgdb_get_max(node,buff);
       buf=atol(buff);
       if(strlen(buff)>0)
            return buf;
       else
            return def;
}

int int_rgdb_get_single_index(char *node ,int index){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index);
        return int_rgdb_get(buf);
}

int int_rgdb_get_double_index(char *node ,int index1,int index2){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index1,index2);
        return int_rgdb_get(buf);
}

//string set
void rgdb_set(char *node,char *value)
{
        xml_debug(XMLDEBUG,"Set",node,value);
        xmldbc_set(NULL,0,node,value);
}

void rgdb_set_single_index(char *node ,char *value,int index){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index);
        rgdb_set(buf,value);
}

void rgdb_set_double_index(char *node ,char *value,int index1,int index2){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index1,index2);
        rgdb_set(buf,value);
}

//int set
int int_rgdb_set(char *node,int value){
        char buf[256];
        memset(buf,0x0,256);
        sprintf(buf,"%d",value);
        xml_debug(XMLDEBUG,"Set",node,buf);
        xmldbc_set(NULL,0,node,buf);
}


int int_rgdb_set_single_index(char *node ,int value,int index){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index);
        return int_rgdb_set(buf,value);
}

int int_rgdb_set_double_index(char *node ,int value ,int index1,int index2){
        char buf[256];
        memset(buf,0x0,sizeof(buf));
        sprintf(buf,node,index1,index2);
        return int_rgdb_set(buf,value);
}


//safe funcation for write funcation
void rgdb_set_safe(char *node,char *value,int size){
        char buf[256];
        memset(buf,0x0,256);
        memcpy(buf,value,size);
        xml_debug(XMLDEBUG,"Set",node,buf);
        xmldbc_set(NULL,0,node,buf);
}


void int_rgdb_set_safe(char *node,char *value){
        char buf[256];
        int var;
        var=*(long *)value;
        memset(buf,0x0,256);
        sprintf(buf,"%d",var);
        xml_debug(XMLDEBUG,"Set",node,buf);
        xmldbc_set(NULL,0,node,buf);
}

#endif


void xmldbc_integer_increase(char *node){
    char buf[256];
    int num=0;
    memset(buf,0x0,sizeof(buf));
    xmldbc_get_wb(NULL,0,node,buf,sizeof(buf));
    num=atol(buf);
    num++;
    memset(buf,0x0,sizeof(buf));
    sprintf(buf,"%d",num);
    xmldbc_set(NULL,0,node,buf);
}


void    trap_counter(char *cmd){
        int trap;
        if (strncmp(cmd, "[SNMP-TRAP]", 11)==0){
            cmd+=11;
                if(strncmp(cmd, "[Generic=", 9)==0){
                    cmd+=9;
                    trap=atoi(cmd);
                }
                if (strncmp(cmd, "[Specific=", 10)==0){
                    cmd+=10;
                    trap=atoi(cmd);

			if(trap==7){
                        xmldbc_integer_increase("/runtime/stats/wireless/up_times");   //counter wireless up times
                        }
                }
        #if ELBOX_PROGS_GPL_SNMP_TRAP_TELECOM
		  if(strncmp(cmd, "[Telecom=", 9)==0){
			   cmd+=9;
                	   trap=atoi(cmd);
                        if(trap==18){
                            xmldbc_integer_increase("/runtime/stats/wireless/up_times");
                        }
                }
         #endif
            }
}


void sendGenericTrap(int trap){
	int host=1;
    	unsigned char buff[30], host_buff[30], comm_buff[30], sec_buff[30], cmd_buff[100];
        #ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    	unsigned char *entOID = "1.3.6.1.4.1.38045";
        #else
        unsigned char *entOID = "1.3.6.1.4.1.171";
        #endif
       while(host<=10/*MAX_EPMADDR_ENTRIES*/){
                    /*Get host IP*/
                    memset(buff, 0x0, 30);
                    sprintf(buff, "%s%d", "/sys/snmpd/hostip:", host);
                    memset(host_buff, 0x0, 30);
                    xmldbc_get_wb(NULL, 0, buff, host_buff, 29);

                    if(strlen(host_buff)){
                        /*Get SNMP Version*/
                        memset(buff, 0x0, 30);
                        sprintf(buff, "%s%d", "/sys/snmpd/secumodel:", host);
                        memset(sec_buff, 0x0, 30);
                        xmldbc_get_wb(NULL, 0, buff, sec_buff, 29);
                        
                        /*SNMP v1*/
                        if(memcmp(sec_buff, "1", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
                            sprintf(cmd_buff, "\nsnmptrap -v 1 -c %s %s %s \"\" %d 0 \"\"\n", comm_buff, host_buff, entOID, trap);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v2c*/
                        if(memcmp(sec_buff, "2", 1)==0){
                        }
                        /*SNMP v3*/
                        if(memcmp(sec_buff, "3", 1)==0){
                        }
                        host++;
                    }else{
                        host=11;
                    }
 	}
 }

#define MAC_path 						    "/runtime/wan/inf:1/mac"
#define CPUUTILIZATION_path  "/runtime/cpu/status/alluser_utilize"
#define MEMORYUTILIZATION_path  "/runtime/mem/status/alluser_utilize"
#define APMONITOR_path					"/wlan/inf:1/apmonitor"
#define APMONITOR_Last_path					"/wlan/inf:1/apmonitor_last"
#define systemFirmwareVersion_path 		"/runtime/sys/info/firmwareversion"
#define DOT11SSID_path					"/wlan/inf:%d/ssid" 		
#define DOT11USERLIMIT_path         	"/wlan/inf:1/assoc_limit/number" 

#define DOT11CHANNEL_path				"/wlan/inf:1/channel"
#define DOT11CHANNEL_OLD_path			"/wlan/inf:1/channel_old"
#define DOT11AUTOCHANNEL_path			"/runtime/stats/wlan/inf:1/channel"
#define DOT11AUTOCHANNELSCAN_path		"/wlan/inf:1/autochannel"		
#define DOT11CLIENTINFO_path			"/runtime/stats/wlan/inf:1/client:1/mac" 


#define interfereDevice_path      "/runtime/interfereDevice"
#define interfereChannel_path    "/runtime/interfereChannel"

#define trap_debug_path "/debug/trap"

char *trapName[]={
    "nothing",
    "CPUusageTooHighTrap",
    "CPUusageTooHighRecovTrap",
    "MemUsageTooHighTrap",
    "MemUsageTooHighRecovTrap",
    "APOfflineTrap",
    "APOnlineTrap",
    "APMtWorkModeChgTrap",
    "APSWUpdateFailTrap",
    "SSIDkeyConflictTrap",
    "APCoInterfDetectedTrap",
    "APCoInterfClearTrap",
    "APNerborInterfDetectedTrap",
    "APNeiborInterfClearTrap",
    "StaInterfDetectedTrap",
    "StaInterfClearTrap",
    "OtherDeviceInterfDetectedTrap",
    "OtherDevInterfClearTrap",
    "RadioDownTrap",
    "RadioDownRecovTrap",
    "APStaFullTrap",
    "APStaFullRecoverTrap",
    "APMtRdoChanlChgTrap",
    "StaAuthErrorTrap",
    "StaAssociationFailTrap",
    "UserWithInvalidCerTrap",
    "StationRepititiveAttackTrap",
    "TamperAttackTrap",
    "LowSafeLevelAttackTrap",
    "AddressRedirectionAttackTrap",
    "detectRogueTrap"
};


//traveller add trap switch for china telecom
#define cpuUsageTooHighTrapSwitch_path	        "/sys/snmptrap/switch/cpuUsageTooHighTrapSwitch"
#define cpuUsageTooHighRecovTrapSwitch_path	 "/sys/snmptrap/switch/cpuUsageTooHighRecovTrapSwitch"
#define memUsageTooHighTrapSwitch_path	        "/sys/snmptrap/switch/memUsageTooHighTrapSwitch"
#define memUsageTooHighRecovTrapSwitch_path	  "/sys/snmptrap/switch/memUsageTooHighRecovTrapSwitch"
#define apOfflineTrapSwitch_path	                        "/sys/snmptrap/switch/apOfflineTrapSwitch"
#define apOnlineTrapSwitch_path	                        "/sys/snmptrap/switch/apOnlineTrapSwitch"
#define apMtWorkModeChgTrapSwitch_path	         "/sys/snmptrap/switch/apMtWorkModeChgTrapSwitch"
#define apSWUpdateFailTrapSwitch_path	                "/sys/snmptrap/switch/apSWUpdateFailTrapSwitch"
#define ssidKeyConflictTrapSwitch_path	                "/sys/snmptrap/switch/ssidKeyConflictTrapSwitch"
#define apCoInterfDetectedTrapSwitch_path	            "/sys/snmptrap/switch/apCoInterfDetectedTrapSwitch"
#define apCoInterfClearTrapSwitch_path	                 "/sys/snmptrap/switch/apCoInterfClearTrapSwitch"
#define apNerborInterfDetectedTrapSwitch_path	       "/sys/snmptrap/switch/apNerborInterfDetectedTrapSwitch"
#define apNeiborInterfClearTrapSwitch_path	         "/sys/snmptrap/switch/apNeiborInterfClearTrapSwitch"
#define staInterfDetectedTrapSwitch_path        	 "/sys/snmptrap/switch/staInterfDetectedTrapSwitch"
#define staInterfClearTrapSwitch_path	         "/sys/snmptrap/switch/staInterfClearTrapSwitch"
#define otherDeviceInterfDetectedTrapSwitch_path	 "/sys/snmptrap/switch/otherDeviceInterfDetectedTrapSwitch"
#define otherDevInterfClearTrapSwitch_path	         "/sys/snmptrap/switch/otherDevInterfClearTrapSwitch"
#define radioDownTrapSwitch_path	                    "/sys/snmptrap/switch/radioDownTrapSwitch"
#define radioDownRecovTrapSwitch_path	            "/sys/snmptrap/switch/radioDownRecovTrapSwitch"
#define apStaFullTrapSwitch_path	                    "/sys/snmptrap/switch/apStaFullTrapSwitch"
#define apStaFullRecoverTrapSwitch_path	 "/sys/snmptrap/switch/apStaFullRecoverTrapSwitch"
#define apMtRdoChanlChgTrapSwitch_path	 "/sys/snmptrap/switch/apMtRdoChanlChgTrapSwitch"
#define staAuthErrorTrapSwitch_path	         "/sys/snmptrap/switch/staAuthErrorTrapSwitch"
#define stAssociationFailTrapSwitch_path	         "/sys/snmptrap/switch/stAssociationFailTrapSwitch"
#define userWithInvalidCerficationInbreakNetworkTrapSwitch_path	 "/sys/snmptrap/switch/userWithInvalidCerficationInbreakNetworkTrapSwitch"
#define stationRepititiveAttackTrapSwitch_path	 "/sys/snmptrap/switch/stationRepititiveAttackTrapSwitch"
#define tamperAttackTrapSwitch_path	         "/sys/snmptrap/switch/tamperAttackTrapSwitch"
#define lowSafeLevelAttackTrapSwitch_path	 "/sys/snmptrap/switch/lowSafeLevelAttackTrapSwitch"
#define addressRedirectionAttackTrapSwitch_path	 "/sys/snmptrap/switch/addressRedirectionAttackTrapSwitch"
#define detectRogueTrapSwitch_path	         "/sys/snmptrap/switch/detectRogueTrapSwitch"

#define defaultSwitch 1

char *trapSwitchTable[]={
    "nothing",
    cpuUsageTooHighTrapSwitch_path,
    cpuUsageTooHighRecovTrapSwitch_path,
    memUsageTooHighTrapSwitch_path,
    memUsageTooHighRecovTrapSwitch_path,
    apOfflineTrapSwitch_path,
    apOnlineTrapSwitch_path,
    apMtWorkModeChgTrapSwitch_path,
    apSWUpdateFailTrapSwitch_path,
    ssidKeyConflictTrapSwitch_path,
    apCoInterfDetectedTrapSwitch_path,
    apCoInterfClearTrapSwitch_path,
    apNerborInterfDetectedTrapSwitch_path,
    apNeiborInterfClearTrapSwitch_path,
    staInterfDetectedTrapSwitch_path,
    staInterfClearTrapSwitch_path,
    otherDeviceInterfDetectedTrapSwitch_path,
    otherDevInterfClearTrapSwitch_path,
    radioDownTrapSwitch_path,
    radioDownRecovTrapSwitch_path,
    apStaFullTrapSwitch_path,
    apStaFullRecoverTrapSwitch_path,
    apMtRdoChanlChgTrapSwitch_path,
    staAuthErrorTrapSwitch_path,
    stAssociationFailTrapSwitch_path,
    userWithInvalidCerficationInbreakNetworkTrapSwitch_path,
    stationRepititiveAttackTrapSwitch_path,
    tamperAttackTrapSwitch_path,
    lowSafeLevelAttackTrapSwitch_path,
    addressRedirectionAttackTrapSwitch_path,
    detectRogueTrapSwitch_path
};


//traveller add for variable binding start
void trapbind(char *bindbuf,int trap){
	char mac[256];
	rgdb_get_max(MAC_path, mac);
//  1 CPUusageTooHighTrap 
//  2 CPUusageTooHighRecovTrap 
	if(trap == 1 || trap ==2){   //cpu
		char cpu[256];
		rgdb_get_max(CPUUTILIZATION_path, cpu);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s cpu:\"%s\" ",mac,cpu);
	}
    
//  3 MemUsageTooHighTrap 
//  4 MemUsageTooHighRecovTrap 
	if(trap ==3 || trap ==4){  //mem
		char mem[256];
		rgdb_get_max(MEMORYUTILIZATION_path, mem);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s mem:\"%s\" ",mac,mem);
	}

//  5 APOfflineTrap 
	if(trap ==5){                      //ap ac disconnect
		char *reason="eth disconnect";
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s reason:\"%s\" ",mac,reason);
	}

//  6 APOnlineTrap 
	if(trap ==6){                      //ap ac connect
		sprintf(bindbuf," 1 s ap:\"%s\" ",mac);
	}

//  7 APMtWorkModeChgTrap 
	if(trap ==7){			//ap monitor type change
		char monitor_prev[256];
		char monitor_now[256];
		rgdb_get_max(APMONITOR_path, monitor_now);
		rgdb_get_max(APMONITOR_Last_path, monitor_prev);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s monitor_now:\"%s\" 1 s monitor_prev:\"%s\" ",mac,monitor_now,monitor_prev);
	}


//  8 APSWUpdateFailTrap 
	if(trap ==8){		  //ap fw update fail
		char *reason="fw version wrong";
		char version[256];
		rgdb_get_max(systemFirmwareVersion_path,version);
		sprintf(bindbuf," 1 s reason:\"%s\" 1 s version:\"%s\" 1 s \"version unknow\" ",reason,version);
	}

//  9 SSIDkeyConflictTrap 
	if(trap == 9){   //ssid collision
		char ssid1[256];
		char ssid2[256];
		rgdb_get_single_index(DOT11SSID_path,ssid1 , 1);
		rgdb_get_single_index(DOT11SSID_path,ssid2 , 2);
		sprintf(bindbuf," 1 s ap\"%s\" 1 s ssid1:\"%s\" 1 s ssid2:\"%s\" 1 s \"wepkey1\"",mac,ssid1,ssid2);
	}


//  10 APCoInterfDetectedTrap 
//  11 APCoInterfClearTrap 
	if(trap==10 || trap ==11){
		char autochan[256];
		char channel[256];
		rgdb_get_max(DOT11AUTOCHANNELSCAN_path, autochan);
		if(atol(autochan)==1){
			rgdb_get_max(DOT11AUTOCHANNEL_path, channel);
		}else{
			rgdb_get_max(DOT11CHANNEL_path, channel);
		}
		char interfere[256];
		rgdb_get_max(interfereDevice_path, interfere);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s channel:\"%s\" 1 s interfere:\"%s\" ",mac,channel,interfere);

	}

//  12 APNerborInterfDetectedT rap 
//  13 APNeiborInterfClearTrap 
	if(trap==12 || trap ==13){  //adjacent interfere
		char autochan[256];
		char channel[256];
		rgdb_get_max(DOT11AUTOCHANNELSCAN_path, autochan);
		if(atol(autochan)==1){
			rgdb_get_max(DOT11AUTOCHANNEL_path, channel);
		}else{
			rgdb_get_max(DOT11CHANNEL_path, channel);
		}
		char interfere[256];
		rgdb_get_max(interfereDevice_path, interfere);
		char interChan[256];
		rgdb_get_max(interfereChannel_path, interChan);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s channel:\"%s\" 1 s interfere:\"%s\" 1 s interchannel:\"%s\" ",mac,channel,interfere,interChan);

	}
//  14 StaInterfDetectedTrap 
//  15 StaInterfClearTrap 
	if(trap==14 || trap ==15){  //terminal interfere
		char autochan[256];
		char channel[256];
		rgdb_get_max(DOT11AUTOCHANNELSCAN_path, autochan);
		if(atol(autochan)==1){
			rgdb_get_max(DOT11AUTOCHANNEL_path, channel);
		}else{
			rgdb_get_max(DOT11CHANNEL_path, channel);
		}
		char client[256];
		rgdb_get_max(DOT11CLIENTINFO_path,client);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s channel:\"%s\" 1 s client:\"%s\" ",mac,channel,client);

	}
//  16 OtherDeviceInterfDetect edTrap 
//  17 OtherDevInterfClearTrap 
	if(trap==16 || trap ==17){
		char autochan[256];
		char channel[256];
		rgdb_get_max(DOT11AUTOCHANNELSCAN_path, autochan);
		if(atol(autochan)==1){
			rgdb_get_max(DOT11AUTOCHANNEL_path, channel);
		}else{
			rgdb_get_max(DOT11CHANNEL_path, channel);
		}
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s channel:\"%s\"  ",mac,channel);

	}


//  18 RadioDownTrap 
//  19 RadioDownRecovTrap 
	if(trap ==18 || trap ==19){	//wlan link down
		char *reason="admin down";
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s reason:\"%s\" ",mac,reason);
	}


//  20 APStaFullTrap 
//  21 APStaFullRecoverTrap 
	if(trap ==20 || trap ==21){   //user limited
		char limit[256];
		rgdb_get_max(DOT11USERLIMIT_path, limit);
		char *reason="user limited";
		sprintf(bindbuf," 1 s limit:\"%s\" 1 s reason:\"%s\" ",limit,reason);
	}


//  22 APMtRdoChanlChgTrap 
	if(trap ==22){
		char autochan[256];
		char channel[256];
		char channel_old[256];
		char mode[256];
		memset(mode,0x0,256);
		rgdb_get_max(DOT11AUTOCHANNELSCAN_path, autochan);
		if(atol(autochan)==1){
			sprintf(mode,"%s","auto");
			rgdb_get_max(DOT11AUTOCHANNEL_path, channel);
		}else{
			sprintf(mode,"%s","manual");
			rgdb_get_max(DOT11CHANNEL_path, channel);
		}
		rgdb_get_max(DOT11CHANNEL_OLD_path,channel_old);
		
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s channel:\"%s\" 1 s channel_old:\"%s\" 1 s changemode:\"%s\" ",mac,channel,channel_old,mode);
	}


//  23 StaAuthErrorTrap
	if(trap==23){   //auth fail
		char ssid1[256];
		rgdb_get_single_index(DOT11SSID_path,ssid1 , 1);
		char client[256];
		rgdb_get_max(DOT11CLIENTINFO_path,client);
		char *auth="wep";
		char *reason="wep key fail";
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s ssid1:\"%s\" 1 s client:\"%s\" 1 s auth:\"%s\" 1 s reason:\"%s\" ",mac,ssid1,client,auth,reason);
	}


//  24 StaAssociationFailTrap
	if(trap==24){  //assoc fail
		char ssid1[256];
		rgdb_get_single_index(DOT11SSID_path,ssid1 , 1);
		char client[256];
		rgdb_get_max(DOT11CLIENTINFO_path,client);
		char *reason="user limited";
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s ssid1:\"%s\" 1 s client:\"%s\" 1 s reason:\"%s\"",mac,ssid1,client,reason);
	}


//  25 UserWithInvalidCerficationInbreakNetworkTrap 
	if(trap==25){   //wapi invalid cert
		char ssid1[256];
		rgdb_get_single_index(DOT11SSID_path,ssid1 , 1);
		char client[256];
		rgdb_get_max(DOT11CLIENTINFO_path,client);
		sprintf(bindbuf,"  1 s ap:\"%s\" 1 s ssid1:\"%s\" 1 s client:\"%s\" ",mac,ssid1,client);
	}

//  26 StationRepititiveAttackTrap 
//  27 TamperAttackTrap 
//  28 LowSafeLevelAttackTrap 
//  29 AddressRedirectionAttackTrap 
	if(trap==26|| trap==27 || trap ==28 || trap ==29){  //wapi repetive attack ,sophisticate ,security down,redirect
		char ssid1[256];
		rgdb_get_single_index(DOT11SSID_path,ssid1 , 1);
		char client[256];
		rgdb_get_max(DOT11CLIENTINFO_path,client);
		sprintf(bindbuf," 1 s ap:\"%s\" 1 s ssid1:\"%s\" 1 s client:\"%s\" ",mac,ssid1,client);
	}

// 30 detectRogueTrap

	int debug=int_rgdb_get(trap_debug_path);
	if(debug){
		printf("trap:%s\n",bindbuf);
		}
}

//traveller add for variable binding end


 void sendSpecificTrap(int trap){
    int model=0;
    int host;
    unsigned char buff[30], host_buff[30], comm_buff[30], sec_buff[30], cmd_buff[100],oid_buf[256];
    unsigned char bindbuf[256];
#ifdef  ELBOX_MODEL_DAP2553
    model=35;
#elif  	ELBOX_MODEL_DAP2590
    model=36;
#elif	ELBOX_MODEL_DAP3520
	model=37;
#elif  	ELBOX_MODEL_DAP1353B
    model=38;
#elif 	ELBOX_MODEL_DAP2690
    model=39;
#elif	ELBOX_MODEL_DAP2360
    model=40;
#elif	ELBOX_MODEL_DWP2360
	model=40;
#elif  	ELBOX_MODEL_DAP3690
    model=41;
#elif  	ELBOX_MODEL_DAP2310
    model=43;
#elif  	ELBOX_MODEL_DAP3340
    model=44;
#elif 	ELBOX_MODEL_DAP2690B
    model=45;
#elif 	ELBOX_MODEL_NEC_MAGNUS
    model=45;
#elif   ELBOX_MODEL_DAP2553B
    model=46;
#else
    model=35;
#endif

		memset(oid_buf,0x0,256);
		//allenxiao change oid from "1.3.6.1.4.1.171.10.37.%d.5.7.2.%d" to "1.3.6.1.4.1.171.10.37.%d.5.7.1.%d" 2011.10.19
		#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
			sprintf(oid_buf,"1.3.6.1.4.1.38045.10.37.%d.5.7.1.%d",model,trap);
		#else
       			sprintf(oid_buf,"1.3.6.1.4.1.171.10.37.%d.5.7.2",model);
		#endif
		//allenxiao end 2011.10.19
		memset(bindbuf,0x0,256);
		//printf("oid_buf:%s\n",oid_buf);
                /*Go through host table*/
                host=1;
                while(host<=10/*MAX_EPMADDR_ENTRIES*/){
                    /*Get host IP*/
                    memset(buff, 0x0, 30);
                    sprintf(buff, "%s%d", "/sys/snmpd/hostip:", host);
                    memset(host_buff, 0x0, 30);
                    xmldbc_get_wb(NULL, 0, buff, host_buff, 29);

                    if(strlen(host_buff)){
                        /*Get SNMP Version*/
                        memset(buff, 0x0, 30);
                        sprintf(buff, "%s%d", "/sys/snmpd/secumodel:", host);
                        memset(sec_buff, 0x0, 30);
                        xmldbc_get_wb(NULL, 0, buff, sec_buff, 29);
                        
                        /*SNMP v1*/
                        if(memcmp(sec_buff, "1", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
//                            sprintf(cmd_buff, "snmptrap -v 1 -c %s %s %s \"\" 6 %d \"\"", comm_buff, host_buff, entOID, trap);//paley for device send the wrong trap OID
                            sprintf(cmd_buff, "\nsnmptrap -v 1 -c %s %s %s \"\" 6 %d \"\" %s\n", comm_buff, host_buff,oid_buf, trap,bindbuf);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v2c*/
                        if(memcmp(sec_buff, "2", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
                            sprintf(cmd_buff, "snmptrap -v 2c -c %s %s \"\" %s.%d", comm_buff, host_buff, oid_buf, trap);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v3*/
                        if(memcmp(sec_buff, "3", 1)==0){
                        }
                        host++;
                    }else{
                        host=11;
                    }
                }
 }

 void sendTelecomTrap(int trap){

    int model=0;
    int host;
    unsigned char buff[30], host_buff[30], comm_buff[30], sec_buff[30], cmd_buff[100],oid_buf[256];
    unsigned char bindbuf[256];
    char namebuf[32];
    int trapswitch=0;

              //traveller add for trap switch
               trapswitch=int_default_rgdb_get(trapSwitchTable[trap],defaultSwitch);
              //printf("trapswitch:%d\n",trapswitch);
              if(trapswitch==0){
                    return;
                }else{
                        //printf("trap switch on\n");
                    }
              //traveller end

		memset(oid_buf,0x0,256);
        #ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
        sprintf(oid_buf,"1.3.6.1.4.1.38045.10.37.39.32.9.%d",trap);
        #else
		sprintf(oid_buf,"1.3.6.1.4.1.171.10.37.39.32.9.%d",trap);
        #endif
        memset(bindbuf,0x0,256);
		trapbind(bindbuf,trap);
              sprintf(namebuf,"%s",trapName[trap]);
                /*Go through host table*/
                host=1;
                while(host<=10/*MAX_EPMADDR_ENTRIES*/){
                    /*Get host IP*/
                    memset(buff, 0x0, 30);
                    sprintf(buff, "%s%d", "/sys/snmpd/hostip:", host);
                    memset(host_buff, 0x0, 30);
                    xmldbc_get_wb(NULL, 0, buff, host_buff, 29);

                    if(strlen(host_buff)){
                        /*Get SNMP Version*/
                        memset(buff, 0x0, 30);
                        sprintf(buff, "%s%d", "/sys/snmpd/secumodel:", host);
                        memset(sec_buff, 0x0, 30);
                        xmldbc_get_wb(NULL, 0, buff, sec_buff, 29);
                        
                        /*SNMP v1*/
                        if(memcmp(sec_buff, "1", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
//                            sprintf(cmd_buff, "snmptrap -v 1 -c %s %s %s \"\" 6 %d \"\"", comm_buff, host_buff, entOID, trap);//paley for device send the wrong trap OID
                            sprintf(cmd_buff, "\nsnmptrap -v 1 -c %s %s %s \"\" 6 %d \"\" 1 s %s %s \n", comm_buff, host_buff,oid_buf, trap,namebuf,bindbuf);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v2c*/
                        if(memcmp(sec_buff, "2", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
                            sprintf(cmd_buff, "snmptrap -v 2c -c %s %s \"\" %s", comm_buff, host_buff, oid_buf);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v3*/
                        if(memcmp(sec_buff, "3", 1)==0){
                        }
                        host++;
                    }else{
                        host=11;
                    }
                }

}

int getBindArray(char *s,char argv[16][32]){
  char *p =s;
  int num=0;
  int argc=0;
  int move=0;
  int state=0;
  int i;
  
  while(*p!='\0' && num<256 ){
     //printf("%c\n",*p);
     if(*p=='['){
        state=1;
        move=0;
        p++;
        num++;
        continue;
     }
  
     if(state==1){
        if(*p==']'){
           state=0;
           move=0;
           p++;
           num++;
           argc++;
           continue;            
        }          
        argv[argc][move]=*p;
        move++;          
     }
     
          
     p++;             
     num++;
  }     

    return argc;
}


void sendTeBindTrap(char *cmd){
    //char *cmd ="[TeBind=31][name][time=xxx][rssi=xxx][channel=xxx][mac=xxx] \n";
    char argv[16][32];
    memset(argv,0x0,16*32);
    int argc = getBindArray(cmd,argv);
    
    int trap=atoi(argv[0]+7);
    //printf("trap:%d\n",trap);
    
    char namebuf[32];
    memset(namebuf,0x0,32);
    sprintf(namebuf,"%s",argv[1]);
    
    //" 1 s ap:\"%s\" 1 s cpu:\"%s\" "
    unsigned char bindbuf[256];
    memset(bindbuf,0x0,256);
    char tempbuf[64];
    int i;
    for(i=1;i<=argc-2;i++){
        memset(tempbuf,0x0,64);
        sprintf(tempbuf," 1 s %s ",argv[i+1]);
        strcat(bindbuf,tempbuf);
        }


    int model=0;
    int host;
    unsigned char buff[30], host_buff[30], comm_buff[30], sec_buff[30], cmd_buff[100],oid_buf[256];

		memset(oid_buf,0x0,256);
        #ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
        sprintf(oid_buf,"1.3.6.1.4.1.38045.10.37.39.32.9.%d",trap);
        #else
		sprintf(oid_buf,"1.3.6.1.4.1.171.10.37.39.32.9.%d",trap);
        #endif
                /*Go through host table*/
                host=1;
                while(host<=10/*MAX_EPMADDR_ENTRIES*/){
                    /*Get host IP*/
                    memset(buff, 0x0, 30);
                    sprintf(buff, "%s%d", "/sys/snmpd/hostip:", host);
                    memset(host_buff, 0x0, 30);
                    xmldbc_get_wb(NULL, 0, buff, host_buff, 29);

                    if(strlen(host_buff)){
                        /*Get SNMP Version*/
                        memset(buff, 0x0, 30);
                        sprintf(buff, "%s%d", "/sys/snmpd/secumodel:", host);
                        memset(sec_buff, 0x0, 30);
                        xmldbc_get_wb(NULL, 0, buff, sec_buff, 29);
                        
                        /*SNMP v1*/
                        if(memcmp(sec_buff, "1", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
//                            sprintf(cmd_buff, "snmptrap -v 1 -c %s %s %s \"\" 6 %d \"\"", comm_buff, host_buff, entOID, trap);//paley for device send the wrong trap OID
                            sprintf(cmd_buff, "\nsnmptrap -v 1 -c %s %s %s \"\" 6 %d \"\" 1 s %s %s \n", comm_buff, host_buff,oid_buf, trap,namebuf,bindbuf);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v2c*/
                        if(memcmp(sec_buff, "2", 1)==0){
                            memset(buff, 0x0, 30);
                            sprintf(buff, "%s%d", "/sys/snmpd/commorun:", host);
                            memset(comm_buff, 0x0, 30);
                            xmldbc_get_wb(NULL, 0, buff, comm_buff, 29);
                            memset(cmd_buff, 0x0, 100);
                            sprintf(cmd_buff, "snmptrap -v 2c -c %s %s \"\" %s", comm_buff, host_buff, oid_buf);
							/* eric fu, 2009/03/02, stdout&stderr to /dev/null */
							strcat(cmd_buff," 1>/dev/null 2>&1 ");
                            system(cmd_buff);
                        }
                        /*SNMP v3*/
                        if(memcmp(sec_buff, "3", 1)==0){
                        }
                        host++;
                    }else{
                        host=11;
                    }
                }
    
}


void sendtrap(unsigned char* cmd){

    int trap, host=1;
    unsigned char buff[30], host_buff[30], comm_buff[30], sec_buff[30], cmd_buff[100];
    memset(buff, 0x0, 30);
    xmldbc_get_wb(NULL, 0, "/sys/snmptrap/status" , buff, 29);
    trap = atoi(buff);


    trap_counter(cmd);
    
    if(trap==1){
	if(strncmp(cmd, "[SNMP-TRAP]", 11)==0){
		 cmd+=11;
	         if(strncmp(cmd, "[Generic=", 9)==0){
                	cmd+=9;
                	trap=atoi(cmd);
			sendGenericTrap(trap);
	         }
    #if ELBOX_PROGS_GPL_SNMP_TRAP_TELECOM
		  if(strncmp(cmd, "[TeBind=", 8)==0){
			sendTeBindTrap(cmd);		
	         }       
		  if(strncmp(cmd, "[Telecom=", 9)==0){
			cmd+=9;
                	trap=atoi(cmd);
			sendTelecomTrap(trap);		
	         }                
    #else
		  if(strncmp(cmd, "[Specific=", 10)==0){
			cmd+=10;
                	trap=atoi(cmd);
			sendSpecificTrap(trap);		
	         }
    #endif   
	}
    }
  return;
}


