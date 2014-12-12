<?
fwrite("w",$START, "#!/bin/sh\nrtlioc enlan\nsleep 3\nevent IPV6ENABLE\nexit 0\n"); 
fwrite("w",$STOP,  "#!/bin/sh\nexit 0\n");
?>
