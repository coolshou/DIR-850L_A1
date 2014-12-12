<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";

fwrite("w", $START, "#!/bin/sh\n");
fwrite("w", $STOP,  "#!/bin/sh\n");

$path = XNODE_getpathbytarget("", "inf", "uid", "LAN-1", 0);
$lan_ip_addr = INF_getcurripaddr("LAN-1");
$wan_ip_addr = INF_getcurripaddr("WAN-1");
$path_wan = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$wan_https_rport = query($path_wan."/https_rport");
$stunnel = query($path."/stunnel");
if(query("webaccess/enable")=="1") $wfa_stunnel = 1;
else $wfa_stunnel = 0;

$mydlink = query("/mydlink/register_st");

$stunnel_conf = "/var/stunnel.conf";

/* prepare data for http to create httpd.conf (service STUNNEL) */	
if ($stunnel==1 || $wfa_stunnel==1 || $mydlink==1)
{
	//prepare stunnel needed file
	fwrite("w",$stunnel_conf,"cert = /etc/stunnel_cert.pem\n");
	fwrite("a",$stunnel_conf,"key =/etc/stunnel.key\n");
	fwrite("a",$stunnel_conf,"pid = /var/run/stunnel.pid\n");
	fwrite("a",$stunnel_conf,"setuid = 0\n");
	fwrite("a",$stunnel_conf,"setgid = 0\n");
	fwrite("a",$stunnel_conf,"debug = 7\n");
	fwrite("a",$stunnel_conf,"output = /var/log/stunnel.log\n");

	if($stunnel==1)
	{
		fwrite("a",$stunnel_conf,"[https]\n");
		fwrite("a",$stunnel_conf,"accept  = ".$lan_ip_addr.":443\n");
		fwrite("a",$stunnel_conf,"connect = 127.0.0.1:80\n");
		if($wan_https_rport!="")
		{
			fwrite("a",$stunnel_conf,"[https]\n");
			fwrite("a",$stunnel_conf,"accept  = ".$wan_ip_addr.":".$wan_https_rport."\n");
			fwrite("a",$stunnel_conf,"connect = 127.0.0.1:80\n");		
		}
	}
	if($wfa_stunnel==1)
	{
		fwrite("a",$stunnel_conf,"[https]\n");
		fwrite("a",$stunnel_conf,"accept  = ".query("/webaccess/httpsport")."\n");
		fwrite("a",$stunnel_conf,"connect = 127.0.0.1:".query("/webaccess/httpport")."\n");
		
	}
	if($mydlink==1)
	{
		fwrite("a",$stunnel_conf,"[https]\n");
		fwrite("a",$stunnel_conf,"accept  = 8183\n");
		fwrite("a",$stunnel_conf,"connect = 127.0.0.1:8182\n");
	}	
	
	fwrite("a", $START, "echo \"Start Stunnel service ..\"  > /dev/console\n");
	fwrite("a", $START, "stunnel ".$stunnel_conf."&\n");
	fwrite("a", $STOP, "echo \"Stop Stunnel service ..\"  > /dev/console\n");
	fwrite("a", $STOP, "killall stunnel\n");

	/* Prepare data for http to listen ssl data from stunel. (127.0.0.1:80) */
	$stsp = XNODE_getpathbytarget("/runtime/services/http", "server", "uid", "STUNNEL", 0);
	$dirty=0;

	if ($stsp=="")
	{
		$dirty++;
		$stsp = XNODE_getpathbytarget("/runtime/services/http", "server", "uid", "STUNNEL", 1);
		set($stsp."/mode",	"STUNNEL");
		set($stsp."/ifname", "lo");
		set($stsp."/ipaddr","127.0.0.1");
		set($stsp."/port",	80);
		set($stsp."/af",	"inet");
		set($stsp."/stunnel", $stunnel);
		set($stsp."/wfa_stunnel", $wfa_stunnel);
	}
	else
	{
		if (query($stsp."/mode")!="STUNNEL")		{ $dirty++; set($stsp."/mode", "STUNNEL"); }
		if (query($stsp."/ifname")!="lo")		{ $dirty++; set($stsp."/ifname", "lo"); }
		if (query($stsp."/ipaddr")!="127.0.0.1")		{ $dirty++; set($stsp."/ipaddr", "127.0.0.1"); }
		if (query($stsp."/port")!= 80)		{ $dirty++; set($stsp."/port", 80); }
		if (query($stsp."/af")!="inet")		{ $dirty++; set($stsp."/af", "inet"); }
		if (query($stsp."/stunnel")!=$stunnel)		{ $dirty++; set($stsp."/stunnel", $stunnel); }
		if (query($stsp."/wfa_stunnel")!=$wfa_stunnel)		{ $dirty++; set($stsp."/wfa_stunnel", $wfa_stunnel); }
	}

	if ($dirty>0) $action="restart"; else $action="start";
	fwrite("a", $START, "service HTTP ".$action);
}
else
{
	$stsp = XNODE_getpathbytarget("/runtime/services/http", "server", "uid", "STUNNEL", 0);
	if($stsp != "")	del($stsp);
}	
?>
