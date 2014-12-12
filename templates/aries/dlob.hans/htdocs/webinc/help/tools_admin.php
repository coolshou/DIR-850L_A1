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
	echo I18N("h","When enabling Remote Management, you can specify the IP address of the computer on the Internet that you want to have access to your router, or leave it blank to allow access to any computer on the Internet.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Select a filter that controls access as needed for this admin port. If you do not see the filter you need in the list of filters, go to the <a href='/adv_inb_filter.php'>Advanced -> Inbound Filter</a> screen and create a new filter.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_tools.php#Admin">'.I18N("h","More...").'</a>';
?></p>
