<?
include "/etc/services/PHYINF/phyinf.php";
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");
phyinf_setup("ETH-2");
/* Create WAN interface. */
$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-2", 0);
setattr($path."/linkstatus",	"get","psts wan");
fwrite("a", $START, "exit 0\n");
fwrite("a", $STOP,  "exit 0\n");
?>
