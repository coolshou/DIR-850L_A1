<?
	include "/htdocs/phplib/lang.php";
	$lang_code = wiz_set_LANGPACK();
	if($lang_code=="en") $lang="English";
	else if($lang_code=="es") $lang="Español";
	else if($lang_code=="de") $lang="Deutsch";
	else if($lang_code=="fr") $lang="Français";
	else if($lang_code=="it") $lang="Italiano";
	else if($lang_code=="ru") $lang="Русский";
	else if($lang_code=="pt") $lang="Português";
	else if($lang_code=="jp") $lang="日本語";
	else if($lang_code=="zhtw") $lang="繁體中文";
	else if($lang_code=="zhcn") $lang="简体中文";
	else if($lang_code=="ko") $lang="한국어";
	else if($lang_code=="cs") $lang="Česky";
	else if($lang_code=="da") $lang="Dansk";
	else if($lang_code=="el") $lang="Ελληνικά";
	else if($lang_code=="fi") $lang="Suomi";
	else if($lang_code=="hr") $lang="Hrvatski";
	else if($lang_code=="hu") $lang="Magyar";
	else if($lang_code=="nl") $lang="Nederlands";
	else if($lang_code=="no") $lang="Norsk";
	else if($lang_code=="pl") $lang="Polski";
	else if($lang_code=="ro") $lang="Română";
	else if($lang_code=="sl") $lang="Slovenščina";
	else if($lang_code=="sv") $lang="Svenska";
	else $lang="English";
?>
<div style="position:relative;">
	<div style="position:absolute;left:600px;bottom:30px;">
		<table cellspacing="0" cellpadding="0" border="0">
			<tbody>
				<tr>
					<td><strong><? echo I18N("h", "Language");?>&nbsp;:&nbsp;</strong></td>
					<td><input type="text" readonly="" style="cursor:default" value="" maxlength="15" size="20" id="tlang" onclick="OBJ('language_menu').style.display=='none'?OBJ('language_menu').style.display='block':OBJ('language_menu').style.display='none';"></td>
					<td><img width="18" height="20" src="/pic/wiz_lang_button1.png" onclick="OBJ('language_menu').style.display=='none'?OBJ('language_menu').style.display='block':OBJ('language_menu').style.display='none';" onload='OBJ("tlang").value="<? echo $lang;?>";'></td>
				</tr>
			</tbody>
		</table>
	</div>	
	<div id="language_menu" class="langmenu" onclick="OBJ('language_menu').style.display='none';" style="display:none;">
		<div class="langmenu_column">
			<ul>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=en">English</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=es">Español</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=de">Deutsch</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=fr">Français</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=it">Italiano</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=ru">Русский</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=pt">Português</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=jp">日本語</a></li>
			</ul>
		</div>
		<div class="langmenu_column">
			<ul>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=zhtw">繁體中文</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=zhcn">简体中文</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=ko">한국어</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=cs">Česky</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=da">Dansk</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=el">Ελληνικά</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=fi">Suomi</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=hr">Hrvatski</a></li>
			</ul>
		</div>
		<div class="langmenu_column">
			<ul>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=hu">Magyar</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=nl">Nederlands</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=no">Norsk</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=pl">Polski</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=ro">Română</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=sl">Slovenščina</a></li>
				<li><a href="/<? echo $_GLOBALS['TEMP_MYNAME'].'.php';?>?language=sv">Svenska</a></li>
			</ul>
		</div>
	</div>
</div>	