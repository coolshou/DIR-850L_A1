<? /* vi: set sw=4 ts=4: */
/********************************************************************************
 *	NOTE: 
 *		The commands in this configuration generator is for Broadcom wireless.
 *******************************************************************************/
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/trace.php";
include "/etc/services/PHYINF/phywifi.php";

/***************************** functions ************************************/
function wmm_paramters($wlif_bss_idx)
{
	/* Wifi-WMM parameters */
	echo "nvram set wl".$wlif_bss_idx."_wme_ap_be=\"15 63 3 0 0 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_ap_bk=\"15 1023 7 0 0 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_ap_vi=\"7 15 1 6016 3008 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_ap_vo=\"3 7 1 3264 1504 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_apsd=on\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_bss_disable=0\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_no_ack=off\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_sta_be=\"15 1023 3 0 0 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_sta_bk=\"15 1023 7 0 0 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_sta_vi=\"7 15 2 6016 3008 off off\"\n";
	echo "nvram set wl".$wlif_bss_idx."_wme_sta_vo=\"3 7 2 3264 1504 off off\"\n";
}

function country_setup($wlif_bss_idx, $ccode, $wlif_is_5g, $pci_2g_path, $pci_5g_path)
{
	/* No-DFS: No use some special channels.
	 * DFS-enable: if detect special channel, then will not use it auto. 
	 * Our design will use No-DFS most.
	 * For SR and RU, they just only have two options: one is DFS enable or no use 5G band. 
	 */
	$ctry_code = $ccode;
	
	if ($ccode == "US") 
	{
		$regrev = 0;
		//$ctry_code = "Q2";	
	}
	else if ($ccode == "CN") 
		$regrev = 0;
	else if ($ccode == "TW")
		$regrev = 0;
	else if ($ccode == "CA") 
	{
		$regrev = 0;
		//$ctry_code = "Q2";	
	}
	else if ($ccode == "KR")
		$regrev = 1;
	else if ($ccode == "JP")
		$regrev = 1;
	else if ($ccode == "AU")
		$regrev = 0;
	else if ($ccode == "SG")
	{
		/* Singaport two choice:
			1. DFS enable= SG/0  or 
			2. No use 5G band= SG/1
		*/
		$regrev = 0;
	}
	else if ($ccode == "LA")
		$regrev = 0;
	else if ($ccode == "IL")
		$regrev = 0;
	else if ($ccode == "EG")
		$regrev = 0;
	else if ($ccode == "BR")
		$regrev = 0;
	else if ($ccode == "RU")
	{
		/* Russia two choice:
			1. DFS enable= RU/1  or 
			2. No use 5G band= RU/0
		*/
		$regrev = 1;
	}
	else if ($ccode == "GB" || $ccode == "EU")
		$regrev = 0;
	else
		$regrev = 0;

	if ($wlif_is_5g == 1)
	{
		/*echo "nvram set ".$pci_5g_path."regrev=".$regrev."\n";
		echo "nvram set ".$pci_5g_path."ccode=".$ctry_code."\n";*/
		echo "nvram set ".$pci_5g_path."regrev=0\n";
		echo "nvram set ".$pci_5g_path."ccode=0\n";
	}
	else
	{
		/*echo "nvram set ".$pci_2g_path."regrev=".$regrev."\n";
		echo "nvram set ".$pci_2g_path."ccode=".$ctry_code."\n";*/
		echo "nvram set ".$pci_2g_path."regrev=0\n";
		echo "nvram set ".$pci_2g_path."ccode=0\n";
	}
	
	echo "nvram set wl".$wlif_bss_idx."_country_code=".$ctry_code."\n";
	echo "nvram set wl".$wlif_bss_idx."_country_rev=".$regrev."\n";
	/* alpha create nvram parameter: it's value include country code and regulatory revision */
	echo "nvram set wl".$wlif_bss_idx."_alpha_country_code=".$ctry_code."/".$regrev."\n";
}

function set_sta_mode($wl_prefix , $enabled)
{
	set("/runtime/wifi/".$wl_prefix."/sta" , $enabled);
}

function dev_stop($uid)
{
	$dev_name = devname($uid);
	$guestzone = isguestzone($uid);

	if ($guestzone == 1)
	{
		$dev_name = devname($uid);
	} else {
		$wlif_bss_idx = get_wlif_bss($uid);
		$dev_name = "wl".$wlif_bss_idx;
	}
	echo "nvram set ".$dev_name."_bss_enabled=0\n";
	echo "nvram set ".$dev_name."_radio=0\n";

	set_sta_mode($dev_name , "0");
}

function add_srom_value($prefix, $name, $value)
{
	echo "nvram set ".$prefix.$name."=".$value."\n";
}

