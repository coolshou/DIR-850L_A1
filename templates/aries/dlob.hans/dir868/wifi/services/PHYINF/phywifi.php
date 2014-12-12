<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/webinc/config.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}

/**********************************************************************/
// function: get_globals_devname
// $uid: the uid in /phyinf nodes
// return value: device name (ex: wifia0, wifig0 ...)
function get_globals_devname($uid)
{
	if($uid == "BAND24G-1.1")
		return $_GLOBALS["BAND24G_DEVNAME"];
	else if($uid == "BAND24G-1.2")
		return $_GLOBALS["BAND24G_GUEST_DEVNAME"];
	else if($uid == "BAND5G-1.1")
		return $_GLOBALS["BAND5G_DEVNAME"];
	else if($uid == "BAND5G-1.2")
		return $_GLOBALS["BAND5G_GUEST_DEVNAME"];
	else
		return "";
}

function isguestzone($uid)
{
	$postfix = cut($uid, 1,"-");
	$minor = cut($postfix, 1,".");
	if($minor=="2")	return 1;
	else			return 0;
}

// function: get_phyinf_sta_mode
// $uid: the uid in /phyinf nodes 
function get_phyinf_sta_mode($wifi_uid)
{
	$phy = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid);
	if($phy == "")
		return "0";

	$wifi = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy."/wifi"));
	if($wifi == "")
		return "0";

	$opmode = query($wifi."/opmode");
	if($opmode == "STA")
		return "1";
	else
		return "0";
}

function get_parent_phy_uid($uid)
{
	if (isguestzone($uid) == 1)
	{
		$prefix = cut($uid, 0, ".");
		$uid = $prefix.".1";
	}

	return $uid;
}

// function: get_phyinf_freq
// $uid: the uid in /phyinf nodes
function get_phyinf_freq($uid)
{
	// a guest zone interface has no freq information, get it from host zone interface
	$uid = get_parent_phy_uid($uid);

	$phy = XNODE_getpathbytarget("", "phyinf", "uid", $uid);
	if($phy == "")
		return "";

	if(query($phy."/type") != "wifi")
		return "";

	$freq = query($phy."/media/freq");
	return $freq;
}

// function: devname
// $uid: the uid in /phyinf nodes
function devname($uid)
{
	//station mode or not
	$sta_mode = get_phyinf_sta_mode($uid);
	if($sta_mode == "1")
	{
		//is the interface 2.4g or 5g?
		$freq = get_phyinf_freq($uid);

		if($freq == "2.4")
			return $_GLOBALS["BAND24G_DEVNAME"];
		else if($freq == "5")
			return $_GLOBALS["BAND5G_DEVNAME"];
		else
			return "";
	}
	else
	return get_globals_devname($uid);
}

/* what we check ?
1. if host is disabled, then our guest must also be disabled !!
*/
function host_guest_dependency_check($uid)
{
	if($uid == "BAND24G-1.2") 			$host_uid = "BAND24G-1.1";
	else if ($uid == "BAND5G-1.2")		$host_uid = "BAND5G-1.1";
	else return 1;
	
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $host_uid, 0);
	if (query($p."/active")!=1) return 0;
	else 						return 1;
}

function get_parent_wifi($uid)
{
	$uid = get_parent_phy_uid($uid);
	return devname($uid);
}

function isband5g($uid)
{
	if(get_phyinf_freq($uid) == "5")
		return 1;
	else
		return 0;
}

function find_brdev($phyinf)
{
	foreach ("/runtime/phyinf")
	{
		if (query("type")!="eth") continue;
		foreach ("bridge/port") if ($VaLuE==$phyinf) {$find = "yes"; break;}
		if ($find=="yes") return query("name");
	}
	return "";
}

