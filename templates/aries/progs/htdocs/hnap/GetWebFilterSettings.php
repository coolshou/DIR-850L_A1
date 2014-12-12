HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

$result = "OK";
$WebFilterMethod = get("","/acl/accessctrl/webfilter/policy");

if($WebFilterMethod == "ACCEPT")			{$WebFilterMethod = "ALLOW";}
else if($WebFilterMethod == "DROP") 	{$WebFilterMethod = "DENY";}
else 																	{$result = "ERROR";}

function print_WebFilterURLs()
{
	echo "<WebFilterURLs>";
	foreach("/acl/accessctrl/webfilter/entry")
	{
		echo "<string>".get("x","url")."</string>";
	}
	echo "</WebFilterURLs>";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
<soap:Body>
<GetWebFilterSettingResponse xmlns="http://purenetworks.com/HNAP1/">
	<GetWebFilterSettingsResult><?=$result?></GetWebFilterSettingsResult>
	<WebFilterMethod><?=$WebFilterMethod?></WebFilterMethod>
	<?print_WebFilterURLs();?>
</GetWebFilterSettingResponse>
</soap:Body>
</soap:Envelope>

