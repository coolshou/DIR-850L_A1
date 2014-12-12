<div class="orangebox">
	<h1><?echo i18n("Status Help");?></h1>
	<ul>
		<li id="st_device_menu"><a href="#Device"><?echo i18n("Device Info");?></a></li>
		<li id="st_log_menu"><a href="#Logs"><?echo i18n("Logs");?></a></li>
		<li id="st_stats_menu"><a href="#Statistics"><?echo i18n("Statistics");?></a></li>
		<li id="st_session_menu"><a href="#Sessions"><?echo i18n("Internet Sessions");?></a></li>
		<li id="st_wlan_menu"><a href="#Wireless"><?echo i18n("Wireless");?></a></li>
		<li id="st_routing_menu"><a href="#Routingv4"><?echo i18n("Routing");?></a></li>
		<?if ($FEATURE_NOIPV6 =="0") echo '<li id="st_ipv6_menu"><a href="#IPv6">'.i18n("IPv6").'</a></li>\n';?>			
		<?if ($FEATURE_NOIPV6 =="0") echo '<li id="st_routingv6_menu"><a href="#Routing">'.i18n("IPv6 Routing").'</a></li>\n';?>		
	</ul>
</div>
<div class="blackbox" id="st_device">
	<h2><a name="Device"><?echo i18n("Device Info");?></a></h2>
	<p><?
		echo i18n("This page displays the current information for the router. The page will show the version of the firmware currently loaded in the device. ");
	?></p>
	<dl>
		<dt><?echo i18n("LAN (Local Area Network)");?></dt>
		<dd><?
			echo i18n("This displays the MAC Address of the Ethernet LAN interface, the IP Address and Subnet Mask of the LAN interface, and whether or not the router's built-in DHCP server is Enabled or Disabled.");
		?></dd>
		<dt><?echo i18n("WAN (Wide Area Network)");?></dt>
		<dd><?
			echo i18n("This displays the MAC Address of the WAN interface, as well as the IP Address, Subnet Mask, Default Gateway, and DNS server information that the router has obtained from your ISP. It will also display the connection type (Dynamic, Static, or PPPoE) that is used establish a connection with your ISP. If the router is configured for Dynamic, then there will be buttons for releasing and renewing the IP Address assigned to the WAN interface. If the router is configured for PPPoE, there will be buttons for connecting and disconnecting the PPPoE session.");
		?></dd>
		<dt><?echo i18n("Wireless LAN");?></dt>
		<dd><?
			echo i18n("This displays the SSID, Channel, and whether or not Encryption is enabled on the Wireless interface.");
		?></dd>
		
		<dt><?echo i18n("Lan Computers");?></dt>
		<dd><?
			echo I18N("h", "This area of the screen continually updates to show all DHCP enabled computers and devices connected to the LAN side of your router. If the DHCP Client (i.e. a computer configured to 'Automatically obtain an address') supplies a Host Name then that will also be shown. However, any computer or device that has a static IP address would not show its host name.");
		?></dd>
		<dt><?echo i18n("IGMP Multicast memberships");?></dt>
		<dd><?
			echo I18N("h","If IGMP is enabled, this area of the screen shows all multicast groups of which any LAN devices are members.");
		?></dd>				
	</dl>
</div>
<div class="blackbox" id="st_log">
	<h2><a name="Logs"><?echo i18n("Logs");?></a></h2>
	<p><?
		echo i18n("You can save the log file to a local drive which can later be used to send to a network administrator for troubleshooting.");
	?></p>
	<dl>
		<dt><?echo i18n("Save");?></dt>
		<dd><?echo i18n("Click this button to save the log entries to a text file.");?></dd>
	</dl>
	<p><?
		echo i18n("The router keeps a running log of events and activities occurring on it at all times. The log will display up to 400 recent system logs, 50 firewall and security logs, and 50 router status logs. Newer log activities will overwrite the older logs.");
	?></p>
	<dl>
		<dt><?echo i18n("First Page");?></dt>
		<dd><?echo i18n("Click this button to go to the first page of the log.");?></dd>
		<dt><?echo i18n("Last Page");?></dt>
		<dd><?echo i18n("Click this button to go to the last page of the log.");?></dd>
		<dt><?echo i18n("Previous");?></dt>
		<dd><?echo i18n("Moves back one log page.");?></dd>
		<dt><?echo i18n("Next");?></dt>
		<dd><?echo i18n("Moves forward one log page.");?></dd>
		<dt><?echo i18n("Clear");?></dt>
		<dd><?echo i18n("Clears the logs completely.");?></dd>
		<dt><?echo i18n("Log Type");?></dt>
		<dd><?echo i18n("Select the type of information you would like the $1 to log.", query("/runtime/device/modelname"));?></dd>
		<dt><?echo i18n("Log Level");?></dt>
		<dd><?echo i18n("Select the level of information you would like the $1 to log.", query("/runtime/device/modelname"));?></dd>
		<dt><?echo i18n("Link To Email Log Setting");?></dt>
		<dd><?echo i18n("If you provided email information, clicking the <strong>Link To Email Log Setting</strong> button sends the router log to the configured email address.");?></dd>
	</dl>
