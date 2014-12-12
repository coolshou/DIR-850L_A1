<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "RUNTIME.INF.LAN-1,RUNTIME.INF.LAN-2,RUNTIME.PHYINF,INET.WAN-1,RUNTIME.INF.WAN-1",
	OnLoad: null,
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
		if (this.IsConnected())
			OBJ("connected").style.background="url(../pic/netmorkmap_connection.gif) no-repeat";
		else
			OBJ("connected").style.background="url(../pic/netmorkmap_not_connection.gif) no-repeat";

		var infp = PXML.FindModule("RUNTIME.INF.LAN-1");
		var infp2 = PXML.FindModule("RUNTIME.INF.LAN-2");
		this.wifib = PXML.FindModule("RUNTIME.PHYINF");
		if (!infp||!infp2||!this.wifib) {BODY.ShowMessage("Error","InitDHCPS() ERROR !"); return false;}

		this.wifib += "/runtime";
		this.Update(infp, "LAN-1");
		this.Update(infp2, "LAN-2");
		return true;
	},
	PreSubmit: null,
	//////////////////////////////////////////////////
	wifib: null,
	wifiuid: ["BAND24G-1.1","BAND24G-1.2","BAND5G-1.1","BAND5G-1.2"],
	Update: function(infp, network)
	{
		var base = GPBT(infp+"/runtime", "inf", "uid", network, false);
		var leasep = base + "/dhcps4/leases";
		var entry = leasep+"/entry";
		var cnt = XG(entry+"#");
		for (var i=1; i<=cnt; i++)
		{
			var host = XG(entry+":"+i+"/hostname");
			var ipaddr = XG(entry+":"+i+"/ipaddr");
			var macaddr = XG(entry+":"+i+"/macaddr");
			var expires = XG(entry+":"+i+"/expire");
			var static = (S2I(expires) > 6000000)? '<span class="led_on">ON</span>':'<span class="led_off">OFF</span>';
			/* wireless: <p class="map_ico wireless">PC</p> */
			/* usb: <p class="map_ico usb">PC</p> */
			if (this.IsWifiClient(macaddr))
				var data = ['<p class="map_ico wireless">PC</p>', host, ipaddr, macaddr];
			else
				var data = ['<p class="map_ico pc">PC</p>', host, ipaddr, macaddr];
			var type = ["", "text", "text", "text"];
			TEMP_InjectTable("leases_list", null, data, type);
		}
	},
	IsWifiClient: function(macaddr)
	{
		for (var i=0; i<this.wifiuid.length; i++)
		{
			var base = GPBT(this.wifib, "phyinf", "uid", this.wifiuid[i], false);
			base += "/media/clients";
			var cnt = S2I(XG(base+"/entry#"));
			if (cnt==0) continue;
			for (var j=1; j<=cnt; j++)
				if (XG(base+"/entry:"+j+"/macaddr").toLowerCase()==macaddr.toLowerCase()) return true;
		}
		return false;
	},
	IsConnected: function()
	{
		var wan = PXML.FindModule("INET.WAN-1");
		var rwan = PXML.FindModule("RUNTIME.INF.WAN-1");
		var rphy = PXML.FindModule("RUNTIME.PHYINF");
		var waninetuid = XG(wan+"/inf/inet");
		var wanphyuid = XG(wan+"/inf/phyinf");
		var waninetp = GPBT(wan+"/inet", "entry", "uid", waninetuid, false);
		var rwaninetp = GPBT(rwan+"/runtime/inf", "inet", "uid", waninetuid, false);
		var rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", wanphyuid, false);

		var str_networkstatus = str_Disconnected = "<?echo I18N("j","Disconnected");?>";
		var str_Connected = "<?echo I18N("j","Connected");?>";
		var wancable_status = 0;
		var wan_network_status = 0;
		if ((XG(rwanphyp+"/linkstatus")!="0")&&(XG(rwanphyp+"/linkstatus")!=""))
		{
			wancable_status = 1;
		}
		if (wancable_status!=1) return false;

		if (XG(waninetp+"/addrtype") == "ipv4")
		{
			if (XG(waninetp+"/ipv4/ipv4in6/mode")!=""||
				XG(waninetp+"/ipv4/static")=="1"||
				XG(rwaninetp+"/ipv4/valid")=="1")
				wan_network_status = 1;
		}
		else if (XG(waninetp+"/addrtype")=="ppp4"||
				XG(waninetp+"/addrtype")=="ppp10")
		{
			if (XG(rwaninetp+"/ppp4/valid")=="1")
				wan_network_status = 1;

			switch (XG(rwan+"/runtime/inf/pppd/status"))
			{
			case "connected":
				if (wan_network_status==1) break;
			case "":
			case "disconnected":
			case "on demand":
			default:
				return false;
			}
		}
		return (wan_network_status==1)?true:false;
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
