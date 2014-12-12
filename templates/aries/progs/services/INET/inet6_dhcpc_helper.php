#!/bin/sh
<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function cmd($cmd) {echo $cmd."\n";}
function msg($msg) {cmd("echo [DHCP6C]: ".$msg." > /dev/console");}

/*****************************************/
function add_each($list, $path, $node)
{
	//echo "# add_each(".$list.",".$path.",".$node.")\n";
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
/*****************************************/

function handle_stateful($inf, $devnam, $opt)
{
	//$stsp = XNODE_getpathbytarget("/runtime",  "inf", "uid", $inf, 0);
	$stsp = XNODE_getpathbytarget("/runtime",  "inf", "uid", $inf, 1);//for wizard detection
	if ($stsp=="" || $devnam=="") return;

	/* Preparing ... */
	//$conf	= "/proc/sys/net/ipv6/conf/".$devnam;
	$conf	= "/var/run/".$devnam;
	$child	= query($stsp."/child/uid");
	$childgz	= query($stsp."/childgz/uid");

	/* Strip the tailing spaces. */
	$DNS			= strip($_GLOBALS["DNS"]);
	$NAMESERVERS	= strip($_GLOBALS["NAMESERVERS"]);
	$DOMAIN			= strip($_GLOBALS["DOMAIN"]);
	$NEW_ADDR		= strip($_GLOBALS["NEW_ADDR"]);
	$NEW_PD_PREFIX	= strip($_GLOBALS["NEW_PD_PREFIX"]);
	$NEW_PD_PLEN	= strip($_GLOBALS["NEW_PD_PLEN"]);
	$NEW_AFTR_NAME	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	$NEW_PD_PLFT	= strip($_GLOBALS["NEW_PD_PLTIME"]);
	$NEW_PD_VLFT	= strip($_GLOBALS["NEW_PD_VLTIME"]);

	if($DOMAIN!="")
	{
		XNODE_set_var($child."_DOMAIN", $DOMAIN);
		XNODE_set_var($childgz."_DOMAIN", $DOMAIN);
	}
	XNODE_set_var($child."_PDPLFT", $NEW_PD_PLFT);
	XNODE_set_var($child."_PDVLFT", $NEW_PD_VLFT);

	XNODE_set_var($childgz."_PDPLFT", $NEW_PD_PLFT);
	XNODE_set_var($childgz."_PDVLFT", $NEW_PD_VLFT);

	/* Combine the user config and DHCP server setting. */
	$dns = $DNS;

	/* get primary dns and secondary dns */
	$pdns = '';
	$sdns = '';
	$dns_cnt = scut_count($dns, "");
	if($dns_cnt!=0)
	{
		$pdns = scut($dns, 0, "");
		$sdns = scut($dns, 1, "");
	}
	$nameserver = ''; 
	if($NAMESERVERS!="")
	{
		$i=0;
		$cnt = scut_count($NAMESERVERS, "");
		while($i<$cnt)
		{
			$val = scut($NAMESERVERS, $i, "");
			if($val==$pdns || $val==$sdns)
			{
				msg("Got the same dns server as dns manual setting");
			}
			else
			{
				if($nameserver=="") $nameserver = $val;
				else				$nameserver = $nameserver." ".$val; 
			}
			$i++;
		}
		$NAMESERVERS = $nameserver;
	}
	 
    
	$oflag = fread("e", $conf.".ra_oflag");
	//if($oflag=="1") // for M flag = 1, O flag = 0 case
	//{
	if ($NAMESERVERS!="")
	{
		if ($dns=="")	$dns = $NAMESERVERS;
		else			$dns = $dns." ".$NAMESERVERS;
	}
	//}

	/* Get RA info. */
	if ($_GLOBALS["MODE"]!="PPPDHCP")
	{
		$prefix	= fread("e", $conf.".ra_prefix");
		$pfxlen	= fread("e", $conf.".ra_prefix_len");
		$router	= fread("e", $conf.".ra_saddr");
		//if ($router=="" || $router=="NULL") {msg("no ra_saddr"); return;}/*TR-124 issue 2 */
		//if ($prefix=="" || $prefix=="NULL") {msg("no ra_prefix"); return;}
		//if ($pfxlen==0) {msg("no ra_prefix_len"); return;}
	}

	/* Check renew, do nothing if we got the same prefix and prefix len as previous ones */
	$ip = query($stsp."/inet/ipv6/ipaddr");
	$pfl = query($stsp."/inet/ipv6/prefix");

	//IOL if already do renew IA-PD
	if($ip=="")
	{
		$ip = query($stsp."/oldinet/ipv6/ipaddr");
		$pfl = query($stsp."/oldinet/ipv6/prefix");
	}
	
	$childip = query($stsp."/child/ipaddr");
	$childpfl = query($stsp."/child/prefix");	
	if($opt=="IA-NA+IA-PD") 
	{
		if ($NEW_ADDR!="" && $NEW_PD_PREFIX=="" && $NEW_PD_PLEN== "") { $renew=1; $renewip=1; msg("STATEFUL - opt: ".$opt." RENEW IANA...");}
		if ($NEW_ADDR=="" && $NEW_PD_PREFIX!="" && $NEW_PD_PLEN!= "") { $renew=1; $renewip=0; msg("STATEFUL - opt: ".$opt." RENEW IAPD...");}
	}

	if($opt=="IA-NA") 
	{
		if ($NEW_ADDR!="" && $NEW_PD_PREFIX=="" && $NEW_PD_PLEN== "") { $renew=1;msg("STATEFUL - opt: ".$opt." RENEW IANA...");}
	}

	if ($opt=="IA-PD")
	{
		if ($child!="")
		{
			$mac	= PHYINF_getphymac($child);
			$hostid	= ipv6eui64($mac);
			if($NEW_PD_PLEN < 64)
			{
				$slalen = 64-$NEW_PD_PLEN;
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 1, $slalen);
				$cpfxlen = 64;
			}
			else
			{
				$slalen = 1;
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 1, 1);
				$cpfxlen = $NEW_PD_PLEN+1;
			}
			if($childip == $ipaddr && $childpfl == $NEW_PD_PLEN+1) {msg("STATEFUL - Renew but do nothing"); return;}

			msg("PREFIX=".$NEW_PD_PREFIX.", PLEN=".$NEW_PD_PLEN.", HOSTID=".$hostid.", SLA=1, SLALEN=".$slalen);
			msg("IPADDR=".$ipaddr);

			set($stsp."/child/ipaddr", $ipaddr);
			set($stsp."/child/prefix", $cpfxlen);
			set($stsp."/child/pdnetwork", $NEW_PD_PREFIX);
			set($stsp."/child/pdprefix", $NEW_PD_PLEN);
			set($stsp."/child/pdplft", $NEW_PD_PLFT);
			set($stsp."/child/pdvlft", $NEW_PD_VLFT);
		}
		if ($childgz!="")
		{
			$mac	= PHYINF_getphymac($childgz);
			$hostid	= ipv6eui64($mac);
			if($NEW_PD_PLEN < 64)
			{
				$slalen = 64-$NEW_PD_PLEN;
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 0, $slalen);
				$cpfxlen = 64;
			}
			else
			{
				$slalen = 1;
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 2, 1);
				$cpfxlen = $NEW_PD_PLEN+1;
			}
			if($childip == $ipaddr && $childpfl == $NEW_PD_PLEN+1) {msg("STATEFUL - Renew but do nothing"); return;}

			msg("PREFIX=".$NEW_PD_PREFIX.", PLEN=".$NEW_PD_PLEN.", HOSTID=".$hostid.", SLA=1, SLALEN=".$slalen);
			msg("IPADDR=".$ipaddr);

			set($stsp."/childgz/ipaddr", $ipaddr);
			set($stsp."/childgz/prefix", $cpfxlen);
			set($stsp."/childgz/pdnetwork", $NEW_PD_PREFIX);
			set($stsp."/childgz/pdprefix", $NEW_PD_PLEN);
			set($stsp."/childgz/pdplft", $NEW_PD_PLFT);
			set($stsp."/childgz/pdvlft", $NEW_PD_VLFT);
		}
		$ipaddr = ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, 1, 0, 0);
		$pfxlen = $NEW_PD_PLEN;
		if($ip == $ipaddr && $pfl == $NEW_PD_PLEN) 
		{
			msg("STATEFUL - opt: ".$opt." Renew but do nothing"); 
			set($stsp."/inet/ipv6/ipaddr", $ip);
			set($stsp."/inet/ipv6/prefix", $pfl);
			return;
		}
	}
	else
	{
		if ($child!="" && strstr($opt,"IA-PD")!="")
		{
			$mac	= PHYINF_getphymac($child);
			$hostid	= ipv6eui64($mac);
			if($NEW_PD_PLEN <= 64)//IOL
			{
				msg("STATEFUL - Record ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." to blackhole"); 
				/* handle blackhole issue */
				//cmd("ip -6 route add blackhole ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." dev lo");
				//set($stsp."/blackhole/prefix", $NEW_PD_PREFIX);
				//set($stsp."/blackhole/plen", $NEW_PD_PLEN);

				$holecnt = query($stsp."/blackhole/count");
				if($holecnt=="")
				{
					$holecnt = 1;
					set($stsp."/blackhole/count", $holecnt);
					set($stsp."/blackhole/entry:".$holecnt."/prefix", $NEW_PD_PREFIX);
					set($stsp."/blackhole/entry:".$holecnt."/plen", $NEW_PD_PLEN);
					if($NEW_PD_PLEN!=64)
					{
						cmd("ip -6 route add blackhole ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." dev lo");
					}
				}

				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 1,64-$NEW_PD_PLEN);
				$cpfxlen = 64;
			}
			else
			{
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 0,0);
				$cpfxlen = $NEW_PD_PLEN;
			}

			if($renew==1 && $childip==$ipaddr && $childpfl==$cpfxlen){msg("STATEFUL - opt: ".$opt." Renew but do nothing"); return;}
			
			if($renew==1 && $NEW_PD_PREFIX!="" && $NEW_PD_PLEN!="")
			{
				//IOL test, add blackhoel to ip6tables table FORWARD
				if($NEW_PD_PLEN <= 64)
				{
					$OLD_PD_PREFIX = query($stsp."/blackhole/entry:".$holecnt."/prefix");
					$OLD_PD_PLEN = query($stsp."/blackhole/entry:".$holecnt."/plen");
				}
				msg("STATEFUL - Renew and record ".$OLD_PD_PREFIX."/".$OLD_PD_PLEN." to oldblackhole"); 
				set($stsp."/oldblackhole/entry:1/prefix", $OLD_PD_PREFIX);
				set($stsp."/oldblackhole/entry:1/plen", $OLD_PD_PLEN);
				if($OLD_PD_PREFIX!=64)
				{
					cmd("ip -6 route del blackhole ".$OLD_PD_PREFIX."/".$OLD_PD_PLEN." dev lo");
				}
				cmd("ip -6 route add ".$OLD_PD_PREFIX."/".$OLD_PD_PLEN." dev br0");
	
				//renew and get the different prefix
					//$holecnt = $holecnt + 1;
					//set($stsp."/blackhole/entry:".$holecnt."/prefix", $NEW_PD_PREFIX);
					//set($stsp."/blackhole/entry:".$holecnt."/plen", $NEW_PD_PLEN);
					//set($stsp."/blackhole/count", $holecnt);
					//cmd("ip -6 route add blackhole ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." dev lo");
				if($NEW_PD_PLEN <= 64)//IOL
				{
					msg("STATEFUL - Add new prefix ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." to blackhole"); 
					$holecnt = $holecnt + 1;
					set($stsp."/blackhole/entry:".$holecnt."/prefix", $NEW_PD_PREFIX);
					set($stsp."/blackhole/entry:".$holecnt."/plen", $NEW_PD_PLEN);
					set($stsp."/blackhole/count", $holecnt);
					if($NEW_PD_PLEN != 64)
					{
						cmd("ip -6 route add blackhole ".$NEW_PD_PREFIX."/".$NEW_PD_PLEN." dev lo");
					}
				}
			}

			if($ipaddr!="")/* check if renew IANA, $NEW_PD_PREFIX and $NEW_PD_PLEN are zero */
			{
				set($stsp."/child/ipaddr", $ipaddr);
				set($stsp."/child/prefix", $cpfxlen);
				set($stsp."/child/pdnetwork", $NEW_PD_PREFIX);
				set($stsp."/child/pdprefix", $NEW_PD_PLEN);
				set($stsp."/child/pdplft", $NEW_PD_PLFT);
				set($stsp."/child/pdvlft", $NEW_PD_VLFT);
			}
		}

		if ($childgz!="" && strstr($opt,"IA-PD")!="")
		{
			$mac	= PHYINF_getphymac($childgz);
			$hostid	= ipv6eui64($mac);
			if($NEW_PD_PLEN<64)
			{
				$ipaddr	= ipv6ip($NEW_PD_PREFIX, $NEW_PD_PLEN, $hostid, 0,64-$NEW_PD_PLEN);
				$cpfxlen = 64;
			}
			else
			{
				//If pd length is bigger than 63, don't set to guest zone
				$ipaddr = "";
				$cpfxlen = "";
			}
		
			if($ipaddr!="")/* check if renew IANA, $NEW_PD_PREFIX and $NEW_PD_PLEN are zero */
			{
				set($stsp."/childgz/ipaddr", $ipaddr);
				set($stsp."/childgz/prefix", $cpfxlen);
				set($stsp."/childgz/pdnetwork", $NEW_PD_PREFIX);
				set($stsp."/childgz/pdprefix", $NEW_PD_PLEN);
				set($stsp."/childgz/pdplft", $NEW_PD_PLFT);
				set($stsp."/childgz/pdvlft", $NEW_PD_VLFT);
			}
		}

		if (strstr($opt,"IA-NA")=="") {msg("no IA-NA"); return;}

		if($renew==1 && $renewip==1 && $ip==$NEW_ADDR)
		{
			msg("STATEFUL - opt: ".$opt." Renew but do nothing"); 
			set($stsp."/inet/ipv6/ipaddr", $ip);
			set($stsp."/inet/ipv6/prefix", $pfl);
			return;
		}

		if ($NEW_ADDR=="" && $renewip==1) {msg("no NEW_ADDR"); return;}
		$ipaddr = $NEW_ADDR;
		$pfxlen = 128;
	}

	set($stsp."/inet/ipv6/dhcpopt", $opt);

	/* DS-Lite info */
	$remote	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	if($remote!="")
		set($stsp."/inet/ipv4/ipv4in6/remote", $remote);

	/* domain info */
	if($DOMAIN!="")
	{
		set($stsp."/inet/ipv6/domain",$DOMAIN);
	}
		
	//IOL test, backup
	if($ipaddr!="")
	{ 
		set($stsp."/oldinet/ipv6/ipaddr", $ipaddr);
		set($stsp."/oldinet/ipv6/prefix", $pfxlen);
	}

	cmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
		" INF=".$_GLOBALS["INF"].
		" MODE=".$_GLOBALS["MODE"].
		" DEVNAM=".$devnam.
		" IPADDR=".$ipaddr.
		" PREFIX=".$pfxlen.
		" GATEWAY=".$router.
		' "DNS='.$dns.'"');

	return;
}

