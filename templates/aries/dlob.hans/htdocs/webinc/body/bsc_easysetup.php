<form id="mainform" onsubmit="return false;">
<!-- start of internet status -->
<div id="internet_status">
	<img src="/pic/title.jpg"/>
	<div class="networkmap" align=center>
	<div class="gap"></div>
	<div class="gap"></div>
		<table id="map">
		<tr>
			<th width="134"><?echo I18N("h","Client");?></th>
			<th width="114"></th>
			<th width="134"><?echo I18N("h","Router");?></th>
			<th width="114"></th>
			<th width="134"><?echo I18N("h","Internet");?></th>
		</tr>
		</table>
		<div class="gap"></div>
		<div class="gap"></div>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
</div>
<!-- end of internet status -->
<div class="gap"></div>
<!-- start of stage setting -->
<div id="stage_set" style="display:none;">
	<div id="wan_set" class="networkmap">
		<!-- start of wan mode -->
		<h2><?echo I18N("h","Internet Connection");?></h2>
		<div class="gap"></div>
		<div class="gap"></div>
		<table width="100%">
			<tbody>
				<tr>
					<td class="br_tb" width="40%"><?echo I18N("h","Internet Connection Type");?></td>
					<td class="l_tb" width="60%"><b>:</b>&nbsp;
						<select class="text_style" id="wan_mode" onChange="PAGE.OnChangeWanType(this.value);">
							<option value="STATIC"><?echo I18N("h","Static IP");?></option>
							<option value="DHCP"><?echo I18N("h","Dynamic IP");?></option>
							<option value="PPPoE"><?echo I18N("h","PPPoE");?></option>
							<option value="PPTP"><?echo I18N("h","PPTP");?></option>
							<option value="L2TP"><?echo I18N("h","L2TP");?></option>
							<?if ($FEATURE_DHCPPLUS=="1") echo '<option value="DHCPPLUS">'.I18N("h","DHCP+").'</option>\n';?>
						</select>
		        <b><?echo " (".I18N("h","Other mode").")";?></b>
					</td>
				</tr>	
			</tbody>
		</table>
		<!-- end of wan mode -->
		<!-- start of WAN configuration -->
		<div id="box_wan_cfg">
			<input id="ipv4_mtu" type="hidden" />
			<input id="ppp4_mtu" type="hidden" />
			<input id="ppp4_mru" type="hidden" />
			<input id="ppp4_timeout" type="hidden" />
			<input id="ppp4_mode" type="hidden" />
			<!-- start of STATIC -->
			<div id="STATIC">
				<table width="100%"><tbody>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_static_ipaddr" class="text_style" type="text" size="20" maxlength="15" /><font color="#0000FF"><b><?echo " (".I18N("h","* is required field.").")";?></b></font>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Subnet Mask");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_static_mask" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Gateway Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_static_gw" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Primary DNS Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_static_dns1" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Secondary DNS Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_static_dns2" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
				</tbody></table>
			</div>
			<!-- end of STATIC -->
			<!-- start of DHCP -->
			<div id="DHCP"></div>
			<!-- end of DHCP -->
			<!-- start of DHCPPLUS -->
			<div id="DHCPPLUS">
				<table width="100%"><tbody>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Username");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_dhcpplus_user" class="text_style" type="text" size="20" maxlength="63" /><font color="#0000FF"><b><?echo " (".I18N("h","* is required field.").")";?></b></font>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Password");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_dhcpplus_pass" class="text_style" type="password" size="20" maxlength="63" />
						</td>
					</tr>
				</tbody></table>
			</div>
			<!-- end of DHCPPLUS -->
			<!-- start of PPPoE -->
			<div id="PPPoE">
				<table width="100%"><tbody>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","User Name");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pppoe_usr" class="text_style" type="text" size="20" maxlength="63" /><font color="#0000FF"><b><?echo " (".I18N("h","* is required field.").")";?></b></font>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","User Password");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pppoe_passwd" class="text_style" type="password" size="20" maxlength="63" />
						</td>
					</tr>
				</tbody></table>
			</div>
			<!-- end of PPPoE -->
			<!-- start of PPTP -->
			<div id="PPTP">
				<table width="100%"><tbody>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Address Mode");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input name="wiz_pptp_conn_mode" class="text_style" type="radio" value="dynamic" checked onClick="PAGE.OnChangePPTPMode();" />
							<?echo I18N("h","Dynamic IP (DHCP)");?>
							<input name="wiz_pptp_conn_mode" class="text_style" type="radio" value="static" onClick="PAGE.OnChangePPTPMode();" />
							<?echo I18N("h","Static IP");?>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","PPTP IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_ipaddr" class="text_style" type="text" size="20" maxlength="15" /><font color="#0000FF"><b><?echo " (".I18N("h","* is required field.").")";?></b></font>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","PPTP Subnet Mask");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_mask" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","PPTP Gateway IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_gw" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","PPTP Server IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_svr" class="text_style" type="text" size="20" maxlength="30" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","User Name");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_usr" class="text_style" type="text" size="20" maxlength="63"/>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Password");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_passwd" class="text_style" type="password" size="20" maxlength="63" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Primary DNS Server");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_dns1" class="text_style" type="text" size="20" maxlength="15"/>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Secondary DNS Server");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_dns2" class="text_style" type="text" size="20" maxlength="15"/>
						</td>
					</tr>
					<tr style="display:none;">
						<td class="br_tb" width="40%"><b><?echo I18N("h","MAC Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_pptp_mac" class="text_style" type="text" size="20" maxlength="17"/> 
						</td>
					</tr>
					<tr style="display:none;">
						<td class="br_tb" width="40%">&nbsp;</td>
						<td class="l_tb" width="60%">&nbsp;&nbsp;
							<input class="button_submit" type="button" id="wiz_pptp_clone_mac_addr" value="<?echo I18N("h","Clone Your PC's MAC Address");?>" onclick="PAGE.OnClickMacButton('wiz_pptp_mac');"/>
						</td>
					</tr>
				</tbody></table>
			</div>
			<!-- end of PPTP -->
			<!-- start of L2TP -->
			<div id="L2TP">
				<table width="100%"><tbody>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Address Mode");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input name="wiz_l2tp_conn_mode" type="radio" value="dynamic" checked onClick="PAGE.OnChangeL2TPMode();" />
							<?echo I18N("h","Dynamic IP")." (".I18N("h","DHCP").")";?>
							<input name="wiz_l2tp_conn_mode" type="radio" value="static" onClick="PAGE.OnChangeL2TPMode();" />
							<?echo I18N("h","Static IP");?>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","L2TP IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_ipaddr" class="text_style" type="text" size="20" maxlength="15" /><font color="#0000FF"><b><?echo " (".I18N("h","* is required field.").")";?></b></font>
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","L2TP Subnet Mask");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_mask" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","L2TP Gateway IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_gw" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","L2TP Server IP Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_svr" class="text_style" type="text" size="20" maxlength="30" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","User Name");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_usr" class="text_style" type="text" size="20" maxlength="63" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><font color="#0000FF">*</font><b><?echo I18N("h","Password");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_passwd" class="text_style" type="password" size="20" maxlength="63" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Primary DNS Server");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_dns1" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr>
						<td class="br_tb" width="40%"><b><?echo I18N("h","Secondary DNS Server");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_dns2" class="text_style" type="text" size="20" maxlength="15" />
						</td>
					</tr>
					<tr style="display:none;">
						<td class="br_tb" width="40%"><b><?echo I18N("h","MAC Address");?></b></td>
						<td class="l_tb" width="60%"><b>:</b>&nbsp;
							<input id="wiz_l2tp_mac" class="text_style" type="text" size="20" maxlength="17" />
						</td>
					</tr>
					<tr style="display:none;">
						<td class="br_tb" width="40%">&nbsp;</td>
						<td class="l_tb" width="60%">&nbsp;&nbsp;
							<input class="button_submit" type="button" id="l2tp_clone_mac_addr" value="<?echo I18N("h","Clone Your PC's MAC Address");?>" onclick="PAGE.OnClickMacButton('wiz_l2tp_mac');"/>
						</td>
					</tr>
				</tbody></table>
			</div>
			<!-- end of L2TP -->
			<div class="gap"></div>
			<div class="gap"></div>
		</div>
		<!-- end of WAN configuration -->
	</div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<!-- start of WLAN configuration -->
	<div id="wlan_set" class="networkmap">
		<h2><?echo I18N("h","Wireless Connection");?></h2>
		<div><p id="wifi24_name_pwd_show" class="wiz_strong">
			<? echo I18N("h","2.4GHz Band");?>
		</p></div>
		<table width="100%"><tbody>
			<tr>
				<td class="br_tb" width="40%"><b><?echo I18N("h","Wireless Network Name")." (".I18N("h","SSID").")";?></b></td>
				<td class="l_tb" width="60%"><b>:</b>&nbsp;
					<input id="wiz_ssid" class="text_style" type="text" size="32" maxlength="32" />
				</td>
			</tr>
			<tr>
				<td class="br_tb" width="40%"><b><?echo I18N("h","Wireless Password");?></b></td>
				<td class="l_tb" width="60%"><b>:</b>&nbsp;
					<input id="wiz_key" class="text_style" type="text" size="32" maxlength="63" />
				</td>
			</tr>
		</tbody></table>
		<div class="gap"></div>
		<div id="div_ssid_A" name="div_ssid_A" style="display:none;">
			<div><p class="wiz_strong">
				<? echo I18N("h","5GHz Band");?>
			</p></div>
			<table width="100%"><tbody>
				<tr>
					<td class="br_tb" width="40%"><b><?echo I18N("h","Wireless Network Name")." (".I18N("h","SSID").")";?></b></td>
					<td class="l_tb" width="60%"><b>:</b>&nbsp;
						<input id="wiz_ssid_Aband" class="text_style" type="text" size="32" maxlength="32" />
					</td>
				</tr>
				<tr>
					<td class="br_tb" width="40%"><b><?echo I18N("h","Wireless Password");?></b></td>
					<td class="l_tb" width="60%"><b>:</b>&nbsp;
						<input id="wiz_key_Aband" class="text_style" type="text" size="32" maxlength="63" />
					</td>
				</tr>
			</tbody></table>
		</div>
	</div>
	<!-- end of WLAN configuration -->
	<div class="gap"></div>
	<div class="centerline">
		<table width="100%">
			<tr>
				<td width="280"></td>
				<td align="left">
					<input type="button" id="smartsetup" class="submit_button" value="<?echo I18N("h","Connect");?>" onClick="PAGE.OnClickConnectButton();"/>
				</td>
				<td width="20"></td>
				<td align="right" width="75">
				<div>
					<span onclick="PAGE.OnClickSLLink();">
						<p class="strong_white"><?echo I18N("h","Save & Logout");?></p>
					</span>
				</div>
				</td>
				<td width="15"></td>
				<td align="right" width="90">
				<div>
					<span onclick="PAGE.OnClickANSLink();">
						<p class="strong_white"><?echo I18N("h","Advanced Network Settings");?></p>
					</span>
				</div>
				</td>
			</tr>
		</table>
	</div>
