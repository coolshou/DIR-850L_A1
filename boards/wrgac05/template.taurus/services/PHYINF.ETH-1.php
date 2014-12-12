<?
include "/etc/services/PHYINF/phyinf.php";
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");
phyinf_setup("ETH-1");
$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
setattr($path."/linkstatus:1",	"get","psts lan1");
setattr($path."/linkstatus:2",	"get","psts lan2");
setattr($path."/linkstatus:3",	"get","psts lan3");
setattr($path."/linkstatus:4",	"get","psts lan4");
fwrite("a", $START, "exit 0\n");
fwrite("a", $STOP,  "exit 0\n");
?>
