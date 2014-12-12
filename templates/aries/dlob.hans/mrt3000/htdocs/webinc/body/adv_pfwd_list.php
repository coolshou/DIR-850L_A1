<? include "/htdocs/webinc/body/draw_elements.php"; ?>
		<tr>
			<td rowspan="2" class="gray_bg gray_border_btm dent_padding border_left" style="vertical-align: middle;">
				<!-- the uid of PFWD -->
				<input type="hidden" id="uid_<?=$INDEX?>" value="">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td class="gray_bg gray_border_btm"><?echo I18N("h","Name");?><br /><input id="dsc_<?=$INDEX?>" type="text" size="20" maxlength="15" /><?drawlabel("dsc_"+$INDEX);?></td>
			<td class="gray_bg gray_border_btm">
				<br>
				<input type="button" value="<<" class="arrow" onClick="OnClickAppArrow('<?=$INDEX?>');" />
				<span id="span_app_<?=$INDEX?>"></span>
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","TCP");?><br />
				<input id="port_tcp_<?=$INDEX?>" type="text" size="15" /><?drawlabel("port_tcp_"+$INDEX);?>
			</td>
			<?
			if ($FEATURE_NOSCH != "1")
			{
				echo '<td class="gray_bg gray_border_btm border_right">\n'.I18N("h","Schedule").'<br />\n';
				DRAW_select_sch("sch_".$INDEX, I18N("h","Always"), "-1", "", "0", "narrow");
				echo '</td>\n';
			}
			?>
		</tr>
		<tr>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","IP Address");?><br />
				<input id="ip_<?=$INDEX?>" type="text" size="20" maxlength="15" /><?drawlabel("ip_"+$INDEX);?>
			</td>
			<td class="gray_bg gray_border_btm">
				<br>
				<input type="button" value="<<" class="arrow" onClick="OnClickPCArrow('<?=$INDEX?>');" />
				<? DRAW_select_dhcpclist("LAN-1","pc_".$INDEX, I18N("h","Computer Name"), "",  "", "1", "broad"); ?>
			</td>
			<td class="gray_bg gray_border_btm">
				<?echo I18N("h","UDP");?><br />
				<input id="port_udp_<?=$INDEX?>" type="text" size="15" /><?drawlabel("port_udp_"+$INDEX);?> 
			</td>
			<?
			if ($FEATURE_INBOUNDFILTER == "1")
			{
				echo '<td class="gray_bg gray_border_btm border_right">';
				echo I18N("h","Inbound Filter").'<br />\n';
				DRAW_select_inbfilter("inbfilter_".$INDEX, I18N("h","Allow All"), "-1", I18N("h","Deny All"), "denyall", "", 0, "narrow");
				echo '</td>\n';
			}
			?>
		</tr>

