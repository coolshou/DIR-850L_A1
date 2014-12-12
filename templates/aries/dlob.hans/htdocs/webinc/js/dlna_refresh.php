<script type="text/javascript">
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
		if (upnpav==="") { alert("InitValue ERROR!"); return false; }		
		this.bodyStyle();
		this.ScanMediaUpdate();				
		return true;
	},
	PreSubmit: function()
	{										
		PXML.IgnoreModule("UPNPAV");		
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	setSB: function(v) 
	{
		OBJ("sbChild1").style.width= parseInt(v)*2 + "px";		
		OBJ("percent").innerHTML = v + "%";	
		if(v=="100")
		{
			OBJ("scan").innerHTML = "Scan Successful";	
		}
		else
		{
			OBJ("scan").innerHTML = "Please wait a moment";	
		}	
	},
	bodyStyle: function()
	{
		var ieorff = (navigator.appName=="Microsoft Internet Explorer"?"IE":"FF"); //default
		var main=OBJ("mainDIV");
		main.style.padding='0px';
		if(ieorff=="FF")
			main.style.width='98%';
		else	//IE
			main.style.width='104%';
		main.style.position='absolute';
		main.style.left='0px';
		main.style.top='0px';
		return;
	},	
	ScanMediaUpdate: function()
	{											
		var ajaxObj = GetAjaxObj("DLNAGetState");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.ScanInProgressCallBack(xml);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("dlnastate.php", "dummy=dummy");
	},
	ScanInProgressCallBack: function(xml)
	{
		var usb_device		 	= xml.Get("/dlnastate/device/storage/count");
		if(usb_device==="0")
		{
			BODY.ShowAlert("<?echo I18N("j", "The USB device is withdrawn!!!");?>");
			parent.Page.prototype.CloseScanWindow();
			return false;
		}
		var total_file 			= xml.Get("/dlnastate/total_file");			if(total_file=="0") total_file="1";
		var current_scanned 	= xml.Get("/dlnastate/current_scanned");
		var scan_done 			= xml.Get("/dlnastate/scan_done");
	
		var progress	= Math.round(current_scanned/total_file*100,2);
		
		//BODY.ShowAlert("total_file="+total_file+",current="+current_scanned+",done="+scan_done);
		if(scan_done=="1" || (parseInt(progress) >= 100))
			progress = "100";			
			
		this.ShowPath(progress);
		
		if (scan_done!="1")
			this.dlna_timer = setTimeout('PAGE.ScanMediaUpdate()',3000);
	},
	ShowPath: function(txt)
		{
		this.setSB(txt);
	},		
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>
}
</script>
