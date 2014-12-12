HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
	<h2><a name="VSR"><?echo I18N("h","Virtual Server");?></a></h2>
	<p><?
		echo I18N("h",'The Virtual Server option gives Internet users access to services on your LAN. This feature is useful for hosting online services such as FTP, Web, or game servers. For each Virtual Server, you define a public port on your router for redirection to an internal LAN IP Address and LAN port.');
	?></p>
	<dl>
		<dt><strong><?echo I18N("h","Example");?>: </strong></dt>
		<dd><?echo I18N("h","You are hosting a Web Server on a PC that has LAN IP Address of 192.168.0.50 and your ISP is blocking Port 80.");?>
			<ol>
				<li><?echo I18N("","Name the Virtual Server (for example: <code>Web Server</code>)");?></li>
				<li><?echo I18N("","Enter the IP Address of the machine on your LAN (for example: <code>192.168.0.50</code>)");?></li>
				<li><?echo I18N("h","Enter the Private Port as [80]");?></li>
				<li><?echo I18N("h","Enter the Public Port as [8888]");?></li>
				<li><?echo I18N("","Select the Protocol (for example <code>TCP</code>).");?></li>
				<li><?echo I18N("","Repeat these steps for each Virtual Server Rule you wish to add. After the list is complete, click <span class='button_ref'>Save Settings</span> at the top of the page.");?></li>
			</ol>
			<?echo I18N("h","With this Virtual Server entry, all Internet traffic on Port 8888 will be redirected to your internal web server on port 80 at IP Address 192.168.0.50.");?>
		</dd>
	</dl>
	<dl>
		<dt><strong><?echo I18N("h","Virtual Server Parameters");?></strong></dt>
		<dd>
			<dl>
				<dt><?echo I18N("h","Name");?></dt>
				<dd><?
					echo I18N("","Assign a meaningful name to the virtual server, for example <code>Web Server</code>. Several well-known types of virtual server are available from the 'Application Name' drop-down list. Selecting one of these entries fills some of the remaining parameters with standard values for that type of server.");
				?></dd>
				<dt><?echo I18N("h","IP Address");?></dt>
				<dd><?
					echo I18N("","The IP address of the system on your internal network that will provide the virtual service, for example <code>192.168.0.50</code>. You can select a computer from the list of DHCP clients in the 'Computer Name' drop-down menu, or you can manually enter the IP address of the server computer.");
				?></dd>
				<dt><?echo I18N("h","Traffic Type");?></dt>
				<dd><?
					echo I18N("",'Select the protocol used by the service. The common choices -- UDP, TCP, and both UDP and TCP -- can be selected from the drop-down menu. To specify any other protocol, select "Other" from the list, then enter the corresponding protocol number (<a href="http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xml"> as assigned by the IANA</a>) in the Protocol box.');					
				?></dd>
				<dt><?echo I18N("h","Private Port");?></dt>
				<dd><?echo I18N("h","The port that will be used on your internal network.");?></dd>
				<dt><?echo I18N("h","Public Port");?></dt>
				<dd><?echo I18N("h","The port that will be accessed from the Internet.");?></dd>
			</dl>
		</dd>
	</dl>		
	<dl>	
		<dt><strong><?echo I18N("h","Virtual Servers List");?></strong></dt>
		<dd><?echo I18N("h","Use the checkboxes at the left to activate or deactivate completed Virtual Server entries.");?></dd>
	</dl>
</body>
</html>
