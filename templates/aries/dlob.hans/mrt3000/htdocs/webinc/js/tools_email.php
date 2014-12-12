<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	//services: "DEVICE.LOG,RUNTIME.LOG,EMAIL,SCHEDULE",
	services: "DEVICE.LOG,RUNTIME.LOG,EMAIL",
	OnLoad: function()
	{
		//if (!this.rgmode) BODY.DisableCfgElements(true);
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: null,
	
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
		
		devlog_p = PXML.FindModule("DEVICE.LOG");	
		//this.schp = PXML.FindModule("SCHEDULE");
		if (!devlog_p) { BODY.ShowMessage("<?echo I18N("j","Error");?>","<?echo I18N("j","InitValue ERROR!");?>"); return false; }
		devlog_p += "/device/log";
		
		OBJ("en_mail").checked			= (XG(devlog_p+"/email/enable")==="1");
		OBJ("from_addr").value			= XG(devlog_p+"/email/from");
		OBJ("to_addr").value			= XG(devlog_p+"/email/to");
		OBJ("email_subject").value		= XG(devlog_p+"/email/subject");
		OBJ("smtp_server_addr").value	= XG(devlog_p+"/email/smtp/server");
		if(XG(devlog_p+"/email/smtp/port")=="")	OBJ("smtp_server_port").value = "25"
		else					OBJ("smtp_server_port").value = XG(devlog_p+"/email/smtp/port");
		OBJ("authenable").checked = (XG(devlog_p+"/email/authenable") == 1);
		OBJ("account_name").value		= XG(devlog_p+"/email/smtp/user");
		OBJ("passwd").value = OBJ("verify_passwd").value = XG(devlog_p+"/email/smtp/password");
		this.OnClickAuthEnable();
		/*OBJ("en_logfull").checked			= (XG(devlog_p+"/email/logfull")==="1");
		OBJ("en_log_sch").checked			= (XG(devlog_p+"/email/logsch")==="1");
		COMM_SetSelectValue(OBJ("log_sch"), XG(devlog_p+"/email/schedule"));
		OBJ("log_detail").disabled 			= true;*/
		this.OnClickEnable();
		//this.OnClickEnableSchedule();
		//this.OnChangeSchedule();

		return true;
	},
	PreSubmit: function()
	{
		if(!this.EmailErrorCheck()) return null;
		XS(devlog_p+"/email/enable",		OBJ("en_mail").checked ? "1" : "0");
		XS(devlog_p+"/email/from",			OBJ("from_addr").value);
		XS(devlog_p+"/email/to",			OBJ("to_addr").value);
		XS(devlog_p+"/email/subject",		OBJ("email_subject").value);
		XS(devlog_p+"/email/smtp/server",	OBJ("smtp_server_addr").value);
		XS(devlog_p+"/email/smtp/port",		OBJ("smtp_server_port").value);
		if(OBJ("authenable").checked)
		{
			XS(devlog_p+"/email/authenable",	"1");
			XS(devlog_p+"/email/smtp/user",		OBJ("account_name").value);
			XS(devlog_p+"/email/smtp/password",	OBJ("passwd").value);
		}
		else
			XS(devlog_p+"/email/authenable",	"0");
		//XS(devlog_p+"/email/logfull",		OBJ("en_logfull").checked ? "1" : "0");
		//XS(devlog_p+"/email/logsch",		OBJ("en_log_sch").checked ? "1" : "0");
		/*if(OBJ("en_log_sch").checked)
			XS(devlog_p+"/email/schedule",		(OBJ("log_sch").value==="-1") ? "" : OBJ("log_sch").value);
		else
			XS(devlog_p+"/email/schedule",		"");*/

		PXML.IgnoreModule("RUNTIME.LOG");	
		//PXML.IgnoreModule("SCHEDULE");		
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	devlog_p: null,
	//schp: null,

	EmailErrorCheck: function()
	{
		var fro = OBJ("from_addr").value;
		var to = OBJ("to_addr").value;
		var sub = OBJ("email_subject").value;
		var server = OBJ("smtp_server_addr").value;
		var SMTP_port = OBJ("smtp_server_port").value;
		var user = OBJ("account_name").value;
		var passwd = OBJ("passwd").value;
		var verpasswd = OBJ("verify_passwd").value;
	
		if(this.IsBlank(fro))
		{
			BODY.ShowConfigError("from_addr", "<?echo I18N("j","Please enter a valid Email Address.");?>");
			OBJ("from_addr").focus();
			return null;
		}
		if(this.IsBlank(to))
		{
			BODY.ShowConfigError("to_addr", "<?echo I18N("j","Please enter a valid Email Address.");?>");
			OBJ("to_addr").focus();
			return null;
		}
		if(!this.IsVaildEmail(fro))
		{
			BODY.ShowConfigError("from_addr", "<?echo I18N("j","Please enter a valid Email Address.");?>");
			OBJ("from_addr").focus();
			return null;
		}
		if(!this.IsVaildEmail(to))
		{
			BODY.ShowConfigError("to_addr", "<?echo I18N("j","Please enter a valid Email Address.");?>");
			OBJ("to_addr").focus();
			return null;
		}
		if(this.IsBlank(server))
		{
			BODY.ShowConfigError("smtp_server_addr", "<?echo I18N("j","Please enter another SMTP Server or IP Address.");?>");
			OBJ("smtp_server_addr").focus();
			return null;
		}
	/*	if(!this.IsIPv4(server))
		{
			BODY.ShowAlert("<?echo I18N("j","Invalid SMTP Server Address");?>");
			OBJ("smtp_server_addr").focus();
			return null;
		}*/		
		if(!this.IsVaildPort(SMTP_port))
		{
			BODY.ShowConfigError("smtp_server_port", "<?echo I18N("j","The SMTP port is invalid.");?>");
			OBJ("smtp_server_port").focus();
			return null;
		}
		if(OBJ("authenable").checked)
		{
			if(this.IsBlank(user))
			{
				BODY.ShowConfigError("account_name", "<?echo I18N("j","Please enter a user name.");?>");
				OBJ("account_name").focus();
				return null;
			}
			if(this.IsBlank(passwd))
			{
				BODY.ShowConfigError("passwd", "<?echo I18N("j","Please enter a valid Password.");?>");
				OBJ("passwd").focus();
				return null;
			}
			if(this.IsBlank(verpasswd))
			{
				BODY.ShowConfigError("verify_passwd", "<?echo I18N("j","Please enter a valid Password.");?>");
				OBJ("verify_passwd").focus();
				return null;
			}
			if(passwd != verpasswd)
			{
				BODY.ShowConfigError("passwd", "<?echo I18N("j","Password doesn't match.");?>");
				OBJ("passwd").focus();
				return null;
			}
		}
		return true;
	},	
	OnClickSendMail: function()
	{
		if(COMM_IsDirty(false))
		{ 
			BODY.ShowMessage("<?echo I18N("j","Error");?>","<?echo I18N("j","The email settings have been modified, please save the settings first.");?>");
			return null;
		}
		if(!this.EmailErrorCheck()) return null;

		var ajaxObj = GetAjaxObj("sendmail");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			OBJ("send_msg").style.display="block";
			setTimeout('OBJ("send_msg").style.display="none";', 6000);        	
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("service.cgi", "EVENT=SENDMAIL");
	},
	OnClickAuthEnable: function()
	{
		if (OBJ("authenable").checked && OBJ("en_mail").checked)
		{
			OBJ("account_name").setAttribute("modified", "false");
			OBJ("account_name").disabled = false;
			OBJ("passwd").setAttribute("modified", "false");
			OBJ("passwd").disabled = false;
			OBJ("verify_passwd").setAttribute("modified", "false");
			OBJ("verify_passwd").disabled = false;
		}
		else
		{
			OBJ("account_name").setAttribute("modified", "ignore");
			OBJ("account_name").disabled = true;
			OBJ("passwd").setAttribute("modified", "ignore");
			OBJ("passwd").disabled = true;
			OBJ("verify_passwd").setAttribute("modified", "ignore");
			OBJ("verify_passwd").disabled = true;
		}
	},
	OnClickEnable: function()
	{
		if(OBJ("en_mail").checked)
		{
			OBJ("from_addr").disabled			= false;
			OBJ("to_addr").disabled				= false;
			OBJ("email_subject").disabled		= false;
			OBJ("smtp_server_addr").disabled	= false;
			OBJ("smtp_server_port").disabled	= false;
			OBJ("authenable").disabled			= false;
			this.OnClickAuthEnable();
			OBJ("sendmail").disabled			= false;
			/*OBJ("en_logfull").disabled			= false;
			OBJ("en_log_sch").disabled			= false;
			OBJ("log_sch").disabled				= false;*/
		}   
		else
		{
			OBJ("from_addr").disabled			= true;
			OBJ("to_addr").disabled				= true;
			OBJ("email_subject").disabled		= true;
			OBJ("smtp_server_addr").disabled	= true;
			OBJ("smtp_server_port").disabled	= true;
			OBJ("authenable").disabled			= true;
			this.OnClickAuthEnable();
			OBJ("sendmail").disabled			= true;
			/*OBJ("en_logfull").disabled			= true;
			OBJ("en_log_sch").disabled			= true;
			OBJ("log_sch").disabled				= true;*/
		}   
	},
	/*OnClickEnableSchedule: function()
	{
		if(OBJ("en_log_sch").checked && OBJ("en_mail").checked)
			OBJ("log_sch").disabled			= false;
		else
			OBJ("log_sch").disabled			= true;
	},*/
	// this function is used to check if the inputted string is blank or not.
	IsBlank: function(s)
	{
		var i=0;
		for(i=0;i<s.length;i++)
		{
			c=s.charAt(i);
			if((c!=' ')&&(c!='\n')&&(c!='\t'))return false;
		}
		return true;
	},
	IsVaildEmail: function(str)
	{
		for(var i=0; i<str.length; i++)
		{
			if( (str.charAt(i) != '@') )	continue;
			else	return true;
		}
		return false;
	},
	IsIPv4: function(ipv4)
	{
	    var vals = ipv4.split(".");
	    if (vals.length!==4)    return false;
	    for (var i=0; i<4; i++) if (!TEMP_IsDigit(vals[i]) || vals[i]>255)  return false;
	    return true;
	},
	IsVaildPort: function(port)
	{		
		if (!TEMP_IsDigit(port) || parseInt(port, 10)>65535) return false;
		return true;
	},
	/*OnChangeSchedule: function()
	{
		if(OBJ("log_sch").value!="-1")
		{
			var days = "", comm="", ret="";
			var schp = GPBT(this.schp+"/schedule", "entry", "uid", OBJ("log_sch").value, false);
			if(schp!=null)	
			{
				if (XG(schp+"/sun")=="1") { ret=ret+comm+"Sun"; comm=","; }
				if (XG(schp+"/mon")=="1") { ret=ret+comm+"Mon"; comm=","; }
				if (XG(schp+"/tue")=="1") { ret=ret+comm+"Tue"; comm=","; }
				if (XG(schp+"/wed")=="1") { ret=ret+comm+"Wed"; comm=","; }
				if (XG(schp+"/thu")=="1") { ret=ret+comm+"Thu"; comm=","; }
				if (XG(schp+"/fri")=="1") { ret=ret+comm+"Fri"; comm=","; }
				if (XG(schp+"/sat")=="1") { ret=ret+comm+"Sat"; comm=","; }
				ret = ret+" "+XG(schp+"/start")+"~"+XG(schp+"/end");
			}
			OBJ("log_detail").value = ret;
		}
	},*/
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

//]]>
</script>
