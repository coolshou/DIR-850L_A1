<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF,PHYINF.WIFI",
	OnLoad: function() {
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
	{
		switch (code)
		{
		case "OK":
			BODY.OnReload();
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
		if (node.indexOf("ssid")>0)		{ BODY.ShowConfigError("ssid",				msg); return; }
		if (node.indexOf("radius")>0)	{ BODY.ShowConfigError("radius_srv_ip",		msg); return; }
		if (node.indexOf("port")>0)		{ BODY.ShowConfigError("radius_srv_port",	msg); return; }
		if (node.indexOf("secret")>0)	{ BODY.ShowConfigError("radius_srv_sec",	msg); return; }
		if (node.indexOf("groupintv")>0){ BODY.ShowConfigError("wpa_grp_key_intrv",	msg); return; }

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
		this.wifi_module = PXML.FindModule("WIFI.PHYINF");
		if (!this.wifi_module||!this.Initial("BAND24G-1.1"))
		{
			BODY.ShowMessage("Error","Initial() ERROR!!!");
			return false;
		}

		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if (this.dual_band)
		{
			this.Show11A(true);
			if (!this.Initial("BAND5G-1.1"))
			{
				BODY.ShowMessage("Error","Initial() ERROR!!!");
				return false;
			}
		}
		else
			this.Show11A(false);

		return true;
	},
	PreSubmit: function()
	{
		if (!this.ValidityCheck("BAND24G-1.1")) return null;
		if (!this.SaveXML("BAND24G-1.1")) return null;
		if (!this.WPSCHK("BAND24G-1.1")) return null;
		if (this.dual_band)
		{
			if (!this.ValidityCheck("BAND5G-1.1")) return null;
			if (!this.SaveXML("BAND5G-1.1")) return null;
			if (!this.WPSCHK("BAND5G-1.1")) return null;
			this.WPS_CONFIGURED_CHK("BAND24G-1.1","BAND5G-1.1");
		}
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	wifip: null,
	phyinf: null,
	wifi_module: null,
	sec_type: null,
	sec_type_11a: null,
	bandWidth: null,
	shortGuard: null,
	dual_band: 0,
	str_Aband: null,
	dual_band: null,
	feature_nosch: null,
	Initial: function(wlan_uid)
	{
		this.phyinf = GPBT(this.wifi_module, "phyinf", "uid",wlan_uid, false);
		var uid		= XG(this.phyinf+"/wifi");
		var freq	= XG(this.phyinf+"/media/freq");
		this.wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", uid, false);

		if (freq == "5")postfix = "_11a";
		else			postfix = "";

		COMM_SetSelectValue(OBJ("wlan_mode"+postfix), XG(this.phyinf+"/media/wlmode"));
		OBJ("en_wifi"+postfix).checked = COMM_ToBOOL(XG(this.phyinf+"/active"));
		OBJ("ssid"+postfix).value = XG(this.wifip+"/ssid");
		OBJ("auto_ch"+postfix).checked = (XG(this.phyinf+"/media/channel")=="0")? true : false;
		if (!OBJ("auto_ch"+postfix).checked)
			COMM_SetSelectValue(OBJ("channel"+postfix), XG(this.phyinf+"/media/channel"));

		if (COMM_ToBOOL(XG(this.wifip+"/ssidhidden")))
			OBJ("ssid_invisible"+postfix).checked = true;
		else
			OBJ("ssid_visible"+postfix).checked = true;

		this.OnChangeWLMode(postfix); //move from last sequence, bc. need to create security list

		///////////////// initial WEP /////////////////
		var auth = XG(this.wifip+"/authtype");
		var len = (XG(this.wifip+"/nwkey/wep/size")=="")? "64" : XG(this.wifip+"/nwkey/wep/size");
		var defkey = (XG(this.wifip+"/nwkey/wep/defkey")=="")? "1" : XG(this.wifip+"/nwkey/wep/defkey");
		var wepauth = (auth=="SHARED") ? "SHARED" : "WEPAUTO";

		COMM_SetSelectValue(OBJ("auth_type"+postfix), wepauth);
		COMM_SetSelectValue(OBJ("wep_key_len"+postfix), len);
		COMM_SetSelectValue(OBJ("wep_def_key"+postfix), defkey);
		var keys = document.getElementsByName("wepkey_"+len+postfix);
		for (var i=0; i<keys.length; i++)
			keys[i].value = XG(this.wifip+"/nwkey/wep/key:"+(i+1));
		///////////////// initial WPA /////////////////
		var cipher = XG(this.wifip+"/encrtype");
		var type = null;
		var sec_type = null;

		switch (auth)
		{
		case "WPA":
		case "WPA2":
		case "WPA+2":
		case "WPAEAP":
		case "WPA+2EAP":
		case "WPA2EAP":
			sec_type = "wpa_enterprise";
			wpa_mode = auth;
			break;
		case "WPAPSK":
			sec_type = "wpa_personal";
			wpa_mode = "WPA";
			break;
		case "WPA2PSK":
			sec_type = "wpa_personal";
			wpa_mode = "WPA2";
			break;
		case "WPA+2PSK":
			sec_type = "wpa_personal";
			wpa_mode = "WPA+2";
			break;
		default:
			sec_type = "";
			wpa_mode = "WPA+2";
		}

		if (cipher=="WEP") sec_type = "wep";

		COMM_SetSelectValue(OBJ("security_type"+postfix), sec_type);
		COMM_SetSelectValue(OBJ("wpa_mode"+postfix), wpa_mode);
		COMM_SetSelectValue(OBJ("cipher_type"+postfix), cipher);

		if (postfix == "")	this.sec_type = sec_type;
		else				this.sec_type_11a = sec_type;

		OBJ("wpa_psk_key"+postfix).value = XG(this.wifip+"/nwkey/psk/key");
		OBJ("wpa_grp_key_intrv"+postfix).value = (XG(this.wifip+"/nwkey/wpa/groupintv")=="")? "3600" : XG(this.wifip+"/nwkey/wpa/groupintv");
		if (XG(this.wifip+"/nwkey/eap/radius")!="")
			TEMP_SetFieldsByDelimit("radius_srv_ip"+postfix, XG(this.wifip+"/nwkey/eap/radius"), '.');
		OBJ("radius_srv_port"+postfix).value = (XG(this.wifip+"/nwkey/eap/port")==""?"1812":XG(this.wifip+"/nwkey/eap/port"));
		OBJ("radius_srv_sec"+postfix).value	= XG(this.wifip+"/nwkey/eap/secret");

		this.OnChangeSecurityType(postfix);
		this.OnChangeWEPKey(postfix);
		this.OnClickEnWLAN(postfix);
		this.OnClickEnAutoChannel(postfix);
		this.OnChangeChannel(postfix);

		return true;
	},
	SetWps: function(string)
	{
		var phyinf = GPBT(this.wifi_module, "phyinf", "uid","BAND24G-1.1", false);
		var wifip = GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);

		if (this.dual_band)
		{
			var phyinf2 = GPBT(this.wifi_module, "phyinf", "uid","BAND5G-1.1", false);
			var wifip2 = GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf2+"/wifi"), false);
		}

		if (string=="enable")
		{
			XS(wifip+"/wps/enable", "1");
			if (this.dual_band) XS(wifip2+"/wps/enable", "1");
		}
		else
		{
			XS(wifip+"/wps/enable", "0");
			if (this.dual_band) XS(wifip2+"/wps/enable", "0");
		}
	},
	WPS_CONFIGURED_CHK: function(wlan_uid,wlan2_uid)
	{
		var phyinf	= GPBT(this.wifi_module,"phyinf","uid",wlan_uid,false);
		var wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);
		var phyinf2	= GPBT(this.wifi_module,"phyinf","uid",wlan2_uid,false);
		var wifip2	= GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf2+"/wifi"), false);

		var wps_configured = XG(wifip+"/wps/configured");
		var wps_configured2 = XG(wifip2+"/wps/configured");
		if (wps_configured!=wps_configured2)
		{
			XS(wifip+"/wps/configured", "1");
			XS(wifip2+"/wps/configured", "1");
		}
	},
	WPSCHK: function(wlan_uid)
	{
		var phyinf	= GPBT(this.wifi_module,"phyinf","uid",wlan_uid,false);
		var freq	= XG(phyinf+"/media/freq");
		var wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);

		if (freq == "5")postfix = "_Aband";
		else			postfix = "";

		if (COMM_EqBOOL(OBJ("ssid"+postfix).getAttribute("modified"),true)||
			COMM_EqBOOL(OBJ("security_type"+postfix).getAttribute("modified"),true)||
			COMM_EqBOOL(OBJ("cipher_type"+postfix).getAttribute("modified"),true)||
			COMM_EqBOOL(OBJ("wpa_psk_key"+postfix).getAttribute("modified"),true)||
			COMM_EqBOOL(OBJ("wep_def_key"+postfix).getAttribute("modified"),true))
		{
			XS(wifip+"/wps/configured", "1");
			XS(wifip+"/wps/locksecurity", "1");
		}

		//for pass WPS 2.0 test, we add warning when security is disabled.
		var wifi_enabled = OBJ("en_wifi"+postfix).checked;
		if (wifi_enabled && OBJ("security_type"+postfix).value=="")
		{
			if (postfix=="")
			{
				if (!confirm('<?echo I18N("j", "Warning ! Selecting None in Security Mode will make your 2.4Ghz wifi network vulnerable. Proceed ? ");?>'))
					return false;
			}
			else
			{
				if (!confirm('<?echo I18N("j", "Warning ! Selecting None in Security Mode will make your 5Ghz wifi network vulnerable. Proceed ? ");?>'))
					return false;
			}
		}
		return true;
	},
	SaveXML: function(wlan_uid)
	{
		var phyinf	= GPBT(this.wifi_module, "phyinf", "uid", wlan_uid, false);
		var uid		= XG(phyinf+"/wifi");
		var wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", uid, false);
		var freq	= XG(phyinf+"/media/freq");

		if (freq == "5")postfix = "_11a";
		else			postfix = "";

		if (OBJ("en_wifi"+postfix).checked)
			XS(phyinf+"/active", "1");
		else
		{
			XS(phyinf+"/active", "0");
			return true;
		}

		if (this.feature_nosch==0)
			XS(phyinf+"/schedule", OBJ("sch"+postfix).value);

		XS(wifip+"/ssid", OBJ("ssid"+postfix).value);

		if (OBJ("auto_ch"+postfix).checked)
			XS(phyinf+"/media/channel", "0");
		else
			XS(phyinf+"/media/channel", OBJ("channel"+postfix).value);

		if (OBJ("txrate"+postfix).value=="-1")
		{
			XS(phyinf+"/media/dot11n/mcs/auto", "1");
			XS(phyinf+"/media/dot11n/mcs/index", "");
		}
		else
		{
			XS(phyinf+"/media/dot11n/mcs/auto", "0");
			XS(phyinf+"/media/dot11n/mcs/index", OBJ("txrate"+postfix).value);
		}
		XS(phyinf+"/media/wlmode", OBJ("wlan_mode"+postfix).value);
		if (/n/.test(OBJ("wlan_mode"+postfix).value))
		{
			XS(phyinf+"/media/dot11n/bandwidth", OBJ("bw"+postfix).value);
			this.bandWidth = OBJ("bw"+postfix).value;
		}
		XS(wifip+"/ssidhidden", SetBNode(OBJ("ssid_invisible"+postfix).checked));

		if (OBJ("security_type"+postfix).value=="wep")
		{
			if (OBJ("auth_type"+postfix).value=="SHARED")
				XS(wifip+"/authtype", "SHARED");
			else
				XS(wifip+"/authtype", "WEPAUTO");
			XS(wifip+"/encrtype", "WEP");
			XS(wifip+"/nwkey/wep/size", "");
			XS(wifip+"/nwkey/wep/ascii", "");
			XS(wifip+"/nwkey/wep/defkey", OBJ("wep_def_key"+postfix).value);
			for (var i=0,cnt=1, len=OBJ("wep_key_len"+postfix).value; i<4; i++,cnt++)
			{
				if (cnt==OBJ("wep_def_key"+postfix).value)
					XS(wifip+"/nwkey/wep/key:"+cnt, document.getElementsByName("wepkey_"+len+postfix)[i].value);
				else
					XS(wifip+"/nwkey/wep/key:"+cnt, "");
			}
		}
		else if (OBJ("security_type"+postfix).value=="wpa_personal")
		{
			XS(wifip+"/authtype",				OBJ("wpa_mode"+postfix).value+"PSK");
			XS(wifip+"/encrtype",				OBJ("cipher_type"+postfix).value);
			XS(wifip+"/nwkey/psk/passphrase",	"");
			XS(wifip+"/nwkey/psk/key",			OBJ("wpa_psk_key"+postfix).value);
			XS(wifip+"/nwkey/wpa/groupintv",	OBJ("wpa_grp_key_intrv"+postfix).value);
		}
		else if (OBJ("security_type"+postfix).value=="wpa_enterprise")
		{
			XS(wifip+"/authtype",				OBJ("wpa_mode"+postfix).value);
			XS(wifip+"/encrtype",				OBJ("cipher_type"+postfix).value);
			XS(wifip+"/nwkey/wpa/groupintv",	OBJ("wpa_grp_key_intrv"+postfix).value);
			XS(wifip+"/nwkey/eap/port",			OBJ("radius_srv_port"+postfix).value);
			XS(wifip+"/nwkey/eap/secret",		OBJ("radius_srv_sec"+postfix).value);
			XS(wifip+"/nwkey/eap/radius", 		TEMP_GetFieldsValue("radius_srv_ip"+postfix, '.'));
		}
		else
		{
			XS(wifip+"/authtype", "OPEN");
			XS(wifip+"/encrtype", "NONE");
		}
		return true;
	},
	OnClickEnWLAN: function(postfix)
	{
		if (AUTH.AuthorizedGroup >= 100) return;
		var isdisabled = (!OBJ("en_wifi"+postfix).checked)?true:false;
		if (this.feature_nosch==0)
		{
			OBJ("sch"+postfix).disabled =
			OBJ("go2sch"+postfix).disabled =
			isdisabled;
		}
		OBJ("ssid"+postfix).disabled =
		OBJ("wlan_mode"+postfix).disabled =
		OBJ("auto_ch"+postfix).disabled =
		OBJ("txrate"+postfix).disabled =
		OBJ("ssid_visible"+postfix).disabled =
		OBJ("ssid_invisible"+postfix).disabled =
		OBJ("security_type"+postfix).disabled =
		isdisabled;

		if (OBJ("en_wifi"+postfix).checked)
		{
			if (!OBJ("auto_ch"+postfix).checked)
				OBJ("channel"+postfix).disabled = false;
			if (/n/.test(OBJ("wlan_mode"+postfix).value))
				OBJ("bw"+postfix).disabled = false;

			if (postfix=="")COMM_SetSelectValue(OBJ("security_type"+postfix), this.sec_type);
			else			COMM_SetSelectValue(OBJ("security_type"+postfix), this.sec_type_11a);
		}
		else
		{
			OBJ("channel"+postfix).disabled = true;
			OBJ("bw"+postfix).disabled = true;

			if (postfix=="")this.sec_type = OBJ("security_type"+postfix).value;
			else			this.sec_type_11a = OBJ("security_type"+postfix).value;

			COMM_SetSelectValue(OBJ("security_type"+postfix), "");
		}
		this.OnChangeSecurityType(postfix);
	},
	OnChangeChannel: function(postfix)
	{
		if (!OBJ("auto_ch"+postfix).checked)
		{
			if (OBJ("channel"+postfix).value=="140"||
				OBJ("channel"+postfix).value=="165"||
				OBJ("channel"+postfix).value=="12"||
				OBJ("channel"+postfix).value=="13")
			{
				OBJ("bw"+postfix).value="20";
				OBJ("bw"+postfix).disabled = true;
			}
			else
			{
				OBJ("bw"+postfix).disabled = false;
			}
		}
	},
	OnClickEnAutoChannel: function(postfix)
	{
		if (OBJ("auto_ch"+postfix).checked || !OBJ("en_wifi"+postfix).checked)
			OBJ("channel"+postfix).disabled = true;
		else
			OBJ("channel"+postfix).disabled = false;
	},
	SetStyleDisplay: function(name, value)
	{
		var objs = document.getElementsByName(name);
		for (var i=0; i< objs.length; i++)
			objs[i].style.display = value;
	},
	OnChangeSecurityType: function(postfix)
	{
		switch (OBJ("security_type"+postfix).value)
		{
		case "":
			this.SetStyleDisplay("wep"+postfix, "none");
			this.SetStyleDisplay("wpa"+postfix, "none");
			this.SetStyleDisplay("wpapsk"+postfix, "none");
			this.SetStyleDisplay("wpaeap"+postfix, "none");
			break;
		case "wep":
			this.SetStyleDisplay("wep"+postfix, "");
			this.SetStyleDisplay("wpa"+postfix, "none");
			this.SetStyleDisplay("wpapsk"+postfix, "none");
			this.SetStyleDisplay("wpaeap"+postfix, "none");
			break;
		case "wpa_personal":
			this.SetStyleDisplay("wep"+postfix, "none");
			this.SetStyleDisplay("wpa"+postfix, "");
			this.SetStyleDisplay("wpapsk"+postfix, "");
			this.SetStyleDisplay("wpaeap"+postfix, "none");
			break;
		case "wpa_enterprise":
			this.SetStyleDisplay("wep"+postfix, "none");
			this.SetStyleDisplay("wpa"+postfix, "");
			this.SetStyleDisplay("wpapsk"+postfix, "none");
			this.SetStyleDisplay("wpaeap"+postfix, "");
			break;
		}
	},
	OnChangeWPAMode: function(postfix)
	{
		switch (OBJ("wpa_mode"+postfix).value)
		{
		case "WPA":
			OBJ("cipher_type"+postfix).value = "TKIP";
			break;
		case "WPA2":
			OBJ("cipher_type"+postfix).value = "AES";
			break;
		default :
			OBJ("cipher_type"+postfix).value = "TKIP+AES";
		}
	},
