#!/bin/sh
<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/xnode.php";

function add_each($list, $path, $node)
{
	$i = 0;
	$cnt = scut_count($list, "");
	while ($i < $cnt)
	{
		$val = scut($list, $i, "");
		if ($val!="") add($path."/".$node, $val);
		$i++;
	}
	return $cnt;
}

/*sam_pan add*/
function netbios_handler($SCOPE, $WINSTYPE,$WINS)
{							
	foreach("/dhcps4/entry")
	{
		$active       = query("netbios/active");
		$learnfromwan = query("netbios/learnfromwan");
		$old_scpoe    = query("netbios/scope");		
		$old_winstype = query("netbios/ntype");
		$old_win1     = query("wins/entry:1");
		$old_win2     = query("wins/entry:2");								
		$SCOPE        = strip($SCOPE);
		$WINSTYPE     = strip($WINSTYPE);
		$WINS         = strip($WINS);
								
		if($active == "1" && $learnfromwan == "1") 
		{																																												
			$winlist = $old_win1;
			if($old_win2!="")
			{
				$winlist= $winlist." ".$old_win2;			
			}
			
			//echo '\necho scope='.$SCOPE.' winstype='.$WINSTYPE.' winlist = '.$winlist.' wins ='.$WINS.'\n';
			if($old_scpoe == $SCOPE && $old_winstype == $WINSTYPE && $winlist == $WINS)
			{				
				return 0;	
			}	
			//echo '\necho netbios changed\n';
						
			set("netbios/scope", $SCOPE);
			set("netbios/ntype", $WINSTYPE);															 			
			del("wins/entry:1");	
			del("wins/entry:2");
			
			if($WINS == "") 
			{
				set("wins/count", 0);				
			}
			else
			{	
				$total_len = strlen($WINS);
				$sub_len = strstr($WINS, " ");			
				
				echo 'echo sublen'.$sub_len.'\n';					
				if($sub_len != "")
				{				
					$win1 = substr($WINS, 0, $sub_len);														
					$win2 = substr($WINS, $sub_len+1, $total_len - $sub_len-1); 														
					
					set("wins/count", 2);
					set("wins/entry:1", $win1);								
					set("wins/entry:2", $win2);				
					
				}
				else
				{			
					set("wins/count", 1);		
					set("wins/entry:1", $WINS);								
				}
			}				
			return 1;													 				
		}						
	}
	return 0;					
}

