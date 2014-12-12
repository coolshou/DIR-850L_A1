HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Port Forwarding");?></h2>
<p><?echo I18N("h","The Port Forwarding option gives Internet users access to services on your LAN. This feature is useful for hosting online services such as FTP, Web or game servers. For each entry, you define a public port on your router for redirection to an internal LAN IP Address and LAN port.");?></p>
<dl>
<dt><?echo I18N("","<strong>Port Forwarding Parameters</strong>");?></dt>
<dd>
<dl>
<dt><?echo I18N("","<strong>Name</strong>");?></dt>
<dd><?echo I18N("h",'Assign a meaningful name to the port forwarding, for example Web Server. Several well-known types of port forwarding are available from the "Application Name" drop-down list. Selecting one of these entries fills some of the remaining parameters with standard values for that type of server.');?></dd>
<dt><?echo I18N("","<strong>IP Address</strong>");?></dt>
<dd><?echo I18N("h",'The IP address of the system on your internal network that will provide the virtual service, for example 192.168.0.50. You can select a computer from the list of DHCP clients in the "Computer Name" drop-down menu, or you can manually enter the IP address of the server computer.');?></dd>
<dt><?echo I18N("","<strong>Application Name</strong>");?></dt>
<dd><?echo I18N("h","A list of pre-defined popular applications that users can choose from for faster configuration.");?></dd>
<dt><?echo I18N("","<strong>Computer Name</strong>");?></dt>
<dd><?echo I18N("h","A list of DHCP clients.");?></dd>
<dt><?echo I18N("","<strong>TCP</strong>");?></dt>
<dd><?echo I18N("h","Select the protocol used by the service. ");?></dd>
<dt><?echo I18N("","<strong>UDP</strong>");?></dt>
<dd><?echo I18N("h","Select the protocol used by the service. ");?></dd>
</dl>
</dd>
</dl>
</body>
</html>
