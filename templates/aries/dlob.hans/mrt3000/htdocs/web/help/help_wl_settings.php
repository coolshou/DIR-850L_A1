HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Wireless Settings");?></h2>
<p><?echo I18N("h","The Wireless Setup page contains the settings for the (Access Point) Portion of the router. This page allows you to customize your wireless network or configure the router to fit an existing wireless network that you may already have setup.");?></p>
<dl>
<dt><?echo I18N("h","Wireless Network Name");?></dt>
<dd><?echo I18N("h","Also known as the SSID (Service Set Identifier), this is the name of your Wireless Local Area Network (WLAN). This can be easily changed to establish a new wireless network or to add the router to an existing wireless network.");?></dd>
<!--
<dt>Schedule</dt>
<dd>Select a schedule for when the service will be enabled.
If you do not see the schedule you need in the list of schedules, go to the <a href='../maintenance/mt_schedules.htm'> Tools -> Schedules</a> screen and create a new schedule.</dd>
-->
<dt><?echo I18N("h","802.11 Mode");?></dt>
<dd><?echo I18N("h","If all of the wireless devices you want to connect with this access point can connect in the same transmission mode, you can improve performance slightly by choosing the appropriate \"Only\" mode. If you have some devices that use a different transmission mode, choose the appropriate \"Mixed\" mode.");?></dd>
<dt><?echo I18N("h","Enable Auto Channel Selection");?></dt>
<dd><?echo I18N("h","Enable Auto Channel Selection allows the router to select the best possible channel for your wireless network to operate on.");?></dd>
<dt><?echo I18N("h","Wireless Channel");?></dt>
<dd><?echo I18N("h","Indicates which channel the router is operating on. By default the channel is set to Auto Scan. This can be changed to fit the channel setting for an existing wireless network or to customize your new wireless network. Click the Enable Auto Scan checkbox to have the router automatically select the channel that it will operate on. This option is recommended because the router will choose the channel with the least amount of interference.");?></dd>
<dt><?echo I18N("h","Transmission Rates");?></dt>
<dd><?echo I18N("h","Select the basic transfer rates based on the speed of wireless adapters on the WLAN (wireless local area network).");?></dt>
<dt><?echo I18N("h","Channel Width");?></dt>
<dd><?echo I18N("h","The \"Auto 20/40 MHz\" option is usually best. The other options are available for special circumstances.");?></dd>

<!--
<dt>Transmission (TX) Rates</dt>
<dd>Select the basic transfer rates based on the speed of wireless adapters on the WLAN (wireless local area network).</dd>
-->
<dt><?echo I18N("h","Enable Hidden Wireless");?></dt>
<dd><?echo I18N("h","Select Enabled if you would not like the SSID of your wireless network to be broadcasted by the router. If this option is Enabled, the SSID of the router will not be seen by Site Survey utilities, so when setting up your wireless clients, you will have to know the SSID of your router and enter it manually in order to connect to the router. This option is disabled by default.");?></dd>
<dt><?echo I18N("h","Wireless Security Mode");?></dt>
<dd><?echo I18N("h","Securing your wireless network is important as it is used to protect the integrity of the information being transmitted over your wireless network. The router is capable of 2 types of wireless security; WEP and WPA/WPA2 (auto-detect)");?>
<dl>
<dt><?echo I18N("h","WEP");?></dt>
<dd><?echo I18N("h","Wired Equivalent Protocol (WEP) is a wireless security protocol for Wireless Local Area Networks (WLAN). WEP provides security by encrypting the data that is sent over the WLAN. The router supports 2 levels of WEP Encryption: 64-bit and 128-bit. WEP is disabled by default. The WEP setting can be changed to fit an existing wireless network or to customize your wireless network.");?>
<dl>
<dt><?echo I18N("h","Authentication");?></dt>
<dd><?echo I18N("h","Authentication is a process by which the router verifies the identity of a network device that is attempting to join the wireless network. There are two types authentication for this device when using WEP.");?></dd>
<dl>
<dt><?echo I18N("h","Open System");?></dt>
<dd><?echo I18N("h","Select this option to allow all wireless devices to communicate with the router before they are required to provide the encryption key needed to gain access to the network.");?></dd>
<dt><?echo I18N("h","Shared Key");?></dt>
<dd><?echo I18N("h","Select this option to require any wireless device attempting to communicate with the router to provide the encryption key needed to access the network before they are allowed to communicate with the router.");?></dd>
</dl>
<dt><?echo I18N("h","WEP Encryption");?></dt>
<dd><?echo I18N("h","Select the level of WEP Encryption that you would like to use on your network. The two supported levels of WEP encryption are 64-bit and 128-bit.");?></dd>
<dt><?echo I18N("h","Key Type");?></dt>
<dd><?echo I18N("h","The Key Types that are supported by the router are HEX (Hexadecimal) and ASCII (American Standard Code for Information Interchange.) The Key Type can be changed to fit an existing wireless network or to customize your wireless network.");?></dd>
<dt><?echo I18N("h","Keys");?></dt>
<dd><?echo I18N("h","Keys 1-4 allow you to easily change wireless encryption settings to maintain a secure network. Simply select the specific key to be used for encrypting wireless data on the network.");?></dd>
</dl>
</dd>
<dt><?echo I18N("h","WPA/WPA2");?></dt>
<dd><?echo I18N("h","Wi-Fi Protected Access (2) authorizes and authenticates users onto the wireless network. WPA(2) uses stronger security than WEP and is based on a key that changes automatically at regular intervals.");?>
<dl>
<dt><?echo I18N("h","Cipher Type");?></dt>
<dd><?echo I18N("h","The router supports two different cipher types when WPA is used as the Security Type. These two options are TKIP (Temporal Key Integrity Protocol) and AES (Advanced Encryption Standard).");?></dd>
<dt><?echo I18N("h","Network Key");?></dt>
<dd><?echo I18N("h","This is what your wireless clients will need in order to communicate with your router, When PSK is selected enter 8-63 alphanumeric characters. Be sure to write this Passphrase down as you will need to enter it on any other wireless devices you are trying to add to your network.");?></dd>
</dl>
</dd>
</dl>
</dd>
</dl>
</body>
</html>
