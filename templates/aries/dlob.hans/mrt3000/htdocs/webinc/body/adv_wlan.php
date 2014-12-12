	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Advanced Wireless");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
        <!-- ===================== 2.4Ghz, BG band ============================== -->
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Advanced Wireless Settings");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","These options are for advanced users who wish to change the behavior of their 802.11n wireless radio.")." ".
		I18N("h","We do not recommend changing these settings from the factory defaults.")." ".
		I18N("h","Incorrect settings may impact the performance of your wireless radio.")." ".
		I18N("h","The default settings should provide the best wireless radio performance in most environments.");?>
				</cite>
			</td>
		</tr>
        
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Advanced Wireless Settings");?></p></td>
        </tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Band");?> :</td>
			<td width="76%" class="gray_bg border_right"><b><?echo I18N("h","2.4GHz Band");?></b></td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Transmit Power");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<select id="tx_power">
				<option value="100">High</option>
				<option value="50">Medium</option>
				<option value="25">Low</option>
				</select>
           </td>
		</tr>
        
		<tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Beacon period");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="beacon" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","msec").", ".I18N("h","range").": 20~1000, ".I18N("h","default");?>: 100)</td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RTS Threshold");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="rts" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","range").": 256~2346, ".I18N("h","default");?>: 2346)</td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Fragmentation");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="frag" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","range").": 1500~2346, ".I18N("h","default").": 2346, ".I18N("h","even number only");?>)</td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DTIM interval");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="dtim" type="text" size="4" maxlength="3" />
			(<?echo I18N("h","range").": 1~255, ".I18N("h","default");?>: 1)</td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WLAN Partition");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="wlan_partition" type="checkbox" /></td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Preamble Type");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input id="preamble" name="preamble" type="radio" value="short" /><?echo I18N("h","Short Preamble");?>
				<input id="preamble" name="preamble" type="radio" value="long" /><?echo I18N("h","Long Preamble");?>
            </td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WMM Enable");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input id="en_wmm" type="checkbox" />
            </td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Short GI");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input id="sgi" type="checkbox" />
            </td>
		</tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","HT 20/40 Coexistence");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input type="radio" class="name" id="coexist_enable" name="coexist_type" value="enable" /> <label>Enable</label>
	        <input type="radio" class="name" id="coexist_disable" name="coexist_type" value="disable" /> <label>Disable</label>
            </td>
		</tr>
        <!-- ===================== 5Ghz, A band ============================== -->
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Advanced Wireless Settings");?></h2></th>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Band");?> :</td>
			<td width="76%" class="gray_bg border_right"><b><?echo I18N("h","5GHz Band");?></b></td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Transmit Power");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<select id="tx_power_Aband">
				<option value="100">High</option>
				<option value="50">Medium</option>
				<option value="25">Low</option>
				</select>
            </td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Beacon period");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="beacon_Aband" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","msec").", ".I18N("h","range").": 20~1000, ".I18N("h","default");?>: 100)</td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RTS Threshold");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="rts_Aband" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","range").": 256~2346, ".I18N("h","default");?>: 2346)</td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Fragmentation");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="frag_Aband" type="text" size="4" maxlength="4" />
			(<?echo I18N("h","range").": 1500~2346, ".I18N("h","default").": 2346, ".I18N("h","even number only");?>)</td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DTIM interval");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="dtim_Aband" type="text" size="4" maxlength="3" />
			(<?echo I18N("h","range").": 1~255, ".I18N("h","default");?>: 1)</td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WLAN Partition");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="wlan_partition_Aband" type="checkbox" /></td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Preamble Type");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input id="preamble_Aband" name="preamble_Aband" type="radio" value="short" /><?echo I18N("h","Short Preamble");?>
				<input id="preamble_Aband" name="preamble_Aband" type="radio" value="long" /><?echo I18N("h","Long Preamble");?>
            </td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WMM Enable");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="en_wmm_Aband" type="checkbox" /></td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Short GI");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="sgi_Aband" type="checkbox" /></td>
		</tr>
        <tr <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","HT 20/40 Coexistence");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<input type="radio" class="name" id="coexist_enable_Aband"	name="coexist_type_Aband" value="enable" /> <label>Enable</label>
	        	<input type="radio" class="name" id="coexist_disable_Aband" name="coexist_type_Aband" value="disable"/> <label>Disable</label>
            </td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
        
		</table>
	</div>
	</form>
