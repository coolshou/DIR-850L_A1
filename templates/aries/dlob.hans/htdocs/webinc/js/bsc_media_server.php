<script type="text/javascript">
var EventName = null;
function Page() {}
Page.prototype =
{
	services: "UPNPAV,ITUNES",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnReload: function()
	{
		//Reload the page to refresh $sharepath from PHP.
		self.location.href="bsc_media_server.php";
	},	
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	upnpav: null,
	itunes: null,
	path_selected: null,
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.upnpav = PXML.FindModule("UPNPAV");
		this.itunes = PXML.FindModule("ITUNES");
		initOverlay("white");			
		if (this.upnpav=="" || this.itunes=="") { alert("InitValue ERROR!"); return false; }
		
		COMM_SetRadioValue("dlna_active", XG(this.upnpav+"/upnpav/dms/active"));
		COMM_SetRadioValue("itunes_active", XG(this.itunes+"/itunes/server/active"));
		
		OBJ("dlna_name").value       = XG(this.upnpav+"/upnpav/dms/name");		
		
		this.check_sharepath("dlna");
		this.check_sharepath("itunes");
		this.check_enable("dlna");
		this.check_enable("itunes");
		
		this.check_sdstatus();	
		
		return true;
	},
	check_sharepath: function(service)
	{
		if(service=="dlna")
		{
			var sharepath = XG(this.upnpav+"/upnpav/dms/sharepath");
			var obj_root		= "dlna_root";
			var obj_sharepath	= "dlna_sharepath";
			var obj_chamber		= "dlna_chamber";
			<? $sharepath = "/var/tmp/storage/".get("", "/upnpav/dms/sharepath");?>
			var sharepath_exist = COMM_ToBOOL(<? echo isdir($sharepath);?>);
			if(!sharepath_exist) var sharepath_delete_msg = "<?echo I18N("j","The share path for DLNA server is eliminated.");?>";
		}
		else if(service=="itunes")
		{
			var sharepath = XG(this.itunes+"/itunes/server/sharepath");
			var obj_root		= "itunes_root";
			var obj_sharepath	= "itunes_sharepath";
			var obj_chamber		= "itunes_chamber";			
			<? $sharepath = "/var/tmp/storage/".get("", "/itunes/server/sharepath");?>
			var sharepath_exist = COMM_ToBOOL(<? echo isdir($sharepath);?>);
			if(!sharepath_exist) var sharepath_delete_msg = "<?echo I18N("j","The share path for iTunes server is eliminated.");?>";
		}
		
		if(sharepath == "")
		{
			OBJ(obj_root).checked = false;	
			OBJ(obj_sharepath).value ="/";
			OBJ(obj_chamber).style.display ="";
		}	
		else if(sharepath == "/")
		{
			OBJ(obj_root).checked = true;	
			OBJ(obj_sharepath).value ="/";
			OBJ(obj_chamber).style.display ="none";
		}
		else
		{
			OBJ(obj_root).checked = false;
			OBJ(obj_sharepath).value = sharepath;
			OBJ(obj_chamber).style.display ="";
			if(!sharepath_exist) BODY.ShowAlert(sharepath_delete_msg);
		}
	},
	PreSubmit: function()
	{	
		if(!this.check_sdstatus()) return null;
		
		if(!this.OnCheckDMSName(OBJ("dlna_name").value))
		{	
			BODY.ShowAlert("<?echo I18N("j","The input media library name is illegal.");?>");
			OBJ("dlna_name").focus();
			return null;
		}
		
		XS(this.upnpav+"/upnpav/dms/active", COMM_GetRadioValue("dlna_active"));
		XS(this.upnpav+"/upnpav/dms/name", OBJ("dlna_name").value);		
		XS(this.upnpav+"/upnpav/dms/sharepath", OBJ("dlna_sharepath").value);				
		
		XS(this.itunes+"/itunes/server/active", COMM_GetRadioValue("itunes_active"));			
		XS(this.itunes+"/itunes/server/sharepath", OBJ("itunes_sharepath").value);		
		return PXML.doc;
	},
	OnCheckDMSName: function(dmsname)
	{
		var reg = new RegExp("[A-Za-z0-9\-_]{"+dmsname.length+"}");
		/* the label must start with a letter */
		if (!dmsname.match(/^[A-Za-z]/))
		{
			return false;
		}
		/* the label has interior characters that only letters, digits and hyphen */
		else if (!reg.exec(dmsname))
			return false;
		return true;
	},	
	IsDirty: null,
	Synchronize: function() {},
	open_browser: function()
	{	
		window_make_new(-1, -1, 500, 400, "portal/explorer.php?path=/","Explorer");
	},
	refresh: function()
	{								
		var dlna_sharepath = XG(this.upnpav+"/upnpav/dms/sharepath");
		var itunes_sharepath = XG(this.itunes+"/itunes/server/sharepath");
		
		if(dlna_sharepath!=OBJ("dlna_sharepath").value || itunes_sharepath!=OBJ("itunes_sharepath").value)
		{
			alert("<?echo I18N("j","The folder is changed. Please save setting before Refresh");?>");			
		}
		else
		{									
			this.SendEvent("UPNPAV.REFRESH");		
			window_make_new(-1, -1, 500, 110, "dlna_refresh.php?path=/","Now Scanning");			
		}
		
		setTimeout(PAGE.CloseScanWindow, 550000);
	},
	check_sdstatus: function()
	{
		var storage_count = XG(this.upnpav+"/runtime/device/storage/count");		
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
	SendEvent: function(str)
	{
		var ajaxObj = GetAjaxObj(str);	
		if (EventName != null) return;
		
		EventName = str;			
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			//setTimeout("OnLoadBody()", 3*1000);
			EventName = null;
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("service.cgi", "EVENT="+EventName);
	},	
	CloseScanWindow: function()
	{				
		var ourDiv = document.getElementById((window_made_count-1)+"our_dragable_window");		
		if(ourDiv != null)
		{
			disalbeOverlay();		
			var ourParent = ourDiv.parentNode;
			ourParent.removeChild(ourDiv);			
		}	
	},
	check_enable: function(service)
	{
		if(service=="dlna")
		{
			if(COMM_GetRadioValue("dlna_active")=="1")
			{
				OBJ("dlna_name").disabled=false;
				OBJ("dlna_root").disabled=false;
				OBJ("dlna_sharepath").disabled=false;
				OBJ("dlna_browse").disabled=false;
			}	
			else
			{
				OBJ("dlna_name").disabled=true;
				OBJ("dlna_root").disabled=true;
				OBJ("dlna_sharepath").disabled=true;
				OBJ("dlna_browse").disabled=true;
			}		 
		}	
		else if(service=="itunes")
		{
			if(COMM_GetRadioValue("itunes_active")=="1")
			{
				OBJ("itunes_root").disabled=false;
				OBJ("itunes_sharepath").disabled=false;
				OBJ("itunes_browse").disabled=false;
			}	
			else
			{
				OBJ("itunes_root").disabled=true;
				OBJ("itunes_sharepath").disabled=true;
				OBJ("itunes_browse").disabled=true;
			}			
		}	
	},
	check_root_path: function(service)
	{
		if(service=="dlna")
		{
			if (OBJ("dlna_root").checked)
			{
				OBJ("dlna_chamber").style.display = "none";
				OBJ("dlna_sharepath").value="/";
			}
			else
				OBJ("dlna_chamber").style.display = "";
		}
		else if(service=="itunes")
		{
			if (OBJ("itunes_root").checked)
			{
				OBJ("itunes_chamber").style.display = "none";
				OBJ("itunes_sharepath").value="/";
			}
			else
				OBJ("itunes_chamber").style.display = "";			
		}				
	},
	window_destroy_callback: function()
	{
		if(this.path_selected=='dlna')
			OBJ("dlna_sharepath").value = OBJ("the_sharepath").value;
		else if(this.path_selected=='itunes')
			OBJ("itunes_sharepath").value = OBJ("the_sharepath").value;
	},	
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>
}
</script>
