<script type="text/javascript">
function Page() {}
Page.prototype =
{
	//services: "DDNS4.WAN-1, DDNS4.WAN-3, RUNTIME.DDNS4.WAN-1,DDNS6.WAN-1",    
	services: "DDNS6.WAN-1,DDNS4.WAN-1, DDNS4.WAN-3, RUNTIME.DDNS4.WAN-1",    
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}

	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) { return false; },
	org: null,
	cfg: null,
	g_table_index: 1,
	g_edit: 0,
	InitValue: function(xml)
	{
		if (this.org) delete this.org;
		if (this.cfg) delete this.cfg;
		this.org = new Array();
		this.cfg = new Array();
		
		PXML.doc = xml;
		var p = PXML.FindModule("DDNS6.WAN-1");						
		var table = OBJ("v6ddns_list");
		var i=1;
 		var cnt=XG(p+"/ddns6/cnt");
 		 		
 		BODY.CleanTable("v6ddns_list");
		for(i=1;i<=cnt;i++)
		{
			var b = p+"/ddns6/entry:"+i;
			var data	= [	'<input type="checkbox" id="en_ddns_v6'+i+'">',
					'<span id="en_ddns_v6host'+i+'"></span>',
					'<span id="en_ddns_v6addr'+i+'"></span>',
					'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif"></a>',
					'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif"></a>'
					];
			var type	= ["","", "","",""];
	
			BODY.InjectTable("v6ddns_list", i, data, type);
			if(XG(b+"/enable") =="1")
			{
				OBJ("en_ddns_v6"+i).checked=true;	
			}
			else
			{
				OBJ("en_ddns_v6"+i).checked=false;
		
			}		
			OBJ("en_ddns_v6addr"+i).innerHTML=XG(b+"/v6addr");
			OBJ("en_ddns_v6host"+i).innerHTML=XG(b+"/hostname");
			OBJ("en_ddns_v6"+i).disabled=false;
			
			this.org[i] = {
				enable:	XG(b+"/enable"),
				v6addr:	XG(b+"/v6addr"),
				hostname:	XG(b+"/hostname")
				};
				
			this.cfg[i] = {
				enable:	XG(b+"/enable"),
				v6addr:	XG(b+"/v6addr"),
				hostname:	XG(b+"/hostname")
				};	
		}
		this.g_table_index=i;
		
		var p = PXML.FindModule("DDNS4."+this.devicemode);
		if (p === "") alert("ERROR!");
		OBJ("en_ddns").checked = (XG(p+"/inf/ddns4")!=="");
		var ddnsp = GPBT(p+"/ddns4", "entry", "uid", this.ddns, 0);
		OBJ("server").value	= XG(ddnsp+"/provider");
		OBJ("host").value	= XG(ddnsp+"/hostname");
		OBJ("user").value	= XG(ddnsp+"/username");
		OBJ("passwd").value	= XG(ddnsp+"/password");
		OBJ("passwd_verify").value	= XG(ddnsp+"/password");
		var interval = XG(ddnsp+"/interval")/60;	if(interval=="" || interval==0)	interval = 567;
		OBJ("timeout").value	= interval;
		OBJ("report").innerHTML = "";

		var self = this;
		var ajaxObj = GetAjaxObj("updatenow");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			self.GetReport(xml);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("ddns_act.php", "act=getreport");
		this.EnableDDNS();
		return true;
	},
	PreSubmit: function()
	{
		var p4 = PXML.FindModule("DDNS4."+this.devicemode);
		var p6 = PXML.FindModule("DDNS6.WAN-1");		
		
		if(OBJ("en_ddns").checked)
		{
			if(COMM_EatAllSpace(OBJ("server").value) == "")
			{			
				BODY.ShowAlert("<?echo I18N("j","Please select server address.");?>");
				return null;	
			}	
			
			if(COMM_EatAllSpace(OBJ("host").value) == "")
			{			
				BODY.ShowAlert("<?echo I18N("j","Please input the host name.");?>");
				return null;	
			}	
			
			if(COMM_EatAllSpace(OBJ("user").value) == "")
			{			
				BODY.ShowAlert("<?echo I18N("j","Please input the user account.");?>");
				return null;	
			}	
			
			if(COMM_EatAllSpace(OBJ("passwd").value) == "")
			{			
				BODY.ShowAlert("<?echo I18N("j","Please input the password.");?>");
				return null;	
			}		
		
			if (!COMM_EqSTRING(OBJ("passwd").value, OBJ("passwd_verify").value))
			{
				BODY.ShowAlert("<?echo I18N("j","Password and Verify Password do not match the new User password.");?>");
				return null;
			}
			if (!TEMP_IsDigit(OBJ("timeout").value))
			{
				BODY.ShowAlert("<?echo I18N("j","Invalid period. The range of Timeout is 1~8670.");?>");
				OBJ("timeout").focus();
				return null;
			}
			
			//+++ Jerry Kao, move the codes below within the "if(OBJ("en_ddns").checked)".			
			if ( (OBJ("server").value!=="")	|| (OBJ("host").value!=="") ||
				 (OBJ("user").value!=="")	|| (OBJ("passwd").value!=="") )
			{
				var ddnsp = GPBT(p4+"/ddns4", "entry", "uid", this.ddns, 0);
				if (!ddnsp)
				{
					var c = XG(p4+"/ddns4/count");
					var s = XG(p4+"/ddns4/seqno");
					c += 1;
					s += 1;
					XS(p4+"/ddns4/entry:"+c+"/uid", this.ddns);
					XS(p4+"/ddns4/count", c);
					XS(p4+"/ddns4/seqno", s);
					ddnsp = p4+"/ddns4/entry:"+c;
				}
				XS(ddnsp+"/provider", OBJ("server").value);
				XS(ddnsp+"/hostname", OBJ("host").value);
				XS(ddnsp+"/username", OBJ("user").value);
				XS(ddnsp+"/password", OBJ("passwd").value);
				var timeout_value=OBJ("timeout").value*60;
				XS(ddnsp+"/interval", timeout_value);
			}
			//--- Jerry Kao.			
			
		}		
		
		if(OBJ("en_ddns_v6").checked)
		{								
			/*+++ Jerry Kao, checks below should be in AddDDNS() for del DDNS list, but
			//               without input IPv6 address and host name.
			if(COMM_EatAllSpace(OBJ("v6addr").value) == "")
			{			
				alert("Please input the IPv6 address.");
				return null;	
			}
			
			if(COMM_EatAllSpace(OBJ("v6host").value) == "")
			{			
				alert("Please input the IPv6 host name.");
				return null;	
			}
			*/
			
			//+++ Jerry Kao, move codes below in the "if(OBJ("en_ddns").checked)".			
			//var p6 = PXML.FindModule("DDNS6.WAN-1");		
			var table = OBJ("v6ddns_list");
			var rows = table.getElementsByTagName("tr");
		
			var row_len = rows.length;								
			if(this.org.length > rows.length) rowslen = this.org.length;
			
			var cnt = rows.length-1;
			while (cnt > 0) {XD(p6+"/ddns6/entry");cnt-=1;}
					
			XS(p6+"/ddns6/cnt",rows.length-1);
	
			var table_ind=1;
			var b=null;
			for(var i = 1;i <= row_len+1;i++)
			{												
				if(OBJ("en_ddns_v6"+i)!=null)
				{				
					b = p6+"/ddns6/entry:"+table_ind;
					XS(b+"/enable",   OBJ("en_ddns_v6"+i).checked? "1" : "0");
					XS(b+"/v6addr",   OBJ("en_ddns_v6addr"+i).innerHTML);
					XS(b+"/hostname", OBJ("en_ddns_v6host"+i).innerHTML);
					table_ind++;
				}
			}
			//--- Jerry Kao.		
		}													
		
		XS(p4+"/inf/ddns4", OBJ("en_ddns").checked ? this.ddns : "");
		
		// PXML.ActiveModule("DDNS6.WAN-1");  
		// PXML.ActiveModule("DDNS4.WAN-1");  		
		// PXML.ActiveModule("DDNS4.WAN-3");

		PXML.IgnoreModule("RUNTIME.DDNS4.WAN-1");
				
		if (this.devicemode == "WAN-3")	PXML.IgnoreModule("DDNS4.WAN-1");
		else							PXML.IgnoreModule("DDNS4.WAN-3");
				
		return PXML.doc;
	},
	//IsDirty: null,
	IsDirty: function()
	{
		var table = OBJ("v6ddns_list");
		var rows = table.getElementsByTagName("tr");
		var tab_index=1;
		
		if (this.cfg) delete this.cfg;
		this.cfg = new Array();
		
		for(var i = 1;i <= rows.length;i++)
		{
			if(OBJ("en_ddns_v6"+i)!=null)
			{
				this.cfg[tab_index] = {
				enable:	OBJ("en_ddns_v6"+i).checked? "1" : "0",
				v6addr:	OBJ("en_ddns_v6addr"+i).innerHTML,
				hostname:	OBJ("en_ddns_v6host"+i).innerHTML
				};				
				tab_index++;
			}
		}
				
		if (this.org.length !== this.cfg.length) return true;
		for (var i=1; i<this.cfg.length; i++)
		{				
			if (this.org[i].enable !== this.cfg[i].enable ||
				this.org[i].v6addr!== this.cfg[i].v6addr||
				this.org[i].hostname !== this.cfg[i].hostname ) 
				return true; 
		}
		
		return false;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	devicemode: "WAN-1",
	ddns: "DDNS4-1",
	GrayItems: function(disabled)
	{
		var frmObj = document.forms[0];
		for (var idx = 0; idx < frmObj.elements.length; idx+=1)
		{
			var obj = frmObj.elements[idx];
			var name = obj.tagName.toLowerCase();
			if (name === "input" || name === "select")
			{
				obj.disabled = disabled;
			}
		}
	},
	
	GetReport: function(xml)
	{
		
		var self = this;
		{
			if (xml.Get("/ddns4/valid")==="1" )
			{
				var s = xml.Get("/ddns4/status");
				var r = xml.Get("/ddns4/result");
				var msg = "";
				if (s === "IDLE")
				{
					if		(r === "SUCCESS") msg = "<?echo I18N("j","Connected");?>";
					else					  msg = "<?echo I18N("j","Disconnected");?>";
				}
				else
				{
					msg = "<?echo I18N("j","Disconnected");?>";
				}
			}
			else
			{	
				msg = "<?echo I18N("j","Disconnected");?>";
			}
			OBJ("report").innerHTML = msg;
		}

	},
	OnEdit: function(i)
	{		
		OBJ("en_ddns_v6").checked=OBJ("en_ddns_v6"+i).checked;
		OBJ("v6addr").disabled = false;
		OBJ("v6host").disabled = false;
		OBJ("v6addr").value=OBJ("en_ddns_v6addr"+i).innerHTML;
		OBJ("v6host").value=OBJ("en_ddns_v6host"+i).innerHTML;
		OBJ("add_ddns_v6").disabled = false;
		OBJ("clear_ddns_v6").disabled = false;
		this.g_edit=i;
	},
	
	OnDelete: function(i)
	{
		var z;
		var table = OBJ("v6ddns_list");
		var rows = table.getElementsByTagName("tr");

		
		for (z=1; z<=rows.length; z++) 
		{
			if(rows[z]!=null)
			{
				if (rows[z].id==i)
				{
					table.deleteRow(z);
				}
			}
		}
		
		
	},
	ddns_count: 0,
	ddns_testtime: "",
	OnClickUpdateNow: function()
	{	
		/*
		if(OBJ("host").value == "") return alert("Please input the host name.");
		if(OBJ("user").value == "") return alert("Please input the user account");
		if(OBJ("passwd").value == "") return alert("Please input the password");
		*/
		
		PXML.IgnoreModule("DDNS4.WAN-1");
		PXML.IgnoreModule("DDNS4.WAN-3");
		PXML.ActiveModule("RUNTIME.DDNS4.WAN-1");
		
		var self = this;
		var time_now = new Date();
		self.ddns_testtime = time_now.getHours().toString() + time_now.getMinutes().toString() + time_now.getSeconds().toString();
		
		var p = PXML.FindModule("RUNTIME.DDNS4.WAN-1");
		XS(p+"/runtime/inf/ddns4/provider", OBJ("server").value);
		XS(p+"/runtime/inf/ddns4/hostname", OBJ("host").value);
		XS(p+"/runtime/inf/ddns4/username", OBJ("user").value);
		XS(p+"/runtime/inf/ddns4/password", OBJ("passwd").value);
		XS(p+"/runtime/inf/ddns4/testtime", self.ddns_testtime);
		
		var xml = PXML.doc;
		PXML.UpdatePostXML(xml);
        COMM_CallHedwig(PXML.doc, function(xml){PXML.hedwig_callback(xml);});
 		
		this.GrayItems(true);   
		OBJ("report").innerHTML = "<?echo I18N("j","Start updating...");?>";
		self.ddns_count = 0 ;
		
		var ajaxObj = GetAjaxObj("updatenow");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			self.GetReport();
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("ddns_act.php", "act=getreport");
	},
	EnableDDNS: function()
	{
		if(OBJ("en_ddns").checked)
			OBJ("server").disabled = OBJ("host").disabled = OBJ("user").disabled = OBJ("passwd").disabled = OBJ("passwd_verify").disabled = OBJ("timeout").disabled = false; 
		else
			OBJ("server").disabled = OBJ("host").disabled = OBJ("user").disabled = OBJ("passwd").disabled = OBJ("passwd_verify").disabled = OBJ("timeout").disabled = true;
					
		if(OBJ("en_ddns_v6").checked)
		{
			OBJ("v6addr").disabled = OBJ("v6host").disabled = OBJ("add_ddns_v6").disabled = OBJ("clear_ddns_v6").disabled = false; 			
			for(var i = 1;i < this.g_table_index; i++)
			{
				OBJ("en_ddns_v6"+i).disabled = false;		
			}			
		}
		else
		{
			OBJ("v6addr").disabled = OBJ("v6host").disabled = OBJ("add_ddns_v6").disabled = OBJ("clear_ddns_v6").disabled =  true;						
			for(var i = 1;i < this.g_table_index; i++)
			{
				OBJ("en_ddns_v6"+i).disabled = true;		
			}			
		}			
	},
	AddDDNS: function()
	{
		var add_index=0;
				
		if(COMM_EatAllSpace(OBJ("v6addr").value) == "")
		{			
			BODY.ShowAlert("<?echo I18N("j", "Please input the IPv6 address.");?>");
			return null;	
		}
		
		if(COMM_EatAllSpace(OBJ("v6host").value) == "")
		{			
			BODY.ShowAlert("<?echo I18N("j", "Please input the IPv6 host name.");?>");
			return null;
		}	
		
		/* +++ Jerry Kao, move here to below for limit the number of records.
		//+++sam_pan add			
		var e = 
		{
			enable:OBJ("en_ddns_v6").checked? "1" : "0",
			v6addr:OBJ("v6addr").value,
			hostname:OBJ("v6host").value
		};
				
		for (j=1; j<this.cfg.length; j++)
		{														
			if(this.cfg[j].v6addr == e.v6addr)
			{
				alert("<?echo I18N("j", "The IPv6 address");?>"+" '"+e.v6addr+"'<?echo I18N("j", " is already existed!");?>");
				return null;
			}
			
			if(this.cfg[j].hostname == e.hostname)
			{
				alert("<?echo I18N("j", "The IPv6 host name");?>"+" '"+e.hostname+"'<?echo I18N("j", " is already existed!");?>");
				return null;
			}
		}	
				
		this.cfg[j++] = e;											 		
		//---sam_pan add
		*/
		
		//+++ Jerry Kao, remove all space in ipv6 address and host name.		
		OBJ("v6addr").value = COMM_EatAllSpace(OBJ("v6addr").value);			
		OBJ("v6host").value = COMM_EatAllSpace(OBJ("v6host").value);
		
							
		if(this.g_edit!=0)
		{
			add_index=this.g_edit;
		}
		else
		{
			add_index=this.g_table_index;
		}
		
		//+++ Jerry Kao, added max number for "IPv6 DDNS list".
		var p = PXML.FindModule("DDNS6.WAN-1");
		var max_cnt = XG(p+"/ddns6/max");	// added node "/ddns6/max" in dir-845, only.				
		
		if (max_cnt=="")
			max_cnt = 32;				
		
		if (add_index <= max_cnt)
		{
			//+++sam_pan add		
			var e = 
			{
				enable:OBJ("en_ddns_v6").checked? "1" : "0",
				v6addr:OBJ("v6addr").value,
				hostname:OBJ("v6host").value
			};
							
			for (j=1; j<this.cfg.length; j++)
			{														
				//+++ Jerry Kao, added for modifiing exist records easier.			
				if (this.g_edit != 0 && j != this.g_edit)
				{					
					if(this.cfg[j].v6addr == e.v6addr)
					{
						alert("<?echo I18N("j", "The IPv6 address");?>"+" '"+e.v6addr+"'<?echo I18N("j", " is already existed!");?>");
						return null;
					}
					
					if(this.cfg[j].hostname == e.hostname)
					{
						alert("<?echo I18N("j", "The IPv6 host name");?>"+" '"+e.hostname+"'<?echo I18N("j", " is already existed!");?>");
						return null;
					}
				}
			}
													
			this.cfg[j++] = e;											 		
			//---sam_pan add
				
			var data	= [	'<input type="checkbox" id="en_ddns_v6'+add_index+'">',
							'<span id="en_ddns_v6host'+add_index+'"></span>',
							'<span id="en_ddns_v6addr'+add_index+'"></span>',
							'<a href="javascript:PAGE.OnEdit('+add_index+');"><img src="pic/img_edit.gif"></a>',
							'<a href="javascript:PAGE.OnDelete('+add_index+');"><img src="pic/img_delete.gif"></a>'
							];
			var type	= ["","", "","",""];
			
			BODY.InjectTable("v6ddns_list", add_index, data, type);
			if(OBJ("en_ddns_v6").checked)
			{
				OBJ("en_ddns_v6"+add_index).checked=true;	
			}
			else
			{
				OBJ("en_ddns_v6"+add_index).checked=false;
		
			}		
			OBJ("en_ddns_v6addr"+add_index).innerHTML=OBJ("v6addr").value;
			OBJ("en_ddns_v6host"+add_index).innerHTML=OBJ("v6host").value;
			OBJ("en_ddns_v6"+add_index).disabled=false;
			if(this.g_edit!=0)
			{
				this.g_edit=0;
			}
			else
			{
				this.g_table_index++;	
			}
			
			//+++ Jerry Kao, Reset after save.
			OBJ("v6addr").value = "";
			OBJ("v6host").value = "";
			
			OBJ("mainform").setAttribute("modified", "true");
		}
		else
		{			
			alert("<?echo I18N("j", "The maximum number of list is");?>"+" "+max_cnt);								
		}
	}		
};


function ClearDDNS()
{
	OBJ("v6addr").value = "";
	OBJ("v6host").value = "";
	OBJ("mainform").setAttribute("modified", "true");
}
function OnClickPCArrow(idx)
{
	OBJ("v6addr").value = OBJ("pc_"+idx).value;

}
</script>
