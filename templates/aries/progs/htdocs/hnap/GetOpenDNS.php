HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/webinc/config.php";

$WAN1_INF = INF_getinfpath($WAN1);
if(query($WAN1_INF."/open_dns/type")=="") $EnableOpenDNS=false;
else $EnableOpenDNS=true;
$DeviceID = query($WAN1_INF."/open_dns/deviceid");
if(query($WAN1_INF."/open_dns/type")=="parent") $OpenDNSMode="Parental"; 
else if(query($WAN1_INF."/open_dns/type")=="family") $OpenDNSMode="FamilyShield";
else $OpenDNSMode="Advanced";
$OpenDNSMode = query($WAN1_INF."/open_dns/type");
$DeviceKey = query($WAN1_INF."/open_dns/nonce");

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
	<soap:Body>
		<GetOpenDNSResponse xmlns="http://purenetworks.com/HNAP1/">
			<GetOpenDNSResult>OK</GetOpenDNSResult>
			<EnableOpenDNS><?=$EnableOpenDNS?></EnableOpenDNS>
			<OpenDNSDeviceID><?=$DeviceID?></OpenDNSDeviceID>
			<OpenDNSMode><?=$OpenDNSMode?></OpenDNSMode>
			<OpenDNSDeviceKey><?=$DeviceKey?></OpenDNSDeviceKey>
		</GetOpenDNSResponse>
	</soap:Body>
</soap:Envelope>