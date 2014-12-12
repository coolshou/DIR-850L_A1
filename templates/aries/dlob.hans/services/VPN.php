<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/xnode.php";

fwrite("w", $START, "#!/bin/sh\n");
fwrite("a", $START, "echo \"Start VPN service ..\"  > /dev/console\n");
fwrite("w", $STOP,  "#!/bin/sh\n");
fwrite("a", $STOP, "echo \"Stop VPN service ..\"  > /dev/console\n");
fwrite("a", $STOP, "killall pptpd\n");

$chap_secrets = "/var/etc/ppp/chap-secrets";
$options_pptpd = "/var/etc/ppp/options.pptpd";
$pptpd_conf = "/var/etc/pptpd.conf";
$pptpd_enable = 0;
$max_connections = 10;

//Create /var/etc/ppp/chap-secrets
foreach("/vpn/account/entry")
{
	if($InDeX==1) fwrite("w",$chap_secrets, get("", "name")." pptpd ".get("", "password")." *\n");
	else fwrite("a",$chap_secrets, get("", "name")." pptpd ".get("", "password")." *\n");
}

if(get("", "/vpn/pptp")=="1")
{
	$pptpd_enable = 1;
	fwrite("a", $START, "pptpd -c ".$pptpd_conf."&\n");	
}	

if($pptpd_enable == "1")
{
	// iptables rules
	fwrite("a",$START, "iptables -t nat -A PRE.VPN -p tcp --dport 1723 -j ACCEPT\n");

	$auth_chap = query("/vpn/authtype/chap");
	$auth_mschap = query("/vpn/authtype/mschap");
	$encr_mppe_128 = query("/vpn/encrtype/");
	$isolation = query("/vpn/isolation");

	//Create /var/etc/ppp/options.pptpd
	fwrite("w",$options_pptpd,"name \"pptpd\"\n");
	fwrite("a",$options_pptpd,"mtu 1400\n");
	fwrite("a",$options_pptpd,"mru 1400\n");
	fwrite("a",$options_pptpd,"nobsdcomp\n");
	fwrite("a",$options_pptpd,"nodeflate\n");
	if ($auth_chap=="0" && $auth_mschap=="0")
	{
		fwrite("a",$options_pptpd,"noauth\n");
	} else {
		fwrite("a",$options_pptpd,"refuse-eap\n");
		fwrite("a",$options_pptpd,"refuse-pap\n");
		if ($auth_chap=="1")
			fwrite("a",$options_pptpd,"require-chap\n");
		else
			fwrite("a",$options_pptpd,"refuse-chap\n");
		if ($auth_mschap=="1")
		{
			fwrite("a",$options_pptpd,"require-mschap\n");
			fwrite("a",$options_pptpd,"require-mschap-v2\n");
		} else {
			fwrite("a",$options_pptpd,"refuse-mschap\n");
			fwrite("a",$options_pptpd,"refuse-mschap-v2\n");
		}
		if ($encr_mppe_128=="1")
			fwrite("a",$options_pptpd,"require-mppe-128\n");
	}

	//Create /var/etc/pptpd.conf
	fwrite("w",$pptpd_conf,"option /etc/ppp/options.pptpd\n");
	fwrite("a",$pptpd_conf,"speed 115200\n");
	fwrite("a",$pptpd_conf,"stimeout 10\n");
	fwrite("a",$pptpd_conf,"localip 172.16.0.254\n");
	fwrite("a",$pptpd_conf,"remoteip 172.16.0.1-253\n");
	fwrite("a",$pptpd_conf,"connections ".$max_connections."\n");

	if ($isolation == "1")
	{
		/* can not accesss lan */
		fwrite("a",$START, "iptables -A FWD.VPN -s 172.16.0.0/24 -j DROP\n");
	}
}

fwrite("a",$STOP,  "iptables -t nat -F PRE.VPN\n");
fwrite("a",$STOP,  "iptables -F FWD.VPN\n");

?>
