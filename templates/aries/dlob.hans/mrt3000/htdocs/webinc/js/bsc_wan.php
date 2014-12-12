<?include "/htdocs/phplib/inet.php";?>
<?include "/htdocs/phplib/inf.php";?>
<?
	$inet = INF_getinfinfo("LAN-1", "inet");
	$ipaddr = INET_getinetinfo($inet, "ipv4/ipaddr");
?><script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "DEVICE.LAYOUT,DEVICE.HOSTNAME,PHYINF.WAN-1,INET.BRIDGE-1,INET.INF,WAN,RUNTIME.INF.WAN-3,INET.LAN-4,RUNTIME.INF.WAN-1,RUNTIME.INF.WAN-4,DHCPC4.WAN,REBOOT,SAMBA",
	OnLoad: null, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
	{
		switch (code)
		{
		case "OK":
			//add for to check ds-lite mode, when it was changed, rg should be reboot.
			if (this.ori_wan_mode != OBJ("wan_ip_mode").value && this.ori_wan_mode=="dslite")
			{
				Service("REBOOT");
				return true;
			}

			if (COMM_Equal(OBJ("brmode").getAttribute("modified"), true))
			{
				if (OBJ("rgmode").checked)
				{
					if (this.bridge_addrtype==="static")
					{
						/* change to bridge mode and use static ip. */
						var url = '<a href="http://'+this.bridge_ipaddr+'" style="color:#0000ff;">http://'+this.bridge_ipaddr+'</a>';
						var msgArray = [
							'<?echo I18N("j","The device is changing to AP mode.");?>',
							'<?echo I18N("j","You can access the device by clicking the link below.");?>',
							url
						];
					}
					else
					{
						/* change to bridge mode and use DHCP. */
						var msgArray = [
							'<?echo I18N("j","The device is changing to AP mode.");?>',
							'<?echo I18N("j","The device will dynamically get an IP address from DHCP server.");?>',
							'<?echo I18N("j","Please check the DHCP server for the IP address of the device.");?>',
							'<?echo I18N("j","Or use UPnP tools to discover the device.");?>'
						];
					}
				}
				else
				{
					/* change to router mode. */
					var msgArray = [
						'<?echo I18N("j","The device is changing to router mode.");?>',
						'<?echo I18N("j","You may need to change the IP address of your computer to access the device.");?>',
						'<?echo I18N("j","You can access the device by clicking the link below.");?>',
						'<a href="http://<?echo $ipaddr;?>" style="color:#0000ff;">http://<?echo $ipaddr;?></a>'
					];
				}
				BODY.ShowCountdown('<?echo I18N("j","Device Mode");?>', msgArray, this.bootuptime, null);
			}
			else if(this.maccloned)
			{
				var msgArray = ['<?echo I18N("j","It'll take a moment, please wait");?>...'];
				BODY.ShowCountdown('<?echo I18N("j","Clone MAC Address");?>...', msgArray, this.bootuptime, "http://<?echo $_SERVER['HTTP_HOST'];?>/bsc_wan.php");
			}
			else
			{
				BODY.OnReload();
			}
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
		var wan = OBJ("wan_ip_mode").value;
		var noprefix = wan.replace("r_", "");
		/* common error control */
		if (wan=="static"||wan=="dhcp"||wan=="dhcpplus")
		{
			if (node.indexOf("dns/entry:1")>0)	{ BODY.ShowConfigError("ipv4_dns1", msg); return; }
			if (node.indexOf("dns/entry:2")>0)	{ BODY.ShowConfigError("ipv4_dns2", msg); return; }
		}
		else
		{
			if (node.indexOf("ipaddr")>0)		{ BODY.ShowConfigError(noprefix+"_ipadr",	msg); return; }
			if (node.indexOf("mask")>0)			{ BODY.ShowConfigError(noprefix+"_mask",	msg); return; }
			if (node.indexOf("gw")>0)			{ BODY.ShowConfigError(noprefix+"_gw",		msg); return; }
			if (node.indexOf("server")>0)		{ BODY.ShowConfigError(noprefix+"_server",	msg); return; }
			if (node.indexOf("dns/entry:1")>0)	{ BODY.ShowConfigError(noprefix+"_dns1",	msg); return; }
			if (node.indexOf("dns/entry:2")>0)	{ BODY.ShowConfigError(noprefix+"_dns2",	msg); return; }
			if (node.indexOf("mtu")>0)			{ BODY.ShowConfigError(noprefix+"_mtu",		msg); return; }
			if (node.indexOf("idletimeout")>0)	{ BODY.ShowConfigError(noprefix+"_timeout",	msg); return; }
		}

		/* unigue error control */
		switch (wan)
		{
		case "static":
			if (node.indexOf("ipaddr")>0)	{ BODY.ShowConfigError("fixed_ipadr",	msg); return; }
			if (node.indexOf("mask")>0)		{ BODY.ShowConfigError("fixed_mask",	msg); return; }
			if (node.indexOf("gateway")>0)	{ BODY.ShowConfigError("fixed_gw",		msg); return; }
			break;
		case "dhcp":
			break;
		case "dhcpplus":
			if (node.indexOf("username")>0) { BODY.ShowConfigError("dhcpplus_user",	msg); return; }
			if (node.indexOf("password")>0) { BODY.ShowConfigError("dhcpplus_pwd",	msg); return; }
			break;
		case "r_pppoe":
		case "pppoe":
			break;
		case "r_pptp":
		case "pptp":
			break;
		case "r_l2tp":
		case "l2tp":
			break;
		default:
		}
		BODY.ShowMessage("Error",msg);
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
		this.defaultCFGXML = xml;
		PXML.doc = xml;
		//xml.dbgdump();

		/* Clean all input fields */
		var inputs = document.getElementsByTagName("input");
		for (var i=0; i<inputs.length; i++)
			inputs[i].value = "";
		/* Set all dynamic DNS enabled */
		OBJ("pppoe_dyn").checked = OBJ("pptp_dyn").checked =
		OBJ("l2tp_dyn").checked = true;;

		/* Process the XML document, fill the correct value to elements in BODY. */
		/* init the WAN-# & br-# obj */
		var base = PXML.FindModule("INET.INF");
		this.wan1.infp  = GPBT(base, "inf", "uid", "WAN-1", false);
		this.wan1.inetp = GPBT(base+"/inet", "entry", "uid", XG(this.wan1.infp+"/inet"), false);
		var b = PXML.FindModule("PHYINF.WAN-1");
		this.wan1.phyinfp = GPBT(b, "phyinf", "uid", XG(b+"/inf/phyinf"), false);

		this.wan2.infp	= GPBT(base, "inf", "uid", "WAN-2", false);
		this.wan2.inetp	= GPBT(base+"/inet", "entry", "uid", XG(this.wan2.infp+"/inet"), false);
		this.wan3.infp	= GPBT(base, "inf", "uid", "WAN-3", false);
		this.wan3.inetp	= GPBT(base+"/inet", "entry", "uid", XG(this.wan3.infp+"/inet"), false);
		this.wan4.infp	= GPBT(base, "inf", "uid", "WAN-4", false);
		this.wan4.inetp	= GPBT(base+"/inet", "entry", "uid", XG(this.wan4.infp+"/inet"), false);

		this.br1.infp	= GPBT(base, "inf", "uid", "BRIDGE-1", false);
		this.br1.inetp	= GPBT(base+"/inet", "entry", "uid", XG(this.br1.infp+"/inet"), false);

		this.lan4.infp	= GPBT(base, "inf", "uid", "LAN-4", false);
		this.lan4.inetp	= GPBT(base+"/inet", "entry", "uid", XG(this.lan4.infp+"/inet"), false);

		if (!base) { alert("InitValue ERROR!"); return false; }

		var layout = PXML.FindModule("DEVICE.LAYOUT");
		if (!layout) { alert("InitLayout ERROR !"); return false; }

		this.device_host = PXML.FindModule("DEVICE.HOSTNAME");
		if (!this.device_host) { alert("Init Device Host ERROR !"); return false; }

		OBJ("brmode").checked = XG(layout+"/device/layout")==="bridge" ? true :false;
		/* init wan type */
		var wan1addrtype = XG(this.wan1.inetp+"/addrtype");
		if (wan1addrtype === "ipv4")
		{
			if (XG(this.wan1.inetp+"/ipv4/static")==="1")
				this.wantype = "static";
			else
			{
				OBJ("dhcpplus_user").value = XG(this.wan1.inetp+"/ipv4/dhcpplus/username");
				OBJ("dhcpplus_pwd").value = XG(this.wan1.inetp+"/ipv4/dhcpplus/password");
				this.wantype = (XG(this.wan1.inetp+"/ipv4/dhcpplus/enable")==="1")? "dhcpplus":"dhcp";
			}

			if (XG(this.wan1.inetp+"/ipv4/ipv4in6/mode")!="")
				this.wantype = XG(this.wan1.inetp+"/ipv4/ipv4in6/mode");
		}
		else if (wan1addrtype === "ppp4")
		{
			var over = XG(this.wan1.inetp+"/ppp4/over");
			if (XG(this.wan2.infp+"/nat")==="NAT-1"&&
				XG(this.wan2.infp+"/active")==="1")
				var prefix = "r_";
			else
				var prefix = "";

			if (over=="eth")		this.wantype = prefix + "pppoe";
			else if (over=="pptp")	this.wantype = prefix + "pptp";
			else if (over=="l2tp")	this.wantype = prefix + "l2tp";
		}
		else if (wan1addrtype === "ppp10")
		{
			if (XG(this.wan1.inetp+"/ppp4/over")==="eth")
				this.wantype = "pppoe";
		}
		COMM_SetSelectValue(OBJ("wan_ip_mode"), this.wantype);

		/* init ip setting */
		if (!this.InitIpv4Value()) return false;
		if (!this.InitPpp4Value()) return false;
		TEMP_SetFieldsByDelimit("maccpy", XG(this.wan1.phyinfp+"/macaddr"), ':');

		if (wan1addrtype==="ppp10"&&
			XG(this.wan1.inetp+"/ppp4/over")==="eth")
		{
			if (XG(this.wan1.inetp+"/ppp4/static")==="1")
				OBJ("pppoe_static").checked = true;
			else
				OBJ("pppoe_dyn").checked = true;
			TEMP_SetFieldsByDelimit("pppoe_ipadr", XG(this.wan1.inetp+"/ppp4/ipaddr"), '.');
			OBJ("pppoe_user").value = XG(this.wan1.inetp+"/ppp6/username");
			OBJ("pppoe_pwd").value = XG(this.wan1.inetp+"/ppp6/password");
			OBJ("pppoe_pwd2").value = XG(this.wan1.inetp+"/ppp6/password");
			OBJ("pppoe_svcname").value = XG(this.wan1.inetp+"/ppp6/pppoe/servicename");
			OBJ("pppoe_timeout").value = XG(this.wan1.inetp+"/ppp6/dialup/idletimeout");
			var dnscnt = XG(this.wan1.inetp+"/ppp4/dns/count");
			if (dnscntt > 0)
				OBJ("pppoe_dns_manual").checked = true;
			else
				OBJ("pppoe_dns_isp").checked = true;
			if (dnscntt > 0) TEMP_SetFieldsByDelimit("pppoe_dns1", XG(this.wan1.inetp+"/ppp4/dns/entry:1"), '.');
			if (dnscntt > 1) TEMP_SetFieldsByDelimit("pppoe_dns2", XG(this.wan1.inetp+"/ppp4/dns/entry:2"), '.');
		}
		this.OnClickBRMode("InitValue");
		/* If Open DNS function is enabled, the DNS server would be fixed. */
		if (XG(this.wan1.infp+"/open_dns/type")!=="") this.DisableDNS();

		//add for to check ds-lite mode, when it was changed, rg should be reboot.
		this.ori_wan_mode = OBJ("wan_ip_mode").value;

		this.dhcpc4 = PXML.FindModule("DHCPC4.WAN");
		OBJ("dhcp_unicast").checked = XG(this.dhcpc4+"/dhcpc4/unicast")=="yes"?true:false;

		this.OnChangeWanIpMode();
		return true;
	},
	PreSubmit: function()
	{
		/* disable all modules */
		PXML.IgnoreModule("DEVICE.LAYOUT");
		PXML.IgnoreModule("DEVICE.HOSTNAME");
		PXML.IgnoreModule("PHYINF.WAN-1");
		PXML.IgnoreModule("INET.BRIDGE-1");
		PXML.IgnoreModule("WAN");
		PXML.IgnoreModule("DHCPC4.WAN");
		PXML.IgnoreModule("RUNTIME.INF.WAN-1");
		PXML.IgnoreModule("RUNTIME.INF.WAN-3");
		PXML.IgnoreModule("RUNTIME.INF.WAN-4");

		/* router/bridge mode setting */
		if (COMM_Equal(OBJ("brmode").getAttribute("modified"), "true"))
		{
			var layout = PXML.FindModule("DEVICE.LAYOUT")+"/device/layout";

			PXML.ActiveModule("DEVICE.LAYOUT");
			PXML.CheckModule("INET.BRIDGE-1", "ignore", "ignore", null);

			if (OBJ("brmode").checked)
			{
				/* router -> bridge mode */
				XS(layout, "bridge");
				/* If WAN-1 uses static IP address, use the IP as the bridge's IP. */
				if (XG(this.wan1.inetp+"/addrtype")==="ipv4" && XG(this.wan1.inetp+"/ipv4/static")==="1")
				{
					XS(this.br1.infp+"/previous/inet", XG(this.br1.infp+"/inet"));
					XS(this.br1.infp+"/inet", XG(this.wan1.infp+"/inet"));
					this.bridge_addrtype = "static";
					this.bridge_ipaddr = XG(this.wan1.inetp+"/ipv4/ipaddr");
				}
				else
					this.bridge_addrtype = "dhcp";
				/* ignore other services */
				return PXML.doc;
			}
			else
			{
				/* bridge -> router */
				XS(layout, "router");

				/* restore the inet of bridge */
				if (XG(this.br1.infp+"/previous/inet")!=="")
				{
					XS(this.br1.infp+"/inet", XG(this.br1.infp+"/previous/inet"));
					XD(this.br1.infp+"/previous/inet");
				}
			}
		}

		/* clear WAN-2 & clone mac */
		XS(this.wan1.infp+"/schedule","");
		XS(this.wan1.infp+"/lowerlayer","");
		XS(this.wan1.infp+"/upperlayer","");
		XS(this.wan2.infp+"/schedule","");
		XS(this.wan2.infp+"/lowerlayer","");
		XS(this.wan2.infp+"/upperlayer","");
		XS(this.wan2.infp+"/active", "0");
		XS(this.wan2.infp+"/nat","");
		XS(this.wan1.inetp+"/ipv4/ipv4in6/mode","");
		XS(this.wan1.infp+"/infprevious", "");
		if (XG(this.wan4.infp+"/infnext").indexOf("WAN")!=-1)	XS(this.wan4.infp+"/infnext", "");
		if (XG(this.wan4.infp+"/infnext:2").indexOf("WAN")!=-1)	XS(this.wan4.infp+"/infnext:2", "");

		if (COMM_Equal(OBJ("dhcp_hostname").getAttribute("modified"), "true"))
		{
			PXML.ActiveModule("DEVICE.HOSTNAME");
			PXML.CheckModule("SAMBA", "ignore", "ignore", null); //SAMBA need hostname
		}	
		else
		{	
			PXML.IgnoreModule("DEVICE.HOSTNAME");
			PXML.IgnoreModule("SAMBA");
		}	
			
		/* disable dns6 relay */
		XS(this.lan4.infp+"/dns6", "");
		XS(this.lan4.infp+"/dnsrelay", "0");

		var mtu_obj = "ipv4_mtu";
		switch(OBJ("wan_ip_mode").value)
		{
		case "static":
			if (!this.PreStatic()) return false;
			break;
		case "dhcp":
		case "dhcpplus":
			if (!this.PreDhcp()) return false;
			break;
		case "r_pppoe":
			if (!this.PreRPppoe()) return false;
		case "pppoe":
			if (!this.PrePppoe()) return false;
			mtu_obj = "pppoe_mtu";
			break;
		case "r_pptp":
			if (!this.PrePptp("russia")) return false;
		case "pptp":
			if (!this.PrePptp()) return false;
			mtu_obj = "pptp_mtu";
			break;
		case "r_l2tp":
			if (!this.PreL2tp("russia")) return false;
		case "l2tp":
			if (!this.PreL2tp("")) return false;
			mtu_obj = "l2tp_mtu";
			break;
		}
		if (!TEMP_IsDigit(OBJ(mtu_obj).value))
		{
			BODY.ShowConfigError(mtu_obj, "<?echo I18N("j","Invalid MTU value");?>");
			return false;
		}

		/* If mac is changed, restart PHYINF.WAN-1 and WAN, else restart WAN. */
		objs = document.getElementsByName("maccpy");
		for (var i=0; i<objs.length; i++)
		{
			if (COMM_Equal(objs[i].getAttribute("modified"), true))
			{
				this.maccloned = true;
				break;
			}
		}
		if (this.maccloned)
		{
			var p = PXML.FindModule("PHYINF.WAN-1");
			var b = GPBT(p, "phyinf", "uid", XG(p+"/inf/phyinf"), false);
			XS(b+"/macaddr", TEMP_GetFieldsValue("maccpy",':'));
			PXML.ActiveModule("PHYINF.WAN-1");
			PXML.CheckModule("WAN", null, "ignore", null);
		}
		else
		{
			PXML.CheckModule("WAN", null, "ignore", null);
			PXML.CheckModule("INET.LAN-4", "ignore", "ignore", null);
			/*If MAC clone is used, the device would reboot.*/
			PXML.IgnoreModule("REBOOT");
		}

		PXML.CheckModule("INET.INF", null, null, "ignore");
		PXML.ActiveModule("DHCPC4.WAN");

		//PXML.doc.dbgdump();
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	wantype: null,
	dhcpc4: null,
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="")$bt=30;
		else		$bt=$bt+10;
		echo $bt; ?>,
	defaultCFGXML: null,
	device_host: null,
	wan1:	{infp: null, inetp:null, phyinfp:null},
	wan2:	{infp: null, inetp:null},
	wan3:	{infp: null, inetp:null},
	wan4:	{infp: null, inetp:null},
	br1:	{infp: null, inetp:null},
	lan4:	{infp: null, inetp:null},
	/* for bridge/router mode changing */
	bridge_addrtype: null,
	bridge_ipaddr: null,
	ori_wan_mode: null,
	maccloned: false,
	InitIpv4Value: function()
	{
		/* static ip */
		TEMP_SetFieldsByDelimit("fixed_ipadr",	XG(this.wan1.inetp+"/ipv4/ipaddr"), '.');
		TEMP_SetFieldsByDelimit("fixed_gw",		XG(this.wan1.inetp+"/ipv4/gateway"), '.');
		if (XG(this.wan1.inetp+"/ipv4/mask")!="")
			TEMP_SetFieldsByDelimit("fixed_mask", COMM_IPv4INT2MASK(XG(this.wan1.inetp+"/ipv4/mask")), '.');
		/* dns server */
		var cnt = XG(this.wan1.inetp+"/ipv4/dns/count");
		if (cnt > 0) OBJ("dhcp_dns_manual").checked = true;
		else 		 OBJ("dhcp_dns_isp").checked = true;
		if (cnt > 0) TEMP_SetFieldsByDelimit("ipv4_dns1", XG(this.wan1.inetp+"/ipv4/dns/entry:1"), '.');
		if (cnt > 1) TEMP_SetFieldsByDelimit("ipv4_dns2", XG(this.wan1.inetp+"/ipv4/dns/entry:2"), '.');
		OBJ("ipv4_mtu").value = XG(this.wan1.inetp+"/ipv4/mtu");
		/* dhcp & dhcp plus */
		OBJ("dhcp_hostname").value	= XG(this.device_host+"/device/hostname");
		OBJ("dhcpplus_user").vlaue	= XG(this.ipv4+"/dhcpplus/username");
		OBJ("dhcpplus_pwd").vlaue	= XG(this.ipv4+"/dhcpplus/password");
		OBJ("dhcpplus_pwd2").vlaue	= XG(this.ipv4+"/dhcpplus/password");

		return true;
	},
	InitPpp4Value: function()
	{
		var over = XG(this.wan1.inetp+"/ppp4/over");
		if (over=="eth")
		{
			if (XG(this.wan1.inetp+"/ppp4/static")==="1")
				OBJ("pppoe_static").checked = true;
			TEMP_SetFieldsByDelimit("pppoe_ipadr", XG(this.wan1.inetp+"/ppp4/ipaddr"), '.');
			OBJ("pppoe_user").value		= XG(this.wan1.inetp+"/ppp4/username");
			OBJ("pppoe_mppe").checked	= XG(this.wan1.inetp+"/ppp4/mppe/enable")==="1"? true:false;
			OBJ("pppoe_pwd").value		= XG(this.wan1.inetp+"/ppp4/password");
			OBJ("pppoe_pwd2").value		= XG(this.wan1.inetp+"/ppp4/password");
			OBJ("pppoe_svcname").value	= XG(this.wan1.inetp+"/ppp4/pppoe/servicename");
			OBJ("pppoe_mtu").value		= XG(this.wan1.inetp+"/ppp4/mtu");
			var dialup = XG(this.wan1.inetp+"/ppp4/dialup/mode");
			//if (dialup === "auto")			OBJ("pppoe_always").checked = true;
			if (dialup === "ondemand")			OBJ("pppoe_ondemand").checked = true;
			else if (dialup === "manual")	OBJ("pppoe_manual").checked = true;
			else							OBJ("pppoe_always").checked = true;
			OBJ("pppoe_timeout").value = XG(this.wan1.inetp+"/ppp4/dialup/idletimeout");
			var dnscount = XG(this.wan1.inetp+"/ppp4/dns/count");
			if (dnscount > 0)
				OBJ("pppoe_dns_manual").checked = true;
			else
				OBJ("pppoe_dns_isp").checked = true;
			if (dnscount > 0) TEMP_SetFieldsByDelimit("pppoe_dns1", XG(this.wan1.inetp+"/ppp4/dns/entry:1"), '.');
			if (dnscount > 1) TEMP_SetFieldsByDelimit("pppoe_dns2", XG(this.wan1.inetp+"/ppp4/dns/entry:2"), '.');
			//COMM_SetSelectValue(OBJ("schedule"), XG(this.wan1.infp+"/schedule"));
			OBJ("en_fakeos").checked = XG(this.wan1.inetp+"/ppp4/pppoe/fakeos/enable")==="1"? true:false;
		}
		else if (over=="pptp")
		{
			if (XG(this.wan2.inetp+"/ipv4/static")==="1")
				OBJ("pptp_static").checked = true;
			TEMP_SetFieldsByDelimit("pptp_ipadr",	XG(this.wan2.inetp+"/ipv4/ipaddr"), '.');
			TEMP_SetFieldsByDelimit("pptp_gw",		XG(this.wan2.inetp+"/ipv4/gateway"), '.');
			TEMP_SetFieldsByDelimit("pptp_server",	XG(this.wan1.inetp+"/ppp4/pptp/server"), '.');
			if (XG(this.wan2.inetp+"/ipv4/mask")!="")
				TEMP_SetFieldsByDelimit("pptp_mask", COMM_IPv4INT2MASK(XG(this.wan2.inetp+"/ipv4/mask")), '.');
			OBJ("pptp_user").value	= XG(this.wan1.inetp+"/ppp4/username");
			OBJ("pptp_mppe").checked= XG(this.wan1.inetp+"/ppp4/mppe/enable")==="1" ? true : false;
			OBJ("pptp_pwd").value	= XG(this.wan1.inetp+"/ppp4/password");
			OBJ("pptp_pwd2").value  = XG(this.wan1.inetp+"/ppp4/password");
			OBJ("pptp_mtu").value	= XG(this.wan1.inetp+"/ppp4/mtu");
			var dialup = XG(this.wan1.inetp+"/ppp4/dialup/mode");
			if (dialup === "auto")			OBJ("pptp_always").checked= true;
			else if (dialup === "manual")	OBJ("pptp_manual").checked = true;
			else							OBJ("pptp_ondemand").checked = true;
			OBJ("pptp_timeout").value = XG(this.wan1.inetp+"/ppp4/dialup/idletimeout");
			var dnscount = XG(this.wan1.inetp+"/ppp4/dns/count");
			if (dnscount > 0) TEMP_SetFieldsByDelimit("pptp_dns1", XG(this.wan1.inetp+"/ppp4/dns/entry:1"), '.');
			if (dnscount > 1) TEMP_SetFieldsByDelimit("pptp_dns2", XG(this.wan1.inetp+"/ppp4/dns/entry:2"), '.');
			//COMM_SetSelectValue(OBJ("pptp_sch"), XG(this.wan1.infp+"/schedule"));
		}
		else if (over=="l2tp")
		{
			if (XG(this.wan2.inetp+"/ipv4/static")==="1")
				OBJ("l2tp_static").checked = true;
			TEMP_SetFieldsByDelimit("l2tp_ipadr",	XG(this.wan2.inetp+"/ipv4/ipaddr"), '.');
			TEMP_SetFieldsByDelimit("l2tp_gw",		XG(this.wan2.inetp+"/ipv4/gateway"), '.');
			TEMP_SetFieldsByDelimit("l2tp_server",	XG(this.wan1.inetp+"/ppp4/l2tp/server"), '.');
			if (XG(this.wan2.inetp+"/ipv4/mask")!="")
				TEMP_SetFieldsByDelimit("l2tp_mask", COMM_IPv4INT2MASK(XG(this.wan2.inetp+"/ipv4/mask")), '.');
			OBJ("l2tp_user").value	= XG(this.wan1.inetp+"/ppp4/username");
			OBJ("l2tp_pwd").value	= XG(this.wan1.inetp+"/ppp4/password");
			OBJ("l2tp_pwd2").value  = XG(this.wan1.inetp+"/ppp4/password");
			OBJ("l2tp_mtu").value	= XG(this.wan1.inetp+"/ppp4/mtu");
			var dialup = XG(this.wan1.inetp+"/ppp4/dialup/mode");
			if (dialup === "auto")			OBJ("l2tp_always").checked= true;
			else if (dialup === "manual")	OBJ("l2tp_manual").checked = true;
			else							OBJ("l2tp_ondemand").checked = true;
			OBJ("l2tp_timeout").value = XG(this.wan1.inetp+"/ppp4/dialup/idletimeout");
			var dnscount = XG(this.wan1.inetp+"/ppp4/dns/count");
			if (dnscount > 0) TEMP_SetFieldsByDelimit("l2tp_dns1", XG(this.wan1.inetp+"/ppp4/dns/entry:1"), '.');
			if (dnscount > 1) TEMP_SetFieldsByDelimit("l2tp_dns2", XG(this.wan1.inetp+"/ppp4/dns/entry:2"), '.');
		}
		/* common */
		//COMM_SetSelectValue(OBJ("schlist"), XG(this.wan1.infp+"/schedule"));
		return true;
	},
	SetDisabled: function(name, bool)
	{
		var objs = document.getElementsByName(name);
		if (objs==null) return false;

		for (var i=0; i< objs.length; i++)
			objs[i].disabled = bool;
	},
	SetStyleDisplay: function(name, value)
	{
		var objs = document.getElementsByName(name);
		for (var i=0; i< objs.length; i++)
			objs[i].style.display = value;
	},
	ShowFields: function()
	{
		var selected = OBJ("wan_ip_mode").value;
		var areas = ["static","dhcpplus","dhcp","ipv4_comm","pppoe","pptp","l2tp","ppp_comm","dslite"];
		var ppptitle = ["title_pppoe","title_pptp","title_l2tp","title_r_pppoe","title_r_pptp","title_r_l2tp"];
		var isipv4 = false;
		var dhcpplus = false;

		if (selected.indexOf("r_")==0)
			var name = selected.replace("r_", "");
		else
			var name = selected;

		/* control special input fields in this switch */
		switch(name)
		{
		case "dhcpplus":
			dhcpplus = true;
		case "static":
		case "dhcp":
			//OBJ("schedule").style.display = "none";
			isipv4 = true;
			break;
		case "pppoe":
		case "pptp":
		case "l2tp":
			for (var i=0; i<ppptitle.length; i++)
				/* show title of PPP connection */
				OBJ(ppptitle[i]).style.display = (ppptitle[i]==("title_"+selected))? "":"none";

			//OBJ("schedule").style.display = "";
			/* russia pppoe/pptp features */
			var en = (selected=="r_pppoe")? "":"none";
			this.SetStyleDisplay("r_pppoe", en);
			var en = (name!==selected)? "":"none";
			OBJ("r_pppoe_mppe").style.display = en;
			OBJ("r_pptp_mppe").style.display = en;
			break;
/*			
		case "dslite":
			OBJ("schedule").style.display = "none";
			break;
*/			
		default:
		}

		/* control common input fields there */
		for (var i=0; i<areas.length; i++)
		{
			if (areas[i]==name)
				this.SetStyleDisplay(name, "");
			else if (areas[i]=="dhcp"&&dhcpplus)	/* DHCP+ fields */
				this.SetStyleDisplay("dhcp", "");
			else if (areas[i]=="ipv4_comm"&&isipv4)	/* IPv4 common fields */
				this.SetStyleDisplay("ipv4_comm", "");
			else
				this.SetStyleDisplay(areas[i], "none");
		}
	},
	/* for Pre-Submit */
	PreStatic: function()
	{
		var cnt = 0;
		XS(this.wan1.inetp+"/addrtype",		"ipv4");
		XS(this.wan1.inetp+"/ipv4/static",	"1");
		XS(this.wan1.inetp+"/ipv4/ipaddr",	TEMP_GetFieldsValue("fixed_ipadr",'.'));
		XS(this.wan1.inetp+"/ipv4/mask",	COMM_IPv4MASK2INT(TEMP_GetFieldsValue("fixed_mask",'.')));
		XS(this.wan1.inetp+"/ipv4/gateway",	TEMP_GetFieldsValue("fixed_gw",'.'));
		XS(this.wan1.inetp+"/ipv4/mtu",		OBJ("ipv4_mtu").value);

		if (!CheckIP("fixed_ipadr",	"<?echo I18N("j","Invalid IP address");?>"))
			return false;
		for (var i=1; i<3; i++)
		{
			if (TEMP_CheckFieldsEmpty("ipv4_dns1"))
			{
				BODY.ShowConfigError("ipv4_dns1", "<?echo I18N("j","Invalid Primary DNS address.");?>");
				return false;
			}
			else
			{
				if (!TEMP_CheckFieldsEmpty("ipv4_dns"+i))
				{
					if(i==1)
					{
						if (CheckIP("ipv4_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>")==false)
						return false;
					}
					else
					{
						if (CheckIP("ipv4_dns"+i, "<?echo I18N("j","Invalid Secondary DNS address.");?>")==false)
						return false;
					}
					cnt++;
					XS(this.wan1.inetp+"/ipv4/dns/entry:"+cnt, TEMP_GetFieldsValue("ipv4_dns"+i,'.'));
				}
			}
		}
		XS(this.wan1.inetp+"/ipv4/dns/count", cnt);
		return true;
	},
	PreDhcp: function()
	{
		var cnt = 0;
		XS(this.wan1.inetp+"/addrtype", "ipv4");
		XS(this.wan1.inetp+"/ipv4/static", "0");

		if (this.IsNumber(OBJ("dhcp_hostname").value))
		{
			BODY.ShowConfigError("dhcp_hostname","<?echo I18N("j","Host name format error, must contain alphabet.");?>");
			return false;
		}
		if ((OBJ("dhcp_hostname").value).indexOf(".")!=(-1))
		{
			BODY.ShowConfigError("dhcp_hostname","<?echo I18N("j","Host name format error, can't contain dot.");?>");
			OBJ("dhcp_hostname").focus();
			return false;
		}
		XS(this.device_host+"/device/hostname", OBJ("dhcp_hostname").value);

		if (OBJ("dhcp_dns_isp").checked)
		{
			XD(this.wan1.inetp+"/ipv4/dns");
			XS(this.wan1.inetp+"/ipv4/dns/count", "0");
		}
		else
		{
			for (var i=1; i<3; i++)
			{
				//if (TEMP_CheckFieldsEmpty("ipv4_dns"+i))
				if (TEMP_CheckFieldsEmpty("ipv4_dns1"))
				{
					BODY.ShowConfigError("ipv4_dns1", "<?echo I18N("j","Invalid Primary DNS address.");?>");
					return false;
				}
				else
				{
					/*
					cnt++;
					if (!CheckIP("ipv4_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>"))
						return false;
					XS(this.wan1.inetp+"/ipv4/dns/entry:"+cnt, TEMP_GetFieldsValue("ipv4_dns"+i,'.'));
					*/
					if (!TEMP_CheckFieldsEmpty("ipv4_dns"+i))
					{
						if(i==1)
						{
							if (CheckIP("ipv4_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>")==false)
							return false;
						}
						else
						{
							if (CheckIP("ipv4_dns"+i, "<?echo I18N("j","Invalid Secondary DNS address.");?>")==false)
							return false;
						}
						cnt++;
						XS(this.wan1.inetp+"/ipv4/dns/entry:"+cnt, TEMP_GetFieldsValue("ipv4_dns"+i,'.'));
					}
				}
			}
			XS(this.wan1.inetp+"/ipv4/dns/count", cnt);
		}
		XS(this.wan1.inetp+"/ipv4/mtu", OBJ("ipv4_mtu").value);
		if (OBJ("wan_ip_mode").value === "dhcpplus")
		{
			if (OBJ("dhcpplus_pwd").value!=OBJ("dhcpplus_pwd2").value)
			{
				BODY.ShowConfigError("dhcpplus_pwd","<?echo I18N("j","The password is mismatched.");?>");
				return false;
			}
			XS(this.wan1.inetp+"/ipv4/dhcpplus/enable", "1");
			XS(this.wan1.inetp+"/ipv4/dhcpplus/username", OBJ("dhcpplus_user").value);
			XS(this.wan1.inetp+"/ipv4/dhcpplus/password", OBJ("dhcpplus_pwd").value);
		}
		else
		{
			XS(this.wan1.inetp+"/ipv4/dhcpplus/enable", "0");
		}
		//sam_pan add
		XS(this.dhcpc4+"/dhcpc4/unicast", OBJ("dhcp_unicast").checked?"yes":"no");
		return true;
	},
	PrePppoe: function()
	{
		if (COMM_EatAllSpace(OBJ("pppoe_user").value) == "")
		{
			BODY.ShowConfigError("pppoe_user","<?echo I18N("j","The user name cannot be empty.");?>");
			return false;
		}

		if (OBJ("pppoe_pwd").value !== OBJ("pppoe_pwd2").value)
		{
			BODY.ShowConfigError("pppoe_pwd","<?echo I18N("j","The password is mismatched.");?>");
			return false;
		}
		var wan1addrtype = XG(this.wan1.inetp+"/addrtype");
		var over = XG(this.wan1.inetp+"/ppp4/over");
		//XS(this.wan1.inetp+"/addrtype", "ppp4");
		if (wan1addrtype=="ppp10" && over=="eth")
		{
			XS(this.wan1.inetp+"/addrtype", "ppp10");
			XS(this.wan1.inetp+"/ppp6/username", OBJ("pppoe_user").value);
			XS(this.wan1.inetp+"/ppp6/password", OBJ("pppoe_pwd").value);
			XS(this.wan1.inetp+"/ppp6/pppoe/servicename", OBJ("pppoe_svcname").value);
			XS(this.wan1.inetp+"/ppp6/mtu", OBJ("pppoe_mtu").value);
			XS(this.wan1.inetp+"/ppp6/over", "eth");
		}
		else
			XS(this.wan1.inetp+"/addrtype", "ppp4");

		XS(this.wan1.inetp+"/ppp4/over", "eth");
		XS(this.wan1.inetp+"/ppp4/username", OBJ("pppoe_user").value);
		var mppe = 0;
		if (OBJ("pppoe_mppe").checked&&
			OBJ("wan_ip_mode").value==="r_pppoe")
			mppe = "1";
		XS(this.wan1.inetp+"/ppp4/mppe/enable", mppe);
		XS(this.wan1.inetp+"/ppp4/password", OBJ("pppoe_pwd").value);
		XS(this.wan1.inetp+"/ppp4/pppoe/servicename", OBJ("pppoe_svcname").value);
		if (OBJ("pppoe_dyn").checked)
		{
			XS(this.wan1.inetp+"/ppp4/static", "0");
			XD(this.wan1.inetp+"/ppp4/ipaddr");
		}
		else
		{
			XS(this.wan1.inetp+"/ppp4/static", "1");
			XS(this.wan1.inetp+"/ppp4/ipaddr", TEMP_GetFieldsValue("pppoe_ipadr",'.'));
			var st_ip = TEMP_GetFieldsValue("pppoe_ipadr",'.');
			if (!check_ip_validity(st_ip))
			{
				BODY.ShowConfigError("pppoe_ipadr","<?echo I18N("j","Invalid IP address");?>");
				return false;
			}

			if (OBJ("pppoe_dns_manual").checked)
			{
				if(TEMP_CheckFieldsEmpty("pppoe_dns1"))
				{
					BODY.ShowConfigError("pppoe_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>");
					return false;
				}
				else
				{
					if (!CheckIP("pppoe_dns1", "<?echo I18N("j","Invalid Primary DNS address.");?>"))
						return false;
				}	
			}
		}
		/* star fakeos */
		XS(this.wan1.inetp+"/ppp4/pppoe/fakeos/enable", OBJ("en_fakeos").checked ? "1" : "0");

		/* dns */
		var cnt = 0;
		if (OBJ("pppoe_dns_isp").checked)
		{
			XS(this.wan1.inetp+"/ppp4/dns/entry:1","");
			XS(this.wan1.inetp+"/ppp4/dns/entry:2","");
		}
		else
		{
			for (var i=1; i<3; i++)
			{
				//if (TEMP_CheckFieldsEmpty("pppoe_dns"+i))
				//{
				//	BODY.ShowConfigError("pppoe_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>");
				//	return false;
				//}
				//else					
				//{
				//	cnt++;
				//	if (!CheckIP("pppoe_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>"))
				//		return false;
				//	XS(this.wan1.inetp+"/ppp4/dns/entry:"+cnt, TEMP_GetFieldsValue("pppoe_dns"+i,'.'));
				//}
				if (TEMP_CheckFieldsEmpty("pppoe_dns1"))
				{
					BODY.ShowConfigError("pppoe_dns1", "<?echo I18N("j","Invalid Primary DNS address.");?>");
					return false;
				}
				else
				{
					if (!TEMP_CheckFieldsEmpty("pppoe_dns"+i))
					{
						if(i==1)
						{
							if (CheckIP("pppoe_dns"+i, "<?echo I18N("j","Invalid Primary DNS address.");?>")==false)
							return false;
						}
						else
						{
							if (CheckIP("pppoe_dns"+i, "<?echo I18N("j","Invalid Secondary DNS address.");?>")==false)
							return false;
						}
						cnt++;
						XS(this.wan1.inetp+"/ppp4/dns/entry:"+cnt, TEMP_GetFieldsValue("pppoe_dns"+i,'.'));
					}
				}
			}
		}
		XS(this.wan1.inetp+"/ppp4/dns/count", cnt);
		XS(this.wan1.inetp+"/ppp4/mtu", OBJ("pppoe_mtu").value);
		if (OBJ("pppoe_timeout").value==="") OBJ("pppoe_timeout").value = 0;
		if (!TEMP_IsDigit(OBJ("pppoe_timeout").value))
		{
			BODY.ShowConfigError("pppoe_timeout","<?echo I18N("j","Invalid idle timeout value");?>");
			return null;
		}
		XS(this.wan1.inetp+"/ppp4/dialup/idletimeout", OBJ("pppoe_timeout").value);
		var dialup = "ondemand";
		if (OBJ("pppoe_always").checked)
		{
			dialup = "auto";
			//XS(this.wan1.infp+"/schedule", OBJ("pppoe_sch").value);
		}
		else if (OBJ("pppoe_manual").checked)   dialup = "manual";
		XS(this.wan1.inetp+"/ppp4/dialup/mode", dialup);

		return true;
	},
	PreRPppoe: function()
	{
		var rpppoe_static = OBJ("pppoe_static").checked;
		var cnt = 0;
		XS(this.wan2.infp+"/active", "1");
		XS(this.wan2.infp+"/nat","NAT-1");
		XS(this.wan2.inetp+"/ipv4/static",  (rpppoe_static)? "1":"0");
		XS(this.wan2.inetp+"/ipv4/ipaddr",  (rpppoe_static)? TEMP_GetFieldsValue("pppoe_ipadr",'.'):"");
		XS(this.wan2.inetp+"/ipv4/mask",    (rpppoe_static)? COMM_IPv4MASK2INT(TEMP_GetFieldsValue("pppoe_mask",'.')):"");
		XS(this.wan2.inetp+"/ipv4/gateway", (rpppoe_static)? TEMP_GetFieldsValue("pppoe_gw",'.'):"");
		if (rpppoe_static)
		{
			if (!TEMP_CheckFieldsEmpty("pppoe_dns1"))
			{
				XS(this.wan2.inetp+"/ipv4/dns/entry", TEMP_GetFieldsValue("pppoe_dns1"),'.');
				cnt+=1;
			}
			else
			{
				BODY.ShowConfigError("pppoe_dns1","<?echo I18N("j","Invalid Primary DNS address.");?>");
				return false;
			}
			if (!TEMP_CheckFieldsEmpty("pppoe_dns2"))
			{
				XS(this.wan2.inetp+"/ipv4/dns/entry:2", TEMP_GetFieldsValue("pppoe_dns2",'.'));
				cnt+=1;
			}
		}
		XS(this.wan2.inetp+"/ipv4/dns/count", cnt);

		return true;
	},
	PrePptp: function(type)
	{
		if (OBJ("pptp_pwd").value !== OBJ("pptp_pwd2").value)
		{
			BODY.ShowConfigError("pptp_pwd","<?echo I18N("j","The password is mismatched.");?>");
			return false;
		}
		/* Note : Russia mode need two WANs to be active simultaneously. So we remove the lowerlayer connection.
		   For normal pptp, the lowerlayer/upperlayer connection still remains. */

		if (type == "russia")    //normal pptp
		{
			/* defaultroute value will become metric value.
			   As for Russia, physical WAN (wan2) priority should be lower than
			   ppp WAN (wan1) */
			XS(this.wan1.infp+"/defaultroute", "100");
			XS(this.wan2.infp+"/defaultroute", "200");
		}
		else
		{
			XS(this.wan1.infp+"/defaultroute", "100");
			XS(this.wan2.infp+"/defaultroute", "");

			XS(this.wan1.infp+"/lowerlayer", "WAN-2");
			XS(this.wan2.infp+"/upperlayer", "WAN-1");
		}

		XS(this.wan2.infp+"/active", "1");
		XS(this.wan2.infp+"/nat", (OBJ("wan_ip_mode").value==="r_pptp") ? "NAT-1" : "");
		XS(this.wan1.inetp+"/addrtype", "ppp4");
		XS(this.wan1.inetp+"/ppp4/over", "pptp");
		XS(this.wan1.inetp+"/ppp4/static", "0");
		XS(this.wan1.inetp+"/ppp4/username", OBJ("pptp_user").value);
		XS(this.wan1.inetp+"/ppp4/password", OBJ("pptp_pwd").value);
		XS(this.wan1.inetp+"/ppp4/pptp/server", TEMP_GetFieldsValue("pptp_server",'.'));
		var cnt = 0;
		if (OBJ("pptp_static").checked)
		{
			XS(this.wan2.inetp+"/ipv4/static", "1");
			XS(this.wan2.inetp+"/ipv4/ipaddr", TEMP_GetFieldsValue("pptp_ipadr",'.'));
			XS(this.wan2.inetp+"/ipv4/mask", COMM_IPv4MASK2INT(TEMP_GetFieldsValue("pptp_mask",'.')));
			XS(this.wan2.inetp+"/ipv4/gateway", TEMP_GetFieldsValue("pptp_gw",'.'));
			if (TEMP_CheckFieldsEmpty("pptp_dns1"))
			{
				BODY.ShowConfigError("pptp_dns1","<?echo I18N("j","Invalid Primary DNS address.");?>");
				return false;
			}
			XS(this.wan2.inetp+"/ipv4/dns/entry:1", TEMP_GetFieldsValue("pptp_dns1",'.'));
			XS(this.wan1.inetp+"/ppp4/dns/entry:1", TEMP_GetFieldsValue("pptp_dns1",'.'));
			cnt++;
			if (!TEMP_CheckFieldsEmpty("pptp_dns2"))
			{
				XS(this.wan2.inetp+"/ipv4/dns/entry:2", TEMP_GetFieldsValue("pptp_dns2",'.'));
				XS(this.wan1.inetp+"/ppp4/dns/entry:2", TEMP_GetFieldsValue("pptp_dns2",'.'));
				cnt++;
			}
			XS(this.wan2.inetp+"/ipv4/dns/count", cnt);
			XS(this.wan1.inetp+"/ppp4/dns/count", cnt);
		}
		else
		{
			XS(this.wan2.inetp+"/ipv4/static", "0");
			XD(this.wan2.inetp+"/ipv4/dns");
			XD(this.wan1.inetp+"/ppp4/dns");
			XS(this.wan2.inetp+"/ipv4/dns/count", "0");
			XS(this.wan1.inetp+"/ppp4/dns/count", "0");
		}
		var mppe = "0";
		if (OBJ("pptp_mppe").checked && OBJ("wan_ip_mode").value=="r_pptp")  mppe ="1";
		XS(this.wan1.inetp+"/ppp4/mppe/enable", mppe);
		XS(this.wan1.inetp+"/ppp4/mtu", OBJ("pptp_mtu").value);
		if (OBJ("pptp_timeout").value==="") OBJ("pptp_timeout").value = 0;
		if (!TEMP_IsDigit(OBJ("pptp_timeout").value))
		{
			BODY.ShowConfigError("pptp_timeout","<?echo I18N("j","Invalid idle timeout value");?>");
			return false;
		}
		XS(this.wan1.inetp+"/ppp4/dialup/idletimeout", OBJ("pptp_timeout").value);
		var dialup = "ondemand";
		if (OBJ("pptp_always").checked)
		{
			dialup = "auto";
			//XS(this.wan1.infp+"/schedule", OBJ("pptp_sch").value);
		}
		else if (OBJ("pptp_manual").checked)    dialup = "manual";
		XS(this.wan1.inetp+"/ppp4/dialup/mode", dialup);
		return true;
	},
	PreL2tp: function(type)
	{
		var cnt;
		if (OBJ("l2tp_pwd").value !== OBJ("l2tp_pwd2").value)
		{
			BODY.ShowConfigError("l2tp_pwd","<?echo I18N("j","The password is mismatched.");?>");
			return false;
		}

		/* Note : Russia mode need two WANs to be active simultaneously. So we remove the lowerlayer connection.
		   For normal l2tp, the lowerlayer/upperlayer connection still remains. */

		if (type == "russia")    //normal l2tp
		{
			/* defaultroute value will become metric value.
			   As for Russia, physical WAN (wan2) priority should be lower than
			   ppp WAN (wan1) */
			XS(this.wan1.infp+"/defaultroute", "100");
			XS(this.wan2.infp+"/defaultroute", "200");
		}
		else
		{
			XS(this.wan1.infp+"/defaultroute", "100");
			XS(this.wan2.infp+"/defaultroute", "");

			XS(this.wan1.infp+"/lowerlayer", "WAN-2");
			XS(this.wan2.infp+"/upperlayer", "WAN-1");
		}

		XS(this.wan2.infp+"/active", "1");
		XS(this.wan2.infp+"/nat", (OBJ("wan_ip_mode").value==="r_l2tp") ? "NAT-1" : "");
		XS(this.wan1.inetp+"/addrtype", "ppp4");
		XS(this.wan1.inetp+"/ppp4/over", "l2tp");
		XS(this.wan1.inetp+"/ppp4/static", "0");
		XS(this.wan1.inetp+"/ppp4/username", OBJ("l2tp_user").value);
		XS(this.wan1.inetp+"/ppp4/password", OBJ("l2tp_pwd").value);
		XS(this.wan1.inetp+"/ppp4/l2tp/server", TEMP_GetFieldsValue("l2tp_server",'.'));
		cnt = 0;
		if (OBJ("l2tp_static").checked)
		{
			XS(this.wan2.inetp+"/ipv4/static", "1");
			XS(this.wan2.inetp+"/ipv4/ipaddr", TEMP_GetFieldsValue("l2tp_ipadr",'.'));
			XS(this.wan2.inetp+"/ipv4/mask", COMM_IPv4MASK2INT(TEMP_GetFieldsValue("l2tp_mask",'.')));
			XS(this.wan2.inetp+"/ipv4/gateway", TEMP_GetFieldsValue("l2tp_gw",'.'));
			if (TEMP_CheckFieldsEmpty("l2tp_dns1",'.'))
			{
				BODY.ShowConfigError("l2tp_dns1","<?echo I18N("j","Invalid Primary DNS address.");?>");
				return false;
			}
			XS(this.wan2.inetp+"/ipv4/dns/entry:1", TEMP_GetFieldsValue("l2tp_dns1",'.'));
			XS(this.wan1.inetp+"/ppp4/dns/entry:1", TEMP_GetFieldsValue("l2tp_dns1",'.'));
			cnt++;
			if (!TEMP_CheckFieldsEmpty("l2tp_dns2"))
			{
				XS(this.wan2.inetp+"/ipv4/dns/entry:2", TEMP_GetFieldsValue("l2tp_dns2",'.'));
				XS(this.wan1.inetp+"/ppp4/dns/entry:2", TEMP_GetFieldsValue("l2tp_dns2",'.'));
				cnt++;
			}
			XS(this.wan2.inetp+"/ipv4/dns/count", cnt);
			XS(this.wan1.inetp+"/ppp4/dns/count", cnt);
		}
		else
		{
			XS(this.wan2.inetp+"/ipv4/static", "0");
			if (!TEMP_CheckFieldsEmpty("l2tp_dns1"))   { XS(this.wan1.inetp+"/ppp4/dns/entry:1", TEMP_GetFieldsValue("l2tp_dns1",'.')); cnt++; }
			else XS(this.wan1.inetp+"/ppp4/dns/entry:1", "");
			if (!TEMP_CheckFieldsEmpty("l2tp_dns2"))   { XS(this.wan1.inetp+"/ppp4/dns/entry:2", TEMP_GetFieldsValue("l2tp_dns2",'.')); cnt++; }
			XS(this.wan1.inetp+"/ppp4/dns/count", cnt);

			XD(this.wan2.inetp+"/ipv4/dns");
			XS(this.wan2.inetp+"/ipv4/dns/count", "0");
		}

		if (OBJ("l2tp_timeout").value === "") OBJ("l2tp_timeout").value = 0;
		if (!TEMP_IsDigit(OBJ("l2tp_timeout").value))
		{
			BODY.ShowConfigError("l2tp_timeout","<?echo I18N("j","Invalid idle timeout value");?>");
			return false;
		}
		XS(this.wan1.inetp+"/ppp4/mtu", OBJ("l2tp_mtu").value);
		XS(this.wan1.inetp+"/ppp4/dialup/idletimeout", OBJ("l2tp_timeout").value);
		var dialup = "ondemand";
		if (OBJ("l2tp_always").checked)
		{
			dialup = "auto";
			//XS(this.wan1.infp+"/schedule", OBJ("l2tp_sch").value);
		}
		else if (OBJ("l2tp_manual").checked)    dialup = "manual";
		XS(this.wan1.inetp+"/ppp4/dialup/mode", dialup);
		return true;
	},
	OnChangeWanIpMode: function()
	{
		//for dslite
		var wanmode = OBJ("wan_ip_mode").value;
		RemoveItemFromSelect(OBJ("wan_ip_mode"),"dslite");
		var wanmode6 = XG(this.wan4.inetp+"/ipv6/mode");
		var wanactive6 = XG(this.wan4.infp+"/active");
		var addrtype_wan1 = XG(this.wan1.inetp+"/addrtype");
		var addrtype_wan3 = XG(this.wan3.inetp+"/addrtype");
		OBJ("wan_ip_mode").value = wanmode;

		var mtu = XG(this.wan1.inetp+"/ppp4/mtu");
		var russia = false;
		switch(OBJ("wan_ip_mode").value)
		{
		case "static":
		case "dhcp":
		case "dhcpplus":
			this.OnClickDnsMode();
			break;
		case "r_pppoe":
			russia = true;
			if (mtu==""||(this.wantype!="r_pppoe" && S2I(mtu)>1492))
				OBJ("pppoe_mtu").value = "1492";
		case "pppoe":
			if (russia==false&&(mtu==""||(this.wantype!="ppoe" && S2I(mtu)>1400)))
				OBJ("pppoe_mtu").value = "1492";
			this.OnClickAddrType("pppoe");
			this.OnClickReconn("pppoe");
			this.OnClickDnsMode();
			break;
		case "r_pptp":
			russia = true;
			if (mtu==""||(this.wantype!="r_pptp" && S2I(mtu)>1400))
				OBJ("pptp_mtu").value = "1400";
		case "pptp":
			if (russia==false&&(mtu==""||(this.wantype!="pptp" && S2I(mtu)>1400)))
				OBJ("pptp_mtu").value = "1400";
			this.OnClickAddrType("pptp");
			this.OnClickReconn("pptp");
			break;
		case "r_l2tp":
			russia = true;
			if (mtu==""||(this.wantype!="r_l2tp" && S2I(mtu)>1400))
				OBJ("l2tp_mtu").value = "1400";
		case "l2tp":
			if (russia==false&&(mtu==""||(this.wantype!="l2tp" && S2I(mtu)>1400)))
				OBJ("l2tp_mtu").value = "1400";
			this.OnClickAddrType("l2tp");
			this.OnClickReconn("l2tp");
			break;
		}
		this.ShowFields();
	},
	OnClickReconn: function(type)
	{
		OBJ(type+"_timeout").disabled = OBJ(type+"_ondemand").checked? false:true;
	},
	OnClickDnsMode: function()
	{
		/* DHCP */
		var isdyn = OBJ("dhcp_dns_isp").checked;
		if (OBJ("wan_ip_mode").value=="static") isdyn = false;
		this.SetDisabled("ipv4_dns1", isdyn);
		this.SetDisabled("ipv4_dns2", isdyn);
		/* PPPoE */
		isdyn = OBJ("pppoe_dns_isp").checked;
		this.SetDisabled("pppoe_dns1", isdyn);
		this.SetDisabled("pppoe_dns2", isdyn);
	},
	OnClickAddrType: function(type)
	{
		var isdyn = OBJ(type+"_dyn").checked;
		if (type!="dslite")
		{
			this.SetDisabled(type+"_ipadr",	isdyn);
			this.SetDisabled(type+"_mask",	isdyn);
			this.SetDisabled(type+"_gw",	isdyn);
			if (type=="pptp"||type=="l2tp")
			{
				this.SetDisabled(type+"_dns1", isdyn);
				this.SetDisabled(type+"_dns2", isdyn);
			}
			/* PPPoE with OPEN DNS */
			if (type.indexOf("pppoe")!=-1&&
				XG(this.wan1.infp+"/open_dns/type")==="")
			{
				this.SetDisabled(type+"_dns1", isdyn);
				this.SetDisabled(type+"_dns2", isdyn);
			}
		}
	},
	OnClickMacButton: function()
	{
		value = "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
		if (value=="")
		{
			BODY.ShowConfigError("maccpy", "<?echo I18N("j","Can't find Your PC's MAC Address, please enter Your MAC manually.");?>");
			return false;
		}
		TEMP_SetFieldsByDelimit("maccpy", value, ':');
	},
	OnClickBRMode: function(from)
	{
		if (OBJ("brmode").checked)
		{
			/* reinit the all setting. */
			if (from === "checkbox")    this.InitValue(this.defaultCFGXML);
			OBJ("brmode").checked = true;
			this.OnChangeWanIpMode();
			BODY.DisableCfgElements(true);
			if (AUTH.AuthorizedGroup < 100)
			{
				OBJ("brmode").disabled = false;
				OBJ("topsave").disabled = false;
				OBJ("topcancel").disabled = false;
			}
		}
		else
		{
			BODY.DisableCfgElements(false);
			this.OnChangeWanIpMode();
		}
	},
	DisableDNS: function()
	{
		if (XG(this.wan1.infp+"/open_dns/type")==="advance")
			var open_dns_srv = "adv_dns_srv";
		else if (XG(this.wan1.infp+"/open_dns/type")==="family")
			var open_dns_srv = "family_dns_srv";
		else
			var open_dns_srv = "parent_dns_srv";
		var opendns_dns1 = XG(this.wan1.infp+"/open_dns/"+open_dns_srv+"/dns1");
		var opendns_dns2 = XG(this.wan1.infp+"/open_dns/"+open_dns_srv+"/dns2");
		var type = ["ipv4", "pppoe", "pptp", "l2tp"];

		var list = ["ipv4_dns1","ipv4_dns2","dhcp_dns_isp","dhcp_dns_manual",
					"pppoe_dns1","pppoe_dns2","pppoe_dns_isp","pppoe_dns_manual",
					"pptp_dns1","pptp_dns2","l2tp_dns1","l2tp_dns2"
		];
		for (var i=0; i<list.length; i++)
			this.SetDisabled(list[i], true);

		TEMP_SetFieldsByDelimit("ipv4_dns1",opendns_dns1,'.');
		TEMP_SetFieldsByDelimit("ipv4_dns2",opendns_dns2,'.');
		TEMP_SetFieldsByDelimit("pppoe_dns1",opendns_dns1,'.');
		TEMP_SetFieldsByDelimit("pppoe_dns2",opendns_dns2,'.');
		TEMP_SetFieldsByDelimit("pptp_dns1",opendns_dns1,'.');
		TEMP_SetFieldsByDelimit("pptp_dns2",opendns_dns2,'.');
		TEMP_SetFieldsByDelimit("l2tp_dns1",opendns_dns1,'.');
		TEMP_SetFieldsByDelimit("l2tp_dns2",opendns_dns2,'.');
	},
	RemoveItemFromSelect: function(objSelect,objectItemValue)
	{
		//judge if exist
		for (var i=0;i<objSelect.length;i++)
		{
			if (objSelect[i].value==objectItemValue)
			{
				objSelect.remove(i);
				break;
			}
		}
	},
	IsNumber: function(str)
	{
		var num = "0123456789.";
		var isnum=true;
		var c;

		for (i = 0; i < str.length; i++)
		{
			c = str.charAt(i);
			if (num.indexOf(c) == -1)
			{
				isnum = false;
				return isnum;
			}
		}
		return isnum;
	},
	//////////////////////////////////////////////////
	dummy: null
}

function IdleTime(value)
{
	if (value=="")
		return "0";
	else
		return S2I(value);
}

function CheckIP(name, msg)
{
	if (check_ip_validity(TEMP_GetFieldsValue(name,'.')))
		return true;

	BODY.ShowConfigError(name, msg);
	return false;
}

function check_ip_validity(ipstr)
{
	var vals = ipstr.split(".");

	for (var i=0; i<4; i++)
	{
		if (!TEMP_IsDigit(vals[i]) || vals[i]>255 || vals[i]=="")
			return false;
	}
	return true;
}

function RemoveItemFromSelect(objSelect,objectItemValue)
{
	//judge if exist
	for (var i=0; i<objSelect.length; i++)
	{
		if (objSelect[i].value==objectItemValue)
		{
			objSelect.remove(i);
			break;
		}
	}
	return;
}

function AddItemFromSelect(objSelect,objItemText,objectItemValue)
{
	//judge if exist
	for (var i=0;i<objSelect.length;i++)
	{
		if (objSelect[i].value==objectItemValue) return;
	}
	var varItem = document.createElement("option");
	varItem.text = objItemText;
	varItem.value = objectItemValue;
	try {objSelect.add(varItem, null);}
	catch(e){objSelect.add(varItem);}
	return;
}

function Service(svc)
{
	var banner = "<?echo I18N("j","Rebooting");?>...";
	var msg = "<?echo I18N("j","Reboot...");?>";
	var delay = 10;
	var sec = <?echo query("/runtime/device/bootuptime");?> + delay;
	var url = null;
	var ajaxObj = GetAjaxObj("SERVICE");
	if (svc=="FRESET")		url = "http://192.168.0.1/index.php";
	else if (svc=="REBOOT")	url = "http://<?echo $_SERVER["HTTP_HOST"];?>/index.php";
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowMessage("Error","Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		else
			BODY.ShowCountdown(banner, msg, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
//]]>
</script>
