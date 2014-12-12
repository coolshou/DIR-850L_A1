<?
include "/htdocs/webinc/body/draw_elements.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Guest Zone");?></h1>
	<p><?echo i18n("Use this section to configure the guest zone settings of your router. The guest zone provide a separate network zone for guest to access Internet.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>

<div id="div_route_zone_dualband" name="div_route_zone_dualband" class="blackbox" style="display:none;">
	<h2><?echo i18n("Guest Zone");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Routing Between Zones");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_routing_dualband" type="checkbox" /></span>
	</div>
	<div class="emptyline"></div>
</div>

<div id="div_24G" class="blackbox">
	<h2 id="div_24G_title" name="div_24G_title" ><?echo i18n("Guest Zone Selection");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Guest Zone");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_gzone" type="checkbox" onClick="PAGE.OnClickEnGzone('');" />
		<?
			if ($FEATURE_NOSCH!="1")
			{
				DRAW_select_sch("sch_gz", i18n("Always"), "", "", "0", "narrow");
				echo '<input id="go2sch_gz" type="button" value="'.i18n("New Schedule").'" onClick="javascript:self.location.href=\'./tools_sch.php\';" />\n';
			}
		?>
		</span>
	</div>
	<div id="div_route_zone" class="textinput">
		<span class="name"><?echo i18n("Enable Routing Between Zones");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_routing" type="checkbox" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Band");?></span>
		<span class="delimiter">:</span>
		<span class="value"><b><?echo i18n("2.4GHz Band");?></b></span>
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
		<span class="name"  ><?echo i18n("Security Mode");?></span>
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
	<div class="textinput">
		<span class="name"><?echo i18n("WEP Encryption");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_key_len" onChange="PAGE.OnChangeWEPKey('');">
				<option value="64">64 bit (10 hex digits)</option>
				<option value="128">128 bit (26 hex digits)</option>
			</select>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Default WEP Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_def_key" onChange="PAGE.OnChangeWEPKey('');">
				<option value="1">WEP Key 1</option>
<!--				<option value="2">WEP Key 2</option>
				<option value="3">WEP Key 3</option>
				<option value="4">WEP Key 4</option>-->
			</select>
		</span>
	</div>
	<div id="wep_64" class="textinput">
		<span class="name"><?echo i18n("WEP Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wep_64_1" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_2" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_3" name="wepkey_64" type="text" size="15" maxlength="10" />
			<input id="wep_64_4" name="wepkey_64" type="text" size="15" maxlength="10" />
		</span>
	</div>
	<div id="wep_128" class="textinput">
		<span class="name"><?echo i18n("WEP Key");?></span>
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
	<p><?echo i18n("Enter an 8- to 63-character alphanumeric pass-phrase.")." ".
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
</div>

<div id="box_wpa_enterprise" class="blackbox" style="display:none;">
	<h2><?echo i18n("EAP (802.1x)");?></h2>
	<p><?echo i18n("<strong>When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.</strong>");?></p>
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
		<span class="name"></span>
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
	

<div id="div_5G" class="blackbox" style="display:none;">
	<h2 id="div_5G_title" name="div_5G_title"><?echo i18n("Guest Zone Selection");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Guest Zone");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_gzone_Aband" type="checkbox" onClick="PAGE.OnClickEnGzone('_Aband');" />
		<?
			if ($FEATURE_NOSCH!="1")
			{
				DRAW_select_sch("sch_gz_Aband", i18n("Always"), "", "", "0", "narrow");
				echo '<input id="go2sch_gz_Aband" type="button" value="'.i18n("New Schedule").'" onClick="javascript:self.location.href=\'./tools_sch.php\';" />\n';
			}
		?>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Band");?></span>
		<span class="delimiter">:</span>
		<span class="value"><b><?echo i18n("5GHz Band");?></b></span>
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
		<span class="name"  ><?echo i18n("Security Mode");?></span>
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
	<div class="textinput">
		<span class="name"><?echo i18n("WEP Encryption");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_key_len_Aband" onChange="PAGE.OnChangeWEPKey('_Aband');">
				<option value="64">64Bit</option>
				<option value="128">128Bit</option>
			</select>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Default WEP Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wep_def_key_Aband" onChange="PAGE.OnChangeWEPKey('_Aband');">
				<option value="1">WEP Key 1</option>
<!--			<option value="2">WEP Key 2</option>
				<option value="3">WEP Key 3</option>
				<option value="4">WEP Key 4</option>-->
			</select>
		</span>
	</div>
	<div id="wep_64_Aband" class="textinput">
		<span class="name"><?echo i18n("WEP Key");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wep_64_1_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
			<input id="wep_64_2_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
			<input id="wep_64_3_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
			<input id="wep_64_4_Aband" name="wepkey_64_Aband" type="text" size="15" maxlength="10" />
		</span>
	</div>
	<div id="wep_128_Aband" class="textinput">
		<span class="name"><?echo i18n("WEP Key");?></span>
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
	<p><?echo i18n("Enter an 8- to 63-character alphanumeric pass-phrase.")." ".
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
</div>
	
<div id="box_wpa_enterprise_Aband" class="blackbox" style="display:none;">
	<h2><?echo i18n("EAP (802.1x)");?></h2>
	<p><?echo i18n("<strong>When WPA enterprise is enabled, the router uses EAP (802.1x) to authenticate clients via a remote RADIUS server.</strong>");?></p>
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
		<span class="name"></span>
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
	
<div class="emptyline"></div>


<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>
