<?
if (query("/runtime/device/devconfsize")=="0")
{
	dophp("load", "/htdocs/web/wiz_freset.php");	
}	
else	dophp("load", "/htdocs/web/bsc_internet.php");
?>
