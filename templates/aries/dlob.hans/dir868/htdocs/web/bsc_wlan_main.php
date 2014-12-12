HTTP/1.1 200 OK

<?
if(query("/device/layout")=="router") $TEMP_MYNAME = "bsc_wlan_main";
else $TEMP_MYNAME = "bsc_wlan_br"; 
$TEMP_MYGROUP   = "basic";
$TEMP_STYLE		= "complex";
include "/htdocs/webinc/templates.php";
?>
