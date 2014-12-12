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
	services: "INBFILTER",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
		BODY.CleanTable("inbftable");
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			BODY.OnReload();
			return true;
			break;
		default : 	//if fatlady return error
			/* we should load the original configs. Can't count on PXML object, since its already modified. 
			We can count on our original table */
			delete this.cfg;
			this.cfg = new Array();
			var cnt = this.org.length;
			for (var i=0; i<cnt; i+=1)
			{
				this.cfg[i] = 
				{
					uid:	this.org[i].uid,
					desc:	this.org[i].desc,
					act:	this.org[i].act
				};
			}
			delete this.ipcfg;
			this.ipcfg = new Array();
			var ipcnt = this.iporg.length;
			for (var k=0; k<ipcnt; k+=1)
			{
				this.ipcfg[k] = 
				{
					uid:	this.iporg[k].uid,
					seq:	this.iporg[k].seq,
					enable:	this.iporg[k].enable,	
					startip:this.iporg[k].startip,	
					endip:	this.iporg[k].endip
				};
			}
			return false;
			break;	
		}
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;

		if (this.org) delete this.org;
		if (this.cfg) delete this.cfg;
		if (this.iporg) delete this.iporg;
		if (this.ipcfg) delete this.ipcfg;
		this.org = new Array();
		this.cfg = new Array();
		this.iporg = new Array();
		this.ipcfg = new Array();
		
		var p = PXML.FindModule("INBFILTER");
		p += "/acl/inbfilter";
		var cnt = S2I(XG(p+"/entry#"));
		var k=0;
		for (var i=0; i<cnt; i+=1)
		{
			var ii=i+1;
			this.org[i] = 
			{
				uid:	XG(p+"/entry:"+ii+"/uid"),
				desc:	XG(p+"/entry:"+ii+"/description"),
				act:	XG(p+"/entry:"+ii+"/act")
			};
			
			this.cfg[i] = 
			{
				uid:	XG(p+"/entry:"+ii+"/uid"),
				desc:	XG(p+"/entry:"+ii+"/description"),
				act:	XG(p+"/entry:"+ii+"/act")
			};
		
			var s = p+"/entry:"+ii+"/iprange";
			for (var j=1; j<=8; j+=1)
			{
				this.iporg[k] = 
				{
					uid:	XG(p+"/entry:"+ii+"/uid"),
					seq:	XG(s+"/entry:"+j+"/seq"),
					enable:	XG(s+"/entry:"+j+"/enable"),	
					startip:XG(s+"/entry:"+j+"/startip"),	
					endip:	XG(s+"/entry:"+j+"/endip")
				};	
				this.ipcfg[k] = 
				{
					uid:	XG(p+"/entry:"+ii+"/uid"),
					seq:	XG(s+"/entry:"+j+"/seq"),
					enable:	XG(s+"/entry:"+j+"/enable"),	
					startip:XG(s+"/entry:"+j+"/startip"),	
					endip:	XG(s+"/entry:"+j+"/endip")
				};
				k++;
			}
		}	
		this.InitEntryList();
		this.OnClickInbFCancel();
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("INBFILTER");
		p += "/acl/inbfilter";
		var cnt = S2I(XG(p+"/entry#"));
		while (cnt > 0) {XD(p+"/entry");cnt-=1;}
		
		for (var i=0; i<this.cfg.length; i+=1)
		{
			var ii = i+1;
			var m = p+"/entry:"+ii;
			XS(m+"/uid",	this.cfg[i].uid);
			XS(m+"/description",this.cfg[i].desc);
			XS(m+"/act",this.cfg[i].act);
			var j = 1;
			for (var k=0; k<this.ipcfg.length; k+=1)
			{
				var s = m+"/iprange/entry:"+j;				
				if(this.cfg[i].uid === this.ipcfg[k].uid && this.ipcfg[k].seq !== "")
				{
					XS(s+"/uid",		this.cfg[i].uid			); 
					XS(s+"/seq",		this.ipcfg[k].seq		);
					XS(s+"/enable",		this.ipcfg[k].enable	);	
					XS(s+"/startip",	this.ipcfg[k].startip	);	
					XS(s+"/endip",		this.ipcfg[k].endip		);
					j++;
				}	
				
			}	
		}	
		XS(p+"/count", this.cfg.length);	
		return PXML.doc;
	},
	IsDirty: function()
	{
		return true;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	org: null,
	cfg: null,
	iporg: null,
	ipcfg: null,
	edit: null,
	IPString: function(i)
	{
		var note="", d="";
		for (var k=0; k<this.ipcfg.length; k+=1)
		{
			if (this.cfg[i].uid === this.ipcfg[k].uid && this.ipcfg[k].enable === "1") 
			{
				note+=d+this.ipcfg[k].startip+"-"+this.ipcfg[k].endip;
				d=",";
			}
		}	
		return note;
	},
	InitEntryList: function()
	{
		BODY.CleanTable("inbftable");
		for (var i=0; i<this.cfg.length; i+=1)
		{
			var data = [this.cfg[i].desc, this.cfg[i].act=="allow"?"<? echo I18N("j", "Allow");?>":"<? echo I18N("j", "Deny");?>",this.IPString(i),
				'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif" alt="<?echo I18N("h", "Edit");?>" title="<?echo I18N("h", "Edit");?>"></a>',
				'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif" alt="<?echo I18N("h", "Delete");?>" title="<?echo I18N("h", "Delete");?>"></a>'
				];
			var type = ["text","text","text","",""];
			BODY.InjectTable("inbftable", this.cfg[i].uid, data, type);
		}
	},
	OnClickInbFSubmit: function()
	{		
		if (OBJ("inbfdesc").value === "")
		{
			alert("<?echo I18N("j", "The 'Name' field can not be blank.");?>");
			return;
		}

		for(var j=0; j < OBJ("inbfdesc").length ; j++)
		{
			if (e.desc.charAt(j) === " ")
			{
				alert("<?echo I18N("j", "The 'Name' field can't have blank space.");?>");
				return;
			}
		}
	
		for(var j=0; j < this.cfg.length; j++)
		{
			if (OBJ("inbfdesc").value === this.cfg[j].desc)
			{
				if(this.edit !== this.cfg[j].uid)
				{
					alert("<?echo I18N("j", "The 'Name' could not be the same.");?>");
					return;
				}
			}
		}
		for (var j=1; j<=8; j+=1)
		{
			if(OBJ("en_inbf"+j).checked === true)	break;
			if(j === 8)	
			{
				alert("<?echo I18N("j", "Enable at least one Remote IP range.");?>");
				return;
			}	
		}
		
		var i = 0;
		var k = 0;
		if (!this.edit || this.edit === "")	/* To add a new inbound filter list */
		{
			i = this.cfg.length;
			k = this.ipcfg.length;
		}
		else								/* To update an old inbound filter list */
		{	
			for (i=0; i<this.cfg.length; i+=1)	if (this.cfg[i].uid === this.edit)	break;	
			for (k=0; i<this.ipcfg.length; k+=1)
			{
				if (this.ipcfg[k].uid === this.edit)
				{
					alert("<?echo I18N("j", "You must reboot after update.");?>");
					break;
				}
			}	
		}
		this.cfg[i] = 
		{
			uid:	this.edit?this.edit:"",
			desc:	OBJ("inbfdesc").value,
			act:	OBJ("inbfact").value
		};
		for (var j=1; j<=8; j+=1)
		{
			if(OBJ("en_inbf"+j).checked === true || OBJ("inbf_startip"+j).value !== "0.0.0.0" || OBJ("inbf_endip"+j).value !== "255.255.255.255")
			{
				this.ipcfg[k] = 
				{
					uid:	this.edit?this.edit:"",
					seq:	j,
					enable:	OBJ("en_inbf"+j).checked?"1":"0",	
					startip:OBJ("inbf_startip"+j).value,	
					endip:	OBJ("inbf_endip"+j).value
				};	
				k++;
			}	
		}	
		BODY.OnSubmit();
	},
	OnClickInbFCancel: function()
	{
		this.edit = null;
		
        OBJ("inbfdesc").value = "";
        OBJ("inbfact").value = "allow";
        for (var i=1; i<=8; i+=1)
		{
			OBJ("en_inbf"+i).checked = false; 
			OBJ("inbf_startip"+i).value = "0.0.0.0";
			OBJ("inbf_endip"+i).value = "255.255.255.255";	
		}
		OBJ("inbfsubmit").value = "<?echo I18N("h", "Add");?>";
	},
	OnEdit: function(i)
	{
		OBJ("inbfdesc").value = this.cfg[i].desc;
		OBJ("inbfact").value = this.cfg[i].act;
		for (var j=1; j<=8; j+=1)
		{
			OBJ("en_inbf"+j).checked = false; 
			OBJ("inbf_startip"+j).value = "0.0.0.0";
			OBJ("inbf_endip"+j).value = "255.255.255.255";	
		}
		for (var k=0; k<this.ipcfg.length; k+=1)
		{	
			if (this.cfg[i].uid === this.ipcfg[k].uid && this.ipcfg[k].seq !== "") 
			{
				var m=this.ipcfg[k].seq;
				OBJ("en_inbf"+m).checked = COMM_ToBOOL(this.ipcfg[k].enable); 
				OBJ("inbf_startip"+m).value = this.ipcfg[k].startip;
				OBJ("inbf_endip"+m).value = this.ipcfg[k].endip;
			}
		}	
		OBJ("inbfsubmit").value = "<?echo I18N("h", "Update");?>";
		this.edit = this.cfg[i].uid;
	},
	OnDelete: function(i)
	{
		var used_inbf=[''<?
		foreach("/inf")		{	$uid=query("inbfilter"); if($uid!=""){echo ",'".$uid."'"."\n";}	}	
		foreach("/nat/entry")
		{
			foreach("virtualserver/entry")	{	$uid=query("inbfilter"); if($uid!=""){echo ",'".$uid."'"."\n";}	} 
			foreach("portforward/entry")	{	$uid=query("inbfilter"); if($uid!=""){echo ",'".$uid."'"."\n";}	}			
		}
		?>];
		for(var j=1;j < used_inbf.length; j++)
		{	
			if(this.cfg[i].uid==used_inbf[j])
			{
				alert("<?echo I18N("j", "The Rule").I18N("j", " is already used !");?>");
				return;
			}
		}
		this.cfg.splice(i,1);
		BODY.OnSubmit();
	},
	CursorFocus: function(node)
	{
		var i = node.lastIndexOf("ip:");
		var idx = node.charAt(i+3);
		if (node.match("startip")) OBJ("inbf_startip"+idx).focus();
		if (node.match("endip")) OBJ("inbf_endip"+idx).focus();
	}
}
</script>
