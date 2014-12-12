<?include "/htdocs/phplib/inet.php";?>
<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "DEVICE.ACCOUNT,DEVICE.TIME,DEVICE.HOSTNAME,PHYINF.WAN-1,INET.WAN-1,INET.WAN-2,INET.WAN-3,INET.WAN-4,WAN,OPENDNS4",
	OnLoad: function()
	{
		this.ShowCurrentStage();
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		if (COMM_Equal(OBJ("wiz_dhcp_mac").getAttribute("modified"), true)) 
		{
			var msgArray = ['<?echo I18N("j","It would spend a little time, please wait");?>...'];	
			BODY.ShowCountdown('<?echo I18N("j","Clone MAC Address");?>...', msgArray, this.bootuptime, "http://<?echo $_SERVER['HTTP_HOST'];?>/wiz_wlan_freset.php");			
		}	
		else self.location.href = "./wiz_wlan_freset.php";
		return true;
	},
	InitValue: function(xml)
	{
		/*Enter wizard without login when it is factory default.*/
		<?
			if(query("/runtime/device/devconfsize")=="0")
			{
				echo 'AUTH.AuthorizedGroup = -1;';
				echo 'AUTH.Login(null, "Admin", "", null);';
			}	
		?>
		
		PXML.doc = xml;
		if (!this.Initial()) return false;
		if (!this.InitWANSettings()) return false;
		if (!this.InitADVDNS()) return false;
		return true;
	},
	PreSubmit: function()
	{
		PXML.ActiveModule("DEVICE.ACCOUNT");
		PXML.ActiveModule("DEVICE.TIME");
		PXML.ActiveModule("DEVICE.HOSTNAME");
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		PXML.CheckModule("INET.WAN-2", null, null, "ignore");
		if (COMM_Equal(OBJ("wiz_dhcp_mac").getAttribute("modified"), true))
		{
			PXML.ActiveModule("PHYINF.WAN-1");
			PXML.DelayActiveModule("PHYINF.WAN-1", "3");
			PXML.IgnoreModule("WAN");
		}
		else
		{
			PXML.CheckModule("PHYINF.WAN-1", null, null, "ignore");
			PXML.CheckModule("WAN", "ignore", "ignore", null);
		}
		if (!this.PreTZ()) return null;
		if (!this.PreADVDNS()) return null;
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
	opendns_wan1_infp:null,
	opendns_adv_dns1:null,
	opendns_adv_dns2:null,
	macaddrp: null,
	operatorp: null,
	stages: new Array ("stage_desc", "stage_passwd", "stage_tz", "stage_adv_dns", "stage_wan_detect","stage_ether", "stage_ether_cfg"),
	wanTypes: new Array ("DHCP", "DHCPPLUS", "PPPoE", "PPTP", "L2TP", "STATIC", "R_PPTP", "R_PPPoE"),
	wanDetectProcess: new Array ("wan_detect", "cable_fail", "wantype_unkown"),
	wanDetectNum: 0,
	currentStage: 0,	// 0 ~ this.stages.length
	currentWanType: 0,	// 0 ~ this.wanTypes.length
	Initial: function()
	{
		this.passwdp = PXML.FindModule("DEVICE.ACCOUNT");
		this.tzp = PXML.FindModule("DEVICE.TIME");
		if (!this.tzp||!this.passwdp)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.captcha = this.passwdp + "/device/session/captcha";
		this.passwdp = GPBT(this.passwdp+"/device/account", "entry", "name", "Admin", false);
		this.passwdp += "/password";
		this.tzp += "/device/time/timezone";
		OBJ("wiz_passwd").value = OBJ("wiz_passwd2").value = XG(this.passwdp);
		OBJ("en_captcha").checked = COMM_EqBOOL(XG(this.captcha), true);
		COMM_SetSelectValue(OBJ("wiz_tz"), COMM_ToNUMBER(XG(this.tzp)));
		return true;
	},
	InitADVDNS: function()
	{
		var p = PXML.FindModule("OPENDNS4");
		this.opendns_wan1_infp  = GPBT(p, "inf", "uid", "WAN-1", false);
		if (XG(this.opendns_wan1_infp+"/open_dns/type")==="advance")	OBJ("en_adv_dns").checked = true;
		else	OBJ("en_adv_dns").checked = false;					
		this.opendns_adv_dns1	= XG(this.opendns_wan1_infp+"/open_dns/adv_dns_srv/dns1");
		this.opendns_adv_dns2	= XG(this.opendns_wan1_infp+"/open_dns/adv_dns_srv/dns2");
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
		if(XG(this.inet1p+"/inf/open_dns/type")==="advance")	OBJ("en_adv_dns").checked = true;
		else	OBJ("en_adv_dns").checked = false;
		this.OnChangeRussiaPPPoEMode();
		this.OnChangePPPoEMode();
		this.OnChangePPTPMode();
		this.OnChangeL2TPMode();
		
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
		XS(this.inf2p+"/defaultroute", 0);
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
			if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet1p+"/ipv4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
			else SetDNSAddress(this.inet1p+"/ipv4/dns", "", "");
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
			if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet1p+"/ppp4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
			else SetDNSAddress(this.inet1p+"/ppp4/dns", OBJ("dns1").value, OBJ("dns2").value);
			if (russia)
			{
				XS(this.inf1p+"/lowerlayer",	"WAN-2");
				XS(this.inf2p+"/active",		1);
				XS(this.inf2p+"/nat",			"NAT-1");
				XS(this.inf2p+"/upperlayer",	"WAN-1");
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
					if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet2p+"/ipv4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
					else SetDNSAddress(this.inet2p+"/ipv4/dns", OBJ("wiz_rpppoe_dns1").value, OBJ("wiz_rpppoe_dns2").value);
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
			XS(this.inf1p+"/lowerlayer", "WAN-2");
			XS(this.inf2p+"/upperlayer", "WAN-1");
			if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet1p+"/ppp4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
			else SetDNSAddress(this.inet1p+"/ppp4/dns", OBJ("dns1").value, OBJ("dns2").value);
			if (russia)
			{
				XS(this.inf2p+"/nat", "NAT-1");
			}
			break;
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
			XS(this.inf1p+"/lowerlayer", "WAN-2");
			XS(this.inf2p+"/upperlayer", "WAN-1");
			if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet1p+"/ppp4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
			else SetDNSAddress(this.inet1p+"/ppp4/dns", OBJ("dns1").value, OBJ("dns2").value);
			break;
		case "STATIC":
			/////////////////////////// prepare STATIC IP settings ///////////////////////////
			XS(this.inet1p+"/addrtype",		"ipv4");
			XS(this.inet1p+"/ipv4/static",	1);
			XS(this.inet1p+"/ipv4/ipaddr",	OBJ("wiz_static_ipaddr").value);
			XS(this.inet1p+"/ipv4/mask",	COMM_IPv4MASK2INT(OBJ("wiz_static_mask").value));
			XS(this.inet1p+"/ipv4/gateway",	OBJ("wiz_static_gw").value);
			XS(this.inet1p+"/ipv4/mtu",		OBJ("ipv4_mtu").value);
			if(OBJ("en_adv_dns").checked) SetDNSAddress(this.inet1p+"/ipv4/dns", this.opendns_adv_dns1, this.opendns_adv_dns2);
			else SetDNSAddress(this.inet1p+"/ipv4/dns", OBJ("wiz_static_dns1").value, OBJ("wiz_static_dns2").value);
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
			XS(this.inet1p+"/ppp4/dialup/idletimeout", OBJ("ppp4_timeout").value);
			XS(this.inet1p+"/ppp4/dialup/mode", (OBJ("ppp4_mode").value=="") ? "ondemand": OBJ("ppp4_mode").value);
			if (type != "PPPoE" && ( OBJ("ppp4_mtu").value < 576 || OBJ("ppp4_mtu").value > 1400 ) ) XS(this.inet1p+"/ppp4/mtu", "1400");  
			else XS(this.inet1p+"/ppp4/mtu", OBJ("ppp4_mtu").value);
		}

		return true;
	},
	PreADVDNS: function()
	{
		if (OBJ("en_adv_dns").checked)	XS(this.opendns_wan1_infp+"/open_dns/type", "advance");
		else	XS(this.opendns_wan1_infp+"/open_dns/type", "");
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
					else if (type=="R_PPPoE")	type = "PPPoE";
					else if (type=="DHCPPLUS")	type = "DHCP";
					OBJ(type).style.display = "block";
				}
			}
			else	OBJ(this.stages[i]).style.display = "none";
		}
		if(this.wanTypes[this.currentWanType]=="DHCP" || this.wanTypes[this.currentWanType]=="DHCPPLUS" || this.wanTypes[this.currentWanType]=="STATIC" || this.wanTypes[this.currentWanType]=="R_PPPoE")	
			OBJ("DNS").style.display = "none";
		else	OBJ("DNS").style.display = "block";

		if (this.stages[this.currentStage]=="stage_wan_detect")	this.WanDetectPre();
		else	for (var j=0; j<this.wanDetectProcess.length; j++)	OBJ(this.wanDetectProcess[j]).style.display = "none";

		if (this.currentStage==0)
		{
			SetButtonDisabled("b_pre", true);
			SetButtonDisabled("b_send", true);
		}
		else	SetButtonDisabled("b_pre", false);
	},
	SetStage: function(offset)
	{
		var length = this.stages.length;
		switch (offset)
		{
		case 1:
			if (this.currentStage < length-1)
				this.currentStage += 1;
			break;
		case -1:
			if (this.currentStage > 0)
				this.currentStage -= 1;
			break;
		}
	},
	OnClickPre: function()
	{
		this.SetStage(-1);
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		var stage = this.stages[this.currentStage];
		if (stage == "stage_passwd")
		{
			if (OBJ("wiz_passwd").value!=OBJ("wiz_passwd2").value)
			{
				BODY.ShowAlert("<?echo i18n("Please make the two passwords the same and try again.");?>");
				return false;
			}
			this.PrePasswd();
			CheckAccount();			
		}
		else if (stage == "stage_adv_dns")
		{
			if (OBJ("en_adv_dns").checked)
			{
				OBJ("wiz_rpppoe_dns1").value = OBJ("wiz_static_dns1").value	= OBJ("dns1").value = this.opendns_adv_dns1;
				OBJ("wiz_rpppoe_dns2").value = OBJ("wiz_static_dns2").value	= OBJ("dns2").value = this.opendns_adv_dns2;
				OBJ("wiz_rpppoe_dns1").disabled = OBJ("wiz_static_dns1").disabled = OBJ("dns1").disabled = true;
				OBJ("wiz_rpppoe_dns2").disabled = OBJ("wiz_static_dns2").disabled = OBJ("dns2").disabled = true;
				OBJ("wiz_rpppoe_dns1").title = OBJ("wiz_static_dns1").title	= OBJ("dns1").title = "<?echo I18N("h", "Locked by parental control");?>";
				OBJ("wiz_rpppoe_dns2").title = OBJ("wiz_static_dns2").title	= OBJ("dns2").title = "<?echo I18N("h", "Locked by parental control");?>";				
			}
			else
			{
				OBJ("wiz_rpppoe_dns1").value = OBJ("wiz_static_dns1").value	= OBJ("dns1").value = "";
				OBJ("wiz_rpppoe_dns2").value = OBJ("wiz_static_dns2").value	= OBJ("dns2").value = "";
				OBJ("wiz_rpppoe_dns1").disabled = OBJ("wiz_static_dns1").disabled = OBJ("dns1").disabled = false;
				OBJ("wiz_rpppoe_dns2").disabled = OBJ("wiz_static_dns2").disabled = OBJ("dns2").disabled = false;
				OBJ("wiz_rpppoe_dns1").title = OBJ("wiz_static_dns1").title	= OBJ("dns1").title = "";
				OBJ("wiz_rpppoe_dns2").title = OBJ("wiz_static_dns2").title	= OBJ("dns2").title = "";											
			}
			this.SetStage(1);
			this.ShowCurrentStage();					
		}		
		else if (stage == "stage_ether_cfg")
		{
			this.PreWANSettings();
			var type = this.wanTypes[this.currentWanType];
			if (type=="R_PPTP")			type = "PPTP";
			else if (type=="R_PPPoE")	type = "PPPoE";
			else if (type=="DHCPPLUS")	type = "DHCP";
			CheckWANSettings(type);
			return
		}
		else
		{	
			this.SetStage(1);
			this.ShowCurrentStage();
		}	
	},
	OnClickCancel: function()
	{
		if (!COMM_IsDirty(false)||confirm("<?echo i18n("Do you want to abandon all changes you made to this wizard?");?>"))
			self.location.href = "./bsc_internet.php";
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
		if (OBJ("en_adv_dns").checked)	OBJ("wiz_rpppoe_dns1").disabled = OBJ("wiz_rpppoe_dns2").disabled = true;
		else	OBJ("wiz_rpppoe_dns1").disabled = OBJ("wiz_rpppoe_dns2").disabled = disable;
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
	WanDetectPre: function()
	{
		OBJ("cable_fail").style.display = OBJ("wantype_unkown").style.display = "none";
		OBJ("wan_detect").style.display = "block";
		this.wanDetectNum = 0;
		this.WanDetect("WANDETECT");
	},
	WanDetect: function(action)
	{
		if(this.stages[this.currentStage]!=="stage_wan_detect")	return;
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
		if(this.stages[this.currentStage]!=="stage_wan_detect")	return;
		switch (result)
		{
			case "OK":
				setTimeout('PAGE.WanDetect("WANTYPERESULT")', 20000);
				break;
			case "DHCP":
				for(var i=0; i < this.stages.length; i++)	if(this.stages[i]==="stage_ether_cfg")	this.currentStage=i;
				for(var i=0; i < this.wanTypes.length; i++)	if(this.wanTypes[i]==="DHCP")	this.currentWanType=i;
				SetRadioValue("wan_mode", "DHCP");
				this.ShowCurrentStage();
				break;	
			case "PPPoE":
				for(var i=0; i < this.stages.length; i++)	if(this.stages[i]==="stage_ether_cfg")	this.currentStage=i;
				for(var i=0; i < this.wanTypes.length; i++)	if(this.wanTypes[i]==="PPPoE")	this.currentWanType=i;
				SetRadioValue("wan_mode", "PPPoE");		
				this.ShowCurrentStage();	
				break;
			case "None":
				OBJ("wan_detect").style.display = OBJ("wantype_unkown").style.display = "none";
				OBJ("cable_fail").style.display = "block";
				break;
			case "unknown":
				OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = "none";
				OBJ("wantype_unkown").style.display = "block";			
			case "":
				if (this.wanDetectNum < 2)
				{
					setTimeout('PAGE.WanDetect("WANTYPERESULT")', 10000);
					this.wanDetectNum++;
				}
				else
				{
					OBJ("wan_detect").style.display = OBJ("cable_fail").style.display = "none";
					OBJ("wantype_unkown").style.display = "block";
				}
				break;
			case "FAIL":
			default:
				for(var i=0; i < this.stages.length; i++)	if(this.stages[i]==="stage_ether")	this.currentStage=i;
				this.ShowCurrentStage();				
				break;
		}
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
	case "PPTP":
	case "L2TP":
		if (OBJ("wiz_"+type.toLowerCase()+"_passwd").value!=
			OBJ("wiz_"+type.toLowerCase()+"_passwd2").value)
		{
			BODY.ShowAlert("<?echo i18n("Please make the two passwords the same and try again.");?>");
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
				BODY.OnSubmit();
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
</script>
