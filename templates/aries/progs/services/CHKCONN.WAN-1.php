<?
include "/htdocs/phplib/xnode.php";
include "/etc/services/CHKCONN/chkconn.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

chkconn("WAN-1");

fwrite("a",$START, "exit 0\n");
fwrite("a", $STOP, "exit 0\n");
?>
