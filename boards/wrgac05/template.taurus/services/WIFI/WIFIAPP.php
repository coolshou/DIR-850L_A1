<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

function wifiapp_error($errno)
{
	fwrite("a", $_GLOBALS["START"], "exit ".$errno."\n");
	fwrite("a", $_GLOBALS["STOP"],  "exit ".$errno."\n");
}


function wifiapp_service()
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND24G-1.1", 0);
	$phy1		= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy1."/wifi"), 0);

	/* Is the phyinf active? */
	$active1 = query($phy1."/active");
	if($active1!=1) return;

	$winfname	= query($stsp."/name");
	$ssid = query($wifi1."/ssid");
	$authtype = query($wifi1."/authtype");
	$encrtype = query($wifi1."/encrtype");
	$psk	= query($wifi1."/nwkey/psk/key");
	$wep_defkey = query($wifi1."/nwkey/wep/defkey");
	$ascii = query($wifi1."/nwkey/wep/ascii");
	$wep_key_1 = query($wifi1."/nwkey/wep/key:1");
	$wep_key_2 = query($wifi1."/nwkey/wep/key:2");
	$wep_key_3 = query($wifi1."/nwkey/wep/key:3");
	$wep_key_4 = query($wifi1."/nwkey/wep/key:4");
	if($ascii==1)
	{
		$wep_key_1 = ascii($wep_key_1);
		$wep_key_2 = ascii($wep_key_2);
		$wep_key_3 = ascii($wep_key_3);
		$wep_key_4 = ascii($wep_key_4);
	}
	// WPS config START
	$wscd_conf = "/var/wsc.conf";
	$wps_en = query($wifi1."/wps/enable");
	$wps_configured = query($wifi1."/wps/configured");
	$wps_pin = query($wifi1."/wps/pin");
	if($wps_pin == "") $wps_pin = query("/runtime/devdata/pin"); /* Factory default PIN. (label) */
	$dtype = "urn:schemas-wifialliance-org:device:WFADevice:1";
	$dpath = XNODE_getpathbytarget("/runtime/upnp", "dev", "deviceType", $dtype, 0);
	if ($dpath != "")
	{
		$UUID_tmp = query($dpath."/guid");
		$UUID = cut($UUID_tmp,"0","-").cut($UUID_tmp,"1","-").cut($UUID_tmp,"2","-").cut($UUID_tmp,"3","-").cut($UUID_tmp,"4","-");
		$uuid = tolower($UUID);
		$modelname = query($dpath."/devdesc/device/modelName");
		$modelnum = query($dpath."/devdesc/device/modelNumber");
		$serialnum = query($dpath."/devdesc/device/serialNumber");
		$vendor = query($dpath."/devdesc/device/manufacturer");
		$vendorurl = query($dpath."/devdesc/device/manufacturerURL");
		$modeldesc = query($dpath."/devdesc/device/modelDescription");
	}
	// WPS config END
	$iapp = query($phy1."/media/iapp");

	$phy2		= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.2", 0); 
	$phy3		= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.3", 0);
	$phy4		= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.4", 0);
	$phy5		= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.5", 0);
	$mssid1active = query($phy2."/active");
	$mssid2active = query($phy3."/active");
	$mssid3active = query($phy4."/active");
	$mssid4active = query($phy5."/active");

	if($iapp==1){
		$infstr="";
		if($mssid1active==1) {
			$infstr=$infstr."wlan0-va0 ";
		}
		if($mssid2active==1) {
			$infstr=$infstr."wlan0-va1 ";
		}
		if($mssid3active==1) {
		    $infstr=$infstr."wlan0-va2 ";
		}
		if($mssid4active==1) {
		    $infstr=$infstr."wlan0-va3 ";
		}
	}

