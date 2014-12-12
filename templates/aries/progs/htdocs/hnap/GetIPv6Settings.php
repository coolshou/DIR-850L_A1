HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/webinc/config.php";

function findthelastipv6($str)
{
	$i=1;
	while($i > 0)
	{
		$lastipv6=scut($str, $i-1, "");
		if(scut($str, $i, "")=="") break;
		else $i++;
	}
	return	$lastipv6;
}
function LanIPv6AddressRange($network, $start_addr, $count, $start_or_end)
{
	$start_ip_value = scut($start_addr, 0, $network);
	if($start_or_end == "start")
	{
		if(strlen($start_ip_value)==2) $start_ip_value = "00".$start_ip_value;
		else if (strlen($start_ip_value)==1) $start_ip_value = "000".$start_ip_value;
		$LanIPv6Addr = $network.$start_ip_value;
	}	
	else if($start_or_end == "end")
	{
		$end_ip_value = strtoul($start_ip_value, 16) + $count - 1;	
		$end_ip_value = dec2strf('%x', $end_ip_value);
		if(strlen($end_ip_value)==2) $end_ip_value = "00".$end_ip_value;
		else if (strlen($end_ip_value)==1) $end_ip_value = "000".$end_ip_value;
		$LanIPv6Addr = $network.$end_ip_value;
	}
	else return false;

	return $LanIPv6Addr;
}

/*<--InitWAN()  */
$wan = $WAN4;
$wan1 = INF_getinfpath($WAN1);
$wan3 = INF_getinfpath($WAN3);
$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN4, "0");		
$is_ppp6 = 0;
$is_ppp10 = 0;
$wan3inetp = INET_getpathbyinf($WAN3);
$wan1inetp = INET_getpathbyinf($WAN1);
$DEBUG_HNAP = "y"; //Sammy
if($DEBUG_HNAP == "y")
{
	if($wan1inetp=="")	TRACE_debug("$wan1inetp is empty.");
	if($wan3inetp=="")	TRACE_debug("$wan3inetp is empty.");
	TRACE_info("query($wan3inetp.'/addrtype') = ".query($wan3inetp."/addrtype"));
	TRACE_info("query($wan1inetp.'/addrtype') = ".query($wan1inetp."/addrtype"));
}	

if(query($wan3inetp."/addrtype") == "ppp6")
{
	$wan = $WAN3;
	$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN3, "0");
	$is_ppp6 = 1;
}
if(query($wan1inetp."/addrtype") == "ppp10")
{
	$wan = $WAN1;
	$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN1, "0");
	$is_ppp10 = 1;
}
$waninetuid = query(INF_getinfpath($wan)."/inet");
$wanphyuid = query(INF_getinfpath($wan)."/phyinf");
$waninetp = XNODE_getpathbytarget("/inet", "entry", "uid", $waninetuid, "0");
$rwanphyp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wanphyuid, "0");
if($DEBUG_HNAP == "y")
{
	TRACE_info("1: $wan = ".$wan);
	TRACE_info("1: $rwan = ".$rwan);
	if($waninetuid=="")	TRACE_debug("$waninetuid is empty.");
	if($wanphyuid=="")	TRACE_debug("$wanphyuid is empty.");
	if($waninetp=="")	TRACE_debug("$waninetp is empty.");
	if($rwanphyp=="")	TRACE_debug("$rwanphyp is empty.");
}

if(query(INF_getinfpath($wan)."/active") == "0" && $is_ppp6 != 1 && $is_ppp10 != 1)
{
	$wan = $WAN3;
	$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN3, "0");
	$is_ll = 1;
}

$wancable_status=0;
if(query($rwanphyp."/linkstatus")!="0" && query($rwanphyp."/linkstatus")!="")
{
   $wancable_status=1;
}

