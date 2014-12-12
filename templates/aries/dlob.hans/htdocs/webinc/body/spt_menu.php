<div class="orangebox">
	<h1><?echo i18n("Support Menu");?></h1>
	<ul>
		<li><a href="./support.php#Setup"><?echo i18n("Setup");?></a></li>
		<li><a href="./support.php#Advanced"><?echo i18n("Advanced");?></a></li>
		<li><a href="./support.php#Tools"><?echo i18n("Tools");?></a></li>
		<li><a href="./support.php#Status"><?echo i18n("Status");?></a></li>
	</ul>
</div>
<div class="blackbox">
	<h2><a name="Setup"><?echo i18n("Setup Help");?></a></h2>
	<ul>
		<li id="bsc_internet_menu"><a href="./spt_setup.php#Internet"><?echo i18n("Internet");?></a></li>
		<li id="bsc_wlan_main_menu"><a href="./spt_setup.php#Wireless"><?echo i18n("Wireless Settings");?></a></li>
		<li id="bsc_lan_menu"><a href="./spt_setup.php#Network"><?echo i18n("Network Settings");?></a></li>
		<li id="adv_parent_ctrl_menu"><a href="./spt_setup.php#ParentCtrl"><?echo i18n("Parental Control");?></a></li>
		<li id="bsc_web_access_menu"><a href="./spt_setup.php#Storage"><?echo i18n("Storage");?></a></li>
		<li id="bsc_media_server_menu"><a href="./spt_setup.php#MediaServer"><?echo i18n("Media Server");?></a></li>
		<?if ($FEATURE_NOSMS =="0") echo '<li id="bsc_sms_menu"><a href="./spt_setup.php#SMS">'.i18n("Message Service").'</a></li>\n';?>
		<?
			if ($FEATURE_NOIPV6 == "0")
			{ 
				echo '<li id="bsc_internetv6_menu"><a href="./spt_setup.php#IPv6">'.i18n("IPv6").'</a></li>\n';
			}
		?>
		<li id="bsc_mydlink_menu"><a href="./spt_setup.php#MYDLINK"><?echo i18n("MYDLINK SETTINGS");?></a></li>
	</ul>
</div>
<div class="blackbox">
	<h2><a name="Advanced"><?echo i18n("Advanced Help");?></a></h2>
	<ul>
		<li id="adv_vsvr_menu"><a href="./spt_adv.php#VSR"><?echo i18n("Virtual Server");?></a></li>
		<li id="adv_pfwd_menu"><a href="./spt_adv.php#PFD"><?echo i18n("Port Forwarding");?></a></li>
		<?if ($FEATURE_NOAPP!="1")echo '<li id="adv_app_menu"><a href="./spt_adv.php#App">'.i18n("Application Rules").'</a></li>\n';?>
		<?if ($FEATURE_NOQOS!="1")echo '<li id="adv_qos_menu"><a href="./spt_adv.php#QoS">'.i18n("QoS Engine").'</a></li>\n';?>
		<li id="adv_mac_filter_menu"><a href="./spt_adv.php#NetFilter"><?echo i18n("Network Filter");?></a></li>
		<li id="adv_access_ctrl_menu"><a href="./spt_adv.php#AccessCtrl"><?echo i18n("Access Control");?></a></li>
		<li id="adv_web_filter_menu"><a href="./spt_adv.php#WebFilter"><?echo i18n("Website Filter");?></a></li>
		<li id="adv_parent_ctrl_menu"><a href="./spt_adv.php#ParentCtrl"><?echo i18n("Parental Control");?></a></li>		
		<li id="adv_inb_filter_menu"><a href="./spt_adv.php#InboundFilter"><?echo i18n("Inbound Filter");?></a></li>
		<li id="adv_firewall_menu"><a href="./spt_adv.php#Firewall"><?echo i18n("Firewall Settings");?></a></li>
		<?if ($FEATURE_NORT!="1")echo '<li id="adv_routing_menu"><a href="./spt_adv.php#Routing">'.i18n("Routing").'</a></li>\n';?>
		<li id="adv_wlan_menu"><a href="./spt_adv.php#Wireless"><?echo i18n("Advanced Wireless");?></a></li>
		<li id="adv_wps_menu"><a href="./spt_adv.php#WPS"><?echo i18n("Wi-Fi Protected Setup");?></a></li>
		<li id="adv_network_menu"><a href="./spt_adv.php#Network"><?echo i18n("Advanced Network");?></a></li>
		<li id="adv_dlna_menu"><a href="./spt_adv.php#DLNA"><?echo i18n("DLNA Settings");?></a></li>
		<li id="adv_itunes_menu"><a href="./spt_adv.php#ITUNES"><?echo i18n("iTunes Server");?></a></li>
		<li id="adv_gzone_menu"><a href="./spt_adv.php#Guestzone"><?echo i18n("Guest Zone");?></a></li>
		<?if ($FEATURE_NOCALLMGR =="0") echo '<li><a href="./spt_adv.php#CallMgr">'.i18n("Call Setting").'</a></li>\n';?>
		<?
			if ($FEATURE_NOIPV6 == "0")
			{ 
				echo '<li id="adv_firewallv6_menu"><a href="./spt_adv.php#IPv6Firewall">'.i18n("IPv6 Firewall").'</a></li>\n';
				echo '<li id="adv_routingv6_menu"><a href="./spt_adv.php#IPv6Routing">'.i18n("IPv6 Routing").'</a></li>\n';
			}
		?>
	</ul>
