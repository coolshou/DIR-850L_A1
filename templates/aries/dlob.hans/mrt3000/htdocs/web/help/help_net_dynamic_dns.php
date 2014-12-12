HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Dynamic DNS");?></h2>
<p><?echo I18N("h","Dynamic DNS (Domain Name Service) is a method of keeping a domain name linked to a changing (dynamic) IP address. With most Cable and DSL connections, you are assigned a dynamic IP address and that address is used only for the duration of that specific connection. With the router, you can setup your DDNS service and the router will automatically update your DDNS server every time it receives a new WAN IP address.");?></p>
<dl>
<dt><?echo I18N("h","Server Address");?></dt>
<dd><?echo I18N("h","Choose your DDNS provider from the drop down menu.");?></dd>
<dt><?echo I18N("h","Host Name");?></dt>
<dd><?echo I18N("h","Enter the Host Name that you registered with your DDNS service provider.");?></dd>
<dt><?echo I18N("h","User Account");?></dt>
<dd><?echo I18N("h","Enter the username for your DDNS account.");?></dd>
<dt><?echo I18N("h","Password");?></dt>
<dd><?echo I18N("h","Enter the password for your DDNS account.");?></dd>
</dl>
</body>
</html>
