<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}
function enable_ipv6($d){fwrite(w, "/proc/sys/net/ipv6/conf/".$d."/disable_ipv6", 0);}

/***********************************************************************/

function get_dns($p)
{
	anchor($p);
	$cnt = query("dns/count")+0;
	foreach ("dns/entry")
	{
		if ($InDeX > $cnt) break;
		if ($dns=="") $dns = $VaLuE;
		else $dns = $dns." ".$VaLuE;
	}
	return $dns;
}

function ipaddr_6to4($v4addr, $hostid)
{
	$a = dec2strf("%02x", cut($v4addr,0,'.'));
	$b = dec2strf("%02x", cut($v4addr,1,'.'));
	$c = dec2strf("%02x", cut($v4addr,2,'.'));
	$d = dec2strf("%02x", cut($v4addr,3,'.'));
	return "2002:".$a.$b.":".$c.$d."::".$hostid;
}

function ipaddr_6rd($prefix, $pfxlen, $v4addr, $v4mask, $hostid)
{
	$sla = ipv4hostid($v4addr, $v4mask);
	/*TRACE_debug("INET: ipaddr_6rd sla: [".$sla."]");*/
	$slalen = 32-$v4mask;
	/*TRACE_debug("INET: ipaddr_6rd slalen: [".$slalen."]");*/
	return ipv6ip($prefix, $pfxlen, $hostid, $sla, $slalen);
}

/***********************************************************************/

function inet_ipv6_ll($inf, $phyinf)
{
	startcmd("# inet_ipv6_ll(".$inf.",".$phyinf.")");

	/* Get the Link Local IP. */
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	if ($p=="") return error("9");

	/* Get device name */
	$devnam = query($p."/name");
	if( substr($inf, 0, 3) == "WAN" )
	{
		enable_ipv6($devnam);
	}

	/* Get the link local address. */
	$ipaddr = query($p."/ipv6/link/ipaddr");
	$prefix = query($p."/ipv6/link/prefix");
	if ($ipaddr=="" || $prefix=="")
	{
		/* maybe ipv6 not ready restart it */
		startcmd("ifconfig br0 down ");
		startcmd("ifconfig br0 up ");
		startcmd('xmldbc -t "inet.'.$inf.':1:service INET.'.$inf.' restart"');
	} else {
		/* Start script */
		startcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
				" MODE=LL".
				" INF=".$inf.
				" DEVNAM=".$devnam.
				" IPADDR=".$ipaddr.
				" PREFIX=".$prefix
				);
	}

	/* Stop script */
	stopcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=DETACH INF=".$inf);
}

/***********************************************************************/

function inet_ipv6_ul($inf, $phyinf)
{
	startcmd("# inet_ipv6_ul(".$inf.",".$phyinf.")");

	/* Get the Link Local IP. */
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	if ($p=="") return error("9");

	/* Get device name */
	$devnam = query($p."/name");
	//fwrite(w, "/proc/sys/net/ipv6/conf/".$devnam."/disable_ipv6", 0);

	/* Get the unique local address. */
	$mac = PHYINF_getphymac($inf);
	$tmac1 = cut($mac, 0, ":");
	$tmac2 = cut($mac, 1, ":");
	$tmac3 = cut($mac, 2, ":");
	$tmac4 = cut($mac, 3, ":");
	$tmac5 = cut($mac, 4, ":");
	$tmac6 = cut($mac, 5, ":");
	$tmac = $tmac1.$tmac2.$tmac3.$tmac4.$tmac5.$tmac6;
	//startcmd("# inet_ipv6_ul(mac is ".$mac.", tmac is ".$tmac.")");

	/* check if static ula prefix */
	$infp   = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$inet   = query($infp."/inet");
	$inetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);

	$ula_prefix = query($inetp."/ipv6/ipaddr");
	$eui64 		= ipv6eui64($mac);

	if($ula_prefix!="")
	{
		$ula_plen 	= query($inetp."/ipv6/prefix");
		$ula_prefix = ipv6networkid($ula_prefix, $ula_plen);
		$ipaddr 	= ipv6ip($ula_prefix, $ula_plen, $eui64, 0, 0);
	}
	else
	{
		$globalid 	= ipv6globalid($tmac);
		$globalid1 	= cut($globalid, 0, ":");
		$globalid2 	= cut($globalid, 1, ":");
		$globalid3 	= cut($globalid, 2, ":");
		$prefix_temp = "fd".$globalid1.":".$globalid2.":".$globalid3."::";
		//startcmd("# inet_ipv6_ul(prefix_temp is ".$prefix_temp.")");
		$ipaddr 	= ipv6ip($prefix_temp, 48, $eui64, 1, 16);
		$ula_plen 	= 64;
	}
	$prefix = $ula_plen;
	startcmd("# inet_ipv6_ul(ULA is ".$ipaddr.")");

	/* save ula prefix to runtime node */
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 1);
	$ula_network = ipv6networkid($ipaddr, $prefix);
	set($stsp."/inet/ipv6/network", $ula_network);

	/* check if we should save ula prefix to db */
	$is_static = query($inetp."/ipv6/staticula");
	if($is_static=="0")
	{
		set($inetp."/ipv6/ipaddr", $ula_network);
		set($inetp."/ipv6/prefix", $prefix);
		startcmd("event DBSAVE");
	}

	/* Start script */
	//startcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
	//	" MODE=UL".
	//	" INF=".$inf.
	//	" DEVNAM=".$devnam.
	//	" IPADDR=".$ipaddr.
	//	" PREFIX=".$prefix
	//	);

	$ipv6enable = fread("e", "/proc/sys/net/ipv6/conf/".$devnam."/disable_ipv6");
	if($ipv6enable=="0")
	{
		/* Start script */
		startcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
			" MODE=UL".
			" INF=".$inf.
			" DEVNAM=".$devnam.
			" IPADDR=".$ipaddr.
			" PREFIX=".$prefix
		);
	}
	else
	{
		/* Generate wait script. */
		$enula = "/var/servd/INET.".$inf."-enula.sh";
		fwrite(w, $enula,
		"#!/bin/sh\n".
		"phpsh /etc/scripts/ENULA.php".
			" INF=".$inf.
			" DEVNAM=".$devnam.
			" IPADDR=".$ipaddr.
			" PREFIX=".$prefix
		);

		/* Start script ... */
		startcmd("chmod +x ".$enula);
		startcmd('xmldbc -t "enula.'.$inf.':5:'.$enula.'"');
	}

	/* Stop script */
	stopcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=DETACH INF=".$inf);
}