function handle_stateless($inf, $prefix, $pfxlen)
{
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);
	if ($stsp=="") {msg("STATLESS - no runtime for ".$inf); return;}
	if ($prefix=="" || $pfxlen=="")	{msg("STATELESS - no prefix"); return;}

	/* Prepare child. */
	msg("STATELESS - Got prefix ".$prefix."/".$pfxlen);
	$NEW_PD_PREFIX	= $prefix;
	$NEW_PD_PLEN	= $pfxlen;

	$child	= query($stsp."/child/uid");
	$mac	= PHYINF_getphymac($child);
	$hostid	= ipv6eui64($mac);
	if($NEW_PD_PLEN < 64)
	{
		/* handle blackhole issue */
		//cmd("ip -6 route add blackhole ".$prefix."/".$pfxlen." dev lo");
		//set($stsp."/blackhole/prefix", $prefix);
		//set($stsp."/blackhole/plen", $pfxlen);

		$holecnt = query($stsp."/blackhole/count");
		if($holecnt=="")
		{
			$holecnt = 1;
			set($stsp."/blackhole/entry:".$holecnt."/prefix", $prefix);
			set($stsp."/blackhole/entry:".$holecnt."/plen", $pfxlen);
			set($stsp."/blackhole/count", $holecnt);
			cmd("ip -6 route add blackhole ".$prefix."/".$pfxlen." dev lo");
		}

		$slalen = 64-$pfxlen;
		$ipaddr	= ipv6ip($prefix, $pfxlen, $hostid, 1, $slalen);
		$pfxlen = 64;
	}
	else
	{
		$ipaddr	= ipv6ip($prefix, $pfxlen, $hostid,0,0);
	}

	/* Check renew, do nothing if we got the same prefix and prefix len as previous ones */
	$oldip = query($stsp."/child/ipaddr");
	$oldpfl = query($stsp."/child/prefix");	
	if($oldip == $ipaddr && $oldpfl == $pfxlen) {msg("STATELESS - Renew but do nothing"); return;}

	set($stsp."/child/ipaddr", $ipaddr);
	set($stsp."/child/prefix", $pfxlen);
	set($stsp."/child/pdnetwork", strip($_GLOBALS["NEW_PD_PREFIX"]));
	set($stsp."/child/pdprefix", strip($_GLOBALS["NEW_PD_PLEN"]));
	set($stsp."/child/pdplft", strip($_GLOBALS["NEW_PD_PLTIME"]));
	set($stsp."/child/pdvlft", strip($_GLOBALS["NEW_PD_VLTIME"]));
	
		$childgz = query($stsp."/childgz/uid");
		if ($childgz!="")
		{
			$mac	= PHYINF_getphymac($childgz);
			msg("Guest zone mac is ".$mac);
			$hostid	= ipv6eui64($mac);
			if($NEW_PD_PLEN < 64)
			{
				$cipaddr	= ipv6ip($prefix, $pfxlen, $hostid, 0,64-$pfxlen);
				$cpfxlen = 64;
			}
			else
			{
				//If pd length is bigger than 63, don't set to guest zone
				$cipaddr = "";
				$cpfxlen = "";
			}
		
			if($cipaddr!="")/* check if renew IANA, $NEW_PD_PREFIX and $NEW_PD_PLEN are zero */
			{
				set($stsp."/childgz/ipaddr", $cipaddr);
				set($stsp."/childgz/prefix", $cpfxlen);
				set($stsp."/childgz/pdnetwork", strip($_GLOBALS["NEW_PD_PREFIX"]));
				set($stsp."/childgz/pdprefix", strip($_GLOBALS["NEW_PD_PLEN"]));
				set($stsp."/childgz/pdplft", strip($_GLOBALS["NEW_PD_PLTIME"]));
				set($stsp."/childgz/pdvlft", strip($_GLOBALS["NEW_PD_VLTIME"]));
			}
		}
	$DOMAIN			= strip($_GLOBALS["DOMAIN"]);
	if($DOMAIN!="")
		XNODE_set_var($child."_DOMAIN", $DOMAIN);
	XNODE_set_var($child."_PDPLFT", strip($_GLOBALS["NEW_PD_PLTIME"]));
	XNODE_set_var($child."_PDVLFT", strip($_GLOBALS["NEW_PD_VLTIME"]));

	msg("STATELESS - Child ".$ipaddr."/".$pfxlen);

	$DNS			= strip($_GLOBALS["DNS"]);
	$NAMESERVERS	= strip($_GLOBALS["NAMESERVERS"]);
	/* Combine the user config and DHCP server setting. */
	$dns = $DNS;
	/*
	if ($NAMESERVERS!="")
	{
		if ($dns=="")	$dns = $NAMESERVERS;
		else			$dns = $dns." ".$NAMESERVERS;
	}
	*/
	$infprev = query($stsp."/infprevious");
	if($infprev!="")
	{
		$stsprevp = XNODE_getpathbytarget("/runtime", "inf", "uid", $infprev, 0);
		$pdevnam  = query($stsprevp."/devnam");
		$conf	= "/var/run/".$pdevnam;
	}
	else
		$conf	= "/var/run/".query($stsp."/devnam");
	//$oflag = fread("e", $conf.".ra_oflag");
	//if($oflag=="1")
	//{
		if ($NAMESERVERS!="")
		{
			if ($dns=="")	$dns = $NAMESERVERS;
			else			$dns = $dns." ".$NAMESERVERS;
		}
	//}

	/* DS-Lite info */
	$remote	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	if($remote!="")
		set($stsp."/inet/ipv4/ipv4in6/remote", $remote);

	anchor($stsp);
	cmd("phpsh /etc/scripts/IPV6.INET.php ACTION=ATTACH".
			" MODE=STATELESS".
			" INF=".	$inf.
			" DEVNAM=".	query("devnam").
			" IPADDR=".	query("stateless/ipaddr").
			" PREFIX=".	query("stateless/prefix").
			" GATEWAY=".query("stateless/gateway").
			//' "DNS='.	query("stateless/dns").'"');
			' "DNS='.$dns.'"');
	/*del($stsp."/stateless");*/ /*renew may cause wan ip be disappered*/
}