$rstlwan = $rwan."/stateless";
$rwan_6rd = $rwan;//If IPv6 WAN type is 6RD, It would be used.
$rwan = $rwan."/inet";
if ($is_ll == 1)
{
	$str_wantype = "Link-Local";
	$str_wanipaddr = "";
	$str_wanprefix = "";
	$str_wangateway = "";
	$str_wanDNSserver = "";
	$str_wanDNSserver2 = "";
}
else if (query($waninetp."/addrtype")=="ipv6" && $wancable_status==1)
{
	$str_wantype = query($waninetp."/ipv6/mode");
	if (query($rwan."/ipv6/ipaddr")!="")
	{
		$str_wanipaddr = query($rwan."/ipv6/ipaddr");
		$str_wanprefix = query($rwan."/ipv6/prefix");
		$str_wangateway = query($rwan."/ipv6/gateway");
		/* Added by Joseph */
		if ($str_wantype=="6RD")
		{
			$waninetp_6rd = INET_getpathbyinf(query($rwan_6rd."/uid"));
			if(query($waninetp_6rd)."/ipv6/ipv6in4/rd/ipaddr"!="") $str_6rd_Conf = "Manual";
			else $str_6rd_Conf = "Dhcpv4Option";
			
			$str_6rd_IPv6Prefix = query($rwan."/ipv6/ipv6in4/rd/ipaddr");
			$str_6rd_IPv6PrefixLength = query($rwan."/ipv6/ipv6in4/rd/prefix");
			$str_6rd_IPv4MaskLength = query($rwan."/ipv6/ipv6in4/rd/v4mask");			
		}
		else if ($str_wantype=="6IN4")
		{
			$str_6in4_RemoteIPv4 = query($waninetp."/ipv6/ipv6in4/remote");
		}
	}
	else
	{
		$str_wanipaddr = query($rwanphyp."/ipv6/global/ipaddr");
		$str_wanprefix = query($rwanphyp."/ipv6/global/prefix");
		$str_wangateway = query($rstlwan."/gateway");
	}
	if(query($rwan."/ipv6/dns:1")!="") $str_wanDNSserver = query($rwan."/ipv6/dns:1");
	else $str_wanDNSserver = "";
	if(query($rwan."/ipv6/dns:2")!="") $str_wanDNSserver2 = query($rwan."/ipv6/dns:2");
	else $str_wanDNSserver2 = "";
	
	if($str_wantype == "AUTO")
	{
		//if wan has more than one ipaddr, we need to get it by runtime phyinf and get the last one
		$str_wanipaddr = query($rwanphyp."/ipv6/global/ipaddr");
		$str_wanipaddr = findthelastipv6($str_wanipaddr);
		$str_wanprefix = query($rwanphyp."/ipv6/global/prefix");
		$str_wanprefix = findthelastipv6($str_wanprefix);
	}
}
else if ($is_ppp6 == 1 && $wancable_status==1)
{
	$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN3, "0");
	$rwan = $rwan + "/inet";
	$rwan4 = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN4, "0");
	$str_wantype = "PPPoE";
	$str_wanipaddr = query($rwan."/ppp6/local");
	$str_wanprefix = "64";
	$str_wangateway = query($rwan."/ppp6/peer");
	if(query($rwan."/ppp6/dns:1")!="")
	{
		$str_wanDNSserver = query($rwan."/ppp6/dns:1");
		if(query($rwan."/ppp6/dns:2")!="")	$str_wanDNSserver2 = query($rwan."/ppp6/dns:2");
		else	$str_wanDNSserver2 = query($rwan4."/inet/ipv6/dns:1");
	}
	else
	{
		$str_wanDNSserver = query($rwan4."/inet/ipv6/dns:1");
		$str_wanDNSserver2 = query($rwan4."/inet/ipv6/dns:2");
	}
}
else if ($is_ppp10 == 1 && $wancable_status==1)
{
	$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN1, "0");
	$rwanc = $rwan + "/child";
	$rwan4 = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN4, "0");
	$str_wantype = "PPPoE";
	$str_wanipaddr = query($rwanc."/ipaddr");
	$str_wanprefix = "64";
	$str_wangateway = query($rwanc."/ppp6/peer");
	if(query($wan1inetp."/ppp6/dns/count") > 0)
	{
		$str_wanDNSserver = query($rwan."/inet/ppp6/dns:1");
		if(query($rwan."/inet/ppp6/dns:2")!="")	$str_wanDNSserver2 = query($rwan."/inet/ppp6/dns:2");
		else	$str_wanDNSserver2 = query($rwan4."/inet/ipv6/dns:1");
	}
	else
	{
		$str_wanDNSserver = query($rwan4."/inet/ipv6/dns:1");
		$str_wanDNSserver2 = query($rwan4."/inet/ipv6/dns:2");
	}
}
else if($wancable_status==0)
{
	if(query($waninetp."/addrtype")=="ipv6") $str_wantype = query($waninetp."/ipv6/mode");
	else if($is_ppp6==1 || $is_ppp10==1)	$str_wantype = "PPPoE";

	$str_wanipaddr = "";
	$str_wanprefix = "";
	$str_wangateway = "";
	$str_wanDNSserver = "";
	$str_wanDNSserver2 = "";
}

	
if($is_ll!=1)
{
	if($str_wantype=="STATIC") $str_wantype = "Static";
	else if($str_wantype=="AUTO")
	{
		$rwan4 = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN4, "0");
		$rwanmode = query($rwan4."/inet/ipv6/mode");
		if($rwanmode=="STATEFUL") $str_wantype = "DHCPv6";
		else if($rwanmode=="STATELESS") $str_wantype = "SLAAC";
		else 							$str_wantype = "Autoconfiguration";
	}
}
/*  InitWAN()-->*/

