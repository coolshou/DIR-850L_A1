<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w",$STOP,  "#!/bin/sh\n");


$sch = query("/device/log/email/logsch");
if ($sch=="0" || $sch=="") $cmd = "start";
else
{
	$sch_name = XNODE_getschedule("/device/log/email");
	$days = XNODE_getscheduledays($sch_name);
	$start = query($sch_name."/start");
	$end = query($sch_name."/end");
	if (query($sch_name."/exclude")=="1") $cmd = 'schedule!';
	else $cmd = 'schedule';
	$cmd = $cmd.' "'.$days.'" "'.$start.'" "'.$end.'"';
	//TRACE_debug("cmd=".$cmd);
}
fwrite(a, $START, 'service LOG.EMAIL '.$cmd.'\n');
fwrite(a, $STOP, 'service LOG.EMAIL stop\n');

fwrite("a",$START, "exit 0\n");
fwrite("a",$STOP,  "exit 0\n");
?>
