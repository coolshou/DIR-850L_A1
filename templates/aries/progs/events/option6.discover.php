<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

$infp	= XNODE_getpathbytarget("", "inf", "uid", $INF, 0);
$phyinf	= query($infp."/phyinf");
$ifname	= PHYINF_getifname($phyinf);
$inet   = query($infp."/inet");
$inetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
anchor($inetp."/ppp4");
$user	= get("s", "username");                                                                                      
$pass	= get("s", "password"); 

//TRACE_debug("==Jerry2: ".$INFV4);

if ($user =="" || $pass =="")
{		
	$infp	= XNODE_getpathbytarget("", "inf", "uid", $INFV4, 0);	// WAN-1
	$inet   = query($infp."/inet");
	$inetp  = XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
	
	$user = query($inetp."/ppp4/username");		
	$pass = query($inetp."/ppp4/password");		
}

echo 'noauth nodeflate nobsdcomp nodetach noccp\n';
echo 'lcp-echo-failure 3\n';
echo 'lcp-echo-interval 30\n';
echo 'lcp-echo-failure-2 14\n';
echo 'lcp-echo-interval-2 6\n';
echo 'lcp-timeout-1 10\n';
echo 'lcp-timeout-2 10\n';
echo '+ipv6 ipcp-accept-remote ipcp-accept-local\n';
echo 'mtu 1454\n';
echo 'linkname DISCOVER\n';
echo 'ipparam DISCOVER\n';
echo 'usepeerdns\n';
echo 'defaultroute\n';
echo 'user "'.$user.'"\n';
echo 'password "'.$pass.'"\n';
echo 'noipdefault\n';
echo 'kpppoe pppoe_device '.$ifname.'\n';
echo 'pppoe_hostuniq\n';
?>
