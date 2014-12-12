		<tr>
			<td rowspan="2" class="gray_bg gray_border_btm border_left" style="vertical-align: middle;">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","Name");?>
				<input id="dsc_<?=$INDEX?>" type="text" size="8" maxlength="31" /><?drawlabel("dsc_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<select id="src_inf_<?=$INDEX?>">
					<option value=""><?echo I18N("h","Source");?></option>
					<option value="LAN-1">LAN</option>
					<option value="WAN-1">WAN</option>
				</select>
				<?drawlabel("src_inf_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<input id="src_startip_<?=$INDEX?>" type="text" maxlength="15" size="16" />~
				<input id="src_endip_<?=$INDEX?>" type="text" maxlength="15" size="16" />
				<br><?drawlabel("src_startip_"+$INDEX);?><?drawlabel("src_endip_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","Protocol");?><br>
				<select id="pro_<?=$INDEX?>" onchange="PAGE.OnChangeProt(<?=$INDEX?>)">
					<option value="ALL"><?echo I18N("h","All");?></option>
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
					<option value="ICMP">ICMP</option>
				</select>
			</td>
			<?
			if ($FEATURE_NOSCH!="1")
			{
				echo '<td class="gray_bg gray_border_btm border_right">\n';
				DRAW_select_sch("sch_".$INDEX, I18N("h","Always"), "-1", "", 0, "narrow");
				echo '<br>\n';
				echo '<input type="button" id=sch_'.$INDEX.'_btn value="'.I18N("h","New Schedule").'"'.
					 ' onclick="javascript:self.location.href=\'tools_sch.php\'">\n';
				echo '</td>\n';
			}
			?>
		</tr>
		<tr>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","Action");?>
				<select id="action_<?=$INDEX?>">
					<option value="ACCEPT"><?echo I18N("h","Allow");?></option>
					<option value="DROP"><?echo I18N("h","Deny");?></option>
				</select>
			</td>
			<td class="gray_bg gray_border_btm">
				<select id="dst_inf_<?=$INDEX?>">
					<option value=""><?echo I18N("h","Dest");?></option>
					<option value="LAN-1">LAN</option>
					<option value="WAN-1">WAN</option>
				</select>
				<?drawlabel("dst_inf_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<input id="dst_startip_<?=$INDEX?>" type="text" maxlength="15" size="16" />~
				<input id="dst_endip_<?=$INDEX?>" type="text" maxlength="15" size="16" />
				<br><?drawlabel("dst_startip_"+$INDEX);?><?drawlabel("dst_endip_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm <?if ($FEATURE_NOSCH=="0"){echo 'border_right';}?>"<?if ($FEATURE_NOSCH=="0"){echo ' colspan="2"';}?>>
				<?echo I18N("h","Port Range");?><br>
				<input id="dst_startport_<?=$INDEX?>" type="text"  maxlength="5" size="6"/>~
				<input id="dst_endport_<?=$INDEX?>" type="text" maxlength="5" size="6" /><?drawlabel("dst_startport_"+$INDEX);?><?drawlabel("dst_endport_"+$INDEX);?>
			</td>
		</tr>
