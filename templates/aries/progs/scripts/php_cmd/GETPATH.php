<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/inf.php";
/*************************Find Path***************************************/
$path=XNODE_getpathbytarget($BASE, $NODE, $TARGET,$VALUE, $CREATE);
/**************************Return Path************************************/
echo $path;
?>