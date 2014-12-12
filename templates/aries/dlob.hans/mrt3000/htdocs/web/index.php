<?
if (query("/device/wiz_freset")!="1")
{
	dophp("load", "/htdocs/web/wiz_freset.php");	
}	
else dophp("load", "/htdocs/web/home.php");
?>
