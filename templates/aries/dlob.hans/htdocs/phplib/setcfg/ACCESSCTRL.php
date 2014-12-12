<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
$cnt = query("/acl/accessctrl/entry#");
while ($cnt>0)
{
	del("/acl/accessctrl/entry");
	$cnt--;
}

$cnt = query($SETCFG_prefix."/acl/accessctrl/count");
$seqno = query($SETCFG_prefix."/acl/accessctrl/seqno");

foreach ($SETCFG_prefix."/acl/accessctrl/entry")
{
	if ($InDeX > $cnt) break;
	if (query("uid")=="")
	{	
		set("uid", "ACS-".$seqno);
		$seqno++;
	}	
}
movc($SETCFG_prefix."/acl/accessctrl/", "/acl/accessctrl/");
set("/acl/accessctrl/seqno", $seqno);
set("/acl/accessctrl/count", $cnt);
?>
