<?/* vi: set sw=4 ts=4: */
/* update WPS status */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

/* we force the wps state as one. */

/*
$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $PHY_UID);
set($p."/media/wps/enrollee/state", $STATE);
*/
/* When $p is empty, the phyinf is down. */
$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND24G-1.1");
if ($p!="")	set($p."/media/wps/enrollee/state", $STATE);

$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND5G-1.1");
if ($p!="")	set($p."/media/wps/enrollee/state", $STATE);

?>
