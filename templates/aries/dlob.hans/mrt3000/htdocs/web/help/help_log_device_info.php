HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Device Info");?></h2>
<p><?echo I18N("h","This page displays the current information for the router. The page will show the version of the firmware currently loaded in the device. ");?></p>
<dl>
<dt><?echo I18N("h","LAN (Local Area Network)");?></dt>
<dd><?echo I18N("h","This displays the MAC Address of the Ethernet LAN interface, the IP Address and Subnet Mask of the LAN interface, and whether or not the router's built-in DHCP server is Enabled or Disabled.");?></dd>
<dt><?echo I18N("h","WAN (Wide Area Network)");?></dt>
<dd><?echo I18N("h","This displays the MAC Address of the WAN interface, as well as the IP Address, Subnet Mask, Default Gateway, and DNS server information that the router has obtained from your ISP. It will also display the connection type (Dynamic, Static, or PPPoE) that is used establish a connection with your ISP. ");?></dd>
<dt><?echo I18N("h","Wireless LAN");?></dt>
<dd><?echo I18N("h","This displays the SSID, Channel, and whether or not Encryption is enabled on the Wireless interface.");?></dd>
</dl>
</body>
</html>
