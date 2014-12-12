<? include "/htdocs/webinc/config.php";?>
<script type="text/javascript">
function Page() {}
var WLan_BAND=null;
Page.prototype =
{
	services: "WIFI.PHYINF,PHYINF.WIFI,RUNTIME.PHYINF,MACCLONE.WLAN-2",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) {	return false; },
	InitValue: function(xml)
	{		
		PXML.doc = xml;
	
		var wlan_uid = "<? echo $WIFI_STA;?>";
		this.wifip 			= PXML.FindModule("WIFI.PHYINF");
		this.runtime_phyinf = PXML.FindModule("RUNTIME.PHYINF");
		this.rphyinf 		= GPBT(this.runtime_phyinf, "phyinf","uid",wlan_uid, false);		
		
		if (!this.wifip)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.phyinf 		= GPBT(this.wifip, "phyinf", "uid",wlan_uid, false);
		var wifi_profile 	= XG(this.phyinf+"/wifi");
		this.wifip 			= GPBT(this.wifip+"/wifi", "entry", "uid", wifi_profile, false);
		if(!this.Initial_STA(wlan_uid,"WIFI.PHYINF","RUNTIME.PHYINF")) return false;

		return true;
	},
	PreSubmit: function()
	{	
		if(!this.SaveXMLSta("<? echo $WIFI_STA;?>","WIFI.PHYINF","RUNTIME.PHYINF")) return null;
		PXML.IgnoreModule("RUNTIME.PHYINF");
		PXML.CheckModule("WIFI.PHYINF", null,null, "ignore");	//we now use PHYINF.WIFI to activate service

		var wep_key=OBJ("wep_def_key"+WLan_BAND).value;
		var wep_key_len=OBJ("wep_key_len"+WLan_BAND).value;
		if(OBJ("ssid"+WLan_BAND).value.charAt(0)===" "|| OBJ("ssid"+WLan_BAND).value.charAt(OBJ("ssid"+WLan_BAND).value.length-1)===" ")
		{
			alert("<?echo I18N("h", "The prefix or postfix of the 'Wireless Network Name' could not be blank.");?>");
			return null;
		}
		if((OBJ("security_type"+WLan_BAND).value==="WPA" || OBJ("security_type"+WLan_BAND).value==="WPA2" ) && OBJ("psk_eap"+WLan_BAND).value=="psk")
		{
			if (OBJ("wpapsk"+WLan_BAND).value.charAt(0)===" " || OBJ("wpapsk"+WLan_BAND).value.charAt(OBJ("wpapsk"+WLan_BAND).value.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'Network Key' could not be blank.");?>");
				return null;
			}
		}

		if(OBJ("security_type"+WLan_BAND).value==="WEP") //wep_64_1_Aband
		{
			if (OBJ("wep_"+wep_key_len+"_"+wep_key+WLan_BAND).value.charAt(0) === " "|| OBJ("wep_"+wep_key_len+"_"+wep_key+WLan_BAND).value.charAt(OBJ("wep_"+wep_key_len+"_"+wep_key+WLan_BAND).value.length-1)===" ")
			{
				alert("<?echo I18N("h", "The prefix or postfix of the 'WEP Key' could not be blank.");?>");
				return null;
			}
			
			var strlen = OBJ("wep_"+wep_key_len+"_"+wep_key+WLan_BAND).value.length;
			if(wep_key_len == "64")
			{
				if((strlen!=5) && (strlen!=10))
				{
					BODY.ShowAlert("The WEP key should be 5 or 10 characters long." );
					return null;
				}
			}else //if wep_key_len == "128"
			{
				if((strlen!=13) && (strlen!=26))
				{
					BODY.ShowAlert("The WEP key should be 13 or 26 characters long.");
					return null;
				}
			}
		}	
		return PXML.doc;
	},	
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	wifip: null,
	phyinf: null,
	runtime_phyinf: null,
	rphyinf: null,
	swithmode: null,
	sec_type: null,
	sec_type_Aband: null,
	sec_type_sta: null,
	wlanMode: null,
	bandWidth: null,
	str_Aband: null,
	feature_nosch: <? if($FEATURE_NOSCH!="1")echo '0'; else echo '1'; ?>,
	Initial_STA: function(wlan_uid,wifi_phyinf,runtime_phyinf)
	{
		OBJ("div_station").style.display = "block";	
		
		OBJ("en_wifi_sta").checked = COMM_ToBOOL(XG(this.phyinf+"/active"));
		WLan_BAND="_sta";
		//if(this.feature_nosch==0)
			//COMM_SetSelectValue(OBJ("sch_sta"), XG(this.phyinf+"/schedule"));
	
		OBJ("ssid_sta").value = XG(this.wifip+"/ssid");
		
		var freq = XG(this.phyinf+"/media/freq")+"G";
		COMM_SetSelectValue(OBJ("freq_sta"), freq);
		if(freq=="5G") OBJ("bw_sta").innerHTML = "20/40/80 Mhz (Auto)";
		else OBJ("bw_sta").innerHTML = "20/40 MHz (Auto)";
		
		if (!OBJ("en_wifi_sta").checked)
			this.sec_type_sta = "";
		else if (XG(this.wifip+"/encrtype")=="WEP")
			this.sec_type_sta = "WEP";
		else if (XG(this.wifip+"/authtype")=="WPAPSK" || XG(this.wifip+"/authtype")=="WPAEAP")
			this.sec_type_sta = "WPA"
		else if (XG(this.wifip+"/authtype")=="WPA2PSK" || XG(this.wifip+"/authtype")=="WPA2EAP")
			this.sec_type_sta = "WPA2"
		else if (XG(this.wifip+"/authtype")=="WPA+2" || XG(this.wifip+"/authtype")=="WPA+2PSK")
			this.sec_type_sta = "WPA+2"					
		else
			this.sec_type_sta = "";
		COMM_SetSelectValue(OBJ("security_type_sta"), this.sec_type_sta);

		///////////////// initial WEP /////////////////
		var auth = XG(this.wifip+"/authtype");
		var len = (XG(this.wifip+"/nwkey/wep/size")=="")? "64" : XG(this.wifip+"/nwkey/wep/size");
		var defkey = (XG(this.wifip+"/nwkey/wep/defkey")=="")? "1" : XG(this.wifip+"/nwkey/wep/defkey");
		if (auth!="SHARED") auth = "OPEN";
		COMM_SetSelectValue(OBJ("auth_type_sta"),	auth);
		COMM_SetSelectValue(OBJ("wep_key_len_sta"),	len);
		COMM_SetSelectValue(OBJ("wep_def_key_sta"),	defkey);
		for (var i=1; i<5; i++)
			OBJ("wep_"+len+"_"+i+"_sta").value = XG(this.wifip+"/nwkey/wep/key:"+i);
		///////////////// initial WPA /////////////////
		var cipher = XG(this.wifip+"/encrtype");
		var type = null;
		switch (XG(this.wifip+"/authtype"))
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
				cipher = "TKIP+AES";
		}
		COMM_SetSelectValue(OBJ("cipher_type_sta"), cipher);
		COMM_SetSelectValue(OBJ("psk_eap_sta"), type);

		OBJ("wpapsk_sta").value		= XG(this.wifip+"/nwkey/psk/key");
		OBJ("srv_ip_sta").value		= XG(this.wifip+"/nwkey/eap/radius");
		OBJ("srv_port_sta").value	= XG(this.wifip+"/nwkey/eap/port");
		OBJ("srv_sec_sta").value	= XG(this.wifip+"/nwkey/eap/secret");
		
		this.OnChangeSecurityType("_sta");
		this.OnChangeWEPKey("_sta");
		this.OnChangeWPAAuth("_sta");
		this.OnClickEnWLANsta();
		
		return true;
	},
	
	SaveXMLSta: function(wlan_uid,wifi_phyinf,runtime_phyinf)
	{
		var wifi_module 	= PXML.FindModule(wifi_phyinf);
		this.phyinf 		= GPBT(wifi_module,"phyinf","uid",wlan_uid,false);
		var wifi_profile 	= XG(this.phyinf+"/wifi");
		this.wifip 			= GPBT(wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		if (OBJ("en_wifi_sta").checked)
		{
			XS(this.phyinf+"/active", "1");
			XS(wifi_module+"/wifi/enable", "1");
		}
		else
		{
			XS(this.phyinf+"/active", "0");
			XS(wifi_module+"/wifi/enable", "0");
			return true;
		}
		
		//if(this.feature_nosch==0)
			//XS(this.phyinf+"/schedule",	OBJ("sch_sta").value);
		
		XS(this.wifip+"/ssid",		OBJ("ssid_sta").value);
		if (OBJ("security_type_sta").value=="WEP")
		{
			if (OBJ("auth_type_sta").value=="SHARED")
				XS(this.wifip+"/authtype", "SHARED");
			else
				XS(this.wifip+"/authtype", "OPEN");
			XS(this.wifip+"/encrtype",			"WEP");
			XS(this.wifip+"/nwkey/wep/size",	"");
			XS(this.wifip+"/nwkey/wep/ascii",	"");
			XS(this.wifip+"/nwkey/wep/defkey",	OBJ("wep_def_key_sta").value);
			for (var i=1, len=OBJ("wep_key_len_sta").value; i<5; i++)
			{
				if (i==OBJ("wep_def_key_sta").value)
					XS(this.wifip+"/nwkey/wep/key:"+i, OBJ("wep_"+len+"_"+i+"_sta").value);
				else
					XS(this.wifip+"/nwkey/wep/key:"+i, "");
			}
		}
		/*
		else if (OBJ("security_type_sta").value=="wpa")
		{
			XS(this.wifip+"/encrtype", OBJ("cipher_type_sta").value);
			
			XS(this.wifip+"/authtype",				"WPAPSK");
			XS(this.wifip+"/nwkey/psk/passphrase",	"");
			XS(this.wifip+"/nwkey/psk/key",			OBJ("wpapsk_sta").value);
		}
		else if (OBJ("security_type_sta").value=="wpa2")
		{
			XS(this.wifip+"/encrtype", OBJ("cipher_type_sta").value);
			
			XS(this.wifip+"/authtype",				"WPA2PSK");
			XS(this.wifip+"/nwkey/psk/passphrase",	"");
			XS(this.wifip+"/nwkey/psk/key",			OBJ("wpapsk_sta").value);
		}
		*/
		else if (OBJ("security_type_sta").value=="WPA" || 
				OBJ("security_type_sta").value=="WPA2" ||
				OBJ("security_type_sta").value=="WPA+2")
		{
			XS(this.wifip+"/encrtype", OBJ("cipher_type_sta").value);
			
			if (OBJ("psk_eap_sta").value=="psk")
			{
				XS(this.wifip+"/authtype",				OBJ("security_type_sta").value+"PSK");
				XS(this.wifip+"/nwkey/psk/passphrase",	"");
				XS(this.wifip+"/nwkey/psk/key",			OBJ("wpapsk_sta").value);
			}
			else
			{
				XS(this.wifip+"/authtype",			OBJ("security_type_sta").value);
				XS(this.wifip+"/nwkey/eap/radius",	OBJ("srv_ip_sta").value);
				XS(this.wifip+"/nwkey/eap/port",	OBJ("srv_port_sta").value);
				XS(this.wifip+"/nwkey/eap/secret",	OBJ("srv_sec_sta").value);
			}
		}
		else
		{
			XS(this.wifip+"/authtype", "OPEN");
			XS(this.wifip+"/encrtype", "NONE");
		}
		
		if(OBJ("freq_sta").value=="2.4G")
		{
			XS(this.phyinf+"/media/freq", "2.4");
			XS(this.phyinf+"/media/dot11n/bandwidth",	"20+40");
		}
		else if(OBJ("freq_sta").value=="5G")
		{
			XS(this.phyinf+"/media/freq", "5");
			XS(this.phyinf+"/media/dot11n/bandwidth",	"20+40+80");
		}
		
		if(OBJ("channel").value=="") XS(this.phyinf+"/media/channel", "0");
		else XS(this.phyinf+"/media/channel", OBJ("channel").value);
		
		return true;
	},
	
	OnClickEnWLANsta:function()
	{
		var val = (OBJ("en_wifi_sta").checked)?false:true;
		
		if((OBJ("en_wifi_sta").checked) == false) 	
		{
				this.sec_type_sta="";
		}
		if(this.feature_nosch==0)
		{
			//OBJ("sch_sta").disabled		= val;
			//OBJ("go2sch_sta").disabled	= val;This is station mode schedule function. If body's bsc_wlan_br.php turns on schedule function, then this line should be enabled.
		}
		
		OBJ("site_survey_button").disabled	= val;
		OBJ("ssid_sta").disabled			= val;
		OBJ("security_type_sta").disabled	= val;
		COMM_SetSelectValue(OBJ("security_type_sta"), this.sec_type_sta);

		this.OnChangeSecurityType("_sta");
	},
	OnChangeSecurityType: function(postfix)
	{
		OBJ("bw"+postfix).disabled = false;
		switch (OBJ("security_type"+postfix).value)
		{
			case "":
				OBJ("wep"+postfix).style.display = "none";
				OBJ("wpa"+postfix).style.display = "none";
				OBJ("pad").style.display = "block";
				break;
			case "WEP":
				OBJ("wep"+postfix).style.display = "block";
				OBJ("wpa"+postfix).style.display = "none";
				OBJ("pad").style.display = "none";
				OBJ("bw"+postfix).value = "20";
				OBJ("bw"+postfix).disabled = true;
				break;
			case "WPA":
				OBJ("wep"+postfix).style.display = "none";
				OBJ("wpa"+postfix).style.display = "block";
				OBJ("pad").style.display = "none";	
				PAGE.OnChangeCipher(postfix);
				break;
			case "WPA2":
				OBJ("wep"+postfix).style.display = "none";
				OBJ("wpa"+postfix).style.display = "block";
				OBJ("pad").style.display = "none";	
				PAGE.OnChangeCipher(postfix);
				break;
			case "WPA+2":
				OBJ("wep"+postfix).style.display = "none";
				OBJ("wpa"+postfix).style.display = "block";
				OBJ("pad").style.display = "none";	
				PAGE.OnChangeCipher(postfix);
				break;
		}
	},
	OnChangeWEPKey: function(postfix)
	{
		var no = S2I(OBJ("wep_def_key"+postfix).value) - 1;
		
		switch (OBJ("wep_key_len"+postfix).value)
		{
			case "64":
				OBJ("wep_64"+postfix).style.display = "block";
				OBJ("wep_128"+postfix).style.display = "none";
				SetDisplayStyle(null, "wepkey_64"+postfix, "none");
				document.getElementsByName("wepkey_64"+postfix)[no].style.display = "inline";
				break;
			case "128":
				OBJ("wep_64"+postfix).style.display = "none";
				OBJ("wep_128"+postfix).style.display = "block";
				SetDisplayStyle(null, "wepkey_128"+postfix, "none");
				document.getElementsByName("wepkey_128"+postfix)[no].style.display = "inline";
		}
	},
	OnChangeWPAAuth: function(postfix)
	{
		switch (OBJ("psk_eap"+postfix).value)
		{
			case "psk":
				SetDisplayStyle("div", "psk"+postfix, "block");
				SetDisplayStyle("div", "eap"+postfix, "none");
				break;
			case "eap":
				SetDisplayStyle("div", "psk"+postfix, "none");
				SetDisplayStyle("div", "eap"+postfix, "block");
		}
	},
	OnChangeCipher: function(postfix)
	{
		switch (OBJ("cipher_type"+postfix).value)
		{
			case "TKIP":
				OBJ("bw"+postfix).value = "20";
				OBJ("bw"+postfix).disabled = true;
				break;
			default:
				OBJ("bw"+postfix).disabled = false;
		}
	},
	OnClickSiteSurvey: function()
	{
		var	options="toolbar=0,status=0,menubar=0,scrollbars=1,resizable=1,width=800,height=600";
		window.open("bsc_wlan_sitesurvey.php","bsc_wlan_sitesurvey",options);
	},
	OnChangeWirelessBand: function(freq)
	{
		if(freq=="5G") OBJ("bw_sta").innerHTML = "20/40/80 Mhz (Auto)";
		else OBJ("bw_sta").innerHTML = "20/40 MHz (Auto)";		
	},	
	curpin: null,
	defpin: "<? echo query("/runtime/devdata/pin");?>"
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
</script>
