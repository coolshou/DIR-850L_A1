<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "",
	OnLoad: function()
	{
		BODY.CleanTable("routing_list");
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) { return false; },

	InitValue: function(xml)
	{
		PXML.doc = xml;
		var self = this;
		this.CallAjax("v6");
		return true;
	},
	CallAjax: function(version)
	{
		var ajaxObj = GetAjaxObj("report");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function(text)
		{
			ajaxObj.release();
			PAGE.FillRouteTable(text);
		}
		ajaxObj.returnXml = false;
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("routing_stat.php", "version="+version);
	},
	PreSubmit: function()
	{
		return null;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	wcount: 0,

	ParseRouteV6: function(route_str)
	{
		if (!route_str)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var r1_str = route_str.split("\n");
		for (var i=0; i< r1_str.length-1; i++)
		{
			if(r1_str[i].substring(0,2)=="==" || r1_str[i].substring(0,4)=="fe80")
			{
				continue;
			}
			var r2_str = r1_str[i].split(" ");
			var data_ary = new Array();
			for(var j=0;j< r2_str.length;j++)
			{
				if(r2_str[j]!="" )		data_ary.push(r2_str[j]);
			}
			if(!(data_ary[1]=="dev" || data_ary[1]=="via"))
				continue;
			var dest = data_ary[0];
			if(dest == "default")
			{
				if(data_ary[1]=="via")
					var gw = data_ary[2];
				dest = "::/0";
			}
			else
				var gw = "::";
				
			for (var z=0; z< data_ary.length-1; z++)
			{
				if(data_ary[z]=="metric")
					var metric = data_ary[z+1];
				if(data_ary[z]=="dev")
					var iface = change_inter(data_ary[z+1]);
			}
			var data	= [dest,gw,metric,iface];
			var type	= ["text","text","text","text"];
			BODY.InjectTable("routing_list", null, data, type);
		}
	},
	ParseStatic: function(route_str)
	{
		if (!route_str)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var r1_str = route_str.split("\n");
		for (var i=0; i< r1_str.length-1; i++)
		{
			if(r1_str[i].substring(0,2)=="==" || r1_str[i].substring(0,2)=="")
			{
				continue;
			}
			var r2_str = r1_str[i].split(" ");
			var data_ary = new Array();
			for(var j=0;j<=r2_str.length;j++)
			{
				if(r2_str[j]!="")		data_ary.push(r2_str[j]);
			}
			var dest = data_ary[0];
			var gw = data_ary[2];
			var metric = data_ary[6];
			var iface = change_inter(data_ary[4]);
			var data	= [dest,gw,metric,iface];
			var type	= ["text","text","text","text"];
			BODY.InjectTable("routing_list", null, data, type);
		}
	},

	FillRouteTable: function(route_str)
	{
		if (!route_str)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var  str1 = route_str.slice(route_str.lastIndexOf("==ROUTEV6==") ,route_str.indexOf("==END1=="));
		this.ParseRouteV6(str1);
		var  str2 = route_str.slice(route_str.lastIndexOf("==STATIC==") ,route_str.indexOf("==END2=="));
		this.ParseStatic(str2);
	}
}

function change_inter(obj_value)
{
  var return_word= obj_value;
  if(obj_value == "br0"||obj_value == "br1")
  {
	 return_word= "LAN";
   }
  else if(obj_value="eth"||obj_value == "WAN")
  {
	 return_word= "INTERNET";
	}
  else if(obj_value=="ra0"||obj_value == "ra1")
	{
	  return_word= "WIRELESS";
	 }
	return return_word;
}
</script>


