<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";

function startcmd($cmd) {fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)  {fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

//write config file
$infp = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0);
$lanip=query($infp."/inet/ipv4/ipaddr");
$lanif=query($infp."/devnam");
$http_forward=query("/miiicasa/http_forward");

if($lanif=="")
	$lanif="br0";

$proxyd_port="48723";
$config_file="/var/run/miiicasa_proxyd.conf";

fwrite("w", $config_file, "CONTROL\n{\n");
fwrite("a", $config_file, "\tTIMEOUT_CONNECT\t120\n");
fwrite("a", $config_file, "\tTIMEOUT_READ\t120\n");
fwrite("a", $config_file, "\tTIMEOUT_WRITE\t120\n");
fwrite("a", $config_file, "\tMAX_CLIENT\t128\n");
fwrite("a", $config_file, "}\n\n");
fwrite("a", $config_file, "HTTP\n{\n");
fwrite("a", $config_file, "\tINTERFACE\t".$lanif."\n");
fwrite("a", $config_file, "\tPORT\t".$proxyd_port."\n");
fwrite("a", $config_file, "\tALLOW_TYPE\t{ gif jpg css png }\n");
fwrite("a", $config_file, "\tERROR_PAGE\n\t{\n");
fwrite("a", $config_file, "\t\tdefault\t/var/run/blocked.html\n");
fwrite("a", $config_file, "\t\t403\t/var/run/blocked.html\n");
fwrite("a", $config_file, "\t\t404\t/none_exist_file\n");
fwrite("a", $config_file, "\t}\n}\n\n");

$config_url_file="/var/run/miiicasa_proxyd_url.conf";
fwrite("w", $config_url_file, "0\n");   // Allow to access,

if($http_forward == "1"){
	startcmd("/usr/sbin/miiicasa_proxyd -f ".$config_file." -u ".$config_url_file." & > /dev/console\n");
	startcmd("iptables -t nat -N MIIICASA_PROXYD\n");
	startcmd("iptables -t nat -I PREROUTING -i ".$lanif." -p tcp ! -d ".$lanip." --dport 80 -j MIIICASA_PROXYD\n");

	stopcmd("iptables -t nat -D PREROUTING -i ".$lanif." -p tcp ! -d ".$lanip." --dport 80 -j MIIICASA_PROXYD\n");
	stopcmd("iptables -t nat -X MIIICASA_PROXYD\n");
	stopcmd("killall miiicasa_proxyd\n");
}

?>
