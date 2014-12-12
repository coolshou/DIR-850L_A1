HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Logs");?></h2>
<p><?echo I18N("h","You can save the log file to a local drive which can later be used to send to a network administrator for troubleshooting.");?></p>
<dl>
<dt><?echo I18N("h","Save");?></dt>
<dd><?echo I18N("h","Click this button to save the log entries to a text file.");?></dd>
</dl>
<p><?echo I18N("h","The router keeps a running log of events and activities occurring on it at all times. The log will display up to 400 recent system logs, 50 firewall and security logs, and 50 router status logs. Newer log activities will overwrite the older logs.");?></p>
<dl>
<dt><?echo I18N("h","First Page");?></dt>
<dd><?echo I18N("h","Click this button to go to the first page of the log.");?></dd>
<dt><?echo I18N("h","Last Page");?></dt>
<dd><?echo I18N("h","Click this button to go to the last page of the log.");?></dd>
<dt><?echo I18N("h","Previous");?></dt>
<dd><?echo I18N("h","Moves back one log page.");?></dd>
<dt><?echo I18N("h","Next");?></dt>
<dd><?echo I18N("h","Moves forward one log page.");?></dd>
<dt><?echo I18N("h","Clear");?></dt>
<dd><?echo I18N("h","Clears the logs completely.");?></dd>
<dt><?echo i18n("Link To Email Log Setting");?></dt>
<dd><?echo i18n("If you provided email information, clicking the <strong>Link To Email Log Setting</strong> button sends the router log to the configured email address.");?></dd>
<!--<dt>Log Type</dt>
<dd>Select the type of information you would like the DWR-117 to log.</dd>-->
<dt><?echo I18N("h","Log Level");?></dt>
<dd><?echo I18N("h","Select the level of information you would like the MRT-3000 to log.");?></dd>
</dl>
</body>
</html>
