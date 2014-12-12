<?
include "/htdocs/phplib/xnode.php";

setattr("/runtime/devdata/mfcmode" ,"get","devdata get -e mfcmode");

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

if(query("/runtime/devdata/mfcmode") != "1"){
	if(query("/device/layout")=="router")
	{
	fwrite("a",$START,	
		"service PHYINF.BAND24G-1.1 ".schcmd("BAND24G-1.1")."\n".
		"service PHYINF.BAND24G-1.2 ".schcmd("BAND24G-1.2")."\n".
		"service PHYINF.BAND5G-1.1 ".schcmd("BAND5G-1.1")."\n".
		"service PHYINF.BAND5G-1.2 ".schcmd("BAND5G-1.2")."\n".
		);

	fwrite("a",$STOP,
		"service PHYINF.BAND5G-1.2 stop\n".
		"service PHYINF.BAND5G-1.1 stop\n".
		"service PHYINF.BAND24G-1.2 stop\n".
		"service PHYINF.BAND24G-1.1 stop\n".
		);
	}
	else if(query("/device/layout")=="bridge")
	{
		fwrite("a",$START,	
			"service PHYINF.BAND5G-1.1 ".schcmd("BAND5G-1.1")."\n".
			"service PHYINF.WIFI-REPEATER ".schcmd("WIFI-REPEATER")."\n".
			);

		fwrite("a",$STOP,
			"service PHYINF.WIFI-REPEATER stop\n".
			"service PHYINF.BAND5G-1.1 stop\n".
			);
	}
}
fwrite("a",$START,	"exit 0\n");
fwrite("a",$STOP,	"exit 0\n");
?>
