HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
$URL_MAX_COUNT = query("/acl/accessctrl/webfilter/max");
if ($URL_MAX_COUNT == "") $URL_MAX_COUNT = 32;

$TEMP_MYNAME = "adv_web_filter";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Website Filter");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
