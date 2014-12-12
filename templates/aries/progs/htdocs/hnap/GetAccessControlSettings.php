HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
$result = OK;
$nodebase="/runtime/hnap/GetAccessControlSettings";
del("/runtime/hnap/GetAccessControlSettings");
$nodebase=$nodebase."/entry";
$k=0;

if(query("/acl/accessctrl/enable")=="1")
{

        $i=0;
        foreach ("/acl/accessctrl/entry")
        {
                if(query("enable")=="1")
                {
                        $i++;
			   $j=0;
			   set($nodebase.":".$i."/policy", query("description"));
                        foreach ("machine/entry")
                        {
                        	    $j++;
                                if(query("type")=="IP")
                                {
                                	     set($nodebase.":".$i."/machine/entry:".$j."/AddressType", "IPAddress");
					     set($nodebase.":".$i."/machine/entry:".$j."/IPAddress", query("value"));
					     set($nodebase.":".$i."/machine/entry:".$j."/MacAddress", ""); 
                                }
                                else if(query("type")=="MAC")
                                {
                                	     set($nodebase.":".$i."/machine/entry:".$j."/AddressType", "MacAddress");
					     set($nodebase.":".$i."/machine/entry:".$j."/IPAddress", "");
					     set($nodebase.":".$i."/machine/entry:".$j."/MacAddress", query("value"));                                          
                                }
                                else
                                {
                                        set($nodebase.":".$i."/machine/entry:".$j."/AddressType", "OtherMachine");
					     set($nodebase.":".$i."/machine/entry:".$j."/IPAddress", query("value"));
					     set($nodebase.":".$i."/machine/entry:".$j."/MacAddress", query("value"));  
					     
                                }
                        }

				/* time */
                        $uid = query("schedule");
                        if ($uid=="")
                        {
                        		 set($nodebase.":".$i."/schedule", "Always");  
                        }
                        else
                        {
			   		$sch = XNODE_getpathbytarget("/schedule", "entry", "uid", $uid, 0);
					set($nodebase.":".$i."/schedule", query($sch."/description")); 
                        }


                        if(query("action")=="LOGWEBONLY")
                        {
                        		 set($nodebase.":".$i."/method", "LogWebAccessOnly");  
                        }
                        else if(query("action")=="BLOCKALL")
                        {
                        		set($nodebase.":".$i."/method", "BlockAllAccess");
                        }
			   else
			   {
			   		set($nodebase.":".$i."/method", "BlockSomeAccess");
			   }
                }
        }
}
else
{
	$result = ERROR;
}
?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
	<GetAccessControlSettingsResponse xmlns="http://purenetworks.com/HNAP1/">
		<GetAccessControlSettingsResult><?=$result?></ GetAccessControlSettingsResult>
 		<AccessControlInfoLists>
foreach($nodebase)
{
	$k++;
	echo "        <AccessControl>\n";
	echo "          <Policy>".query("policy")."</Policy>\n";
	echo "          <Schedule>".query("schedule")."</Schedule>\n";
	foreach($nodebase.":".$k."/machine/entry)
	{
		echo "          <AddressType>".query("AddressType")."</AddressType>\n";
		echo "          <MacAddress>".query("MacAddress")."</MacAddress>\n";
		echo "          <IPAddress>".query("IPAddress")."</IPAddress>\n";
	}
	echo "          <Method>".query("method")."</Method>\n";
	echo "        </AccessControl>\n";
}
		</AccessControlInfoLists>
	</GetAccessControlSettingsResponse>
  </soap:Body>
</soap:Envelope>

