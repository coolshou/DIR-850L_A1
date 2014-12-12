<?include "/htdocs/phplib/inet.php";?>
<?include "/htdocs/phplib/lang.php";?>
<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
div.simplecontainer2
{
	font-family: Tahoma, Helvetica, Geneva, Arial, sans-serif;
	font-size: 14px;
	clear: both;
	background-color: #404042;
	padding-top: 20px;
	padding-bottom: 20px;
	width: 841px;
}
div.simplecontainer2 div.simplebody
{
	clear: both;
	margin-left: auto;
	margin-right: auto;
	width: 710px;
}
div.networkmap
{
	border: none;
	padding: 0 10px 10px 10px;
	background-color: #d4d4d4;	
}
.networkmap h2 
{
	color: #000000;
	background-color: #da7228;
	font-weight: bold;
	font-size: 14px;
	letter-spacing: 1px;
	text-transform: none;
	text-align: left;
	margin: 0 -10px 5px -10px;
	padding: 5px 5px 5px 10px;  
}
div.progress_bar_status
{
	overflow: hidden;
	width:400px;
	height:14px;
	margin: 0 auto;
	border: 2px solid black;
}
.br_tb
{
	font-family: Tahoma, Helvetica, Geneva, Arial, sans-serif;
	font-size: 14px;
	text-align:right;
	font-weight: bold;
}
.l_tb
{
	font-family: Tahoma, Helvetica, Geneva, Arial, sans-serif;
	font-size: 14px;
	text-align:left;
}
p.strong_white
{
	color: #FFF;
	font-weight: bold;
	text-decoration: underline;
	cursor: pointer;
}
.text_style
{
	font-family: Tahoma, Helvetica, Geneva, Arial, sans-serif;
	font-size: 14px;
}
.text_style2
{
	font-weight: bold;
	text-align: center;
}
.submit_button
{
	font-family: Tahoma, Helvetica, Geneva, Arial, sans-serif;
	font-size: 14px;
	background: url(../pic/button_n.jpg);
	border-style: none;
	width: 187px;
	height: 48px;
	cursor: pointer;
}
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "RUNTIME.INF.LAN-1,INET.LAN-1,RUNTIME.INF.WAN-1,RUNTIME.PHYINF,PHYINF.WAN-1,INET.WAN-1,INET.WAN-2,WAN,WIFI.PHYINF,PHYINF.WIFI",
	logindefault: 0,
	OnLoad: function()
	{
		var confsize = <?echo query("/runtime/device/devconfsize");?>
		if(confsize==0 && this.logindefault==0)
		{
			AUTH.AuthorizedGroup = -1;
			AUTH.Login(
				function login_callback()
				{
					if(AUTH.AuthorizedGroup>=0) /*login success*/
					{
						BODY.GetCFG();
					}
					else /*login fail show login page.*/
					{
						alert("password is not default(Admin/blank), should not run here.");
						BODY.OnLoad();
					}
				},
				"Admin","");
			
			this.logindefault = 1;
		}
	},
	OnUnload: function() {},
	ShowSavingMessage: function() {},
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
			case "OK":
				for(var i=0; i<this.stages.length; i++) 
				{
					if(this.stages[i]==="stage_check_connect" && !this.saveonly)
						this.currentStage = i;
				}
				this.ShowCurrentStage();
				return true;
				break;
			case "BUSY":
				setTimeout('PAGE.SaveXML()', 500);
				break;
			case "HEDWIG":
				BODY.ShowAlert(result.Get("/hedwig/message"));
				break;
			case "PIGWIDGEON":
				if (result.Get("/pigwidgeon/message")=="no power")
				{
					BODY.NoPower();
				}
				else
				{
					BODY.ShowAlert(result.Get("/pigwidgeon/message"));
				}
				break;
			default : 
				this.currentStage--;
				this.ShowCurrentStage();
				return false;
		}
		return true;
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;	
	
		if (!this.InitWAN()) return false;
		if (!this.InitWLAN()) return false;
		this.ShowCurrentStage();
		return true;
	},
	PreSubmit: function()
	{
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		PXML.CheckModule("INET.WAN-2", null, null, "ignore");
		PXML.CheckModule("PHYINF.WAN-1", null, null, "ignore");
		PXML.CheckModule("WAN", null, "ignore", null);
		
		if (!this.PreWAN()) return null;
		if (!this.PreWLAN()) return null;
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	inet1p: null,
	inet2p: null,
	inet3p: null,
	inet4p: null,
	inf1p: null,
	inf2p: null,
	inf3p: null,
	inf4p: null,
	wifip: null,
	wifip2: null,
	wlanbase: null,
	phyinf: null,
	phyinf2: null,
	stages: new Array ("stage_set","stage_check_connect","stage_no_cable","stage_pppoe_error","stage_login_success"),
	wanTypes: new Array ("DHCP","DHCPPLUS", "PPPoE", "PPTP", "L2TP", "STATIC"),
	currentStage: 0, //0 ~ this.stages.length
	currentWanType: 0, //0 ~ this.wanTypes.length
	isFreset: <?if (query("/runtime/device/devconfsize")>0) echo 'false'; else echo 'true';?>,
	internetCheck: 0,
	drawbar: 0,
	dhcps4: null,
	dhcps4_inet: null,
	leasep: null,
	lanip: null,
	waninetp: null,
	rwaninetp: null,
	rwanphyp: null,
	macaddrp: null,
	wancable_status: 0,
	saveonly: false,
	refresh_timer: null,
	old_ipaddr: null,
	old_wantype: null,
	old_networkstatus: null,
	old_wanipaddr: null,
	dual_band: null,
	InitWAN: function()
	{
		this.inet1p = PXML.FindModule("INET.WAN-1");
		this.inet2p = PXML.FindModule("INET.WAN-2");
		var phyinfp = PXML.FindModule("PHYINF.WAN-1");
		
		if (!this.inet1p || !this.inet2p || !phyinfp)
		{
			BODY.ShowAlert("InitWAN() ERROR !");
			return false;
		}
		
		var inet1 = XG(this.inet1p+"/inf/inet");
		var inet2 = XG(this.inet2p+"/inf/inet");
		var eth = XG(phyinfp+"/inf/phyinf");
		this.inf1p = this.inet1p+"/inf";
		this.inf2p = this.inet2p+"/inf";
		this.inet1p = GPBT(this.inet1p+"/inet", "entry", "uid", inet1, false);
		this.inet2p = GPBT(this.inet2p+"/inet", "entry", "uid", inet2, false);
		phyinfp = GPBT(phyinfp, "phyinf", "uid", eth, false);
		this.macaddrp = phyinfp+"/macaddr";
		var macaddr = XG(this.macaddrp);
		
		/*initial wan type*/
		this.GetWanType();
		COMM_SetSelectValue(OBJ("wan_mode"), this.wanTypes[this.currentWanType]);
		
		/*initial settings*/
		/*Static IP*/
		OBJ("wiz_static_ipaddr").value = ResAddress(XG(this.inet1p+"/ipv4/ipaddr"));
		OBJ("wiz_static_mask").value = COMM_IPv4INT2MASK(XG(this.inet1p+"/ipv4/mask"));
		OBJ("wiz_static_gw").value = ResAddress(XG(this.inet1p+"/ipv4/gateway"));
		var ipv4_cnt = XG(this.inet1p+"/ipv4/dns/count");
		OBJ("wiz_static_dns1").value = ipv4_cnt>0 ? XG(this.inet1p+"/ipv4/dns/entry:1") : "0.0.0.0";
		OBJ("wiz_static_dns2").value = ipv4_cnt>1 ? XG(this.inet1p+"/ipv4/dns/entry:2") : "0.0.0.0";
		
		/*DHCPPLUS*/
		OBJ("wiz_dhcpplus_user").value = XG(this.inet1p+"/ipv4/dhcpplus/username");
		OBJ("wiz_dhcpplus_pass").value = XG(this.inet1p+"/ipv4/dhcpplus/password");
		
		/*PPPv4 hidden nodes*/
		OBJ("ppp4_timeout").value	= IdleTime(XG(this.inet1p+"/ppp4/dialup/idletimeout"));
		OBJ("ppp4_mode").value = XG(this.inet1p+"/ppp4/dialup/mode");
		OBJ("ppp4_mtu").value = XG(this.inet1p+"/ppp4/mtu");
		
		/*PPPoE*/
		OBJ("wiz_pppoe_usr").value = XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_pppoe_passwd").value = XG(this.inet1p+"/ppp4/password");
		
		/*PPTP*/
		OBJ("wiz_pptp_ipaddr").value = ResAddress(XG(this.inet2p+"/ipv4/ipaddr"));
		OBJ("wiz_pptp_mask").value = COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask"));
		OBJ("wiz_pptp_gw").value = ResAddress(XG(this.inet2p+"/ipv4/gateway"));
		OBJ("wiz_pptp_svr").value = ResAddress(XG(this.inet1p+"/ppp4/pptp/server"));
		OBJ("wiz_pptp_usr").value = XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_pptp_passwd").value = XG(this.inet1p+"/ppp4/password");
		var ppp4_cnt = XG(this.inet1p+"/ppp4/dns/count");
		OBJ("wiz_pptp_dns1").value = ppp4_cnt>0 ? XG(this.inet1p+"/ppp4/dns/entry:1") : "";
		OBJ("wiz_pptp_dns2").value = ppp4_cnt>1 ? XG(this.inet1p+"/ppp4/dns/entry:2") : "";
		OBJ("wiz_pptp_mac").value = XG(this.macaddrp);
		
		/*L2TP*/
		OBJ("wiz_l2tp_ipaddr").value = ResAddress(XG(this.inet2p+"/ipv4/ipaddr"));
		OBJ("wiz_l2tp_mask").value = COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask"));
		OBJ("wiz_l2tp_gw").value = ResAddress(XG(this.inet2p+"/ipv4/gateway"));
		OBJ("wiz_l2tp_svr").value = ResAddress(XG(this.inet1p+"/ppp4/l2tp/server"));
		OBJ("wiz_l2tp_usr").value = XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_l2tp_passwd").value = XG(this.inet1p+"/ppp4/password");
		var ppp4_cnt = XG(this.inet1p+"/ppp4/dns/count");
		OBJ("wiz_l2tp_dns1").value = ppp4_cnt>0 ? XG(this.inet1p+"/ppp4/dns/entry:1") : "";
		OBJ("wiz_l2tp_dns2").value = ppp4_cnt>1 ? XG(this.inet1p+"/ppp4/dns/entry:2") : "";
		OBJ("wiz_l2tp_mac").value = XG(this.macaddrp);
		
		if (XG(this.inet2p+"/ipv4/static")=="1")
		{
			document.getElementsByName("wiz_pptp_conn_mode")[1].checked = true;
			document.getElementsByName("wiz_l2tp_conn_mode")[1].checked = true;
		}
		else
		{
			document.getElementsByName("wiz_pptp_conn_mode")[0].checked = true;
			document.getElementsByName("wiz_l2tp_conn_mode")[0].checked = true;
		}
		
		this.OnChangePPTPMode();
		this.OnChangeL2TPMode();
		
		return true;
	},
	InitWLAN: function()
	{
		this.wlanbase = PXML.FindModule("WIFI.PHYINF");
		this.phyinf = GPBT(this.wlanbase, "phyinf", "uid", "BAND24G-1.1", false);
		var wifi_profile1 = XG(this.phyinf+"/wifi");
		this.wifip = GPBT(this.wlanbase+"/wifi", "entry", "uid", wifi_profile1, false);
		
		if (!this.wifip)
		{
			BODY.ShowAlert("InitWLAN() ERROR !");
			return false;
		}
		
		this.randomkey = RandomHex(10);
		OBJ("wiz_ssid").value = XG(this.wifip+"/ssid");
		/*Show the default Pre-Shared Key if the default security mode is WPA+2PSK.*/
		if(XG(this.wifip+"/authtype")=="WPA+2PSK" || XG(this.wifip+"/authtype")=="WPA2PSK" || XG(this.wifip+"/authtype")=="WPAPSK")
		{	
			OBJ("wiz_key").value=XG(this.wifip+"/nwkey/psk/key");
		}	
		
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if(this.dual_band)
		{
			this.phyinf2 = GPBT(this.wlanbase, "phyinf", "uid", "BAND5G-1.1", false);
			var wifi_profile2 = XG(this.phyinf2+"/wifi");
			this.wifip2 = GPBT(this.wlanbase+"/wifi", "entry", "uid", wifi_profile2, false);
			if (!this.wifip2)
			{
				BODY.ShowAlert("InitWLAN() ERROR !");
				return false;
			}
			/*Show the default Pre-Shared Key if the default security mode is WPA+2PSK.*/
			if(XG(this.wifip2+"/authtype")=="WPA+2PSK" || XG(this.wifip2+"/authtype")=="WPA2PSK" || XG(this.wifip2+"/authtype")=="WPAPSK")
			{	
				OBJ("wiz_key_Aband").value=XG(this.wifip2+"/nwkey/psk/key");
			}
			OBJ("wiz_ssid_Aband").value = XG(this.wifip2+"/ssid"); 
			OBJ("div_ssid_A").style.display = "block"; 
		}
		else
			OBJ("wifi24_name_pwd_show").innerHTML	= '<?echo I18N("j","Give your Wi-Fi network a name.");?>';
		return true;
	},
	PreWAN: function()
	{
		var type = this.wanTypes[this.currentWanType];
		var cnt = 0;
		XD(this.inet1p+"/ipv4");
		XD(this.inet1p+"/ppp4");
		XS(this.inf1p+"/lowerlayer", "");
		XS(this.inf1p+"/upperlayer", "");
		XS(this.inf1p+"/schedule", "");
		XS(this.inf1p+"/child", "");
		XS(this.inf2p+"/lowerlayer", "");
		XS(this.inf2p+"/upperlayer", "");
		XS(this.inf2p+"/schedule", "");
		XS(this.inf2p+"/active", 0);
		XS(this.inf2p+"/defaultroute", "");
		XS(this.inf2p+"/nat", "");
		
		switch (type)
		{
			case "STATIC":
				XS(this.inet1p+"/addrtype", "ipv4");
				XS(this.inet1p+"/ipv4/static", "1");
				XS(this.inet1p+"/ipv4/ipaddr", OBJ("wiz_static_ipaddr").value);
				XS(this.inet1p+"/ipv4/mask", COMM_IPv4MASK2INT(OBJ("wiz_static_mask").value));
				XS(this.inet1p+"/ipv4/gateway", OBJ("wiz_static_gw").value);
				XS(this.inet1p+"/ipv4/mtu", "1500"); /*default*/
				SetDNSAddress(this.inet1p+"/ipv4/dns", OBJ("wiz_static_dns1").value, OBJ("wiz_static_dns2").value);
				break;
			case "DHCP":
				XS(this.inet1p+"/addrtype", "ipv4");
				XS(this.inet1p+"/ipv4/dhcpplus/enable", "0");
				XS(this.inet1p+"/ipv4/static", "0");
				XS(this.inet1p+"/ipv4/mtu", "1500");
				SetDNSAddress(this.inet1p+"/ipv4/dns", "", "");
				break;
			case "DHCPPLUS":
				XS(this.inet1p+"/ipv4/dhcpplus/enable", "1");
				XS(this.inet1p+"/ipv4/dhcpplus/username", OBJ("wiz_dhcpplus_user").value);
				XS(this.inet1p+"/ipv4/dhcpplus/password", OBJ("wiz_dhcpplus_pass").value);
				break;
			case "PPPoE":
				/*use dynamic*/
				XS(this.inet1p+"/addrtype", "ppp4");
				XS(this.inet1p+"/ppp4/over", "eth");
				XS(this.inet1p+"/ppp4/static", "0");
				XS(this.inet1p+"/ppp4/username", OBJ("wiz_pppoe_usr").value);
				XS(this.inet1p+"/ppp4/password", OBJ("wiz_pppoe_passwd").value);
				XD(this.inet1p+"/ppp4/ipaddr");
				XS(this.inet1p+"/ppp4/mppe/enable",	"0");
				XS(this.inet1p+"/ppp4/dns/count", "0");
				XS(this.inet1p+"/ppp4/dns/entry:1","");
				XS(this.inet1p+"/ppp4/dns/entry:2","");
				break;
			case "PPTP":
				var dynamic_pptp = document.getElementsByName("wiz_pptp_conn_mode")[0].checked ? true: false;
				XS(this.inf2p+"/active", "1");
				XS(this.inet2p+"/nat", "");
				XS(this.inet1p+"/addrtype", "ppp4");
				XS(this.inet1p+"/ppp4/over", "pptp");
				XS(this.inet1p+"/ppp4/static", "0");
				XS(this.inet1p+"/ppp4/username", OBJ("wiz_pptp_usr").value);
				XS(this.inet1p+"/ppp4/password", OBJ("wiz_pptp_passwd").value);
				XS(this.inet1p+"/ppp4/pptp/server", OBJ("wiz_pptp_svr").value);
				
				cnt = 0;
				if (dynamic_pptp) /*dynamic*/
				{
					XS(this.inet2p+"/ipv4/static", "0");
					
					if (OBJ("wiz_pptp_dns1").value !== "")
					{
						XS(this.inet1p+"/ppp4/dns/entry:1", OBJ("wiz_pptp_dns1").value);
						cnt++;
					}
					else XS(this.inet1p+"/ppp4/dns/entry:1", "");
					if (OBJ("wiz_pptp_dns2").value !== "")
					{
						XS(this.inet1p+"/ppp4/dns/entry:2", OBJ("wiz_pptp_dns2").value);
						cnt++;
					}
					XS(this.inet1p+"/ppp4/dns/count", cnt);
					
					XD(this.inet2p+"/ipv4/dns");
					XS(this.inet2p+"/ipv4/dns/count", "0");
				}
				else /*static*/
				{
					XS(this.inet2p+"/ipv4/static",	"1");
					XS(this.inet2p+"/ipv4/ipaddr", OBJ("wiz_pptp_ipaddr").value);
					XS(this.inet2p+"/ipv4/mask", COMM_IPv4MASK2INT(OBJ("wiz_pptp_mask").value));
					XS(this.inet2p+"/ipv4/gateway", OBJ("wiz_pptp_gw").value);
					
					if (OBJ("wiz_pptp_dns1").value==="")
					{
						BODY.ShowAlert('<?echo i18n("Invalid Primary DNS address .");?>');
						return null;
					}
					XS(this.inet2p+"/ipv4/dns/entry:1", OBJ("wiz_pptp_dns1").value);
					XS(this.inet1p+"/ppp4/dns/entry:1", OBJ("wiz_pptp_dns1").value);
					cnt++;
					if (OBJ("wiz_pptp_dns2").value!=="")
					{
						XS(this.inet2p+"/ipv4/dns/entry:2", OBJ("wiz_pptp_dns2").value);
						XS(this.inet1p+"/ppp4/dns/entry:2", OBJ("wiz_pptp_dns2").value);
						cnt++;
					}
					XS(this.inet2p+"/ipv4/dns/count", cnt);
					XS(this.inet1p+"/ppp4/dns/count", cnt);
				}
				XS(this.inet1p+"/ppp4/mppe/enable", "0");
				XS(this.macaddrp, OBJ("wiz_pptp_mac").value);

				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "");
				XS(this.inf1p+"/lowerlayer", "WAN-2");
				XS(this.inf2p+"/upperlayer", "WAN-1");
				break;
			case "L2TP":
				var dynamic_l2tp = document.getElementsByName("wiz_l2tp_conn_mode")[0].checked ? true: false;
				XS(this.inf2p+"/active", "1");
				XS(this.inet2p+"/nat", "");
				XS(this.inet1p+"/addrtype", "ppp4");
				XS(this.inet1p+"/ppp4/over", "l2tp");
				XS(this.inet1p+"/ppp4/static", "0");
				XS(this.inet1p+"/ppp4/username",	OBJ("wiz_l2tp_usr").value);
				XS(this.inet1p+"/ppp4/password",	OBJ("wiz_l2tp_passwd").value);
				XS(this.inet1p+"/ppp4/l2tp/server",	OBJ("wiz_l2tp_svr").value);
				
				cnt = 0;
				if (dynamic_l2tp) /*dynamic*/
				{
					XS(this.inet2p+"/ipv4/static", "0");
					if (OBJ("wiz_l2tp_dns1").value !== "")
					{
						XS(this.inet1p+"/ppp4/dns/entry:1", OBJ("wiz_l2tp_dns1").value);
						cnt++;
					}
					else XS(this.inet1p+"/ppp4/dns/entry:1", "");
					if (OBJ("wiz_l2tp_dns2").value !== "")
					{
						XS(this.inet1p+"/ppp4/dns/entry:2", OBJ("wiz_l2tp_dns2").value);
						cnt++;
					}
					XS(this.inet1p+"/ppp4/dns/count", cnt);
					
					XD(this.inet2p+"/ipv4/dns");
					XS(this.inet2p+"/ipv4/dns/count", "0");
				}
				else /*static*/
				{
					XS(this.inet2p+"/ipv4/static", "1");
					XS(this.inet2p+"/ipv4/ipaddr",	OBJ("wiz_l2tp_ipaddr").value);
					XS(this.inet2p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_l2tp_mask").value));
					XS(this.inet2p+"/ipv4/gateway",	OBJ("wiz_l2tp_gw").value);
					
					if (OBJ("wiz_l2tp_dns1").value==="")
					{
						BODY.ShowAlert('<?echo i18n("Invalid Primary DNS address .");?>');
						return null;
					}
					XS(this.inet2p+"/ipv4/dns/entry:1", OBJ("wiz_l2tp_dns1").value);
					XS(this.inet1p+"/ppp4/dns/entry:1", OBJ("wiz_l2tp_dns1").value);
					cnt++;
					if (OBJ("wiz_l2tp_dns2").value!=="")
					{
						XS(this.inet2p+"/ipv4/dns/entry:2", OBJ("wiz_l2tp_dns2").value);
						XS(this.inet1p+"/ppp4/dns/entry:2", OBJ("wiz_l2tp_dns2").value);
						cnt++;
					}
					XS(this.inet2p+"/ipv4/dns/count", cnt);
					XS(this.inet1p+"/ppp4/dns/count", cnt);
				}
				XS(this.macaddrp, OBJ("wiz_l2tp_mac").value);
				
				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "");
				XS(this.inf1p+"/lowerlayer", "WAN-2");
				XS(this.inf2p+"/upperlayer", "WAN-1");	
				break;
		}
		
		if (type=="DHCP" || type=="STATIC")
		{
			XS(this.inet2p+"/ipv4/static", "0");
			XS(this.inet2p+"/ipv4/ipaddr", "");
			XS(this.inet2p+"/ipv4/mask", "");
			XS(this.inet2p+"/ipv4/gateway", "");
		}
		else
		{
			/*PPPv4 hidden nodes*/
			XS(this.inet1p+"/ppp4/dialup/idletimeout", "5");
			XS(this.inet1p+"/ppp4/dialup/mode", "ondemand");
			if (type != "PPPoE")
				XS(this.inet1p+"/ppp4/mtu", "1400");
			else
				XS(this.inet1p+"/ppp4/mtu", "1492");
		}
		return true;
	},
	PreWLAN: function()
	{
		XS(this.wifip+"/ssid", OBJ("wiz_ssid").value);
		XS(this.wifip+"/ssidhidden", "0");
		XS(this.wifip+"/authtype", "WPA+2PSK");
		XS(this.wifip+"/encrtype", "TKIP+AES");
		XS(this.wifip+"/nwkey/psk/passphrase", "");
		XS(this.wifip+"/nwkey/psk/key", OBJ("wiz_key").value);
		XS(this.wifip+"/wps/configured", "1");
		XS(this.phyinf+"/active", "1");
		
		if(this.dual_band)
		{
			XS(this.wifip2+"/ssid", OBJ("wiz_ssid_Aband").value);			
			XS(this.wifip2+"/ssidhidden", "0");
			XS(this.wifip2+"/authtype", "WPA+2PSK");
			XS(this.wifip2+"/encrtype", "TKIP+AES");
			XS(this.wifip2+"/nwkey/psk/passphrase", "");
			XS(this.wifip2+"/nwkey/psk/key", OBJ("wiz_key_Aband").value);
			XS(this.wifip2+"/wps/configured", "1");
			XS(this.phyinf2+"/active", "1");
		}
		return true;
	},
	DRAWWAN: function()
	{
		var wan = PXML.FindModule("INET.WAN-1");
		var rwan = PXML.FindModule("RUNTIME.INF.WAN-1");
		var rphy = PXML.FindModule("RUNTIME.PHYINF");
		
		var waninetuid = XG(wan+"/inf/inet");
		var wanphyuid = XG(wan+"/inf/phyinf");
		this.waninetp = GPBT(wan+"/inet", "entry", "uid", waninetuid, false);
		this.rwaninetp = GPBT(rwan+"/runtime/inf", "inet", "uid", waninetuid, false);
		this.rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", wanphyuid, false);
		var str_networkstatus = str_Disconnected = "Disconnected";
		var str_Connected = "Connected";
		var str_wanipaddr = "0.0.0.0";
		var wan_network_status = 0;
		var str_wantype = null;
		
		if ((!this.waninetp))
		{
			BODY.ShowAlert("DRAWWAN() ERROR !");
			return false;
		}
		
		this.wancable_status = 0;
		if((XG(this.rwanphyp+"/linkstatus")!="0") && (XG(this.rwanphyp+"/linkstatus")!=""))
			this.wancable_status = 1;
		
		if (XG(this.waninetp+"/addrtype")=="ipv4")
		{
			if(XG(this.waninetp+"/ipv4/static")=="1") /*Static IP*/
			{
				str_wantype = "Static IP";
				str_networkstatus = this.wancable_status == 1 ? str_Connected : str_Disconnected;
				wan_network_status = this.wancable_status;
			}
			else /*DHCP*/
			{
				str_wantype = "DHCP";
				if ((XG(this.rwaninetp+"/ipv4/valid")=="1") && (this.wancable_status==1))
				{
					wan_network_status = 1;
					str_networkstatus = str_Connected;
				}
				else if (this.wancable_status==1)
				{
					wan_network_status = 1;
					str_networkstatus = "Connecting";
				}
			}
		}
		else if (XG(this.waninetp+"/addrtype")=="ppp4")
		{
			if (XG(this.waninetp+"/ppp4/over")=="eth")
				str_wantype = "PPPoE";
			else if (XG(this.waninetp+"/ppp4/over")=="pptp")
				str_wantype = "PPTP";
			else if (XG(this.waninetp+"/ppp4/over")=="l2tp")
				str_wantype = "L2TP";
			else
				str_wantype = "Unknow WAN type";
			
			var connStat = XG(rwan+"/runtime/inf/pppd/status");
			if ((XG  (this.rwaninetp+"/ppp4/valid")=="1")&& (this.wancable_status==1))
				wan_network_status=1;
			switch (connStat)
			{
				case "connected":
					if (wan_network_status == 1)
						str_networkstatus = str_Connected;
					else
						str_networkstatus = str_Disconnected;
					break;
				case "":
				case "disconnected":
					str_networkstatus = str_Disconnected;
					wan_network_status = 0;
					break;
				case "on demand":
					str_networkstatus = "Idle";
					wan_network_status = 0;
					break;
				default:
					str_networkstatus = "Busy";
					break;
			}
		}
		if ((XG(this.rwaninetp+"/addrtype")=="ipv4") && wan_network_status==1)
		{
			str_wanipaddr = XG(this.rwaninetp+"/ipv4/ipaddr");
		}
		else if ((XG(this.rwaninetp+"/addrtype") == "ppp4")&& wan_network_status==1)
		{
			str_wanipaddr = XG(this.rwaninetp+"/ppp4/local");
		}
		
		if(str_wantype!=this.old_wantype)
		{
			OBJ("st_wantype").innerHTML  = str_wantype;
		}

		if (str_networkstatus!=this.old_networkstatus || this.old_wanipaddr!=str_wanipaddr)
		{
			if (str_networkstatus == str_Connected)
			{
				OBJ("connect").src = "/pic/line1.jpg";
				OBJ("internet").src = "/pic/earth.jpg";
				OBJ("st_wanipaddr").innerHTML = "IP:"+str_wanipaddr;
			}
			else
			{
				OBJ("connect").src = "/pic/line2.jpg";
				OBJ("internet").src = "/pic/earth_no.jpg";
			}
		}
		
		this.old_wantype = str_wantype;
		this.old_networkstatus = str_networkstatus;
		this.old_wanipaddr = str_wanipaddr;
		
		return true;
	},
	DRAWLAN: function()
	{
		var lan = PXML.FindModule("INET.LAN-1");
		var inetuid = XG  (lan+"/inf/inet");
		this.inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);
		
		if (!this.inetp)
		{
			BODY.ShowAlert("DRAWLAN() ERROR !");
			return false;
		}
		
		if (XG(this.inetp+"/addrtype") == "ipv4")
		{
			var b = this.inetp+"/ipv4";
			this.lanip = XG  (b+"/ipaddr");
		}

		if(this.old_ipaddr!=this.lanip)
		{
		var uid = "map_1";
		var computer = '<a href="./st_device.php"><img src="/pic/computer.jpg" width="134"/></a></br><center>LAN IP:'+this.lanip+'</center>';
		var line = '<img src="/pic/line1.jpg" width="114"/>';
		var router = '<a href="./st_device.php"><img src="/pic/router.jpg" width="134" id="router" /></a></br><center><?echo I18N("j","INTERNET");?></center><center><span class="value" id="st_wanipaddr"></span></center>';
		var connect = '<img src="" width="114" id="connect"/>';
		var internet = '<a href="./bsc_wan.php"><img src="" width="134" id="internet" /></a><center><span class="value" id="st_wantype"></span></center>';
		var data = [computer, line , router, connect, internet];
		var type = ["img", "img", "img", "img", "img"];
		BODY.InjectTable("map", uid, data, type);
		}
		
		this.old_ipaddr = this.lanip;

		return true;
	},
	ShowCurrentStage: function()
	{
		var i = 0;
		var type = "";
		for (i=0; i<this.wanTypes.length; i++)
		{
			type = this.wanTypes[i];
			OBJ(type).style.display = "none";
		}
		
		for (i=0; i<this.stages.length; i++)
		{
			if (i == this.currentStage)
			{
				if (this.stages[i] == "stage_set")
					OBJ("internet_status").style.display = "block";
				else
					OBJ("internet_status").style.display = "none";
				
				OBJ(this.stages[i]).style.display = "block";
				
				type = this.wanTypes[this.currentWanType];
				OBJ(type).style.display = "block";
			}
			else
				OBJ(this.stages[i]).style.display = "none";
		}
		if (this.stages[this.currentStage]=="stage_set")
		{
			if(this.refresh_timer == null) PAGE.RefreshStatus();
		}
		else if (this.stages[this.currentStage]=="stage_no_cable")
		{
			DisplayTAButton("block");
		}
		else if (this.stages[this.currentStage]=="stage_check_connect")
		{
			PAGE.DrawBar();
			setTimeout('PAGE.CheckInternet()', 10*1000);
		}
	},
	SetStage: function(offset)
	{
		var length = this.stages.length;
		switch (offset)
		{
			case 3:
				if (this.currentStage < length-1)
					this.currentStage += 3;
				break;
			case 2:
				if (this.currentStage < length-1)
					this.currentStage += 2;
				break;
			case 1:
				if (this.currentStage < length-1)
					this.currentStage += 1;
				break;
			case 0:
				if (this.currentStage < length-1)
					this.currentStage = this.currentStage;
				break;
		case -1:
			if (this.currentStage > 0)
				this.currentStage -= 1;
			break;
		case -2:
			if (this.currentStage > 0)
				this.currentStage -= 2;
		case -3:
			if (this.currentStage > 0)
				this.currentStage -= 3;
			break;
		}
	},
	GetWanType: function()
	{
		var addrtype = XG(this.inet1p+"/addrtype");
		var type = null;
		switch (addrtype)
		{
			case "ipv4":
				if (XG(this.inet1p+"/ipv4/static")==="1")
					type = "STATIC";
				else
				{
					if (XG(this.inet1p+"/ipv4/dhcpplus/enable")==="1")
						type = "DHCPPLUS";
					else
						type = "DHCP";
				}
				break;
			case "ppp4":
				var over = XG(this.inet1p+"/ppp4/over");
				if (over==="eth")
					type = "PPPoE";
				else if (over==="pptp")
					type = "PPTP";
				else if (over==="l2tp")
					type = "L2TP";
				break;
			default:
				BODY.ShowAlert("Internal Error !");
		}
		for (var i=0; i<this.wanTypes.length; i++)
		{
			if (this.wanTypes[i]==type)
				this.currentWanType = i;
		}
	},
	OnClickMacButton: function(objname)
	{
		OBJ(objname).value = "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
		if(OBJ(objname).value == "")
			alert("Can't find Your PC's MAC Address, please enter Your MAC manually.");
	},
	OnClickConnectButton: function()
	{
		if (this.stages[this.currentStage]=="stage_set")
		clearTimeout(this.refresh_timer);
		
		var type = this.wanTypes[this.currentWanType];
		if (!this.CheckSetValue(type)) return null;
		
    if (!COMM_IsDirty(false))
    {
    	BODY.ShowAlert('<?echo I18N("j","Settings have not changed.");?>');
    	return null;
    }

    PAGE.RegetXML_sync();
	  
		if((XG(this.rwanphyp+"/linkstatus")!="0") && (XG(this.rwanphyp+"/linkstatus")!=""))
			this.wancable_status = 1;
		else
			this.wancable_status = 0;
			
		if (this.wancable_status==1)
		{
			this.saveonly = false;
			if (!this.SaveXML()) return null;
		}
		else if (this.wancable_status==0 && this.stages[this.currentStage]==="stage_no_cable")
		{
			PAGE.SetStage(0);
			PAGE.ShowCurrentStage();
		}
		else
		{
			PAGE.SetStage(2);
			PAGE.ShowCurrentStage();
		}
	},
	OnClickSLLink: function()
	{
		var type = this.wanTypes[this.currentWanType];
		this.saveonly = true;
		
		if (!this.CheckSetValue(type)) return null;
		CheckSettings(this.stages[this.currentStage]);
		PAGE.PreSubmit();
		
		var xml = PXML.doc;
		PXML.UpdatePostXML(xml);
		PXML.Post(
		function(code, result)
		{
			if(code != "OK") BODY.ShowAlert('<?echo I18N("h","Settings error, please check it again!!");?>');
			BODY.Logout();
		});
	},
	OnClickANSLink: function()
	{
		self.location.href = "./bsc_internet.php";
	},
	OnClickCCButton: function()
	{
		self.location.href = "http://www.dlink.com/";
	},
	OnClickTAButton: function()
	{
		DisplayTAButton("none");
    setTimeout('DisplayTAButton("block")', 5*1000);
    PAGE.RegetXML_sync();
    
		if((XG(this.rwanphyp+"/linkstatus")!="0") && (XG(this.rwanphyp+"/linkstatus")!=""))
			this.wancable_status = 1;
		else
			this.wancable_status = 0;
		
		if (this.wancable_status==1)
		{
			this.saveonly = false;
			if (!this.SaveXML()) return null;
		}
		else if (this.wancable_status==0 && this.stages[this.currentStage]==="stage_no_cable")
		{
			setTimeout('PAGE.SetStage(0);PAGE.ShowCurrentStage();', 1000);
		}
		else
		{
			PAGE.SetStage(2);
			PAGE.ShowCurrentStage();
		}
	},
	OnClickPreviousButton: function()
	{
		self.location.href = "./bsc_easysetup.php";
	},
	OnChangeWanType: function(type)
	{
		for (var i=0; i<this.wanTypes.length; i++)
		{
			if (this.wanTypes[i]==type)
				this.currentWanType = i;
		}
		this.ShowCurrentStage();
	},
	OnChangePPTPMode: function()
	{
		var disable = document.getElementsByName("wiz_pptp_conn_mode")[0].checked ? true: false;
		OBJ("wiz_pptp_ipaddr").disabled = disable;
		OBJ("wiz_pptp_mask").disabled = disable;
		OBJ("wiz_pptp_gw").disabled = disable;
		OBJ("wiz_pptp_dns1").disabled = disable;
		OBJ("wiz_pptp_dns2").disabled = disable;
	},
	OnChangeL2TPMode: function()
	{
		var disable = document.getElementsByName("wiz_l2tp_conn_mode")[0].checked ? true: false;
		OBJ("wiz_l2tp_ipaddr").disabled = disable;
		OBJ("wiz_l2tp_mask").disabled = disable;
		OBJ("wiz_l2tp_gw").disabled = disable;
		OBJ("wiz_l2tp_dns1").disabled = disable;
		OBJ("wiz_l2tp_dns2").disabled = disable;
	},
	DrawBar: function()
	{
		PAGE.drawbar++;
		if(PAGE.drawbar>=50) PAGE.drawbar=0;
		RefreshBar(PAGE.drawbar);
		setTimeout('PAGE.DrawBar()', 1000);
	},
	CheckInternet: function()
	{
		if(PAGE.internetCheck%4==0)
			var action="ping";
		else
			var action="pingreport";
		
		var dst = "www.dlink.com";
		PAGE.internetCheck++;
		
		var ajaxObj = GetAjaxObj("InternetCheck");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			if(xml.Get("/diagnostic/report")=="www.dlink.com is alive!")
			{
				PAGE.SetStage(3);
				PAGE.ShowCurrentStage();
			}
			else if(PAGE.internetCheck>=23) /*do 6 ping check */
			{
				if(PAGE.wanTypes[PAGE.currentWanType]=="PPPoE")
				{
					PAGE.SetStage(2);
					PAGE.ShowCurrentStage();
				}
				else
				{
					if (confirm('<?echo I18N("j","The internet check is failed. Do you want to restart the wizard again?");?>')) 
						self.location.href="bsc_easysetup.php";
				}
			}	
			else setTimeout('PAGE.CheckInternet()', 2000);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("diagnostic.php", "act="+action+"&dst="+dst);
	},
	RefreshStatus: function()
	{
		if(AUTH.AuthorizedGroup>=0)
		{
			PAGE.RegetXML();
			PAGE.DRAWLAN();
			PAGE.DRAWWAN();
			this.refresh_timer = setTimeout('PAGE.RefreshStatus()', 5*1000);
		}
	},
	RegetXML: function()
	{
		COMM_GetCFG(
			false,
			PAGE.services,
			function(xml) {
				PXML.doc = xml;
			}
		);
	},
	RegetXML_sync: function()
  {
		sync_GetCFG(
			false,
			PAGE.services,
			function(xml) {
				PXML.doc = xml;
    	}
		);
	},
	SaveXML: function()
	{
    CheckSettings(this.stages[this.currentStage]);
    PAGE.PreSubmit();
    
		var xml = PXML.doc;
		PXML.UpdatePostXML(xml);
		PXML.Post(function(code, result){PAGE.OnSubmitCallback(code,result);});
		
		return true;
	},
	CheckSetValue: function(type)
	{
		/* check wan settings */
		switch (type)
		{
			case "STATIC":
				if(OBJ("wiz_static_ipaddr").value==="" || OBJ("wiz_static_mask").value==="" || 
				OBJ("wiz_static_gw").value==="" || OBJ("wiz_static_dns1").value==="")
				{
					BODY.ShowAlert('<?echo I18N("j","* is required field.");?>');
					return false;
				}
				
				var st_ip = OBJ("wiz_static_ipaddr").value;
				if(!CheckIpValidity(st_ip))
				{
					BODY.ShowAlert('<?echo I18N("j","Invalid IP address");?>');
					OBJ("wiz_static_ipaddr").focus();
					return false;
				}
				if (OBJ("wiz_static_dns1").value === "0.0.0.0")
				{
					BODY.ShowAlert('<?echo I18N("j","Invalid Primary DNS address.");?>');
					OBJ("wiz_static_dns1").focus();
					return false;
				}
				break;
			case "DHCPPLUS":
				if(OBJ("wiz_dhcpplus_user").value==="")
				{
					BODY.ShowAlert('<?echo I18N("j","The user name can not be empty");?>');
					return false;
				}
				if(OBJ("wiz_dhcpplus_pass").value==="")
				{
					BODY.ShowAlert('<?echo I18N("j","The user password can not be empty");?>');
					return false;
				}
				break;
			case "PPPoE":
				if(OBJ("wiz_pppoe_usr").value=="") 
				{
					BODY.ShowAlert('<?echo I18N("j","The user name can not be empty");?>');			
					return false;
				}
				if(OBJ("wiz_pppoe_passwd").value=="")
				{
					BODY.ShowAlert('<?echo I18N("j","The user password can not be empty");?>');			
					return false;
				}
				break;
			case "PPTP":
				if(OBJ("wiz_pptp_ipaddr").value=="" || OBJ("wiz_pptp_mask").value=="" || OBJ("wiz_l2tp_gw").value=="" || OBJ("wiz_pptp_svr").value=="") 
				{
					BODY.ShowAlert('<?echo I18N("j","* is required field.");?>');			
					return false;
				}
				if(OBJ("wiz_pptp_usr").value=="") 
				{
					BODY.ShowAlert('<?echo I18N("j","The user name can not be empty");?>');			
					return false;
				}
				if(OBJ("wiz_pptp_passwd").value=="")
				{
					BODY.ShowAlert('<?echo I18N("j","The user password can not be empty");?>');			
					return false;
				}
				if (document.getElementsByName("wiz_pptp_conn_mode")[1].checked == true)
				{
					if(OBJ("wiz_pptp_dns1").value=="" || OBJ("wiz_pptp_dns1").value=="0.0.0.0")
					{
						BODY.ShowAlert('<?echo I18N("j","Invalid Primary DNS address.");?>');			
						return false;
					}
				}
				break;
			case "L2TP":
				if(OBJ("wiz_l2tp_ipaddr").value=="" || OBJ("wiz_l2tp_mask").value=="" || OBJ("wiz_l2tp_gw").value=="" || OBJ("wiz_l2tp_svr").value=="") 
				{
					BODY.ShowAlert('<?echo I18N("j","* is required field.");?>');			
					return false;
				}
				if(OBJ("wiz_l2tp_usr").value=="") 
				{
					BODY.ShowAlert('<?echo I18N("j","The user name can not be empty");?>');			
					return false;
				}
				if(OBJ("wiz_l2tp_passwd").value=="")
				{
					BODY.ShowAlert('<?echo I18N("j","The user password can not be empty");?>');			
					return false;
				}
				if (document.getElementsByName("wiz_l2tp_conn_mode")[1].checked == true)
				{
					if(OBJ("wiz_l2tp_dns1").value=="" || OBJ("wiz_l2tp_dns1").value=="0.0.0.0")
					{
						BODY.ShowAlert('<?echo I18N("j","Invalid Primary DNS address.");?>');			
						return false;
					}
				}
				break;
		}
		
		/* check wifi settings */
		if (OBJ("wiz_key").value.length < 8)
		{
			BODY.ShowAlert('<?echo I18N("j","Incorrect key length, should be 8 to 63 characters long.");?>');
			OBJ("wiz_key").focus();
			return false;
		}
		if(this.dual_band && OBJ("wiz_key_Aband").value.length < 8)
		{
			BODY.ShowAlert('<?echo I18N("j","Incorrect key length, should be 8 to 63 characters long.");?>');
			OBJ("wiz_key_Aband").focus();
			return false;
		}
		if(OBJ("wiz_key").value.indexOf(" ") >= 0)
		{
			BODY.ShowAlert('<?echo I18N("j","Password can not contain blank.");?>');
			OBJ("wiz_key").focus();
			return false;
		}
		if(this.dual_band && OBJ("wiz_key_Aband").value.indexOf(" ") >= 0)
		{
			BODY.ShowAlert('<?echo I18N("j","Password can not contain blank.");?>');
			OBJ("wiz_key_Aband").focus();
			return false;
		}
		
		
		var obj_wiz_ssid 	= OBJ("wiz_ssid").value;
		if(obj_wiz_ssid.charAt(0)===" "|| obj_wiz_ssid.charAt(obj_wiz_ssid.length-1)===" ")
		{
			alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
			return false;
		}
		
		if(this.dual_band)
		{
			obj_wiz_ssid 	= OBJ("wiz_ssid_Aband").value;
			if(obj_wiz_ssid.charAt(0)===" "|| obj_wiz_ssid.charAt(obj_wiz_ssid.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
				return false;
			}
		}
		return true;
	}
	
	
	
}
function DisplayTAButton(dis)
{
	OBJ("ta_button").style.display = dis;
}
function ResAddress(address)
{
	if (address=="")
		return "0.0.0.0";
	else if (address=="0.0.0.0")
		return "";
	else
		return address;
}
function SetDNSAddress(path, dns1, dns2)
{
	var cnt = 0;
	var dns = new Array (false, false);
	if (dns1!="0.0.0.0"&&dns1!="") {dns[0] = true; cnt++;}
	if (dns2!="0.0.0.0"&&dns2!="") {dns[1] = true; cnt++;}
	XS(path+"/count", cnt);
	if (dns[0]) XS(path+"/entry", dns1);
	if (dns[1]) XS(path+"/entry:2", dns2);
}
function RandomHex(len)
{
	var c = "0123456789abcdef";
	var str = '';
	for (var i = 0; i < len; i+=1)
	{
		var rand_char = Math.floor(Math.random() * c.length);
		str += c.substring(rand_char, rand_char + 1);
	}
	return str;
}

