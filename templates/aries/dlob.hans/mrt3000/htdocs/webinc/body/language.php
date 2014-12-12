	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Language");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","System language");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","You may change your display language.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Language settings");?></p></td>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","Please select your language.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Language settings");?> : </td>
			<td width="76%" class="gray_bg border_right">
				<select id="lang" name="multilanguage">
					<option value="zhtw"><?echo I18N("h","Traditional Chinese");?></option>
					<option value="en"<?if (query("/device/features/language")!="zhtw") echo " selected";?>><?echo I18N("h","English");?></option>
				</select>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick='BODY.OnClickSetLang(OBJ("lang").value);'><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
