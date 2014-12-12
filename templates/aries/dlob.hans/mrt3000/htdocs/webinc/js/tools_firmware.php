<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "INET.WAN-1, RUNTIME.PHYINF, RUNTIME.FIRMWARE",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var wan	= PXML.FindModule("INET.WAN-1");
		var rphy = PXML.FindModule("RUNTIME.PHYINF");
		var rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", XG(wan+"/inf/phyinf"), false);
		if(XG(rwanphyp+"/linkstatus")=="")	{ OBJ("chkfw_btn").disabled = true; }
		
		return true;
	},
	PreSubmit: function()
	{
		PXML.CheckModule("RUNTIME.FIRMWARE", null, null, "ignore");
		PXML.IgnoreModule("INET.WAN-1");
		PXML.IgnoreModule("RUNTIME.PHYINF");
		
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	fw_ver: "<?echo query("/runtime/device/firmwareversion");?>",
	OnClickChkFW: function()
	{
		OBJ("chkfw_btn").disabled = "ture";
		OBJ("fw_message").style.display="block";
		OBJ("fw_message").innerHTML = "<?echo I18N("j","Connecting with the server for firmware information");?> ...";
		var ajaxObj = GetAjaxObj("checkfw");
		var times = 1;
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			setTimeout('PAGE.GetCheckReport()',5*1000);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("service.cgi", "EVENT=CHECKFW");
	},
	GetCheckReport: function()
	{
		var ajaxObj = GetAjaxObj("checkreport");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			
			if(xml.Get("/firmware/state") == "WAIT")
				setTimeout('PAGE.GetCheckReport()',5*1000);
			else if(xml.Get("/firmware/state") == "NORESPONSE")
				OBJ("fw_message").innerHTML = "<?echo I18N("j","No response. Please make sure you are connected properly to the internet.");?>";
			else
				PAGE.UpdateFWInfo(xml);
				
			/*var havenewfw = xml.Get("/firmware/havenewfirmware");
			var state	  = xml.Get("/firmware/state");
			
			if(state == "NORESPONSE")
			{
				OBJ("fw_message").innerHTML = "<?echo I18N("j","No response. Please make sure you are connected properly to the internet.");?>";
			} 
			else if(havenewfw == "1")	OBJ("fw_message").innerHTML = "<?echo I18N("j","A new version is available.");?>";
			else	OBJ("fw_message").innerHTML = "<?echo I18N("j","This firmware is the latest version.");?>";*/
			OBJ("chkfw_btn").disabled = "";
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("firmversion.php", "act=checkreport");
	},
	OnClickDownloadFW: function()
	{
		var downloadSelect = OBJ("fw_dl_locs");
		var selectBox = downloadSelect.selectedIndex;
		var path = downloadSelect.options[selectBox].value;
		self.location.href=path;
	},
	UpdateFWInfo: function(xmlDoc)
	{
		var xmlFirmwareVerMajor = xmlDoc.XDoc.getElementsByTagName("major")[0].childNodes[0].nodeValue;
		var xmlFirmwareVerMinor = xmlDoc.XDoc.getElementsByTagName("minor")[0].childNodes[0].nodeValue;
		xmlFirmwareVerMajor=3;
		/* get major and minor from /tmp/fwinfo.xml and write to db */
		var firmware_p = PXML.FindModule("RUNTIME.FIRMWARE");
		XS(firmware_p+"/runtime/firmware/fwversion/Major",xmlFirmwareVerMajor);
		XS(firmware_p+"/runtime/firmware/fwversion/Minor",xmlFirmwareVerMinor);
		OBJ("langup").setAttribute("modified", "true"); //bypass dirty check

		var cfw_maj=0, cfw_min=0;
		var fw_maj=parseInt(xmlFirmwareVerMajor, [10]);
		var fw_min=parseInt(xmlFirmwareVerMinor, [10]);
		
		var xmlFirmwareDate = xmlDoc.XDoc.getElementsByTagName("date")[0].childNodes[0].nodeValue;
		var i=1, j=0;
		var ie = is_IE();
		var Loc_list=OBJ("fw_dl_locs");
		var tr_ckfwver_obj = document.getElementsByName("tr_ckfwver");

		// Get sw ver
		fwstr  = PAGE.fw_ver.split(".");
		cfw_maj = parseInt(fwstr[0], [10]);
		cfw_min = parseInt(fwstr[1], [10]);

		if((fw_maj < cfw_maj) || (fw_maj == cfw_maj && fw_min <= cfw_min))
		{
			OBJ("fw_message").innerHTML = "<?echo I18N("j","This firmware is the latest version.");?>";
			//OBJ("div_ckfwver").style.display="none";
			if (tr_ckfwver_obj.length)
			{
				for (var k=0; k<tr_ckfwver_obj.length; k++)
					tr_ckfwver_obj[k].style.display="none";
			}
			return;
		}
		OBJ("fw_message").innerHTML = "<?echo I18N("j","A new version is available.");?>";
		//OBJ("div_ckfwver").style.display="";
		if (tr_ckfwver_obj.length)
		{
			for (var k=0; k<tr_ckfwver_obj.length; k++)
				tr_ckfwver_obj[k].style.display="";
		}

		while(Loc_list.firstChild != null) /* check whether select box has member inside, if yes remove them */
			Loc_list.removeChild(Loc_list.firstChild);

		if (ie) /* browser is IE */
		{
			var xmlDownloadLocation = xmlDoc.XDoc.getElementsByTagName("urls")[0].childNodes[0].nodeName;
			var xmlFirmwareDownload = xmlDoc.XDoc.getElementsByTagName("url")[0].childNodes[0].nodeValue;
			var opt = document.createElement("option");
			opt.text = xmlDownloadLocation;
			opt.value = xmlFirmwareDownload;
			Loc_list.options.add(opt);
		}
		else /* besides IE */
		{
			var xmlDownloadLocation = xmlDoc.XDoc.getElementsByTagName("urls")[0].childNodes[0].nodeName;
			var xmlFirmwareDownload = xmlDoc.XDoc.getElementsByTagName("url")[0].childNodes[0].nodeValue;
			var opt = document.createElement("option");
			opt.text = xmlDownloadLocation;
			opt.value = xmlFirmwareDownload;
			Loc_list.appendChild(opt);
		}

		/* Put xml data to html */
		var serverFirmwareVer = ("v"+ xmlFirmwareVerMajor + "." + xmlFirmwareVerMinor);

		OBJ("latest_fw_ver").innerHTML	= serverFirmwareVer;
		OBJ("latest_fw_date").innerHTML	= xmlFirmwareDate;
		
		/* write major and minor to db */
		BODY.OnSubmit();
	}
}
// return true is brower is IE.
function is_IE()
{
	if (navigator.userAgent.indexOf("MSIE")>-1) return true;
	return false
}
</script>
