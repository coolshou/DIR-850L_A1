<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "",
	OnLoad: function() {
		BODY.DisableCfgElements(false);
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
	InitValue: null,
	PreSubmit: function() { return null; },
	//////////////////////////////////////////////////
	wcount: 0,
	OnClick_Ping: function(postfix)
	{
		BODY.ClearConfigError();
		this.ResetPing();
		OBJ("ping"+postfix).disabled = true;
		OBJ("dst"+postfix).value= COMM_EatAllSpace(OBJ("dst"+postfix).value);
		if (OBJ("dst"+postfix).value==="")
		{
			BODY.ShowConfigError("dst"+postfix,"<?echo I18N("j","Please enter a host name or IP address for ping.");?>");
			this.ResetPing();
			return false;
		}

		var self = this;
		var ajaxObj = GetAjaxObj("dst"+postfix);
		OBJ("report").innerHTML = "<?echo I18N("j","Pinging...");?>"
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			self.GetPingReport();
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("diagnostic.php", "act=ping&dst="+OBJ("dst"+postfix).value);
	},
	GetPingReport: function()
	{
		if (this.wcount > 5)
		{
			OBJ("report").innerHTML = "<?echo I18N("j","Ping timeout.");?>";
			this.ResetPing();
			return;
		}

		var self = this;
		var ajaxObj = GetAjaxObj("pingreport");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(xml)
		{
			ajaxObj.release();
			if (xml.Get("/diagnostic/report")==="")
			{
				setTimeout('PAGE.GetPingReport()',1000);
				self.wcount += 1;
			}
			else
			{
				OBJ("report").innerHTML = self.WrapRetString( xml.Get("/diagnostic/report") );
				self.ResetPing();
			}
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("diagnostic.php", "act=pingreport");
	},
	ResetPing: function()
	{
		this.wcount = 0;
		OBJ("ping_v4").disabled = false;
		//OBJ("ping_v6").disabled = false;//remove
	},
	StrArray: ["is alive", "No response from"],
	WrapRetString: function(OrgStr)
	{
		var WrappedStr = OrgStr;
		var index;

		for(var i=0; i<this.StrArray.length; i+=1)
		{
			index = OrgStr.indexOf(this.StrArray[i]);
			if(index != -1)
			{
				var BgnStr = OrgStr.substring(0, index);
				var EndStr = OrgStr.substring(index+this.StrArray[i].length, OrgStr.length);
				var MidStr = null;
				switch(i)
				{
				case 0:
					MidStr = "<?echo I18N("j","is alive");?>";
					break;
				case 1:
					MidStr = "<?echo I18N("j","No response from");?>";
				}
				WrappedStr = BgnStr+MidStr+EndStr;
				break;
			}
		}
		return WrappedStr;
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}
//]]>
</script>
