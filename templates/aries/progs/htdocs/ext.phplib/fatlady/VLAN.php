<?
/* fatlady is used to validate the configuration for the specific service.
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

function verify_PORT($path)
{
	/* Port number should be the same, no port can be added/deleted.
	 * It is h/w board dependent. */
	$portnum = query($path."/port#");
	TRACE_debug("verify_PORT: portnum=".$portnum.", port#=".query("/vlan/port#"));
	if ($portnum != query("/vlan/port#"))
		return result("FAILED", $path."/port:".$portnum, "Invalid PORT number.");

	/* Walk through each port setting */
	foreach ($path."/port")
	{
		$entry = $path."/entry:".$InDeX;

		/* Check UID, it's read-only. */
		$val = query("/vlan/port:".$InDeX."/uid");
		if ($val != query("uid")) return result("FAILED", $entry."/uid", "UID is read-only.");

		/* Check VID. */
		$val = query("vid");
		if (isdigit($val)==0 || $val>4096 || $vlan<0)
			return result("FAILED", $entry."/vid",
					i18n("VLAN ID should be a decimal number between 0~4095."));
	}
	return "OK";
}

function verify_vlan_member($path, $max)
{
	$count = query($path."/count"); if ($count=="") $count=0;
	$seqnp = query($path."/seqno"); if ($seqno=="") $seqno=1;
	if ($count > $max) $count = $max;
	$count+=0; $seqno+=0;

	TRACE_debug("FATLADY: VLAN members: max=".$max.", count=".$count.", seqno=".$seqno.", path=".$path);

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
		$port = query("port");
		$val = XNODE_getpathbytarget("/vlan", "port", "uid", $port, 0);
		if ($port=="") return result("FAILED", $entry."/port", i18n("Invalid member port."));

		/* check tag. */
		if (query("tag")!=1) set("tag", 0);
	}
	return "OK";
}

function verify_VLAN($path)
{
	$max	= query("/vlan/max");
	$count	= query($path."/count"); if ($count=="") $count=0;
	$seqno	= query($path."/seqno"); if ($seqno=="") $seqno=1;
	if ($count > $max) $count = $max;
	$count+=0; $seqno+=0;

	TRACE_debug("FATLADY: VLAN: max=".$max.", count=".$count.", seqno=".$seqno.", path=".$path);

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
			$uid = "VLAN-".$seqno;
			set("uid", $uid);
			$seqno++;
			set($path."/seqno", $seqno);
		}
		/* Check duplicated UID */
		if ($$uid == "1") return result("FAILED", $entry."/uid", "Duplicated UID - ".$uid);
		$$UId = 1;

		/* Check ID */
		$val = query("id");
		if (isdigit($val)==0 || $id<0 || $id>4095)
			return result("FAILED", $entry."/id",
					i18n("VLAN ID should be a decimal number between 0~4095."));

		$max = query("/vlan/port#");
		$ret = verify_vlan_member($entry."/members", $max);
		if ($ret != "OK") return $ret;
	}
	return "OK";
}

///////////////////////////////////////////////

if (verify_PORT($FATLADY_prefix."/vlan")=="OK" &&
	verify_VLAN($FATLADY_prefix."/vlan")=="OK")
{
	set($FATLADY_prefix."/valid", "1");
	result("OK", "", "");
}

?>
