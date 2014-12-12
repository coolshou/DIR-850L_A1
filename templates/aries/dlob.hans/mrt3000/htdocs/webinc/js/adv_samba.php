<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "SAMBA",
/*	
	OnLoad: function()
	{
		if (!this.rgmode) BODY.DisableCfgElements(true);
	},
*/	
	OnLoad: null, /* Things we need to do at the onunload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function (code, result)
	{
		switch (code)
		{
		case "OK":
			self.location.href = "./share.php";
			break;
		default:
			BODY.ShowMessage("<?echo I18N("j","Error");?>",result);
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
		//xml.dbgdump();

		this.samba = PXML.FindModule("SAMBA");
		if (this.samba==="") { alert("InitValue ERROR!"); return false; }
		var enable = XG(this.samba+"/device/samba/enable");
		if (XG(this.samba+"/device/samba/enable")==="1")
			OBJ("en_samba").checked = true;
		else
			OBJ("dis_samba").checked = true;

		return true;
	},
	PreSubmit: function()
	{
		XS(this.samba+"/device/samba/enable", (OBJ("en_samba").checked?"1":"0"));
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	samba: null,
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
