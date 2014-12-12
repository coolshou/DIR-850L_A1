<?
/*************************************************************************************/
/****************** config file is design like below**********************************/
/*****command,command,.....@help text,php file name,service name,protected,***********/
/*protected has 4 types execute mode, NULL and HIDDEN and SUPER and HIDDEN_SUPER mode*/
/***********Please follow the sequence with character to add the commands ************/
/*************************************************************************************/

fwrite("a","/var/tmp/config_cli", "alpha@Super user,ALPHA,NULL,".$_GLOBALS["HIDDEN"].",\n");
fwrite("a","/var/tmp/config_cli", "exit@exit cli,EXIT,NULL,".$_GLOBALS["HIDDEN"].",\n");
fwrite("a","/var/tmp/config_cli", "?@HELP LIST,HELP_CLI,NULL,".$_GLOBALS["SUPER"].",\n");
fwrite("a","/var/tmp/config_cli", "help@HELP LIST,HELP_CLI,NULL,NULL,\n");
?>