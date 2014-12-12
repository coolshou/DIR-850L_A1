# Auto generated topology file by HOSTAPD service
<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";
include "/etc/services/WIFI/function.php";

$UID="BAND24G";
$UID5G="BAND5G";
/********************************************************************/
function phyinf_check($phyinfuid)
{
	if($phyinfuid == "") return 0;
	$phy = XNODE_getpathbytarget("", "phyinf", "uid", $phyinfuid, 0);
	$run_phy = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinfuid, 0);
	$wifi = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy."/wifi"), 0);
	if ($phy == "" || $run_phy == "") return 0;
	if (query($wifi."/opmode") != "AP") return 0;
	$active = query($phy."/active");
	if ($active != 1) return 0;

	return 1;
}
function is_upnp_enabled($phyinf)
{
	foreach ("/runtime/inf")
	{
		$cnt = 0;
		if (query("phyinf")==$phyinf) $cnt = INF_getinfinfo(query("uid"), "upnp/count");
		if ($cnt > 0) return 1;
	}
	return 0;
}
function find_bridge($phyinf)
{
	foreach ("/runtime/phyinf")
	{
		if (query("type")!="eth") continue;
		foreach ("bridge/port") if ($VaLuE==$phyinf) {$find = "yes"; break;}
		if ($find=="yes") return query("uid");
	}
	return "";
}
function is_dual_band()
{
	$p1 = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.1", 0);
	$wifi1 = XNODE_getpathbytarget("/wifi", "entry", "uid", query($p1."/wifi"), 0);
	$p2 = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID5G"]."-1.1", 0);
	$wifi2 = XNODE_getpathbytarget("/wifi", "entry", "uid", query($p2."/wifi"), 0);
	$BAND24G_enable = query($p1."/active");
	$BAND5G_enable = query($p2."/active");
	$BAND24G_wps_enable = query($wifi1."/wps/enable");
	$BAND5G_wps_enable = query($wifi2."/wps/enable");
	if($BAND24G_enable=="1" && $BAND5G_enable=="1" && $BAND24G_wps_enable=="1" && $BAND5G_wps_enable=="1")
		return 1;
	else
		return 0;
}

/********************************************************************/

