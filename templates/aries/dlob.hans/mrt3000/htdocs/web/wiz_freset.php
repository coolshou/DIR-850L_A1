HTTP/1.1 200 OK
Content-Type: text/html

<?
/* Enter wizard without login when it is factory default.*/
if(query("/device/wiz_freset")!="1") $AUTHORIZED_GROUP = 0;

$TEMP_MYNAME = "wiz_freset";
$TEMP_TITLE = I18N("h","Welcome to the setup page of your miiiCasa $1 router", query("/runtime/device/modelname"));
$ICON_HOME = 0;
$ICON_ADV = 0;
$ICON_NETMAP = 0;
$FIRMWARE = 1;
include "/htdocs/webinc/templates.php";
?>
