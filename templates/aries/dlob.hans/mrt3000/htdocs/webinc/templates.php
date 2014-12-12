<?
include "/htdocs/phplib/langpack.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/feature.php";
include "/htdocs/webinc/libdraw.php";
include "/htdocs/webinc/draw_elements.php";
?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
	<link rel="shortcut icon" href="/favicon.ico" >	
	<meta http-equiv="Content-Type" content="no-cache" />
	<meta http-equiv="Pragma" content="no-cache" />
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE8" />
	<title><?echo $TEMP_TITLE;?></title>

	<link type="text/css" rel="stylesheet" href="css/base.css" />
<?
	if (isfile("/htdocs/web/css/".$TEMP_MYNAME.".css")==1)
	echo '\t<link rel="stylesheet" href="./css/'.$TEMP_MYNAME.'.css" type="text/css" />';
?>	<!-- miiiCasa js -->
	<script type="text/javascript" charset="utf-8" src="./js/roundcorner/jquery-1.4.2.min.js"></script>
	<script type="text/javascript" charset="utf-8" src="./js/popup/jquery.lightbox_me.js"></script>
	<script type="text/javascript" charset="utf-8" src="./js/popup/popup_form.js"></script>
	<!-- seattle js -->
	<script type="text/javascript" charset="utf-8" src="./js/comm.js"></script>
	<script type="text/javascript" charset="utf-8" src="./js/libajax.js"></script>
	<script type="text/javascript" charset="utf-8" src="./js/postxml.js"></script>
<?
	$file = "/htdocs/webinc/js/".$TEMP_MYNAME.".php";
	if (isfile($file)==1) { $HAVE_CFGJAVA=1; dophp("load", $file); }
?>	<script type="text/javascript">
//<![CDATA[
var OBJ = COMM_GetObj;
var XG = function(n) {return PXML.doc.Get(n);};
var XS = function(n,v) {return PXML.doc.Set(n,v);};
var XD = function(n) {return PXML.doc.Del(n);};
var XA = function(n,v) {return PXML.doc.Add(n,v);};
var GPBT = function(r,e,t,v,c) {return PXML.doc.GetPathByTarget(r,e,t,v,c);};
var S2I = function(str) {var num=parseInt(str,10); return isNaN(num)?0:num;};