</div>
<div class="blackbox">
	<h2><a name="Tools"><?echo i18n("Tools Help");?></a></h2>
	<ul>
		<li id="tools_admin_menu"><a href="./spt_tools.php#Admin"><?echo i18n("Device Administration");?></a></li>
		<li id="tools_time_menu"><a href="./spt_tools.php#Time"><?echo i18n("Time");?></a></li>
		<li id="tools_syslog_menu"><a href="./spt_tools.php#Syslog"><?echo i18n("Syslog");?></a></li>		
		<li id="tools_email_menu"><a href="./spt_tools.php#Email"><?echo i18n("Email Settings");?></a></li>
		<li id="tools_system_menu"><a href="./spt_tools.php#System"><?echo i18n("System");?></a></li>
		<li id="tools_firmware_menu"><a href="./spt_tools.php#Firmware"><?echo i18n("Firmware");?></a></li>
		<li id="tools_ddns_menu"><a href="./spt_tools.php#DDNS"><?echo i18n("Dynamic DNS");?></a></li>
		<li id="tools_check_menu"><a href="./spt_tools.php#SystemCheck"><?echo i18n("System Check");?></a></li>
		<?if ($FEATURE_NOSCH!="1")echo '<li id="tools_sch_menu"><a href="./spt_tools.php#Schedules">'.i18n("Schedules").'</a></li>\n';?>
	</ul>
</div>
<div class="blackbox">
	<h2><a name="Status"><?echo i18n("Status Help");?></a></h2>
	<ul>
		<li id="st_device_menu"><a href="./spt_status.php#Device"><?echo i18n("Device Info");?></a></li>
		<li id="st_log_menu"><a href="./spt_status.php#Logs"><?echo i18n("Logs");?></a></li>
		<li id="st_stats_menu"><a href="./spt_status.php#Statistics"><?echo i18n("Statistics");?></a></li>
		<li id="st_session_menu"><a href="./spt_status.php#Sessions"><?echo i18n("Internet Sessions");?></a></li>
		<li id="st_wlan_menu"><a href="./spt_status.php#Wireless"><?echo i18n("Wireless");?></a></li>
		<li id="st_routing_menu"><a href="./spt_status.php#Routingv4"><?echo i18n("Routing");?></a></li>
		<?if ($FEATURE_NOIPV6 =="0") echo '<li id="st_ipv6_menu"><a href="./spt_status.php#IPv6">'.i18n("IPv6").'</a></li>\n';?>	
		<?if ($FEATURE_NOIPV6 =="0") echo '<li id="st_routingv6_menu"><a href="./spt_status.php#Routing">'.i18n("IPv6 Routing").'</a></li>\n';?>		
	</ul>
</div>

<script type="text/javascript">
	var li_array = document.getElementsByTagName("li");
	for(var i=0; i < li_array.length; i++)
	{
		if(li_array[i].id!="") li_array[i].style.display="none";
	}
	
	<?
	$TEMP_MYGROUP = "basic";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	var LinkString = "<? echo $link; ?>";
	DisplayUsedMenu(LinkString);
	<?
	$TEMP_MYGROUP = "advanced";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	var LinkString = "<? echo $link; ?>";
	DisplayUsedMenu(LinkString);
	<?
	$TEMP_MYGROUP = "tools";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	var LinkString = "<? echo $link; ?>";
	DisplayUsedMenu(LinkString);
	<?
	$TEMP_MYGROUP = "status";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	var LinkString = "<? echo $link; ?>";
	DisplayUsedMenu(LinkString);			
	<?
	$TEMP_MYGROUP = "support";
	include "/htdocs/webinc/menu.php";		/* The menu definitions */
	?>
	function DisplayUsedMenu(LinkString)
	{
		var LinkString_array = LinkString.split("|");
		for(var i=0; i < LinkString_array.length; i++)
		{
			var LinkID = LinkString_array[i].substring(0, LinkString_array[i].length-4);
			try{OBJ(LinkID+"_menu").style.display = "";} catch(e){}
		}	
	}
</script>
