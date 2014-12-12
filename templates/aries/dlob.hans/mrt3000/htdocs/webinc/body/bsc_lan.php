	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Network Settings");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Network Settings");?></h2></th>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","Use this section to configure the internal network settings of your router and also to configure the built-in DHCP server to assign IP addresses to computers on your network.");?>
		<?echo I18N("h","The IP address that is configured here is the IP address that you use to access the Web-based management interface.");?>
		<?echo I18N("h","If you change the IP address in this section, you may need to adjust your PC's network settings to access the network again.");?></cite>
        <p><strong>
		<?echo I18N("h","Please note that this section is optional and you do not need to change any of the settings here to get your network up and running.");?>
	</strong></p>
        	</td>
        </tr>
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Router Settings");?></p></td>
        </tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","Use this section to configure the internal network settings of your router.");?>
		<?echo I18N("h","The IP address that is configured here is the IP address that you use to access the Web-based management interface.");?>
		<?echo I18N("h","If you change the IP address here, you may need to adjust your PC's network settings to access the network again.");?></cite>
            </td>
        </tr>
        <tr>
        	<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Router IP Address");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<!--<input id="ipaddr" type="text" size="20" maxlength="15" />-->
                <input name="ipaddr_" type="text" id="ipaddr_1" maxlength="3" class="ip_add text_block" />.
                <input name="ipaddr_" type="text" id="ipaddr_2" maxlength="3" class="ip_add text_block" />.
                <input name="ipaddr_" type="text" id="ipaddr_3" maxlength="3" class="ip_add text_block" />.
                <input name="ipaddr_" type="text" id="ipaddr_4" maxlength="3" class="ip_add text_block" />
                <?drawlabel("ipaddr_1");?>
            </td>
		</tr>
        <tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Default Subnet Mask");?> :</td>
			<td width="76%" class="gray_bg border_right">
            	<!--<input id="netmask" type="text" size="20" maxlength="15" />-->
                <input name="netmask_" type="text" id="netmask_1" maxlength="3" class="ip_add text_block" />.
                <input name="netmask_" type="text" id="netmask_2" maxlength="3" class="ip_add text_block" />.
                <input name="netmask_" type="text" id="netmask_3" maxlength="3" class="ip_add text_block" />.
                <input name="netmask_" type="text" id="netmask_4" maxlength="3" class="ip_add text_block" />
                <?drawlabel("netmask_1");?>
            </td>
		</tr>
        <tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Host Name");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="device" type="text" size="20" maxlength="15" />
            <?drawlabel("device");?></td>
		</tr>
        <tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Local Domain Name");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="domain" type="text" size="20" maxlength="30" />
			(<?echo I18N("h","optional");?>)<?drawlabel("domain");?></td>
		</tr>
        <tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable DNS Relay");?> :</td>
			<td width="76%" class="gray_bg border_right"><input id="dnsr" type="checkbox" /><?drawlabel("m_dnsr");?></td>
		</tr>
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","DHCP Server Settings");?></p></td>
        </tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","Use this section to configure the built-in DHCP server to assign IP address to the computers on your network.");?></cite>
            </td>
        </tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable DHCP Server");?> :</td>
			<td class="gray_bg border_right"><input id="dhcpsvr" type="checkbox" onClick="PAGE.OnClickDHCPSvr();" /></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DHCP IP Address Range");?> :</td>
			<td class="gray_bg border_right"><input id="startip" type="text" size="3" maxlength="3" /> to
			<input id="endip" type="text" size="3" maxlength="3" />
			(<?echo I18N("h","addresses within the LAN subnet");?>)<?drawlabel("startip");?></td></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","DHCP Lease Time");?> :</td>
			<td class="gray_bg border_right"><input id="leasetime" type="text" size="6" maxlength="5" />
			(<?echo I18N("h","minutes");?>)<?drawlabel("leasetime");?></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Always broadcast");?> :</td>
			<td class="gray_bg border_right"><input name="broadcast" type="checkbox" id="broadcast" />
			(<?echo I18N("h","compatibility for some DHCP Clients");?>)</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","NetBIOS announcement");?> :</td>
			<td class="gray_bg border_right"><input name="netbios_enable" type="checkbox" id="netbios_enable" onclick="PAGE.on_check_netbios();"/></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Learn NetBIOS from WAN");?> :</td>
			<td class="gray_bg border_right"><input name="netbios_learn" type="checkbox" id="netbios_learn" onclick="PAGE.on_check_learn();"/></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","NetBIOS Scope");?> :</td>
			<td class="gray_bg border_right"><input type="text" id="netbios_scope" name="netbios_scope" size="20" maxlength="30" value=""/>
			(<?echo I18N("h","optional");?>)</td>
		</tr>
        
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","NetBIOS node type");?> :</td>
			<td class="gray_bg border_right">
            <input type="radio" name="winstype" value="1" /><?echo I18N("h","Broadcast only");?> (<?echo I18N("h","use when no WINS servers configured");?>)<br />
            <input type="radio" name="winstype" value="2" /><?echo I18N("h","Point-to-Point");?> (<?echo I18N("h","no broadcast");?>)<br />
            <input type="radio" name="winstype" value="4" checked /><?echo I18N("h","Mixed-mode");?> (<?echo I18N("h","Broadcast then Point-to-Point");?>)<br />
            <input type="radio" name="winstype" value="8" /><?echo I18N("h","Hybrid");?> (<?echo I18N("h","Point-to-Point then Broadcast");?>)<br />
            </td>
		</tr>       
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Primary WINS IP Address");?> :</td>
			<td class="gray_bg border_right">
            	<!--<input type="text" id="primarywins" size="20" maxlength="15" value="">-->
                <input name="primarywins_" type="text" id="primarywins_1" maxlength="3" class="ip_add text_block" />.
                <input name="primarywins_" type="text" id="primarywins_2" maxlength="3" class="ip_add text_block" />.
                <input name="primarywins_" type="text" id="primarywins_3" maxlength="3" class="ip_add text_block" />.
                <input name="primarywins_" type="text" id="primarywins_4" maxlength="3" class="ip_add text_block" />
                <?drawlabel("primarywins_1");?>
            </td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Secondary WINS IP Address");?> :</td>
			<td class="gray_bg border_right">
            	<!--<input type="text" id="secondarywins" size="20" maxlength="15" value="">-->
                <input name="secondarywins_" type="text" id="secondarywins_1" maxlength="3" class="ip_add text_block" />.
                <input name="secondarywins_" type="text" id="secondarywins_2" maxlength="3" class="ip_add text_block" />.
                <input name="secondarywins_" type="text" id="secondarywins_3" maxlength="3" class="ip_add text_block" />.
                <input name="secondarywins_" type="text" id="secondarywins_4" maxlength="3" class="ip_add text_block" />
                <?drawlabel("secondarywins_1");?>
            </td>
		</tr>      
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Add DHCP Reservation");?></p></td>
        </tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable");?> :</td>
			<td class="gray_bg border_right"><input type="checkbox" id="en_dhcp_reserv"></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Computer Name");?> :</td>
			<td class="gray_bg border_right"><input type="text" id="reserv_host" maxlength="60" size="25">						
			<input type="button" value="<<" class="arrow" onclick="PAGE.OnChangeGetClient();" />
				<? DRAW_select_dhcpclist("LAN-1","pc", I18N("h","Computer Name"), "",  "", "1", "broad"); ?>
                <?drawlabel("reserv_host");?></td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","IP Address");?> :</td>
			<td class="gray_bg border_right">
            	<!--<input type="text" id="reserv_ipaddr" maxlength="60" size="25">-->
                <input name="reserv_ipaddr_" type="text" id="reserv_ipaddr_1" maxlength="3" class="ip_add text_block" />.
                <input name="reserv_ipaddr_" type="text" id="reserv_ipaddr_2" maxlength="3" class="ip_add text_block" />.
                <input name="reserv_ipaddr_" type="text" id="reserv_ipaddr_3" maxlength="3" class="ip_add text_block" />.
                <input name="reserv_ipaddr_" type="text" id="reserv_ipaddr_4" maxlength="3" class="ip_add text_block" />
                <?drawlabel("reserv_ipaddr_1");?>
            </td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","MAC Address");?> :</td>
			<td class="gray_bg border_right">
            	<!--<input type="text" id="reserv_macaddr" maxlength="17" size="25">-->
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_1" maxlength="2" class="mac_add text_block" />-
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_2" maxlength="2" class="mac_add text_block" />-
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_3" maxlength="2" class="mac_add text_block" />-
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_4" maxlength="2" class="mac_add text_block" />-
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_5" maxlength="2" class="mac_add text_block" />-
                <input name="reserv_macaddr_" type="text" id="reserv_macaddr_6" maxlength="2" class="mac_add text_block" /><?drawlabel("reserv_macaddr_1");?>
                <br />
                <input id="ipv4_mac_button" type="button" value="<?echo I18N("h","Clone Your PC's MAC Address");?>" onclick="PAGE.OnClickMacButton();" /><br />
                <input type="button" value="<?echo I18N("h","Add / Update");?>" onclick="PAGE.AddDHCPReserv();" />
                <input type="button" value="<?echo I18N("h","Clear");?>" onclick="PAGE.ClearDHCPReserv();" />
			</td>
		</tr>
        
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","DHCP Reservations List");?></p></td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side"><cite>
            <div class="rc_map">
          	<table id="reserves_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            	<tr>
                	<td width="10%"><strong><?echo I18N("h","Enable");?></strong></td>
                    <td width="20%"><strong><?echo I18N("h","Host Name");?></strong></td>
                    <td width="30%"><strong><?echo I18N("h","IP Address");?></strong></td>
                    <td width="30%"><strong><?echo I18N("h","MAC Address");?></strong></td>
                    <td width="5%"></td>
                    <td width="5%"></td>
               	</tr>
            </table>
            </div></cite>
            </td>
       	</tr>
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Number of Dynamic DHCP Clients");?></p></td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side"><cite>
            <div class="rc_map" >
            <table id="leases_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            	<tr>
                	<td width="20%"><strong><?echo I18N("h","Host Name");?></strong></td>
                    <td width="30%"><strong><?echo I18N("h","IP Address");?></strong></td>
                    <td width="30%"><strong><?echo I18N("h","MAC Address");?></strong></td>
                    <td width="20%"><strong><?echo I18N("h","Expired Time");?></strong></td>
                </tr>
            </table>
            </div></cite>
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
