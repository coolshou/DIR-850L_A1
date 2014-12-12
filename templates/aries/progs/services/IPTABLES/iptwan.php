<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function IPTWAN_build_command($name)
{
	fwrite(w, $_GLOBALS["START"],
		"#!/bin/sh\n".
		"iptables -t nat -F PRE.".$name."\n".
		"iptables -F FWD.".$name."\n".
		"iptables -F INP.".$name."\n"
		);

	$iptcmd = "iptables -t nat -A PRE.".$name;
	$iptcmd2= "iptables -t filter -A INP.".$name;
	$path = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
	if ($path!="")
	{
		/* PREROUTING (for DNAT) */
		$nat = query($path."/nat");
		if ($nat!="")
		{
			/* Is there anyone enable IGD ? */
			$igd = 0;
			foreach ("/inf")
			{
				$cnt = query("upnp/count");
				$i = 0;
				while ($i < $cnt)
				{
					$i++;
					$value = query("upnp/entry:".$i);
					if ($value=="urn:schemas-upnp-org:device:InternetGatewayDevice:1") $igd++;
				}
			}
			$vsvr  = XNODE_get_var("DNAT.VSVR.".$nat.".USED");
			$pfwd  = XNODE_get_var("DNAT.PFWD.".$nat.".USED");
			$dmz   = XNODE_get_var("DNAT.DMZ.".$nat.".USED");
			$portt = XNODE_get_var("PORTT.".$nat.".USED");
			//TRACE_debug("IPT.".$name.": DNAT.VSVR.USED= ".$vsvr.
			//	" DNAT.PFWD.USED= ".$pfwd." PORTT.USED= ".$portt." DNAT.DMZ.USED= ".$dmz);
			if ($igd>0)   fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.UPNP\n");
			if ($vsvr>0)  fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.VSVR.".$nat."\n");
			if ($pfwd>0)  fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.PFWD.".$nat."\n");
			if ($portt>0) fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.PORTT.".$nat."\n");

			/* Move DMZ above the FIREWALL. Builder.*/
			//if ($dmz>0)   fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.DMZ.".$nat."\n");
		}

		/* ICMP */
		$val = query($path."/icmp");
		/*hendry*/
		if ($val=="") { $val="DROP"; }				//default
		if ($val=="ACCEPT" || $val=="DROP")
		{
			if ($val=="DROP")
			{
				$logcmd = " -j LOG --log-level notice --log-prefix 'DRP:003:'";
				fwrite("a",$_GLOBALS["START"],
					$iptcmd." -p icmp --icmp-type echo-request -m limit --limit 10/m".$logcmd."\n".
					$iptcmd." -p icmp --icmp-type echo-reply   -m limit --limit 10/m".$logcmd."\n".
					);
				fwrite("a",$_GLOBALS["START"],
					$iptcmd2." -p icmp --icmp-type echo-request -m limit --limit 10/m".$logcmd."\n".
					);
			}
			fwrite("a",$_GLOBALS["START"],
				$iptcmd." -p icmp --icmp-type echo-request -j ".$val."\n".
				$iptcmd." -p icmp --icmp-type echo-reply -j ".$val."\n"
				);
			fwrite("a",$_GLOBALS["START"],
				$iptcmd2." -p icmp --icmp-type echo-request -j ".$val."\n".
				);
			if($name=="WAN-1")
			{
				if ($val=="DROP")
				{
					fwrite("a",$_GLOBALS["START"],
						"logger -p 192.0 \"Block WAN PING is enabled.\"\n"
						);	
				}
				else
				{
					fwrite("a",$_GLOBALS["START"],
						"logger -p 192.0 \"Block WAN PING is disabled.\"\n"
						);
				}
			}
		}
		/* IGMPProxy */
		$val = query("/device/multicast/igmpproxy");
		if ($val=="1") fwrite("a",$_GLOBALS["START"], $iptcmd." -j PRE.IGMP\n");
		/* Remote management */
		$web = query($path."/web");
		if ($web != "")
		{
			$hostip = query($path."/weballow/hostv4ip");
			if ($hostip != "")
				fwrite("a",$_GLOBALS["START"], $iptcmd." -s ".$hostip." -p tcp --dport ".$web." -j ACCEPT\n");
			else
				fwrite("a",$_GLOBALS["START"], $iptcmd." -p tcp --dport ".$web." -j ACCEPT\n");
		}

		if($name=="WAN-1")
		{
			if ($web != "")
			{
				fwrite("a",$_GLOBALS["START"],
					"logger -p 192.0 \"Remote management is enabled at port ".$web.".\"\n"
					);
			}
			else
			{
				fwrite("a",$_GLOBALS["START"],
					"logger -p 192.0 \"Remote management is disabled.\"\n"
					); 
			}
		}

	        /*if wan is dhcp accept udp port 68*/
		$inet	= query($path."/inet"); 
		$inetp = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
		if ($inetp != "")
		{
			if(query($inetp."/addrtype")=="ipv4" && query($inetp."/ipv4/static")==0)
			{
				fwrite("a",$_GLOBALS["START"],$iptcmd." -p udp --dport 68 -j ACCEPT \n");
			}
		}
		/* if wan is snmp accept udp port 161  */
		$ipaddr=query($inetp."/ipv4/ipaddr");
    	$enable_snmp = query("/snmp/active");
    	$snmp_inf = query("/snmp/inf");
    	$dev = PHYINF_getruntimeifname($name);
    	if ($ipaddr != "")
		{
      		if($enable_snmp=="1")
    		{
    	  		if($snmp_inf== $name || $snmp_inf == "ALL")//snmp open wan set this rule 
      			{
    	 			fwrite("a",$_GLOBALS["START"], $iptcmd." -p udp --dport 161 -d ".$ipaddr." -j ACCEPT\n");
    			}
   	 		}		
    	}		
		/* FIREWALL */
		$firewall = XNODE_get_var("FIREWALL.USED");
		if($firewall>0) fwrite("a",$_GLOBALS["START"],"iptables -A FWD.".$name." -j FIREWALL\n");

		/* FIREWALL-2 */
		$firewall2 = XNODE_get_var("FIREWALL-2.USED");
		if($firewall2>0) fwrite("a",$_GLOBALS["START"],"iptables -A FWD.".$name." -j FIREWALL-2\n");

		/* FIREWALL-3 */
		$firewall3 = XNODE_get_var("FIREWALL-3.USED");
		if($firewall3>0) fwrite("a",$_GLOBALS["START"],"iptables -A FWD.".$name." -j FIREWALL-3\n");

		/* IPv6 Tunnel: 6TO4, tunnel and so on. */
		//$inf6to4 = query($path."/inf6to4/mode");	
		//if ($inf6to4 != "") fwrite("a",$_GLOBALS["START"], $iptcmd." -p 41 -j ACCEPT\n");
	}

	fwrite("a",$_GLOBALS["START"], $iptcmd." -p 41 -j ACCEPT\n"); /* Accept all IPv6 tunnel traffic. */
	fwrite("a",$_GLOBALS["START"], $iptcmd." -m state --state ESTABLISHED,RELATED -j ACCEPT\n");

	/* With Daniel's NAT, we need rules in PREROUTING to do DNAT (use to control the cone NAT type. */
	$type = fread("e","/etc/config/nat");
	if ($type == "Daniel's NAT" && $nat!="")
		fwrite(a, $_GLOBALS["START"],
			$iptcmd." -p tcp -j PRE.MASQ.".$nat."\n".
			$iptcmd." -p udp -j PRE.MASQ.".$nat."\n"
			);

	/* Make DMZ the lowest priority. */
	if ($dmz>0) fwrite("a",$_GLOBALS["START"], $iptcmd." -j DNAT.DMZ.".$nat."\n");
	fwrite("a",$_GLOBALS["START"], $iptcmd." -j DROP\nexit 0\n");

	fwrite("w", $_GLOBALS["STOP"],
		"#!/bin/sh\n".
		"iptables -t nat -F PRE.".$name."\n".
		"iptables -F FWD.".$name."\n".
		"iptables -F INP.".$name."\n".
		"exit 0\n"
		);
}

?>
