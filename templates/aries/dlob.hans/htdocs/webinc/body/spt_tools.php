<div class="orangebox">
	<h1><?echo i18n("Tools Help");?></h1>
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
<div class="blackbox" id="tools_admin">
	<h2><a name="Admin"><?echo i18n("Device Administration");?></a></h2>
	<dl>
		<dt><?echo i18n("Administrator password");?></dt>
		<dd><?
			echo i18n("Enter and confirm the password that the <strong>admin</strong> account will use to access the router's management interface.");
		?></dd>
		<dt style="display:none;"><?echo i18n("User password");?></dt>
		<dd><?
			echo i18n("Enter and confirm the password that the <strong>user</strong> account will use to read-only access the router's management interface.");
		?></dd>	
		<dt><?echo i18n("Gateway Name");?></dt>
		<dd><?
			echo i18n("Enter a name for this router.");
		?></dd>		
		<dt><?echo i18n("Graphical Authentication");?></dt>
		<dd><?
			echo i18n("Require users to type letters or numbers from a distorted image displayed on the screen to prevent online hackers and unauthorized users from gaining access to your router's network settings.");
		?></dd>
		<dt><?echo i18n("HTTPS Server");?></dt>
		<dd><?
			echo i18n("You can set up Secure Sockets Layer (SSL) security features on the web server in the router to verify the integrity of your content, verify the identity of users, and encrypt network transmissions.");
		?></dd>			
		<dt><?echo i18n("Remote Management");?></dt>
		<dd><?
			echo i18n("Remote Management allows the device to be configured through the WAN (Wide Area Network) port from the Internet using a web browser. A username and password is still required to access the router's management interface.");
		?></dd>
		<dt><?echo i18n("Remote Admin Port");?></dt>
		<dd><?
			echo i18n("The port that you will use to address the management interface from the Internet. For example, if you specify port 1080 here, then, to access the router from the Internet, you would use a URL of the form <strong>http://my.domain.com:1080/.</strong>");
		?></dd>
		<dt><?echo i18n("Remote Admin Inbound Filter");?></dt>
		<dd><?
			echo i18n("Select a filter that controls access as needed for this virtual server. If you do not see the filter you need in the list of filters, go to the <a href='adv_inb_filter.php'> Advanced --> Inbound Filter</a> screen and create a new filter.");
		?></dd>		
		<!--
		<dt><?echo i18n("IP Allowed to Access");?></dt>
		<dd><?
			echo i18n("This option allows users to specify a particular IP address from the Internet to be allowed to access the router remotely. This field is left blank by default which means any IP address from the Internet can access the router remotely once remote management is enabled.");
		?></dd>
		<div class="help_example">
		<dl>
			<dt><?echo i18n("Example");?>:</dt>
			<dd><?
				echo i18n("http://x.x.x.x:8080 whereas x.x.x.x is the WAN IP address of the router and 8080 is the port used for the Web-Management interface.");
			?></dd>
		</dl>
		</div>
		-->
	</dl>
</div>
<div class="blackbox" id="tools_time">
	<h2><a name="Time"><?echo i18n("Time");?></a></h2>
	<p><?
		echo i18n('The Time Configuration settings are used by the router for synchronizing scheduled services and system logging activities. You will need to set the time zone corresponding to your location. The time can be set manually or the device can connect to a NTP (Network Time Protocol) server to retrieve the time. You may also set Daylight Saving dates and the system time will automatically adjust on those dates.');
	?></p>
	<dl>
		<dt><?echo i18n("Time Zone");?></dt>
		<dd><?
			echo i18n("Select the Time Zone for the region you are in.");
		?></dd>
		<dt><?echo i18n("Daylight Saving");?></dt>
		<dd><?
			echo i18n("If the region you are in observes Daylight Savings Time, enable this option and specify the Starting and Ending Month, Week, Day, and Time for this time of the year.");
		?></dd>
		<dt><?echo i18n("Automatic Time Configuration");?></dt>
		<dd><?
			echo i18n("Select a D-Link time server which you would like the router to synchronize its time with. The interval at which the router will communicate with the D-Link NTP server is set to 7 days.");
		?></dd>
		<dt><?echo i18n("Set the Date and Time Manually");?></dt>
		<dd><?
			echo i18n("Select this option if you would like to specify the time manually. You must specify the Year, Month, Day, Hour, Minute, and Second, or you can click the Copy Your Computer's Time Settings button to copy the system time from the computer being used to access the management interface.");
		?></dd>
	</dl>
</div>
<div class="blackbox" id="tools_syslog">
	<h2><a name="Syslog"><?echo I18N("h","Syslog");?></a></h2>
	<p><?
		echo I18N("h",'This section allows you to archive your log files to a Syslog Server.');
	?></p>
	<dl>
		<dt><?echo I18N("h","Enable Logging to Syslog Server");?></dt>
		<dd><?
			echo I18N("h","Enable this option if you have a syslog server currently running on the LAN and wish to send log messages to it.");
		?></dd>
		<dt><?echo I18N("h","Syslog Server IP Address");?></dt>
		<dd><?
			echo I18N("h","Enter the LAN IP address of the Syslog Server.");
		?></dd>
	</dl>
