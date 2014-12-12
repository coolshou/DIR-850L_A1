<?
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w",$STOP,  "#!/bin/sh\nexit 0\n");
fwrite("a",$START, "event IPV6ENABLE\n");
fwrite("a",$START, "echo 1 > /proc/sys/net/ipv6/conf/all/forwarding\n");
fwrite("a",$START, "exit 0\n");
?>
