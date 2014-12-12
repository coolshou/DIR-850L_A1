<?
include "/htdocs/phplib/xnode.php";
?><script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "RUNTIME.PHYINF,INET.WAN-1,RUNTIME.INF.WAN-1",
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

		/* Checking Internet Connection*/
		if (this.IsConnected())
		{
			OBJ("inet_led").innerHTML = "<b><?echo I18N("h","Internet");?></b>";
			OBJ("inet_icon").className = "lan_connected";
		}
		else
		{
			OBJ("inet_led").innerHTML = "<s><?echo I18N("h","Internet");?></s>";
			OBJ("inet_icon").className = "lan_disconnected";
		}

		return true;
	},
	PreSubmit: null,
	//////////////////////////////////////////////////
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