/***********************************************************************/

function prepare_6in4_child($stsp, $child, $prefix, $plen, $slaid)
{
	$mac = PHYINF_getphymac($child);
	$hostid = ipv6eui64($mac);

	/* If the prefix is less than 64, the child can use 64 bits prefix length. */
	if ($plen<64)	$slalen = 64-$plen;
	//else			$slalen = 1;
	else			$slalen = 0;

	while($slalen > 32)
	{
		$slalen = $slalen-32;
		$ipaddr = ipv6ip($prefix, $plen, "0", "0", 32);
		$prefix = $ipaddr;
		$plen = $plen+32;
		/*TRACE_debug("INET: 6IN4 Child prepare use ".$prefix."/".$plen);*/
	}

	if($slaid=="")
	$ipaddr = ipv6ip($prefix, $plen, $hostid, "1", $slalen);
	else
		$ipaddr = ipv6ip($prefix, $plen, $hostid, $slaid, $slalen);
	$pfxlen	= $plen + $slalen;

	TRACE_debug("INET: 6IN4 Child [".$child."] use ".$ipaddr."/".$pfxlen);
	set($stsp."/child/uid", $child);
	set($stsp."/child/ipaddr", $ipaddr);
	set($stsp."/child/prefix", $pfxlen);
	if($slaid!="")	set($stsp."/child/slaid", $slaid);
}

function inet_ipv6_6in4($mode, $inf, $infp, $stsp, $inetp)
{
	startcmd("# inet_ipv6_6in4(".$mode.",".$inf."@".$infp."/".$stsp.",".$inetp.")");

	/* Get the IPv4 address of the previous interface. */
	$child = query($infp."/child");
	$prev = query($infp."/infprevious");
	if ($prev!="") $local = INF_getcurripaddr($prev);

	/* Get mtu of the previous interface */
	$previnfp = XNODE_getpathbytarget("","inf","uid",$prev,0);
	$previnet = query($previnfp."/inet");
	$previnetp = XNODE_getpathbytarget("/inet","entry","uid",$previnet,0);
	$prevaddrt = query($previnetp."/addrtype");
	$prevmtu = query($previnetp."/".$prevaddrt."/mtu");

	/* Get INET setting */
	anchor($inetp."/ipv6");
	$mtu = query("mtu");

	if($mtu=="") $mtu=$prevmtu+1-1-20;/* minus ipv4 hdr */

	if ($mode=="6TO4")
	{
		/* convert the 6to4 address */
		$relay	= query("ipv6in4/relay");
		$ipaddr = ipaddr_6to4($local, "1");
		$prefix = 16;
		if ($relay=="")	$gateway = "::192.88.99.1";
		else			$gateway = "::".$relay;

		$slaid	= query("ipv6in4/ipv6to4/slaid");
		/* prepare child setting */
		if ($child!="") prepare_6in4_child($stsp, $child, $ipaddr, 48, $slaid);
	}
	else if ($mode=="6RD")
	{
		/* convert the 6rd address */
		$relay	= query("ipv6in4/relay");
		$pfx	= query("ipv6in4/rd/ipaddr");
		$prefix	= query("ipv6in4/rd/prefix");
		$v4mask	= query("ipv6in4/rd/v4mask");
		$hubspoke = query("ipv6in4/rd/hubspokemode");

		if($pfx=="")
		{
			/* 6rd dhcpv4 option */
			$prevstsp = XNODE_getpathbytarget("/runtime","inf","uid",$prev,0);
			$pfx = query($prevstsp."/udhcpc/sixrd_pfx");
			$prefix = query($prevstsp."/udhcpc/sixrd_pfxlen");
			$v4mask = query($prevstsp."/udhcpc/sixrd_msklen");
			$relay = query($prevstsp."/udhcpc/sixrd_brip");
		}
		
		//check assigned prefix is larger than 64
		//>>>>
		$slalen = 32-$v4mask;
		/*TRACE_debug("INET: ipaddr_6rd slalen: [".$slalen."]");*/
		$total_plen = $prefix+$slalen;
		if($toatl_plen >= 64) $bypasswan=1;
		else                  $bypasswan=0;
		//<<<<

		//$ipaddr = ipaddr_6rd($pfx, $prefix, $local, $v4mask, "1");
		//if ($ipaddr=="") return error("9");

		//>>>>
		if($bypasswan=="0")
		{
			$ipaddr = ipaddr_6rd($pfx, $prefix, $local, $v4mask, "1");
			if ($ipaddr=="") return error("9");
		}
		else
		{
			$ipaddr = "";
		}

		/*TRACE_debug("INET: 6RD ipaddr [".$ipaddr."]");*/
		if ($relay=="")	$gateway = "::192.88.99.1";
		else			$gateway = "::".$relay;

		/* save related info */
		set($stsp."/inet/ipv6/ipv6in4/rd/ipaddr",$pfx);
		set($stsp."/inet/ipv6/ipv6in4/rd/prefix",$prefix);
		set($stsp."/inet/ipv6/ipv6in4/rd/v4mask",$v4mask);
		set($stsp."/inet/ipv6/ipv6in4/rd/hubspokemode",$hubspoke);

		/* prepare child setting */
		if ($child!="")
		{
			prepare_6in4_child($stsp, $child, $ipaddr, $prefix+32-$v4mask, "");

			/* add blackhole routing rule */
			$ipaddrb = ipaddr_6rd($pfx, $prefix, $local, $v4mask, "0");
			$slalen = $prefix+32-$v4mask;
			if($v4mask>0 && $slalen<64)
			{
				startcmd('ip -6 route add blackhole '.$ipaddrb.'/'.$slalen.' dev lo');
				stopcmd('ip -6 route del blackhole '.$ipaddrb.'/'.$slalen.' dev lo');
				stopcmd('xmldbc -X '.$stsp.'/blackhole');
				set($stsp."/blackhole/count","1");
				set($stsp."/blackhole/entry:1/prefix",$ipaddrb);
				set($stsp."/blackhole/entry:1/plen",$slalen);
			}
		}
	}
	else
	{
		$ipaddr = query("ipaddr");
		$prefix = query("prefix");
		$gateway= query("gateway");
		$remote = query("ipv6in4/remote");

		/* prepare child setting */
		if ($child!="") prepare_6in4_child($stsp, $child, $ipaddr, $prefix, "");
	}

	/* Start script ... */
	anchor($inetp."/ipv6");
	startcmd("phpsh /etc/scripts/V6IN4-TUNNEL.php ACTION=CREATE".
		" INF=".$inf." MODE=".$mode.
		" DEVNAM=".	"sit.".$inf.
		" MTU=".$mtu.
		" IPADDR=".	$ipaddr.
		" PREFIX=".	$prefix.
		" GATEWAY=".$gateway.
		" REMOTE=".	$remote.
		" LOCAL=".	$local.
		' "DNS='.get_dns($inetp."/ipv6").'"'
		);

	/* Stop script */
	stopcmd('phpsh /etc/scripts/V6IN4-TUNNEL.php ACTION=DESTROY INF='.$inf);
}

