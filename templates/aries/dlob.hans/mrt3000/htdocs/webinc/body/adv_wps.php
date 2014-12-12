	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Wi-Fi Protected Setup");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Wi-Fi Protected Setup");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Wi-Fi Protected Setup is used to easily add devices to a network using a PIN or button press.")." ".
						I18N("h","Devices must support Wi-Fi Protected Setup in order to be configured by this method.");?><br/>
						<?echo I18N("h","If the PIN changes, the new PIN will be used in following Wi-Fi Protected Setup process.")." ".
							I18N("h","Clicking on ''Don't Save Settings'' button will not reset the PIN.");?><br/>
						<?echo I18N("h","However, if the new PIN is not saved, it will get lost when the device reboots or loses power.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td width="30%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable");?> :</td>
			<td width="70%" class="gray_bg border_right">
				<input id="en_wps" type="checkbox" onClick="PAGE.OnClickEnWPS();" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WiFi Protected Setup");?> :</td>
			<td class="gray_bg border_right">
				<span id="wifi_info_str" class="value"></span>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Disable WPS-PIN Method");?> :</td>
			<td class="gray_bg border_right">
				<input id="lock_wifi_security" type="checkbox" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
			<td class="gray_bg border_right">
				<input id="reset_cfg" type="button" value="<?echo I18N("h","Reset to Unconfigured");?>"
					onClick="PAGE.OnClickResetCfg();" />
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","PIN Settings");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PIN");?> :</td>
			<td class="gray_bg border_right">
				<span id="pin" class="value"></span>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
			<td class="gray_bg border_right">
				<input id="reset_pin" type="button" value="<?echo I18N("h","Reset PIN to Default");?>" onClick="PAGE.OnClickResetPIN();" />
				<input id="gen_pin" type="button" value="<?echo I18N("h","Generate New PIN");?>" onClick="PAGE.OnClickGenPIN();" />

			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Add Wireless Station");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
			<td class="gray_bg border_right">
				<input id="go_wps" type="button" value="<?echo I18N("h","Connect your Wireless Device");?>" onClick='self.location.href="./wiz_wps.php";' />
			</td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="self.location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
