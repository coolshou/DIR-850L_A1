HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
$p = XNODE_getpathbytarget("/nat", "entry", "uid", "NAT-1", 0);
$PFWD_MAX_COUNT = query($p."/portforward/max");

$TEMP_MYNAME = "adv_pfwd";
$TEMP_MYHELP = "help_net_port_forwarding";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Port Forwarding");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
