	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
<!-- show this if wps is disabled -->
	<div id="wiz_stage_wps_disabled" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Add Wireless Device with WPS");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h",'The WPS Function is currently set to disable. Please click "Yes" to enable it or "No" to exit the wizard.');?></h6>
			<div class="gradient_form_content">
				<p>
					<input type="button" value="<?echo I18N("h","Yes");?>" onclick="self.location.href='./adv_wps.php';" />
					<input type="button" value="<?echo I18N("h","No");?>" onclick="self.location.href='./adv_wps.php';" />
				</p>
			</div>
		</div>
	</div>
<!-- show this if wps is disabled -->
<!-- Start of Stage 1 -->
	<div id="wiz_stage_1" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 1").": ".I18N("h","Select Configuration Method for your Wireless Network");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Please select one of following configuration methods and click next to continue.");?></h6>
			<div class="gradient_form_content">
				<p>
					<label for="auto"><b><?echo I18N("h","Auto");?></b></label>
					<input id="auto" name="wps" type="radio" value="auto" checked />
					<label for="auto"><?echo I18N("h","Select this option if your wireless device supports WPS")." (".I18N("h","Wi-Fi Protected Setup").")";?></label>
				</p>
				<p>
					<label for="manual"><b><?echo I18N("h","Manual");?></b></label>
					<input id="manual" name="wps" type="radio" value="manual" />
					<label for="manual"><?echo I18N("h","Select this option will display the current wireless settings for you to configure the wireless device manually");?></label>
				</p>
			</div>
		</div>
	</div>
<!-- End of Stage 1 -->
<!-- Start of Stage 2 -->
	<div id="wiz_stage_2" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 2").": ".I18N("h","Connect your Wireless Device");?></h2>
		</div>
	</div>
	<!-- Stage 2: AUTO -->
	<div id="wiz_stage_2_auto" style="display:none;">
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","There are two ways to add wireless device to your wireless network:");?></h6>
			<h6>-PIN (<?echo I18N("h","Personal Identification Number");?>)</h6>
			<h6>-PBC (<?echo I18N("h","Push Button Configuration)");?>)</h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"></td>
				<td width="674"><b>
					<input id="pin" name="wps_method" type="radio" /><label for="pin"> <?echo I18N("h","PIN");?> :</label>
					<input id="pincode" type="text" size="20" maxlength="9" class="text_block" />
				</b></td>
			</tr>
			</table>
			<h6><?echo I18N("h",'please enter the PIN from your wireless device and click the below "Connect" Button within 120 seconds');?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"></td>
				<td width="674"><b>
					<input id="pbc" name="wps_method" type="radio" /><label for="pbc"> <?echo I18N("h","PBC");?></label>
				</b></td>
			</tr>
			</table>
			<h6><?echo I18N("h",'please press the push button on your wireless device and click the below "Connect" Button within 120 seconds');?></h6>
		</div>
	</div>
	<!-- Stage 2: MANUAL -->
	<div id="wiz_stage_2_manual" style="display:none;">
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Below is a detailed summary of your wireless security settings.");?>
				<?echo I18N("h","Please print this page out, or write the information on a piece of paper, so you can configure the correct settings on your wireless devices.");?></h6>
			<p class="assistant"><?echo I18N("h","2.4 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674"><span id="ssid" class="maroon"></span></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Security Mode");?> :</b></td>
				<td width="674"><span id="security" class="maroon"></span></td>
			</tr>
			<tr id="st_wep" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","WEP Key");?> :</b></td>
				<td width="674"><span id="wep" class="maroon"></span></td>
			</tr>
			<tr id="st_cipher" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Cipher Type");?> :</b></td>
				<td width="674"><span id="cipher" class="maroon"></span></td>
			</tr>
			<tr id="st_pskkey" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Pre-shared Key");?> :</b></td>
				<td width="674"><span id="pskkey" class="maroon"></span></td>
			</tr>
			</table>
			<div id="div_5G_manual" style="display:none;">
			<p class="assistant"><?echo I18N("h","5 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674"><span id="ssid_11a" class="maroon"></span></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Security Mode");?> :</b></td>
				<td width="674"><span id="security_11a" class="maroon"></span></td>
			</tr>
			<tr id="st_wep" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","WEP Key");?> :</b></td>
				<td width="674"><span id="wep" class="maroon"></span></td>
			</tr>
			<tr id="st_cipher" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Cipher Type");?> :</b></td>
				<td width="674"><span id="cipher" class="maroon"></span></td>
			</tr>
			<tr id="st_pskkey" style="display:none;">
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Pre-shared Key");?> :</b></td>
				<td width="674"><span id="pskkey" class="maroon"></span></td>
			</tr>
			</table>
			</div>
		</div>
	</div>
	<!-- Stage 2: Message -->
	<div id="wiz_stage_2_msg" style="display:none;">
		<div class="rc_gradient_bd h_initial">
			<h6><span id="msg"></span></h6>
		</div>
	</div>
<!-- End of Stage 2 -->
	<div class="rc_gradient_submit">
		<button id="b_exit" type="button" class="submitBtn floatLeft" onclick="PAGE.OnClickCancel();">
			<b><?echo I18N("h","Cancel");?></b>
		</button>
<!--		<button id="b_back" type="button" class="submitBtn floatLeft" style="display:none;" onclick="PAGE.OnClickPre();">
			<b><?echo I18N("h","Back");?></b>
		</button>-->
		<button id="b_next" type="button" class="submitBtn floatRight" onclick="PAGE.OnClickNext();">
			<b><?echo I18N("h","Next");?></b>
		</button>
		<button id="b_send" type="button" class="submitBtn floatRight" style="display:none;" onclick="PAGE.OnSubmit();">
			<b><?echo I18N("h","Connect");?></b>
		</button>
		<button id="b_stat" type="button" class="submitBtn floatRight" style="display:none;" onclick="self.location.href='./st_wlan.php';">
			<b><?echo I18N("h","Wireless Status");?></b>
		</button><i></i>
	</div>
	</div>
	</form>