function generate_srom_values($pci_2g_path, $pci_5g_path)
{
	add_srom_value($pci_2g_path , "aa2g" , "7");
	add_srom_value($pci_2g_path , "ag0" , "0");
	add_srom_value($pci_2g_path , "ag1" , "0");
	add_srom_value($pci_2g_path , "ag2" , "0");
	add_srom_value($pci_2g_path , "antswctl2g" , "0");
	add_srom_value($pci_2g_path , "antswitch" , "0");
	add_srom_value($pci_2g_path , "boardflags" , "0x80001200");
	add_srom_value($pci_2g_path , "boardflags2" , "0x00100000");
	add_srom_value($pci_2g_path , "boardvendor" , "0x14E4");
	add_srom_value($pci_2g_path , "cckbw202gpo" , 0x2200);
	add_srom_value($pci_2g_path , "cckbw20ul2gpo" , "0x2200");
	add_srom_value($pci_2g_path , "ccode" , "0");
	add_srom_value($pci_2g_path , "devid" , "0x4332");
	add_srom_value($pci_2g_path , "elna2g" , "2");
	add_srom_value($pci_2g_path , "extpagain2g" , "3");
	add_srom_value($pci_2g_path , "ledbh0" , "11");
	add_srom_value($pci_2g_path , "ledbh1" , "11");
	add_srom_value($pci_2g_path , "ledbh12" , "2");
	add_srom_value($pci_2g_path , "ledbh2" , "11");
	add_srom_value($pci_2g_path , "ledbh3" , "11");
	add_srom_value($pci_2g_path , "leddc" , "0xFFFF");
	add_srom_value($pci_2g_path , "legofdm40duppo" , "0x0000");
	add_srom_value($pci_2g_path , "legofdmbw202gpo" , "0x88765433");
	add_srom_value($pci_2g_path , "legofdmbw20ul2gpo" , "0x88765433");
	add_srom_value($pci_2g_path , "macaddr" , "00:90:4C:0D:C0:18");
	add_srom_value($pci_2g_path , "maxp2ga0" , "0x74");
	add_srom_value($pci_2g_path , "maxp2ga1" , "0x74");
	add_srom_value($pci_2g_path , "maxp2ga2" , "0x74");
	add_srom_value($pci_2g_path , "mcs32po" , "0x0003");
	add_srom_value($pci_2g_path , "mcsbw202gpo" , "0x88765433");
	add_srom_value($pci_2g_path , "mcsbw20ul2gpo" , "0x88765433");
	add_srom_value($pci_2g_path , "mcsbw402gpo" , "0x99855433");
	add_srom_value($pci_2g_path , "pa2gw0a0" , "0xFE7C");
	add_srom_value($pci_2g_path , "pa2gw0a1" , "0xFE85");
	add_srom_value($pci_2g_path , "pa2gw0a2" , "0xFE82");
	add_srom_value($pci_2g_path , "pa2gw1a0" , "0x1F1B");
	add_srom_value($pci_2g_path , "pa2gw1a1" , "0x1EA5");
	add_srom_value($pci_2g_path , "pa2gw1a2" , "0x1EC5");
	add_srom_value($pci_2g_path , "pa2gw2a0" , "0xF89C");
	add_srom_value($pci_2g_path , "pa2gw2a1" , "0xF8BF");
	add_srom_value($pci_2g_path , "pa2gw2a2" , "0xF8B8");
	add_srom_value($pci_2g_path , "parefldovoltage" , "60");
	add_srom_value($pci_2g_path , "pdetrange2g" , "3");
	add_srom_value($pci_2g_path , "phycal_tempdelta" , "0");
	add_srom_value($pci_2g_path , "regrev" , "0");
	add_srom_value($pci_2g_path , "rxchain" , "7");
	add_srom_value($pci_2g_path , "sromrev" , "9");
	add_srom_value($pci_2g_path , "tempoffset" , "0");
	add_srom_value($pci_2g_path , "temps_hysteresis" , "5");
	add_srom_value($pci_2g_path , "temps_period" , "5");
	add_srom_value($pci_2g_path , "tempthresh" , "120");
	add_srom_value($pci_2g_path , "tssipos2g" , "1");
	add_srom_value($pci_2g_path , "txchain" , "7");
	add_srom_value($pci_2g_path , "venid" , "0x14E4");
	add_srom_value($pci_2g_path , "xtalfreq" , "20000");
}

function dev_init($pci_2g_path, $pci_5g_path)
{
	//we decide to write pci/1/0 and pci/1/1 both in bootcode
	//so, BCM SDK firmware can work correctly (tom, 20121022)
	//generate_srom_values($pci_2g_path, $pci_5g_path);

	foreach ("/phyinf")
	{
		if (query("type")!="wifi") continue;
		$uid = query("uid");

		//wifi station profile is not a real phy interface, we don't need to initial it
		if(get_phyinf_sta_mode($uid) == 1)
			continue;

		$wlif_bss_idx = get_wlif_bss($uid);
		$dev_name = devname($uid);
		$guestzone = isguestzone($uid);

		echo "# ".$uid."\n";
		echo "nvram set wl".$wlif_bss_idx."_ifname=".$dev_name."\n";
		dev_stop($uid);
		if ($guestzone != 1)
		{
			$prefix = cut($uid, 0, ".");
			$guest_uid = $prefix.".2";
			$dev_name = devname($guest_uid);
			echo "nvram set wl".$wlif_bss_idx."_vifs=".$dev_name."\n";

			if(isband5g($uid) == 1)
			{
				$wmac5addr  = PHYINF_getdevdatamac("wlanmac2");
				echo "nvram set ".$pci_5g_path."macaddr=".$wmac5addr."\n";				
			}
			else
			{
				$wmac24addr = PHYINF_getdevdatamac("wlanmac");
				echo "nvram set ".$pci_2g_path."macaddr=".$wmac24addr."\n";				
			}
		}
	}
}

