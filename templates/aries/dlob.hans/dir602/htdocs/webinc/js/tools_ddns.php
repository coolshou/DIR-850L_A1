<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "DDNS4.WAN-1, DDNS4.WAN-3, RUNTIME.DDNS4.WAN-1, RUNTIME.PHYINF",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("DDNS4."+this.devicemode);
		if (p === "") alert("ERROR!");
		OBJ("en_ddns").checked = (XG(p+"/inf/ddns4")!=="");
		var ddnsp = GPBT(p+"/ddns4", "entry", "uid", this.ddns, 0);
		
		if(XG(ddnsp+"/provider")=="") OBJ("server").value = "DLINK"
		else OBJ("server").value	= XG(ddnsp+"/provider");
		OBJ("host").value	= XG(ddnsp+"/hostname");
		OBJ("user").value	= XG(ddnsp+"/username");
		OBJ("passwd").value	= XG(ddnsp+"/password");
		OBJ("report").innerHTML = "";
		
		this.ori_enabled    = (XG(p+"/inf/ddns4")!=="");
		this.ori_server     = XG(ddnsp+"/provider");

		this.OnChangeServer();
		this.OnClickEnDdns();
		this.after_submit_en = false;
	
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("DDNS4."+this.devicemode);
		
		if( !this.Check() ) return null;

		XS(p+"/inf/ddns4", OBJ("en_ddns").checked ? this.ddns : "");
		
		if (OBJ("en_ddns").checked 
			|| (OBJ("server").value!=="")	|| (OBJ("host").value!=="")
			|| (OBJ("user").value!=="")		|| (OBJ("passwd").value!==""))
		{
			var ddnsp = GPBT(p+"/ddns4", "entry", "uid", this.ddns, 0);
			if (!ddnsp)
			{
				var c = XG(p+"/ddns4/count");
				var s = XG(p+"/ddns4/seqno");
				c += 1;
				s += 1;
				XS(p+"/ddns4/entry:"+c+"/uid", this.ddns);
				XS(p+"/ddns4/count", c);
				XS(p+"/ddns4/seqno", s);
				ddnsp = p+"/ddns4/entry:"+c;
			}
			XS(ddnsp+"/provider", OBJ("server").value);
			XS(ddnsp+"/hostname", OBJ("host").value);
			XS(ddnsp+"/username", OBJ("user").value);
			XS(ddnsp+"/password", OBJ("passwd").value);
		}
		if (this.devicemode == "WAN-3")	PXML.IgnoreModule("DDNS4.WAN-1");
		else				PXML.IgnoreModule("DDNS4.WAN-3");
		
		PXML.IgnoreModule("RUNTIME.PHYINF");
		
		if(OBJ("en_ddns").checked) this.after_submit_en   = true;
		
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	devicemode: "<?if (query("/runtime/device/router/mode")=="3G") echo "WAN-3"; else echo "WAN-1";?>",
	ddns: "DDNS4-1",
	ori_enabled: false,
	ori_server: null,
	after_submit_en: false,
	timer_refresh: null,
	
	GrayItems: function(disabled)
	{
		var frmObj = document.forms[0];
		for (var idx = 0; idx < frmObj.elements.length; idx+=1)
		{
			var obj = frmObj.elements[idx];
			var name = obj.tagName.toLowerCase();
			if (name === "input" || name === "select")
			{
				obj.disabled = disabled;
			}
		}
	},
	OnChangeServer: function()
	{
		if(OBJ("server").value == "ORAY")
		{
			OBJ("dsc_dlink").style.display="none";
			OBJ("dsc_DYNDNS").style.display="none";
			OBJ("dsc_dlink_cn").style.display="none";
			OBJ("dsc_oray").style.display="block";
			
			OBJ("host_div").style.display="none";
			OBJ("test_div").style.display="none";
			OBJ("report_div").style.display="none";
			
			OBJ("peanut_status_div").style.display="block";

			var p = PXML.FindModule("RUNTIME.DDNS4."+this.devicemode);
			if (p==="")  { alert("ERROR!"); return;}
			var status = XG(p+"/runtime/inf/ddns4/status");	
			
			if( this.CableLinkage()!=true || this.ori_enabled!=true )
				status = "failed";
			else if  ( this.after_submit_en )	// After enabling oray, immediately shows the status as connecting.
				status = "connecting";
			
			if( status=="successed" )
			{
				OBJ("peanut_status").innerHTML="<?echo i18n("Online");?>" + " / " + "<?echo i18n("Domain Name Registered");?>";
				
				if(XG(p+"/runtime/inf/ddns4/usertype")==="1")
					 OBJ("peanut_level").innerHTML="<?echo i18n("Professional Service");?>";
				else OBJ("peanut_level").innerHTML="<?echo i18n("Normal Service");?>";
				OBJ("peanut_detail_div").style.display="";
			}
			else if( status=="connecting" )
			{
				OBJ("peanut_status").innerHTML="<?echo i18n("Connecting");?>"+"......";
				OBJ("peanut_detail_div").style.display="none";
			}
			else if( status=="badAuth" )
			{
				OBJ("peanut_status").innerHTML="<?echo i18n("Offline");?>" + " / " + "<?echo i18n("Authentication Failed");?>" + "<br>" +
				"<?echo i18n("Please check the account, the password, and the connectivity to the Internet");?>" +".";
				OBJ("peanut_detail_div").style.display="none";
			}
			else //if( status=="failed" )
			{
				OBJ("peanut_status").innerHTML="<?echo i18n("Offline");?>";
				OBJ("peanut_detail_div").style.display="none";
			}
			
			if( this.ori_enabled && this.ori_server=="ORAY" )  // Refresh the page, only when the initial data is as "enable ORAY".
			{
				clearTimeout(this.timer_refresh);
				this.timer_refresh = setTimeout('self.location.href="/tools_ddns.php"' ,20000);
			}
			
		}
		else
		{
			if(OBJ("server").value == "DLINK")
			{
				OBJ("dsc_dlink").style.display="block";
				OBJ("dsc_oray").style.display="none";
				OBJ("dsc_DYNDNS").style.display="none";
				OBJ("dsc_dlink_cn").style.display="none";	
				OBJ("host_div").style.display="block";
			OBJ("test_div").style.display="block";
			OBJ("report_div").style.display="block";	
			
			OBJ("peanut_status_div").style.display="none";
			OBJ("peanut_detail_div").style.display="none";
			}
			
			else if(OBJ("server").value == "DLINK.COM.CN")
			{
				OBJ("dsc_dlink").style.display="none";
				OBJ("dsc_oray").style.display="none";
				OBJ("dsc_DYNDNS").style.display="none";
				OBJ("dsc_dlink_cn").style.display="block";	
				
				OBJ("host_div").style.display="block";
			OBJ("test_div").style.display="block";
			OBJ("report_div").style.display="block";	
			
			OBJ("peanut_status_div").style.display="none";
			OBJ("peanut_detail_div").style.display="none";
			
			}
			else 
			{
				OBJ("dsc_dlink").style.display="none";
				OBJ("dsc_oray").style.display="none";
				OBJ("dsc_DYNDNS").style.display="block";
				OBJ("dsc_dlink_cn").style.display="none";	
			}
			
			OBJ("host_div").style.display="block";
			OBJ("test_div").style.display="block";
			OBJ("report_div").style.display="block";	
			
			OBJ("peanut_status_div").style.display="none";
			OBJ("peanut_detail_div").style.display="none";
			// When switching to servers other than the ORAY, stops refreshing.
			//if(this.timer_refresh) { clearTimeout(this.timer_refresh); this.timer_refresh=null; }
		}
	},
	CableLinkage: function()
	{	
		var p = PXML.FindModule("DDNS4."+this.devicemode);
		var rphy = PXML.FindModule("RUNTIME.PHYINF");

		var wanphyuid = XG  (p+"/inf/phyinf");
		this.rwanphyp = GPBT(rphy+"/runtime", "phyinf", "uid", wanphyuid, false);

		if((XG  (this.rwanphyp+"/linkstatus")!="0")&&(XG  (this.rwanphyp+"/linkstatus")!=""))
			return true;
		else
			return false;
	},
	GetReport: function()
	{
		var self = this;
		var ajaxObj = GetAjaxObj("getreport");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			//xml.dbgdump();
			ajaxObj.release();
			if (xml.Get("/ddns4/valid")==="1" || xml.Get("/ddns4/provider")!== "")  
			{
				var s = xml.Get("/ddns4/status");
				var r = xml.Get("/ddns4/result");
				var msg = "";
				if (s === "IDLE")
				{
					if		(r === "SUCCESS") msg = "<?echo i18n("The update was successful, and the hostname is now updated.");?>";
					else if (r === "NOAUTH")  msg = "<?echo i18n("The username or password specified are incorrect.");?>";
					else if (r === "NOTFQDN") msg = "<?echo i18n("The hostname specified is not a fully-qualified domain name (not in the form hostname.dyndns.org or domain.com).");?>";
					else if (r === "BADHOST") msg = "<?echo i18n("The hostname specified does not exist (or is not in the service specified in the system parameter)");?>";
					else if (r === "SVRERR")  msg = "<?echo i18n("Can't connect to server.");?>";
					else					  msg = "<?echo i18n("Update fail.");?>";

					self.GrayItems(false); 
				}
				else
				{
					if		(s === "CONNECTING")msg = "<?echo i18n("Connecting");?>"	+ "...";
					else if (s === "UPDATING")	msg = "<?echo i18n("Updating");?>"		+ "...";
					else						msg = "<?echo i18n("Waiting");?>"	+ "...";
                    
                    self.ddns_count += 1 ;
					if (self.ddns_count < 10) setTimeout('PAGE.GetReport()', 1000);
					else
					{	
						self.GrayItems(false); 
						msg = "<?echo i18n("Update fail.");?>";
					}
				}
			}
			else
			{
				msg = "<?echo i18n("Update fail.");?>";
				self.GrayItems(false);  
			}
			OBJ("report").innerHTML = msg;
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("ddns_act.php", "act=getreport");
	},
	ddns_count: 0,
	OnClickUpdateNow: function()
	{
		// if(OBJ("host").value == "") return alert("Please input the host name.");
		// if(OBJ("user").value == "") return alert("Please input the user account");
		// if(OBJ("passwd").value == "") return alert("Please input the password");
		
		if( !this.Check() ) return;

		PXML.IgnoreModule("DDNS4.WAN-1");
		PXML.IgnoreModule("DDNS4.WAN-3");
		PXML.ActiveModule("RUNTIME.DDNS4.WAN-1");
		
		var p = PXML.FindModule("RUNTIME.DDNS4.WAN-1");
		XS(p+"/runtime/inf/ddns4/provider", OBJ("server").value);
		XS(p+"/runtime/inf/ddns4/hostname", OBJ("host").value);
		XS(p+"/runtime/inf/ddns4/username", OBJ("user").value);
		XS(p+"/runtime/inf/ddns4/password", OBJ("passwd").value);
		
		var xml = this.PreSubmit();
		PXML.UpdatePostXML(xml);
        COMM_CallHedwig(PXML.doc, function(xml){PXML.hedwig_callback(xml);});
 		
		var self = this;
		self.ddns_count = 0 ;
		
		this.GrayItems(true);   
		OBJ("report").innerHTML = "<?echo i18n("Start updating...");?>";
		
		var ajaxObj = GetAjaxObj("updatenow");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			self.GetReport();
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("ddns_act.php", "act=update");
	},
	OnClickEnDdns: function()
	{
		if (OBJ("en_ddns").checked)
		{
			OBJ("server").disabled = false;
			OBJ("host").disabled = false;
			OBJ("user").disabled = false;
			OBJ("passwd").disabled = false;
			//OBJ("updatenow").disabled = false;
			OBJ("test_div").disabled = false;
			OBJ("DdnsTest").disabled = false;

		}
		else
		{
			OBJ("server").disabled = true;
			OBJ("host").disabled = true;
			OBJ("user").disabled = true;
			OBJ("passwd").disabled = true;	
			//OBJ("updatenow").disabled = true;	
			OBJ("test_div").disabled = true;
			OBJ("DdnsTest").disabled = true;
			
		}
	},
	Check: function()
	{
		if(OBJ("server").value!="ORAY") /* not Oray.net */
		{
			if(is_blank(OBJ("host").value) || __is_str_in_allow_chars(OBJ("host").value, 1, "._-")==false)
			{
				BODY.ShowAlert("<?echo i18n("Invalid host name.");?>");
				return false;
			}
		}
		
		if(is_blank(OBJ("user").value))
		{
			BODY.ShowAlert("<?echo i18n("The User Account is invalid.");?>");
			return false;
		}
		if(OBJ("server").value!="ORAY") /* not Oray.net */
		{
			if(__is_str_in_allow_chars(OBJ("user").value, 1, "._-@")==false)
			{
				BODY.ShowAlert("<?echo i18n("The User Account is invalid.");?>");
				return false;
			}
		}
		else
		{
			if(__is_str_in_allow_chars(OBJ("user").value, 1, "_-")==false)
			{
				BODY.ShowAlert("<?echo i18n("The User Account is invalid.");?>");
				return false;
			}			
		}
		
		if(is_blank(OBJ("passwd").value))
		{
			BODY.ShowAlert("<?echo i18n("The Password is invalid.");?>");
			return false;
		}
		if(strchk_unicode(OBJ("passwd").value)==true)
		{
			BODY.ShowAlert("<?echo i18n("The Password is invalid.");?>");
			return false;
		}
		return true;
	}
};
</script>
<script>
// this function is used to check if the inputted string is blank or not.
function is_blank(s)
{
	var i=0;
	for(i=0;i<s.length;i++)
	{
		c=s.charAt(i);
		if((c!=' ')&&(c!='\n')&&(c!='\t'))return false;
	}
	return true;
}
// if the characters of "char_code" is in following ones: 0~9, A~Z, a~z, some control key and TAB.
function __is_comm_chars(char_code)
{
	if (char_code == 0)  return true;						/* some control key. */
	if (char_code == 8)  return true;						/* TAB */
	if (char_code >= 48 && char_code <= 57)  return true;	/* 0~9 */
	if (char_code >= 65 && char_code <= 90)  return true;	/* A~Z */
	if (char_code >= 97 && char_code <= 122) return true;	/* a~z */
	
	return false;
}
function __is_char_in_string(target, pattern)
{
	var len = pattern.length;
	var i;
	for (i=0; i<len; i++)
	{
		if (target == pattern.charCodeAt(i)) return true;
	}
	return false;
}
//if the characters of "str" are all in the allowed "allow_chars".
function __is_str_in_allow_chars(str, allow_comm_chars, allow_chars)
{
	var char_code;
	var i;

	for (i=0; i<str.length; i++)
	{
		char_code=str.charCodeAt(i);
		if (allow_comm_chars == "1" && __is_comm_chars(char_code) == true) continue;
		if (allow_chars.length > 0 && __is_char_in_string(char_code, allow_chars) == true) continue;
		return false;
	}
	return true;
}
// check if any unicode characters in the string.
function strchk_unicode(str)
{
	var strlen=str.length;
	if(strlen>0)
	{
		var c = '';
		for(var i=0;i<strlen;i++)
		{
			c = escape(str.charAt(i));
			if(c.charAt(0) == '%' && c.charAt(1)=='u')
				return true;
		}
	}
	return false;
}

</script>

