<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">
<!-- Start of Stage Description -->
<div id="stage_desc" class="blackbox" style="display:none;">
	<h2><?echo i18n("Welcome to the D-Link Setup Wizard");?></h2>
	<div><p class="strong">
		<?echo i18n("This wizard will guide you through a step-by-step process to configure your new D-Link router and connect to the Internet.");?>
	</p></div>
	<div>
		<ul>
			<li><?echo i18n("Step 1: Configure your Internet Connection");?></li>
			<li><?echo i18n("Step 2: Configure your Wi-Fi Security");?></li>
			<li><?echo i18n("Step 3: Set your Password");?></li>
			<li><?echo i18n("Step 4: Select your Time Zone");?></li>
			<li><?echo i18n("Step 5: Confirm WI-FI settings");?></li>
			<li><?echo i18n("Step 6: mydlink Registration");?></li>			
		</ul>
	</div>
	<div class="emptyline"></div>
	<div class="centerline">	
		<input type="button" name="b_exit" value="<?echo i18n("Cancel");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
		<input type="button" name="b_next" value="<?echo i18n("Next");?>" onClick="PAGE.OnClickNext();" />
	</div>
	<div class="emptyline"></div>		
</div>
<!-- End of Stage Description -->
<!-- Start of Stage Wan Detect -->
<div id="stage_wan_detect" style="display:none;">
	<div id="wan_detect" style="display:none;">
		<div class="blackbox"> 
			<h2><?echo i18n("Step 1: Configure your Internet Connection");?></h2>
			<div><p class="strong">
				<?echo i18n("Routers is detecting your Internet connection type, please wait...");?>
			</p></div>
			<div align="center">
				<img src="/pic/wan_detect_process_bar.gif" width="300" height="30">
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<div id="cable_fail" style="display:none;">
		<div class="blackbox"> 
			<h2><?echo i18n("Step 1: Configure your Internet Connection");?></h2>
			<div><p class="strong">
				<?echo i18n("Please plug one end of the included Ethernet cable that came with your router into the port labeled INTERNET on the back of the router. Plug the other end of this cable into the Ethernet port on your modem and power cycle the modem.");?>
			</p></div>
			<div align="center">
				<img src="/pic/cable_connection.jpg">
			</div>
			<div class="gap"></div>
			<div class="centerline">
				<input type="button" value="<?echo i18n("Cancel");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
				<input type="button" value="<?echo i18n("Prev");?>" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;
				<input type="button" value="<?echo i18n("Connect");?>" onClick="PAGE.WanDetectAgain();" />&nbsp;&nbsp;
			</div>
			<div class="gap"></div>		
		</div>	
	</div>
	<div id="wantype_unknown" style="display:none;">
		<div class="blackbox"> 
			<h2><?echo i18n("Step 1: Configure your Internet Connection");?></h2>
			<div><p class="strong">
				<?echo i18n("Router is unable to detect your Internet connection type.");?>
			</p></div>
			<div class="gap"></div>
			<div class="centerline">
				<input type="button" value="<?echo i18n("Cancel");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
				<input type="button" value="<?echo i18n("Try again");?>" onClick="PAGE.WanDetectAgain();" />&nbsp;&nbsp;
				<input type="button" id="b_next" value="<?echo i18n("Guide me through the Internet connection settings");?>" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;
			</div>
			<div class="gap"></div>	
		</div>
	</div>		
