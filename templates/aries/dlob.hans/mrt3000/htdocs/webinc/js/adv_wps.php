<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "MACCTRL,WIFI.PHYINF,PHYINF.WIFI,RUNTIME.WPS",
	OnLoad: function() {
		BODY.DisableCfgElements(false);
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: null,
	
	/* Things we need to do with the configuration.
	 * Some implementation will have visual elements and hidden elements,
	 * both handle the same item. Synchronize is used to sync the visual to the hidden. */
	Synchronize: function()
	{
		if (OBJ("pin").innerHTML!=this.curpin)
		{
			OBJ("mainform").setAttribute("modified", "true");
			XS(this.wifip+"/wps/pin", OBJ("pin").innerHTML);
			if (this.dual_band)
				XS(this.wifip2+"/wps/pin", OBJ("pin").innerHTML);
		}
	},
	
	/* The page specific dirty check function. */
	IsDirty: null,
	
	/* The "services" will be requested by GetCFG(), then the InitValue() will be
	 * called to process the services XML document. */
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var base	= PXML.FindModule("WIFI.PHYINF");
		this.phyinf	= GPBT(base, "phyinf", "uid","BAND24G-1.1", false);
		var uid		= XG(this.phyinf+"/wifi");
		this.wifip	= GPBT(base+"/wifi", "entry", "uid", uid, false);
		this.wpsp	= PXML.FindModule("RUNTIME.WPS");
		if (!this.wifip || !this.wpsp)
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","Initial() ERROR!!!");
			return false;
		}

		this.wpsp += "/runtime/wps/setting";	
		var wps_enable		= XG(this.wifip+"/wps/enable");
		var wps_configured	= XG(this.wifip+"/wps/configured");
		var str_info = "";

		OBJ("en_wps").checked = COMM_ToBOOL(wps_enable);
		if (XG(this.wifip+"/wps/pin")=="")
			this.curpin = OBJ("pin").innerHTML = this.defpin;
		else
			this.curpin = OBJ("pin").innerHTML = XG(this.wifip+"/wps/pin");

		if (this.dual_band)
		{
			this.phyinf2= GPBT(base, "phyinf", "uid","BAND5G-1.1", false);
			uid			= XG(this.phyinf2+"/wifi");
			this.wifip2	= GPBT(base+"/wifi", "entry", "uid", uid, false);
		}
		if (XG(this.wpsp+"/aplocked") != "1")
		{
			OBJ("lock_wifi_security").disabled	= true;
			OBJ("lock_wifi_security").checked	= false;
		}
		else
		{
			OBJ("lock_wifi_security").disabled	= false;
			OBJ("lock_wifi_security").checked	= true;
		}
		this.OnClickEnWPS();

		if (wps_enable == "1")		str_info = "<?echo I18N("j","Enable");?>"; else str_info ="<?echo I18N("j","Disable");?>";
		if (wps_configured == "1")	str_info += "/<?echo I18N("j","Configured");?>"; else str_info += "/<?echo I18N("j","Not Configured");?>";
		OBJ("wifi_info_str").innerHTML = str_info;

		return true;
	},
	PreSubmit: function()
	{
		var lock_wps_security = OBJ("lock_wifi_security").checked ? "1":"0";

		XS(this.wifip+"/wps/enable", (OBJ("en_wps").checked)? "1":"0");

		if (this.dual_band)
		{
			XS(this.wifip2+"/wps/enable", (OBJ("en_wps").checked)? "1":"0");
		}

		//check authtype, if we use radius server, then wps can't be enabled.
		//check authtype, if we use WEP security, then wps can't be enabled.
		if (OBJ("en_wps").checked)
		{
			if (!this.Is_SecuritySupportedByWps(this.wifip) ||
				(this.dual_band&&!this.Is_SecuritySupportedByWps(this.wifip2)))
			{
				OBJ("en_wps").checked = false;
				BODY.ShowAlert("<?echo I18N("j", "WPS isn't supported for these securities : "). "\\n".
						I18N("j","  - WPA-Personal (WPA Only or TKIP only)") . "\\n".
						I18N("j","  - WPA-Enterprise") . "\\n".
						I18N("j","  - WEP security") . "\\n".
						I18N("j","Please select other security in SETUP --> WIRELESS SETTINGS to enable WPS.");?>");
				return null;
			}

			if (this.Is_HiddenSsid(this.wifip) ||
				(this.dual_band && this.Is_HiddenSsid(this.wifip2)))
			{
				OBJ("en_wps").checked = false;
				BODY.ShowAlert("<?echo I18N("j", "WPS can't be enabled when hidden ssid (invinsible) is selected."). "\\n".
						I18N("j","Please select use visible ssid in SETUP --> WIRELESS SETTINGS to enable WPS.");?>");
				return null;
			}

			if (this.Is_MacFilterEnabled())
			{
				OBJ("en_wps").checked = false;
				BODY.ShowAlert("<?echo I18N("j", "WPS can't be enabled when network filter is enabled."). "\\n".
						I18N("j","Please select disable network filter in ADVANCED --> NETWORK FILTER to enable WPS.");?>");
				return null;
			}
		}
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	wifip: null,
	wifip2: null,
	phyinf: null,
	phyinf2: null,
	defpin: '<?echo query("/runtime/devdata/pin");?>',
	curpin: null,
	dual_band: COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>'),
	Is_SecuritySupportedByWps: function(wifipath)
	{
		var auth = XG(wifipath+"/authtype");
		var cipher = XG(wifipath+"/encrtype");
		var issupported = true;

		//wpa-enterprise all not supported
		switch(auth)
		{
		case "WPA":
		case "WPA2":
		case "WPA+2":
		case "WPAEAP":
		case "WPA+2EAP":
		case "WPA2EAP":
			issupported = false;
			break;
		default:
			issupported = true;
			break;
		}

		//wep all not supported
		if (cipher=="WEP")
			issupported = false;

		//wpa-personal, "wpa only" or "tkip only" not supported
		if (auth=="WPAPSK" || cipher=="TKIP")
			issupported = false;
		return issupported;
	},
	Is_MacFilterEnabled:function()
	{
		this.macfp = PXML.FindModule("MACCTRL");
		if (!this.macfp) { return false; }
		this.macfp += "/acl/macctrl";
		var policy = "";

		if ((policy = XG(this.macfp+"/policy")) !== "")
		{
			if (policy == "DISABLE")
				return false;
			else
				return true;
		}

		return false;
	},
	Is_HiddenSsid:function(wifipath)
	{
		if (XG(wifipath+"/ssidhidden") == "1")
			return true;
		else
			return false;
	},
	OnClickEnWPS: function()
	{
		var en_wlan = XG(this.phyinf+"/active");
		var en_wlan2 = XG(this.phyinf2+"/active");

		if (en_wlan == 0 && en_wlan2 == 0)
		{
			OBJ("en_wps").checked	= false;
			OBJ("en_wps").disabled	= true;
		}
		if (OBJ("en_wps").checked)
		{
			if (XG(this.wifip+"/wps/configured")=="0")
			{
				OBJ("reset_cfg").disabled = true;
			}
			else
			{
				OBJ("reset_cfg").disabled = false;
			}
			OBJ("reset_pin").disabled	= false;
			OBJ("gen_pin").disabled		= false;
			OBJ("go_wps").disabled		= false;
		}
		else
		{
			OBJ("reset_cfg").disabled	= true;
			OBJ("reset_pin").disabled	= true;
			OBJ("gen_pin").disabled		= true;
			OBJ("go_wps").disabled		= true;
		}
	},
	OnClickResetPIN: function()
	{
		OBJ("pin").innerHTML = this.defpin;
	},
	OnClickGenPIN: function()
	{
		var pin = "";
		var sum = 0;
		var check_sum = 0;
		var r = 0;
		for(var i=0; i<7; i++)
		{
			r = (Math.floor(Math.random()*9));
			pin += r;
			sum += parseInt(r, [10]) * (((i%2)==0) ? 3:1);
		}
		check_sum = (10-(sum%10))%10;
		pin += check_sum;
		OBJ("pin").innerHTML = pin;
	},
	OnClickResetCfg: function()
	{
		if (confirm("<?echo I18N("j","Are you sure you want to reset the device to Unconfigured?")."\\n".
					I18N("j","This will cause wireless settings to be lost.");?>"))
		{
			Service("RESETCFG.WIFI");
			PXML.CheckModule("WIFI.PHYINF", "ignore", "ignore", "ignore");

			OBJ("mainform").setAttribute("modified", "true");
			OBJ("lock_wifi_security").checked = false;
			BODY.OnSubmit();
		}
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

function Service(svc)
{
	var ajaxObj = GetAjaxObj("SERVICE");
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowMessage("Internal ERROR!", "EVENT "+svc+": "+xml.Get("/report/message"));
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
//]]>
</script>
