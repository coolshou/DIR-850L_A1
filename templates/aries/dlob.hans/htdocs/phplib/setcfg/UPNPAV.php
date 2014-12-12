<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

$active     = query($SETCFG_prefix."/upnpav/dms/active");
$name       = query($SETCFG_prefix."/upnpav/dms/name");
$sharepath  = query($SETCFG_prefix."/upnpav/dms/sharepath");
$count      = query($SETCFG_prefix."/runtime/device/storage/count");
$total_file = query($SETCFG_prefix."/runtime/scan_media/total_file");
$total_scan_file = query($SETCFG_prefix."/runtime/scan_media/total_scan_file");
$scan_done  = query($SETCFG_prefix."/runtime/scan_media/scan_done");
$scan_enable  = query($SETCFG_prefix."/runtime/scan_media/enable");

set("/upnpav/dms/active", $active);
set("/upnpav/dms/name",   $name);
set("/upnpav/dms/sharepath",   $sharepath);
set("/runtime/device/storage/count", $count);
set("/runtime/scan_media/total_file",   $total_file);
set("/runtime/scan_media/total_scan_file",   $total_scan_file);
set("/runtime/scan_media/enable",   $scan_enable);
?>
