<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
if (query("/runtime/device/layout")!="router")
{
	$_GLOBALS["errorCode"]=501;
}
else
{
	anchor($_GLOBALS["ACTION_NODEBASE"]."/DeletePortMapping");
	$NewRemoteHost		= query("NewRemoteHost");
	$NewExternalPort	= query("NewExternalPort");
	$NewProtocol		= query("NewProtocol");

	$target = 0;
	foreach ("/runtime/upnpigd/portmapping/entry")
	{
		if ($NewProtocol == "TCP")	$proto = "TCP";
		else						$proto = "UDP";
		if ($NewRemoteHost		== query("remotehost") &&
			$NewExternalPort	== query("externalport") &&
			$proto				== query("protocol"))
		{
			$target = $InDeX;
			break;
		}
	}
	if ($target != 0)
	{
		/* contruct the iptable rule to delete. */
		anchor("/runtime/upnpigd/portmapping/entry:".$target);
		if (query("enable")==1)
		{
			$remotehost = get("s", "remotehost");
			if ($remotehost != "") $sourceip = ' -s "'.$remotehost.'"';
			if (query("protocol") == "TCP")	$proto = ' -p tcp';
			else							$proto = ' -p udp';
			$extport = query("externalport");
			$intport = query("internalport");
			$intclnt = query("internalclient");

			$cmd =	'iptables -t nat -D DNAT.UPNP'.$proto.' --dport '.$extport.
					' -j DNAT --to-destination "'.$intclnt.'":'.$intport;
			SHELL_info("a", $_GLOBALS["SHELL_FILE"], "UPNP:".$cmd);
			fwrite("a", $_GLOBALS["SHELL_FILE"], $cmd."\n");
			
			$infp = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
			if($infp!="")
			{
				$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);       
				anchor($stsp."/inet");
        		$addrtype = query("addrtype");
        		if              ($addrtype=="ipv4" && query("ipv4/valid")=="1") $ipaddr=query("ipv4/ipaddr");
        		else if ($addrtype=="ppp4" && query("ppp4/valid")=="1") $ipaddr=query("ppp4/local");
        		
				fwrite("a", $_GLOBALS["SHELL_FILE"], "sh /etc/scripts/upnpc.sh del ".$ipaddr." ".$extport." ".$proto."\n");
			}
		}
		XNODE_del_entry("/runtime/upnpigd/portmapping", $target);
		$_GLOBALS["errorCode"]=200;
	}
	else
	{
		$_GLOBALS["errorCode"]=714;
	}
}
?>
