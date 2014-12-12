<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";


function enable_fakedns()
{
	echo "xmldbc -s /runtime/smart404/fakedns 1\n";

	//redirect all HTTP access to 404 page
	//remove this rule because the next rule will handle all http requests
	$proxyd_port="5449";
	echo "iptables -t nat -D PREROUTING -d 1.33.203.39 -p tcp --dport 80 -j REDIRECT --to-ports ".$proxyd_port."\n";

	echo "iptables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80\n";
	echo "iptables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80\n";

	//redirect all DNS to fakedns
   echo "iptables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 63481\n";
    echo "iptables -t nat -I PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 63481\n";

	echo "iptables -t nat -D PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports 63481\n";
	echo "iptables -t nat -I PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports 63481\n";
}

function start_proxyd()
{
	echo "killall -9 proxyd\n";
	
	$r_infp = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0);
	$lanif=query($r_infp."/devnam");
	if($lanif=="")
	{
		echo "echo   Not found the name of LAN interface, set it as br0\n";
		$lanif="br0";
	}
	
	$proxyd_port="5449";
	
	$config_file="/var/run/proxyd.conf";
	fwrite("w", $config_file, "CONTROL\n{\n");
	fwrite("a", $config_file, "\tTIMEOUT_CONNECT\t30\n");
	fwrite("a", $config_file, "\tTIMEOUT_READ\t30\n");
	fwrite("a", $config_file, "\tTIMEOUT_WRITE\t30\n");
	fwrite("a", $config_file, "\tMAX_CLIENT\t32\n");
	fwrite("a", $config_file, "}\n\n");
	fwrite("a", $config_file, "HTTP\n{\n");
	fwrite("a", $config_file, "\tINTERFACE\t".$lanif."\n");
	fwrite("a", $config_file, "\tPORT\t".$proxyd_port."\n");
	fwrite("a", $config_file, "\tALLOW_TYPE\t{ gif jpg css png }\n");
	fwrite("a", $config_file, "\tERROR_PAGE\n\t{\n");
	fwrite("a", $config_file, "\t\tdefault\t/htdocs/smart404/index.php\n");
	fwrite("a", $config_file, "\t\t403\t/htdocs/smart404/index.php\n");
	fwrite("a", $config_file, "\t\t404\t/none_exist_file\n");
	fwrite("a", $config_file, "\t}\n}\n\n");
	
	$config_url_file="/var/run/proxyd_url.conf";
	fwrite("w", $config_url_file, "0\n");   // Allow to access,
	
	echo "proxyd -m 1.33.203.39 -f ".$config_file." -u ".$config_url_file." & > /dev/console\n";
	echo "iptables -t nat -D PREROUTING -d 1.33.203.39 -p tcp --dport 80 -j REDIRECT --to-ports ".$proxyd_port." 2>&-\n";
	echo "iptables -t nat -I PREROUTING -d 1.33.203.39 -p tcp --dport 80 -j REDIRECT --to-ports ".$proxyd_port."\n";
}
function disable_fakedns()
{
	echo "xmldbc -s /runtime/smart404/fakedns 0\n";

	//cancel all rules
	echo "iptables -t nat -D PREROUTING -p tcp --dport 53 -j REDIRECT --to-ports 63481 2>&-\n";
	echo "iptables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports 63481 2>&-\n";
	echo "iptables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80 2>&-\n";

	// Some web bowsers will cache the DNS respons for a while, and our fakeDNS isn't exceptional.
	// When WAN is up, we use the proxyd program to handle this situation that the web browser employ the fake ip to request HTTP.
	start_proxyd();
}

function is_ppp_class()
{
	$addr_type = INF_getcurraddrtype("WAN-1");

	if($addr_type == "ppp4" || $addr_type == "ppp6")
		return "true";
	
	return "false";
}

function wan_is_ready()
{
	/*remove the 404 just redirect to wizard if factory reset.*/
	if(query("/runtime/device/devconfsize")==0) 
	{
		return "false";	
	}
	else
	{
		return "true";
	}
/*
	$wanlink = query("/runtime/smart404/wanlink");

	if(query("/runtime/device/devconfsize")==0) 
	{
		return "false";	
	}

	if($wanlink != "1")
		return "false"; //cable is not connected

	$is_ppp = is_ppp_class();

	if($is_ppp == "true")
		return "true"; //in ppp mode, we only care about phy link status

	$wan1up = query("/runtime/smart404/wan1up");

	if($wan1up == "1")
		return "true";
	else
		return "false";
*/
}

$smart404_enable = query("/runtime/smart404");

//smart404 disable
if($smart404_enable != "1")
	exit;

//not in router mode, remove all rules of fakedns
$device_layout = query("/device/layout");
if($device_layout != "router")
{
	disable_fakedns();
	exit;
}

if(wan_is_ready() == "true")
	disable_fakedns();
else
	enable_fakedns();
?>
