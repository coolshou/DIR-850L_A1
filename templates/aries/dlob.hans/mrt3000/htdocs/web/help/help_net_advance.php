HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Advanced Network");?></h2>
<p><?echo I18N("h","This section contains settings which can change the way the router handles certain types of traffic. We recommend that you not change any of these settings unless you are already familiar with them or have been instructed to change them by one of our support personnel.");?></p>
<dl>
<dt><?echo I18N("","<strong>UPnP</strong>");?></dt>
<dd><?echo I18N("h","UPnP is short for Universal Plug and Play which is a networking architecture that provides compatibility among networking equipment, software, and peripherals. The device is an UPnP enabled router, meaning it will work with other UPnP devices/software. If you do not want to use the UPnP functionality, it can be disabled by selecting \"Disabled\".");?></dd>
<dt><?echo I18N("","<strong>WAN Ping</strong>");?></dt>
<dd><?echo I18N("h","When you Enable WAN Ping respond, you are causing the public WAN (Wide Area Network) IP address on the device to respond to ping commands sent by Internet users. Pinging public WAN IP addresses is a common method used by hackers to test whether your WAN IP address is valid.");?></dd>
<dt><?echo I18N("","<strong>WAN Port Speed</strong>");?></dt>
<dd><?echo I18N("h","This allows you to select the speed of the WAN interface of the router: Choose 1000Mbps, 100Mbps, 10Mbps, or 10/100/1000Mbps Auto.");?></dd>
<dt><?echo I18N("","<strong>Multicast Streams</strong>");?></dt>
<dd><?echo I18N("h","Enable this option to allow Multicast traffic to pass from the Internet to your network more efficiently.");?></dd>
<dt><?echo I18N("","<strong>Enable Multicast Streams</strong>");?></dt>
<dd><?echo I18N("h","Enable this option if you are receiving video on demand type of service from the Internet. The router uses the IGMP protocol to support efficient multicasting -- transmission of identical content, such as multimedia, from a source to a number of recipients. This option must be enabled if any applications on the LAN participate in a multicast group. If you have a multimedia LAN application that is not receiving content as expected, try enabling this option.");?></dd>
<dt><?echo I18N("","<strong>Wireless Enhance Mode</strong>");?></dt>
<dd><?echo I18N("h","Enable this option to allow more smooth Multicast traffic.");?></dd>
</dl>
</body>
</html>
