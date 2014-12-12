<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

$to = "/runtime/wps/setting";
$from = $SETCFG_prefix."/runtime/wps/setting";

set($to."/aplocked",	query($from."/aplocked"));
?>
