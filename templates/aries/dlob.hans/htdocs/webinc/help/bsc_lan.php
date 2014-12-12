<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("If you already have a DHCP server on your network or are using static IP addresses on all the devices on your network, uncheck <strong>Enable DHCP Server</strong> to disable this feature.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("If you have devices on your network that should always have fixed IP addresses, add a <strong>DHCP Reservation</strong> for each such device.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_setup.php#Network">'.I18N("h","More...").'</a>';
?></p>
