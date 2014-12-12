<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	if ($USR_ACCOUNTS=="1")
		echo I18N("h","For security reasons, it is recommended that you change the password for the Admin account.");
	else
		echo I18N("h","For security reasons, it is recommended that you change the password for the Admin and User accounts.");
	echo " ";
	echo I18N("h","Be sure to write down the new password to avoid having to reset the router in case they are forgotten.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","Enabling Remote Management allows you to manage the router from anywhere on the Internet. Disabling Remote Management allows you to manage the router only from computers on your LAN.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_tools.php#Admin">'.I18N("h","More...").'</a>';
?></p>