/*<--InitLAN()  */
$lan = INF_getinfpath($LAN4);
$rlan = XNODE_getpathbytarget("/runtime", "inf", "uid", $LAN4, "0");
$dhcps6p = XNODE_getpathbytarget("/dhcps6", "entry", "uid", query($lan."/dhcps6"), "0");
$inetuid = query($lan."/inet");
$phyuid = query($lan."/phyinf");
$rlanphyp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyuid, "0");
if($DEBUG_HNAP == "y")
{
	if($lan=="")	TRACE_debug("$lan is empty.");
	if($rlan=="")	TRACE_debug("$rlan is empty.");
	if($inetuid=="")	TRACE_debug("$inetuid is empty.");
	if($phyuid=="")	TRACE_debug("$phyuid is empty.");
	if($rlanphyp=="")	TRACE_debug("$rlanphyp is empty.");
}

if($is_ll==1)
{
	$ll_lan_ll_address = query($rlanphyp."/ipv6/link/ipaddr");
	$ll_lan_ll_pl = "64";
}
else
{
	$lan_ll_address = query($rlanphyp."/ipv6/link/ipaddr");
	$lan_ll_pl = "64";
}

$rwan = XNODE_getpathbytarget("/runtime", "inf", "uid", $WAN4, "0");
$child = query($rwan."/child/uid");
$pdnetwork = query($rwan."/child/pdnetwork");
$pdprefix = query($rwan."/child/pdprefix");
$enpd="0";
if($child!="")	$enpd = "Enable";
else			$enpd = "Disable";

$b = $rlan."/inet/ipv6";
$lanip = query($b."/ipaddr");
$prefix = query($b."/prefix");
/*  InitLAN()-->*/

$wan4inetp = INET_getpathbyinf($WAN4);

