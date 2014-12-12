<script type="text/javascript">
function Page() {}
Page.prototype =
{	
	services: "WIFI.PHYINF,RUNTIME.INF.LAN-1,RUNTIME.INF.LAN-2,RUNTIME.PHYINF",
	OnLoad: function() { BODY.CleanTable("client_list");BODY.CleanTable("client_list_Aband"); this.idx_24=null; this.idx_5;},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },

	idx_24 : null,
	idx_5 : null,
	inf: null,
	inf2: null,	
	
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.inf = PXML.FindModule("RUNTIME.INF.LAN-1");
		this.inf += "/runtime/inf/dhcps4/leases";
		this.inf2 = PXML.FindModule("RUNTIME.INF.LAN-2");
		this.inf2 += "/runtime/inf/dhcps4/leases";		

		if (!COMM_ToBOOL('<?=$FEATURE_NOGUESTZONE?>')) {
		PAGE.FillTable("BAND24G-1.2", "WIFI.PHYINF", "RUNTIME.PHYINF");
		}
		PAGE.FillTable("BAND24G-1.1", "WIFI.PHYINF", "RUNTIME.PHYINF");
		
		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if(this.dual_band)
		{
			OBJ("div_5G").style.display = "block";
			PAGE.FillTable("BAND5G-1.1", "WIFI.PHYINF", "RUNTIME.PHYINF");
			PAGE.FillTable("BAND5G-1.2", "WIFI.PHYINF", "RUNTIME.PHYINF");
		}
	},	
	
	FillTable : function (wlan_uid, wifi_phyinf ,runtime_phyinf)
	{		
		var wifi_module 	= PXML.FindModule(wifi_phyinf);
		var rwifi_module 	= PXML.FindModule(runtime_phyinf);
		var phyinf = GPBT(wifi_module, "phyinf", "uid",wlan_uid, false);
		
		var wifi_profile 	= XG(phyinf+"/wifi");
		var wifip 	= GPBT(wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		var freq 	= XG(phyinf+"/media/freq");	
		var rphyinf = GPBT(rwifi_module+"/runtime","phyinf","uid",wlan_uid, false);
		rphyinf += "/media/clients";
		
		if (!this.inf||!phyinf)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		
		var uid_prefix = wlan_uid.split("-")[0];
		if(uid_prefix == "BAND5G")
			str_Aband = "_Aband";
		else
			str_Aband = "";
		
		/* Fill table */
		var cnt = XG(rphyinf +"/entry#");
		var ssid = XG(wifip+"/ssid");
		
		if(cnt == "") cnt = 0;
		var idx = (uid_prefix=="BAND5G") ? this.idx_5 : this.idx_24 ;
		if(idx==null)	idx = 1;
		for (var i=1; i<=cnt; i++)
		{
			var uid		= "DUMMY-"+idx;	idx++;
			var mac		= XG(rphyinf+"/entry:"+i+"/macaddr");
			var ipaddr	= this.GetIP(mac, wlan_uid);
			var mode	= XG(rphyinf+"/entry:"+i+"/band");
			var rssi	= XG(rphyinf+"/entry:"+i+"/rssi");
			//if(freq == "5")
			if(uid_prefix=="BAND5G")
			{
				if(mode=="11g") mode="11a";
				else if(mode=="11n") mode="11n";
			}
			var rate	= XG(rphyinf+"/entry:"+i+"/rate");
			var data	= [mac, ipaddr, mode, rate, rssi];
			var type	= ["text", "text", "text", "text", "text"];
			BODY.InjectTable("client_list"+str_Aband, uid, data, type);
		}
		(uid_prefix=="BAND5G") ? this.idx_5 = idx : this.idx_24 = idx;
		OBJ("client_cnt"+str_Aband).innerHTML = idx-1;
	},
	
	PreSubmit: function() { return null; },
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	IsGuestZone: function(wlan_uid)
	{
		var str = wlan_uid.split('.');
		if(str[1]==="2")	return true;
		else 				return false;
	}, 
	GetIP: function(mac, wlan_uid)
	{
		var is_guestzone =  this.IsGuestZone(wlan_uid);
		var path = null;
		var ip = null;
		
		if(!is_guestzone)
		{
			path = GPBT(this.inf, "entry", "macaddr", mac.toLowerCase(), false);
			ip =  XG(path+"/ipaddr");	
		}
		else 
		{
			path = GPBT(this.inf2, "entry", "macaddr", mac.toLowerCase(), false);
			ip =  XG(path+"/ipaddr");
		}
		return ip;		
	}
}
</script> 