function inet_ipv6_tspc($inf, $infp, $stsp, $inetp)
{
	startcmd('# inet_ipv6_tspc('.$inf.'@'.$infp.'/'.$stsp.','.$inetp.')');

	/* Get INET setting */
	anchor($inetp.'/ipv6/ipv6in4');
	$mtu	= query('mtu');
	$remote	= query('remote');
	$userid	= query('tsp/username');
	$passwd = query('tsp/password');
	$prelen = query('tsp/prefix');

	/* TSPC config */
	$tspc_dir	= '/var/etc';
	$tspc_sh	= 'tspc_helper-'.$inf;
	$callback	= $tspc_dir.'/template/'.$tspc_sh.'.sh';
	$config		= $tspc_dir."/tspc-".$inf.".conf";

	/* the host type option. */
	$hosttype = "host";
	if (query("/runtime/device/layout")=="router")
	{
		$child = query($infp."/child");
		if ($child!="")
		{
			$hosttype = "router";
			set($stsp."/child/uid", $child);
		}
	}

	/* Generate the config file for tspc. */
	fwrite(w, $config,
		"# tspc.conf - Automatically generated for INET.".$inf."\n".
		"tsp_dir=".$tspc_dir."\n".
		"userid=".$userid."\n".
		"passwd=".$passwd."\n".
		"template=".$tspc_sh."\n".
		"server=".$remote."\n".
		"host_type=".$hosttype."\n".
		"prefixlen=".$prelen."\n".
		"if_tunnel_v6v4=sit.".$inf."\n".
		"if_tunnel_v6udpv4=tun.".$inf."\n".
		"auth_method=any\nclient_v4=auto\nretry_delay=30\ntunnel_mode=v6anyv4\nproxy_client=no\n".
		"keepalive=yes\nkeepalive_interval=30\n".

		);

	/* Generate the call back script. */
	fwrite(w, $callback,
		'#!/bin/sh\n'.
		'phpsh /etc/scripts/V6IN4-TUNNEL.php ACTION=CREATE'.
			' INF='.$inf.' MODE=TSP TYPE=$TSP_TUNNEL_MODE'.
			' DEVNAM=$TSP_TUNNEL_INTERFACE'.
			' MTU='.$mtu.
			' IPADDR=$TSP_CLIENT_ADDRESS_IPV6'.
			' PREFIX=$TSP_TUNNEL_PREFIXLEN'.
			' GATEWAY=$TSP_SERVER_ADDRESS_IPV6'.
			' REMOTE=$TSP_SERVER_ADDRESS_IPV4'.
			' "DNS='.get_dns($inetp."/ipv6").'"'.

			' "TSP_HOST_TYPE=$TSP_HOST_TYPE"'.
			' "TSP_SERVER_ADDRESS_IPV4=$TSP_SERVER_ADDRESS_IPV4"'.
			' "TSP_SERVER_ADDRESS_IPV6=$TSP_SERVER_ADDRESS_IPV6"'.
			' "TSP_CLIENT_ADDRESS_IPV4=$TSP_CLIENT_ADDRESS_IPV4"'.
			' "TSP_CLIENT_ADDRESS_IPV6=$TSP_CLIENT_ADDRESS_IPV6"'.
			' "TSP_PREFIX=$TSP_PREFIX"'.
			' "TSP_PREFIXLEN=$TSP_PREFIXLEN"\n'.
		'exit 0\n'
		);


	/* Start script */
	startcmd('chmod +x '.$callback);
	startcmd('tspc -vvv -f '.$config);

	/* Stop script */
	stopcmd('killall tspc');
	stopcmd('rm -f '.$config.' '.$callback);
	stopcmd('phpsh /etc/scripts/V6IN4-TUNNEL.php ACTION=DESTROY INF='.$inf);
}

