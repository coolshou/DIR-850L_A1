HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
$p = XNODE_getpathbytarget("/nat", "entry", "uid", "NAT-1", 0);
$VSVR_MAX_COUNT = query($p."/virtualserver/max");
if ($VSVR_MAX_COUNT == "") $VSVR_MAX_COUNT = 5;

$TEMP_MYNAME = "adv_vsvr";
$TEMP_MYHELP = "help_net_virtual_server";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Virtual Server");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
