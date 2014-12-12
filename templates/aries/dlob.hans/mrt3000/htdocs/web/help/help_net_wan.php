HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Internet");?></h2>
<!--
<p>If you are configuring the device for the first time, we recommend that you click on the Internet Connection Setup Wizard, and follow the instructions on the screen.</p>
-->	<dl>
<!--
<dt>Internet Connection Setup Wizard</dt>
<dd>Click this button to have the router walk you through a few simple steps to help you connect your router to the Internet.</dd>
-->
<!--	<dt>Manual Internet Connection Setup</dt>
<dd>Choose this option if you would like to input the settings needed to connect your router to the Internet manually.
<dl>-->
<dt><?echo I18N("h","Internet Connection Type ");?></dt>
<dd><?echo I18N("h","The Internet Connection Settings are used to connect the router to the Internet. Any information that needs to be entered on this page will be provided to you by your ISP and often times referred to as \"public settings\". Please select the appropriate option for your specific ISP. If you are unsure of which option to select, please contact your ISP.");?>
<dl>
<dt><?echo I18N("h","Static IP Address");?></dt>
<dd><?echo I18N("h","Select this option if your ISP (Internet Service Provider) has provided you with an IP address, Subnet Mask, Default Gateway, and a DNS server address. Enter this information in the appropriate fields. If you are unsure of what to enter in these fields, please contact your ISP.");?></dd>
<dt><?echo I18N("h","Dynamic IP Address");?></dt>
<dd><?echo I18N("h","Select this option if your ISP (Internet Service Provider) provides you with an IP address automatically. Cable modem providers typically use dynamic assignment of IP Addresses.");?>
<dl>
<p><?echo I18N("","<strong>Host Name</strong> (optional)<br/>The Host Name field is optional but may be required by some Internet Service Providers. The default host name is the model number of the router.");?></p>
<p><?echo I18N("","<strong>MAC Address</strong> (optional)<br/>The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the \"Clone MAC Address\" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.");?></p>
<p><?echo I18N("","<strong>DNS Mode</strong><br/>Select Receive DNS from ISP if you would like your ISP to provide Dns address information automatically.Select Enter DNS Manually if you would like to enter the Dns address by yourself.");?></p>
<p><?echo I18N("","<strong>Primary DNS Address</strong><br/>Primary DNS (Domain Name System) server IP address, which may be provided by your ISP. ");?></p>
<p><?echo I18N("","<strong>Secondary DNS Address</strong> (optional)<br/>If you were given a secondary DNS server IP address from your ISP, enter it in this field.");?></p>
<p><?echo I18N("","<strong>MTU</strong><br/>MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1500 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.");?></p>
</dl>
</dd>
<dt><?echo I18N("h","PPPoE");?></dt>
<dd><?echo I18N("h","Select this option if your ISP requires you to use a PPPoE (Point to Point Protocol over Ethernet) connection. DSL providers typically use this option. Select Dynamic PPPoE to obtain an IP address automatically for your PPPoE connection (used by majority of PPPoE connections). Select Static PPPoE to use a static IP address for your PPPoE connection.");?>
<dl>
<p><?echo I18N("","<strong>User Name</strong><br/>Enter your PPPoE username.");?></p>
<p><?echo I18N("","<strong>Password</strong><br/>Enter your PPPoE password.");?></p>
<p><?echo I18N("","<strong>Service Name</strong> (optional)<br/>If your ISP uses a service name for the PPPoE connection, enter the service name here.");?></p>
<p><?echo I18N("","<strong>IP Address</strong><br/>This option is only available for Static PPPoE. Enter in the static IP address for the PPPoE connection.");?></p>
<p><?echo I18N("","<strong>MAC Address</strong> (optional)<br/>The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to be the MAC address of the WAN interface on the router. You can use the \"Clone MAC Address\" button to automatically copy the MAC address of the Ethernet Card installed in the computer that is being used to configure the device. It is only necessary to fill in this field if required by your ISP.");?></p>
<p><?echo I18N("","<strong>DNS Mode</strong><br/>Select Receive DNS from ISP if you would like your ISP to provide Dns address information automatically.Select Enter DNS Manually if you would like to enter the Dns address by yourself.");?></p>
<p><?echo I18N("","<strong>Primary DNS Address</strong><br/>Primary DNS (Domain Name System) server IP address, which may be provided by your ISP. You should only need to enter this information if you selected Static PPPoE. If Dynamic PPPoE is chosen, leave this field at its default value as your ISP will provide you this information automatically.");?></p>
<p><?echo I18N("","<strong>Secondary DNS Address</strong> (optional)<br/>If you were given a secondary DNS server IP address from your ISP, enter it in this field.");?></p>
<p><?echo I18N("","<strong>Maximum Idle time</strong><br/>The amount of inactivity time (in minutes) before the device will disconnect your PPPoE session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the defined Maximum Idle Time, then the connection will be dropped. This option only applies to the on demand Connection mode.");?></p>
<p><?echo I18N("","<strong>MTU</strong><br/>MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1492 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.");?></p>
<p><?echo I18N("","<strong>Reconnect Mode</strong><br/>Select Always-on if you would like the router to never disconnect the PPPoE session. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The on demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.");?></p>
</dl>
</dd>
<dt><?echo I18N("h","PPTP");?></dt>
<dd><?echo I18N("h","Select this option if your ISP uses a PPTP (Point to Point Tunneling Protocol) connection and has assigned you a username and password in order to access the Internet. Select Dynamic PPTP to obtain an IP address automatically for your PPTP connection. Select Static PPTP to use a static IP address for your PPTP connection.");?>
<dl>
<p><?echo I18N("","<strong>PPTP IP Address</strong><br/>Enter the IP address that your ISP has assigned to you.");?></p>
<p><?echo I18N("","<strong>PPTP Subnet Mask</strong><br/>Enter the Subnet Mask that your ISP has assigned to you.");?></p>
<p><?echo I18N("","<strong>PPTP Gateway</strong><br/>Enter the Gateway IP address assigned to you by your ISP.");?></p>
<p><?echo I18N("","<strong>Primary DNS Address</strong><br/>Primary DNS (Domain Name System) server IP address, which may be provided by your ISP. You should only need to enter this information if you selected Static PPTP. If Dynamic PPTP is chosen, leave this field at its default value as your ISP will provide you this information automatically.");?></p>
<p><?echo I18N("","<strong>Secondary DNS Address</strong> (optional)<br/>If you were given a secondary DNS server IP address from your ISP, enter it in this field.");?></p>
<p><?echo I18N("","<strong>PPTP Server IP</strong><br/>Enter the IP address of the server, which will be provided by your ISP, that you will be connecting to.");?></p>
<p><?echo I18N("","<strong>Username</strong><br/>Enter your PPTP Username.");?></p>
<p><?echo I18N("","<strong>Password</strong><br/>Enter your PPTP Password.");?></p>
<p><?echo I18N("","<strong>Maximum Idle time</strong><br/>The amount of time of inactivity before the device will disconnect your PPTP session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the specified Maximum Idle Time, the connection will be dropped. This option only applies to the on demand Connection mode.");?></p>
<p><?echo I18N("","<strong>MTU</strong><br/>MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1400 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.");?></p>
<p><?echo I18N("","<strong>Reconnect Mode</strong><br/>Select Always-on if you would like the router to never disconnect the PPTP session. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The on demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.");?></p>
<p><?echo I18N("","<strong>MAC Address</strong> (optional)<br/>The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the \"Clone MAC Address\" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.");?></p>
</dl>
</dd>
<dt><?echo I18N("h","L2TP");?></dt>
<dd><?echo I18N("h","Select this option if your ISP uses a L2TP (Layer 2 Tunneling Protocol) connection and has assigned you a username and password in order to access the Internet. Select Dynamic L2TP to obtain an IP address automatically for your L2TP connection. Select Static L2TP to use a static IP address for your L2TP connection.");?>
<dl>
<p><?echo I18N("","<strong>L2TP IP Address</strong><br/>Enter the IP address that your ISP has assigned to you.");?></p>
<p><?echo I18N("","<strong>L2TP Subnet Mask</strong><br/>Enter the Subnet Mask that your ISP has assigned to you.");?></p>
<p><?echo I18N("","<strong>L2TP Gateway</strong><br/>Enter the Gateway IP address assigned to you by your ISP.");?></p>
<p><?echo I18N("","<strong>Primary DNS Address</strong><br/>Primary DNS (Domain Name System) server IP address, which may be provided by your ISP. You should only need to enter this information if you selected Static L2TP. If Dynamic L2TP is chosen, leave this field at its default value as your ISP will provide you this information automatically.");?></p>
<p><?echo I18N("","<strong>Secondary DNS Address</strong> (optional)<br/>If you were given a secondary DNS server IP address from your ISP, enter it in this field.");?></p>
<p><?echo I18N("","<strong>L2TP Server IP</strong><br/>Enter the IP address of the server, which will be provided by your ISP, that you will be connecting to.");?></p>
<p><?echo I18N("","<strong>Username</strong><br/>Enter your L2TP Username.");?></p>
<p><?echo I18N("","<strong>Password</strong><br/>Enter your L2TP Password.");?></p>
<p><?echo I18N("","<strong>Maximum Idle time</strong><br/>The amount of inactivity time (in minutes) before the device will disconnect your L2TP session. Enter a Maximum Idle Time (in minutes) to define a maximum period of time for which the Internet connection is maintained during inactivity. If the connection is inactive for longer than the defined Maximum Idle Time, then the connection will be dropped. This option only applies to the on demand Connection mode.");?></p>
<p><?echo I18N("","<strong>MTU</strong><br/>MTU (Maximum Transmission/Transfer Unit) is the largest packet size that can be sent over a network. Messages larger than the MTU are divided into smaller packets. 1400 is the default value for this option. Changing this number may adversely affect the performance of your router. Only change this number if instructed to by one of our Technical Support Representatives or by your ISP.");?></p>
<p><?echo I18N("","<strong>Reconnect Mode</strong><br/>Select Always-on if you would like the router to never disconnect the L2TP session. Select Manual if you would like to control when the router is connected and disconnected from the Internet. The on demand option allows the router to establish a connection to the Internet only when a device on your network tries to access a resource on the Internet.");?></p>
<p><?echo I18N("","<strong>MAC Address</strong> (optional)<br/>The MAC (Media Access Control) Address field is required by some Internet Service Providers (ISP). The default MAC address is set to the MAC address of the WAN interface on the router. You can use the \"Clone MAC Address\" button to automatically copy the MAC address of the Ethernet Card installed in the computer used to configure the device. It is only necessary to fill in the field if required by your ISP.");?></p>
</dl>
</dd>

</dl>
</body>
</html>
