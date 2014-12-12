<?
include "/etc/services/HTTP/httpsvcs.php";
fwrite("w",$START,"#!/bin/sh\n");
fwrite("w", $STOP,"#!/bin/sh\n");
fwrite("a",$START,"service miiicasa_agent start\n");

fwrite("a", $STOP,"service miiicasa_agent stop\n");
fwrite("a", $STOP,"service miiicasa_proxyd stop\n");
miiicasasetup("WAN-1");
?>