/* let guestzone_mac = host_mac + 1*/
function get_guestzone_mac($host_mac)
{
	$index = 5;
	$guestzone_mac = "";
	$carry = 0;

	//loop from low byte to high byte
	//ex: 00:01:02:03:04:05
	//05 -> 04 -> 03 -> 02 -> 01 -> 00
	while($index >= 0)
	{
		$field = cut($host_mac , $index , ":");

		//check mac format
		if($field == "")
			return "";

		//to value
		$value = strtoul($field , 16);
		if($value == "")
			return "";

		if($index == 5)
			$value = $value + 1;

		//need carry?
		$value = $value + $carry;
		if($value > 255)
		{
			$carry = 1;
			$value = $value % 256;
		}
		else
			$carry = 0;

		//from dec to hex
		$hex_value = dec2strf("%02X" , $value);

		if($guestzone_mac == "")
			$guestzone_mac = $hex_value;
		else
			$guestzone_mac = $hex_value.":".$guestzone_mac;

		$index = $index - 1;
	}

	return $guestzone_mac;
}

/* let guestzone_mac = local(host_mac) */
/*function get_guestzone_mac($host_mac)
{
	$index = 5;
	$guestzone_mac = "";

	//loop from low byte to high byte
	//ex: 00:01:02:03:04:05
	//05 -> 04 -> 03 -> 02 -> 01 -> 00
	while($index >= 0)
	{
		$field = cut($host_mac , $index , ":");

		//check mac format
		if($field == "")
			return "";

		//to value
		$value = strtoul($field , 16);
		if($value == "")
			return "";

		if($index == 0)
			$value = 2;

		//from dec to hex
		$hex_value = dec2strf("%02X" , $value);

		if($guestzone_mac == "")
			$guestzone_mac = $hex_value;
		else
			$guestzone_mac = $hex_value.":".$guestzone_mac;

		$index = $index - 1;
	}

	return $guestzone_mac;
}*/

function alpha_auth_to_bcm_akm($auth)
{
	if($auth == "WPA")
		return "wpa";
	else if($auth == "WPAPSK")
		return "psk";
	else if($auth == "WPA2")
		return "wpa2";
	else if($auth == "WPA2PSK")
		return "psk2";
	else if($auth == "WPA+2")
		return "wpa2";
	else if($auth == "WPA+2PSK")
		return "psk psk2";
	else
		return "";
}

function try_set_psk_passphrase($wl_prefix, $wifi)
{
	$auth = query($wifi."/authtype");
	if($auth != "WPAPSK" && $auth != "WPA2PSK" && $auth != "WPA+2PSK")
		return;

//	if(query($wifi."/nwkey/psk/passphrase") != "1")
//		return;

	$key = query($wifi."/nwkey/psk/key");
	echo "nvram set ".$wl_prefix."_wpa_psk=\"".$key."\"\n";
}

function alpha_enc_to_bcm_crypto($enc)
{
	if($enc == "TKIP")
		return "tkip";
	else if($enc == "AES")
		return "aes";
	else if($enc == "TKIP+AES")
		return "tkip+aes";
	else
		return "";
}

function sta_security_setup($wl_prefix , $wifi)
{
	echo "nvram set ".$wl_prefix."_akm=\"".alpha_auth_to_bcm_akm(query($wifi."/authtype"))."\"\n";
	echo "nvram set ".$wl_prefix."_crypto=\"".alpha_enc_to_bcm_crypto(query($wifi."/encrtype"))."\"\n";
	try_set_psk_passphrase($wl_prefix, $wifi);
}

function clean_nvram_values($wl_prefix , $keys)
{
	$value_qty = cut_count($keys , " ");
	$index = 0;

	if($wl_prefix != "")
		$wl_prefix = $wl_prefix."_";

	while($index < $value_qty)
	{
		$key = cut($keys , $index , " ");
		if($key != "")
			echo "nvram unset ".$wl_prefix.$key."\n";

		$index += 1;
	}
}

