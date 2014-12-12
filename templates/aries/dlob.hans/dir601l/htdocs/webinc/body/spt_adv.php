<div class="orangebox">
	<h1><?echo i18n("Advanced Help");?></h1>
	<ul>
		<li><a href="#VSR"><?echo i18n("Virtual Server");?></a></li>
		<li><a href="#PFD"><?echo i18n("Port Forwarding");?></a></li>
		<?if ($FEATURE_NOAPP!="1") echo '<li><a href="#App">'.i18n("Application Rules").'</a></li>';?>
		<?if ($FEATURE_NOQOS!="1") echo '<li><a href="#QoS">'.i18n("QoS Engine").'</a></li>';?>
		<li><a href="#NetFilter"><?echo i18n("Network Filter");?></a></li>
		<li><a href="#AccessCtrl"><?echo i18n("Access Control");?></a></li>
		<li><a href="#WebFilter"><?echo i18n("Website Filter");?></a></li>		
		<li><a href="#InboundFilter"><?echo i18n("Inbound Filter");?></a></li>
		<li><a href="#Firewall"><?echo i18n("Firewall Settings");?></a></li>
		<?if ($FEATURE_NORT!="1") echo '<li><a href="#Routing">'.i18n("Routing").'</a></li>';?>
		<li><a href="#Wireless"><?echo i18n("Advanced Wireless");?></a></li>
		<li><a href="#WPS"><?echo i18n("Wi-Fi Protected Setup");?></a></li>
		<li><a href="#Network"><?echo i18n("Advanced Network");?></a></li>
	</ul>
</div>
<div class="blackbox">
	<h2><a name="VSR"><?echo i18n("Virtual Server");?></a></h2>
	<p><?
		echo i18n('The Virtual Server option gives Internet users access to services on your LAN. This feature is useful for hosting online services such as FTP, Web, or game servers. For each Virtual Server, you define a public port on your router for redirection to an internal LAN IP Address and LAN port.');
	?></p>
	<div class="help_example">
	<dl>
		<dt><strong><?echo i18n("Example");?>: </strong></dt>
		<dd><?echo i18n("You are hosting a Web Server on a PC that has LAN IP Address of 192.168.0.50 and your ISP is blocking Port 80.");?>
			<ol>
				<li><?echo i18n("Name the Virtual Server (for example: <code>Web Server</code>)");?></li>
				<li><?echo i18n("Enter the IP Address of the machine on your LAN (for example: <code>192.168.0.50</code>)");?></li>
				<li><?echo i18n("Enter the Private Port as [80]");?></li>
				<li><?echo i18n("Enter the Public Port as [8888]");?></li>
				<li><?echo i18n("Select the Protocol (for example <code>TCP</code>).");?></li>
				<? if ($FEATURE_NOSCH!="1")echo '<li>'.i18n("Ensure the schedule is set to <code>Always</code>").'</li>\n';?>
				<li><?echo i18n("Repeat these steps for each Virtual Server Rule you wish to add. After the list is complete, click <span class='button_ref'>Save Settings</span> at the top of the page.");?></li>
			</ol>
			<?echo i18n("With this Virtual Server entry, all Internet traffic on Port 8888 will be redirected to your internal web server on port 80 at IP Address 192.168.0.50.");?>
		</dd>
	</dl>
	</div>
	<dl>
		<dt><strong><?echo i18n("Virtual Server Parameters");?></strong></dt>
		<dd>
			<dl>
				<dt><?echo i18n("Name");?></dt>
				<dd><?
					echo i18n("Assign a meaningful name to the virtual server, for example <code>Web Server</code>. Several well-known types of virtual server are available from the 'Application Name' drop-down list. Selecting one of these entries fills some of the remaining parameters with standard values for that type of server.");
				?></dd>
				<dt><?echo i18n("IP Address");?></dt>
				<dd><?
					echo i18n("The IP address of the system on your internal network that will provide the virtual service, for example <code>192.168.0.50</code>. You can select a computer from the list of DHCP clients in the 'Computer Name' drop-down menu, or you can manually enter the IP address of the server computer.");
				?></dd>
				<dt><?echo i18n("Traffic Type");?></dt>
				<dd><?
					echo i18n('Select the protocol used by the service. The common choices -- UDP, TCP, and both UDP and TCP -- can be selected from the drop-down menu. To specify any other protocol, select "Other" from the list, then enter the corresponding protocol number (<a href="http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xml"> as assigned by the IANA</a>) in the Protocol box.');					
				?></dd>
				<dt><?echo i18n("Private Port");?></dt>
				<dd><?echo i18n("The port that will be used on your internal network.");?></dd>
				<dt><?echo i18n("Public Port");?></dt>
				<dd><?echo i18n("The port that will be accessed from the Internet.");?></dd>
				<dt><?echo i18n("Inbound Filter");?></dt>
				<dd><?echo i18n("Select a filter that controls access as needed for this virtual server. If you do not see the filter you need in the list of filters, go to the <a href='adv_inb_filter.php'> Advanced --> Inbound Filter</a> to screen and create a new filter.");?></dd>
				<?
				if ($FEATURE_NOSCH!="1")
				{
					echo '<dt>'.i18n("Schedule").'</dt>\n';
					echo '<dd>'.i18n("Select a schedule for when the service will be enabled.").'\n';
					echo i18n("If you do not see the schedule you need in the list of schedules, go to the <a href='tools_sch.php'> Tools --> Schedules</a> screen and create a new schedule.").'</dd>\n';
				}
				?>
			</dl>
		</dd>
		<dt><strong><?=$VSVR_MAX_COUNT?> -- <?echo i18n("Virtual Servers List");?></strong></dt>
		<dd><?echo i18n("Use the checkboxes at the left to activate or deactivate completed Virtual Server entries.");?></dd>
	</dl>
