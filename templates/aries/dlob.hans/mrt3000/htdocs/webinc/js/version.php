<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		GetLangcode();
		EncodeHex();
		GetQueryUrl();
		Configured();
		return true;
	},
	PreSubmit: function() { return null; },
	IsDirty: null,
	Synchronize: function() {}
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
}

function GetLangcode()
{
	var langcode = "<?echo query("/runtime/device/langcode");?>";
	OBJ("langcode").innerHTML = (langcode=="")? "en":langcode;
}

function toHex( n )
{
	var digitArray = new Array('0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f');
	var result = ''
	var start = true;

	for (var i=32; i>0;)
	{
		i -= 4;
		var digit = ( n >> i ) & 0xf;

		if (!start || digit != 0)
		{
			start = false;
			result += digitArray[digit];
		}
	}

	return ( result == '' ? '0' : result );
}
function pad( str, len, pad )
{
	var result = str;

	for (var i=str.length; i<len; i++)
	{
		result = pad + result;
	}

	return  result;
}
function EncodeHex()
{
	var str = "<?echo cut(fread("", "/etc/config/builddaytime"), "0", "\n");?>";
	var result = "";

	for (var i=0; i<str.length; i++)
	{
		if (str.substring(i,i+1).match(/[^\x00-\xff]/g) != null)
		{
			result += escape(str.substring(i,i+1), 1).replace(/%/g,'\\');
		}
		else
		{
			result += pad(toHex(str.substring(i,i+1).charCodeAt(0)&0xff),2,'0');
		}
	}
	OBJ("checksum").innerHTML = result.substring(result.length-8,result.length);
}

function GetQueryUrl()
{
	var fwsrv = "<?echo query("/runtime/device/fwinfosrv");?>";
	var fwpath= "<?echo query("/runtime/device/fwinfopath");?>";
	var brand = "<?echo query("/runtime/device/vendor");?>";
	var model = "<?echo query("/runtime/device/modelname");?>";	
	var major = '<?echo cut(query("/runtime/device/firmwareversion"),0,".");?>';	
	var minor = '<?echo cut(query("/runtime/device/firmwareversion"),1,".");?>';		

	OBJ("fwq").innerHTML = "http:\/\/"+fwsrv+fwpath+"?brand="+brand+"&model="+model+"&major="+major+"&minor="+minor;
}

function Configured()
{
	OBJ("configured").innerHTML = "<?

	$wiz_freset = query("/device/wiz_freset");
	if		($wiz_freset == 1)	echo I18N("h","0");
	else if ($wiz_freset == 0)		echo I18N("h","1");
	else					echo I18N("h","N/A");

	?>";
}
</script>
