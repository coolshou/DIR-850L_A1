<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

anchor($SETCFG_prefix."/device/time");
set("/device/time/ntp/enable",	query("ntp/enable"));
set("/runtime/device/ntp/state", "RUNNING");
set("/device/time/ntp/period",	query("ntp/period"));
set("/device/time/ntp/server",	query("ntp/server"));
set("/device/time/timezone",	query("timezone"));
set("/device/time/dst",			query("dst"));
set("/device/time/dstmanual",	query("dstmanual"));
set("/device/time/dstoffset",	query("dstoffset"));
set("/device/time/time",	query("time"));
set("/device/time/date",	query("date"));
/* ipv6 */
set("/device/time/ntp6/enable",	query("ntp6/enable"));
set("/runtime/device/ntp6/state", "RUNNING");
set("/device/time/ntp6/period",	query("ntp6/period"));
?>
