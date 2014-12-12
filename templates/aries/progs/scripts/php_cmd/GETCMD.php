<?
/*************************************************************************************/
/****************** config file is design like below**********************************/
/*****command,command,.....@help text,php file name,service name,protected,***********/
/*protected has 4 types execute mode, NULL and HIDDEN and SUPER and HIDDEN_SUPER mode*/
/***********Please follow the sequence with character to add the commands ************/
/*************************************************************************************/
fwrite("a","/var/tmp/config_cli", "get,ip@show ipaddr,GET_LANIP,NULL,NULL,\n");
fwrite("a","/var/tmp/config_cli", "get,ipmask@show ipmask,GET_LANMASK,NULL,NULL,\n");
fwrite("a","/var/tmp/config_cli", "get,lan_dns@show lan dns,GET_LANDNS,INET.LAN-1,NULL,\n");
?>