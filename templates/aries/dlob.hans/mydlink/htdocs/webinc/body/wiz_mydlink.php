
<?
	include "/htdocs/phplib/lang.php";
	
	if($_GET["freset"]=="1") $step_wiz_freset = i18n("Step 6").": ";
	else $step_wiz_freset = "";	
	
	function language()
	{
		$lang_code = wiz_set_LANGPACK();
		if($lang_code=="en") $lang="en";
		else if($lang_code=="fr") $lang="fr";
		else if($lang_code=="ru") $lang="ru";
		else if($lang_code=="es") $lang="es";
		else if($lang_code=="pt") $lang="pt";
		else if($lang_code=="jp") $lang="ja";
		else if($lang_code=="zhtw") $lang="zh_TW";
		else if($lang_code=="zhcn") $lang="zh_CN";
		else if($lang_code=="ko") $lang="ko";
		else if($lang_code=="cs") $lang="cs";
		else if($lang_code=="da") $lang="da";
		else if($lang_code=="de") $lang="de";
		else if($lang_code=="el") $lang="el";
		else if($lang_code=="hu") $lang="hu";
		else if($lang_code=="hr") $lang="hr";
		else if($lang_code=="it") $lang="it";
		else if($lang_code=="nl") $lang="nl";
		else if($lang_code=="no") $lang="no";
		else if($lang_code=="pl") $lang="pl";
		else if($lang_code=="ro") $lang="ro";
		else if($lang_code=="sl") $lang="sl";
		else if($lang_code=="sv") $lang="sv";
		else if($lang_code=="fi") $lang="fi";
		else $lang = "en";
		return $lang;
		//echo $lang;		
	}
	function Hyperlink_lang()
	{
	$lang=language()	;
		if($lang=="zh_CN")
		{
			$hyperlink="http://cn.mydlink.com/termsOfUse?lang=".$lang;
		}
		else if($lang=="zh_TW")
		{
			$hyperlink="http://tw.mydlink.com/termsOfUse?lang=".$lang;
		}
		else
		{
			$hyperlink="http://eu.mydlink.com/termsOfUse?lang=".$lang;
		}
		//echo $hyperlink;
		return $hyperlink;

	}
?>
<!-- Start of Stage Registration -->
<div id="register" class="blackbox" style="display:none;">
	<h2><?echo $step_wiz_freset.i18n("mydlink Registration");?></h2>
	<div><p class="strong" style="line-height:20px;">
		<? echo i18n("<strong>This device is mydlink-enabled, which allows you to remotely monitor and manage your network through the mydlink.com website, or through the mydlink mobile app. You will be able to check your network speeds, see who is connected, view device browsing history, and receive notifications about new users or intrusion attempts.</strong>").;?>
	</p></div>
	<div><p class="strong" style="line-height:20px;">
		<? echo i18n("You can register this device with your existing mydlink account. If you do not have one, you can create one now.");?>
	</p></div>
	<div style="margin-left:120px;"><? echo i18n("Do you have mydlink account?");?></div>
	<div style="margin-left:120px;"><input name="register_account" type="radio" value="yes"/><? echo i18n("Yes, I have a mydlink account.");?></div>
	<div style="margin-left:120px;"><input name="register_account" type="radio" value="no" checked/><? echo i18n("No, I want to register and login with a new mydlink account.");?></div>	
	<div class="emptyline"><input type="hidden" id="lang" value="<? echo language();?>" /></div>
	<div class="centerline">
		<input type="button" value="<? echo i18n("Skip");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Next");?>" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;
	</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage Registration -->
<!-- Start of Stage Registration Fill -->
<div id="register_fill" class="blackbox" style="display:none;">
	<h2><?echo $step_wiz_freset.i18n("mydlink Registration");?></h2>
	<div class="centerline"><p class="strong"><? echo i18n("Please fulfill the options to complete the registration.");?></p></div>
	<div class="emptyline"></div>
	<div class="textinput">
		<span class="name"><?echo i18n("E-mail Address (Account Name)");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="email" type="text" size="20" maxlength="128" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="register_pwd" type="password" size="20" maxlength="128" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Confirm Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="register_pwd_confirm" type="password" size="20" maxlength="128" style="float:left;"/>
			&nbsp;<!--img src="pic/stop.jpg"-->
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Last name");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="register_lastname" type="text" size="20" maxlength="128" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("First Name");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="register_firstname" type="text" size="20" maxlength="128" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><input id="register_accept" type="checkbox" /></span>
		<span class="delimiter"></span>
		<span class="value"><a href='<? echo Hyperlink_lang();?>' target=_blank><? echo i18n("I Accept the mydlink terms and conditions.");?></a></span>	
	</div>
	<div class="emptyline"></div>
	<div class="centerline">
		<input type="button" value="<? echo i18n("Skip");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Prev");?>" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Sign up");?>" onClick="PAGE.OnClickMydlinkRegister();" id="mydlink_regist_button" />&nbsp;&nbsp;
	</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage Registration Fill -->
<!-- Start of Stage Registration Verify and Login -->
<div id="register_verify" class="blackbox" style="display:none;">
	<h2><?echo $step_wiz_freset.i18n("mydlink Registration");?></h2>
	<div><p><? echo i18n("To complete your mydlink registration, please check your Inbox for an email with confirmation instructions.").i18n(" After confirming your email address, click the Login button.");?></p></div>
	<div class="emptyline"></div>
	<div class="centerline">
		<input type="button" value="<? echo i18n("Skip");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Login");?>" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;
	</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage Registration Verify and Login -->
<!-- Start of Stage Registration Login -->
<div id="register_login" class="blackbox" style="display:none;">
	<h2><?echo $step_wiz_freset.i18n("mydlink Registration");?></h2>
	<div class="emptyline"></div>
	<div class="textinput">
		<span class="name"><?echo i18n("E-mail Address (Account Name)");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="email_login" type="text" size="20" maxlength="128" />
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="register_pwd_login" type="password" size="20" maxlength="128" />
		</span>
	</div>
	<div class="emptyline"></div>
	<div class="centerline">
		<input type="button" value="<? echo i18n("Skip");?>" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Prev");?>" onClick="PAGE.SetStage(-3);PAGE.ShowCurrentStage();" />&nbsp;&nbsp;
		<input type="button" value="<? echo i18n("Login");?>" onClick="PAGE.OnClickMydlinkLogin();" id="mydlink_login_button"/>&nbsp;&nbsp;
	</div>
	<div class="emptyline"></div>
</div>
<!-- End of Stage Registration Login -->
</form>
