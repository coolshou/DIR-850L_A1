	<div id="content" class="maincolumn">
		<form>
			<div class="rc_gradient_hd"><h2><?echo I18N("h","Enter account details");?></h2></div>
			<div class="rc_gradient_bd h_initial">
				<h6><?echo I18N("h",'You are now connected to the Internet. If you want to change your Internet conneciton setting, please click on "Next".');?></h6>
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","User name");?> :</strong></td>
					<td width="674"><span class="maroon">User name</span> (PPPoE)</td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Password");?> :</strong></td>
					<td><span class="maroon">password</span></td>
				</tr>
				</table>
			</div>
			<div class="rc_gradient_submit">
				<button value="cancel" type="button" class="submitBtn floatLeft" onclick="javascript:location.href='home.php'">
					<b><?echo I18N("h","Cancel");?></b>
				</button>
				<button value="next" type="button" class="submitBtn floatRight" onclick="javascript:location.href='change.php'">
					<b><?echo I18N("h","Next");?></b>
				</button>
				<i></i>
			</div>
		</form>
	</div>
