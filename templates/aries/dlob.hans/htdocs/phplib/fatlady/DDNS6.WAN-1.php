<? /* vi: set sw=4 ts=4: */
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/inet6.php";

function set_result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;

	return $result;
}

function verify_v6ip($node)
{
	TRACE_debug($node);
	foreach ($node."/ddns6/entry")
	{
		$ip=query("v6addr");
		TRACE_debug($ip);
		if (INET_validv6addr($ip) == 0)
		{
			return set_result("FAILED", "", i18n("Invalid IPv6 address"));
		}
		
		$type = INET_v6addrtype($ip);
		
		TRACE_debug("FATLADY: INET_IPV6: ipv6 type = ".$type);
	
		if($type=="ANY" || $type=="MULTICAST" || $type=="LOOPBACK")
			return set_result("FAILED", "", i18n("Invalid IPv6 address"));
		$hostname=query("hostname");	
		if($hostname=="")
		{
			return set_result("FAILED","", i18n("Invalid hostname") );
		}
		
	}
	return "OK";
}
if(verify_v6ip($FATLADY_prefix)=="OK")
{
	set($FATLADY_prefix."/valid", "1");
	set_result("OK", "", "");
}

?>
