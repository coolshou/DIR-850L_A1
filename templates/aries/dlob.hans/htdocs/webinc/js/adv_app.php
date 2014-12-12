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
	services: "PORTT.NAT-1",
	OnLoad: function()
	{
		/* draw the 'Application Name' select */
		var str = "";
		for(var i=1; i<=<?=$APP_MAX_COUNT?>; i+=1)
		{
			str = "";
			str += '<select id="app_'+i+'">';
			for(var j=0; j<this.apps.length; j+=1)
				str += '<option value="'+j+'">'+this.apps[j].name+'</option>';
			str += '</select>';
			OBJ("span_app_"+i).innerHTML = str;
		}
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
		var p = PXML.FindModule(this.services);
		
		if (p === "") alert("<?echo i18n("ERROR!");?>");
		p += "/nat/entry/porttrigger";
		TEMP_RulesCount(p, "rmd");
		var cnt = XG(p+"/count");
		for (var i = 1;i <= <?=$APP_MAX_COUNT?>;i++)
		{
			var b = p + "/entry:" + i;
			OBJ("en_" + i).checked = XG(b + "/enable") == "1" ? true : false;

			if( XG(b + "/schedule") !="" )
				OBJ("sch_" + i).value = XG(b + "/schedule");
			else
				OBJ("sch_" + i).value = -1;

			OBJ("name_" + i).value = XG(b + "/description");
			
			var d = b + "/trigger";
			
			if(  XG(d + "/protocol") != "" )
				OBJ("pripro_" + i).value = XG(d + "/protocol");
			else
				OBJ("pripro_" + i).value = "TCP+UDP"
				
			if (XG(d+"/end")=="")
				OBJ("priport_" + i).value = XG(d + "/start");
			else
				OBJ("priport_" + i).value = XG(d + "/start") + "-" + XG(d + "/end");

			d = b + "/external";
			if( XG(d + "/protocol") != "" )
				OBJ("pubpro_" + i).value = XG(d + "/protocol");
			else
				OBJ("pubpro_" + i).value = "TCP+UDP";
				
			OBJ("pubport_" + i).value = XG(d + "/portlist");
		}
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule(this.services);
		p += "/nat/entry/porttrigger";
		var old_count = XG(p+"/count");
		var cur_count = 0;
		var j = 0;
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p+"/entry:"+old_count);
			old_count -= 1;
		}
		/* update the entries */
		for (var i=1; i<=<?=$APP_MAX_COUNT?>; i+=1)
		{
			/* if the description field is empty, it means to remove this entry,
			 * so skip this entry. */
			if (OBJ("name_"+i).value === "") continue;
			
			// check same name exist or not
			for(j=1; j < i; j++)
			{
				var name = OBJ("name_"+i).value;
				if( OBJ("name_"+i).value === OBJ("name_"+j).value )
				{
					BODY.ShowAlert('<?echo i18n("The Name ");?>"'+name+'\"<?echo i18n(" is already used !");?>');
					OBJ("name_"+i).focus();
					return null;
				}				
			}
			if (OBJ("priport_"+i).value=="")
			{
				BODY.ShowAlert("<?echo i18n("Please input the trigger port range !");?>");
				OBJ("priport_"+i).focus();
				return null;
			}
			if (OBJ("priport_"+i).value!="" && !check_trigger_port(OBJ("priport_"+i).value))
			{
				BODY.ShowAlert("<?echo i18n("Invalid Trigger port setting !");?>");
				OBJ("priport_"+i).focus();
				return null;
			}
			if (OBJ("pubport_"+i).value=="")
			{
				BODY.ShowAlert("<?echo i18n("Please input the port range of firewall !");?>");
				OBJ("pubport_"+i).focus();
				return null;
			}
			if (OBJ("pubport_"+i).value!="" && !check_public_port(OBJ("pubport_"+i).value))
			{
				BODY.ShowAlert("<?echo i18n("Invalid Firewall port setting !");?>");
				OBJ("pubport_"+i).focus();
				return null;
			}
			// check same rule exist or not
			for(j=1; j < i; j++)
			{
				var name = OBJ("name_"+i).value;
				if( OBJ("priport_"+i).value === OBJ("priport_"+j).value &&
					OBJ("pripro_"+i).value === OBJ("pripro_"+j).value   &&
					OBJ("pubport_"+i).value === OBJ("pubport_"+j).value &&
					OBJ("pubpro_"+i).value === OBJ("pubpro_"+j).value &&
					OBJ("sch_"+i).value === OBJ("sch_"+j).value	  )
				{
					BODY.ShowAlert('<?echo i18n("The Rule ");?>"'+name+'\"<?echo i18n(" is already existed!");?>');
					OBJ("priport_"+i).focus();
					return null;
				}				
			}
			
			cur_count+=1;
			var b = p+"/entry:"+cur_count;
			XS(b+"/enable",			OBJ("en_"+i).checked ? "1" : "0");
			XS(b+"/schedule",		(OBJ("sch_"+i).value==="-1") ? "" : OBJ("sch_"+i).value);
			XS(b+"/description",	OBJ("name_"+i).value);
			var d = b + "/trigger";
			XS(d + "/protocol",		OBJ("pripro_" + i).value);
			var str = OBJ("priport_" + i).value.split("-");
			XS(d + "/start",		str[0]);
			XS(d + "/end",			str.length == 1 ? "":str[1]);
			d = b + "/external";
			XS(d + "/protocol",		OBJ("pubpro_" + i).value);
			XS(d + "/portlist", 	OBJ("pubport_" +i).value);
		}
		/* we only handle 'count' here, the 'seqno' and 'uid' will handle by setcfg.
		 * so DO NOT modified/generate 'seqno' and 'uid' here. */
		XS(p+"/count", cur_count);
		return PXML.doc;

	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	apps: [	{name: "<?echo i18n("Application Name");?>",
										protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"",		pub:""}},
			{name: "AIM Talk",		    protocol:{pri:"TCP"		,pub:"TCP"},	 port:{ pri:"4099",	pub:"5190"}},
			{name: "BitTorrent",		protocol:{pri:"TCP"		,pub:"TCP"},	 port:{ pri:"6969",	pub:"6881-6889" }},
			{name: "Calista IP Phone",	protocol:{pri:"TCP"		,pub:"UDP"}, 	 port:{ pri:"5190",	pub:"3000" }},
			{name: "ICQ",	            protocol:{pri:"UDP"		,pub:"TCP"}, 	 port:{ pri:"4000", pub:"20000,20019,20039,20059" }},
			{name: "PalTalk",		    protocol:{pri:"TCP"		,pub:"TCP+UDP"}, port:{ pri:"5001-5020",pub:"2090,2091,2095" }}			
			/* old style							
			{name: "Battle.net",		protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"6112",	pub:"6112"}},
			{name: "Dialpad",			protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"7175",	pub:"51200-51201,51210" }},
			{name: "ICU II",			protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"2019",	pub:"2000-2038,2050-2051,2069,2085,3010-3030" }},
			{name: "MSN Gaming Zone",	protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"47624",pub:"2300-2400,28800-29000" }},
			{name: "PC-to-Phone",		protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"12053",pub:"12120,12122,24150-24220" }},
			{name: "Quick Time 4",		protocol:{pri:"TCP+UDP"	,pub:"TCP+UDP"}, port:{ pri:"554",	pub:"6970-6999" }}
			*/
		  ],
	CursorFocus: function(node)
	{
		var i = node.lastIndexOf("entry:");
		if(node.charAt(i+7)==="/") var idx = parseInt(node.charAt(i+6), 10);
		else var idx = parseInt(node.charAt(i+6), 10)*10 + parseInt(node.charAt(i+7), 10);
		var indx = 1;
		var valid_name_cnt = 0;		
		for(indx=1; indx <= <?=$APP_MAX_COUNT?>; indx++)
		{
			if(OBJ("name_"+indx).value!=="") valid_name_cnt++;
			if(valid_name_cnt===idx) break;
		}
		if(node.match("description"))	OBJ("name_"+indx).focus();
		else if(node.match("trigger"))	OBJ("priport_"+indx).focus();
		else if(node.match("portlist"))	OBJ("pubport_"+indx).focus();
	}	  
};

