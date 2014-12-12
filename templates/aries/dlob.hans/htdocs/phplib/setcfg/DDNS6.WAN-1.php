<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

$cnt = query("/ddns6/entry#");
while ($cnt>0)
{
	del("/ddns6/entry");
	$cnt--;
}

$cnt=query($SETCFG_prefix."/ddns6/cnt");
set("/ddns6/cnt", $cnt);

$index=1;
while($index <= $cnt)
{

	set("/ddns6/entry:".$index."/enable",	query($SETCFG_prefix."/ddns6/entry:".$index."/enable")	);
	set("/ddns6/entry:".$index."/v6addr",	query($SETCFG_prefix."/ddns6/entry:".$index."/v6addr"));
	set("/ddns6/entry:".$index."/hostname",	query($SETCFG_prefix."/ddns6/entry:".$index."/hostname"));
	$index++;
}


?>
