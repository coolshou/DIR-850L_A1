<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF,PHYINF.WIFI",
	OnLoad: function()
	{
		this.ShowCurrentStage();
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
			case "OK":
				/*It should login again when the wizard is finished*/
				AUTH.Logout(function(){self.location.href = "./index.php";});
				return true;
				break;
			default : 
				this.currentStage--;
				this.ShowCurrentStage();
				return false;
		}
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		if (!this.Initial()) return false;
		this.InitWAN();
		return true;
	},
	PreSubmit: function()
	{
		if(this.getwantimer) clearTimeout(this.getwantimer);
		if(confirm('<?echo i18n('Do you want to bookmark "D-Link Router Web Management"?');?>'))
			addBookmarkForBrowser("D-Link Router Web Management","http://192.168.0.1/");	
		if (!this.SaveXML()) return null;
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	wifip: null,
	wifip2: null,
	phyinf: null,
	phyinf2: null,
	randomkey: null,
	getwantimer: null,
	stages: new Array ("wiz_stage_1", "wiz_stage_2"),
	currentStage: 0,	// 0 ~ this.stages.length
	Initial: function()
	{
		this.wlanbase = PXML.FindModule("WIFI.PHYINF");
		this.phyinf = GPBT(this.wlanbase, "phyinf", "uid", "BAND24G-1.1", false);
		var wifi_profile1 = XG(this.phyinf+"/wifi");
		this.wifip = GPBT(this.wlanbase+"/wifi", "entry", "uid", wifi_profile1, false);
		if (!this.wifip)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.randomkey = RandomHex(64);
		OBJ("wiz_ssid").value = XG(this.wifip+"/ssid");
		this.OnChangeNetworkKeyType();
		
		return true;
	},
	InitWAN: function ()
	{
		COMM_GetCFG(false,"INET.WAN-1,RUNTIME.INF.WAN-1,INET.WAN-2,RUNTIME.INF.WAN-2",function(xml_wan) {PAGE.ShowWAN(xml_wan);});				
	},
	ShowWAN: function (xml_wan)
	{
		var PXML_WAN = new PostXML();
		PXML_WAN.doc = xml_wan;
		
		var wan	= PXML_WAN.FindModule("INET.WAN-1");
		var rwan = PXML_WAN.FindModule("RUNTIME.INF.WAN-1");
		var waninetuid = xml_wan.Get(wan+"/inf/inet");
		var wanphyuid = xml_wan.Get(wan+"/inf/phyinf");
		this.waninetp = xml_wan.GetPathByTarget(wan+"/inet", "entry", "uid", waninetuid, false);
		this.rwaninetp = xml_wan.GetPathByTarget(rwan+"/runtime/inf", "inet", "uid", waninetuid, false);
		var str_wantype = null;   
		var str_wanipaddr = str_wangateway = str_wanDNSserver = str_wanDNSserver2 = str_wannetmask ="0.0.0.0";
		var str_name_wanipaddr = "<?echo i18n("IP Address");?>";
		var str_name_wangateway = "<?echo i18n("Default Gateway");?>";

		if (xml_wan.Get(this.waninetp+"/addrtype") == "ipv4")
		{
            if(xml_wan.Get( this.waninetp+"/ipv4/static")== "1")	str_wantype = "Static IP";
			else	str_wantype = "DHCP Client";
		    str_wanipaddr = xml_wan.Get(this.rwaninetp+"/ipv4/ipaddr");
		    str_wangateway =  xml_wan.Get(this.rwaninetp+"/ipv4/gateway");
		    str_wannetmask =  COMM_IPv4INT2MASK(xml_wan.Get(this.rwaninetp+"/ipv4/mask"));
		    str_wanDNSserver = xml_wan.Get(this.rwaninetp+"/ipv4/dns:1");
		    str_wanDNSserver2 = xml_wan.Get(this.rwaninetp+"/ipv4/dns:2");				
		}
		else if (xml_wan.Get(this.waninetp+"/addrtype") == "ppp4")
		{
            if(xml_wan.Get( this.waninetp+"/ppp4/over")== "eth")		str_wantype = "PPPoE";
			else if(xml_wan.Get( this.waninetp+"/ppp4/over")== "pptp")	str_wantype = "PPTP";
			else if(xml_wan.Get( this.waninetp+"/ppp4/over")== "l2tp")	str_wantype = "L2TP";
			else	str_wantype = "Unknow WAN type";
				
		    str_wanipaddr = xml_wan.Get(this.rwaninetp+"/ppp4/local");
		    str_wangateway = xml_wan.Get(this.rwaninetp+"/ppp4/peer");
		    str_wannetmask = "255.255.255.255";
		    str_wanDNSserver = xml_wan.Get(this.rwaninetp+"/ppp4/dns:1");
		    str_wanDNSserver2 = xml_wan.Get(this.rwaninetp+"/ppp4/dns:2");
		    if(str_wanDNSserver == "" && str_wanDNSserver2 == "")
		    {
		        var wan2 = PXML_WAN.FindModule("INET.WAN-2");
		        var rwan2 = PXML_WAN.FindModule("RUNTIME.INF.WAN-2");
		        var waninetuid2 = xml_wan.Get(wan2+"/inf/inet");
		        var rwaninetp2 = xml_wan.GetPathByTarget(rwan2+"/runtime/inf", "inet", "uid", waninetuid2, false); 
		        str_wanDNSserver = xml_wan.Get(rwaninetp2+"/ipv4/dns:1");
		        str_wanDNSserver2 = xml_wan.Get(rwaninetp2+"/ipv4/dns:2");
		    }				 
				
			str_name_wanipaddr = "<?echo i18n("Local address");?>";
			str_name_wangateway = "<?echo i18n("Peer address");?>";
		}

		if(xml_wan.Get(wan+"/inf/open_dns/type")==="advance")
		{
			str_wanDNSserver  = xml_wan.Get(wan+"/inf/open_dns/adv_dns_srv/dns1");
			str_wanDNSserver2  = xml_wan.Get(wan+"/inf/open_dns/adv_dns_srv/dns2");
		}

		if(str_wanipaddr==="0.0.0.0" || str_wanipaddr==="")
		{
			this.getwantimer = setTimeout('PAGE.InitWAN()',2000);
			OBJ("st_wanipaddr").innerHTML = OBJ("st_wangateway").innerHTML = OBJ("st_wannetmask").innerHTML = OBJ("st_wanDNSserver").innerHTML = OBJ("st_wanDNSserver2").innerHTML = "N/A"
		}
		else
		{
			OBJ("st_wanipaddr").innerHTML  = str_wanipaddr;
			OBJ("st_wangateway").innerHTML  =  str_wangateway;
			OBJ("st_wannetmask").innerHTML  =  str_wannetmask;
			OBJ("st_wanDNSserver").innerHTML  = str_wanDNSserver!="" ? str_wanDNSserver:"0.0.0.0";
			OBJ("st_wanDNSserver2").innerHTML  = str_wanDNSserver2!="" ? str_wanDNSserver2:"0.0.0.0";			
		}
		OBJ("st_wantype").innerHTML = str_wantype;		
		OBJ("name_wanipaddr").innerHTML = str_name_wanipaddr;
		OBJ("name_wangateway").innerHTML = str_name_wangateway;
	},
	SaveXML: function()
	{
		XS(this.wifip+"/ssid", OBJ("wiz_ssid").value);			
		XS(this.wifip+"/ssidhidden", "0");
		XS(this.wifip+"/authtype", "WPA+2PSK");
		XS(this.wifip+"/encrtype", "TKIP+AES");
		XS(this.wifip+"/nwkey/psk/passphrase", "");
		if (OBJ("autokey").checked)
			XS(this.wifip+"/nwkey/psk/key", this.randomkey);
		else
			XS(this.wifip+"/nwkey/psk/key", OBJ("wiz_key").value);
		XS(this.wifip+"/wps/configured", "1");
		XS(this.phyinf+"/active", "1");
		
		return true;
	},
	ShowCurrentStage: function()
	{
		for (var i=0; i<this.stages.length; i++)
		{
			if (i==this.currentStage)
				OBJ(this.stages[i]).style.display = "block";
			else
				OBJ(this.stages[i]).style.display = "none";
		}

		if (this.currentStage==0)
			SetButtonDisabled("b_pre", true);
		else
			SetButtonDisabled("b_pre", false);

		if (this.currentStage==this.stages.length-1)
		{
			SetButtonDisabled("b_next", true);
			SetButtonDisabled("b_send", false);
			OBJ("mainform").setAttribute("modified", "true");
			UpdateCFG();
		}
		else
		{
			SetButtonDisabled("b_next", false);
			SetButtonDisabled("b_send", true);
		}
	},
	SetStage: function(offset)
	{
		var length = this.stages.length;
		this.currentStage += offset;
	},
	OnClickPre: function()
	{
		this.SetStage(-1);
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		switch (this.currentStage)
		{
		case 0:
			if (OBJ("wiz_ssid").value=="")
			{
				BODY.ShowAlert("<?echo i18n("The SSID field can not be blank.");?>");
				return;
			}
			if (OBJ("manukey").checked && OBJ("wiz_key").value.length < 8)
			{
				BODY.ShowAlert("<?echo i18n("Incorrect key length, should be 8 to 63 characters long.");?>");
				return;
			}
			break;
		default:
		}
		this.SetStage(1);
		this.ShowCurrentStage();
	},
	OnClickCancel: function()
	{
		if (!COMM_IsDirty(false)||confirm("<?echo i18n("Do you want to abandon all changes you made to this wizard?");?>"))
			self.location.href = "./bsc_wlan_main.php";
	},
	OnChangeNetworkKeyType: function()
	{
		if (OBJ("autokey").checked)	OBJ("wiz_key").disabled = true;
		else	OBJ("wiz_key").disabled = false;	
	}	
}

