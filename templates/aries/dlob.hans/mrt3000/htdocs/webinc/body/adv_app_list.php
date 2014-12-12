<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<? if($INDEX%2==0)
	echo '<tr class="light_bg">';
   else
	echo '<tr class="gray_bg">';
?>
        	<td rowspan="2" class="border_left gray_border_btm">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td rowspan="2" class="border_left gray_border_btm"><?echo I18N("h","Name");?><br /><input id="name_<?=$INDEX?>" type="text" size="15" maxlength="15"/><?drawlabel("<? echo 'name_'.$INDEX;?>");?></td>
			<td rowspan="2" class="border_left gray_border_btm">
				<?echo I18N("h","Application");?><br />
				<input type="button" value="<<" class="arrow" onclick="OnClickAppArrow('<?=$INDEX?>');" modified="ignore" />
				<span id="span_app_<?=$INDEX?>"></span>
			</td>
			
			<td class="border_left gray_border_btm"><?echo I18N("h","Trigger");?><br /><input id="priport_<?=$INDEX?>" type="text" size="6" maxlength="11" /><?drawlabel("<? echo 'priport_'.$INDEX;?>");?></td>
			<td class="border_left gray_border_btm"><!--<?echo I18N("h","Protocol");?><br />-->
			<br />
				<select id="pripro_<?=$INDEX?>">
				    <option value="TCP+UDP"><?echo I18N("h","All");?></option>
					<option value="TCP"><?echo I18N("h","TCP");?></option>
					<option value="UDP"><?echo I18N("h","UDP");?></option>
				</select>
			</td>

		</tr>
<? if($INDEX%2==0)
	echo '<tr class="light_bg">';
   else
	echo '<tr class="gray_bg">';
?>
			<td class="border_left gray_border_btm"><?echo I18N("h","Firewall ");?><br /><input id="pubport_<?=$INDEX?>" type="text" size="6" maxlength="60" /><?drawlabel("<? echo 'pubport_'.$INDEX;?>");?></td>
			<td class="border_left gray_border_btm"><!--<?echo I18N("h","Protocol");?><br/>-->
			<br />	
				<select id="pubpro_<?=$INDEX?>">
				        <option value="TCP+UDP"><?echo I18N("h","All");?></option>
						<option value="TCP"><?echo I18N("h","TCP");?></option>
						<option value="UDP"><?echo I18N("h","UDP");?></option>
				</select>
			</td>
		</tr>
