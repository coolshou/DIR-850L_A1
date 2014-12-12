<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "ICMP.WAN-1,ICMP.WAN-2,PHYINF.WAN-1,MULTICAST,UPNP.LAN-1,WIFI.PHYINF,DEVICE",
	ipv6_feature: !COMM_ToBOOL('<?=$FEATURE_NOIPV6?>'), 
	eee_feature: COMM_ToBOOL('<?=$FEATURE_EEE?>'), 
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			OBJ("upnp").disabled = true;
			OBJ("icmprsp").disabled = true;
			OBJ("wanspeed").disabled = true;
			OBJ("mcast").disabled = true;
			if(this.ipv6_feature)	OBJ("mcast6").disabled = true;
		}
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
		var upnp = PXML.FindModule("UPNP.LAN-1");
		var icmprsp = PXML.FindModule("ICMP.WAN-1");
		var phy = PXML.FindModule("PHYINF.WAN-1");
		var wanphyuid = PXML.doc.Get(phy+"/inf/phyinf");
		var wan = PXML.doc.GetPathByTarget(phy, "phyinf", "uid", wanphyuid, false);
		this.DEVICEp = PXML.FindModule("DEVICE");
	
		var mcast = PXML.FindModule("MULTICAST");
		if (upnp==="" || icmprsp==="" || mcast==="" || wan==="")
		{ BODY.ShowMessage("<?echo I18N("j","Error");?>","InitValue ERROR!"); return false; }

		OBJ("upnp").checked = (XG(upnp+"/inf/upnp/count") == 1);
		OBJ("icmprsp").checked = (XG(icmprsp+"/inf/icmp")==="ACCEPT");
		
		var speed = XG(wan+"/media/linktype");
		if(speed === "AUTO")
			{OBJ("wanspeed").value = "0";}
		else if(speed === "10F")
			{OBJ("wanspeed").value = "1";}
		else if(speed === "100F")
			{OBJ("wanspeed").value = "2";}
<? if($FEATURE_WAN1000FTYPE!="1") {echo "/*";}?>
		else if(speed === "1000F")
			{OBJ("wanspeed").value = "3";}
<? if($FEATURE_WAN1000FTYPE!="1") {echo "*/";}?>
		
		OBJ("mcast").checked = (XG(mcast+"/device/multicast/igmpproxy")==="1");
		OBJ("enhance").checked = (XG(mcast+"/device/multicast/wifienhance")==="1");
		
		if(this.ipv6_feature) {
			OBJ("mcast6").checked = (XG(mcast+"/device/multicast/mldproxy")==="1");
			OBJ("enhance6").checked = (XG(mcast+"/device/multicast/wifienhance6")==="1");
		}
		
		this.Click_Multicast_Enable();
		OBJ("eee").checked = (XG(this.DEVICEp+"/device/eee")==="1");
	
		return true;
	},
PreSubmit: function()
	{
		if (this.rgmode)
		{
			var upnp = PXML.FindModule("UPNP.LAN-1");
			var icmprsp = PXML.FindModule("ICMP.WAN-1");
			var icmprsp2 = PXML.FindModule("ICMP.WAN-2");
			var phy = PXML.FindModule("PHYINF.WAN-1");
			var wanphyuid = PXML.doc.Get(phy+"/inf/phyinf");
			var wan = PXML.doc.GetPathByTarget(phy, "phyinf", "uid", wanphyuid, false);
			XS(icmprsp+"/inf/icmp",	OBJ("icmprsp").checked ? "ACCEPT":"DROP");
			XS(icmprsp2+"/inf/icmp",	OBJ("icmprsp").checked ? "ACCEPT":"DROP");
			XS(upnp+"/inf/upnp/count",	OBJ("upnp").checked ? "1":"0");		
			if(OBJ("upnp").checked)
			{
				XS(upnp+"/inf/upnp/entry:1", "urn:schemas-upnp-org:device:InternetGatewayDevice:1");			
			}
			else
			{
				XS(upnp+"/inf/upnp/entry:1", "");
			}
			
			var wanspeed = OBJ("wanspeed").value;
			if(wanspeed === "0")
				{XS(wan+"/media/linktype", "AUTO");}
			else if(wanspeed === "1")
				{XS(wan+"/media/linktype", "10F");}
			else if(wanspeed === "2")
				{XS(wan+"/media/linktype", "100F");}
<? if($FEATURE_WAN1000FTYPE!="1") {echo "/*";}?>
			else if(wanspeed === "3")
				{XS(wan+"/media/linktype", "1000F");}
<? if($FEATURE_WAN1000FTYPE!="1") {echo "*/";}?>
		}
		else
		{
			PXML.IgnoreModule("UPNP.LAN-1");
			PXML.IgnoreModule("ICMP.WAN-1");
			PXML.IgnoreModule("PHYINF.WAN-1");
		}
		var mcast = PXML.FindModule("MULTICAST");
		XS(mcast+"/device/multicast/igmpproxy", OBJ("mcast").checked ? "1":"0");
		XS(mcast+"/device/multicast/wifienhance", OBJ("enhance").checked ? "1":"0");
		if(this.eee_feature) XS(this.DEVICEp+"/device/eee", OBJ("eee").checked ? "1":"0");
		if(this.ipv6_feature) {
			XS(mcast+"/device/multicast/mldproxy", OBJ("mcast6").checked ? "1":"0");
			XS(mcast+"/device/multicast/wifienhance6", OBJ("enhance6").checked ? "1":"0");
		}
		PXML.CheckModule("WIFI", null, "ignore", null);

		return PXML.doc;
	},
	//////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	Click_Multicast_Enable: function()
	{	
		
		if(OBJ("mcast").checked)
		{
			OBJ("enhance").disabled = false;			
		}
		else
		{	
			OBJ("enhance").checked = false;
			OBJ("enhance").disabled = true;						
		}
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

//]]>
</script>
