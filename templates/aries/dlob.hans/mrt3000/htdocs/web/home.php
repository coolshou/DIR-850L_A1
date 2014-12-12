HTTP/1.1 200 OK
Content-Type: text/html

<?
$TEMP_MYNAME = "home";
$TEMP_TITLE = query("/runtime/device/vendor").' '.query("/runtime/device/modelname").' '.I18N("h","Setup Page");
$ICON_ADV = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
