<tr>
	<td class="centered" rowspan="2">
		<input type=checkbox id="<? echo 'enable_'.$ROUTING_INDEX;?>">
		<input type="hidden" id="<?echo "uid_".$ROUTING_INDEX;?>" value="<?=$ROUTING_INDEX?>"/>	
	</td>
	<td><?echo i18n("Name");?><br/><input type=text id="<? echo 'name_'.$ROUTING_INDEX;?>" size=16 maxlength=15></td>
	<td><?echo i18n("Destination IP");?><br/><input type=text id="<? echo 'dstip_'.$ROUTING_INDEX;?>" size=16 maxlength=15></td>
	<td class="centered" rowspan="2"><input type=text id="<? echo 'metric_'.$ROUTING_INDEX;?>" size=3 maxlength=3></td>
	
	<td class="centered" rowspan="2">
		<select id="<?echo "inf_".$ROUTING_INDEX;?>" style="width: 150px;">
			
<?		
			include "/htdocs/phplib/xnode.php";
			include "/htdocs/phplib/inf.php";
			
			$wan_lowerlayer = "";
			$russia_ppp = 0;
			
			$i=1;
			while ($i>0 && $i<4)
			{
				$ifname = "WAN-".$i;
				$ifpath = XNODE_getpathbytarget("runtime", "inf", "uid", $ifname, 0);
				$ifpath2 = XNODE_getpathbytarget("", "inf", "uid", $ifname, 0);
				
				if ($ifpath == "") { $i++; continue; }
				
				$inet_addrtype = query($ifpath."/inet/addrtype");
				
				$lowerlayer = query($ifpath2."/lowerlayer");
				if($lowerlayer != "") 
				{ 
					$wan_lowerlayer = $lowerlayer; 
					if(get("", INF_getinfpath($wan_lowerlayer)."/active")=="1" && 
						get("", INF_getinfpath($wan_lowerlayer)."/nat")=="NAT-1")
					{$russia_ppp = 1;}
				}
				
				$str = "";
				if($wan_lowerlayer == $ifname) { $str = "Physical"; }
				//for normal PPP mode, we skip the physical interface, since we don't do NAT in this physical interface.
				//if want to set routing to this interface, then we should do NAT in this physical.
				//if($ifname == $wan_lowerlayer) { $i++; continue; }
				
				if($inet_addrtype == "ipv4" && $wan_lowerlayer == "") //Static or DHCP
				{
					$ip = query($ifpath."/inet/ipv4/ipaddr");
					$show_ifname = "WAN ".$str." (".$ip.")";
				}
				else if($inet_addrtype == "ipv4" && $russia_ppp == 1) //Physical layer for Russia PPP mode.
				{
					$ip = query($ifpath."/inet/ipv4/ipaddr");
					$show_ifname = "WAN ".$str." (".$ip.")";
				}				
				else if($inet_addrtype == "ppp4")
				{
					$ip = query($ifpath."/inet/ppp4/local");
					$show_ifname = "WAN ".$str. "(".$ip.")";
				}
				else
				{
					$i++; 
					continue;
				}
					echo '<option value="'.$ifname.'">'.$show_ifname.'</option>';
					$i++;
			}
			
			/*
			//$i=1;
			$i=3;
			while ($i>0 && $i<4)
			{
				$ifname = "LAN-".$i;
				$ifpath = XNODE_getpathbytarget("runtime", "inf", "uid", $ifname, 0);
				//if ($ifpath == "") { $i++; continue; }
				if ($ifpath == "") { $i--; continue; }
				$inet_uid = query($ifpath."/inet/uid");
				$inet_path = XNODE_getpathbytarget("inet", "entry", "uid", $inet_uid, 0);
				$inet_addrtype = query($inet_path."/addrtype");
				
				//$show_ifname = $ifname;
		        if($inet_addrtype == "ipv4")
		        {
					//$show_ifname = $ifname."(".query($inet_path.'/ipv4/ipaddr').")";
					$show_ifname = $ifname;
		        } else
		        {
		        	//$i++;
		        	$i--;
		        	continue;	
		        }
		        	
				echo '<option value="'.$ifname.'">'.$show_ifname.'</option>';
				//$i++;
				$i--;
			}
			*/
?>
		</select>
	</td>
</tr>
<tr>
	<td><?echo i18n("Netmask");?><br/><input type=text id="<? echo 'netmask_'.$ROUTING_INDEX;?>" size=16 maxlength=15></td>
	<td><?echo i18n("Gateway");?><br/><input type=text id="<? echo 'gateway_'.$ROUTING_INDEX;?>" size=16 maxlength=15></td>
</tr>

