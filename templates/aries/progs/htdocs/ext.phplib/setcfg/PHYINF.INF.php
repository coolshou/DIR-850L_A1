<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/xnode.php";

function copy_bridgeports($dst, $src)
{
	$cnt = query($dst."/entry#");
	while ($cnt > 0) {del($dst."/entry:".$cnt); $cnt--;}

	$cnt = query($src."/count"); if ($cnt=="") $cnt=0;
	set($dst."/seqno", query($src."/seqno"));
	set($dst."/count", $cnt);
	foreach ($src."/entry")
	{
		if ($InDeX>$cnt) break;
		set($dst."/entry:".$InDeX."/uid",	query("uid"));
		set($dst."/entry:".$InDeX."/phyinf",query("phyinf"));
	}
}

function copy_PHYINF($dst, $src)
{
	foreach ($src."/phyinf")
	{
		$uid = query("uid");
		$dpath = XNODE_getpathbytarget($dst, "phyinf", "uid", $uid, 0);
		if ($dpath!="")
		{
			set($dpath."/active",			query("active"));
			set($dpath."/macaddr",			query("macaddr"));
			set($dpath."/mtu",				query("mtu"));
			set($dpath."/bridge/enable",	query("bridge/enable"));
			set($dpath."/bridge/vid",		query("bridge/vid"));
			copy_bridgeports($dpath."/bridge/ports", $src."/phyinf:".$InDeX."/bridge/ports");
		}
	}
	foreach ($src."/inf")
	{
		$uid = query("uid");
		$phy = query("phyinf");
		$p = XNODE_getpathbytarget($dst, "inf", "uid", $uid, 0);
		if ($p!="") set($p."/phyinf", $phy);
	}
}

//////////////////////////////////////
copy_PHYINF("", $SETCFG_prefix);
?>
