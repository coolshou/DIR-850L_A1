<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/webinc/config.php";

fwrite("w", $START, "");
fwrite("w", $STOP, "");

$HOSTNAME = query("/device/hostname");
$INF = "br0";
$MDNS_CONF   = "/var/rendezvous.conf";
fwrite("w", $MDNS_CONF, "");
$layout_mode = query("/device/layout");
$mac = PHYINF_getmacsetting("LAN-1");
$macstr = cut($mac, 4, ":").cut($mac, 5, ":");

if (query("/device/mdnsresponder/enable")=="1")
{
	foreach ("/runtime/services/mdnsresponder/server")
	{
		if(strstr(query("uid"), "MDNSRESPONDER")!="") 
		{
			fwrite("a", $MDNS_CONF, query("srvname")."\n");
			fwrite("a", $MDNS_CONF, query("srvcfg")."\n");
			fwrite("a", $MDNS_CONF, query("port")."\n");
						
			//+++ Jerry Kao, added mDNS TXT entries for HNAP services.
			if(query("uid") == "MDNSRESPONDER.HNAP")
			{
				$model_name = query("/runtime/device/modelname");
				fwrite("a", $MDNS_CONF, "model_number=".$model_name."\n");					
				
				// Add LAN MAC address.
				$lan_path = XNODE_getpathbytarget("/runtime", "phyinf", "name", $INF, 0);
				$mac  = toupper(query($lan_path."/macaddr"));
				fwrite("a", $MDNS_CONF, "mac=".$mac."\n");
				
				// Add 2.4G SSID.
				$24g_path	= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $WLAN1, 0);
				$24g_valid	= query($24g_path."/valid");
				if ($24g_valid == "1")
				{
					$24g_ssid = query($24g_path."/media/wifi/ssid");
					fwrite("a", $MDNS_CONF, "wlan0_ssid=".$24g_ssid."\n");
				}
				
				// Add 5G SSID.
				$5g_path	= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $WLAN2, 0);
				$5g_valid	= query($5g_path."/valid");
				if ($5g_valid == "1")
				{
					$5g_ssid = query($5g_path."/media/wifi/ssid");
					fwrite("a", $MDNS_CONF, "wlan1_ssid=".$5g_ssid."\n");
				}
			}
								
			fwrite("a", $MDNS_CONF, "\n");
		
	  }
	}
	fwrite("a", $START, "echo \"mdnsresponder server start !\" > /dev/console\n");
	//fwrite("a", $START, "HOSTNAME=`hostname`\n");
	//fwrite("a", $START, "if [ \"$HOSTNAME\" == \"\" ]; then hostname ".$HOSTNAME."; fi\n");
	if($layout_mode == "router")
		fwrite("a", $START, "hostname ".$HOSTNAME."\n");
	else
		fwrite("a", $START, "hostname ".$HOSTNAME.$macstr."\n");
//jef add +   for support use shareport.local to access shareportmobile
	$web_file_access = query("/webaccess/enable");
	if($web_file_access == 1)
		fwrite("a", $START, "mDNSResponderPosix -b -i ".$INF." -f ".$MDNS_CONF." -e shareport \n");
	else
		fwrite("a", $START, "mDNSResponderPosix -b -i ".$INF." -f ".$MDNS_CONF." \n");
//jef add -
	fwrite("a", $STOP, "echo \"mdnsresponder server stop !\" > /dev/console\n");
	fwrite("a", $STOP, "killall -9 mDNSResponderPosix\n");
}
else
{
	fwrite("a", $START, "echo \"mdnsresponder server is disabled !\" > /dev/console\n");
	fwrite("a", $STOP, "echo \"mdnsresponder server is disabled !\" > /dev/console\n");
}

?>
