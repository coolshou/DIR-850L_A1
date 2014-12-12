	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Device Info");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>

		<div class="rc_gradient_hd" style="clear:both">
			<h2><?echo I18N("h","Device Information");?></h2></div>
		<div class="rc_gradient_bd ">
			<h6>All of your Internet and network connection details are displayed on this page. The firmware version is also displayed here. 
				<br />
			</h6>
		</div>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="adv">
			<tr>
				<td>&nbsp;</td>
				<td>&nbsp;</td>
				<td>&nbsp;</td>
			</tr>
			<tr>
			<th width="49%" class="rc_gray_hd"><p class="adv_icon application"><?echo I18N("h","General");?></p></th>
			<td width="2%">&nbsp;</td>
			<th width="49%" class="rc_gray_hd"><p class="adv_icon network"><?echo I18N("h","WAN");?></p></th>
			</tr>
			<tr>
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellpadding="0" cellspacing="0" class="status_report">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Time");?> :</td>
							<td width="76%"><span class="value" id="st_time"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Firmware Version");?> :</td>
							<td width="76%"><span class="value"><?echo query("/runtime/device/firmwareversion").' '.query("/runtime/device/firmwarebuilddate");?></span></td>
						</tr>
					</table>
				</td>
				<td width="2%">&nbsp;</td>
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellspacing="0" cellpadding="0" class="status_report" id="wan_ethernet_block" style="display:none">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Connection Type");?> :</td>
							<td width="76%"><span class="value" id="st_wantype"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Cable Status");?> :</td>
							<td width="76%"><span class="value" id="st_wancable"></span></td>
						</tr>
<!--						
						<tr id="wan_failover_block">
							<td width="24%" nowrap="nowrap"><?echo I18N("h","WAN Failover Status");?> :</td>
							<td width="76%"><span class="value" id="st_wan_failover"></span></td>
						</tr>
-->						
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Network Status");?> :</td>
							<td width="76%"><span class="value" id="st_networkstatus"></span></td>
						</tr>
						<tr id="st_wan_dhcp_action" style="display:none;">
							<td width="24%" nowrap="nowrap"></td>
							<td width="38%">
								<input type="button" id="st_wan_dhcp_renew" value="<?echo I18N("h","Renew");?>" onClick="PAGE.DHCP_Renew();"/>
							</td>
							<td width="38%">
								<input type="button" id="st_wan_dhcp_release" value="<?echo I18N("h","Release");?>" onClick="PAGE.DHCP_Release();"/> 
							</td>
						</tr>
							<tr id="st_wan_ppp_action" style="display:none;">
								<td width="24%" nowrap="nowrap"></td>
								<td width="38%">
									<input type="button" id="st_wan_ppp_connect" value="<?echo I18N("h","Connect");?>" onClick="PAGE.PPP_Connect();"/>
								</td>
								<td width="38%">
									<input type="button" id="st_wan_ppp_disconnect" value="<?echo I18N("h","Disconnect");?>" onClick="PAGE.PPP_Disconnect();"/> 
								</td>
							</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Connection Up Time");?> :</td>
							<td width="76%"><span class="value" id="st_connection_uptime"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","MAC Address");?> :</td>
							<td width="76%"><span class="value" id="st_wan_mac"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><span class="name" id= "name_wanipaddr"></span> :</td>
							<td width="76%"><span class="value" id="st_wanipaddr"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Subnet Mask");?> :</td>
							<td width="76%"><span class="value" id="st_wannetmask"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><span class="name" id= "name_wangateway"></span> :</td>
							<td width="76%"><span class="value" id="st_wangateway"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Primary DNS Server");?> :</td>
							<td width="76%"><span class="value" id="st_wanDNSserver"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Secondary DNS Server");?> :</td>
							<td width="76%"><span class="value" id="st_wanDNSserver2"></span></td>
						</tr>
					</table>
				</td>
			</tr>
			<tr>
				<td>&nbsp;</td>
				<td>&nbsp;</td>
				<td>&nbsp;</td>
			</tr>
<?
	if (isfile("/htdocs/webinc/body/st_device_3G.php")==1)
		dophp("load", "/htdocs/webinc/body/st_device_3G.php");
