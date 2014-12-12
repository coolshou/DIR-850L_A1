<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Administrator Settings");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>

		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Administrator Settings");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?
						if ($USR_ACCOUNTS=="1")
							echo I18N("h","The 'admin' account can access the management interface.")." ".
							I18N("h","The admin has read/write access and can change password.");
						else
							echo I18N("h","The 'admin' and 'user' accounts can access the management interface.")." ".
							I18N("h","The admin has read/write access and can change passwords, while the user has read-only access.");
					?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Admin Password");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right"><input id="admin_p1" type="password" size="20" maxlength="15" /></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Verify Password");?> :</td>
			<td class="gray_bg border_right"><input id="admin_p2" type="password" size="20" maxlength="15" /><?drawlabel("admin_p2");?></td>
		</tr>
		<tr <?if($USR_ACCOUNTS=="1") echo 'style="display:none;"';?>>
		<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","User Password");?></p></td>
		</tr>
		<tr	<?if($USR_ACCOUNTS=="1") echo 'style="display:none;"';?>>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","Please enter the same password into both boxes, for confirmation.");?></cite></td>
		</tr>
		<tr <?if($USR_ACCOUNTS=="1") echo 'style="display:none;"';?>>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password");?> :</td>
			<td class="gray_bg border_right"><input id="usr_p1" type="password" size="20" maxlength="15" /></td>
		</tr>
		<tr <?if($USR_ACCOUNTS=="1") echo 'style="display:none;"';?>>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Verify Password");?> :</td>
			<td class="gray_bg border_right"><input id="usr_p2" type="password" size="20" maxlength="15" /><?drawlabel("usr_p2");?></td>
		</tr>

		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","System Name");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Gateway Name");?> :</td>
			<td class="gray_bg border_right"><input id="gw_name" type="text" size="20" maxlength="15" /><?drawlabel("gw_name");?></td>
		</tr>

		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Administration");?></p></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Graphical Authentication");?> :</td>
			<td class="gray_bg border_right"><input id="en_captcha" type="checkbox" /></td>
		</tr>
<!--		
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable HTTPS Server");?> :</td>
			<td class="gray_bg border_right"><input id="stunnel" type="checkbox" onClick="PAGE.OnClickStunnel();" /></td>
		</tr>
-->
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Remote Management");?> :</td>
			<td class="gray_bg border_right"><input id="en_remote" type="checkbox" onClick="PAGE.OnClickEnRemote();" /></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Remote Admin Port");?> :</td>
			<td class="gray_bg border_right"><input id="remote_port" type="text" size="5" maxlength="5" /><?drawlabel("remote_port");?>
<!--						&nbsp;<?echo I18N("h","Use HTTPS:");?><input id="enable_https" type="checkbox" onClick="PAGE.OnClickEnableHttps();" /> --></td>
		</tr>
<!--		
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Remote Admin");?>&nbsp<a href="./adv_inb_filter.php"><? echo I18N("h","Inbound Filter");?> :</td>
			<td class="gray_bg border_right">
				<select id="remote_inb_filter" onChange="PAGE.OnClickRemoteInbFilter(this.value)">
					<option value=""><?echo I18N("h","Allow All");?></option>
					<option value="denyall"><?echo I18N("h","Deny All");?></option>
<?
				foreach ("/acl/inbfilter/entry")
				echo '\t\t\t<option value="'.query("uid").'">'.query("description").'</option>\n';
?>
				</select></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Details");?> :</td>
			<td class="gray_bg border_right"><input type="text" id="inb_filter_detail" maxlength="60" size="40" disabled></td>
		</tr>
-->
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
			</td>
		</tr>
		</table>
	</div>
</form>
