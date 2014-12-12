<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
select.broad	{ width: 120px; }
select.narrow	{ width: 65px; }
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	<?
		if(isfile("/etc/services/UPNPC.php")==1) echo 'services: "VSVR.NAT-1,UPNPC",';
		else echo 'services: "VSVR.NAT-1",';
	?>	
	OnLoad: function()
	{
		/* draw the 'Application Name' select */
		var str = "";
		for(var i=1; i<=<?=$VSVR_MAX_COUNT?>; i+=1)
		{
			str = "";
			str += '<select id="app_'+i+'" class="broad">';
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
		var p = PXML.FindModule("VSVR.NAT-1");
		if (p === "") alert("ERROR!");
		p += "/nat/entry/virtualserver";
		TEMP_RulesCount(p, "rmd");
		var count = XG(p+"/count");
		var netid = COMM_IPv4NETWORK(this.lanip, this.mask);
		for (var i=1; i<=<?=$VSVR_MAX_COUNT?>; i+=1)
		{
			var b = p+"/entry:"+i;
			OBJ("uid_"+i).value = XG(b+"/uid");
			OBJ("en_"+i).checked = XG(b+"/enable")==="1";
			OBJ("dsc_"+i).value = XG(b+"/description");
			if (XG(b+"/protocol")!=="")	OBJ("pro_"+i).value = XG(b+"/protocol");
			OBJ("pubport_"+i).value = (XG(b+"/external/start")==="") ? "" : XG(b+"/external/start");
			OBJ("priport_"+i).value = (XG(b+"/internal/start")==="") ? "" : XG(b+"/internal/start");
			COMM_SetSelectValue(OBJ("pro_"+i), (XG(b+"/protocol")=="")? "TCP+UDP":XG(b+"/protocol"));
			this.OnClickProtocal(i);
			OBJ("pronum_"+i).value = XG(b+"/protocolnum");
			<?
			if ($FEATURE_NOSCH!="1")	echo 'COMM_SetSelectValue(OBJ("sch_"+i), (XG(b+"/schedule")=="")? "-1":XG(b+"/schedule"));\n';
			if ($FEATURE_INBOUNDFILTER=="1")	echo 'COMM_SetSelectValue(OBJ("inbfilter_"+i), (XG(b+"/inbfilter")=="")? "-1":XG(b+"/inbfilter"));\n';
			?>
			var hostid = XG(b+"/internal/hostid");
			if (hostid !== "")	OBJ("ip_"+i).value = COMM_IPv4IPADDR(netid, this.mask, hostid);
			else				OBJ("ip_"+i).value = "";
			OBJ("pc_"+i).value = "";
		}
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("VSVR.NAT-1");
		p += "/nat/entry/virtualserver";
		var old_count = parseInt(XG(p+"/count"), 10);
		var cur_count = 0;
		var cur_seqno = parseInt(XG(p+"/seqno"), 10);
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p+"/entry:"+old_count);
			old_count -= 1;
		}
		/*Error check and update the entries */
		for (var i=1; i<=<?=$VSVR_MAX_COUNT?>; i+=1)
		{
			if (OBJ("pubport_"+i).value!="" && !TEMP_IsDigit(OBJ("pubport_"+i).value))
			{
				BODY.ShowAlert("<?echo I18N("j", "The input public port is invalid.");?>");
				OBJ("pubport_"+i).focus();
				return null;
			}
			if (OBJ("priport_"+i).value!="" && !TEMP_IsDigit(OBJ("priport_"+i).value))
			{
				BODY.ShowAlert("<?echo I18N("j", "The input private port is invalid.");?>");
				OBJ("priport_"+i).focus();
				return null;
			}
			if (OBJ("ip_"+i).value!="" && !TEMP_CheckNetworkAddr(OBJ("ip_"+i).value, null, null))
			{
				BODY.ShowAlert("<?echo I18N("j", "Invalid host IP address.");?>");
				OBJ("ip_"+i).focus();
				return null;
			}
			if (OBJ("pubport_"+i).value=="" && OBJ("priport_"+i).value=="" && OBJ("dsc_"+i).value!="" && OBJ("pro_"+i).value!="Other")
			{
				BODY.ShowAlert("<?echo I18N("j", "Invalid Port !.");?>");
				OBJ("pubport_"+i).focus();
				return null;
			}
			if (OBJ("pronum_"+i).value!="" && !TEMP_IsDigit(OBJ("pronum_"+i).value))
			{
				BODY.ShowAlert("<?echo I18N("h", "The input protocol number is invalid.");?>");
				OBJ("pronum_"+i).focus();
				return null;
			}
			// Make sure the different rules have different names and public ports.
			// Make sure the rules could not be the same.
			for (var j=1; j < i; j+=1)
			{
				if(OBJ("dsc_"+i).value==OBJ("dsc_"+j).value && OBJ("dsc_"+i).value!="") 
				{
					BODY.ShowAlert("<?echo I18N("j", "The different rules could not set the same name.");?>");
					OBJ("dsc_"+j).focus();
					return null;
				}	
				if(OBJ("pubport_"+i).value==OBJ("pubport_"+j).value && OBJ("pubport_"+i).value!="") 
				{
					BODY.ShowAlert("<?echo I18N("j", "The different rules could not set the same public port.");?>");
					OBJ("pubport_"+j).focus();
					return null;
				}
				if(OBJ("ip_"+i).value==OBJ("ip_"+j).value && OBJ("pro_"+i).value=="Other" && 
					OBJ("pro_"+j).value=="Other" && OBJ("pronum_"+i).value==OBJ("pronum_"+j).value) 
				{
					BODY.ShowAlert("<?echo I18N("j", "The rules could not be the same.");?>");
					OBJ("pronum_"+j).focus();
					return null;
				}
				if(OBJ("pro_"+i).value=="Other" && OBJ("pro_"+j).value=="Other" && OBJ("pronum_"+i).value==OBJ("pronum_"+j).value) 
				{
					BODY.ShowAlert("<?echo I18N("j", "The different rules could not set the same protocol number.");?>");
					OBJ("pronum_"+j).focus();
					return null;
				}
				if(OBJ("pro_"+i).value=="Other" && OBJ("pronum_"+i).value==6 && OBJ("pro_"+j).value=="TCP") 
				{
					BODY.ShowAlert("<?echo I18N("j", "All TCP ports are reserved for the rule which protocol number is 6 with entire public and private ports.");?>");
					OBJ("pronum_"+i).focus();
					return null;
				}
				if(OBJ("pro_"+j).value=="Other" && OBJ("pronum_"+j).value==6 && OBJ("pro_"+i).value=="TCP") 
				{
					BODY.ShowAlert("<?echo I18N("j", "All TCP ports are reserved for the rule which protocol number is 6 with entire public and private ports.");?>");
					OBJ("pronum_"+j).focus();
					return null;
				}
				if(OBJ("pro_"+i).value=="Other" && OBJ("pronum_"+i).value==17 && OBJ("pro_"+j).value=="UDP") 
				{
					BODY.ShowAlert("<?echo I18N("j", "All UDP ports are reserved for the rule which protocol number is 17 with entire public and private ports.");?>");
					OBJ("pronum_"+i).focus();
					return null;
				}
				if(OBJ("pro_"+j).value=="Other" && OBJ("pronum_"+j).value==17 && OBJ("pro_"+i).value=="UDP") 
				{
					BODY.ShowAlert("<?echo I18N("j", "All UDP ports are reserved for the rule which protocol number is 17 with entire public and private ports.");?>");
					OBJ("pronum_"+j).focus();
					return null;
				}																				
			}			
			
			/* if the description field is empty, it means to remove this entry,
			 * so skip this entry. */
			if (OBJ("dsc_"+i).value!=="")
			{
				cur_count+=1;
				var b = p+"/entry:"+cur_count;
				XS(b+"/enable",			OBJ("en_"+i).checked ? "1" : "0");
				XS(b+"/uid",			OBJ("uid_"+i).value);
				if (OBJ("uid_"+i).value == "")
				{
					XS(b+"/uid",	"VSVR-"+cur_seqno);
					cur_seqno += 1;
				}
				XS(b+"/description",	OBJ("dsc_"+i).value);
				XS(b+"/external/start",	OBJ("pubport_"+i).value);
				XS(b+"/protocol",		OBJ("pro_"+i).value);
				XS(b+"/protocolnum",	OBJ("pronum_"+i).value);
				<?
				if ($FEATURE_NOSCH!="1")	echo 'XS(b+"/schedule",		(OBJ("sch_"+i).value==="-1") ? "" : OBJ("sch_"+i).value);\n';
				if ($FEATURE_INBOUNDFILTER=="1")	echo 'XS(b+"/inbfilter",	(OBJ("inbfilter_"+i).value==="-1") ? "" : OBJ("inbfilter_"+i).value);\n';
				?>
				XS(b+"/internal/inf",	"LAN-1");
				if (OBJ("ip_"+i).value == "") XS(b+"/internal/hostid", "");
				else XS(b+"/internal/hostid",COMM_IPv4HOST(OBJ("ip_"+i).value, this.mask));
				XS(b+"/internal/start",	OBJ("priport_"+i).value);
			}
			
			//Check the port confliction between STORAGE, VSVR, PFWD and REMOTE.
			if(OBJ("pubport_"+i).value!="" && (OBJ("pro_"+i).value=="TCP" || OBJ("pro_"+i).value=="TCP+UDP"))
			{
				var admin_remote_port = "<? echo query(INF_getinfpath("WAN-1")."/web");?>";
				var admin_remote_port_https = "<? echo query(INF_getinfpath("WAN-1")."/https_rport");?>";
				if(OBJ("pubport_"+i).value==admin_remote_port || OBJ("pubport_"+i).value==admin_remote_port_https)
				{
					BODY.ShowAlert("<?echo i18n("The input public port could not be the same as the remote admin port in the administration function.");?>");
					OBJ("pubport_"+i).focus();
					return null;
				}
				var webaccess_remote_en = <? if(query("/webaccess/enable")==1 && query("/webaccess/remoteenable")==1) echo "true" ;else echo "false";?>;
				var webaccess_httpport = "<? echo query("/webaccess/httpport");?>";
				var webaccess_httpsport = "<? echo query("/webaccess/httpsport");?>";
				if(webaccess_remote_en && (OBJ("pubport_"+i).value==webaccess_httpport || OBJ("pubport_"+i).value==webaccess_httpsport))
				{
					BODY.ShowAlert("<?echo i18n("The input public port could not be the same as the web access port in the storage function.");?>");
					OBJ("pubport_"+i).focus();
					return null;	
				}
			}						
		}
		XS(p+"/count", cur_count);
		XS(p+"/seqno", cur_seqno);
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	apps: [	{name: "<?echo I18N("h", "Application name");?>",
									protocol:"TCP", port:{ pri:"0",		pub:"0"}},
			{name: "TELNET",		protocol:"TCP", port:{ pri:"23",	pub:"23" }},
			{name: "HTTP",			protocol:"TCP", port:{ pri:"80",	pub:"80" }},
			{name: "HTTPS",			protocol:"TCP", port:{ pri:"443",	pub:"443" }},
			{name: "FTP",			protocol:"TCP", port:{ pri:"21",	pub:"21" }},
			{name: "DNS",			protocol:"UDP", port:{ pri:"53",	pub:"53" }},
			{name: "SMTP",			protocol:"TCP", port:{ pri:"25",	pub:"25" }},
			{name: "POP3",			protocol:"TCP", port:{ pri:"110",	pub:"110" }},
			{name: "H.323",			protocol:"TCP", port:{ pri:"1720",	pub:"1720" }},
			{name: "REMOTE DESKTOP",protocol:"TCP", port:{ pri:"3389",	pub:"3389" }},
			{name: "PPTP",			protocol:"TCP", port:{ pri:"1723",	pub:"1723" }},
			{name: "L2TP",			protocol:"UDP", port:{ pri:"1701",	pub:"1701" }},
			{name: "Wake-On-Lan",	protocol:"UDP", port:{ pri:"9",		pub:"9" }}
		  ],
	lanip:	"<? echo INF_getcurripaddr("LAN-1"); ?>",
	mask:	"<? echo INF_getcurrmask("LAN-1"); ?>",
	OnClickAppArrow: function(idx)
	{
		var i = OBJ("app_"+idx).value;
		OBJ("dsc_"+idx).value = (i==="0") ? "" : PAGE.apps[i].name;
		OBJ("pro_"+idx).value = (PAGE.apps[i].protocol==="") ? "TCP+UDP" : PAGE.apps[i].protocol;
		OBJ("pubport_"+idx).value	= (PAGE.apps[i].port.pub==="") ? "0" : PAGE.apps[i].port.pub;
		OBJ("priport_"+idx).value	= (PAGE.apps[i].port.pri==="") ? "0" : PAGE.apps[i].port.pri;
		this.OnClickProtocal(idx);
	},
	OnClickPCArrow: function(idx)
	{
		OBJ("ip_"+idx).value = OBJ("pc_"+idx).value;
	},
	CursorFocus: function(node)
	{
		var i = node.lastIndexOf("entry:");
		if(node.charAt(i+7)==="/") var idx = parseInt(node.charAt(i+6), 10);
		else var idx = parseInt(node.charAt(i+6), 10)*10 + parseInt(node.charAt(i+7), 10);
		var indx = 1;
		var valid_dsc_cnt = 0;		
		for(indx=1; indx <= <?=$VSVR_MAX_COUNT?>; indx++)
		{
			if(OBJ("dsc_"+indx).value!=="") valid_dsc_cnt++;
			if(valid_dsc_cnt===idx) break;
		}	
		if(node.match("description"))			OBJ("dsc_"+indx).focus();
		else if(node.match("internal/hostid"))	OBJ("ip_"+indx).focus();
		else if(node.match("external/start"))	OBJ("pubport_"+indx).focus();
		else if(node.match("internal/start"))	OBJ("priport_"+indx).focus();
		else if(node.match("protocolnum"))		OBJ("pronum_"+indx).focus();	
	},
	OnClickProtocal: function(idx)
	{
		var pro_value = OBJ("pro_"+idx).value;
		if(pro_value=="Other")
		{
			OBJ("pubport_"+idx).value = "";
			OBJ("priport_"+idx).value = "";
			OBJ("pubport_"+idx).disabled = true;
			OBJ("priport_"+idx).disabled = true;
			OBJ("pronum_"+idx).disabled = false;
			OBJ("pronum_"+idx).value = "";
		}
		else
		{
			OBJ("pubport_"+idx).disabled = false;
			OBJ("priport_"+idx).disabled = false;
			OBJ("pronum_"+idx).disabled = true;
			if(pro_value=="TCP")
				OBJ("pronum_"+idx).value = "6";
			else if(pro_value=="UDP")
				OBJ("pronum_"+idx).value = "17";
			else if(pro_value=="TCP+UDP")
				OBJ("pronum_"+idx).value = "256";
		}
	}
};

</script>
