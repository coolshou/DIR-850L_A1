	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Virtual Server");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="5" class="rc_gray5_hd"><h2><?echo I18N("h","Virtual Server");?></h2></th>
		</tr>
		<tr>
			<td colspan="5" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","The Virtual Server option allows you to define a single public port on your router for redirection to an internal LAN IP Address and Private LAN port if required.");?>
					<?echo I18N("h","This feature is useful for hosting online services such as FTP or Web Servers.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="5" class="gray_bg border_2side"><p class="subitem"><?=$VSVR_MAX_COUNT?> -- <?echo I18N("h","Virtual Servers List");?></p></td>
		</tr>
		<tr>
			<td colspan="5" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span>
				</cite>
			</td>
		</tr>
		<tr>
			<td width="5%">&nbsp;</td>
			<td width="25%">&nbsp;</td>
			<td width="30%">&nbsp;</td>
			<td width="10%">
				<strong><? echo I18N("h","Port");?></strong>
			</td>
			<td width="20%">
				<strong><? echo I18N("h","Traffic Type");?></strong>
			</td>
		</tr>		
		
<?
$INDEX = 1;
while ($INDEX <= $VSVR_MAX_COUNT) {	dophp("load", "/htdocs/webinc/body/adv_vsvr_list.php");	$INDEX++; }
?>

		<tr>
			<td colspan="5" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>				
			</td>
		</tr>

		</table>
	</div>
	</form>
