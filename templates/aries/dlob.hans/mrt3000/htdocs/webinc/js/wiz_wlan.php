<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF,PHYINF.WIFI",
	OnLoad: function()
	{
		this.ShowCurrentStage();
	},
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
	{
		switch (code)
		{
		case "OK":
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
		if (node.indexOf("ssid")>0)	{ BODY.ShowConfigError("ssid",	msg); return; }
		if (node.indexOf("key")>0)	{ BODY.ShowConfigError("key",	msg); return; }

		this.currentStage = 1;
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

		this.wifi_module = PXML.FindModule("WIFI.PHYINF");
		if (!this.Initial("BAND24G-1.1", '')) return false;
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if (this.dual_band)
		{
			OBJ("div_5G").style.display = "block";
			if (!this.Initial("BAND5G-1.1", "_11a")) return false;
		}
		return true;
	},
	PreSubmit: function(uid_wlan)
	{
		if (!this.ValidityCheck("BAND24G-1.1", '')) return null;
		this.SaveXML("BAND24G-1.1", '');

		if (this.dual_band)
		{
			if (!this.ValidityCheck("BAND5G-1.1", "_11a")) return null;
			this.SaveXML("BAND5G-1.1", "_11a");
		}

		PXML.CheckModule("WIFI.PHYINF", null, null, "ignore");
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	wifi_module: null,
	dual_band: null,
	stages: new Array ("wlan_status", "wlan_config", "wlan_status"),
	currentStage: 0,	// 0 ~ this.stages.length
	auth: null,
	Initial: function(uid_wlan, postfix)
	{
		var phyinf = GPBT(this.wifi_module, "phyinf", "uid", uid_wlan, false);
		var wifip = GPBT(this.wifi_module+"/wifi" ,"entry", "uid" ,XG(phyinf+"/wifi"), false);
		if (!wifip)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.auth = XG(wifip+"/authtype");
		var cipher = XG(wifip+"/encrtype");
		var sec_type = "";

		/* show key of wep or none for fool-proof design */
		OBJ("st_key"+postfix).innerHTML	= COMM_EscapeHTMLSC(XG(wifip+"/nwkey/psk/key"));
		if (this.auth.indexOf("+2")>0)
			sec_type = "<?echo I18N("j","[Auto (WPA or WPA2)]");?>";
		else if (this.auth.indexOf("2")>0)
			sec_type = "<?echo I18N("j","[WPA2]");?>";
		else if (this.auth.indexOf("WPA")>0)
			sec_type = "<?echo I18N("j","[WPA]");?>";
		else if (this.auth=="OPEN")
		{
			OBJ("st_key"+postfix).innerHTML	= "";	
			sec_type = "<?echo I18N("j","[NONE]");?>"; 	
		}	
		else if (this.auth=="WEPAUTO" || auth=="SHARED")
		{
			var defkey = (XG(wifip+"/nwkey/wep/defkey")=="")? "1" : XG(wifip+"/nwkey/wep/defkey");
			OBJ("st_key"+postfix).innerHTML	= COMM_EscapeHTMLSC(XG(wifip+"/nwkey/wep/key:"+defkey));	
			sec_type = "<?echo I18N("j","[WEP]");?>"; 				
		}	
/*
		if (sec_type!="")
		{
			if (this.auth.indexOf("PSK")>0)
				sec_type = "("+ sec_type +" - <?echo I18N("j","Personal");?>)";
			else
				sec_type = "("+ sec_type +" - <?echo I18N("j","Enterprise");?>)";
		}
*/
		OBJ("st_encry"+postfix).innerHTML = sec_type;
		OBJ("st_ssid"+postfix).innerHTML= COMM_EscapeHTMLSC(XG(wifip+"/ssid"));
		OBJ("ssid"+postfix).value		= XG(wifip+"/ssid");
		OBJ("key"+postfix).value		= XG(wifip+"/nwkey/psk/key");
		if (COMM_ToBOOL(XG(phyinf+"/active")))
		{
			OBJ("wlan_disabled").style.display = "none";
			OBJ("wlan_enabled").style.display = "block";
		}
		else
		{
			OBJ("wlan_disabled").style.display = "block";
			OBJ("wlan_enabled").style.display = "none";
		}

		return true;
	},
	SaveXML: function(uid_wlan, postfix)
	{
		var phyinf = GPBT(this.wifi_module, "phyinf", "uid", uid_wlan, false);
		var wifip = GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);
		var freq = XG(phyinf+"/media/freq");

		XS(phyinf+"/active", "1");
		XS(wifip+"/ssid", OBJ("ssid"+postfix).value);
		XS(wifip+"/ssidhidden", "0");
		XS(wifip+"/authtype", "WPA+2PSK");
		XS(wifip+"/encrtype", "TKIP+AES");
		XS(wifip+"/nwkey/psk/passphrase", "");
		XS(wifip+"/nwkey/psk/key", OBJ("key"+postfix).value);
		XS(wifip+"/nwkey/wpa/groupintv", 3600);

		return true;
	},
	/* For ssid, WEP key, WPA key, we don't allow whitespace in front OR behind !!! */
	ValidityCheck: function(wlan_uid, postfix)
	{
		var phyinf	= GPBT(this.wifi_module,"phyinf","uid",wlan_uid,false);
		var uid		= XG(phyinf+"/wifi");
		var wifip	= GPBT(this.wifi_module+"/wifi", "entry", "uid", uid, false);
		var ssid	= OBJ("ssid"+postfix).value;
		var key		= OBJ("key"+postfix).value;
		var iserr	= false;

		if (ssid.charAt(0)===" "|| ssid.charAt(ssid.length-1)===" ")
		{
			BODY.ShowConfigError("ssid"+postfix, "<?echo I18N("j", "The prefix or postfix of the 'Wireless Network Name' cannot be blank.");?>");
			iserr = true;
		}
		if (key.charAt(0)===" "||key.charAt(key.length-1)===" ")
		{
			BODY.ShowConfigError("key"+postfix, "<?echo I18N("j", "The prefix or postfix of the 'Pre-Shared Key' cannot be blank.");?>");
			iserr = true;
		}
		if (iserr)
		{
			this.currentStage = 1;;
			this.ShowCurrentStage();
			return false;
		}
		return true;
	},
	OnClickPre: function()
	{
		this.currentStage--;
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		/* if not dirty, just go to next step. */
		if (this.currentStage==1&&COMM_IsDirty(false))
			BODY.OnSubmit();
		else
		{
			this.currentStage++;
			this.ShowCurrentStage();
		}
	},
	ShowCurrentStage: function()
	{
		OBJ("b_exit").style.display = "none";
		OBJ("b_back").style.display = "none";
		OBJ("b_next").style.display = "none";
		OBJ("b_send").style.display = "none";
		if (this.currentStage==0)
		{
			OBJ("btname").innerHTML = "<?echo I18N("j","Change Settings");?>";
			OBJ("b_exit").style.display = "";
			OBJ("b_next").style.display = "";
		}
		else if (this.currentStage==1)
		{
			OBJ("btname").innerHTML = "<?echo I18N("j","Next");?>";
			OBJ("b_back").style.display = "";
			OBJ("b_next").style.display = "";
			
			/* ensure the Security Mode is WPA+2PSK */
			if(this.auth!="WPA+2PSK")
			{
				OBJ("mainform").setAttribute("modified", "true");			
			}
		}
		else if (this.currentStage==2)
		{
			OBJ("st_key").innerHTML = OBJ("key").value;
			OBJ("st_key_11a").innerHTML = OBJ("key_11a").value;
			OBJ("b_back").style.display = "";
			OBJ("b_send").style.display = "";
		}

		if (this.currentStage==1)
		{
			OBJ("wlan_status").style.display = "none";
			OBJ("wlan_config").style.display = "block";
		}
		else
		{
			OBJ("wlan_status").style.display = "block";
			OBJ("wlan_config").style.display = "none";
		}
	},
	GenKey: function(postfix)
	{
		var c = "0123456789abcdef";
		var str = '';
		for (var i=0; i<8; i++)
		{
			var rand_char = Math.floor(Math.random() * c.length);
			str += c.substring(rand_char, rand_char + 1);
		}
		OBJ("key"+postfix).value = str;
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
