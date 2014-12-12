<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "",
	OnLoad: function()	{},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
			case "OK":
				setTimeout('PAGE.SiteSurvey()',2000);
				break;
			case "":	//Logout
				break;
			default:
				BODY.ShowAlert(result);
				break;
		}
		return true;
	},
	InitValue: function(xml)
	{		
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
	SS_freq: new Array(),
	SS_encrtype: new Array(),
	SS_authtype: new Array(),
	SS_channel: new Array(),
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
		var SiteSurveyP = "/postxml/module/runtime/wifi_tmpnode/sitesurvey/entry";
		this.site_survey_cnt = xml.Get(SiteSurveyP+"#");
		BODY.CleanTable("SiteSurveyTable");
		for (var i=1; i<=this.site_survey_cnt; i++)
		{	
			SiteSurveyPath = SiteSurveyP+":"+i; 
			this.SS_ssid[i] = xml.Get(SiteSurveyPath+"/ssid");
			var SS_bssid = xml.Get(SiteSurveyPath+"/macaddr");
			this.SS_channel[i] = xml.Get(SiteSurveyPath+"/channel");
			this.SS_freq[i] = xml.Get(SiteSurveyPath+"/wlmode");
			var SS_channel_freq = this.SS_channel[i]+"("+this.SS_freq[i]+")";
			this.SS_encrtype[i] = xml.Get(SiteSurveyPath+"/encrtype");
			this.SS_authtype[i] = xml.Get(SiteSurveyPath+"/authtype");
			if(this.SS_encrtype[i]=="WEP" && this.SS_authtype[i]=="WEPAUTO") 
				var SS_encrypt = this.SS_encrtype[i];
			else
			var SS_encrypt = this.SS_encrtype[i]+"/"+this.SS_authtype[i];
			var SS_signal = xml.Get(SiteSurveyPath+"/rssi");
			var SS_radio = '<input id="Site_Survey_'+i+'" name="Site_Survey_'+i+'" type="radio" onClick="PAGE.OnChangeSSRadio('+i+');" />';
			
			var SS_ssid_show = "";
			var strlen_SS_ssid=this.SS_ssid[i].length;
			if(strlen_SS_ssid>18)
			{
				var orl_SS_ssid=this.SS_ssid[i].substring(0,16);
				var cut_first_SS_ssid=this.SS_ssid[i].substring(16,strlen_SS_ssid);
			//	var cut_second_SS_ssid=this.SS_ssid[i].substring(36,strlen_SS_ssid);
				SS_ssid_show = orl_SS_ssid + "<br>" + cut_first_SS_ssid;
			}
			else
			{
				SS_ssid_show = this.SS_ssid[i];	
			}
			SS_ssid_show=SS_ssid_show.replace(/[\s]/g,"&nbsp;"); //Replace " " to "&nbsp;" to show the whole space words in the SSID block.
			var data = [SS_ssid_show, SS_bssid, SS_channel_freq, "AP", SS_encrypt, SS_signal, SS_radio];	
			var type = ["text","text","text","text","text","text",""];
			injecttable("SiteSurveyTable", "SS_table_"+i, data, type);
		}
		
		this.site_survey_timer = setTimeout('PAGE.OnSubmit()',7000);
	},
	OnClickConnect: function()
	{
		window.opener.OBJ("ssid_sta").value = this.SS_ssid[this.site_survey_num];
		window.opener.OBJ("channel").value = this.SS_channel[this.site_survey_num];		
		COMM_SetSelectValue(window.opener.OBJ("freq_sta"), this.SS_freq[this.site_survey_num]);
		window.opener.PAGE.OnChangeWirelessBand(this.SS_freq[this.site_survey_num]);
		
		var encrytpe = this.SS_encrtype[this.site_survey_num];
		var authtype = this.SS_authtype[this.site_survey_num];
		var sec_type = "";
		
		if (encrytpe=="WEP")
			sec_type = "WEP";
		else if (authtype=="WPAPSK" || authtype=="WPAEAP")
			sec_type = "WPA";
		else if (authtype=="WPA2PSK" || authtype=="WPA2")
			sec_type = "WPA2";
		else if (authtype=="WPA+2PSK" || authtype=="WPA+2")
			sec_type = "WPA2";
			//sec_type = "WPA+2" //DIR-865 not support WPA/WPA2 auto
		else
			sec_type = "";
			
		var cipher = encrytpe;
		var type = null;
		switch (authtype)
		{
			case "WPA":
			case "WPA2":
			case "WPA+2":
				type = "eap";
				break;
			default:
				type = "psk";
		}
		switch (cipher)
		{
			case "TKIP":
			case "AES":
				break;
			default:
				//Marco says that the cipher type with auto mode has problem to connect to AP due to Broadcom wireless driver in DIR-865L. So we would use AES mode when the cipher type is auto mode. 20120525
				cipher = "AES";
		}
			
		if(this.SS_encrtype[this.site_survey_num]==="NONE")	
		{	
			window.opener.OBJ("wep_sta").style.display = "none";
			window.opener.OBJ("wpa_sta").style.display = "none";
			window.opener.OBJ("pad").style.display = "block";
		}
		else if(this.SS_encrtype[this.site_survey_num]==="WEP")						
		{
			window.opener.OBJ("wep_sta").style.display = "block";
			window.opener.OBJ("wpa_sta").style.display = "none";
			window.opener.OBJ("pad").style.display = "none";
		}	
		else
		{
			window.opener.OBJ("wep_sta").style.display = "none";
			window.opener.OBJ("wpa_sta").style.display = "block";
			window.opener.OBJ("pad").style.display = "none";			
		}	
	
		COMM_SetSelectValue(window.opener.OBJ("security_type_sta"), sec_type);		
		COMM_SetSelectValue(window.opener.OBJ("cipher_type_sta"), cipher);
		COMM_SetSelectValue(window.opener.OBJ("psk_eap_sta"), type);

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
