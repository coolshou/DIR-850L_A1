<?
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

function result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;
	return $result;
}

function valid_mac($mac)
{
	if ($mac=="") return 0;

	/* Use ':' as delimiter. */
	$delimiter = ":";
	$num = cut_count($mac, $delimiter);
	if ($num != 6)
	{
		/* Use '-' as delimiter and try again. */
		$delimiter = "-";
		$num = cut_count($mac, $delimiter);
		if ($num != 6) return 0;
	}
	/* Found the MAC string. */
	$num--;
	while ($num >= 0)
	{
		$tmp = cut($mac, $num, $delimiter);
		if (isxdigit($tmp)==0) return 0;
		$num--;
	}
	return 1;
}

function verify_bridgeports($path, $max)
{
	$count = query($path."/count"); if ($count=="") $count=0;
	$seqno = query($path."/seqno"); if ($seqno=="") $seqno=1;
	if ($count > $max) $count = $max;
	$count+=0; $seqno+=0;

	TRACE_debug("FATLADY: BridgePorts: max=".$max.", count=".$count.", seqno=".$seqno.", path=".$path);

	/* Delete the extra entries. */
	$num = query($path."/entry#");
	while ($num>$count) { del($path."/entry:".$num); $num--; }

	/* verify each entry */
	set($path."/count", $count);
	set($path."/seqno", $seqno);
	foreach ($path."/entry")
	{
		if ($InDeX>$count) break;

		/* The current entry path. */
		$entry = $path."/entry:".$InDeX;

		/* Check empty UID */
		$uid = query("uid");
		if ($uid == "")
		{
			$uid = "MBR-".$seqno;
			set("uid", $uid);
			$seqno++;
			set($path."/seqno", $seqno);
		}
		/* Check duplicated UID */
		if ($$uid == "1") return result("FAILED", $entry."/uid", "Duplicated UID - ".$uid);
		$$uid = 1;

		/* Check member port */
		$val = query("phyinf");
		$val = XNODE_getpathbytarget("", "phyinf", "uid", $val, 0);
		if ($val=="") return result("FAILED", $entry."/phyinf", "Invalid member port..");
	}
	return "OK";
}

function verify_PHYINF($path)
{
	foreach ($path."/phyinf")
	{
		$entry = $path."/phyinf:".$InDeX;
		$uid = query("uid");
		$ret = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
		TRACE_debug("verify_PHYINF: entry=".$entry.", uid=".$uid.", ret=".$ret);
		if ($ret=="")
			return result("FAILED", $entry."/uid", "Invalid PHYINF - ".$uid);

		if (query("active")!="1") set("active","0");

		/* Check MAC address */
		$val = query("macaddr");
		if ($val!="" && valid_mac($val)==0)
			return result("FAILED", $entry."/macaddr", i18n("Invalid MAC address value."));

		/* Check MTU */
		$val = query("mtu");
		if ($val != "")
		{
			if (isdigit($val)==0 || $val>1500 || $val<0)
				return result("FAILED", $entry."/mtu", i18n("Invalid MTU value."));
		}

		/* Check bridge settings */
		if (query("bridge/enable")!="1") set("bridge/enable", "0");

		/* Check bridge ports */
		$max = query("/phyinf#");
		$ret = verify_bridgeports($entry."/bridge/ports", $max);
		if ($ret != "OK") return $ret;
	}
	foreach ($path."/inf")
	{
		$phy = query("phyinf");
		$entry = $path."/inf:".$InDeX;
		$p = XNODE_getpathbytarget("", "phyinf", "uid", $phy, 0);
		if ($p=="") return result("FAILED", $entry."/phyinf", "Invalid PHYINF - ".$phy);
	}
	return "OK";
}

/////////////////////////////////////////////

if (verify_PHYINF($FATLADY_prefix)=="OK")
{
	set($FATLADY_prefix."/valid", "1");
	result("OK", "", "");
}

?>
