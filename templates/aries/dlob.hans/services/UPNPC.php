<?
include "/htdocs/phplib/inf.php";

$UPNPC_CFG	= "/var/upnpc.conf";
$total_client=query("/runtime/upnpigd/portmapping/count")+1;

$infp = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$phyinf=query($infp."/phyinf");
if($infp!="")
{
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "phyinf", $phyinf, 0);       
	anchor($stsp."/inet");
	$addrtype = query("addrtype");
	if              ($addrtype=="ipv4" && query("ipv4/valid")=="1") $ipaddr=query("ipv4/ipaddr");
	else if ($addrtype=="ppp4" && query("ppp4/valid")=="1") $ipaddr="";	
}

fwrite("w",$UPNPC_CFG,"Protocol:TCP\n");
fwrite("a",$UPNPC_CFG,"Ext_Port:5555\n");
fwrite("a",$UPNPC_CFG,"Int_Port:5555\n");
fwrite("a",$UPNPC_CFG,"Desc:MIIICASA\n");
fwrite("a",$UPNPC_CFG,"Lease:0\n");
fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");
	
foreach ("/runtime/upnpigd/portmapping/entry")
{
	$proto=query("protocol");
	$ext_port=query("externalport");
	$desc=query("description");
	$lease=query("leaseduration");
	
	fwrite("a",$UPNPC_CFG,"Protocol:".$proto."\n");
	fwrite("a",$UPNPC_CFG,"Ext_Port:".$ext_port."\n");
	fwrite("a",$UPNPC_CFG,"Int_Port:".$ext_port."\n");
	fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
	fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
	fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");
}
/*virtual server*/
$cnt = query("/nat/count"); if ($cnt=="") $cnt = 0;

foreach ("/nat/entry")
{
	/* beyond the count are garbage */
	if ($InDeX>$cnt) break;

	/* Walk through the rules. */
	$ecnt = query("virtualserver/count"); if ($ecnt=="") $ecnt=0;
	foreach ("virtualserver/entry")
	{
		/* beyond the count are garbage */
		if ($InDeX>$ecnt) break;
		/* enable ? */
		if (query("enable")!=1 ) continue;

		/* check the protocol */
		$prot_tcp = 0; $prot_udp = 0; $prot_other = 0; $offset = 0;
		$prot = query("protocol");
		if ($prot=="TCP+UDP") {	$prot_tcp++; $prot_udp++; }
		else if	($prot=="TCP")	$prot_tcp++;
		else if	($prot=="UDP")	$prot_udp++;
		else if	($prot=="Other")$prot_other++;
		else continue;

		$ext_port	= query("external/start");	if ($ext_port=="") continue;
		$desc=query("description");
		$lease=0;
		
		if($prot_tcp >0)
		{
			fwrite("a",$UPNPC_CFG,"Protocol:TCP\n");
			fwrite("a",$UPNPC_CFG,"Ext_Port:".$ext_port."\n");
			fwrite("a",$UPNPC_CFG,"Int_Port:".$ext_port."\n");
			fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
			fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
			fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");			
			$total_client++;
		}
		if($prot_udp>0)
		{
			fwrite("a",$UPNPC_CFG,"Protocol:UDP\n");
			fwrite("a",$UPNPC_CFG,"Ext_Port:".$ext_port."\n");
			fwrite("a",$UPNPC_CFG,"Int_Port:".$ext_port."\n");
			fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
			fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
			fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");
			$total_client++;
		}
	}
}
/*remote access*/
$remote_port=query($infp."/web");
if($remote_port!="")
{
	$desc="RemoteAccess";
	$lease=0;
	fwrite("a",$UPNPC_CFG,"Protocol:TCP\n");
	fwrite("a",$UPNPC_CFG,"Ext_Port:".$remote_port."\n");
	fwrite("a",$UPNPC_CFG,"Int_Port:".$remote_port."\n");
	fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
	fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
	fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");
	$total_client++;
}
/*web file access*/
$web_access=query("/webaccess/enable");
if($web_access=="1")
{
	$http_en=query("/webaccess/httpenable");
	$httpport=query("/webaccess/httpport");
	
	$https_en=query("/webaccess/httpsenable");
	$httpsport=query("/webaccess/httpsport");

	$desc="WebFileAccess";
	$lease=0;	
	
	if($http_en=="1" && $httpport!="")
	{
		fwrite("a",$UPNPC_CFG,"Protocol:TCP\n");
		fwrite("a",$UPNPC_CFG,"Ext_Port:".$httpport."\n");
		fwrite("a",$UPNPC_CFG,"Int_Port:".$httpport."\n");
		fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
		fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
		fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");
		$total_client++;
	}
	if($https_en=="1" && $httpsport !="")
	{
		fwrite("a",$UPNPC_CFG,"Protocol:TCP\n");
		fwrite("a",$UPNPC_CFG,"Ext_Port:".$httpsport."\n");
		fwrite("a",$UPNPC_CFG,"Int_Port:".$httpsport."\n");
		fwrite("a",$UPNPC_CFG,"Desc:".$desc."\n");
		fwrite("a",$UPNPC_CFG,"Lease:".$lease."\n");
		fwrite("a",$UPNPC_CFG,"Action:ADD\n\n");		
		$total_client++;
	}
}

fwrite("w",$START, "#!/bin/sh\n");
if($ipaddr=="")
{
	fwrite("a",$START, "Can't get wan ip address\n");
}
else
{
	$dev=query($stsp."/devnam");
	$iptables_rules=query("/runtime/upnpc_rules");
	//fwrite("a",$START,"iptables -t nat -D PREROUTING -i ".$dev." -d ".$ipaddr." -p UDP --dport 41900 -j ACCEPT\n");//for upnpc daemon
	if($iptables_rules!="")
	{
		fwrite("a",$START,"iptables -t nat -D PREROUTING ".$iptables_rules."\n");//for upnpc daemon
	}
	fwrite("a",$START,"iptables -t nat -I PREROUTING -i ".$dev." -d ".$ipaddr." -p UDP --dport 41900 -j ACCEPT\n");//for upnpc daemon
	set("/runtime/upnpc_rules","-i ".$dev." -d ".$ipaddr." -p UDP --dport 41900 -j ACCEPT");
	fwrite("a",$START, "upnpc_daemon -m ".$ipaddr." -t 120 -l ".$total_client." -f ".$UPNPC_CFG."&\n");
}

fwrite("w",$STOP,  "#!/bin/sh\n");
fwrite("a",$STOP,  "killall upnpc_daemon\n");
fwrite("a",$STOP,  "rm ".$UPNPC_CFG."\n");
?>
