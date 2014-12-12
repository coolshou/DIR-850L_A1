<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/setcfg/libs/wifi.php";
wifi_setcfg($SETCFG_prefix);
set("/device/radio24gonoff",query($SETCFG_prefix."/device/radio24gonoff"));
set("/device/radio24gcfged",query($SETCFG_prefix."/device/radio24gcfged"));
set("/device/wiz_freset",query($SETCFG_prefix."/device/wiz_freset"));
set("/device/wiz_clonemac",query($SETCFG_prefix."/device/wiz_clonemac"));
?>