</div>
<!-- end of stage setting -->
<!-- start of stage connection to internet connection -->
<div id="stage_check_connect" class="networkmap" style="display:none;">
	<h2><?echo I18N("h","CONNECTING TO INTERNET");?></h2>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div><p class="text_style2"><?echo I18N("h","Connecting to the Internet, please wait a moment.");?></p></div>
	<div class="gap"></div>
	<div id="progress_bar" class="progress_bar_status">
		<span id="block0">&nbsp;</span><span id="block1">&nbsp;</span><span id="block2">&nbsp;</span><span id="block3">&nbsp;</span><span id="block4">&nbsp;</span><span id="block5">&nbsp;</span><span id="block6">&nbsp;</span><span id="block7">&nbsp;</span><span id="block8">&nbsp;</span><span id="block9">&nbsp;</span><span id="block10">&nbsp;</span><span id="block11">&nbsp;</span><span id="block12">&nbsp;</span><span id="block13">&nbsp;</span><span id="block14">&nbsp;</span><span id="block15">&nbsp;</span><span id="block16">&nbsp;</span><span id="block17">&nbsp;</span><span id="block18">&nbsp;</span><span id="block19">&nbsp;</span><span id="block20">&nbsp;</span><span id="block21">&nbsp;</span><span id="block22">&nbsp;</span><span id="block23">&nbsp;</span><span id="block24">&nbsp;</span><span id="block25">&nbsp;</span><span id="block26">&nbsp;</span><span id="block27">&nbsp;</span><span id="block28">&nbsp;</span><span id="block29">&nbsp;</span><span id="block30">&nbsp;</span><span id="block31">&nbsp;</span><span id="block32">&nbsp;</span><span id="block33">&nbsp;</span><span id="block34">&nbsp;</span><span id="block35">&nbsp;</span><span id="block36">&nbsp;</span><span id="block37">&nbsp;</span><span id="block38">&nbsp;</span><span id="block39">&nbsp;</span><span id="block40">&nbsp;</span><span id="block41">&nbsp;</span><span id="block42">&nbsp;</span><span id="block43">&nbsp;</span><span id="block44">&nbsp;</span><span id="block45">&nbsp;</span><span id="block46">&nbsp;</span><span id="block47">&nbsp;</span><span id="block48">&nbsp;</span><span id="block49">&nbsp;</span><span id="block50">&nbsp;</span>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
