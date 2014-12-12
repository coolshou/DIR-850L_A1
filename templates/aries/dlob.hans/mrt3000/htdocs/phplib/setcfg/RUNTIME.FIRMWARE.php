<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

set("/runtime/firmware/fwversion/Major",query($SETCFG_prefix."/runtime/firmware/fwversion/Major"));
set("/runtime/firmware/fwversion/Minor",query($SETCFG_prefix."/runtime/firmware/fwversion/Minor"));
?>
