<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "DEVICE.LOG,MYDLINK.LOG",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.LOG_path = PXML.FindModule("DEVICE.LOG");
		this.LOG_path = this.LOG_path+"/device/log";
		this.MyDlinkp = this.LOG_path+"/mydlink";
		var MyDlinkRegister = <? if(query("/mydlink/register_st")=="1"){ echo "true";} else { echo "false";} ?>;
		if(MyDlinkRegister)
		{
			OBJ("en_dns_query").checked = COMM_ToBOOL(XG(this.MyDlinkp+"/dnsquery"));
			OBJ("en_push_event").checked = COMM_ToBOOL(XG(this.MyDlinkp+"/eventmgnt/pushevent/enable"));
			OBJ("en_notice_userlog").checked = COMM_ToBOOL(XG(this.MyDlinkp+"/eventmgnt/pushevent/types/userlogin"));
			OBJ("en_notice_fwupgrade").checked = COMM_ToBOOL(XG(this.MyDlinkp+"/eventmgnt/pushevent/types/firmwareupgrade"));
			OBJ("en_notice_wireless").checked = COMM_ToBOOL(XG(this.MyDlinkp+"/eventmgnt/pushevent/types/wirelessintrusion"));
		}
		else
		{
			OBJ("en_dns_query").checked = false;
			OBJ("en_push_event").checked = false;
			OBJ("en_dns_query").disabled = true;
			OBJ("en_push_event").disabled = true;					
		}		
		this.PushEvent();
		
		return true;
	},
	PreSubmit: function()
	{
		if(OBJ("en_push_event").checked)
		{
			if(XG(this.LOG_path+"/email/to")=="" || XG(this.LOG_path+"/email/from")=="")
			{
				if(confirm('<? echo I18N("j", "The email settings should be complete first to enable the PUSH EVENT function. Would you want to set the email settings?");?>'))
					self.location.href="tools_email.php";
				else return null;
			}
		}
		
		PXML.CheckModule("DEVICE.LOG", null, null, "ignore"); 
		PXML.CheckModule("MYDLINK.LOG", "ignore", "ignore", null);
		XS((this.MyDlinkp+"/dnsquery"), OBJ("en_dns_query").checked?"1":"0");
		XS((this.MyDlinkp+"/eventmgnt/pushevent/enable"), OBJ("en_push_event").checked?"1":"0");
		XS((this.MyDlinkp+"/eventmgnt/pushevent/types/userlogin"), OBJ("en_notice_userlog").checked?"1":"0");
		XS((this.MyDlinkp+"/eventmgnt/pushevent/types/firmwareupgrade"), OBJ("en_notice_fwupgrade").checked?"1":"0");
		XS((this.MyDlinkp+"/eventmgnt/pushevent/types/wirelessintrusion"), OBJ("en_notice_wireless").checked?"1":"0");		
		
		return PXML.doc;
	},
	IsDirty: null,
	LOG_path: null,
	MyDlinkp: null,
	Synchronize: function() {},
	PushEvent: function() 
	{
		if(OBJ("en_push_event").checked)
		{
			OBJ("en_notice_userlog").disabled = false;
			OBJ("en_notice_fwupgrade").disabled = false;
			OBJ("en_notice_wireless").disabled = false;
		}	
		else
		{
			OBJ("en_notice_userlog").disabled = true;
			OBJ("en_notice_fwupgrade").disabled = true;
			OBJ("en_notice_wireless").disabled = true;			
		}		
	}
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
}

</script>
