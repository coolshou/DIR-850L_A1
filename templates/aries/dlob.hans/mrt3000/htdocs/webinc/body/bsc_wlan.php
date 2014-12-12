<?
include "/htdocs/phplib/wifi.php";
?>	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Wireless Settings");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Wireless Network");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h",'Use this section to configure the wireless settings for your miiiCasa router.');?>
					<?echo I18N("h",'Please note that changes made in this section may also need to be duplicated on your wireless client.');?><br/>
					<?echo I18N("h",'To protect your privacy you can configure wireless security features.');?>
					<?echo I18N("h",'This device supports three wireless security modes: WEP, WPA and WPA2.');?>
				</cite>
			</td>
		</tr>
<!-- 2.4G: START -->
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Wireless Network Settings");?></p></td>
		</tr>
		<tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Band");?> :</td>
			<td width="76%" class="gray_bg border_right"><?echo I18N("h","2.4GHz Band");?></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Wireless");?> :</td>
			<td class="gray_bg border_right">
				<input id="en_wifi" type="checkbox" onClick="PAGE.OnClickEnWLAN('');" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Network Name");?> :</td>
			<td class="gray_bg border_right">
				<input id="ssid" type="text" size="20" maxlength="32" class="text_block" />
				(<?echo I18N("h","Also called the SSID");?>)
				<?drawlabel("ssid");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","802.11 Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="wlan_mode" onChange="PAGE.OnChangeWLMode('');">
					<option value="b">802.11b only</option>
					<option value="g">802.11g only</option>
					<option value="n">802.11n only</option>
					<option value="bg">Mixed 802.11g and 802.11b</option>
					<option value="gn">Mixed 802.11n and 802.11g</option>
					<option value="bgn">Mixed 802.11n, 802.11g and 802.11b</option>
				</select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Auto Channel Scan");?> :</td>
			<td class="gray_bg border_right">
				<input id="auto_ch" type="checkbox" onClick="PAGE.OnClickEnAutoChannel('');" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Channel");?> :</td>
			<td class="gray_bg border_right">
				<select id="channel" onChange="PAGE.OnChangeChannel('');">
<?
$clist = WIFI_getchannellist("g");
$count = cut_count($clist, ",");

$i = 0;
while($i < $count)
{
	$ch = cut($clist, $i, ',');
	$str = $ch;
	//for 2.4 Ghz
	if      ($ch=="1")   { $str = "2.412 GHz - CH 1";  }
	else if ($ch=="2")   { $str = "2.417 GHz - CH 2";  }
	else if ($ch=="3")   { $str = "2.422 GHz - CH 3";  }
	else if ($ch=="4")   { $str = "2.427 GHz - CH 4";  }
	else if ($ch=="5")   { $str = "2.432 GHz - CH 5";  }
	else if ($ch=="6")   { $str = "2.437 GHz - CH 6";  }
	else if ($ch=="7")   { $str = "2.442 GHz - CH 7";  }
	else if ($ch=="8")   { $str = "2.447 GHz - CH 8";  }
	else if ($ch=="9")   { $str = "2.452 GHz - CH 9";  }
	else if ($ch=="10")  { $str = "2.457 GHz - CH 10"; }
	else if ($ch=="11")  { $str = "2.462 GHz - CH 11"; }
	else if ($ch=="12")  { $str = "2.467 GHz - CH 12"; }
	else if ($ch=="13")  { $str = "2.472 GHz - CH 13"; }
	else if ($ch=="14")  { $str = "2.484 GHz - CH 14"; }

	echo '\t\t\t\t<option value="'.$ch.'">'.$str.'</option>\n';
	$i++;
}
?>          </select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Transmission Rate");?> :</td>
			<td class="gray_bg border_right">
				<select id="txrate">
					<option value="-1"><?echo I18N("h","Best")." (".I18N("h","automatic").")";?></option>
				</select>
				(Mbit/s)
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Channel Width");?> :</td>
			<td class="gray_bg border_right">
				<select id="bw">
					<option value="20">20 MHz</option>
					<option value="20+40">20/40 MHz(<?echo I18N("h","Auto");?>)</option>
				</select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Visibility Status");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" class="name" id="ssid_visible" name="invisible_type" value="visible" />
				<label for="ssid_visible"><?echo I18N("h","Visible");?></label>
				<input type="radio" class="name" id="ssid_invisible" name="invisible_type" value="invisible" />
				<label for="ssid_invisible"><?echo I18N("h","Invisible");?></label>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Wireless Security Mode");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Security Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="security_type" onChange="PAGE.OnChangeSecurityType('');">
					<option value=""><?echo I18N("h","None");?></option>
					<option value="wep"><?echo I18N("h","WEP");?></option>
					<option value="wpa_personal"><?echo I18N("h","WPA");?></option>
