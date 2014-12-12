<? include "/htdocs/webinc/body/draw_elements.php"; ?>
		<tr>
			<td rowspan="2" class="centered">
				<!-- the uid of VSVR -->
				<input type="hidden" id="uid_<?=$INDEX?>" value="">
				<input id="en_<?=$INDEX?>" type="checkbox" />
			</td>
			<td><?echo i18n("Name");?><br /><input id="dsc_<?=$INDEX?>" type="text" size="15" maxlength="15" /></td>
			<td class="bottom">
				<input type="button" value="<<" class="arrow" onclick="PAGE.OnClickAppArrow('<?=$INDEX?>');" modified="ignore" />
				<span id="span_app_<?=$INDEX?>"></span>
			</td>
			<td class="centered"><?echo i18n("Public Port");?><br /><input id="pubport_<?=$INDEX?>" type="text" size="4" maxlength="5" /></td>
			<td class="centered"><?echo i18n("Protocol");?><br />
				<select id="pro_<?=$INDEX?>" onchange="PAGE.OnClickProtocal('<?=$INDEX?>');">
					<option value="TCP">TCP</option>
					<option value="UDP">UDP</option>
					<option value="TCP+UDP"><?echo i18n("Both");?></option>
					<option value="Other"><?echo i18n("Other");?></option>
				</select>
			</td>
			<?
			if ($FEATURE_NOSCH != "1")
			{
				echo '<td class="centered">';
				echo i18n("Schedule").'<br />\n';
				DRAW_select_sch("sch_".$INDEX, i18n("Always"), "-1", "", 0, "narrow");
				echo '</td>\n';
			}
			?>
		</tr>
		<tr>
			<td><?echo i18n("IP Address");?><br />
				<input id="ip_<?=$INDEX?>" type="text" size="15" maxlength="15" />
			</td>
			<td class="bottom">
				<input type="button" value="<<" class="arrow" onclick="PAGE.OnClickPCArrow('<?=$INDEX?>');" modified="ignore" />
				<? DRAW_select_dhcpclist("LAN-1","pc_".$INDEX, i18n("Computer Name"), "",  "", "1", "broad"); ?>
			</td>
			<td class="centered"><?echo i18n("Private Port");?><br />
				<input id="priport_<?=$INDEX?>" type="text" size="4" maxlength="5" />
			</td>
			<td class="centered">&nbsp;<br />
				<input id="pronum_<?=$INDEX?>" type="text" size="4" maxlength="5" />
			</td>
			<?
			if ($FEATURE_INBOUNDFILTER == "1")
			{
				echo '<td class="centered">';
				echo i18n("Inbound Filter").'<br />\n';
				DRAW_select_inbfilter("inbfilter_".$INDEX, i18n("Allow All"), "-1", i18n("Deny All"), "denyall", "", 0, "narrow", "72px");
				echo '</td>\n';
			}
			?>
		</tr>

