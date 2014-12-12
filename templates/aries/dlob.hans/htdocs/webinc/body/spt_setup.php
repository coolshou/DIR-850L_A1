<div class="orangebox">
	<h1><?echo i18n("Setup Help");?></h1>
	<ul>
		<li id="bsc_internet_menu"><a href="#Internet"><?echo i18n("Internet");?></a></li>
		<li id="bsc_wlan_main_menu"><a href="#Wireless"><?echo i18n("Wireless Settings");?></a></li>
		<li id="bsc_lan_menu"><a href="#Network"><?echo i18n("Network Settings");?></a></li>
		<?if ($FEATURE_PARENTALCTRL == "1") echo '<li id="adv_parent_ctrl_menu"><a href="#ParentCtrl">'.i18n("Parental Control").'</a></li>\n';?>	
		<li id="bsc_web_access_menu"><a href="#Storage"><?echo i18n("Storage");?></a></li>
		<li id="bsc_media_server_menu"><a href="#MediaServer"><?echo i18n("Media Server");?></a></li>
		<?if ($FEATURE_NOIPV6 == "0") echo '<li id="bsc_internetv6_menu"><a href="#IPv6">'.i18n("IPv6").'</a></li>\n';?>
		<li id="bsc_mydlink_menu"><a href="#MYDLINK"><?echo i18n("MYDLINK SETTINGS");?></a></li>
		<?if ($FEATURE_NOSMS =="0") echo '<li id="bsc_sms_menu"><a href="#SMS">'.i18n("Message Service").'</a></li>\n';?>
	</ul>