<!--					<option value="wpa_enterprise"><?echo I18N("h","WPA-Enterprise");?></option>  -->
				</select>
			</td>
		</tr>
		<tr id="wep" name="wep">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WEP");?></p></td>
		</tr>
		<tr id="wep" name="wep">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","WEP is the wireless encryption standard.")." ".
						I18N("h","To use it you must enter the same key(s) into the router and the wireless stations.")." ".
						I18N("h","For 64-bit keys you must enter 10 hex digits into each key box.")." ".
						I18N("h","For 128-bit keys you must enter 26 hex digits into each key box. ")." ".
						I18N("h","A hex digit is either a number from 0 to 9 or a letter from A to F.")." ".
						I18N("h",'For the most secure use of WEP set the authentication type to "Shared Key" when WEP is enabled.');?></cite>
				<cite><?echo I18N("h","You may also enter any text string into a WEP key box, in which case it will be converted into a hexadecimal key using the ASCII values of the characters.")." ".
						I18N("h","A maximum of 5 text characters can be entered for 64-bit keys, and a maximum of 13 characters for 128-bit keys.");?></cite>

				<cite><?echo I18N("","If you choose the WEP security option this device will <strong>ONLY</strong> operate in <strong>Legacy Wireless mode (802.11B/G)</strong>.")." ".
						I18N("","This means you will <strong>NOT</strong> get 11N performance due to the fact that WEP is not supported by the Draft 11N specification.");?></cite>
			</td>
		</tr>
		<tr id="wep" name="wep">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Authentication");?> :</td>
			<td class="gray_bg border_right">
				<select id="auth_type">
					<option value="WEPAUTO">Both</option>
					<option value="SHARED">Shared Key</option>
				</select>
			</td>
		</tr>
		<tr id="wep" name="wep">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WEP Key Length");?> :</td>
			<td class="gray_bg border_right">
				<select id="wep_key_len" onChange="PAGE.OnChangeWEPKey('');">
					<option value="64">64 bit (10 hex digits)</option>
					<option value="128">128 bit (26 hex digits)</option>
				</select>
				(length applies to all keys)
			</td>
		</tr>
		<tr id="wep" name="wep">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Default WEP Key");?> :</td>
			<td class="gray_bg border_right">
				<select id="wep_def_key" onChange="PAGE.OnChangeWEPKey('');">
					<option value="1">WEP Key 1</option>
					<option value="2">WEP Key 2</option>
					<option value="3">WEP Key 3</option>
					<option value="4">WEP Key 4</option>
				</select>
			</td>
		</tr>
		<tr id="wep" name="wep">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WEP Key");?> :</td>
			<td class="gray_bg border_right">
				<input id="wepkey_64" name="wepkey_64" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64" name="wepkey_64" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64" name="wepkey_64" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64" name="wepkey_64" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_128" name="wepkey_128" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128" name="wepkey_128" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128" name="wepkey_128" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128" name="wepkey_128" type="text" size="31" maxlength="26" class="text_block" />
				<?drawlabel("wepkey_64");?>
				<?drawlabel("wepkey_128");?>
			</td>
		</tr>
		<tr id="wpa" name="wpa">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WPA Setting");?></p></td>
		</tr>
		<tr id="wpa" name="wpa">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("","Use <strong>WPA or WPA2</strong> mode to achieve a balance of strong security and best compatibility.")." ".
						I18N("","This mode uses WPA for legacy clients while maintaining higher security with stations that are WPA2 capable.")." ".
						I18N("","Also the strongest cipher that the client supports will be used. For best security, use <strong>WPA2 Only</strong> mode.")." ".
						I18N("","This mode uses AES(CCMP) cipher and legacy stations are not allowed access with WPA security.")." ".
						I18N("","For maximum compatibility, use <strong>WPA Only</strong>. This mode uses TKIP cipher. Some gaming and legacy devices work only in this mode.");?></cite>
				<cite><?echo I18N("","To achieve better wireless performance use <strong>WPA2 Only</strong> security mode (or in other words AES cipher).");?></cite>
			</td>
		</tr>
		<tr id="wpa" name="wpa">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WPA Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="wpa_mode" onChange="PAGE.OnChangeWPAMode('');">
					<option value="WPA+2">Auto(WPA or WPA2)</option>
					<option value="WPA2">WPA2 Only</option>
					<option value="WPA">WPA Only</option>
				</select>
			</td>
		</tr>
		<tr id="wpa" name="wpa">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Cipher Type");?> :</td>
			<td class="gray_bg border_right">
				<select id="cipher_type">
					<option value="TKIP">TKIP</option>
					<option value="AES">AES</option>
					<option value="TKIP+AES">TKIP and AES</option>
				</select>
			</td>
		</tr>
		<tr id="wpa" name="wpa">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Group Key Update Interval");?> :</td>
			<td class="gray_bg border_right">
				<input id="wpa_grp_key_intrv" type="text" size="12" maxlength="10" class="text_block" />
				(<?echo I18N("h","seconds");?>)
				<?drawlabel("wpa_grp_key_intrv");?>
			</td>
		</tr>
		<tr id="wpapsk" name="wpapsk">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Pre-Shared Key");?></p></td>
		</tr>
		<tr id="wpapsk" name="wpapsk">
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","Enter an 8- to 63-character alphanumeric pass-phrase.");?>
					<?echo I18N("h","For stronger security it should be of ample length and should not be a commonly known phrase.");?>
				</cite>
			</td>
		</tr>
		<tr id="wpapsk" name="wpapsk">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Pre-Shared Key");?> :</td>
			<td class="gray_bg border_right">
				<input id="wpa_psk_key" type="text" size="20" maxlength="63" class="text_block" />
				<?drawlabel("wpa_psk_key");?>
			</td>
		</tr>
		<tr id="wpaeap" name="wpaeap">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","EAP");?> (802.1x)</p></td>
		</tr>
		<tr id="wpaeap" name="wpaeap">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.");?></cite>
			</td>
		</tr>
		<tr id="wpaeap" name="wpaeap">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("radius_srv_ip");?>
				<?drawlabel("radius_srv_ip");?>
			</td>
		</tr>
		<tr id="wpaeap" name="wpaeap">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server Port");?> :</td>
			<td class="gray_bg border_right">
				<input id="radius_srv_port" type="text" size="5" maxlength="5" class="text_block" />
				<?drawlabel("radius_srv_port");?>
			</td>
		</tr>
		<tr id="wpaeap" name="wpaeap">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server Shared Secret");?> :</td>
			<td class="gray_bg border_right">
				<input id="radius_srv_sec" type="password" size="50" maxlength="64" class="text_block" />
				<?drawlabel("radius_srv_sec");?>
			</td>
		</tr>
