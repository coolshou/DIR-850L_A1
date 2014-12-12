	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Internet Connection");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr<?if ($FEATURE_NOAPMODE=="1") echo ' style="display:none;"';?>>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Access Point Mode");?></h2></th>
		</tr>
		<tr<?if ($FEATURE_NOAPMODE=="1") echo ' style="display:none;"';?>>
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Use this to disable NAT on the router and turn it into an Access Point.");?></cite>
				<cite>
					<input type="checkbox" id="brmode" onclick='PAGE.OnClickBRMode("checkbox");' />
					<?echo I18N("h","Enabled Access Point Mode");?>
				</cite>
			</td>
		</tr>
		<tr>
			<?
			if ($FEATURE_NOAPMODE=="1")
				echo '<th colspan="2" class="rc_gray5_hd"><h2>';
			else
				echo '<td colspan="2" class="gray_bg border_2side"><p class="subitem">';
			echo I18N("h","Internet Connection Type");
			if ($FEATURE_NOAPMODE=="1")
				echo '</h2></th>';
			else
				echo '</p></td>';
			?>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Choose your router's Internet connection mode.");?></cite>
			</td>
		</tr>
		<tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","My Internet Connection is");?> :</td>
			<td width="76%" class="gray_bg border_right">
			<select name="select" class="text_block" id="wan_ip_mode" onchange="PAGE.OnChangeWanIpMode();">
				<option value="static"><?echo I18N("h","Static IP");?></option>
				<option value="dhcp"><?echo I18N("h","Dynamic IP (DHCP)");?></option>
				<?if ($FEATURE_DHCPPLUS=="1") echo '<option value="dhcpplus">'.I18N("h","DHCP Plus")." (".I18N("h","User name/Password").')</option>';?>
				<option value="pppoe"><?echo I18N("h","PPPoE (User name/Password)");?></option>
				<?if ($FEATURE_NOPPTP!="1") echo '<option value="pptp">'.I18N("h","PPTP")." (".I18N("h","User name/Password").')</option>';?>
				<?if ($FEATURE_NOL2TP!="1") echo '<option value="l2tp">'.I18N("h","L2TP")." (".I18N("h","User name/Password").')</option>';?>
				<?if ($FEATURE_NORUSSIAPPPOE!="1") echo '<option value="r_pppoe">'.I18N("h","Russia PPPoE (Dual Access)").'</option>';?>
				<?if ($FEATURE_NORUSSIAPPTP!="1") echo '<option value="r_pptp">'.I18N("h","Russia PPTP (Dual Access)").'</option>';?>
				<?if ($FEATURE_NORUSSIAL2TP!="1") echo '<option value="r_l2tp">'.I18N("h","Russia L2TP (Dual Access)").'</option>';?>
			</select></td>
		</tr>
<!-- Static IP: Start -->
<!-- tag tr and td has no property "name" in IE, so add "id" for compatibility. -->
		<tr id="static" name="static" style="display:none;">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Static IP");?></p></td>
		</tr>
		<tr id="static" name="static" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Enter the static address information provided by your Internet Service Provider (ISP).");?></cite>
			</td>
		</tr>
		<tr id="static" name="static" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("fixed_ipadr");?>
				<?drawlabel("fixed_ipadr");?>
			</td>
		</tr>
		<tr id="static" name="static" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Subnet Mask");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("fixed_mask");?>
				<?drawlabel("fixed_mask");?>
			</td>
		</tr>
		<tr id="static" name="static" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Default Gateway");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("fixed_gw");?>
				<?drawlabel("fixed_gw");?>
			</td>
		</tr>
