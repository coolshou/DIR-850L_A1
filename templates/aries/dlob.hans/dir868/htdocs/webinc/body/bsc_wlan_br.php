<?
include "/htdocs/webinc/body/draw_elements.php";
include "/htdocs/phplib/wifi.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Wireless Network");?></h1>
	<p><?echo i18n("Use this section to configure the wireless settings for your D-Link AP or wireless stations.")." ".
		i18n("Please note that changes made in this section may also need to be duplicated on your wireless client. ");?></p>
	<p><?echo i18n("To protect your privacy you can configure wireless security features.")." ".
		i18n("This device supports three wireless security modes including: WEP, WPA and WPA2.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>

<div id="div_station" >
	<div class="blackbox">
		<h2><?echo i18n("Wireless Network Settings");?></h2>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Band");?></span>
			<span class="delimiter">:</span>
			<span id="sta_wifi_mode" class="value">
				<b><?echo i18n("Station (2.4GHz/5GHz)");?></b>
				<input type="button" id="site_survey_button" value="<?echo i18n("Site Survey");?>" onClick="PAGE.OnClickSiteSurvey('');" />
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Enable Wireless");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="en_wifi_sta" type="checkbox" onClick="PAGE.OnClickEnWLANsta();" />
				<!--Disable station mode function.
				<?
				if ($FEATURE_NOSCH!="1")
				{
					DRAW_select_sch("sch_sta", i18n("Always"), "", "", "0", "narrow");
					echo '<input id="go2sch_sta" type="button" value="'.i18n("New Schedule").'" onClick="javascript:self.location.href=\'./adv_sch.php\';" />\n';
				}
				?>
				-->
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Network Name");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="ssid_sta" type="text" size="20" maxlength="32" />
				(<?echo i18n("Also called the SSID");?>)
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Band");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="freq_sta" onChange="PAGE.OnChangeWirelessBand(this.value)">
					<option value="2.4G">2.4GHz</option>
					<option value="5G">5GHz</option>
				</select>
			</span>
		</div>	
		<div class="textinput" style="display:none">
			<span class="name"><?echo i18n("Channel");?></span>
			<span class="delimiter">:</span>
			<span class="value" id="channel"></span>
		</div>			
		<div class="textinput">
			<span class="name"><?echo i18n("Band Width");?></span>
			<span class="delimiter">:</span>
			<span class="value" id="bw_sta"></span>
		</div>
		<div class="gap"></div>
	</div>
	<div class="blackbox">
		<h2><?echo i18n("Wireless Security Mode");?></h2>
		<div class="textinput">
			<span class="name" <? if(query("/runtime/device/langcode")!="en") echo 'style="width: 28%;"';?> ><?echo i18n("Security Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="security_type_sta" onChange="PAGE.OnChangeSecurityType('_sta');">
					<option value=""><?echo i18n("Disable Wireless Security (not recommended)");?></option>
					<option value="WEP"><?echo i18n("Enable WEP Wireless Security (basic)");?></option>
					<option value="WPA"><?echo i18n("Enable WPA Wireless Security");?></option>
					<option value="WPA2"><?echo i18n("Enable WPA2 Wireless Security (enhanced)");?></option>
					<!-- DIR-865 not support WPA/WPA2 auto
					<option value="WPA+2"><?echo i18n("Enable WPA/WPA2 Wireless Security (enhanced)");?></option>
					-->
				</select>
			</span>
		</div>
		<div class="gap"></div>
	</div>
	<div id="wep_sta" class="blackbox" style="display:none;">
		<h2><?echo i18n("WEP");?></h2>
		<p><?echo i18n("WEP is the wireless encryption standard.")." ".
			i18n("To use it you must enter the same key(s) into the AP and the wireless stations.")." ".
			i18n("For 64-bit keys you must enter 10 hex digits into each key box.")." ".
			i18n("For 128-bit keys you must enter 26 hex digits into each key box. ")." ".
			i18n("A hex digit is either a number from 0 to 9 or a letter from A to F.")." ".;?></p>
<!--		i18n('For the most secure use of WEP set the authentication type to "Shared Key" when WEP is enabled.');?></p>-->
		<p><?echo i18n("You may also enter any text string into a WEP key box, in which case it will be converted into a hexadecimal key using the ASCII values of the characters.")." ".
			i18n("A maximum of 5 text characters can be entered for 64-bit keys, and a maximum of 13 characters for 128-bit keys.");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("Authentication");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="auth_type_sta">
					<option value="OPEN">Open</option>
					<option value="SHARED">Shared Key</option>
				</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("WEP Encryption");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wep_key_len_sta" onChange="PAGE.OnChangeWEPKey('_sta');">
					<option value="64">64Bit</option>
					<option value="128">128Bit</option>
				</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Default WEP Key");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="wep_def_key_sta" onChange="PAGE.OnChangeWEPKey('_sta');">
					<option value="1">WEP Key 1</option>
					<option value="2">WEP Key 2</option>
					<option value="3">WEP Key 3</option>
					<option value="4">WEP Key 4</option>
				</select>
			</span>
		</div>
		<div id="wep_64_sta" class="textinput">
			<span class="name"><?echo i18n("WEP Key");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wep_64_1_sta" name="wepkey_64_sta" type="text" size="15" maxlength="10" />
				<input id="wep_64_2_sta" name="wepkey_64_sta" type="text" size="15" maxlength="10" />
				<input id="wep_64_3_sta" name="wepkey_64_sta" type="text" size="15" maxlength="10" />
				<input id="wep_64_4_sta" name="wepkey_64_sta" type="text" size="15" maxlength="10" />
				(5 ASCII or 10 HEX)
			</span>
		</div>
		<div id="wep_128_sta" class="textinput">
			<span class="name"><?echo i18n("WEP Key");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="wep_128_1_sta" name="wepkey_128_sta" type="text" size="31" maxlength="26" />
				<input id="wep_128_2_sta" name="wepkey_128_sta" type="text" size="31" maxlength="26" />
				<input id="wep_128_3_sta" name="wepkey_128_sta" type="text" size="31" maxlength="26" />
				<input id="wep_128_4_sta" name="wepkey_128_sta" type="text" size="31" maxlength="26" />
				(13 ASCII or 26 HEX)
			</span>
		</div>
		<div class="gap"></div>
	</div>
	<div id="wpa_sta" class="blackbox">
		<h2><?echo i18n("WPA/WPA2");?></h2>
		<p><?echo i18n("WPA/WPA2 requires stations to use high grade encryption and authentication.");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("Cipher Type");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="cipher_type_sta" onChange="PAGE.OnChangeCipher('_sta');">
				<!--Marco says that the cipher type with auto mode has problem to connect to AP due to Broadcom wireless driver in DIR-865L. So we would use AES mode when the cipher type is auto mode. 20120525-->
				<!--<option value="TKIP+AES">AUTO(TKIP/AES)</option>-->
					<option value="AES">AES</option>
					<option value="TKIP">TKIP</option>
				</select>
			</span>
		</div>
		<div class="textinput" style="display:none;">
			<span class="name"><?echo i18n("PSK");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="psk_eap_sta" onChange="PAGE.OnChangeWPAAuth('_sta');">
					<option value="psk">PSK</option>
					<option value="eap">EAP</option>
				</select>
			</span>
		</div>
		<div name="psk_sta" class="textinput">
			<span class="name"><?echo i18n("Network Key");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="wpapsk_sta" type="text" size="40" maxlength="64" /></span>
		</div>
		<div name="psk_sta" class="textinput">
			<span class="name"></span>
			<span class="delimiter"></span>
			<span class="value">(8~63 ASCII or 64 HEX)</span>
		</div>
		<div name="eap_sta" class="textinput">
			<span class="name"><?echo i18n("RADIUS Server IP Address");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="srv_ip_sta" type="text" size="15" maxlength="15" /></span>
		</div>
		<div name="eap_sta" class="textinput">
			<span class="name"><?echo i18n("Port");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="srv_port_sta" type="text" size="5" maxlength="5" /></span>
		</div>
		<div name="eap_sta" class="textinput">
			<span class="name"><?echo i18n("Shared Secret");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="srv_sec_sta" type="password" size="50" maxlength="64" /></span>
		</div>
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