function security_setup($wl_prefix, $wifi)
{
	echo "#wireless security setup start ---\n";
	echo "nvram set ".$wl_prefix."_auth_mode=none\n";
	echo "nvram set ".$wl_prefix."_auth=0\n";
	echo "nvram set ".$wl_prefix."_wep=disabled\n";
	/* some stuff about wpa. we don't need this setting, just clean it (tom, 20120406) */
	clean_nvram_values($wl_prefix , "akm");

	/* authtype */
	$auth = query($wifi."/authtype");
	/* encrtype */
	$encrypt = query($wifi."/encrtype");

	if ( $auth == "OPEN" || $auth == "SHARED" || $auth == "WEPAUTO")
	{
		/* be careful! help message of wl says open/share is value 3, but driver uses 2 (tom, 20120410) */
		if ( $auth == "SHARED" )	{echo "nvram set ".$wl_prefix."_auth=1\n";}
		else if ( $auth == "WEPAUTO" ) {echo "nvram set ".$wl_prefix."_auth=2\n";}
		else						{echo "nvram set ".$wl_prefix."_auth=0\n";}
		if ( $encrypt == "WEP" )
		{
			echo "nvram set ".$wl_prefix."_wep=enabled\n";
			/* Now the wep key must be hex number, so using "query" is ok. */
			$defkey = query($wifi."/nwkey/wep/defkey");
			$keystring = query($wifi."/nwkey/wep/key:".$defkey);
			echo "nvram set ".$wl_prefix."_key=".$defkey."\n";
			echo "nvram set ".$wl_prefix."_key".$defkey."=\"".$keystring."\"\n";
		}
		else
		{
			echo "nvram set ".$wl_prefix."_wep=disabled\n";
		}
	}
	if( $auth=="WPA2" || $auth=="WPA" || $auth=="WPA+2")
	{
		echo "nvram set ".$wl_prefix."_preauth=0\n";
	}
	if(query($wifi."/opmode") == "STA")
		sta_security_setup($wl_prefix , $wifi);

	echo "#wireless security setup end ---\n";
}

function checking_bandwidth($wlif_is_5g, $bandwidth, $nmode)
{
	if($bandwidth == "0" || $nmode == "0")
		return "20";
	else if($bandwidth == "1")
		return "40";
	else if($bandwidth == "2" && $wlif_is_5g == 1)	
		return "80";
	else if($bandwidth == "2" && $wlif_is_5g != 1)	
		return "40";		

	echo "#check bandwidth setting, we cannot find correct bandwidth (rtcfg.php)\n";
	return "20";
}

function control_sideband($wlif_is_5g, $bandwidth, $channel)
{
	/* how to get sideband list? just use "wl -i ifname -b <2|5> -w <20|40|80>" */
	if($channel == "0" || $bandwidth == "20")
		return "";

	/* for 5g, we don't need sideband setting, wlconf will do this for us */
	if($wlif_is_5g != 1)
	{
		/* for 2.4g */
		if($channel >=5)
			return "u";
		else
			return "l";
	}

	return "";
}

function channel_idx_to_channel_number_5g($idx)
{
	$path_a = query("/runtime/freqrule/channellist/a");
	if($path_a=="")
	{
		return 36;
	}
	
	$cnt = cut_count($path_a, ",");
	if($cnt != 0)
	{
		$idx = $idx % $cnt;
		$token = cut($path_a, $idx, ",");
	}
	
	if($token == "")
	{
		$token = 36;
	}
	return $token;
}

function channel_idx_to_channel_number_24g($idx)
{
	$path_g = query("/runtime/freqrule/channellist/g");
	
	if($path_g=="")
	{
		return 6;
	}
	$cnt = cut_count($path_g, ",");
	if($cnt != 0)
	{
		$idx = $idx % $cnt;
		$token = cut($path_g, $idx, ",");
	}

	if($token == "")
	{
		$token = 6;
	}
	return 	$token;
}
function pseudo_autochannel($wlif_is_5g)
{
	$uptime = fread("" , "/proc/uptime");
	$uptime = cut($uptime , 0 , ".");

	if($wlif_is_5g == "1")
	{
		return channel_idx_to_channel_number_5g($uptime);
	}
	else
	{
		return channel_idx_to_channel_number_24g($uptime);
	}
}

function channel_bandwidth_setup($wlif_bss_idx, $wlif_is_5g, $bandwidth, $nmode, $channel)
{
	echo "#wireless channel/bandwidth setup start ---\n";

	/* use new channel spec to config, so we clean this */
	echo "nvram set wl".$wlif_bss_idx."_channel=0\n";

	$bandwidth = checking_bandwidth($wlif_is_5g, $bandwidth, $nmode);

	if($bandwidth == "20")
		echo "nvram set wl".$wlif_bss_idx."_bw_cap=1\n";
	else if($bandwidth == "40")
		echo "nvram set wl".$wlif_bss_idx."_bw_cap=3\n";
	else if($bandwidth == "80")
		echo "nvram set wl".$wlif_bss_idx."_bw_cap=7\n";

//	if($channel == 0)
//		$channel = pseudo_autochannel($wlif_is_5g);

	if($channel != 0)
	{
		$sideband = control_sideband($wlif_is_5g, $bandwidth, $channel);
		if($wlif_is_5g == 1)
			$channel_spec = "5g";
		else
			$channel_spec = "2g";

		$channel_spec = $channel_spec.$channel."/".$bandwidth.$sideband;

		echo "nvram set wl".$wlif_bss_idx."_chanspec=".$channel_spec."\n";
	}
	else
	{
		echo "nvram set wl".$wlif_bss_idx."_chanspec=0\n";
		if($wlif_is_5g==1)
		{
			$ifname = "wifia0";
		}
		else
		{
			$ifname = "wifig0";
		}
		echo "nvram set acs_ifnames=".$ifname."\n";
	}

	echo "#wireless channel/bandwidth setup end ---\n";
}

