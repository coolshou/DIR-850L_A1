<?
include "/htdocs/phplib/xnode.php";

function startcmd($cmd) {fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)  {fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

$http_forward=query("/miiicasa/http_forward");

if($http_forward == "1")
{
	//miiicasa agent will use miiicasa.cgi to update information, so
	//we just enable miiicasa agent
	startcmd("/usr/sbin/miiicasa_agent &\n");
	stopcmd("killall miiicasa_agent\n");
}
else
{
	//if miiicasa agent is disabled, we need to update information by ourself
	startcmd("/usr/sbin/miiicasa.cgi -action update_status noretry &\n");
}

?>
