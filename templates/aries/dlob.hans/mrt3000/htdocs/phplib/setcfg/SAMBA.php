<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/trace.php";
set("/device/samba/enable",query($SETCFG_prefix."/device/samba/enable"));
?>
