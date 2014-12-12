<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF, PHYINF.WIFI, INET.LAN-2, OBFILTER, OBFILTER-2, INET.LAN-5",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	feature_nosch: null,
	dual_band: 0, 
	radius_adv_flag: 0,		
	radius_adv_flag_Aband: 0,		
	
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.wifi_module = PXML.FindModule("WIFI.PHYINF");		
		this.gz_obf_module = PXML.FindModule("OBFILTER");			//enable/disable routing zones 
		this.gz_obf2_module = PXML.FindModule("OBFILTER-2");			//enable/disable routing zones 
		<? if($FEATURE_NOSCH!="1")echo 'this.feature_nosch=0;'; else echo 'this.feature_nosch=1;'; ?>
		
		if(!this.Initial("BAND24G-1.2")) return false;  		
		
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if(this.dual_band)
		{
			OBJ("div_5G").style.display = "block";
			OBJ("div_route_zone_dualband").style.display = "block";
			OBJ("div_route_zone").style.display = "none";
			OBJ("div_24G_title").innerHTML 	= '<?echo I18N("h", "Session 2.4Ghz");?>';
			OBJ("div_5G_title").innerHTML 	= '<?echo I18N("h", "Session 5Ghz");?>';			
			if(!this.Initial("BAND5G-1.2")) return false;  
		}
	},
	
	Initial: function(uid_wlan)
	{
		var phyinf 	= GPBT(this.wifi_module			,"phyinf", "uid",uid_wlan, false);
		var wifip 	= GPBT(this.wifi_module+"/wifi"	,"entry", "uid" ,XG(phyinf+"/wifi"), false); 
		if (!wifip)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var freq 		= XG(phyinf+"/media/freq");
		
		//to get the wlan mode, we must access to host zone phyinf
		var wlanmode = this.GetWlanMode(uid_wlan);
		var str_Aband = (GetBand(uid_wlan)=="a") ? "_Aband" : "" ;
	
		DrawSecurityList(wlanmode, str_Aband);
		///////////////// initial WEP /////////////////
		var auth = XG(wifip+"/authtype");
		var len = (XG(wifip+"/nwkey/wep/size")=="")? "64" : XG(wifip+"/nwkey/wep/size");
		var defkey = (XG(wifip+"/nwkey/wep/defkey")=="")? "1" : XG(wifip+"/nwkey/wep/defkey");
		this.wps = COMM_ToBOOL(XG(wifip+"/wps/enable"));
		var wepauth = (auth=="SHARED") ? "SHARED" : "WEPAUTO";
		
		COMM_SetSelectValue(OBJ("auth_type"+str_Aband),	wepauth);
		COMM_SetSelectValue(OBJ("wep_key_len"+str_Aband),	len);
		COMM_SetSelectValue(OBJ("wep_def_key"+str_Aband),	defkey);
		for (var i=1; i<5; i++)
			OBJ("wep_"+len+"_"+i+str_Aband).value = XG(wifip+"/nwkey/wep/key:"+i);
		
		///////////////// initial WPA /////////////////
		var cipher = XG(wifip+"/encrtype");
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
			
		OBJ("wpa_psk_key"+str_Aband).value		= XG(wifip+"/nwkey/psk/key");
		OBJ("wpa_grp_key_intrv"+str_Aband).value 	= (XG(wifip+"/nwkey/wpa/groupintv")=="")? "3600" : XG(wifip+"/nwkey/wpa/groupintv");		
				
		OBJ("radius_srv_ip"+str_Aband).value	= XG(wifip+"/nwkey/eap/radius");
		OBJ("radius_srv_port"+str_Aband).value	= (XG(wifip+"/nwkey/eap/port")==""?"1812":XG(wifip+"/nwkey/eap/port"));
		OBJ("radius_srv_sec"+str_Aband).value	= XG(wifip+"/nwkey/eap/secret");
		
		OBJ("radius_srv_ip_second"+str_Aband).value		= XG(wifip+"/nwkey/eap:2/radius");
		OBJ("radius_srv_port_second"+str_Aband).value	= (XG(wifip+"/nwkey/eap:2/port")==""?"1812":XG(wifip+"/nwkey/eap:2/port"));
		OBJ("radius_srv_sec_second"+str_Aband).value	= XG(wifip+"/nwkey/eap:2/secret");

		OBJ("en_gzone"+str_Aband).checked 	= COMM_ToBOOL(XG(phyinf+"/active"));
		OBJ("ssid"+str_Aband).value 		= XG(wifip+"/ssid");
		
		/* for routing zone */
		/* Note /acl/obfilter for(apply at FORWARD chain): 
		 *  rule FWL-1 -> drop guestzone traffic to hostzone
		 *  rule FWL-2 -> drop hostzone  traffic to guestzone
		 */
		/* Note /acl/obfilter2(apply at INPUT chain) for traffic that from guestzone and destination is hostzone's ip or guestzone's ip.  
		 */		
		var policy_rule = XG(this.gz_obf_module+"/acl/obfilter/policy");
		var policy_rule2 = XG(this.gz_obf2_module+"/acl/obfilter2/policy");
		/* just only use policy_rule for check, it is ok */
		if(policy_rule == "" || policy_rule == "DISABLE" )	
		{
			OBJ("en_routing").checked 			= true;
			OBJ("en_routing_dualband").checked 	= true;
		}
		else
		{
			OBJ("en_routing").checked 			= false;		
			OBJ("en_routing_dualband").checked 	= false;	
		}
		
		<? if($FEATURE_NOSCH!="1")echo 'this.feature_nosch=0;'; else echo 'this.feature_nosch=1;'; ?>
		if(this.feature_nosch==0)
		{			
			COMM_SetSelectValue(OBJ("sch_gz"+str_Aband), XG(phyinf+"/schedule"));
		}	
		
		this.OnClickEnGzone(str_Aband);
		this.OnChangeSecurityType(str_Aband);
		this.OnChangeWEPKey(str_Aband);
		this.OnHostEnableCheck(str_Aband, uid_wlan);
		return true;
	},
	OnHostEnableCheck: function (str_Aband, uid_wlan)
	{
		/* if host is disabled, then our guestzone should also disabled	*/
		//host info
		if(uid_wlan=="BAND24G-1.2") 	host_uid = "BAND24G-1.1"
		else 							host_uid = "BAND5G-1.1"
		var host_phyinf = GPBT(this.wifi_module	,"phyinf", "uid",host_uid, false);
		var host_enable = XG(host_phyinf+"/active");
		
		if (host_enable != "1")
		{
			OBJ("en_gzone"+str_Aband).disabled		= true;
			OBJ("ssid"+str_Aband).disabled			= true;
			OBJ("security_type"+str_Aband).disabled	= true;
			OBJ("sch_gz"+str_Aband).disabled		= true;
			OBJ("go2sch_gz"+str_Aband).disabled		= true;
			OBJ("en_routing").disabled				= true;
			OBJ("en_routing_dualband").disabled		= true;
			
			COMM_SetSelectValue(OBJ("security_type"+str_Aband), "");		
			this.OnChangeSecurityType(str_Aband);
			OBJ("security_type"+str_Aband).disabled	= true;
			
			if(str_Aband=="_Aband") 
				OBJ("div_5G").title 	= '<?echo I18N("j", "The wireless 5Ghz is disabled. To use the guestzone feature, you need to enable it in SETUP --> WIRELESS SETTINGS.");?>';
			else 					
				OBJ("div_24G").title 	= '<?echo I18N("j", "The wireless 2.4Ghz is disabled. To use the guestzone feature, you need to enable it in SETUP --> WIRELESS SETTINGS.");?>';
		}
	},
	
	PreSubmit: function(uid_wlan)
	{
		if(!this.ValidityCheck("BAND24G-1.2")) return null; 	
		if(!this.SaveXML("BAND24G-1.2")) return null; 
		
		if(this.dual_band)
		{
			if(!this.ValidityCheck("BAND5G-1.2")) return null; 	
			if(!this.SaveXML("BAND5G-1.2")) return null; 
		}
		
		PXML.DelayActiveModule("FIREWALL-2", "12");
		PXML.CheckModule("WIFI.PHYINF", null,null, "ignore");
	  //PXML.CheckModule("INET.LAN-5", "ignore","ignore", null);
	  PXML.DelayActiveModule("INET.LAN-5","20");
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function()
	{
	},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
		
	SaveXML: function(uid_wlan)
	{		
		var phyinf = GPBT(this.wifi_module,"phyinf","uid",uid_wlan,false);
		var wifip = GPBT(this.wifi_module+"/wifi", "entry", "uid", XG(phyinf+"/wifi"), false);
		var freq = XG(phyinf+"/media/freq");
		
		var str_Aband = (GetBand(uid_wlan)=="a") ? "_Aband" : "" ;		
		
		if (OBJ("en_gzone"+str_Aband).checked)
			XS(phyinf+"/active", "1");
		else
		{
			XS(phyinf+"/active", "0");
			return true;
		}
		
		XS(wifip+"/ssid", OBJ("ssid"+str_Aband).value);
		
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
			XS(wifip+"/authtype",			OBJ("wpa_mode"+str_Aband).value);
			XS(wifip+"/encrtype", 			OBJ("cipher_type"+str_Aband).value);
			XS(wifip+"/nwkey/wpa/groupintv",	OBJ("wpa_grp_key_intrv"+str_Aband).value);
						
			XS(wifip+"/nwkey/eap/radius",	OBJ("radius_srv_ip"+str_Aband).value);
			XS(wifip+"/nwkey/eap/port",		OBJ("radius_srv_port"+str_Aband).value);
			XS(wifip+"/nwkey/eap/secret",	OBJ("radius_srv_sec"+str_Aband).value);	

			XS(wifip+"/nwkey/eap:2/radius",		OBJ("radius_srv_ip_second"+str_Aband).value);
			XS(wifip+"/nwkey/eap:2/port",		OBJ("radius_srv_port_second"+str_Aband).value);
			XS(wifip+"/nwkey/eap:2/secret",		OBJ("radius_srv_sec_second"+str_Aband).value);			
		}
		else
		{
			XS(wifip+"/authtype", "OPEN");
			XS(wifip+"/encrtype", "NONE");
		}
		
		//BODY.ShowAlert("SAVE --> sec="+OBJ("security_type"+str_Aband).value+",auth_type="+XG(wifip+"/authtype")+",wpa_mode="+OBJ("wpa_mode"+str_Aband).value+",encrtype="+XG(wifip+"/encrtype"));
		
		/* for enable routing */
		this.gz_obf_module = PXML.FindModule("OBFILTER");			//enable/disable routing zones 
		this.gz_obf2_module = PXML.FindModule("OBFILTER-2");			//enable/disable routing zones 

		var en_route = this.dual_band ? OBJ("en_routing_dualband").checked : OBJ("en_routing").checked ; 
		
        if ( en_route )
        {        	
        	if (XG(this.gz_obf_module+"/acl/obfilter/policy") != "")
        		XS(this.gz_obf_module+"/acl/obfilter/policy", "DISABLE" );
        	if (XG(this.gz_obf2_module+"/acl/obfilter2/policy") != "")
        		XS(this.gz_obf2_module+"/acl/obfilter2/policy", "DISABLE" );        	
        }
        else
        {        	
        		XS(this.gz_obf_module+"/acl/obfilter/policy", "ACCEPT" );
         		XS(this.gz_obf2_module+"/acl/obfilter2/policy", "ACCEPT" );
       }
		
		if(this.feature_nosch==0)
			XS(phyinf+"/schedule",	OBJ("sch_gz"+str_Aband).value);

		return true;
	},
	OnClickEnGzone: function(str_Aband)
	{
		if (OBJ("en_gzone"+str_Aband).checked)
		{
			OBJ("ssid"+str_Aband).disabled			= false;
			OBJ("security_type"+str_Aband).disabled	= false;
			if(this.feature_nosch==0)
			{
				OBJ("sch_gz"+str_Aband).disabled		= false;
				OBJ("go2sch_gz"+str_Aband).disabled	= false;
			}
			
			OBJ("security_type"+str_Aband).disabled= false;

			if(str_Aband == "")
				COMM_SetSelectValue(OBJ("security_type"+str_Aband), this.sec_type);
			else
				COMM_SetSelectValue(OBJ("security_type"+str_Aband), this.sec_type_Aband);
		}
		else
		{
			OBJ("ssid"+str_Aband).disabled			= true;
			OBJ("security_type"+str_Aband).disabled	= true;
			if(this.feature_nosch==0)
			{
				OBJ("sch_gz"+str_Aband).disabled	= true;
				OBJ("go2sch_gz"+str_Aband).disabled	= true;
			}

			OBJ("security_type"+str_Aband).disabled= true;
			
			if(str_Aband == "")
				this.sec_type = OBJ("security_type"+str_Aband).value;
			else 
				this.sec_type_Aband = OBJ("security_type"+str_Aband).value;

			COMM_SetSelectValue(OBJ("security_type"+str_Aband), "");				
		}
		this.OnChangeSecurityType(str_Aband);		
		
		//for enable routing zones between hostzone and guestzone
		if(this.dual_band)		
		{	
			//sam_pan add					
			if(OBJ("en_gzone_Aband").checked == false && OBJ("en_gzone").checked == false)
			{								
				OBJ("en_routing_dualband").checked	= false;
				OBJ("en_routing_dualband").disabled	= true;
			}
			else
			{												
				OBJ("en_routing_dualband").disabled     = false;
			}
		}
		else
		{
			if(OBJ("en_gzone"+str_Aband).checked==false)
            {
               OBJ("en_routing").disabled      = true;
               OBJ("en_routing_dualband").disabled     = true;
            }
            else
            {
               OBJ("en_routing").disabled      = false;
               OBJ("en_routing_dualband").disabled     = false;
            }
		}							
	},
	OnChangeSecurityType: function(str_Aband)
	{
		switch (OBJ("security_type"+str_Aband).value)
		{
			case "":
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "none";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";
				break;
			case "wep":
				OBJ("wep"+str_Aband).style.display = "block";
				OBJ("box_wpa"+str_Aband).style.display = "none";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";				
				break;
			case "wpa_personal":
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "block";
				OBJ("box_wpa_personal"+str_Aband).style.display = "block";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "none";
				break;
			case "wpa_enterprise":
				OBJ("wep"+str_Aband).style.display = "none";
				OBJ("box_wpa"+str_Aband).style.display = "block";
				OBJ("box_wpa_personal"+str_Aband).style.display = "none";
				OBJ("box_wpa_enterprise"+str_Aband).style.display = "block";
				break;
		}
		OBJ("div_second_radius"+str_Aband).style.display = "none";
		OBJ("radius_adv"+str_Aband).value = "Advanced >>";
		if(str_Aband=="")
		{ this.radius_adv_flag=0; }
		else{ this.radius_adv_flag_Aband=0; }
	},
	OnChangeWPAMode: function(str_Aband)
	{
		switch (OBJ("wpa_mode"+str_Aband).value)
		{
			case "WPA":
				OBJ("cipher_type"+str_Aband).value = "TKIP";
				break;
			case "WPA2":
				OBJ("cipher_type"+str_Aband).value = "AES";
				break;	
			default :
				OBJ("cipher_type"+str_Aband).value = "TKIP+AES";
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
	OnClickRadiusAdvanced: function(str_Aband)
    {
    	if(str_Aband=="")
    	{
        if (this.radius_adv_flag) {
            OBJ("div_second_radius"+str_Aband).style.display = "none";
            OBJ("radius_adv"+str_Aband).value = "Advanced >>";
            this.radius_adv_flag = 0;
        }
        else {
            OBJ("div_second_radius"+str_Aband).style.display = "block";
            OBJ("radius_adv"+str_Aband).value = "<< Advanced";
            this.radius_adv_flag = 1;
		}
		}
		else
		{
        if (this.radius_adv_flag_Aband) {
            OBJ("div_second_radius"+str_Aband).style.display = "none";
            OBJ("radius_adv"+str_Aband).value = "Advanced >>";
            this.radius_adv_flag_Aband = 0;
        }
        else {
            OBJ("div_second_radius"+str_Aband).style.display = "block";
            OBJ("radius_adv"+str_Aband).value = "<< Advanced";
            this.radius_adv_flag_Aband = 1;
		}
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
	},
	GetWlanMode: function(guestzone_uidwlan)
	{
		//if guest uid=BAND24G-1.2, then we need to get BAND24G-1.1 wlan mode !!
		var index 	= guestzone_uidwlan.indexOf(".")+1;
		var tmp 	= guestzone_uidwlan.slice(0,index);
		var minor = guestzone_uidwlan.slice(-1);
		var wlanmode = "bgn";
		if(minor == 2)
		{
			var host_uid 	= tmp.concat("1");
			var host_phyinf	= GPBT(this.wifi_module	,"phyinf", "uid",host_uid, false);
			var phyinf 	= GPBT(this.wifi_module			,"phyinf", "uid",guestzone_uidwlan, false);		
			var wlanmode 	= XG(host_phyinf+"/media/wlmode");		
			return wlanmode;
		}
		else 
			return "bgn";	//just default
	}
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

function DrawSecurityList(wlan_mode, str_Aband)
{
	var security_list = null;
	var cipher_list = null;
	if (wlan_mode === "n")
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

function GetBand(uid_wlan)
{
	var index 	= 0;
	index 		= uid_wlan.indexOf("-");
	var prefix 	= uid_wlan.substring(0,index);

	if(prefix == "BAND5G")
		return "a";	
	else
		return "g";
}


</script>
