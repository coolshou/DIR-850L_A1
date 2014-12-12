<?
include "/htdocs/phplib/xnode.php";
include "../htdocs/phplib/inet.php";

$phy_wlan1 = XNODE_getpathbytarget("","phyinf","uid","BAND24G-1.1", 0);
$phy_wlan2 = XNODE_getpathbytarget("","phyinf","uid","BAND5G-1.1", 0);

$wifi_1 = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy_wlan1."/wifi"), 0);
$wifi_2 = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy_wlan2."/wifi"), 0);

function set_profile($phy)
{
	if($_GLOBALS["1xenable"]=="1")//support 1x
	{
		//eap=%s\n
	}
	else
	{
		set($phy."/authtype",toupper($_GLOBALS["auth"]) );	
			
		set($phy."/encrtype",toupper($_GLOBALS["encr"]) );
			
		set($phy."/wps/configured","1");
		
		if($_GLOBALS["auth"]!="open" && $_GLOBALS["auth"] !="shared")
		{
			set($phy."/nwkey/psk/key",$_GLOBALS["key"] );
		}
		else 
		{
			if($_GLOBALS["encr"]=="WEP")
			{
				set($phy."/nwkey/wep/key",$_GLOBALS["key"]);			
				set($phy."/nwkey/wep/defkey",$_GLOBALS["keyindex"]);
					
				$key_len=strlen($_GLOBALS["key"]);
				if($key_len=="5" || $key_len=="13")
				{
					set($phy."/nwkey/wep/ascii","1");
				}
				else
				{
					set($phy."/nwkey/wep/ascii","0");
					set($phy."/nwkey/wep/ascii","0");						
				}
			}			
		}
	}
}