/***********************************************************************/

function inet_ipv6_static($inf, $devnam, $inetp)
{
	startcmd("# inet_start_ipv6_static(".$inf.",".$devnam.",".$inetp.")");			
	
	if( substr($inf, 0, 3) == "WAN" )
	{
		enable_ipv6($devnam);
	}

	/* if having previous inf */
	$infp   = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$previnf = query($infp."/infprevious");
	if($previnf!="")
	{
		if(isfile("/var/run/".$previnf.".UP")==0)
		{
			TRACE_debug("File /var/run/".$previnf.".UP not existed!");
			return error("9");
		}
	}

	anchor($inetp."/ipv6");
		
	//+++ Jerry Kao, Get the link local address by phyinf.	
	$phyinf = query($infp."/phyinf");	
	
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phyinf, 0);
	if ($p=="") return error("9");
	
	$ipaddr = query($p."/ipv6/link/ipaddr");
	$prefix = query($p."/ipv6/link/prefix");
	if ($ipaddr=="" || $prefix=="")
	{
		/* If IPv6 have not ready, restart it. */
		startcmd('xmldbc -t inet.'.$inf.':1:"service INET.'.$inf.' restart"');
	}
	else 
	{
		/* Start script */
		startcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
			" MODE=STATIC INF=".$inf.
			" DEVNAM=".		$devnam.
			" IPADDR=".		query("ipaddr").
			" PREFIX=".		query("prefix").
			" GATEWAY=".	query("gateway").
			" ROUTERLFT=".	query("routerlft").
			" PREFERLFT=".	query("preferlft").
			" VALIDLFT=".	query("validlft").
			' "DNS='.get_dns($inetp."/ipv6").'"'
			);
	}

	/* Stop script */
	stopcmd("phpsh /etc/scripts/IPV6.INET.php ACTION=DETACH INF=".$inf);
}

/************************************************************/

