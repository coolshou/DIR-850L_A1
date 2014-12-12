<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".I18N("h","Enable").":</strong><br/>";
	echo I18N("h","Specifies whether the entry will be enabled or disabled.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".I18N("h","Interface").":</strong><br/>";
	echo I18N("h","Specifies the interface -- WAN -- that the IP packet must use to transit out of the router, when this route is used.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".I18N("h","Destination IP").":</strong><br/>";
	echo I18N("h","The IP address of packets that will take this route.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".I18N("h","Netmask").":</strong><br/>";
	echo I18N("h","One bit in the mask specifies which bits of the IP address must match.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".I18N("h","Gateway").":</strong><br/>";
	echo I18N("h","The gateway IP address is the IP address of the router, if any, used to reach the specified destination.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_adv.php#Routing">'.I18N("h","More...").'</a>';
?></p>
