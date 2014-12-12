	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Dynamic DNS");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Dynamic DNS");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","The Dynamic DNS feature allows you to host a server (Web, FTP, Game Server, etc...) using a domain name that you have purchased (www.whateveryournameis.com) with your dynamically assigned IP address.");?>
					<?echo I18N("h","Most broadband Internet Service Providers assign dynamic (changing) IP addresses.");?>
					<?echo I18N("h","Using a DDNS service provider, your friends can enter your host name to connect to your game server no matter what your IP address is.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Dynamic DNS Settings");?></p></td>
		</tr>
		<tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Dynamic DNS");?> :</td>
			<td width="76%" class="gray_bg border_right"><input type="checkbox" id="en_ddns" onclick="PAGE.EnableDDNS();" /></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Server Address");?> : </td>
			<td class="gray_bg border_right">
				<select id="server">
					<option value="DYNDNS.C">DynDns.org(Custom)</option>
					<option value="DYNDNS">DynDns.org(Free)</option>
					<option value="DYNDNS.S">DynDns.org(Static)</option>
				</select>
				<?drawlabel("server");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Host Name");?> : </td>
			<td class="gray_bg border_right">
				<input type="text" id="host" maxlength="60" size="40" class="text_block" />
				<?drawlabel("host");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Username or Key");?> : </td>
			<td class="gray_bg border_right">
				<input type="text" id="user" maxlength="16" size="40" class="text_block" />
				<?drawlabel("user");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Password or Key");?> : </td>
			<td class="gray_bg border_right">
				<input type="password" id="passwd" maxlength="16" size="40" class="text_block" />
				<?drawlabel("passwd");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Verify Password or Key");?> : </td>
			<td class="gray_bg border_right">
				<input type="password" id="passwd_verify" maxlength="16" size="40" class="text_block" />
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Timeout");?> : </td>
			<td class="gray_bg border_right">
				<input type="text" id="timeout" maxlength="4" size="10" class="text_block" />
				<?echo I18N("h","(hours)");?>
				<?drawlabel("timeout");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Status");?> : </td>
			<td class="gray_bg border_right">
				<span id="report" ></span>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
