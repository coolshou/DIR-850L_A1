<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

function set_result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;
	return $result;
}

function filter_setcfg($prefix, $inf)
{
	$base = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$phyinf_name = query($base."/phyinf");
	//TRACE_debug("1. base=".$base.", phyinf=".$phyinf_name);
	/* Check FILTER */
	
	$phyinf_base = XNODE_getpathbytarget("", "phyinf", "uid", $phyinf_name, 0);
	$filter_uid = query($phyinf_base."/filter");
	//TRACE_debug("2. phyinf_base=".$phyinf_base.", filter_uid=".$filter_uid);
	
	//$filter_uid = query($prefix."/phyinf/filter");
	$filterp = XNODE_getpathbytarget($prefix."/filter", "remote_mnt", "uid", $filter_uid, 0);
	//if ($filterp == "")
	//{
		/* internet error, no i18n(). */
	//	set_result("FAILED", $prefix."/inf/phyinf", "Invalid filter");
	//	return;
	//}	
	
	/* set filter name from runtime node. */
	/*
	<phyinf>
		<uid>ETH-1</uid>
		<active>1</active>
		<type>eth</type>
		<filter>FILTER-1</filter>
	</phyinf>
	*/
	$runtime_phyinf_base = XNODE_getpathbytarget($prefix, "phyinf", "uid", $phyinf_name, 0);
	$runtime_filter_uid = query($runtime_phyinf_base."/filter");
	//TRACE_debug("2. runtime_phyinf_base=".$runtime_phyinf_base);
	//TRACE_debug("2. runtime_filter_uid=".$runtime_filter_uid);
	$dst_phyinfp = XNODE_getpathbytarget("", "phyinf", "uid", $phyinf_name, 0);
	set($dst_phyinfp."/filter",		$runtime_filter_uid);		
	//TRACE_debug("3. dst_phyinfp=".$dst_phyinfp.", runtime_filter_uid=".$runtime_filter_uid);
	
	/* move the filter profile. */
	if ($filterp != "")
	{		
		$dst = XNODE_getpathbytarget("/filter", "remote_mnt", "uid", $filter_uid, 0);		
		$src = XNODE_getpathbytarget($prefix."/filter", "remote_mnt", "uid", $filter_uid, 0);
		if ($src!="" || $dst!="")
		{
			 movc($src, $dst);
		}		
		//set($to."/entry:".$bwc_index."/enable",		query("enable"));
	}
}

?>
