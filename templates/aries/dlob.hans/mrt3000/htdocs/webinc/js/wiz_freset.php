<?
include "/htdocs/phplib/inet.php";
?><script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "DEVICE.ACCOUNT,PHYINF.WAN-1,INET.WAN-1,INET.WAN-2,WAN,WIFI.PHYINF,PHYINF.WIFI,DEVICE.TIME,RUNTIME.TIME,RUNTIME.PHYINF,RUNTIME.INF.WAN-1",
	logindefault: 0,
	OnLoad: function()
	{
		var wiz_freset = <? $w=query("/device/wiz_freset"); if($w!=1){echo 0;}else{echo 1;} ?>;
		var wiz_clonemac = <? $c=query("/device/wiz_clonemac"); if($c!=1){echo 0;}else{echo 1;} ?>;
//		var usr_pwd = "<? $p=query("/device/account/entry/password"); if($p==""){echo '\"\"';}else{echo $p;} ?>";	
		var usr_pwd = "<? $p=query("/device/account/entry/password"); if($p==""){echo '';}else{echo $p;} ?>";	

		if (wiz_freset!=1 && this.logindefault == 0)
		{
			AUTH.AuthorizedGroup = -1;
			AUTH.Login(
				function login_callback() {
					if (AUTH.AuthorizedGroup>=0)/*login success*/
						BODY.GetCFG();
					else /*login fail show login page.*/
					{
						alert("password is not default(admin/blank),should not run here.");
						BODY.OnLoad();
					}
				},
				"admin",usr_pwd);

			this.logindefault=1;
		}	
		if(wiz_clonemac==1)
		{
			SetRadioValue("connect_mode", "dhcp");
			this.OnClickWANType();
			this.currentStage = 5; //goto init_dhcp stage
			this.ShowCurrentStage();
		}	
	},
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
	{
		switch (code)
		{
		case "OK":
			if(this.maccloned)
			{
				var msgArray = ['<?echo I18N("j","It'll take a moment, please wait");?>...'];
				BODY.ShowCountdown('<?echo I18N("j","Clone MAC Address");?>...', msgArray, this.bootuptime, "http://<?echo $_SERVER['HTTP_HOST'];?>");
				break;
			}
			this.currentStage++;

			if(this.stages[this.currentStage] == "init_status")
			{
				setTimeout('PAGE.show_init_status()', 15000);
				
				return true;
			}
			this.ShowCurrentStage();
			break;
		case "BUSY":
			BODY.ShowMessage("Error","<?echo I18N("j","Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			this.ErrorHandler(result.Get("/hedwig/node"), result.Get("/hedwig/message"));
			break;
		case "PIGWIDGEON":
			if (result.Get("/pigwidgeon/message")==="no power")
				BODY.NoPower();
			else
				BODY.ShowMessage("Error",result.Get("/pigwidgeon/message"));
			break;
		}
		BODY.ShowContent();
		return true;
	},	
	/* This handler handles error checking messgae from fatlady */
	ErrorHandler: function(node, msg)
	{
		var wantype = GetRadioValue("connect_mode");
		var prefix = "";

		if (node.indexOf("macaddr")>0)	{ BODY.ShowConfigError("macaddr", msg); return; }
		if (node.indexOf("username")>0)	{ BODY.ShowConfigError(wantype+"_user", msg); return; }
		if (node.indexOf("server")>0)	{ BODY.ShowConfigError(wantype+"_svr", msg); return; }
		if (node.indexOf("ipaddr")>0)	{ BODY.ShowConfigError(wantype+"_ip", msg); return; }
		if (node.indexOf("mask")>0)		{ BODY.ShowConfigError(wantype+"_mask", msg); return; }
		if (node.indexOf("dns")>0)		{ BODY.ShowConfigError(wantype+"_dns", msg); return; }
		if (node.indexOf("gateway")>0)	{ BODY.ShowConfigError(wantype+"_gw", msg); return; }
		if (node.indexOf(this.wifip)>0)			prefix = "wlan_";
		else if (node.indexOf(this.gzonep)>0)	prefix = "gz_";
		if (prefix!="")
		{
			if (node.indexOf("ssid")>0)	{ BODY.ShowConfigError(prefix+"ssid", msg); return; }
			if (node.indexOf("key")>0)	{ BODY.ShowConfigError(prefix+"key", msg); return; }
		}
		this.ShowCurrentStage();
		BODY.ShowMessage("Error", msg);
	},
	
	/* Things we need to do with the configuration.
	 * Some implementation will have visual elements and hidden elements,
	 * both handle the same item. Synchronize is used to sync the visual to the hidden. */
	Synchronize: null,
	
	/* The page specific dirty check function. */
	IsDirty: null,
	
	/* The "services" will be requested by GetCFG(), then the InitValue() will be
	 * called to process the services XML document. */
	InitValue: function(xml)
	{
		PXML.doc = xml;
		//xml.dbgdump();

		if (!this.Initial()) return false;
		if (!this.InitTZ()) return false;
		if (!this.InitWANSettings()) return false;
		if (!this.InitWLAN()) return false;
		this.ShowCurrentStage();
		return true;
	},
	PreSubmit: function()
	{
		this.PrePasswd();
		this.PreWANSettings();
		this.PreWLAN();
		if (!this.PreTZ()) return null;	
		
		var device_p = PXML.FindModule("WIFI.PHYINF");		
		XS(device_p+"/device/wiz_freset", 1);		
		
		PXML.ActiveModule("DEVICE.ACCOUNT");
		PXML.ActiveModule("DEVICE.TIME");
		PXML.ActiveModule("RUNTIME.TIME");	
		PXML.ActiveModule("WIFI.PHYINF");
		PXML.CheckModule("PHYINF.WIFI", "ignore", "ignore", null);
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		PXML.CheckModule("INET.WAN-2", null, null, "ignore");
		//PXML.CheckModule("PHYINF.WAN-1", null, null, "ignore");
		PXML.CheckModule("PHYINF.WAN-1", null, "ignore", "ignore");
		PXML.CheckModule("WAN", "ignore", "ignore", null);
		
		//PXML.doc.dbgdump();	
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	dual_band: COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>'),
	passwdp: null,
	devtime_p: null,
	runtime_p: null,
	inet1p: null,
	inet2p: null,
	inf1p: null,
	inf2p: null,
	macaddrp: null,
	wifip: null,
	gzonep: null,
	en_gzone: false,
	phyinf: null,
	gzphyinf: null,
	stages: new Array("init_intro","init_passwd","init_wandetect","init_wantype","init_dhcp","init_wlan","init_status"),
	wanTypes: new Array("dhcp", "pppoe", "pptp", "l2tp", "static"),
	en_static_pppoe: false,
	en_static_pptp: false,
	en_static_l2tp: false,
	wanDetectCheckNum: 0,
	wanDetectNum: 0,
	wanDetectResult: null,
	wanDetectCheckTimer: null,
	wanDetectTimer: null,
	currentStage: 0,	// 0 ~ this.stages.length
	currentWanType: 0,	// 0 ~ this.wanTypes.length
	waninetp: null,
	rwaninetp: null,
	rwanphyp: null,
	maccloned: false,
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="")$bt=30;
		else		$bt=$bt+10;
		echo $bt; ?>,
	Initial: function()
	{
		this.passwdp = PXML.FindModule("DEVICE.ACCOUNT");
		if (!this.passwdp)
		{
			BODY.ShowMessage("<?echo I18N("j", "Error");?>","Initial() ERROR!!!");
			return false;
		}
		this.passwdp = GPBT(this.passwdp+"/device/account", "entry", "name", "admin", false);
		this.passwdp += "/password";
		OBJ("usr_pwd").value = OBJ("usr_pwd2").value = XG(this.passwdp);

		return true;
	},
	PreTZ: function()
	{
		XS(this.devtime_p+"/device/time/timezone", OBJ("pc_timezone_offset").value);
		XS(this.devtime_p+"/device/time/date", OBJ("pc_date").value);
		XS(this.devtime_p+"/device/time/time", OBJ("pc_time").value);
		XS(this.runtime_p+"/runtime/device/date", OBJ("pc_date").value);
		XS(this.runtime_p+"/runtime/device/time", OBJ("pc_time").value);		
			
		return true;
	},	
	InitTZ: function()
	{
		this.runtime_p = PXML.FindModule("RUNTIME.TIME");
		this.devtime_p = PXML.FindModule("DEVICE.TIME");
		if (!this.devtime_p)
		{
			BODY.ShowMessage("<?echo I18N("j", "Error");?>","Initial() ERROR!!!");
			return false;
		}
	
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
									
		var timezone_array=[''<? foreach("/runtime/services/timezone/zone")	{ $gen=query("gen"); if($gen!=""){echo ",'".$gen."_Index=".$InDeX."'\n";}	}	?>];	
		var timezone_index = "";
		for(var i=1; i < timezone_array.length; i++)
		{
			if(time_offset_string===timezone_array[i].substr(3,6))
			{
				if(timezone_array[i].length===17) timezone_index = timezone_array[i].substr(16,1);
				else timezone_index = timezone_array[i].substr(16,2);	
				break;
			}	
		}	
		OBJ("pc_timezone_offset").value = timezone_index;
		
		/* Auto time settings */
		var dateObj = new Date();
		var date = (dateObj.getMonth()+1)+"/"+dateObj.getDate()+"/"+dateObj.getFullYear();
		var time = dateObj.getHours()+":"+dateObj.getMinutes()+":"+dateObj.getSeconds();	
		OBJ("pc_date").value = date;
		OBJ("pc_time").value = time;	
				
		return true;
	},		
	PrePasswd: function()
	{
		if (COMM_EatAllSpace(OBJ("usr_pwd").value)=="")
		{
			BODY.ShowConfigError("usr_pwd", "<?echo I18N("j","The password cannot be empty");?>");
			return false;
		}
		if (OBJ("usr_pwd").value!=OBJ("usr_pwd2").value)
		{
			BODY.ShowConfigError("usr_pwd", "<?echo I18N("j","Password and Verify Password do not match.");?>");
			return false;
		}
		XS(this.passwdp, OBJ("usr_pwd").value);
		OBJ("st_pwd").innerHTML = OBJ("usr_pwd").value;
		return true;
	},
	InitWANSettings: function()
	{
		this.inet1p = PXML.FindModule("INET.WAN-1");
		this.inet2p = PXML.FindModule("INET.WAN-2");
		var phyinfp = PXML.FindModule("PHYINF.WAN-1");
		if (!this.inet1p||!this.inet2p||!phyinfp)
		{
			BODY.ShowMessage("<?echo I18N("j", "Error");?>","InitWANSettings() ERROR!!!");
			return false;
		}
		var inet1 = XG(this.inet1p+"/inf/inet");
		var inet2 = XG(this.inet2p+"/inf/inet");
		this.inf1p = this.inet1p+"/inf";
		this.inf2p = this.inet2p+"/inf";
		this.inet1p = GPBT(this.inet1p+"/inet", "entry", "uid", inet1, false);
		this.inet2p = GPBT(this.inet2p+"/inet", "entry", "uid", inet2, false);
		phyinfp = GPBT(phyinfp, "phyinf", "uid", XG(phyinfp+"/inf/phyinf"), false);
 
		this.macaddrp = phyinfp+"/macaddr";
		this.GetWanType();
		switch (this.wanTypes[this.currentWanType])
		{
		/////////////////////////// initial DHCP settings ///////////////////////////
		case "dhcp":
			if (XG(this.macaddrp)!="")
				TEMP_SetFieldsByDelimit("macaddr", XG(this.macaddrp), ':');
			break;
		/////////////////////////// initial PPPoE settings ///////////////////////////
		case "pppoe":
			OBJ("pppoe_user").value = XG(this.inet1p+"/ppp4/username");
			OBJ("pppoe_pwd").value = XG(this.inet1p+"/ppp4/password");
			if (XG(this.inet1p+"/ipv4/static")=="1")
			{
				TEMP_SetFieldsByDelimit("pppoe_ip", XG(this.inet1p+"/ppp4/ipaddr"), '.');
				TEMP_SetFieldsByDelimit("pppoe_dns", XG(this.inet1p+"/ppp4/dns/entry"), '.');
			}
			break;
		/////////////////////////// initial PPTP settings ///////////////////////////
		case "pptp":
			if (XG(this.inet2p+"/ipv4/static")=="1")
			{
				TEMP_SetFieldsByDelimit("pptp_ip", XG(this.inet2p+"/ipv4/ipaddr"), '.');
				TEMP_SetFieldsByDelimit("pptp_mask", COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask")), '.');
				TEMP_SetFieldsByDelimit("pptp_gw", XG(this.inet2p+"/ipv4/gw"), '.');
			}
			TEMP_SetFieldsByDelimit("pptp_svr", XG(this.inet1p+"/ppp4/pptp/server"), '.');
			OBJ("pptp_user").value = XG(this.inet1p+"/ppp4/username");
			OBJ("pptp_pwd").value = XG(this.inet1p+"/ppp4/password");
			break;
		/////////////////////////// initial L2TP settings ///////////////////////////
		case "l2tp":
			if (XG(this.inet2p+"/ipv4/static")=="1")
			{
				TEMP_SetFieldsByDelimit("l2tp_ip", XG(this.inet2p+"/ipv4/ipaddr"), '.');
				TEMP_SetFieldsByDelimit("l2tp_mask", COMM_IPv4INT2MASK(XG(this.inet2p+"/ipv4/mask")), '.');
				TEMP_SetFieldsByDelimit("l2tp_gw", XG(this.inet2p+"/ipv4/gw"), '.');
			}
			TEMP_SetFieldsByDelimit("l2tp_svr", XG(this.inet1p+"/ppp4/l2tp/server"), '.');
			OBJ("l2tp_user").value = XG(this.inet1p+"/ppp4/username");
			OBJ("l2tp_pwd").value = XG(this.inet1p+"/ppp4/password");
			break;
		/////////////////////////// initial STATIC IP settings ///////////////////////////
		case "static":
			TEMP_SetFieldsByDelimit("static_ip", XG(this.inet1p+"/ipv4/ipaddr"), '.');
			TEMP_SetFieldsByDelimit("static_mask", COMM_IPv4INT2MASK(XG(this.inet1p+"/ipv4/mask")), '.');
			TEMP_SetFieldsByDelimit("static_gw", XG(this.inet1p+"/ipv4/gateway"), '.');
			TEMP_SetFieldsByDelimit("static_dns", XG(this.inet1p+"/ipv4/dns/entry"), '.');
			break;
		default:
		}
		return true;
	},
	PreWANSettings: function()
	{
		var type = GetRadioValue("connect_mode");
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
		switch (type)
		{
		/////////////////////////// prepare DHCP settings ///////////////////////////
		case "dhcp":
			XS(this.inet1p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ipv4/static", 0);
			XS(this.inet1p+"/ipv4/mtu", 1500);
			XS(this.inet1p+"/ipv4/dns/count", 0);
			
			/* If mac is changed, restart PHYINF.WAN-1 and WAN, else restart WAN. */
			var objs = document.getElementsByName("macaddr");
			for (var i=0; i<objs.length; i++)
			{
				if (!COMM_Equal(objs[i].getAttribute("default"), objs[i].value))
				{	
					this.maccloned = true;
					break;
				}
			}
			if(this.maccloned==true)			
			{
				var macaddr = TEMP_GetFieldsValue("macaddr", ':');
				var device_p = PXML.FindModule("WIFI.PHYINF");	
				XS(this.macaddrp, macaddr);	
				XS(device_p+"/device/wiz_clonemac", 1);				
			}					

			OBJ("st_wanmode").innerHTML = "<?echo I18N("h","DHCP");?>";
			break;
		/////////////////////////// prepare STATIC IP settings ///////////////////////////
		case "static":
			XS(this.inet1p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ipv4/static", 1);
			XS(this.inet1p+"/ipv4/mtu", 1500);
			XS(this.inet1p+"/ipv4/ipaddr",		TEMP_GetFieldsValue("static_ip", '.'));
			XS(this.inet1p+"/ipv4/mask",		COMM_IPv4MASK2INT(TEMP_GetFieldsValue("static_mask", '.')));
			XS(this.inet1p+"/ipv4/gateway",		TEMP_GetFieldsValue("static_gw", '.'));
			XS(this.inet1p+"/ipv4/dns/entry",	TEMP_GetFieldsValue("static_dns", '.'));
			XS(this.inet1p+"/ipv4/dns/count", 1);
			
			OBJ("st_wanmode").innerHTML = "<?echo I18N("h","Static IP");?>";
			OBJ("st_ipaddr").innerHTML = TEMP_GetFieldsValue("static_ip", '.');
			OBJ("st_mask").innerHTML = TEMP_GetFieldsValue("static_mask", '.');
			OBJ("st_gw").innerHTML = TEMP_GetFieldsValue("static_gw", '.');
			OBJ("st_dns").innerHTML = TEMP_GetFieldsValue("static_dns", '.');
			var waninfo = new Array("f_ip","f_mask","f_gw","f_dns");
			for (var i=0; i<waninfo.length; i++)
				OBJ(waninfo[i]).style.display = "block";
			break;
		/////////////////////////// prepare PPPoE settings ///////////////////////////
		case "pppoe":
			XS(this.inet1p+"/addrtype", "ppp4");
			XS(this.inet1p+"/ppp4/mtu", 1492);
			XS(this.inet1p+"/ppp4/over", "eth");
			XS(this.inet1p+"/ppp4/static", this.en_static_pppoe? 1:0);
			if (this.en_static_pppoe)
			{
				XS(this.inet1p+"/ppp4/ipaddr", TEMP_GetFieldsValue("pppoe_ip", '.'));
				XS(this.inet1p+"/ppp4/dns/entry", TEMP_GetFieldsValue("pppoe_dns", '.'));
				XS(this.inet1p+"/ppp4/dns/count", 1);
				OBJ("st_ipaddr").innerHTML = TEMP_GetFieldsValue("pppoe_ip", '.');
				OBJ("st_dns").innerHTML = TEMP_GetFieldsValue("pppoe_dns", '.');
				var waninfo = new Array("f_ip","f_dns");
				for (var i=0; i<waninfo.length; i++)
					OBJ(waninfo[i]).style.display = "block";
			}
			XS(this.inet1p+"/ppp4/username", OBJ("pppoe_user").value);
			XS(this.inet1p+"/ppp4/password", OBJ("pppoe_pwd").value);
			
			OBJ("st_wanmode").innerHTML = "<?echo I18N("h","PPPoE");?>";
			OBJ("st_pppname").innerHTML = OBJ("pppoe_user").value;
			OBJ("st_ppppwd").innerHTML = OBJ("pppoe_pwd").value;
			var waninfo = new Array("f_pppname","f_ppppwd");
			for (var i=0; i<waninfo.length; i++)
				OBJ(waninfo[i]).style.display = "block";
			break;
		/////////////////////////// prepare PPTP settings ///////////////////////////
		case "pptp":
			XS(this.inf2p+"/active", 1);
			XS(this.inet1p+"/addrtype", "ppp4");
			XS(this.inet2p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ppp4/mtu", 1400);
			XS(this.inet1p+"/ppp4/over", "pptp");
			if (this.en_static_pptp)
			{
				XS(this.inet2p+"/ipv4/static", 1);
				XS(this.inet2p+"/ipv4/ipaddr",  TEMP_GetFieldsValue("pptp_ip", '.'));
				XS(this.inet2p+"/ipv4/mask",    COMM_IPv4MASK2INT(TEMP_GetFieldsValue("pptp_mask", '.')));
				XS(this.inet2p+"/ipv4/gateway", TEMP_GetFieldsValue("pptp_gw", '.'));
				XS(this.inet2p+"/ipv4/dns/entry", TEMP_GetFieldsValue("pptp_dns", '.'));
				XS(this.inet2p+"/ipv4/dns/count", 1);
				OBJ("st_ipaddr").innerHTML = TEMP_GetFieldsValue("pptp_ip", '.');
				OBJ("st_mask").innerHTML = TEMP_GetFieldsValue("pptp_mask", '.');
				OBJ("st_gw").innerHTML = TEMP_GetFieldsValue("pptp_gw", '.');
				OBJ("st_dns").innerHTML = TEMP_GetFieldsValue("pptp_dns", '.');
				var waninfo = new Array("f_ip","f_mask","f_gw","f_dns");
				for (var i=0; i<waninfo.length; i++)
					OBJ(waninfo[i]).style.display = "block";
			}
			XS(this.inet1p+"/ppp4/pptp/server", TEMP_GetFieldsValue("pptp_svr", '.'));
			XS(this.inet1p+"/ppp4/username", OBJ("pptp_user").value);
			XS(this.inet1p+"/ppp4/password", OBJ("pptp_pwd").value);
			XS(this.inf1p+"/lowerlayer", "WAN-2");
			XS(this.inf2p+"/upperlayer", "WAN-1");
			
			OBJ("st_wanmode").innerHTML = "<?echo I18N("h","PPTP");?>";
			OBJ("st_pppname").innerHTML = OBJ("pptp_user").value;
			OBJ("st_ppppwd").innerHTML = OBJ("pptp_pwd").value;
			OBJ("st_svr").innerHTML = TEMP_GetFieldsValue("pptp_svr", '.');
			var waninfo = new Array("f_pppname","f_ppppwd","f_svr");
			for (var i=0; i<waninfo.length; i++)
				OBJ(waninfo[i]).style.display = "block";
			break;
		/////////////////////////// prepare L2TP settings ///////////////////////////
		case "l2tp":
			XS(this.inf2p+"/active", 1);
			XS(this.inet1p+"/addrtype", "ppp4");
			XS(this.inet2p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ppp4/mtu", 1400);
			XS(this.inet1p+"/ppp4/over", "l2tp");
			if (this.en_static_l2tp)
			{
				XS(this.inet2p+"/ipv4/static", 1);
				XS(this.inet2p+"/ipv4/ipaddr",  TEMP_GetFieldsValue("l2tp_ip", '.'));
				XS(this.inet2p+"/ipv4/mask",    COMM_IPv4MASK2INT(TEMP_GetFieldsValue("l2tp_mask", '.')));
				XS(this.inet2p+"/ipv4/gateway", TEMP_GetFieldsValue("l2tp_gw", '.'));
				XS(this.inet2p+"/ipv4/dns/entry", TEMP_GetFieldsValue("l2tp_dns", '.'));
				XS(this.inet2p+"/ipv4/dns/count", 1);
				OBJ("st_ipaddr").innerHTML = TEMP_GetFieldsValue("l2tp_ip", '.');
				OBJ("st_mask").innerHTML = TEMP_GetFieldsValue("l2tp_mask", '.');
				OBJ("st_gw").innerHTML = TEMP_GetFieldsValue("l2tp_gw", '.');
				OBJ("st_dns").innerHTML = TEMP_GetFieldsValue("l2tp_dns", '.');
				var waninfo = new Array("f_ip","f_mask","f_gw","f_dns");
				for (var i=0; i<waninfo.length; i++)
					OBJ(waninfo[i]).style.display = "block";
			}
			XS(this.inet1p+"/ppp4/l2tp/server", TEMP_GetFieldsValue("l2tp_svr", '.'));
			XS(this.inet1p+"/ppp4/username",    OBJ("l2tp_user").value);
			XS(this.inet1p+"/ppp4/password",    OBJ("l2tp_pwd").value);
			XS(this.inf1p+"/lowerlayer", "WAN-2");
			XS(this.inf2p+"/upperlayer", "WAN-1");
			
			OBJ("st_wanmode").innerHTML = "<?echo I18N("h","L2TP");?>";
			OBJ("st_pppname").innerHTML = OBJ("l2tp_user").value;
			OBJ("st_ppppwd").innerHTML = OBJ("l2tp_pwd").value;
			OBJ("st_svr").innerHTML = TEMP_GetFieldsValue("l2tp_svr", '.');
			var waninfo = new Array("f_pppname","f_ppppwd","f_svr");
			for (var i=0; i<waninfo.length; i++)
				OBJ(waninfo[i]).style.display = "block";
			break;
		default:
		}
		if (type=="dhcp"||type=="static")
		{
			XS(this.inet2p+"/ipv4/static", 0);
			XS(this.inet2p+"/ipv4/ipaddr", "");
			XS(this.inet2p+"/ipv4/mask", "");
			XS(this.inet2p+"/ipv4/gateway", "");
		}
		else
		{
			XS(this.inet1p+"/ppp4/dialup/mode", "auto");
		}

		return true;
	},
	InitWLAN: function()
	{
		var wlanbase = PXML.FindModule("WIFI.PHYINF");
		this.phyinf = GPBT(wlanbase, "phyinf", "uid", "BAND24G-1.1", false);
		//this.gzonep = GPBT(wlanbase, "phyinf", "uid", "BAND24G-1.2", false);
		this.gzphyinf = GPBT(wlanbase, "phyinf", "uid", "BAND24G-1.2", false);
		this.wifip = GPBT(wlanbase+"/wifi", "entry", "uid", XG(this.phyinf+"/wifi"), false);
		this.gzonep = GPBT(wlanbase+"/wifi", "entry", "uid",  XG(this.gzphyinf+"/wifi"), false);
		if (!this.wifip||!this.gzonep)
		{
			BODY.ShowMessage("<?echo I18N("j", "Error");?>","Initial() ERROR!!!");
			return false;
		}
		OBJ("wlan_ssid").value = XG(this.wifip+"/ssid");
		OBJ("wlan_key").value = XG(this.wifip+"/nwkey/psk/key");
		OBJ("gz_ssid").value = XG(this.gzonep+"/ssid");
		OBJ("gz_key").value = XG(this.gzonep+"/nwkey/psk/key");

		return true;
	},
	PreWLAN: function()
	{
		var wlanif = [["wlan_", this.wifip]];
		if (this.en_gzone)
			wlanif[1] = ["gz_", this.gzonep];
		for (var i=0; i<wlanif.length; i++)
		{
			XS(wlanif[i][1]+"/ssid", OBJ(wlanif[i][0]+"ssid").value);
			XS(wlanif[i][1]+"/ssidhidden", 0);
			XS(wlanif[i][1]+"/authtype", "WPA+2PSK");
			XS(wlanif[i][1]+"/encrtype", "TKIP+AES");
			XS(wlanif[i][1]+"/nwkey/psk/passphrase", "");
			XS(wlanif[i][1]+"/nwkey/psk/key", OBJ(wlanif[i][0]+"key").value);
			XS(wlanif[i][1]+"/nwkey/wpa/groupintv", 3600);
		}
		XS(this.wifip+"/wps/configured", "1");
		XS(this.wifip+"/wps/locksecurity", "1");
		XS(this.phyinf+"/active", "1");
		if (this.en_gzone)	XS(this.gzphyinf+"/active", "1");
		OBJ("st_ssid").innerHTML = OBJ("wlan_ssid").value;
		OBJ("st_key").innerHTML = OBJ("wlan_key").value;

		return true;
	},
	GetWanType: function()
	{
		var prefix = "";
		var type = "";
		switch (XG(this.inet1p+"/addrtype"))
		{
		case "ipv4":
			type = (XG(this.inet1p+"/ipv4/static")=="0")? "dhcp":"static";
			break;
		case "ppp4":
		case "ppp10":
			if (XG(this.inf2p+"/active")=="1"&&XG(this.inf2p+"/nat")=="NAT-1")
				prefix = "r_";
			var over = XG(this.inet1p+"/ppp4/over");
			if (over=="eth")		type = prefix + "pppoe";
			else if (over=="pptp")	type = prefix + "pptp";
			else if (over=="l2tp")	type = prefix + "l2tp";
			break;
		default:
		}
		for (var i=0; i<this.wanTypes.length; i++)
			if (this.wanTypes[i]==type) this.currentWanType = i;
			
	},
	OnClickCloneMAC: function()
	{
		TEMP_SetFieldsByDelimit("macaddr", "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>", ':');
	},
	OnClickPre: function()
	{
		this.currentStage--;
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		var stage = this.stages[this.currentStage];
		var precheck = false;
		PXML.IgnoreModule("DEVICE.ACCOUNT");
		PXML.IgnoreModule("WIFI.PHYINF");
		PXML.IgnoreModule("PHYINF.WIFI");
		PXML.IgnoreModule("INET.WAN-1");
		PXML.IgnoreModule("INET.WAN-2");
		PXML.IgnoreModule("PHYINF.WAN-1");
		PXML.IgnoreModule("WAN");
		PXML.IgnoreModule("RUNTIME.INF.WAN-1");
		if (stage=="init_passwd")
		{
			BODY.ClearConfigError();
			if (COMM_EatAllSpace(OBJ("usr_pwd").value)=="")
			{
				BODY.ShowConfigError("usr_pwd", "<?echo I18N("j","The password cannot be empty");?>");
				return false;
			}
			if (OBJ("usr_pwd").value!=XG(this.passwdp)||OBJ("usr_pwd").value!=OBJ("usr_pwd2").value)
			{
				precheck = true;
				if (!this.PrePasswd()) return;
				PXML.CheckModule("DEVICE.ACCOUNT", null, "ignore", "ignore");
			}
			if(!this.WanCableDetect()) this.currentStage++; //skip stage "init_wandetect" if no wan connected
		}
		else if (stage=="init_dhcp")
		{
			precheck = true;
			this.PreWANSettings();

			if (this.maccloned)
			{
				this.PrePasswd();
				if (!this.PreTZ()) return null;		
				
				PXML.ActiveModule("DEVICE.ACCOUNT");
				PXML.ActiveModule("DEVICE.TIME");
				PXML.ActiveModule("RUNTIME.TIME");		
				PXML.ActiveModule("WIFI.PHYINF");
//				PXML.CheckModule("PHYINF.WIFI", "ignore", "ignore", null);
				PXML.CheckModule("INET.WAN-1", null, null, "ignore");
				PXML.CheckModule("INET.WAN-2", null, null, "ignore");
				PXML.ActiveModule("PHYINF.WAN-1");
				PXML.CheckModule("WAN", null, "ignore", null);
			}
			else
			{
				PXML.CheckModule("INET.WAN-1", null, "ignore", "ignore");
				PXML.CheckModule("INET.WAN-2", null, "ignore", "ignore");
				PXML.CheckModule("PHYINF.WAN-1", null, "ignore", "ignore");				
			}
		}
		else if (stage=="init_static"||stage=="init_pppoe"||stage=="init_pptp"||stage=="init_l2tp")
		{
			precheck = true;
			this.PreWANSettings();
			PXML.CheckModule("INET.WAN-1", null, "ignore", "ignore");
			PXML.CheckModule("INET.WAN-2", null, "ignore", "ignore");
			PXML.CheckModule("PHYINF.WAN-1", null, "ignore", "ignore");
		}
//		else if (stage=="init_wlan")
//		{
//			precheck = true;
//			this.PreWLAN();
//			PXML.CheckModule("WIFI.PHYINF", null, "ignore", "ignore");
//		}

		if (precheck)
		{
			BODY.ClearConfigError();
			BODY.ShowInProgress();
			PXML.UpdatePostXML(PXML.doc);
			PXML.Post(function(code, result){PAGE.OnSubmitCallback(code,result);});
		}
		else if (stage=="init_wlan")
		{
			BODY.OnSubmit();
		}
		else
		{
			this.currentStage++;
			this.ShowCurrentStage();
		}
	},
	ShowCurrentStage: function()
	{
		var stage = this.stages[this.currentStage];
		if (stage=="init_intro")
		{
			OBJ("b_back").style.display = "none";
			/* Wan detect would act when the wizard setup starts */
			if (!this.wanDetectTimer)
			{
				this.wanDetectResult = "";
				this.wanDetectNum = 0;
				this.WanDetect("WANDETECT");
			}
		}
		else if (stage=="init_wandetect")
		{
			OBJ("b_back").style.display = "none";
			OBJ("b_next").style.display = "none";
			this.wanDetectCheckNum = 0;
			this.wanDetectCheckTimer = setTimeout('PAGE.WanDetectCheck()', 2000);
		}
		else if (stage=="init_wantype"||stage=="init_dhcp"||stage=="init_static"||stage=="init_pppoe"||stage=="init_pptp"||stage=="init_l2tp")
		{
			OBJ("b_back").style.display = "block";
			OBJ("b_next").style.display = "block";
			/* stage "init_wandetect" will disappear after run it */
			var len = this.stages.length;
			var wantype = GetRadioValue("connect_mode");
			this.stages = new Array("init_intro","init_passwd","init_wantype","init_"+wantype,"init_wlan","init_status");
			this.currentStage = this.currentStage - len + this.stages.length;
			OBJ("init_wandetect").style.display = "none";;
		}
		else if (stage=="init_status")
		{
			var networkConnected = this.NetworkStatusDetect();
			if(networkConnected==1)
			{
				OBJ("b_casa").onclick = function(){self.location.href='http://www.miiicasa.com/doc/welcome';};
				OBJ("b_casa_text1").innerHTML = OBJ("b_casa_text2").innerHTML = '<?echo I18N("h","Experience miiiCasa now!");?>';
			}
			else
			{
				OBJ("b_casa").onclick = function(){self.location.href='./home.php';};
				OBJ("b_casa_text1").innerHTML = OBJ("b_casa_text2").innerHTML = '<?echo I18N("h","Go to Router Setup Page");?>';	
			}		
			
			OBJ("b_next").style.display = "none";
			OBJ("b_casa").style.display = "block";
		}
		else
		{
			OBJ("b_back").style.display = "block";
			OBJ("b_next").style.display = "block";
			OBJ("b_casa").style.display = "none";
		}
		for (var i=0; i<this.wanTypes.length; i++)
			OBJ("init_"+this.wanTypes[i]).style.display = "none";
		for (var i=0; i<this.stages.length; i++)
			OBJ(this.stages[i]).style.display = (i==this.currentStage)? "block":"none";

	},
	GenKey: function(prefix)
	{
		var c = "0123456789abcdef";
		var str = '';
		for (var i=0; i<8; i++)
		{
			var rand_char = Math.floor(Math.random() * c.length);
			str += c.substring(rand_char, rand_char + 1);
		}
		OBJ(prefix+"key").value = str;
	},
	GZToggle: function()
	{
		this.en_gzone = !this.en_gzone;
		if (this.en_gzone)
			OBJ("gzone").style.display = "block";
		else
			OBJ("gzone").style.display = "none";
	},
	OnClickPPPStaticIP: function(type)
	{
		var en = false;
		if (type=="pppoe")
		{
			this.en_static_pppoe = !this.en_static_pppoe;
			en = this.en_static_pppoe;
		}
		else if (type=="pptp")
		{
			this.en_static_pptp = !this.en_static_pptp;
			en = this.en_static_pptp;
		}
		else if (type=="l2tp")
		{
			this.en_static_l2tp = !this.en_static_l2tp;
			en = this.en_static_l2tp;
		}

		if (en)
			OBJ(type+"_static").style.display = "block";
		else
			OBJ(type+"_static").style.display = "none";

		/* hide all wan status fields */
		var waninfo = new Array("f_pppname","f_ppppwd","f_ip","f_mask","f_gw","f_dns");
		for (var i=0; i<waninfo.length; i++)
			OBJ(waninfo[i]).style.display = "none";
	},
	OnClickWANType: function()
	{
		var objs = document.getElementsByName("connect_mode");
		for (var i=0; i<objs.length; i++)
		{
			if (objs[i].checked)
			{
				this.stages[4] = "init_" + objs[i].value;
			}
		}
		/* clear all error message */
		BODY.ClearConfigError();
		/* reload config to avoid XS() effect in preXXXX() */
		COMM_GetCFG(false, PAGE.services, function(xml) {PXML.doc = xml;});
		/* hide all wan status fields */
		var waninfo = new Array("f_pppname","f_ppppwd","f_ip","f_mask","f_gw","f_dns");
		for (var i=0; i<waninfo.length; i++)
			OBJ(waninfo[i]).style.display = "none";
	},
	WanDetect: function(action)
	{
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
		switch (result)
		{
		case "OK":
			this.wanDetectTimer = setTimeout('PAGE.WanDetect("WANTYPERESULT")', 2000);
			break;
		case "DHCP":
			this.wanDetectResult = "dhcp";
			break;
		case "PPPoE":
			this.wanDetectResult = "pppoe";
			break;
		case "None":
			this.wanDetectResult = "none";
			break;
		case "unknown":
			this.wanDetectResult = "unknown";
		case "":
		case "detecting":
			if (this.wanDetectNum < 10)
			{
				clearTimeout(this.wanDetectTimer);
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
	WanDetectCheck: function()
	{
		if (this.stages[this.currentStage]!=="init_wandetect") return;

		if (this.wanDetectResult===""&&this.wanDetectCheckNum<20)
		{
			clearTimeout(this.wanDetectCheckTimer);
			this.wanDetectCheckTimer = setTimeout('PAGE.WanDetectCheck()', 1000);
		}
		else if (this.wanDetectResult==="dhcp"||this.wanDetectResult==="pppoe")
		{
			SetRadioValue("connect_mode", this.wanDetectResult);
			this.OnClickWANType();
			this.currentStage = 4;
			this.ShowCurrentStage();
		}
		else
		{
			// If the wan detect is fail at the first time, do it again.
			//if (this.wanDetectCheckNum === 0) this.WanDetectAgain(); //rbj

			SetRadioValue("connect_mode", "static");
			this.OnClickWANType();
			this.currentStage = 4;
			this.ShowCurrentStage();
		}
		this.wanDetectCheckNum++;
	},
	WanDetectAgain: function()
	{
		clearTimeout(this.wanDetectTimer);
		clearTimeout(this.wanDetectCheckTimer);
		this.wanDetectResult = "";
		this.wanDetectNum = 0;
		this.WanDetect("WANDETECT");
		this.wanDetectCheckNum = 0;
		this.WanDetectCheck();
	},
	WanCableDetect: function()
	{
		var wan	= PXML.FindModule("INET.WAN-1");
		var wanphyuid = XG(wan+"/inf/phyinf");
		var rphy = PXML.FindModule("RUNTIME.PHYINF");
		var rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", wanphyuid, false);
		var connect = XG(rwanphyp+"/linkstatus");
        return ((connect!="0")&&(connect!=""))?true:false;
	},
	NetworkStatusDetect: function ()
	{
		var wan	= PXML.FindModule("INET.WAN-1");
		var rwan = PXML.FindModule("RUNTIME.INF.WAN-1");
		var rphy = PXML.FindModule("RUNTIME.PHYINF");
		var waninetuid = XG  (wan+"/inf/inet");
		var wanphyuid = XG  (wan+"/inf/phyinf");
		this.waninetp = GPBT(wan+"/inet", "entry", "uid", waninetuid, false);
		this.rwaninetp = GPBT(rwan+"/runtime/inf", "inet", "uid", waninetuid, false);      
		this.rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", wanphyuid, false);     			
        var wancable_status=0;
		var wan_network_status=0;
		
		if ((!this.waninetp))
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","WanInetDetect() ERROR!!!");
			return false;
		}
        if((XG  (this.rwanphyp+"/linkstatus")!="0")&&(XG  (this.rwanphyp+"/linkstatus")!="")) {wancable_status=1;}	
           
		if (XG  (this.waninetp+"/addrtype") == "ipv4")
		{
			if(XG(this.waninetp+"/ipv4/ipv4in6/mode")!="")
			{
				if (wancable_status==1)
					wan_network_status=1;
			}
			else
			{
				if(XG  ( this.waninetp+"/ipv4/static")== "1")
					wan_network_status=wancable_status;
				else
					if ((XG  (this.rwaninetp+"/ipv4/valid")== "1")&& (wancable_status==1)) 
						wan_network_status=1;
		  	}
		}
		else if (XG  (this.waninetp+"/addrtype") == "ppp4" || XG(this.waninetp+"/addrtype") == "ppp10")
		{	    					
			var connStat = XG(rwan+"/runtime/inf/pppd/status");
			if ((XG  (this.rwaninetp+"/ppp4/valid")== "1")&& (wancable_status==1))
				wan_network_status=1;
		    switch (connStat)
			{
                case "connected":
	           		break;
	            case "":
                case "disconnected":
                case "on demand":
                    wan_network_status=0;
	            	break;
                default:
	                break;
			}
		}	
			
		return wan_network_status;
	},	
	saveRouterInfo: function ()
	{
		window.location.href="/get_wiz_result.php?st_wanmode=" + OBJ("st_wanmode").innerHTML +
											"&st_pppname=" + OBJ("st_pppname").innerHTML +
											"&st_ppppwd=" + OBJ("st_ppppwd").innerHTML +
											"&st_svr=" + OBJ("st_svr").innerHTML +											
											"&st_ipaddr=" + OBJ("st_ipaddr").innerHTML +
											"&st_mask=" + OBJ("st_mask").innerHTML +	
											"&st_gw=" + OBJ("st_gw").innerHTML +	
											"&st_dns=" + OBJ("st_dns").innerHTML;
	},
	SetDelayTime: function(millis)
	{
		var date = new Date();
		var curDate = null;
		curDate = new Date();
		do { curDate = new Date(); }
		while(curDate-date < millis);
	},
	show_init_status: function()
	{
		BODY.GetCFG();
		this.ShowCurrentStage();
		BODY.ShowContent();
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

function GetRadioValue(name)
{
	var radio = document.getElementsByName(name);
	for (i=0; i<radio.length; i++)
		if (radio[i].checked) return radio[i].value;
}
function SetRadioValue(name, value)
{
	var radio = document.getElementsByName(name);
	for (i=0; i<radio.length; i++)
		if (radio[i].value==value) radio[i].checked = true;
}
//]]>
</script>