function get_wlif_bss($UID)
{
	$is_band_5g=isband5g($UID);
	$is_guest_zone=isguestzone($UID);
	$wlif_bss="";
	if ($is_band_5g==1)
	{
		$wlif_bss="1";
	} else {
		$wlif_bss="0";
	}
	if ($is_guest_zone==1)
	{
		$wlif_bss=$wlif_bss.".1";
	}
	return $wlif_bss;
}

/* broadcom */
function wifi_service($wifi_uid)
{	
	$pwinfname = get_parent_wifi($wifi_uid);  
	$is_guest_zone = isguestzone($wifi_uid);
	$clean_output = " 1>/dev/null 2>&1";

	/* config file of wireless driver */
	startcmd(
			'xmldbc -P /etc/services/WIFI/rtcfg.php -V PHY_UID='.$wifi_uid.' -V ACTION=START > /var/run/'.$wifi_uid.'.cfg\n'.
			'chmod 777 /var/run/'.$wifi_uid.'.cfg\n'.
			'sh /var/run/'.$wifi_uid.'.cfg\n'
		  );

	/* Start up this interface, but will not up this interface, hostapd will up it later. */
	startcmd(
		'wlconf '.$pwinfname.' down'.$clean_output.'\n'.
		'wlconf '.$pwinfname.' up'.$clean_output.'\n');

	$phy= XNODE_getpathbytarget("",	"phyinf", "uid", $wifi_uid);
	anchor($phy."/media");		
	$channel		= query("channel");		
	if($channel=="0" && $wifi_uid!="WIFI-STA")
	{
		startcmd('acsd\n');	
	}
	
	startcmd('wlconf '.$pwinfname.' start'.$clean_output.'\n');

	if($channel=="0" && $wifi_uid!="WIFI-STA")
	{	
		startcmd('killall acsd\n');
	}
	/* down the interface */
	stopcmd(
			'xmldbc -P /etc/services/WIFI/rtcfg.php -V PHY_UID='.$wifi_uid.' -V ACTION=STOP > /var/run/'.$wifi_uid.'.cfg\n'.
			'chmod 777 /var/run/'.$wifi_uid.'.cfg\n'.
			'sh /var/run/'.$wifi_uid.'.cfg\n'
		  );
	stopcmd( 'wlconf '.$pwinfname.' down'.$clean_output.'\n' );
	
	if(get_phyinf_sta_mode($wifi_uid) == "1")	
	{
		startcmd(
			'killall eapd\n'.
			'killall nas'
		);

		startcmd('nas;eapd&');

		stopcmd(
			'killall eapd\n'.
			'killall nas'
		);
	}

	if ($is_guest_zone == 1)
	{
		stopcmd(
				'wlconf '.$pwinfname.' up'.$clean_output.'\n'.
				'wlconf '.$pwinfname.' start'.$clean_output.'\n'
			   );
	}
}

