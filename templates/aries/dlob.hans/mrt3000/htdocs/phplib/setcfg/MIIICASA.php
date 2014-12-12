<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

$enable     = query($SETCFG_prefix."/miiicasa/enable");

set("/miiicasa/enable", $enable);
?>
