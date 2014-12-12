<script type="text/javascript">
var EventName = null;
function Page() {}
Page.prototype =
{
	services: "ITUNES",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	itunes: null,
	InitValue: function(xml)
	{
		PXML.doc = xml;
		itunes = PXML.FindModule("ITUNES");
		initOverlay("white");			
		if (itunes==="") { alert("InitValue ERROR!"); return false; }				
		this.set_radio_value("itunes_active", XG(itunes+"/itunes/server/active"));		
		
		this.check_itunes_enable(0, "itunes_active");
												
		var sharepath = XG(itunes+"/itunes/server/sharepath");						
		if(sharepath == "")
		{
			OBJ("itunes_root").checked = false;	
			OBJ("the_sharepath").value ="/";
		}	
		else if(sharepath == "/")
		{
			OBJ("itunes_root").checked = true;	
			OBJ("the_sharepath").value ="/";
		}
		else
		{
			OBJ("itunes_root").checked = false;
			OBJ("the_sharepath").value = sharepath;
			/* check the sharepath value is exist */
			this.CheckPathValidity(sharepath);
		}
				
		this.check_path(1);			
		
		return true;
	},
	PreSubmit: function()
	{				
		itunes = PXML.FindModule("ITUNES");		
		if(!this.check_sdstatus()) return null;		
		XS(itunes+"/itunes/server/active", this.get_radio_value("itunes_active"));			
		XS(itunes+"/itunes/server/sharepath", OBJ("the_sharepath").value);				
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	disable_item: function(status)
	{
		if(status==1)	//enable
		{
			OBJ("itunes_root").disabled=false;
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=false;			
		}
		else
		{
			OBJ("itunes_root").disabled=true;
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
	check_itunes_enable: function(flag, name)
	{
		if(flag != 0)
		{
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
		itunes = PXML.FindModule("ITUNES");
		var storage_count = XG(itunes+"/runtime/device/storage/count");		
		if((storage_count != "0" && storage_count != "") || OBJ("itunes_active").checked==false)
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
	
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>
}
</script>
