HTTP/1.1 200 OK 
Content-Type: text/xml; charset=utf-8 
Content-Length: <Number of Bytes/Octets in the Body> 
<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
$Result = OK;
if ("1" == query("/mydlink/register_st"))
{
	$register_st = true;
}
else if ("0" == query("/mydlink/register_st"))
{
	$register_st = false;
}
else
{
	$register_st = false;
	$Result = ERROR;
}
?>

<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"> 
<soap:Body> 
<GetMyDlinkSettingsResponse xmlns="http://purenetworks.com/HNAP1/"> 
	<GetMyDLinkSettingsResult><?=$Result?></GetMyDLinkSettingsResult> 
    	<Enabled><?=$register_st?> </Enabled> 
 	<Email><?echo query("/mydlink/regemail");?></Email>  
 	<Password>""</Password> 
 	<LastName>""</LastName> 
 	<FirstName>""</FirstName> 
 	<DeviceUserName><?echo query("/device/account/entry/name");?></DeviceUserName> 
 	<DevicePassword><?echo query("/device/account/entry/password");?></DevicePassword> 
</GetMyDlinkSettingsResponse> 
</soap:Body> 
</soap:Envelope>