function handle_infoonly($inf)
{
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);
	if ($stsp=="") {msg("INFOONLY - no runtime for ".$inf); return;}

	$DNS			= strip($_GLOBALS["DNS"]);
	$NAMESERVERS	= strip($_GLOBALS["NAMESERVERS"]);
	/* Combine the user config and DHCP server setting. */
	$dns = $DNS;
	if ($NAMESERVERS!="")
	{
		if ($dns=="")	$dns = $NAMESERVERS;
		else			$dns = $dns." ".$NAMESERVERS;
	}

	//$NEW_AFTR_NAME	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	$remote	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	msg("INFOONLY - DNS: ".$dns.", aftr server: ".$remote);

	add_each($dns, $stsp."/inet/ipv6", "dns");

	/* DS-Lite info */
	//$remote	= strip($_GLOBALS["NEW_AFTR_NAME"]);
	if($remote!="")
		set($stsp."/inet/ipv4/ipv4in6/remote", $remote);
}

function handle_pppdhcp($inf, $devnam, $opt)
{
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);
	if ($stsp=="" || $devnam=="") return;

	$infprev = query($stsp."/infprevious");
	$prevstsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $infprev, 0);
	$prevphyinf = query($prevstsp."/phyinf"); 
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $prevphyinf, 0);
	if ($p=="") {msg("PPPDHCP - no runtime phyinf for ".$infprev); return;}
	$ipaddr = query($p."/ipv6/global/ipaddr");

	$child = query($stsp."/child/uid");
	if($child!="")
	{
		/* we use global ip of ppp interface to judge stateless or stateful */
		if($ipaddr!="")
		{
			msg("PPPDHCP - Stateless - PD/".$opt);
			/* IA-PD is assigned to lan */
			handle_stateless($_GLOBALS["INF"],strip($_GLOBALS["NEW_PD_PREFIX"]),strip($_GLOBALS["NEW_PD_PLEN"]));
		}	
		else
		{
			msg("PPPDHCP - Stateful - PD/".$opt);
			/* IA-NA+IA-PD/IA-PD is assigned to wan and lan */
			handle_stateful($_GLOBALS["INF"],$_GLOBALS["DEVNAM"], $_GLOBALS["DHCPOPT"]);
		}
	}
	else
	{
			msg("PPPDHCP - Stateful - IANA");
			handle_stateful($_GLOBALS["INF"],$_GLOBALS["DEVNAM"], $_GLOBALS["DHCPOPT"]);
	}
	return;
}
/**************************************************************/
/* dhcpv6c has tailing space character in the arguments to the callback script.
 * strip the extra space characters with strip(). */

