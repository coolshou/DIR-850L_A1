<? /* vi: set sw=4 ts=4: */
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";

function set_result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;
}

set_result("FAILED","","");
$rlt = "0";
$cnt = query($FATLADY_prefix."/acl/inbfilter/count");

$lan_run_inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0);
$lanip = query($lan_run_inf."/inet/ipv4/ipaddr");
$lanmask = query($lan_run_inf."/inet/ipv4/mask");

/*Care about guest zone*/
$lan2_run_inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-2", 0);
$lan2ip = query($lan2_run_inf."/inet/ipv4/ipaddr");
$lan2mask = query($lan2_run_inf."/inet/ipv4/mask");

if ($cnt > query("/acl/inbfilter/max"))
{
	set_result("FAILED", $FATLADY_prefix."/acl/inbfilter/count", i18n("The rules exceed maximum."));
	$rlt = "-1";
}
else
{
	foreach ($FATLADY_prefix."/acl/inbfilter/entry")
	{
		if ($InDeX > $cnt) break;
		$INX = $InDeX;
		foreach ($FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry")
		{
			$INX2 = $InDeX;
			if(INET_validv4addr(query("startip")) != 1)
			{
				set_result("FAILED",$FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/startip:".query("seq"),i18n("The start IP address is invalid."));
				$rlt = "-1";
				break;
			}	 
			if(INET_validv4addr(query("endip")) != 1)
			{
				set_result("FAILED",$FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/endip:".query("seq"),i18n("The end IP address is invalid."));
				$rlt = "-1";
				break;
			}
			
			if(ipv4networkid(query("startip"), $lanmask)==ipv4networkid($lanip, $lanmask) || ipv4networkid(query("startip"), $lan2mask)==ipv4networkid($lan2ip, $lan2mask))
			{
				set_result("FAILED",$FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/startip:".query("seq"),i18n("The start IP address could not be in LAN network."));
				$rlt = "-1";
				break;
			}
			if(ipv4networkid(query("endip"), $lanmask)==ipv4networkid($lanip, $lanmask) || ipv4networkid(query("endip"), $lan2mask)==ipv4networkid($lan2ip, $lan2mask))
			{
				set_result("FAILED",$FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/endip:".query("seq"),i18n("The end IP address could not be in LAN network."));
				$rlt = "-1";
				break;
			}			
			
			$delta = ipv4hostid(query("endip"), 0)-ipv4hostid(query("startip"), 0);
			if ($delta < 0)
			{
				set_result("FAILED",$FATLADY_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/startip:".query("seq"),i18n("The end IP address should be greater than the start IP address."));
				$rlt = "-1";
				break;
			}	
		}	
		if ($rlt=="-1") break;
	}
}

if ($rlt=="0")
{
	set($FATLADY_prefix."/valid", "1");
	set_result("OK", "", "");
}
?>