</div>
<!-- end of stage connection to internet connection -->
<!-- start of stage no cable connected -->
<div id="stage_no_cable" style="display:none;">
	<div class="networkmap">
		<h2><?echo I18N("h","PHYSICAL CONNECTION OF INTERNET PORT IS DISCONNECTED");?></h2>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div>
			<p><B><?echo I18N("h","Please make sure the cable between the ADSL/cable modem and the router is properly connected.");?></B></p>
			<p><B><?echo I18N("h","Please follow the quick installation manual hardware connection graph to check all configuration, confirm normal operation click \"try again\"");?></B></p>
		</div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
	</div>
	<div class="gap"></div>
	<div id="ta_button" style="display:block;">
		<table width="100%">
			<tr>
				<td align="center">
					<input type="button" id="complete_config" class="submit_button" value="<? echo I18N("h","Try Again");?>" onClick="PAGE.OnClickTAButton();" />
				</td>
			</tr>
		</table>
	</div>
	<div class="gap"></div>
</div>
<!-- end of stage no cable connected -->
<!-- start of stage pppoe username/passwd failed -->
<div id="stage_pppoe_error" style="display:none;">
	<div class="networkmap">
		<h2><?echo I18N("h","PPPoE DIAL UP FAILURE");?></h2>
		<div class="gap"></div>
		<div class="gap"></div>
		<div>
			<p><b><?echo I18N("h","PPPoE user name or password is incorrect, please follow the instructions");?></b></p>
		</div>
		<div class="gap"></div>
		<div>
			<p><b><?echo I18N("h","* If you lock the keyboard caps function?");?></b>
			<p><b><?echo I18N("h","User name and password is case-sensitive, please check the \"Cap Lock\" or \"A\" lights are lit. if yes , please click on the \"Cap Lock\" key, then press \"Previous\" and re-enter the pppoe user name and password, and click \"connect\" to connect to the internet.");?></b></p>
		</div>
		<div>
			<p><b><?echo I18N("h","* If you forgot your pppoe username or password?");?></b>
			<p><b><?echo I18N("h","Please contact with your internet service provider or to check your internet account card, then press \"Previous\" and re-enter the PPPoE user name and password, and click \"Connect\" to connect to the internet.");?></b></p>
		</div>
		<div class="gap"></div>
		<div class="gap"></div>
	</div>
	<div class="gap"></div>
	<table width="100%">
		<tr>
			<td align="center">
				<input type="button" id="complete_config" class="submit_button" value="<? echo I18N("h","Previous");?>" onClick="PAGE.OnClickPreviousButton();" />
			</td>
		</tr>
	</table>
	<div class="gap"></div>
</div>
<!-- end of stage pppoe username/passwd failed -->
<!-- start of stage login success -->
<div id="stage_login_success" style="display:none;">
	<div class="networkmap">
		<h2><?echo I18N("h","SETUP IS COMPLETED");?></h2>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div><p class="text_style2"><?echo I18N("","Congratulations. You have<br> connected to the Internet.");?></p></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
		<div class="gap"></div>
	</div>
	<div class="gap"></div>
	<table width="100%">
		<tr>
			<td align="center">
				<input type="button" id="complete_config" class="submit_button" value="<? echo I18N("h","Complete Configure");?>" onClick="PAGE.OnClickCCButton();" />
			</td>
		</tr>
	</table>
	<div class="gap"></div>
</div>
<!-- end of stage login success -->
</form>