</div>
<!-- End of Stage Wan Detect -->
<!-- Start of Stage Ethernet -->
<div id="stage_ether" style="display:none;">
<div class="blackbox">
	<h2><?echo i18n("Step 1: Configure your Internet Connection");?></h2>
	<div class="gap"></div>
	<div><p class="strong">
		<?echo i18n("If your Internet Service Provider was not listed or you don't know who it is, please select the Internet connection type below:");?>
	</p></div>
	<div class="gap"></div>	
	<div class="wiz-l1">
		<input name="wan_mode" type="radio" value="DHCP" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("DHCP Connection (Dynamic IP Address)");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Choose this if your Internet connection automatically provides you with an IP Address. Most Cable Modems use this type of connection.");?>
	</div>
	<div class="wiz-l1"<?if ($FEATURE_DHCPPLUS!="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="DHCPPLUS" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("DHCP Plus Connection (Dynamic IP Address)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_DHCPPLUS!="1") echo ' style="display:none;"';?>>
		<?echo i18n("Choose this if your Internet connection automatically provides you with an IP Address. Most Cable Modems use this type of connection.");?>
	</div>
	<div class="wiz-l1">
		<input name="wan_mode" type="radio" value="PPPoE" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Username / Password Connection (PPPoE)");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Choose this option if your Internet connection requires a username and password to get online. Most DSL modems use this type of connection.");?>
	</div>
	<div class="wiz-l1"<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="PPTP" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Username / Password Connection (PPTP)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>>
		<?echo i18n("PPTP client.");?>
	</div>
	<div class="wiz-l1"<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="L2TP" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Username / Password Connection (L2TP)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>>
		<?echo i18n("L2TP client.");?>
	</div>
	<div class="wiz-l1">
		<input name="wan_mode" type="radio" value="STATIC" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Static IP Address Connection");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Choose this option if your Internet Setup Provider provided you with IP Address information that has to be manually configured.");?>
	</div>
	<div class="wiz-l1"<?if ($FEATURE_NORUSSIAPPTP=="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="R_PPTP" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Russia PPTP (Dual Access)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_NORUSSIAPPTP=="1") echo ' style="display:none;"';?>>
		<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network. Certain ISPs in Russia use this type of connection.");?>
	</div>
	<div class="wiz-l1"<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="R_L2TP" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Russia L2TP (Dual Access)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>>
		<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network. Certain ISPs in Russia use this type of connection.");?>
	</div>	
	<div class="wiz-l1"<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>>
		<input name="wan_mode" type="radio" value="R_PPPoE" onClick="PAGE.OnChangeWanType(this.value);" />
		<?echo i18n("Russia PPPoE (Dual Access)");?>
	</div>
	<div class="wiz-l2"<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>>
		<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network. Certain ISPs in Russia use this type of connection.");?>
	</div>
	<? wiz_buttons();?>
</div>
</div>
<!-- End of Stage Ethernet -->
<!-- Start of Stage Ethernet WAN Settings -->
<div id="stage_ether_cfg" style="display:none;">
	<input id="ppp4_timeout" type="hidden" />
	<input id="ppp4_mode" type="hidden" />
	<input id="ppp4_mtu" type="hidden" />
	<input id="ipv4_mtu" type="hidden" />
	<!-- Start of DHCP -->
	<div id="DHCP">
		<div class="blackbox">
			<h2><?echo i18n("DHCP Connection (Dynamic IP Address)");?></h2>
			<div><p class="strong">
				<?echo I18N("h","To set up this connection, please make sure that you are connected to the D-Link Router with the PC that was originally connected to your broadband connection. If you are, then click the Clone MAC button to copy your computer's MAC Address to the D-Link Router.");?>
			</p></div>
			<div class="textinput">
				<span class="name"><?echo i18n("MAC Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_mac" type="text" size="20" maxlength="17" />
					<?echo i18n("(optional)");?>
				</span>
			</div>
			<div class="textinput">
				<span class="name"></span>
				<span class="delimiter"></span>
				<span class="value">
					<input type="button" value="<?echo i18n("Copy Your PC's MAC Address");?>" onClick="PAGE.OnClickCloneMAC();" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Host Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_host" type="text" size="25" maxlength="39" />
				</span>
			</div>
			<div id="DHCPPLUS" style="display:none">
				<div class="textinput">
					<span class="name"><?echo i18n("Username");?></span>
					<span class="delimiter">:</span>
					<span class="value">
						<input id="wiz_dhcpplus_user" type="text" size="25" maxlength="63" />
					</span>
				</div>
				<div class="textinput">
					<span class="name"><?echo i18n("Password");?></span>
					<span class="delimiter">:</span>
					<span class="value">
						<input id="wiz_dhcpplus_pass" type="password" size="25" maxlength="63" />
					</span>
				</div>
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<!-- End of DHCP -->
	<!-- Start of PPPoE -->
	<div id="PPPoE">
		<div class="blackbox">
			<h2><?echo i18n("Set Username and Password Connection (PPPoE)");?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider. If you do not have this information, please contact your ISP.");?>
			</p></div>
			<div class="textinput" style="display:none;">
				<span class="name"><?echo i18n("Address Mode");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input name="wiz_pppoe_conn_mode" type="radio" value="dynamic" checked onChange="PAGE.OnChangePPPoEMode();" />
					<?echo i18n("Dynamic IP");?>
					<span class="value">
					<input name="wiz_pppoe_conn_mode" type="radio" value="static" onChange="PAGE.OnChangePPPoEMode();" />
					<?echo i18n("Static IP");?>
				</span>
				</span>
			</div>
			<div class="textinput" style="display:none;">
				<span class="name"><?echo i18n("IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pppoe_ipaddr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("User Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pppoe_usr" type="text" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pppoe_passwd" type="text" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput" style="display:none;">
				<span class="name"><?echo i18n("Verify Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pppoe_passwd2" type="password" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput" style="display:none;">
				<span class="name"><?echo i18n("Service Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pppoe_svc" type="text" size="20" maxlength="39" />
					<?echo i18n("(optional)");?>
				</span>
			</div>
			<div style="display:none;">
			<p>
				<?echo i18n("Note: You may also need to provide a Service Name. If you do not have or know this information, please contact your ISP.");?>
			</p>
			</div>
			<? wiz_buttons();?>
			<div class="gap"></div>
		</div>
		<div id="R_PPPoE" class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("WAN Physical Settings");?></h2>
			<div class="textinput">
				<span class="name"></span>
				<span class="delimiter"></span>
				<span class="value">
					<input name="wiz_rpppoe_conn_mode" type="radio" value="dynamic" checked onChange="PAGE.OnChangeRussiaPPPoEMode();" />
					<?echo i18n("Dynamic IP");?>
					<input name="wiz_rpppoe_conn_mode" type="radio" value="static" onChange="PAGE.OnChangeRussiaPPPoEMode();" />
					<?echo i18n("Static IP");?>
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_rpppoe_ipaddr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Subnet Mask");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_rpppoe_mask" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Gateway");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_rpppoe_gw" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Primary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_rpppoe_dns1" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Secondary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_rpppoe_dns2" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<!-- End of PPPoE -->
	<!-- Start of PPTP -->
	<div id="PPTP">
		<div class="blackbox">
			<h2><?echo i18n("Set Username and Password Connection (PPTP)");?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider. You also need PPTP IP address. If you do not have this information, please contact your ISP.");?>
			</p></div>
			<div class="textinput">
				<span class="name"><?echo i18n("Address Mode");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input name="wiz_pptp_conn_mode" type="radio" value="dynamic" checked onChange="PAGE.OnChangePPTPMode();" />
					<?echo i18n("Dynamic IP");?>
					<input name="wiz_pptp_conn_mode" type="radio" value="static" onChange="PAGE.OnChangePPTPMode();" />
					<?echo i18n("Static IP");?>
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("PPTP IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_ipaddr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("PPTP Subnet Mask");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_mask" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("PPTP Gateway IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_gw" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("PPTP Server IP Address (may be same as gateway)");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_svr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("User Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_usr" type="text" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_passwd" type="password" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Verify Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_passwd2" type="password" size="20" maxlength="63" />
				</span>
			</div>
			<div class="emptyline"></div>
		</div>
	</div>
	<!-- End of PPTP -->
	<!-- Start of L2TP -->
	<div id="L2TP">
		<div class="blackbox">
			<h2><?echo i18n("Set Username and Password Connection (L2TP)");?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider. You also need L2TP IP address. If you do not have this information, please contact your ISP.");?>
			</p></div>
			<div class="textinput">
				<span class="name"><?echo i18n("Address Mode");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input name="wiz_l2tp_conn_mode" type="radio" value="dynamic" checked onChange="PAGE.OnChangeL2TPMode();" />
					<?echo i18n("Dynamic IP");?>
					<input name="wiz_l2tp_conn_mode" type="radio" value="static" onChange="PAGE.OnChangeL2TPMode();" />
					<?echo i18n("Static IP");?>
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("L2TP IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_ipaddr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("L2TP Subnet Mask");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_mask" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("L2TP Gateway IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_gw" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("L2TP Server IP Address (may be same as gateway)");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_svr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("User Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_usr" type="text" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_passwd" type="password" size="20" maxlength="63" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Verify Password");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_passwd2" type="password" size="20" maxlength="63" />
				</span>
			</div>
			<div class="emptyline"></div>
		</div>
	</div>
	<!-- End of L2TP -->
	<!-- Start of STATIC -->
	<div id="STATIC">
		<div class="blackbox">
			<h2><?echo i18n("Set Static IP Address Connection");?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a complete list of IP information provided by your Internet Service Provider. If you have a Static IP connection and do not have this information, please contact your ISP.");?>
			</p></div>
			<div class="textinput">
				<span class="name"><?echo i18n("IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_ipaddr" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Subnet Mask");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_mask" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Gateway Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_gw" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<br>
		</div>			
		<div class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("DNS settings");?></h2>
			<div class="textinput">
				<span class="name"><?echo i18n("Primary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_dns1" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Secondary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_dns2" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<!-- End of STATIC -->
	<div id="DNS">
		<div class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("DNS settings");?></h2>
			<div class="textinput">
				<span class="name"><?echo i18n("Primary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="dns1" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Secondary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="dns2" type="text" size="20" maxlength="15" />
				</span>
			</div>			
			<div class="gap"></div>
			<? wiz_buttons();?>
			<div class="gap"></div>		
		</div>	
	</div>	
</div>
<!-- End of Stage Ethernet WAN Settings -->
<!-- Start of Stage WLAN settings -->
<div id="stage_wlan_set" class="blackbox" style="display:none;">
	<h2><? echo i18n("Step 2: Configure your Wi-Fi Security");?></h2>
	<div class="gap"></div>
	<div><p id="wifi24_name_pwd_show" class="wiz_strong">
		<? echo i18n("Give your Wi-Fi network a name and a password.")." (2.4GHz Band)";?>
	</p></div>
	<div>
		<span id="fld_ssid_24" name="fld_ssid_24" class="wiz_input"><?echo i18n("Wi-Fi Network Name (SSID)");?></span>
		<span>:</span>
	</div>
	<div class="gap"></div>
	<div>
		<span class="wiz_input">
			<input id="wiz_ssid" type="text" size="32" maxlength="32" />
		</span>	
		<span class="wiz_input_script"><? echo i18n("(Using up to 32 characters)");?></span>	
	</div>
	<div class="gap"></div>
	<div id="wifi24_pwd_show" style="display:none;"><p class="wiz_strong">
		<? echo i18n("Give your Wi-Fi network a password.");?>
	</p></div>
	<div>
		<span class="wiz_input"><?echo i18n("Wi-Fi Password");?></span>
		<span>:</span>
	</div>
	<div class="gap"></div>		
	<div>
		<span class="wiz_input">
			<input id="wiz_key" type="text" size="32" maxlength="63" />
		</span>	
		<span class="wiz_input_script"><? echo i18n("(Between 8 and 63 characters)");?></span>	
	</div>

	<div class="gap"></div>
	<div class="gap"></div>	
	<div class="gap"></div>			
	<div id="div_ssid_A" name="div_ssid_A" style="display:none;">
		<div><p class="wiz_strong">
			<? echo i18n("Give your Wi-Fi network a name and a password.")." (5GHz Band)";?>
		</p></div>
		<div>
			<span id="fld_ssid_5" name="fld_ssid_5" class="wiz_input"><?echo i18n("Wi-Fi Network Name (SSID)");?></span>
			<span>:</span>
		</div>
		<div class="gap"></div>		
		<div>
			<span class="wiz_input">
				<input id="wiz_ssid_Aband" type="text" size="32" maxlength="32" />
			</span>	
			<span class="wiz_input_script"><? echo i18n("(Using up to 32 characters)");?></span>	
		</div>
		<div class="gap"></div>
		<div>
			<span class="wiz_input"><?echo i18n("Wi-Fi Password");?></span>
			<span>:</span>
		</div>
		<div class="gap"></div>			
		<div>
			<span class="wiz_input">
				<input id="wiz_key_Aband" type="text" size="32" maxlength="63" />
			</span>	
			<span class="wiz_input_script"><? echo i18n("(Between 8 and 63 characters)");?></span>	
		</div>
	</div>	
	<?wiz_buttons();?>
</div>
<!-- End of Stage WLAN settings -->
<!-- Start of Stage Password -->
<div id="stage_passwd" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 3: Set your Password");?></h2>
	<div><p class="strong">
		<?echo i18n("By default, your new D-Link Router does not have a password configured for administrator access to the Web-based configuration pages. To secure your new networking device, please set and verify a password below, and enabling CAPTCHA Graphical Authentication provides added security protection to prevent unauthorized online users and hacker software from accessing your network settings.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wiz_passwd" type="password" size="20" maxlength="15" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Verify Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wiz_passwd2" type="password" size="20" maxlength="15" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Graphical Authentication");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_captcha" type="checkbox" /></span>
	</div>	
	<? wiz_buttons();?>
</div>
<!-- End of Stage Password -->
<!-- Start of Stage Time Zone -->
<div id="stage_tz" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 4: Select your Time Zone");?></h2>
	<div><p class="strong">
		<?echo i18n("Select the appropriate time zone for your location. This information is required to configure the time-based options for the router.");?>
	</p></div>
	<div class="gap"></div>
	<div style="text-align:center;">
		<span>
			<select id="wiz_tz">
<?
				foreach ("/runtime/services/timezone/zone")
				{
					echo '\t\t\t<option value="'.get("h","uid").'">'.get("h","name").'</option>\n';
				}
?>			</select>
		</span>
	</div>
	<? wiz_buttons();?>
</div>
<!-- End of Stage Time Zone -->
<!-- Start of Stage WLAN result -->
<div id="stage_wlan_result" class="blackbox" style="display:none;">
	<h2><? echo i18n("Step 5: Confirm WI-FI settings");?></h2>
	<div><p class="strong">
		<? echo i18n("Below is a detailed summary of your Wi-Fi security settings. Please print this page out, or write the information on a piece of paper, so you can configure the correct settings on your Wi-Fi devices.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span id="fld_ssid_24_result" name="fld_ssid_24_result" class="name"><?echo i18n("Wi-Fi Network Name (SSID)")." 2.4GHz";?></span>
		<span class="delimiter">:</span>
		<pre style="font-family:Tahoma"><span id="ssid" class="value"></span></pre>
	</div>
	<div class="textinput">
		<span class="name"><? echo i18n("Wi-Fi Password");?></span>
		<span class="delimiter">:</span>
		<span id="wiz_key_result" class="value" style="word-wrap: break-word;"></span>
	</div>
	
	<div id="div_ssid_A_result" name="div_ssid_A_result" style="display:none;">
		<div class="gap"></div>
		<div class="textinput">
			<span id="fld_ssid_5" name="fld_ssid_5" class="name"><?echo i18n("Wi-Fi Network Name (SSID)")." 5GHz";?></span>
			<span class="delimiter">:</span>
			<pre style="font-family:Tahoma"><span id="ssid_Aband" class="value"></span></pre>
		</div>
		<div class="textinput">
			<span class="name"><? echo i18n("Wi-Fi Password");?></span>
			<span class="delimiter">:</span>
			<span id="wiz_key_result_Aband" class="value" style="word-wrap: break-word;"></span>
		</div>
	</div>

	<div class="gap"></div>
	<div class="emptyline"></div>
		<div class="centerline">
			<input type="button" name="b_exit" value="<? echo i18n("Cancel");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
			<input type="button" name="b_pre" value="<? echo i18n("Prev");?>" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;
			<input type="button" name="b_send" value="<? echo i18n("Next");?>" onClick="BODY.OnSubmit();" />&nbsp;&nbsp; 
		</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage WLAN result -->
<!-- Start of Stage Check internet connection -->
<div id="check_wan_connect" class="blackbox" style="display:none;">
	<h2><?echo i18n("SAVING SETTINGS");?></h2>
	<div class="centerline"><p><? echo i18n("Your settings are being saved.");?></p></div>
	<div class="centerline"><p><? echo i18n("Please wait...");?></p></div>
	<div class="centerline"><p><? echo i18n("Checking internet connectivity.");?></p></div>
</div>
<!-- End of Stage Check internet connection -->
<!-- Start of Stage Check internet connection bar -->
<div id="check_wan_connect_bar" class="blackbox" style="display:none;">
	<h2><?echo i18n("SAVING SETTINGS");?></h2>
	<div><p class="strong"><?echo i18n("Routers is checking Internet connectivity, please wait.");?></p></div>
	<div align="center"><img src="/pic/wan_detect_process_bar.gif" width="300" height="30"></div>
	<div class="emptyline"></div>
		<div class="centerline">
			<input type="button" name="b_exit" value="<? echo i18n("Skip");?>" onClick="PAGE.OnClickSkip();" />&nbsp;&nbsp;
			<input type="button" name="b_send" value="<? echo i18n("Next");?>" onClick="BODY.OnSubmit();" disabled="disabled"/>&nbsp;&nbsp; 
		</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage Check internet connection bar -->
</form>
