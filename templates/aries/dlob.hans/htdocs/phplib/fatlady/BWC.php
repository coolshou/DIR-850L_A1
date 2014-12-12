<?
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
function set_result($result, $node, $message)
{
    $_GLOBALS["FATLADY_result"] = $result;
    $_GLOBALS["FATLADY_node"]   = $node;
    $_GLOBALS["FATLADY_message"]= $message;
}

set_result("FAILED","","");
$rlt = "0";
$bwcf_cnt = query($FATLADY_prefix."/bwc/bwcf/count");
if ($bwcf_cnt > query("/bwc/bwcf/max"))
{
	set_result("FAILED", $FATLADY_prefix."/bwc/bwcf/count", i18n("The rules exceed maximum."));
	$rlt = "-1";
}

if($rlt == "0")
{
	set($FATLADY_prefix."/valid", "1");
	set_result("OK", "", "");
}
?>