function TEMP_IsDigit(str)
{
	var y = parseInt(str);
	if (isNaN(y)) return false;
	return str===y.toString();
}
function TEMP_InjectTable(tblID, uid, data, type)
{
	var rows = OBJ(tblID).getElementsByTagName("tr");
	var tagTR = null;
	var tagTD = null;
	var i;
	var str;
	var found = false;

	/* Search the rule by UID. */
	for (i=0; !found && i<rows.length; i++) if (rows[i].id == uid) found = true;
	if (found)
	{
		for (i=0; i<data.length; i++)
		{
			tagTD = OBJ(uid+"_"+i);
			switch (type[i])
			{
			case "checkbox":
					str = "<input type='checkbox'";
				str += " id="+uid+"_check_"+i;
				if (COMM_ToBOOL(data[i])) str += " checked";
				str += " disabled>";
				tagTD.innerHTML = str;
				break;
			case "text":
					str = data[i];
				if(typeof(tagTD.innerText) !== "undefined")	tagTD.innerText = str;
				else if(typeof(tagTD.textContent) !== "undefined")	tagTD.textContent = str;
				else	tagTD.innerHTML = str;
				break;	
			default:
				str = data[i];
				tagTD.innerHTML = str;
				break;
			}
		}
		return;
	}

	/* Add a new row for this entry */
	tagTR = OBJ(tblID).insertRow(rows.length);
	tagTR.className = (rows.length%2)?"light_bg":"gray_bg";
	tagTR.id = uid;
	/* save the rule in the table */
	for (i=0; i<data.length; i++)
	{
		tagTD = tagTR.insertCell(i);
		tagTD.id = uid+"_"+i;
		tagTD.className = (i==0)?"gray_border_btm":"border_left_t gray_border_btm";
		switch (type[i])
		{
		case "checkbox":
			str = "<input type='checkbox'";
			str += " id="+uid+"_check_"+i;
			if (COMM_ToBOOL(data[i])) str += " checked";
			str += " disabled>";
			tagTD.innerHTML = str;
			break;
		case "text":
			str = data[i];
			if(typeof(tagTD.innerText) !== "undefined")	tagTD.innerText = str;
			else if(typeof(tagTD.textContent) !== "undefined")	tagTD.textContent = str;
			else	tagTD.innerHTML = str;
			break;
		default:
			str = data[i];
			tagTD.innerHTML = str; 
			break;
		}
	}
}
function TEMP_CleanTable(tblID)
{
	table = OBJ(tblID);
	var rows = table.getElementsByTagName("tr");
	while (rows.length > 1) table.deleteRow(rows.length - 1);
}
function TEMP_CheckNetworkAddr(ipaddr, lanip, lanmask)
{
	if (lanip)
	{
		var network = lanip;
		var mask = lanmask;
	}
	else
	{
		var network = "<?$inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0); echo query($inf."/inet/ipv4/ipaddr");?>";
		var mask = "<?echo query($inf."/inet/ipv4/mask");?>";
	}
	var vals = ipaddr.split(".");

	if (vals.length!=4)
		return false;

	for (var i=0; i<4; i++)
		if (!TEMP_IsDigit(vals[i]) || vals[i]>255)  return false;

	if (COMM_IPv4NETWORK(ipaddr, mask)!=COMM_IPv4NETWORK(network, mask))
		return false;

	return true;
}
function TEMP_SetFieldsByDelimit(name, value, delimit)
{
	if (value==""||value==null)
		for (var i=0; i<20; i++) value += ""+delimit;
	
	var data = value.split(delimit);
	var objs = document.getElementsByName(name);
	for (var i=0; i<objs.length; i++)
		objs[i].value = data[i];
}
function TEMP_GetFieldsValue(name, delimit)
{
	var objs = document.getElementsByName(name);
	if (!objs) return "";

	var value = "";
	for (var i=0; i<(objs.length-1); i++)
		value += objs[i].value + delimit;
	value += objs[i].value;

	value = COMM_EatAllSpace(value);
	if(value==":::::")
		return "";
	else
		return value;
}
function TEMP_CheckFieldsEmpty(name)
{
	var objs = document.getElementsByName(name);
	for (var i=0; i<(objs.length); i++)
		if (objs[i].value!=="") return false;

	return true;
}
function TEMP_RulesCount(path, id)
{
	var max = S2I(XG(path+"/max"), 10);
	var cnt = S2I(XG(path+"/count"), 10);
	var rmd = max - cnt;
	OBJ(id).innerHTML = rmd;
}

