<?
fwrite("w",$START, "#!/bin/sh\n");
fwrite("a",$START, "et robowr 0x10 0x0 0x8000\n");
fwrite("a",$START, "et robowr 0x11 0x0 0x8000\n");
fwrite("a",$START, "et robowr 0x12 0x0 0x8000\n");
fwrite("a",$START, "et robowr 0x13 0x0 0x8000\n");

//this sleep will increase our bootup time 
//we use it for ipv6 logo test (tom, 20121115)
$ipv6logo = get("" , "/runtime/devdata/ipv6logo");
if($ipv6logo == "1")
	fwrite("a",$START, "sleep 5;event IPV6ENABLE\n");
else
	fwrite("a",$START, "event IPV6ENABLE\n");

fwrite("a",$START, "echo 1 > /proc/sys/net/ipv6/conf/all/forwarding\n");
fwrite("a",$START, "exit 0\n");
fwrite("w",$STOP,  "#!/bin/sh\nexit 0\n");
?>
