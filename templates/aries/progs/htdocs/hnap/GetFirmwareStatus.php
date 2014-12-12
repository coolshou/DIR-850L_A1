HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

$result = "OK";
$buildver = fread("s", "/etc/config/buildver");
$CurrentMajor = cut($buildver,0,".");
$CurrentMinor = substr(cut($buildver,1,"."), 0, 2);
$CurrentFWVersion = $CurrentMajor.".".$CurrentMinor;
$path_run_inf_lan1 = XNODE_getpathbytarget("/runtime", "inf", "uid", $LAN1, 0);
$FWUploadUrl = get("",$path_run_inf_lan1."/inet/ipv4/ipaddr")."/fwupload.cgi";

//we run checkfw.sh in hnap.cgi, so we can get fw info. now
if(isfile("/tmp/fwinfo.xml")==1)
{
	TRACE_debug("checkfw.sh success!");
	
	$LatesMajor = substr(get("","/runtime/firmware/fwversion/Major"), 1, 2);
	$LatesMinor = get("","/runtime/firmware/fwversion/Minor");
	$LatestFWVersion = $LatesMajor.".".$LatesMinor;
	$FWDownloadUrl = get("","/runtime/firmware/FWDownloadUrl");
}
else
{
	TRACE_debug("checkfw.sh fail!");
	
	$result = "ERROR";
	$LatestFWVersion = "null";
	$FWDownloadUrl = "null";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
<soap:Body>
	<GetFirmwareStatusResponse xmlns="http://purenetworks.com/HNAP1/">
		<GetFirmwareStatusResult><?=$result?></GetFirmwareStatusResult>
		<CurrentFWVersion><?=$CurrentFWVersion?></CurrentFWVersion>
		<LatestFWVersion><?=$LatestFWVersion?></LatestFWVersion>
		<FWDownloadUrl><? echo escape("x",$FWDownloadUrl); ?></FWDownloadUrl>
		<FWUploadUrl><? echo escape("x",$FWUploadUrl); ?></FWUploadUrl>
	</GetFirmwareStatusResponse>
</soap:Body>
</soap:Envelope>