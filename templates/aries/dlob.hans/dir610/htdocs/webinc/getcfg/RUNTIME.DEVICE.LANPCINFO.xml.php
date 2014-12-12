<module>
	<service><?=$GETCFG_SVC?></service>
	<runtime>
		<lanpcinfo>
<?
include "/htdocs/phplib/xnode.php";
foreach("/runtime/mydlink/userlist/entry")
{
	$ip = query("ipv4addr");
	$mac = query("macaddr");
	$hostname = query("hostname");
	$ipv6 = query("ipv6addr");
	echo "\t\t\t<entry>\n";
	echo "\t\t\t\t<ipaddr>".$ip."</ipaddr>\n";
	echo "\t\t\t\t<macaddr>".$mac."</macaddr>\n";
	echo "\t\t\t\t<hostname>".$hostname."</hostname>\n";
	echo "\t\t\t\t<ipv6addr>".$ipv6."</ipv6addr>\n";
	echo "\t\t\t</entry>\n";
}
?>
		</lanpcinfo>
	</runtime>
	<SETCFG>ignore</SETCFG>
	<ACTIVATE>ignore</ACTIVATE>
</module>
