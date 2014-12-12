<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function get_filter($path)
{
	$cnt = query($path."/count");
	foreach ($path."/entry")
	{
		if ($InDeX > $cnt) break;
		$enable = query("enable");
		$string = query("string");
		if ($enable==1 && $string!="") $filter = $filter.$string."/";
	}
	/* Add a leading slash if we do have filter. */
	if ($filter!="") $filter = "/".$filter;
	return $filter;
}

function genconf($dnsx, $uid, $conf, $opendns_type)
{
	/* Get the DNS profile */
	$dnsp = XNODE_getpathbytarget("/".$dnsx, "entry", "uid", $uid, 0);
	$scnt = query($dnsp."/count");
	$needhelper = 0;
	foreach ($dnsp."/entry")
	{
		if ($InDeX > $scnt) break;
		$filter = get_filter($dnsp."/entry:".$InDeX."/filter");
		if ($filter != "") $needhelper++;
		$type = query("type");
		if ($type == "local")
		{
			if($opendns_type == "") fwrite(a,$conf, "server=".$filter."local\n");
		}
		else if ($type == "static")
		{
			if($opendns_type == "") foreach ("ipaddr") fwrite(a,$conf, "server=".$filter.$VaLuE."\n");
		}
		else if ($type == "inf")
		{
			/* Check if this inf is default route ? */
			$inf = query("inf");
			$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);

			if ($stsp=="" || $inf=="WAN-2") continue;/*Don't use the physic WAN DNS server in PPTP or L2TP WAN settings.*/
			$default = query($stsp."/defaultroute");
			$addrtype = query($stsp."/inet/addrtype");
			if ($addrtype=="ipv4" || $addrtype=="ppp4")
			{
				foreach ($stsp."/inet/".$addrtype."/dns")
				{
					if($opendns_type == "") fwrite(a,$conf, "server=".$filter.$VaLuE."\n");
					/* Record the server so we can do the dirty check in the furture. */
					$svrp = XNODE_getpathbytarget("/runtime/services/dnsmasq/svrlist", "server", "ipaddr", $VaLuE, 1);
					set($svrp."/defaultroute", $default);
					set($svrp."/inf", $inf);
				}
			}
			else if ($addrtype=="ipv6" || $addrtype=="ppp6")
			{
				foreach ($stsp."/inet/".$addrtype."/dns")
				{
					fwrite(a,$conf, "server=".$filter.$VaLuE."\n");
					/* Record the server so we can do the dirty check in the furture. */
					$svrp = XNODE_getpathbytarget("/runtime/services/dnsmasq/svrlist", "server", "ipaddr", $VaLuE, 1);
					set($svrp."/defaultroute", $default);
					set($svrp."/inf", $inf);
				}
			}
		}
	}
	if ($needhelper > 0) fwrite("a", $conf, "dns-helper=/etc/scripts/dns-helper.sh\n");
}

/* set cache size */
$csize = query("cachesize"); if ($csize=="") $csize=0;

fwrite("w", $CONF, "no-resolv\n");
fwrite("a", $CONF, "cache-size=".$csize."\n");

$wan1_infp = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$opendns_type = query($wan1_infp."/open_dns/type");
$phy_wan1 = query($wan1_infp."/phyinf");
$run_phy_wan1 = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phy_wan1, 0);
$mac_wan1=query($run_phy_wan1."/macaddr");
$DNSPROFILES = "/runtime/services/dnsprofiles/";

$dnsquery_enable = query("/device/log/mydlink/dnsquery");
$mydlinkregist = query("/mydlink/register_st");
if($mydlinkregist != "1")
{
	$dnsquery_enable = 0;	
}

foreach ("/inf")
{
	/*
	$dns4 = query("dns4");
	if ($dns4=="") continue;
	*/

	$dns4 = query("dns4");
	$dns6 = query("dns6");
	
	if($dnsquery_enable != 1)  //jef add for mydlink
	{
		if ($dns4=="" && $dns6=="") continue;
	}
	
	if( $dns4 != "") $dns = $dns4;
	else $dns = $dns6;

	$dnsname = cut($dns,0,'-');
	$dnsnamel = tolower($dnsname);

	/* The same profile may be used by different interface,
	 * so skip this profile if it has been processed. */
	if (query($DNSPROFILES."/".$dns)!=1)
	{
		/* This profile is not setup yet. */
		genconf($dnsnamel, $dns, $CONF, $opendns_type);
		set($DNSPROFILES."/".$dns, 1);
	}

	$uid = query("uid");
	$phyinf = PHYINF_getruntimeifname($uid);
	if ($phyinf!="" && $$phyinf != 1)
	{
		$$phyinf = 1;
		fwrite("a", $CONF, "interface="         .$phyinf."\n");
		fwrite("a", $CONF, "no-dhcp-interface=" .$phyinf."\n");
	}
}

if($opendns_type == "advance")
{
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/adv_dns_srv/dns1")."\n");
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/adv_dns_srv/dns2")."\n");
	fwrite("a", $CONF, "adv-dns-enable=1\n");
	fwrite("a", $CONF, "adv-dns-id=".$mac_wan1."\n");
}
else if($opendns_type == "family")
{
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/family_dns_srv/dns1")."\n");
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/family_dns_srv/dns2")."\n");
	fwrite("a", $CONF, "adv-dns-enable=1\n");
	fwrite("a", $CONF, "adv-dns-id=".$mac_wan1."\n");
}
else if($opendns_type == "parent")
{
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/parent_dns_srv/dns1")."\n");
	fwrite("a", $CONF, "server=".query($wan1_infp."/open_dns/parent_dns_srv/dns2")."\n");
	fwrite("a", $CONF, "adv-dns-enable=1\n");
	fwrite("a", $CONF, "adv-dns-id=".$mac_wan1."\n");
	fwrite("a", $CONF, "open-dns-parent-enable=1\n");
	fwrite("a", $CONF, "open-dns-deviceid=".query($wan1_infp."/open_dns/deviceid")."\n");
}

$dnsquery_enable = query("/device/log/mydlink/dnsquery");
$mydlinkregist = query("/mydlink/register_st");
if($mydlinkregist != "1")
{
	$dnsquery_enable = 0;	
}
if($dnsquery_enable == 1)
{
	fwrite("a", $CONF, "log-queries\n");
}
	
del($DNSPROFILES);
?>
