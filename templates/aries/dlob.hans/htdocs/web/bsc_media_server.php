HTTP/1.1 200 OK

<?
/*necessary and basic definition */
$TEMP_MYNAME    = "bsc_media_server";
$TEMP_MYGROUP   = "basic";
$TEMP_STYLE		= "complex";
include "/htdocs/webinc/templates.php";
dophp("load", "/htdocs/web/portal/comm/drag.php");
dophp("load", "/htdocs/web/portal/comm/event.php");
dophp("load", "/htdocs/web/portal/comm/fade.php");
dophp("load", "/htdocs/web/portal/comm/overlay.php");
dophp("load", "/htdocs/web/portal/comm/scoot.php");
?>
