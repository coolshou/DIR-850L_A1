<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF",
	OnLoad: function()
	{
		OBJ("auto").checked = true;
		OBJ("pin").checked = true;
		this.ShowCurrentStage();
	},
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
		case "OK":
			this.WPSInProgress();
			break;
		default:
			BODY.ShowMessage("",result);
			break;
		}
		return true;
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

		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		this.wifi_phyinf = PXML.FindModule("WIFI.PHYINF");
		if (!this.wifi_phyinf)
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","Initial() ERROR!!!");
			return false;
		}
		if (!this.Initial("BAND24G-1.1", this.wifi_phyinf)) return false;

		if (this.dual_band)
		{
			OBJ("div_5G_manual").style.display = "block";
			if (!this.Initial("BAND5G-1.1", this.wifi_phyinf)) return false;
		}
		else
			OBJ("div_5G_manual").style.display = "none";

		return true;
	},
	PreSubmit: null,
	//////////////////////////////////////////////////
	m_prefix: "<?echo I18N("j","Adding wireless device").": ";?>",
	m_success: "<?echo I18N("j","Succeeded").". ".I18N("j","To add another device click on the Cancel button below or click on the Wireless Status button to check wireless status.");?>",
	m_timeout: "<?echo I18N("j","Session Time-Out").".";?>",
	wifip: null,
	wpsp: null,
	statep: null,
	en_wps: false,
	method: null,
	start_count_down: false,
	wps_timer: null,
	phyinf: null,
	dual_band: 0,
	wifi_phyinf: null,
	stages: new Array ("wiz_stage_1", "wiz_stage_2_auto", "wiz_stage_2_manual", "wiz_stage_2_msg"),
	currentStage: 0,	// 0 ~ this.stages.length
	Initial: function(wlan_phyinf)
	{
		this.phyinf = GPBT(this.wifi_phyinf, "phyinf", "uid", wlan_phyinf, false);
		var uid = XG(this.phyinf+"/wifi");
		this.wifip = GPBT(this.wifi_phyinf+"/wifi", "entry", "uid", uid, false);

		PXML.IgnoreModule("WIFI.PHYINF");

		var freq = XG(this.phyinf+"/media/freq");
		if (freq=="5")	postfix = "_11a";
		else			postfix = "";

		this.en_wps = XG(this.wifip+"/wps/enable")=="1" ? true : false ;
		if (!this.en_wps)
		{
			this.ShowWpsDisabled();
			return true;
		}

		OBJ("ssid"+postfix).innerHTML = XG(this.wifip+"/ssid");
		switch (XG(this.wifip+"/authtype"))
		{
		case "WPA":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WPA-EAP");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_cipher"+postfix).style.display = "";
			OBJ("st_pskkey"+postfix).style.display = "";
			break;
		case "WPA2":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WPA2-EAP");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_cipher"+postfix).style.display = "";
			OBJ("st_pskkey"+postfix).style.display = "";
			break;
		case "WPAPSK":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WPA-PSK");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_cipher"+postfix).style.display = "";
			OBJ("st_pskkey"+postfix).style.display = "";
			break;
		case "WPA2PSK":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WPA2-PSK");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_cipher"+postfix).style.display = "";
			OBJ("st_pskkey"+postfix).style.display = "";
			break;
		case "WPA+2PSK":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","Auto")." (".I18N("j","WPA or WPA2").") - ".I18N("j","Personal");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_cipher"+postfix).style.display = "";
			OBJ("st_pskkey"+postfix).style.display = "";
			break;
		case "WPA+2":
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","Auto")." (".I18N("j","WPA or WPA2").") - ".I18N("j","Enterprise");?>";
			OBJ("cipher"+postfix).innerHTML = CipherTypeParse(XG(this.wifip+"/encrtype"));
			OBJ("st_cipher"+postfix).style.display = "";
			this.en_wps = false;
			DisableWPS();
			break;
		case "SHARED":
			var key_no = XG(this.wifip+"/nwkey/wep/defkey");
			OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WEP")." - ".I18N("j","SHARED");?>";
			OBJ("wepkey"+postfix).innerHTML = key_no + ": " + XG(this.wifip+"/nwkey/wep/key:"+key_no);
			OBJ("pskkey"+postfix).innerHTML = XG(this.wifip+"/nwkey/psk/key");
			OBJ("st_wep"+postfix).style.display = "";
			this.en_wps = false;
			DisableWPS();
			break;
		case "OPEN":
			if (XG(this.wifip+"/encrtype")=="WEP")
			{
				var key_no = XG(this.wifip+"/nwkey/wep/defkey");
				OBJ("security"+postfix).innerHTML = "<?echo I18N("j","WEP")." - ".I18N("j","OPEN");?>";
				OBJ("wepkey"+postfix).innerHTML = key_no + ": " + XG(this.wifip+"/nwkey/wep/key:"+key_no);
				OBJ("st_wep"+postfix).style.display = "";
			}
			else
			{
				OBJ("security"+postfix).innerHTML = "<?echo I18N("j","None");?>";
			}
			break;
		}
		if (XG(this.wifip+"/wps/configured")=="1"&&
			XG(this.wifip+"/wps/locksecurity")!="0")
		{
			OBJ("pin").disabled = true;
			OBJ("pincode").disabled = true;
			OBJ("pbc").checked = true;
		}
		return true;
	},
	ShowCurrentStage: function()
	{
		OBJ("wiz_stage_2").style.display = (this.currentStage==0)? "none":"block";
		for (var i=0; i<this.stages.length; i++)
		{
			if (i==this.currentStage)
				OBJ(this.stages[i]).style.display = "block";
			else
				OBJ(this.stages[i]).style.display = "none";
		}

		OBJ("b_next").style.display = "none";
		OBJ("b_send").style.display = "none";
		OBJ("b_stat").style.display = "none";
		if (this.currentStage==0)
		{
			OBJ("b_exit").disabled = false;
			OBJ("b_next").disabled = false;
			OBJ("b_next").style.display = "";
		}
		else if (this.currentStage==1)
		{
			OBJ("b_exit").disabled = false;
			OBJ("b_next").disabled = false;
			OBJ("b_send").style.display = "";
			if (!this.en_wps)
				OBJ("b_send").disabled = false;
		}
		else if (this.currentStage==2||this.currentStage==3)
		{
			OBJ("b_stat").style.display = "";
		}
	},
	ShowWPSMessage: function(state)
	{
		switch (state)
		{
		case "WPS_NONE":
			OBJ("msg").innerHTML = this.m_prefix + "<?echo I18N("j","Session Time-Out").".";?>";
			OBJ("b_exit").disabled = false;
			OBJ("b_stat").disabled = false;
			break;
		case "WPS_ERROR":
			OBJ("msg").innerHTML = this.m_prefix + "WPS_ERROR.";
			OBJ("b_exit").disabled = false;
			break;
		case "WPS_OVERLAP":
			OBJ("msg").innerHTML = this.m_prefix + "WPS_OVERLAP.";
			OBJ("b_exit").disabled = false;
			break;
		case "WPS_IN_PROGRESS":
			OBJ("b_exit").disabled = true;
			OBJ("b_send").disabled = true;
			OBJ("b_stat").disabled = true;
			break;
		case "WPS_SUCCESS":
			OBJ("msg").innerHTML = this.m_prefix + "<?echo I18N("j","Succeeded").". ".I18N("j","To add another device click on the Cancel button below or click on the Wireless Status button to check wireless status.");?>";
			OBJ("b_exit").disabled = false;
			OBJ("b_stat").disabled = false;
			OBJ("b_send").style.display = "none";
			OBJ("b_stat").style.display = "";
			break;
		}
		this.currentStage = 3;
		this.ShowCurrentStage();
		if (state=="WPS_IN_PROGRESS") return;
		PAGE.start_count_down = false;
		if (this.cd_timer)	clearTimeout(this.cd_timer);
		if (this.wps_timer)	clearTimeout(this.wps_timer);
	},
	OnClickPre: function()
	{
		this.currentStage = 0;
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		if (OBJ("auto").checked)
			this.currentStage = 1;
		else
			this.currentStage = 2;
		this.ShowCurrentStage();
	},
	OnClickCancel: function()
	{
		if (this.currentStage==3||this.currentStage==2)
		{
			self.location.href = "./wiz_wps.php";
			return;
		}
		if (!COMM_IsDirty(false)||confirm("<?echo I18N("j","Do you want to abandon all changes you made to this wizard?");?>"))
			self.location.href = "./adv_wps.php";
	},
	OnSubmit: function()
	{
		var ajaxObj = GetAjaxObj("WPS");
		var action = (OBJ("pin").checked)? "PIN":"PBC";
		var uid = "BAND24G-1.1";
		var value = OBJ("pincode").value;
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.OnSubmitCallback(xml.Get("/wpsreport/result"), xml.Get("/wpsreport/reason"));
		}

		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("wpsacts.php", "action="+action+"&uid="+uid+"&pin="+value);
		AUTH.UpdateTimeout();
	},
	WPSInProgress: function()
	{
		if (!this.start_count_down)
		{
			this.start_count_down = true;
			var str = "";
			if (OBJ("pin").checked)
			{
				str = "<?echo I18N("j","Please start WPS on the wireless device you are adding to your wireless network.");?><br />";
			}
			else
			{
				str = "<?echo I18N("j","Please press down the Push Button (physical or virtual) on the wireless device you are adding to your wireless network.");?><br />";
			}
			str += '<?echo I18N("j","Remain time in second");?>: <span id="ct">120</span><br /><br />';
			str += this.m_prefix + "<?echo I18N("j","Started");?>.";
			OBJ("msg").innerHTML = str;
			this.ShowWPSMessage("WPS_IN_PROGRESS");
			setTimeout('PAGE.WPSCountDown()',1000);
		}

		var ajaxObj = GetAjaxObj("WPS");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.WPSInProgressCallBack(xml);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("wpsstate.php", "dummy=dummy");
	},
	WPSInProgressCallBack: function(xml)
	{
		var cnt = xml.Get("/wpsstate/count");

		for (var i=1; i<=cnt; i++)
		{
			var state=xml.Get("/wpsstate/phyinf:"+i+"/state");
			if (state==="WPS_SUCCESS")
				break;
		}
		if (state=="WPS_IN_PROGRESS" || state=="")
			this.wps_timer = setTimeout('PAGE.WPSInProgress()',2000);
		else
			this.ShowWPSMessage(state);
	},
	WPSCountDown: function()
	{
		var time = parseInt(OBJ("ct").innerHTML, 10);
		if (time > 0)
		{
			time--;
			this.cd_timer = setTimeout('PAGE.WPSCountDown()',1000);
			OBJ("ct").innerHTML = time;
		}
		else
		{
			clearTimeout(this.cd_timer);
			this.ShowWPSMessage("WPS_NONE");
		}
	},
	ShowWpsDisabled: function()
	{
		for (var i=0; i<this.stages.length; i++)
		{
			OBJ(this.stages[i]).style.display = "none";
		}
		OBJ("b_exit").style.display = "none";
		OBJ("b_next").style.display = "none";
		OBJ("wiz_stage_wps_disabled").style.display = "block";
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

function CipherTypeParse(cipher)
{
	switch (cipher)
	{
	case "TKIP+AES":
		return "<?echo I18N("j","TKIP and AES");?>";
	case "TKIP":
		return "<?echo I18N("j","TKIP");?>";
	case "AES":
		return "<?echo I18N("j","AES");?>";
	}
}
function DisableWPS()
{
	OBJ("pin").disabled = true;
	OBJ("pbc").disabled = true;
	OBJ("pincode").disabled = true;
	OBJ("b_send").disabled = true;
}
//]]>
</script>