if ($ACTION=="bound")
{
	/* Actuall, we don't need to set the 'udhcpc' nodes under /runtime/inf.
	 * Those were temporary nodes of the old implementation. (That's why I did not put into the documentation.)
	 * But there are still some modules referencing these nodes, so I keep these code for compatible reason.
	 *			David Hsieh <david_hsieh@alphanetworks.com> */

	/* Anchor to the target interface's runtime status path.  */
	$sts = XNODE_getpathbytarget("/runtime",  "inf", "uid", $INF, 1);

	/* Check if there are existing setting ? */
	if (query($sts."/udhcpc/inet")==$INET)
	{
		anchor($sts."/udhcpc");
		if (query("interface")==$INTERFACE &&
			query("ip")==$IP &&
			query("subnet")==$SUBNET &&
			query("broadcast")==$BROADCAST &&
			query("lease")==$LEASE &&
			query("domain")==$DOMAIN &&
			query("raw_router")==$ROUTER &&
			query("raw_dns")==$DNS &&
			query("raw_clsstrout")==$CLSSTROUT &&
			query("raw_sstrout")==$SSTROUT &&
			query("sixrd_pfx")==$SIXRDPFX &&
			query("sixrd_pfxlen")==$SIXRDPFXLEN &&
			query("sixrd_msklen")==$SIXRDMSKLEN &&
			query("sixrd_bripaddr")==$SIXRDBRIP)
		{
			$nochange = 1;			
		}
		else
		{
			$nochange = 0;			
		}								
	}
		
	$netbios_changed = netbios_handler($SCOPE, $WINSTYPE, $WINS);
	if($netbios_changed == 1)
	{ 			
		$nochange = 0;						
	}
	
	if($nochange ==1)
	{
		echo 'echo "[$0]: no changed in '.$INF.' ..." > /dev/console';			
	}	
	else
	{			
		echo "phpsh /etc/scripts/IPV4.INET.php ACTION=DETACH INF=".$INF."\n";
	}	
		
	if ($nochange!=1)
	{
		del($sts."/udhcpc");
		set($sts."/udhcpc/inet",$INET);
		anchor($sts."/udhcpc");

		/* Record the arguments */
		set("interface",$INTERFACE);
		set("ip",		$IP);
		set("subnet",	$SUBNET);
		set("broadcast",$BROADCAST);
		set("lease",	$LEASE);
		set("domain",	$DOMAIN);
		set("raw_router",	$ROUTER);
		set("raw_dns",		$DNS);
		set("raw_clsstrout",$CLSSTROUT);
		set("raw_sstrout",	$SSTROUT);		

		/* 6rd info */
		set("sixrd_pfx",	$SIXRDPFX);		
		set("sixrd_pfxlen",	$SIXRDPLEN);		
		set("sixrd_msklen",	$SIXRDMSKLEN);		
		set("sixrd_brip",	$SIXRDBRIP);		

		add_each($ROUTER,	$statusp."/udhcpc", "router");
		add_each($DNS,		$statusp."/udhcpc", "dns");
		add_each($CLSSTROUT,$statusp."/udhcpc", "cltrout");
		add_each($SSTROUT,	$statusp."/udhcpc", "sstrout");

		/* check ip alias */
		$cnt = query($sts."/ipalias/cnt")+0;
		$same_ip = "0";
		if($cnt!="" && $cnt!="0")
		{
			foreach($sts."/ipalias/ipv4/ipaddr")
			{
				if($InDeX > $cnt) break;
				$aip = query($sts."/ipalias/ipv4/ipaddr:".$InDeX);
				/*
				if($aip!="" && $IP==$aip)
				{
					$same_ip = "1";
					echo "ifconfig ".$INTERFACE.":".$InDeX." down\n";
					$index = $InDeX;
					break;	
				}
				*/
				$amask = "";
				$anetid = ipv4networkid($aip,"24");
				if($anetid == "192.168.0.0") $amask = "24";
				if($amask=="")
				{
					$anetid = ipv4networkid($aip, "16");
					if($anetid == "169.254.0.0") $amask = "16";
				} 
				$netid = ipv4networkid($IP, $amask);
				/* check if subnet is the same */
				if($aip!="" && $netid==$anetid)
				{
					$same_ip = "1";
					echo "ifconfig ".$INTERFACE.":".$InDeX." down\n";
					$index = $InDeX;
					break;	
				}
				
			}
		}	

		echo "phpsh /etc/scripts/IPV4.INET.php ACTION=ATTACH".
				" STATIC=0".
				" INF=".$INF.
				" DEVNAM=".$INTERFACE.
				" MTU=".$MTU.
				" IPADDR=".$IP.
				" SUBNET=".$SUBNET.
				" BROADCAST=".$BROADCAST.
				" GATEWAY=".$ROUTER.
				' "DOMAIN='.$DOMAIN.'"'.
				' "DNS='.$DNS.'"'.
				' "CLSSTROUT='.$CLSSTROUT.'"'.
				' "SSTROUT='.$SSTROUT.'"'.
				'\n';
		$restartdhcpswer = 0;
		if ($DOMAIN != query("/runtime/device/domain"))
		{
			echo "xmldbc -s /runtime/device/domain \"".$DOMAIN."\"\n";
			$restartdhcpswer = 1;
		}
		/*Check LAN DHCP setting. We will resatrt DHCP server if the DNS relay is disabled*/
		foreach ("/inf")
		{
		    $disable= query("disable");
		    $active = query("active");
		    $dhcps4 = query("dhcps4");
		    $dns4 = query("dns4");
		    if ($disable != "1" && $active=="1" && $dhcps4!="")
		    {
	            if ($dns4 =="")
	            {
	                $restartdhcpswer = 1;
	            }
		    }
		}
				
		if ($restartdhcpswer == 1 || $netbios_changed ==1)
		{
		    echo "event DHCPS4.RESTART\n";
		}	
	}
}
else if ($ACTION=="classlessstaticroute")
{
	$netid	= ipv4networkid($SDEST, $SSUBNET);
	$cfg = XNODE_getpathbytarget("", "inf", "uid", $INF, 0);
	if ($cfg=="") return $_GLOBALS["INF"]."does not exist!";
	$sts = XNODE_getpathbytarget("/runtime", "inf", "uid", $INF, 1);

	echo "result=`xmldbc -w /runtime/inf:4/inet/ipv4/ipaddr`\n";
	echo 'while [ "$result" == "" ]; do sleep 2; result=`xmldbc -w '.$sts.'/inet/ipv4/ipaddr`; done\n';
	echo "ip route add ".$netid."/".$SSUBNET." via ".$SROUTER." table CLSSTATICROUTE\n";
}
else if ($ACTION=="staticroute")
{
	$netid	= ipv4networkid($SDEST, $SSUBNET);
	$cfg = XNODE_getpathbytarget("", "inf", "uid", $INF, 0);
	if ($cfg=="") return $_GLOBALS["INF"]."does not exist!";
	$sts = XNODE_getpathbytarget("/runtime", "inf", "uid", $INF, 1);

	echo "result=`xmldbc -w /runtime/inf:4/inet/ipv4/ipaddr`\n";
	echo 'while [ "$result" == "" ]; do sleep 2; result=`xmldbc -w '.$sts.'/inet/ipv4/ipaddr`; done\n';
	echo "ip route add ".$SDEST." via ".$SROUTER." table CLSSTATICROUTE\n";
}
else if ($ACTION=="deconfig")
{
	$sts = XNODE_getpathbytarget("/runtime",  "inf", "uid", $INF, 0);
	if ($sts=="") echo 'echo "[$0]: no interface '.$INF.' ..." > /dev/console';
	else
	{
		/* check alias ip */
		$cnt = query($sts."/ipalias/cnt")+0;
		$ip = query($sts."/udhcpc/ip");
		$same_ip = "0";
		if($cnt!="" && $cnt!="0")
		{
			foreach($sts."/ipalias/ipv4/ipaddr")
			{
				if($InDeX > $cnt) break;
				$aip = query($sts."/ipalias/ipv4/ipaddr:".$InDeX);
				/*
				if($aip!="" && $ip==$aip)
				{
					$same_ip = "1";
					$index = $InDeX;
					break;	
				}
				*/
				$amask = "";
				$anetid = ipv4networkid($aip,"24");
				if($anetid == "192.168.0.0") $amask = "24";
				if($amask=="")
				{
					$anetid = ipv4networkid($aip, "16");
					if($anetid == "169.254.0.0") $amask = "16";
				} 
				$netid = ipv4networkid($ip, $amask);
				/* check if subnet is the same */
				if($aip!="" && $netid==$anetid)
				{
					$same_ip = "1";
					$index = $InDeX;
					break;	
				}
			}
		}	

		del($sts."/udhcpc");
		echo "ip route flush table CLSSTATICROUTE\n";
		echo "phpsh /etc/scripts/IPV4.INET.php ACTION=DETACH INF=".$INF;
	}
	/* ip alias */
	if($same_ip == "1")
	{
		//echo "ifconfig ".$INTERFACE.":".$index." ".$ip." up\n";
		echo "ifconfig ".$INTERFACE.":".$index." ".$aip." up\n";
	}
}
else if ($ACTION=="renew")
{
	echo 'echo "[$0]: got renew for '.$INF.' ..." > /dev/console';
}
else if ($ACTION=="dhcpplus")
{
	echo 'echo "[DHCP+]: config '.$INTERFACE.' '.$IP.'/'.$SUBNET.' default gw '.$ROUTER.'" > /dev/console\n';
	/* Get the netmask */
	if ($SUBNET!="") $mask = ipv4mask2int($SUBNET);
	/* Get the broadcast address */
	if ($BROADCAST!="") $brd = $BROADCAST;
	else
	{
		$max = ipv4maxhost($mask);
		$brd = ipv4ip($IP, $mask, $max);
	}
//marco
	echo "echo 0 > /proc/sys/net/ipv4/ip_forward\n";	
	echo "ip addr add ".$IP."/".$mask." broadcast ".$brd." dev ".$INTERFACE."\n";
	
	/* gateway */
	$cfg = XNODE_getpathbytarget("", "inf", "uid", $INF, 0);

	/* Get the defaultroute metric from config. */
	$defrt = query($cfg."/defaultroute");
	if ($ROUTER!="")
	{
		$netid = ipv4networkid($IP, $mask);
		if ($defrt!="" && $defrt>0)
		{	
			echo "ip route add default via ".$ROUTER." metric ".$defrt." table default\n";
		}
		else
		{	
			echo "ip route add ".$netid."/".$mask." dev ".$INTERFACE." src ".$IP." table ".$INF."\n";
		}
	}
}
else
{
	echo '# unknown action - ['.$ACTION.']';
}
?>
exit 0
