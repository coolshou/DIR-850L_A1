<?
/* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/phplib/mdnsresponder.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w",$STOP,  "#!/bin/sh\n");

$port	= "548";
$product = query("/runtime/device/modelname");
$srvname = "D-Link ".$product;
$srvcfg = "_afpovertcp._tcp. local.";
$mdirty = setup_mdns("MDNSRESPONDER.NETATALK",$port,$srvname,$srvcfg);

$active = query("/netatalk/active");
$sharepath = query("/runtime/device/storage/disk/entry/mntp");

$AFPD_CONF = "/var/afpd.conf";
$APPLE_VOLUME_CONF = "/var/AppleVolumes.default";

$stsp = XNODE_getpathbytarget("/inet", "entry", "uid","INET-1", 0);
$ipaddr=query($stsp."/ipv4/ipaddr");

$afpd_cmd="".$product." -tcp -unixcodepage UTF8 -ipaddr ".$ipaddr." -uamlist uams_guest.so -nosavepassword -defaultvol ".$APPLE_VOLUME_CONF." -systemvol /etc/AppleVolumes.system -uservol -uampath /lib\n";
$vol_cmd="".$sharepath." ".$product." perm:0777 options:usedots,tm dbpath:".$sharepath." cnidscheme:dbd\n";
if ($active=="1")
{
    fwrite("w", $AFPD_CONF, "".$afpd_cmd."\n");
    fwrite("w", $APPLE_VOLUME_CONF, "".$vol_cmd."\n");
	
	fwrite("a",$START, "mkdir /var/lock\n");
	fwrite("a",$START, "cnid_metad -d -s cnid_dbd &\n");
	fwrite("a",$START, "afpd -F ".$AFPD_CONF."&\n");
	if ($mdirty>0)  fwrite("a",$START,"service MDNSRESPONDER restart");

	fwrite("a", $STOP, "echo \"Netatalk is disabled !\" > /dev/console\n");
	
	fwrite("a",$STOP, "killall -9 afpd\n");
	fwrite("a",$STOP, "killall -9 cnid_metad\n");
	
}
else
{
	$mdirty = setup_mdns("MDNSRESPONDER.NETATALK","0",null,null);
	if ($mdirty>0)  fwrite("a", $STOP,"service MDNSRESPONDER restart");
}

?>