<!-- Static IP: End -->
<!-- DHCP: Start -->
		<tr id="dhcp" name="dhcp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Dynamic IP (DHCP)");?></p></td>
		</tr>
		<tr id="dhcp" name="dhcp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Use this Internet connection type if your Internet Service Provider (ISP) didn't provide you with IP Address information and/or a username and password.");?></cite>
			</td>
		</tr>
		<tr id="dhcp" name="dhcp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Host Name");?> :</td>
			<td class="gray_bg border_right">
				<input id="dhcp_hostname" type="text" size="20" maxlength="15" class="text_block" />
				<?drawlabel("dhcp_hostname");?>
			</td>
		</tr>
		<tr id="dhcpplus" name="dhcpplus" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","User Name");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="dhcpplus_user" class="text_block" size="20" maxlength="63" />
				<?drawlabel("dhcpplus_user");?>
			</td>
		</tr>
		<tr id="dhcpplus" name="dhcpplus" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" id="dhcpplus_pwd" class="text_block" size="20" maxlength="63" />
				<?drawlabel("dhcpplus_pwd");?>
			</td>
		</tr>
		<tr id="dhcpplus" name="dhcpplus" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Confirm Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" class="text_block" id="dhcpplus_pwd2" size="20" maxlength="63" />
			</td>
		</tr>
		<tr id="dhcp" name="dhcp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Use Unicasting");?> :</td>
			<td class="gray_bg border_right">
				<input type="checkbox" id="dhcp_unicast" />&nbsp;&nbsp;
				<?echo I18N("h","(compatibility for some DHCP Servers)");?>
			</td>
		</tr>
		<tr id="dhcp" name="dhcp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DNS Mode");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" name="dhcp_dns" id="dhcp_dns_isp" onclick="PAGE.OnClickDnsMode();" checked />
				<label for="dhcp_dns_isp"><?echo I18N("h","Receive DNS from ISP");?></label>
				<input type="radio" name="dhcp_dns" id="dhcp_dns_manual" onclick="PAGE.OnClickDnsMode();" />
				<label for="dhcp_dns_manual"><?echo I18N("h","Enter DNS Manually");?></label>
			</td>
		</tr>
<!-- DHCP: End -->
<!-- IPv4 Common: Start -->
		<tr id="ipv4_comm" name="ipv4_comm" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Primary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("ipv4_dns1");?>
				<?drawlabel("ipv4_dns1");?>
			</td>
		</tr>
		<tr id="ipv4_comm" name="ipv4_comm" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Secondary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("ipv4_dns2");?>
				(<?echo I18N("h","optional");?>)
				<?drawlabel("ipv4_dns2");?>
			</td>
		</tr>
		<tr id="ipv4_comm" name="ipv4_comm" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MTU");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="ipv4_mtu" size="5" class="text_block" />
				<?drawlabel("ipv4_mtu");?>
			</td>
		</tr>
<!-- IPv4 Common: End -->
<!-- PPPoE: Start -->
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<p class="subitem">
					<span id="title_pppoe" style="display:none;"><?echo I18N("h","PPPoE");?></span>
					<span id="title_r_pppoe" style="display:none;"><?echo I18N("h","Russia PPPoE");?></span>
				</p>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Enter the information provided by your Internet Service Provider (ISP).");?></cite>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td class="td_right gray_bg border_left">&nbsp;</td>
			<td class="gray_bg border_right">
				<input type="radio" name="pppoe_addr" id="pppoe_dyn" onclick="PAGE.OnClickAddrType('pppoe');" />
				<label id="dyn_pppoe" for="pppoe_dyn"><?echo I18N("h","Dynamic PPPoE");?></label>
				<input type="radio" name="pppoe_addr" id="pppoe_static" onclick="PAGE.OnClickAddrType('pppoe');" />
				<label id="static_pppoe" for="pppoe_static"><?echo I18N("h","Static PPPoE");?></label>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","User Name");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pppoe_user" class="text_block" size="20" maxlength="63" />
				<span id="r_pppoe_mppe" style="display:none" >&nbsp;<?echo I18N("h","MPPE");?> :
					<input type="checkbox" id="pppoe_mppe" />
				</span>
				<?drawlabel("pppoe_user");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" id="pppoe_pwd" class="text_block" size="20" maxlength="63" />
				<?drawlabel("pppoe_pwd");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Confirm Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" class="text_block" id="pppoe_pwd2" size="20" maxlength="63" />
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Service Name");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pppoe_svcname" class="text_block" size="30" maxlength="39" />
				<?drawlabel("pppoe_svcname");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pppoe_ipadr");?>
				<?drawlabel("pppoe_ipadr");?>
			</td>
		</tr>
		<tr id="r_pppoe" name="r_pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Subnet Mask");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pppoe_mask");?>
				<?drawlabel("pppoe_mask");?>
			</td>
		</tr>
		<tr id="r_pppoe" name="r_pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Gateway IP");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pppoe_gw");?>
				<?drawlabel("pppoe_gw");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DNS Mode");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" name="pppoe_dns" id="pppoe_dns_isp" onclick="PAGE.OnClickDnsMode();" checked />
				<label for="pppoe_dns_isp"><?echo I18N("h","Receive DNS from ISP");?></label>
				<input type="radio" name="pppoe_dns" id="pppoe_dns_manual" onclick="PAGE.OnClickDnsMode();" />
				<label for="pppoe_dns_manual"><?echo I18N("h","Enter DNS Manually");?></label>
			</td>
		</tr>
		<tr<?if ($FEATURE_FAKEOS!="1") echo ' id="pppoe" name="pppoe"';?> style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
			<td class="gray_bg border_right">
				<input type="checkbox" id="en_fakeos" />
				<label for="en_fakeos"><?echo I18N("h","I want to use Netblock.");?></label>
			</td>
		</tr>
