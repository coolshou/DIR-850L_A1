<?
function get_auth_string($value)
{
	if($value == 0) return "OPEN";
	else if($value == 1) return "WEP";
	else if($value == 2) return "WPA";
	else if($value == 4) return "WPA2";
	else if($value == 6) return "WPA/WPA2";
	
	return "";
}

function get_wep_auth_string($value)
{
	if($value == 1) return "OPEN";
	else if($value == 2) return "SHARED";
	else {return "WEPAUTO";}
	
	return "";
}

function get_wpa_type_string($value)
{
	if($value == 1) return "EAP";
	else if($value == 2) return "PSK";
	
	return "";
}

function get_wpa_cipher_string($value)
{
	if($value == 1) return "TKIP";
	else if($value == 2) return "AES";
	else if($value == 3) return "auto";
	
	return "";
}

function get_wep_size($wep, $isascii)
{
	if($value == 1) return "TKIP";
	else if($value == 2) return "AES";
	else if($value == 3) return "auto";
	
	return 64;
}

function check_key_type_and_valid($key_type, $key)  //0: digital 1:ascii 2:invalid
{
	$ret = 2;  //invalid
	if($key != "")
	{
		if($key_type == "WEP")
		{
			if(strlen($key) == 10 || strlen($key) == 26)   //digital
			{
				if(isxdigit($key) == 1)
					$ret = 0;  //digital
				else
					$ret = 2;  //invalid
			}
			else if (strlen($key) == 5 || strlen($key) == 13)  //ascii
			{
				$ret = 1;
			}
		}
		else  //wpa
		{   //wpa range 64 for hex   8~63 for ascii
			
			if(strlen($key) == 64)
			{
				if(isxdigit($key) == 1)
					$ret = 0;  //hex
			}
			else if(strlen($key) > 7 && strlen($key) < 64)
			{
				$ret = 1;  //legal
			}
		}
	}

	return $ret;
}

function check_wep_len($wep_len, $wep, $ascii_or_valid)
{
	$size = 0;
	$len=strlen($wep);
	
	if($wep_len != "" && $ascii_or_valid < 2)
	{
		if($ascii_or_valid == 0)
		{  //digit
			if($len == 10)
				$size = 64;
			else if($len == 26)
				$size = 128;
		}
		else
		{  //ascii
			if($len == 5)
				$size = 64;
			else if($len == 13)
				$size = 128;
		} 
	}
	return $size;
}

$phy  = XNODE_getpathbytarget("", "phyinf", "uid", $WLAN, 0);
$wifi = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy."/wifi"), 0);

$settingsChanged = $_POST["settingsChanged"];
$enable 		 = $_POST["f_enable"];
$channel 		 = $_POST["f_channel"];
$auto_channel 	 = $_POST["f_auto_channel"];
$ap_hidden 		 = $_POST["f_ap_hidden"];
$ssid 			 = $_POST["f_ssid"];
$authentication  = $_POST["f_authentication"];
$wep_auth_type 	 = $_POST["f_wep_auth_type"];
$cipher 		 = $_POST["f_cipher"];
$wep_len 		 = $_POST["f_wep_len"];
$wep_format 	 = $_POST["f_wep_format"];
$wep_def_key 	 = $_POST["f_wep_def_key"];
$wep 			 = $_POST["f_wep"];
$wpa_psk  		 = $_POST["f_wpa_psk"];
$wpa_psk_type 	 = $_POST["f_wpa_psk_type"];
$radius_ip1 	 = $_POST["f_radius_ip1"];
$radius_port1 	 = $_POST["f_radius_port1"];
$radius_secret1  = $_POST["f_radius_secret1"];

