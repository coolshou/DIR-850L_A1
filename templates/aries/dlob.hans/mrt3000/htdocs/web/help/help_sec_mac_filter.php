HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Network Filter (MAC Address Filter)");?></h2>
<p><?echo I18N("h","Use MAC Filters to deny computers within the local area network from accessing the Internet. You can either manually add a MAC address or select the MAC address from the list of clients that are currently connected to the unit.");?></p>
<p><?echo I18N("h",'Select "Turn MAC Filtering ON and ALLOW computers with MAC address listed below to access the network" if you only want selected computers to have network access and all other computers not to have network access.');?></p></p>
<p>Select "Turn MAC Filtering ON and DENY computers with MAC address listed below to access the network" if you want all computers to have network access except those computers in the list.</p>
<dl>
<dt><?echo I18N("","<strong>MAC Address</strong>");?></dt>
<dd><?echo I18N("h","The MAC address of the network device to be added to the MAC Filter List.");?></dd>
<dt><?echo I18N("","<strong>DHCP Client List</strong>");?></dt>
<dd><?echo I18N("h","DHCP clients will have their hostname in the Computer Name drop down menu. You can select the client computer you want to add to the MAC Filter List and click arrow button. This will automatically add that computer's MAC address to the appropriate field.");?></dd>
</dd>
</dl>
<!--
<p><?echo I18N("","Users can use the <strong>Always</strong> drop down menu to select a previously defined schedule or click the <strong>New Schedule</strong> button to add a new schedule.");?></p>
-->
<p><?echo I18N("h","The check box is used to enable or disable a particular entry.");?></p>
</body>
</html>
