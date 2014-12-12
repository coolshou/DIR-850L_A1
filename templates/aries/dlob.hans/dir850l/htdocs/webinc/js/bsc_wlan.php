<?include "/htdocs/phplib/phyinf.php";?>
<script type="text/javascript">
function Page() {}
Page.prototype =
{	
	services: "WIFI.PHYINF,PHYINF.WIFI",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		return false; 
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;		
		if(!this.Initial("BAND24G-1.1","WIFI.PHYINF")) return false; 				
		
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		
		if(this.dual_band)
		{
			OBJ("div_5G").style.display = "block";
			if(!this.Initial("BAND5G-1.1","WIFI.PHYINF")) return false; 				
		}
		else 				
			OBJ("div_5G").style.display = "none";
		
		//alert(OBJ("bw_Aband").options[2].value);//Joseph
		return true;
	},
	PreSubmit: function()
	{		
		if(!this.ValidityCheck("BAND24G-1.1")) return null; 				
		
		if(!this.SaveXML("BAND24G-1.1")) return null; 				
		if(!this.WPSCHK("BAND24G-1.1")) return null; 
		
		if(this.dual_band)
		{
			if(!this.ValidityCheck("BAND5G-1.1")) return null; 				
			if(!this.SaveXML("BAND5G-1.1")) return null;
			if(!this.WPSCHK("BAND5G-1.1")) return null;
			this.WPS_CONFIGURED_CHK("BAND24G-1.1","BAND5G-1.1");
		}
		return PXML.doc;
	},			
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	wifip: null,
	phyinf: null,
	sec_type: null,
	sec_type_Aband: null,
	bandWidth: null,
	shortGuard: null,
	wps: true,
	dual_band: 0,
	radius_adv_flag: 0,

	str_Aband: null,
	dual_band: null,
	feature_nosch: null,
	Initial: function(wlan_uid,wifi_module)
	{
		this.wifi_module 			= PXML.FindModule(wifi_module);
		if (!this.wifi_module)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.phyinf = GPBT(this.wifi_module, "phyinf", "uid",wlan_uid, false);

		var wifi_profile 	= XG(this.phyinf+"/wifi");
		var freq 			= XG(this.phyinf+"/media/freq");
		this.wifip 			= GPBT(this.wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		
		if(freq == "5") 	str_Aband = "_Aband";
		else				str_Aband = "";
		
		COMM_SetSelectValue(OBJ("wlan_mode"+str_Aband), XG(this.phyinf+"/media/wlmode"));
		OBJ("en_wifi"+str_Aband).checked = COMM_ToBOOL(XG(this.phyinf+"/active"));
		
		<? if($FEATURE_NOSCH!="1")echo 'this.feature_nosch=0;'; else echo 'this.feature_nosch=1;'; ?>
		
		if(this.feature_nosch==0)
			COMM_SetSelectValue(OBJ("sch"+str_Aband), XG(this.phyinf+"/schedule"));
		
		OBJ("ssid"+str_Aband).value 				= XG(this.wifip+"/ssid");
		OBJ("auto_ch"+str_Aband).checked 			= (XG(this.phyinf+"/media/channel")=="0")? true : false;
		if (!OBJ("auto_ch"+str_Aband).checked)
			COMM_SetSelectValue(OBJ("channel"+str_Aband), XG(this.phyinf+"/media/channel"));
		
		OBJ("en_wmm"+str_Aband).checked = COMM_ToBOOL(XG(this.phyinf+"/media/wmm/enable"));
					
		if(COMM_ToBOOL(XG(this.wifip+"/ssidhidden"))== true) 	OBJ("ssid_invisible"+str_Aband).checked = true;
		else 													OBJ("ssid_visible"+str_Aband).checked = true;

		this.OnChangeWLMode(str_Aband);	//move from last sequence, bc. need to create security list
		
		///////////////// initial WEP /////////////////
		var auth = XG(this.wifip+"/authtype");
		var len = (XG(this.wifip+"/nwkey/wep/size")=="")? "64" : XG(this.wifip+"/nwkey/wep/size");
		var defkey = (XG(this.wifip+"/nwkey/wep/defkey")=="")? "1" : XG(this.wifip+"/nwkey/wep/defkey");
		this.wps = COMM_ToBOOL(XG(this.wifip+"/wps/enable"));
		var wepauth = (auth=="SHARED") ? "SHARED" : "WEPAUTO";
		
		COMM_SetSelectValue(OBJ("auth_type"+str_Aband),	wepauth);
		COMM_SetSelectValue(OBJ("wep_key_len"+str_Aband),	len);
		COMM_SetSelectValue(OBJ("wep_def_key"+str_Aband),	defkey);
		for (var i=1; i<5; i++)
			OBJ("wep_"+len+"_"+i+str_Aband).value = XG(this.wifip+"/nwkey/wep/key:"+i);
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
		
		if (cipher=="WEP")
			sec_type = "wep";
					
		COMM_SetSelectValue(OBJ("security_type"+str_Aband), sec_type);
		COMM_SetSelectValue(OBJ("wpa_mode"+str_Aband), wpa_mode);
		COMM_SetSelectValue(OBJ("cipher_type"+str_Aband), cipher);
		
		//BODY.ShowAlert("auth="+auth+",sec_type="+sec_type+",cipher_type="+cipher+",wpa_mode="+wpa_mode);
		
		if(str_Aband == "")	this.sec_type 		= sec_type;
		else 				this.sec_type_Aband = sec_type;
		
		OBJ("wpa_psk_key"+str_Aband).value		= XG(this.wifip+"/nwkey/psk/key");
		OBJ("wpa_grp_key_intrv"+str_Aband).value 	= (XG(this.wifip+"/nwkey/wpa/groupintv")=="")? "3600" : XG(this.wifip+"/nwkey/wpa/groupintv");
		
		OBJ("radius_srv_ip"+str_Aband).value	= XG(this.wifip+"/nwkey/eap/radius");
		OBJ("radius_srv_port"+str_Aband).value	= (XG(this.wifip+"/nwkey/eap/port")==""?"1812":XG(this.wifip+"/nwkey/eap/port"));
		OBJ("radius_srv_sec"+str_Aband).value	= XG(this.wifip+"/nwkey/eap/secret");
		
		OBJ("radius_srv_ip_second"+str_Aband).value		= XG(this.wifip+"/nwkey/eap:2/radius");
		OBJ("radius_srv_port_second"+str_Aband).value	= (XG(this.wifip+"/nwkey/eap:2/port")==""?"1812":XG(this.wifip+"/nwkey/eap:2/port"));
		OBJ("radius_srv_sec_second"+str_Aband).value	= XG(this.wifip+"/nwkey/eap:2/secret");
		
		/* modify the sequence of this function is to make sure the default value of channel bandwith is right. */
		this.OnChangeCipherType(str_Aband);
		this.OnChangeSecurityType(str_Aband);
		this.OnChangeWEPKey(str_Aband);
		
		this.OnClickEnWLAN(str_Aband);
		this.OnClickEnAutoChannel(str_Aband);
		this.OnChangeChannel(str_Aband);

	
		return true;
	},
	
	SetWps: function(string)
	{
		var phyinf 		= GPBT(this.wifi_module, "phyinf", "uid","BAND24G-1.1", false);
		var wifip 		= GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);
		
		if(this.dual_band)
		{
			var phyinf2 	= GPBT(this.wifi_module, "phyinf", "uid","BAND5G-1.1", false);
			var wifip2 		= GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf2+"/wifi"), false);	
		}
		
		if(string=="enable")
		{
			XS(wifip+"/wps/enable", "1");
			if(this.dual_band) XS(wifip2+"/wps/enable", "1");
		}
		else
		{
			XS(wifip+"/wps/enable", "0");
			if(this.dual_band) XS(wifip2+"/wps/enable", "0");			
		}
	},
	
	WPS_CONFIGURED_CHK: function(wlan_uid,wlan2_uid)
	{
		var wifi_module = this.wifi_module;
		var phyinf 		= GPBT(wifi_module,"phyinf","uid",wlan_uid,false);
		var wifip 		= GPBT(wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);			
		var phyinf2 	= GPBT(wifi_module,"phyinf","uid",wlan2_uid,false);
		var wifip2 		= GPBT(wifi_module+"/wifi", "entry", "uid", XG(phyinf2+"/wifi"), false);			

		var wps_configured  = XG(wifip+"/wps/configured");		
		var wps_configured2  = XG(wifip2+"/wps/configured");		
		if(wps_configured!=wps_configured2)
		{
			XS(wifip+"/wps/configured", "1");
			XS(wifip2+"/wps/configured", "1");
		}
	},
	
	WPSCHK: function(wlan_uid)
	{
		var wifi_module = this.wifi_module;
		var phyinf 		= GPBT(wifi_module,"phyinf","uid",wlan_uid,false);
		var freq 		= XG(phyinf+"/media/freq");
		var wifip 		= GPBT(wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);			
		
		if(freq == "5")	str_Aband = "_Aband";
		else			str_Aband = "";
			
		if (COMM_EqBOOL(OBJ("ssid"+str_Aband).getAttribute("modified"),true) ||
		COMM_EqBOOL(OBJ("security_type"+str_Aband).getAttribute("modified"),true) ||
		COMM_EqBOOL(OBJ("cipher_type"+str_Aband).getAttribute("modified"),true) ||
		COMM_EqBOOL(OBJ("wpa_psk_key"+str_Aband).getAttribute("modified"),true) ||
		COMM_EqBOOL(OBJ("wep_def_key"+str_Aband).getAttribute("modified"),true))
		{
			XS(wifip+"/wps/configured", "1");
		}
		
		//check authtype, if radius server is used, then wps must be disabled.
		var wps_enable = COMM_ToBOOL(XG(wifip+"/wps/enable"));
		
		if(wps_enable)
		{
			if(OBJ("security_type"+str_Aband).value=="wpa_enterprise")
			{
				if(confirm('<?echo I18N("j", "To use WPA-enterprise security, WPS must be disabled. Proceed ? ");?>'))
					//XS(wifip+"/wps/enable", "0");
					this.SetWps("disable");
				else 
					return false;
			}
			else if(OBJ("security_type"+str_Aband).value=="wep")
			{
				if(confirm('<?echo I18N("j", "To use WEP security, WPS must be disabled. Proceed ? ");?>'))
					//XS(wifip+"/wps/enable", "0");
					this.SetWps("disable");
				else 
					return false;
			}
			
			if(OBJ("security_type"+str_Aband).value=="wpa_personal")
			{
				if(OBJ("cipher_type"+str_Aband).value == "TKIP" || OBJ("wpa_mode"+str_Aband).value == "WPA")
				{
					if(confirm('<?echo I18N("j", "To use WPA only or TKIP only security, WPS must be disabled. Proceed ? ");?>'))
						this.SetWps("disable");
					else
						return false;
				}
			}
		
			if(OBJ("ssid_invisible"+str_Aband).checked)
			{
				if(confirm('<?echo I18N("j", "To use hidden SSID (invisible), WPS must be disabled. Proceed ? ");?>'))
					//XS(wifip+"/wps/enable", "0");
					this.SetWps("disable");
				else 
					return false;
			}
		}
		
		//for pass WPS 2.0 test, we add warning when security is disabled. 
		var wifi_enabled = OBJ("en_wifi"+str_Aband).checked;
		if(wifi_enabled && OBJ("security_type"+str_Aband).value=="")
		{
			if(str_Aband=="")
			{
				if(!confirm('<?echo I18N("j", "Warning ! By selecting None in Security Mode will make your 2.4GHz wifi connection vulnerable. Proceed ? ");?>'))
					return false;
			}
			else
			{
				if(!confirm('<?echo I18N("j", "Warning ! By selecting None in Security Mode will make your 5GHz wifi connection vulnerable. Proceed ? ");?>'))
					return false;	
			}
		}
		return true;
	},
	
	SaveXML: function(wlan_uid)
	{
		var wifi_module 	= this.wifi_module;
		var phyinf 			= GPBT(wifi_module,"phyinf","uid",wlan_uid,false);
		var wifi_profile 	= XG(phyinf+"/wifi");
		var wifip 			= GPBT(wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		var freq 			= XG(phyinf+"/media/freq");

		if(freq == "5")		str_Aband = "_Aband";
		else				str_Aband = "";
		
		if (OBJ("en_wifi"+str_Aband).checked)
			XS(phyinf+"/active", "1");
		else
		{
			XS(phyinf+"/active", "0");
			return true;
		}

		if(this.feature_nosch==0)
			XS(phyinf+"/schedule",	OBJ("sch"+str_Aband).value);
		
		XS(wifip+"/ssid",		OBJ("ssid"+str_Aband).value);

		if (OBJ("auto_ch"+str_Aband).checked)
			XS(phyinf+"/media/channel", "0");
		else		
			XS(phyinf+"/media/channel", OBJ("channel"+str_Aband).value);			
		
		if (OBJ("txrate"+str_Aband).value=="-1")
		{
			XS(phyinf+"/media/dot11n/mcs/auto", "1");
			XS(phyinf+"/media/dot11n/mcs/index", "");
		}
		else
		{
			XS(phyinf+"/media/dot11n/mcs/auto", "0");
			XS(phyinf+"/media/dot11n/mcs/index", OBJ("txrate"+str_Aband).value);
		}
		XS(phyinf+"/media/wlmode",		OBJ("wlan_mode"+str_Aband).value);
		if (OBJ("wlan_mode"+str_Aband).value=="n" || OBJ("wlan_mode"+str_Aband).value=="gn" || OBJ("wlan_mode"+str_Aband).value=="bgn" || OBJ("wlan_mode"+str_Aband).value=="ac_only" || OBJ("wlan_mode"+str_Aband).value=="an" || OBJ("wlan_mode"+str_Aband).value=="acn" || OBJ("wlan_mode"+str_Aband).value=="ac")
		{
			XS(phyinf+"/media/dot11n/bandwidth",		OBJ("bw"+str_Aband).value);
			this.bandWidth = OBJ("bw"+str_Aband).value;
		}
		XS(phyinf+"/media/wmm/enable",	SetBNode(OBJ("en_wmm"+str_Aband).checked));
		XS(wifip+"/ssidhidden",			SetBNode(OBJ("ssid_invisible"+str_Aband).checked));
		
		if (OBJ("security_type"+str_Aband).value=="wep")
		{
			if (OBJ("auth_type"+str_Aband).value=="SHARED")
				XS(wifip+"/authtype", "SHARED");
			else
				XS(wifip+"/authtype", "WEPAUTO");
			XS(wifip+"/encrtype",			"WEP");
			XS(wifip+"/nwkey/wep/size",	"");
			XS(wifip+"/nwkey/wep/ascii",	"");
			XS(wifip+"/nwkey/wep/defkey",	OBJ("wep_def_key"+str_Aband).value);
			for (var i=1, len=OBJ("wep_key_len"+str_Aband).value; i<5; i++)
			{
				if (i==OBJ("wep_def_key"+str_Aband).value)
					XS(wifip+"/nwkey/wep/key:"+i, OBJ("wep_"+len+"_"+i+str_Aband).value);
				else
					XS(wifip+"/nwkey/wep/key:"+i, "");
			}
		}
		else if (OBJ("security_type"+str_Aband).value=="wpa_personal")
		{
			XS(wifip+"/authtype",				OBJ("wpa_mode"+str_Aband).value+"PSK");
			XS(wifip+"/encrtype", 				OBJ("cipher_type"+str_Aband).value);
			
			XS(wifip+"/nwkey/psk/passphrase",	"");
			XS(wifip+"/nwkey/psk/key",			OBJ("wpa_psk_key"+str_Aband).value);
			XS(wifip+"/nwkey/wpa/groupintv",	OBJ("wpa_grp_key_intrv"+str_Aband).value);
		}
		else if (OBJ("security_type"+str_Aband).value=="wpa_enterprise")
		{
			XS(wifip+"/authtype",				OBJ("wpa_mode"+str_Aband).value);
			XS(wifip+"/encrtype", 				OBJ("cipher_type"+str_Aband).value);
			XS(wifip+"/nwkey/wpa/groupintv",	OBJ("wpa_grp_key_intrv"+str_Aband).value);
			
			XS(wifip+"/nwkey/eap/radius",		OBJ("radius_srv_ip"+str_Aband).value);
			XS(wifip+"/nwkey/eap/port",			OBJ("radius_srv_port"+str_Aband).value);
			XS(wifip+"/nwkey/eap/secret",		OBJ("radius_srv_sec"+str_Aband).value);
			
			XS(wifip+"/nwkey/eap:2/radius",		OBJ("radius_srv_ip_second"+str_Aband).value);
			XS(wifip+"/nwkey/eap:2/port",		OBJ("radius_srv_port_second"+str_Aband).value);
			XS(wifip+"/nwkey/eap:2/secret",		OBJ("radius_srv_sec_second"+str_Aband).value);
		}
		else
		{
			XS(wifip+"/authtype", "OPEN");
			XS(wifip+"/encrtype", "NONE");
		}
		//BODY.ShowAlert("SAVE --> sec="+OBJ("security_type"+str_Aband).value+",auth_type="+XG(this.wifip+"/authtype")+",wpa_mode="+OBJ("wpa_mode"+str_Aband).value+",encrtype="+XG(this.wifip+"/encrtype"));
		return true;
	},
	
	OnClickEnWLAN: function(str_Aband)
	{
		if (AUTH.AuthorizedGroup >= 100) return;
		if (OBJ("en_wifi"+str_Aband).checked)
		{
			if(this.feature_nosch==0)
			{
				OBJ("sch"+str_Aband).disabled		= false;
				OBJ("go2sch"+str_Aband).disabled	= false;
			}
			
			OBJ("ssid"+str_Aband).disabled	= false;
			OBJ("auto_ch"+str_Aband).disabled	= false;
			if (!OBJ("auto_ch"+str_Aband).checked) OBJ("channel"+str_Aband).disabled = false;
			OBJ("txrate"+str_Aband).disabled	= false;
			OBJ("wlan_mode"+str_Aband).disabled	= false;
			if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "an" || OBJ("wlan_mode"+str_Aband).value == "acn" || OBJ("wlan_mode"+str_Aband).value == "ac")
			{
				OBJ("bw"+str_Aband).disabled	= false;
				OBJ("en_wmm"+str_Aband).disabled = true;
			}
			else
				OBJ("en_wmm"+str_Aband).disabled = false;
			OBJ("ssid_visible"+str_Aband).disabled = false;
			OBJ("ssid_invisible"+str_Aband).disabled = false;
			
			OBJ("security_type"+str_Aband).disabled= false;
			
			if(str_Aband == "") COMM_SetSelectValue(OBJ("security_type"+str_Aband), this.sec_type);
			else				COMM_SetSelectValue(OBJ("security_type"+str_Aband), this.sec_type_Aband);
		}
		else
		{
			if(this.feature_nosch==0)
			{
				OBJ("sch"+str_Aband).disabled		= true;
				OBJ("go2sch"+str_Aband).disabled	= true;
			}
			
			OBJ("ssid"+str_Aband).disabled	= true;
			OBJ("auto_ch"+str_Aband).disabled	= true;
			OBJ("channel"+str_Aband).disabled	= true;
			OBJ("txrate"+str_Aband).disabled	= true;
			OBJ("wlan_mode"+str_Aband).disabled	= true;
			//OBJ("bw"+str_Aband).disabled	= true;
			OBJ("en_wmm"+str_Aband).disabled = true;
			OBJ("ssid_visible"+str_Aband).disabled = true;
			OBJ("ssid_invisible"+str_Aband).disabled = true;
			
			OBJ("security_type"+str_Aband).disabled= true;
			
			if(str_Aband == "") this.sec_type 		= OBJ("security_type"+str_Aband).value;
			else 				this.sec_type_Aband = OBJ("security_type"+str_Aband).value;

			COMM_SetSelectValue(OBJ("security_type"+str_Aband), "");
		}
		this.OnChangeSecurityType(str_Aband);
	},
	OnChangeChannel: function(str_Aband)
	{
		<?
		$country = query("runtime/devdata/countrycode");
		echo 'var country = "'.$country.'";';
		?>
		if (!OBJ("auto_ch"+str_Aband).checked && ( OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "an" || OBJ("wlan_mode"+str_Aband).value == "acn" || OBJ("wlan_mode"+str_Aband).value == "ac") )
		{
			if(OBJ("channel"+str_Aband).value=="140"|OBJ("channel"+str_Aband).value=="165" |  OBJ("channel"+str_Aband).value=="12"|OBJ("channel"+str_Aband).value=="13")
			{
				OBJ("bw"+str_Aband).value="20";
				OBJ("bw"+str_Aband).disabled = true;
			}
			else if(OBJ("channel"+str_Aband).value=="132" || OBJ("channel"+str_Aband).value=="136")
			{
				OBJ("bw"+str_Aband).disabled	= false;
				COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80");
			}
			else if(OBJ("channel"+str_Aband).value=="56" && country =="TW" )
			{
				OBJ("bw"+str_Aband).value="20";
				COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40");
				COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80");
				OBJ("bw"+str_Aband).disabled = true;
			}
			else if((OBJ("channel"+str_Aband).value=="60" || OBJ("channel"+str_Aband).value=="64") && country =="TW")
			{
				OBJ("bw"+str_Aband).disabled	= false;
				COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80");
				COMM_AddSelectOptionIfNoExist(OBJ("bw"+str_Aband), "20+40", "20/40 MHz(<?echo i18n("Auto");?>)");
			}
			else
			{
				OBJ("bw"+str_Aband).disabled = false;
				if(OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || OBJ("wlan_mode"+str_Aband).value == "ac")
				COMM_AddSelectOptionIfNoExist(OBJ("bw"+str_Aband), "20+40", "20/40 MHz(<?echo i18n("Auto");?>)");
				if(str_Aband =="_Aband" && OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || OBJ("wlan_mode"+str_Aband).value == "ac")
				COMM_AddSelectOptionIfNoExist(OBJ("bw"+str_Aband), "20+40+80", "20/40/80 MHz(<?echo i18n("Auto");?>)");
			}	
		}

	},
	OnClickEnAutoChannel: function(str_Aband)
	{
		if (OBJ("auto_ch"+str_Aband).checked || !OBJ("en_wifi"+str_Aband).checked)
			OBJ("channel"+str_Aband).disabled = true;
		else
			OBJ("channel"+str_Aband).disabled = false;
	},
	OnChangeSecurityType: function(str_Aband)
	{
		switch (OBJ("security_type"+str_Aband).value)
		{
			case "":
				if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || 
					  OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "an" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac")
				{
					OBJ("bw"+str_Aband).disabled = false;
				}
				else OBJ("bw"+str_Aband).disabled = true;
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "none";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";
				OBJ("pad").style.display = "block";
				break;
			case "wep":
				OBJ("bw"+str_Aband).disabled = true;
				OBJ("wep"+str_Aband).style.display = "block";
				OBJ("box_wpa"+str_Aband).style.display = "none";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";				
				OBJ("pad").style.display = "none";
				break;
			case "wpa_personal":
				if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || 
					  OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "an" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac")
				{
					this.OnChangeCipherType(str_Aband);
				}
				else OBJ("bw"+str_Aband).disabled = true;
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "block";
				OBJ("box_wpa_personal"+str_Aband).style.display = "block";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";
				OBJ("pad").style.display = "none";
				break;
			case "wpa_enterprise":
				if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || 
					  OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "an" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || 
					  OBJ("wlan_mode"+str_Aband).value == "ac")
				{
					this.OnChangeCipherType(str_Aband);
				}
				else OBJ("bw"+str_Aband).disabled = true;
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "block";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "block";
				OBJ("pad").style.display = "none";
				break;
		}
		if (!OBJ("en_wifi"+str_Aband).checked) OBJ("bw"+str_Aband).disabled = true;
	},
	OnChangeWPAMode: function(str_Aband)
	{
		switch (OBJ("wpa_mode"+str_Aband).value)
		{
			case "WPA":
				if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn")
				{
					OBJ("cipher_type"+str_Aband).value = "AES";
				}
				else
				{
					OBJ("bw"+str_Aband).disabled = true;
					OBJ("cipher_type"+str_Aband).value = "TKIP";
				}
				break;
			case "WPA2":
				OBJ("bw"+str_Aband).disabled = false;
				OBJ("cipher_type"+str_Aband).value = "AES";
				break;	
			default :
				OBJ("bw"+str_Aband).disabled = false;
				if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn")
					OBJ("cipher_type"+str_Aband).value = "AES";
				else
					OBJ("cipher_type"+str_Aband).value = "TKIP+AES";
		}
	},
	OnChangeCipherType: function(str_Aband)
	{
		switch (OBJ("cipher_type"+str_Aband).value)
		{
			case "TKIP":
				OBJ("bw"+str_Aband).disabled = true;
				break;
			case "AES":
				OBJ("bw"+str_Aband).disabled = false;
				break;
			case "TKIP+AES":
				OBJ("bw"+str_Aband).disabled = false;
				break;
		}
	},
	OnChangeWEPAuth: function(str_Aband)
	{
		if(OBJ("auth_type"+str_Aband).value == "SHARED" && this.wps==true)
		{
			BODY.ShowAlert("<?echo I18N("j","Can't choose shared key when wps is enable !!");?>");
			OBJ("auth_type"+str_Aband).value = "WEPAUTO";
		}
	},
	OnChangeWEPKey: function(str_Aband)
	{
		var no = S2I(OBJ("wep_def_key"+str_Aband).value) - 1;
		
		switch (OBJ("wep_key_len"+str_Aband).value)
		{
			case "64":
				OBJ("wep_64"+str_Aband).style.display = "block";
				OBJ("wep_128"+str_Aband).style.display = "none";
				SetDisplayStyle(null, "wepkey_64"+str_Aband, "none");
				document.getElementsByName("wepkey_64"+str_Aband)[no].style.display = "inline";
				break;
			case "128":
				OBJ("wep_64"+str_Aband).style.display = "none";
				OBJ("wep_128"+str_Aband).style.display = "block";
				SetDisplayStyle(null, "wepkey_128"+str_Aband, "none");
				document.getElementsByName("wepkey_128"+str_Aband)[no].style.display = "inline";
		}
	},
	OnChangeWLMode: function(str_Aband)
	{	
		<?
		$country = query("runtime/devdata/countrycode");
		echo 'var country = "'.$country.'";';
		?>
		var phywlan = "";
		if(str_Aband==="")	phywlan = GPBT(this.wifi_module, "phyinf", "uid","BAND24G-1.1", false);
		else				phywlan = GPBT(this.wifi_module, "phyinf", "uid","BAND5G-1.1", false);
		if (OBJ("wlan_mode"+str_Aband).value == "n" || OBJ("wlan_mode"+str_Aband).value == "gn" || OBJ("wlan_mode"+str_Aband).value == "bgn" || OBJ("wlan_mode"+str_Aband).value == "an")
		{
			this.bandWidth	= XG(phywlan+"/media/dot11n/bandwidth");
			if (this.bandWidth === "20+40+80")
				COMM_SetSelectValue(OBJ("bw"+str_Aband), "20+40");
			else
			COMM_SetSelectValue(OBJ("bw"+str_Aband), this.bandWidth);
			OBJ("bw"+str_Aband).disabled	= false;
			OBJ("en_wmm"+str_Aband).checked = true;
			OBJ("en_wmm"+str_Aband).disabled = true;
			COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80")
	
		}
		else if(OBJ("wlan_mode"+str_Aband).value == "ac_only" || OBJ("wlan_mode"+str_Aband).value == "acn" || OBJ("wlan_mode"+str_Aband).value == "ac")
		{
			this.bandWidth	= XG(phywlan+"/media/dot11n/bandwidth");
			COMM_AddSelectOptionIfNoExist(OBJ("bw"+str_Aband), "20+40+80", "20/40/80 MHz(<?echo i18n("Auto");?>)");
			COMM_SetSelectValue(OBJ("bw"+str_Aband), this.bandWidth);
			OBJ("bw"+str_Aband).disabled	= false;
			if(country =="TW")
			{ //ready for default,hill
				if(OBJ("channel"+str_Aband).value=="56")
				{
					OBJ("bw"+str_Aband).value="20";
					OBJ("bw"+str_Aband).disabled = true;
					COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40");
					COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80");
				}
			
				if(OBJ("channel"+str_Aband).value=="60" || OBJ("channel"+str_Aband).value=="64")
				{
					COMM_AddSelectOptionIfNoExist(OBJ("bw"+str_Aband), "20+40", "20/40 MHz(<?echo i18n("Auto");?>)");
					COMM_RemoveSelectOptionIfExist(OBJ("bw"+str_Aband), "20+40+80");
					COMM_SetSelectValue(OBJ("bw"+str_Aband), this.bandWidth);
				}
			}
		
		}		
		else
		{
			OBJ("bw"+str_Aband).disabled = true;
			OBJ("en_wmm"+str_Aband).disabled = false;
		}
		this.shortGuard = XG(phywlan+"/media/dot11n/guardinterval");
		DrawTxRateList(OBJ("bw"+str_Aband).value, this.shortGuard, str_Aband);
		if (OBJ("wlan_mode"+str_Aband).value === "n")
		{
			var rate = XG(phywlan+"/media/dot11n/mcs/index");
			if (rate=="") rate = "-1";
			COMM_SetSelectValue(OBJ("txrate"+str_Aband), rate);
		}
		DrawSecurityList(OBJ("wlan_mode"+str_Aband).value, str_Aband);
		this.OnChangeSecurityType(str_Aband);
		this.OnChangeChannel(str_Aband);	
	},
	OnClickRadiusAdvanced: function(str_Aband)
    {
        if (this.radius_adv_flag) {
            OBJ("div_second_radius"+str_Aband).style.display = "none";
            OBJ("radius_adv"+str_Aband).value = "Advanced >>";
            this.radius_adv_flag = false;
        }
        else {
            OBJ("div_second_radius"+str_Aband).style.display = "block";
            OBJ("radius_adv"+str_Aband).value = "<< Advanced";
            this.radius_adv_flag = true;
		}
    },
    /*
    For ssid, WEP key, WPA key, we don't allow whitespace in front OR behind !!!
    */
    ValidityCheck: function(wlan_uid)
	{ 
		var wifi_module 	= this.wifi_module;
		var phyinf 			= GPBT(wifi_module,"phyinf","uid",wlan_uid,false);
		var wifi_profile 	= XG(phyinf+"/wifi");
		var wifip 			= GPBT(wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		var freq 			= XG(phyinf+"/media/freq");

		if(freq == "5")		str_Aband = "_Aband";
		else				str_Aband = "";
		
		var obj_ssid 	= OBJ("ssid"+str_Aband).value;
		var obj_wpa_key = OBJ("wpa_psk_key"+str_Aband).value;

		var wep_key		= OBJ("wep_def_key"+str_Aband).value;
		var wep_key_len	= OBJ("wep_key_len"+str_Aband).value;			
		var obj_wep_key = OBJ("wep_"+wep_key_len+"_"+wep_key+str_Aband).value;		
		
		if(obj_ssid.charAt(0)===" "|| obj_ssid.charAt(obj_ssid.length-1)===" ")
		{
			alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
			return false;
		}
		
		if(OBJ("security_type"+str_Aband).value==="wep") //wep_64_1_Aband
		{
			if (obj_wep_key.charAt(0) === " "|| obj_wep_key.charAt(obj_wep_key.length-1)===" ")
			{
				alert("The prefix or postfix of the 'WEP Key' could not be blank.");
				return false;
			}
		}
		else if(OBJ("security_type"+str_Aband).value==="wpa_personal")
		{
			if (obj_wpa_key.charAt(0)===" " || obj_wpa_key.charAt(obj_wpa_key.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'Pre-Shared Key' could not be blank.");?>");
				return false;
			}
		}
		else if(OBJ("security_type"+str_Aband).value==="wpa_enterprise")
		{	
			var radius_key 			= OBJ("radius_srv_sec"+str_Aband).value;
			var radius_key_second	= OBJ("radius_srv_sec_second"+str_Aband).value;
			
			if (radius_key.charAt(0)===" " || radius_key.charAt(radius_key.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'RADIUS server Shared Secret' could not be blank.");?>");
				return false;
			}
			
			if(radius_key_second!=="")
			{
				if (radius_key_second.charAt(0)===" " || radius_key_second.charAt(radius_key_second.length-1)===" ")
				{
					alert("<?echo I18N("h", "The prefix or postfix of the 'Second RADIUS server Shared Secret' could not be blank.");?>");
					return false;
				}
			}
		}
		return true;
	}
}

function SetBNode(value)
{
	if (COMM_ToBOOL(value))
		return "1";
	else
		return "0";
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
	for(i = 0,iarr = 0; i < elem.length; i++)
	{
		att = elem[i].getAttribute("name");
		if(att == name)
		{
			arr[iarr] = elem[i];
			iarr++;
		}
	}
	return arr;
}

function DrawTxRateList(bw, sgi, str_Aband)
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
		listOptions = new Array("0 - 7.2","1 - 14.4","2 - 21.7","3 - 28.9","4 - 43.3","5 - 57.8","6 - 65.0","7 - 72.2"<?
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

	var tr_length = OBJ("txrate"+str_Aband).length;
	for(var idx=1; idx<tr_length; idx++)
	{
		OBJ("txrate"+str_Aband).remove(1);
	}
	if (OBJ("wlan_mode"+str_Aband).value === "n")
	{
		for(var idx=0; idx<listOptions.length; idx++)
		{
			var item = document.createElement("option");
			item.value = idx;
			item.text = listOptions[idx];
			try		{ OBJ("txrate"+str_Aband).add(item, null); }
			catch(e){ OBJ("txrate"+str_Aband).add(item); }
		}
	}
}

function DrawSecurityList(wlan_mode, str_Aband)
{
	var security_list = null;
	var cipher_list = null;
	if (wlan_mode === "n" || wlan_mode === "ac_only" || wlan_mode === "acn")
	{
		security_list = ['wpa_personal', '<?echo i18n("WPA-Personal");?>',
						'wpa_enterprise', '<?echo i18n("WPA-Enterprise");?>'];
		cipher_list = ['AES'];
	}
	else
	{
		security_list = ['wep', '<?echo i18n("WEP");?>',
						 'wpa_personal', '<?echo i18n("WPA-Personal");?>',
						 'wpa_enterprise', '<?echo i18n("WPA-Enterprise");?>'];
		cipher_list = ['TKIP+AES','TKIP','AES'];
	}
	//modify security_type
	var sec_length = OBJ("security_type"+str_Aband).length;
	for(var idx=1; idx<sec_length; idx++)
	{
		OBJ("security_type"+str_Aband).remove(1);
	}
	for(var idx=0; idx<security_list.length; idx++)
	{
		var item = document.createElement("option");
		item.value = security_list[idx++];
		item.text = security_list[idx];
		try		{ OBJ("security_type"+str_Aband).add(item, null); }
		catch(e){ OBJ("security_type"+str_Aband).add(item); }
	}
	// modify cipher_type
	var ci_length = OBJ("cipher_type"+str_Aband).length;
	for(var idx=0; idx<ci_length; idx++)
	{
		OBJ("cipher_type"+str_Aband).remove(0);
	}
	for(var idx=0; idx<cipher_list.length; idx++)
	{
		var item = document.createElement("option");
		item.value = cipher_list[idx];
		if (item.value=="TKIP+AES") item.text = "TKIP and AES";
		else						item.text = cipher_list[idx];
		try		{ OBJ("cipher_type"+str_Aband).add(item, null); }
		catch(e){ OBJ("cipher_type"+str_Aband).add(item); }
	}
}
</script>