$ret="ok";
$set_configured=0;
$wps_disable=0;
if($settingsChanged == 1)
{

	if($wep_def_key == 0) {$wep_def_key=1;}
	$now_auth = query($wifi."/authtype");
	$now_encrypt = query($wifi."/encrtype");
	$new_auth = get_auth_string($authentication);
	$new_wep_auth = get_wep_auth_string($wep_auth_type);
	$new_wpa_type = get_wpa_type_string($wpa_psk_type);
	$new_wpa_cipher = get_wpa_cipher_string($cipher);
	
	
	if($new_auth != "WEP" && $new_wpa_type != "PSK")
	{
		//Wireless Enable State
		if($enable != "") {set($phy."/active", $enable);}
		
		//Wireless Channel
		if($auto_channel == 1)  {set($phy."/media/channel", 0);}
		else if($channel != "")	{set($phy."/media/channel", $channel);}
		
		//Wireless Visibility Status
		if($ap_hidden != "")    {set($wifi."/ssidhidden", $ap_hidden);}
		
		//Wireless SSID
		if($ssid != "")
		{
			$now_ssid = query($wifi."/ssid");
			if($now_ssid != $ssid)
			{
				set($wifi."/ssid", $ssid);
				$set_configured = 1;
			}
		}
	}
	
	
	//Wireless Security
	if($new_auth != "")
	{
		if($new_auth == "OPEN") 
		{
			set($wifi."/authtype", "OPEN");
			set($wifi."/encrtype", "NONE");
			if($now_auth != "OPEN") {$set_configured = 1;}
		}
		else if($new_auth == "WEP")
		{
			$ascii_or_valid = check_key_type_and_valid($new_auth, $wep);
			$size = check_wep_len($wep_len, $wep, $ascii_or_valid);	
			if($ascii_or_valid < 2 && $size > 0)
			{
				$wps_disable = 1;
				$now_wepdef = query($wifi."/nwkey/wep/defkey");
				$now_wepkey = query($wifi."/nwkey/wep/key:".$now_wepdef);
		
				if($new_wep_auth != "")
				{
					set($wifi."/authtype", $new_wep_auth);
					if($new_wep_auth != $now_auth) {$set_configured = 1;}
				}
					
				set($wifi."/encrtype", "WEP");
				if($wep_def_key != "") {set($wifi."/nwkey/wep/defkey",$wep_def_key);}
				if($wep_format != "")
				{
					set($wifi."/nwkey/wep/ascii",$ascii_or_valid);
					//if($wep_format == 1) 	  {set($wifi."/nwkey/wep/ascii",1);}
					//else if($wep_format == 2) {set($wifi."/nwkey/wep/ascii",0);}
				}
				
				/*
					if($wep_len != "")
					{
						if($wep_len == 0)
							$size=64;
						else if(wep_len == 1)
							$size=128;
					}
				*/
				set($wifi."/nwkey/wep/size",$size);

				if($wep_def_key != "" && $wep != "") {set($wifi."/nwkey/wep/key:".$wep_def_key,$wep);}
				if($now_encrypt != "WEP") {$set_configured = 1;}
				if($wep_def_key != $now_wepdef) {$set_configured = 1;}
				if($now_wepkey != $wep) {$set_configured = 1;}
			}else
				$ret = "fail";
		}
		else //WPA/WPA2
		{
			if($new_wpa_type != "")
			{
				if($new_wpa_type == "PSK")
				{
					$wpa_type = "PSK";
					$ascii_or_valid = check_key_type_and_valid($new_wpa_type, $wpa_psk);
					if($ascii_or_valid < 2)
					{
						$now_pskkey = query($wifi."/nwkey/psk/key");
						if($wpa_psk != "") {set($wifi."/nwkey/psk/key", $wpa_psk);}
						if($wpa_psk != $now_pskkey) {$set_configured = 1;}
						if(isxdigit($wpa_psk) == 1 && strlen($wpa_psk)==64){set($wifi."/nwkey/psk/passphrase", 0);}
						else {set($wifi."/nwkey/psk/passphrase", 1);}  //ascii
					}else
						$ret = "fail";
				}
				else if($new_wpa_type == "EAP")
				{
					$wps_disable = 1;
					$wpa_type = "";
					if($radius_ip1 != "") 		{set($wifi."/nwkey/eap/radius", $radius_ip1);}
					if($radius_port1 != "") 	{set($wifi."/nwkey/eap/port", $radius_port1);}
					if($radius_secret1 != "")	{set($wifi."/nwkey/eap/secret", $radius_secret1);}
				}
				
				if($new_auth == "WPA")
				{
					$wpa_auth = "WPA".$wpa_type;
					if($ascii_or_valid < 2)
					{
						$wps_disable = 1;
						set($wifi."/authtype", $wpa_auth);
					}
				}
				else if($new_auth == "WPA2")
				{
					$wpa_auth = "WPA2".$wpa_type;
					if($ascii_or_valid < 2)
						set($wifi."/authtype", $wpa_auth);
				}
				else if($new_auth == "WPA/WPA2")
				{
					$wpa_auth = "WPA+2".$wpa_type;
					if($ascii_or_valid < 2)
						set($wifi."/authtype", $wpa_auth);
				}
				if($now_auth != $wpa_auth) {$set_configured = 1;}
			}
			
			if($new_wpa_cipher != "")
			{
				if($new_wpa_type != "PSK" || $ascii_or_valid < 2)
				{
					if($new_wpa_cipher == "auto") 
					{
						set($wifi."/encrtype", "TKIP+AES");
						if($now_encrypt != "TKIP+AES") {$set_configured = 1;}
					}
					else 
					{
						if($new_wpa_cipher == "TKIP") {$wps_disable = 1;}
						set($wifi."/encrtype", $new_wpa_cipher);
						if($new_wpa_cipher != $now_encrypt) {$set_configured = 1;}
					}
				}
			}
		}
	}
	
	if($new_auth == "WEP" || $new_wpa_type == "PSK")
	{
		if($ascii_or_valid < 2)
		{
			//Wireless Enable State
			if($enable != "") {set($phy."/active", $enable);}
			
			//Wireless Channel
			if($auto_channel == 1)  {set($phy."/media/channel", 0);}
			else if($channel != "")	{set($phy."/media/channel", $channel);}
			
			//Wireless Visibility Status
			if($ap_hidden != "")    {set($wifi."/ssidhidden", $ap_hidden);}
			
			//Wireless SSID
			if($ssid != "")
			{
				$now_ssid = query($wifi."/ssid");
				if($now_ssid != $ssid)
				{
					set($wifi."/ssid", $ssid);
					$set_configured = 1;
				}
			}
		}
	}
	
	if($wps_disable == 1) {set($wifi."/wps/enable", 0);}
	if($set_configured == 1) {set($wifi."/wps/configured", 1);}
	//$ret="ok";
}
?>
<?=$ret?>