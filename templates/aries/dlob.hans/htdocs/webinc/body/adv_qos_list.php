		<tr>
			<td rowspan="3" class="centered">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td>
				<?echo i18n("Name");?><br>
				<input id="dsc_<?=$INDEX?>" type="text" size="20" maxlength="31" />
			</td>
			<td>
				<?echo i18n("Queue ID");?><br>
				<select id="pri_<?=$INDEX?>">
					<option value="VO"><?echo i18n("1 - Highest");?></option>
					<option value="VI"><?echo i18n("2 - Higher");?></option>
					<option value="BG"><?echo i18n("3 - Normal");?></option>
					<option value="BE"><?echo i18n("4 - Best Effort");?></option>
				</select>
			</td>
			<td>
				<?echo i18n("Protocol");?><br>
				<input id="pro_<?=$INDEX?>" type="text" size=6 maxlength=6>
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="select_pro_<?=$INDEX?>" onclick="PAGE.OnChangeProt(<?=$INDEX?>)">
					<option value="ALL"><?echo i18n("ALL");?></option>
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
				</select>
			</td>			
		</tr>
		<tr>
			<td colspan="2">
				<?echo i18n("Local IP Range");?><br>
				<input id="src_startip_<?=$INDEX?>" type="text" maxlength="15" size="27" />  to 
				<input id="src_endip_<?=$INDEX?>" type="text" maxlength="15" size="27" />
			</td>
			<td rowspan="2">
				<?echo i18n("Application Port");?><br>
				<input id="app_port_<?=$INDEX?>" type="text" size=15 maxlength=15 onkeyup="PAGE.OnChangeAppPortInput(<?=$INDEX?>)" />
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="select_app_port_<?=$INDEX?>" onclick="PAGE.OnChangeAppPort(<?=$INDEX?>)">
					<option value="ALL"><?echo i18n("ALL");?></option>
					<option value="YOUTUBE">YOUTUBE</option>
					<option value="VOICE">VOICE</option>
					<option value="HTTP_AUDIO">HTTP_AUDIO</option>
					<option value="HTTP_VIDEO">HTTP_VIDEO</option>
					<option value="HTTP_DOWNLOAD">HTTP_DOWNLOAD</option>
					<option value="HTTP">HTTP</option>
					<option value="FTP">FTP</option>
					<option value="P2P">P2P</option>
				</select>
			</td>
		</tr>
		<tr>
			<td colspan="2">
				<?echo i18n("Remote IP Range");?><br>
				<input id="dst_startip_<?=$INDEX?>" type="text" maxlength="15" size="27" />  to 
				<input id="dst_endip_<?=$INDEX?>" type="text" maxlength="15" size="27" />
			</td>
		</tr>
				