msg("AFTR name is ".$_GLOBALS["NEW_AFTR_NAME"]);
msg("MODE is ".$_GLOBALS["MODE"]);
msg("DHCPOPT is ".$_GLOBALS["DHCPOPT"]);
/* ip release case */
if(strip($_GLOBALS["NEW_ADDR"])=="" && strip($_GLOBALS["NEW_PD_PREFIX"])=="" && strip($_GLOBALS["NEW_PD_PLEN"])=="" && $_GLOBALS["MODE"]!="INFOONLY") return;
 
set("/runtime/device/ntp6/server", strip($_GLOBALS["NTPSERVER"]));

set("/runtime/services/wandetect6/autoconf/CHECK", "STATEFUL");//for autodetect

if ($_GLOBALS["MODE"]=="STATELESS")
	handle_stateless($_GLOBALS["INF"],strip($_GLOBALS["NEW_PD_PREFIX"]),strip($_GLOBALS["NEW_PD_PLEN"]));
//else if ($_GLOBALS["MODE"]=="STATEFUL" || $_GLOBALS["MODE"]=="PPPDHCP")
else if ($_GLOBALS["MODE"]=="STATEFUL")
	handle_stateful($_GLOBALS["INF"],$_GLOBALS["DEVNAM"], $_GLOBALS["DHCPOPT"]);
else if ($_GLOBALS["MODE"]=="PPPDHCP")
	handle_pppdhcp($_GLOBALS["INF"],$_GLOBALS["DEVNAM"], $_GLOBALS["DHCPOPT"]);
else if ($_GLOBALS["MODE"]=="INFOONLY")
	handle_infoonly($_GLOBALS["INF"]);
else msg("Unknown mode - ".$_GLOBALS["MODE"]);
?>
