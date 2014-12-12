HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Internet Sessions");?></h2>
<p><?echo I18N("h","Internet Session display Source and Destination sessions passing through the router.");?></p>
<dl>
<dt><?echo I18N("h","IP Address");?></dt>
<dd><?echo I18N("h","The source IP address of where the sessions are originated from.");?></dd>
<dt><?echo I18N("h","TCP Sessions");?></dt>
<dd><?echo I18N("h","This shows the number of TCP sessions are being sent from the source IP address.");?></dd>
<dt><?echo I18N("h","UDP Sessions");?></dt>
<dd><?echo I18N("h","This shows the number of UDP sessions are being sent from the source IP address.");?></dd>
</dl>
</body>
</html>
