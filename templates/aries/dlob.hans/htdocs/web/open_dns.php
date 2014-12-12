<?
include "/htdocs/phplib/xnode.php";
$inf_wan1 = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$mac=query("/runtime/devdata/wanmac");
$newMac="";
$num=0;
while ($num < 6)
{
    $tmpMac = cut($mac, $num, ":");
    $newMac = $newMac.$tmpMac;
    $num++;
}

$vendor="dlink";
$model=query("/runtime/device/modelname"); 

$uptime=query("/runtime/device/uptime");
set($inf_wan1."/open_dns/nonce_uptime", $uptime);
$n=0;
while ($n < 100)
{
	if(strlen($uptime) >= 8)
	{
		$random8=substr($uptime, 0, 8);
		break;	
	}	
	$uptime=$uptime*3 + $uptime;
	$n++;
}

$lan1_infp = XNODE_getpathbytarget("", "inf", "uid", "LAN-1", 0);
$lan1_inetp = XNODE_getpathbytarget("inet", "entry", "uid", query($lan1_infp."/inet"), 0);
$lan_ip_router = query($lan1_inetp."/ipv4/ipaddr");

$nonce="nonce=".$random8;
set($inf_wan1."/open_dns/nonce", $random8);
$mapfile="adv_parent_ctrl_map.php";
$routerurl="http%3A%2F%2F".$lan_ip_router."%2F".$mapfile."%3F".$nonce;
$style="web_v1_0";
$account_map_addr="https://www.opendns.com/device/register/?vendor=".$vendor."&model=".$model."&desc=".$model."&mac=".$newMac."&url=".$routerurl."&style=".$style;
?>
<html>
	<head> 
		<div align="center">
<?
			if ($AUTHORIZED_GROUP < 0){echo "Authenication fail";}
			else
			{
				echo    '\t\t<meta http-equiv="Refresh" content="0; url='.$account_map_addr.'"/>';
			}
?>
		<div>
	</head>
</html>

