<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");} 

fwrite(w,$_GLOBALS["START"], "#!/bin/sh\n");
fwrite(w,$_GLOBALS["STOP"], "#!/bin/sh\n"); 

startcmd("service UPNPAV start");
startcmd("service ITUNES start");
startcmd("service SAMBA start");

stopcmd("service UPNPAV stop");
stopcmd("service ITUNES stop");
stopcmd("service SAMBA stop");
?>
