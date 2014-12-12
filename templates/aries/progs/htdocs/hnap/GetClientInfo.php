HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

$result = "OK";

function find_hotstname($mac, $inf)
{
	$path = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);
	$entry_path = $path."/dhcps4/leases/entry";
	$cnt = query($entry_path."#");
	while ($cnt > 0) 
	{						
		$mac2 = query($entry_path.":".$cnt."/macaddr" );
		$hostname = query($entry_path.":".$cnt."/hostname" );		
		if(toupper($mac2) == toupper($mac)) return $hostname;		
		$cnt--; 
	}
	
	return "";	
}	

function check_wireless_band($mac, $phyinf)
{
	if($mac == "" || $phyinf == "") return false;
	TRACE_debug("check_wireless_band(): mac=".$mac." phyinf=".$phyinf);
	$rphyinf = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	
	foreach($rphyinf."/media/clients/entry")
	{
		if($mac == get("","macaddr"))
		{
			TRACE_debug("check_wireless_band(): match!!\n");
			
			return true;
		}
	}
	
	return false;
}
	
function print_ClientInfoLists()
{
	include "/htdocs/webinc/config.php";
	
	$arptable   = fread("", "/proc/net/arp");
	$tailindex  = strstr($arptable, "\n")+1;
	$macindex   = strstr($arptable, "HW address");
	$devindex   = strstr($arptable, "Device");
	$tablelen   = strlen($arptable);
	$iplen      = strlen("255.255.255.255");
	$macnull    = "00:00:00:00:00:00";
	$maclen     = strlen($macnull);
	$line       = substr($arptable, $tailindex, $tablelen-$tailindex);
	
	$cnt        = 1;
	
	echo "<ClientInfoLists>";
	/* print <ClientInfo> */
	while($line != "")
	{
		$tailindex  = strstr($line, "\n")+1;
		$tmp       = substr($line, 0, $tailindex);
					
		if($line != "")
		{	
			if(strstr($tmp, "br0")!="" || strstr($tmp, "br1")!="") 
			{
				$ip     = strip(substr($tmp, 0, $iplen+1));
				$mac    = strip(substr($tmp, $macindex, $maclen));
				if($mac != $macnull)							
				{
					$hostname = find_hotstname($mac, $LAN1);		
					if($hostname == "") $hostname = find_hotstname($mac, $LAN2);
					if($hostname == "") $hostname = "N/A";
					
					if(check_wireless_band($mac,$WLAN1)==true || check_wireless_band($mac,$WLAN1_GZ)==true) 				$type = "WiFi_2.4G";
					else if(check_wireless_band($mac,$WLAN2)==true || check_wireless_band($mac,$WLAN2_GZ)==true) 	$type = "WiFi_5G";
					else 																																											 				$type = "LAN";
	
					echo "<ClientInfo>";
					echo 		"<MacAddress>".$mac."</MacAddress>";
					echo 		"<IPv4Address>".$ip."</IPv4Address>";
					echo 		"<IPv6Address>".""."</IPv6Address>"; //todo, Sammy
					echo 		"<Type>".$type."</Type>";
					echo 		"<DeviceName>".escape("x",$hostname)."</DeviceName>";
					echo "</ClientInfo>";
	
					$cnt++;
						
				}	
			}
			$tablelen = strlen($line);
			$line     = substr($line, $tailindex, $tablelen-$tailindex);			
		}
	}
	echo "</ClientInfoLists>";

}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
<soap:Body>
<GetClientInfoResponse xmlns="http://purenetworks.com/HNAP1/">
	<GetClientInfoResult><?=$result?></GetClientInfoResult>
	<?print_ClientInfoLists();?>
</GetClientInfoResponse>
</soap:Body>
</soap:Envelope>
