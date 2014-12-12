<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "ROUTE.STATIC",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("ROUTE.STATIC");
		if (p === "") { alert("InitValues() ERROR !"); return false; }
		p += "/route/static";
		
		TEMP_RulesCount(p, "rmd");
		/* load rules into table */
		var count = XG(p+"/count");
		for (var i=1; i<=<?=$ROUTING_MAX_COUNT?>; i++)
		{
			var b = p+"/entry:"+i;
			var uid = XG(b+"/uid");
			var enable = (XG(b+"/enable") == "1") ? true:false;
			var name 	= (XG(b+"/name") != "") ? XG(b+"/name") : "" ;
			var network = (XG(b+"/network")!="") ? XG(b+"/network") : "" ;
			var metric	= (XG(b+"/metric")!="") ? XG(b+"/metric") : "1" ;
			var netmask = (XG(b+"/mask") != "") ? COMM_IPv4INT2MASK(XG(b+"/mask")) : "" ;
			var inf 	= (XG(b+"/inf") != "") ? XG(b+"/inf") : "WAN-1" ;
			var gateway = (XG(b+"/via") != "") ? XG(b+"/via") : "" ;
						
			OBJ("uid_"+i).value			= uid;
			OBJ("enable_"+i).checked	= enable;
			OBJ("name_"+i).value		= name;
			OBJ("dstip_"+i).value		= network;
			OBJ("metric_"+i).value		= metric;
			OBJ("netmask_"+i).value		= netmask;
			OBJ("inf_"+i).value			= inf;
			OBJ("gateway_"+i).value		= gateway;
		}
		return true;
	},
	PreSubmit: function()
	{
		
		var p = PXML.FindModule("ROUTE.STATIC");
		p += "/route/static";
		var old_count = XG(p + "/count");
		var cur_count = 0;
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p + "entry:" + old_count);
			old_count--;
		}
		
		for(var i = 1;i <= <?=$ROUTING_MAX_COUNT?>;i++)
		{
			var en		= OBJ("enable_"+i).checked ? "1" : "0";
			var inf		= OBJ("inf_"+i).value;
			var dip		= OBJ("dstip_"+i).value;
			var netmask	= OBJ("netmask_"+i).value;
			var mask 	= COMM_IPv4MASK2INT(netmask);
			var gw		= OBJ("gateway_"+i).value;
			var name 	= OBJ("name_"+i).value;
			var metric	= OBJ("metric_"+i).value;
									
			if(en === "1" || name!="" || dip != "" || netmask != "" || gw != "")
			{
				if(name === "")
				{
					BODY.ShowAlert("<?echo I18N("h", "The name could not be blank");?>");
					OBJ("name_"+i).focus();
					return null;
				}
				var vals = dip.split(".");
				if (vals.length!=4 || dip === "0.0.0.0")
				{
					BODY.ShowAlert("<?echo I18N("h", "Invalid Destination IP address.");?>");
					OBJ("dstip_"+i).focus();
					return null;
				}
				for (var j=0; j<4; j++)
				{
					if (!TEMP_IsDigit(vals[j]) || vals[j]>255)
					{
						BODY.ShowAlert("<?echo I18N("h", "Invalid Destination IP address.");?>");
						OBJ("dstip_"+i).focus();
						return null;
					}
				}
				if(netmask === "" || mask === -1)
				{
					BODY.ShowAlert("<?echo I18N("h", "Invalid Subnet Mask.");?>");
					OBJ("netmask_"+i).focus();
					return null;
				}
				if(gw === "")
				{
					BODY.ShowAlert("<?echo I18N("h", "Invalid Gateway.");?>");
					OBJ("gateway_"+i).focus();
					return null;
				}				
				if(metric < 1 || 16 < metric || !TEMP_IsDigit(metric))
				{
					BODY.ShowAlert("<?echo I18N("h", "The metric should be 1~16");?>");
					OBJ("metric_"+i).focus();
					return null;
				}
				cur_count++;
				var b = p + "/entry:" + cur_count;
				XS(b + "/uid",		"SRT-"+cur_count);
				XS(b + "/enable",	en);
				XS(b + "/inf",		inf);
				OBJ("dstip_"+i).value = COMM_IPv4NETWORK(dip, mask);
				XS(b + "/network",	OBJ("dstip_"+i).value);
				XS(b + "/mask",		mask);
				XS(b + "/via",		gw);
				XS(b + "/name",		name);
				XS(b + "/metric",	metric);
			}
		}
		// Make sure the different rules have different names 
		for (var i=1; i<cur_count; i+=1)
		{
			for (var j=i+1; j<=cur_count; j+=1)
			{
				if(OBJ("name_"+i).value != "" && OBJ("name_"+j).value !="")
				{
					if(OBJ("name_"+i).value == OBJ("name_"+j).value) 
					{
						BODY.ShowAlert("<?echo I18N("j", "The different rules could not set the same name.");?>");
						OBJ("name_"+j).focus();
						return null;
					}
				}	
			}
		}
		
		XS(p + "/count", cur_count);
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	CursorFocus: function(node)
	{
		var i = node.lastIndexOf("entry:");
		if(node.charAt(i+7)==="/") var idx = parseInt(node.charAt(i+6), 10);
		else var idx = parseInt(node.charAt(i+6), 10)*10 + parseInt(node.charAt(i+7), 10);
		var indx = 1;
		var valid_dstip_cnt = 0;		
		for(indx=1; indx <= <?=$ROUTING_MAX_COUNT?>; indx++)
		{
			if(OBJ("dstip_"+indx).value!=="") valid_dstip_cnt++;
			if(valid_dstip_cnt===idx) break;
		}
		if(node.match("network"))	OBJ("dstip_"+indx).focus();
		else if(node.match("mask"))	OBJ("netmask_"+indx).focus();
		else if(node.match("via"))	OBJ("gateway_"+indx).focus();
	}	  	
};

</script>