?>
			<tr>
				<th width="49%" class="rc_gray_hd"><p class="adv_icon lan"><?echo I18N("h","LAN");?></p></th>
				<td>&nbsp;</td>
				<th class="rc_gray_hd"><p class="adv_icon wireless"><?echo I18N("h","WIRELESS LAN");?></p></th>
			</tr>
			<tr>
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellspacing="0" cellpadding="0" class="status_report" id="lan_ethernet_block" style="display:none">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","MAC Address");?> :</td>
							<td width="76%"><span class="value"><?echo toupper(query("/runtime/devdata/lanmac"));?></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","IP Address");?> :</td>
							<td width="76%"><span class="value" id="st_ip_address"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Subnet Mask");?> :</td>
							<td width="76%"><span class="value" id="st_netmask"></span></td>
						</tr>
						<tr>
							<td width="24%"><?echo I18N("h","DHCP Server");?> :</td>
							<td width="76%"><span class="value" id="st_dhcpserver_enable"></span></td>
						</tr>
					</table>
					<table border="0" cellspacing="0" cellpadding="0" class="status_report" id="ethernet_block" style="display:none">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Connection Type");?> :</td>
							<td width="76%"><span class="value" id="br_wantype"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","MAC Address");?> :</td>
							<td width="76%"><span class="value"><?echo toupper(query("/runtime/devdata/lanmac"));?></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","IP Address");?> :</td>
							<td width="76%"><span class="value" id="br_ipaddr"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Subnet Mask");?> :</td>
							<td width="76%"><span class="value" id="br_netmask"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Default Gateway");?> :</td>
							<td width="76%"><span class="value" id="br_gateway"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Primary DNS Server");?> :</td>
							<td width="76%"><span class="value" id="br_dns1"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Secondary DNS Server");?> :</td>
							<td width="76%"><span class="value" id="br_dns2"></span></td>
						</tr>
					</table>
				</td>
				<td width="2%">&nbsp;</td>
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellspacing="0" cellpadding="0" class="status_report">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Wireless Radio");?> :</td>
							<td width="76%"><span class="value" id="st_wireless_radio"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","MAC Address");?> :</td>
							<td width="76%"><span class="value"><?echo toupper(query("/runtime/devdata/wlanmac"));?></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","802.11 Mode");?> :</td>
							<td width="76%"><span class="value" id="st_80211mode"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Channel Width");?> :</td>
							<td width="76%"><span class="value" id="st_Channel_Width"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Channel");?> :</td>
							<td width="76%"><span class="value" id="st_Channel"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Network Name (SSID)");?> :</td>
							<td width="76%"><span class="value" id="st_SSID"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Security");?> :</td>
							<td width="76%"><span class="value" id="st_security"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Wireless Radio");?> :</td>
							<td width="76%"><span class="value" id="gz_st_wireless_radio"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Network Name (SSID)");?> :</td>
							<td width="76%"><span class="value" id="gz_st_SSID"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Security");?> :</td>
							<td width="76%"><span class="value" id="gz_st_security"></span></td>
						</tr>
					</table>
					<table border="0" cellspacing="0" cellpadding="0" class="status_report" id="wlan2" style="display:none">
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Wireless Radio");?> :</td>
							<td width="76%"><span class="value" id="st_wireless_radio_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","MAC Address");?> :</td>
							<td width="76%"><span class="value"><?echo query("/runtime/devdata/wlanmac2");?></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","802.11 Mode");?> :</td>
							<td width="76%"><span class="value" id="st_80211mode_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Channel Width");?> :</td>
							<td width="76%"><span class="value" id="st_Channel_Width_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Channel");?> :</td>
							<td width="76%"><span class="value" id="st_Channel_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Network Name (SSID)");?> :</td>
							<td width="76%"><span class="value" id="st_SSID_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Wi-Fi Protected Setup");?> :</td>
							<td width="76%"><span class="value" id="st_WPS_status_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Security");?> :</td>
							<td width="76%"><span class="value" id="st_security_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Wireless Radio");?> :</td>
							<td width="76%"><span class="value" id="gz_st_wireless_radio_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Network Name (SSID)");?> :</td>
							<td width="76%"><span class="value" id="gz_st_SSID_Aband"></span></td>
						</tr>
						<tr>
							<td width="24%" nowrap="nowrap"><?echo I18N("h","Guest Zone Security");?> :</td>
							<td width="76%"><span class="value" id="gz_st_security_Aband"></span></td>
						</tr>
					</table>
				</td>
			</tr>
			<tr  style="display:none"> <!-- Hide belows cause of miiiCase sample page no these item -->
				<td>&nbsp;</td>
				<td>&nbsp;</td>
				<td>&nbsp;</td>
			</tr> 
      <tr style="display:none">
			<th width="49%" class="rc_gray_hd"><p class="adv_icon application"><?echo I18N("h","LAN COMPUTERS");?></p></th>
			<td width="2%">&nbsp;</td>
			<th width="49%" class="rc_gray_hd"><p class="adv_icon network"><?echo I18N("h","IGMP MULTICAST MEMBERSHIPS");?></p></th>
			</tr>
			<tr style="display:none">
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellpadding="0" cellspacing="0" class="status_report" id="client_list">
          	<tr>
							<th class="rc_gradient_bd"><?echo I18N("h","MAC Address");?></th>
							<th class="rc_gradient_bd"><?echo I18N("h","IP Address");?></th>			
							<th class="rc_gradient_bd"><?echo I18N("h","Name(if any)");?></th>			
						</tr>
					</table>
				</td>
				<td width="2%">&nbsp;</td>
				<td width="49%" class="rc_gray5_ft_lt padding_aisle">
					<table border="0" cellspacing="0" cellpadding="0" class="status_report" id="igmp_groups" style="display:none">
          	<tr>
							<th class="rc_gradient_bd"><?echo I18N("h","IPv4 Multicast Group Address");?></th>
						</tr>
					</table>
				</td>
			</tr>	<!-- Hide belows cause of miiiCase sample page no these item -->     
		</table>
    <table width="100%">
			<tr>
				<td class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="BODY.OnReload();"><b><?echo I18N("h","Refresh");?></b></button>
				</td>
			</tr>
    </table>

	</div>
	</form>
