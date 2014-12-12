<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";

$active = query("/upnpav/dms/active");

fwrite("w", $START, "#!/bin/sh\n");
fwrite("w", $STOP,  "#!/bin/sh\n");

if ($active != "1")
{
	fwrite("a", $START, "echo \"Upnp-av server is disabled !\" > /dev/console\n");
}
else
{	
	fwrite("a", $START, "echo \"Start scan media !\" > /dev/console\n");
	fwrite("a", $START, "ScanMedia &\n");
}	

fwrite("a", $STOP,  "killall -9 ScanMedia\n");

?>
