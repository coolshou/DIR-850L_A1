<?
include "/htdocs/phplib/trace.php";


/* call $target to do error checking, 
 * and it will modify and return the variables, '$FATLADY_XXXX'. */
$FATLADY_result	= "OK";
$FATLADY_node	= "";
$FATLADY_message= "No modules";	/* this should not happen */

//TRACE_debug("FATLADY dump ====================\n".dump(0, "/runtime/session"));

foreach ($prefix."/module")
{
	del("valid");
	if (query("FATLADY")=="ignore") continue;
	$service = query("service");
	if ($service == "") continue;
	TRACE_debug("FATLADY: got service [".$service."]");
	$target = "/htdocs/phplib/fatlady/".$service.".php";
	$FATLADY_prefix = $prefix."/module:".$InDeX;
	$FATLADY_base	= $prefix;
	if (isfile($target)==1) dophp("load", $target);
	else TRACE_debug("FATLADY: no file - ".$target);
	if ($FATLADY_result!="OK") break;
}
if($FATLADY_result=="OK")
{
	echo "1";
}
else if($FATLADY_result=="FAILED")
{
	del($prefix);
	echo $FATLADY_message;
}

?>
