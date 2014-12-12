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
	services: "ACCESSCTRL",
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
		this.OnClickClearURL();
		PXML.doc = xml;
		var p = PXML.FindModule("ACCESSCTRL");
		if (p === "") alert("ERROR!");
		p += "/acl/accessctrl/webfilter";

		var count = XG(p+"/count");
		for (var i=1; i<=count; i+=1)
		{
			var b = p+"/entry:"+i;
			OBJ("url_"+i).value = XG(b+"/url");
		}
		var policy = XG(p+"/policy");
		if(policy !== "")	OBJ("url_mode").value = policy;
		else 			OBJ("url_mode").value = "DROP";

		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("ACCESSCTRL");
		p += "/acl/accessctrl/webfilter";
		var old_count = XG(p+"/count");
		var cur_count = 0;
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p+"/entry:"+old_count);
			old_count -= 1;
		}
		/* update the entries */
		for (var i=1; i<=<?=$URL_MAX_COUNT?>; i+=1)
		{
			/* if the url field is empty, it means to remove this entry, so skip this entry. */
			if (OBJ("url_"+i).value!=="")
			{
				cur_count+=1;
				var b = p+"/entry:"+cur_count;
				XS(b+"/uid",		"URLF-"+i);
				XS(b+"/url",		OBJ("url_"+i).value);
			}
		}
		XS(p+"/count", cur_count);
		XS(p+"/policy",	OBJ("url_mode").value);

		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	OnClickClearURL: function()
	{
		for (var i=1; i<=<?=$URL_MAX_COUNT?>; i+=1)	OBJ("url_"+i).value="";
	},	
	CursorFocus: function(node)
	{
		if (node.match("url")) 
		{
			
			var i = node.lastIndexOf("entry:");
			if(node.charAt(i+7)==="/") var idx = parseInt(node.charAt(i+6), 10);
			else var idx = parseInt(node.charAt(i+6), 10)*10 + parseInt(node.charAt(i+7), 10);			
			var indx = 1;
			var valid_url_cnt = 0;		
			for(indx=1; indx <= <?=$URL_MAX_COUNT?>; indx++)
			{
				if(OBJ("url_"+indx).value!=="") valid_url_cnt++;
				if(valid_url_cnt===idx) break;
			}	
			OBJ("url_"+indx).focus();
		}
	}
}
</script>