<!-- 2.4G: END-->
<!-- 5G: START -->
		<tr id="11a_start">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Wireless Network Settings");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Band");?> :</td>
			<td class="gray_bg border_right"><?echo I18N("h","5GHz Band");?></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Wireless");?> :</td>
			<td class="gray_bg border_right">
				<input id="en_wifi_11a" type="checkbox" onClick="PAGE.OnClickEnWLAN('_11a');" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Network Name");?> :</td>
			<td class="gray_bg border_right">
				<input id="ssid_11a" type="text" size="20" maxlength="32" class="text_block" />
				(<?echo I18N("h","Also called the SSID");?>)
				<?drawlabel("ssid_11a");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","802.11 Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="wlan_mode_11a" onChange="PAGE.OnChangeWLMode('_11a');">
					<option value="a">802.11a only</option>
					<option value="n">802.11n only</option>
					<option value="an">Mixed 802.11a and 802.11n</option>
				</select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Auto Channel Scan");?> :</td>
			<td class="gray_bg border_right">
				<input id="auto_ch_11a" type="checkbox" onClick="PAGE.OnClickEnAutoChannel('_11a');" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Channel");?> :</td>
			<td class="gray_bg border_right">
				<select id="channel_11a" onChange="PAGE.OnChangeChannel('_11a');">
<?
//++++ hendry add for dfs
function execute_cmd($cmd)
{
	fwrite("w","/var/run/exec.sh",$cmd);
	event("EXECUTE");
}

