<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/trace.php";
include "/etc/services/PHYINF/phywifi.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($errno)	{startcmd("exit ".$errno); stopcmd("exit ".$errno);}

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

startcmd("killall hostapd > /dev/null 2>&1;");
startcmd("xmldbc -P /etc/services/WIFI/rtcfg.php -V PHY_UID=BAND24G-1.1 > /var/run/RT2860.dat");

stopcmd("killall hostapd > /dev/null 2>&1; sleep 1");

/* for each interface. */
$i= 1;
while ($i>0)
{
	$uid = "BAND24G-1.".$i;
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	if ($p=="") {$i=0; break;}

	$active = query($p."/active");
	$dev = devname($uid);
	if ($i==1 && $active!=1)
	{
		startcmd("# The major interface (".$uid.") is not active.");
		//break;
		$i++;
		continue;
	}
	
	/* special command for guestzone 
	 * Guestzone interface can only be brought up IF hostzone interface HAS BEEN BROUGHT UP BEFORE ! 
	 * So when we meet condition :
	 * 	1. hostzone intf hasn't up
	 *  2. guestzone need to up 
	 *  --> We just bring the hostzone intf up and bring it down again. 
	 *
	 * Remember that this kind of situation happens when : 
	 *  1. Hostzone is disabled from web, while guestzone is enabled. 
	 *  2. Hostzone is disabled because of schedule, while guestzone is enabled.
	 */

	$up = fread("", "/var/run/BAND24G-1.1.UP");
	if($i==2 && $active==1 && $up=="")
	{
		startcmd("ip link set ".devname('BAND24G-1.1')." up");
		startcmd("ip link set ".devname('BAND24G-1.1')." down");
	}
		
	$down = fread("", "/var/run/".$uid.".DOWN"); $down+=0;
	if ($down=="1") startcmd("# ".$uid." has been shutdown.");
	else
	{
		if ($active!=1) startcmd("# ".$uid." is inactive!");
		else
		{
			startcmd("# ".$uid.", dev=".$dev);
			PHYINF_setup($uid, "wifi", $dev);
			$brdev = find_brdev($uid);

		//	startcmd("ip link set ".$dev." up");

			/* bring up guestzone bridge */
			if(isguestzone($uid)=="1")
			{
				startcmd("ip link set ".$brdev." up");
			}			

			/* set smaller tx queue len */
			startcmd("ifconfig ".$dev." txqueuelen 250");

			if ($brdev!="") startcmd("brctl addif ".$brdev." ".$dev);
			startcmd("phpsh /etc/scripts/wifirnodes.php UID=".$uid);
			
			/* +++ upwifistats */
			startcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
			startcmd("phpsh /var/run/restart_upwifistats.sh");
			/* --- upwifistats */

			/* Note : we let dev down only in BAND24G-X.X service.
			 * Example if we want to bring 1.2 up : 
			 * 	Advantage    : if 1.1 already up, it wouldn't be brought down. If brought down, wl clients associated to 
			 *                 1.1 will be disconnected.
			 * 	Disadvantage : Sometimes print error messages in console. If 1.1 already up, when bringing 1.2 up, will 
			 *				   cause modules can't be removed.  */
			//stopcmd("ip link set ".$dev." down");
			stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);
		}
	}
	if ($i==1)
	{
		startcmd("phpsh /etc/scripts/wpsevents.php ACTION=ADD");
		startcmd("event BAND24G-1.LED.ON");

		stopcmd("event BAND24G-1.LED.OFF");
		stopcmd("phpsh /etc/scripts/wpsevents.php ACTION=FLUSH");
		stopcmd('xmldbc -t \"close_WPS_led:3:event WPS.NONE\"\n');
	}
	$i++;
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


startcmd("service MULTICAST restart");
stopcmd("service MULTICAST restart");

error(0);

?>
