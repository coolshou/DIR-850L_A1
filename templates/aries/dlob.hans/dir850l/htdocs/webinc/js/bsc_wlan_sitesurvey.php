<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "",
	OnLoad: function()	{},
	OnUnload: function() {},
	OnSubmitCallback: function (result, code)
	{
		switch (result)
		{
			case "OK":
				PAGE.SiteSurvey();
				break;
			case "":
				if(this.site_survey_timer)
				{
					clearTimeout(this.site_survey_timer);
					this.site_survey_timer = null;
				}
				if(code == "DOING")
					this.site_survey_timer = setTimeout('PAGE.OnSubmit()',2000);
				break;
			default:
				BODY.ShowAlert(code);
				break;
		}
		return true;
	},
	InitValue: function(xml)
	{		
		PXML.doc = xml;
		OBJ("content").style.backgroundColor="#DFDFDF";
		PAGE.OnSubmit();
		return true;
	},
	PreSubmit: function(){},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	site_survey_timer: null,
	site_survey_cnt: 0,
	site_survey_num: 0,
	SS_ssid: new Array(),
	SS_channel : new Array(),
	SS_encrtype: new Array(),
	SS_authtype: new Array(),
	OnSubmit: function()
	{	
		var ajaxObj = GetAjaxObj("SITESURVEY");
		var action = "SITESURVEY";
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.OnSubmitCallback(xml.Get("/sitesurveyreport/result"), xml.Get("/sitesurveyreport/reason"));
		}
		
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("sitesurvey.php", "action="+action);
		AUTH.UpdateTimeout();			
	},
	SiteSurvey: function()
	{
		COMM_GetCFG(false, "RUNTIME.SITESURVEY", function(xml) {PAGE.SiteSurveyCallback(xml);});	
	},
	SiteSurveyCallback: function(xml)
	{		
		/*Show site survey table*/
		var SiteSurveyPrefix = "/postxml/module/runtime/wifi_tmpnode/sitesurvey/entry";
		this.site_survey_cnt = xml.Get(SiteSurveyPrefix+"#");
		BODY.CleanTable("SiteSurveyTable");
		for (var i=1; i<=this.site_survey_cnt; i++)
		{	
			SiteSurveyPath = SiteSurveyPrefix+":"+i; 
			this.SS_ssid[i] = xml.Get(SiteSurveyPath+"/ssid");
			var SS_bssid = xml.Get(SiteSurveyPath+"/macaddr");
			var channel = xml.Get(SiteSurveyPath+"/channel");
			this.SS_channel[i] = xml.Get(SiteSurveyPath+"/channel");
			var wlmode = xml.Get(SiteSurveyPath+"/wlmode");
			var SS_channel = channel;//+"("+wlmode.substring(2, wlmode.length)+")";
			this.SS_encrtype[i] = xml.Get(SiteSurveyPath+"/encrtype");
			this.SS_authtype[i] = xml.Get(SiteSurveyPath+"/authtype");
			var SS_encrypt;
			if(this.SS_encrtype[i]=="WEP" && this.SS_authtype[i]=="WEPAUTO") 
				SS_encrypt = this.SS_encrtype[i];
			else
				SS_encrypt = this.SS_encrtype[i]+"/"+this.SS_authtype[i];
			var SS_signal = xml.Get(SiteSurveyPath+"/rssi")+"%";
			
			/* 
			   If we meet hidden SSID AP, we just disable radio button. 
			   The window can be a viewer to see what APs in the space.
			*/
			var strlen_SS_ssid=this.SS_ssid[i].length;
			var orl_SS_ssid;
			var cut_first_SS_ssid;
			var SS_radio;
			if(strlen_SS_ssid > 0)
			{
//				if(strlen_SS_ssid>24)
//				{
//					orl_SS_ssid=COMM_EscapeHTMLSC(this.SS_ssid[i].substring(0,24));
//					cut_first_SS_ssid=COMM_EscapeHTMLSC(this.SS_ssid[i].substring(24,strlen_SS_ssid));
				//	var cut_second_SS_ssid=this.SS_ssid[i].substring(36,strlen_SS_ssid);
//				}
//				else
//				{	
					orl_SS_ssid=COMM_EscapeHTMLSC(this.SS_ssid[i]);
					cut_first_SS_ssid="";
					//var cut_second_SS_ssid="";	
//				}
				SS_radio = '<input id="Site_Survey_'+i+'" name="Site_Survey_'+i+'" type="radio" onClick="PAGE.OnChangeSSRadio('+i+');" />';
			}
			else
			{
				orl_SS_ssid="<font color=\"#FF0000\"><?echo i18n("Hidden AP");?></font>";
				cut_first_SS_ssid="";
				SS_radio = '<input id="Site_Survey_'+i+'" name="Site_Survey_'+i+'" type="radio" onClick="PAGE.OnChangeSSRadio('+i+');" disabled/>';
			}
			
			var data = [orl_SS_ssid+"<br>"+cut_first_SS_ssid, SS_bssid, SS_channel, "AP", SS_encrypt, SS_signal, SS_radio];
			var type = ["text","text","text","text","text","text",""];
			injecttable("SiteSurveyTable", "SS_table_"+i, data, type);
		}
		
		this.site_survey_timer = setTimeout('PAGE.OnSubmit()',8000);
	},
	OnClickConnect: function()
	{
		window.opener.OBJ("ssid_WDS").value = this.SS_ssid[this.site_survey_num];
		window.opener.OBJ("channel").value = this.SS_channel[this.site_survey_num];
		if (window.opener.OBJ("ssid_Remain").checked)
		{
			window.opener.OBJ("ssid").value = this.SS_ssid[this.site_survey_num];
		}
			
		var authtype = this.SS_authtype[this.site_survey_num];
		var encrytpe = this.SS_encrtype[this.site_survey_num];
		var sec_type = "";
		var isWPA =0;
		
		if (encrytpe=="WEP" || authtype=="WEPAUTO")
		{
			sec_type = "wep";
		}
		else if (authtype=="WPAPSK" || authtype=="WPA")
		{
			sec_type = "WPA";
			isWPA =1;
		}
		else if (authtype=="WPA2PSK" || authtype=="WPA2")
		{
			sec_type = "WPA2";
			isWPA =1;
		}
		else if (authtype=="WPA+2PSK" || authtype=="WPA+2")
		{	
//			sec_type = "WPA+2";
			sec_type = "WPA2";
			isWPA =1;
		}
		else
		{
			sec_type = "";
		}
		
		if(this.SS_encrtype[this.site_survey_num]==="NONE")	
		{	
			window.opener.OBJ("wep_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_personal_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_enterprise_WDS").style.display = "none";
			window.opener.OBJ("pad").style.display = "block";
		}
		else if(this.SS_encrtype[this.site_survey_num]==="WEP")						
		{
			window.opener.OBJ("wep_WDS").style.display = "block";
			window.opener.OBJ("box_wpa_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_personal_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_enterprise_WDS").style.display = "none";
			window.opener.OBJ("pad").style.display = "none";
		}	
		else
		{
			window.opener.OBJ("wep_WDS").style.display = "none";
			window.opener.OBJ("box_wpa_WDS").style.display = "block";
			window.opener.OBJ("box_wpa_personal_WDS").style.display = "block";
			window.opener.OBJ("box_wpa_enterprise_WDS").style.display = "none";
			window.opener.OBJ("pad").style.display = "none";
		}
		
		var wpamode = "";
		
		if(isWPA == 1)
		{
			var type = "";
			switch (authtype)
		{
			case "WPA":
			case "WPA2":
			case "WPA+2":
				type = "wpa_enterprise";
				break;
			case "WPAPSK":
			case "WPA2PSK":
			case "WPA+2PSK":
				type = "wpa_personal";
				break;	
			
		}				
			
			switch (authtype)
		{
			case "WPA":
			case "WPAPSK":			
				wpamode = "WPA";
				break;
			case "WPA2":			
			case "WPA2PSK":
				wpamode = "WPA2";
				break;			
			case "WPA+2PSK":
			case "WPA+2":
//				wpamode = "WPA+2";
				wpamode = "WPA2";
				break;				
			default:
				wpamode = "WPA+2";
			
		}				
			
			var cipher="";
			switch (encrytpe)
			{
				case "TKIP":
				case "AES":
					cipher = encrytpe;
					break;
				default:
//					cipher = "TKIP+AES";
					cipher = "AES";
			}			
			COMM_SetSelectValue(window.opener.OBJ("security_type_WDS"), type);		
			COMM_SetSelectValue(window.opener.OBJ("cipher_type_WDS"), cipher);
			COMM_SetSelectValue(window.opener.OBJ("wpa_mode_WDS"), wpamode);			
		}
		else
		{  
			COMM_SetSelectValue(window.opener.OBJ("security_type_WDS"), sec_type);
		}
		
//		if(cipher == "TKIP" || sec_type == "WEP")
//		{
//			COMM_SetSelectValue(window.opener.OBJ("bw"), "20");
//			window.opener.OBJ("bw").disabled = true;
//		}
//		else
//		{
//			window.opener.OBJ("bw").disabled = true;//false;
//		}
		window.close();
	},	
	OnClickExit: function()
	{
		window.close();
	},	
	OnChangeSSRadio: function(i)
	{
		for (var j=1; j<=this.site_survey_cnt; j++)	if(j!==i) OBJ("Site_Survey_"+j).checked = false;
		this.site_survey_num = i;
	}		
}

	function injecttable(tblID, uid, data, type)
		{
			var rows = OBJ(tblID).getElementsByTagName("tr");
			var tagTR = null;
			var tagTD = null;
			var i;
			var str;
			var found = false;
			
			/* Search the rule by UID. */
			for (i=0; !found && i<rows.length; i++) if (rows[i].id == uid) found = true;
			if (found)
			{
				for (i=0; i<data.length; i++)
				{
					tagTD = OBJ(uid+"_"+i);
					switch (type[i])
					{
					case "checkbox":
						str = "<input type='checkbox'";
						str += " id="+uid+"_check_"+i;
						if (COMM_ToBOOL(data[i])) str += " checked";
						str += " disabled>";
						break;
					default:
						str = data[i];
						break;
					}
					tagTD.innerHTML = str;
				}
				return;
			}

			/* Add a new row for this entry */
			tagTR = OBJ(tblID).insertRow(rows.length);
			tagTR.id = uid;
			/* save the rule in the table */
			for (i=0; i<data.length; i++)
			{
				tagTD = tagTR.insertCell(i);
				tagTD.id = uid+"_"+i;
				tagTD.className = "content";
				switch (type[i])
				{
				case "checkbox":
					str = "<input type='checkbox'";
					str += " id="+uid+"_check_"+i;
					if (COMM_ToBOOL(data[i])) str += " checked";
					str += " disabled>";
					tagTD.innerHTML = str;
					break;
				default:
					str = data[i];
					break;
				}
				tagTD.innerHTML = str; 
			}
		}

</script>