function setToRuntimeNode($blocked_channel, $timeleft)
{
	/* find blocked channel if already in runtime node */
	$blocked_chn_total = query("/runtime/dfs/blocked/entry#");
	/* if blocked channel exist before, use the old index. */
	$index = 1;
	while($index <= $blocked_chn_total)
	{
		if($blocked_chn_total == 0) {break;}
		$ch = query("/runtime/dfs/blocked/entry:".$index."/channel");
		if($ch == $blocked_channel)
		{
			break;
		}
		$index++;
	}
	set("/runtime/dfs/blocked/entry:".$index."/channel",$blocked_channel);
	execute_cmd("xmldbc -t \"dfs-".$blocked_channel.":".$timeleft.":xmldbc -X /runtime/dfs/blocked/entry:".$index."\"");
	//execute_cmd("xmldbc -t \"dfs-".$blocked_channel.":5:xmldbc -X /runtime/dfs/blocked/entry:".$index."\"");
}

/*1. Update new blocked channel to runtime nodes */
$blockch_list = fread("", "/proc/dfs_blockch");
//format is : "100,960;122,156;" --> channel 100, remaining time is 960 seconds
//                               --> channel 122, remaining time is 156 seconds
$ttl_block_chn = cut_count($blockch_list, ";")-1;
$i = 0;
while($i < $ttl_block_chn)
{
	//assume that blocked channel can be more than one channel.
	$ch_field = cut($blockch_list, $i, ';');    //i mean each "100,960;" represent 1 field
	$ch = cut ($ch_field, 0, ',');
	$remaining_time = cut ($ch_field, 1, ',');

	setToRuntimeNode($ch, $remaining_time);
	$i++;
}

$clist = WIFI_getchannellist("a");
$count = cut_count($clist, ",");

$i = 0;
while($i < $count)
{
	$ch = cut($clist, $i, ',');
	$str = $ch;
	//for 5 Ghz
	if      ($ch=="34")    { $str = "5.170 GHz - CH 34";   }
	else if ($ch=="38")    { $str = "5.190 GHz - CH 38";   }
	else if ($ch=="42")    { $str = "5.210 GHz - CH 42";   }
	else if ($ch=="46")    { $str = "5.230 GHz - CH 46";   }

	else if ($ch=="36")    { $str = "5.180 GHz - CH 36";   }
	else if ($ch=="40")    { $str = "5.200 GHz - CH 40";   }
	else if ($ch=="44")    { $str = "5.220 GHz - CH 44";   }
	else if ($ch=="48")    { $str = "5.240 GHz - CH 48";   }
	else if ($ch=="52")    { $str = "5.260 GHz - CH 52";   }
	else if ($ch=="56")    { $str = "5.280 GHz - CH 56";   }
	else if ($ch=="60")    { $str = "5.300 GHz - CH 60";   }
	else if ($ch=="64")    { $str = "5.320 GHz - CH 64";   }
	else if ($ch=="100")   { $str = "5.500 GHz - CH 100";  }
	else if ($ch=="104")   { $str = "5.520 GHz - CH 104";  }
	else if ($ch=="108")   { $str = "5.540 GHz - CH 108";  }
	else if ($ch=="112")   { $str = "5.560 GHz - CH 112";  }
	else if ($ch=="116")   { $str = "5.580 GHz - CH 116";  }
	else if ($ch=="120")   { $str = "5.600 GHz - CH 120";  }
	else if ($ch=="124")   { $str = "5.620 GHz - CH 124";  }
	else if ($ch=="128")   { $str = "5.640 GHz - CH 128";  }
	else if ($ch=="132")   { $str = "5.660 GHz - CH 132";  }
	else if ($ch=="136")   { $str = "5.680 GHz - CH 136";  }
	else if ($ch=="140")   { $str = "5.700 GHz - CH 140";  }

	else if ($ch=="149")   { $str = "5.745 GHz - CH 149";  }
	else if ($ch=="153")   { $str = "5.765 GHz - CH 153";  }
	else if ($ch=="157")   { $str = "5.785 GHz - CH 157";  }
	else if ($ch=="161")   { $str = "5.805 GHz - CH 161";  }
	else if ($ch=="165")   { $str = "5.825 GHz - CH 165";  }
	else if ($ch=="184")   { $str = "4.920 GHz - CH 184";  }
	else if ($ch=="188")   { $str = "4.940 GHz - CH 188";  }
	else if ($ch=="192")   { $str = "4.960 GHz - CH 192";  }
	else if ($ch=="196")   { $str = "4.980 GHz - CH 196";  }
	else { $str = $ch ; }

	/* check all channel for A band whether it is blocked (radar detected) */
	$ttl_blocked_chn = query("/runtime/dfs/blocked/entry#");
	$ct=1;
	$channel_is_blocked = 0;
	while($ct <= $ttl_blocked_chn)
	{
		if($ttl_blocked_chn == 0) {break;}
		$blocked_ch = query("/runtime/dfs/blocked/entry:".$ct."/channel");
		if($ch == $blocked_ch)
		{
			$channel_is_blocked = 1;
			break;
		}
		$ct++;
	}

	if($channel_is_blocked=="1")
	{
		echo '\t\t\t\t<option value="'.$ch.'">'.$str.' (disabled) </option>\n';
	}
	else
	{
		echo '\t\t\t\t<option value="'.$ch.'">'.$str.'</option>\n';
	}
	$i++;
}
?>				</select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Transmission Rate");?> :</td>
			<td class="gray_bg border_right">
				<select id="txrate_11a">
					<option value="-1"><?echo I18N("h","Best")." (".I18N("h","automatic").")";?></option>
				</select>
				(Mbit/s)
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Channel Width");?> :</td>
			<td class="gray_bg border_right">
				<select id="bw_11a">
					<option value="20">20 MHz</option>
					<option value="20+40">20/40 MHz(<?echo I18N("h","Auto");?>)</option>
				</select>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Visibility Status");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" class="name" id="ssid_visible_11a" name="invisible_type" value="visible" />
				<label for="ssid_visible"><?echo I18N("h","Visible");?></label>
				<input type="radio" class="name" id="ssid_invisible_11a" name="invisible_type" value="invisible" />
				<label for="ssid_invisible"><?echo I18N("h","Invisible");?></label>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Wireless Security Mode");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Security Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="security_type_11a" onChange="PAGE.OnChangeSecurityType('_11a');">
					<option value=""><?echo I18N("h","None");?></option>
					<option value="wep"><?echo I18N("h","WEP");?></option>
					<option value="wpa_personal"><?echo I18N("h","WPA");?></option>
