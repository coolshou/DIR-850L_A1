<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}

/**********************************************************************/
function devname($uid)
{
	if ($uid=="BAND24G-1.1")	return "wlan0";
	else if ($uid=="BAND24G-1.3")   return "wlan0-vxd";
	return "";
}

/* what we check ?
1. if host is disabled, then our guest must also be disabled !!
*/
function host_guest_dependency_check($uid)
{
	if($uid == "BAND24G-1.2") 			$host_uid = "BAND24G-1.1";
	else if ($uid == "BAND24G-1.3")      $host_uid = "BAND24G-1.1";
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

function wificonfig($uid)
{
    fwrite(w,$_GLOBALS["START"], "#!/bin/sh\n");
	fwrite(w,$_GLOBALS["STOP"],  "#!/bin/sh\n");
	
	$dev	= devname($uid);
	$prefix = cut($uid, 0,"-");
	if		($prefix=="BAND24G")	$drv="WIFI";	
	if		($prefix=="BAND5G")		$drv="";	

	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	if ($p=="" || $drv=="" || $dev=="")		return error(9);
	if (query($p."/active")!=1) return error(8);
	$wifi = XNODE_getpathbytarget("/wifi",  "entry",  "uid", query($p."/wifi"), 0);
	$opmode = query($wifi."/opmode");
	
	if(host_guest_dependency_check($uid)==0)	return error(8);

	startcmd("rm -f /var/run/".$uid.".DOWN");
	startcmd("echo 1 > /var/run/".$uid.".UP");
	startcmd("phpsh /etc/scripts/wifirnodes.php UID=".$uid);
	startcmd("service ".$drv." restart");
	
	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);
	
	stopcmd("ip link set ".$dev." down");
	stopcmd("rm -f /var/run/".$uid.".UP");
	if($opmode == "AP")
	{
		/* +++ upwifistats */
		stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats.sh;");
		stopcmd("sh /var/run/restart_upwifistats.sh");
		/* --- upwifistats */	
	}
	/* for closing guestzone bridge */
	if(isguestzone($uid)=="1")
	{
		//get brname of guestzone
		$brname = find_brdev($uid);
		stopcmd("ip link set ".$brname." down");
	}
	stopcmd("service ".$drv." stop");
	stopcmd("echo 1 > /var/run/".$uid.".DOWN");
	return error(0);
}

?>
