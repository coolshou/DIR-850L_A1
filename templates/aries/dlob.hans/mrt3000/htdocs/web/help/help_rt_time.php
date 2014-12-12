HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Time");?></h2>
<p><?echo I18N("h","The Time Configuration settings are used by the router for synchronizing scheduled services and system logging activities. You will need to set the time zone corresponding to your location. The time can be set manually or the device can connect to a NTP (Network Time Protocol) server to retrieve the time. You may also set Daylight Saving dates and the system time will automatically adjust on those dates.");?></p>
<dl>
<dt><?echo I18N("h","Time Zone");?></dt>
<dd><?echo I18N("h","Select the Time Zone for the region you are in.");?></dd>
<dt><?echo I18N("h","Daylight Saving");?></dt>
<dd><?echo I18N("h","If the region you are in observes Daylight Savings Time, enable this option and specify the Starting and Ending Month, Week, Day, and Time for this time of the year.");?></dd>
<dt><?echo I18N("h","Automatic Time Configuration");?></dt>
<dd><?echo I18N("h","Select a D-Link time server which you would like the router to synchronize its time with. The interval at which the router will communicate with the D-Link NTP server is set to 7 days.");?></dd>
<dt><?echo I18N("h","Set the Date and Time Manually");?></dt>
<dd><?echo I18N("h","Select this option if you would like to specify the time manually. You must specify the Year, Month, Day, Hour, Minute, and Second, or you can click the Copy Your Computer's Time Settings button to copy the system time from the computer being used to access the management interface.");?></dd>
</dl>
</body>
</html>
