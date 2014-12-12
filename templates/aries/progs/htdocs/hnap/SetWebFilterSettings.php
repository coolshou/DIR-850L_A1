HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

function XNODE_add_entry_for_QRS($base, $uid)
{
	$seqno = query($base."/seqno");
	$count = query($base."/count");
	$max   = query($base."/max");
	if ($seqno == "" && $count == "")
	{
		$seqno = 1; 
		$count = 0;
	}
	if ($max != "" && $count >= $max) return "";

	$seqno++;
	$count++;
	set($base."/seqno", $seqno);
	set($base."/count", $count);
	set($base."/entry:".$count."/uid", $uid);
	return $base."/entry:".$count;
}

/* QRS send WebFilterMethod=DENY and NumberOfEntry=0 when:
		1. Disable Website Filter
		2. Enable Website Filter but no rules
	 In these case, we disable the ACL policy and not modify the original Website Filter Rules
*/
$result = "OK";
$req_WebFilterMethod = get("","/runtime/hnap/SetWebFilterSettings/WebFilterMethod");
$req_NumberOfEntry = get("","/runtime/hnap/SetWebFilterSettings/NumberOfEntry");

if($req_NumberOfEntry != "0")
{
	$tmp_node = "/acl/accessctrl/tmp_node";
	
	/* set webfilter rules to a tmp_node, we copy back to orignal node latter, don't change the node sequence */
	$max_entry = get("","/acl/accessctrl/webfilter/max");
	if($max_entry == "") $max_entry = 40;

	set($tmp_node."/seqno",1);
	set($tmp_node."/max",$max_entry);	
	set($tmp_node."/count",0);	
	
	/* set webfilter rules */
	if($req_WebFilterMethod == "ALLOW") 			{set($tmp_node."/policy","ACCEPT");}
	else if($req_WebFilterMethod == "DENY") 	{set($tmp_node."/policy","DROP");}
	else 																			
	{
		$result = "ERROR";
		TRACE_error("SetWebFilterSettings is not OK: req_WebFilterMethod=".$req_WebFilterMethod); 
	}
	if($req_NumberOfEntry <= $max_entry)
	{
		foreach("/runtime/hnap/SetWebFilterSettings/WebFilterURLs/string")
		{
			$req_string = get("","/runtime/hnap/SetWebFilterSettings/WebFilterURLs/string:".$InDeX);
			$newentry = XNODE_add_entry($tmp_node,"URLF");
			
			anchor($newentry);
			set("url",$req_string);		
		}
		
		//move tmp node
		movc($tmp_node,"/acl/accessctrl/webfilter");
		del($tmp_node); 
	}
	else 
	{
		$result = "ERROR";
		TRACE_error("SetWebFilterSettings is not OK: NumberOfEntry > max_entry"); 
	}
}

/* set ACL rules */
if($result == "OK")
{
	$QRS_ACL_UID = "ca874728-1390-41e4-b67b-d2737e0bfca6";
	$acl_nodebase = "/acl/accessctrl";
	
	$QRS_ACL_EXIST = false;
	$i=1;
	foreach($acl_nodebase."/entry:".$i)
	{
		if(get("","uid") == $QRS_ACL_UID)
		{
			$QRS_ACL_EXIST = true;
			break;
		}
		$i++;
	}
	
	/* if ACL ploicy for website filter EXIST, disable it when NumberOfEntry=0 */
	if($req_NumberOfEntry == "0")
	{
		if($QRS_ACL_EXIST == true) set($acl_nodebase."/entry:".$i."/enable",0);
	}
	
	/* create an ACL ploicy for website filter */
	if($req_NumberOfEntry != "0")
	{
		set($acl_nodebase."/enable",1);
		
		if($QRS_ACL_EXIST == false)
		{
			$newentry = XNODE_add_entry_for_QRS($acl_nodebase,$QRS_ACL_UID); //set a unique uid
			anchor($newentry);
			set("enable",1);
			set("description","QRS_Website_Filter");
			set("action","BLOCKSOME");
			set("portfilter/enable",0);
			set("webfilter/enable",1);
			set("webfilter/logging",0);
			set("schedule","");
			set("machine/entry/type","OTHERMACHINES");
			set("machine/entry/value","Other Machines");
		}
		else
		{
			$setentry = $acl_nodebase."/entry:".$i;
			anchor($setentry);
			set("enable",1);
			set("description","QRS_Website_Filter");
			set("action","BLOCKSOME");
			set("portfilter/enable",0);
			set("webfilter/enable",1);
			set("webfilter/logging",0);
			set("schedule","");
			set("machine/entry/type","OTHERMACHINES");
			set("machine/entry/value","Other Machines");			
		}
	}
	
	$result = "REBOOT";	
	
	fwrite("w",$ShellPath, "#!/bin/sh\n"); 
	fwrite("a",$ShellPath, "echo [$0] > /dev/console\n");
	fwrite("a",$ShellPath, "/etc/scripts/dbsave.sh > /dev/console\n");	
	fwrite("a",$ShellPath, "service ACCESSCTRL restart > /dev/console\n");
	fwrite("a",$ShellPath, "sleep 3 > /dev/console\n"); //Sammy
	fwrite("a",$ShellPath, "reboot > /dev/console\n"); 
	set("/runtime/hnap/dev_status", "ERROR");

}
else
{
	fwrite("w",$ShellPath, "#!/bin/sh\n"); 
	fwrite("a",$ShellPath, "echo [$0] > /dev/console\n");
	fwrite("a",$ShellPath, "echo \"We got a error in setting, so we do nothing...\" > /dev/console");
}

?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<soap:Body>
<SetWebFilterSettingResponse xmlns="http://purenetworks.com/HNAP1/">
	<SetWebFilterSettingsResult><?=$result?></SetWebFilterSettingsResult>
</SetWebFilterSettingResponse>
</soap:Body>
</soap:Envelope>
