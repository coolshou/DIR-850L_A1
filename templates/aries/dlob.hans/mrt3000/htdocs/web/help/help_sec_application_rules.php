HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Application Rules");?></h2>
<p><?echo I18N("h",'Some applications require multiple connections, such as Internet gaming, video conferencing, Internet telephony and others. These applications have difficulties working through NAT (Network Address Translation). If you need to run applications that require multiple connections, specify the port normally associated with an application in the "Trigger Port" field, select the protocol type as TCP (Transmission Control Protocol) or UDP (User Datagram Protocol), then enter the public ports associated with the trigger port in the Firewall Port field to open them for inbound traffic. There are already defined well-known applications in the Application Name drop down menu.');?></p>
<dl>
<dt><?echo I18N("","<strong>Name</strong>");?></dt>
<dd><?echo I18N("h","This is the name referencing the application.");?></dd>
<dt><?echo I18N("","<strong>Trigger Port</strong>");?></dt>
<dd><?echo I18N("h","This is the port used to trigger the application. It can be either a single port or a range of ports.");?></dd>
<dt><?echo I18N("","<strong>Traffic Type</strong>");?></dt>
<dd><?echo I18N("h","This is the protocol used to trigger the application.");?></dd>
<dt><?echo I18N("","<strong>Firewall Port</strong>");?></dt>
<dd><?echo I18N("h","This is the port number on the WAN side that will be used to access the application. You may define a single port or a range of ports. You can use a comma to add multiple ports or port ranges.");?></dd>
<dt><?echo I18N("","<strong>Traffic Type</strong>");?></dt>
<dd><?echo I18N("h","This is the protocol used for the application. ");?></dd>
<!--<dt>Schedule</dt>
<dd>Select a schedule for when the service will be enabled.
If you do not see the schedule you need in the list of schedules, go to the <a href='../maintenance/mt_schedules.htm'> Tools  Schedules</a> <?echo I18N("h","screen and create a new schedule.");?></dd>
</dl>-->
</body>
</html>
