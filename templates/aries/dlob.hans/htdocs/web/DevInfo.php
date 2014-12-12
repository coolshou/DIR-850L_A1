HTTP/1.1 200 OK

<?
if ($AUTHORIZED_GROUP < 0)
{
	echo "Authentication Fail. Please Login First!";
}
else
{
	echo "Firmware External Version: V".cut(fread("", "/etc/config/buildver"), "0", "\n")."\n";
	echo "Firmware Internal Version: ".cut(fread("", "/etc/config/buildno"), "0", "\n")."\n";
	echo "Model Name: ".query("/runtime/device/modelname")."\n";
	echo "Hardware Version: ".query("/runtime/devdata/hwrev")."\n";
	echo "WLAN Domain: ".query("/runtime/devdata/countrycode")."\n";
	$ver = cut(fread("", "/proc/version"), "0", "(")."\n";
	echo "Kernel: ".cut($ver, "2", " ")."\n";
	$lang = query("/runtime/device/langcode"); 
	if ($lang=="") $lang="en";
	echo "Language: ".$lang."\n";
	if (query("/device/session/captcha")=="1") $captcha="Enable"; 
	else $captcha="Disable";
	echo "Graphcal Authentication: ".$captcha."\n";
	echo "LAN MAC: ".query("/runtime/devdata/lanmac")."\n";
	echo "WAN MAC: ".query("/runtime/devdata/wanmac")."\n";
	echo "WLAN MAC: ".query("/runtime/devdata/wlanmac")."\n";	
}		
?>