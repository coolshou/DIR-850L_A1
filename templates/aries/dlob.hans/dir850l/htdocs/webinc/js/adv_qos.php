<?include "/htdocs/phplib/inet.php";?>
<?include "/htdocs/phplib/inf.php";?>
<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "BWC, INET.INF",
	OnLoad: function()
	{		
		for (var i=1; i<=<?=$QOS_MAX_COUNT?>; i+=1)
		{
			OBJ("en_"+i).checked			= false;
			OBJ("dsc_"+i).value				= "";
			OBJ("pro_"+i).value				= "";
			OBJ("src_startip_"+i).value		= "";
			OBJ("src_endip_"+i).value		= "";				
			OBJ("dst_startip_"+i).value		= "";
			OBJ("dst_endip_"+i).value		= "";
			OBJ("app_port_"+i).value		= "";
		}
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			Service("REBOOT");								
			break;
		case "BUSY":
			BODY.ShowAlert("<?echo i18n("Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			if (result.Get("/hedwig/result")=="FAILED")
			{
				FocusObj(result);
				BODY.ShowAlert(result.Get("/hedwig/message"));
			}
			break;
		case "PIGWIDGEON":
			BODY.ShowAlert(result.Get("/pigwidgeon/message"));
			break;
		}
		return true;
	},
	InitValue: function(xml)
	{		
		PXML.doc = xml;
		bwc = PXML.FindModule("BWC");					
		var inet = PXML.FindModule("INET.INF");
		PXML.IgnoreModule("INET.INF"); 
		if (!inet) { alert("InitValue ERROR!"); return false; }
																				
		if (this.activewan==="")
		{
			BODY.ShowAlert("<?echo I18N("j", "There is no interface can access the Internet!  Please check the cable, and the Internet settings!");?>");
			return false;
		}
		if (bwc === "")		{ alert("InitValue ERROR!"); return false; }		
		OBJ("en_qos").checked = (XG(bwc+"/bwc/entry:1/enable")==="1" && XG(bwc+"/runtime/device/layout")==="router");
		OBJ("en_qos").disabled =(XG(bwc+"/runtime/device/layout")==="bridge");
		TEMP_RulesCount(bwc+"/bwc/bwcf", "rmd"); /*marco*/
		/*If bwc1 is in WAN interface, bwc1 is for uplink
		  If bwc1 is in LAN interface, bwc1 is for downlink*/
		var bwc1		= XG(bwc+"/bwc/entry:1/uid");
		var bwc1infp	= GPBT(inet, "inf", "bwc", bwc1, false);
		var inetinf		= XG(bwc1infp+"/uid");
		if(inetinf.substr(0,3) === "WAN") 
		{
			OBJ("upstream").value	= XG(bwc+"/bwc/entry:1/bandwidth");
			OBJ("downstream").value	= XG(bwc+"/bwc/entry:2/bandwidth");
			this.bwc1link = "up";
		}
		else if(inetinf.substr(0,3) === "LAN")
		{
			OBJ("upstream").value	= XG(bwc+"/bwc/entry:2/bandwidth");
			OBJ("downstream").value	= XG(bwc+"/bwc/entry:1/bandwidth");
			this.bwc1link = "down";
		}		
		
		bwcqd1p	= GPBT(bwc+"/bwc/bwcqd", "entry", "uid", "BWCQD-1", false);
		bwcqd2p	= GPBT(bwc+"/bwc/bwcqd", "entry", "uid", "BWCQD-2", false);
		bwcqd3p	= GPBT(bwc+"/bwc/bwcqd", "entry", "uid", "BWCQD-3", false);
		bwcqd4p	= GPBT(bwc+"/bwc/bwcqd", "entry", "uid", "BWCQD-4", false);
		if(XG(bwc+"/bwc/entry:1/flag") === "TC_SPQ")
		{
			OBJ("Qtype_SPQ").checked	=	true;
			OBJ("Qtype_WFQ").checked	=	false;
			this.OnClickQtype("SPQ");
		}
		else
		{
			OBJ("Qtype_SPQ").checked	=	false;
			OBJ("Qtype_WFQ").checked	=	true;
			this.OnClickQtype("WFQ");
		}			
		
		var bwc1cnt = S2I(XG(bwc+"/bwc/entry:1/rules/count"));
		for (var j=1; j<=bwc1cnt; j+=1)
		{
			if (XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/enable")==="1")	OBJ("en_"+j).checked=true;
			else	OBJ("en_"+j).checked=false;	
			OBJ("dsc_"+j).value	=	XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/description");
			if(XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/bwcqd")==="BWCQD-1")		OBJ("pri_"+j).value	="VO";	
			else if(XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/bwcqd")==="BWCQD-2")	OBJ("pri_"+j).value	="VI";			
			else if(XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/bwcqd")==="BWCQD-3")	OBJ("pri_"+j).value	="BG";
			else if(XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/bwcqd")==="BWCQD-4")	OBJ("pri_"+j).value	="BE";		
			
			var bwcf = XG(bwc+"/bwc/entry:1/rules/entry:"+j+"/bwcf");
			var bwcfp = GPBT(bwc+"/bwc/bwcf", "entry", "uid", bwcf, false);
			OBJ("pro_"+j).value	= XG(bwcfp+"/protocol");
			OBJ("src_startip_"+j).value		= XG(bwcfp+"/ipv4/start");
			OBJ("src_endip_"+j).value		= XG(bwcfp+"/ipv4/end");				
			OBJ("dst_startip_"+j).value		= XG(bwcfp+"/dst/ipv4/start");
			OBJ("dst_endip_"+j).value		= XG(bwcfp+"/dst/ipv4/end");
			if(XG(bwcfp+"/dst/port/type")==="1")	OBJ("app_port_"+j).value = XG(bwcfp+"/dst/port/name");	
			else	OBJ("app_port_"+j).value = XG(bwcfp+"/dst/port/start");							
		}	
		this.OnClickQOSEnable();
		return true;
	},
	
	PreSubmit: function()
	{		
		if (this.activewan==="")
		{
			BODY.ShowAlert("<?echo I18N("j", "There is no interface can access the Internet!  Please check the cable, and the Internet settings!");?>");
			return null;
		}
		
		if (!TEMP_IsDigit(OBJ("upstream").value) || OBJ("upstream").value < 1)
		{			
			BODY.ShowAlert("<?echo I18N("j", "The input uplink speed is invalid.");?>");
			OBJ("upstream").focus();
			return null;
		}
		if (!TEMP_IsDigit(OBJ("downstream").value) || OBJ("downstream").value < 1)
		{			
			BODY.ShowAlert("<?echo I18N("j", "The input downlink speed is invalid.");?>");
			OBJ("downstream").focus();
			return null;
		}
		
		if(OBJ("Qtype_WFQ").checked === true)
		{
			for (var i=1; i<=4; i+=1)
			{
				if (!TEMP_IsDigit(OBJ("priority_dsc"+i).value) || parseInt(OBJ("priority_dsc"+i).value, 10) < 1)
				{			
					BODY.ShowAlert("<?echo I18N("j", "The input queue weight is invalid.");?>");
					OBJ("priority_dsc"+i).focus();
					return null;
				}
			}
			var queue_weight_total = parseInt(OBJ("priority_dsc1").value, 10)+parseInt(OBJ("priority_dsc2").value, 10)+parseInt(OBJ("priority_dsc3").value, 10)+parseInt(OBJ("priority_dsc4").value, 10);
			if (queue_weight_total !== 100)
			{			
				BODY.ShowAlert("<?echo I18N("j", "The total amount of queue weight should be");?>"+"100%.");
				OBJ("priority_dsc1").focus();
				return null;
			}
		}           
        
		/* If one of the local IP is empty, fill it with the other. The same way about destination IP.*/    		
		for(var i=1; i <= <?=$QOS_MAX_COUNT?>; i++)
		{
			if(OBJ("src_startip_"+i).value !== "" && OBJ("src_endip_"+i).value ==="") OBJ("src_endip_"+i).value=OBJ("src_startip_"+i).value;
			else if(OBJ("src_startip_"+i).value === "" && OBJ("src_endip_"+i).value !=="") OBJ("src_startip_"+i).value=OBJ("src_endip_"+i).value;
			if(OBJ("dst_startip_"+i).value !== "" && OBJ("dst_endip_"+i).value ==="") OBJ("dst_endip_"+i).value=OBJ("dst_startip_"+i).value;
			else if(OBJ("dst_startip_"+i).value === "" && OBJ("dst_endip_"+i).value !=="") OBJ("dst_startip_"+i).value=OBJ("dst_endip_"+i).value;			
		}
			            		
		for(var i=1; i <= <?=$QOS_MAX_COUNT?>; i++)
		{		
			if(OBJ("dsc_"+i).value !== "")
			{
				for(var j=1; j <= <?=$QOS_MAX_COUNT?>; j++)
				{
					if(OBJ("dsc_"+j).value !== "")
					{
						if(i!==j && OBJ("dsc_"+i).value===OBJ("dsc_"+j).value)
						{
							BODY.ShowAlert("<?echo I18N("j","The 'Name' could not be the same.");?>");
							OBJ("dsc_"+i).focus();
							return null;			
						}
						if(i!==j && OBJ("src_startip_"+i).value===OBJ("src_startip_"+j).value && OBJ("src_endip_"+i).value===OBJ("src_endip_"+j).value 
							&& OBJ("dst_startip_"+i).value===OBJ("dst_startip_"+j).value && OBJ("dst_endip_"+i).value===OBJ("dst_endip_"+j).value)
						{
							if(OBJ("app_port_"+i).value===OBJ("app_port_"+j).value || OBJ("app_port_"+i).value==="ALL" || OBJ("app_port_"+j).value==="ALL")
							{	
								if(OBJ("pro_"+i).value===OBJ("pro_"+j).value || OBJ("pro_"+i).value==="ALL" || OBJ("pro_"+j).value==="ALL")
								{
									BODY.ShowAlert("<?echo i18n("The rules could not be the same");?>");
									OBJ("src_startip_"+i).focus();
									return null;						
								}
							}	
						}
					}						
				}	
			}
		}		
		
		XS(bwc+"/bwc/entry:1/enable", OBJ("en_qos").checked?"1":"0");		
		if (this.bwc1link==="up")
		{	
			XS(bwc+"/bwc/entry:1/bandwidth", OBJ("upstream").value);
			XS(bwc+"/bwc/entry:2/bandwidth", OBJ("downstream").value);
		}	
		else if (this.bwc1link==="down")
		{
			XS(bwc+"/bwc/entry:1/bandwidth", OBJ("downstream").value);
			XS(bwc+"/bwc/entry:2/bandwidth", OBJ("upstream").value);			
		}					
			
		if (OBJ("Qtype_WFQ").checked === true)
		{
			XS(bwc+"/bwc/entry:1/flag", "TC_WFQ");
			XS(bwc+"/bwc/entry:2/flag", "TC_WFQ");
			XS(bwcqd1p+"/weight", OBJ("priority_dsc1").value);
			XS(bwcqd2p+"/weight", OBJ("priority_dsc2").value);
			XS(bwcqd3p+"/weight", OBJ("priority_dsc3").value);
			XS(bwcqd4p+"/weight", OBJ("priority_dsc4").value);
		}	
		else if (OBJ("Qtype_SPQ").checked === true)
		{
			XS(bwc+"/bwc/entry:1/flag", "TC_SPQ");
			XS(bwc+"/bwc/entry:2/flag", "TC_SPQ");
		}		
		
		/* if the description field is empty, it means to remove this entry,
		 * so skip this entry. */
		var bwcfn = 0;
		var bwcqd = null;
		for (var i=1; i<=<?=$QOS_MAX_COUNT?>; i+=1)
		{
			if (OBJ("en_"+i).checked && OBJ("dsc_"+i).value === "")
			{
				BODY.ShowAlert("<?echo I18N("j", "The 'Name' field can not be blank.");?>");
				OBJ("dsc_"+i).focus();
				return null;
			}
			if (OBJ("dsc_"+i).value !== "")
			{
				if (OBJ("pro_"+i).value!=="ALL" && OBJ("pro_"+i).value!=="TCP" && OBJ("pro_"+i).value!=="UDP")
				{
					BODY.ShowAlert("<?echo I18N("j", "The protocol is invalid.");?>");
					OBJ("pro_"+i).focus();
					return null;
				}
				
				if (!this.IPRangeCheck(i)) 	return null;
				if (!this.AppPortCheck(i))	return null;
				
				bwcfn++;
				XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/uid",	"BWCF-"+bwcfn);
				XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/protocol", OBJ("pro_"+i).value);
    			XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/ipv4/start",	OBJ("src_startip_"+i).value);
				XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/ipv4/end",	OBJ("src_endip_"+i).value);
				XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/ipv4/start", 	OBJ("dst_startip_"+i).value);
				XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/ipv4/end", 	OBJ("dst_endip_"+i).value);            		
							
				if (TEMP_IsDigit(OBJ("app_port_"+i).value))
				{
					XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/port/type", 	"0");					
					XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/port/start", 	OBJ("app_port_"+i).value);							
				}
				else
				{
					XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/port/type", 	"1");					
					XS(bwc+"/bwc/bwcf/entry:"+bwcfn+"/dst/port/name", 	OBJ("app_port_"+i).value);
				}		
				
				if(OBJ("pri_"+i).value==="VO")		bwcqd="BWCQD-1";
				else if(OBJ("pri_"+i).value==="VI")	bwcqd="BWCQD-2";			
				else if(OBJ("pri_"+i).value==="BG")	bwcqd="BWCQD-3";
				else if(OBJ("pri_"+i).value==="BE")	bwcqd="BWCQD-4";
				XS(bwc+"/bwc/entry:1/rules/entry:"+bwcfn+"/enable", OBJ("en_"+i).checked?"1":"0");
				XS(bwc+"/bwc/entry:1/rules/entry:"+bwcfn+"/description", OBJ("dsc_"+i).value);
				XS(bwc+"/bwc/entry:1/rules/entry:"+bwcfn+"/bwcqd", bwcqd);
				XS(bwc+"/bwc/entry:1/rules/entry:"+bwcfn+"/bwcf", "BWCF-"+bwcfn);		
				XS(bwc+"/bwc/entry:2/rules/entry:"+bwcfn+"/enable", OBJ("en_"+i).checked?"1":"0");					
				XS(bwc+"/bwc/entry:2/rules/entry:"+bwcfn+"/description", OBJ("dsc_"+i).value);
				XS(bwc+"/bwc/entry:2/rules/entry:"+bwcfn+"/bwcqd", bwcqd);
				XS(bwc+"/bwc/entry:2/rules/entry:"+bwcfn+"/bwcf", "BWCF-"+bwcfn);
			}	
		}	
		XS(bwc+"/bwc/entry:1/rules/count", bwcfn);
		XS(bwc+"/bwc/entry:2/rules/count", bwcfn);
		XS(bwc+"/bwc/bwcf/count", bwcfn);
		
		return PXML.doc;
	},
	bwc: null,
	bwcqd1p: null,
	bwcqd2p: null,
	bwcqd3p: null,
	bwcqd4p: null,
	bwc1link: null,
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: function()
	{	
		devmode = XG(bwc+"/runtime/device/layout");
		if(devmode == "bridge") return false;
		return true;
	},			
	activewan: function()
	{
		wan = XG(bwc+"/runtime/device/activewan");		
		return wan;				
	},			
	OnClickQOSEnable: function()
	{
		if (OBJ("en_qos").checked)
		{
			OBJ("upstream").disabled = OBJ("select_upstream").disabled = false;
			OBJ("downstream").disabled = OBJ("select_downstream").disabled = false;
			OBJ("Qtype_SPQ").disabled = OBJ("Qtype_WFQ").disabled =false;
			OBJ("queue_table").disabled = false;
			if(OBJ("Qtype_WFQ").checked) OBJ("priority_dsc1").disabled = OBJ("priority_dsc2").disabled = OBJ("priority_dsc3").disabled = OBJ("priority_dsc4").disabled = false;
			OBJ("qos_table").disabled =false;
			for (var i=1; i<=<?=$QOS_MAX_COUNT?>; i+=1)
			{
				OBJ("en_"+i).disabled =	OBJ("dsc_"+i).disabled = OBJ("pri_"+i).disabled = OBJ("pro_"+i).disabled = OBJ("select_pro_"+i).disabled = false;
				OBJ("src_startip_"+i).disabled = OBJ("src_endip_"+i).disabled = false;
				OBJ("dst_startip_"+i).disabled = OBJ("dst_endip_"+i).disabled = false;
				OBJ("app_port_"+i).disabled = OBJ("select_app_port_"+i).disabled = false;
			}	
		}
		else
		{
			OBJ("upstream").disabled = OBJ("select_upstream").disabled = true;
			OBJ("downstream").disabled = OBJ("select_downstream").disabled = true;
			OBJ("Qtype_SPQ").disabled = OBJ("Qtype_WFQ").disabled =true;
			OBJ("queue_table").disabled = true;
			if(OBJ("Qtype_WFQ").checked) OBJ("priority_dsc1").disabled = OBJ("priority_dsc2").disabled = OBJ("priority_dsc3").disabled = OBJ("priority_dsc4").disabled = true;
			OBJ("qos_table").disabled =true;
			for (var i=1; i<=<?=$QOS_MAX_COUNT?>; i+=1)
			{
				OBJ("en_"+i).disabled =	OBJ("dsc_"+i).disabled = OBJ("pri_"+i).disabled = OBJ("pro_"+i).disabled = OBJ("select_pro_"+i).disabled = true;	
				OBJ("src_startip_"+i).disabled = OBJ("src_endip_"+i).disabled = true;
				OBJ("dst_startip_"+i).disabled = OBJ("dst_endip_"+i).disabled = true;
				OBJ("app_port_"+i).disabled = OBJ("select_app_port_"+i).disabled = true;					
			}
		}
	},
	OnChangeQOSUpstream: function()
	{
		OBJ("upstream").value = OBJ("select_upstream").value;
		OBJ("select_upstream").value=0;
	},
	OnChangeQOSDownstream: function()
	{
		OBJ("downstream").value = OBJ("select_downstream").value;
		OBJ("select_downstream").value=0;
	},	
	OnClickQtype: function(Qtype)
	{
		if(Qtype === "SPQ")
		{	
			OBJ("priority").innerHTML = '<?echo I18N("j", "Queue Priority");?>';		
			OBJ("priority1").innerHTML = '<?echo I18N("h", "Highest");?>';
			OBJ("priority2").innerHTML = '<?echo I18N("h", "Higher");?>';
			OBJ("priority3").innerHTML = '<?echo I18N("h", "Normal");?>';
			OBJ("priority4").innerHTML = '<?echo I18N("h", "Best Effort");?>(<?echo I18N("h", "default");?>)';
		}
		else
		{
			OBJ("priority").innerHTML = '<?echo I18N("j", "Queue Weight");?>';
			OBJ("priority1").innerHTML = '<input id="priority_dsc1" type="text" size="3" maxlength="3" />%';
			OBJ("priority2").innerHTML = '<input id="priority_dsc2" type="text" size="3" maxlength="3" />%';
			OBJ("priority3").innerHTML = '<input id="priority_dsc3" type="text" size="3" maxlength="3" />%';
			OBJ("priority4").innerHTML = '<input id="priority_dsc4" type="text" size="3" maxlength="3" />%';					
			OBJ("priority_dsc1").value	=	XG(bwcqd1p+"/weight");
			OBJ("priority_dsc2").value	=	XG(bwcqd2p+"/weight");
			OBJ("priority_dsc3").value	=	XG(bwcqd3p+"/weight");
			OBJ("priority_dsc4").value	=	XG(bwcqd4p+"/weight");			
		}
	},	
	OnChangeProt: function(idx)
	{
		OBJ("pro_"+idx).value	=	OBJ("select_pro_"+idx).value;
	},
	OnChangeAppPort: function(idx)
	{
		OBJ("app_port_"+idx).value	=	OBJ("select_app_port_"+idx).value;
		this.OnChangeAppPortInput(idx);
	},
	OnChangeAppPortInput: function(idx)
	{
		if(OBJ("app_port_"+idx).value==="YOUTUBE" || OBJ("app_port_"+idx).value==="FTP" || OBJ("app_port_"+idx).value==="HTTP"
			|| OBJ("app_port_"+idx).value==="HTTP_AUDIO" || OBJ("app_port_"+idx).value==="HTTP_VIDEO" || OBJ("app_port_"+idx).value==="HTTP_DOWNLOAD")
		{
			OBJ("pro_"+idx).value = "TCP";
			OBJ("pro_"+idx).disabled = true;
			OBJ("select_pro_"+idx).disabled = true;			
		}
		else if(OBJ("app_port_"+idx).value==="P2P")
		{
			OBJ("pro_"+idx).value = "ALL";
			OBJ("pro_"+idx).disabled = true;
			OBJ("select_pro_"+idx).disabled = true;
		}	
		else	 	
		{
			OBJ("pro_"+idx).disabled = false;
			OBJ("select_pro_"+idx).disabled = false;			
		}	
	},
	IPRangeCheck: function(i)
	{
		var lan1ip 	= 	"<?$inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0); echo query($inf."/inet/ipv4/ipaddr");?>";
		var lan2ip 	= 	"<?$inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-2", 0); echo query($inf."/inet/ipv4/ipaddr");?>";
		var lan2mask = 	"<?$inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-2", 0); echo query($inf."/inet/ipv4/mask");?>";
		var router_mode = "<?echo query("/runtime/device/router/mode");?>";
		if(OBJ("src_startip_"+i).value !== "")
		{
			if(lan1ip === OBJ("src_startip_"+i).value || (router_mode === "1W2L" && lan2ip === OBJ("src_startip_"+i).value))
			{
				alert("<?echo I18N("j", "The IP Address could not be the same as LAN IP Address.");?>");
				OBJ("src_startip_"+i).focus();
				return false;
			}	
			if(!TEMP_CheckNetworkAddr(OBJ("src_startip_"+i).value) && (router_mode === "1W2L" && !TEMP_CheckNetworkAddr(OBJ("src_startip_"+i).value, lan2ip, lan2mask)))
			{	
				alert("<?echo I18N("j", "IP address should be in LAN subnet.");?>");
				OBJ("src_startip_"+i).focus();
				return false;
			}
		}
		if(OBJ("src_endip_"+i).value !== "")
		{
			if(lan1ip === OBJ("src_endip_"+i).value || (router_mode === "1W2L" && lan2ip === OBJ("src_endip_"+i).value))
			{
				alert("<?echo I18N("j", "The IP Address could not be the same as LAN IP Address.");?>");
				OBJ("src_endip_"+i).focus();
				return false;
			} 
			if(!TEMP_CheckNetworkAddr(OBJ("src_endip_"+i).value) && (router_mode === "1W2L" && !TEMP_CheckNetworkAddr(OBJ("src_endip_"+i).value, lan2ip, lan2mask)))
			{	
				alert("<?echo I18N("j", "IP address should be in LAN subnet.");?>");
				OBJ("src_endip_"+i).focus();
				return false;
			}
		}
		if(OBJ("src_startip_"+i).value !== "" && OBJ("src_endip_"+i).value !== "")
		{
			if(COMM_IPv4ADDR2INT(OBJ("src_startip_"+i).value) > COMM_IPv4ADDR2INT(OBJ("src_endip_"+i).value))
			{
				alert("<?echo I18N("j", "The end IP address should be greater than the start address.");?>");
				OBJ("src_startip_"+i).focus();
				return false;
			}
			if(!(TEMP_CheckNetworkAddr(OBJ("src_startip_"+i).value) && TEMP_CheckNetworkAddr(OBJ("src_endip_"+i).value)) &&     
				!(TEMP_CheckNetworkAddr(OBJ("src_startip_"+i).value, lan2ip, lan2mask) && TEMP_CheckNetworkAddr(OBJ("src_endip_"+i).value, lan2ip, lan2mask)))
			{	
				alert("<?echo I18N("j", "The start IP address and the end IP address should be in the same LAN subnet.");?>");
				OBJ("src_startip_"+i).focus();
				return false;
			}
		}
		
		if(OBJ("dst_startip_"+i).value !== "")
		{
			var DSIP_array	= OBJ("dst_startip_"+i).value.split(".");
			if (DSIP_array.length!==4 || OBJ("dst_startip_"+i).value === "0.0.0.0")
			{
				alert("<?echo I18N("j", "Incorrect Dest IP address. The start IP address is invalid.");?>");
				OBJ("dst_startip_"+i).focus();
				return false;
			}
			for (var j=0; j<4; j++)
			{
				if (!TEMP_IsDigit(DSIP_array[j]) || DSIP_array[j]>255)
				{
					alert("<?echo I18N("j", "Incorrect Dest IP address. The start IP address is invalid.");?>");
					OBJ("dst_startip_"+i).focus();
					return false;
				}
			}	
		}
		if(OBJ("dst_endip_"+i).value !== "")
		{
			var DEIP_array	= OBJ("dst_endip_"+i).value.split(".");
			if (DEIP_array.length!==4 || OBJ("dst_endip_"+i).value === "255.255.255.255")
			{
				alert("<?echo I18N("j", "Incorrect Dest IP address. The end IP address is invalid.");?>");
				OBJ("dst_endip_"+i).focus();
				return false;
			}
			for (var j=0; j<4; j++)
			{
				if (!TEMP_IsDigit(DEIP_array[j]) || DEIP_array[j]>255)
				{
					alert("<?echo I18N("j", "Incorrect Dest IP address. The end IP address is invalid.");?>");
					OBJ("dst_endip_"+i).focus();
					return false;
				}
			}
		}
		if(OBJ("dst_startip_"+i).value !== "" && OBJ("dst_endip_"+i).value !== "")
		{
			if(COMM_IPv4ADDR2INT(OBJ("dst_startip_"+i).value) > COMM_IPv4ADDR2INT(OBJ("dst_endip_"+i).value))
			{
				alert("<?echo I18N("j", "The end IP address should be greater than the start address.");?>");
				OBJ("dst_startip_"+i).focus();
				return false;
			}
		}
		
		return true;
	},
	AppPortCheck: function(i)
	{
		if(TEMP_IsDigit(OBJ("app_port_"+i).value))
		{
			if(parseInt(OBJ("app_port_"+i).value, 10) < 1 || parseInt(OBJ("app_port_"+i).value, 10) > 65535)
			{
				alert("<?echo I18N("j", "Invalid application port value.");?>");
				OBJ("app_port_"+i).focus();
				return false;
			}	
		}	
		else
		{
			var AppPortArray = ['ALL','YOUTUBE','VOICE','HTTP_AUDIO','HTTP_VIDEO','HTTP_DOWNLOAD','HTTP','FTP','P2P'];
			var NotAppPort = true;
			for(var j=0; j < AppPortArray.length; j++) if(OBJ("app_port_"+i).value === AppPortArray[j]) NotAppPort = false;
			if(NotAppPort === true)
			{
				alert("<?echo I18N("j", "Invalid application port value.");?>");
				OBJ("app_port_"+i).focus();
				return false;				
			}	
		}		
		
		return true;	
	}		
}

function Service(svc)
{	
	var banner = "<?echo i18n("Rebooting");?>...";
	var msgArray = ['<?echo I18N("j","QoS settings changed. Reboot device to take effect.");?>',
					'<?echo I18N("j","If you changed the IP address of the router you will need to change the IP address in your browser before accessing the configuration web page again.");?>'];
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
