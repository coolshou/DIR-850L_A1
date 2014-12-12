	<? include "/htdocs/webinc/body/draw_elements.php";?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Application Rules");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="Help"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="6" class="rc_gray5_hd"><h2><?echo I18N("h","Application Rules");?></h2></th>
		</tr>
        <tr>
			<td colspan="6" class="gray_bg border_2side"><cite><?echo I18N("h",'The Application Rules option is used to open single or multiple ports in your firewall when the router senses data sent to the Internet on an outgoing "Trigger" port or port range.')." ";
		echo I18N("h","Special Application rules apply to all computers on your internal network.");?></cite>
        	</td>
        </tr>
        <tr>
        	<td colspan="6" class="gray_bg border_2side"><p class="subitem"><?=$APP_MAX_COUNT?> -- <?echo I18N("h","Application Rules ");?></p></td>
        </tr>
        <tr>
        	<td colspan="6" class="gray_bg border_2side"><cite>
        		<?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span></cite></td>
        </tr>
        
		<tr>
        	<td colspan="6" nowrap="nowrap" class="gray_bg border_2side"><cite>
            <div class="rc_map">
            <table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            <tr>
				<td width="5%">&nbsp;</td>
				<td width="25%">&nbsp;</td>
				<td width="30%">&nbsp;</td>
				<td width="10%"><strong><?echo I18N("h","Port");?></strong></td>
				<td width="20%"><strong><?echo I18N("h","Traffic Type");?></strong></td>
			</tr>
        	<?
			$INDEX = 1;
			while ($INDEX <= $APP_MAX_COUNT) {	dophp("load", "/htdocs/webinc/body/adv_app_list.php");	$INDEX++; }
			?>
            </table>
            </div></cite>
            </td>
       	</tr>
        	
		<tr>
			<td colspan="6" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
