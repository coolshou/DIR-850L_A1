<?
dophp("load", "/htdocs/web/portal/comm/drag.php");
dophp("load", "/htdocs/web/portal/comm/event.php");
dophp("load", "/htdocs/web/portal/comm/fade.php");
dophp("load", "/htdocs/web/portal/comm/overlay.php");
dophp("load", "/htdocs/web/portal/comm/scoot.php");
?>
<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "ITUNES , RUNTIME.DEVICE",
	OnLoad: function()
	{
		if (!this.rgmode) BODY.DisableCfgElements(true);
	},
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
		case "OK":
			self.location.href = "./share.php";
			break;
		default:
			BODY.ShowMessage("<?echo I18N("j","Error");?>",result);
			break;
		}
		return true;
	},
	
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

		this.device = PXML.FindModule("RUNTIME.DEVICE");
		this.itunes = PXML.FindModule("ITUNES");
		initOverlay("white");
		if (this.itunes==="" || this.device==="") { BODY.ShowMessage("<?echo I18N("j","Error");?>","InitValue ERROR!"); return false; }
		if (XG(this.itunes+"/itunes/server/active")=="1")
			OBJ("active").checked = true;
		else
			OBJ("inactive").checked = true;

		this.InitPartation();
		this.showSelect();
		this.check_itunes_enable(0);		
		this.InitSelect();

		return true;
	},
	PreSubmit: function()
	{
		if (!this.check_sdstatus()) return null;
		XS(this.itunes+"/itunes/server/active", this.get_radio_value("itunes_active"));
		XS(this.itunes+"/itunes/server/sharepath", OBJ("the_sharepath").value);
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	itunes: null,
	device: null,
	partition_count: 0,
	Label_name: null,
	partition_size: null,
	disable_item: function(status)
	{
		if (status)
		{
			OBJ("itunes_root").disabled=true;
			OBJ("the_sharepath").disabled=true;
			OBJ("But_Browse").disabled=true;
			OBJ("selectPartition").disabled=true;
		}
		else
		{
			OBJ("itunes_root").disabled=false;
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=false;
			OBJ("selectPartition").disabled=false;
		}
	},
	get_radio_value: function(name)
	{
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{
			if (obj[i].checked) return obj[i].value;
		}
	},
	check_itunes_enable: function(flag)
	{
		if (flag != 0)
		{
			if (!this.check_sdstatus()) return;
		}

		if (OBJ("active").checked)
			this.disable_item(false);
		else	
			this.disable_item(true);
	},
	open_browser: function()
	{
		window_make_new(-1, -1, 500, 400, "portal/explorer.php?path=/","Explorer");
	},
	check_path: function(clear)
	{
		if (OBJ("itunes_root").checked || clear!=1)
		{
			OBJ("chamber2").style.display = "none";
			OBJ("the_sharepath").value="/";
		}
		else
		{
			OBJ("chamber2").style.display = "";
		}
	},
	check_sdstatus: function()
	{
		var storage_count = XG(this.itunes+"/runtime/device/storage/count");
		if ((storage_count!="0"&&storage_count!="") || OBJ("active").checked==false)
			return true;
		BODY.ShowMessage("<?echo I18N("j","Error");?>", "<?echo I18N("j","No storage device!");?>");
		return false;
	},
	CheckPathCallback: function(xml)
	{
		if (xml.Get("/checkreport/result")=="NOTEXIST")
			BODY.ShowAlert("<?echo I18N('j', 'The selected folder is not available. Please choose another folder !!');?>");
	},
	CheckPathValidity: function(path)
	{
		var ajaxObj = GetAjaxObj("ITUNESCheckSharePath");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.CheckPathCallback(xml);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("check.php", "act=checkdir&dirname="+path);
	},
	byteConverter: function(kb)
	{
		var mb = Math.round(kb/1024);
		var gb = Math.round(kb/1024/1024);

		if(gb>0)
			return gb+"GB";
		else if(mb>0)
			return mb+"MB";
		else
			return kb+"KB";
	},
	InitPartation: function()
	{
		var disk_qty = XG(this.device+"/runtime/device/storage/count");
		this.partition_count =0;
		this.Label_name = new Array();
		this.partition_size = new Array();
		var opt_list = "";
		
		/* get each partition's label_name and storage size,
			partition_count = disk_qty * partition_count */
		for(var i=1;i<=disk_qty;i++)
		{
			var partition_qty =  XG(this.device+"/runtime/device/storage/disk:"+i+"/count");
			for(var j=1;j<=partition_qty;j++)
			{
				var partition_mntp = XG(this.device+"/runtime/device/storage/disk:"+i+"/entry:"+j+"/mntp");
				var Label = partition_mntp.split("/");
				var partition_state = XG(this.device+"/runtime/device/storage/disk:"+i+"/entry:"+j+"/state");
				
				if(partition_state == "MOUNTED") //we want surported partitions
				{
					this.Label_name[this.partition_count] = Label[4];
					this.partition_size[this.partition_count] = XG(this.device+"/runtime/device/storage/disk:"+i+"/entry:"+j+"/space/size");
					this.partition_count++;
				}
			}
		}
	},
	InitSelect: function()
	{
		var sharepath = XG(this.itunes+"/itunes/server/sharepath");

		this.CheckPathValidity(sharepath);
		if(this.check_sdstatus())
		{		
			var has_sharepath = false;
			for(var i=0;i<this.partition_count;i++)
			{
				if(sharepath === this.Label_name[i])
				{
					OBJ("selectPartition").options.selectedIndex = i+1;
					has_sharepath = true;
					break;
				}	
			}		
			if (has_sharepath == false)
			{
				OBJ("the_sharepath").value = this.Label_name[0];
				OBJ("selectPartition").options.index = 0;
			}
		}		
	},	
	showSelect: function()
	{
		var opt_list = "<option value=\"\">Please Select a storage as your iTunes media server</option>";	
		if(this.partition_count==0)
		{
			opt_list = "<option>No storage device!</option>";
		}	
		else
		{		
			for(var i=0;i<this.partition_count;i++)
			{
				opt_list = opt_list + "<option value=\"" + this.Label_name[i] + "\">" + 
										this.Label_name[i] + "(" + this.byteConverter(this.partition_size[i]) + ")" + "</option>";
			}
		}
		var selectHTML = '<select name="select" class="text_block" id="selectPartition" onChange="PAGE.setSharepath(this);">' + 
							opt_list + '</select>';
		OBJ("select_partition").innerHTML = selectHTML;
	},
	setSharepath: function(selectOBJ)
	{
		OBJ("the_sharepath").value = selectOBJ.options[selectOBJ.selectedIndex].value;
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