<!--					<option value="wpa_enterprise"><?echo I18N("h","WPA-Enterprise");?></option>  -->
				</select>
			</td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WEP");?></p></td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","WEP is the wireless encryption standard.")." ".
						I18N("h","To use it you must enter the same key(s) into the router and the wireless stations.")." ".
						I18N("h","For 64-bit keys you must enter 10 hex digits into each key box.")." ".
						I18N("h","For 128-bit keys you must enter 26 hex digits into each key box. ")." ".
						I18N("h","A hex digit is either a number from 0 to 9 or a letter from A to F.")." ".
						I18N("h",'For the most secure use of WEP set the authentication type to "Shared Key" when WEP is enabled.');?></cite>
				<cite><?echo I18N("h","You may also enter any text string into a WEP key box, in which case it will be converted into a hexadecimal key using the ASCII values of the characters.")." ".
						I18N("h","A maximum of 5 text characters can be entered for 64-bit keys, and a maximum of 13 characters for 128-bit keys.");?></cite>

				<cite><?echo I18N("","If you choose the WEP security option this device will <strong>ONLY</strong> operate in <strong>Legacy Wireless mode (802.11B/G)</strong>.")." ".
						I18N("","This means you will <strong>NOT</strong> get 11N performance due to the fact that WEP is not supported by the Draft 11N specification.");?></cite>
			</td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Authentication");?> :</td>
			<td class="gray_bg border_right">
				<select id="auth_type_11a">
					<option value="OPEN">Open</option>
					<option value="WEPAUTO">Both</option>
					<option value="SHARED">Shared Key</option>
				</select>
			</td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WEP Key Length");?> :</td>
			<td class="gray_bg border_right">
				<select id="wep_key_len_11a" onChange="PAGE.OnChangeWEPKey('_11a');">
					<option value="64">64 bit (10 hex digits)</option>
					<option value="128">128 bit (26 hex digits)</option>
				</select>
				(length applies to all keys)
			</td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Default WEP Key");?> :</td>
			<td class="gray_bg border_right">
				<select id="wep_def_key_11a" onChange="PAGE.OnChangeWEPKey('_11a');">
					<option value="1">WEP Key 1</option>
					<option value="2">WEP Key 2</option>
					<option value="3">WEP Key 3</option>
					<option value="4">WEP Key 4</option>
				</select>
			</td>
		</tr>
		<tr id="wep_11a" name="wep_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WEP Key");?> :</td>
			<td class="gray_bg border_right">
				<input id="wepkey_64_11a" name="wepkey_64_11a" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64_11a" name="wepkey_64_11a" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64_11a" name="wepkey_64_11a" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_64_11a" name="wepkey_64_11a" type="text" size="15" maxlength="10" class="text_block" />
				<input id="wepkey_128_11a" name="wepkey_128_11a" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128_11a" name="wepkey_128_11a" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128_11a" name="wepkey_128_11a" type="text" size="31" maxlength="26" class="text_block" />
				<input id="wepkey_128_11a" name="wepkey_128_11a" type="text" size="31" maxlength="26" class="text_block" />
				<?drawlabel("wepkey_64_11a");?>
				<?drawlabel("wepkey_128_11a");?>
			</td>
		</tr>
		<tr id="wpa_11a" name="wpa_11a">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WPA Setting");?></p></td>
		</tr>
		<tr id="wpa_11a" name="wpa_11a">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("","Use <strong>WPA or WPA2</strong> mode to achieve a balance of strong security and best compatibility.")." ".
						I18N("","This mode uses WPA for legacy clients while maintaining higher security with stations that are WPA2 capable.")." ".
						I18N("","Also the strongest cipher that the client supports will be used. For best security, use <strong>WPA2 Only</strong> mode.")." ".
						I18N("","This mode uses AES(CCMP) cipher and legacy stations are not allowed access with WPA security.")." ".
						I18N("","For maximum compatibility, use <strong>WPA Only</strong>. This mode uses TKIP cipher. Some gaming and legacy devices work only in this mode.");?></cite>
				<cite><?echo I18N("","To achieve better wireless performance use <strong>WPA2 Only</strong> security mode (or in other words AES cipher).");?></cite>
			</td>
		</tr>
		<tr id="wpa_11a" name="wpa_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WPA Mode");?> :</td>
			<td class="gray_bg border_right">
				<select id="wpa_mode_11a" onChange="PAGE.OnChangeWPAMode('_11a');">
					<option value="WPA+2">Auto(WPA or WPA2)</option>
					<option value="WPA2">WPA2 Only</option>
					<option value="WPA">WPA Only</option>
				</select>
			</td>
		</tr>
		<tr id="wpa_11a" name="wpa_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Cipher Type");?> :</td>
			<td class="gray_bg border_right">
				<select id="cipher_type_11a">
					<option value="TKIP">TKIP</option>
					<option value="AES">AES</option>
					<option value="TKIP+AES">TKIP and AES</option>
				</select>
			</td>
		</tr>
		<tr id="wpa_11a" name="wpa_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Group Key Update Interval");?> :</td>
			<td class="gray_bg border_right">
				<input id="wpa_grp_key_intrv_11a" type="text" size="12" maxlength="10" class="text_block" />
				(<?echo I18N("h","seconds");?>)
				<?drawlabel("wpa_grp_key_intrv_11a");?>
			</td>
		</tr>
		<tr id="wpapsk_11a" name="wpapsk_11a">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Pre-Shared Key");?></p></td>
		</tr>
		<tr id="wpapsk_11a" name="wpapsk_11a">
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","Enter an 8- to 63-character alphanumeric pass-phrase.");?>
					<?echo I18N("h","For stronger security it should be of ample length and should not be a commonly known phrase.");?>
				</cite>
			</td>
		</tr>
		<tr id="wpapsk_11a" name="wpapsk_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Pre-Shared Key");?> :</td>
			<td class="gray_bg border_right">
				<input id="wpa_psk_key_11a" type="text" size="20" maxlength="64" class="text_block" />
				<?drawlabel("wpa_psk_key_11a");?>
			</td>
		</tr>
		<tr id="wpaeap_11a" name="wpaeap_11a">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","EAP");?> (802.1x)</p></td>
		</tr>
		<tr id="wpaeap_11a" name="wpaeap_11a">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.");?></cite>
			</td>
		</tr>
		<tr id="wpaeap_11a" name="wpaeap_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("radius_srv_ip_11a");?>
				<?drawlabel("radius_srv_ip_11a");?>
			</td>
		</tr>
		<tr id="wpaeap_11a" name="wpaeap_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server Port");?> :</td>
			<td class="gray_bg border_right">
				<input id="radius_srv_port_11a" type="text" size="5" maxlength="5" class="text_block" />
				<?drawlabel("radius_srv_port_11a");?>
			</td>
		</tr>
		<tr id="wpaeap_11a" name="wpaeap_11a">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RADIUS Server Shared Secret");?> :</td>
			<td class="gray_bg border_right">
				<input id="radius_srv_sec_11a" type="password" size="50" maxlength="64" class="text_block" />
				<?drawlabel("radius_srv_sec_11a");?>
			</td>
		</tr>
<!-- 5G: END-->
		<tr id="11a_end">
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
