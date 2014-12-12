<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "WIFI.PHYINF,RUNTIME.INF.LAN-1,RUNTIME.INF.LAN-2,RUNTIME.PHYINF",
	OnLoad: function() {
		this.idx_5 = null;
		this.idx_24 = null;
		TEMP_CleanTable("client_list");
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: null,
	
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
		this.inf = PXML.FindModule("RUNTIME.INF.LAN-1");
		this.inf += "/runtime/inf/dhcps4/leases";
		this.inf2 = PXML.FindModule("RUNTIME.INF.LAN-2");
		this.inf2 += "/runtime/inf/dhcps4/leases";
		this.wifi_module = PXML.FindModule("WIFI.PHYINF");
		this.rwifi_module = PXML.FindModule("RUNTIME.PHYINF");

		PAGE.FillTable("BAND24G-1.2");
		PAGE.FillTable("BAND24G-1.1");

		this.dual_band = COMM_ToBOOL('<?=$FEATURE_DUAL_BAND?>');
		if (this.dual_band)
		{
			objs = document.getElementsByName("div_5G");
			for (var i=0; i<objs.length; i++)
				objs[i].style.display = "";
			PAGE.FillTable("BAND5G-1.1");
			PAGE.FillTable("BAND5G-1.2");
		}
		return true;
	},
	PreSubmit: function() { return null; },
	//////////////////////////////////////////////////
	idx_24 : null,
	idx_5 : null,
	inf: null,
	inf2: null,
	wifi_module: null,
	rwifi_module: null,
	FillTable : function (wlan_uid)
	{
		var phyinf		= GPBT(this.wifi_module, "phyinf", "uid",wlan_uid, false);
		var wifi_profile= XG(phyinf+"/wifi");
		var wifip		= GPBT(this.wifi_module+"/wifi", "entry", "uid", wifi_profile, false);
		var freq		= XG(this.wifi_module+"/media/freq");
		var rphyinf		= GPBT(this.rwifi_module+"/runtime","phyinf","uid",wlan_uid, false);
		rphyinf += "/media/clients";

		if (!this.inf||!phyinf)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}

		var uid_prefix = wlan_uid.split("-")[0];
		if (uid_prefix == "BAND5G")
			str_Aband = "_Aband";
		else
			str_Aband = "";

		/* Fill table */
		var cnt = XG(rphyinf +"/entry#");
		var ssid = XG(wifip+"/ssid");

		if (cnt == "") cnt = 0;
		var idx = (uid_prefix=="BAND5G") ? this.idx_5 : this.idx_24 ;
		if (idx==null) idx = 1;
		for (var i=1; i<=cnt; i++)
		{
			var uid		= "DUMMY-"+idx; idx++;
			var mac		= XG(rphyinf+"/entry:"+i+"/macaddr");
			var ipaddr	= this.GetIP(mac, wlan_uid);
			var mode	= XG(rphyinf+"/entry:"+i+"/band");
			if (freq == "5")
			{
				if (mode=="11g") mode="11a";
				else if (mode=="11n") mode="11n(5GHz)";
			}
			var rate	= XG(rphyinf+"/entry:"+i+"/rate");
			var data	= [mac, ipaddr, mode, rate];
			var type	= ["text", "text", "text", "text"];
			TEMP_InjectTable("client_list"+str_Aband, uid, data, type);
		}
		(uid_prefix=="BAND5G") ? this.idx_5 = idx : this.idx_24 = idx;
		OBJ("client_cnt"+str_Aband).innerHTML = idx-1;
	},
	IsGuestZone: function(wlan_uid)
	{
		var str = wlan_uid.split('.');
		if (str[1]==="2")	return true;
		else				return false;
	},
	GetIP: function(mac, wlan_uid)
	{
		if (!this.IsGuestZone(wlan_uid))var base = this.inf;
		else							var base = this.inf2;

		var path = GPBT(base, "entry", "macaddr", mac.toLowerCase(), false);
		return XG(path+"/ipaddr");
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