function test_override($wlif_bss_idx , $wlif_is_5g)
{
	echo "#test override start ---\n";

	/* those values come from broadcom's SDK firmware */
	//echo "nvram set wl".$wlif_bss_idx."_rxchain_pwrsave_enable=0\n";

	/* just for throughput test */
	echo "echo 300 > /proc/sys/net/core/netdev_budget\n";
	echo "echo 1000 > /proc/sys/net/core/netdev_max_backlog\n";

	echo "#test override end ---\n";
}

function dev_default_values($PHY_UID, $wl_prefix)
{
	$guestzone = isguestzone($PHY_UID);

	echo "#defaule values start ---\n";

	if ($guestzone == 1)
	{
		//guest zone default setting here
		return;
	}

	//master default setting here
	echo "nvram set ".$wl_prefix."_mimo_preamble=gfbcm\n";
	echo "nvram unset ".$wl_prefix."_nmode_protection\n";
	echo "nvram unset ".$wl_prefix."_gmode_protection\n";
	echo "nvram unset ".$wl_prefix."_nmode\n";
	echo "nvram unset ".$wl_prefix."_gmode\n";

	echo "#default values end ---\n";
}

function get_ure_disable()
{
	$index = 0;
	while($index < 2)
	{
		$sta_enabled = query("/runtime/wifi/wl".$index."/sta");
		if($sta_enabled == "1")
			return "0";

		$index += 1;
	}

	return "1";
}

function operation_mode_setup($PHY_UID, $wl_prefix, $wifi)
{
	echo "#operation mode setup start ---\n";

	$opmode = query($wifi."/opmode");
	clean_nvram_values($wl_prefix , "sta_retry_time wps_mode wps_oob ure");

	if($opmode == "STA")
	{
		//echo "nvram set ".$wl_prefix."_mode=wet\n";
		//set_sta_mode($wl_prefix , "1");
		//echo "nvram set ".$wl_prefix."_ure=1\n";
		echo "nvram set ".$wl_prefix."_mode=psta\n";
		echo "nvram set ".$wl_prefix."_sta_retry_time=5\n";
		//echo "nvram set ".$wl_prefix."_wps_mode=disabled\n";
		//echo "nvram set ".$wl_prefix."_wps_oob=disabled\n";
		clean_nvram_values("" , "wan_ifnames lan_ifname");
		//those values are used by nas and eapd daemons
		echo "nvram set lan_ifname=\"".devname($PHY_UID)."\"\n";
		echo "nvram set lan_ifnames=\"".$wl_prefix."\"\n";
		echo "nvram set ".$wl_prefix."_ifname=".devname($PHY_UID)."\n";
	}
	else
	{
	echo "nvram set ".$wl_prefix."_mode=ap\n";
	//set_sta_mode($wl_prefix , "0");
	}

	//echo "nvram set ure_disable=".get_ure_disable()."\n";
	echo "nvram set ure_disable=1\n";

	echo "#operation mode setup end ---\n";
}

