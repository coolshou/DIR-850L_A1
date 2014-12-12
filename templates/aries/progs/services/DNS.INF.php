<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

fwrite(w, $START, "#!/bin/sh\n");
fwrite(w, $STOP,  "#!/bin/sh\n");

function get_filter($path)
{
	//TRACE_debug("[DNS.INF/filter]path=".$path);
	$cnt = query($path."/count");
	foreach ($path."/entry")
	{
		if ($InDeX > $cnt) break;
		$enable = query("enable");
		$string = query("string");
		//TRACE_debug("[DNS.INF/filter]string=".$string);
		if ($enable==1 && $string!="") $filter = $filter.$string."/";
	}
	/* Add a leading slash if we do have filter. */
	if ($filter!="") $filter = "/".$filter;
	return $filter;
}

$infdncmd = "";
/* Get host domain name */
$enhdn = query("/device/hostdomainname/enable");
$hdn = query("/device/hostdomainname/name");

if ($enhdn != "" && $enhdn != "0" && $hdn != "")
{
	/* There is an issue about this.
	   If we name the same name on several interfaces,
	   the dnsmasq will return the first match interface but not the specific interface (input interface).
	   For this, we should seperate  different interfaces to use individual dnsmasq daemon.
	   By Enos. 2010/07/19  */
	$i = 1;
	while ($i > 0)
	{
		/* get LAN path */
		$lan = "LAN-".$i;
		$linfp = XNODE_getpathbytarget("", "inf", "uid", $lan, 0);
		$lstsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $lan, 0);
		if ($lstsp=="" || $linfp=="") { $i=0; break; }
		/* Get phyinf */
		$laninf = PHYINF_getruntimeifname($lan);
		$infdncmd = $infdncmd." --interface-name=".$hdn.",".$laninf;
		$i++;
	}
}

$dnscfg = "/var/run/dnsmasq.conf";
$dnspid = "/var/run/dnsmasq.pid";
$DNSPROFILES = "/runtime/services/dnsprofiles";
foreach ("/runtime/inf")
{
	$uid = query("uid");
	//TRACE_debug("[DNS.INF]uid=".$uid);
	$infp = XNODE_getpathbytarget("", "inf", "uid", $uid, 0);
	$dns = query($infp."/dns");
	//TRACE_debug("[DNS.INF]dns=".$dns);
	if ($dns == "") continue;

	if (query($DNSPROFILES) == $dns) continue;

	$dnsp = XNODE_getpathbytarget("/dns", "entry", "uid", $dns, 0);

	/* set cache size */
	$csize = query($dnsp."/cachesize"); if ($csize=="") $csize=0;

	$dnsname = cut($dns,0,'-');
	$dnsnamel = tolower($dnsname);

	fwrite("w", $dnscfg, "no-resolv\n");
	fwrite("a", $dnscfg, "cache-size=".$csize."\n");
	
	$scnt = query($dnsp."/server/count");
	$needhelper = 0;
	foreach ($dnsp."/server")
	{
		if ($InDeX > $scnt) break;
		//TRACE_debug("[DNS.INF]server:".$InDeX);
		foreach ("entry")
		{
			$type = query("type");
			$filter = get_filter($dnsp."/server/entry:".$InDeX."/filter");
			if ($filter != "") $needhelper++;
			if ($type == "IPV4" || $type == "IPV6")
			{
				fwrite(a,$dnscfg, "server=".$filter.query($dnsp."/server/entry:".$InDeX."/".tolower($type))."\n");
			}
			else if ($type == "INF")
			{
				/* Check if this inf is default route ? */
				$inf = query("inf");
				$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);
				//TRACE_debug("[DNS.INF/server]inf=".$inf);
				if ($stsp=="") continue;
				$default = query($stsp."/defaultroute");
				$addrtype = query($stsp."/inet/addrtype");
				//TRACE_debug("[DNS.INF/server]addrtype=".$addrtype);
				foreach ($stsp."/inet/".$addrtype."/dns")
				{
					//TRACE_debug("[DNS.INF/server]VaLuE=".$VaLuE);
					fwrite(a,$dnscfg, "server=".$filter.$VaLuE."\n");
					/* Record the server so we can do the dirty check in the furture. */
					$svrp = XNODE_getpathbytarget("/runtime/services/dnsmasq/svrlist", "server", "ipaddr", $VaLuE, 1);
					set($svrp."/defaultroute", $default);
					set($svrp."/inf", $inf);
				}
			}
		}
	}
	/*
	foreach ("/runtime/inf")
	{
		$inet = query("inet/uid");
		$inetp = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
		$addrtype = query($inetp."/addrtype");
		//TRACE_debug("[DNS.INF]inet=".$inet);
		//TRACE_debug("[DNS.INF]inetp=".$inetp);
		//TRACE_debug("[DNS.INF]addrtype=".$addrtype);
		foreach ($inetp."/".$addrtype."/dns")
		{
			$s = query("entry:".$InDeX);
			if ($s != "")
			{
				fwrite(a, $dnscfg, "server=".$s."\n");
			}
		}
	}
	*/
	if ($needhelper > 0) fwrite("a", $dnscfg, "dns-helper=/etc/scripts/dns-helper.sh\n");

	foreach ("/runtime/inf")
	{
		$uid = query("uid");
		$tmp = cut($uid, 0, '-');
		if ($tmp == "LAN")
		{
			$phyinf = PHYINF_getruntimeifname($uid);
			if ($phyinf!="" && $$phyinf != 1)
			{
				$$phyinf = 1;
				fwrite("a", $dnscfg, "interface="         .$phyinf."\n");
				fwrite("a", $dnscfg, "no-dhcp-interface=" .$phyinf."\n");
			}
		}
	}
	set($DNSPROFILES, $dns);
}
fwrite("a", $START,
	'if [ -f '.$dnscfg.' ]; then\n'.
	'   dnsmasq -C '.$dnscfg.' -x '.$dnspid.' '.$infdncmd.' &\n'.
	'fi\n'
	);

/* stop the dnsmaq */
fwrite("w", $STOP,
		'/etc/scripts/killpid.sh '.$dnspid.'\n'.
		'/etc/scripts/dns-helper.sh flush\n'
		);
fwrite("a", $STOP,
		'if [ -f '.$dnscfg.' ]; then\n'.
		'	rm -f '.$dnscfg.'\n'.
		'fi\n'
	  );
fwrite("a", $START, 'exit 0\n');
fwrite("a", $STOP, 'exit 0\n');

del($DNSPROFILES);
?>
