<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

/*************set ripng **********************************/
	
    $ripng_cnt = query("/route6/dynamic/ripng/entry#");
	while ($ripng_cnt>0)
	{
	     del("/route6/dynamic/ripng/entry");
	     $ripng_cnt--;
	}
	$ripng_cnt = query($SETCFG_prefix."/route6/dynamic/ripng/count");
	$ripng_seqno = query($SETCFG_prefix."/route6/dynamic/ripng/seqno");
	foreach ($SETCFG_prefix."/route6/dynamic/ripng/entry")
	{
		if ($InDeX > $ripng_cnt) break;
		if (query("uid")=="")
		{
			set("uid", "RIPNG-".$ripng_seqno);
			$ripng_seqno++;
		}		
		set("/route6/dynamic/ripng/entry:".$InDeX."/uid",               query("uid"));
        set("/route6/dynamic/ripng/entry:".$InDeX."/enable",          query("enable"));
		set("/route6/dynamic/ripng/entry:".$InDeX."/inf",             query("inf"));
		set("/route6/dynamic/ripng/entry:".$InDeX."/description",           query("description"));
	}
	set("/route6/dynamic/ripng/seqno", $ripng_seqno);
	set("/route6/dynamic/ripng/count", $ripng_cnt);
	
/*************set rip **********************************/	
    $rip_cnt = query("/route/dynamic/rip/entry#");
	while ($rip_cnt>0)
	{
		del("/route/dynamic/rip/entry");
		$rip_cnt--;
	}
	$rip_cnt = query($SETCFG_prefix."/route/dynamic/rip/count");
	$rip_seqno = query($SETCFG_prefix."/route/dynamic/rip/seqno");
	foreach ($SETCFG_prefix."/route/dynamic/rip/entry")
	{
		if ($InDeX > $rip_cnt) break;
		if (query("uid")=="")
		{
			set("uid", "RIP-".$rip_seqno);
			$rip_seqno++;
	    }
		set("/route/dynamic/rip/entry:".$InDeX."/uid",              query("uid"));
		set("/route/dynamic/rip/entry:".$InDeX."/enable",         query("enable"));
		set("/route/dynamic/rip/entry:".$InDeX."/inf",            query("inf"));
		set("/route/dynamic/rip/entry:".$InDeX."/transmit",         query("transmit"));
		set("/route/dynamic/rip/entry:".$InDeX."/receive",          query("receive"));
	}
	set("/route/dynamic/rip/seqno", $rip_seqno);
    set("/route/dynamic/rip/count", $rip_cnt);
?>
