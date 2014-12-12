<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

set("/device/simple_security",	query($SETCFG_prefix."/device/simple_security"));
?>
