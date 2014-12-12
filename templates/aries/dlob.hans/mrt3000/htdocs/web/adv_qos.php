HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
$QOS_MAX_COUNT = query("/bwc/bwcf/max");
if ($QOS_MAX_COUNT == "") $QOS_MAX_COUNT = 32;

$TEMP_MYNAME = "adv_qos";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","QoS");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
