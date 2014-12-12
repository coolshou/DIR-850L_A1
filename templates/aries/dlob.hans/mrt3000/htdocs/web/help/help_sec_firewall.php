HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Firewall Settings");?></h2>
<p><?echo I18N("h","The Firewall Settings section contains the option to configure a DMZ Host.");?></p>
<dl>
<dt><?echo I18N("","<strong>Enable SPI</strong>");?></dt>
<dd><?echo I18N("h","SPI (\"stateful packet inspection\" also known as \"dynamic packet filtering\") helps to prevent cyber attacks by tracking more state per session. It validates traffic passing through that session conforms to the protocol. When the protocol is TCP, SPI checks that packet sequence numbers are within the valid range for the session, discarding packets that do not have valid sequence number. Whether SPI is enabled or not, the router always tracks TCP connection states and ensures that each TCP packet's flags are valid for the current state.");?></dd>
<dt><strong><?echo i18n("NAT Endpoint Filtering ");?></strong></dt>
<dd><?echo I18N("h","The NAT Endpoint Filtering options control how the router's NAT manages incoming connection requests to ports that are already being used. ");?></dd>
<dd>
<dl>
<dt><strong><?echo i18n("Endpoint Independent");?></strong></dt>
<dd><?echo I18N("h",'Once a LAN-side application has created a connection through a specific port, the NAT will forward any incoming connection requests with the same port to the LAN-side application regardless of their origin. This is the least restrictive option, giving the best connectivity and allowing some applications (P2P applications in particular) to behave almost as if they are directly connected to the Internet.');?></dd>
<dt><strong><?echo i18n("Address Restricted");?></strong></dt>
<dd><?echo I18N("h",'The NAT forwards incoming connection requests to a LAN-side host only when they come from the same IP address with which a connection was established. This allows the remote application to send data back through a port different from the one used when the outgoing session was created.');?></dd>
<dt><strong><?echo i18n("Port And Address Restricted");?></strong></dt>
<dd><?echo I18N("h", 'The NAT does not forward any incoming connection requests with the same port address as an already establish connection.');?></dd>
</dl>
</dd>
<dd><?echo I18N("h","Note that some of these options can interact with other port restrictions. Endpoint Independent Filtering takes priority 
					over inbound filters or schedules, so it is possible for an incoming session request related to an outgoing session to enter 
					through a port in spite of an active inbound filter on that port. However, packets will be rejected as expected when sent to 
					blocked ports (whether blocked by schedule or by inbound filter) for which there are no active sessions. 
					Port and Address Restricted Filtering ensures that inbound filters and schedules work precisely, but prevents some level of 
					connectivity, and therefore might require the use of port triggers, virtual servers, or port forwarding to open the ports needed 
					by the application. Address Restricted Filtering gives a compromise position, which avoids problems when communicating with 
					certain other types of NAT router (symmetric NATs in particular) but leaves inbound filters and scheduled access working as 
					expected.");?></dd>
<dd>
<dl>
<dt><strong><?echo i18n("UDP Endpoint Filtering");?></strong></dt>
<dd><?echo I18N("h",'Controls endpoint filtering for packets of the UDP protocol.');?></dd>
<dt><strong><?echo i18n("TCP Endpoint Filtering ");?></strong></dt>
<dd><?echo I18N("h",'Controls endpoint filtering for packets of the TCP protocol.');?></dd>
</dl>
</dd>
<dd><?echo I18N("h","Formerly, the terms 'Full Cone', 'Restricted Cone', 'Port Restricted Cone' and 'Symmetric' were used to refer to different 
			variations of NATs. These terms are purposely not used here, because they do not fully describe the behavior of this router's NAT. 
			While not a perfect mapping, the following loose correspondences between the 'cone' classification and the 'endpoint filtering' modes 
			can be drawn: if this router is configured for endpoint independent filtering, it implements full cone behavior; address restricted 
			filtering implements restricted cone behavior; and port and address restricted filtering implements port restricted cone behavior.");?></dd>
<dt><strong><?echo i18n("Anti-Spoof checking");?></strong></dt>
<dd><?echo I18N("h","This mechanism protects against activity from spoofed or forged IP addresses, mainly by blocking packets appearing on 
interfaces and in directions which are logically not possible.");?></dd>	
<dt><?echo I18N("","<strong>DMZ</strong>");?></dt>
<dd><?echo I18N("h","If you have a computer that cannot run Internet applications properly from behind the router, then you can allow the computer to have unrestricted Internet access. Enter the IP address of that computer as a DMZ (Demilitarized Zone) host with unrestricted Internet access. Adding a client to the DMZ may expose that computer to a variety of security risks; so only use this option as a last resort.");?></dd>
<dt><?echo I18N("","<strong>Firewall Rules</strong>");?></dt>
<dd><?echo I18N("h","The firewall rules is used to allow or deny traffic coming in or going out to the router based on the source and destination IP addresses as well as the traffic type and the specific port the data runs on.");?>
<dl>
<dt><?echo I18N("","<strong>Name</strong>");?></dt>
<dd><?echo I18N("h","Users can specify a name for a firewall rule.");?></dd>
<dt><?echo I18N("","<strong>Action</strong>");?></dt>
<dd><?echo I18N("h","Users can choose to allow or deny traffic.");?></dd>
<!--<dt><strong>Source Interface</strong></dt>
<dd>Use the <strong>Source</strong> drop down menu to select the starting point of the traffic that's to be allowed or denied is from LAN or WAN interface.</dd>
<dt><strong>Dest Interface</strong></dt>
<dd>Use the <strong>Dest</strong> drop down menu to select the ending point of the traffic that's to be allowed or denied is arriving at LAN or WAN interface.</dd>
-->
<dt><?echo I18N("","<strong>Source IP & Dest IP</strong>");?></dt>
<dd><?echo I18N("h","Here you can specify a single source or dest IP by entering the IP in the top box or enter a range of IPs by entering the first IP of the range in the top box and the last IP of the range in the bottom one.");?></dd>
<dt><?echo I18N("","<strong>Protocol</strong>");?></dt>
<dd><?echo I18N("","Use the <strong>Protocol</strong> drop down menu to select the traffic type.");?></dd>
<dt><?echo I18N("","<strong>Dest Port Range</strong>");?></dt>
<dd><?echo I18N("h","Enter the same port number in both boxes or only enter the port number in the top box to specify a single port or enter the first port of the range in the top box and last port of the range in the bottom one to specify a range of ports.");?></dd>
<!--
<dt><strong>Schedule</strong></dt>
<dd>Use the <strong>Always</strong> drop down menu to select a previously defined schedule or click on <strong>New Schedule</strong> button to add a new schedule.</dd>
-->
</dl>
</dd>
</dl>
</body>
</html>
