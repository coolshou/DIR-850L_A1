<script type="text/javascript">
var EventName = null;
function Page() {}
Page.prototype =
{
	services: "UPNPAV",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	upnpav: null,
	InitValue: function(xml)
	{
		PXML.doc = xml;
		upnpav = PXML.FindModule("UPNPAV");
		initOverlay("white");			
		if (upnpav==="") { alert("InitValue ERROR!"); return false; }		
		OBJ("dms_active").checked   = (XG(upnpav+"/upnpav/dms/active")==="1");
		OBJ("dms_name").value       = XG(upnpav+"/upnpav/dms/name");		
		
		this.check_dms_enable(0);
												
		var sharepath = XG(upnpav+"/upnpav/dms/sharepath");						
		if(sharepath == "")
		{
			OBJ("dms_root").checked = false;	
			OBJ("the_sharepath").value ="/";
		}	
		else if(sharepath == "/")
		{
			OBJ("dms_root").checked = true;	
			OBJ("the_sharepath").value ="/";
		}
		else
		{
			OBJ("dms_root").checked = false;
			OBJ("the_sharepath").value = sharepath;
		}
				
		this.check_path(1);
		
		var rescan = XG(upnpav+"/upnpav/dms/rescan");
		if(rescan == "1")
		{
			window_make_new(-1, -1, 500, 110, "refresh.php?path=/","Now Scanning");
		}
		
		return true;
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
	PreSubmit: function()
	{				
		upnpav = PXML.FindModule("UPNPAV");		
		if(!this.check_sdstatus()) return null;		
		XS(upnpav+"/upnpav/dms/active", (OBJ("dms_active").checked ? "1":"0"));
		
		if(!this.OnCheckDMSName(OBJ("dms_name").value))
		{	
			BODY.ShowAlert("<?echo I18N("j","The input media library name is illegal.");?>");
			OBJ("dms_name").focus();
			return null;
		}
		else 
		XS(upnpav+"/upnpav/dms/name", OBJ("dms_name").value);		
		XS(upnpav+"/upnpav/dms/sharepath", OBJ("the_sharepath").value);				
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	disable_item: function(status)
	{
		if(status==1)	//enable
		{
			OBJ("dms_root").disabled=false;
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=false;
		}
		else
		{
			OBJ("dms_root").disabled=true;
			OBJ("the_sharepath").disabled=false;
			OBJ("But_Browse").disabled=true;
		}
	},
	check_dms_enable: function(flag)
	{
		if(flag != 0)
		{
			if(!this.check_sdstatus()) return;		
		}	
			
		if(OBJ("dms_active").checked == true)
		{
			this.disable_item(1);
			OBJ("dms_active").value=1;
		}else
		{
			this.disable_item(0);
			OBJ("dms_active").value=0;
		}		
	},
	open_browser: function()
	{	
		window_make_new(-1, -1, 500, 400, "portal/explorer.php?path=/","Explorer");
	},
	check_path: function(clear)
	{
		if (OBJ("dms_root").checked || clear!=1)
		{
			OBJ("chamber2").style.display = "none";
			OBJ("the_sharepath").value="/";
		}
		else
		{
			OBJ("chamber2").style.display = "";
		}	
	},
	refresh: function()
	{								
		var sharepath = XG(upnpav+"/upnpav/dms/sharepath");				
		
		if(sharepath!=OBJ("the_sharepath").value)
		{
			alert("The folder is changed. Please save setting before Refresh");			
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
		upnpav = PXML.FindModule("UPNPAV");
		var storage_count = XG(upnpav+"/runtime/device/storage/count");		
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
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>
}
</script>
