<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF, PHYINF.WIFI",
	
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		if (!this.Initial("BAND24G-1.1", "WIFI.PHYINF")) return false;	
		if(this.dual_band) 
			if(!this.Initial("BAND5G-1.1", "WIFI.PHYINF")) return false;	
		return true;
	},
	PreSubmit: function()
	{
		if (!this.SaveXML("BAND24G-1.1", "WIFI.PHYINF")) return null;		
		if(this.dual_band) 
			if(!this.SaveXML("BAND5G-1.1", "WIFI.PHYINF")) return null;		
		return PXML.doc;
	},
	IsDirty: null,
	str_Aband: null,
	
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	phyinf: null,
	wifi_module: null,
	dual_band: COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>'),
	Initial: function(wlan_phyinf,wifi_phyinf)
	{
		this.wifi_module = PXML.FindModule(wifi_phyinf);
		if (!this.wifi_module)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		
		this.phyinf 	= GPBT(this.wifi_module, "phyinf", "uid", wlan_phyinf, false);
		var freq 		= XG(this.phyinf+"/media/freq");
		var wlanmode 	= XG(this.phyinf+"/media/wlmode");
		if(freq == "5")
			str_Aband = "_Aband";
		else
			str_Aband = "";
		
		COMM_SetSelectValue(OBJ("tx_power"+str_Aband), XG(this.phyinf+"/media/txpower"));
		//COMM_SetSelectValue(OBJ("wlan_mode"+str_Aband), );
		//COMM_SetSelectValue(OBJ("bw"+str_Aband), XG(this.phyinf+"/media/dot11n/bandwidth"));

		SetRadioValue("preamble"+str_Aband, XG(this.phyinf+"/media/preamble"));
		
		OBJ("beacon"+str_Aband).value	= XG(this.phyinf+"/media/beacon");
		OBJ("rts"+str_Aband).value	= XG(this.phyinf+"/media/rtsthresh");
		OBJ("frag"+str_Aband).value	= XG(this.phyinf+"/media/fragthresh");
		OBJ("dtim"+str_Aband).value	= XG(this.phyinf+"/media/dtim");
		OBJ("sgi"+str_Aband).checked	= COMM_EqNUMBER(XG(this.phyinf+"/media/dot11n/guardinterval"), 400);
		if (/n/.test(XG(this.phyinf+"/media/wlmode")))	OBJ("sgi"+str_Aband).disabled	= false;
		else											OBJ("sgi"+str_Aband).disabled	= true;
			
		OBJ("en_wmm"+str_Aband).checked = COMM_ToBOOL(XG(this.phyinf+"/media/wmm/enable"));			
		if (/n/.test(wlanmode))
		{
			OBJ("en_wmm"+str_Aband).disabled = true;
		}
		else
		{
			if( (str_Aband != "") && (/ac/.test(wlanmode)) )//ac mode For 5G
			{
				OBJ("en_wmm"+str_Aband).disabled = true;
			}
			else
			{
				OBJ("en_wmm"+str_Aband).disabled = false;
			}
		}
			
		if(COMM_ToBOOL(XG(this.phyinf+"/media/dot11n/bw2040coexist"))== true) 	OBJ("coexist_enable"+str_Aband).checked = true;
		else 																	OBJ("coexist_disable"+str_Aband).checked = true;			
		
		var bandWidth	= XG(this.phyinf+"/media/dot11n/bandwidth");
		if(bandWidth == "20")
		{
    		OBJ("coexist_enable"+str_Aband).disabled = true;
			OBJ("coexist_disable"+str_Aband).disabled = true;
    	}
    	else
    	{
			OBJ("coexist_enable"+str_Aband).disabled = false;
			OBJ("coexist_disable"+str_Aband).disabled = false;
    	}
		var wifi_profile 	= XG(this.phyinf+"/wifi");
		this.wifip 			= GPBT(this.wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		OBJ("wlan_partition"+str_Aband).checked = COMM_ToBOOL(XG(this.wifip+"/acl/isolation"));
	
		return true;
	},
	SaveXML: function(wlan_phyinf , wifi_phyinf)
	{
		this.wifi_module = PXML.FindModule(wifi_phyinf);
		if (!this.wifi_module)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		
		this.phyinf = GPBT(this.wifi_module, "phyinf", "uid", wlan_phyinf, false);
		var freq = XG(this.phyinf+"/media/freq");
		if(freq == "5")
			str_Aband = "_Aband";
		else
			str_Aband = "";

		this.phyinf += "/media";
		if (!TEMP_IsDigit(OBJ("beacon"+str_Aband).value))
		{
			BODY.ShowAlert("<?echo i18n("The input beacon interval is invalid.");?>");
			OBJ("beacon"+str_Aband).focus();
			return null;
		}
		else if (!TEMP_IsDigit(OBJ("rts"+str_Aband).value))
		{
			BODY.ShowAlert("<?echo i18n("The input RTS threshold is invalid.");?>");
			OBJ("rts"+str_Aband).focus();
			return null;
		}
		else if (!TEMP_IsDigit(OBJ("frag"+str_Aband).value))
		{
			BODY.ShowAlert("<?echo i18n("The input fragmentation is invalid.");?>");
			OBJ("frag"+str_Aband).focus();
			return null;
		}
		else if (!TEMP_IsDigit(OBJ("dtim"+str_Aband).value))
		{
			BODY.ShowAlert("<?echo i18n("The input DTIM interval is invalid.");?>");
			OBJ("dtim"+str_Aband).focus();
			return null;
		}
		XS(this.phyinf+"/txpower",		OBJ("tx_power"+str_Aband).value);
		XS(this.phyinf+"/beacon",		OBJ("beacon"+str_Aband).value);
		XS(this.phyinf+"/rtsthresh",	OBJ("rts"+str_Aband).value);
		XS(this.phyinf+"/fragthresh",	OBJ("frag"+str_Aband).value);
		XS(this.phyinf+"/dtim",			OBJ("dtim"+str_Aband).value);
		
		XS(this.phyinf+"/preamble",		GetRadioValue("preamble"+str_Aband));
//		XS(this.phyinf+"/ctsmode",		GetRadioValue("cts"+str_Aband));
//		XS(this.phyinf+"/wlmode",		OBJ("wlan_mode"+str_Aband).value);
		XS(this.phyinf+"/wmm/enable",	SetBNode(OBJ("en_wmm"+str_Aband).checked));

		var host_phyinf = GPBT(this.wifi_module, "phyinf", "uid",wlan_phyinf, false);
		var host_profile 	= XG(host_phyinf+"/wifi");
		var host_wifip 		= GPBT(this.wifi_module+"/wifi", "entry", "uid", host_profile, false);
		XS(host_wifip+"/acl/isolation",	SetBNode(OBJ("wlan_partition"+str_Aband).checked));

		if (!COMM_ToBOOL('<?=$FEATURE_NOGUESTZONE?>'))
		{
			//for guestzone wlan partition, we follow host
			if(wlan_phyinf=="BAND24G-1.1") 	wlan_phyinf="BAND24G-1.2";
			else 							wlan_phyinf="BAND5G-1.2";
			var guest_phyinf 	= GPBT(this.wifi_module, "phyinf", "uid",wlan_phyinf, false);
			var guest_profile 	= XG(guest_phyinf+"/wifi");
			var guest_wifip 	= GPBT(this.wifi_module+"/wifi", "entry", "uid", guest_profile, false);	
			XS(guest_wifip+"/acl/isolation",	SetBNode(OBJ("wlan_partition"+str_Aband).checked));
		}
		XS(this.phyinf+"/dot11n/guardinterval",	OBJ("sgi"+str_Aband).checked==true ? "400" : "800" );
		XS(this.phyinf+"/dot11n/bw2040coexist",		SetBNode(OBJ("coexist_enable"+str_Aband).checked));
		
		return true;
	},
	GetIP: function(mac)
	{
		var path = PXML.doc.GetPathByTarget(this.inf, "entry", "mac", mac.toLowerCase(), false);
		return XG(path+"/ipaddr");
	}
}

function SetBNode(value)
{
	if (COMM_ToBOOL(value))
		return "1";
	else
		return "0";
}


function GetRadioValue(name)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].checked)	return obj[i].value;
	}
}
function SetRadioValue(name, value)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].value==value)
		{
			obj[i].checked = true;
			break;
		}
	}
}
</script>