if($str_wantype == "Link-Local")
{
	$ConnectionType = "IPv6_LinkLocalOnly";
	$LanLinkLocalAddress = $ll_lan_ll_address;
	$LanLinkLocalAddressPrefixLength = $ll_lan_ll_pl;
}
else if($str_wantype == "Static")
{
	$ConnectionType = "IPv6_Static";
	if($str_wanipaddr == INF_getcurripaddr($WAN3)) $UseLinkLocalAddress = "Enable";
	else $UseLinkLocalAddress = "Disable";
	$Address = $str_wanipaddr;
	$SubnetPrefixLength = $str_wanprefix;
	$DefaultGateway = $str_wangateway;
}
else if($str_wantype == "6TO4")
{
	$ConnectionType = "IPv6_6To4";
	$6To4Address = $str_wanipaddr;
	$6To4Relay = $str_wangateway;
}
else if($str_wantype == "6RD")
{
	$ConnectionType = "IPv6_6RD";
	$6Rd_Configuration = $str_6rd_Conf;
	$6Rd_IPv4Address = INF_getcurripaddr($WAN1);
	$6Rd_IPv4MaskLength = $str_6rd_IPv4MaskLength;
	$6Rd_IPv6Prefix = $str_6rd_IPv6Prefix;
	$6Rd_IPv6PrefixLength = $str_6rd_IPv6PrefixLength;
	$6Rd_BorderRelayIPv4Address = $str_wangateway;
}
else if($str_wantype == "6IN4")
{
	$ConnectionType = "IPv6_IPv6InIPv4Tunnel";	
	$6In4LocalIPv4Address = INF_getcurripaddr($WAN1);
	$6In4LocalIPv6Address = $str_wanipaddr;
	$6In4RemoteIPv4Address = $str_6in4_RemoteIPv4;
	$6In4RemoteIPv6Address = $str_wangateway;
}
else if($str_wantype=="AUTO" || $str_wantype=="DHCPv6" || $str_wantype=="SLAAC" || $str_wantype=="Autoconfiguration")
{	
	if(query(INF_getinfpath($WAN5)."/active")=="1")
	{
		$ConnectionType = "IPv6_AutoDetection";
		$ConnectionType2 = "IPv6_AutoConfiguration";
	}
	else $ConnectionType = "IPv6_AutoConfiguration";
	$Address = $str_wanipaddr;
	$SubnetPrefixLength = $str_wanprefix;
	$DefaultGateway = $str_wangateway;
}
else if($str_wantype == "PPPoE")
{
	if(query(INF_getinfpath($WAN5)."/active")=="1")
	{
		$ConnectionType = "IPv6_AutoDetection";
		$ConnectionType2 = "IPv6_DynamicPPPoE";		
	}	
	else if(query($wan1inetp."/addrtype")=="ppp10")
	{
		if(query($wan1inetp."/ppp6/static")=="1") $ConnectionType = "IPv6_StaticPPPoE";
		else $ConnectionType = "IPv6_DynamicPPPoE";
		$PppoeNewSession = "SharedWithIPv4";
		$PppoeUsername = query($wan1inetp."/ppp6/username");
		$PppoePassword = query($wan1inetp."/ppp6/password");
		$PppoeReconnectMode = query($wan1inetp."/ppp6/dialup/mode");
		$PppoeMaxIdleTime = query($wan1inetp."/ppp6/dialup/idletimeout");		
		$PppoeMTU = query($wan1inetp."/ppp6/mtu");
		$PppoeServiceName = query($wan1inetp."/ppp6/pppoe/servicename");
	}	
	else
	{
		if(query($wan3inetp."/ppp6/static")=="1") $ConnectionType = "IPv6_StaticPPPoE";
		else $ConnectionType = "IPv6_DynamicPPPoE";
		$PppoeNewSession = "NewSession";
		$PppoeUsername = query($wan3inetp."/ppp6/username");
		$PppoePassword = query($wan3inetp."/ppp6/password");
		$PppoeReconnectMode = query($wan3inetp."/ppp6/dialup/mode");
		$PppoeMaxIdleTime = query($wan3inetp."/ppp6/dialup/idletimeout");		
		$PppoeMTU = query($wan3inetp."/ppp6/mtu");
		$PppoeServiceName = query($wan3inetp."/ppp6/pppoe/servicename");		
	}
	$Address = $str_wanipaddr;
	$SubnetPrefixLength = $str_wanprefix;
	$DefaultGateway = $str_wangateway;
}

