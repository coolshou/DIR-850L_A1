	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Routing");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="5" class="rc_gray5_hd"><h2><?echo I18N("h","Routing");?></h2></th>
		</tr>
        <tr>
			<td colspan="5" class="gray_bg border_2side"><cite><?echo I18N("h","The Routing option allows you to define static routes to specific destinations. ");?></cite>
        	</td>
        </tr>
        <tr>
        	<td colspan="5" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h",$ROUTING_MAX_COUNT);?> -- <?echo I18N("h","ROUTE LIST");?></p></td>
        </tr>
        <tr>
			<td colspan="5" nowrap="nowrap" class="gray_bg border_2side"><cite><?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span></cite>
            </td>
		</tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side"><cite>
            <div class="rc_map" >
            <table id="leases_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            	<tr>
					<td width="5%">&nbsp;</td>
            		<td width="25%">&nbsp;</td>
            		<td width="30%">&nbsp;</td>
            		<td width="10%">&nbsp;</strong></td>
            		<td width="30%">&nbsp;</strong></td>
            	</tr>
                <?
				$ROUTING_INDEX = 1;
				while ($ROUTING_INDEX <= $ROUTING_MAX_COUNT) {	dophp("load", "/htdocs/webinc/body/adv_routing_list.php");							
				$ROUTING_INDEX++; }
				?>
			</table></div></cite>
            </td>
		</tr>
		<tr>
			<td colspan="5" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
