<?
/*************************************************************************************/
/****************** config file is design like below**********************************/
/*****command,command,.....@help text,php file name,service name,protected,***********/
/*protected has 4 types execute mode, NULL and HIDDEN and SUPER and HIDDEN_SUPER mode*/
/***********Please follow the sequence with character to add the commands ************/
/*************************************************************************************/
fwrite("a","/var/tmp/config_cli", "set,apply@Set APPLY,APPLY,NULL,NULL,\n");
fwrite("a","/var/tmp/config_cli", "set,lan_dns,disabled@Set LAN DNS DISABLED,LANDNS_DISABLED,INET.LAN-1,NULL,\n");
fwrite("a","/var/tmp/config_cli", "set,lan_dns,enabled@Set LAN DNS ENABLED,LANDNS_ENABLED,INET.LAN-1,NULL,\n");
/*fwrite("a","/var/tmp/config_cli", "set,lan_dns,ooooo@test,test,INET.LAN-1,".$_GLOBALS["SUPER"].",\n");    test */
/*fwrite("a","/var/tmp/config_cli", "set,lan_ip@Set lan ip address,SET_LANIP,INET.LAN-1,NULL,\n"); test */
?>