function dev_start($PHY_UID, $pci_2g_path, $pci_5g_path)
{
	$wmac24addr = PHYINF_getdevdatamac("wlanmac");
	$wmac5addr  = PHYINF_getdevdatamac("wlanmac2");

	$phy	= XNODE_getpathbytarget("",			"phyinf", "uid", $PHY_UID);
	$phyrp  = XNODE_getpathbytarget("/runtime",     "phyinf", "uid", $PHY_UID);
	$wifi	= XNODE_getpathbytarget("/wifi",	"entry",  "uid", query($phy."/wifi"));
	$winf   = query($phyrp."/name");
	$brphyinf   = find_brdev($PHY_UID);
	$guestzone = isguestzone($PHY_UID);

	$wlif_is_5g = isband5g($PHY_UID);
	$wlif_bss_idx = get_wlif_bss($PHY_UID);

	$opmode = query($wifi."/opmode");

	echo "#PHY_UID == ".$PHY_UID."\n";
	echo "#phy == ".$phy."\n";
	echo "#wifi == ".$wifi."\n";
	echo "#wmac24addr == ".$wmac24addr."\n";
	echo "#wmac5addr == ".$wmac5addr."\n";
	echo "#brphyinf == ".$brphyinf."\n";

	if ($guestzone == 1)
	{
		$wl_prefix=$winf;
	} else {
		$wl_prefix="wl".$wlif_bss_idx;
	}

	dev_default_values($PHY_UID, $wl_prefix);

	/* Start the master */
	
	echo "nvram set ".$wl_prefix."_ssid=\"".get("s",$wifi."/ssid")."\"\n";
	echo "nvram set ".$wl_prefix."_closed=0\n";
	echo "nvram set ".$wl_prefix."_bss_enabled=1\n";

	/* Broadcom driver will read following parameters when WPA is enabled */
	echo "nvram set ".$wl_prefix."_radio=1\n";
	echo "nvram set ".$wl_prefix."_unit=0\n";
	echo "nvram set ".$wl_prefix."_maxassoc=128\n";
	echo "nvram set ".$wl_prefix."_bss_maxassoc=128\n";

	operation_mode_setup($PHY_UID, $wl_prefix, $wifi);

	security_setup($wl_prefix, $wifi);

	if ($guestzone == 1)
	{
		$guestzone_mac = "";

		if ($wlif_is_5g == 1)
			$guestzone_mac = get_guestzone_mac($wmac5addr);
		else
			$guestzone_mac = get_guestzone_mac($wmac24addr);

		echo "nvram set wl".$wlif_bss_idx."_hwaddr=".$guestzone_mac."\n";
		return 0;
	}

	/* ----------------------------- get configuration -----------------------------------*/
	/* country code */
	/* for country code we nove it to layout.php.This is not change in runtime
	$ccode = query("/runtime/devdata/countrycode");
	echo "#ccode == ".$ccode."\n";
	if (isdigit($ccode)==1)
	{
		TRACE_debug("WIFI.WLAN-1 service [rtcfg.php (ralink conf)]:".
				"Your country code (".$ccode.") is in number format. ".
				"Please change the country code as ISO name. ".
				"Use 'US' as country code.");
		$ccode = "US";
	}
	if ($ccode == "")
	{
		TRACE_error("WIFI.WLAN-1 service: no country code! ".
				"Please check the initial value of this board! ".
				"Use 'US' as country code.");
		$ccode = "US";
	}

	country_setup($wlif_bss_idx, $ccode, $wlif_is_5g, $pci_2g_path, $pci_5g_path);
	*/
	if ($wlif_is_5g == 1)
	{ 	
		echo "nvram set wl".$wlif_bss_idx."_hwaddr=".$wmac5addr."\n";
		/* set phytype to PHY_TYPE_AC */
		echo "nvram set wl".$wlif_bss_idx."_phytype=v\n";
		/* 0: Auto, 1: A band, 2: G band, 3: All */
		echo "nvram set wl".$wlif_bss_idx."_nband=1\n";
		echo "nvram set wl".$wlif_bss_idx."_rateset=default\n";
	}
	else 					
	{	
		echo "nvram set wl".$wlif_bss_idx."_hwaddr=".$wmac24addr."\n";
		/* set phytype to PHY_TYPE_HT */
		echo "nvram set wl".$wlif_bss_idx."_phytype=h\n";
		echo "nvram set wl".$wlif_bss_idx."_nband=2\n";
		echo "nvram set wl".$wlif_bss_idx."_rateset=default\n";
	}

	/* Get configuration */
	anchor($phy."/media");
	$channel		= query("channel");
	$autochannel	= query("autochannel");			if ($autochannel=="1")		{$channel="0";}
	$beaconinterval	= query("beacon");				if ($beaconinterval=="")	{$beaconinterval="100";}
	$fraglength		= query("fragthresh");			if ($fraglength=="")		{$fraglength="2346";}
	$rtslength		= query("rtsthresh");			if ($rtslength=="")			{$rtslength="2346";}
	$ssidhidden		= query($wifi."/ssidhidden");			if ($ssidhidden!="1")		{$ssidhidden="0";}

	//$ctsmode		= query("ctsmode");				if ($ctsmode=="")			{$ctsmode="0";}
	$preamble		= query("preamble");			if ($preamble=="")			{$preamble="long";}
	$txrate			= query("txrate");
	$txpower		= query("txpower");
	$dtim			= query("dtim");				if ($dtim=="")				{$dtim="1";}
	$wlan2wlan		= query("bridge/wlan2wlan");	if ($wlan2wlan!="0")		{$wlan2wlan="1";}
	$wlan2lan		= query("bridge/wlan2lan");		if ($wlan2lan!="0")			{$wlan2lan="1";}
	$bandwidth		= query("dot11n/bandwidth");

	/* "20": 0, "20+40": 1, "20+40+80: 2" */
	if($opmode == "STA")
	{
		$bandwidth = "2";
	}
	else
	{
		if($bandwidth=="20" )			{$bandwidth="0";}
		else if ($bandwidth=="20+40+80" ) {$bandwidth="2";}
		else 							{$bandwidth="1";}
	}

	$wmm			= query("wmm/enable");			if ($wmm!="1")				{$wmm="0";}
	$mcs_auto		= query("dot11n/mcs/auto");			
	$mcs_idx		= query("dot11n/mcs/index");			
	if ( $mcs_auto != 1 && $mcs_idx == "" )		{ $mcs_auto = 1;	}

	/* set short quard interval - this nvram var was created by ALPHA , and code at wlconf */
	$short_guardintv		= query("dot11n/guardinterval");			
	if ($short_guardintv=="400" )			{$short_guardintv="1";}
	else		{$short_guardintv="0";}


	if ($txrate == "5.5") { $TXRATE_CMD="5500000"; } 
	else if ($txrate == "auto") { $TXRATE_CMD=0; } 
	else { $TXRATE_CMD = $txrate * 1000000; }

	$wlmode			= query("wlmode");
	/* /wireless/wlanmode : 1:11b, 2:11g, 3:11b+11g, 4:11n, 5:11g+11n, 6:11g+11n, 7: 11b+11g+11n, 8:a, 9:a+n, 10: a+g+n, 11:n in 5G band only, 12:ac in 5g band */
	$gmode=""; $nmode="";
	$wlmode = query($phy."/media/wlmode");
	$freq = query($phy."/media/freq");

$can_support_n_mode=1;
$authtype	= query($wifi."/authtype");
$encrtype	= query($wifi."/encrtype");
if($authtype=="OPEN"|| $authtype=="SHARED" ||$authtype=="WEPAUTO")
{
	if($encrtype=="WEP")
	{
		$can_support_n_mode=0;
	}
}
else if($authtype=="WPAPSK"|| $authtype=="WPA2PSK" ||$authtype=="WPA+2PSK")
{
	if($encrtype=="TKIP")
	{
		$can_support_n_mode=0;
	}
}

	if		($wlmode == "a")	{$wlmode = 8; $gmode="0"; $nmode="0";}
	else if ($wlmode == "an")	
	{
		if($can_support_n_mode==1)
		{
			$wlmode = 9; $gmode="0"; $nmode="1";
		}
		else
		{
			$wlmode = 8; $gmode="0"; $nmode="0";
		}
	}
	else if ($wlmode == "agn")	{$wlmode = 10; $gmode="1"; $nmode="1";}
	else if	($wlmode == "bgn")	
	{
		if($can_support_n_mode==1)
		{
			$wlmode = 7; $gmode="1"; $nmode="1";
		}
		else
		{
			$wlmode = 3; $gmode="1"; $nmode="0";
		}
	}
	else if ($wlmode == "bg")	{$wlmode = 3; $gmode="1"; $nmode="0";}
	else if ($wlmode == "n")	
	{
		if ($freq == "5" || $freq == 5 ) 	{ $wlmode = 11; } 
		else 								{ $wlmode = 4; }	

		$gmode="0"; 
		$nmode="1";
	}
	else if ($wlmode == "g")	{$wlmode = 2; $gmode="2"; $nmode="0";}
	else if ($wlmode == "b")	{$wlmode = 1; $gmode="0"; $nmode="0";}
	else if ($wlmode == "ac")	{$wlmode = 12; $gmode="0";}
	else
	{
		/* use 'bgn' as default.*/
		TRACE_info("rtcfg (broadcom conf): Not supported wireless mode: [".$wlmode."].Use 'bng' as default wireless mode.");
		$wlmode = 7;	$gmode="1"; $nmode="1";
	}

	/* For preamble: default is long preamble. */
	echo "nvram set wl".$wlif_bss_idx."_plcphdr=".$preamble."\n";

	/* obss_coex, only 2.4G has this */
	if($wlif_is_5g == 1)
	{
		echo "nvram unset wl".$wlif_bss_idx."_obss_coex\n";
		echo "nvram set wl".$wlif_bss_idx."_frameburst=on\n";	
	}
	else
	{
		if(query($phy."/media/dot11n/bw2040coexist") == "1")
			echo "nvram set wl".$wlif_bss_idx."_obss_coex=1\n";
		else
			echo "nvram set wl".$wlif_bss_idx."_obss_coex=0\n";
			
		echo "nvram set wl".$wlif_bss_idx."_frameburst=on\n";	
	}

	if($gmode!="") { echo "nvram set wl".$wlif_bss_idx."_gmode=".$gmode."\n"; }
	if($nmode!="") { echo "nvram set wl".$wlif_bss_idx."_nmode=".$nmode."\n"; }
	if ($wlmode == 4 && $mcs_auto != 1) 
	{	
		echo "nvram set wl".$wlif_bss_idx."_nmcsidx=".$mcs_idx."\n"; 
	}
	else if ($wlmode == 11 && $mcs_auto != 1) 
	{	
		echo "nvram set wl".$wlif_bss_idx."_nmcsidx=".$mcs_idx."\n"; 
	}
	else {echo "nvram set wl".$wlif_bss_idx."_nmcsidx=-1\n"; }

	/* if 802.11n only, shall set wlx_nreqd=1 to limit only 11N STA can connect */
	if ($wlmode == 4 || $wlmode == 11) 		{	echo "nvram set wl".$wlif_bss_idx."_nreqd=1\n";	}
	else					{	echo "nvram set wl".$wlif_bss_idx."_nreqd=\n";	}

	/* 802.11h, we didn't use it, so disable it. If it enabled, the IE will contain coutry code.*/
	/* Regulatory Mode:off(disabled), h(802.11h), d(802.11d)*/
	echo "nvram set wl".$wlif_bss_idx."_reg_mode=off\n";

	/* generic settings ____________________________________________________ */
	echo "nvram set wl".$wlif_bss_idx."_wme_bss_disable=0\n";
	echo "nvram set wl".$wlif_bss_idx."_wme=";
	if ($wmm==1) { echo "on"; } else { echo "off"; }
	echo "\n";

	/* Wifi-WMM parameters */
	wmm_paramters( $wlif_bss_idx );

	echo "nvram set wl".$wlif_bss_idx."_bcn=".$beaconinterval."\n";	
	echo "nvram set wl".$wlif_bss_idx."_dtim=".$dtim."\n";

	echo "nvram set wl".$wlif_bss_idx."_closed=".$ssidhidden."\n";

	echo "nvram set wl".$wlif_bss_idx."_rts=".$rtslength."\n";

	echo "nvram set wl".$wlif_bss_idx."_frag=".$fraglength."\n";
	echo "nvram set wl".$wlif_bss_idx."_rate=".$TXRATE_CMD."\n";
	

	/* bandwidth : 0:20Mhz, 1:40Mhz, 2:80Mhz */
	channel_bandwidth_setup($wlif_bss_idx, $wlif_is_5g, $bandwidth, $nmode, $channel);

	/* set short quard interval - this nvram var was created by ALPHA , and code at wlconf */
	echo "nvram set wl".$wlif_bss_idx."_short_sgi_enable=".$short_guardintv."\n";	

	/* WLAN/LAN bridge _____________________________________________________ */
	//echo "echo ".$wlan2lan." > /proc/net/br_forward_br0\n";
	echo "nvram set wl".$wlif_bss_idx."_ap_isolate=";
	$isolate=query($wifi."/acl/isolation");
	if ($isolate == "0") { echo "0\n"; } else { echo "1\n"; }
//	if ($wlan2wlan == "1") { echo "0\n"; } else { echo "1\n"; }

	/* aclmode 0:disable, 1:allow all of the list, 2:deny all of the list */
	$aclmode = query($wifi."/acl/policy");
	if		($aclmode == "ACCEPT" )	{ $ACLMODE_CMD="allow"; $aclmode=1;	}
	else if	($aclmode == "DROP" )	{ $ACLMODE_CMD="deny";  $aclmode=2;	}
	else					{ $ACLMODE_CMD="disabled"; $aclmode=0; }
	echo "nvram set wl".$wlif_bss_idx."_macmode=".$ACLMODE_CMD."\n";

	if ($aclmode > 0)
	{
		$acl_count	= query($wifi."/acl/count");
		$acl_max	= query($wifi."/acl/max");
		$acl_list = "";
		foreach ($wifi."/acl/entry")
		{
			if ($InDeX > $acl_count || $InDeX > $acl_max) break;
			if ($acl_list!="")	$acl_list = $acl_list." ".query("mac");
			else				$acl_list = query("mac");
		}
		if ($acl_list!="")	echo "nvram set wl".$wlif_bss_idx."_maclist=".$acl_list."\n";
	}

	/* Set Transmit Power. 
	   bc. broadcom's power value was setting by HW/RD, it's fix. 
	   Setting 17dbm is max values, 13dbm is min values(include CCK/OFDM 20/40BW), so avg. is 15dbm.
	   If $txpower==50 then ((15-3)/15)*100=80%
	   If $txpower==25 then ((15-6)/15)*100=60%
	   If $txpower==12.5 then ((15-9)/15)*100=40%
	 */
	/*if ($txpower == "100" ) { echo "nvram set wl".$wlif_bss_idx."_pwr_percent=100\n"; }
	else if ($txpower == "50" ) { echo "nvram set wl".$wlif_bss_idx."_pwr_percent=80\n"; }
	else if ($txpower == "25" ) { echo "nvram set wl".$wlif_bss_idx."_pwr_percent=60\n"; }
	else if ($txpower == "12.5" ) { echo "nvram set wl".$wlif_bss_idx."_pwr_percent=40\n"; }
	else					{ echo "nvram set wl".$wlif_bss_idx."_pwr_percent=95\n"; }*/

	if ( $wmm =="1" )	{	echo "et qos 1\n";	}
	else				{	echo "et qos 0\n";	}

	$wps_en=query("/runtime/wps_sta/enable");
	if($wps_en=="1")
	{
		echo "nvram set ".$wl_prefix."_mode=wet\n";
		set_sta_mode($wl_prefix , "1");
		echo "nvram set ".$wl_prefix."_ure=1\n";
		echo "nvram unset ".$wl_prefix."_ssid\n";
		echo "nvram set ".$wl_prefix."_wps_mode=enabled\n";
		echo "nvram set ".$wl_prefix."_wps_mode=enabled\n";
		echo "nvram set wps_pbc_apsta=enabled\n";
		echo "nvram set wps_method=2\n";
	}
	test_override($wlif_bss_idx , $wlif_is_5g);
}

/**********************************************************************************/

echo "#!/bin/sh\n";

//in SDK 143, slot number is 1 (tom, 20121022)
//$pci_2g_path = "pci/1/0/";
$pci_2g_path = "pci/1/1/";
$pci_5g_path = "pci/2/0/";

if ($ACTION=="START")
{
	dev_start($PHY_UID, $pci_2g_path, $pci_5g_path);
} else if ($ACTION=="STOP") {
	dev_stop($PHY_UID);
} else if ($ACTION=="INIT") {
	dev_init($pci_2g_path, $pci_5g_path);
}

?>