if($type=="SET")//set profile
{
	if($band=="DUAL" || $band=="DUAL1")//both band need to be set
	{
		if($band=="DUAL1")//append -2 at the end of 2.4G ssid
		{
			if(strlen($ssid)>30)
			{
				$new_ssid=substr($ssid, 0, 30);
				$new_ssid=$new_ssid."-2";
			}
			else
			{
				$new_ssid=$ssid."-2";
			}
			set($wifi_1."/ssid",$new_ssid);
			set($wifi_2."/ssid",$ssid);	
		}
		else
		{
			set($wifi_1."/ssid",$ssid);
			set($wifi_2."/ssid",$ssid);
		}
		if($channel2Dot4!="0")
		{
			set($phy_wlan1."/media/channel",$channel2Dot4);
		}
		if($channel5Dot0!="0")
		{
			set($phy_wlan2."/media/channel",$channel5Dot0);
		}
		set_profile($wifi_1);
		set_profile($wifi_2);

	}
	else if ($band=="2.4")//need to set 2.4G 
	{
		set($wifi_1."/ssid",$ssid);
		if($channel2Dot4!="0")
		{
			set($phy_wlan1."/media/channel",$channel2Dot4);
		}		
		set_profile($wifi_1);
	}
	else if ($band=="5")//need to set 5G 
	{
		set($wifi_2."/ssid",$ssid);
		if($channel5Dot0!="0")
		{
			set($phy_wlan2."/media/channel",$channel5Dot0);
		}
		set_profile($wifi_2);
	}
	event("DBSAVE");
}
else if($type=="WRITE_FILE")//write config file
{
	$mac=query("/runtime/devdata/wlanmac");
	$modelName      = query("/runtime/device/modelname");
	$manufacturer		= query("/runtime/device/vendor");	
	$desp=query("/runtime/device/description");
	$firmware=query("/runtime/device/firmwareversion");
	
	$mac=substr($mac, 6, 11);

	$i=0;
	while ($i < 4)
	{
    	$tmp = cut($mac, $i, ":");
    	$filename=$filename.toupper($tmp);
    	$i++;
	}
	
	fwrite("w+","".$path."".$filename.".WFC","<?xml version=\"1.0\"?>\n");
	fwrite("a+","".$path."".$filename.".WFC","<device xmlns=\"http://www.microsoft.com/provisioning/DeviceProfile/2004\">\n");
	fwrite("a+","".$path."".$filename.".WFC","	<configId>".$config_id."</configId>\n");
	fwrite("a+","".$path."".$filename.".WFC","	<manufacturer>".$manufacturer."</manufacturer>\n");
	fwrite("a+","".$path."".$filename.".WFC","	<modelName>".$modelName."</modelName>\n");
	fwrite("a+","".$path."".$filename.".WFC","	<serialNumber>".$desp."</serialNumber>\n");
	fwrite("a+","".$path."".$filename.".WFC","	<firmwareVersion>".$firmware."</firmwareVersion>\n");
	fwrite("a+","".$path."".$filename.".WFC","	<deviceType>Access_Point</deviceType>\n");
	fwrite("a+","".$path."".$filename.".WFC","</device>");
}
else if($type=="WRITE_SETTING")//write WSETTING.WFC
{
	$wpa=0;
	$wep=0;
	$upnpp		= XNODE_getpathbytarget("/runtime/upnp", "dev", "deviceType",
					"urn:schemas-wifialliance-org:device:WFADevice:1", 0);
	$uuid		= query($upnpp."/guid");
	$config_id=query("/runtime/genuuid");//generate uuid
	anchor($wifi_1);
	
	$manufacturer=query("/runtime/device/vendor");
	$ssid=query("ssid");
	$authtype	= query("authtype");
	$encrtype	= query("encrtype");
	
	if		($authtype=="OPEN")		{ $wpa=0;	$ieee8021x=0; $auth="open";}
	else if ($authtype=="SHARED")	{ $wpa=0;	$ieee8021x=0; $auth="shared";}
	else if ($authtype=="WPA")		{ $wpa=1;	$ieee8021x=1; $auth="WPA";}
	else if ($authtype=="WPAPSK")	{ $wpa=1;	$ieee8021x=0; $auth="WPAPSK";}
	else if ($authtype=="WPA2")		{ $wpa=2;	$ieee8021x=1; $auth="WPA2";}
	else if ($authtype=="WPA2PSK")	{ $wpa=2;	$ieee8021x=0; $auth="WPA2PSK";}
	else if ($authtype=="WPA+2")	{ $wpa=3;	$ieee8021x=1; $auth="WPA2";}
	else if ($authtype=="WPA+2PSK")	{ $wpa=3;	$ieee8021x=0; $auth="WPA2PSK";}
	
	if ($encrtype == "TKIP")			$encr="TKIP";
	else if ($encrtype == "AES")		$encr="AES";
	else if ($encrtype == "TKIP+AES")	$encr="AES";
	else if ($encrtype=="WEP")			{$encr="WEP"; $wep=1;}
	else 								$encr="none";
	
	if($wpa>0)
	{
		$key=query("nwkey/psk/key");
	}
	else if($wep>0)
	{
		$key=query("nwkey/wep/key");
	}
	
	fwrite("w+",$path,"<?xml version=\"1.0\"?>\n");
	fwrite("a+",$path,"<wirelessProfile xmlns=\"http://www.microsoft.com/provisioning/WirelessProfile/2004\">\n");
	fwrite("a+",$path,"	<config>\n");
	fwrite("a+",$path,"		<configId>".$config_id."");
	fwrite("a+",$path,"</configId>\n");
	fwrite("a+",$path,"		<configAuthorId>".$uuid."");
	fwrite("a+",$path,"</configAuthorId>\n");
	fwrite("a+",$path,"		<configAuthor>".$manufacturer."</configAuthor>\n");
	fwrite("a+",$path,"	</config>\n");
	fwrite("a+",$path,"	<ssid xml:space=\"preserve\">".$ssid."");
	fwrite("a+",$path,"</ssid>\n");
	fwrite("a+",$path,"	<connectionType>ESS</connectionType>\n");
//	fwrite("a+",$path,"	<channel2Dot4>0</channel2Dot4>\n");
//	$channel=query($phy_wlan2."/media/channel");
//	fwrite("a+",$path,"	<channel5Dot0>".$channel."</channel5Dot0>\n");
	$channel=query($phy_wlan1."/media/channel");
	fwrite("a+",$path,"	<channel2Dot4>".$channel."</channel2Dot4>\n");
	fwrite("a+",$path,"	<primaryProfile>\n");
	fwrite("a+",$path,"		<authentication>".$auth."");
	fwrite("a+",$path,"</authentication>\n");
	fwrite("a+",$path,"		<encryption>".$encr."");
	fwrite("a+",$path,"</encryption>\n");
	fwrite("a+",$path,"		<networkKey xml:space=\"preserve\">".$key."");
	fwrite("a+",$path,"</networkKey>\n");
	fwrite("a+",$path,"		<keyProvidedAutomatically>0</keyProvidedAutomatically>\n");
	fwrite("a+",$path,"		<ieee802Dot1xEnabled>0</ieee802Dot1xEnabled>\n");
	fwrite("a+",$path,"	</primaryProfile>\n");
	fwrite("a+",$path,"</wirelessProfile>\n");

}   
?>