function CheckSettings(current_stage)
{
	PXML.IgnoreModule("WAN");                                
	PXML.IgnoreModule("INET.LAN-1");                         
	PXML.IgnoreModule("RUNTIME.INF.LAN-1");                  
	PXML.IgnoreModule("RUNTIME.INF.WAN-1");                             
	PXML.IgnoreModule("RUNTIME.PHYINF");                     
	PXML.CheckModule("INET.WAN-1", null, "ignore", "ignore");
	PXML.CheckModule("INET.WAN-2", null, "ignore", "ignore");

	//if(!saveonly) CallHedwig(PXML.doc, current_stage);
}
function sync_GetCFG(Cache, Services, Handler)
{
	var ajaxObj = GetAjaxObj("getData");
	var payload = "";
  ajaxObj.requestAsyn=false;
  
	if (Cache) payload = "CACHE=true";
	if (payload!="") payload += "&";
	payload += "SERVICES="+escape(COMM_EatAllSpace(Services));

	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (Handler!=null) Handler(xml);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("getcfg.php", payload);
}
function CallHedwig(doc, current_stage)
{
	COMM_CallHedwig(doc, 
		function (xml)
		{
			switch (xml.Get("/hedwig/result"))
			{
				case "OK":
					if(current_stage==="stage_set") PAGE.SetStage(1);
					else PAGE.SetStage(0);

					PAGE.ShowCurrentStage();
					break;
				case "FAILED":
					BODY.ShowAlert(xml.Get("/hedwig/message"));
					break;
			}
		}
	);
}
function CheckIpValidity(ipstr)
{
	var vals = ipstr.split(".");
	if (vals.length!=4) 
		return false;
	
	for (var i=0; i<4; i++)
	{
		if (!TEMP_IsDigit(vals[i]) || vals[i]>255)
			return false;
	}
	return true;
}
function IdleTime(value)
{
	if (value=="")
		return "0";
	else
		return parseInt(value, 10);
}
function RefreshBar(index)
{
	var bar_color = "#FF6F00";
	var clean_color = "#d4d4d4";
	var clear = "&nbsp;&nbsp;"

	if(index==0)
	{
		for (i = 0; i <= 50; i++)
		{
			var block = OBJ("block" + i);
			block.style.backgroundColor = clean_color;
		}
	}
	
	for (i = 0; i <= index; i++)
	{
		var block = OBJ("block" + i);
		block.innerHTML = clear;
		block.style.backgroundColor = bar_color;
	}
}
</script>