if($ConnectionType != "IPv6_LinkLocalOnly")
{
	// IPV6 DNS SETTINGS
	if($ConnectionType!="IPv6_Static" && $ConnectionType!="IPv6_6To4" && $ConnectionType!="IPv6_6RD")
	{	
		if(query($wan4inetp."/ipv6/dns/count")!="0") $ObtainDNS = "Manual";
		else $ObtainDNS = "Automatic";
	}	
 	$PrimaryDNS = $str_wanDNSserver;
	$SecondaryDNS = $str_wanDNSserver2;
	
	// LAN IPV6 ADDRESS SETTINGS
	if($ConnectionType!="IPv6_Static" && $ConnectionType!="IPv6_6To4" && $ConnectionType!="IPv6_6RD")
	{	
		$DhcpPd = $enpd;
	}
	$LanAddress = $lanip;
	$LanAddressPrefixLength = $prefix;
	$LanLinkLocalAddress = $lan_ll_address;
	$LanLinkLocalAddressPrefixLength = $lan_ll_pl;
	
	// ADDRESS AUTOCONFIGURATION SETTINGS	
	if(query(INF_getinfpath($LAN4)."/dhcps6") != "")
	{
		$LanIPv6AddressAutoAssignment = "Enable";
		if(query($$dhcps6p."/pd/enable")=="1") $LanAutomaticDhcpPd = "Enable";
		else $LanAutomaticDhcpPd = "Disable";
	}
	else	
	{
		$LanIPv6AddressAutoAssignment = "Disable";
		$LanAutomaticDhcpPd = "Disable";
	}
	
	if(query($dhcps6p."/mode")=="STATELESS" && query("/device/rdnss")=="1")
	{
		$LanAutoConfigurationType = "SLAAC_RDNSS";
		if(query(INET_getpathbyinf($LAN4)."/ipv6/routerlft") == "") $LanRouterAdvertisementLifeTime = "";
		else $LanRouterAdvertisementLifeTime = query(INET_getpathbyinf($LAN4)."/ipv6/routerlft")/60;
	}
	else if(query($dhcps6p."/mode")=="STATELESS" && query("/device/rdnss")=="0")
	{
		$LanAutoConfigurationType = "SLAAC_StatelessDhcp";
		if(query(INET_getpathbyinf($LAN4)."/ipv6/routerlft") == "") $LanRouterAdvertisementLifeTime = "";
		else $LanRouterAdvertisementLifeTime = query(INET_getpathbyinf($LAN4)."/ipv6/routerlft")/60;
	}	
	else if(query($dhcps6p."/mode")=="STATEFUL")
	{
		$LanAutoConfigurationType = "Stateful";
		$LanIPv6AddressRangeStart = LanIPv6AddressRange(query($rlan."/dhcps6/network"), query($dhcps6p."/start"), query($dhcps6p."/count"), "start");
		$LanIPv6AddressRangeEnd = LanIPv6AddressRange(query($rlan."/dhcps6/network"), query($dhcps6p."/start"), query($dhcps6p."/count"), "end");
		if(query(INET_getpathbyinf($LAN4)."/ipv6/preferlft") == "") $LanDhcpLifeTime = "";
		else $LanDhcpLifeTime = query(INET_getpathbyinf($LAN4)."/ipv6/preferlft")/60;
	}	
}	