function Body() {}
Body.prototype =
{
	ShowLogin: function()
	{
		if (this.enCaptcha) this.RefreshCaptcha();
		OBJ("loginpwd").value	= "";
		OBJ("logincap").value	= "";

		if (this.enCaptcha)	OBJ("captcha").style.display = "";
		else				OBJ("captcha").style.display = "none";
		if(OBJ("content"))  OBJ("content").style.display= "none";
		OBJ("all_content").style.display= "none";
		OBJ("login").style.display	= "block";
		
		if(OBJ("icon_adv")) OBJ("icon_adv").style.display = "none"; //hide in login page
		if(OBJ("icon_netmap")) OBJ("icon_netmap").style.display = "none";
		if(OBJ("icon_home")) OBJ("icon_home").style.display = "none";
		if (OBJ("loginusr")&&OBJ("loginusr").tagName.toLowerCase()=="input")
		{
			OBJ("loginusr").value = "";
			OBJ("loginpwd").value = "";
			OBJ("loginusr").focus();
		}
		else
			OBJ("loginpwd").focus();
	},
	ShowContent: function()
	{
		OBJ("login").style.display	= "none";
		OBJ("loginfo").style.display = "none";
		if(OBJ("content")) OBJ("content").style.display= "block";
		OBJ("all_content").style.display= "block";
		this.CloseDialog();
	},
	ShowMessage: function(banner, message)
	{
		OBJ("msg_banner").innerHTML = banner;
		OBJ("msg_content").innerHTML = message;
		function launch() {
			$('#popup_msg').lightbox_me({centered: true});
		}

		$("#loader").lightbox_me({centered: true});
		setTimeout(launch, 50);
	},
	ShowInProgress: function() {<?if ($NOINPROGRESS!=1) echo "$('#main-loading').show();$('#mainform').hide();";?>},
	CloseDialog: function() {$('#main-loading').hide();$('#mainform').show();},
	ShowConfigError: function(name, msg)
	{
		/* because of IE compatiable, some tag's id has the same name, so I check tag name at first. */
		if (document.getElementsByName(name).length)
		{
			objs = document.getElementsByName(name);
			this.BackupStyle = objs[0].className;
			for (var i=0; i<objs.length; i++)
				objs[i].className += " error";
			loc = objs[objs.length-1].nextSibling;
		}
		else
		{
			this.BackupStyle = OBJ(name).className;
			OBJ(name).className += " error";
			loc = OBJ(name).nextSibling;
		}

		while (loc.tagName!="LABEL") loc = loc.nextSibling;
		if (!loc) return false;
		loc.innerHTML = msg;
		this.NameOfErrorMsg = name;
		this.LabelOfErrorMsg = loc;
	},
	ClearConfigError: function()
	{
		if (!this.NameOfErrorMsg) return true;

		/* because of IE compatiable, some tag's id has the same name, so I check tag name at first. */
		if (document.getElementsByName(this.NameOfErrorMsg).length)
		{
			objs = document.getElementsByName(this.NameOfErrorMsg);
			for (var i=0; i<objs.length; i++)
				objs[i].className = this.BackupStyle;
		}
		else
			OBJ(this.NameOfErrorMsg).className = this.BackupStyle;
		this.LabelOfErrorMsg.innerHTML = "";
		this.NameOfErrorMsg = null;
		this.LabelOfErrorMsg = null;
	},
	NameOfErrorMsg: null,
	LabelOfErrorMsg: null,
	BackupStyle: null,
	OnClickSetLang: function(lang)
	{
		var ajaxObj = GetAjaxObj("Lang");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function ()
		{
			ajaxObj.release();
			location.reload(true);
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("lan.php", "multilanguage="+lang);
	},
	//////////////////////////////////////////////////
	rtnURL: null,
	seconds: null,
	timerId: null,
	Countdown: function()
	{
		this.seconds--;
		OBJ("timer").innerHTML = this.seconds;
		if (this.seconds < 1) this.GotResult();
		else this.timerId = setTimeout('BODY.Countdown()',1000);
	},
	GotResult: function()
	{
		clearTimeout(this.timerId);
		if (this.rtnURL)	self.location.href = this.rtnURL;
		else				this.ShowContent();
	},
	ShowCountdown: function(banner, msgArray, sec, url)
	{
		this.rtnURL = url;
		this.seconds = sec;
		var str = "";
		for (var i=0; i<msgArray.length; i++)
			str += '<div>'+msgArray[i]+'</div>';
		str += '<div><?echo I18N("j","Waiting time");?> : ';
		str += '<span id="timer" style="color:red;"></span>';
		str += '&nbsp; <?echo I18N("j","second(s)");?></div>';
		this.CloseDialog();
		this.ShowMessage(banner, str);
		this.Countdown();
	},
	//////////////////////////////////////////////////
	enCaptcha: COMM_ToBOOL(<?echo query("/device/session/captcha");?>),
	RefreshCaptcha: function()
	{
		var self = this;
		var img = OBJ("auth_img");
		var obj = OBJ("captcha_msg");
		img.style.display = "none";
		obj.innerHTML = "<font color=red>(<?echo I18N("j","Wait a moment");?> ...)</font>";
		AUTH.Captcha(function(xml)
		{
			switch (xml.Get("/captcha/result"))
			{
			case "OK":
				self.captcha = xml.Get("/captcha/message");
				img.src = self.captcha+"?"+COMM_RandomStr(6);
				obj.style.display = "none";
				img.style.display = "block";
				break;
			case "FAIL":
				obj.innerHTML = "<?echo I18N("j","No room for new session, please try again later !");?>";
				break;
			default:
				obj.innerHTML = "<?echo I18N("j","Internel error");?> ("+xml.Get("/captcha/result")+")";
				break;
			}
			COMM_GetObj("logincap").value = "";
			COMM_GetObj("logincap").focus();
		});
	},
	//////////////////////////////////////////////////
	LoginCallback: null,
	LoginSubmit: function()
	{
		var self = this;
		OBJ("loginpwd_err").style.display = "none";

		user = "admin";
/* miiicasa has no username field
		if (OBJ("loginusr").value=="")
		{
			this.ShowMessage("<?echo I18N("j","Error");?>", "<?echo I18N("j","Please input the User Name.");?>");
			OBJ("loginusr").focus();
			return false;
		}
*/
// miiiCasa allows blank password
/*		else if (OBJ("loginpwd").value=="")
		{
			OBJ("loginpwd_err").style.display = "inline";
			OBJ("loginpwd").className = "error";
			OBJ("loginpwd").focus();
			return false;
		}
		else if
*/
		if (this.enCaptcha&&OBJ("logincap").value=="")
		{
			OBJ("logincap").className = "error";
			OBJ("logincap").focus();
			return false;
		}
		AUTH.Login(
			function(xml)
			{
				switch (xml.Get("/report/RESULT"))
				{
				case "SUCCESS":
					if(OBJ("icon_adv")) OBJ("icon_adv").style.display = "block"; //hide in login page
					if(OBJ("icon_netmap")) OBJ("icon_netmap").style.display = "block";
					if(OBJ("icon_home")) OBJ("icon_home").style.display = "block";
					if (self.LoginCallback) self.LoginCallback();
					BODY.OnReload();
					self.ShowContent();
					break;
				case "FAIL":
				case "INVALIDUSER":
				case "INVALIDPASSWD":
					OBJ("loginfo").innerHTML = "<?echo I18N("j","User Name or Password is incorrect.");?>";
					OBJ("loginfo").style.display = "";
					break;
				case "INVALIDCAPTCHA":
					OBJ("loginfo").innerHTML = "<?echo I18N("j","The graphical verification code is incorrect.");?>";
					OBJ("loginfo").style.display = "";
					break;
				case "SESSFULL":
					OBJ("loginfo").innerHTML = "<?echo I18N("j","Too many active sessions, please try again later.");?>";
					OBJ("loginfo").style.display = "";
					break;
				case "BAD REQUEST":
					self.ShowMessage("Internal error", "BAD REQUEST.");
					break;
				}
			},
			user,
//			OBJ("loginusr").value,	/* miiiCasa has no username field */
			OBJ("loginpwd").value,
			OBJ("logincap").value.toUpperCase()
		);
	},
	Login: function(callback)
	{
		if (AUTH.AuthorizedGroup >= 0) { AUTH.UpdateTimeout(); return true; }
		//this.ShowLogin();//Sammy
		this.LoginCallback = callback;
		return false;
	},
	Logout: function()
	{
		var self = this;
		AUTH.Logout(function(){
			self.ShowMessage("<?echo I18N("j","Information");?>", "<?echo I18N("j","You have logged out.");?>");
		});
	},
	NoPower: function()
	{
		this.ShowMessage("<?echo I18N("j","Warning");?>",
			"<?echo I18N("j","Your connection session is invalid, please re-login.");?>");
		AUTH.Logout();
		BODY.ShowLogin();
	},
	//////////////////////////////////////////////////
	GetCFG: function()
	{
		var self = this;
		if (!this.Login(function(){self.GetCFG();})) return;

		if (AUTH.AuthorizedGroup >= 100) this.DisableCfgElements(true);
		if (PAGE && PAGE.services!=null)
		{
			//this.ShowInProgress();
			COMM_GetCFG(
				false,
				PAGE.services,
				function(xml) {
					if (PAGE.InitValue)		PAGE.InitValue(xml);
					if (PAGE.Synchronize)	PAGE.Synchronize();
					COMM_DirtyCheckSetup();
					if (AUTH.AuthorizedGroup >= 100)
						BODY.DisableCfgElements(true);
					self.CloseDialog();
				}
			);
		}
		return;
	},
	OnSubmit: function()
	{
		if (!PAGE || !PAGE.services || PAGE.services==="") return;
		if (PAGE.Synchronize) PAGE.Synchronize();
		var dirty = COMM_IsDirty(false);
		if (!dirty && PAGE.IsDirty) dirty = PAGE.IsDirty();
		if (!dirty)
		{
			this.ShowMessage(
				"<?echo I18N("j","Information");?>",
				"<?echo I18N("j","The configuration has not been changed.");?>"
			);
			return;
		}

		this.ClearConfigError();
		var xml = PAGE.PreSubmit();
		if (!xml) return;

		this.ShowInProgress();
		AUTH.UpdateTimeout();

		var self = this;
		PXML.UpdatePostXML(xml);
		PXML.Post(function(code, result){self.SubmitCallback(code,result);});
	},
	SubmitCallback: function(code, result)
	{
		if (PAGE && PAGE.OnSubmitCallback && PAGE.OnSubmitCallback(code, result)) return;
		this.ShowContent();
		switch (code)
		{
			case "OK":
				this.OnReload();
				break;
			case "BUSY":
				this.ShowMessage("<?echo I18N("j","Device is busy");?>",
					"<?echo I18N("j","Someone is configuring the device, please try again later.");?>");
				break;
			case "HEDWIG":
				this.ShowMessage("<?echo I18N("j","Invalid Setting");?>", result.Get("/hedwig/message"));
				break;
			case "PIGWIDGEON":
				if (result.Get("/pigwidgeon/message")==="no power")
					BODY.NoPower();
				else
					this.ShowMessage("<?echo I18N("j","Invalid Setting");?>", result.Get("/pigwidgeon/message"));
				break;
		}
	},
	DisableCfgElements: function(type)
	{
		for (var i = 0; i < document.forms.length; i+=1)
		{
			var frmObj = document.forms[i];
			for (var idx = 0; idx < frmObj.elements.length; idx+=1)
			{
				if (frmObj.elements[idx].getAttribute("usrmode")=="enable") continue;
				frmObj.elements[idx].disabled = type;
			}
		}
	},
	//////////////////////////////////////////////////
	OnReload: function()
	{
		if (PAGE && PAGE.OnLoad) PAGE.OnLoad();
		if (this.LabelOfErrorMsg!=null) this.ClearConfigError();
		this.GetCFG();
	},
	OnLoad: function()
	{
		var self = this;
		if (AUTH.AuthorizedGroup < 0)
		{
			this.ShowLogin();
			return;
		}
		else
			this.ShowContent();

		AUTH.TimeoutCallback = function()
		{
			OBJ("loginfo").innerHTML = "<?echo I18N("j","You have successfully logged out after $1 minutes.", query("/device/session/timeout")/60);?>";
			OBJ("loginfo").style.display = "";
			self.DisableCfgElements(false);
			self.ShowLogin();
		};

		if (PAGE && PAGE.OnLoad) PAGE.OnLoad();
		if (this.LabelOfErrorMsg!=null) this.ClearConfigError();
		this.GetCFG();
	},
	OnUnload: function()
	{
		if (PAGE && PAGE.OnUnload) PAGE.OnUnload();
		OnunloadAJAX();
	},
	OnKeydown: function(e)
	{
		switch (COMM_Event2Key(e))
		{
		case 13: this.LoginSubmit();
		default: return;
		}
	}
}

var AUTH = new Authenticate(<?=$AUTHORIZED_GROUP?>, <?echo query("/device/session/timeout");?>);
var PXML = new PostXML();
var BODY = new Body();
var PAGE = <?if ($HAVE_CFGJAVA==1) echo "new Page()"; else echo "null";?>;
<?
/* generate cookie */
if ($_SERVER["HTTP_COOKIE"] == "")
echo 'if (navigator.cookieEnabled) document.cookie = "uid="+COMM_RandomStr(10)+"; path=/";\n';
?>//]]>
	</script>
</head>

<body onload="BODY.OnLoad();" onunload="BODY.OnUnload();">
<div class="container">
	<div class="header"><a href="index.php" class="miiicasa"><?echo query("/runtime/device/vendor");?></a>
<?if ($FIRMWARE==1)		echo '\t\t<div class="fm_ware">'.I18N("h","Firmware version").": ".query("/runtime/device/firmwareversion")."</div>";?>
		<ul>
<?
if ($ICON_HOME==1)		echo '\t\t\t<li><a id="icon_home" href="home.php" class="icon_home"><span>'.I18N("h","Home").'</span></a></li>'.'\n';
if ($ICON_ADV==1)		echo '\t\t\t<li><a id="icon_adv" href="advanced.php" class="icon_advanced"><span>'.I18N("h","Advanced Mode").'</span></a></li>'.'\n';
if ($ICON_NETMAP==1)	echo '\t\t\t<li><a id="icon_netmap" href="netmap.php" class="icon_netmork_map"><span>'.I18N("h","Network Map").'</span></a></li>'.'\n';
?>		</ul>
	</div>
<?
echo '<div id="all_content" style="display:none;">\n';
dophp("load","/htdocs/webinc/body/".$TEMP_MYNAME.".php");
echo '</div>\n';
?>	<div id="login" class="maincolumn" style="display:none;">
		<form>
			<div class="rc_gradient_hd">
				<h2><?echo I18N("h","Welcome to the miiiCasa $1 Router",query("/runtime/device/modelname"));?></h2>
			</div>
			<div class="rc_gradient_ft h_initial">
				<h6><?echo I18N("h","Please enter the router login password.");?></h6>
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td nowrap="nowrap" class="td_left"><?echo I18N("h","Password");?> :</td>
					<td>
						<input type="password" class="text_block" id="loginpwd" size="40" onkeydown="BODY.OnKeydown(event);" />
						<label id="loginpwd_err" for="username" generated="true" class="mandatory" style="display:none;"><?echo I18N("h","Please enter password");?></label>
					</td>
				</tr>
				<tr id="captcha">
					<td nowrap="nowrap" class="td_left"><?echo I18N("h","Check Code");?> :</td>
					<td>
						<div id="captcha_msg"></div>
						<img id="auth_img" alt="" align="absbottom" class="captcha" />
						<input type="button" value="Regenerate" class="captcha" onClick="BODY.RefreshCaptcha();" /><br />
						<?echo I18N("h","Please type the characters you see in the picture above");?> :<br />
						<input type="text" class="text_block" id="logincap" size="40" style="text-transform:uppercase;" onkeydown="BODY.OnKeydown(event);" />
					</td>
				</tr>
				<tr>
					<td>&nbsp;</td>
					<td><label id="loginfo" generated="true" class="mandatory" style="display:none;"></label></td>
				</tr>
				<tr>
					<td>&nbsp;</td>
					<td>
						<button value="login" type="button" class="submitBtn" onclick="BODY.LoginSubmit();" style="margin-left:0;">
							<b><?echo I18N("h","Login");?></b>
						</button>
					</td>
				</tr>
				</table>
			</div>
		</form>
	</div>
	<!-- Processing Window -->
	<div id="main-loading" class="mod-loading" style="display:none;">
		<div class="mask"></div>
		<div class="center">
			<img class="loading" src="./pic/processing.gif" width="150" height="150">
		</div>
	</div>
	<!-- Processing Window -->
	<!-- Information popup -->
	<div id="popup_info" style="display:none;">
		<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form">
			<tr>
				<th class="rc_orange_hd"><a id="close_x" class="close" href="#"><?echo I18N("h","close");?></a><?echo I18N("h","Help");?></th>
			</tr>
			<tr>
				<td class="rc_orange_ft_lightbg"><iframe src="./support.php?temp=<?=$TEMP_MYHELP?>" frameborder="0" width="100%" height="500px" scrolling="no"></iframe></td>
			</tr>
		</table>
	</div>
	<!-- Information popup -->
	<div id="popup_msg" style="display:none;">
		<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form">
			<tr>
				<th class="rc_orange_hd"><a id="close_x" class="close" href="#"><?echo I18N("h","close");?></a><span id="msg_banner"></span></th>
			</tr>
			<tr>
				<td class="rc_orange_ft_lightbg"><div class="padding_aisle">
					<table border="0" cellpadding="0" cellspacing="0" class="status_report">
					<tr>
						<td style="border:none"><strong><span id="msg_content"></span></strong></td>
					</tr>
					</table>
				</div></td>
			</tr>
		</table>
	</div>
</div>
</body>
</html>
