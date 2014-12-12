<?
include "/htdocs/phplib/inet.php";
include "/htdocs/phplib/phyinf.php";
?><script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "PHYINF.WAN-1,INET.WAN-1,INET.WAN-2,WAN",
	OnLoad: function() {},
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
	{
		switch (code)
		{
		case "OK":
			if (this.maccloned)
			{
				var msgArray = ['<?echo I18N("j","It'll take a moment, please wait");?>...'];
				BODY.ShowCountdown('<?echo I18N("j","Clone MAC Address");?>...', msgArray, this.bootuptime, "http://<?echo $_SERVER['HTTP_HOST'];?>/index.php");
			}
			BODY.GetCFG();			
			this.currentStage++;
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
		/* DHCP error */
		if (OBJ("en_dhcp").checked)
		{
			if (node.indexOf("macaddr")>0)	{ BODY.ShowConfigError("macclone", msg); return; }
		}
		/* static IP error */
		else if (OBJ("en_static").checked)
		{
			if (node.indexOf("ipaddr")>0)	{ BODY.ShowConfigError("static_ipaddr", msg); return; }
			if (node.indexOf("mask")>0)		{ BODY.ShowConfigError("static_mask", msg); return; }
			if (node.indexOf("gateway")>0)	{ BODY.ShowConfigError("static_gw", msg); return; }
			if (node.indexOf("dns")>0)		{ BODY.ShowConfigError("static_dns", msg); return; }
		}
		/* PPPoE error */
		else if (OBJ("en_pppoe").checked)
		{
			if (node.indexOf("username")>0)	{ BODY.ShowConfigError("ppp_user", msg); return; }
			if (node.indexOf("ipaddr")>0)	{ BODY.ShowConfigError("pppoe_ipaddr", msg); return; }
			if (node.indexOf("dns")>0)		{ BODY.ShowConfigError("pppoe_dns", msg); return; }
		}

		this.currentStage = 2;
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

		this.inet1p = PXML.FindModule("INET.WAN-1");
		this.inet2p = PXML.FindModule("INET.WAN-2");
		this.phyinfp = PXML.FindModule("PHYINF.WAN-1");
		if (!this.inet1p||!this.inet2p||!this.phyinfp)
		{
			BODY.ShowMessage("Error","InitWANSettings() ERROR!!!");
			return false;
		}
		var inet1 = XG(this.inet1p+"/inf/inet");
		var inet2 = XG(this.inet2p+"/inf/inet");
		this.inf1p = this.inet1p+"/inf";
		this.inf2p = this.inet2p+"/inf";
		this.inet1p = GPBT(this.inet1p+"/inet", "entry", "uid", inet1, false);
		this.inet2p = GPBT(this.inet2p+"/inet", "entry", "uid", inet2, false);
		this.phyinfp = GPBT(this.phyinfp, "phyinf", "uid", XG(this.phyinfp+"/inf/phyinf"), false);

		this.InitWANSettings();
		this.ShowCurrentStage();

		return true;
	},
	PreSubmit: function(uid_wlan)
	{
		var objs = document.getElementsByName("macclone");
		for (var i=0; i<objs.length; i++)
		{
			if (COMM_Equal(objs[i].getAttribute("modified"), true))
			{
				this.maccloned = true;
				break;
			}
		}

		this.PreWANSettings();
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		PXML.CheckModule("INET.WAN-2", null, null, "ignore");
		if (this.maccloned)
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
		//PXML.doc.dbgdump();
		
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="") echo "30";
		else echo $bt+10; ?>,
	wantype: null,
	inf1p: null,
	inf2p: null,
	inet1p: null,
	inet2p: null,
	phyinfp: null,
	maccloned: false,
	banner: null,
	stages: new Array("wan_status", "wan_config", "wan_config_pppoe", "wan_status"),
	currentStage: 0,
	InitWANSettings: function()
	{
		OBJ("st_ppp").style.display = "none";
		OBJ("st_static").style.display = "none";
		OBJ("st_dhcp").style.display = "none";
		switch (XG(this.inet1p+"/addrtype"))
		{
		case "ipv4":
			if (XG(this.inet1p+"/ipv4/static")=="1")
			{
				this.wantype = "static";
				this.stages[2] = "wan_config_static";
				this.banner = "<?echo I18N("j", "Enter IP address");?>";
				OBJ("st_static").style.display = "block";
				OBJ("en_static").checked = true;
				var ipaddr	= XG(this.inet1p+"/ipv4/ipaddr");
				var mask	= COMM_IPv4INT2MASK(XG(this.inet1p+"/ipv4/mask"));
				var gw		= XG(this.inet1p+"/ipv4/gateway");
				var dns		= XG(this.inet1p+"/ipv4/dns/entry:1");
				/* init static ip address fields */
				TEMP_SetFieldsByDelimit("static_ipaddr",ipaddr, '.');
				TEMP_SetFieldsByDelimit("static_mask",	mask, '.');
				TEMP_SetFieldsByDelimit("static_gw",	gw, '.');
				TEMP_SetFieldsByDelimit("static_dns",	dns, '.');
				/* static ip address status */
				OBJ("st_static_ipaddr").innerHTML	= ipaddr;
				OBJ("st_static_mask").innerHTML		= mask;
				OBJ("st_static_gw").innerHTML		= gw;
				OBJ("st_static_dns").innerHTML		= dns;
			}
			else
			{
				this.wantype = "dhcp";
				this.stages[2] = "wan_config_dhcp";
				this.banner = "<?echo I18N("j", "ISP assigns IP automatically")." (".I18N("j","DHCP").")";?>";
				OBJ("st_dhcp").style.display = "block";
				OBJ("en_dhcp").checked = true;
				var mac = XG(this.phyinfp+"/macaddr");
				var rmac = "<?echo toupper(PHYINF_getruntimephymac('WAN-1'));?>";

				TEMP_SetFieldsByDelimit("macclone", mac, ':');
				OBJ("st_dhcp_mac").innerHTML = (mac=="")? rmac:mac;
			}
			break;
		case "ppp4":
			if (XG(this.inf2p+"/active")=="1" && XG(this.inf2p+"/nat")=="NAT-1")
				prefix = "r_";
			else
				prefix = "";
			if (XG(this.inet1p+"/ppp4/over")=="eth")
			{
				this.wantype = prefix + "pppoe";
				this.stages[2] = "wan_config_static";
				OBJ("st_ppp").style.display = "block";
				if (prefix=="")
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "PPPoE");?>)";
				else
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "Russia PPPoE");?>)";
				OBJ("en_pppoe").checked = true;
				if (XG(this.inet1p+"/ppp4/static")=="1")
				{
					OBJ("pppoe_fixedip").checked = true;
					TEMP_SetFieldsByDelimit("pppoe_ipaddr", XG(this.inet1p+"/ppp4/ipaddr"), '.');
					TEMP_SetFieldsByDelimit("pppoe_dns", XG(this.inet1p+"/ppp4/dns/entry"), '.');
				}
			}
			else if (XG(this.inet1p+"/ppp4/over")=="pptp")
			{
				this.wantype = prefix + "pptp";
				if (prefix=="")
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "PPTP");?>)";
				else
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "Russia PPTP");?>)";
				OBJ("others").checked = true;
			}
			else if (XG(this.inet1p+"/ppp4/over")=="l2tp")
			{
				this.wantype = prefix + "l2tp";
				if (prefix=="")
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "L2TP");?>)";
				else
					OBJ("st_ppp_type").innerHTML = "(<?echo I18N("j", "Russia L2TP");?>)";
				OBJ("others").checked = true;
			}
			var user = XG(this.inet1p+"/ppp4/username");
			var pwd = XG(this.inet1p+"/ppp4/password");
			/* init ppp fields */
			this.banner = "<?echo I18N("j", "Enter account details");?>";
			OBJ("st_ppp_user").innerHTML= user;
			OBJ("st_ppp_pwd").innerHTML	= pwd;
			/* ppp status */
			OBJ("ppp_user").value	= user;
			OBJ("ppp_pwd").value	= pwd;
			break;
		default:
		}
		this.OnClickFixedIP();
	},
	PreWANSettings: function()
	{
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
		if (OBJ("en_dhcp").checked)
		{
			XS(this.inet1p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ipv4/static", 0);
			XS(this.inet1p+"/ipv4/mtu", 1500);
			XD(this.inet1p+"/ipv4/dns");
			XS(this.inet1p+"/ipv4/dns/count", 0);
			
			if (this.maccloned)
			{
				var macaddr = TEMP_GetFieldsValue("macclone" ,':');
				if(macaddr != "")
					XS(this.phyinfp+"/macaddr", macaddr);
				else
					XS(this.phyinfp+"/macaddr", "");	
			}
		}
		else if (OBJ("en_static").checked)
		{
			XS(this.inet1p+"/addrtype", "ipv4");
			XS(this.inet1p+"/ipv4/static", 1);
			XS(this.inet1p+"/ipv4/ipaddr", TEMP_GetFieldsValue("static_ipaddr", '.'));
			XS(this.inet1p+"/ipv4/mask", COMM_IPv4MASK2INT(TEMP_GetFieldsValue("static_mask", '.')));
			XS(this.inet1p+"/ipv4/gateway", TEMP_GetFieldsValue("static_gw", '.'));
			XS(this.inet1p+"/ipv4/mtu", 1500);
			XD(this.inet1p+"/ipv4/dns");
			XS(this.inet1p+"/ipv4/dns/count", 1);
			XS(this.inet1p+"/ipv4/dns/entry", TEMP_GetFieldsValue("static_dns", '.'));
		}
		else if (OBJ("en_pppoe").checked)
		{
			XS(this.inet1p+"/addrtype", "ppp4");
			XS(this.inet1p+"/ppp4/over", "eth");
			XS(this.inet1p+"/ppp4/static", 0);
			XS(this.inet1p+"/ppp4/mtu", 1492);
			XD(this.inet1p+"/ppp4/dns");
			XS(this.inet1p+"/ppp4/dns/count", 0);
			XS(this.inet1p+"/ppp4/username", OBJ("ppp_user").value);
			XS(this.inet1p+"/ppp4/password", OBJ("ppp_pwd").value);
			XS(this.inet1p+"/ppp4/mppe/enable", 0);
			XS(this.inet1p+"/ppp4/dialup/mode", "auto");
			if (OBJ("pppoe_fixedip").checked)
			{
				XS(this.inet1p+"/ppp4/static", 1);
				XS(this.inet1p+"/ppp4/ipaddr", TEMP_GetFieldsValue("pppoe_ipaddr", '.'));
				XS(this.inet1p+"/ppp4/dns/count", 1);
				XS(this.inet1p+"/ppp4/dns/entry", TEMP_GetFieldsValue("pppoe_dns", '.'));
			}
		}
	},
	ShowCurrentStage: function()
	{
		OBJ("btname").innerHTML = "<?echo I18N("j","Next");?>";
		OBJ("b_exit").style.display = "none";
		OBJ("b_back").style.display = "none";
		OBJ("b_next").style.display = "none";
		OBJ("b_send").style.display = "none";
		if (this.currentStage==0)
		{
			OBJ("banner").innerHTML = this.banner;
			OBJ("st_banner").style.display = "inline";
			OBJ("b_exit").style.display = "inline";
			OBJ("b_next").style.display = "inline";
		}
		else if (this.currentStage==1)
		{
			OBJ("banner").innerHTML = "<?echo I18N("j","Connect your router to Internet");?>";
			OBJ("b_back").style.display = "inline";
			OBJ("b_next").style.display = "inline";
			OBJ("btname").innerHTML = "<?echo I18N("j","Change Settings");?>";
		}
		else if (this.currentStage==2)
		{
			if (OBJ("en_pppoe").checked)
			{
				this.stages[2] = "wan_config_pppoe";
				OBJ("banner").innerHTML = "<?echo I18N("j", "Enter account details");?>";
			}
			else if (OBJ("en_static").checked)
			{
				this.stages[2] = "wan_config_static";
				OBJ("banner").innerHTML = "<?echo I18N("j", "Enter IP address");?>";
			}
			else if (OBJ("en_dhcp").checked)
			{
				this.stages[2] = "wan_config_dhcp";
				OBJ("banner").innerHTML = "<?echo I18N("j", "ISP assigns IP automatically")." (".I18N("j","DHCP").")";?>";
				
				var mac = XG(this.phyinfp+"/macaddr");
				var rmac = "<?echo toupper(PHYINF_getruntimephymac('WAN-1'));?>";

				TEMP_SetFieldsByDelimit("macclone", mac, ':');
				OBJ("st_dhcp_mac").innerHTML = (mac=="")? rmac:mac;		
			}
			else if (OBJ("others").checked)
			{
				self.location.href = "./bsc_wan.php";
				return;
			}
			OBJ("b_back").style.display = "inline";
			OBJ("b_next").style.display = "inline";
		}
		else if (this.currentStage==3)
		{
			OBJ("st_banner").style.display = "none";
			OBJ("b_back").style.display = "inline";
			OBJ("b_send").style.display = "inline";
		}
		for (var i=0; i<this.stages.length; i++)
			OBJ(this.stages[i]).style.display = (this.stages[this.currentStage]==this.stages[i])? "block":"none";
	},
	OnClickPre: function()
	{
		this.currentStage--;
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		if (this.currentStage==2&&COMM_IsDirty(false))
			BODY.OnSubmit();
		else
		{
			this.currentStage++;
			this.ShowCurrentStage();
		}
	},
	OnClickFixedIP: function()
	{
		var disabled = !OBJ("pppoe_fixedip").checked;
		this.SetDisabled("pppoe_ipaddr", disabled);
		this.SetDisabled("pppoe_dns", disabled);
	},
	OnClickMacClone: function()
	{
		var mac = "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
		if (mac=="")
		{
			BODY.ShowConfigError("<?echo I18N("j","Error");?>", "<?echo I18N("j","Can't find Your PC's MAC Address, please enter Your MAC manually.");?>");
			return false;
		}
		TEMP_SetFieldsByDelimit("macclone", mac, ':');
	},
	SetDisabled: function(name, bool)
	{
		var objs = document.getElementsByName(name);
		for (var i=0; i< objs.length; i++)
			objs[i].disabled = bool;
	},
	OnChangeConnMode: function()
	{
		/* reload services to clean all XS() set before */
		if (!OBJ("others").checked)
			COMM_GetCFG(false, PAGE.services, function(xml) {PXML.doc = xml;});
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