<!-- PPPoE: End -->
<!-- PPTP: Start -->
		<tr id="pptp" name="pptp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<p class="subitem">
					<span id="title_pptp" style="display:none;"><?echo I18N("h","PPTP");?></span>
					<span id="title_r_pptp" style="display:none;"><?echo I18N("h","Russia PPTP");?></span>
				</p>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Enter the information provided by your Internet Service Provider (ISP).");?></cite>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td class="td_right gray_bg border_left">&nbsp;</td>
			<td class="gray_bg border_right">
				<input type="radio" name="pptp_addr" id="pptp_dyn" onclick="PAGE.OnClickAddrType('pptp');" />
				<label id="dyn_pptp" for="pptp_dyn"><?echo I18N("h","Dynamic PPTP");?></label>
				<input type="radio" name="pptp_addr" id="pptp_static" onclick="PAGE.OnClickAddrType('pptp');" />
				<label id="static_pptp" for="pptp_static"><?echo I18N("h","Static PPTP");?></label>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","User Name");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pptp_user" class="text_block" size="20" maxlength="63" />
				<span id="r_pptp_mppe" style="display:none" >&nbsp;<?echo I18N("h","MPPE");?> :
					<input type="checkbox" id="pptp_mppe" />
				</span>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" id="pptp_pwd" class="text_block" size="20" maxlength="63" />
				<?drawlabel("pptp_pwd");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Confirm Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" class="text_block" id="pptp_pwd2" size="20" maxlength="63" />
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PPTP IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_ipadr");?>
				<?drawlabel("pptp_ipadr");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PPTP Subnet Mask");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_mask");?>
				<?drawlabel("pptp_mask");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PPTP Gateway IP");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_gw");?>
				<?drawlabel("pptp_gw");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","PPTP Server IP");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_server");?>
				<?drawlabel("pptp_server");?>
			</td>
		</tr>
<!-- PPTP: End -->
<!-- L2TP: Start -->
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<p class="subitem">
					<span id="title_l2tp" style="display:none;"><?echo I18N("h","L2TP");?></span>
					<span id="title_r_l2tp" style="display:none;"><?echo I18N("h","Russia L2TP");?></span>
				</p>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h","Enter the information provided by your Internet Service Provider (ISP).");?></cite>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td class="td_right gray_bg border_left">&nbsp;</td>
			<td class="gray_bg border_right">
				<input type="radio" name="l2tp_addr" id="l2tp_dyn" onclick="PAGE.OnClickAddrType('l2tp');" />
				<label id="dyn_l2tp" for="l2tp_dyn"><?echo I18N("h","Dynamic L2TP");?></label>
				<input type="radio" name="l2tp_addr" id="l2tp_static" onclick="PAGE.OnClickAddrType('l2tp');" />
				<label id="static_l2tp" for="l2tp_static"><?echo I18N("h","Static L2TP");?></label>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","User Name");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="l2tp_user" class="text_block" size="20" maxlength="63" />
				<?drawlabel("l2tp_user");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" id="l2tp_pwd" class="text_block" size="20" maxlength="63" />
				<?drawlabel("l2tp_pwd");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Confirm Password");?> :</td>
			<td class="gray_bg border_right">
				<input type="password" class="text_block" id="l2tp_pwd2" size="20" maxlength="63" />
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","L2TP IP Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_ipadr");?>
				<?drawlabel("l2tp_ipadr");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","L2TP Subnet Mask");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_mask");?>
				<?drawlabel("l2tp_mask");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","L2TP Gateway IP");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_gw");?>
				<?drawlabel("l2tp_gw");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","L2TP Server IP");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_server");?>
				<?drawlabel("l2tp_server");?>
			</td>
		</tr>
<!-- L2TP: End -->
		<tr id="macclone">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MAC Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputmac("maccpy");?>
				(<?echo I18N("h","optional");?>)<br />
				<button id="button" title="<?echo I18N("h","You can use this button to automatically copy the MAC address to your device.");?>" onclick="PAGE.OnClickMacButton();"><?echo I18N("h","Clone MAC Address");?></button>
				<?drawlabel("maccpy");?>
			 </td>
		</tr>