if($DEBUG_HNAP == "y")
{
	TRACE_info("$str_wantype = ".$str_wantype);
	TRACE_info("$str_wanipaddr = ".$str_wanipaddr);
	TRACE_info("$str_wanprefix = ".$str_wanprefix);
	TRACE_info("$str_wangateway = ".$str_wangateway);
	TRACE_info("$str_wanDNSserver = ".$str_wanDNSserver);
	TRACE_info("$str_wanDNSserver2 = ".$str_wanDNSserver2);
	TRACE_info("$lanip = ".$lanip);
	TRACE_info("$prefix(lanip) = ".$prefix);
	TRACE_info("$LanAutoConfigurationType = ".$LanAutoConfigurationType);
	TRACE_info("$LanRouterAdvertisementLifeTime = ".$LanRouterAdvertisementLifeTime);
	TRACE_info("$LanIPv6AddressRangeStart = ".$LanIPv6AddressRangeStart);
	TRACE_info("$LanIPv6AddressRangeEnd = ".$LanIPv6AddressRangeEnd);
	TRACE_info("$LanDhcpLifeTime = ".$LanDhcpLifeTime);
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
	<soap:Body>
	<GetIPv6SettingsResponse xmlns="http://purenetworks.com/HNAP1/">
		<GetIPv6SettingsResult>OK</GetIPv6SettingsResult>
		<IPv6_ConnectionType><?=$ConnectionType?></IPv6_ConnectionType>
<?
/*<--IPV6 SETTINGS  */
if($ConnectionType=="IPv6_AutoDetection")
{
	echo "		<IPv6_ConnectionType2>".$ConnectionType2."</IPv6_ConnectionType2>\n";	
}	
else if($ConnectionType=="IPv6_DynamicPPPoE" || $ConnectionType=="IPv6_StaticPPPoE")
{
	echo "		<IPv6_PppoeNewSession>".$PppoeNewSession."</IPv6_PppoeNewSession>\n";
	echo "		<IPv6_PppoeUsername>".$PppoeUsername."</IPv6_PppoeUsername>\n";
	echo "		<IPv6_PppoePassword>".$PppoePassword."</IPv6_PppoePassword>\n";	
	echo "		<IPv6_PppoeReconnectMode>".$PppoeReconnectMode."</IPv6_PppoeReconnectMode>\n";
	echo "		<IPv6_PppoeMaxIdleTime>".$PppoeMaxIdleTime."</IPv6_PppoeMaxIdleTime>\n";
	echo "		<IPv6_PppoeMTU>".$PppoeMTU."</IPv6_PppoeMTU>\n";
	echo "		<IPv6_PppoeServiceName>".$PppoeServiceName."</IPv6_PppoeServiceName>\n";
}
else if($ConnectionType=="IPv6_Static")
{
	echo "		<IPv6_UseLinkLocalAddress>".$UseLinkLocalAddress."</IPv6_UseLinkLocalAddress>\n";	
}
else if($ConnectionType=="IPv6_IPv6InIPv4Tunnel")
{
	echo "		<IPv6_6In4LocalIPv4Address>".$6In4LocalIPv4Address."</IPv6_6In4LocalIPv4Address>\n";
	echo "		<IPv6_6In4LocalIPv6Address>".$6In4LocalIPv6Address."</IPv6_6In4LocalIPv6Address>\n";
	echo "		<IPv6_6In4RemoteIPv4Address>".$6In4RemoteIPv4Address."</IPv6_6In4RemoteIPv4Address>\n";
	echo "		<IPv6_6In4RemoteIPv6Address>".$6In4RemoteIPv6Address."</IPv6_6In4RemoteIPv6Address>\n";	
}	
else if($ConnectionType=="IPv6_6To4")
{
	echo "		<IPv6_6To4Address>".$6To4Address."</IPv6_6To4Address>\n";
	echo "		<IPv6_6To4Relay>".$6To4Relay."</IPv6_6To4Relay>\n";	
}
else if($ConnectionType=="IPv6_6RD")
{
	echo "		<IPv6_6Rd_Configuration>".$6Rd_Configuration."</IPv6_6Rd_Configuration>\n";
	echo "		<IPv6_6Rd_IPv4Address>".$6Rd_IPv4Address."</IPv6_6Rd_IPv4Address>\n";
	echo "		<IPv6_6Rd_IPv4MaskLength>".$6Rd_IPv4MaskLength."</IPv6_6Rd_IPv4MaskLength>\n";	
	echo "		<IPv6_6Rd_IPv6Prefix>".$6Rd_IPv6Prefix."</IPv6_6Rd_IPv6Prefix>\n";
	echo "		<IPv6_6Rd_IPv6PrefixLength>".$6Rd_IPv6PrefixLength."</IPv6_6Rd_IPv6PrefixLength>\n";
	echo "		<IPv6_6Rd_BorderRelayIPv4Address>".$6Rd_BorderRelayIPv4Address."</IPv6_6Rd_BorderRelayIPv4Address>\n";	
}
/*  IPV6 SETTINGS-->*/
/*<--WAN IPV6 ADDRESS SETTINGS  */
if($ConnectionType=="IPv6_AutoDetection" || $ConnectionType=="IPv6_AutoConfiguration" || $ConnectionType=="IPv6_DynamicPPPoE" || $ConnectionType=="IPv6_StaticPPPoE"
	|| $ConnectionType=="IPv6_Static")
{
	echo "		<IPv6_Address>".$Address."</IPv6_Address>\n";
	echo "		<IPv6_SubnetPrefixLength>".$SubnetPrefixLength."</IPv6_SubnetPrefixLength>\n";
	echo "		<IPv6_DefaultGateway>".$DefaultGateway."</IPv6_DefaultGateway>\n";					
}
/*  WAN IPV6 ADDRESS SETTINGS-->*/
/*<--IPV6 DNS SETTINGS  */
if($ConnectionType=="IPv6_AutoDetection" || $ConnectionType=="IPv6_AutoConfiguration" || $ConnectionType=="IPv6_DynamicPPPoE" 
	|| $ConnectionType=="IPv6_StaticPPPoE" || $ConnectionType=="IPv6_Static" || $ConnectionType=="IPv6_IPv6InIPv4Tunnel" 
	|| $ConnectionType=="IPv6_6To4" || $ConnectionType=="IPv6_6RD")
{
	if($ConnectionType!="IPv6_Static" && $ConnectionType!="IPv6_6To4" && $ConnectionType!="IPv6_6RD")
	{	
		echo "		<IPv6_ObtainDNS>".$ObtainDNS."</IPv6_ObtainDNS>\n";
	}
	echo "		<IPv6_PrimaryDNS>".$PrimaryDNS."</IPv6_PrimaryDNS>\n";
	echo "		<IPv6_SecondaryDNS>".$SecondaryDNS."</IPv6_SecondaryDNS>\n";
}
/*  IPV6 DNS SETTINGS-->*/
/*<--LAN IPV6 ADDRESS SETTINGS  */
if($ConnectionType=="IPv6_AutoDetection" || $ConnectionType=="IPv6_AutoConfiguration" || $ConnectionType=="IPv6_DynamicPPPoE" 
	|| $ConnectionType=="IPv6_StaticPPPoE" || $ConnectionType=="IPv6_Static" || $ConnectionType=="IPv6_IPv6InIPv4Tunnel" 
	|| $ConnectionType=="IPv6_6To4" || $ConnectionType=="IPv6_6RD")
{
	if($ConnectionType!="IPv6_Static" && $ConnectionType!="IPv6_6To4" && $ConnectionType!="IPv6_6RD")
	{	
		echo "		<IPv6_DhcpPd>".$DhcpPd."</IPv6_DhcpPd>\n";
	}
	echo "		<IPv6_LanAddress>".$LanAddress."</IPv6_LanAddress>\n";		
	echo "		<IPv6_LanAddressPrefixLength>".$LanAddressPrefixLength."</IPv6_LanAddressPrefixLength>\n";	
}
/*  LAN IPV6 ADDRESS SETTINGS-->*/
?>		<IPv6_LanLinkLocalAddress><?=$LanLinkLocalAddress?></IPv6_LanLinkLocalAddress>
			<IPv6_LanLinkLocalAddressPrefixLength><?=$LanLinkLocalAddressPrefixLength?></IPv6_LanLinkLocalAddressPrefixLength>
<?
/*<--ADDRESS AUTOCONFIGURATION SETTINGS   */
if($ConnectionType!="IPv6_LinkLocalOnly")
{
	echo "		<IPv6_LanIPv6AddressAutoAssignment>".$LanIPv6AddressAutoAssignment."</IPv6_LanIPv6AddressAutoAssignment>\n";
	if($DhcpPd=="Enable")
	{	
		echo "		<IPv6_LanAutomaticDhcpPd>".$LanAutomaticDhcpPd."</IPv6_LanAutomaticDhcpPd>\n";
	}
	
	if($LanAutoConfigurationType=="SLAAC_RDNSS")
	{
		echo "		<IPv6_LanAutoConfigurationType>SLAAC_RDNSS</IPv6_LanAutoConfigurationType>\n";
		echo "		<IPv6_LanRouterAdvertisementLifeTime>".$LanRouterAdvertisementLifeTime."</IPv6_LanRouterAdvertisementLifeTime>\n";	
	}
	else if($LanAutoConfigurationType=="SLAAC_StatelessDhcp")
	{
		echo "		<IPv6_LanAutoConfigurationType>SLAAC_StatelessDhcp</IPv6_LanAutoConfigurationType>\n";
		echo "		<IPv6_LanRouterAdvertisementLifeTime>".$LanRouterAdvertisementLifeTime."</IPv6_LanRouterAdvertisementLifeTime>\n"; 	
	}
	else if($LanAutoConfigurationType=="Stateful")
	{
		echo "		<IPv6_LanAutoConfigurationType>Stateful</IPv6_LanAutoConfigurationType>\n";
		echo "		<IPv6_LanIPv6AddressRangeStart>".$LanIPv6AddressRangeStart."</IPv6_LanIPv6AddressRangeStart>\n";
		echo "		<IPv6_LanIPv6AddressRangeEnd>".$LanIPv6AddressRangeEnd."</IPv6_LanIPv6AddressRangeEnd>\n";
		echo "		<IPv6_LanDhcpLifeTime>".$LanDhcpLifeTime."</IPv6_LanDhcpLifeTime>\n";	
	}
}
/*  ADDRESS AUTOCONFIGURATION SETTINGS-->*/
?>	</GetIPv6SettingsResponse>
		</soap:Body>
</soap:Envelope>
