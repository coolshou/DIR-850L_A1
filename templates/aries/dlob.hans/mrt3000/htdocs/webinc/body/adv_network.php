<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Advanced Network");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
			<tr>
				<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Advanced Network Settings");?></h2></th>
			</tr>
			<tr>
				<td colspan="2" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","These options are for advanced users who wish to change the LAN settings. We do not recommend changing these settings from factory default.");?>
						<?echo I18N("h","Changing these settings may affect the behavior of your network.");?>
					</cite>
				</td>
			</tr>

<!-- UPNP START -->
			<tr>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","UPNP");?></p></td>
			</tr>
			<tr>
				<td colspan="2" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","Universal Plug and Play(UPnP) helps network devices to discover and connect to other devices in the network.");?>
					</cite>
				</td>
			</tr>
			<tr>
				<td width="34%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable UPnP IGD");?> :</td>
				<td width="66%" class="gray_bg border_right">
					<input id="upnp" value="" type="checkbox"/>
				</td>
			</tr>

<!-- WAN Ping START -->
			<tr>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WAN Ping");?></p></td>
			</tr>
			<tr>
				<td colspan="2" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","If you enable this feature, the WAN port of your router will respond to ping requests from the Internet that are sent to the WAN IP Address.");?>
					</cite>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable WAN Ping Response");?> :</td>
				<td class="gray_bg border_right">
					<input id="icmprsp" value="" type="checkbox"/>
				</td>
			</tr>

<!-- WAN Port Speed START -->
			<tr>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WAN Port Speed");?></p></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","WAN Port Speed");?> :</td>
				<td class="gray_bg border_right">
					<select id="wanspeed">
						<option value="1"><?echo I18N("h","10Mbps");?></option>
						<option value="2"><?echo I18N("h","100Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE!="1") {echo "<!--";}?>
						<option value="3"><?echo I18N("h","1000Mbps");?></option>
						<option value="0"><?echo I18N("h","Auto 10/100/1000Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE!="1") {echo "-->";}
   else {echo "<!--";}?>
						<option value="0"><?echo I18N("h","Auto 10/100Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE=="1") {echo "-->";}?>
					</select>
				</td>
			</tr>

<!-- Multicast Streams START -->
			<tr>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><? if ($FEATURE_NOIPV6 =="0") { echo I18N("h","IPv4 Multicast Streams"); } else { echo I18N("h","Multicast Streams"); } ?></p></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><? if ($FEATURE_NOIPV6 =="0") { echo I18N("h","IPv4 Enable Multicast Streams"); } else { echo I18N("h","Enable Multicast Streams"); } ?> :</td>
				<td class="gray_bg border_right">
					<input id="mcast" type="checkbox" onclick="PAGE.Click_Multicast_Enable();"/>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Wireless Enhance Mode");?> :</td>
				<td class="gray_bg border_right">
					<input id="enhance" type="checkbox" onclick="PAGE.Click_Multicast_Enable();"/>
				</td>
			</tr>

<!-- Multicast Streams START -->
			<tr <?if ($FEATURE_EEE !="1") echo ' style="display:none;"';?>>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","EEE");?></p></td>
			</tr>
			<tr <?if ($FEATURE_EEE !="1") echo ' style="display:none;"';?>>
				<td colspan="2" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","The goal of Energy Efficient Ethernet(EEE) is to reduce Ethernet power consumption by 50 percent or more.");?>
					</cite>
				</td>
			</tr>
			<tr <?if ($FEATURE_EEE !="1") echo ' style="display:none;"';?>>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable EEE");?> :</td>
				<td class="gray_bg border_right">
					<input id="eee" type="checkbox" />
				</td>
			</tr>

			<tr>
				<td colspan="2" class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				</td>
			</tr>
		</table>
	</div>
</form>