</div>
<div class="blackbox">
	<h2><a name="PFD"><?echo i18n("Port Forwarding");?></a></h2>
	<p><?
		echo i18n('The Port Forwarding option gives Internet users access to services on your LAN. This feature is useful for hosting online services such as FTP, Web or game servers. For each entry, you define a public port on your router for redirection to an internal LAN IP Address and LAN port.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("Port Forwarding Parameters");?></strong></dt>
		<dd>
			<dl>
				<dt><strong><?echo i18n("Name");?></strong></dt>
				<dd><?
					echo i18n('Assign a meaningful name to the port forwarding, for example Web Server. Several well-known types of port forwarding are available from the "Application Name" drop-down list. Selecting one of these entries fills some of the remaining parameters with standard values for that type of server.');
				?></dd>
				<dt><strong><?echo i18n('IP Address');?></strong></dt>
				<dd><?
					echo i18n('The IP address of the system on your internal network that will provide the virtual service, for example 192.168.0.50. You can select a computer from the list of DHCP clients in the "Computer Name" drop-down menu, or you can manually enter the IP address of the server computer.');
				?></dd>
				<dt><strong><?echo i18n('Application Name');?></strong></dt>
				<dd><?
					echo i18n('A list of pre-defined popular applications that users can choose from for faster configuration.');
				?></dd>
				<dt><strong><?echo i18n('Computer Name');?></strong></dt>
				<dd><?
					echo i18n('A list of DHCP clients.');
				?></dd>
				<dt><strong><?echo i18n('TCP');?></strong></dt>
				<dd><?
					echo i18n('Enter the TCP ports for port forwarding traffic control.');
				?></dd>
				<dt><strong><?echo i18n('UDP');?></strong></dt>
				<dd><?
					echo i18n('Enter the UDP ports for port forwarding traffic control.');
				?></dd>
				<dt><?echo i18n("Inbound Filter");?></dt>
				<dd><?echo i18n("Select a filter that controls access as needed for this virtual server. If you do not see the filter you need in the list of filters, go to the <a href='adv_inb_filter.php'> Advanced --> Inbound Filter</a> to screen and create a new filter.");?></dd>
				<?
				if ($FEATURE_NOSCH!="1")
				{
					echo '<dt>'.i18n("Schedule").'</dt>\n';
					echo '<dd>'.i18n("Select a schedule for when the service will be enabled.").'\n';
					echo i18n("If you do not see the schedule you need in the list of schedules, go to the <a href='tools_sch.php'> Tools --> Schedules</a> screen and create a new schedule.").'</dd>\n';
				}
				?>										
			</dl>
		</dd>
	</dl>
</div>
<div class="blackbox"<? if ($FEATURE_NOAPP=="1") echo ' style="display:none;"';?>>
	<h2><a name="App"><?echo i18n("Application Rules");?></a></h2>
	<p><?
		echo i18n('Some applications require multiple connections, such as Internet gaming, video conferencing, Internet telephony and others. These applications have difficulties working through NAT (Network Address Translation). If you need to run applications that require multiple connections, specify the port normally associated with an application in the "Trigger Port" field, select the protocol type as TCP (Transmission Control Protocol) or UDP (User Datagram Protocol), then enter the public ports associated with the trigger port in the Firewall Port field to open them for inbound traffic. There are already defined well-known applications in the Application Name drop down menu.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("Name");?></strong></dt>
		<dd><?
			echo i18n('This is the name referencing the application.');
		?></dd>
		<dt><strong><?echo i18n("Trigger Port");?></strong></dt>
		<dd><?
			echo i18n('This is the port used to trigger the application. It can be either a single port or a range of ports.');
		?></dd>
		<dt><strong><?echo i18n("Traffic Type");?></strong></dt>
		<dd><?
			echo i18n('This is the protocol used to trigger the application.');
		?></dd>
		<dt><strong><?echo i18n("Firewall Port");?></strong></dt>
		<dd><?
			echo i18n('This is the port number on the WAN side that will be used to access the application. You may define a single port or a range of ports. You can use a comma to add multiple ports or port ranges.');
		?></dd>
		<dt><strong><?echo i18n("Traffic Type");?></strong></dt>
		<dd><?
			echo i18n('This is the protocol used for the application. ');
		?></dd>
		<?
		if ($FEATURE_NOSCH!="1")
		{
			echo '<dt>'.i18n("Schedule").'</dt>\n';
			echo '<dd>'.i18n("Select a schedule for when the service will be enabled.").'\n';
			echo i18n("If you do not see the schedule you need in the list of schedules, go to the <a href='tools_sch.php'> Tools --> Schedules</a> screen and create a new schedule.").'</dd>\n';
		}
		?>
	</dl>
</div>
<div class="blackbox"<? if ($FEATURE_NOQOS=="1") echo ' style="display:none;"';?>>
	<h2><a name="QoS"><?echo i18n("QoS Engine");?></a></h2>
	<p><?
		echo i18n('Smart QoS improves VoIP voice quality or streaming by ensuring your VoIP or streaming traffic is prioritized over other network traffic, such as FTP or Web.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("Enable QoS");?></strong></dt>
		<dd><?
			echo i18n('Check this option if you want to enable QoS function.');
		?></dd>
		<dt><strong><?echo i18n("Uplink Speed");?></strong></dt>
		<dd><?
			echo i18n('Select the transmission uplink speed from drop-down menu or input uplink speed directly.');
		?></dd>
		<dt><strong><?echo i18n("Downlink Speed");?></strong></dt>
		<dd><?
			echo i18n('Select the transmission downlink speed from drop-down menu or input downlink speed directly.');
		?></dd>
	</dl>
</div>
<div class="blackbox">
	<h2><a name="NetFilter"><?echo i18n("Network Filter (MAC Address Filter)");?></a></h2>
	<p><?
		echo i18n('Use MAC Filters to deny computers within the local area network from accessing the Internet. You can either manually add a MAC address or select the MAC address from the list of clients that are currently connected to the unit.');
	?></p>
	<p><?
		echo i18n('Select "Turn MAC Filtering ON and ALLOW computers with MAC address listed below to access the network" if you only want selected computers to have network access and all other computers not to have network access.');
	?></p>
	<p><?
		echo i18n('Select "Turn MAC Filtering ON and DENY computers with MAC address listed below to access the network" if you want all computers to have network access except those computers in the list.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("MAC Address");?></strong></dt>
		<dd><?
			echo i18n('The MAC address of the network device to be added to the MAC Filter List.');
		?></dd>
		<dt><strong><?echo i18n("DHCP Client List");?></strong></dt>
		<dd><?
			echo i18n("DHCP clients will have their hostname in the Computer Name drop down menu. You can select the client computer you want to add to the MAC Filter List and click arrow button. This will automatically add that computer's MAC address to the appropriate field.");
		?></dd>
	  </dd>
	</dl>
	<p<?if ($FEATURE_NOSCH=="1") echo ' style="display:none;"';?>><?
		echo i18n('Users can use the <strong>Always</strong> drop down menu to select a previously defined schedule or click the <strong>New Schedule</strong> button to add a new schedule.');
	?></p>
	<p><?
		echo i18n('The check box is used to enable or disable a particular entry.');
	?></p>
</div>
<div class="blackbox">
	<h2><a name="AccessCtrl"><?echo I18N("h","Access Control");?></a></h2>
	<p><?
		echo I18N("h","The Access Control section allows you to control access in and out of devices on your network. Use this feature as Parental Controls to only grant access to approved sites, limit web access based on time or dates, and/or block access from applications such as peer-to-peer utilities or games.");
	?></p>
	<dt><strong><?echo I18N("h","Enable");?></strong></dt>
	<dd><?
		echo I18N("h","By default, the Access Control feature is disabled. If you need Access Control, check this option.");
	?></dd>
	<dd><?
		echo '<strong>'.I18N("h","Note").':</strong> '.I18N("h","When Access Control is disabled, every device on the LAN has unrestricted access to the Internet. However, if you enable Access Control, Internet access is restricted for those devices that have an Access Control Policy configured for them. All other devices have unrestricted access to the Internet.");
	?></dd>
	<dt><strong><?echo I18N("h","Policy Wizard");?></strong></dt>
	<dd><?
		echo I18N("h",'The Policy Wizard guides you through the steps of defining each access control policy. A policy is the "Who, What, When, and How" of access control -- whose computer will be affected by the control, what internet addresses are controlled, when will the control be in effect, and how is the control implemented. You can define multiple policies. The Policy Wizard starts when you click the button below and also when you edit an existing policy.');
	?></dd>	
	<dd>
		<dl>
			<dt><strong><?echo I18N("h","Add Policy");?></strong></dt>
			<dd><?
				echo I18N("h","Click this button to start creating a new access control policy.");
			?></dd>
		</dl>
	</dd>	
	<dt><strong><?echo I18N("h","Policy Table");?></strong></dt>
	<dd><?
		echo I18N("h",'This section shows the currently defined access control policies. A policy can be changed by clicking the Edit icon, or deleted by clicking the Delete icon. When you click the Edit icon, the Policy Wizard starts and guides you through the process of changing a policy. You can enable or disable specific policies in the list by clicking the "Enable" checkbox.');
	?></dd>	
</div>
<div class="blackbox">
	<h2><a name="WebFilter"><?echo i18n("Website Filter");?></a></h2>
	<p><?
		echo i18n('Website Filter is used to allow or deny computers on your network from accessing specific web sites by keywords or specific Domain Names. Select "ALLOW computers access to ONLY these sites" in order only allow computers on your network to access the specified URLs and Domain Names. "DENY computers access to ONLY these sites" in order deny computers on your network to access the specified URLs and Domain Names.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("Example");?> 1:</strong></dt>
		<dd><?
			echo i18n('If you wanted to block LAN users from any website containing a URL pertaining to shopping, you would need to select "DENY computers access to ONLY these sites" and then enter "shopping" into the Website Filtering Rules list. Sites like these will be denied access to LAN users because they contain the keyword in the URL.');
			?>
			<ul>
				<li>http://shopping.yahoo.com</li>
				<li>http://shopping.msn.com</li>
			</ul>
		</dd>
	</dl>
	<dl>
		<dt><strong><?echo i18n("Example");?> 2:</strong></dt>
		<dd><?
			echo i18n('If you want your children to only access particular sites, you would then choose "ALLOW computers access to ONLY these sites" and then enter in the domains you want your children to have access to.');
			?>
			<ul>
				<li>Google.com</li>
				<li>Cartoons.com</li>
				<li>Discovery.com</li>
			</ul>
		</dd>
	</dl>
</div>
<div class="blackbox">
	<h2><a name="InboundFilter"><?echo I18N("h","Inbound Filter");?></a></h2>
	<p><?
		echo I18N("h","When you use the Virtual Server, Port Forwarding, or Remote Administration features to open specific ports to traffic from the Internet, you could be increasing the exposure of your LAN to cyberattacks from the Internet. In these cases, you can use Inbound Filters to limit that exposure by specifying the IP addresses of internet hosts that you trust to access your LAN through the ports that you have opened. You might, for example, only allow access to a game server on your home LAN from the computers of friends whom you have invited to play the games on that server.");
	?></p>
	<p><?
		echo I18N("h",'Inbound Filters can be used for limiting access to a server on your network to a system or group of systems. Filter rules can be used with Virtual Server, Gaming, or Remote Administration features. Each filter can be used for several functions; for example a "Game Clan" filter might allow all of the members of a particular gaming group to play several different games for which gaming entries have been created. At the same time an "Admin" filter might only allows systems from your office network to access the WAN admin pages and an FTP server you use at home. If you add an IP address to a filter, the change is effected in all of the places where the filter is used.');
	?></p>	
	<dt><strong><?echo I18N("h","Add/Update Inbound Filter Rule");?></strong></dt>
	<dd><?
		echo I18N("h","Here you can add entries to the Inbound Filter Rules List below, or edit existing entries.");
		?>
		<dl>
			<dt><strong><?echo I18N("h","Name");?></strong></dt>
			<dd><?echo I18N("h","Enter a name for the rule that is meaningful to you.");?></dd>
			<dt><strong><?echo I18N("h","Action");?></strong></dt>
			<dd><?echo I18N("h","The rule can either Allow or Deny messages.");?></dd>
			<dt><strong><?echo I18N("h","Remote IP Range");?></strong></dt>
			<dd><?
				echo i18n("Define the ranges of Internet addresses this rule applies to. For a single IP address, enter the same address in both the <strong>Start</strong> and <strong>End</strong> boxes. Up to eight ranges can be entered. The <strong>Enable</strong> checkbox allows you to turn on or off specific entries in the list of ranges.");
			?></dd>
			<dt><strong><?echo I18N("h","Add/Update");?></strong></dt>
			<dd><?
				echo I18N("h","Saves the new or edited Inbound Filter Rule in the following list.");
			?></dd>
		</dl>
	</dd>
	<dt><strong><?echo I18N("h","Inbound Filter Rules List");?></strong></dt>	
	<dd>		
		<dd><?
			echo I18N("h",'The section lists the current Inbound Filter Rules. An Inbound Filter Rule can be changed by clicking the Edit icon, or deleted by clicking the Delete icon. When you click the Edit icon, the item is highlighted, and the "Update Inbound Filter Rule" section is activated for editing.');
		?></dd>
		<dd><?
			echo I18N("h","In addition to the filters listed here, two predefined filters are available wherever inbound filters can be applied:");
		?></dd>
		<dd>
			<dl>
				<dt><strong><?echo I18N("h","Allow All");?></strong></dt>
				<dd><?echo I18N("h","Permit any WAN user to access the related capability.");?></dd>
				<dt><strong><?echo I18N("h","Deny All");?></strong></dt>
				<dd><?echo I18N("h","Prevent all WAN users from accessing the related capability. (LAN users are not affected by Inbound Filter Rules.)");?></dd>
			</dl>
		</dd>
	</dd>	
</div>
<div class="blackbox">
	<h2><a id="Firewall" name="Firewall"><?echo i18n("Firewall Settings");?></a></h2>
	
	<p><?
		echo i18n('The Firewall Settings section contains the option to configure a DMZ Host.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("Firewall Settings");?></strong></dt>
		<dd>
			<dl>
				<dt><strong><?echo i18n("Enable SPI");?></strong></dt>
				<dd><?
					echo i18n('SPI ("stateful packet inspection" also known as "dynamic packet filtering") helps to prevent cyber attacks by tracking more state per session. It validates that the traffic passing through that session conforms to the protocol.');
					echo " ";
					echo i18n("Whether SPI is enabled or not, the router always tracks TCP connection states and ensures that each TCP packet's flags are valid for the current state.");
				?></dd>
			</dl>
		</dd>
		<dt><strong><?echo i18n("Anti-Spoof checking");?></strong></dt>
		<dd><?
			echo I18N("h","This mechanism protects against activity from spoofed or forged IP addresses, mainly by blocking packets appearing on 
			interfaces and in directions which are logically not possible.");
		?></dd>	
	
		<dt><strong><?echo i18n("DMZ");?></strong></dt>
		<dd><?
			echo i18n('If you have a computer that cannot run Internet applications properly from behind the router, then you can allow the computer to have unrestricted Internet access. Enter the IP address of that computer as a DMZ (Demilitarized Zone) host with unrestricted Internet access. Adding a client to the DMZ may expose that computer to a variety of security risks; so only use this option as a last resort.');
		?></dd>

		<dt><strong><?echo i18n("Application Level Gateway (ALG) Configuration");?></strong></dt>
		<dd><?
			echo I18N("h",'Here you can enable or disable ALGs. Some protocols and applications require special handling of the IP payload to make 
			them work with network address translation (NAT). Each ALG provides special handling for a specific protocol or application. A number of 
			ALGs for common applications are enabled by default.');
		?></dd>
		
		<dd>
			<dl>
				<dt><strong><?echo i18n("PPTP");?></strong></dt>
				<dd><?
					echo I18N("h","Allows multiple machines on the LAN to connect to their corporate networks using PPTP protocol. When the PPTP ALG 
					is enabled, LAN computers can establish PPTP VPN connections either with the same or with different VPN servers. When the PPTP ALG 
					is disabled, the router allows VPN operation in a restricted way -- LAN computers are typically able to establish VPN tunnels to 
					different VPN Internet servers but not to the same server. The advantage of disabling the PPTP ALG is to increase VPN performance. 
					Enabling the PPTP ALG also allows incoming VPN connections to a LAN side VPN server (refer to Advanced -> Virtual Server ).");
				?></dd>
				<dt><strong><?echo i18n("IPSec (VPN)");?></strong></dt>
				<dd><?
					echo I18N("h","Allows multiple VPN clients to connect to their corporate networks using IPSec. Some VPN clients support traversal 
					of IPSec through NAT. This option may interfere with the operation of such VPN clients. If you are having trouble connecting with 
					your corporate network, try disabling this option.");
				?></dd>
				<dd><?
					echo I18N("h","Check with the system administrator of your corporate network whether your VPN client supports NAT traversal.");
				?></dd>
				<dd><?
					echo I18N("h","Note that L2TP VPN connections typically use IPSec to secure the connection. To achieve multiple VPN pass-through 
					in this case, the IPSec ALG must be enabled.");
				?></dd>
				<dt><strong><?echo i18n("RTSP");?></strong></dt>
				<dd><?
					echo I18N("h","Allows applications that use Real Time Streaming Protocol to receive streaming media from the internet. QuickTime 
					and Real Player are some of the common applications using this protocol.");
				?></dd>
				<dt><strong><?echo i18n("SIP");?></strong></dt>
				<dd><?
					echo I18N("h","Allows devices and applications using VoIP (Voice over IP) to communicate across NAT. Some VoIP applications and 
					devices have the ability to discover NAT devices and work around them. This ALG may interfere with the operation of such devices. 
					If you are having trouble making VoIP calls, try turning this ALG off.");
				?></dd>
			</dl>
		</dd>
		
	</dl>
</div>
<div class="blackbox"<?if ($FEATURE_NORT=="1") echo ' style="display:none;"';?>>
	<h2><a name="Routing"><?echo i18n("Routing");?></a></h2>
	<p><?echo i18n("The Routing option allows you to define fixed routes to defined destinations.");?></p>
	<dl>
		<dt><strong><?echo i18n("Enable");?></strong></dt>
		<dd><?
			echo i18n("Specifies whether the entry will be enabled or disabled.");
		?></dd>
		<dt><strong><?echo i18n("Interface");?></strong></dt>
		<dd><?
			echo i18n("Specifies the interface -- WAN -- that the IP packet must use to transit out of the router, when this route is used.");
		?></dd>
		<dt><strong><?echo i18n("Interface (WAN)");?></strong></dt>
		<dd><?
			echo i18n("This is the interface to receive the IP Address on from the ISP to access the Internet.");
		?></dd>
		<dt><strong><?echo i18n("Destination IP");?></strong></dt>
		<dd><?
			echo i18n("The IP address of packets that will take this route.");
		?></dd>
		<dt><strong><?echo i18n("Netmask");?></strong></dt>
		<dd><?
			echo i18n("One bit in the mask specify which bits of the IP address must match.");
		?></dd>
		<dt><strong><?echo i18n("Gateway");?></strong></dt>
		<dd><?
			echo i18n("Specifies the next hop to be taken if this route is used. A gateway of 0.0.0.0 implies there is no next hop, and the IP address matched is directly connected to the router on the interface specified: WAN or WAN Physical.");
		?></dd>
	</dl>
</div>
<div class="blackbox">
	<h2><a name="Wireless"><?echo i18n("Advanced Wireless");?></a></h2>
	<p><?
		echo i18n('The options on this page should be changed by advanced users or if you are instructed to by one of our support personnel, as they can negatively affect the performance of your router if configured improperly.');
	?>
	<dl>
		<dt><strong><?echo i18n("Transmit Power");?></strong></dt>
		<dd><?
			echo i18n("You can lower the output power of the router by selecting lower percentage Transmit Power values from the drop down. Your choices are: High, Medium and Low.");
		?></dd>
		<dt><strong><?echo i18n("WLAN Partition");?></strong></dt>
		<dd><?
			echo i18n("WLAN Partition allows you to segment your Wireless network by managing access to both the internal station and Ethernet access to your WLAN.");
		?></dd>	
		<dt><strong><?echo i18n("WMM Enable");?></strong></dt>
		<dd><?
			echo i18n("Enabling WMM can help control latency and jitter when transmitting multimedia content over a wireless connection.");
		?></dd>
		<dt><? echo i18n("Short Guard Interval"); ?></dt>
		<dd>
		<? echo i18n("Using a short guard interval can increase throughput. However, it can also increase error rate in some installations, due to increased sensitivity to radio-frequency reflections. Select the option that works best for your installation
.");
		?></dd>
		<dt><strong><?echo i18n("HT 20/40 Coexistence");?></strong></dt>
		<dd><?
			echo i18n("A mode of operation in which two channels, or paths on which data can travel, are combined to increase performance in some environments.");
		?></dd>		
		<!--
		<dt><strong><?echo i18n("CTS Mode");?></strong></dt>
		<dd><?
			echo i18n("Select None to disable this feature. Select Always to force the router to require each wireless device on the network to perform and RTS/CTS handshake before they are allowed to transmit data. Select Auto to allow the router to decide when RTS/CTS handshakes are necessary.");
		?></dd>
		-->
	</dl>
</div>
<div class="blackbox">
	<h2><a name="WPS"><?echo i18n("Wi-Fi Protected Setup");?></a></h2>
	<dl>
		<dt><strong><?echo i18n("Wi-Fi Protected Setup");?></strong></dt>
		<dd>
			<dl>
				<dt><strong><?echo i18n("Enable");?></strong></dt>
				<dd><?echo i18n("Enable the Wi-Fi Protected Setup feature.");?></dd>
			</dl>
			<dl>
				<dt><strong><?echo i18n("Lock WPS-PIN Setup");?></strong></dt>
				<dd><?echo i18n("Locking the WPS-PIN Method prevents the settings from being changed by any new external registrar using its PIN. Devices can still be added to the wireless network using Wi-Fi Protected Setup Push Button Configuration (WPS-PIN). It is still possible to change wireless network settings with <a href='bsc_wlan.php'>Manual Wireless Network Setup</a> or <a href='wiz_wlan.php'>Wireless Network Setup Wizard</a>.");?></dd>
			</dl>			
		</dd>
		<dt><strong><?echo i18n("PIN Settings");?></strong></dt>
		<dd>
			<p><?
				echo i18n('A PIN is a unique number that can be used to add the router to an existing network or to create a new network. The default PIN may be printed on the bottom of the router. For extra security, a new PIN can be generated. You can restore the default PIN at any time. Only the Administrator ("admin" account) can change or reset the PIN.');
			?></p>
			<dl>
				<dt><strong><?echo i18n("PIN");?></strong></dt>
				<dd><?echo i18n("Shows the current value of the router's PIN.");?></dd>
				<dt><strong><?echo i18n("Reset PIN to Default");?></strong></dt>
				<dd><?echo i18n("Restore the default PIN of the router.");?></dd>
				<dt><strong><?echo i18n("Generate New PIN");?></strong></dt>
				<dd><?
					echo i18n("Create a random number that is a valid PIN. This becomes the router's PIN. You can then copy this PIN to the user interface of the registrar.");
				?></dd>
			</dl>
		</dd>
		<dt><strong><?echo i18n("Add Wireless Station");?></strong></dt>
		<dd>
			<p><?echo i18n("This Wizard helps you add wireless devices to the wireless network.");?></p>
			<p><?
				echo i18n("The wizard will either display the wireless network settings to guide you through manual configuration, prompt you to enter the PIN for the device, or ask you to press the configuration button on the device. If the device supports Wi-Fi Protected Setup and has a configuration button, you can add it to the network by pressing the configuration button on the device and then the on the router within 120 seconds. The status LED on the router will flash three times if the device has been successfully added to the network.");
			?></p>
			<p><?
				echo i18n('There are several ways to add a wireless device to your network. Access to the wireless network is controlled by a "registrar". A registrar only allows devices onto the wireless network if you have entered the PIN, or pressed a special Wi-Fi Protected Setup button on the device. The router acts as a registrar for the network, although other devices may act as a registrar as well.');
			?></p>
			<dl>
				<dt><strong><?echo i18n("Connect your Wireless Device");?></strong></dt>
				<dd><?echo i18n("Start the wizard.");?></dd>
			</dl>
		</dd>
	</dl>
</div>
<div class="blackbox">
	<h2><a name="Network"><?echo i18n("Advanced Network");?></a></h2>
	<p><?
		echo i18n('This section contains settings which can change the way the router handles certain types of traffic. We recommend that you not change any of these settings unless you are already familiar with them or have been instructed to change them by one of our support personnel.');
	?></p>
	<dl>
		<dt><strong><?echo i18n("UPnP");?></strong></dt>
		<dd><?
			echo i18n('UPnP is short for Universal Plug and Play which is a networking architecture that provides compatibility among networking equipment, software, and peripherals. The device is a UPnP enabled router, meaning it will work with other UPnP devices/software. If you do not want to use the UPnP functionality, it can be disabled by selecting "Disabled".');
		?></dd>
		<dt><strong><?echo i18n("WAN Ping Response");?></strong></dt>
		<dd><?
			echo i18n("When you Enable WAN Ping response, you are causing the public WAN (Wide Area Network) IP address on the device to respond to ping commands sent by Internet users. Pinging public WAN IP addresses is a common method used by hackers to test whether your WAN IP address is valid.");
		?></dd>
		<dt><strong><?echo i18n("WAN Port Speed");?></strong></dt>
		<dd><?
			echo i18n("This allows you to select the speed of the WAN interface of the router: Choose 100Mbps, 10Mbps, or Auto 10/100Mbps.");
		?></dd>
		<dt><strong><?echo i18n("Multicast Streams");?></strong></dt>
		<dd><?
			echo i18n("Enable this option to allow Multicast traffic to pass from the Internet to your network more efficiently.");
		?></dd>
		<dt><strong><?echo i18n("Enable Multicast Streams");?></strong></dt>
		<dd><?
			echo i18n("Enable this option if you are receiving video on demand type of service from the Internet. The router uses the IGMP protocol to support efficient multicasting -- transmission of identical content, such as multimedia, from a source to a number of recipients. This option must be enabled if any applications on the LAN participate in a multicast group. If you have a multimedia LAN application that is not receiving content as expected, try enabling this option.");
		?></dd>
	</dl>
</div>

