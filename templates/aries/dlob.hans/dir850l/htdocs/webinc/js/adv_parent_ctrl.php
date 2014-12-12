<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "OPENDNS4,INET.LAN-1,REBOOT",
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	OnLoad: function() 
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}	
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) 
	{ 
		switch (code)
		{
			case "OK":
				BODY.ShowContent();
				if(this.reboot===1)
				{
					var msgArray = ['<?echo I18N("j","It would spend a little time, please wait");?>...'];	
					BODY.ShowCountdown('<?echo i18n("Enable DNS Relay");?>...', msgArray, this.bootuptime, "http://<?echo $_SERVER['HTTP_HOST'];?>/adv_parent_ctrl.php");
				}	
				else BODY.OnReload();
				break;
			default:
			return false;
		}
		return true;
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("OPENDNS4");
		this.wan1_infp  = GPBT(p, "inf", "uid", "WAN-1", false);

		//OBJ("adv_dns").checked = OBJ("familyshield").checked = OBJ("parent_ctrl").checked = OBJ("none_opendns").checked = false;
		OBJ("parent_ctrl").checked = OBJ("none_opendns").checked = false;
		//if(XG(this.wan1_infp+"/open_dns/type") === "advance") OBJ("adv_dns").checked = true;
		//else if(XG(this.wan1_infp+"/open_dns/type") === "family") OBJ("familyshield").checked = true;
		/*else*/ if(XG(this.wan1_infp+"/open_dns/type") === "parent") OBJ("parent_ctrl").checked = true;
		else OBJ("none_opendns").checked = true;
		
		return true;
	},
	PreSubmit: function()
	{
		//if(OBJ("adv_dns").checked) XS(this.wan1_infp+"/open_dns/type", "advance");
		//else if(OBJ("familyshield").checked) XS(this.wan1_infp+"/open_dns/type", "family");
		/*else*/ if(OBJ("parent_ctrl").checked) XS(this.wan1_infp+"/open_dns/type", "parent");
		else XS(this.wan1_infp+"/open_dns/type", "");
			
		/*Enable DNS Relay when openDNS is used*/
		var lan	= PXML.FindModule("INET.LAN-1");
		if(OBJ("none_opendns").checked)
		{
			PXML.IgnoreModule("INET.LAN-1");
			PXML.IgnoreModule("REBOOT");
		}
		else if(XG(lan+"/inf/dns4")!=="") //DNS Relay is enable already. 
		{
			PXML.IgnoreModule("INET.LAN-1");
			PXML.IgnoreModule("REBOOT");	
		}
		else
		{					
			XS(lan+"/inf/dns4", "DNS4-1");
			PXML.CheckModule("INET.LAN-1", null, null, "ignore");
			this.reboot=1;
		}	

		return PXML.doc;
	},
	IsDirty: null,
	wan1_infp: null,
	reboot: null,
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="")	$bt=30;
		else			$bt=$bt+10;
		echo $bt;
	?>,	
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	OnChangeOpenDNSType: function(opendnsid)
	{
		//OBJ("adv_dns").checked = OBJ("familyshield").checked = OBJ("parent_ctrl").checked = OBJ("none_opendns").checked = false;
		OBJ("parent_ctrl").checked = OBJ("none_opendns").checked = false;
		OBJ(opendnsid).checked = true;
	}	  
}
</script>