</div>
<div class="blackbox" id="st_stats">
	<h2><a name="Statistics"><?echo i18n("Statistics");?></a></h2>
	<p><?
		echo i18n("The router keeps statistic of the data traffic that it handles. You are able to view the amount of packets that the router has Received and Transmitted on the Internet (WAN), LAN, and Wireless interfaces.");
	?></p>
	<dl>
		<dt><?echo i18n("Refresh");?></dt>
		<dd><?echo i18n("Click this button to update the counters.");?></dd>
		<dt><?echo i18n("Reset");?></dt>
		<dd><?echo i18n("Click this button to clear the counters. The traffic counter will reset when the device is rebooted.");?></dd>
	</dl>
</div>
<div class="blackbox" id="st_session">
	<h2><a name="Sessions"><?echo i18n("Internet Sessions");?></a></h2>
	<p><?
		echo i18n("Internet Session display Source and Destination sessions passing through the router.");
	?></p>
	<dl>
		<dt><?echo i18n("IP Address");?></dt>
		<dd><?echo i18n("The source IP address of where the sessions are originated from.");?></dd>
		<dt><?echo i18n("TCP Sessions");?></dt>
		<dd><?echo i18n("This shows the number of TCP sessions are being sent from the source IP address.");?></dd>
		<dt><?echo i18n("UDP Sessions");?></dt>
		<dd><?echo i18n("This shows the number of UDP sessions are being sent from the source IP address.");?></dd>
	</dl>
</div>
<div class="blackbox" id="st_wlan">
	<h2><a name="Wireless"><?echo i18n("Wireless");?></a></h2>
	<p><?
		if ($FEATURE_DUAL_BAND=="1") echo i18n("Use this page in order to view how many wireless clients have associated with the router. This page shows the MAC address of each associated client, and the mode they are connecting in (802.11a or 802.11b or 802.11g or 802.11n).");
		else echo i18n("Use this page in order to view how many wireless clients have associated with the router. This page shows the MAC address of each associated client, and the mode they are connecting in (802.11b or 802.11g or 802.11n).");
	?></p>
</div>
<div class="blackbox"  id="st_routing">
	<h2><a name="Routingv4"><?echo i18n("Routing");?></a></h2>
	<p><?
		echo i18n("The Routing Status menu shows information about the routes that have been enabled on your router. The list will display the destination IP address, gateway IP address, subnet mask, metric and interface for each route.");
	?></p>
</div>
<div class="blackbox"<?if($FEATURE_NOIPV6=="1")echo ' style="display:none"';?> id="st_ipv6">
	<h2><a name="IPv6"><?echo I18N("h","IPv6");?></a></h2>
	<p><?
		echo I18N("h","This page displays your IPv6 internet and network connection details.");
	?></p>
	<dt><strong><?echo I18N("h","IPv6 Connection Information");?></strong></dt>	
	<dd>
		<dl>
			<dt><?echo I18N("h","IPv6 Connection Type");?></dt>
			<dd><?
				echo I18N("h","The type of IPv6 connection.");
			?></dd>
			<dt><?echo I18N("h","LAN IPv6 Link-Local Address");?></dt>
			<dd><?
				echo I18N("h","The LAN IPv6 Link-Local Address.");
			?></dd>			
		</dl>
	</dd>
	<dt><strong><?echo I18N("h","LAN IPv6 Computers");?></strong></dt>
	<dd>
		<dl>
			<dt><?echo I18N("h","Name");?></dt>
			<dd><?
				echo I18N("h","The name of the connected computer.");
			?></dd>
			<dt><?echo I18N("h","IPv6 Address");?></dt>
			<dd><?
				echo I18N("h","The IPv6 address of the connected computer.");
			?></dd>			
		</dl>
	</dd>	
</div>
<div class="blackbox"<?if($FEATURE_NOIPV6=="1")echo ' style="display:none"';?> id="st_routingv6">
	<h2><a name="Routing"><?echo I18N("h","IPv6 Routing");?></a></h2>
	<p><?
		echo I18N("h","This page displays IPv6 routing details configured for your router.");
	?></p>
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
	$TEMP_MYGROUP = "status";
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
