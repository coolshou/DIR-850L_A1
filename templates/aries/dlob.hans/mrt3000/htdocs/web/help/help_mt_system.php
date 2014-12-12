HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","System");?></h2>
<p>
<?echo I18N("","The current system settings can be saved as a file onto the local hard drive. The saved file or any other saved setting file created by device can be uploaded into the unit. To reload a system settings file, click on <strong>Browse</strong> to search the local hard drive for the file to be used. The device can also be reset back to factory default settings by clicking on <strong>Restore Device</strong>. Use the restore feature only if necessary. This will erase previously save settings for the unit. Make sure to save your system settings before doing a factory restore.");?>
</p>
<dl>
<dt><?echo I18N("h","Save");?></dt>
<dd><?echo I18N("h","Click this button to save the configuration file from the router.");?></dd>
<dt><?echo I18N("h","Browse");?></dt>
<dd><?echo I18N("h","Click Browse to locate a saved configuration file and then click to Load to apply these saved settings to the router.");?></dd>
<dt><?echo I18N("h","Restore Device");?></dt>
<dd><?echo I18N("h","Click this button to restore the router to its factory default settings.");?></dd>
<dt><?echo I18N("h","Reboot The Device");?></dt>
<dd><?echo I18N("h","This restarts the router. Useful for restarting when you are not near the device.");?></dd>
<dt><?echo I18N("h","Clear Language Pack");?></dt>
<dd><?echo I18N("","If you previously installed a language pack and want to revert all the menus on the Router interface back to the default
language settings, click the <strong>Clear</strong> button.");?></dd>
</dl>
</body>
</html>
