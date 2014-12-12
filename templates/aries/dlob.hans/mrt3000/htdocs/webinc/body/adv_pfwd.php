	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Port Forwarding");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="rc_gray5_hd"><h2><?echo I18N("h","Port Forwarding");?></h2></th>
		</tr>
		<tr>
			<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","This option is used to open multiple ports or a range of ports in your router and redirect data through those ports to a single PC on your network.")." ".
										I18N("h","This feature allows you to enter ports in the format, Port Ranges (100-150), Individual Ports (80, 68, 888), or Mixed (1020-5000, 689).")." ".
										I18N("h","This option is only applicable to the INTERNET session.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side"><p class="subitem"><?=$PFWD_MAX_COUNT?> -- <?echo I18N("h","Port Forwarding Rules");?></p></td>
		</tr>

		<tr>
			<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span>
				</cite>
			</td>
		</tr>

		<tr>
			<th width="20px" class="gray_bg border_left">&nbsp;</th>
			<th width="136px" class="gray_bg">&nbsp;</th>
			<th width="162px" class="gray_bg">&nbsp;</th>
			<th width="105px" class="gray_bg<?if ($FEATURE_NOSCH=="1"){echo ' border_right';}?>"><?echo I18N("h","Ports to Open");?></th>
			<?if ($FEATURE_NOSCH!="1"){echo '<th width="83px" class="gray_bg border_right">&nbsp;</th>\n';}?>
		</tr>
<?
$INDEX = 1;
while ($INDEX <= $PFWD_MAX_COUNT) {	dophp("load", "/htdocs/webinc/body/adv_pfwd_list.php");	$INDEX++; }
?>

		<tr>
			<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>				
			</td>
		</tr>

		</table>
	</div>
	</form>
