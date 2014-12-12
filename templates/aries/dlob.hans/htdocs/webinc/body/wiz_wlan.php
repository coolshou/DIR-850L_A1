<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_send" value="'.i18n("Save").'" onClick="BODY.OnSubmit();" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">
<!-- Start of Stage 1 -->
<div id="wiz_stage_1" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 1").": ".i18n("Welcome TO THE D-LINK WIRELESS SECURITY SETUP WIZARD");?></h2>
	<div><p class="strong">
		<?echo i18n("Give your network a name, using up to 32 characters.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span id="fld_ssid_24" name="fld_ssid_24" class="name"><?echo i18n("Network Name (SSID)")." 2.4GHz";?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wiz_ssid" type="text" size="20" maxlength="32" />
		</span>
	</div>
	<div id="div_ssid_A" name="div_ssid_A" class="textinput" style="display:none;">
		<span class="name"><?echo i18n("Network Name (SSID)")." 5Ghz";?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wiz_ssid_Aband" type="text" size="20" maxlength="32" />
		</span>
	</div>
	<br>
	
	<div class="gap"></div>	
	<div class="wiz-l1" text-align="left">
		<input type="radio" id="autokey" name="key" checked="checked" />
		<?echo i18n("Automatically assign a network key")." (".i18n("Recommended").")";?><br />
	</div>
	<div class="wiz-l2">
		<?echo i18n("To prevent outsiders from accessing your network, the router will automatically assign a security (also called WEP or WPA key) to your network.");?>
	</div>
	<div class="gap"></div>
	<div class="wiz-l1">
		<input type="radio" id="manukey" name="key" />
		<?echo i18n("Manually assign a network key");?><br />
	</div>
	<div class="wiz-l2">
		<?echo i18n("Use this options if you prefer to create our own key.");?>
	</div>
	<div><p class="strong">
		<?echo i18n("Note").": ".i18n("All D-Link wireless adapters currently support WPA.");?>
	</p></div>
	<?wiz_buttons();?>
</div>
<!-- End of Stage 1 -->
<!-- Start of Stage 2 -->
<div id="wiz_stage_2" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 2").": ".i18n("Set your Wireless Security Password");?></h2>
	<div>
		<p class="strong">
			<?echo i18n("You have selected your security level")." - ".
					i18n("you will need to set a wireless security password.");?>
		</p>
		<p class="strong"><?echo i18n("The WPA (Wi-Fi Protected Access) key must meet one of following guidelines:");?></p>
		<p class="strong"><?echo "- ".i18n("Between 8 and 63 characters")." (".i18n("A longer WPA key is more secure than a short one")." )".;?></p>
		<p class="strong"><?echo "- ".i18n("Exactly 64 characters using 0-9 and A-F");?></p>
	</div>
	
	<div id="div_5g_sync" name="div_5g_sync" style="display:none;">
	<ul>
		<input type="checkbox" id="set_5g_security_id" onclick="set_5g_security(this.checked)" checked="checked"/>
			<?echo I18N("h", "Use the same Wireless Security Password on both 2.4GHz and 5GHz band");?>
    </ul>
	</div>
	
	<div class="textinput">
		<span class="name" id="wl_sec"><?echo i18n("Wireless Security Password");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="wiz_key" type="text" maxlength="64" /></span>
	</div>
	<div class="textinput" style="display:none;" id="wl_sec_Aband_div">
		<span class="name" id="wl_sec_Aband"><?echo i18n("Wireless Security Password");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="wiz_key_Aband" type="text" maxlength="64" /></span>
	</div>	
	
	<div>
		<p class="strong"><?echo i18n("Note").": ".i18n("You will need to enter the same password as keys in this step into your wireless clients in order to enable proper wireless communication.");?></p>
	</div>
	<?wiz_buttons();?>
</div>
<!-- End of Stage 2 -->
<!-- Start of Stage 3 -->
<div id="wiz_stage_3" class="blackbox" style="display:none;">
	<h2><?echo i18n("Setup Complete!");?></h2>
	<div><p class="strong">
		<?echo i18n("Below is a detailed summary of your wireless security settings.")." ".
				i18n("Please print this page out, or write the information on a piece of paper, so you can configure the correct settings on your wireless client adapters.");?>
	</p></div>
	<fieldset>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Band");?></span>
			<span class="delimiter">:</span>
			<span class="value">2.4<?echo i18n("GHz Band");?></span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Network Name")." (SSID)";?></span>
			<span class="delimiter">:</span>
			<pre style="font-family:Tahoma"><span id="ssid" class="value"></span></pre>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Security Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value"><?echo i18n("Auto (WPA or WPA2) - Personal");?></span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Cipher Type");?></span>
			<span class="delimiter">:</span>
			<span class="value">TKIP and AES</span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Pre-Shared Key");?></span>
			<span class="delimiter">:</span>
			<span id="s_key" class="value"></span>
			<span id="l_key" class="value" style="display:none;"></span>
		</div>
		<!--<div id="l_key" class="centerline" style="display:none;"></div>-->
	</fieldset>
	
	<fieldset id="div_summary_A" name="div_summary_A" style="display:none;">
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Band");?></span>
			<span class="delimiter">:</span>
			<span class="value">5<?echo i18n("GHz Band");?></span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Wireless Network Name")." (SSID)";?></span>
			<span class="delimiter">:</span>
			<pre style="font-family:Tahoma"><span id="ssid_Aband" class="value"></span></pre>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Security Mode");?></span>
			<span class="delimiter">:</span>
			<span class="value"><?echo i18n("Auto (WPA or WPA2) - Personal");?></span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Cipher Type");?></span>
			<span class="delimiter">:</span>
			<span class="value">TKIP and AES</span>
		</div>
		<div class="gap"></div>
		<div class="textinput">
			<span class="name"><?echo i18n("Pre-Shared Key");?></span>
			<span class="delimiter">:</span>
			<span id="s_key_Aband" class="value"></span>
			<span id="l_key_Aband" class="value" style="display:none;"></span>
		</div>
		<!--
		<div id="l_key_Aband" class="centerline" style="display:none;"></div>
		-->
	</fieldset>
	
	<?wiz_buttons();?>
</div>
<!-- End of Stage 3 -->
</form>