</div>
<div class="blackbox" id="tools_email">
	<h2><a name="Email"><?echo i18n("Email Settings");?></a></h2>
	<p><?echo i18n("The Email feature can be used to send the system log files and router alert messages to your email address.");?></p>
	<dl>
		<dt><?echo i18n("Enable");?>
		<dd><dl>
			<dt><?echo i18n("Enable Email Notification");?></dt>
			<dd><?
				echo i18n("When this option is enabled, router activity logs can be emailed to a designated email address, and the following parameters are displayed.");
			?></dd>
		</dl></dd></dt>
		<dt><?echo i18n("Email Settings");?>
		<dd><dl>
			<dt><?echo i18n("From Email Address");?></dt>
			<dd><?
				echo i18n("This email address will appear as the sender when you receive a log file via email.");
			?></dd>
			<dt><?echo i18n("To Email Address");?></dt>
			<dd><?
				echo i18n("Enter the email address where you want the email sent.");
			?></dd>
			<dt><?echo i18n("Email Subject");?></dt>
			<dd><?
				echo i18n("Enter the email subject to specify the purpose of sending the message.");
			?></dd>
			<dt><?echo i18n("SMTP Server Address");?></dt>
			<dd><?
				echo i18n("Enter the SMTP server address for sending email.");
			?></dd>
			<dt><?echo i18n("Enable Authentication");?></dt>
			<dd><?
				echo i18n("If your SMTP server requires authentication, select this option.");
			?></dd>
			<dt><?echo i18n("SMTP Server Port");?></dt>
			<dd><?
				echo i18n("25 by default");
			?></dd>
			<dt><?echo i18n("Enable Authentication");?></dt>
			<dd><?
				echo i18n("To enable this option for authentication.");
			?></dd>
			<dt><?echo i18n("Account Name");?></dt>
			<dd><?
				echo i18n("Enter your account for sending email.");
			?></dd>
			<dt><?echo i18n("Password");?></dt>
			<dd><?
				echo i18n("Enter the password associated with the account.");
			?></dd>
			<dt><?echo i18n("Verify Password");?></dt>
			<dd><?
				echo i18n("Re-type the password associated with the account.");
			?></dd>
		</dl></dd></dt>
		<dt><?echo i18n("Email Log When Full or on Schedule");?>
		<dd><dl>
			<dt><?echo i18n("On Log Full");?></dt>
			<dd><?
				echo i18n("Select this option if you want logs to be sent by email when the log is full.");
			?></dd>
			<dt><?echo i18n("On Schedule");?></dt>
			<dd><?
				echo i18n("Select this option if you want logs to be sent by email according to a schedule.");
			?></dd>
			<dt><?echo i18n("Schedule");?></dt>
			<dd><?
				echo i18n("If you selected the On Schedule option, select one of the defined schedule rules. If you do not see the schedule you need in the list of schedules, go to the Tools->Schedules screen and create a new schedule.");
			?></dd>
			<dd><strong><?echo i18n("Note");?>: </strong>
				<?echo i18n("Normally email is sent at the start time defined for a schedule, and the schedule end time is not used. However, rebooting the router during the schedule period will cause additional emails to be sent.");?></dd>
		</dl></dd></dt>
	</dl>
</div>
<div class="blackbox" id="tools_system">
	<h2><a name="System"><?echo i18n("System");?></a></h2>
	<p><?
		echo i18n("The current system settings can be saved as a file onto the local hard drive. The saved file or any other saved setting file created by device can be uploaded into the unit. To reload a system settings file, click on <strong>Browse</strong> to search the local hard drive for the file to be used. The device can also be reset back to factory default settings by clicking on <strong>Restore Device</strong>. Use the restore feature only if necessary. This will erase previously save settings for the unit. Make sure to save your system settings before doing a factory restore.");
	?></p>
	<dl>
		<dt><?echo i18n("Save");?></dt>
		<dd><?
			echo i18n("Click this button to save the configuration file from the router.");
		?></dd>
		<dt><?echo i18n("Browse");?></dt>
		<dd><?
			echo i18n("Click Browse to locate a saved configuration file and then click to Load to apply these saved settings to the router.");
		?></dd>
		<dt><?echo i18n("Restore To Factory Default Settings");?></dt>
		<dd><?
			echo i18n("This option restores all configuration settings back to the settings that were in effect at the time the router was shipped from the factory. Any settings that have not been saved will be lost. If you want to save your router configuration settings, use the Save Settings option above.");
		?></dd>
		<dt><?echo i18n("Reboot The Device");?></dt>
		<dd><?
			echo i18n("This restarts the router. Useful for restarting when you are not near the device.");
		?></dd>
		<dt <?if ($FEATURE_NOLANGPACK=="1") echo ' style="display:none;"';?>><?echo i18n("Clear");?></dt>
		<dd <?if ($FEATURE_NOLANGPACK=="1") echo ' style="display:none;"';?>><?
			echo i18n("If the device is of other language than English, clicking this button to convert the router's language to English.");
		?></dd>
	</dl>
