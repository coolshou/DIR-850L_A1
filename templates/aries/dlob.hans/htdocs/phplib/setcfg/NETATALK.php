<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

$active     = query($SETCFG_prefix."/netatalk/active");
$sharepath  = query($SETCFG_prefix."/netatalk/sharepath");

set("/netatalk/active", 		$active);
set("/netatalk/sharepath",   	$sharepath);

?>
