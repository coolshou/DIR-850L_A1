<?include "/htdocs/phplib/inet.php";?>

<?include "/htdocs/phplib/lang.php";?>
<? $langcode = wiz_set_LANGPACK();?>
<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
div p.wiz_strong
{
	margin-left:46px;
	color: #13376B;
	font-weight: bold;
}
div span.wiz_input
{
	width: 35%;
	font-weight: bold;
	margin-left:50px;
	margin-top: 4px;
}
div span.wiz_input_script
{
	width: 61%;
	margin-left:5px;
	margin-top: 4px;
}	
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "DEVICE.ACCOUNT,DEVICE.TIME,DEVICE.HOSTNAME,PHYINF.WAN-1,INET.WAN-1,INET.WAN-2,INET.WAN-3,INET.WAN-4,WAN,WIFI.PHYINF,PHYINF.WIFI",
	logindefault: 0,
	OnLoad: function()
	{
		var confsize = <?echo query("/runtime/device/devconfsize");?>
		if(confsize==0 && this.logindefault == 0)
		{
			AUTH.AuthorizedGroup = -1;
			AUTH.Login(
				function login_callback()
				{
					
					if(AUTH.AuthorizedGroup>=0)/*login success*/
					{
						
						BODY.GetCFG();
					}
					else /*login fail show login page.*/
					{
						alert("password is not default(Admin/blank),should not run here.");
						BODY.OnLoad();
					}
				},
				"Admin","");
			
			this.logindefault=1;
		}
	},
	OnUnload: function() {},
	ShowSavingMessage: function() {},
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
			case "OK":
				for(var i=0; i < this.stages.length; i++) if(this.stages[i]==="check_wan_connect") this.currentStage=i;
				PAGE.ShowCurrentStage();
				return true;
				break;
			default : 
				this.currentStage--;
				this.ShowCurrentStage();
				return false;
		}
	},
	InitValue: function(xml)
	{
		/*Enter wizard without login when it is factory default.*/
		PXML.doc = xml;
		if (!this.Initial()) return false;
		if (!this.InitTZ()) return false;
		if (!this.InitWANSettings()) return false;
		if (!this.InitWLAN()) return false;
		this.ShowCurrentStage();
		return true;
	},
	PreSubmit: function()
	{
		if(!(/Safari/.test(navigator.userAgent) && !/Chrome/.test(navigator.userAgent))) //Don't show add book mark message in Safari browser.
		{
			if(confirm('<?echo i18n("Do you want to bookmark \"D-Link Router Web Management\"?");?>'))
				addBookmarkForBrowser("D-Link Router Web Management","http://192.168.0.1/");
		}
		PXML.ActiveModule("DEVICE.ACCOUNT");
		PXML.ActiveModule("DEVICE.TIME");
		PXML.ActiveModule("DEVICE.HOSTNAME");
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		PXML.CheckModule("INET.WAN-2", null, null, "ignore");
		PXML.CheckModule("PHYINF.WAN-1", null, null, "ignore");
		PXML.CheckModule("WAN", null, "ignore", null);
		
		if (!this.PreTZ()) return null;
		if (!this.PreWLAN()) return null;
		
		PXML.CheckModule("WIFI.PHYINF", null, null, "ignore");
		PXML.IgnoreModule("PHYINF.WIFI");
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="")	$bt=30;
		else			$bt=$bt+10;
		echo $bt;
	?>,
	dual_band: null,
	passwdp: null,
	tzp: null,
	hostp: null,
	inet1p: null,
	inet2p: null,
	inet3p: null,
	inet4p: null,
	inf1p: null,
	inf2p: null,
	inf3p: null,
	inf4p: null,
	macaddrp: null,
	operatorp: null,
	wifip: null,
	wifip2: null,
	wlanbase: null,
	phyinf: null,
	phyinf2: null,
	randomkey: null,
	stages: new Array ("stage_desc", "stage_wan_detect","stage_ether", "stage_ether_cfg", "stage_wlan_set", "stage_passwd", "stage_tz", "stage_wlan_result", "check_wan_connect", "check_wan_connect_bar"),
	wanTypes: new Array ("DHCP", "DHCPPLUS", "PPPoE", "PPTP", "L2TP", "STATIC", "R_PPTP", "R_L2TP", "R_PPPoE"),
	wanDetectCheckNum: 0,
	wanDetectNum: 0,
	wanDetectResult: null,
	wanDetectCheckTimer:null,
	wanDetectTimer:null,
	currentStage: 0,	// 0 ~ this.stages.length
	currentWanType: 0,	// 0 ~ this.wanTypes.length
	internetCheck:0 ,
	Initial: function()
	{
		this.passwdp = PXML.FindModule("DEVICE.ACCOUNT");
		if (!this.passwdp)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.captcha = this.passwdp + "/device/session/captcha";
		this.passwdp = GPBT(this.passwdp+"/device/account", "entry", "name", "Admin", false);
		this.passwdp += "/password";
		OBJ("wiz_passwd").value = OBJ("wiz_passwd2").value = XG(this.passwdp);
		OBJ("en_captcha").checked = COMM_EqBOOL(XG(this.captcha), true);
		
		return true;
	},
	InitTZ: function()
	{
		this.tzp = PXML.FindModule("DEVICE.TIME");
		if (!this.tzp)
		{
			BODY.ShowAlert("InitTZ() ERROR!!!");
			return false;
		}
		this.tzp += "/device/time/timezone";
	
		//Auto Timezone settings
		var date_no_daylightsaving = new Date(2011, 2, 12, 1, 0, 0, 0);// The date without day light saving around the world
		var time_offset = date_no_daylightsaving.getTimezoneOffset();
		var time_offset_hour = (time_offset > 0)?Math.floor(time_offset/60):Math.ceil(time_offset/60);
		var time_offset_minute = Math.abs(time_offset%60);
		var time_offset_string = "";

		if(time_offset_hour > 9) time_offset_string = "+" + time_offset_hour.toString() + ":";
		else if(time_offset_hour <= 9 && time_offset_hour >= 0) time_offset_string = "+0" + time_offset_hour.toString() + ":";
		else if(time_offset_hour < -9) time_offset_string = "-" + time_offset_hour.toString().substr(1,2) + ":";
		else time_offset_string = "-0" + time_offset_hour.toString().substr(1,1) + ":";
		if(time_offset_minute !== 0) time_offset_string = time_offset_string + time_offset_minute.toString();
		else time_offset_string = time_offset_string + "0" + time_offset_minute.toString();
		/*
		Timezone may represents more than one country.
		So each timezone selects only one country into popular_timezone array according to country's population.
		In other words, a country has more people, it may have lots of chance to be added into array.
		*/
		var popular_timezone=
		[
			"GMT+12:00_Index=1","GMT+11:00_Index=2","GMT+10:00_Index=3",
			"GMT+09:00_Index=4","GMT+08:00_Index=5","GMT+07:00_Index=7",
			"GMT+06:00_Index=9","GMT+05:00_Index=14","GMT+04:30_Index=15",
			"GMT+04:00_Index=17","GMT+03:30_Index=19","GMT+03:00_Index=20",
			"GMT+02:00_Index=23","GMT+01:00_Index=24","GMT+00:00_Index=27",
			"GMT-01:00_Index=28","GMT-02:00_Index=33","GMT-03:00_Index=39",
			"GMT-03:30_Index=42","GMT-04:00_Index=45","GMT-04:30_Index=46",
			"GMT-05:00_Index=48","GMT-05:30_Index=49","GMT-05:45_Index=50",
			"GMT-06:00_Index=47","GMT-06:30_Index=54","GMT-07:00_Index=55",
			"GMT-08:00_Index=61","GMT-09:00_Index=62","GMT-09:30_Index=65",
			"GMT-10:00_Index=68","GMT-11:00_Index=71","GMT-12:00_Index=73",
			"GMT-13:00_Index=75"
		];
		var timezone_index = "";
		for(var i=1; i < popular_timezone.length; i++)
		{
			if(time_offset_string===popular_timezone[i].substr(3,6))
			{
				if(popular_timezone[i].length===17) timezone_index = popular_timezone[i].substr(16,1);
				else timezone_index = popular_timezone[i].substr(16,2);	
				break;
			}	
		}
		//if we can't find timezone in popular_timezone array, we use auto get method
		if(timezone_index === "")
		{
			var timezone_array=[''<? foreach("/runtime/services/timezone/zone")	{ $gen=query("gen"); if($gen!=""){echo ",'".$gen."_Index=".query("uid")."'\n";}	}	?>];	
			for(var i=1; i < timezone_array.length; i++)
			{
				if(time_offset_string===timezone_array[i].substr(3,6))
				{
					if(timezone_array[i].length===17) timezone_index = timezone_array[i].substr(16,1);
					else timezone_index = timezone_array[i].substr(16,2);	
					break;
				}	
			}
		}
		
		COMM_SetSelectValue(OBJ("wiz_tz"), timezone_index);
		return true;
	},	
	PrePasswd: function()
	{
		XS(this.passwdp, OBJ("wiz_passwd").value);
		if (OBJ("en_captcha").checked)
		{
			XS(this.captcha, "1");
			BODY.enCaptcha = true;
		}
		else
		{
			XS(this.captcha, "0");
			BODY.enCaptcha = false;
		}	
		return true;
	},
	PreTZ: function()
	{
		XS(this.tzp, OBJ("wiz_tz").value);
		return true;
	},
	InitWANSettings: function()
	{
		this.hostp = PXML.FindModule("DEVICE.HOSTNAME");
		this.inet1p = PXML.FindModule("INET.WAN-1");
		this.inet2p = PXML.FindModule("INET.WAN-2");
		this.inet3p = PXML.FindModule("INET.WAN-3");
		this.inet4p = PXML.FindModule("INET.WAN-4");
		var phyinfp = PXML.FindModule("PHYINF.WAN-1");
		if (!this.hostp||!this.inet1p||!this.inet2p||!phyinfp)
		{
			BODY.ShowAlert("InitWANSettings() ERROR!!!");
			return false;
		}
		var inet1 = XG(this.inet1p+"/inf/inet");
		var inet2 = XG(this.inet2p+"/inf/inet");
		var inet3 = "INET-8"; //may be null
		var inet4 = XG(this.inet4p+"/inf/inet");
		var eth = XG(phyinfp+"/inf/phyinf");
		this.inf1p = this.inet1p+"/inf";
		this.inf2p = this.inet2p+"/inf";
		this.inf3p = this.inet3p+"/inf";
		this.inf4p = this.inet4p+"/inf";
		this.inet1p = GPBT(this.inet1p+"/inet", "entry", "uid", inet1, false);
		this.inet2p = GPBT(this.inet2p+"/inet", "entry", "uid", inet2, false);
		this.inet3p = GPBT(this.inet3p+"/inet", "entry", "uid", inet3, false);
		this.inet4p = GPBT(this.inet4p+"/inet", "entry", "uid", inet4, false);
		phyinfp = GPBT(phyinfp, "phyinf", "uid", eth, false);
		this.macaddrp = phyinfp+"/macaddr";
		this.operatorp += "/runtime/services/operator";
		this.GetWanType();
		SetRadioValue("wan_mode", this.wanTypes[this.currentWanType]);
		/////////////////////////// initial PPPv4 hidden nodes ///////////////////////////
		OBJ("ppp4_timeout").value	= IdleTime(XG(this.inet1p+"/ppp4/dialup/idletimeout"));
		OBJ("ppp4_mode").value		= XG(this.inet1p+"/ppp4/dialup/mode");
		OBJ("ppp4_mtu").value		= XG(this.inet1p+"/ppp4/mtu");
		/////////////////////////// initial DHCP settings ///////////////////////////
		OBJ("ipv4_mtu").value		= XG(this.inet1p+"/ipv4/mtu");
		OBJ("wiz_dhcp_mac").value	= XG(this.macaddrp);
		OBJ("wiz_dhcp_host").value	= XG(this.hostp+"/device/hostname");
		OBJ("wiz_dhcpplus_user").value	= XG(this.inet1p+"/ipv4/dhcpplus/username");
		OBJ("wiz_dhcpplus_pass").value	= XG(this.inet1p+"/ipv4/dhcpplus/password");
		/////////////////////////// initial PPPoE settings ///////////////////////////
		OBJ("wiz_pppoe_ipaddr").value	= ResAddress(XG(this.inet1p+"/ppp4/ipaddr"));
		OBJ("wiz_pppoe_usr").value		= XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_pppoe_passwd").value	= XG(this.inet1p+"/ppp4/password");
		OBJ("wiz_pppoe_passwd2").value	= XG(this.inet1p+"/ppp4/password");
		OBJ("wiz_pppoe_svc").value		= XG(this.inet1p+"/ppp4/pppoe/servicename");
		if (XG(this.inet2p+"/ipv4/static")=="1")
		{
			OBJ("wiz_rpppoe_ipaddr").value	= ResAddress(XG(this.inet2p+"/ipv4/ipaddr"));
			OBJ("wiz_rpppoe_mask").value	= COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask"));
			OBJ("wiz_rpppoe_gw").value		= ResAddress(XG(this.inet2p+"/ipv4/gateway"));
			OBJ("wiz_rpppoe_dns1").value	= ResAddress(XG(this.inet2p+"/ipv4/dns/entry:1"));
			OBJ("wiz_rpppoe_dns2").value	= ResAddress(XG(this.inet2p+"/ipv4/dns/entry:2"));
		}
		/////////////////////////// initial PPTP settings ///////////////////////////
		OBJ("wiz_pptp_ipaddr").value	= ResAddress(XG(this.inet2p+"/ipv4/ipaddr"));
		OBJ("wiz_pptp_mask").value		= COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask"));
		OBJ("wiz_pptp_gw").value		= ResAddress(XG(this.inet2p+"/ipv4/gateway"));
		OBJ("wiz_pptp_svr").value		= ResAddress(XG(this.inet1p+"/ppp4/pptp/server"));
		OBJ("wiz_pptp_usr").value		= XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_pptp_passwd").value	= XG(this.inet1p+"/ppp4/password");
		OBJ("wiz_pptp_passwd2").value	= XG(this.inet1p+"/ppp4/password");
		/////////////////////////// initial L2TP settings ///////////////////////////
		OBJ("wiz_l2tp_ipaddr").value	= ResAddress(XG(this.inet2p+"/ipv4/ipaddr"));
		OBJ("wiz_l2tp_mask").value		= COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask"));
		OBJ("wiz_l2tp_gw").value		= ResAddress(XG(this.inet2p+"/ipv4/gateway"));
		OBJ("wiz_l2tp_svr").value		= ResAddress(XG(this.inet1p+"/ppp4/l2tp/server"));
		OBJ("wiz_l2tp_usr").value		= XG(this.inet1p+"/ppp4/username");
		OBJ("wiz_l2tp_passwd").value	= XG(this.inet1p+"/ppp4/password");
		OBJ("wiz_l2tp_passwd2").value	= XG(this.inet1p+"/ppp4/password");
		/////////////////////////// initial STATIC IP settings ///////////////////////////
		OBJ("wiz_static_ipaddr").value	= ResAddress(XG(this.inet1p+"/ipv4/ipaddr"));
		OBJ("wiz_static_mask").value	= COMM_IPv4INT2MASK(XG(this.inet1p+"/ipv4/mask"));
		OBJ("wiz_static_gw").value		= ResAddress(XG(this.inet1p+"/ipv4/gateway"));
		OBJ("wiz_static_dns1").value	= ResAddress(XG(this.inet1p+"/ipv4/dns/entry:1"));
		OBJ("wiz_static_dns2").value	= ResAddress(XG(this.inet1p+"/ipv4/dns/entry:2"));
		
		if (XG(this.inet1p+"/ppp4/static")=="1")
		{
			document.getElementsByName("wiz_pppoe_conn_mode")[1].checked = true;
		}
		else
		{
			document.getElementsByName("wiz_pppoe_conn_mode")[0].checked = true;
		}
		if (XG(this.inet2p+"/ipv4/static")=="1")
		{
			document.getElementsByName("wiz_rpppoe_conn_mode")[1].checked = true;
			document.getElementsByName("wiz_pptp_conn_mode")[1].checked = true;
			document.getElementsByName("wiz_l2tp_conn_mode")[1].checked = true;
		}
		else
		{
			document.getElementsByName("wiz_rpppoe_conn_mode")[0].checked = true;
			document.getElementsByName("wiz_pptp_conn_mode")[0].checked = true;
			document.getElementsByName("wiz_l2tp_conn_mode")[0].checked = true;
		}
		this.OnChangeRussiaPPPoEMode();
		this.OnChangePPPoEMode();
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
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.randomkey = RandomHex(10);
		OBJ("wiz_ssid").value = XG(this.wifip+"/ssid");
		//Show the default Pre-Shared Key if the default security mode is WPA+2PSK.
		if(XG(this.wifip+"/authtype")=="WPA+2PSK" || XG(this.wifip+"/authtype")=="WPA2PSK" || XG(this.wifip+"/authtype")=="WPAPSK")
		{	
			OBJ("wiz_key").value=XG(this.wifip+"/nwkey/psk/key");
		}	
		
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>')
		if(this.dual_band)
		{
			this.phyinf2 = GPBT(this.wlanbase, "phyinf", "uid", "BAND5G-1.1", false);
			var wifi_profile2 = XG(this.phyinf2+"/wifi");
			this.wifip2 = GPBT(this.wlanbase+"/wifi", "entry", "uid", wifi_profile2, false);
			if (!this.wifip2)
			{
				BODY.ShowAlert("Initial() ERROR!!!");
				return false;
			}
			//Show the default Pre-Shared Key if the default security mode is WPA+2PSK.
			if(XG(this.wifip2+"/authtype")=="WPA+2PSK" || XG(this.wifip2+"/authtype")=="WPA2PSK" || XG(this.wifip2+"/authtype")=="WPAPSK")
			{	
				OBJ("wiz_key_Aband").value=XG(this.wifip2+"/nwkey/psk/key");
			}
			OBJ("wiz_ssid_Aband").value = XG(this.wifip2+"/ssid"); 
			OBJ("div_ssid_A").style.display = "block"; 
			OBJ("div_ssid_A_result").style.display = "block";
		}
		else
		{
			OBJ("wifi24_name_pwd_show").innerHTML	= "<?echo i18n("Give your Wi-Fi network a name.");?>";
			OBJ("wifi24_pwd_show").style.display 	= "block";
			OBJ("fld_ssid_24_result").innerHTML		= "<?echo i18n("Wi-Fi Network Name (SSID)");?>";	
		}
				
		return true;
	},	
	PreWANSettings: function()
	{
		var type = GetRadioValue("wan_mode");
		var russia = false;
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
		XS(this.inf3p+"/infnext", "");
		XS(this.inf3p+"/inet", "INET-8");
		XS(this.inet3p+"/addrtype", "ipv6");
		XS(this.inf4p+"/active", "0");
		XS(this.inf4p+"/child", "");
		XS(this.inf4p+"/infprevious", "");
		XS(this.inet4p+"/ipv6/mode", "");
		XS(this.macaddrp, OBJ("wiz_dhcp_mac").value);
		switch (type)
		{
		case "DHCPPLUS":
			XS(this.inet1p+"/ipv4/dhcpplus/username", OBJ("wiz_dhcpplus_user").value);
			XS(this.inet1p+"/ipv4/dhcpplus/password", OBJ("wiz_dhcpplus_pass").value);
		case "DHCP":
			if (type == "DHCPPLUS")
				XS(this.inet1p+"/ipv4/dhcpplus/enable", "1");
			else
				XS(this.inet1p+"/ipv4/dhcpplus/enable", "0");
			/////////////////////////// prepare DHCP settings ///////////////////////////
			XS(this.inet1p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ipv4/static", 0);
			XS(this.inet1p+"/ipv4/mtu", OBJ("ipv4_mtu").value);
			XS(this.hostp+"/device/hostname", OBJ("wiz_dhcp_host").value);
			SetDNSAddress(this.inet1p+"/ipv4/dns", "", "");
			break;
		case "R_PPPoE":
			russia = true;
		case "PPPoE":
			/////////////////////////// prepare PPPoE settings ///////////////////////////
			var dynamic_pppoe = document.getElementsByName("wiz_pppoe_conn_mode")[0].checked ? true: false;
			XS(this.inet1p+"/addrtype", "ppp4");
			XS(this.inet1p+"/ppp4/over", "eth");
			XS(this.inet1p+"/ppp4/static", document.getElementsByName("wiz_pppoe_conn_mode")[0].checked ? 0:1);
			if (!dynamic_pppoe)	XS(this.inet1p+"/ppp4/ipaddr", OBJ("wiz_pppoe_ipaddr").value);
			XS(this.inet1p+"/ppp4/username", OBJ("wiz_pppoe_usr").value);
			XS(this.inet1p+"/ppp4/password", OBJ("wiz_pppoe_passwd").value);
			XS(this.inet1p+"/ppp4/pppoe/servicename", OBJ("wiz_pppoe_svc").value);
			if (russia)
			{
				XS(this.inf2p+"/active",		1);
				XS(this.inf2p+"/nat",			"NAT-1");
				XS(this.inet2p+"/addrtype",		"ipv4");
				if (GetRadioValue("wiz_rpppoe_conn_mode")=="dynamic")
				{
					XS(this.inet2p+"/ipv4/static", 0);
				}
				else
				{
					XS(this.inet2p+"/ipv4/static",	1);
					XS(this.inet2p+"/ipv4/ipaddr",	OBJ("wiz_rpppoe_ipaddr").value);
					XS(this.inet2p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_rpppoe_mask").value));
					XS(this.inet2p+"/ipv4/gateway",	OBJ("wiz_rpppoe_gw").value);
					SetDNSAddress(this.inet2p+"/ipv4/dns", OBJ("wiz_rpppoe_dns1").value, OBJ("wiz_rpppoe_dns2").value);
				}
			}
			break;
		case "R_PPTP":
			russia = true;
		case "PPTP":
			/////////////////////////// prepare PPTP settings ///////////////////////////
			var dynamic_pptp = document.getElementsByName("wiz_pptp_conn_mode")[0].checked ? true: false;
			XS(this.inf2p+"/active",		1);
			XS(this.inet1p+"/addrtype",		"ppp4");
			XS(this.inet1p+"/ppp4/over",	"pptp");
			XS(this.inet1p+"/ppp4/static",	0);
			XS(this.inet2p+"/addrtype",		"ipv4");
			if (dynamic_pptp)
			{
				XS(this.inet2p+"/ipv4/static", 0);
			}
			else
			{
				XS(this.inet2p+"/ipv4/static",	1);
				XS(this.inet2p+"/ipv4/ipaddr",	OBJ("wiz_pptp_ipaddr").value);
				XS(this.inet2p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_pptp_mask").value));
				XS(this.inet2p+"/ipv4/gateway",	OBJ("wiz_pptp_gw").value);
			}
			XS(this.inet1p+"/ppp4/pptp/server",	OBJ("wiz_pptp_svr").value);
			XS(this.inet1p+"/ppp4/username",	OBJ("wiz_pptp_usr").value);
			XS(this.inet1p+"/ppp4/password",	OBJ("wiz_pptp_passwd").value);
			SetDNSAddress(this.inet1p+"/ppp4/dns", OBJ("dns1").value, OBJ("dns2").value);
			
			/* Note : Russia mode need two WANs to be active simultaneously. So we remove the lowerlayer connection. 
			For normal pptp, the lowerlayer/upperlayer connection still remains. */			
			if (russia)
			{
				/* defaultroute value will become metric value.
				As for Russia, physical WAN (wan2) priority should be lower than 
				ppp WAN (wan1) */
				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "200");
								
				XS(this.inf2p+"/nat", "NAT-1");
			}
			else
			{
				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "");
	
				XS(this.inf1p+"/lowerlayer", "WAN-2");
				XS(this.inf2p+"/upperlayer", "WAN-1");				
			}		
			break;
		case "R_L2TP":
			russia = true;		
		case "L2TP":
			/////////////////////////// prepare L2TP settings ///////////////////////////
			var dynamic_l2tp = document.getElementsByName("wiz_l2tp_conn_mode")[0].checked ? true: false;
			XS(this.inf2p+"/active",		1);
			XS(this.inet1p+"/addrtype",		"ppp4");
			XS(this.inet1p+"/ppp4/over",	"l2tp");
			XS(this.inet1p+"/ppp4/static",	0);
			XS(this.inet2p+"/addrtype",		"ipv4");
			if (dynamic_l2tp)
			{
				XS(this.inet2p+"/ipv4/static", 0);
			}
			else
			{
				XS(this.inet2p+"/ipv4/static",	1);
				XS(this.inet2p+"/ipv4/ipaddr",	OBJ("wiz_l2tp_ipaddr").value);
				XS(this.inet2p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_l2tp_mask").value));
				XS(this.inet2p+"/ipv4/gateway",	OBJ("wiz_l2tp_gw").value);
			}
			XS(this.inet1p+"/ppp4/l2tp/server",	OBJ("wiz_l2tp_svr").value);
			XS(this.inet1p+"/ppp4/username",	OBJ("wiz_l2tp_usr").value);
			XS(this.inet1p+"/ppp4/password",	OBJ("wiz_l2tp_passwd").value);
			SetDNSAddress(this.inet1p+"/ppp4/dns", OBJ("dns1").value, OBJ("dns2").value);
			
			/* Note : Russia mode need two WANs to be active simultaneously. So we remove the lowerlayer connection. 
			For normal l2tp, the lowerlayer/upperlayer connection still remains. */			
			if (russia)
			{
				/* defaultroute value will become metric value.
				As for Russia, physical WAN (wan2) priority should be lower than 
				ppp WAN (wan1) */
				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "200");
								
				XS(this.inf2p+"/nat", "NAT-1");
			}
			else
			{
				XS(this.inf1p+"/defaultroute", "100");
				XS(this.inf2p+"/defaultroute", "");
	
			XS(this.inf1p+"/lowerlayer", "WAN-2");
			XS(this.inf2p+"/upperlayer", "WAN-1");
			}			
			break;
		case "STATIC":
			/////////////////////////// prepare STATIC IP settings ///////////////////////////
			XS(this.inet1p+"/addrtype",		"ipv4");
			XS(this.inet1p+"/ipv4/static",	1);
			XS(this.inet1p+"/ipv4/ipaddr",	OBJ("wiz_static_ipaddr").value);
			XS(this.inet1p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_static_mask").value));
			XS(this.inet1p+"/ipv4/gateway",	OBJ("wiz_static_gw").value);
			XS(this.inet1p+"/ipv4/mtu",		OBJ("ipv4_mtu").value);
			SetDNSAddress(this.inet1p+"/ipv4/dns", OBJ("wiz_static_dns1").value, OBJ("wiz_static_dns2").value);
			break;
		}
		if (type=="DHCP"||type=="STATIC")
		{
			XS(this.inet2p+"/ipv4/static",  0);
			XS(this.inet2p+"/ipv4/ipaddr",  "");
			XS(this.inet2p+"/ipv4/mask",    "");
			XS(this.inet2p+"/ipv4/gateway", "");
		}
		else
		{
			/////////////////////////// prepare PPPv4 hidden nodes ///////////////////////////
			XS(this.inet1p+"/ppp4/dialup/idletimeout", (OBJ("ppp4_timeout").value=="0") ? 5:OBJ("ppp4_timeout").value);
			XS(this.inet1p+"/ppp4/dialup/mode", (OBJ("ppp4_mode").value=="") ? "ondemand": OBJ("ppp4_mode").value);
			if ((type!="PPPoE" && type!="R_PPPoE") && ( OBJ("ppp4_mtu").value < 576 || OBJ("ppp4_mtu").value > 1400 ) ) XS(this.inet1p+"/ppp4/mtu", "1400");  
			else XS(this.inet1p+"/ppp4/mtu", OBJ("ppp4_mtu").value);
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
	ShowCurrentStage: function()
	{
		var i = 0;
		var type = "";
		for (i=0; i<this.wanTypes.length; i++)
		{
			type = this.wanTypes[i];
			if (type=="R_PPTP")			type = "PPTP";
			else if (type=="R_L2TP")	type = "L2TP";
			else if (type=="R_PPPoE")	type = "PPPoE";
			else if (type=="DHCPPLUS")	type = "DHCP";
			OBJ(type).style.display = "none";
		}
		for (i=0; i<this.stages.length; i++)
		{
			if (i==this.currentStage)
			{
				OBJ(this.stages[i]).style.display = "block";
				if (this.stages[this.currentStage]=="stage_ether_cfg")
				{
					type = this.wanTypes[this.currentWanType];
					if (type=="R_PPTP")			type = "PPTP";
					else if (type=="R_L2TP")	type = "L2TP";
					else if (type=="R_PPPoE")	type = "PPPoE";
					else if (type=="DHCPPLUS")	type = "DHCP";
					OBJ(type).style.display = "block";
				}
			}
			else	OBJ(this.stages[i]).style.display = "none";
		}
		if(this.wanTypes[this.currentWanType]=="DHCP" || this.wanTypes[this.currentWanType]=="DHCPPLUS" 
			|| this.wanTypes[this.currentWanType]=="STATIC" || this.wanTypes[this.currentWanType]=="R_PPPoE"
			|| this.wanTypes[this.currentWanType]=="PPPoE")	
			OBJ("DNS").style.display = "none";
		else	
			OBJ("DNS").style.display = "block";
	
		if (this.stages[this.currentStage]=="stage_desc")
		{
			// Wan detect would act when the wizard setup starts.
			if(this.wanDetectTimer) clearTimeout(this.wanDetectTimer);
			this.wanDetectResult = "";
			this.wanDetectNum = 0;
			this.WanDetect("WANDETECT");			
		}	

		if (this.stages[this.currentStage]=="stage_wan_detect")
		{
			if(this.wanDetectCheckTimer) clearTimeout(this.wanDetectCheckTimer);
			this.wanDetectCheckNum = 0;
			this.WanDetectCheck();
		}
		else OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = OBJ("wantype_unknown").style.display = "none";	

		if (this.stages[this.currentStage]=="stage_wlan_result")
		{
			UpdateWLANCFG("");
			if(this.dual_band) UpdateWLANCFG("_Aband");
		}

		if (this.stages[this.currentStage]=="check_wan_connect")
		{
			BODY.ShowContent();
			this.internetCheck=0;
			setTimeout('PAGE.SetStage(1);PAGE.ShowCurrentStage();', 2000);
			setTimeout('PAGE.CheckInternet()', 10000);
		}
		
	},
	SetStage: function(offset)
	{
		var length = this.stages.length;
		switch (offset)
		{
		case 2:
			if (this.currentStage < length-1)
				this.currentStage += 2;
			break;			
		case 1:
			if (this.currentStage < length-1)
				this.currentStage += 1;
			break;
		case -1:
			if (this.currentStage > 0)
				this.currentStage -= 1;
			break;
		case -2:
			if (this.currentStage > 1)
				this.currentStage -= 2;
			break;			
		}
	},
	OnClickPre: function()
	{
		var stage = this.stages[this.currentStage];
		var type = this.wanTypes[this.currentWanType];
		if(stage=="stage_wlan_set" && (type=="DHCP" || type=="DHCPPLUS")) this.SetStage(-2);
		else this.SetStage(-1);	
		stage = this.stages[this.currentStage];	
		if(stage=="stage_wan_detect") this.WanDetectAgain();
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		var stage = this.stages[this.currentStage];
		if (stage == "stage_passwd")
		{
			/* The IE browser would treat the text with all spaces as empty according as 
				it would ignore the text node with all spaces in XML DOM tree for IE6, 7, 8, 9.*/		
			if(COMM_IsAllSpace(OBJ("wiz_passwd").value))
			{
				BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
				return false;
			}	
			if (OBJ("wiz_passwd").value=="")
			{
				BODY.ShowAlert("<?echo i18n("Administrator password cannot be blank.");?>");
				return false;			
			}	
			for(var i=0;i < OBJ("wiz_passwd").value.length;i++)
			{
				if (OBJ("wiz_passwd").value.charCodeAt(i) > 256) //avoid holomorphic word
				{ 
					BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
					return false;
				}
			}			
			if (OBJ("wiz_passwd").value!=OBJ("wiz_passwd2").value)
			{
				BODY.ShowAlert("<?echo i18n("Please make the two admin passwords the same and try again");?>");
				return false;
			}
			this.PrePasswd();
			CheckAccount();			
		}	
		else if (stage == "stage_ether")
		{
			var type = this.wanTypes[this.currentWanType];
			if (type=="DHCPPLUS")	type = "DHCP";
			if (type=="DHCP")
			{
				this.PreWANSettings();
				this.SetStage(2);	
			}
			else this.SetStage(1);
			this.ShowCurrentStage();
		}			
		else if (stage == "stage_ether_cfg")
		{
			this.PreWANSettings();
			var type = this.wanTypes[this.currentWanType];
			if (type=="R_PPTP")			type = "PPTP";
			else if (type=="R_L2TP")	type = "L2TP";
			else if (type=="R_PPPoE")	type = "PPPoE";
			else if (type=="DHCPPLUS")	type = "DHCP";
			CheckWANSettings(type);
		}
		else if (stage == "stage_wlan_set")
		{
			if(OBJ("wiz_ssid").value.charAt(0)===" "|| OBJ("wiz_ssid").value.charAt(OBJ("wiz_ssid").value.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
				OBJ("wiz_ssid").focus();
				return ;
			}
			if(this.dual_band && (OBJ("wiz_ssid_Aband").value.charAt(0)===" "|| OBJ("wiz_ssid_Aband").value.charAt(OBJ("wiz_ssid_Aband").value.length-1)===" "))
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
				OBJ("wiz_ssid_Aband").focus();
				return ;
			}			
			if (OBJ("wiz_ssid").value=="")
			{
				BODY.ShowAlert("<?echo i18n("The SSID field can not be blank");?>");
				OBJ("wiz_ssid").focus();
				return;
			}
			if (this.dual_band && OBJ("wiz_ssid_Aband").value=="")
			{
				BODY.ShowAlert("<?echo i18n("The SSID field can not be blank");?>");
				OBJ("wiz_ssid_Aband").focus();
				return;
			}			
			
			if (OBJ("wiz_key").value.length < 8)
			{
				BODY.ShowAlert("<?echo i18n("Incorrect key length, should be 8 to 63 characters long.");?>");
				OBJ("wiz_key").focus();
				return;
			}
			if(this.dual_band && OBJ("wiz_key_Aband").value.length < 8)
			{
				BODY.ShowAlert("<?echo i18n("Incorrect key length, should be 8 to 63 characters long.");?>");
				OBJ("wiz_key_Aband").focus();
				return;
			}
			if(OBJ("wiz_key").value.indexOf(" ") >= 0)
			{
				BODY.ShowAlert("<?echo i18n("Password can not contain blank.");?>");
				OBJ("wiz_key").focus();
				return;
			}
			if(this.dual_band && OBJ("wiz_key_Aband").value.indexOf(" ") >= 0)
			{
				BODY.ShowAlert("<?echo i18n("Password can not contain blank.");?>");
				OBJ("wiz_key_Aband").focus();
				return;
			}				
			this.SetStage(1);
			this.ShowCurrentStage();
		}		
		else
		{	
			this.SetStage(1);
			this.ShowCurrentStage();
		}	
	},
	OnClickCancel: function()
	{
		SendEvent("DBSAVE", "/bsc_internet.php");		
	},
	OnClickSkip: function()
	{
		ActiveWLANService();
	},
	OnChangeWanType: function(type)
	{
		for (var i=0; i<this.wanTypes.length; i++)
		{
			if (type=="R_PPPoE")	OBJ("R_PPPoE").style.display = "block";
			else					OBJ("R_PPPoE").style.display = "none";
			
			if (type=="DHCPPLUS")	OBJ("DHCPPLUS").style.display = "block";
			else					OBJ("DHCPPLUS").style.display = "none";

			if (this.wanTypes[i]==type)
				this.currentWanType = i;
		}
	},
	OnClickCloneMAC: function()
	{
		OBJ("wiz_dhcp_mac").value = "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
	},
	GetWanType: function()
	{
		var addrtype = XG(this.inet1p+"/addrtype");
		var type = null;
		switch (addrtype)
		{
		case "ipv4":
			if (XG(this.inet1p+"/ipv4/static")=="0")
			{
				if (XG(this.inet1p+"/ipv4/dhcpplus/enable")=="1")
					type = "DHCPPLUS";
				else
					type = "DHCP";
			}
			else
				type = "STATIC";
			break;
		case "ppp4":
		case "ppp10":
			if (XG(this.inet1p+"/ppp4/over")=="eth")
			{
				if (XG(this.inf2p+"/active")=="1" && XG(this.inf2p+"/nat")=="NAT-1")
					type = "R_PPPoE";
				else
					type = "PPPoE";
			}
			else if (XG(this.inet1p+"/ppp4/over")=="pptp")
			{
				if (XG(this.inf2p+"/nat")=="NAT-1")
					type = "R_PPTP";
				else
					type = "PPTP";
			}
			else if (XG(this.inet1p+"/ppp4/over")=="l2tp")
			{
				type = "L2TP";
			}
			break;
		default:
			BODY.ShowAlert("Internal Error!!");
		}

		for (var i=0; i<this.wanTypes.length; i++)
		{
			if (this.wanTypes[i]==type)	this.currentWanType = i;
		}
		if (type=="R_PPPoE")	OBJ("R_PPPoE").style.display = "block";
		else					OBJ("R_PPPoE").style.display = "none";
		
		if (type=="DHCPPLUS")	OBJ("DHCPPLUS").style.display = "block";
		else					OBJ("DHCPPLUS").style.display = "none";
	},
	OnChangeRussiaPPPoEMode: function()
	{
		var disable = document.getElementsByName("wiz_rpppoe_conn_mode")[0].checked ? true: false;
		OBJ("wiz_rpppoe_ipaddr").disabled = disable;
		OBJ("wiz_rpppoe_mask").disabled = disable;
		OBJ("wiz_rpppoe_gw").disabled = disable;
		OBJ("wiz_rpppoe_dns1").disabled = OBJ("wiz_rpppoe_dns2").disabled = disable;
	},
	OnChangePPPoEMode: function()
	{
		var disable = document.getElementsByName("wiz_pppoe_conn_mode")[0].checked ? true: false;
		OBJ("wiz_pppoe_ipaddr").disabled = disable;
	},
	OnChangePPTPMode: function()
	{
		var disable = document.getElementsByName("wiz_pptp_conn_mode")[0].checked ? true: false;
		OBJ("wiz_pptp_ipaddr").disabled = disable;
		OBJ("wiz_pptp_mask").disabled = disable;
		OBJ("wiz_pptp_gw").disabled = disable;
	},
	OnChangeL2TPMode: function()
	{
		var disable = document.getElementsByName("wiz_l2tp_conn_mode")[0].checked ? true: false;
		OBJ("wiz_l2tp_ipaddr").disabled = disable;
		OBJ("wiz_l2tp_mask").disabled = disable;
		OBJ("wiz_l2tp_gw").disabled = disable;
	},
	WanDetectCheck: function()
	{
		if(this.stages[this.currentStage]!=="stage_wan_detect") return;
		else if(this.wanDetectResult === "")
		{
			if(this.wanDetectCheckNum < 20)
			{
				OBJ("cable_fail").style.display = OBJ("wantype_unknown").style.display = "none";
				OBJ("wan_detect").style.display = "block";
				this.wanDetectCheckTimer = setTimeout('PAGE.WanDetectCheck()', 1000);
			}
			else
			{
				OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = "none";
				OBJ("wantype_unknown").style.display = "block";				
			}						
		}	
		else if(this.wanDetectResult === "DHCP")
		{
			for(var i=0; i < this.stages.length; i++) if(this.stages[i]==="stage_wlan_set") this.currentStage=i;
			for(var i=0; i < this.wanTypes.length; i++)	if(this.wanTypes[i]==="DHCP")	this.currentWanType=i;
			SetRadioValue("wan_mode", "DHCP");
			this.ShowCurrentStage();
		}
		else if(this.wanDetectResult === "PPPoE")
		{
			for(var i=0; i < this.stages.length; i++) if(this.stages[i]==="stage_ether_cfg") this.currentStage=i;
			for(var i=0; i < this.wanTypes.length; i++)	if(this.wanTypes[i]==="PPPoE")	this.currentWanType=i;
			SetRadioValue("wan_mode", "PPPoE");			
			this.ShowCurrentStage();			
		}			
		else if(this.wanDetectResult === "linkdown")
		{
			OBJ("wan_detect").style.display = OBJ("wantype_unknown").style.display = "none";
			OBJ("cable_fail").style.display = "block";
		}
		else if(this.wanDetectResult === "None")
		{
			// If the wan detect result is fail at the first time check, it would wan detect again.
			if(this.wanDetectCheckNum === 0) this.WanDetectAgain(); 
			else	
			{
				OBJ("wan_detect").style.display = OBJ("wantype_unknown").style.display = "none";
				OBJ("cable_fail").style.display = "block";
			}
		}
		else if(this.wanDetectResult === "unknown")
		{
			// If the wan detect result is fail at the first time check, it would wan detect again.
			if(this.wanDetectCheckNum === 0) this.WanDetectAgain(); 
			else	
			{			
				OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = "none";
				OBJ("wantype_unknown").style.display = "block";
			}	
		}
		else
		{
			// If the wan detect result is fail at the first time check, it would wan detect again.
			if(this.wanDetectCheckNum === 0) this.WanDetectAgain(); 
			else	
			{			
				OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = "none";
				OBJ("wantype_unknown").style.display = "block";
			}
		}		
		this.wanDetectCheckNum++;
	},
	WanDetect: function(action)
	{
		if(this.stages[this.currentStage]!=="stage_desc" && 
			this.stages[this.currentStage]!=="stage_wan_detect")	return;
		var ajaxObj = GetAjaxObj(action);
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.WanDetectCallback(xml.Get("/wandetectreport/result"), xml.Get("/wandetectreport/reason"));
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("wandetect.php", "action="+action);
		AUTH.UpdateTimeout();
	},
	WanDetectCallback: function(result, reason)
	{
		if(this.stages[this.currentStage]!=="stage_desc" && 
			this.stages[this.currentStage]!=="stage_wan_detect")	return;
		switch (result)
		{
			case "OK":
				this.wanDetectTimer = setTimeout('PAGE.WanDetect("WANTYPERESULT")', 2000);
				break;
			case "DHCP":
				this.wanDetectResult = "DHCP";
				break;	
			case "PPPoE":
				this.wanDetectResult = "PPPoE";	
				break;
			case "None":
				this.wanDetectResult = "None";
				break;
			case "linkdown":
				this.wanDetectResult = "linkdown";		
				break
			case "unknown":
				this.wanDetectResult = "unknown";		
			case "detecting":
			case "":
				if (this.wanDetectNum < 10)
				{
					this.wanDetectTimer = setTimeout('PAGE.WanDetect("WANTYPERESULT")', 2000);
					this.wanDetectNum++;
				}
				else this.wanDetectResult = "unknown";
				break;
			case "FAIL":
			default:
				this.wanDetectResult = "unknown";	
				break;
		}
	},
	WanDetectAgain: function()
	{
		if(this.wanDetectTimer) clearTimeout(this.wanDetectTimer);
		this.wanDetectResult = "";
		this.wanDetectNum = 0;
		this.WanDetect("WANDETECT");
		if(this.wanDetectCheckTimer) clearTimeout(this.wanDetectCheckTimer);
		this.wanDetectCheckNum = 0;
		this.WanDetectCheck();		
	},
	CheckInternet: function()
	{
		var action="dnsquery";
		if(PAGE.internetCheck < 10)
		{
			PAGE.internetCheck++;
		}
		var ajaxObj = GetAjaxObj("InternetCheck");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			if(xml.Get("/dnsquery/report")=="Internet detected.")
			{
				self.location.href="wiz_mydlink.php?freset=1&language=<? echo $langcode; ?>";				
			}
			else if(PAGE.internetCheck >= 10)
			{
				if (confirm("<?echo I18N("h","No Internet detected, would you like to restart the wizard?");?>")) { self.location.href="wiz_freset.php"; }
				else{ ActiveWLANService(); }
			}
			else setTimeout('PAGE.CheckInternet()', 2000);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("dnsquery.php", "act="+action);
	}
}

function SetButtonDisabled(name, disable)
{
	var button = document.getElementsByName(name);
	for (i=0; i<button.length; i++)	button[i].disabled = disable;
}
function GetRadioValue(name)
{
	var radio = document.getElementsByName(name);
	var value = null;
	for (i=0; i<radio.length; i++)
	{
		if (radio[i].checked)	return radio[i].value;
	}
}
function SetRadioValue(name, value)
{
	var radio = document.getElementsByName(name);
	for (i=0; i<radio.length; i++)
	{
		if (radio[i].value==value)	radio[i].checked = true;
	}
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
function CheckWANSettings(type)
{
	PXML.IgnoreModule("DEVICE.ACCOUNT");
	PXML.IgnoreModule("DEVICE.TIME");
	PXML.IgnoreModule("DEVICE.HOSTNAME");
	PXML.IgnoreModule("WAN");
	PXML.CheckModule("INET.WAN-1", null, "ignore", "ignore");
	PXML.CheckModule("INET.WAN-2", null, "ignore", "ignore");
	switch (type)
	{
	case "DHCP":
		PXML.CheckModule("DEVICE.HOSTNAME", null, "ignore", "ignore");
		PXML.CheckModule("PHYINF.WAN-1", null, "ignore", "ignore");
		break;
	case "PPPoE":
		if (PAGE.wanTypes[PAGE.currentWanType]=="R_PPPoE" && document.getElementsByName("wiz_rpppoe_conn_mode")[1].checked)
		{
			if (OBJ("wiz_rpppoe_dns1").value==="" || OBJ("wiz_rpppoe_dns1").value==="0.0.0.0")
			{
				BODY.ShowAlert("<?echo i18n("Invalid Primary DNS address.");?>");
				return false;
			}
		}
		break; //hendry, we for PPPoE, don't have verify password right now. So we omit password checking.!!
	case "PPTP":
	case "L2TP":
		if (OBJ("wiz_"+type.toLowerCase()+"_passwd").value!=
			OBJ("wiz_"+type.toLowerCase()+"_passwd2").value)
		{
			BODY.ShowAlert("<?echo i18n("Please make the two admin passwords the same and try again");?>");
			return false;
		}
		break;
	case "STATIC":
		if (OBJ("wiz_static_dns1").value==="" || OBJ("wiz_static_dns1").value==="0.0.0.0")
		{
			BODY.ShowAlert("<?echo i18n("Invalid Primary DNS address.");?>");
			return false;
		}
		break;
	}

	AUTH.UpdateTimeout();
	COMM_CallHedwig(PXML.doc, 
		function (xml)
		{
			switch (xml.Get("/hedwig/result"))
			{
			case "OK":
				PAGE.SetStage(1);
				PAGE.ShowCurrentStage();
				break;
			case "FAILED":
				BODY.ShowAlert(xml.Get("/hedwig/message"));
				break;
			}
		}
	);
}
function CheckAccount()
{
	PXML.CheckModule("DEVICE.ACCOUNT", null, "ignore", "ignore");
	PXML.IgnoreModule("DEVICE.TIME");
	PXML.IgnoreModule("DEVICE.HOSTNAME");
	PXML.IgnoreModule("PHYINF.WAN-1");
	PXML.IgnoreModule("WAN");	
	PXML.IgnoreModule("INET.WAN-1");
	PXML.IgnoreModule("INET.WAN-2");

	AUTH.UpdateTimeout();
	COMM_CallHedwig(PXML.doc, 
		function (xml)
		{
			switch (xml.Get("/hedwig/result"))
			{
			case "OK":
				PAGE.SetStage(1);
				PAGE.ShowCurrentStage();			
				break;
			case "FAILED":
				BODY.ShowAlert(xml.Get("/hedwig/message"));
				break;
			}
		}
	);
}
function ChangeSelectorOptions(id, options)
{
	var slt = OBJ(id);
	for (var i=slt.length; i>=1; i--)
	{
		slt.remove(i);
	}

	for (var i=0; i<options.length; i++)
	{
		var item = document.createElement("option");
		item.text = options[i];
		item.value = options[i];
		try
		{
			slt.add(item, null);
		}
		catch(e)
		{
			slt.add(item);	// IE only
		}
	}
}
function IdleTime(value)
{
	if (value=="")
		return "0";
	else
		return parseInt(value, 10);
}
function UpdateWLANCFG(str_Aband)
{	
	OBJ("ssid"+str_Aband).innerHTML = OBJ("wiz_ssid"+str_Aband).value;
	OBJ("wiz_key_result"+str_Aband).innerHTML = OBJ("wiz_key"+str_Aband).value;
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
function addBookmarkForBrowser(sTitle, sUrl)
{
	if (/Chrome/.test(navigator.userAgent) || /Opera/.test(navigator.userAgent))
		BODY.ShowAlert("<?echo i18n("Please press CTRL-D to bookmark.");?>");	
	else if (window.sidebar && window.sidebar.addPanel)	window.sidebar.addPanel(sTitle, sUrl, "");// For firefox
	else if (window.external)	window.external.AddFavorite(sUrl, sTitle);// For IE
	else	BODY.ShowAlert("<?echo i18n("Please press CTRL-D or CTRL-T to bookmark.");?>");
}
function SendEvent(svc,page)
{	
	var ajaxObj = GetAjaxObj("SendEvent");
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		self.location.href = page;
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
// Activate Wifi setting and redirect user to webUI when user leave the wizard
function ActiveWLANService()
{
	PXML.CheckModule("WIFI.PHYINF", "ignore", "ignore", null);
	PXML.CheckModule("PHYINF.WIFI", "ignore", "ignore", null);
			
	xml = PXML.doc;
	PXML.UpdatePostXML(xml);
	PXML.Post(
	function(code, result)
	{
		if(code != "OK") BODY.ShowAlert("<?echo I18N("h","Wifi settings error, please check it again!!");?>");
		self.location.href="bsc_internet.php";
	});
}
</script>
