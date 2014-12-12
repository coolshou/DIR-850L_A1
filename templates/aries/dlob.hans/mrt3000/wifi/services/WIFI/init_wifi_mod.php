<?
include "/etc/services/PHYINF/phywifi.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

$p = XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);

if ($p!="")
{
	$dev = devname("BAND24G-1.1");
	echo "insmod /lib/modules/rt2860v2_ap.ko\n";
}

?>
