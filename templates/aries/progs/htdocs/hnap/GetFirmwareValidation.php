HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

/***************************************************************************/
/*
 *  call this action after QRS upload firmware to router
 *	firmware location is /var/firmware.seama
 *	ckfw() is used to check firmware validation (return 0 success)
 *	count_fwuptime() is used to count firmware upgrade time
 *	
 *	sammy 2012/8/28
 */
function ckfw()
{
	setattr("/runtime/chfw",  "get", "fwupdater -v /var/firmware.seama; echo $?");
	return get("","/runtime/chfw");
}

function count_fwuptime()
{
	$size	= fread("x","/var/session/imagesize"); if ($size == "") $size = "8000000";
	$fptime	= query("/runtime/device/fptime"); if ($fptime == "") $fptime = "1000";
	$bt		= query("/runtime/device/bootuptime"); if ($bt == "") $bt = "60";
	$delay	= 10;	
	
	return $size/64000*$fptime/1000+$bt+$delay+20;
}

$result = "REBOOT";
$IsValid = ckfw();

if($IsValid == "0")
{
	$IsValid = "true";
	$CountDown = count_fwuptime();
	
	fwrite("a",$ShellPath, "echo [$0] > /dev/console\n");
	fwrite("a",$ShellPath, "sleep 3 > /dev/console\n");
	fwrite("a",$ShellPath, "fw_upgrade /var/firmware.seama\n");
}
else
{
	$IsValid = "false";
	$CountDown = "0";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
<soap:Body>
<GetFirmwareValidationResponse xmlns="http://purenetworks.com/HNAP1/">
	<GetFirmwareValidationResult><?=$result?></GetFirmwareValidationResult>
	<IsValid><?=$IsValid?></IsValid>
	<CountDown><?=$CountDown?></CountDown>
</GetFirmwareValidationResponse>
</soap:Body>
</soap:Envelope>
