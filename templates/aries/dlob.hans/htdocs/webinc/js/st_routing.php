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
    		this.CallAjax("v4");
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
	
	ParseRouteN: function(route_str) 
	{
		if (!route_str)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var r1_str = route_str.split("\n");
		for (var i=1; i<=r1_str.length; i++)
		{
				if(r1_str[i].indexOf("Kernel IP routing table")==0)
				{
					i++;	
					continue;
				}
				if(r1_str[i].substring(0,2)=="==" || r1_str[i].substring(0,2)=="")
				{
					i++;
					continue;
				}	
				var r2_str = r1_str[i].split(" ");
				var data_ary = new Array();
				for(var j=0;j<=r2_str.length;j++)
				{
					if(r2_str[j]!="")		data_ary.push(r2_str[j]);
				}
				var dest = data_ary[0];
				var gw = data_ary[1];
				var gm = data_ary[2];
				var metric = data_ary[4];
				var iface = change_inter(data_ary[7]);
				var creator =change_creator( r1_str[i].substring(70,71));
				var data	= [dest,gw,gm,metric,iface,creator];
				var type	= ["text","text","text","text","text","text"];
				BODY.InjectTable("routing_list", null, data, type);	
		}
    },
    
    
    ParseDefault: function(route_str) 
	{
		if (!route_str)
		{
			BODY.ShowAlert("Initial() ERROR!!!");
			return false;
		}
		var r1_str = route_str.split("\n");
		for (var i=1; i<=r1_str.length; i++)
		{

				if(r1_str[i].substring(0,2)=="==" || r1_str[i].substring(0,2)=="")
				{
					i++;
					continue;
				}
				var r2_str = r1_str[i].split(" ");
				var data_ary = new Array();
				for(var j=0;j<=r2_str.length;j++)
				{
					if(r2_str[j]!="")		data_ary.push(r2_str[j]);
				}
				var dest = "0.0.0.0";
				var gw = data_ary[2];
				var gm = "	255.255.255.255";
				var metric = data_ary[6];
				var iface = change_inter(data_ary[4]);
				var creator =change_creator( r1_str[i].substring(70,71));
				var data	= [dest,gw,gm,metric,iface,creator];
				var type	= ["text","text","text","text","text","text"];
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
		for (var i=1; i<=r1_str.length; i++)
		{
				if(r1_str[i].substring(0,2)=="==" || r1_str[i].substring(0,2)=="")
				{
					i++;
					continue;
				}
				var r2_str = r1_str[i].split(" ");
				var data_ary = new Array();
				for(var j=0;j<=r2_str.length;j++)
				{
					if(r2_str[j]!="")		data_ary.push(r2_str[j]);
				}
				var ip_ary =data_ary[0].split("/");
				var dest = ip_ary[0];
				var gw = data_ary[2];
				var gm = COMM_IPv4INT2MASK(ip_ary[1]);
				var metric = data_ary[6];
				var iface = change_inter(data_ary[4]);
				var creator =change_creator( r1_str[i].substring(70,71));
				var data	= [dest,gw,gm,metric,iface,creator];
				var type	= ["text","text","text","text","text","text"];
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
		var  str1 = route_str.slice(route_str.lastIndexOf("==ROUTEN==") ,route_str.indexOf("==END1=="));
		this.ParseRouteN(str1);
		var  str2 = route_str.slice(route_str.lastIndexOf("==DEFAULT==") ,route_str.indexOf("==END2=="));
		this.ParseDefault(str2);
		var  str3 = route_str.slice(route_str.lastIndexOf("==STATIC==") ,route_str.indexOf("==END3=="));
		this.ParseStatic(str3);
    }
}

function change_inter(obj_value)
	{
		var return_word= obj_value;
		if((obj_value == "br0")||(obj_value == "br1"))
		{
			return_word= "LAN";
		}
		else if((obj_value="eth")||(obj_value == "WAN"))
		{
			return_word= "INTERNET";
		}
		return return_word;
	}
function change_creator(obj_value)
	{
		var return_word = obj_value;
		if(obj_value = "0")
		{
			return_word = "SYSTEM";
		}
		else if(obj_value == "1")
		{
			return_word = "ADMIN";
		}
		return return_word;
	}
</script>
