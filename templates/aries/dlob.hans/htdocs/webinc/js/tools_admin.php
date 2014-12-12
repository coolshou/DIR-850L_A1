<script type="text/javascript">
function Page() {}
Page.prototype =
{
	<?
		if(isfile("/etc/services/UPNPC.php")==1) echo 'services: "DEVICE.ACCOUNT,HTTP.WAN-1,HTTP.WAN-2,INBFILTER,SHAREPORT,UPNPC,SAMBA",';
		else echo 'services: "DEVICE.ACCOUNT,HTTP.WAN-1,HTTP.WAN-2,INBFILTER,SHAREPORT,SAMBA",';
	?>	
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			OBJ("en_remote").disabled = true;
			OBJ("remote_port").disabled = true;
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			if (COMM_Equal(OBJ("en_remote").getAttribute("modified"), "true") || COMM_Equal(OBJ("remote_port").getAttribute("modified"), "true"))
			{
				AUTH.Logout();
				BODY.ShowLogin();
			}
			else
			{
				BODY.OnReload();
			}
			break;
		case "BUSY":
			BODY.ShowAlert("<?echo i18n("Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			BODY.ShowAlert(result.Get("/hedwig/message"));
			break;
		case "PIGWIDGEON":
			if (result.Get("/pigwidgeon/message")=="no power")
			{
				BODY.NoPower();
			}
			else
			{
				BODY.ShowAlert(result.Get("/pigwidgeon/message"));
			}
			break;
		}
		return true;
	},
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
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	admin: null,
	usr: null,
	actp: null,
	captcha: null,
	russia: false,
	rcp: null,
	rcp2:null,
	rport: null,
	stunnel: null,
	https_rport: null,
	inbfilter: null,
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	Initial: function()
	{
		this.actp = PXML.FindModule("DEVICE.ACCOUNT");
		this.rcp = PXML.FindModule("HTTP.WAN-1");
		this.rcp2= PXML.FindModule("HTTP.WAN-2");
		if (!this.actp||!this.rcp||!this.rcp2)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		this.gw_name=this.actp + "/device/gw_name";
		this.captcha = this.actp + "/device/session/captcha";
		this.actp += "/device/account";
		this.inbfilter = this.rcp+"/inf/inbfilter";
		this.russia = (XG(this.rcp2+"/inf/nat")=="NAT-1" && XG(this.rcp2+"/inf/active")=="1")?true:false;
		
		this.stunnel = this.rcp+"/inf/stunnel";
		this.https_rport  = this.rcp+"/inf/https_rport";
		this.rcp += "/inf/web";
		this.rcp2+= "/inf/web";
		this.rport = XG(this.rcp);
		this.admin = OBJ("admin_p1").value = OBJ("admin_p2").value = XG(this.actp+"/entry:1/password");
		this.usr = OBJ("usr_p1").value = OBJ("usr_p2").value = XG(this.actp+"/entry:2/password");
		OBJ("en_captcha").checked = COMM_EqBOOL(XG(this.captcha), true);

		OBJ("stunnel").checked = COMM_EqSTRING(XG(this.stunnel), "1");
		OBJ("enable_https").checked = !COMM_EqSTRING(XG(this.https_rport), "");

		if(!COMM_EqSTRING(this.rport, "") || !COMM_EqSTRING(XG(this.https_rport), ""))
		{
			OBJ("en_remote").checked = true;
		}
		else
		{
			OBJ("en_remote").checked = false;
		}
		OBJ("remote_inb_filter").value = XG(this.inbfilter);
		this.OnClickEnRemote();
		this.OnClickStunnel();
		//this.OnClickEnableHttps();
		this.OnClickRemoteInbFilter(XG(this.inbfilter));
		OBJ("gw_name").value = XG(this.gw_name);
		return true;
	},
	SaveXML: function()
	{
		/* The IE browser would treat the text with all spaces as empty according as 
			it would ignore the text node with all spaces in XML DOM tree for IE6, 7, 8, 9.*/		
		if(COMM_IsAllSpace(OBJ("admin_p1").value))
		{
			BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
			return false;
		}
		for(var i=0;i < OBJ("admin_p1").value.length;i++)
		{
			if (OBJ("admin_p1").value.charCodeAt(i) > 256) //avoid holomorphic word
			{ 
				BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
				return false;
			}
		}
		for(var i=0;i < OBJ("usr_p1").value.length;i++)
		{
			if (OBJ("usr_p1").value.charCodeAt(i) > 256) //avoid holomorphic word
			{ 
				BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
				return false;
			}
		}		
		if (!COMM_EqSTRING(OBJ("admin_p1").value, OBJ("admin_p2").value))
		{
			BODY.ShowAlert("<?echo i18n("Password and Verify Password do not match. Please reconfirm admin password.");?>");
			return false;
		}
		if (!COMM_EqSTRING(OBJ("admin_p1").value, this.admin))
		{
			XS(this.actp+"/entry:1/password", OBJ("admin_p1").value);
		}
		if (!COMM_EqSTRING(OBJ("usr_p1").value, OBJ("usr_p2").value))
		{
			BODY.ShowAlert("<?echo i18n("Password and Verify Password do not match. Please reconfirm user password.");?>");
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
				BODY.ShowAlert("<?echo i18n("The remote admin port number is not valid.");?>");
				return false;
			}
			if(!this.PortConflictCheck()) return false;	
			XS(this.inbfilter, OBJ("remote_inb_filter").value);
			if (OBJ("enable_https").checked)
			{
				XS(this.rcp, "");
				if(this.russia) XS(this.rcp2, "");
				XS(this.https_rport, OBJ("remote_port").value);
			}
			else
			{
				XS(this.rcp, OBJ("remote_port").value);
				if(this.russia) XS(this.rcp2,OBJ("remote_port").value);
				XS(this.https_rport, "");
			}
		}
		else
		{
			XS(this.rcp, "");
			if(this.russia) XS(this.rcp2, "");
			XS(this.https_rport, "");
		}
		
		if (OBJ("stunnel").checked)
		{
			XS(this.stunnel, "1");
		}
		else
		{
			XS(this.stunnel, "0");
		}
		
		if(!COMM_EqSTRING(OBJ("gw_name").value,"") )
		{
			var ori_gwname = XG(this.gw_name);
			if(!this.OnCheckGwName(OBJ("gw_name").value))
			{	
				BODY.ShowAlert("<?echo I18N("j","The input gateway name is illegal.");?>");
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
			OBJ("remote_port").value=COMM_EqSTRING(XG(this.https_rport), "") ? 8090:XG(this.https_rport);
		}
		else
		{
			OBJ("remote_port").value=COMM_EqSTRING(XG(this.rcp), "") ? 8080:XG(this.rcp);
		}
	},	
	
	OnClickEnRemote: function()
	{
		this.OnClickStunnel();
		if (OBJ("en_remote").checked)	
			OBJ("remote_port").disabled = OBJ("remote_inb_filter").disabled = false;//OBJ("inb_filter_detail").disabled = false;
		else							
			OBJ("remote_port").disabled = OBJ("remote_inb_filter").disabled = OBJ("inb_filter_detail").disabled = true;
	},
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
	PortConflictCheck: function()
	{
		//Check the port confliction between STORAGE, VSVR, PFWD and REMOTE.
		var webaccess_remote_en = <? if(query("/webaccess/enable")==1 && query("/webaccess/remoteenable")==1) echo "true" ;else echo "false";?>;
		var webaccess_httpport = "<? echo query("/webaccess/httpport");?>";
		var webaccess_httpsport = "<? echo query("/webaccess/httpsport");?>";
		if(webaccess_remote_en && (OBJ("remote_port").value==webaccess_httpport || OBJ("remote_port").value==webaccess_httpsport))
		{
			BODY.ShowAlert("<?echo i18n("The remote admin port could not be the same as the web access port in the storage function.");?>");
			return false;	
		}
		else if (OBJ("remote_port").value==8182 || OBJ("remote_port").value==8183)//mydlink shareport agent used
		{
		    BODY.ShowAlert("<?echo i18n("The HTTPS Access Port number is not valid.");?>");
			return false;
		}
		<?
			$vsvr_port_str = "";
			foreach ("/nat/entry:1/virtualserver/entry")
			{
				if(query("protocol")=="TCP" || scut_count(query("protocol"),"TCP")!=0)
				{ $vsvr_port_str = $vsvr_port_str.",".query("external/start")."-".query("external/end");}
				if(query("tport_str")!="")
				{ $vsvr_port_str = $vsvr_port_str.",".query("tport_str");}	
			}
			$pfwd_port_str = "";
			foreach ("/nat/entry:1/portforward/entry")
			{
				if(query("protocol")=="TCP" || scut_count(query("protocol"),"TCP")!=0)
				{ $pfwd_port_str = $pfwd_port_str.",".query("external/start")."-".query("external/end");}	
				if(query("tport_str")!="")
				{ $pfwd_port_str = $pfwd_port_str.",".query("tport_str");}	
			}				
		?>
		var pfwd_port_str = "<? echo $pfwd_port_str;?>";
		var vsvr_port_str = "<? echo $vsvr_port_str;?>";
		if(PortStringCheck(pfwd_port_str, OBJ("remote_port").value))
		{
			BODY.ShowAlert("<?echo i18n("The remote admin port had been used in the port forwarding function.");?>");
			return false;			
		}
		if(PortStringCheck(vsvr_port_str, OBJ("remote_port").value))
		{
			BODY.ShowAlert("<?echo i18n("The remote admin port had been used in the virtual server function.");?>");
			return false;			
		}		
		return true;
	}
}
function PortStringCheck(PortString, port)
{
	var PortStrArr = PortString.split(",");
	port = parseInt(port, 10)
	for(var i=0; i < PortStrArr.length; i++)
	{
		if(PortStrArr[i].match("-")=="-")
		{
			var PortRange = PortStrArr[i].split("-");
			var PortRangeStart	= parseInt(PortRange[0], 10);
			var PortRangeEnd	= parseInt(PortRange[1], 10);
			if(PortRangeStart <= port && port <= PortRangeEnd) return true;
		}	
		else if(PortStrArr[i]!="")
		{
			if(parseInt(PortStrArr[i], 10)==port) return true;
		}
	}
	return false;
}
</script>
