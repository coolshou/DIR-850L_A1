<? include "/htdocs/webinc/body/draw_elements.php"; ?>
		<tr>
			<td rowspan="2" class="gray_bg gray_border_btm dent_padding border_left" style="vertical-align: middle;">
				<!-- the uid of VSVR -->
				<input type="hidden" id="uid_<?=$INDEX?>" value="">
				<input id="en_<?=$INDEX?>" type="checkbox" /><?drawlabel("en_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm"><?echo I18N("h","Name");?><br /><input id="dsc_<?=$INDEX?>" type="text" size="15" maxlength="15" /><?drawlabel("dsc_"+$INDEX);?></td>
			<td class="gray_bg gray_border_btm">
				<br>				
				<input type="button" value="<<" class="arrow" onclick="PAGE.OnClickAppArrow('<?=$INDEX?>');" modified="ignore" />
				<span id="span_app_<?=$INDEX?>"></span>
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","Public Port");?><br />
				<input id="pubport_<?=$INDEX?>" type="text" size="4" maxlength="5" /><?drawlabel("pubport_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm"><?echo I18N("h","Protocol");?><br />
				<select id="pro_<?=$INDEX?>" onchange="PAGE.OnClickProtocal('<?=$INDEX?>');">
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
					<option value="TCP+UDP"><?echo I18N("h","Both");?></option>
					<option value="Other"><?echo I18N("h","Other");?></option>
				</select>
			</td>	
		</tr>
		
		<tr>
			<td class="gray_bg gray_border_btm"><?echo I18N("h","IP Address");?>
				<br />
				<input id="ip_<?=$INDEX?>" type="text" size="15" maxlength="15" /><?drawlabel("ip_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<br />
				<input type="button" value="<<" class="arrow" onclick="PAGE.OnClickPCArrow('<?=$INDEX?>');" modified="ignore" />
				<? DRAW_select_dhcpclist("LAN-1","pc_".$INDEX, I18N("h","Computer Name"), "",  "", "1", "broad"); ?>
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","Private Port");?>
				<br />
				<input id="priport_<?=$INDEX?>" type="text" size="4" maxlength="5" /><?drawlabel("priport_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<br />
				<input id="pronum_<?=$INDEX?>" type="text" size="4" maxlength="5" /><?drawlabel("pronum_"+$INDEX);?>
			</td>
		</tr>

