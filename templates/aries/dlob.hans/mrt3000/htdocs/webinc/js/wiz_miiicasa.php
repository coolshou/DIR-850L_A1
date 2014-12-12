<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "MIIICASA,MIIICASA.WAN-1,HTTP.LAN-1,HTTP.WAN-1",
	OnLoad: null, /* Things we need to do at the onunload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function(code, result)
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
	/* IsDirty: null,*/
	IsDirty: null,
	
	/* The "services" will be requested by GetCFG(), then the InitValue() will be
	 * called to process the services XML document. */
	InitValue: function(xml)
	{
		PXML.doc = xml;
		//xml.dbgdump();
		this.miiicasa = PXML.FindModule("MIIICASA");

		if (this.miiicasa==="") { alert("InitVaule ERROR!"); return false; }

		if (XG(this.miiicasa+"/miiicasa/enable")==="1")
		{
			OBJ("en_miiicasa").checked = true;
		}
		else
		{
			OBJ("dis_miiicasa").checked = true;
		}
		return true;
	},
	PreSubmit: function()
	{
		XS(this.miiicasa+"/miiicasa/enable", (OBJ("en_miiicasa").checked ? "1":"0"));
		PXML.CheckModule("MIIICASA", null, null, "ignore");
		PXML.CheckModule("MIIICASA.WAN-1", "ignore", "ignore", null);
		PXML.CheckModule("HTTP.LAN-1", "ignore", "ignore", null);
		PXML.CheckModule("HTTP.WAN-1", "ignore", "ignore", null);
		return PXML.doc;
	},
	//////////////////////////////////////////////////
	miiicasa: null,
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
