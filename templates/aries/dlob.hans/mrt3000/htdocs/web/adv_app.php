HTTP/1.1 200 OK
Content-Type: text/html

<?
/*The variables are used in js and body both, so define them here. */
include "/htdocs/phplib/xnode.php";
$p = XNODE_getpathbytarget("/nat", "entry", "uid", "NAT-1", 0);
$APP_MAX_COUNT = query($p."/porttrigger/max");
if ($APP_MAX_COUNT == "") $APP_MAX_COUNT = 24; 

$TEMP_MYNAME = "adv_app";
$TEMP_MYHELP = "help_sec_application_rules";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Application Rules");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
