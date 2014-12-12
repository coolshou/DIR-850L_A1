<?
include "/htdocs/phplib/xnode.php";
/* set the preferred ssid for WPS registrar. (self-config) */
$lanmac = query("/runtime/devdata/lanmac");
$n1 = cut($lanmac, 0, ":");
$n2 = cut($lanmac, 1, ":");
$n3 = cut($lanmac, 2, ":");
$n4 = cut($lanmac, 3, ":");
$n5 = cut($lanmac, 4, ":");
$n6 = cut($lanmac, 5, ":");
$ssid = "dlink".$n5.$n6;
$ssid2 = "Megared".$n5.$n6;
$key = $n1.$n2.$n3.$n4.$n5.$n6;
$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "WLAN-1", 0);
if ($p != "")
{
	set($p."/media/wps/registrar/preferred/ssid", $ssid);
}

$path = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-1", 0);
if ($path != "")
{
	if(query("/runtime/device/devconfsize")==0)
	{
		set($path."/ssid", $ssid2);
		set($path."/nwkey/psk/key", $key);
	}
}

?>
