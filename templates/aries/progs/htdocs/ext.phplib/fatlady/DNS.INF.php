<?
/* fatlady is used to validate the configuration for the specific service.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdics/phplib/xnode.php";
include "/htdocs/phplib/inet.php";

function result($res, $node, $msg)
{
	$_GLOBALS["FATLADY_result"]	= $res;
	$_GLOBALS["FATLADY_node"]	= $node;
	$_GLOBALS["FATLADY_message"]= $msg;
	return $res;
}

function check_filter($path)
{
	$cnt = query($path."/count");
	$max = query($path."/max");
	$num = query($path."/entry#");
	$seq = query($path."/seqno");
	if ($seq=="") $seq=1;
	if ($cnt>$max) $cnt=$max;
	/* delete the exceeded FILTER entries */
	while ($num>$cnt) {del($path."/entry:".$num); $num--;}

	foreach ($path."/entry")
	{
		/* Verify UID */
		$uid = query("uid");
		if ($uid=="")
		{
			$uid = "FILTER-".$seq;
			$seq++;
			set("uid", $uid);
			set($path."/seqno", $seq);
		}
		if ($$uid==1) return result("FAILED", $path."/entry:".$InDeX."/uid", "Duplicated UID - ".$uid);
		$$uid=1;

		/* Verify setting */
		if (query("enable")!="1") set("enable", "0");
		$domain = query("string");
		if ($domain=="") return result("FAILED", $path."/entry:".$InDeX."/string",
									i18n("Please input the domain name."));

		/* Verify domain name */
		if (charcodeat($domain, 0)=='.') $test = "www".$domain;
		else $test = $domain;
		if (isdomain($test)!=1) return result("FAILED", $path."/entry:".$InDeX."/string",
									i18n("Invalid domain name."));

		/* Verify duplicate domain name */
		$i = 1;
		while ($i<$InDeX)
		{
			if (query($path."/entry:".$i."/string")==$domain)
				return result("FAILED", $path."/entry:".$InDeX."/string", i18n("Duplicated domain name"));
			$i++;
		}
	}
	return "OK";
}

function check_dns_server($base)
{
	anchor($base);
	$max = query("max");
	$cnt = query("count");
	$num = query("entry#");
	$seq = query("seqno");
	if ($seq=="") $seq=1;
	if ($cnt>$max) $cnt=$max;
	/* delete the exceeded SVR entries */
	while ($num>$cnt) {del("entry:".$num); $num--;}

	/* walk through all SVR-# */
	foreach ("entry")
	{
		/* The current path */
		$e = $base."/entry:".$InDeX;
		/* Check the UID */
		$uid = query("uid");
		TRACE_debug("DNS.SERVER[".$InDeX."]: ".$uid);
		/* fill in the uid for new dns server.*/
		if ($uid=="")
		{
			$uid = "SVR-".$seq;
			$seq++;
			set("uid", $uid);
		}
		if ($$uid==1) return result("FAILED", $e."/uid", "Duplicated UID - ".$uid);
		$$uid=1;

		/* Check type */
		$type = query("type");
		TRACE_debug("DNS.INF.SVR: type=".$type);
		if ($type=="INF")
		{
			$inf = query("inf");
			TRACE_debug("DNS.INF.SVR: inf=".$inf);
			$infp = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
			if ($infp=="")
				return result("FAILED", $e."/inf", i18n("Invalid interface name")." - ".$inf);
		}
		else if ($type=="IPV4")
		{
			$ipaddr = query("ipv4");
			TRACE_debug("DNS.INF.SVR: ipv4=".$ipaddr);
			if (INET_validv4addr($ipaddr)==0)
				return result("FAILED", $e."/ipv4", i18n("Invalid IPv4 address")." - ".$ipaddr);
		}
		else if ($type=="IPV6")
		{
			$ipaddr = query("ipv6");
			TRACE_debug("DNS.INF.SVR: ipv6=".$ipaddr);
			if (ipv6checkip($ipaddr)!="1")
				return result("FAILED", $e."/ipv6", i18n("Invalid IPv6 address")." - ".$ipaddr);
		}
		else return result("FAILED", $e."/type", "Unsupported type - ".$type);

		/* Check filter */
		if (check_filter($base."/entry:".$InDeX."/filter")!="OK") return "FAILED";
	}
	set("seqno", $seq);
	return "OK";
}

function fatlady_gothrough_all_dns($prefix)
{
	anchor($prefix);
	$max = query("/dns/max");
	$cnt = query("dns/count");
	$seq = query("dns/seqno");
	$num = query("dns/entry#");

	TRACE_debug("DNS.INF: max=".$max.", count=".$cnt.", seqno=".$seq.", number=".$num);

	/* delete the exceeded DNS entries */
	if ($cnt>$max) $cnt=$max;
	while ($num>$cnt) {del("dns/entry:".$num); $num--;}

	TRACE_debug("DEBUG !!!\n".dump("",$prefix));

	foreach ("dns/entry")
	{
		if ($InDeX > $cnt) break;
		TRACE_debug("DNS.INF: uid=".query("uid"));
		$ret = check_dns_server($prefix."/dns/entry:".$InDeX."/server");
		if ($ret!="OK") break;
		if (query("uid")=="")
		{
			set("uid", "DNS-".$seq);
			$seq++;
		}
	}
	if ($ret=="OK") set("dns/seqno", $seq);
	return $ret;
}

/***********************************************************/
result("FAILED", "", "");
if (fatlady_gothrough_all_dns($FATLADY_prefix)=="OK")
{
	set($FATLADY_prefix."/valid", 1);
	result("OK","","");
}
?>
