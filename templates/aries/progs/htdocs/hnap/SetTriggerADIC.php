HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/phyinf.php";

function wandetect($inf)
{
	$infp    = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$phyinf    = query($infp."/phyinf");
	$phyinfp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	$ifname    = PHYINF_getifname($phyinf);
	$linkstatus = query($phyinfp."/linkstatus");

	if($linkstatus == "")
	{
		return "UNKNOWN";
	}
	setattr("/runtime/detectpppoe",  "get", "pppd pty_pppoe pppoe_discovery pppoe_device ".$ifname);
	$ret = query("/runtime/detectpppoe");
	del("/runtime/detectpppoe");
	if($ret=="yes")
	{
		return "PPPOE";
	}
	setattr("/runtime/detectdhcp",  "get", "udhcpc -i ".$ifname." -d -D 1 -R 2");
	$ret = query("/runtime/detectdhcp");
	del("/runtime/detectdhcp");
	if($ret=="yes")
	{
		return "DHCP";
	}
	return "UNKNOWN";
}

$testret = wandetect("WAN-1");
if($testret == "PPPOE")
{
	 $result="OK_PPPoE";
}
else if($testret == "DHCP")
{
	$result="OK_DHCP";
}
else
{
	$result="ERROR";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"><soap:Body><SetTriggerADICResponse xmlns="http://purenetworks.com/HNAP1/"><SetTriggerADICResult><?=$result?></SetTriggerADICResult></SetTriggerADICResponse></soap:Body></soap:Envelope>
