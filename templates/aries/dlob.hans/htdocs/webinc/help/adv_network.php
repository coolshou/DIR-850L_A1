<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","UPnP helps other UPnP LAN hosts interoperate with the router. Leave the UPnP option enabled as long as the LAN has other UPnP applications.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("For added security, it is recommended that you disable the <strong>WAN Ping Response</strong> option. Ping is often used by malicious Internet users to locate active networks or PCs.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","The WAN speed is usually detected automatically. If you are having problems connecting to the WAN, try selecting the speed manually.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","If you are having trouble receiving video on demand type of service from the Internet, make sure the Multicast Stream option is enabled.");
?></p>
<p <?if ($FEATURE_EEE !="1") echo ' style="display:none;"';?>>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","Energy Efficient Ethernet(EEE), also known as IEEE 802.3az, is a set of enhancements to the twisted-pair and backplane ethernet networking standards that will allow for less power consumption during periods of low data activity.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_adv.php#Network">'.I18N("h","More...").'</a>';
?></p>
