HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
$p = XNODE_getpathbytarget("/dhcps4", "entry", "uid", "DHCPS4-1", 0);
$DHCP_MAX_COUNT = query($p."/staticleases/max");

$TEMP_MYNAME = "bsc_lan";
$TEMP_MYHELP = "help_net_lan";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Network Settings");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
