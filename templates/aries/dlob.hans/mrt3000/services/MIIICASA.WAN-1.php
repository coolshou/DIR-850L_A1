<?
include "/etc/services/HTTP/httpsvcs.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

$miiicasa_enable = query("/miiicasa/enable");
fwrite("w",$START,"#!/bin/sh\n");
fwrite("w", $STOP,"#!/bin/sh\n");
/*fwrite("w",$START,"/usr/sbin/miiicasa.cgi -action update_status noretry\n");*/

if($miiicasa_enable == "1")
{
	fwrite("a",$START,"service miiicasa_agent start\n");
	fwrite("a", $STOP,"service miiicasa_agent stop\n");
	fwrite("a", $STOP,"service miiicasa_proxyd stop\n");
}	
miiicasasetup("WAN-1");
?>
