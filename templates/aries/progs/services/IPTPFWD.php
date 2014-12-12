<?
/* VSVR & PFWD are depends on LAN services.
 * Be sure to start LAN services first. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/inf.php";
include "/etc/services/IPTABLES/iptlib.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w",$STOP,  "#!/bin/sh\n");
if ($ME!="virtualserver") $ME="portforward";

/* Get all the LAN interface IP address. */
IPT_scan_lan();

$cnt = query("/nat/count"); if ($cnt=="") $cnt = 0;
foreach ("/nat/entry")
{
	/* beyond the count are garbage */
	if ($InDeX>$cnt) break;

	/* Get the CHAIN */
	$UID = query("uid");
	if ($ME=="portforward")	$CHAIN="DNAT.PFWD.".$UID;
	else					$CHAIN="DNAT.VSVR.".$UID;
	/* Mark that there is no rules in the CHAIN. */
	XNODE_set_var($CHAIN.".USED", "0");
	/* Flush the CHAIN */
	fwrite("a",$START, "iptables -t nat -F ".$CHAIN."\n");
	fwrite("a",$START, "iptables -t nat -F PFWD.".$UID."\n");
	fwrite("a",$STOP,  "iptables -t nat -F ".$CHAIN."\n");

	/* Walk through the rules. */
	$ecnt = query($ME."/count"); if ($ecnt=="") $ecnt=0;
	foreach ($ME."/entry")
	{
		/* beyond the count are garbage */
		if ($InDeX>$ecnt) break;
		/* enable ? */
		if (query("enable")!=1) continue;

		/* check the protocol */
		$prot_tcp = 0; $prot_udp = 0;
		$prot = query("protocol");
		if ($prot=="TCP+UDP") {	$prot_tcp++; $prot_udp++; }
		else if	($prot=="TCP")	$prot_tcp++;
		else if	($prot=="UDP")	$prot_udp++;
		else continue;

		/* check the destination host */
		$inf	= query("internal/inf");
		$hostid = query("internal/hostid");
		$ipaddr = XNODE_get_var($inf.".IPADDR");
		$mask	= XNODE_get_var($inf.".MASK");
		if ($ipaddr=="" || $mask=="" || $hostid=="" || $inf=="") continue;
		$ipaddr = ipv4ip($ipaddr, $mask, $hostid);
		if ($ipaddr=="") continue;

		/* check port setting */
		$ext_end	= query("external/end");
		$ext_start	= query("external/start");	if ($ext_start=="") continue;
		$int_start	= query("internal/start");	if ($int_start=="") $int_start = $ext_start;
		if		($int_start > $ext_start) $offset = $int_start - $ext_start;
		else if ($int_start < $ext_start) $offset = 65536 - $ext_start + $int_start;
		else							  $offset = 0;

		/* port */
		if ($ext_end=="" || $ext_end==$ext_start) $portcmd = "--dport ".$ext_start;	/* Single port forwarding */
		else $portcmd = "-m mport --dports ".$ext_start.":".$ext_end; /* Multi port forwarding */
		/* DNAT */
		if ($offset=="0") $dnatcmd = "-j DNAT --to-destination ".$ipaddr;
		else $dnatcmd = "-j DNAT --to-shift ".$ipaddr.":".$offset;
		/* time */
		$sch = query("schedule");
		if ($sch=="") $timecmd = "";
		else $timecmd = IPT_build_time_command($sch);

		$iptcmd = "iptables -t nat -A ".$CHAIN." ".$timecmd;
		if ($prot_tcp>0) fwrite("a",$START, $iptcmd." -p tcp ".$portcmd." ".$dnatcmd."\n");
		if ($prot_udp>0) fwrite("a",$START, $iptcmd." -p udp ".$portcmd." ".$dnatcmd."\n");
		XNODE_set_var($CHAIN.".USED", "1");
	}
	/* Add the chain to PFWD.$UID, 
	 * So that the LAN hosts can correctly access the forwarded LAN host with IP of WAN. */
	include "/etc/services/_add_chains_to_pfwd.php";
}

fwrite("a",$START, "exit 0\n");
fwrite("a",$STOP,  "exit 0\n");
?>
