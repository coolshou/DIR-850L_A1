HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
$MAC_FILTER_MAX_COUNT = query("/acl/macctrl/max");
if ($MAC_FILTER_MAX_COUNT == "") $MAC_FILTER_MAX_COUNT = 25;

$TEMP_MYNAME = "adv_mac_filter";
$TEMP_MYHELP = "help_sec_mac_filter";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","MAC Address Filtering");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