//-----------------------------IAPP & iwcontrol setting------------------------------------------------------------------//			
	fwrite("a", $_GLOBALS["START"], 'xmldbc -k "WSCWAITUP"\n');
	fwrite("a", $_GLOBALS["START"], 'shell="/var/run/wscwaitup.sh"\n');
	fwrite("a", $_GLOBALS["START"], 'rm -f $shell\n');
	fwrite("a", $_GLOBALS["START"], 'killall iwcontrol\n');
	fwrite("a", $_GLOBALS["START"], 'rm -f /var/run/iwcontrol.pid\n');
	fwrite("a", $_GLOBALS["START"], 'killall wscd\n');
	fwrite("a", $_GLOBALS["START"], 'rm -f /var/run/wscd-wlan0.pid\n');
	fwrite("a", $_GLOBALS["START"], 'killall iapp\n');
	fwrite("a", $_GLOBALS["START"], 'rm -f /var/wscd-'.$winfname.'.fifo\n');

	if($iapp==1 && $wps_en!=1){
		fwrite("a", $_GLOBALS["START"], 'iapp br0 '.$winfname.' '.$infstr.'\n');
	}			
//--------------------------WSC/WPS setting------------------------------------------------------------------------------//
	if($wps_en==1){
		// WSCD config START //
		if($wps_configured == 1){
			fwrite("w", $wscd_conf, 'mode = 5\n');	//MODE_AP_PROXY_REGISTRAR//a
			if($authtype=="OPEN"){
				fwrite("a", $wscd_conf, 'auth_type = 1\n');	//AUTH_OPEN=1
			} else if($authtype=="SHARED"){
				fwrite("a", $wscd_conf, 'auth_type = 4\n');	//AUTH_SHARED=4
			} else if($authtype=="WPAPSK"){
				fwrite("a", $wscd_conf, 'auth_type = 2\n');	//AUTH_WPAPSK=2
			} else if($authtype=="WPA2PSK"){
				fwrite("a", $wscd_conf, 'auth_type = 32\n');	//AUTH_WPA2PSK=0x20
//			} else if($authtype=="WPA+2PSK"){
			} else{	//WPA+2PSK
				fwrite("a", $wscd_conf, 'auth_type = 34\n');
			}
			if($encrtype=="NONE"){
				fwrite("a", $wscd_conf, 'encrypt_type = 1\n');	//ENCRYPT_NONE=1
			} else if($encrtype=="WEP"){
				fwrite("a", $wscd_conf, 'encrypt_type = 2\n');	//ENCRYPT_WEP=2
			} else if($encrtype=="TKIP"){
				fwrite("a", $wscd_conf, 'encrypt_type = 4\n');	//ENCRYPT_TKIP=4
			} else if($encrtype=="AES"){
				fwrite("a", $wscd_conf, 'encrypt_type = 8\n');	//ENCRYPT_AES=8
//			} else if($encrtype=="TKIP+AES"){
			} else{	//TKIP+AES
				fwrite("a", $wscd_conf, 'encrypt_type = 12\n');
			}
			fwrite("a", $wscd_conf, 'manual_config = 0\n');
			if($encrtype=="WEP"){
				fwrite("a", $wscd_conf, 'wep_transmit_key = '.$wep_defkey.'\n');
				fwrite("a", $wscd_conf, 'network_key = '.$wep_key_1.'\n');
				fwrite("a", $wscd_conf, 'wep_key2 = '.$wep_key_2.'\n');
				fwrite("a", $wscd_conf, 'wep_key3 = '.$wep_key_3.'\n');
				fwrite("a", $wscd_conf, 'wep_key4 = '.$wep_key_4.'\n');
//			} else if($authtype=="WPAPSK" || $authtype=="WPA2PSK" || $authtype=="WPA+2PSK"){
			} else{	
				fwrite("a", $wscd_conf, 'network_key = '.$psk.'\n');
			}
			fwrite("a", $wscd_conf, 'ssid = '.$ssid.'\n');
		} else{
			fwrite("w", $wscd_conf, 'mode = 1\n');	//MODE_AP_UNCONFIG//
			fwrite("a", $wscd_conf, 'auth_type = 32\n');	//AUTH_WPAPSK=2 + AUTH_WPA2PSK=0x20
			fwrite("a", $wscd_conf, 'encrypt_type = 8\n');	//ENCRYPT_TKIP=4 + ENCRYPT_AES=8
			fwrite("a", $wscd_conf, 'manual_config = 0\n');
			if($ssid==""){
				$ssid1= query("/runtime/devdata/lanmac");
				$defssid = "";
				$defssid = $defssid.cut($ssid1, "3", ":");
				$defssid = $defssid.cut($ssid1, "4", ":");
				$defssid = $defssid.cut($ssid1, "5", ":");
				fwrite("a", $wscd_conf, 'ssid = ap-pc-'.$defssid.'\n');
			} else{
				fwrite("a", $wscd_conf, 'ssid = '.$ssid.'\n');
			}
		}
		fwrite("a", $wscd_conf, 'upnp = 1\n');
		fwrite("a", $wscd_conf, 'config_method = 134\n');
		fwrite("a", $wscd_conf, 'connection_type = 1\n');
		fwrite("a", $wscd_conf, 'pin_code = '.$wps_pin.'\n');
		fwrite("a", $wscd_conf, 'rf_band = 1\n');
		fwrite("a", $wscd_conf, 'config_by_ext_reg = 1\n');
		fwrite("a", $wscd_conf, 'device_name = "'.$modelname.'"\n');
		fwrite("a", $wscd_conf, 'use_ie = 1\n');
		fwrite("a", $wscd_conf, 'auth_type_flags = 39\n');	//AUTH_OPEN=1, AUTH_WPAPSK=2, AUTH_SHARED=4, AUTH_WPA2PSK=0x20 //
		fwrite("a", $wscd_conf, 'encrypt_type_flags = 15\n');	//ENCRYPT_NONE=1, ENCRYPT_WEP=2, ENCRYPT_TKIP=4, ENCRYPT_AES=8 //
		fwrite("a", $wscd_conf, 'uuid = '.$uuid.'\n');
		fwrite("a", $wscd_conf, 'device_name = "'.$modelname.'"\n');
		fwrite("a", $wscd_conf, 'manufacturer = "'.$vendor.'"\n');
		fwrite("a", $wscd_conf, 'manufacturerURL = "'.$vendorurl.'"\n');
		fwrite("a", $wscd_conf, 'modelURL = "'.$vendorurl.'"\n');
		fwrite("a", $wscd_conf, 'model_name = "'.$modelname.'"\n');
		fwrite("a", $wscd_conf, 'model_num = "'.$modelnum.'"\n');
		fwrite("a", $wscd_conf, 'serial_num = "'.$serialnum.'"\n');
		fwrite("a", $wscd_conf, 'modelDescription = "'.$modeldesc.'"\n');
		fwrite("a", $wscd_conf, 'device_attrib_id = 1\n');
		fwrite("a", $wscd_conf, 'device_oui = 0050f204\n');
		fwrite("a", $wscd_conf, 'device_category_id = 6\n');
		fwrite("a", $wscd_conf, 'device_sub_category_id = 1\n');
		fwrite("a", $wscd_conf, 'device_password_id = 0\n');
		fwrite("a", $wscd_conf, 'tx_timeout = 5\n');
		fwrite("a", $wscd_conf, 'resent_limit = 2\n');
		fwrite("a", $wscd_conf, 'reg_timeout = 120\n');
		fwrite("a", $wscd_conf, 'block_timeout = 60\n');
		fwrite("a", $wscd_conf, 'WPS_START_LED_GPIO_number = 2\n');
		fwrite("a", $wscd_conf, 'WPS_END_LED_unconfig_GPIO_number = 0\n');
		fwrite("a", $wscd_conf, 'WPS_END_LED_config_GPIO_number = 0\n');
		fwrite("a", $wscd_conf, 'WPS_PBC_overlapping_GPIO_number = 1\n');
		fwrite("a", $wscd_conf, 'PBC_overlapping_LED_time_out = 30\n');
		fwrite("a", $wscd_conf, 'No_ifname_for_flash_set = 2\n');
		fwrite("a", $wscd_conf, 'disable_auto_gen_ssid = 1\n');
		fwrite("a", $wscd_conf, 'disable_hidden_ap = 1\n');
		fwrite("a", $wscd_conf, 'button_hold_time = 3\n');
		fwrite("a", $wscd_conf, 'fix_wzc_wep = 1\n');
		fwrite("a", $wscd_conf, 'WPS_SUCCESS_LED_time_out = 300\n');
		// WSCD config END //
		fwrite("a", $_GLOBALS["START"], 'mkdir /var/wps\n');
		fwrite("a", $_GLOBALS["START"], 'cp /etc/simplecfg*.xml /var/wps/\n');
//		fwrite("a", $_GLOBALS["START"], 'flash upd-wsc-conf /etc/wscd.conf /var/wsc.conf\n');
		fwrite("a", $_GLOBALS["START"], 'shell="/var/run/wscwaitup.sh"\n');
		fwrite("a", $_GLOBALS["START"], 'echo "#!/bin/sh"                    >  $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "if [ -f "/var/run/BRIDGE-1.UP" ] || [ -f "/var/run/LAN-1.UP" ]; then"	>>	$shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\tsleep 1"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\troute del -net 239.255.255.250 netmask 255.255.255.255 br0"	>> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\troute add -net 239.255.255.250 netmask 255.255.255.255 br0"	>> $shell\n');
		if($iapp==1) {fwrite("a", $_GLOBALS["START"], 'echo "\tiapp br0 '.$winfname.' '.$infstr.'"	>> $shell\n');}			
		fwrite("a", $_GLOBALS["START"], 'echo "\twscd -start -c /var/wsc.conf -w '.$winfname.' -fi /var/wscd-'.$winfname.'.fifo -daemon" >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\tsleep 1"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\tiwcontrol '.$winfname.' '.$infstr.'"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\trm -f $shell"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "else"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\tsleep 1"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "\txmldbc -t WSCWAITUP:1:\'sh $shell\' > /dev/console"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'echo "fi"      >> $shell\n');
		fwrite("a", $_GLOBALS["START"], 'xmldbc -t "WSCWAITUP:1:sh $shell" > /dev/console\n');
//		fwrite("a", $_GLOBALS["START"], 'wscd -start -c /var/wsc.conf -w '.$winfname.' -fi /var/wscd-'.$winfname.'.fifo -daemon\n');
		fwrite("a", $_GLOBALS["START"], 'event WPSPIN add "/etc/scripts/wps_tmp.sh pin"\n');
		fwrite("a", $_GLOBALS["START"], 'event WPSPBC.PUSH add "/etc/scripts/wps_tmp.sh pbc"\n');

		fwrite("a", $_GLOBALS["STOP"], 'event WPSPBC.PUSH add true\n');
		fwrite("a", $_GLOBALS["STOP"], 'event WPSPIN add true\n');
		fwrite("a", $_GLOBALS["STOP"], 'xmldbc -k "WSCWAITUP"\n');
		fwrite("a", $_GLOBALS["STOP"], 'shell="/var/run/wscwaitup.sh"\n');
		fwrite("a", $_GLOBALS["STOP"], 'rm -f $shell\n');
		fwrite("a", $_GLOBALS["STOP"], 'killall wscd\n');
		fwrite("a", $_GLOBALS["STOP"], 'rm -f /var/run/wscd-wlan0.pid\n');
		if($iapp==1) {fwrite("a", $_GLOBALS["STOP"], 'killall iapp\n');}
		fwrite("a", $_GLOBALS["STOP"], 'killall iwcontrol\n');
		fwrite("a", $_GLOBALS["STOP"], 'rm -f /var/run/iwcontrol.pid\n');
	}
	if($iapp==1 && $wps_en!=1){
		fwrite("a", $_GLOBALS["START"], 'iwcontrol '.$winfname.' '.$infstr.'\n');
		fwrite("a", $_GLOBALS["STOP"], 'killall iwcontrol\n');
		fwrite("a", $_GLOBALS["STOP"], 'rm -f /var/run/iwcontrol.pid\n');
		fwrite("a", $_GLOBALS["STOP"], 'rm -f /var/wscd-'.$winfname.'.fifo\n');
	}
}

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

/* Get the phyinf */
$phy1	= XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);	if ($phy1 == "")	return;

/* Is the phyinf active? */
$active1 = query($phy1."/active");

if ($active1!=1){
	wifiapp_error("8"); return;
}
if ($active1==1)	{
	wifiapp_service();
}

fwrite("a",$START, "exit 0\n");
fwrite("a",$STOP,  "exit 0\n");
?>
