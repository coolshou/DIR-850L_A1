HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Network Settings");?></h2>
<dl>
<dt><?echo I18N("h","Router Settings");?></dt>
<dd><?echo I18N("h",'These are the settings of the LAN (Local Area Network) interface for the device. These settings may be referred to as "private settings". You may change the Router address if needed. The Router IP address is private to your internal network and cannot be seen on the Internet. The default IP address is 192.168.0.1 with a subnet mask of 255.255.255.0.');?>
<dl>
<p><?echo I18N("","<strong>Router IP Address</strong><br/>IP address of the router, default is 192.168.0.1.");?></p>
<p><?echo I18N("","<strong>Default Subnet Mask</strong><br/>Subnet Mask of the router, default is 255.255.255.0.");?></p>
<p><?echo I18N("","<strong>Host Name</strong><br/>The default host name is the model number of the router.");?></p>
<p><?echo I18N("","<strong>Local Domain Name</strong> (optional)<br/>Enter in the local domain name for your network.");?></p>
<p><?echo I18N("","<strong>DNS Relay</strong><br/>When DNS Relay is enabled, DHCP clients of the router will be assigned the router's LAN IP address as their DNS server. All DNS requests that the router receives will be forwarded to your ISPs DNS servers. When DNS relay is disabled, all DHCP clients of the router will be assigned the ISP's DNS server.");?></p>
</dl>
</dd>
<dt><?echo I18N("h","DHCP Server");?></dt>
<dd><?echo I18N("h","DHCP stands for Dynamic Host Control Protocol. The DHCP server assigns IP addresses to devices on the network that request them. By default, the DHCP Server is enabled on the router. The DHCP address pool contains the range of IP addresses that will automatically be assigned to the clients on the network.");?>
<dl>
<dt><?echo I18N("h","DHCP Reservation");?></dt>
<dd><?echo I18N("h",'Enter the "Computer Name", "IP Address" and "MAC Address" manually for the PC that you want the router to statically assign the same IP to or choose the PC from the drop-down menu, which shows current DHCP clients.');?></dd>
<dt><?echo I18N("h","Starting IP address");?></dt>
<dd><?echo I18N("h","The starting IP address for the DHCP server's IP assignment.");?></dd>
<dt><?echo I18N("h","Ending IP address");?></dt>
<dd><?echo I18N("h","The ending IP address for the DHCP server's IP assignment.");?></dd>
<dt><?echo I18N("h","Lease Time");?></dt>
<dd><?echo I18N("h","The length of time in minutes for the IP lease.");?></dd>

<dt><?echo i18n('Always Broadcast');?></dt>
<dd><?	echo i18n("If all the computers on the LAN successfully obtain their IP addresses from the router's DHCP server as expected, this option can remain disabled. However, if one of the computers on the LAN fails to obtain an IP address from the router's DHCP server, it may have an old DHCP client that incorrectly turns off the broadcast flag of DHCP packets. Enabling this option will cause the router to always broadcast its responses to all clients, thereby working around the problem, at the cost of increased broadcast traffic on the LAN.");?></dd>
<dt><?echo i18n('NetBIOS Announcement');?></dt>
<dd><?echo i18n('Check this box to allow the DHCP Server to offer NetBIOS configuration settings to the LAN hosts. NetBIOS allow LAN hosts to discover all other computers within the network, e.g. within Network Neighbourhood.');?></dd>
<dt><?echo i18n('Learn NetBIOS from WAN');?></dt>
<dd><?echo i18n('If NetBIOS announcement is swicthed on, switching this setting on causes WINS information to be learned from the WAN side, if available. Turn this setting off to configure manually.');?></dd>
<dt><?echo i18n('NetBIOS Scope');?></dt>
<dd><?echo i18n("This is an advanced setting and is normally left blank. This allows the configuration of a NetBIOS 'domain' name under which network hosts operate. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");?></dd>
<dt><?echo i18n('NetBIOS node type');?></dt>

<dd><?echo i18n("Indicates how network hosts are to perform NetBIOS name registration and discovery.<br/>
							Broadcast only, this indicates to use local network broadcast ONLY. This setting is useful where there are no WINS servers available, however, it is preferred you try Mixed-mode operation first.<br/>
							Point-to-Point, this indicates to use WINS servers ONLY. This setting is useful to force all NetBIOS operation to the configured WINS servers. You must have configured at least the primary WINS server IP to point to a working WINS server.<br/>
							Mixed-mode, this indicates a Mixed-Mode of operation. First Broadcast operation is performed to register hosts and discover other hosts, if broadcast operation fails, WINS servers are tried, if any. This mode favours broadcast operation which may be preferred if WINS servers are reachable by a slow network link and the majority of network services such as servers and printers are local to the LAN.<br/>
							Hybrid,this indicates a Hybrid-State of operation. First WINS servers are tried, if any, followed by local network broadcast. This is generally the preferred mode if you have configured WINS servers.<br/>
							This setting has no effect if the 'Learn NetBIOS information from WAN' is activated."); ?></dd>

<dt><?echo i18n('Primary WINS Server IP address');?></dt>
<dd><?echo i18n("Configure the IP address of the preferred WINS server. WINS Servers store information regarding network hosts, allowing hosts to 'register' themselves as well as discover other available hosts, e.g. for use in Network Neighbourhood. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");?></dd>
<dt><?echo i18n('Secondary WINS Server IP address');?></dt>
<dd><?echo i18n("Configure the IP address of the backup WINS server, if any. This setting has no effect if the 'Learn NetBIOS information from WAN' is activated.");?></dd>
</dd>
<dd><?echo I18N("h","Dynamic DHCP client computers connected to the unit will have their information displayed in the DHCP Reservations List. The list will show the Host Name, IP Address, MAC Address, and Expired Time of the DHCP lease for each client computer.");?></dd>
</dl>
</body>
</html>
