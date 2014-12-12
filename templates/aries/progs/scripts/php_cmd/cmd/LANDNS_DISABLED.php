<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
$i=0;
TRACE_debug("COUNT=".$COUNT);
TRACE_debug("PREFIX=".$PREFIX);
while($i<$COUNT)
{
	$VALUE="VALUE".$i;
	TRACE_debug("VALUE".$i."=".$$VALUE);
	$i++;
}
$path_lan=XNODE_getpathbytarget($PREFIX."/module", "inf", "uid","LAN-1", 0);
if($COUNT==0)
{

	set($path_lan."/dns4","");
	set($PREFIX."/statue","0");
}
else
{
	/*invalid parameter*/
	set($PREFIX."/statue","7");
}

?>