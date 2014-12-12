HTTP/1.1 200 OK
Content-Type: text/html

<?
/* The variables are used in js and body both, so define them here. */
include "/htdocs/phplib/inf.php";
$FW_MAX_COUNT = query("/acl/firewall/max");
if ($FW_MAX_COUNT == "") $FW_MAX_COUNT = 32;

$TEMP_MYNAME = "adv_firewall";
$TEMP_MYHELP = "help_sec_firewall";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Firewall Settings");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
