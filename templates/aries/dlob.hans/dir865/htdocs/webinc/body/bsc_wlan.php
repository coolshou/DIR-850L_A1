<?
include "/htdocs/webinc/body/draw_elements.php";
include "/htdocs/phplib/wifi.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Wireless Network");?></h1>
	<p><?echo i18n("Use this section to configure the wireless settings for your D-Link router.")." ".
		i18n("Please note that changes made in this section may also need to be duplicated on your wireless client. ");?></p>
	<p><?echo i18n("To protect your privacy you can configure wireless security features.")." ".
		i18n("This device supports three wireless security modes including: WEP, WPA and WPA2.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>

<!-- ===================== 2.4Ghz, BG band ============================== -->
<div id="div_24G" class="blackbox">
	<h2><?echo i18n("Wireless Network Settings");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Band");?></span>
		<span class="delimiter">:</span>
		<span class="value"><b><?echo i18n("2.4GHz Band");?></b></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Wireless");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="en_wifi" type="checkbox" onClick="PAGE.OnClickEnWLAN('');" />
			<?
			if ($FEATURE_NOSCH!="1")
			{
				DRAW_select_sch("sch", i18n("Always"), "", "", "0", "narrow");
				echo '<input id="go2sch" type="button" value="'.i18n("New Schedule").'" onClick="javascript:self.location.href=\'./tools_sch.php\';" />\n';
			}
			?>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Network Name");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="ssid" type="text" size="20" maxlength="32" />
			(<?echo i18n("Also called the SSID");?>)
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("802.11 Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wlan_mode" onChange="PAGE.OnChangeWLMode('');">
				<option value="b">802.11b only</option>
                <option value="g">802.11g only</option>
				<option value="n">802.11n only</option>
				<option value="bg">Mixed 802.11g and 802.11b</option>
                <option value="gn">Mixed 802.11n and 802.11g</option>
				<option value="bgn">Mixed 802.11n, 802.11g and 802.11b</option>
			</select>
		</span>
	</div>	
	
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Auto Channel Scan");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="auto_ch" type="checkbox" onClick="PAGE.OnClickEnAutoChannel('');" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Channel");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="channel" onChange="PAGE.OnChangeChannel('');">
<?
	$clist = WIFI_getchannellist();
	$count = cut_count($clist, ",");
	
	$i = 0;
	while($i < $count)
	{
		$ch = cut($clist, $i, ',');
		$str = $ch;
		//TRACE_error("11123");
		//for 2.4 Ghz
		if		($ch=="1")	 { $str = "2.412 GHz - CH 1";  } 
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
		
		//for 5 Ghz
		else if	($ch=="34")	   { $str = "5.170 GHz - CH 34";   } 
		else if	($ch=="38")	   { $str = "5.190 GHz - CH 38";   } 
		else if	($ch=="42")	   { $str = "5.210 GHz - CH 42";   } 
		else if	($ch=="46")	   { $str = "5.230 GHz - CH 46";   }
		
		else if	($ch=="36")	   { $str = "5.180 GHz - CH 36";   } 
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
		
		echo '\t\t\t\t<option value="'.$ch.'">'.$str.'</option>\n';
		$i++;
	}
	
?>			</select>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Transmission Rate");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="txrate">
				<option value="-1"><?echo i18n("Best")." (".i18n("automatic").")";?></option>
			</select>
			(Mbit/s)
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Channel Width");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="bw">
				<option value="20">20 MHz</option>
				<option value="20+40">20/40 MHz(<?echo i18n("Auto");?>)</option>
			</select>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Visibility Status");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input type="radio" class="name" id="ssid_visible" 		name="invisible_type" value="visible" 	onclick="PAGE.OnClickInvinsible('');" /> <label><?echo i18n("Visible");?></label>
	        <input type="radio" class="name" id="ssid_invisible" 	name="invisible_type" value="invisible" onclick="PAGE.OnClickInvinsible('');" /> <label><?echo i18n("Invisible");?></label>
		</span>
	</div>
	<div class="textinput" style="display:none;">
		<span class="name"><?echo i18n("WMM Enable");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="en_wmm" type="checkbox" />
			(<?echo i18n("Wireless QoS");?>)
		</span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("Wireless Security Mode");?></h2>
	<div class="textinput">
		<span class="name" <? if(query("/runtime/device/langcode")!="en") echo 'style="width: 28%;"';?> ><?echo i18n("Security Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="security_type" onChange="PAGE.OnChangeSecurityType('');">
				<option value=""><?echo i18n("None");?></option>
				<option value="wep"><?echo i18n("WEP");?></option>
				<option value="wpa_personal"><?echo i18n("WPA-Personal");?></option>
				<option value="wpa_enterprise"><?echo i18n("WPA-Enterprise");?></option>
			</select>
		</span>
	</div>
	<div class="gap"></div>
</div>
<div id="wep" class="blackbox" style="display:none;">
	<h2><?echo i18n("WEP");?></h2>
	<p><?echo i18n("WEP is the wireless encryption standard.")." ".
		i18n("To use it you must enter the same key(s) into the router and the wireless stations.")." ".
		i18n("For 64-bit keys you must enter 10 hex digits into each key box.")." ".
		i18n("For 128-bit keys you must enter 26 hex digits into each key box. ")." ".
		i18n("A hex digit is either a number from 0 to 9 or a letter from A to F.")." ".
		i18n('For the most secure use of WEP set the authentication type to "Shared Key" when WEP is enabled.');?></p>
	<p><?echo i18n("You may also enter any text string into a WEP key box, in which case it will be converted into a hexadecimal key using the ASCII values of the characters.")." ".
		i18n("A maximum of 5 text characters can be entered for 64-bit keys, and a maximum of 13 characters for 128-bit keys.");?></p>

	<p><?echo i18n("If you choose the WEP security option this device will <strong>ONLY</strong> operate in <strong>Legacy Wireless mode (802.11B/G)</strong>.")." ".
		i18n("This means you will <strong>NOT</strong> get 11N performance due to the fact that WEP is not supported by the Draft 11N specification.");?></p>
		

	<div class="textinput">
		<span class="name"><?echo i18n("WEP Key Length");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_key_len" onChange="PAGE.OnChangeWEPKey('');">
				<option value="64">64 bit (10 hex digits)</option>
				<option value="128">128 bit (26 hex digits)</option>
			</select>
			(length applies to all keys)
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Authentication");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="auth_type" onChange="PAGE.OnChangeWEPAuth('');">
				<!--<option value="OPEN">Open</option>-->
				<option value="WEPAUTO">Both</option>
				<option value="SHARED">Shared Key</option>
			</select>
		</span>
	</div>
	<div class="textinput" style="display:none;">
		<span class="name"><?echo i18n("Default WEP Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_def_key" onChange="PAGE.OnChangeWEPKey('');">
				<option value="1">WEP Key 1</option>
				<option value="2">WEP Key 2</option>
				<option value="3">WEP Key 3</option>
				<option value="4">WEP Key 4</option>
			</select>
		</span>
	</div>
	<div id="wep_64" class="textinput">
		<span class="name"><?echo i18n("WEP Key 1");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wep_64_1" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_2" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_3" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_4" name="wepkey_64" type="text" size="15" maxlength="10" />
		</span>
	</div>
	<div id="wep_128" class="textinput">
		<span class="name"><?echo i18n("WEP Key 1");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wep_128_1" name="wepkey_128" type="text" size="31" maxlength="26" />
			<input id="wep_128_2" name="wepkey_128" type="text" size="31" maxlength="26" />
			<input id="wep_128_3" name="wepkey_128" type="text" size="31" maxlength="26" />
			<input id="wep_128_4" name="wepkey_128" type="text" size="31" maxlength="26" />
		</span>
	</div>
	
	<div class="gap"></div>
</div>

<div id="box_wpa" class="blackbox" style="display:none;">
	<h2><?echo i18n("WPA");?></h2>
	<p><?echo i18n("Use <strong>WPA or WPA2</strong> mode to achieve a balance of strong security and best compatibility.")." ".
		i18n("This mode uses WPA for legacy clients while maintaining higher security with stations that are WPA2 capable.")." ".
		i18n("Also the strongest cipher that the client supports will be used. For best security, use <strong>WPA2 Only</strong> mode.")." ".
		i18n("This mode uses AES(CCMP) cipher and legacy stations are not allowed access with WPA security.")." ".
		i18n("For maximum compatibility, use <strong>WPA Only</strong>. This mode uses TKIP cipher. Some gaming and legacy devices work only in this mode.");?></p>
	<p><?echo i18n("To achieve better wireless performance use <strong>WPA2 Only</strong> security mode (or in other words AES cipher).");?></p>

	<div class="textinput">
		<span class="name"><?echo i18n("WPA Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wpa_mode" onChange="PAGE.OnChangeWPAMode('');">
				<option value="WPA+2"><?echo i18n("Auto(WPA or WPA2)");?></option>
				<option value="WPA2"><?echo i18n("WPA2 Only");?></option>
				<option value="WPA"><?echo i18n("WPA Only");?></option>
			</select>
		</span>
	</div>		
	<div class="textinput">
		<span class="name"><?echo i18n("Cipher Type");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="cipher_type">
				<option value="TKIP">TKIP</option>
				<option value="AES">AES</option>
				<option value="TKIP+AES">TKIP and AES</option>
			</select>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Group Key Update Interval");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wpa_grp_key_intrv" type="text" size="20" maxlength="10" /> (seconds)
		</span>
	</div>	
	<div class="gap"></div>
</div>

<div id="box_wpa_personal" class="blackbox" style="display:none;">
	<h2><?echo i18n("Pre-Shared Key");?></h2>
	<p class="strong"><?echo i18n("Enter an 8- to 63-character alphanumeric pass-phrase.")." ".
		i18n("For good security it should be of ample length and should not be a commonly known phrase.");?>
	</p>
	<div class="textinput">
		<span class="name"><?echo i18n("Pre-Shared Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wpa_psk_key" type="text" size="20" maxlength="64" />
		</span>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>	
	</div>


<div id="box_wpa_enterprise" class="blackbox" style="display:none;">
	<h2><?echo i18n("EAP (802.1x)");?></h2>
	<p class="strong"><?echo i18n("<strong>When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.</strong>");?></p>
	<div class="textinput">
		<span class="name"><?echo i18n("RADIUS server IP Address");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="radius_srv_ip" type="text" size="15" maxlength="15" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("RADIUS server Port");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="radius_srv_port" type="text" size="5" maxlength="5" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("RADIUS server Shared Secret");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="radius_srv_sec" type="password" size="50" maxlength="64" /></span>
	</div>

	<div class="textinput">
        <span class="delimiter"></span>
        <span class="value"><input type="button" id="radius_adv" name="radius_adv" value="Advanced >>" onClick="PAGE.OnClickRadiusAdvanced('');"></span>
	</div>
	
	<div id="div_second_radius" name="div_second_radius" style="display:none;">
		<div class="textinput">
			<p class="strong"><?echo i18n("<strong>Optional backup RADIUS server</strong>");?></p>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Second RADIUS server IP Address");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_ip_second" type="text" size="15" maxlength="15" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Second RADIUS server Port");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_port_second" type="text" size="5" maxlength="5" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Second RADIUS server Shared Secret");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_sec_second" type="password" size="50" maxlength="64" /></span>
		</div>
	</div>	
	<div class="gap"></div>
	<div class="gap"></div>
</div>

<!-- ===================== 5Ghz, A band ============================== -->
<div id="div_5G" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("Wireless Network Settings");?></h2>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Band");?></span>
			<span class="delimiter">:</span>
			<span id="wifi_mode_Aband" class="value"><b><?echo i18n("5GHz Band");?></b></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Enable Wireless");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="en_wifi_Aband" type="checkbox" onClick="PAGE.OnClickEnWLAN('_Aband');" />
				<?
				if ($FEATURE_NOSCH!="1")
				{
					DRAW_select_sch("sch_Aband", i18n("Always"), "", "", "0", "narrow");
					echo '<input id="go2sch_Aband" type="button" value="'.i18n("New Schedule").'" onClick="javascript:self.location.href=\'./tools_sch.php\';" />\n';
				}
				?>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Network Name");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="ssid_Aband" type="text" size="20" maxlength="32" />
				(<?echo i18n("Also called the SSID");?>)
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("802.11 Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wlan_mode_Aband" onChange="PAGE.OnChangeWLMode('_Aband');">
					<option value="a">802.11a only</option>
	                <option value="n">802.11n only</option>
			<option value="an">Mixed 802.11a and 802.11n</option>
			<option value="ac">Mixed 802.11ac</option>
				</select>
			</span>
		</div>
		
		<div class="textinput">
			<span class="name"><?echo i18n("Enable Auto Channel Scan");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="auto_ch_Aband" type="checkbox" onClick="PAGE.OnClickEnAutoChannel('_Aband');" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Channel");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="channel_Aband" onChange="PAGE.OnChangeChannel('_Aband');">
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
				
			/*0. Update/delete any entry that has expired */
			/*
			$uptime = query("runtime/device/uptime");
			$index = 1;
			
			// delete any expired entry 
			while($index <= $ttl_blocked_chn)
			{
				if($ttl_blocked_chn == 0) {break;}
				$expiry = query("/runtime/dfs/blocked/entry:.".$index."/expiry");
				if($uptime > $expiry)
				{
					// delete this entry
					del("/runtime/dfs/blocked/entry:".$index);
				}
				$index++; 
			}
			*/
			
			/*1. Update new blocked channel to runtime nodes */
			$blockch_list = fread("", "/proc/dfs_blockch");
			//format is : "100,960;122,156;" --> channel 100, remaining time is 960 seconds
			//								 --> channel 122, remaining time is 156 seconds
			$ttl_block_chn = cut_count($blockch_list, ";")-1;
			$i = 0;
			while($i < $ttl_block_chn)
			{
				//assume that blocked channel can be more than one channel.
				$ch_field = cut($blockch_list, $i, ';');	//i mean each "100,960;" represent 1 field 
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
				//for 2.4 Ghz
				if		($ch=="1")	 { $str = "2.412 GHz - CH 1";  } 
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
				
				//for 5 Ghz
				else if	($ch=="34")	   { $str = "5.170 GHz - CH 34";   } 
				else if	($ch=="38")	   { $str = "5.190 GHz - CH 38";   } 
				else if	($ch=="42")	   { $str = "5.210 GHz - CH 42";   } 
				else if	($ch=="46")	   { $str = "5.230 GHz - CH 46";   }
				
				else if	($ch=="36")	   { $str = "5.180 GHz - CH 36";   } 
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
		
	?>			</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Transmission Rate");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="txrate_Aband">
					<option value="-1"><?echo i18n("Best")." (".i18n("automatic").")";?></option>
				</select>
				(Mbit/s)
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Channel Width");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="bw_Aband">
					<option value="20">20 MHz</option>
					<option value="20+40">20/40 MHz(<?echo i18n("Auto");?>)</option>
					<option value="20+40+80">20/40/80 MHz(<?echo i18n("Auto");?>)</option>
				</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Visibility Status");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="radio" class="name" id="ssid_visible_Aband" 	name="invisible_type_Aband" value="visible"  	/> <label>Visible</label>
		        <input type="radio" class="name" id="ssid_invisible_Aband" 	name="invisible_type_Aband" value="invisible" /> <label>Invisible</label>
			</span>
		</div>
		<div class="textinput" style="display:none;">
			<span class="name"><?echo i18n("WMM Enable");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="en_wmm_Aband" type="checkbox" />
				(<?echo i18n("Wireless QoS");?>)
			</span>
		</div>
		<div class="gap"></div>
	</div>
	<div class="blackbox">
		<h2><?echo i18n("Wireless Security Mode");?></h2>
		<div class="textinput">
			<span class="name" <? if(query("/runtime/device/langcode")!="en") echo 'style="width: 28%;"';?> ><?echo i18n("Security Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="security_type_Aband" onChange="PAGE.OnChangeSecurityType('_Aband');">
					<option value=""><?echo i18n("None");?></option>
					<option value="wep"><?echo i18n("WEP");?></option>
					<option value="wpa_personal"><?echo i18n("WPA-Personal");?></option>
					<option value="wpa_enterprise"><?echo i18n("WPA-Enterprise");?></option>
				</select>
			</span>
		</div>
		<div class="gap"></div>
	</div>
	<div id="wep_Aband" class="blackbox" style="display:none;">
		<h2><?echo i18n("WEP");?></h2>
		<p><?echo i18n("WEP is the wireless encryption standard.")." ".
			i18n("To use it you must enter the same key(s) into the router and the wireless stations.")." ".
			i18n("For 64-bit keys you must enter 10 hex digits into each key box.")." ".
			i18n("For 128-bit keys you must enter 26 hex digits into each key box. ")." ".
			i18n("A hex digit is either a number from 0 to 9 or a letter from A to F.")." ".
			i18n('For the most secure use of WEP set the authentication type to "Shared Key" when WEP is enabled.');?></p>
		<p><?echo i18n("You may also enter any text string into a WEP key box, in which case it will be converted into a hexadecimal key using the ASCII values of the characters.")." ".
			i18n("A maximum of 5 text characters can be entered for 64-bit keys, and a maximum of 13 characters for 128-bit keys.");?></p>
	
		<p><?echo i18n("If you choose the WEP security option this device will <strong>ONLY</strong> operate in <strong>Legacy Wireless mode (802.11B/G)</strong>.")." ".
			i18n("This means you will <strong>NOT</strong> get 11N performance due to the fact that WEP is not supported by the Draft 11N specification.");?></p>
			
	
		<div class="textinput">
			<span class="name"><?echo i18n("WEP Key Length");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wep_key_len_Aband" onChange="PAGE.OnChangeWEPKey('_Aband');">
					<option value="64">64 bit (10 hex digits)</option>
					<option value="128">128 bit (26 hex digits)</option>
				</select>
				(length applies to all keys)
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Authentication");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="auth_type_Aband" onChange="PAGE.OnChangeWEPAuth('_Aband');">
					<!--<option value="OPEN">Open</option>-->
					<option value="WEPAUTO">Both</option>
					<option value="SHARED">Shared Key</option>
				</select>
			</span>
		</div>	
		<div class="textinput" style="display:none;">
			<span class="name"><?echo i18n("Default WEP Key");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wep_def_key_Aband" onChange="PAGE.OnChangeWEPKey('_Aband');">
					<option value="1">WEP Key 1</option>
					<option value="2">WEP Key 2</option>
					<option value="3">WEP Key 3</option>
					<option value="4">WEP Key 4</option>
				</select>
			</span>
		</div>
		<div id="wep_64_Aband" class="textinput">
			<span class="name"><?echo i18n("WEP Key 1");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wep_64_1_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
				<input id="wep_64_2_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
				<input id="wep_64_3_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
				<input id="wep_64_4_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
			</span>
		</div>
		<div id="wep_128_Aband" class="textinput">
			<span class="name"><?echo i18n("WEP Key 1");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wep_128_1_Aband" name="wepkey_128_Aband" type="text" size="31" maxlength="26" />
				<input id="wep_128_2_Aband" name="wepkey_128_Aband" type="text" size="31" maxlength="26" />
				<input id="wep_128_3_Aband" name="wepkey_128_Aband" type="text" size="31" maxlength="26" />
				<input id="wep_128_4_Aband" name="wepkey_128_Aband" type="text" size="31" maxlength="26" />
			</span>
		</div>
		
		<div class="gap"></div>
	</div>
	
	<div id="box_wpa_Aband" class="blackbox" style="display:none;">
		<h2><?echo i18n("WPA");?></h2>
		<p><?echo i18n("Use <strong>WPA or WPA2</strong> mode to achieve a balance of strong security and best compatibility.")." ".
			i18n("This mode uses WPA for legacy clients while maintaining higher security with stations that are WPA2 capable.")." ".
			i18n("Also the strongest cipher that the client supports will be used. For best security, use <strong>WPA2 Only</strong> mode.")." ".
			i18n("This mode uses AES(CCMP) cipher and legacy stations are not allowed access with WPA security.")." ".
			i18n("For maximum compatibility, use <strong>WPA Only</strong>. This mode uses TKIP cipher. Some gaming and legacy devices work only in this mode.");?></p>
		<p><?echo i18n("To achieve better wireless performance use <strong>WPA2 Only</strong> security mode (or in other words AES cipher).");?></p>
	
		<div class="textinput">
			<span class="name"><?echo i18n("WPA Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wpa_mode_Aband" onChange="PAGE.OnChangeWPAMode('_Aband');">
					<option value="WPA+2">Auto(WPA or WPA2)</option>
					<option value="WPA2">WPA2 Only</option>
					<option value="WPA">WPA Only</option>
				</select>
			</span>
		</div>		
		<div class="textinput">
			<span class="name"><?echo i18n("Cipher Type");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="cipher_type_Aband">
					<option value="TKIP">TKIP</option>
					<option value="AES">AES</option>
					<option value="TKIP+AES">TKIP and AES</option>
				</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Group Key Update Interval");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wpa_grp_key_intrv_Aband" type="text" size="20" maxlength="10" /> (seconds)
			</span>
		</div>	
		<div class="gap"></div>
	</div>
	
	<div id="box_wpa_personal_Aband" class="blackbox" style="display:none;">
		<h2><?echo i18n("Pre-Shared Key");?></h2>
		<p class="strong"><?echo i18n("Enter an 8- to 63-character alphanumeric pass-phrase.")." ".
			i18n("For good security it should be of ample length and should not be a commonly known phrase.");?>
		</p>
		<div class="textinput">
			<span class="name"><?echo i18n("Pre-Shared Key");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wpa_psk_key_Aband" type="text" size="20" maxlength="64" />
			</span>
		</div>
		<div class="gap"></div>
		<div class="gap"></div>	
	</div>
	
	
	<div id="box_wpa_enterprise_Aband" class="blackbox" style="display:none;">
		<h2><?echo i18n("EAP (802.1x)");?></h2>
		<p class="strong"><?echo i18n("<strong>When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.</strong>");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("RADIUS server IP Address");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_ip_Aband" type="text" size="15" maxlength="15" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("RADIUS server Port");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_port_Aband" type="text" size="5" maxlength="5" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("RADIUS server Shared Secret");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="radius_srv_sec_Aband" type="password" size="50" maxlength="64" /></span>
		</div>
	
		<div class="textinput">
	        <span class="delimiter"></span>
	        <span class="value"><input type="button" id="radius_adv_Aband" name="radius_adv_Aband" value="Advanced >>" onClick="PAGE.OnClickRadiusAdvanced('_Aband');"></span>
		</div>
		
		<div id="div_second_radius_Aband" name="div_second_radius_Aband" style="display:none;">
			<div class="textinput">
				<p class="strong"><?echo i18n("<strong>Optional backup RADIUS server</strong>");?></p>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Second RADIUS server IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value"><input id="radius_srv_ip_second_Aband" type="text" size="15" maxlength="15" /></span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Second RADIUS server Port");?></span>
				<span class="delimiter">:</span>
				<span class="value"><input id="radius_srv_port_second_Aband" type="text" size="5" maxlength="5" /></span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Second RADIUS server Shared Secret");?></span>
				<span class="delimiter">:</span>
				<span class="value"><input id="radius_srv_sec_second_Aband" type="password" size="50" maxlength="64" /></span>
			</div>
		</div>	
		<div class="gap"></div>
		<div class="gap"></div>
	</div>
</div>

<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>

</form>
<div id="pad" style="display:none;">
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
	<div class="emptyline"></div>
</div>
