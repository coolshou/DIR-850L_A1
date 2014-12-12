<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "DEVICE.ACCOUNT,HTTP.WAN-1,HTTP.WAN-2,INBFILTER,SHAREPORT,SAMBA",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			OBJ("en_remote").disabled = true;
			OBJ("remote_port").disabled = true;
		}
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function (code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			if (COMM_Equal(OBJ("en_remote").getAttribute("modified"), "true") || COMM_Equal(OBJ("remote_port").getAttribute("modified"), "true"))
			{
				BODY.Logout();
				BODY.CloseDialog();
				BODY.ShowLogin();
			}
			else
			{
				BODY.OnReload();
			}
			break;
		case "BUSY":
			BODY.ShowMessage("<?echo I18N("j","Error");?>","<?echo I18N("j","Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			BODY.ShowMessage("<?echo I18N("j","Error");?>",result.Get("/hedwig/message"));
			break;
		case "PIGWIDGEON":
			if (result.Get("/pigwidgeon/message")=="no power")
			{
				BODY.NoPower();
			}
			else
			{
				BODY.ShowMessage("<?echo I18N("j","Error");?>",result.Get("/pigwidgeon/message"));
			}
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
		PXML.IgnoreModule("INBFILTER");
		PXML.CheckModule("SHAREPORT", "ignore",null, "ignore"); 
		if (!this.Initial()) return false;
		return true;
	},
	PreSubmit: function()
	{
		if (!this.SaveXML()) return null;
		PXML.ActiveModule("HTTP.WAN-1");
		PXML.ActiveModule("HTTP.WAN-2");
		if ( COMM_Equal(OBJ("admin_p1").getAttribute("modified"), "true") || COMM_Equal(OBJ("admin_p2").getAttribute("modified"), "true") )
		{
			PXML.CheckModule("SAMBA", "ignore", "ignore", null); //SAMBA need password
		}	
		else
		{	
			PXML.IgnoreModule("SAMBA");
		}			
		
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	admin: null,
	usr: null,
	actp: null,
	captcha: null,
	rcp: null,
	rcp2:null,
	rport: null,
	//stunnel: null,
	https_rport: null,
	//inbfilter: null,
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	Initial: function()
	{
		this.actp = PXML.FindModule("DEVICE.ACCOUNT");
		this.rcp = PXML.FindModule("HTTP.WAN-1");
		this.rcp2= PXML.FindModule("HTTP.WAN-2");
		if (!this.actp||!this.rcp||!this.rcp2)
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","Initial() ERROR!!!");
			return false;
		}
		this.gw_name=this.actp + "/device/gw_name";
		this.captcha = this.actp + "/device/session/captcha";
		this.actp += "/device/account";
		//this.inbfilter = this.rcp+"/inf/inbfilter";
		
		//this.stunnel = this.rcp+"/inf/stunnel";
		this.https_rport  = this.rcp+"/inf/https_rport";
		this.rcp += "/inf/web";
		this.rcp2+= "/inf/web";
		this.rport = XG(this.rcp);
		this.admin = OBJ("admin_p1").value = OBJ("admin_p2").value = XG(this.actp+"/entry:1/password");
		this.usr = OBJ("usr_p1").value = OBJ("usr_p2").value = XG(this.actp+"/entry:2/password");
		OBJ("en_captcha").checked = COMM_EqBOOL(XG(this.captcha), true);

		//OBJ("stunnel").checked = COMM_EqSTRING(XG(this.stunnel), "1");
		//OBJ("enable_https").checked = !COMM_EqSTRING(XG(this.https_rport), "");

		if(!COMM_EqSTRING(this.rport, "") || !COMM_EqSTRING(XG(this.https_rport), ""))
		{
			OBJ("en_remote").checked = true;
		}
		else
		{
			OBJ("en_remote").checked = false;
		}
		//OBJ("remote_inb_filter").value = XG(this.inbfilter);
		this.OnClickEnRemote();
		//this.OnClickStunnel();
		//this.OnClickEnableHttps();
		//this.OnClickRemoteInbFilter(XG(this.inbfilter));
		OBJ("remote_port").value=COMM_EqSTRING(XG(this.rcp), "") ? 8080:XG(this.rcp);
		OBJ("gw_name").value = XG(this.gw_name);
		return true;
	},
	SaveXML: function()
	{
		if (!COMM_EqSTRING(OBJ("admin_p1").value, OBJ("admin_p2").value))
		{
			BODY.ShowConfigError("admin_p2", "<?echo I18N("j","Password and Verify Password do not match.");?>");
			return false;
		}
		if (!COMM_EqSTRING(OBJ("admin_p1").value, this.admin))
		{
			XS(this.actp+"/entry:1/password", OBJ("admin_p1").value);
		}
		if (!COMM_EqSTRING(OBJ("usr_p1").value, OBJ("usr_p2").value))
		{
			BODY.ShowConfigError("usr_p2", "<?echo I18N("j","Password and Verify Password do not match.");?>");
			return false;
		}
		if (!COMM_EqSTRING(OBJ("usr_p1").value, this.usr))
		{
			XS(this.actp+"/entry:2/password", OBJ("usr_p1").value);
		}
		if (OBJ("en_captcha").checked)
		{
			XS(this.captcha, "1");
			BODY.enCaptcha = true;
		}
		else
		{
			XS(this.captcha, "0");
			BODY.enCaptcha = false;
		}
		if (OBJ("en_remote").checked)
		{
			if (!TEMP_IsDigit(OBJ("remote_port").value))
			{
				BODY.ShowConfigError("remote_port", "<?echo I18N("j","Invalid remote admin port number.");?>");
				return false;
			}
/*			
			XS(this.inbfilter, OBJ("remote_inb_filter").value);
			if (OBJ("enable_https").checked)
			{
				XS(this.rcp, "");
				XS(this.https_rport, OBJ("remote_port").value);
			}
			else
			{
				XS(this.rcp, OBJ("remote_port").value);
				XS(this.rcp2,OBJ("remote_port").value);
				XS(this.https_rport, "");
			}
*/			
				XS(this.rcp, OBJ("remote_port").value);
				XS(this.rcp2,OBJ("remote_port").value);
		}
		else
		{
			XS(this.rcp, "");
			XS(this.https_rport, "");
		}
	
/*
		if (OBJ("stunnel").checked)
		{
			XS(this.stunnel, "1");
		}
		else
		{
			XS(this.stunnel, "0");
		}
*/		
		if(!COMM_EqSTRING(OBJ("gw_name").value,"") )
		{
			var ori_gwname = XG(this.gw_name);
			if(!this.OnCheckGwName(OBJ("gw_name").value))
			{	
				BODY.ShowConfigError("gw_name", "<?echo I18N("j","Illegal gateway name");?>");
				OBJ("gw_name").focus();
				return false;
			}
			
			if(OBJ("gw_name").value != ori_gwname)
			{
				XS(this.gw_name, OBJ("gw_name").value);
				this.OnSetGwName(OBJ("gw_name").value);
			}
		}
		return true;
	},
	OnCheckGwName: function(gwname)
	{
		var reg = new RegExp("[A-Za-z0-9\-]{"+gwname.length+"}");
		/* the label must start with a letter */
		if (!gwname.match(/^[A-Za-z]/))
		{
			return false;
		}
		/* the label has interior characters that only letters, digits and hyphen */
		else if (!reg.exec(gwname))
		{
			return false;
		}
		
		return true;
	},
	/*we now change gw_name for shareport without restarting the SHAREPORT service !!*/
	OnSetGwName: function(gwname)
	{
		var ajaxObj = GetAjaxObj("Shareport");
		var action 	= "sethostname";
		var value 	= gwname;
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			PAGE.OnSubmitCallback(xml.Get("/shareportreport/result"), xml.Get("/shareportreport/reason"));
		}
		
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("shareport.php", "action="+action+"&value="+value);
	},
