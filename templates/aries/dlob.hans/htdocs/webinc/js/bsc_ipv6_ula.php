<?include "/htdocs/phplib/phyinf.php";?>
<script type="text/javascript">

function Page() {}
Page.prototype =
{
	services: "INET.LAN-6,RUNTIME.INF.LAN-6",
	OnLoad: function()
	{
		if (!this.rgmode)		{BODY.DisableCfgElements(true);}
	},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},

	InitValue: function(xml)
	{
		PXML.doc = xml;

		//PXML.doc.dbgdump();

		this.ParseAll();
		this.InitUlaVal();
		this.OnClickEnableUla();
		
		return true;
	},

	PreSubmit: function()
	{
		if (!this.PreLAN()) return null;
	
		PXML.CheckModule("INET.LAN-6", null, null, null);
		PXML.IgnoreModule("RUNTIME.INF.LAN-6");

		//PXML.doc.dbgdump();
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},

	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////

	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
        lanulact:     { infp: null, inetp: null},
        rlanul :  null,

	OnClickEnableUla: function()
	{
		OBJ("bbox_ula").style.display = "";
		OBJ("box_ula_title").style.display = "";
		OBJ("box_ula_body").style.display = "";
		OBJ("ula_span").style.display = "";
		OBJ("box_cula_title").style.display = "";
		OBJ("box_cula_prefix_body").style.display = "";
		OBJ("box_cula_addr_body").style.display = "";
		
		if(OBJ("en_ula").checked)
		{
			OBJ("use_default_ula").disabled = false;
		}
		else
		{
			OBJ("use_default_ula").disabled = true;
			OBJ("ula_prefix").disabled = true;
		}
	},
	
	OnClickUseDefault: function()
	{
		if(OBJ("use_default_ula").checked)
		{
			OBJ("ula_prefix").disabled = true;
		}
		else
		{
			OBJ("ula_prefix").disabled = false;
		}
	},

	/* Get lanact, lanllact, wanact and wanllact */
	ParseAll: function()
	{
                var base = PXML.FindModule("INET.LAN-6");
                this.lanulact.infp = GPBT(base, "inf", "uid", "LAN-6", false);
                this.lanulact.inetp = GPBT(base+"/inet", "entry", "uid", XG(this.lanulact.infp+"/inet"), false);
		this.rlanul = PXML.FindModule("RUNTIME.INF.LAN-6");
	},

	InitUlaVal: function()
	{
		var active = XG(this.lanulact.infp+"/active");
		if(active=="1")	OBJ("en_ula").checked = true;
		else		OBJ("en_ula").checked = false;
		var ipaddr = XG(this.lanulact.inetp+"/ipv6/ipaddr");
		var isStatic = XG(this.lanulact.inetp+"/ipv6/staticula");
		//if(ipaddr!="")
		if(isStatic=="1")
		{	
			OBJ("use_default_ula").checked	= false;
			OBJ("ula_prefix").disabled	= false;
			OBJ("ula_prefix").value		= ipaddr;
		}
		else
		{		
			OBJ("use_default_ula").checked = true;
			OBJ("ula_prefix").disabled = true;
		}
		
		/* fill some fixed info */
		OBJ("ula_prefix_pl").innerHTML	= "/64";
		OBJ("cula_prefix").innerHTML	= XG(this.rlanul+"/runtime/inf/inet/ipv6/network");
		OBJ("cula_prefix_pl").innerHTML	= "/64";
		OBJ("cula_addr").innerHTML	= XG(this.rlanul+"/runtime/inf/inet/ipv6/ipaddr");
		OBJ("cula_addr_pl").innerHTML	= "/64";

		return true;
	},

	PreLAN: function()
	{
		XS(this.lanulact.inetp+"/ipv6/mode", "UL");
		if(OBJ("en_ula").checked)
		{
			XS(this.lanulact.infp+"/active", "1");
		}
		else
		{
			XS(this.lanulact.infp+"/active", "0");
			return true;
		}

		if(OBJ("use_default_ula").checked)
		{
			XS(this.lanulact.inetp+"/ipv6/ipaddr", "");
			XS(this.lanulact.inetp+"/ipv6/prefix", "");
			XS(this.lanulact.inetp+"/ipv6/staticula", "0");
		}
		else
		{
			XS(this.lanulact.inetp+"/ipv6/ipaddr", OBJ("ula_prefix").value);
			XS(this.lanulact.inetp+"/ipv6/prefix", "64");
			XS(this.lanulact.inetp+"/ipv6/staticula", "1");
		}
		return true;
	},

	OnClickUsell: function()
	{
		OBJ("w_st_ipaddr").disabled = OBJ("w_st_pl").disabled = OBJ("usell").checked ? true: false;
		if(OBJ("usell").checked)
		{
			var r3ipaddr = XG(this.rwan3+"/runtime/inf/inet/ipv6/ipaddr");
			if(r3ipaddr=="")	r3ipaddr = this.wanllact.ipv6ll;
			OBJ("w_st_ipaddr").value	= r3ipaddr;
			OBJ("w_st_pl").value		= 64;
		}
		else
		{
			OBJ("w_st_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/ipaddr");
			OBJ("w_st_pl").value		= XG(this.wanact.inetp+"/ipv6/prefix");
		}
	},

    	MyArrayShowAlert: function( promptInfo, target )
	{
		alert( promptInfo	+ ": " +
			target.svcsname	+ "  " +
			target.svcs		+ "  " +
			target.inetuid	+ "  " +
             			target.inetp	+ "  " +
			target.phyinf	+ "  " +
			target.ipv6ll);	
	}
}

function GetRadioValue(name)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].checked)	return obj[i].value;
	}
}

function SetRadioValue(name, value)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].value==value)
		{
			obj[i].checked = true;
			break;
		}
	}
}

function Dec2Hex(DecVal)
{
	var HexChars = "0123456789ABCDEF";
	DevVal = parseInt(DecVal);
	if(DecVal > 255 || DecVal < 0) DecVal = 255;
	var Dig1 = DecVal%16;
	var Dig2 = (DecVal-Dig1)/16;
	var HexVal = HexChars.charAt(Dig2)+HexChars.charAt(Dig1);
	return HexVal;
} 

function CutFirstSpace(AddrStr)
{
	var i=0;
	while(i<AddrStr.length)
	{
		if(i==0)
		{
			i++;
			continue;
		}
		if( AddrStr.charAt(i-1)==":" && AddrStr.charAt(i)=="0")
		{
			AddrStr = AddrStr.substring(0,i)+AddrStr.substring(i+1,AddrStr.length);
			i=0;
			continue;
		}
		i++;
	}
	return AddrStr;
}
</script>
