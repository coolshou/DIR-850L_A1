<?
include "/htdocs/phplib/xnode.php";

function schcmd($uid)
{
	/* Get schedule setting */
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	$sch = XNODE_getschedule($p);
	if ($sch=="") $cmd = "start";
	else
	{
		$days = XNODE_getscheduledays($sch);
		$start = query($sch."/start");
		$end = query($sch."/end");
		if (query($sch."/exclude")=="1") $cmd = 'schedule!';
		else $cmd = 'schedule';
		$cmd = $cmd.' "'.$days.'" "'.$start.'" "'.$end.'"';
	}
	return $cmd;
}

/********************************************************************/
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

fwrite("a",$START,	
	"service PHYINF.BAND24G-1.1 ".schcmd("BAND24G-1.1")."\n".
	"service PHYINF.BAND24G-1.2 ".schcmd("BAND24G-1.2")."\n".
	"service PHYINF.BAND5G-1.1 ".schcmd("BAND5G-1.1")."\n".
	"service PHYINF.BAND5G-1.2 ".schcmd("BAND5G-1.2")."\n"
	);

fwrite("a",$STOP,
	"service PHYINF.BAND24G-1.2 stop\n".
	"service PHYINF.BAND24G-1.1 stop\n".	
	"service PHYINF.BAND5G-1.2 stop\n".
	"service PHYINF.BAND5G-1.1 stop\n"	
	);

fwrite("a",$START,	"exit 0\n");
fwrite("a",$STOP,	"exit 0\n");
?>