//	OnChangeWEPAuth: function(postfix)
//	{
//		if (OBJ("auth_type"+postfix).value == "SHARED" && this.wps==true)
//		{
//			BODY.ShowMessage("Error","Can't choose shared key when wps is enable !!");
//			OBJ("auth_type"+postfix).value = "WEPAUTO";
//		}
//	},
	OnChangeWEPKey: function(postfix)
	{
		var no = S2I(OBJ("wep_def_key"+postfix).value) - 1;
		var len = OBJ("wep_key_len"+postfix).value;

		this.SetStyleDisplay("wepkey_64"+postfix, "none");
		this.SetStyleDisplay("wepkey_128"+postfix, "none");
		document.getElementsByName("wepkey_"+len+postfix)[no].style.display = "";
	},
	OnChangeWLMode: function(postfix)
	{
		var phywlan = "";
		if (postfix==="")	phywlan = GPBT(this.wifi_module, "phyinf", "uid","BAND24G-1.1", false);
		else				phywlan = GPBT(this.wifi_module, "phyinf", "uid","BAND5G-1.1", false);
		if (/n/.test(OBJ("wlan_mode"+postfix).value))
		{
			this.bandWidth  = XG(phywlan+"/media/dot11n/bandwidth");
			COMM_SetSelectValue(OBJ("bw"+postfix), this.bandWidth);
			OBJ("bw"+postfix).disabled = false;
		}
		else
		{
			OBJ("bw"+postfix).disabled = true;
		}
		this.shortGuard = XG(phywlan+"/media/dot11n/guardinterval");
		DrawTxRateList(OBJ("bw"+postfix).value, this.shortGuard, postfix);
		if (OBJ("wlan_mode"+postfix).value === "n")
		{
			var rate = XG(phywlan+"/media/dot11n/mcs/index");
			if (rate=="") rate = "-1";
			COMM_SetSelectValue(OBJ("txrate"+postfix), rate);
		}
		DrawSecurityList(OBJ("wlan_mode"+postfix).value, postfix);
		this.OnChangeSecurityType(postfix);
	},
	/* For ssid, WEP key, WPA key, we don't allow whitespace in front OR behind !!! */
	ValidityCheck: function(wlan_uid)
	{
		var phyinf	= GPBT(this.wifi_module,"phyinf","uid",wlan_uid,false);
		var uid		= XG(phyinf+"/wifi");
		var wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", uid, false);
		var freq	= XG(phyinf+"/media/freq");

		if (freq == "5")postfix = "_11a";
		else			postfix = "";

		var obj_ssid	= OBJ("ssid"+postfix).value;
		var obj_wpa_key	= OBJ("wpa_psk_key"+postfix).value;
		var wep_key		= OBJ("wep_def_key"+postfix).value;
		var wep_key_len	= OBJ("wep_key_len"+postfix).value;
		var obj_wep_key	= document.getElementsByName("wepkey_"+wep_key_len+postfix)[wep_key-1].value;

		if (obj_ssid.charAt(0)===" "|| obj_ssid.charAt(obj_ssid.length-1)===" ")
		{
			BODY.ShowConfigError("ssid"+postfix,
				"<?echo I18N("j", "The prefix or postfix of the 'Wireless Network Name' cannot be blank.");?>");
			return false;
		}

		if (OBJ("security_type"+postfix).value==="wep")
		{
			if (obj_wep_key.charAt(0) === " "|| obj_wep_key.charAt(obj_wep_key.length-1)===" ")
			{
				BODY.ShowConfigError("wepkey_64"+postfix,
				"<?echo I18N("j","The prefix or postfix of the 'WEP Key' cannot be blank.");?>");
				return false;
			}
		}
		else if (OBJ("security_type"+postfix).value==="wpa_personal")
		{
			if (obj_wpa_key.charAt(0)===" " || obj_wpa_key.charAt(obj_wpa_key.length-1)===" ")
			{
				BODY.ShowConfigError("wpa_psk_key"+postfix,
					"<?echo I18N("j", "The prefix or postfix of the 'Pre-Shared Key' cannot be blank.");?>");
				return false;
			}
		}
		else if (OBJ("security_type"+postfix).value==="wpa_enterprise")
		{
			var radius_key = OBJ("radius_srv_sec"+postfix).value;

			if (radius_key.charAt(0)===" " || radius_key.charAt(radius_key.length-1)===" ")
			{
				return false;
			}
		}
		return true;
	},
	Show11A: function(bool)
	{
		obj = OBJ("11a_start");
		while (obj)
		{
			if (obj.tagName=="TR") obj.style.display = bool? "":"none";
			obj = obj.nextSibling;
			if (obj.id=="11a_end") break;
		}
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

function SetBNode(value)
{
	return COMM_ToBOOL(value)? "1":"0";
}

function SetDisplayStyle(tag, name, style)
{
	if (tag)	var obj = GetElementsByName_iefix(tag, name);
	else		var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		obj[i].style.display = style;
	}
}
function GetElementsByName_iefix(tag, name)
{
	var elem = document.getElementsByTagName(tag);
	var arr = new Array();
	for (i = 0,iarr = 0; i < elem.length; i++)
	{
		att = elem[i].getAttribute("name");
		if (att == name)
		{
			arr[iarr] = elem[i];
			iarr++;
		}
	}
	return arr;
}
function DrawTxRateList(bw, sgi, postfix)
{
	var listOptions = null;
	var cond = bw+":"+sgi;
	switch(cond)
	{
		case "20:800":
			listOptions = new Array("0 - 6.5","1 - 13.0","2 - 19.5","3 - 26.0","4 - 39.0","5 - 52.0","6 - 58.5","7 - 65.0"<?
					$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND24G-1.1", 0);
					$ms = query($p."/media/multistream");
					if ($ms != "1T1R")
					echo ',"8 - 13.0","9 - 26.0","10 - 39.0","11 - 52.0","12 - 78.0","13 - 104.0","14 - 117.0","15 - 130.0"';
					?>);
		break;
		case "20:400":
			listOptions = new Array("0 - 7.2","1 - 14.4","2 - 21.7","3 - 28.9","4 - 43.3","5 - 57.8","6 - 65.0","7 - 72.0"<?
					if ($ms != "1T1R")
					echo ',"8 - 14.444","9 - 28.889","10 - 43.333","11 - 57.778","12 - 86.667","13 - 115.556","14 - 130.000","15 - 144.444"';
					?>);
		break;
		case "20+40:800":
			listOptions = new Array("0 - 13.5","1 - 27.0","2 - 40.5","3 - 54.0","4 - 81.0","5 - 108.0","6 - 121.5","7 - 135.0"<?
					if ($ms != "1T1R")
					echo ',"8 - 27.0","9 - 54.0","10 - 81.0","11 - 108.0","12 - 162.0","13 - 216.0","14 - 243.0","15 - 270.0"';
					?>);
		break;
		case "20+40:400":
			listOptions = new Array("0 - 15.0","1 - 30.0","2 - 45.0","3 - 60.0","4 - 90.0","5 - 120.0","6 - 135.0","7 - 150.0"<?
					if ($ms != "1T1R")
					echo ',"8 - 30.0","9 - 60.0","10 - 90.0","11 - 120.0","12 - 180.0","13 - 240.0","14 - 270.0","15 - 300.0"';
					?>);
		break;
	}

	var tr_length = OBJ("txrate"+postfix).length;
	for (var idx=1; idx<tr_length; idx++)
	{
		OBJ("txrate"+postfix).remove(1);
	}
	if (OBJ("wlan_mode"+postfix).value === "n")
	{
		for (var idx=0; idx<listOptions.length; idx++)
		{
			var item = document.createElement("option");
			item.value = idx;
			item.text = listOptions[idx];
			try		{ OBJ("txrate"+postfix).add(item, null); }
			catch(e){ OBJ("txrate"+postfix).add(item); }
		}
	}
}
function DrawSecurityList(wlan_mode, postfix)
{
	var security_list = null;
	var cipher_list = null;
	if (wlan_mode === "n")
	{
//		security_list = ['wpa_personal', '<?echo I18N("j","WPA");?>',
//					  'wpa_enterprise', '<?echo I18N("j","WPA-Enterprise");?>'];
		security_list = ['wpa_personal', '<?echo I18N("j","WPA");?>'];
		cipher_list = ['AES'];
	}
	else
	{
//		security_list = ['wep', '<?echo I18N("j","WEP");?>',
//					  'wpa_personal', '<?echo I18N("j","WPA");?>',
//					  'wpa_enterprise', '<?echo I18N("j","WPA-Enterprise");?>'];
		security_list = ['wep', '<?echo I18N("j","WEP");?>',
					  'wpa_personal', '<?echo I18N("j","WPA");?>'];
		cipher_list = ['TKIP+AES','TKIP','AES'];
	}
	//modify security_type
	var sec_length = OBJ("security_type"+postfix).length;
	for (var idx=1; idx<sec_length; idx++)
	{
		OBJ("security_type"+postfix).remove(1);
	}
	for (var idx=0; idx<security_list.length; idx++)
	{
		var item = document.createElement("option");
		item.value = security_list[idx++];
		item.text = security_list[idx];
		try		{ OBJ("security_type"+postfix).add(item, null); }
		catch(e){ OBJ("security_type"+postfix).add(item); }
	}
	// modify cipher_type
	var ci_length = OBJ("cipher_type"+postfix).length;
	for (var idx=0; idx<ci_length; idx++)
	{
		OBJ("cipher_type"+postfix).remove(0);
	}
	for (var idx=0; idx<cipher_list.length; idx++)
	{
		var item = document.createElement("option");
		item.value = cipher_list[idx];
		if (item.value=="TKIP+AES")
			item.text = "TKIP and AES";
		else
			item.text = cipher_list[idx];
		try		{ OBJ("cipher_type"+postfix).add(item, null); }
		catch(e){ OBJ("cipher_type"+postfix).add(item); }
	}
}
//]]>
</script>
