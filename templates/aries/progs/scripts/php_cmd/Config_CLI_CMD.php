<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

fwrite("w","/var/tmp/config_cli", "");
if (isfile("/etc/scripts/php_cmd/OTHER.php")==1)	
	dophp("load", "/etc/scripts/php_cmd/OTHER.php");
if (isfile("/etc/scripts/php_cmd/SETCMD.php")==1)
	dophp("load", "/etc/scripts/php_cmd/SETCMD.php");
if (isfile("/etc/scripts/php_cmd/GETCMD.php")==1)	
	dophp("load", "/etc/scripts/php_cmd/GETCMD.php");
?>