</div>
<div class="blackbox" id="tools_firmware">
	<h2><a name="Firmware"><?echo i18n("Firmware");?></a></h2>
	<dl>
		<dt><?echo i18n("Firmware Information and Upgrade");?></dt>
		<dd><?
			echo i18n('You can upgrade the firmware of the device using this tool. Make sure that the firmware you want to use is saved on the local hard drive of the computer. Click on <strong>Browse</strong> to search the local hard drive for the firmware to be used for the update. Upgrading the firmware will not change any of your system settings but it is recommended that you save your system settings before doing a firmware upgrade. Please check the D-Link <a href="http://support.dlink.com">support site</a> for firmware updates or you can click on <strong>Check Now</strong> button to have the router check the new firmware automatically.');
		?></dd>
		<dt><?echo i18n("Language Pack Upgrade");?></dt>
		<dd><?
			echo i18n('The language pack allows you to change the language of the user interface on the router. We suggest that you upgrade your current language pack if you upgrade the firmware. This ensures that any changes in the firmware are displayed correctly.');
		?></dd>
		<dd><?
			echo i18n('To upgrade the language pack, locate the upgrade file on the local hard drive with the Browse button. Once you have found the file to be used, click the Upload button to start the language pack upgrade.');
		?></dd>			
	</dl>
</div>
<div class="blackbox" id="tools_ddns">
	<h2><a name="DDNS"><?echo i18n("Dynamic DNS");?></a></h2>
	<p><?
		echo i18n("Dynamic DNS (Domain Name Service) is a method of keeping a domain name linked to a changing (dynamic) IP address. With most Cable and DSL connections, you are assigned a dynamic IP address and that address is used only for the duration of that specific connection. With the router, you can setup your DDNS service and the router will automatically update your DDNS server every time it receives a new WAN IP address.");
	?></p>
	<dl>
		<dt><?echo i18n("Server Address");?></dt>
		<dd><?echo i18n("Choose your DDNS provider from the drop down menu.");?></dd>
		<dt><?echo i18n("Host Name");?></dt>
		<dd><?echo i18n("Enter the Host Name that you registered with your DDNS service provider.");?></dd>
		<dt><?echo i18n("User Account");?></dt>
		<dd><?echo i18n("Enter the username for your DDNS account.");?></dd>
		<dt><?echo i18n("Password");?></dt>
		<dd><?echo i18n("Enter the password for your DDNS account.");?></dd>
		<dt><?echo i18n("DDNS for IPv6");?></dt>
		<dd><?echo i18n("We could also use DDNS function for IPv6 with the same account as IPv4.");?></dd>
	</dl>
</div>
<div class="blackbox" id="tools_check">
	<h2><a name="SystemCheck"><?echo i18n("System Check");?></a></h2>
	<dl>
		<dt><?echo i18n("Ping Test");?></dt>
		<dd><?
			echo i18n('This useful diagnostic utility can be used to check if a computer is on the Internet. It sends ping packets and listens for replies from the specific host. Enter in a host name or the IP address that you want to ping (Packet Internet Groper) and click <strong>Ping</strong>. The status of your Ping attempt will be displayed in the Ping Result box.');
		?></dd>
	</dl>
</div>
<div class="blackbox"<?if($FEATURE_NOSCH=="1")echo ' style="display:none"';?> id="tools_sch">
	<h2><a name="Schedules"><?echo i18n("Schedules");?></a></h2>
	<p><?
		echo i18n("This page is used to configure global schedules for the router. Once defined, these schedules can later be applied to the features of the router that support scheduling.");
	?></p>
	<dl>
		<dt><?echo i18n("Name");?></dt>
		<dd><?echo i18n('The name of the schedule being defined.');?></dd>
		<dt><?echo i18n("Day(s)");?></dt>
		<dd><?echo i18n("Select a day, range of days, or select the All Week checkbox to have this schedule apply every day.");?></dd>
		<dt><?echo i18n("All Day - 24 hrs");?></dt>
		<dd><?echo i18n("Check this box to have the schedule active the entire 24 hours on the days specified.");?></dd>
		<dt><?echo i18n("Time Format");?></dt>
		<dd><?echo i18n("Select your time format (12/24hrs).");?></dd>
		<dt><?echo i18n("Start Time");?></dt>
		<dd><?echo i18n("Select the time at which you would like the schedule being defined to become active.");?></dd>
		<dt><?echo i18n("End Time");?></dt>
		<dd><?echo i18n("Select the time at which you would like the schedule being defined to become inactive.");?></dd>
		<dt><?echo i18n("Schedule Rules List");?></dt>
		<dd><?echo i18n("This displays all the schedules that have been defined. ");?></dd>
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
	$TEMP_MYGROUP = "tools";
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
