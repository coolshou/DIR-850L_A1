<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_send" value="'.i18n("Connect").'" onClick="BODY.OnSubmit();" disabled="true" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">
<!-- Start of Stage Description -->
<div id="stage_desc" class="blackbox" style="display:none;">
	<h2><?echo i18n("Welcome to the D-Link Internet Connection Setup Wizard");?></h2>
	<div><p class="strong">
		<?echo i18n("This wizard will guide you through a step-by-step process to configure your new D-Link router and connect to the Internet.");?>
	</p></div>
	<div>
		<ul>
			<li><?echo i18n("Step 1").": ".i18n("Set your Password");?></li>
			<li><?echo i18n("Step 2").": ".i18n("Select your Time Zone");?></li>
			<li><?echo i18n("Step 3").": ".i18n("Configure your Internet Connection");?></li>
			<li><?echo i18n("Step 4").": ".i18n("Save Settings and Connect");?></li>
		</ul>
	</div>
	<? wiz_buttons();?>
</div>
<!-- End of Stage Description -->
<!-- Start of Stage Password -->
<div id="stage_passwd" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 1").": ".i18n("Set your Password");?></h2>
	<div><p class="strong">
		<?echo i18n("By default, your new D-Link Router does not have a password configured for administrator access to the Web-based configuration pages.")." ".
				i18n("To secure your new networking device, please set and verify a password below:");?>
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
	<? wiz_buttons();?>