function inet_ipv6_auto($inf, $infp, $ifname, $phyinf, $stsp, $inetp)
{
	startcmd('# inet_start_ipv6_auto('.$inf.','.$infp.','.$ifname.','.$phyinf.','.$stsp.','.$inetp.')');

	/* Preparing ... */
	/* del it because we do it by rdisc6 */
	//$conf = "/proc/sys/net/ipv6/conf/".$ifname;
	//if (isfile($conf."/ra_mflag")!=1) return error(9);

	//$conf = "/var/run/".$ifname;

	/* Turn off forwarding to enable autoconfig. */
	//fwrite(w, $conf."/forwarding",			"0");
	//fwrite(w, $conf."/autoconf",			"0");/* Don't autoconfigure address, do it by outselves */
	//fwrite(w, $conf."/accept_ra",	"0");/* Don't let kernel add static route according to RA*/
	/* Turn off default route, we will handle the routing table. */
	//fwrite(w, $conf."/accept_ra_defrtr",	"0");
	/* Restart IPv6 function to send RS. */
	//fwrite(w, $conf."/disable_ipv6",		"1");
	//fwrite(w, $conf."/disable_ipv6",		"0");

	/* Record the device name */
	set($stsp."/devnam", $ifname);

	/* Record the infprevious */
	$infprev = query($infp."/infprevious");
	set($stsp."/infprevious", $infprev);

	/* Record the infnext */
	$infnext = query($infp."/infnext");
	set($stsp."/infnext", $infnext);

	/* Record the child uid. */
	if (query("/runtime/device/layout")=="router")
	{
		set($stsp."/child/uid", query($infp."/child"));
		set($stsp."/childgz/uid", query($infp."/childgz"));
	}
	/* Record the pd hint info */
	$pdhint_enable = query($inetp."/ipv6/pdhint/enable");
	if($pdhint_enable=="1")
	{
		$pdhint_network = query($inetp."/ipv6/pdhint/network");
		$pdhint_prefix = query($inetp."/ipv6/pdhint/prefix");
		$pdhint_plft = query($inetp."/ipv6/pdhint/preferlft");
		$pdhint_vlft = query($inetp."/ipv6/pdhint/validlft");

		set($stsp."/pdhint/enable", "1");
		set($stsp."/pdhint/network", $pdhint_network);
		set($stsp."/pdhint/prefix", $pdhint_prefix);
		set($stsp."/pdhint/preferlft", $pdhint_plft);
		if($pdhint_vlft!="")
		{
			set($stsp."/pdhint/validlft", $pdhint_vlft);
		}
	}
	else
	{
		set($stsp."/pdhint/enable", "0");
	}

	/* Generate wait script. */
	/*
	$rawait = "/var/servd/INET.".$inf."-rawait.sh";
	fwrite(w, $rawait,
		"#!/bin/sh\n".
		"phpsh /etc/scripts/RA-WAIT.php".
			" INF=".$inf.
			" PHYINF=".$phyinf.
			" DEVNAM=".$ifname.
			" DHCPOPT=".query($inetp."/ipv6/dhcpopt").
			' "DNS='.get_dns($inetp."/ipv6").'"'.
			" ME=".$rawait.
			"\n");
	*/
	/* need infprev to divide into cable network and broadband network*/
	/* >>>> */
	if($infprev!="")
	{
		$prevstsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $infprev, 0);
		$prevdevnam = query($prevstsp."/devnam");
		$prevphyinf = query($prevstsp."/phyinf");
	}

	if(strstr($prevdevnam,"ppp")=="") /* cable network */
	{
		/* remove pid file */
		//startcmd("rm -f /var/servd/".$inf."-rdisc6.pid");

		/* Generate wait script. */
		$rawait = "/var/servd/INET.".$inf."-rawait.sh";
		fwrite(w, $rawait,
			"#!/bin/sh\n".
			"phpsh /etc/scripts/CABLE-RA-WAIT.php".
				" INF=".$inf.
				" PHYINF=".$phyinf.
				" DEVNAM=".$ifname.
				" DHCPOPT=".query($inetp."/ipv6/dhcpopt").
				' "DNS='.get_dns($inetp."/ipv6").'"'.
				" ME=".$rawait.
				"\n");
	}
	else
	{	/* broadband network */
		/* Generate wait script. */
		$rawait = "/var/servd/INET.".$inf."-rawait.sh";
		fwrite(w, $rawait,
			"#!/bin/sh\n".
			"phpsh /etc/scripts/RA-WAIT.php".
				" INF=".$inf.
				" PHYINF=".$phyinf.
				" DEVNAM=".$ifname.
				" DHCPOPT=".query($inetp."/ipv6/dhcpopt").
				' "DNS='.get_dns($inetp."/ipv6").'"'.
				" ME=".$rawait.
				"\n");
	}
	/* <<<< */

	/* Start script ... */
	startcmd("chmod +x ".$rawait);
	startcmd('xmldbc -t "ra.iptest.'.$inf.':3:'.$rawait.'"');//wait for ipv6 stack stable as device initiating

	/* Stop script ... */
	//stopcmd('echo 1 > /proc/sys/net/ipv6/conf/'.$ifname.'/forwarding');
	//stopcmd('echo 1 > /proc/sys/net/ipv6/conf/'.$ifname.'/autoconf');
	//stopcmd('echo 1 > /proc/sys/net/ipv6/conf/'.$ifname.'/accept_ra');
	//stopcmd('echo 1 > /proc/sys/net/ipv6/conf/'.$ifname.'/accept_ra_defrtr');
	//stopcmd('echo -1 > /proc/sys/net/ipv6/conf/'.$ifname.'/ra_mflag');
	//stopcmd('echo -1 > /proc/sys/net/ipv6/conf/'.$ifname.'/ra_oflag');
	stopcmd('rm -f '.$rawait);
	stopcmd('xmldbc -k ra.iptest.'.$inf);
	stopcmd("/etc/scripts/killpid.sh /var/servd/".$inf."-dhcp6c.pid");
	//if (isfile($conf.".ra_mflag")!=1) stopcmd("rm -f ".$conf.".ra_mflag");
	//if (isfile($conf.".ra_oflag")!=1) stopcmd("rm -f ".$conf.".ra_oflag");
	//if (isfile($conf.".ra_prefix")!=1) stopcmd("rm -f ".$conf.".ra_prefix");
	//if (isfile($conf.".ra_prefix_len")!=1) stopcmd("rm -f ".$conf.".ra_prefix_len");
	//if (isfile($conf.".ra_saddr")!=1) stopcmd("rm -f ".$conf.".ra_saddr");
	$conf = "/var/run/".$ifname;
	if($infprev!="")
	{
		$prevnam = PHYINF_getruntimeifname($infprev);
		$conf = "/var/run/".$prevnam;
	}
	stopcmd("rm -f ".$conf.".ra_mflag");
	stopcmd("rm -f ".$conf.".ra_oflag");
	stopcmd("rm -f ".$conf.".ra_prefix");
	stopcmd("rm -f ".$conf.".ra_prefix_len");
	stopcmd("rm -f ".$conf.".ra_saddr");
	stopcmd("rm -f ".$conf.".ra_rdnss");
	stopcmd("rm -f ".$conf.".ra_dnssl");
	stopcmd("rm -f /var/run/wan_ralft_zero");
	stopcmd("killall rdisc6");
	stopcmd("/etc/scripts/killpid.sh /var/servd/".$inf."-rdisc6.pid");
	stopcmd("rm -f /var/servd/".$inf."-rdisc6.pid");
	stopcmd('phpsh /etc/scripts/IPV6.INET.php ACTION=DETACH INF='.$inf);
}

/************************************************************/

