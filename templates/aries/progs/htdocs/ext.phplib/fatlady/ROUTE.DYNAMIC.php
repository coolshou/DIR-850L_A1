<?
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be breaked for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/inet6.php";


function result($result, $node, $msg)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $msg;
	return $result;
}
function common_setting($path, $max, $uidname)
{
  
	$count = query($path."/count"); if ($count=="") $count=0;
	$seqno = query($path."/seqno");
	if ($count > $max) $count = $max;

	TRACE_debug("FATLADY: ROUTE.DYNAMIC: max=".$max.", count=".$count.", seqno=".$seqno);
	
	/* Delete the extra entries. */
	$num = query($path."/entry#");
	while ($num>$count) { del($path."/entry:".$num); $num--; }

	/* verify each entries */
	set($path."/count", $count);
	foreach ($path."/entry")
	{
		if ($InDeX>$count) break;
  
		/* The current entry path. */
		$entry = $path."/entry:".$InDeX;

		/* Check empty UID */
		$uid = query("uid");
		
		if ($uid=="")
		{
			$uid = $uidname."-".$seqno;
			set("uid", $uid);
			$seqno++;
			set($path."/seqno", $seqno);
		}
		/* Check duplicated UID */
		if ($$uid=="1") return result("FAILED", $entry."/uid", "Duplicated UID - ".$uid);
		$$uid = "1";

		$desc = query("description");
		if($desc!="")	set("description", $desc);

		$inf = query("inf");
		$infp = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
		if ($infp=="") return result("FAILED", $entry."/inf", i18n("Invalid interface"));
	}
	return "OK";
}
function rip_setting($path)
{
	foreach ($path."/entry")
	{
		$transmit = query("transmit");
	  	if($transmit != "DISABLE"&& $transmit != "RIP1"&& $transmit != "RIP2"&& $transmit != "AUTO")
	 	{
      		return result("FAILED","","Transmit shoule be DISABLE/RIP1/RIP2/AUTO.");
   		}	  
	  	$receive = query("receive");
	  	if($receive != "DISABLE"&& $receive != "RIP1"&& $receive != "RIP2"&& $receive != "AUTO")
	  	{
      		return result("FAILED","","Receive shoule be DISABLE/RIP1/RIP2/AUTO.");
    	}	
	}
  return "OK";

}

/*************************************************/


function dynamic_route_check($prefix)
{   
    $rip_path = "route/dynamic/rip";
    $ripng_path = "route6/dynamic/ripng";
    $rip_uidname = "RIP";
    $ripng_uidname = "RIPNG";
    
    $ripng_cnt = query($prefix."/".$ripng_path."/count");
    $enable_ripngd = 0;
    if($ripng_cnt > 0)
    {
      	$i = 0;
      	foreach ($prefix."/".$ripng_path."/entry")
    	{ 
    	  $i++;
    	  $interfaceup= query($prefix."/".$ripng_path."/entry:".$i."/enable");
    	  if($interfaceup == 1)	 
    	  { 
    	    $enable_ripngd = 1;
    	    break;
    	  }	  
    	}
    }
    
    $rip_cnt = query($prefix."/".$rip_path."/count");
    $enable_ripd = 0;
    if($rip_cnt > 0)
    {
      	$i = 0;
      	foreach ($prefix."/".$rip_path."/entry")
    	{
    	  $i++;
    	  $interfaceup= query($prefix."/".$rip_path."/entry:".$i."/enable");
    	  if($interfaceup == 1)	 
    	  { 
    	    $enable_ripd = 1;
    	    break;
    	  }	  
    	}
    }
    if ($enable_ripd==1)
    {
      TRACE_debug("FATLADY: ROUTE.DYNAMIC:".$rip_uidname."=".$enable_ripd);
      $max = query("/route/dynamic/rip/max"); if ($max=="") $max=32;
      if(common_setting($prefix."/".$rip_path, $max, $rip_uidname) != "OK") return "FAILED";
      if(rip_setting($prefix."/".$rip_path) != "OK") return "FAILED";
    }
    if ($enable_ripngd==1)
    {
      TRACE_debug("FATLADY: ROUTE.DYNAMIC:".$ripng_uidname."=".$enable_ripngd);
      $max = query("/route6/dynamic/ripng/max"); if ($max=="") $max=64;
      if(common_setting($prefix."/".$ripng_path, $max, $ripng_uidname) != "OK") return "FAILED";
    }
    return "OK";
}
result("FAILED","","");
if(dynamic_route_check($FATLADY_prefix) == "OK")
{
    set($FATLADY_prefix."/valid", 1);
    result("OK", "", "");
}
?>
