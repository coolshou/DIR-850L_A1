HTTP/1.1 200 OK

<?
$MISSED_CALL_MAX_COUNT = query("runtime/callmgr/voice_service/log/missed#");
if ($MISSED_CALL_MAX_COUNT == "") $MISSED_CALL_MAX_COUNT = 0;

$TEMP_MYNAME    = "st_missed_call";
$TEMP_MYGROUP   = "status";
$TEMP_STYLE		= "complex";
include "/htdocs/webinc/templates.php";
?>
