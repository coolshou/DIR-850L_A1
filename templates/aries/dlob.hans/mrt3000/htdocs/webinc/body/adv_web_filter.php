	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Website Filter");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th class="rc_gray5_hd"><h2><?echo I18N("h","Website Filter");?></h2></th>
		</tr>
        <tr>
			<td class="gray_bg border_2side"><cite><?echo I18N("h", 'The Website Filter option allows you to set up a list of Web sites you would like to allow or deny through your network. To use this feature, you must also select the "Apply Web Filter" checkbox in the Access Control section.');?></cite>
            </td>
		</tr>
        <tr>
        	<td class="gray_bg border_2side"><p class="subitem"><?=$URL_MAX_COUNT?> -- <?echo I18N("h","Application Rules ");?></p></td>
        </tr>
        
        <tr>
        	<td class="gray_bg border_2side"><cite>
            	<?echo I18N("h","Configure Website Filter below:");?></cite>
				<div>	
				&nbsp;	
				<select id="url_mode">
					<option value="ACCEPT"><?echo I18N("h","ALLOW computers access to ONLY these sites");?></option>
					<option value="DROP"><?echo I18N("h","DENY computers access to ONLY these sites");?></option>
				</select>
				</div>
                <div>
				&nbsp;
				<input id="clear_url" type="button" value="<?echo I18N("h","Clear the list below");?>..." onClick="PAGE.OnClickClearURL();" />
				</div>
            </td>
        </tr>
        
        <tr>
        	<td nowrap="nowrap" class="gray_bg border_left">
				<div class="gap"></div>        
            </td>
		<tr>
        
        
        
        <tr>
        	<td nowrap="nowrap" class="gray_bg border_left"><cite>
            <div class="rc_map" >
            <table id="leases_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            	<tr>
                <td colspan="2" align="center"><strong><?echo I18N("h","Website URL");?>/<?echo I18N("h","Domain");?></strong></td>	
               	</tr>
<?
$INDEX = 1;
while ($INDEX <= $URL_MAX_COUNT)
{	
	if($INDEX%2==0)
		echo '<tr class="light_bg">';
   	else
		echo '<tr class="gray_bg">';
	echo	"	<td nowrap=\"nowrap\" class=\"border_left gray_border_btm\" align=\"center\"><input type=text id=url_".$INDEX." size=44 maxlength=99></td>"."\n";
	$INDEX++;
	echo	"	<td nowrap=\"nowrap\" class=\"border_left gray_border_btm\" align=\"center\"><input type=text id=url_".$INDEX." size=44 maxlength=99></td>"."\n";
	$INDEX++;	
	echo	"</tr>"."\n";
}			
?>
            </table>
            </div></cite>
            </td>
		</tr>			
		<tr>
        	<td nowrap="nowrap" class="gray_bg border_left">
				<div class="gap"></div>
			</td>
		<tr>
		<tr>
			<td class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