function inet_ipv6_pppdhcp($inf, $infp, $ifname, $phyinf, $stsp, $inetp, $infprev)
{
	startcmd('# inet_start_ipv6_pppdhcp('.$inf.','.$infp.','.$ifname.','.$phyinf.','.$stsp.','.$inetp.','.$infprev.')');

	$hlp = "/var/servd/".$inf."-dhcp6c.sh";
	$pid = "/var/servd/".$inf."-dhcp6c.pid";
	$cfg = "/var/servd/".$inf."-dhcp6c.cfg";

	/* DHCP over PPP session */
	if ($infprev!="")
	{
		$pppdev = PHYINF_getruntimeifname($infprev);
		if ($pppdev=="")
		{
			TRACE_debug("INET: PPPDHCP - no PPP device");
			return error("9");
		}
	}

	/* Record the device name */
	set($stsp."/devnam", $ifname);
	/* Record the child uid. */
	if (query("/runtime/device/layout")=="router")
		set($stsp."/child/uid", query($infp."/child"));

	/* Generate configuration file. */
	$opt = query($inetp."/ipv6/dhcpopt");
	TRACE_debug("INET: PPPDHCP - dhcpopt: ".$opt);
	if (strstr($opt,"IA-NA")!="") {$send=$send."\tsend ia-na 0;\n"; $idas=$idas."id-assoc na {\n};\n";}
	if (strstr($opt,"IA-PD")!="") {$send=$send."\tsend ia-pd 0;\n"; $idas=$idas."id-assoc pd {\n};\n";}
	fwrite(w, $cfg,
		"interface ".$pppdev." {\n".
		$send.
		"\trequest domain-name-servers;\n".
		"\trequest domain-name;\n".
		"\tscript \"".$hlp."\";\n".
		"};\n".
		$idas);

	/* generate callback script */
	fwrite(w, $hlp,
		"#!/bin/sh\n".
		"echo [$0]: [$new_addr] [$new_pd_prefix] [$new_pd_plen] > /dev/console\n".
		"phpsh /etc/services/INET/inet6_dhcpc_helper.php".
			" INF=".$inf.
			" MODE=PPPDHCP".
			" DEVNAM=".$pppdev.
			" GATEWAY=".$router.
			" DHCPOPT=".$opt.
			' "NAMESERVERS=$new_domain_name_servers"'.
			' "NEW_ADDR=$new_addr"'.
			' "NEW_PD_PREFIX=$new_pd_prefix"'.
			' "NEW_PD_PLEN=$new_pd_plen"'.
			' "DNS='.$dns.'"'.
			' "NEW_AFTR_NAME=$new_aftr_name"'.
			' "NTPSERVER=$new_ntp_servers"'.
			"\n");

	/* Start DHCP client */
	startcmd("chmod +x ".$hlp);
	startcmd("dhcp6c -c ".$cfg." -p ".$pid." -t LL -o ".$ifname." ".$pppdev);

	/* Stop script ... */
	stopcmd("/etc/scripts/killpid.sh /var/servd/".$inf."-dhcp6c.pid");
	stopcmd('phpsh debug /etc/scripts/IPV6.INET.php ACTION=DETACH INF='.$inf);
}

/************************************************************/

