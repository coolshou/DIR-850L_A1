HTTP/1.1 200 OK
Content-Type: text/xml
<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/trace.php";
function addevent($name,$handler)
{
	$cmd = $name." add \"".$handler."\"";
	event($cmd);
}

function wandetectIPV4($inf)
{
	$infp    = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$phyinf    = query($infp."/phyinf");
	$phyinfp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	$ifname    = PHYINF_getifname($phyinf);
	$linkstatus = query($phyinfp."/linkstatus");

	if($linkstatus == "")
	{
		return "ERROR";
	}
	del("/runtime/wanispppoe");
	del("/runtime/wanisdhcp");
	addevent("detectpppoe","xmldbc -s /runtime/wanispppoe \\`pppd pty_pppoe pppoe_discovery pppoe_device ".$ifname."\\` &");
	addevent("detectdhcp","xmldbc -s /runtime/wanisdhcp \\`udhcpc -i ".$ifname." -d -D 1 -R 3\\`&");
	event("detectpppoe");
	event("detectdhcp");
	return "OK";
}
function wanlinkstatus($inf)
{
	$infp = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$phyinf = query($infp."/phyinf");
	$phyinfp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	$linkstatus = query($phyinfp."/linkstatus"); 	
	if($linkstatus == "")
	{
		return "down";
	}
	return $linkstatus;
}
function fail($reason)
{
	$_GLOBALS["RESULT"] = "FAIL";
	$_GLOBALS["REASON"] = $reason;
}
if ($AUTHORIZED_GROUP < 0)
{
	$RESULT = "FAIL";
	$REASON = i18n("Permission deny. The user is unauthorized.");
}
else if ($_POST["action"] == "WANDETECT")
{
	$RESULT = wandetectIPV4("WAN-1");
	$REASON = "";			
	if($RESULT!="OK")
	{
		$RESULT = "linkdown";
	}
}
else if ($_POST["action"] == "WANDETECTV6")
{
	del("/runtime/services/wandetect6");	
	event("WANV6.DETECT");
	$RESULT = "OK";
	$REASON = "";			
}
else if ($_POST["action"] == "WANTYPERESULT")
{
	$link = wanlinkstatus("WAN-1");
	if($link == "down")
	{
		$RESULT = "linkdown";
	}
	else
	{
		$isPPPoE = query("/runtime/wanispppoe");
		$isDHCP = query("/runtime/wanisdhcp");
		
		if($isPPPoE == "yes")
		{
			$RESULT = "PPPoE";
		}
		/*if pppoe not finish we need wait it.*/
		else if($isPPPoE !="" && $isDHCP== "yes")
		{
			$RESULT = "DHCP";	
		}
		else if($isDHCP != "" && $isPPPoE != "")
		{
			$RESULT = "unknown";
		}
		else
		{
			$RESULT = "detecting";
		}
	}
	$REASON = "";	
}
else if ($_POST["action"] == "WANTYPERESULTV6")
{
	$RESULT = query("/runtime/services/wandetect6/wantype");
	$RESON = "";
}
else fail(i18n("Unknown ACTION!"));
?>
<?echo '<?xml version="1.0" encoding="utf-8"?>';?>
<wandetectreport>
	<action><?echo $_POST["action"];?></action>
	<result><?=$RESULT?></result>
	<reason><?=$REASON?></reason>
</wandetectreport>