<!-- PPPoE 2: Start -->
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Primary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pppoe_dns1");?>
				<?drawlabel("pppoe_dns1");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Secondary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pppoe_dns2");?>
				(<?echo I18N("h","optional");?>)
				<?drawlabel("pppoe_dns2");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Maximum Idle Time");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pppoe_timeout" size="5" class="text_block" />
				<?echo I18N("h","Minutes");?>
				<?drawlabel("pppoe_timeout");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MTU");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pppoe_mtu" size="5" class="text_block" />
				<?drawlabel("pppoe_mtu");?>
			</td>
		</tr>
		<tr id="pppoe" name="pppoe" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Connect Mode");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" name="pppoe_recon" id="pppoe_always" onclick="PAGE.OnClickReconn('pppoe');" checked />
				<label for="pppoe_always"><?echo I18N("h","Always");?></label>
				<input type="radio" name="pppoe_recon" id="pppoe_manual" onclick="PAGE.OnClickReconn('pppoe');" />
				<label for="pppoe_manual"><?echo I18N("h","Manual");?></label>
				<input type="radio" name="pppoe_recon" id="pppoe_ondemand" onclick="PAGE.OnClickReconn('pppoe');" />
				<label for="pppoe_ondemand"><?echo I18N("h","On-demand");?></label>
			</td>
		</tr>
<!-- PPPoE 2: End -->
<!-- PPTP 2: Start -->
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Primary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_dns1");?>
				<?drawlabel("pptp_dns1");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Secondary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("pptp_dns2");?>
				(<?echo I18N("h","optional");?>)
				<?drawlabel("pptp_dns2");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Maximum Idle Time");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pptp_timeout" size="5" class="text_block" />
				<?echo I18N("h","Minutes");?>
				<?drawlabel("pptp_timeout");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MTU");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="pptp_mtu" size="5" class="text_block" />
				<?drawlabel("pptp_mtu");?>
			</td>
		</tr>
		<tr id="pptp" name="pptp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Connect Mode");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" name="pptp_recon" id="pptp_always" onclick="PAGE.OnClickReconn('pptp');" checked />
				<label for="pptp_always"><?echo I18N("h","Always");?></label>
				<input type="radio" name="pptp_recon" id="pptp_manual" onclick="PAGE.OnClickReconn('pptp');" />
				<label for="pptp_manual"><?echo I18N("h","Manual");?></label>
				<input type="radio" name="pptp_recon" id="pptp_ondemand" onclick="PAGE.OnClickReconn('pptp');" />
				<label for="pptp_ondemand"><?echo I18N("h","On-demand");?></label>
			</td>
		</tr>
<!-- PPTP 2: End -->
<!-- L2TP 2: Start -->
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Primary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_dns1");?>
				<?drawlabel("l2tp_dns1");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Secondary DNS Address");?> :</td>
			<td class="gray_bg border_right">
				<?drawinputipaddr("l2tp_dns2");?>
				(<?echo I18N("h","optional");?>)
				<?drawlabel("l2tp_dns2");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Maximum Idle Time");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="l2tp_timeout" size="5" class="text_block" />
				<?echo I18N("h","Minutes");?>
				<?drawlabel("l2tp_timeout");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MTU");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="l2tp_mtu" size="5" class="text_block" />
				<?drawlabel("l2tp_mtu");?>
			</td>
		</tr>
		<tr id="l2tp" name="l2tp" style="display:none;">
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Connect Mode");?> :</td>
			<td class="gray_bg border_right">
				<input type="radio" name="l2tp_recon" id="l2tp_always" onclick="PAGE.OnClickReconn('l2tp');" checked />
				<label for="l2tp_always"><?echo I18N("h","Always");?></label>
				<input type="radio" name="l2tp_recon" id="l2tp_manual" onclick="PAGE.OnClickReconn('l2tp');" />
				<label for="l2tp_manual"><?echo I18N("h","Manual");?></label>
				<input type="radio" name="l2tp_recon" id="l2tp_ondemand" onclick="PAGE.OnClickReconn('l2tp');" />
				<label for="l2tp_ondemand"><?echo I18N("h","On-demand");?></label>
			</td>
		</tr>
<!-- L2TP 2: End -->
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button id="topsave" value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();">
					<b><?echo I18N("h","Save");?></b>
				</button>
				<button id="topcancel" value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';">
					<b><?echo I18N("h","Cancel");?></b>
				</button>
			</td>
		</tr>
		</table>
	</div> 
	</form>