function inet_ipv6_autodetect($inf, $infp, $ifname, $phyinf, $stsp, $inetp)
{
	TRACE_debug("== IPv6 Auto Detection ==");
	
	startcmd('# inet_start_ipv6_autodetect('.$inf.','.$infp.','.$ifname.','.$phyinf.','.$stsp.','.$inetp.')');

	$v4actuid = query($inetp."/ipv6/detectuid/v4actuid");
	$v6lluid  = query($inetp."/ipv6/detectuid/v6lluid");
	$v6actuid = query($inetp."/ipv6/detectuid/v6actuid");
	$pdns	  = query($inetp."/ipv6/dns/entry:1");
	$sdns	  = query($inetp."/ipv6/dns/entry:2");
	$dnscnt	  = query($inetp."/ipv6/dns/count");
	$child 	  = query($infp."/child");
	$next     = query($infp."/infnext");
	startcmd('# next('.$next.')');

	$v6actinfp   = XNODE_getpathbytarget("", "inf", "uid", $v6actuid, 0);
	$isactive 	 = query($v6actinfp."/active");
	if($isactive=="0")
	{
		set($v6actinfp."/active", "1");
	}
	$v6actinet   = query($v6actinfp."/inet");
	$v6actinetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $v6actinet, 0);

	/* check if WANv4 is ppp or not */
	$v4infp   = XNODE_getpathbytarget("", "inf", "uid", $v4actuid, 0);
	$v4inet   = query($v4infp."/inet");
	$v4inetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $v4inet, 0);
	$v4mode   = query($v4inetp."/addrtype");

	if($v4mode=="ppp4") $over = query($v4inetp."/ppp4/over");
	if($v4mode=="ppp4" && $over=="eth") $V4isPPP = 1;

	if($V4isPPP==1 || $v4mode=="ppp10")								// IPv4 is PPPoE.
	{
		startcmd('echo IPv4 is PPPoE Mode > /dev/console');

		$username = query($v4inetp."/ppp4/username");
		$password = query($v4inetp."/ppp4/password");
		$dialupmode= query($v4inetp."/ppp4/dialup/mode");
		$dialupmode= "auto"; //force

		$llinf = $v6lluid;
		$llinfp = XNODE_getpathbytarget("", "inf", "uid", $llinf, 0);

		/* Detect PPPOE server */
		//startcmd('sh /etc/events/WANV6_ppp_dis.sh '.$v4actuid.' START');	// WANV6_ppp_dis.sh
		startcmd('sh /etc/events/WANV6_ppp_dis.sh '.$inf.' START '.$v4actuid);	// WANV6_ppp_dis.sh
		
		$pppautodetsh   = "/var/run/".$inf."-autodetect.sh";				
				
		fwrite(w, $pppautodetsh, "#!/bin/sh\n");
		fwrite(a, $pppautodetsh,
			'result=`xmldbc -w /runtime/services/wandetect6/wantype`\n'.
			'desc=`xmldbc -w /runtime/services/wandetect6/desc`\n'.
			'echo pppoedetect result is $result > /dev/console\n'.
			'if [ "$result" == "PPPoE" ]; then\n'.							// if IPv6 $result" == "PPPoE"
			'	echo AUTODETECT change to ppp10 mode > /dev/console\n'.
			'	echo Setup 6 - PPPoE and PPPoEv6 > /dev/console\n'.
			'	xmldbc -s '.$v4infp.'/infprevious "'.$inf.'"\n'.
			'	xmldbc -s '.$v4infp.'/child "'.$llinf.'"\n'.
			'	xmldbc -s '.$v4inetp.'/addrtype "ppp10"\n'.
			'	xmldbc -s '.$v4inetp.'/ppp6/username "'.$username.'"\n'.
			'	xmldbc -s '.$v4inetp.'/ppp6/password "'.$password.'"\n'.
			'	xmldbc -s '.$v4inetp.'/ppp4/dialup/mode "'.$dialupmode.'"\n'.
			'	xmldbc -s '.$v4inetp.'/ppp6/dialup/mode "'.$dialupmode.'"\n'.
			'	xmldbc -s '.$v4inetp.'/ppp6/over "eth"\n'.
			'	xmldbc -s '.$llinfp.'/infnext "'.$v6actuid.'"\n'.
			'	xmldbc -s '.$llinfp.'/inet ""\n'.
			'	xmldbc -s '.$v6actinfp.'/infprevious "'.$llinf.'"\n'.
			'	xmldbc -s '.$v6actinfp.'/infnext "'.$next.'"\n'.
			'	xmldbc -s '.$v6actinfp.'/defaultroute "0"\n'.
			'	xmldbc -s '.$v6actinfp.'/active "1"\n'.
			'	xmldbc -s '.$v6actinfp.'/child "'.$child.'"\n'.
			'	xmldbc -s '.$v6actinetp.'/ipv6/mode "AUTO"\n'.
			'	xmldbc -s '.$v6actinetp.'/ipv6/dhcpopt "IA-NA+IA-PD"\n'.
			'	xmldbc -s '.$v6actinetp.'/ipv6/dns/entry:1 "'.$pdns.'"\n'.
			'	xmldbc -s '.$v6actinetp.'/ipv6/dns/entry:2 "'.$sdns.'"\n'.
			'	xmldbc -s '.$v6actinetp.'/ipv6/dns/count "'.$dnscnt.'"\n'.
			'	echo service INET.'.$v4actuid.' restart      > /var/servd/INET.'.$inf.'_start.sh\n'.
			'	echo event DBSAVE                           >> /var/servd/INET.'.$inf.'_start.sh\n'.
			'	echo service INET.'.$v4actuid.' stop         > /var/servd/INET.'.$inf.'_stop.sh\n'.
			'	echo xmldbc -X /runtime/services/wandetect6 >> /var/servd/INET.'.$inf.'_stop.sh\n'.
			'	service INET.'.$v4actuid.' restart\n'.
			//'else\n'.
			//'	echo IPv6CP is not successful or not receive DHCP-PD !! > /dev/console\n'.
			'fi\n'.
			'if [ "$desc" == "No RA DHCP6" ]; then\n'.
			'	echo Not receive RA or DHCP-PD !! > /dev/console\n'.
			//'	echo Setup 5 - PPPoE and Autoconfiguration > /dev/console\n'.						
			'	sh /etc/events/WANV6_PPP_AUTOCONF_DETECT.sh '.$inf.' NO_DHCP6_START\n'.		/* added by Jerry Kao. */	
			'fi\n'
		);
		
		startcmd('chmod +x '.$pppautodetsh);
		startcmd('xmldbc -t pppdetect.'.$inf.':80:'.$pppautodetsh);
	}
	else		// Not PPP4 or PPP10 (IPv4 is not PPPoE.)
	{
		startcmd('echo IPv4 is not PPPoE Mode > /dev/console');

		$v4stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $v4actuid, 0);

		startcmd('sh /etc/events/WANV6_AUTOCONF_DETECT.sh '.$inf.' START');	// It will set /runtime/services/wandetect6/wantype, by Jerry Kao.

		$autodetsh   = "/var/run/".$inf."-autodetect.sh";
		
		fwrite(w, $autodetsh, "#!/bin/sh\n");
		fwrite(a, $autodetsh,
			'result=`xmldbc -w /runtime/services/wandetect6/wantype`\n'.

			'echo First check if we receive RA or not > /dev/console\n'.
			'echo result is $result > /dev/console\n'.
		
			'if [ "$result" != "unknown" ]; then\n'.					// That is Received RA.
			'	echo RA detected!! > /dev/console\n'.
			'	ipaddr=`xmldbc -w '.$v4stsp.'/inet/ipv4/ipaddr`\n'.		
			'	if [ $ipaddr ]; then\n'.										// if get $ipaddr of IPv4.
			'		echo Have IPv4 address !! > /dev/console\n'.
			'		sh /etc/events/WANV6_AUTOCONF_DETECT.sh '.$inf.' DHCP6START\n'.		// $ACT = DHCP6START
			'		echo "#!/bin/sh"											 									 > /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "result1=`xmldbc -w /runtime/services/wandetect6/wantype`" 								>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "echo result is \\"$result1\\" > /dev/console" 											>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "if [ \\"$result1\\" != \\"unknown\\" ]; then" 											>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo AUTODETECT change to AUTO mode > /dev/console"									>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo \[ Setup 2 - Native IPv4 and Autoconfiguration \]	> /dev/console"				>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		xmldbc -s '.$v6actinfp.'/infprevious \"'.$inf.'\""									>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		xmldbc -s '.$v6actinfp.'/child \"'.$child.'\""										>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		xmldbc -s '.$v6actinetp.'/ipv6/mode \"AUTO\""										>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "      if [ '.$pdns.' ]; then"																>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "			xmldbc -s '.$v6actinetp.'/ipv6/dns/entry:1 \"'.$pdns.'\""						>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "			xmldbc -s '.$v6actinetp.'/ipv6/dns/count \"'.$dnscnt.'\""						>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		fi"												 									>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "      if [ '.$sdns.' ]; then"																>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "			xmldbc -s '.$v6actinetp.'/ipv6/dns/entry:1 \"'.$sdns.'\""						>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		fi"																					>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo service INET.'.$v6actuid.' restart      > /var/servd/INET.'.$inf.'_start.sh"	>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo event DBSAVE                           >> /var/servd/INET.'.$inf.'_start.sh"	>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo service INET.'.$v6actuid.' stop         > /var/servd/INET.'.$inf.'_stop.sh"	>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo xmldbc -X /runtime/services/wandetect6 >> /var/servd/INET.'.$inf.'_stop.sh"	>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo rm -f /var/run/'.$inf.'.UP >> /var/servd/INET.'.$inf.'_stop.sh"	>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		service INET.'.$v6actuid.' restart"													>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "else"									 													>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		echo Not receive DHCP-PD, for Setup 1 > /dev/console"								>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "		sh /etc/events/WANV6_6RD_DETECT.sh '.$inf.' '.$v4actuid.' '.$v6actuid.' 1"			>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		echo "fi" 																						>> /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		chmod +x /var/run/'.$inf.'-dhcp6det.sh\n'.
			'		xmldbc -t dhcp6det.'.$inf.':40:/var/run/'.$inf.'-dhcp6det.sh\n'.			
			'	else\n'.			
					//Setup 3 & Setup 4 - DS-Lite
			'		echo Have no IPv4 address !! > /dev/console\n'.
			'		echo Send DHCPv6 Solicit with DS-Lite option > /dev/console\n'.
			'		sh /etc/events/WANV6_AUTOCONF_DETECT.sh '.$inf.' DHCP6START\n'.		// $ACT = DHCP6START
			'		echo "#!/bin/sh"											 									 > /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "result2=`xmldbc -w /runtime/services/wandetect6/wantype`"	 									>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "echo result is \\"$result2\\" > /dev/console" 												>> /var/run/'.$inf.'-dslitedet.sh\n'.		
			'		echo "if [ \\"$result2\\" != \\"unknown\\" ]; then" 												>> /var/run/'.$inf.'-dslitedet.sh\n'.		
			'		echo "		echo Receive DHCP-PD - Check DS-Lite option > /dev/console"							>> /var/run/'.$inf.'-dslitedet.sh\n'.			
								/* If result is "unknown", restart INET.WAN-5 */
			'		echo "		wizard=`xmldbc -w /runtime/services/wizard6`"								>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "		if [ \\"$wizard\\" != \\"1\\" ]; then" 												>> /var/run/'.$inf.'-dslitedet.sh\n'.									
			'		echo "			sh /etc/events/WANV6_DSLITE_DETECT.sh '.$inf.' '.$v4actuid.' '.$v6actuid.' 1"		>> /var/run/'.$inf.'-dslitedet.sh\n'.					
			'		echo "		else"									 											>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "			sh /etc/events/WANV6_DSLITE_DETECT.sh '.$inf.' '.$v4actuid.' '.$v6actuid.' 0"	>> /var/run/'.$inf.'-dslitedet.sh\n'.					
			'		echo "		fi"																					>> /var/run/'.$inf.'-dslitedet.sh\n'.					
			'		echo "else"									 													>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "		echo Not receive DHCP-PD - detection failed !! > /dev/console"						>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "		xmldbc -s /runtime/services/wandetect6/wantype \"unknown\""							>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "		xmldbc -s /runtime/services/wandetect6/desc	  \"No Response\""						>> /var/run/'.$inf.'-dslitedet.sh\n'.
			'		echo "fi"																						>> /var/run/'.$inf.'-dslitedet.sh\n'.		
			'		chmod +x /var/run/'.$inf.'-dslitedet.sh\n'.
			'		xmldbc -t dslitedet.'.$inf.':60:/var/run/'.$inf.'-dslitedet.sh\n'.
					
			'	fi\n'.				
			'else\n'.
			// Get RA after 10s = No (in Cable network).
			'	echo Cannot detect RA for 10 seconds, for Setup 1\n'.
			'	sh /etc/events/WANV6_6RD_DETECT.sh '.$inf.' '.$v4actuid.' '.$v6actuid.' 1\n'.		
			'fi\n'		
		);

		startcmd('chmod +x '.$autodetsh);
		startcmd('xmldbc -t autodetect.'.$inf.':30:'.$autodetsh);
	}
	//startcmd('event DBSAVE');
	stopcmd('rm -f /var/run/'.$inf.'.UP');
	stopcmd('xmldbc -X /runtime/services/wandetect6/wantype');
}

