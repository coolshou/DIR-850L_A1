<script type="text/javascript">
var EventName = null;
function Page() {}
Page.prototype =
{
	services: "NETATALK",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	afp: null,
	InitValue: function(xml)
	{
		PXML.doc = xml;
		afp = PXML.FindModule("NETATALK");
		initOverlay("white");			
		if (afp==="") { alert("InitValue ERROR!"); return false; }
		this.set_radio_value("afp_active", XG(afp+"/netatalk/active"));		
		this.check_afp_enable(0, "afp_active");
												
		var sharepath = XG(afp+"/netatalk/sharepath");
		OBJ("the_sharepath").value = sharepath;
		/* check the sharepath value is exist */
		this.CheckPathValidity(sharepath);
		return true;
	},
	PreSubmit: function()
	{				
		afp = PXML.FindModule("NETATALK");		

		if(!this.check_sdstatus()) return null;
		
		if(this.get_radio_value("afp_active")=="1")
		{	
			if(OBJ("the_sharepath").value=="")
			{
				BODY.ShowAlert("Please select a folder to share..!!");
				return null;
			}
		}
		
		XS(afp+"/netatalk/active", this.get_radio_value("afp_active"));			
		XS(afp+"/netatalk/sharepath", OBJ("the_sharepath").value);				
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	disable_item: function(status)
	{
		if(status==1)	//enable
		{
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=false;			
		}
		else
		{
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=true;			
		}
	},
	set_radio_value: function(name, value)
	{
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{			
			if(obj[i].value==value)
			{
				obj[i].checked = true;
				break;
			}
		}
	},
	get_radio_value: function(name)
	{
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{
			if (obj[i].checked)	return obj[i].value;
		}
	},
	check_afp_enable: function(flag, name)
	{
		if(flag != 0)
		{
			//hendry, debug 
			if(!this.check_sdstatus()) return;		
		}
			
		this.disable_item(0);
		
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{			
			if(obj[i].value=="1" && obj[i].checked == true)
			{
				this.disable_item(1);
				break;
			}
		}					
	},
	open_browser: function()
	{	
		window_make_new(-1, -1, 500, 400, "portal/explorer.php?path=/","Explorer");
	},
	check_sdstatus: function()
	{
		afp = PXML.FindModule("NETATALK");
		var storage_count = XG(afp+"/runtime/device/storage/count");		
		if(storage_count != "0" && storage_count != "")
		{
			return true; 
		}
		else 
		{
			BODY.ShowAlert("<?echo I18N("j","No storage device!");?>");
			return false;
		}	
	},
	
	CheckPathCallback: function(xml)
	{
		if (xml.Get("/checkreport/result")=="NOTEXIST")
			BODY.ShowAlert("<?echo I18N('j', 'The selected folder is not available. Please choose another folder !!');?>");
	},
	
	CheckPathValidity: function(path)
	{		
		var ajaxObj = GetAjaxObj("afpCheckSharePath");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.CheckPathCallback(xml);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("check.php", "act=checkdir&dirname="+path);
	},
	
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>
}
</script>
