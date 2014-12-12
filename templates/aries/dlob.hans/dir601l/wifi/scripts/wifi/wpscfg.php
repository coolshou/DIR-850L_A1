<?
include "/htdocs/phplib/xnode.php";

$uid1 = "BAND24G-1.1";
$uid2 = "BAND5G-1.1";

$phy1	= XNODE_getpathbytarget("", "phyinf", "uid", $uid1, 0);
$phy2	= XNODE_getpathbytarget("", "phyinf", "uid", $uid2, 0);
$phyrp1	= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $uid1, 0);
$phyrp2	= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $uid2, 0);
$wifi1	= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy1."/wifi"), 0);
$wifi2	= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy2."/wifi"), 0);

$devname1	= query($phyrp1."/name");
$devname2	= query($phyrp2."/name");

$pin = query($phyrp1."/media/wps/enrollee/pin");

$active1 = query($phy1."/active");
$active2 = query($phy2."/active");

$wps_en1 = query($wifi1."/wps/enable");
$wps_en2 = query($wifi2."/wps/enable");


if ($ACTION=="PBC")
{
	if ($active1=="1" && $wps_en1=="1")	echo "wpatalk ".$devname1." configthem &\n";
	if ($active2=="1" && $wps_en2=="1")	echo "wpatalk ".$devname2." configthem &\n";

}
else if ($ACTION=="PIN")
{
	if ($active1=="1" && $wps_en1=="1")	echo 'wpatalk '.$devname1.' "configthem pin='.$pin.'" &\n';
	if ($active2=="1" && $wps_en2=="1")	echo 'wpatalk '.$devname2.' "configthem pin='.$pin.'" &\n';
}
?>
