HTTP/1.1 200 OK
Content-Type:text/plain
Content-Disposition:attachment;filename="Router_Info.txt"

<?
/* Raise a "File Download" dialogue box for wiz_freset, Sammy_hsu */
include "/htdocs/phplib/xnode.php";
if ($AUTHORIZED_GROUP==0)
{
	/* get admin_passwd */
	$admin_p = XNODE_getpathbytarget("/device/account", "entry", "name", "admin", 0);
	$admin_passwd = query($admin_p."/password");
	if($admin_passwd=="") $admin_passwd = "(empty)";
	
	/* get SSID and Password */
	$phyinf = XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);
	$wifip = XNODE_getpathbytarget("wifi", "entry", "uid", query($phyinf."/wifi"), 0);
	$wlan_ssid = query($wifip."/ssid");
	$wlan_key = query($wifip."/nwkey/psk/key");
	
	$wanmode = $_GET["st_wanmode"];
	$pppname = $_GET["st_pppname"];
	$ppppwd = $_GET["st_ppppwd"];
	$svr = $_GET["st_svr"];
	$ipaddr = $_GET["st_ipaddr"];
	$mask = $_GET["st_mask"];
	$gw = $_GET["st_gw"];
	$dns = $_GET["st_dns"];	
	
	echo "Router Login Password\r\n";
	echo "Password:".$admin_passwd."\r\n";
	echo "\r\n";
	echo "Internet Connection (".$wanmode.")\r\n";
	if($pppname!="") echo "Username:".$pppname."\r\n";
	if($ppppwd!="") echo "Password:".$ppppwd."\r\n";	
	if($svr!="") echo "Server IP:".$svr."\r\n";
	if($ipaddr!="") echo "IP address:".$ipaddr."\r\n";	
	if($mask!="") echo "Subnet mask:".$mask."\r\n";
	if($gw!="") echo "Gateway:".$gw."\r\n";
	if($dns!="") echo "Primary DNS:".$dns."\r\n";			
	echo "\r\n";	
	echo "Router Wi-Fi Info (WPA2)\r\n";
	echo "SSID (network name): ".$wlan_ssid."\r\n";
	echo "Password: ".$wlan_key."\r\n";
}
?>
