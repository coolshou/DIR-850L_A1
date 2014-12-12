		<tr>
			<td rowspan="3" class="gray_bg gray_border_btm dent_padding border_2side">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td class="gray_bg gray_border_btm border_2side">
				<?echo I18N("h","Name");?><br>
				<input id="dsc_<?=$INDEX?>" type="text" size="20" maxlength="31" /><?drawlabel("dsc_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm border_2side">
				<?echo I18N("h","Queue ID");?><br>
				<select id="pri_<?=$INDEX?>">
					<option value="VO">1 - <?echo I18N("h","Highest");?></option>
					<option value="VI">2 - <?echo I18N("h","Higher");?></option>
					<option value="BG">3 - <?echo I18N("h","Normal");?></option>
					<option value="BE">4 - <?echo I18N("h","Best Effort");?></option>
				</select>
			</td>
			<td class="gray_bg gray_border_btm border_2side">
				<?echo I18N("h","Protocol");?><br>
				<input id="pro_<?=$INDEX?>" type="text" size=6 maxlength=6>
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="select_pro_<?=$INDEX?>" onclick="PAGE.OnChangeProt(<?=$INDEX?>)">
					<option value="ALL"><?echo I18N("h","ALL");?></option>
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
				</select><?drawlabel("pro_"+$INDEX);?>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg gray_border_btm dent_padding border_2side">
				<?echo I18N("h","Local IP Range");?><br>
				<input id="src_startip_<?=$INDEX?>" type="text" maxlength="15" size="27" />  to 
				<input id="src_endip_<?=$INDEX?>" type="text" maxlength="15" size="27" /><?drawlabel("src_startip_"+$INDEX);?><?drawlabel("src_endip_"+$INDEX);?>
			</td>
			<td rowspan="2" class="gray_bg gray_border_btm border_2side">
				<?echo I18N("h","Application Port");?><br>
				<input id="app_port_<?=$INDEX?>" type="text" size=15 maxlength=15 onkeyup="PAGE.OnChangeAppPortInput(<?=$INDEX?>)" />
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="select_app_port_<?=$INDEX?>" onclick="PAGE.OnChangeAppPort(<?=$INDEX?>)">
					<option value="ALL"><?echo I18N("h","ALL");?></option>
					<option value="YOUTUBE">YOUTUBE</option>
					<option value="VOICE">VOICE</option>
					<option value="HTTP_AUDIO">HTTP_AUDIO</option>
					<option value="HTTP_VIDEO">HTTP_VIDEO</option>
					<option value="HTTP_DOWNLOAD">HTTP_DOWNLOAD</option>
					<option value="HTTP">HTTP</option>
					<option value="FTP">FTP</option>
					<option value="P2P">P2P</option>
				</select><?drawlabel("app_port_"+$INDEX);?>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg gray_border_btm dent_padding border_2side">
				<?echo I18N("h","Remote IP Range");?><br>
				<input id="dst_startip_<?=$INDEX?>" type="text" maxlength="15" size="27" />  to 
				<input id="dst_endip_<?=$INDEX?>" type="text" maxlength="15" size="27" /><?drawlabel("dst_startip_"+$INDEX);?><?drawlabel("dst_endip_"+$INDEX);?>
			</td>
		</tr>
