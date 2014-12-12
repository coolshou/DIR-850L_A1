<?
include "/htdocs/webinc/feature.php";

$layout = query("/device/layout");

if (query("/runtime/device/devconfsize")=="0")
{
	if ($FEATURE_CHINA==1 && $layout!="bridge")
	{	dophp("load", "/htdocs/web/bsc_easysetup.php");}
	else
	{	dophp("load", "/htdocs/web/wiz_freset.php");}
}	
else
{
	if ($FEATURE_CHINA==1 && $layout!="bridge")
	{	dophp("load", "/htdocs/web/bsc_easysetup.php");}
	else
	{	dophp("load", "/htdocs/web/bsc_internet.php");}
}
?>
