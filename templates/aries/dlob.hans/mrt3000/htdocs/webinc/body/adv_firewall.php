<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Firewall Settings");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
			<tr>
				<th colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="rc_gray5_hd"><h2><?echo I18N("h","FIREWALL & DMZ SETTINGS");?></h2></th>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h",'DMZ means "Demilitarized Zone".')." ".
						I18N("h",'DMZ allows particular computers behind the router firewall to be accessible from the Internet.');?>
						<?echo I18N("h","Typically, your DMZ would contain Web servers, FTP servers and others.");?>
					</cite>
				</td>
			</tr>

<!-- Firewall Settings START -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Firewall Settings");?></p></td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable SPI");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="spi" type="checkbox"/>
				</td>
			</tr>

<!-- NAT Endpoint Filtering START -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","NAT Endpoint Filtering");?></p></td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="udp_end" name="udptype" type="radio" value="UDP_END"  /><?echo I18N("h","Endpoint Independent");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","UDP Endpoint Filtering");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="udp_add" name="udptype" type="radio" value="UDP_ADD"  /><?echo I18N("h","Address Restricted");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="udp_pna" name="udptype" type="radio" value="UDP_PNA"  /><?echo I18N("h","Port And Address Restricted");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="tcp_end" name="tcptype" type="radio" value="TCP_END"  /><?echo I18N("h","Endpoint Independent");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","TCP Endpoint Filtering");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="tcp_add" name="tcptype" type="radio" value="TCP_ADD"  /><?echo I18N("h","Address Restricted");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="tcp_pna" name="tcptype" type="radio" value="TCP_PNA"  /><?echo I18N("h","Port And Address Restricted");?>
				</td>
			</tr>

<!-- Anti-Spoof checking -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Anti-Spoof checking");?></p></td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable anti-spoof checking");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input type="checkbox" id="anti_spoof_enable" />
				</td>
			</tr>

<!-- DMZ Host START -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","DMZ Host");?></p></td>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side">
					<cite>
							<?echo I18N("h","The DMZ (Demilitarized Zone) option lets you set a single computer on your network outside of the router firewall.");?>
							<?echo I18N("h","If you have a computer that cannot run Internet applications successfully from behind the router, then you can place the computer into the DMZ for unrestricted Internet access.");?>
							<strong><?echo I18N("h","Note");?>:</strong>
							<?echo I18N("h","Putting a computer in the DMZ may expose that computer to a variety of security risks.")." ".
								I18N("h","Use of this option is only recommended as a last resort.");?>
					</cite>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable DMZ");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input type="checkbox" id="dmzenable" onclick="PAGE.OnClickDMZEnable();"/>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DMZ IP Address");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="dmzhost" size="20" maxlength="15" value="0.0.0.0" type="text"/>
					<input modified="ignore" id="dmzadd" value="<<" onclick="PAGE.OnClickDMZAdd();" type="button"/><?drawlabel("dmzhost");?>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<? DRAW_select_dhcpclist("LAN-1","hostlist", I18N("h","Computer Name"), "", "", "1", "selectSty"); ?><?drawlabel("hostlist");?>
				</td>
			</tr>

<!-- ALG checking -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Application Level Gateway (ALG) Configuration");?></p></td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PPTP");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="pptp" type="checkbox"/>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","IPSec (VPN)");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="ipsec" type="checkbox"/>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RTSP");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="rtsp" type="checkbox"/>
				</td>
			</tr>
			<tr>
				<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","SIP");?> :</td>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '3';}else{echo '4';}?>" class="gray_bg border_right">
					<input id="sip" type="checkbox"/>
				</td>
			</tr>

<!-- Firewall Rules START -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side"><p class="subitem"><?=$FW_MAX_COUNT?> -- <?echo I18N("h","Firewall Rules");?></p></td>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"><?=$FW_MAX_COUNT?>
					</cite>
				</td>
			</tr>
						<tr>
							<th width="5%" class="gray_bg border_left"></th>
							<th width="20%" class="gray_bg"></th>
							<th width="10%" class="gray_bg" align="left"><?echo I18N("h","Interface");?></th>
							<th width="<?if ($FEATURE_NOSCH=="1"){echo '45';}else{echo '40';}?>%" class="gray_bg"><?echo I18N("h","IP Address");?></th>
							<th width="<?if ($FEATURE_NOSCH=="1"){echo '25';}else{echo '15';}?>%" class="gray_bg<?if ($FEATURE_NOSCH=="1"){echo ' border_right';}?>"></th>
							<?if ($FEATURE_NOSCH!="1")	echo '<th width="10%" class="gray_bg border_right">'.I18N("h","Schedule").'</th>\n';?>
						</tr>
<?
$INDEX = 1;
while ($INDEX <= $FW_MAX_COUNT)	{dophp("load", "/htdocs/webinc/body/adv_firewall_list.php");	$INDEX++;}
?>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '5';}else{echo '6';}?>" class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				</td>
			</tr>
		</table>
	</div>
</form>
