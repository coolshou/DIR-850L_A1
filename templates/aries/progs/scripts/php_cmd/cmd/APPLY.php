<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
TRACE_debug("COUNT=".$COUNT);
TRACE_debug("PREFIX=".$PREFIX);
if($COUNT==0)
{
	set($PREFIX."/statue","0");
	set("/runtime/session/function","2");
}
else
{
	set($PREFIX."/statue","7");
}
?>