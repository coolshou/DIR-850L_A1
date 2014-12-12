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
	services: "ROUTE6.STATIC",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	//OnSubmitCallback: function(code, result) { return false; },
	OnSubmitCallback: function(code, result) 
	{
		/* reboot */
		if(code=="OK")
		{
			Service("REBOOT"); 
			return true; 
		}
		return false; 
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("ROUTE6.STATIC");
		if (p === "") { alert("InitValues() ERROR !"); return false; }
		p += "/route6/static";

		var max = XG(p+"/max");
		if(max == "") max=10;		
		//TEMP_RulesCount(p, "rmd");
		/* load rules into table */
		var count = XG(p+"/count");
		for (var i=1; i<=<?=$ROUTING6_MAX_COUNT?>; i++)
		{
			var b = p+"/entry:"+i;
			//OBJ("uid_"+i).value		= XG(b+"/uid");
			OBJ("dsc_"+i).value		= XG(b+"/description");
			OBJ("enable_"+i).checked	= (XG(b+"/enable") == "1") ? true:false;
			OBJ("dest1_"+i).value		= XG(b+"/network");
			//OBJ("dest2_"+i).value		= XG(b+"/prefix");

			if(XG(b+"/prefix") != "") 
			{
				OBJ("dest2_"+i).value		= XG(b+"/prefix");
			}
			else
			{
				OBJ("dest2_"+i).value		= 64;
			}

			if(XG(b+"/inf") != "")		OBJ("inf_"+i).value = XG(b+"/inf");
			else				OBJ("inf_"+i).value = "NULL";
			this.OnChangeinf(i);	
			OBJ("gateway_"+i).value		= XG(b+"/via");
			OBJ("metric_"+i).value		= XG(b+"/metric");
		}
		return true;
	},
	PreSubmit: function()
	{
		
		var p = PXML.FindModule("ROUTE6.STATIC");
		p += "/route6/static";
		var old_count = XG(p + "/count");
		var cur_count = 0;
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p + "entry:" + old_count);
			old_count--;
		}
		
		for(var i = 1;i <= <?=$ROUTING6_MAX_COUNT?>;i++)
		{
			var en		= OBJ("enable_"+i).checked ? "1" : "0";
			var dsc		= OBJ("dsc_"+i).value;
			var inf		= OBJ("inf_"+i).value;
			var network	= OBJ("dest1_"+i).value;
			var pfxlen	= OBJ("dest2_"+i).value;
			var gw		= OBJ("gateway_"+i).value;
			var mtrc	= OBJ("metric_"+i).value;
			//if(en === "1" || network != "" || pfxlen != "" || gw != "")
			if(en === "1" || network != "" || gw != "")
			{
				cur_count++;
				var b = p + "/entry:" + cur_count;
				XS(b + "/uid",		"SRT-"+cur_count);
				XS(b + "/description",	dsc);
				XS(b + "/enable",	en);
				XS(b + "/inf",		inf);
				XS(b + "/network",	network);
				XS(b + "/prefix",	pfxlen);
				XS(b + "/via",		gw);
				XS(b + "/metric",	mtrc);
			}
		}
		
		XS(p + "/count", cur_count);
		//PXML.doc.dbgdump();
		return PXML.doc;
	},
	OnChangeinf: function(index)
	{
		var inf		= OBJ("inf_"+index).value;
		if(inf==="PD")
		{
			OBJ("gateway_"+index).disabled = true;
			OBJ("metric_"+index).disabled = true;
		}
		else
		{
			OBJ("gateway_"+index).disabled = false;
			OBJ("metric_"+index).disabled = false;
		}
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
		var valid_dsct_cnt = 0;		
		for(indx=1; indx <= <?=$ROUTING6_MAX_COUNT?>; indx++)
		{
			if(OBJ("dest1_"+indx).value!=="") valid_dsct_cnt++;
			if(valid_dsct_cnt===idx) break;
		}
		if(node.match("network"))		OBJ("dest1_"+indx).focus();
		else if(node.match("prefix"))	OBJ("dest2_"+indx).focus();
		else if(node.match("via"))		OBJ("gateway_"+indx).focus();
	}	  	
};

function Service(svc)
{	
	var banner = "<?echo i18n("Rebooting");?>...";
	var msgArray = ["<?echo i18n("Reboot...");?>"];
	var delay = 10;
	var sec = <?echo query("/runtime/device/bootuptime");?> + delay;
	var url = null;
	var ajaxObj = GetAjaxObj("SERVICE");
	if (svc=="FRESET")		url = "http://192.168.0.1/index.php";
	else if (svc=="REBOOT")	url = "http://<?echo $_SERVER["HTTP_HOST"];?>/index.php";
	else					return false;
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowAlert("Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		else
			BODY.ShowCountdown(banner, msgArray, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
</script>
