<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
$cnt = query("/acl/inbfilter/entry#");
while ($cnt>0)
{
	del("/acl/inbfilter/entry");
	$cnt--;
}

$cnt = query($SETCFG_prefix."/acl/inbfilter/count");
$seqno = query("/acl/inbfilter/seqno");

foreach ($SETCFG_prefix."/acl/inbfilter/entry")
{
	if ($InDeX > $cnt) break;
	if (query("uid")=="")
	{	
		set("uid", "INBF-".$seqno);
		$seqno++;
	}
	$INX = $InDeX;
	set("/acl/inbfilter/entry:".$INX."/uid",			query("uid")		);
	set("/acl/inbfilter/entry:".$INX."/description",	query("description"));
	set("/acl/inbfilter/entry:".$INX."/act",			query("act")		);
	foreach ($SETCFG_prefix."/acl/inbfilter/entry:".$INX."/iprange/entry")
	{
		$INX2 = $InDeX;
		set("/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/seq",		query("seq")	);
		set("/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/enable",		query("enable")	);
		set("/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/startip",	query("startip"));
		set("/acl/inbfilter/entry:".$INX."/iprange/entry:".$INX2."/endip",		query("endip")	);
	}	
}
set("/acl/inbfilter/seqno", $seqno);
set("/acl/inbfilter/count", $cnt);
?>