/*	
	OnClickStunnel: function()
	{
		if (OBJ("stunnel").checked && OBJ("en_remote").checked)	
		{
			OBJ("enable_https").disabled = false;
		}
		else							
		{
			OBJ("enable_https").disabled = true;
			OBJ("enable_https").checked = false;
		}
		this.OnClickEnableHttps();
	},
	
	OnClickEnableHttps: function()
	{
		if (OBJ("enable_https").checked)	
		{
			OBJ("remote_port").value=COMM_EqSTRING(XG(this.https_rport), "") ? 8181:XG(this.https_rport);
		}
		else
		{
			OBJ("remote_port").value=COMM_EqSTRING(XG(this.rcp), "") ? 8080:XG(this.rcp);
		}
	},	
*/	
	OnClickEnRemote: function()
	{
		//this.OnClickStunnel();
		if (OBJ("en_remote").checked)	
			OBJ("remote_port").disabled = false;//= OBJ("remote_inb_filter").disabled = false;//OBJ("inb_filter_detail").disabled = false;
		else							
			OBJ("remote_port").disabled = true;//OBJ("remote_inb_filter").disabled = OBJ("inb_filter_detail").disabled = true;
	},
/*	
	OnClickRemoteInbFilter: function(inbf_uid)
	{
		var str = "";
		if (inbf_uid === "")	str = "Allow All";
		else if (inbf_uid === "denyall") str = "Deny All";
		else
		{
			var p = PXML.FindModule("INBFILTER");
			var s = PXML.doc.GetPathByTarget(p+"/acl/inbfilter", "entry", "uid", inbf_uid, false);
			var c = S2I(XG(s+"/iprange/entry#"));
			if(XG(s+"/act") === "allow")	str = "Allow ";
			else	str = "Deny ";
			var	d="", startip="", endip="";	
			for(var i=1; i <= c; i++)
			{
				if(XG(s+"/iprange/entry:"+i+"/enable") === "1")
				{			
					startip = XG(s+"/iprange/entry:"+i+"/startip");
					endip 	= XG(s+"/iprange/entry:"+i+"/endip");
					str+=d+startip+"~"+endip;
					d=",\n";	
				}	
			}				
			
		}		
		OBJ("inb_filter_detail").value = str;
		OBJ("inb_filter_detail").disabled = true;
	},
*/	
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

//]]>
</script>
