<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
//	services: "DDNS6.WAN-1,DDNS4.WAN-1,DDNS4.WAN-3,RUNTIME.DDNS4.WAN-1",
	services: "DDNS4.WAN-1,RUNTIME.DDNS4.WAN-1",
	OnLoad: function()
	{
		if (!this.rgmode) BODY.DisableCfgElements(true);
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
/*	IsDirty: function()
	{
		var table = OBJ("v6ddns_list");
		var rows = table.getElementsByTagName("tr");
		var tab_index=1;

		if (this.cfg) delete this.cfg;
		this.cfg = new Array();

		for (var i=1; i<=rows.length; i++)
		{
			if (OBJ("en_ddns_v6"+i)!=null)
			{
				this.cfg[tab_index] = {
					enable: OBJ("en_ddns_v6"+i).checked? "1" : "0",
					v6addr: OBJ("en_ddns_v6addr"+i).innerHTML,
					hostname: OBJ("en_ddns_v6host"+i).innerHTML
				};
				tab_index++;
			}
		}

		if (this.org.length !== this.cfg.length) return true;
		for (var i=1; i<this.cfg.length; i++)
		{
			if (this.org[i].enable!==this.cfg[i].enable ||
				this.org[i].v6addr!== this.cfg[i].v6addr||
				this.org[i].hostname !== this.cfg[i].hostname)
				return true;
		}

		return false;
	},	*/
	
	/* The "services" will be requested by GetCFG(), then the InitValue() will be
	 * called to process the services XML document. */
	InitValue: function(xml)
	{
		if (this.org) delete this.org;
		if (this.cfg) delete this.cfg;
		this.org = new Array();
		this.cfg = new Array();

		PXML.doc = xml;
		//xml.dbgdump();

/*
//		*************** DIR-645 defined DDNS6, but it has no releated functions
		var p = PXML.FindModule("DDNS6.WAN-1");
		TEMP_CleanTable("v6ddns_list");
		var cnt=XG(p+"/ddns6/cnt");
		for (var i=1; i<=cnt; i++)
		{
			var b = p+"/ddns6/entry:"+i;
			var data    = [ '<input type="checkbox" id="en_ddns_v6'+i+'">',
				'<span id="en_ddns_v6host'+i+'"></span>',
				'<span id="en_ddns_v6addr'+i+'"></span>',
				'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif"></a>',
				'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif"></a>'
					];
			var type = ["","","","",""];

			BODY.InjectTable("v6ddns_list", i, data, type);
			if (XG(b+"/enable") =="1")
				OBJ("en_ddns_v6"+i).checked=true;
			else
				OBJ("en_ddns_v6"+i).checked=false;
			OBJ("en_ddns_v6addr"+i).innerHTML=XG(b+"/v6addr");
			OBJ("en_ddns_v6host"+i).innerHTML=XG(b+"/hostname");
			OBJ("en_ddns_v6"+i).disabled=false;
			this.org[i] = {
				enable: XG(b+"/enable"),
				v6addr: XG(b+"/v6addr"),
				hostname: XG(b+"/hostname")
			};

			this.cfg[i] = {
				enable: XG(b+"/enable"),
				v6addr: XG(b+"/v6addr"),
				hostname: XG(b+"/hostname")
			};
		}
		this.g_table_index=i;
*/

		var p = PXML.FindModule("DDNS4."+this.devicemode);
		if (p === "") alert("ERROR!");
		var ddnsp = GPBT(p+"/ddns4", "entry", "uid", this.ddns, 0);
		OBJ("en_ddns").checked		= (XG(p+"/inf/ddns4")!=="");
		OBJ("server").value			= XG(ddnsp+"/provider");
		OBJ("host").value			= XG(ddnsp+"/hostname");
		OBJ("user").value			= XG(ddnsp+"/username");
		OBJ("passwd").value			= XG(ddnsp+"/password");
		OBJ("passwd_verify").value	= XG(ddnsp+"/password");
		var interval = XG(ddnsp+"/interval")/60;
		if (interval=="" || interval==0) interval = 567;
		OBJ("timeout").value		= interval;
		OBJ("report").innerHTML		= "";

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
		if (OBJ("en_ddns").checked)
		{
			if (COMM_EatAllSpace(OBJ("server").value) == "")
			{
				BODY.ShowConfigError("server", "<?echo I18N("j","Please select server address.");?>");
				return null;
			}
			if (COMM_EatAllSpace(OBJ("host").value) == "")
			{
				BODY.ShowConfigError("host", "<?echo I18N("j","Please enter host name");?>");
				return null;
			}
			if (COMM_EatAllSpace(OBJ("user").value) == "")
			{
				BODY.ShowConfigError("user", "<?echo I18N("j","Please enter user account.");?>");
				return null;
			}
			if (COMM_EatAllSpace(OBJ("passwd").value) == "")
			{
				BODY.ShowConfigError("passwd", "<?echo I18N("j","Please enter password.");?>");
				return null;
			}
			if (!COMM_EqSTRING(OBJ("passwd").value, OBJ("passwd_verify").value))
			{
				BODY.ShowConfigError("passwd", "<?echo I18N("j","Password and Verify Password do not match the new User password.");?>");
				return null;
			}
			if (!TEMP_IsDigit(OBJ("timeout").value))
			{
				BODY.ShowConfigError("timeout", "<?echo I18N("j","Invalid period. The range of Timeout is 1~8670.");?>");
				OBJ("timeout").focus();
				return null;
			}
		}

/*
//		*************** DIR-645 defined DDNS6, but it has no releated functions
		if (OBJ("en_ddns_v6").checked)
		{
			if (COMM_EatAllSpace(OBJ("v6addr").value) == "")
			{
				return null;
			}
			if (COMM_EatAllSpace(OBJ("v6host").value) == "")
			{
				return null;
			}
		}

		var p = PXML.FindModule("DDNS6.WAN-1");
		var table = OBJ("v6ddns_list");
		var rows = table.getElementsByTagName("tr");
		var row_len = rows.length;
		if (this.org.length > rows.length) rowslen = this.org.length;

		var cnt = rows.length-1;
		while (cnt > 0)
		{
			XD(p+"/ddns6/entry");
			cnt-=1;
		}

		XS(p+"/ddns6/cnt",rows.length-1);

		var table_ind=1;
		var b=null;
		for (var i=1; i<=row_len+1; i++)
		{
			if (OBJ("en_ddns_v6"+i)!=null)
			{
				b = p+"/ddns6/entry:"+table_ind;
				XS(b+"/enable", OBJ("en_ddns_v6"+i).checked? "1" : "0");
				XS(b+"/v6addr", OBJ("en_ddns_v6addr"+i).innerHTML);
				XS(b+"/hostname", OBJ("en_ddns_v6host"+i).innerHTML);
				table_ind++;
			}
		}
*/

		PXML.IgnoreModule("RUNTIME.DDNS4.WAN-1");
		var p = PXML.FindModule("DDNS4."+this.devicemode);
		XS(p+"/inf/ddns4", OBJ("en_ddns").checked ? this.ddns : "");

		if (OBJ("en_ddns").checked||OBJ("server").value!==""||
			OBJ("host").value!==""||OBJ("user").value!==""||
			OBJ("passwd").value!=="")
		{
			var ddnsp = GPBT(p+"/ddns4", "entry", "uid", this.ddns, 0);
			if (!ddnsp)
			{
				var cnt = COMM_ToNUMBER(XG(p+"/ddns4/count"));
				var seq = COMM_ToNUMBER(XG(p+"/ddns4/seqno"));
				cnt++;
				seq++;
				XS(p+"/ddns4/entry:"+cnt+"/uid", this.ddns);
				XS(p+"/ddns4/count", cnt);
				XS(p+"/ddns4/seqno", seq);
				ddnsp = p+"/ddns4/entry:"+cnt;
			}
			XS(ddnsp+"/provider", OBJ("server").value);
			XS(ddnsp+"/hostname", OBJ("host").value);
			XS(ddnsp+"/username", OBJ("user").value);
			XS(ddnsp+"/password", OBJ("passwd").value);
			var timeout_value=OBJ("timeout").value*60;
			XS(ddnsp+"/interval", timeout_value);
		}
		//if (this.devicemode == "WAN-3")
		//	PXML.IgnoreModule("DDNS4.WAN-1");
		//else
		//	PXML.IgnoreModule("DDNS4.WAN-3");
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	org: null,
	cfg: null,
	g_table_index: 1,
	g_edit: 0,
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	devicemode: "WAN-1",
	ddns: "DDNS4-1",
	GetReport: function(xml)
	{
		if (xml.Get("/ddns4/valid")==="1")
		{
			var status = xml.Get("/ddns4/status");
			var result = xml.Get("/ddns4/result");
			var msg = "";
			if (status === "IDLE")
				msg = (result === "SUCCESS")? "<?echo I18N("j","Connected");?>":"<?echo I18N("j","Disconnected");?>";
			else
				msg = "<?echo I18N("j","Disconnected");?>";
		}
		else
			msg = "<?echo I18N("j","Disconnected");?>";
		OBJ("report").innerHTML = msg;
	},
	OnEdit: function(i)
	{
		OBJ("en_ddns_v6").checked=OBJ("en_ddns_v6"+i).checked;
		OBJ("v6addr").disabled = false;
		OBJ("v6host").disabled = false;
		OBJ("v6addr").value=OBJ("en_ddns_v6addr"+i).innerHTML;
		OBJ("v6host").value=OBJ("en_ddns_v6host"+i).innerHTML;
//		OBJ("add_ddns_v6").disabled = false;
//		OBJ("clear_ddns_v6").disabled = false;
		this.g_edit=i;
	},
	OnDelete: function(i)
	{
		var table = OBJ("v6ddns_list");
		var rows = table.getElementsByTagName("tr");


		for (var z=1; z<=rows.length; z++)
		{
			if (rows[z]!=null)
			{
				if (rows[z].id==i)	table.deleteRow(z);
			}
		}


	},
	EnableDDNS: function()
	{
		if (OBJ("en_ddns").checked)
			OBJ("server").disabled = OBJ("host").disabled =
			OBJ("user").disabled = OBJ("passwd").disabled =
			OBJ("passwd_verify").disabled = OBJ("timeout").disabled = false;
		else
			OBJ("server").disabled = OBJ("host").disabled =
			OBJ("user").disabled = OBJ("passwd").disabled =
			OBJ("passwd_verify").disabled = OBJ("timeout").disabled = true;

/*
		if (OBJ("en_ddns_v6").checked)
			OBJ("v6addr").disabled = OBJ("v6host").disabled = false;
		else
			OBJ("v6addr").disabled = OBJ("v6host").disabled = true;
*/
	},
	AddDDNS: function()
	{
		var add_index=0;

		if (COMM_EatAllSpace(OBJ("v6addr").value) == "")
		{
			return null;
		}
		if (COMM_EatAllSpace(OBJ("v6host").value) == "")
		{
			return null;
		}

		//+++sam_pan add
		var e = {
			enable:OBJ("en_ddns_v6").checked? "1" : "0",
			v6addr:OBJ("v6addr").value,
			hostname:OBJ("v6host").value
		};

		for (j=1; j<this.cfg.length; j++)
		{
			if (this.cfg[j].v6addr == e.v6addr)
			{
				return null;
			}

			if (this.cfg[j].hostname == e.hostname)
			{
				return null;
			}
		}

		this.cfg[j++] = e;
		//---sam_pan add

		if (this.g_edit!=0)
			add_index=this.g_edit;
		else
			add_index=this.g_table_index;

		var data = [
			'<input type="checkbox" id="en_ddns_v6'+add_index+'">',
			'<span id="en_ddns_v6host'+add_index+'"></span>',
			'<span id="en_ddns_v6addr'+add_index+'"></span>',
			'<a href="javascript:PAGE.OnEdit('+add_index+');"><img src="pic/img_edit.gif"></a>',
			'<a href="javascript:PAGE.OnDelete('+add_index+');"><img src="pic/img_delete.gif"></a>'
		];
		var type = ["","", "","",""];

		BODY.InjectTable("v6ddns_list", add_index, data, type);
		if (OBJ("en_ddns_v6").checked)
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
		if (this.g_edit!=0)	this.g_edit=0;
		else				this.g_table_index++;
		OBJ("mainform").setAttribute("modified", "true");
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

function ClearDDNS()
{
	OBJ("en_ddns_v6").checked = false;
	OBJ("v6addr").value = "";
	OBJ("v6host").value = "";
	OBJ("mainform").setAttribute("modified", "true");
}
function OnClickPCArrow(idx)
{
	OBJ("v6addr").value = OBJ("pc_"+idx).value;

}
//]]>
</script>
