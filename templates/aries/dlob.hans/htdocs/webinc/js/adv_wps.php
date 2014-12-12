<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "MACCTRL,WIFI.PHYINF,PHYINF.WIFI,RUNTIME.WPS",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },

	wifip: null,
	defpin: '<?echo query("/runtime/devdata/pin");?>',
	curpin: null,
	dual_band: COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>'),	
	wifi_module: null,

	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.wifi_module 	= PXML.FindModule("WIFI.PHYINF");
		this.phyinf 		= GPBT(this.wifi_module, "phyinf", "uid","BAND24G-1.1", false);
		this.wifip 			= XG(this.phyinf+"/wifi");
		this.wifip 			= GPBT(this.wifi_module+"/wifi", "entry", "uid", this.wifip, false);
		this.wpsp			= PXML.FindModule("RUNTIME.WPS");

		if (!this.wifip || !this.wpsp)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		
		this.wpsp += "/runtime/wps/setting";	
		var wps_enable 		= XG(this.wifip+"/wps/enable");
		var wps_configured  = XG(this.wifip+"/wps/configured");
		var str_info = "";
		
		OBJ("en_wps").checked = COMM_ToBOOL(wps_enable);
		if (XG(this.wifip+"/wps/pin")=="")
			this.curpin = OBJ("pin").innerHTML = this.defpin;
		else
			this.curpin = OBJ("pin").innerHTML = XG(this.wifip+"/wps/pin");

		if(this.dual_band)
		{
			this.phyinf2 	= GPBT(this.wifi_module, "phyinf", "uid","BAND5G-1.1", false);
			this.wifip2 	= XG(this.phyinf2+"/wifi");
			this.wifip2 	= GPBT(this.wifi_module+"/wifi", "entry", "uid", this.wifip2, false);
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
				
		if(wps_enable == "1") 		str_info = "<?echo I18N("j","Enable");?>"; else str_info ="<?echo I18N("j","Disable");?>";
		if(wps_configured == "1") 	str_info +=  "/<?echo I18N("j","Configured");?>"; else str_info += "/<?echo I18N("j","Not Configured");?>";
		OBJ("wifi_info_str").innerHTML = str_info;
		
		return true;
	},
	PreSubmit: function()
	{
		var lock_wps_security = OBJ("lock_wifi_security").checked ? "1":"0";
		
		XS(this.wifip+"/wps/enable", (OBJ("en_wps").checked)? "1":"0");
		
		if(this.dual_band)
		{
			XS(this.wifip2+"/wps/enable", (OBJ("en_wps").checked)? "1":"0");
		}
		XS(this.wpsp+"/aplocked", lock_wps_security);
		//check authtype, if we use radius server, then wps can't be enabled.
		//check authtype, if we use WEP security, then wps can't be enabled.
		if(OBJ("en_wps").checked)
		{
			if(!this.Is_SecuritySupportedByWps(this.wifip) || 
				(this.dual_band && !this.Is_SecuritySupportedByWps(this.wifip2)) )
		{
			OBJ("en_wps").checked		= false;
				BODY.ShowAlert("<?echo I18N("j", "WPS isn't supported for these securities : "). "\\n". 
					I18N("j","  - WPA-Personal (WPA Only or TKIP only)") . "\\n". 
					"  - ".I18N("j","WPA-Enterprise") . "\\n". 
					"  - WEP ".I18N("j","Security") . "\\n". 
							I18N("j","Please select other security in SETUP --> WIRELESS SETTINGS to enable WPS.");?>");
			return null;
		}
			
			if(this.Is_HiddenSsid(this.phyinf, this.wifip) || 
				(this.dual_band && this.Is_HiddenSsid(this.phyinf2, this.wifip2)) )
			{
				OBJ("en_wps").checked		= false;
				BODY.ShowAlert("<?echo I18N("j", "WPS can't be enabled when a hidden SSID (invisible) is selected."). "\\n".
								I18N("j","Please select use visible SSID in SETUP => WIRELESS SETTINGS to enable WPS.");?>");
				return null;
			}
			
			var wifi_verify	= "<? echo get('', '/runtime/devdata/wifiverify');?>";
			if(wifi_verify=="1" && this.Is_MacFilterEnabled())
			{
				OBJ("en_wps").checked		= false;
				BODY.ShowAlert("<?echo I18N("j", "WPS can't be enabled when network filter is enabled."). "\\n".
								I18N("j","Please select disable network filter in ADVANCED --> NETWORK FILTER to enable WPS.");?>");
				return null;
			}
		}
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function()
	{
		if (OBJ("pin").innerHTML!=this.curpin)
		{
			OBJ("mainform").setAttribute("modified", "true");
			XS(this.wifip+"/wps/pin", OBJ("pin").innerHTML);
			if(this.dual_band)
				XS(this.wifip2+"/wps/pin", OBJ("pin").innerHTML);
		}
	},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	//OnCheckWPAEnterprise:function()
	Is_SecuritySupportedByWps:function(wifipath)
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
			default : 
				issupported = true;
				break;
		}
		
		//wep all not supported
		if (cipher=="WEP")
			issupported = false;
		
		//wpa-personal, "wpa only" or "tkip only" not supported
		if(auth=="WPAPSK" || cipher=="TKIP")
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
			if(policy == "DISABLE")
				return false;
			else 
				return true;
		}
		
		return false;
	},
	
	
	Is_HiddenSsid:function(phyinfpath, wifipath)
	{
		if(XG(phyinfpath+"/active")=="1" && XG(wifipath+"/ssidhidden")=="1")
			return true;
		else 
			return false;
	}, 

	OnClickEnWPS: function()
	{
		var en_wlan = XG(this.phyinf+"/active");
		var en_wlan2 = XG(this.phyinf2+"/active");
		
		if(en_wlan == 0 && en_wlan2 == 0)
		{
			OBJ("en_wps").checked 		= false;
			OBJ("en_wps").disabled		= true;
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
		if (confirm("<?echo i18n("Are you sure you want to reset the device to Unconfigured?")."\\n".
					i18n("This will cause wireless settings to be lost.");?>"))
		{
			Service("RESETCFG.WIFI");
			PXML.CheckModule("WIFI.PHYINF", "ignore", "ignore", "ignore");
			
			//self.location='<?=$TEMP_MYNAME?>.php?r='+COMM_RandomStr(5);
			//BODY.OnReload();
			OBJ("mainform").setAttribute("modified", "true");
			OBJ("lock_wifi_security").checked = false;
			BODY.OnSubmit();
			/*
			OBJ("mainform").setAttribute("modified", "true");
			XS(this.wifip+"/ssid",			"dlink"	);
			XS(this.wifip+"/authtype",		"OPEN"	);
			XS(this.wifip+"/encrtype",		"NONE"	);
			XS(this.wifip+"/wps/configured","0"		);			
			XS(this.wifip2+"/ssid",			"dlink_media");
			XS(this.wifip2+"/authtype",		"OPEN"	);
			XS(this.wifip2+"/encrtype",		"NONE"	);
			XS(this.wifip2+"/wps/configured","0"	);
			BODY.OnSubmit();
			*/
		}
	}
}

function Service(svc)
{	
	//var banner = "<?echo i18n("RESET WIFI CONFIG");?>...";
	//var msgArray = ["<?echo i18n("Device is resetting wireless config. Please wait ... ");?>"];
	//var sec = 10;
	//var url = null;
	var ajaxObj = GetAjaxObj("SERVICE");
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowAlert("Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		//else
		//	BODY.ShowCountdown(banner, msgArray, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}

</script>
