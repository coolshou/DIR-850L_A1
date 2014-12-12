HTTP/1.1 200 OK
Content-Type: text/html

<?
$TEMP_MYNAME = "tools_admin";
$TEMP_MYHELP = "help_rt_admin";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Administrator Settings");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
$USR_ACCOUNTS	= query("/device/account/count");
include "/htdocs/webinc/templates.php";
?>