function wificonfig($uid)
{
    fwrite(w,$_GLOBALS["START"], "#!/bin/sh\n");
	fwrite(w,$_GLOBALS["STOP"],  "#!/bin/sh\n");
	
	if($uid=="WIFI-STA")
	{
		
		$wps_en=query("/runtime/wps_sta/enable");//marco
		$sta_p = XNODE_getpathbytarget("", "phyinf", "uid", "WIFI-STA", 0);
		if($wps_en==1)
		{			
			startcmd("xmldbc -k close_WPS_led");
			startcmd("event WPS.INPROGRESS");
			$cur_freq=query($sta_p."/media/freq");
			set("/runtime/wps_sta/freq",$cur_freq);
			set($sta_p."/media/freq","5");
		}
		else if($wps_en==2)
		{
			startcmd("xmldbc -k close_WPS_led");
			set($sta_p."/media/freq","2.4");
		}
		else if($wps_en==3)
		{
			$org_freq=query("/runtime/wps_sta/freq");
			set($sta_p."/media/freq",$org_freq);
		}
	}

	$dev	= devname($uid);
	$wpsprefix = cut($uid, 0,".");
	$devidx = cut($uid, 1,".");

	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	if ($p=="" || $dev=="")		return error(9);
	if (query($p."/active")!=1) return error(8);
	
	if(host_guest_dependency_check($uid)==0)	return error(8);
	/* hostapd */
	startcmd("killall hostapd > /dev/null 2>&1;");
	stopcmd("killall hostapd > /dev/null 2>&1; sleep 1");

	startcmd("rm -f /var/run/".$uid.".DOWN");
	startcmd("echo 1 > /var/run/".$uid.".UP");

	startcmd("# ".$uid.", dev=".$dev);
	PHYINF_setup($uid, "wifi", $dev);
	$brdev = find_brdev($uid);
	wifi_service($uid);

	/* set smaller tx queue len */
	startcmd("ifconfig ".$dev." txqueuelen 250");

	if(get_phyinf_sta_mode($uid) == "1")
	{
		startcmd("ifconfig ".$dev." up");
		stopcmd("ifconfig ".$dev." down");
	}

	/* todo need to bring all wireless to it's bridge interface */
	if ($brdev!="")
	{
		startcmd("brctl addif ".$brdev." ".$dev);
		/* set multicast bandwidth limit to 900 */
		startcmd("brctl setbwctrl ".$brdev." ".$dev." 900");
	}
	startcmd("phpsh /etc/scripts/wifirnodes.php UID=".$uid);

	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);
	
	stopcmd("echo 1 > /var/run/".$uid.".DOWN");
	stopcmd("rm -f /var/run/".$uid.".UP");
	
	/* +++ upwifistats */
	stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
	stopcmd("phpsh /var/run/restart_upwifistats.sh");
	
	startcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
	startcmd("phpsh /var/run/restart_upwifistats.sh");
	/* --- upwifistats */	
	
	/* wps */
    if ($devidx==1|| $uid=="WIFI-STA")
    {
        startcmd("phpsh /etc/scripts/wpsevents.php ACTION=ADD");
        startcmd("event ".$wpsprefix.".LED.ON");

        stopcmd("event ".$wpsprefix.".LED.OFF");
        stopcmd("phpsh /etc/scripts/wpsevents.php ACTION=FLUSH");
        stopcmd('xmldbc -t \"close_WPS_led:3:event WPS.NONE\"\n');
    }

	/*
	   Win7 logo patch. Hostapd must be restarted ONLY once..!
	 */
	if(query("runtime/hostapd_restartap")=="1")
	{
		startcmd('xmldbc -k "HOSTAPD_RESTARTAP"');
		startcmd('xmldbc -t "HOSTAPD_RESTARTAP:5:sh /etc/scripts/restartap_hostapd.sh"');
	}
	else
	{
		startcmd("xmldbc -P /etc/services/WIFI/hostapdcfg.php > /var/topology.conf");
		startcmd("hostapd /var/topology.conf &");
	}
	stopcmd("phpsh /etc/services/WIFI/interfacereboot.php UID=".$uid."");
	startcmd("service MULTICAST restart");
	stopcmd("service MULTICAST restart");
	anchor($p."/media");
	$txpower		= query("txpower");
	if ($txpower == "100" ) { startcmd("wl -i ".$dev." pwr_percent 100"); }
	else if ($txpower == "50" ) { startcmd("wl -i ".$dev." pwr_percent 80"); }
	else if ($txpower == "25" ) { startcmd("wl -i ".$dev." pwr_percent 60"); }
	else if ($txpower == "12.5" ) { startcmd("wl -i ".$dev." pwr_percent 40"); }
	else					{ startcmd("wl -i ".$dev." pwr_percent 95"); }

//marco, start wps_monitor
	if($uid=="WIFI-STA")
	{
		
		if($wps_en=="1" || $wps_en=="2")
		{
			stopcmd("killall wps_monitor");
			startcmd("wps_monitor&");
		}
	}
	return error(0);
}

?>
