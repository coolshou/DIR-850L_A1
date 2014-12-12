<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}

/**********************************************************************/
function devname($uid)
{
	if ($uid=="BAND24G-1.1")	return "wifig0";
	else if ($uid=="BAND24G-1.2")	return "wifig0.1";
	else if ($uid=="BAND5G-1.1")	return "wifia0";
	else if ($uid=="BAND5G-1.2")	return "wifia0.1";
	return "";
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

function isguestzone($uid)
{
	$postfix = cut($uid, 1,"-");
	$minor = cut($postfix, 1,".");
	if($minor=="2")	return 1;
	else			return 0;
}

function get_parent_wifiid($uid)
{
    if (isguestzone($uid) == 1)
    {
        $prefix = cut($uid, 0, ".");
        $uid = $prefix.".1";
    }
    return $uid;
}

function get_parent_wifi($uid)
{
    if (isguestzone($uid) == 1)
    {
        $prefix = cut($uid, 0, ".");
        $uid = $prefix.".1";
    }
    return devname($uid);
}

function isband5g($uid)
{
    $prefix = cut($uid, 0, "-");
    if ($prefix=="BAND5G")
    {
        return 1;
    } else {
        return 0;
    }
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

/* broadcom */
function wifi_service($wifi_uid)
{
    $pwinfname = get_parent_wifi($wifi_uid);
    $pwinfid = get_parent_wifiid($wifi_uid);
    $is_guest_zone = isguestzone($wifi_uid);
    $clean_output = " 1>/dev/null 2>&1";
	$prefix = cut($wifi_uid, 0,"-");
	if ($prefix=="BAND24G") $drv="RT2860";
	if ($prefix=="BAND5G") $drv="RT3572";

	startcmd("xmldbc -P /etc/services/WIFI/rtcfg.php -V PHY_UID=".$wifi_uid." > /var/run/".$drv.".dat");
	/* special command for guestzone
     * Guestzone interface can only be brought up IF hostzone interface HAS BEEN BROUGHT UP BEFORE !
     * So when we meet condition :
     *  1. hostzone intf hasn't up
     *  2. guestzone need to up
     *  --> We just bring the hostzone intf up and bring it down again.
     *
     * Remember that this kind of situation happens when :
     *  1. Hostzone is disabled from web, while guestzone is enabled.
     *  2. Hostzone is disabled because of schedule, while guestzone is enabled.
     */
	if($is_guest_zone=="1") 
	{
		startcmd("ip link set ".$pwinfname." up");
	}
    $up = fread("", "/var/run/".$pwinfid.".UP");
	if($is_guest_zone=="1" && $up=="")
	{
		startcmd("ip link set ".$pwinfname." down");
	}
}

function wificonfig($uid)
{
    fwrite(w,$_GLOBALS["START"], "#!/bin/sh\n");
	fwrite(w,$_GLOBALS["STOP"],  "#!/bin/sh\n");
	
	$dev	= devname($uid);
	$is_guest_zone = isguestzone($uid);
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

	/* bring up guestzone bridge */
	if ($brdev!="" && $is_guest_zone=="1")
	{
		startcmd("ip link set ".$brdev." up");
	}

	wifi_service($uid);

	/* if ralink interface can not add to bridge */
	/* need patch rt_linux.c at ralink_driver, not just up the wifi interface */

	/* set smaller tx queue len */
	startcmd("ifconfig ".$dev." txqueuelen 250");

	/* todo need to bring all wireless to it's bridge interface */
	if ($brdev!="")
	{
		startcmd("brctl addif ".$brdev." ".$dev);
		/* set multicast bandwidth limit to 900 */
		startcmd("brctl setbwctrl ".$brdev." ".$dev." 900");
	}
	startcmd("phpsh /etc/scripts/wifirnodes.php UID=".$uid);

	/* +++ upwifistats */
	startcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
	startcmd("phpsh /var/run/restart_upwifistats.sh");
	/* --- upwifistats */

	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);
	
	stopcmd("ip link set ".$dev." down");
	stopcmd("echo 1 > /var/run/".$uid.".DOWN");
	stopcmd("rm -f /var/run/".$uid.".UP");
	
	/* +++ upwifistats */
	stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
	stopcmd("phpsh /var/run/restart_upwifistats.sh");
	/* --- upwifistats */	
	
	/* for closing guestzone bridge */
	if(isguestzone($uid)=="1")
	{
		$up2g = fread("", "/var/run/BAND24G-1.2.UP");
		$up5g = fread("", "/var/run/BAND5G-1.2.UP");
		if ($up2g=="" && $up5g=="")
		{
			//get brname of guestzone
			$brname = find_brdev($uid);
			stopcmd("ip link set ".$brname." down");
		}
	}

    /* wps */
    if ($devidx==1)
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
        /*if enable gzone this action will run 4 time when wifi restart.
      we pending this action in 3 seconds..................
      all restart actions in 3 seconds ,we just run 1 time....
	    */
	startcmd('xmldbc -k "HOSTAPD_RESTARTAP"');
	startcmd('xmldbc -t "HOSTAPD_RESTARTAP:3:sh /etc/scripts/restartap_hostapd.sh"');
	    
        //startcmd("xmldbc -P /etc/services/WIFI/hostapdcfg.php > /var/topology.conf");
        //startcmd("hostapd /var/topology.conf &");
    }
    startcmd("service MULTICAST restart");
    stopcmd("service MULTICAST restart");

	return error(0);
}

?>
