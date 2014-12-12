<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/webinc/config.php";

$Response="BindSuccess";
$uptime_limit = query(INF_getinfpath($WAN1)."/open_dns/nonce_uptime") + 1800;
if(query(INF_getinfpath($WAN1)."/open_dns/nonce")!=$_GET["nonce"] || $_GET["nonce"]=="")
{
	$Response="BindError";
}
else if(query("/runtime/device/uptime") > $uptime_limit)
{
	$Response="BindTimeout";
}

if($Response=="BindSuccess")
{		
	set(INF_getinfpath($WAN1)."/open_dns/type", "parent");
	set(INF_getinfpath($WAN1)."/open_dns/deviceid", $_GET["deviceid"]);
	set(INF_getinfpath($WAN1)."/open_dns/parent_dns_srv/dns1", $_GET["dnsip1"]);
	set(INF_getinfpath($WAN1)."/open_dns/parent_dns_srv/dns2", $_GET["dnsip2"]);		
		
	//+++sam_pan add	
	$LAN1 = "LAN-1";
	$dns4 = INF_getinfpath($LAN1)."/dns4";	
	set($dns4, "DNS4-1");												
	event("DBSAVE");								
	//---sam_pan 	
			
	setattr(INF_getinfpath($WAN1)."/open_dns/service", "set", "service OPENDNS4.MAP restart");
	set(INF_getinfpath($WAN1)."/open_dns/service", "");		
}	
?>
<html>
	<head>
		<meta http-equiv="Pragma" content="no-cache"> <!--for HTTP 1.1-->
		<meta http-equiv="Cache-Control" content="no-cache"> <!--for HTTP 1.0--> 
		<meta http-equiv="Expires" content="0"> <!--prevents caching at the proxy server-->
		<meta name="Description" content="<?=$Response?>"/>
		<script type="text/javascript">
			var Response = "<? echo $Response; ?>";
			function Body() {}
			Body.prototype =
			{
				OnLoad: function()
				{
					if(Response == "BindSuccess")
					{
						document.getElementById("success").style.display = "block";
					}
					else if(Response=="BindError" || Response=="BindTimeout")
					{
						document.getElementById("fail").style.display = "block";
					}								
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
			<div id="success" style="display:none;">
				<br>
				<br>
				<br>
				<div align="left"><? echo i18n("Congratulations! Your user account and your router have both been registered with OpenDNS&reg;. Click CONTINUE to configure your Cloud-based Parental Control settings.");?></div>
				<br>
				<br>
				<br>		
				<input type="button" id="config_continue" value="<? echo I18N("h", "CONTINUE to configuration website");?>" onClick="self.location.href = 'http://www.opendns.com/device/welcome/?device_id=<? echo $_GET["deviceid"];?>';" />&nbsp;&nbsp;
				<input type="button" id="config_later" value="<? echo I18N("h", "Configure later");?>" onClick="BODY.window_close();"/>
			</div>
			
			<div id="fail" style="display:none;">
				<div align="left"><? echo I18N("h", "SORRY. Right now, it is not possible to register your router with OpenDNS&reg;.");?></div>
				<br>
				<br>
				<div align="left"><? echo I18N("h", "This could be because the operation was time out, or because your router detected a security risk.");?></div>
				<br>
				<br>
				<div align="left"><? echo I18N("h", "Please click RESTART to restart Cloud-based Parental Control program.");?></div>
				<br>
				<br>
				<br>
				<input type="button" id="restart_settings" value="<? echo I18N("h", "Restart");?>" onClick="self.location.href = '/parentalcontrols/register.php?username=admin&password=&overriteDeviceID=no';" />
			</div>
		</div>	
	</body>
</html>