/* IPv6 *********************************************************/
fwrite(a,$START, "# INFNAME = [".$INET_INFNAME."]\n");
fwrite(a,$STOP,  "# INFNAME = [".$INET_INFNAME."]\n");

/* These parameter should be valid. */
$inf    = $INET_INFNAME;
$infp   = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
$phyinf = query($infp."/phyinf");
$default= query($infp."/defaultroute");
$inet   = query($infp."/inet");
$inetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
$ifname = PHYINF_getifname($phyinf);
$infprev = query($infp."/infprevious");
$infnext = query($infp."/infnext");

/* Create the runtime inf. Set phyinf. */
$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 1);
set($stsp."/phyinf", $phyinf);
set($stsp."/defaultroute", $default);

$mode = query($inetp."/ipv6/mode");

if ($mode=="STATIC")	inet_ipv6_static($inf, $ifname, $inetp);
else if	($mode=="LL")	inet_ipv6_ll($inf, $phyinf);
else if	($mode=="UL")	inet_ipv6_ul($inf, $phyinf);
else if	($mode=="AUTO")	inet_ipv6_auto($inf, $infp, $ifname, $phyinf, $stsp, $inetp);
else if	($mode=="PPPDHCP")	inet_ipv6_pppdhcp($inf, $infp, $ifname, $phyinf, $stsp, $inetp, $infprev);
else if	($mode=="TSP")	inet_ipv6_tspc($inf, $infp, $stsp, $inetp);
else if	($mode=="6IN4"
	||	 $mode=="6TO4"
	||	 $mode=="6RD")	inet_ipv6_6in4($mode, $inf, $infp, $stsp, $inetp);
else if	($mode=="AUTODETECT")	inet_ipv6_autodetect($inf, $infp, $ifname, $phyinf, $stsp, $inetp);
?>
