HTTP/1.1 200 OK
Content-Type: text/html

<?
/*The variables are used in js and body both, so define them here. */
$ROUTING_MAX_COUNT = query("/route/static/max");
if ($ROUTING_MAX_COUNT == "") $ROUTING_MAX_COUNT = 24; 

$TEMP_MYNAME = "adv_routing";
$TEMP_MYHELP = "help_net_routing";
$TEMP_TITLE = I18N("h","Advanced Settings")." - ".I18N("h","Routing");
$ICON_HOME = 1;
$ICON_NETMAP = 1;
include "/htdocs/webinc/templates.php";
?>
