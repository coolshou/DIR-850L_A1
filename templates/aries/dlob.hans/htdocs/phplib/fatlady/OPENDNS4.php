<? /* vi: set sw=4 ts=4: */
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

function set_result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;
	return $result;
}

//////////////////////////////////////////////////////////////////////////////

$wan1_infp_fatlady = XNODE_getpathbytarget($FATLADY_prefix, "inf", "uid", "WAN-1", 0);
if (query($wan1_infp_fatlady."/open_dns/type") == "parent" && query($wan1_infp_fatlady."/open_dns/deviceid") == "")
	$ret = set_result("FAILED", $wan1_infp_fatlady."/open_dns/deviceid", i18n("Please register your device."));
$ret = "OK";

TRACE_debug("FATLADY: OPENDNS4: ret = ".$ret);
if ($ret=="OK") set($FATLADY_prefix."/valid", "1");
?>
