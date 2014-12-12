<?
include "/etc/services/PHYINF/phywifi.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

$p = XNODE_getpathbytarget("", "phyinf", "uid", "BAND5G-1.1", 0);
$p2= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);

if ($p=="")		return error(9);

if (query($p."/active")==1)
{
	//don't do anything at startup
}
else  if (query($p2."/active")==1)
{
	/* for RT3092: the clock of RT3092 is based on RT3662, 
	 * so the RT3662 must be up once for RT3092. */

	$dev = devname("BAND5G-1.1");
	echo "insmod /lib/modules/rt2860v2_ap.ko\n";
	echo "xmldbc -P /etc/services/WIFI/rtcfg.php -V PHY_UID=BAND5G-1.1 > /var/run/RT2860.dat\n";
	echo "ip link set ".$dev." up\n";
	echo "ip link set ".$dev." down\n";
	echo "rmmod rt2860v2_ap\n";
	error(0);
}
else 
{
	return error(8);
}
?>
