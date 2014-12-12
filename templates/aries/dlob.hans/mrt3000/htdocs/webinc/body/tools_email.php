<? include "/htdocs/webinc/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Email Settings");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>

		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
			<tr>
				<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Email Settings");?></h2></th>
			</tr>
			<tr>
				<td colspan="2" class="gray_bg border_2side">
					<cite>
						<?	echo I18N("h","The Email feature can be used to send the system log files and router alert messages to your email address.");?>
					</cite>
				</td>
			</tr>
			<tr style="display: none;">
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Email Notification");?></p></td>
			</tr>
			<tr style="display: none;">
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Email Notification");?> :</td>
				<td class="gray_bg border_right"><input id="en_mail" type="checkbox" onclick="PAGE.OnClickEnable();"/></td>
			</tr>
			
			<tr>
				<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Email Settings");?></p></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","From Email Address");?> :</td>
				<td class="gray_bg border_right"><input id="from_addr" type="text" size="20" maxlength="64" /><?drawlabel("from_addr");?></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","To Email Address");?> :</td>
				<td class="gray_bg border_right"><input id="to_addr" type="text" size="20" maxlength="64" /><?drawlabel("to_addr");?></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Email Subject");?> :</td>
				<td class="gray_bg border_right"><input id="email_subject" type="text" size="20" maxlength="64" /></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","SMTP Server Address");?> :</td>
				<td class="gray_bg border_right"><input id="smtp_server_addr" type="text" size="20" maxlength="64" /><?drawlabel("smtp_server_addr");?></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","SMTP Server Port");?> :</td>
				<td class="gray_bg border_right"><input id="smtp_server_port" type="text" size="20" maxlength="64" /><?drawlabel("smtp_server_port");?></td>
			</tr>						
			<tr style="display: none;">
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Authentication");?> :</td>
				<td class="gray_bg border_right"><input id="authenable" type="checkbox" onclick="PAGE.OnClickAuthEnable();"/></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Account Name");?> :</td>
				<td class="gray_bg border_right"><input id="account_name" type="text" size="20" maxlength="64" /><?drawlabel("account_name");?></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
				<td class="gray_bg border_right"><input id="passwd" type="password" size="20" maxlength="64" /><?drawlabel("passwd");?></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Verify Password");?> :</td>
				<td class="gray_bg border_right"><input id="verify_passwd" type="password" size="20" maxlength="64" /><?drawlabel("verify_passwd");?>
								<input id="sendmail" type="button" value="<?echo I18N("h","Send Mail Now");?>" onClick="PAGE.OnClickSendMail();"/></td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td class="gray_bg border_right">
					<div class="textinput" id="send_msg" style="display:none">
						<span class="name"></span>
						<span class="value"><font color="red"><?echo I18N("h","(Mail sent already!)");?></font></span>
					</div>
				</td>
			</tr>
			<tr>
				<td colspan="2" class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				</td>
			</tr>
		</table>
	</div>
</form>
