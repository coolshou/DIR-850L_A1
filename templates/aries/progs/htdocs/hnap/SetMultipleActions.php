HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php"; 
include "/htdocs/webinc/config.php";

fwrite("w",$ShellPath, "#!/bin/sh\n");
$result="OK";

$nodebase="/runtime/hnap/SetMultipleActions/SetDeviceSettings/";
include "etc/templates/hnap/SetMultipleActions_DeviceSettings.php";
if($result!="OK")
{
	//error need break;	
	TRACE_error("SetDeviceSettings is not OK ret=".$result); 
}

$nodebase="/runtime/hnap/SetMultipleActions/SetWanSettings/";
include "etc/templates/hnap/SetMultipleActions_SetWanSettings.php";

if($result!="OK")
{
	TRACE_error("SetWanSettings is not OK ret=".$result); 
}

foreach("/runtime/hnap/SetMultipleActions/SetWLanRadioSettings")
{
	$nodebase="/runtime/hnap/SetMultipleActions/SetWLanRadioSettings:".$InDeX."/";
	include "etc/templates/hnap/SetMultipleActions_WLanRadioSettings.php";
	TRACE_error("nodebase=".$nodebase); 
	if($result!="OK")
	{
		TRACE_error("SetWLanRadioSettings is not OK ret=".$result); 
	}
}

foreach("/runtime/hnap/SetMultipleActions/SetWLanRadioSecurity")
{
	$nodebase="/runtime/hnap/SetMultipleActions/SetWLanRadioSecurity:".$InDeX."/";
	include "etc/templates/hnap/SetMultipleActions_WLanRadioSecurity.php";
    TRACE_error("nodebase=".$nodebase); 
	if($result!="OK")
	{
		//error need break;
		TRACE_error("SetWLanRadioSecurity is not OK ret=".$result); 
	}
}

fwrite("w",$ShellPath, "#!/bin/sh\n");
fwrite("a",$ShellPath, "echo \"[$0]-->multiple setting\" > /dev/console\n");

if($result=="OK")
{
	fwrite("a",$ShellPath, "/etc/scripts/dbsave.sh > /dev/console\n");
	fwrite("a",$ShellPath, "service DEVICE.ACCOUNT restart > /dev/console\n");
	fwrite("a",$ShellPath, "service WAN restart > /dev/console\n");
	fwrite("a",$ShellPath, "service ".$SRVC_WLAN." restart > /dev/console\n");
	fwrite("a",$ShellPath, "xmldbc -s /runtime/hnap/dev_status '' > /dev/console\n");
	set("/runtime/hnap/dev_status", "ERROR");
	//change to reboot ,we need restart much time
	$result="REBOOT";
}
else
{
	fwrite("a",$ShellPath, "echo \"We got a error in setting, so we do nothing...\" > /dev/console");
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"><soap:Body><SetMultipleActionsResponse xmlns="http://purenetworks.com/HNAP1/"><SetMultipleActionsResult><?=$result?></SetMultipleActionsResult></SetMultipleActionsResponse></soap:Body></soap:Envelope>
