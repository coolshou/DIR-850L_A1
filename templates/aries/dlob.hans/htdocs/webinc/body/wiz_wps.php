<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_send" value="'.i18n("Connect").'" onClick="PAGE.OnSubmit();" />\n'.
		 '		<input type="button" name="b_stat" value="'.i18n("Wireless Status").'" onClick="self.location.href=\'./st_wlan.php\';" />\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">

<!-- show this if wps is disabled -->
<div id="wiz_stage_wps_disabled" class="orangebox" style="display:none;">
	<h1>Add Wireless Device with WPS</h1>
	<div><p class="strong">
		<?echo I18N("h",'The WPS Function is currently set to disable. Please click "Yes" to enable it or "No" to exit the wizard.');?>
	</p>
	</div>
	<div class="gap"></div>
	<div class="centerline">
		<input name="b_yes" value="Yes" onclick="self.location.href='./adv_wps.php';" type="button">
		<input name="b_no" value="No" onclick="self.location.href='./bsc_wlan_main.php';" type="button">
	</div>
	<div class="gap"></div>
</div>


<!-- Start of Stage 1 -->
<div id="wiz_stage_1" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 1").": ".i18n("Select Configuration Method for your Wireless Network");?></h2>
	<div><p class="strong">
		<?echo i18n("Please select one of following configuration methods and click next to continue.");?>
	</p></div>
	<div class="gap"></div>
	<div class="wiz-l2">
		<strong><?echo i18n("Auto");?></strong>
		<input id="auto" name="wps" type="radio" value="auto" onClick="PAGE.OnClickMode();" />
		<?echo i18n("Select this option if your wireless device supports WPS")." (".i18n("Wi-Fi Protected Setup").")";?>
	</div>
	<div class="wiz-l2">
		<strong><?echo i18n("Manual");?></strong>
		<input name="wps" type="radio" value="manu" onClick="PAGE.OnClickMode();" />
		<?echo i18n("Select this option will display the current wireless settings for you to configure the wireless device manually");?>
	</div>
	<?wiz_buttons();?>
</div>
<!-- End of Stage 1 -->
<!-- Start of Stage 2 -->
<div id="wiz_stage_2_auto" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 2").": ".i18n("Connect your Wireless Device");?></h2>
	<div>
		<p><?echo i18n("There are two ways to add wireless device to your wireless network").":";?><br />
			<?echo "-PIN (".i18n("Personal Identification Number").")";?><br />
			<?echo "-PBC (".i18n("Push Button Configuration").")";?></p>
	</div>
	<div class="textinput">
		<span class="name"><input id="pin" name="wps_method" type="radio" /> <?echo i18n("PIN");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="pincode" type="text" size="20" maxlength="9" onClick="PAGE.OnClickPINCode();" /></span>
	</div>
	<div>
		<p><?echo i18n('please enter the PIN from your wireless device and click the below "Connect" Button within 120 seconds');?></p>
	</div>
	<div class="textinput">
		<span class="name"><input id="pbc" name="wps_method" type="radio" /> <?echo i18n("PBC");?></span>
		<span class="delimiter"></span>
		<span class="value"></span>
	</div>
	<div>
		<p><?echo i18n('please press the push button on your wireless device and click the below "Connect" Button within 120 seconds');?></p>
	</div>
	<?wiz_buttons();?>
</div>
<div id="wiz_stage_2_manu" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 2").": ".i18n("Connect your Wireless Device");?></h2>
	<div>
		<p class="strong">
			<?echo i18n("Below is a detailed summary of your wireless security settings.")." ".
			i18n("Please print this page out, or write the information on a piece of paper, so you can configure the correct settings on your wireless client adapters.");?>
		</p>
	</div>
	<div id="div_2G_manual">
		<div class="textinput"><b><span id="frequency"></span></b></div>
		<pre style="font-family:Tahoma"><?echo i18n("SSID");?>: <span id="ssid"></span></pre>
		<div class="textinput"><?echo i18n("Security Mode");?>: <span id="security"></span></div>
		<div id="st_wep" class="textinput" style="display:none;"><?echo i18n("WEP Key");?> <span id="wepkey"></span></div>
		<div id="st_cipher" class="textinput" style="display:none;"><?echo i18n("Cipher Type");?>: <span id="cipher"></span></div>
		<div id="st_pskkey" class="textinput" style="display:none;"><?echo i18n("Pre-shared Key");?>: <br><span id="pskkey" class="valueForWPS"></span></div>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div id="div_5G_manual" name="div_5G_manual" style="display:none;">	
		<div class="textinput"><b><span id="frequency_Aband"></span></b></div>
		<pre style="font-family:Tahoma"><?echo i18n("SSID");?>: <span id="ssid_Aband"></span></pre>
		<div class="textinput"><?echo i18n("Security Mode");?>: <span id="security_Aband"></span></div>
		<div id="st_wep_Aband" class="textinput" style="display:none;"><?echo i18n("WEP Key");?> <span id="wepkey_Aband"></span></div>
		<div id="st_cipher_Aband" class="textinput" style="display:none;"><?echo i18n("Cipher Type");?>: <span id="cipher_Aband"></span></div>
		<div id="st_pskkey_Aband" class="textinput" style="display:none;"><?echo i18n("Pre-shared Key");?>: <br><span id="pskkey_Aband" class="valueForWPS"></span></div>
	</div>		
	<br>
	<?wiz_buttons();?>
</div>
<!-- End of Stage 2 -->
<!-- Message of Stage 2 -->
<div id="wiz_stage_2_msg" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 2").": ".i18n("Connect your Wireless Device");?></h2>
	<div><p><span id="msg"></span></p></div>
	<?wiz_buttons();?>
</div>
<!-- Message of Stage 2 -->
</form>
