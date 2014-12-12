HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Statistics");?></h2>
<p><?echo I18N("h","The router keeps statistic of the data traffic that it handles. You are able to view the amount of packets that the router has Received and Transmitted on the Internet (WAN), LAN, and Wireless interfaces.");?></p>
<dl>
<dt><?echo I18N("h","Refresh");?></dt>
<dd><?echo I18N("h","Click this button to update the counters.");?></dd>
<!--
<dt><?echo I18N("h","Reset");?></dt>
<dd><?echo I18N("h","Click this button to clear the counters. The traffic counter will reset when the device is rebooted.");?></dd>
-->
</dl>
</body>
</html>
