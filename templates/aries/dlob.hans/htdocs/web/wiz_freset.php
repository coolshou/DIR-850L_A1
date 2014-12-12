HTTP/1.1 200 OK

<?
/* Enter wizard without login when it is factory default.*/
if(query("/runtime/device/devconfsize")=="0") $AUTHORIZED_GROUP = 0;

$TEMP_MYNAME	= "wiz_freset";
$TEMP_MYGROUP	= "";
$TEMP_STYLE		= "simple";
include "/htdocs/webinc/templates.php";
?>
