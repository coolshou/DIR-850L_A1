<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/webinc/config.php";
$Response="Unauthorized";
$requestname = tolower($_GET["username"]); 
foreach ("/device/account/entry")
{
	$lowername1 = tolower(query("name")); 
	if($lowername1 == $requestname && query("password")==$_GET["password"] && query("group")=="0")
	{
		$Response="";
		break;
	}	
}

if($Response!="Unauthorized")
{
	$Response="InternetUnreachable";
	set("/runtime/diagnostic/ping", "www.opendns.com");
	$uptime = query("/runtime/device/uptime");
	while(query("/runtime/device/uptime") - $uptime < 2)
	{
		if(get("x", "/runtime/diagnostic/ping")=="www.opendns.com is alive!") 
		{
			$Response="";
			break;
		}	
	}
	
	if($Response!="InternetUnreachable")
	{
		if(query(INF_getinfpath($WAN1)."/open_dns/deviceid")!="" && $_GET["overriteDeviceID"]!="yes")
		{
			$Response="Devicebinded";
		}	
		else
		{
			$Response="RedirectTo";
			
			$inf_wan1 = INF_getinfpath($WAN1);
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
			$lan_ip_router=query(INET_getpathbyinf("LAN-1")."/ipv4/ipaddr");
			$nonce="nonce=".$random8;
			set($inf_wan1."/open_dns/nonce", $random8);
			$mapfile="parentalcontrols%2Fbind.php";  
			$routerurl="http%3A%2F%2F".$lan_ip_router."%2F".$mapfile."%3F".$nonce;
			$style="web_v1_0";
			$account_map_addr="https://www.opendns.com/device/register/?vendor=".$vendor."&model=".$model."&desc=".$model."&mac=".$newMac."&url=".$routerurl."&style=".$style;		
		}
	}
}	

$ADMIN_path = XNODE_getpathbytarget("/device/account", "entry", "group", "0", 0);	
$ADMIN_name = query($ADMIN_path."/name");	

?>
<html>
	<head>
		<link rel="stylesheet" href="/css/general.css" type="text/css">
		<meta http-equiv="Pragma" content="no-cache"> <!--for HTTP 1.1-->
		<meta http-equiv="Cache-Control" content="no-cache"> <!--for HTTP 1.0--> 
		<meta http-equiv="Expires" content="0"> <!--prevents caching at the proxy server-->
		<meta name="Description" content="<?=$Response?>"/><?
		if($Response=="RedirectTo")
		{
			echo	"\n";
			echo    '\t\t<meta http-equiv="Refresh" content="0; url='.$account_map_addr.'"/>';	
		}
?>
		<script type="text/javascript">
			var Response = "<? echo $Response; ?>";
			function Body() {}
			Body.prototype =
			{
				OnLoad: function()
				{
					if(Response == "Unauthorized")
					{
						document.getElementById("authorize").style.display = "block";
					}
					else if(Response == "InternetUnreachable")
					{
						document.getElementById("InternetUnreach").style.display = "block";
					}
					else if(Response == "Devicebinded")
					{
						document.getElementById("device_binded").style.display = "block";
					}									
				},
				LoginSubmit: function(password_type, overwrite_deviceid)
				{
					if(password_type === "new") var pwd = document.getElementById("loginpwd").value;
					else var pwd = "<? echo $_GET["password"];?>";
					self.location.href = "/parentalcontrols/register.php?username=" + "<? echo $ADMIN_name;?>" + "&password=" + pwd + "&overriteDeviceID=" + overwrite_deviceid;
				},
				window_close: function()
				{
					window.opener=null;   
					window.open("","_self");   
					window.close();
				}						
			};
			var BODY = new Body();	
		</script>
	</head>
	<body style="text-align:center" onload="BODY.OnLoad();">
		<div style="height:400px;width:800px;">
			<div align="center">
				<img src="/pic/dlink_utility.jpg" width="800" height="80">
			</div>
			<div id="authorize" style="display:none;">
				<div align="left"><? echo I18N("h", "Please type in your ADMIN password for this router so that you can access the Cloud-based Parental Control setup pages.");?></div>
				<br>
				<br>
				<div class="loginbox">
					<span class="name"><? echo I18N("h", "User Name");?></span>
					<span class="delimiter">:</span>
					<span class="value"><? echo $ADMIN_name;?></span>
				</div>
				<div  class="loginbox">
					<span class="name"><? echo I18N("h", "Password");?></span>
					<span class="delimiter">:</span>
					<span class="value">
						<input type="password" id="loginpwd"/>&nbsp;&nbsp;
						<input type="button" id="noGAC" value="<?echo I18N("h", "Login");?>" onClick="BODY.LoginSubmit('new', 'no');" />
					</span>
				</div>
				<br>
				<br>
				<br>
				<div align="left"><? echo i18n("NOTE: After successfully logging into your router, you¡¦ll be presented with the login page for <a href='http://www.opendns.com/' target='_blank'>OpenDNS</a>&reg;, which is the company that provides the Cloud-based Parental Control service. After logging in or register on that page, you¡¦ll be able to configure your Cloud-based Parental Control settings.");?></div>		
			</div>
			
			<div id="InternetUnreach" style="display:none;">
				<div align="left"><? echo I18N("h", "The router could not detect the Cloud-based Parental Control server. Check whether the router's Internet Settings are configured correctly. For instance, if you use a PPPoE connection, check that the username/password settings are correct.");?></div>			
				<br>
				<br>
				<div align="center">
					<img src="/pic/smart_head.jpg" width="600" height="130">
				</div>
				<br>
				<br>
				<br>				
				<div align="center">
					<input type="button" id="try_again" value="<? echo I18N("h", "Try again");?>" onClick="BODY.LoginSubmit('old', 'no');"/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
					<input type="button" id="set_internet" value="<? echo I18N("h", "Go to Internet Settings");?>" onClick="self.location.href='/bsc_lan.php';" />
				</div>			
			</div>
			
			<div id="device_binded" style="display:none;">
				<br>
				<br>
				<br>		
				<div  align="left"><? echo I18N("h", "Your router has already been set up for Cloud-based Parental Control. Are you sure you want to change the settings.");?></div>
				<br>
				<br>
				<br>			
				<input type="button" id="new_settings" value="<? echo I18N("h", "Yes");?>" onClick="BODY.LoginSubmit('old', 'yes');" />&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
				<input type="button" id="close_window" value="<? echo I18N("h", "No");?>" onClick="BODY.window_close();"/>
			</div>
		</div>
	</body>
</html>