function SetButtonDisabled(name, disable)
{
	var button = document.getElementsByName(name);
	for (i=0; i<button.length; i++)
		button[i].disabled = disable;
}

function UpdateCFG()
{	
	OBJ("ssid").innerHTML = OBJ("wiz_ssid").value;
	if (OBJ("autokey").checked)
	{
		OBJ("s_key").style.display = "none";
		OBJ("l_key").style.display = "block";
		OBJ("l_key").innerHTML = PAGE.randomkey;
	}
	else if (OBJ("wiz_key").value.length > 50)
	{
		OBJ("s_key").style.display = "none";
		OBJ("l_key").style.display = "block";
		OBJ("l_key").innerHTML = OBJ("wiz_key").value;
	}
	else
	{
		OBJ("l_key").style.display = "none";
		OBJ("s_key").style.display = "block";
		OBJ("s_key").innerHTML = OBJ("wiz_key").value;
	}
}

function RandomHex(len)
{
	var c = "0123456789abcdef";
	var str = '';
	for (var i = 0; i < len; i+=1)
	{
		var rand_char = Math.floor(Math.random() * c.length);
		str += c.substring(rand_char, rand_char + 1);
	}
	return str;
}

function addBookmarkForBrowser(sTitle, sUrl)
{
	if (/Chrome/.test(navigator.userAgent))	BODY.ShowAlert("<?echo i18n("Your browser does not support the bookmark function automatically. Please enter the bookmark by yourself.");?>");
	else if (window.sidebar && window.sidebar.addPanel)	window.sidebar.addPanel(sTitle, sUrl, "");
	else if (window.external)	window.external.AddFavorite(sUrl, sTitle);
	else	BODY.ShowAlert("<?echo i18n("Your browser does not support the bookmark function automatically. Please enter the bookmark by yourself.");?>");
}
</script>
