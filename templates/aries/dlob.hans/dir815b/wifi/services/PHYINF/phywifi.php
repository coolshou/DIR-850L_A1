<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}

/**********************************************************************/
function devname($uid)
{
	if ($uid=="BAND24G-1.1")	return "ra0";
	else if ($uid=="BAND24G-1.2")	return "ra1";	
	else if ($uid=="BAND5G-1.1")	return "rai0";
	else if ($uid=="BAND5G-1.2")	return "rai1";							
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
	if		($prefix=="BAND24G")	$drv="RT3352";	
	if		($prefix=="BAND5G")	 	$drv="RT3572";	

	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	if ($p=="" || $drv=="" || $dev=="")		return error(9);
	if (query($p."/active")!=1) return error(8);
	
	if(host_guest_dependency_check($uid)==0)	return error(8);

	startcmd("rm -f /var/run/".$uid.".DOWN");
	startcmd("echo 1 > /var/run/".$uid.".UP");
	startcmd("service ".$drv." restart");

	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);
	
	stopcmd("ip link set ".$dev." down");
	stopcmd("echo 1 > /var/run/".$uid.".DOWN");
	stopcmd("rm -f /var/run/".$uid.".UP");
	
	/* +++ upwifistats, modified by Jerry Kao. */
	if ($prefix=="BAND24G")
	{
		stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats_24g.sh;");
		stopcmd("phpsh /var/run/restart_upwifistats_24g.sh");
	}
	if ($prefix=="BAND5G")
	{
		stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$uid." > /var/run/restart_upwifistats_5g.sh;");
		stopcmd("phpsh /var/run/restart_upwifistats_5g.sh");
	}	
	/* --- upwifistats */	
	
	/* for closing guestzone bridge */			
	if(isguestzone($uid)=="1")
	{
		//get brname of guestzone
		$brname = find_brdev($uid);		
		
		if ($prefix=="BAND24G")
		{			
			stopcmd("if [ -f /var/run/BAND5G-1.2.UP ]; then");
			stopcmd("ip link set ra1 down");
			stopcmd("else");
		}
		if ($prefix=="BAND5G")
		{			
			stopcmd("if [ -f /var/run/BAND24G-1.2.UP ]; then");
			stopcmd("ip link set rai1 down");
			stopcmd("else");
		}
		
		stopcmd("ip link set ".$brname." down");
		stopcmd("fi");
	}
	return error(0);
}

?>
