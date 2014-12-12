<?include "/htdocs/phplib/inet.php";?>

<?include "/htdocs/phplib/lang.php";?>
<? $langcode = wiz_set_LANGPACK();?>
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
	services: "INET.WAN-1,WAN",
	logindefault: 0,
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code,result) 
	{
		switch (code)
		{
		case "OK":
			if(PAGE.url_mydlink != "") self.location.href = PAGE.url_mydlink;
			else self.location.href = 'http://www.mydlink.com';
			break;
		case "BUSY":
			this.ShowAlert("<?echo I18N("j","Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			this.ShowAlert(result.Get("/hedwig/message"));
			if (PAGE.CursorFocus) PAGE.CursorFocus(result.Get("/hedwig/node"));  
			break;
		case "PIGWIDGEON":
			if (result.Get("/pigwidgeon/message")=="no power")
			{
				BODY.NoPower();
			}
			else
			{
				this.ShowAlert(result.Get("/pigwidgeon/message"));
			}
			break;
		}		
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.ShowCurrentStage();
		this.CheckPassword();
		return true;
	},
	CheckPassword: function()
	{
		<?
			$password_blank="true";
			foreach ("/device/account/entry")
			{
				if(tolower(query("name"))=="admin" && query("password")!="")
				{
					$password_blank="false";
					break;
				}	
			}
		?>
		var password_blank = <? echo $password_blank;?>;
		if(password_blank)
		{
			if(confirm('<?echo I18N("j", "The Admin password could not be blank to set the mydlink registration. Would you want to change the Admin password?");?>')) self.location.href="tools_admin.php";
			else self.location.href="bsc_mydlink.php";
		}	
	},	
	PreSubmit: function() {},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	passwdp: null,
	url_mydlink: null,
	currentStage: 0,	// 0 ~ this.stages.length
	stages: new Array ("register", "register_fill", "register_verify" , "register_login"),
	ShowCurrentStage: function()
	{
		for (var i=0; i<this.stages.length; i++)
		{
			if (i==this.currentStage) OBJ(this.stages[i]).style.display = "block";
			else OBJ(this.stages[i]).style.display = "none";
		}
	},
	SetStage: function(offset)
	{
		var length = this.stages.length;
		switch (offset)
		{
		case 2:
			if (this.currentStage < length-1)
				this.currentStage += 2;
			break;			
		case 1:
			if (this.currentStage < length-1)
				this.currentStage += 1;
			break;
		case -1:
			if (this.currentStage > 0)
				this.currentStage -= 1;
			break;
		case -2:
			if (this.currentStage > 1)
				this.currentStage -= 2;
		case -3:
			if (this.currentStage > 2)
				this.currentStage -= 3;
			break;			
		}
	},
	OnClickPre: function()
	{
		this.SetStage(-1);
		this.ShowCurrentStage();
	},
	OnClickNext: function()
	{
		var stage = this.stages[this.currentStage];
		if (stage == "register")
		{
			if(GetRadioValue("register_account")=="yes") 
			{ for(var i=0; i < this.stages.length; i++) if(this.stages[i]==="register_login") this.currentStage=i;}
			else this.SetStage(1);
			this.ShowCurrentStage();			
		}			
		else
		{	
			this.SetStage(1);
			this.ShowCurrentStage();
		}
		
	},
	OnClickCancel: function()
	{
		<? 
			if($_GET["freset"]=="1") echo 'self.location.href="bsc_internet.php";';
			else echo 'self.location.href="bsc_mydlink.php";';
		?> 
	},
	OnClickMydlinkRegister: function()
	{
		
		if(OBJ("email").value=="")
		{
			BODY.ShowAlert("<?echo i18n("Please enter the email address.");?>");
			return false;			
		}
		if(!IsVaildEmail(OBJ("email").value))
		{
			BODY.ShowAlert("<?echo i18n("Please enter a valid Email Address.");?>");
			return false;			
		}		
		if (OBJ("register_pwd").value!=OBJ("register_pwd_confirm").value)
		{
			BODY.ShowAlert("<?echo i18n("Please make two passwords the same and try again");?>");
			return false;
		}
		if (OBJ("register_pwd").value.length < 6)
		{
			BODY.ShowAlert("<?echo I18N("j","Password length must be bigger than 6."); ?>");
			return false;
		}
		if(!OBJ("register_accept").checked)
		{
			BODY.ShowAlert("<?echo i18n("You must accept the terms and conditions to continue.");?>");
			return false;			
		}
		
		OBJ("mydlink_regist_button").disabled = true;
		var ajaxObj = GetAjaxObj("Register");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			if(xml.Get("/register_send/result")=="success") PAGE.OnClickNext();
			else 
			{
				BODY.ShowAlert(xml.Get("/register_send/result"));
				OBJ("mydlink_regist_button").disabled = false;
			}
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("register_send.php", "act=signup&lang="+OBJ("lang").value+"&outemail="+OBJ("email").value+"&passwd="+OBJ("register_pwd").value+"&firstname="+OBJ("register_firstname").value+"&lastname="+OBJ("register_lastname").value);
	},
	OnClickMydlinkLogin: function()
	{

		if(OBJ("email_login").value=="")
		{
			BODY.ShowAlert("<?echo i18n("Please enter the email address.");?>");
			return false;			
		}
		if (OBJ("register_pwd_login").value.length < 6)
		{
			BODY.ShowAlert("<?echo I18N("j","Password length must be bigger than 6."); ?>");
			return false;
		}
		if(!IsVaildEmail(OBJ("email_login").value))
		{
			BODY.ShowAlert("<?echo i18n("Please enter a valid Email Address.");?>");
			return false;			
		}
		
		OBJ("mydlink_login_button").disabled = true;
		var ajaxObj = GetAjaxObj("Mydlink_login");
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{
			ajaxObj.release();
			if(xml.Get("/register_send/result")=="success")
			{
				BODY.ShowAlert("<?echo i18n("You may now use mydlink service with this device");?>");
				PAGE.url_mydlink = xml.Get("/register_send/url");
				PAGE.ChangeWanAlwaysOn();//If wan type is ppp4, make the connection mode always on.
			}
			else
			{	 
				BODY.ShowAlert(xml.Get("/register_send/result"));
				OBJ("mydlink_login_button").disabled = false;
			}
		}
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("register_send.php", "act=signin&lang="+OBJ("lang").value+"&outemail="+OBJ("email_login").value+"&passwd="+OBJ("register_pwd_login").value+"&mydlink_cookie=");
	},
	ChangeWanAlwaysOn: function()
	{
		//If wan type is ppp4, make the connection mode always on.
		var wan	= PXML.FindModule("INET.WAN-1");
		var waninetuid = XG  (wan+"/inf/inet");
		var waninetp = GPBT(wan+"/inet", "entry", "uid", waninetuid, false);
		if (XG(waninetp+"/addrtype")=="ppp4" && XG(waninetp+"/ppp4/dialup/mode")!="auto")
		{
			XS(waninetp+"/ppp4/dialup/mode", "auto");
			PXML.CheckModule("INET.WAN-1", null, null, "ignore");
			PXML.CheckModule("WAN", null, "ignore", null);
			xml = PXML.doc;
			PXML.UpdatePostXML(xml);
			PXML.Post(function(code, result){PAGE.OnSubmitCallback(code,result);});			
		}
		else
		{
			if(PAGE.url_mydlink != "") self.location.href = PAGE.url_mydlink;
			else self.location.href = 'http://www.mydlink.com';
		}		
	}	
}

function GetRadioValue(name)
{
	var radio = document.getElementsByName(name);
	var value = null;
	for (i=0; i<radio.length; i++)
	{
		if (radio[i].checked)	return radio[i].value;
	}
}
function SetRadioValue(name, value)
{
	var radio = document.getElementsByName(name);
	for (i=0; i<radio.length; i++)
	{
		if (radio[i].value==value)	radio[i].checked = true;
	}
}
function IsVaildEmail(email)
{
	/*	Email rule:
	*	1.It must have only one ．@・.
	*	2.The first character should not be ．@・ or ．.・.
	*	3.The last character should not be ．@・ or ．.・.
	*	4.It should not contain ．@.・, ．.@・, ．-@・, ．@-・.
	*	5.The words after ．@・ only could be ．A-Z・, ．a-z・ or ．0-9・.	
	*/
	var strReg=/^\w+((-\w+)|(\.\w+))*\@[A-Za-z0-9]+((\.|-)[A-Za-z0-9]+)*\.[A-Za-z0-9]+$/i;
	if(email.search(strReg)==-1) return false;
	else return true;
}
</script>