function generate_configs($phyinfuid, $output)
{
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $phyinfuid, 0);
	$wifi = XNODE_getpathbytarget("/wifi", "entry", "uid", query($p."/wifi"), 0);
	anchor($wifi);

	/* Find the bridge & device names */
	$bruid = find_bridge($phyinfuid);
	if ($bruid!="") $brdev = PHYINF_getifname($bruid);
	$dev = devname($phyinfuid);

	$authtype	= query("authtype");
	$encrtype	= query("encrtype");
	$ssid		= query("ssid");
	$wps		= query("wps/enable");
	$wps_conf	= query("wps/configured");	if ($wps_conf == "")	$wps_conf = 0;
	$wps_aplocked = query("/runtime/wps/setting/aplocked");	if ($wps_aplocked == "") {$wps_aplocked = "0";}
	$wps_pin	= query("wps/pin");			if ($wps_pin == "")		$wps_pin = query("/runtime/devdata/pin");
	//$rekeyint	= query("nwkey/wpa/groupintv");
	$rekey_gtk_interval = query("nwkey/wpa/groupintv"); if ($rekey_gtk_interval == "") {$rekey_gtk_interval = 3600;}
	//wmm primary and secondary use same setting
	if(strstr($phyinfuid,"5G") != "")
		$prip	= XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID5G"]."-1.1", 0);
	else
		$prip	= XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.1", 0);
		
	$wmm		= query($prip."/media/wmm/enable");

	/* for wfa device */
	$vendor		= query("/runtime/device/vendor");
	$model      = query("/runtime/device/modelname");
	$producturl = query("/runtime/device/producturl");
	//$upnpp		= XNODE_getpathbytarget("/runtime/upnp", "dev", "deviceType",
	//				"urn:schemas-wifialliance-org:device:WFADevice:1", 0);
	//$uuid		= query($upnpp."/guid");
	$uuid       = query("/runtime/hostapd/guid");

	$wsc2_version=query("wps/wsc2_version");//marco
	/* EAP user file. */
	$eapuserfile = "/var/run/hostapd-".$dev."wps.eap_user";
	fwrite('w', $eapuserfile,
		'"WFA-SimpleConfig-Registrar-1-0" WPS\n'.
		'"WFA-SimpleConfig-Enrollee-1-0" WPS\n'
		);

	/* Generate config file */
	if		($authtype=="OPEN")		{ $wpa=0;	$ieee8021x=0; }
	else if ($authtype=="SHARED")	{ $wpa=0;	$ieee8021x=0; }
	else if ($authtype=="WEPAUTO")	{ $wpa=0;	$ieee8021x=0; }
	else if ($authtype=="WPA")		{ $wpa=1;	$ieee8021x=1; }
	else if ($authtype=="WPAPSK")	{ $wpa=1;	$ieee8021x=0; }
	else if ($authtype=="WPA2")		{ $wpa=2;	$ieee8021x=1; }
	else if ($authtype=="WPA2PSK")	{ $wpa=2;	$ieee8021x=0; }
	else if ($authtype=="WPA+2")	{ $wpa=3;	$ieee8021x=1; }
	else if ($authtype=="WPA+2PSK")	{ $wpa=3;	$ieee8021x=0; }

	/* generate the config file for hostapd */
	fwrite("w", $output, "");
	fwrite("a", $output,
		'eapol_key_index_workaround=0\n'.
		'logger_syslog=0\nlogger_syslog_level=0\nlogger_stdout=0\nlogger_stdout_level=0\ndebug=0\n'.
		'interface='.$dev.'\n'.
		'bridge='.$brdev.'\n'
		);

	fwrite("a", $output,
		'ssid='.$ssid.'\n'.
		'wpa='.$wpa.'\n'.
		'ieee8021x='.$ieee8021x.'\n'
		);

	fwrite("a", $output,
		'wps_uuid='.$uuid.'\n'.
		'dump_file=/tmp/hostapd.dump\n'.
		'ctrl_interface=/var/run/hostapd\n'.
		'ctrl_interface_group=0\n'.
		'auth_algs=1\n'.
		'wps_auth_type_flags=0x003f\n'.
		'wps_encr_type_flags=0x000f\n'
		);

	if ($ieee8021x == 0)				fwrite("a", $output, 'eap_server=1\n');
	else								fwrite("a", $output, 'eap_server=0\n');
	if ($wps==1 && $ieee8021x==0)	//sam_pan add	
	{		
		fwrite("a", $output, 'wps_disable=0\n');
		fwrite("a", $output, 'wps_upnp_disable=0\n');
	}	
	else								
	{
		fwrite("a", $output, 'wps_disable=1\n');
		fwrite("a", $output, 'wps_upnp_disable=1\n');
	}		
	/* sam_pan skip, I will ask bouble_hung if remove below code.
	if ($wps==1 && $ieee8021x==0)		fwrite("a", $output, 'wps_disable=0\n');
	else								fwrite("a", $output, 'wps_disable=1\n');
	if (is_upnp_enabled($bruid)=="1")	fwrite("a", $output, 'wps_upnp_disable=0\n');
	else								fwrite("a", $output, 'wps_upnp_disable=1\n');
	*/
	
	if($wsc2_version!="")
	{
		fwrite("a", $output,
			'wps_version=0x10\n'.
			'wps_default_pin='.$wps_pin.'\n'.
			'wps_configured='.$wps_conf.'\n'.
			'ap_setup_locked='.$wps_aplocked.'\n'.
			'wps_conn_type_flags=0x01\n'.
			'wps_config_methods=0x278c\n'
			);
	}
	else
	{
		fwrite("a", $output,
			'wps_version=0x10\n'.
			'wps_default_pin='.$wps_pin.'\n'.
			'wps_configured='.$wps_conf.'\n'.
			'ap_setup_locked='.$wps_aplocked.'\n'.
			'wps_conn_type_flags=0x01\n'.
			'wps_config_methods=0x0086\n'
			);
	}	
	$dual_band = is_dual_band();
	if($dual_band==1){
		fwrite("a",$output,'dual_band=1\n');
	}
	if($phyinfuid==$_GLOBALS["UID"]."-1.1")
	{
		fwrite("a",$output,'wps_rf_bands=0x01\n');
	}
	else if($phyinfuid==$_GLOBALS["UID5G"]."-1.1")
	{
		fwrite("a",$output,'wps_rf_bands=0x02\n');
	}
	/*fwrite("a", $output,
		'wps_manufacturer='.$vendor.'\n'.
		'wps_manufacturer_url='.$producturl.'\n'.
		'wps_serial_number=00000000\n'.
		'wps_model_number=00000000\n'.
		'wps_model_name='.$model.'\n'.
		'wps_model_description='.$vendor.' '.$model.' Wireless Broadband Router\n'.
		'wps_friendly_name='.$model.'\n'
		);*/
		fwrite("a", $output,
		'wps_manufacturer='.$vendor.' Corporation\n'.
		'wps_manufacturer_url='.$producturl.'\n'.
		'wps_serial_number=00000000\n'.
		'wps_model_number=00000000\n'.
		'wps_model_name='.$model.'\n'.
		'wps_model_description=Wireless Router\n'.
		'wps_friendly_name='.$model.'\n'
		);

	
	if( strlen($ssid) <=28 && query("/runtime/devdata/psk")=="") /*if /runtime/devdata/psk assigned ,the ssid is append mac*/
	{
		$mac=query("/runtime/devdata/wlanmac");
		
		$mac=substr($mac, 12, 5);
	
		$i=0;
		while ($i < 2)
		{
		    $tmp = cut($mac, $i, ":");
			$last2mac=$last2mac.$tmp;
			$i++;
		}
		$new_ssid=$ssid.$last2mac;
	}
	else
	{
		$new_ssid=$ssid;
	}
	if($wsc2_version!="")
	{
		fwrite("a", $output,
			'wps_dev_name='.$model.'\n'.
			'wps_dev_category=6\n'.
			'wps_dev_sub_category=1\n'.
			'wps_dev_oui=0050f204\n'.
			'wps_os_version=0x80000000\n'.
			'wps_atheros_extension=0\n'.
			'wme_enabled='.$wmm.'\n'.
			'eap_user_file='.$eapuserfile.'\n'.
			'wps_helper=/etc/scripts/wps.sh\n'.
			'self_conf_ssid='.$new_ssid.'\n'.
			'wps_version2='.$wsc2_version.'\n'
			);
	}
	else
	{
		fwrite("a", $output,
			'wps_dev_name='.$model.'\n'.
			'wps_dev_category=6\n'.
			'wps_dev_sub_category=1\n'.
			'wps_dev_oui=0050f204\n'.
			'wps_os_version=0x00000001\n'.
			'wps_atheros_extension=0\n'.
			'wme_enabled='.$wmm.'\n'.
			'eap_user_file='.$eapuserfile.'\n'.
			'wps_helper=/etc/scripts/wps.sh\n'.
			'self_conf_ssid='.$new_ssid.'\n'
			);
	}

	if ($wpa > 0)
	{
		if ($rekey_gtk_interval != "")				fwrite("a", $output, 'wpa_group_rekey='.$rekey_gtk_interval.'\n');
		if ($encrtype == "TKIP"){
			fwrite("a", $output, 'wpa_pairwise=TKIP\n');
		}
		else if ($encrtype == "AES"){
			fwrite("a", $output, 'wpa_pairwise=CCMP\n');
		}
		else if ($encrtype == "TKIP+AES"){
			fwrite("a", $output, 'wpa_pairwise=TKIP CCMP\n');
		}

		if ($ieee8021x == 1)
		{
			fwrite(a, $output, 'wpa_key_mgmt=WPA-EAP\n');
			foreach("nwkey/eap")
			{
			fwrite(a, $output, 
					'auth_server_addr='.query("radius").'\n'.
					'auth_server_port='.query("port").'\n'.
					'auth_server_shared_secret='.query("secret").'\n'
				);
			}
		}
		else
		{
			fwrite("a", $output, 'wpa_key_mgmt=WPA-PSK\n');
			if (query("nwkey/psk/passphrase")=="1")
				 fwrite("a", $output, 'wpa_passphrase='.query("nwkey/psk/key").'\n');
			else fwrite("a", $output, 'wpa_psk='.query("nwkey/psk/key").'\n');
		}
	}
	else if ($encrtype=="WEP")
	{
		if ($authtype=="SHARED") 		$val = 2; 
		else if ($authtype=="OPEN") 	$val = 1; 
		else 							$val = 3;	/*WEPAUTO*/
		
		fwrite("a", $output, "auth_algs=".$val."\n");
		$wep++;
	}
	else /* Open */
	{
		fwrite("a", $output, 'auth_algs=1\n');
	}


	if ($wep > 0)
	{
		$i = query("nwkey/wep/defkey");
		$i--;
		fwrite("a",$output, 'wep_default_key='.$i.'\n');

		$ascii = query("nwkey/wep/ascii");
		foreach ("nwkey/wep/key")
		{
			if ($InDeX>4) break;
			if ($VaLuE!="")
			{
				$i = $InDeX - 1;
				if ($ascii=="1") $key = '"'.$VaLuE.'"'; else $key = $VaLuE;
				fwrite(a, $output, "wep_key".$i."=".$key."\n");
			}
		}
	}
	/*if($wps_conf==1)
	{
		fwrite("a", $output, 'not_support_pin_method=1\n');
	}
	else
	{
		fwrite("a", $output, 'not_support_pin_method=0\n');
	}*/
}

/********************************************************************/

/* generate the bridge list for topology file */
foreach ("/runtime/phyinf")
{
	if (query("type")=="eth" && query("bridge/port#")>0)
	{
		$br = query("name");
		echo "bridge ".$br."\n{\n";
		foreach ("bridge/port")
		{
			if (phyinf_check($VaLuE) == 1) { echo "\tinterface ".devname($VaLuE)."\n"; }
		}
		echo "}\n";
	}
}

$i = 0;
foreach ("/runtime/phyinf")
{
	if (query("type")!="wifi") continue;

	/* generate the radio list for topology file */
	$uid = query("uid");
	$dev = devname($uid);
	if ($dev=="") continue;
	if (phyinf_check($uid)!=1) continue;
	$cfile = '/var/run/hostapd-'.$dev.'.conf';
	echo
		"radio wifi".$i."\n".
		"{\n".
		"	ap\n".
		"	{\n";
	echo
		"		bss ".$dev."\n".
		"		{\n".
		"			config ".$cfile."\n".
		"		}\n";
	echo
		"	}\n".
		"}\n";
	$i++;
	/* generate the config file for hostapd */
	generate_configs($uid, $cfile);
}
?>