function OnClickAppArrow(idx)
{
	var i = OBJ("app_"+idx).value;
	
	if( i === "0" )
	{
		BODY.ShowAlert("<?echo i18n("Please select an Application Name first !");?>");
		return false;
	}
	
	OBJ("name_"+idx).value		= PAGE.apps[i].name;
	OBJ("pubpro_"+idx).value	= PAGE.apps[i].protocol.pub;
	OBJ("pripro_"+idx).value	= PAGE.apps[i].protocol.pri;
	OBJ("pubport_"+idx).value	= PAGE.apps[i].port.pub;
	OBJ("priport_"+idx).value	= PAGE.apps[i].port.pri;
}

function CheckPort(port)
{
	var vals = port.toString().split("-");
	switch (vals.length)
	{
	case 1:
		if (!TEMP_IsDigit(vals))
			return false;
		break;
	case 2:
		if (!TEMP_IsDigit(vals[0])||!TEMP_IsDigit(vals[1]))
			return false;
		break;
	default:
		return false;
	}
	return true;
}
function check_trigger_port(list)
{
	var port = list.split(",");

	if (port.length > 1)  //  Trigger port can have just one port_range;
		return false;
	else
		return CheckPort(port);
}
function check_public_port(list)
{
	var port = list.split(",");

	if (port.length > 1)
	{
		for (var i=0; i<port.length; i++)
		{
			if (!CheckPort(port[i]))
				return false;
		}
		return true;
	}
	else
	{
		return CheckPort(port);
	}
}
</script>