</div>
<div class="blackbox" id="bsc_internet">
	<h2><a name="Internet"><?echo i18n("Internet");?></a></h2>
	<p><?
		echo i18n("If you are configuring the device for the first time, we recommend that you click on the Internet Connection Setup Wizard, and follow the instructions on the screen. If you wish to modify or configure the device settings manually, click Manual Internet Connection Setup.");
	?></p>
	<dl>
		<dt><?echo i18n("Internet Connection Setup Wizard");?></dt>
		<dd><?
			echo i18n("Click this button to have the router walk you through a few simple steps to help you connect your router to the Internet.");
		?></dd>
		<dt><?echo i18n("Manual Internet Connection Setup");?></dt>
		<dd><?
			echo i18n("Choose this option if you would like to input the settings needed to connect your router to the Internet manually.");?>
			<dl>
				<dt <?if ($FEATURE_NOAPMODE=="1") echo ' style="display:none;"';?>><?echo i18n("Access Point Mode");?></dt>
				<dd <?if ($FEATURE_NOAPMODE=="1") echo ' style="display:none;"';?>><?
					echo i18n('Enable "Access Point Mode" will make the device function like a wireless AP. All the NAT functions will be disabled except settings related to the wireless connection.');
				?></dd>
				<dt <?if ($FEATURE_HAVEBGMODE=="0") echo ' style="display:none;"';?>><?echo i18n("Bridge Mode");?></dt>
				<dd <?if ($FEATURE_HAVEBGMODE=="0") echo ' style="display:none;"';?>><?
					echo i18n('Enable "Bridge Mode" will make the device function like a wireless AP. All the NAT functions will be disabled except settings related to the wireless connection.');
				?></dd>				
				<dt><?echo i18n("Internet Connection Type ");?></dt>
				<dd><?
					echo i18n('The Internet Connection Settings are used to connect the router to the Internet. Any information that needs to be entered on this page will be provided to you by your ISP and often times referred to as "public settings". Please select the appropriate option for your specific ISP. If you are unsure of which option to select, please contact your ISP.');?>
					<dl>
						<dt><?echo i18n("Static IP Address");?></dt>
						<dd><?
							echo i18n('Select this option if your ISP (Internet Service Provider) has provided you with an IP address, Subnet Mask, Default Gateway, and a DNS server address. Enter this information in the appropriate fields. If you are unsure of what to enter in these fields, please contact your ISP.');
						?></dd>
						<dt><?echo i18n("Dynamic IP Address");?></dt>
						<dd><?
							echo i18n('Select this option if your ISP (Internet Service Provider) provides you with an IP address automatically. Cable modem providers typically use dynamic assignment of IP Addresses.');?>
							<dl>
							<p><?
								echo '<strong>'.i18n('Host Name').'</strong> ('.i18n('optional').')<br/>';
								echo i18n('The Host Name field is optional but may be required by some Internet Service Providers. The default host name is dlinkrouter.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Use Unicasting').'</strong> ('.i18n('optional').')<br/>';
								echo i18n('This option is normally turned off, and should remain off as long as the WAN-side DHCP server correctly provides an IP address to the router. However, if the router cannot obtain an IP address from the DHCP server, the DHCP server may be one that works better with unicast responses. In this case, turn the unicasting option on, and observe whether the router can obtain an IP address. In this mode, the router accepts unicast responses from the DHCP server instead of broadcast responses.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MAC Address').'</strong> ('.i18n('optional').')<br/>';
								echo I18N("h",'The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the "Clone MAC Address" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Primary DNS Address').'</strong><br/>';
								echo i18n('Enter the Primary DNS (Domain Name Service) server IP address provided to you by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Secondary DNS Address').'</strong> ('.i18n('optional').')<br/>';
								echo i18n('If you were given a secondary DNS server IP address from your ISP, enter it in this field.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MTU').'</strong><br/>';
								echo i18n('MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1500 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.');
							?></p>
							</dl>
						</dd>
						<dt><?echo i18n("PPPoE");?></dt>
						<dd><?
							echo i18n('Select this option if your ISP requires you to use a PPPoE (Point to Point Protocol over Ethernet) connection. DSL providers typically use this option. Select Dynamic PPPoE to obtain an IP address automatically for your PPPoE connection (used by majority of PPPoE connections). Select Static PPPoE to use a static IP address for your PPPoE connection.');?>
							<dl>
							<p><?
								echo '<strong>'.i18n('User Name').'</strong><br/>';
								echo i18n('Enter your PPPoE username.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Password').'</strong><br/>';
								echo i18n('Enter your PPPoE password.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Service Name').'</strong> ('.i18n('optional').')<br/>';
								echo i18n('If your ISP uses a service name for the PPPoE connection, enter the service name here.');
							?></p>
							<p><?
								echo '<strong>'.i18n('IP Address').'</strong><br/>';
								echo i18n('This option is only available for Static PPPoE. Enter in the static IP address for the PPPoE connection.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MAC Address').'</strong> ('.i18n('optional').')<br/>';
								echo I18N("h",'The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to be the MAC address of the WAN interface on the router. You can use the "Clone MAC Address" button to automatically copy the MAC address of the Ethernet Card installed in the computer that is being used to configure the device. It is only necessary to fill in this field if required by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Primary DNS Address').'</strong><br/>';
								echo i18n('Primary DNS (Domain Name System) server IP address, which may be provided by your ISP. You should only need to enter this information if you selected Static PPPoE. If Dynamic PPPoE is chosen, leave this field at its default value as your ISP will provide you this information automatically.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Secondary DNS Address').'</strong> ('.i18n('optional').')<br/>';
								echo i18n('If you were given a secondary DNS server IP address from your ISP, enter it in this field.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Maximum Idle time').'</strong><br/>';
								echo i18n('The amount of inactivity time (in minutes) before the device will disconnect your PPPoE session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the defined Maximum Idle Time, then the connection will be dropped. This option only applies to the Connect-on-demand Connection mode.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MTU').'</strong><br/>';
								echo i18n('MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1492 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Connect mode select').'</strong><br/>';
								echo i18n('Select Always-on if you would like the router to never disconnect the PPPoE session or you can use the drop down menu to select a previously defined schedule or click the <strong>New Schedule</strong> button to add a new schedule. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The Connect-on-demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.');
							?></p>
							</dl>
						</dd>
						<dt<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>><?echo i18n('PPTP');?></dt>
						<dd<?if ($FEATURE_NOPPTP=="1") echo ' style="display:none;"';?>><?
							echo i18n('Select this option if your ISP uses a PPTP (Point to Point Tunneling Protocol) connection and has assigned you a username and password in order to access the Internet. Select Dynamic PPTP to obtain an IP address automatically for your PPTP connection. Select Static PPTP to use a static IP address for your PPTP connection.');?>
							<dl>
							<p><?
								echo '<strong>'.i18n('IP Address').'</strong><br/>';
								echo i18n('Enter the IP address that your ISP has assigned to you.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Subnet Mask').'</strong><br/>';
								echo i18n('Enter the Subnet Mask that your ISP has assigned to you.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Gateway').'</strong><br/>';
								echo i18n('Enter the Gateway IP address assigned to you by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('DNS').'</strong><br/>';
								echo i18n('Enter the DNS address assigned to you by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Server IP').'</strong><br/>';
								echo i18n('Enter the IP address of the server, which will be provided by your ISP, that you will be connecting to.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Username').'</strong><br/>';
								echo i18n('Enter your PPTP Username.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Password').'</strong><br/>';
								echo i18n('Enter your PPTP Password.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Maximum Idle time').'</strong><br/>';
								echo i18n('The amount of time of inactivity before the device will disconnect your PPTP session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the specified Maximum Idle Time, the connection will be dropped. This option only applies to the Connect-on-demand Connection mode.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MTU').'</strong><br/>';
								echo i18n('MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1400 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Connect mode select').'</strong><br/>';
								echo i18n('Select Always-on if you would like the router to never disconnect the PPTP session or you can use the drop down menu to select a previously defined schedule or click the <strong>New Schedule</strong> button to add a new schedule. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The Connect-on-demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MAC Address').'</strong> ('.i18n('optional').')<br/>';
								echo I18N("h",'The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the "Clone MAC Address" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.');
							?></p>
							</dl>
						</dd>
						<dt<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>><?echo i18n('L2TP');?></dt>
						<dd<?if ($FEATURE_NOL2TP=="1") echo ' style="display:none;"';?>><?
							echo i18n('Select this option if your ISP uses a L2TP (Layer 2 Tunneling Protocol) connection and has assigned you a username and password in order to access the Internet. Select Dynamic L2TP to obtain an IP address automatically for your L2TP connection. Select Static L2TP to use a static IP address for your L2TP connection.');?>
							<dl>
							<p><?
								echo '<strong>'.i18n('IP Address').'</strong><br/>';
								echo i18n('Enter the IP address that your ISP has assigned to you.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Subnet Mask').'</strong><br/>';
								echo i18n('Enter the Subnet Mask that your ISP has assigned to you.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Gateway').'</strong><br/>';
								echo i18n('Enter the Gateway IP address assigned to you by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('DNS').'</strong><br/>';
								echo i18n('Enter the DNS address assigned to you by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Server IP').'</strong><br/>';
								echo i18n('Enter the IP address of the server, which will be provided by your ISP, that you will be connecting to.');
							?></p>
							<p><?
								echo '<strong>'.i18n('L2TP Username').'</strong><br/>';
								echo i18n('Enter your L2TP Username.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Password').'</strong><br/>';
								echo i18n('Enter your L2TP Password.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Maximum Idle time').'</strong><br/>';
								echo i18n('The amount of inactivity time (in minutes) before the device will disconnect your L2TP session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the defined Maximum Idle Time, then the connection will be dropped. This option only applies to the Connect-on-demand Connection mode.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MTU').'</strong><br/>';
								echo i18n('MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1400 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Connect mode select').'</strong><br/>';
								echo i18n('Select Always-on if you would like the router to never disconnect the L2TP session or you can use the drop down menu to select a previously defined schedule or click the <strong>New Schedule</strong> button to add a new schedule. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The Connect-on-demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.');
							?></p>
							<p><?
								echo '<strong>'.i18n('MAC Address').'</strong> ('.i18n('optional').')<br/>';
								echo I18N("h",'The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the "Clone MAC Address" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.');
							?></p>							
							</dl>
						</dd>
						<dt<?if ($FEATURE_NORUSSIAPPTP=="1") echo ' style="display:none;"';?>>
							<?echo i18n('Russian PPTP (Dual Access)');?>
						</dt>
						<dd<?if ($FEATURE_NORUSSIAPPTP=="1") echo ' style="display:none;"';?>><?
							echo i18n('To configure a Russian PPTP Internet connection, configure as previously described for PPTP connections.')." "; if($FEATURE_NORT=="0") echo i18n('If any static route needs to be setup by your ISP, please refer to the "Routing" function in "ADVANCED" menu for further setup.');
						?></dd>
						<dt<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>>
							<?echo i18n('Russian PPPoE (Dual Access)');?>
						</dt>
						<dd<?if ($FEATURE_NORUSSIAPPPOE=="1") echo ' style="display:none;"';?>><?
							echo i18n('Some PPPoE connections use a static IP route to the ISP in addition to the global IP settings for the connection. This requires an added step to define IP settings for the physical WAN port. To configure a Russian PPPoE Internet connection, configure as previously described for PPPoE connections and add the physical WAN IP settings as instructed by your ISP.')." "; if($FEATURE_NORT=="0") echo i18n('If any static route needs to be setup by your ISP, please refer to the "Routing" function in "ADVANCED" menu for further setup.');
						?></dd>
						<dt<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>>
							<?echo i18n('Russian L2TP (Dual Access)');?>
						</dt>
						<dd<?if ($FEATURE_NORUSSIAL2TP=="1") echo ' style="display:none;"';?>><?
							echo i18n('To configure a Russian L2TP Internet connection, configure as previously described for L2TP connections.')." "; if($FEATURE_NORT=="0") echo i18n('If any static route needs to be setup by your ISP, please refer to the "Routing" function in "ADVANCED" menu for further setup.');
						?></dd>						
						
						<dt<?if ($FEATURE_NOIPV6=="1") echo ' style="display:none;"';?>>
							<?echo i18n('DS-Lite');?>
						</dt>
						<dd<?if ($FEATURE_NOIPV6=="1") echo ' style="display:none;"';?>><?
							echo i18n('Dual-stack lite enables a broadband service provider to share IPv4 addresses among customers by combining two well-known technologies: IP in IP (IPv4-in-IPv6) and NAT.');
						?></dd>
							<dl>
							<p><?
								echo '<strong>'.i18n('DS-Lite DHCPv6 Option').'</strong><br/>';
								echo i18n('Use this option to get the AFTR IPv6 automatically.');
							?></p>
							<p><?
								echo '<strong>'.i18n('Manual Configuration').'</strong><br/>';
								echo i18n('Use this option if your ISP provide the AFTR IPv6 address.');
							?></p>
							</dl>
					</dl>
				</dd>
			</dl>
		</dd>
	</dl>
</div>
<div class="blackbox" id="bsc_wlan_main">
	<h2><a name="Wireless"><?echo i18n("Wireless Settings");?></a></h2>
	<p><?
		echo i18n('The Wireless Setup page contains the settings for the (Access Point) Portion of the router. This page allows you to customize your wireless network or configure the router to fit an existing wireless network that you may already have setup.');
	?></p>
	<dl>
		<!--
		<dt><?echo i18n('Wi-Fi Protected Setup (Also called WCN 2.0 in Windows Vista)');?></dt>
		<dd><?
			echo i18n("This feature provides users a more intuitive way of setting up wireless security. It is available in two formats: PIN number and Push button. Enter the PIN number that comes with the device in the wireless card utility or Windows Vista's wireless client utility if the wireless card has a certified Windows Vista driver to automatically set up wireless security between the router and the client. The wireless card will have to support Wi-Fi Protected Setup in either format in order to take advantage of this feature.");
		?></dd>
		-->
		<dt><?echo i18n('Wireless Network Name');?></dt>
		<dd><?
			echo i18n('Also known as the SSID (Service Set Identifier), this is the name of your Wireless Local Area Network (WLAN). This can be easily changed to establish a new wireless network or to add the router to an existing wireless network.');
		?></dd>
		<?
		if ($FEATURE_NOSCH!="1")
		{
			echo '<dt>'.i18n("Schedule").'</dt>\n';
			echo '<dd>'.i18n("Select a schedule for when the service will be enabled.").'\n';
			echo i18n("If you do not see the schedule you need in the list of schedules, go to the <a href='tools_sch.php'> Tools --> Schedules</a> screen and create a new schedule.").'</dd>\n';
		}
		?>
		
		<dt><?echo i18n('802.11 Mode');?></dt>
		<dd><?
			echo i18n('If all of the wireless devices you want to connect with this router can connect in the same transmission mode, you can improve performance slightly by choosing the appropriate "Only" mode. If you have some devices that use a different transmission mode, choose the appropriate "Mixed" mode.');
		?></dd>		

		<dt><?echo i18n('Enable Auto Channel Selection');?></dt>
		<dd><?
			echo i18n('Enable Auto Channel Selection let the router can select the best possible channel for your wireless network to operate on.');
		?></dd>		
		<dt><?echo i18n('Wireless Channel');?></dt>
		<dd><?
			echo i18n('Indicates which channel the router is operating on. By default the channel is set to auto. This can be changed to fit the channel setting for an existing wireless network or to customize your new wireless network. Click the Enable Auto Scan checkbox to have the router automatically select the channel that it will operate on. This option is recommended because the router will choose the channel with the least amount of interference.');
		?></dd>
		<dt><?echo i18n("Transmission (TX) Rates");?></dt>
		<dd><?
			echo i18n("Select the basic transfer rates based on the speed of wireless adapters on the WLAN (wireless local area network).");
		?></dd>
		<dt><?echo i18n('Channel Width');?></dt>
		<dd><?
			echo i18n('The "Auto 20/40 MHz" option is usually best. The other options are available for special circumstances.');
		?></dd>		
		<dt><?echo i18n('Visibility Status');?></dt>
		<dd><?
			echo i18n('The Invisible option allows you to hide your wireless network. When this option is set to Visible, your wireless network 
						name is broadcast to anyone within the range of your signal. If you are not using encryption then they could connect to 
						your network. When Invisible mode is enabled, you must enter the Wireless Network Name (SSID) on the client manually to 
						connect to the network.');
		?></dd>
		<dt><?echo i18n('Wireless Security Mode');?></dt>
		<dd><?
			echo i18n("Securing your wireless network is important as it is used to protect the integrity of the information being transmitted. The router is capable of 4 types of wireless security; WEP, WPA only, WPA2 only, and WPA/WPA2 (auto-detect).");
			?>
			<dl>
				<dt><?echo i18n('WEP');?></dt>
				<dd><?
					echo i18n('Wired Equivalent Protocol (WEP) is a wireless security protocol for Wireless Local Area Networks (WLAN). WEP provides security by encrypting the data that is sent over the WLAN. The router supports 2 levels of WEP Encryption: 64-bit and 128-bit. WEP is disabled by default. The WEP setting can be changed to fit an existing wireless network or to customize your wireless network.');
					?>
					<dl>
						<dt><?echo i18n('Authentication');?></dt>
						<dd><?
						echo i18n('Authentication is a process by which the router verifies the identity of a network device that is attempting to join the wireless network. There are two types authentication for this device when using WEP.');
						?></dd>
						<dl>
							<dt><?echo i18n('Both');?></dt>
							<dd><?
								echo i18n('Select Open System or Shared Key for authentication automatically.');
							?></dd>
							<dt><?echo i18n('Shared Key');?></dt>
							<dd><?
								echo i18n('Select this option to require any wireless device attempting to communicate with the router to provide the encryption key needed to access the network before they are allowed to communicate with the router.');
							?></dd>
						</dl>
						<dt><?echo i18n('WEP Key');?></dt>
						<dd><?
							echo i18n('WEP Key 1 allow you to easily change wireless encryption settings to maintain a secure network. Simply select the specific key to be used for encrypting wireless data on the network.');
						?></dd>
					</dl>
				</dd>
				<dt><?echo i18n('WPA-Personal and WPA-Enterprise');?></dt>
				<dd><?
					echo i18n('Both of these options select some variant of Wi-Fi Protected Access (WPA) -- security standards published by the Wi-Fi Alliance. The <strong>WPA Mode</strong> further refines the variant that the router should employ.');
					?>
				</dd>
				
				<dt><?echo i18n('WPA Mode: ');?></dt>
					<dd><?
						echo i18n('WPA is the older standard; select this option if the clients that will be used with the router 
									only support the older standard. WPA2 is the newer implementation of the stronger IEEE 802.11i security standard. 
									With the "WPA2" option, the router tries WPA2 first, but falls back to WPA if the client only supports WPA. 
									With the "WPA2 Only" option, the router associates only with clients that also support WPA2 security.');
					?></dd>
					<dt><?echo i18n('Group Key Update Interval:');?></dt>
					<dd><?
						echo i18n('The amount of time before the group key used for broadcast and multicast data is changed.');
					?></dd>
					<dl>
						<dt><?echo i18n('WPA-Personal');?></dt>
						<dd><?
							echo i18n('This is what your wireless clients will need in order to communicate with your router, When PSK is selected enter 8-63 alphanumeric characters. Be sure to write this Passphrase down as you will need to enter it on any other wireless devices you are trying to add to your network.');
							?></dd>
							<dl>
								<dt><?echo i18n('Pre-Shared Key:');?></dt>
								<dd><?
									echo i18n('The key is entered as a pass-phrase of up to 63 alphanumeric characters in ASCII (American Standard Code for 
										Information Interchange) format at both ends of the wireless connection. It cannot be shorter than eight characters, 
										although for proper security it needs to be of ample length and should not be a commonly known phrase. This phrase is 
										used to generate session keys that are unique for each wireless client.');
								?></dd>
							</dl>
					</dl>
					<dl>
						<dt><?echo i18n('WPA-Enterprise');?></dt>
						<dd><?
							echo i18n('This option works with a RADIUS Server to authenticate wireless clients. Wireless clients should have established 
										the necessary credentials before attempting to authenticate to the Server through this Gateway. Furthermore, it 
										may be necessary to configure the RADIUS Server to allow this Gateway to authenticate users.');
							?></dd>
							<dl>
								<dt><?echo i18n('RADIUS Server IP Address:');?></dt>
								<dd><?
									echo i18n('The IP address of the authentication server.');
								?></dd>
								<dt><?echo i18n('RADIUS Server Port:');?></dt>
								<dd><?
									echo i18n('The port number used to connect to the authentication server.');
								?></dd>
								<dt><?echo i18n('RADIUS Server Shared Secret:');?></dt>
								<dd><?
									echo i18n('A pass-phrase that must match with the authentication server.');
								?></dd>
								<dt><?echo i18n('Advanced:');?></dt>
									<dl>
										<dt><?echo i18n('Optional Backup RADIUS Server:');?></dt>
										<dd><?
											echo i18n('This option enables configuration of an optional second RADIUS server. A second RADIUS server can be 
											used as backup for the primary RADIUS server. The second RADIUS server is consulted only when the primary server 
											is not available or not responding. The fields <strong>Second RADIUS Server IP Address, RADIUS Server Port, Second 
											RADIUS server Shared Secret</strong> provide the corresponding parameters for the second RADIUS 
											Server.');
										?></dd>																		
									</dl>
							</dl>		
					</dl>
			</dl>
		</dd>
	</dl>
</div>
<div class="blackbox" id="bsc_lan">
	<h2><a name="Network" name="Network"><?echo i18n("Network Settings");?></a></h2>
	<dl>
		<dt><?echo i18n('LAN Setup');?></dt>
		<dd><?
			echo i18n('These are the settings of the LAN (Local Area Network) interface for the device. These settings may be referred to as "private settings". You may change the LAN IP address if needed. The LAN IP address is private to your internal network and cannot be seen on the Internet. The default IP address is 192.168.0.1 with a subnet mask of 255.255.255.0.');
			?>
			<dl>
			<p><?
				echo '<strong>'.i18n('IP Address').'</strong><br/>';
				echo i18n('IP address of the router, default is 192.168.0.1.');
			?></p>
			<p><?
				echo '<strong>'.i18n('Subnet Mask').'</strong><br/>';
				echo i18n('Subnet Mask of the router, default is 255.255.255.0.');
			?></p>
			<p><?
				echo '<strong>'.i18n('Host Name').'</strong><br/>';
				echo i18n('The default host name is dlinkrouter.');
			?></p>		
			<p><?
				echo '<strong>'.i18n('Local Domain Name').'</strong> ('.i18n('optional').')<br/>';
				echo i18n('Enter in the local domain name for your network.');
			?></p>
			<p><?
				echo '<strong>'.i18n('DNS Relay').'</strong><br/>';
				echo i18n("When DNS Relay is enabled, DHCP clients of the router will be assigned the router's LAN IP address as their DNS server. All DNS requests that the router receives will be forwarded to your ISPs DNS servers. When DNS relay is disabled, all DHCP clients of the router will be assigned the ISP's DNS server.");
			?></p>
			</dl>
		</dd>
		<dt><?echo i18n('DHCP Server');?></dt>
		<dd><?
			echo i18n('DHCP stands for Dynamic Host Control Protocol. The DHCP server assigns IP addresses to devices on the network that request them. These devices must be set to "Obtain the IP address automatically". By default, the DHCP Server is enabled on the router. The DHCP address pool contains the range of IP addresses that will automatically be assigned to the clients on the network.');
			?>
			<dl>
				<dt><?echo i18n('Starting IP address');?></dt>
				<dd><?
					echo i18n("The starting IP address for the DHCP server's IP assignment.");
				?></dd>
				<dt><?echo i18n('Ending IP address');?></dt>
				<dd><?
					echo i18n("The ending IP address for the DHCP server's IP assignment.");
				?></dd>
				<dt><?echo i18n('Lease Time');?></dt>
				<dd><?
					echo i18n('The length of time in minutes for the IP lease.');
				?></dd>
				<dt><?echo i18n('Always Broadcast');?></dt>
				<dd><?
					echo i18n("If all the computers on the LAN successfully obtain their IP addresses from the router's DHCP server as expected, this option can remain disabled. However, if one of the computers on the LAN fails to obtain an IP address from the router's DHCP server, it may have an old DHCP client that incorrectly turns off the broadcast flag of DHCP packets. Enabling this option will cause the router to always broadcast its responses to all clients, thereby working around the problem, at the cost of increased broadcast traffic on the LAN.");
				?></dd>
				<dt><?echo i18n('NetBIOS Announcement');?></dt>
				<dd><?
					echo i18n('Check this box to allow the DHCP Server to offer NetBIOS configuration settings to the LAN hosts. NetBIOS allow LAN hosts to discover all other computers within the network, e.g. within Network Neighbourhood.');
				?></dd>
				<dt><?echo i18n('Learn NetBIOS from WAN');?></dt>
				<dd><?
					echo i18n('If NetBIOS announcement is swicthed on, switching this setting on causes WINS information to be learned from the WAN side, if available. Turn this setting off to configure manually.');
				?></dd>
				<dt><?echo i18n('Primary WINS Server IP address');?></dt>
				<dd><?
					echo i18n("Configure the IP address of the preferred WINS server. WINS Servers store information regarding network hosts, allowing hosts to 'register' themselves as well as discover other available hosts, e.g. for use in Network Neighbourhood. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");
				?></dd>
				<dt><?echo i18n('Secondary WINS Server IP address');?></dt>
				<dd><?
					echo i18n("Configure the IP address of the backup WINS server, if any. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");
				?></dd>
				<dt><?echo i18n('NetBIOS Scope');?></dt>
				<dd><?
					echo i18n("This is an advanced setting and is normally left blank. This allows the configuration of a NetBIOS 'domain' name under which network hosts operate. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");
				?></dd>
				<dt><?echo i18n('NetBIOS Registration mode');?></dt>
				<dd><?
					echo i18n("Indicates how network hosts are to perform NetBIOS name registration and discovery.
								H-Node, this indicates a Hybrid-State of operation. First WINS servers are tried, if any, followed by local network broadcast. This is generally the preferred mode if you have configured WINS servers.
								M-Node (default), this indicates a Mixed-Mode of operation. First Broadcast operation is performed to register hosts and discover other hosts, if broadcast operation fails, WINS servers are tried, if any. This mode favours broadcast operation which may be preferred if WINS servers are reachable by a slow network link and the majority of network services such as servers and printers are local to the LAN.
								P-Node, this indicates to use WINS servers ONLY. This setting is useful to force all NetBIOS operation to the configured WINS servers. You must have configured at least the primary WINS server IP to point to a working WINS server.
								B-Node, this indicates to use local network broadcast ONLY. This setting is useful where there are no WINS servers available, however, it is preferred you try M-Node operation first.
								This setting has no effect if the 'Learn NetBIOS information from WAN' is activated."); 
				?></dd>
			</dl>	
		</dd>
		<dd><?
			echo i18n('Dynamic DHCP client computers connected to the unit will have their information displayed in the Dynamic DHCP Client Table. The table will show the Host Name, IP Address, MAC Address, and Expired Time of the DHCP lease for each client computer.');
		?></dd>
		<dt><?echo i18n('Add/Edit DHCP Reservation');?></dt>
		<dd><? 
			echo i18n("This option lets you reserve IP addresses, and assign the same IP address to the network device with the specified MAC address any time it requests an IP address. This is almost the same as when a device has a static IP address except that the device must still request an IP address from the D-Link router. The D-Link router will provide the device the same IP address every time. DHCP Reservations are helpful for server computers on the local network that are hosting applications such as Web and FTP. Servers on your network should either use a static IP address or use this option.");	
			?>
			<dl>	
				<dt><?echo i18n('Computer Name');?></dt>
				<dd><?
					echo i18n("You can assign a name for each computer that is given a reserved IP address. This may help you keep track of which computers are assigned this way. Example: GameServer.");
				?></dd>
				<dt><?echo i18n('IP Address');?></dt>
				<dd><?
					echo i18n("The LAN address that you want to reserve.");
				?></dd>
				<dt><?echo i18n('MAC Address');?></dt>
				<dd><?
					echo i18n("To input the MAC address of your system, enter it in manually or connect to the D-Link router's Web-Management interface from the system and click the Copy Your PC's MAC Address button.");
				?></dd>
				<dd><?
					echo i18n("A MAC address is usually located on a sticker on the bottom of a network device. The MAC address is comprised of twelve digits. Each pair of hexadecimal digits are separated by colons such as 00:0D:88:11:22:33. If your network device is a computer and the network card is already located inside the computer, you can connect to the D-Link router from the computer and click the Copy Your PC's MAC Address button to enter the MAC address.");				
				?></dd>
			</dl>	
		</dd>
		<dt><?echo i18n('DHCP Reservations List');?></dt>		
		<dd><? 
			echo i18n("This shows clients that you have specified to have reserved DHCP addresses. An entry can be changed by clicking the Edit icon, or deleted by clicking the Delete icon. When you click the Edit icon, the item is highlighted, and the 'Edit DHCP Reservation' section is activated for editing.");
		?></dd>
		<dt><?echo i18n('Number of Dynamic DHCP Clients');?></dt>		
		<dd><? 
			echo i18n("In this section you can see what LAN devices are currently leasing IP addresses.");	
		?></dd>		
	</dl>
</div>
<div class="blackbox" id="adv_parent_ctrl">
	<h2><a name="ParentCtrl" <?if ($FEATURE_PARENTALCTRL == "0") echo ' style="display:none;"';?>><?echo i18n("Parental Control");?></a></h2>
	<p><?echo i18n('Parental control is a free security option that provides Anti-Phishing to protect your Internet connection from fraud and navigation improvements such as auto-correction of common URL types.');?></p>
	<dl>
		<dt><strong><?echo i18n("Advanced DNS");?></strong></dt>
		<dd><?
			echo i18n('Fast, reliable DNS with minimal blocking of phishing sites only. No OpenDNS account required.');
		?></dd>
		<dt><strong><?echo i18n("FamilyShield");?></strong></dt>
		<dd><?
			echo i18n('Fast, reliable DNS with non-configurable blocking of sites that are inappropriate or risky for children. No OpenDNS account required.');
		?></dd>
		<dt><strong><?echo i18n("Parental Control");?></strong></dt>
		<dd><?
			echo i18n('Fast, reliable DNS with configurable content filtering and phishing protection. Includes an OpenDNS account.');
		?></dd>
	<dl>
</div>
<div class="blackbox" id="bsc_web_access">
	<h2><a name="Storage"><?echo i18n("Storage");?></a></h2>
	<p><?echo i18n("Web File Access allows you to use a web browser to remotely access files stored on an SD card or USB storage drive plugged into the router. To use this feature, check the Enable Web File Access checkbox, then use the Admin account or create user accounts to manage access to your storage devices. After plugging in an SD card or USB storage drive, the new device will appear in the list below.");?></p>
	<dl>
		<dt><strong><?echo i18n("HTTP Storage");?></strong></dt>
		<dd><?
			echo i18n('We could enable the web file access using HTTP or HTTPS remote access with designated port.');
		?></dd>
		<dt><strong><?echo i18n("User Creation");?></strong></dt>
		<dd><?
			echo i18n('User creation and modification section support to crate and modify username and password. User name pull-down menu will show added into system user list, once it need be modified, it may chose existed user name to modify it.');
		?></dd>
		<dl>
			<dt><strong><?echo i18n("USER LIST");?></strong></dt>
			<dd><?
				echo i18n("We could edit or delete the different user's settings.");
				?>
				<dl>
					<dt><strong><?echo i18n("Admin is default account");?></strong></dt>
					<dd><?
						echo i18n('Admin account will prompt for users to provide the same username and password used to access the Router Web GUI, and it should allow to access root directory for all USB or SD card that device detection.');
						?>
					</dd>						
				</dl>
				<dl>
					<dt><strong><?echo i18n("Edit User Settings");?></strong></dt>
					<dd>
						<?
						echo i18n('User shall click User List, "Edit" icon for more detail setting. Device Link, access file folder setting, permission (Read Only, Read/ Write) and Append setting options. Click "Append" button to add more Access Path. A user account must support maximum five Access Paths.');
						?>
					</dd>
				</dl>
			</dd>
		</dl>
		<dt><strong><?echo i18n("Number Devices");?></strong></dt>
		<dd><?
			echo i18n('Number of Device table will show current how many storage devices (USB1, USB2 or SD1) connection with device and space status.');
		?></dd>
		<dt><strong><?echo i18n("HTTP Storage Link");?></strong></dt>
		<dd><?
			echo i18n('If device setting DDNS and plug one USB device, the link should show http://DDNS:8181, if it does not setting DDNS then show http://WAN IP Address:8181in "HTTP Storage Link", user can use this link to connect to the drive remotely after logging in with a user account.');
		?></dd>		
	<dl>
</div>
<div class="blackbox" id="bsc_media_server">
	<h2><a name="MediaServer"><?echo i18n("Media Server");?></a></h2>
	<p><?echo i18n("DLNA (Digital Living Network Alliance) is the standard for the interoperability of Network Media Devices (NMDs). The user can enjoy multi-media applications (music, pictures and videos) on your network connected PC or media devices.");?>
		<?echo i18n("The iTunes server will allow iTunes software to automatically detect and play music from the router.");?>	
	</p>
	<dl>
		<dl>
			<dt><strong><?echo i18n("DLNA SETTINGS");?></strong></dt>
			<dd><?
				echo i18n("The router has a built-in DLNA media server that can be used with DLNA compatible media players.");
				?>
				<dl>
					<dt><strong><?echo i18n("DLNA Media Server");?></strong></dt>
					<dd>
						<?
						echo i18n('Enable or Disable the DLNA Media Server.');
						?>
					</dd>
				</dl>
				<dl>
					<dt><strong><?echo i18n("Folder");?></strong></dt>
					<dd><?
						echo i18n("Click 'Browse' to locate the root folder of your media files (music, photos, and video). Root can be chosen if you want to have access to all content on the router.");
						?>
					</dd>						
				</dl>				
			</dd>
			<dt><strong><?echo i18n("ITUNES SERVER");?></strong></dt>
			<dd><?
				echo i18n("The iTunes server will allow iTunes software to automatically detect and play music from the router.");
				?>
				<dl>
					<dt><strong><?echo i18n("iTunes Server");?></strong></dt>
					<dd><?
						echo i18n('Enable or Disable the iTunes Server.');
						?>
					</dd>						
				</dl>
				<dl>
					<dt><strong><?echo i18n("Folder");?></strong></dt>
					<dd>
						<?
						echo i18n("Click 'Browse' to locate the folder which contains your music files. Root can be chosen if you want to have access to all folders of the router.");
						?>
					</dd>
				</dl>
			</dd>			
		</dl>	
	<dl>
</div>
<div class="blackbox" id="bsc_internetv6">
	<h2><a name="IPv6" <?if ($FEATURE_NOIPV6 == "1") echo ' style="display:none;"';?>><?echo i18n("IPv6");?></a></h2>
	<dl>
		<dt><strong><?echo i18n("IPv6");?></strong></dt>
		<dd><?
			echo i18n('The IPv6 (Internet Protocol version 6) section is where you configure your IPv6 Connection type.');
			?>
		</dd>
	</dl>
	<dl>
		<dt><strong><?echo i18n("IPv6 Connection Type");?> </strong></dt>
		<dd>
			<? echo i18n('There are several connection types to choose from: Link-local Only, Auto Detection, Static IPv6, Autoconfiguration(SLAAC/DHCPv6), PPPoE, IPv6 in IPv4 Tunnel, 6to4 and 6rd.');?>
			<? echo i18n('If you are unsure of your connection method, please contact your IPv6 Internet Service Provider. Note: If using the PPPoE option, you will need to ensure that any PPPoE client software on your computers has been removed or disabled.');?>
			<dl>
				<p>	<dt><strong><?echo i18n("Link-local Only ");?></strong></dt>
				<dd><?echo i18n("The Link-local address is used by nodes and routers when communicating with neighboring nodes on the same link. This mode enables IPv6-capable devices to communicate with each other on the LAN side.");?></dd></p>
				<p>	<dt><strong><?echo i18n("Auto Detection");?></strong></dt>
				<dd><?echo i18n("The auto detection mode would detect the WAN type for Autoconfiguration, PPPoE, 6rd or DS-Lite.");?></dd></p>
				<p>	<dt><strong><?echo i18n("Static IPv6 Mode ");?></strong></dt>
				<dd><?echo i18n("This mode is used when your ISP provides you with a set IPv6 addresses that does not change. The IPv6 information is manually entered in your IPv6 configuration settings. You must enter the IPv6 address, Subnet Prefix Length, Default Gateway, Primary DNS Server, and Secondary DNS Server. Your ISP provides you with all this information. ");?></dd></p>
				<p><dt><strong><?echo i18n("Autoconfiguration(SLAAC/DHCPv6)");?></strong></dt>
				<dt><strong><?echo i18n("SLAAC");?></strong></dt>	
				<dd><?
					echo i18n('IPv6 hosts can configure themselves automatically when connected to a routed IPv6 network using the Neighbor Discovery Protocol via Internet Control Message Protocol version 6(ICMPv6) router discovery messages. When first connected to a network, a host sends a link-local router solicitation multicast request for its configuration parameters; if configured suitably, routers respond to such a request with a router advertisement packet that contains network-layer configuration parameters.');
				?></dd>
				<dd><?
					echo i18n('If IPv6 stateless address autoconfiguration is unsuitable for an application, a network may use stateful configuration with the Dynamic Host Configuration Protocol version 6(DHCPv6) or hosts may be configured statically.');
				?></dd>		
				<dt><strong><?echo i18n("DHCPv6 Mode ");?></strong></dt>
				<dd><?echo i18n("This is a method of connection where the ISP assigns your IPv6 address when your router requests one from the ISP's server. Some ISP's require you to make some settings on your side before your router can connect to the IPv6 Internet. ");?></dd></p>
				<p> <dt><strong><?echo i18n("PPPoE ");?></strong></dt>
				<dd><?echo i18n("Select this option if your ISP requires you to use a PPPoE (Point to Point Protocol over Ethernet) connection to IPv6 Internet. DSL providers typically use this option. This method of connection requires you to enter a <strong>Username</strong> and <strong>Password</strong> (provided by your Internet Service Provider) to gain access to the IPv6 Internet. The supported authentication protocols are PAP and CHAP.");?></dd>
				<dt><strong><?echo i18n("Dynamic IP:");?></strong></dt>
				<dd><?
					echo i18n('Select this option if the ISP\'s servers assign the router\'s WAN IPv6 address upon establishing a connection.');
				?></dd>
				<dt><strong><?echo i18n("Static IP:");?></strong></dt>
				<dd><?
					echo i18n('If your ISP has assigned a fixed IPv6 address, select this option. The ISP provides the value for the <strong>IPv6 Address</strong>.');
				?></dd>
				<dt><strong><?echo i18n("Service Name:");?></strong></dt>
				<dd><?
					echo i18n('Some ISP\'s may require that you enter a Service Name. Only enter a Service Name if your ISP requires one.');
				?></dd>
				</p>
				<p>	<dt><strong><?echo i18n("IPv6 in IPv4 Tunnel Mode ");?></strong></dt>
				<dd><?
					echo i18n('IPv6 in IPv4 tunneling encapsulate of IPv6 packets in IPv4 packets so that IPv6 packets can be sent over an IPv4 infrastructure.');
				?></dd> </p>
				<p> <dt><strong><?echo i18n("6to4 Mode");?></strong></dt>				
				<dd><?
					echo i18n('6to4 is an IPv6 address assignment and automatic tunneling technology that used to provide unicast IPv6 connectivity between IPv6 sites and hosts across the IPv4 Internet.');
				?></dd>
				<dd><?
					echo i18n('The following options apply to all WAN modes.');
				?></dd>
				<dd><?
					echo i18n('Primary DNS Server, Secondary DNS Server: Enter the IPv6 addresses of the DNS Servers. Leave the field for the secondary server empty if not used.');
				?></dd></p>
				<p>	<dt><strong><?echo i18n("6rd Mode");?></strong></dt>
				<dd><?
					echo i18n("6rd is a mechanism to facilitate IPv6 rapid deployment across IPv4 infrastructures of Internet service providers (ISPs). It is derived from 6to4, a preexisting mechanism to transfer IPv6 packets over the IPv4 network, with the significant change that it operates entirely within the end-user's ISP's network, thus avoiding the major architectural problems inherent in the original design of 6to4.");
				?></dd> </p>
			</dl>
		</dd>
	</dl>
	
	<dl>
		<dt><strong><?echo i18n("LAN IPv6 ADDRESS SETTINGS ");?></strong></dt>
		<dd><?
			echo i18n('These are the settings of the LAN (Local Area Network) IPv6 interface for the router. The router\'s LAN IPv6 Address configuration is based on the IPv6 Address and Subnet assigned by your ISP. (A subnet with prefix /64 is supported in LAN.)');
			?>
		</dd>
	</dl>
	<dl>
		<dt><strong><?echo i18n("LAN ADDRESS AUTOCONFIGURATION SETTINGS ");?></strong></dt>
		<dd><?
			echo i18n('Use this section to set up IPv6 Autoconfiguration to assign an IPv6 address to the computers on your local network. A Stateless and a Stateful Autoconfiguration method are provided.');
			?>
			<dl>
				<dt><strong><?echo i18n("Enable Autoconfiguration ");?></strong></dt>
				<dd><?
					echo i18n('RDNSS (Recursive DNS Server) is a new IPv6 Router Advertisement option to allow IPv6 routers to advertise DNS recursive server addresses to IPv6 hosts.');
					?>
				</dd>				
				<dd><?
					echo i18n('These two values (from and to) define a range of IPv6 addresses that the DHCPv6 Server uses when assigning addresses to computers and devices on your Local Area Network. Any addresses that are outside this range are not managed by the DHCPv6 Server. However, these could be used for manually configuring devices or devices that cannot use DHCPv6 to automatically obtain network address details. ');
					?>
				</dd>
				<dd><?
					echo i18n('Stateless DHCP with SLAAC is a common LAN Deployment model in which DHCPv6 is used to advertise DNS and domain name and IPv6 prefixes are assigned using classic SLAAC (Stateless Address Auto-Configuration).');
					?>
				</dd>				
				<dd><?
					echo i18n('When you select Stateful (DHCPv6), the following options are displayed.');
					?>
				</dd>
				<dd><?
					echo i18n('The computers (and other devices) connected to your LAN also need to have their TCP/IP configuration set to "DHCPv6" or "Obtain an IPv6 address automatically".');
					?>
				</dd>						
			</dl>
			<dl>
				<dt><strong><?echo i18n("IPv6 Address Range (DHCPv6)");?></strong></dt>
				<dd>
					<?
					echo i18n('Once your D-Link router is properly configured and this option is enabled, the router will manage the IPv6 addresses and other network configuration information for computers and other devices connected to your Local Area Network. There is no need for you to do this yourself.');
					?>
				</dd>
				<dd>
					<?
					echo i18n('It is possible for a computer or device that is manually configured to have an IPv6 address that does reside within this range.');
					?>
				</dd>
			</dl>
			<dl>
				<dt><strong><?echo i18n("IPv6 Address Lifetime ");?></strong></dt>
				<dd>
					<?
					echo i18n('The amount of time that a computer may have an IPv6 address before it is required to renew the lease.');
					?>
				</dd>
			</dl>
		</dd>
	</dl>

</div>

<div class="blackbox" id="bsc_mydlink">
	<h2><a name="MYDLINK"><?echo i18n("MYDLINK SETTINGS");?></a></h2>
	<dl>
		<dt><strong><?echo i18n("mydlink");?></strong></dt>
		<dd><?
			echo i18n("If you have already registered this router with your mydlink account, the registration will be confirmed here and the name of the registered mydlink account will be shown.");
			?>
		</dd>
	</dl>
	<dl>
		<dt><strong><?echo i18n("Register mydlink Service");?> </strong></dt>
		<dd><?
			echo i18n("If you haven't registered with mydlink, you may click the Register mydlink button to login or create a new account. After completion, the router will automatically be connected to your account.");
			?>
		</dd>
	</dl>
</div>
<div class="blackbox" <? if ($FEATURE_NOSMS !="0") echo ' style="display:none;"';?> id="bsc_sms">
        <h2><a name="SMS"><?echo i18n("Message Service");?></a></h2>
        <dl>
		<dd><?
			 echo i18n('Message Service');
                ?>.
		</dd>
		<dl>
			<dt><?echo i18n('Short Message Service (SMS)');?></dt>
                        <dd><?
				echo i18n('With The Short Message Service (SMS) network service you can send/receive short text messages.');
			?></dd>
                        <dt><?echo i18n('SMS Inbox');?></dt>
                        <dd><?echo i18n("You can browser received short text messages here.");?></dd>
                        <dt><?echo i18n('Create Message');?></dt>
                        <dd><?
                                echo i18n("You can write and edit text messages of up to 511 characters. You can send short text messages to phones which have SMS capability.");
                        ?></dd>
		</dl>
	</dl>
</div>

<script type="text/javascript">
	var li_array = document.getElementsByTagName("li");
	for(var i=0; i < li_array.length; i++)
	{
		if(li_array[i].id!="") li_array[i].style.display="none";
	}
	var blackbox_array = document.getElementsByTagName("div");
	for(var i=0; i < blackbox_array.length; i++)
	{
		if(blackbox_array[i].className=="blackbox") blackbox_array[i].style.display="none";
	}
	
	<?
	$TEMP_MYGROUP = "basic";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	var LinkString = "<? echo $link; ?>";
	<?
	$TEMP_MYGROUP = "support";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>		
	var LinkString_array = LinkString.split("|");
	for(var i=0; i < LinkString_array.length; i++)
	{
		var LinkID = LinkString_array[i].substring(0, LinkString_array[i].length-4);
		try{OBJ(LinkID+"_menu").style.display = "";} catch(e){}
		try{OBJ(LinkID).style.display = "";} catch(e){}
	}
</script>