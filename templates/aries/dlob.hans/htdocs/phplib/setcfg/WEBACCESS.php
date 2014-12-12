<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
set("/webaccess/enable", query($SETCFG_prefix."/webaccess/enable"));
set("/webaccess/httpenable", query($SETCFG_prefix."/webaccess/httpenable"));
set("/webaccess/httpport", query($SETCFG_prefix."/webaccess/httpport")); 
set("/webaccess/httpsenable", query($SETCFG_prefix."/webaccess/httpsenable"));
set("/webaccess/httpsport", query($SETCFG_prefix."/webaccess/httpsport")); 
set("/webaccess/remoteenable", query($SETCFG_prefix."/webaccess/remoteenable"));

movc($SETCFG_prefix."/webaccess/account", "/webaccess/account");
?>