</div>
<!-- End of Stage Password -->
<!-- Start of Stage Time Zone -->
<div id="stage_tz" class="blackbox" style="display:none;">
	<h2><?echo i18n("Step 2").": ".i18n("Select your Time Zone");?></h2>
	<div><p class="strong">
		<?echo i18n("Select the appropriate time zone for your location.")." ".
				i18n("This information is required to configure the time-based options for the router.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span class="name"><?echo i18n("Time Zone");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wiz_tz">
<?
				foreach ("/runtime/services/timezone/zone")
				{
					echo '\t\t\t\t<option value="'.query("uid").'">'.query("name").'</option>\n';
				}
?>			</select>
		</span>
	</div>
	<? wiz_buttons();?>
</div>
<!-- End of Stage Time Zone -->
<!-- Start of Stage Ethernet -->
<div id="stage_ether" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("Step 3").": ".i18n("Configure your Internet Connection");?></h2>
		<!--<div class="gap"></div>
		<div><p class="strong">
			<?echo i18n("Your Internet Connection could not be detected, please select your Internet Service Provider (ISP) from the list below. If your ISP is not listed; select the 'Not Listed or Don't Know' option to manually configure your connection.");?>
		</p></div>&nbsp;&nbsp;
		<select id="select_isp" name="select_isp" onChange="PAGE.OnChangeWanTypeAuto(this.value)">
		  <option value="DHCP">Not Listed or Don't Know</option>
		  <option value="DHCP">Adelphia Power Link</option>
		  <option value="PPPoE">ALLTEL DSL</option>
		  <option value="PPPoE">ATAT DSL Service</option>
		  <option value="PPPoE">Bell Sympatico</option>
		  <option value="PPPoE">Bellsouth</option>
		  <option value="DHCP">Charter High-Speed</option>
		  <option value="DHCP">Comcast</option>
		  <option value="PPPoE">Covad</option>
		  <option value="DHCP">Cox Communications</option>
		  <option value="DHCP">Earthlink Cable</option>
		  <option value="PPPoE">Earthlink DSL</option>
		  <option value="PPPoE">FrontierNet</option>
		  <option value="DHCP">Optimum Online</option>
		  <option value="DHCP">RCN</option>
		  <option value="DHCP">Road Runner</option>
		  <option value="DHCP">Rogers Yahoo!</option>
		  <option value="PPPoE">SBC Yahoo! DSL</option>
		  <option value="DHCP">Shaw</option>
		  <option value="PPPoE">Speakeasy</option>
		  <option value="PPPoE">Sprint FastConnect</option>
		  <option value="PPPoE">Telus</option>
		  <option value="DHCP">Time Warner Cable</option>
		  <option value="PPPoE">US West / Qwest</option>
		  <option value="PPPoE">Verizon Online DSL</option>
		  <option value="PPPoE">XO Communications</option>
		</select>
		<div class="gap"></div>-->
		
		<div><p class="strong">
			<!--<?echo i18n("If your Internet Service Provider was not listed or you don't know who it is, please select the Internet connection type below:");?>-->
			<?echo i18n("Please select the Internet connection type below:");?>
		</p></div>
		<div class="wiz-l1">
			<input name="wan_mode" type="radio" value="DHCP" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("DHCP Connection")." (".i18n("Dynamic IP Address").")";?>
		</div>
		<div class="wiz-l2">
			<?echo i18n("Choose this if your Internet connection automatically provides you with an IP Address.")." ".
					i18n("Most Cable Modems use this type of connection.");?>
		</div>
		<div class="wiz-l1"<?if ($FEATURE_DHCPPLUS!="1") echo ' style="display:none;"';?>>
			<input name="wan_mode" type="radio" value="DHCPPLUS" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("DHCP Plus Connection")." (".i18n("Dynamic IP Address").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_DHCPPLUS!="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this if your Internet connection automatically provides you with an IP Address.")." ".
					i18n("Most Cable Modems use this type of connection.");?>
		</div>
		<div class="wiz-l1">
			<input name="wan_mode" type="radio" value="PPPoE" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("Username")." / ".i18n("Password")." ".i18n("Connection")." (".i18n("PPPoE").")";?>
		</div>
		<div class="wiz-l2">
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online. Most DSL modems use this type of connection.");?>
		</div>
		<div class="wiz-l1"<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>>
			<input name="wan_mode" type="radio" value="PPTP" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("Username")." / ".i18n("Password")." ".i18n("Connection")." (".i18n("PPTP").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online. Most DSL modems use this type of connection.");?>
		</div>
		<div class="wiz-l1"<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>>
			<input name="wan_mode" type="radio" value="L2TP" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("Username")." / ".i18n("Password")." ".i18n("Connection")." (".i18n("L2TP").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online. Most DSL modems use this type of connection.");?>
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
			<?echo i18n("Russia PPTP")." (".i18n("Dual Access").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_NORUSSIAPPTP=="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network.")." ".
					i18n("Certain ISPs in Russia use this type of connection.");?>
		</div>
		<div class="wiz-l1"<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>>
			<input name="wan_mode" type="radio" value="R_L2TP" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("Russia L2TP")." (".i18n("Dual Access").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network.")." ".
					i18n("Certain ISPs in Russia use this type of connection.");?>
		</div>		
		<div class="wiz-l1"<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>>
			<input name="wan_mode" type="radio" value="R_PPPoE" onClick="PAGE.OnChangeWanType(this.value);" />
			<?echo i18n("Russia PPPoE")." (".i18n("Dual Access").")";?>
		</div>
		<div class="wiz-l2"<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>>
			<?echo i18n("Choose this option if your Internet connection requires a username and password to get online as well as a static route to access the Internet Service Provider's internal network.")." ".
					i18n("Certain ISPs in Russia use this type of connection.");?>
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
			<h2><?echo i18n("DHCP Connection")." (".i18n("Dynamic IP Address").")";?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection, please make sure that you are connected to the D-Link Router with the PC that was originally connected to your broadband connection.")." ".
						I18N("h","If you are, then click the Clone MAC button to copy your computer's MAC Address to the D-Link Router.");?>
			</p></div>
			<div class="textinput">
				<span class="name"><?echo i18n("MAC Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_mac" type="text" size="20" maxlength="17" />
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>
			<div class="textinput">
				<span class="name"></span>
				<span class="delimiter"></span>
				<span class="value">
					<input type="button" value="<?echo I18N("h","Clone Your PC's MAC Address");?>" onClick="PAGE.OnClickCloneMAC();" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Host Name");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_host" type="text" size="25" maxlength="15" />
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
			<p>
				<?echo i18n("Note:")." ".i18n("You may also need to provide a Host Name.").
						i18n("If you do not have or know this information, please contact your ISP.");?>
			</p>
			<div class="gap"></div>
		</div>
		<div class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("DNS Settings");?></h2>			
			<div class="textinput">
				<span class="name"><?echo i18n("Primary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_dns1" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Secondary DNS Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_dhcp_dns2" type="text" size="20" maxlength="15" />
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>
			<? wiz_buttons();?>
		</div>

	</div>
	<!-- End of DHCP -->
	<!-- Start of PPPoE -->
	<div id="PPPoE">
		<div class="blackbox">
			<h2><?echo i18n("Set Username and Password Connection")." (".i18n("PPPoE").")";?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider.")." ".
						i18n("If you do not have this information, please contact your ISP.");?>
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
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>
			<div style="display:none;">
			<p>
				<?echo i18n("Note:")." ".i18n("You may also need to provide a Service Name.")." ".
						i18n("If you do not have or know this information, please contact your ISP.");?>
			</p>
			</div>
			<? wiz_buttons();?>
			<div class="gap"></div>
		</div>
		<div id="R_PPPoE" class="blackbox">
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
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<!-- End of PPPoE -->
	<!-- Start of PPTP -->
	<div id="PPTP">
		<div class="blackbox">
			<h2><?echo i18n("Set Username and Password Connection")." (".i18n("PPTP").")";?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider.")." ".
						i18n("You also need PPTP IP address.")." ".i18n("If you do not have this information, please contact your ISP.");?>
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
				<span class="name"><?echo i18n("PPTP Server IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_pptp_svr" type="text" size="20" maxlength="30" />
					<?echo "(".i18n("may be same as gateway").")";?>
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
			<h2><?echo i18n("Set Username and Password Connection")." (".i18n("L2TP").")";?></h2>
			<div><p class="strong">
				<?echo i18n("To set up this connection you will need to have a Username and Password from your Internet Service Provider.")." ".
						i18n("You also need L2TP IP address.")." ".i18n("If you do not have this information, please contact your ISP.");?>
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
				<span class="name"><?echo i18n("L2TP Server IP Address");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_l2tp_svr" type="text" size="20" maxlength="30" />
					<?echo "(".i18n("may be same as gateway").")";?>
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
				<?echo i18n("To set up this connection you will need to have a complete list of IP information provided by your Internet Service Provider.")." ".
						i18n("If you have a Static IP connection and do not have this information, please contact your ISP.");?>
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
				<span class="name"><?echo i18n("Default Gateway");?></span>
				<span class="delimiter">:</span>
				<span class="value">
					<input id="wiz_static_gw" type="text" size="20" maxlength="15" />
				</span>
			</div>
			<br>			
		</div>			
		<div class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("DNS Settings");?></h2>			
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
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>
			<? wiz_buttons();?>
		</div>
	</div>
	<!-- End of STATIC -->
	<div id="DNS">
		<div class="blackbox" style="margin-top:0px;">
			<h2><?echo i18n("DNS Settings");?></h2>
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
					(<?echo I18N("h", "optional");?>)
				</span>
			</div>			
			<div class="gap"></div>
			<? wiz_buttons();?>
			<div class="gap"></div>		
		</div>	
	</div>		
</div>
<!-- End of Stage Ethernet WAN Settings -->
<!-- Start of Stage Finish -->
<div id="stage_finish" class="blackbox" style="display:none;">
	<h2><?echo i18n("Setup Complete!");?></h2>
	<div><p class="strong">
		<?echo i18n("The Internet Connection Setup Wizard has completed.")." ".
				i18n("Click the Connect button to save your settings.");?>
	</p></div>
	<div class="gap"></div>
	<? wiz_buttons();?>
</div>
<!-- End of Stage Finish -->
</form>
