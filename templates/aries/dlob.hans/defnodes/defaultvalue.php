<?
/*rework wifi to
1.ssid dlink+mac
2.password and wpaauto psk.
*/
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/webinc/config.php";

setattr("/runtime/devdata/wifiverify" ,"get","devdata get -e wifiverify");
setattr("/runtime/devdata/ipv6logo" ,"get","devdata get -e ipv6logo");

setattr("/runtime/tmpdevdata/wifipassword" ,"get","devdata get -e psk"); 
setattr("/runtime/tmpdevdata/ssid_2G" ,"get","devdata get -e wifissid_2g"); 
setattr("/runtime/tmpdevdata/ssid_5G" ,"get","devdata get -e wifissid_5g"); 
setattr("/runtime/tmpdevdata/wlanmac" ,"get","devdata get -e wlan24mac");
setattr("/runtime/tmpdevdata/wlanmac_sb" ,"get","devdata get -e wlanmac");
setattr("/runtime/tmpdevdata/wlanmac2" ,"get","devdata get -e wlan5mac"); 
setattr("/runtime/tmpdevdata/lanmac" ,"get","devdata get -e lanmac"); 

function changes_default_wifi($phyinfuid,$ssid,$password,$mac)
{
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $phyinfuid, 0);
	$wifi = XNODE_getpathbytarget("/wifi", "entry", "uid", query($p."/wifi"), 0);
	if($p=="" || $wifi=="")
	{
		return;	
	}
	anchor($wifi); 
	
	if($ssid=="")
	{
		$n5 = cut($mac, 4, ":");
		$n6 = cut($mac, 5, ":");
		$ssidsuffix = $n5.$n6;
		$ssid		= query("ssid");
		if($ssidsuffix!="")
		{		
			//For dlink product, make the default SSID "dlink-mac" for 2.4GHz and "dlink-mac-media" for 5GHz.
			if(substr($ssid, 0, 5) == "dlink")
			{
				$ssid1	= "dlink";
				$ssid2	= scut($ssid, 0, "dlink");
				$ssid	= $ssid1."-".toupper($ssidsuffix).$ssid2;
			}
		}
	}
	//TRACE_error("ssid=".$ssid."=password=".$password."");
	if($password!="" && $ssid!="")
	{
		//chanhe the mode to wpa-auto psk
		set("authtype","WPA+2PSK");
		set("encrtype","TKIP+AES");
		set("wps/configured","1");
		set("ssid",$ssid);
		set("nwkey/psk/passphrase","1");
		set("nwkey/psk/key",$password);
		set("nwkey/wpa/groupintv","3600");
		set("nwkey/rekey/gtk","1800");
	}
	else
	{
		TRACE_error("the mfc do not init wifi password,using default");
	}
}

$password = query("/runtime/tmpdevdata/wifipassword");
$wifiverify = query("/runtime/devdata/wifiverify");
if($wifiverify=="1") $password = "";

$wlanmac = query("/runtime/tmpdevdata/wlanmac");
if ($wlanmac == "")
	$wlanmac = query("/runtime/tmpdevdata/wlanmac_sb");
$ssid = query("/runtime/tmpdevdata/ssid_2G");
changes_default_wifi($WLAN1,$ssid,$password,$wlanmac);
$wlanmac = query("/runtime/tmpdevdata/wlanmac2");
$ssid = query("/runtime/tmpdevdata/ssid_5G");
changes_default_wifi($WLAN2,$ssid,$password,$wlanmac);
/*remove links*/
del("/runtime/tmpdevdata");
?>

