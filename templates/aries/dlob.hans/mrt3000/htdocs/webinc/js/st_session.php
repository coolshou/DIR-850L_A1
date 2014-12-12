<? include "/htdocs/phplib/xnode.php"; ?>
<script type="text/javascript" charset="utf-8">
//<![CDATA[
/* The PAGE specific part of this page. */
function Page() {}
Page.prototype =
{
	services: "RUNTIME.INF.LAN-1,INET.WAN-1,RUNTIME.INF.WAN-1,INET.LAN-1,INET.LAN-2",
	OnLoad: function()
	{
		//if (!this.rgmode) BODY.DisableCfgElements(true);
	}, /* Things we need to do at the onload event of the body. */
	OnUnload: null, /* Things we need to do at the onunload event of the body. */
	
	/* Things we need to do at the submit callback.
	* Usually the BODY will handle callback of submit. */
	OnSubmitCallback: function (code, result) { return false; },
	
	/* Things we need to do with the configuration.
	 * Some implementation will have visual elements and hidden elements,
	 * both handle the same item. Synchronize is used to sync the visual to the hidden. */
	Synchronize: null,
	
	/* The page specific dirty check function. */
	IsDirty: null,
	
	/* The "services" will be requested by GetCFG(), then the InitValue() will be
	 * called to process the services XML document. */
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var infp = PXML.FindModule("RUNTIME.INF.LAN-1");
		infp += "/runtime/inf/inet/ipv4";
		this.ipaddr	= XG(infp+"/ipaddr");
		this.mask	= XG(infp+"/mask");
		this.OnClickRefresh();
		return true;
	},
	PreSubmit: function() { return null; },
	//////////////////////////////////////////////////
	ipaddr: null,
	mask: null,
	ctp: "/conntrack/message",
	OnClickRefresh: function()
	{
		OBJ("loading").style.display = "block";
		OBJ("loading").innerHTML  = "Now Loading...";
		//BODY.CleanTable("sess_list");

		//jim, for old style
		//TEMP_CleanTable("session_list");	 

		var ajaxObj = GetAjaxObj("SERVICE");
		
		AUTH.UpdateTimeout();

		ajaxObj.createRequest();
		ajaxObj.onCallback = function (xml)
		{			
			OBJ("loading").style.display = "none"; 
			OBJ("loading").innerHTML  = "";
			ajaxObj.release();
			PAGE.RefreshTable(xml);
		}
		ajaxObj.returnXml=false;
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		ajaxObj.sendRequest("session_act.php", "1");
	},
	Check_protocol: function(conntrack)
	{
		if(conntrack.indexOf("tcp")>0)
		{
			return "TCP";
		}
		else if(conntrack.indexOf("udp")>0)
		{
			return "UDP";
		}
		else if(conntrack.indexOf("udp")>0)
		{
			return "ICMP";
		}
		else
		{
			return "UNKNOWN";
		}	
	},
	Cut_unnessary_string: function(ip_str,port_str)
	{
		var ip_index=null;
		var port_index=null;
		
		if(ip_str!=""&&port_str!="")
		{
			ip_index=ip_str.indexOf("=");
			ip_index++;
			port_index=port_str.indexOf("=");
			port_index++;
			return ip_str.substring(ip_index)+":"+port_str.substring(port_index);
		}
		else if(port_str=="")
		{
			ip_index=ip_str.indexOf("=");
			ip_index++;
			return ip_str.substring(ip_index);
		}		
	},
	GetWanIp:function()
	{
		var ip_addr="0.0.0.0";
		var rwan = PXML.FindModule("RUNTIME.INF.WAN-1");
		var wan	= PXML.FindModule("INET.WAN-1");
		var waninetuid = XG(wan+"/inf/inet");
		var rwaninetp = GPBT(rwan+"/runtime/inf", "inet", "uid", waninetuid, false);

		if ( (XG(rwaninetp+"/addrtype") == "ipv4") )
		{
			 ip_addr = XG(rwaninetp+"/ipv4/ipaddr");
		}
		else if ( (XG(rwaninetp+"/addrtype") == "ppp4") )
		{
			ip_addr = XG(rwaninetp+"/ppp4/local");
		}
		return ip_addr;
	},
	GetLanIP:function()
	{
		var ip_addr="0.0.0.0";
		var lan	= PXML.FindModule("INET.LAN-1");
		var inetuid = XG  (lan+"/inf/inet");
		var inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);
		if (!inetp)
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","InitLAN() ERROR!!!");
			return false;
		}
		if (XG  (inetp+"/addrtype") == "ipv4")
		{
			var b = inetp+"/ipv4";
			ip_addr=XG(b+"/ipaddr");
		}
		return ip_addr;
	},
	GetGuestZoneIP:function()
	{
		var ip_addr="0.0.0.0";
		var lan	= PXML.FindModule("INET.LAN-2");
		var inetuid = XG  (lan+"/inf/inet");
		var inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);
		if (!inetp)
		{
			BODY.ShowMessage("<?echo I18N("j","Error");?>","InitLAN() ERROR!!!");
			return false;
		}
		if (XG  (inetp+"/addrtype") == "ipv4")
		{
			var b = inetp+"/ipv4";
			ip_addr=XG(b+"/ipaddr");
		}
		//alert("ip_addr="+ip_addr);
		return ip_addr;
	},
	FindDir:function(wan_ip,dst_ip)
	{
		var direction="OUT";
		if(dst_ip.indexOf(wan_ip)>0)
		{
			direction="IN";
		}		
		return direction;
	},
	TCP_state:function(conntrack_stat)
	{
		switch (conntrack_stat)
		{
			case "NONE":
				return "NO";
			case "SYN_SENT":
			case "SYN_RECV":
			case "SYN_SENT2":
				return "SS";
			case "ESTABLISHED":
				return "EST	";
			case "FIN_WAIT":
				return "FW";
			case "CLOSE_WAIT":
				return "CW";
			case "LAST_ACK":
				return "LA";
			case "TIME_WAIT":
				return "TW";
			case "CLOSE":
				return "CL";
			default:
				return "UNKNOWN";
		}
	},
	RefreshTable: function(xml)
	{
		var wan_ip=null;
		var lan_ip=null;
		var guest_zone_ip=null;
		var table_index=1;
		var conntrack=new Array(); 
		var localip_port=null;
		var nat_port=null;
		var dstip_port=null;
		var pro=null;
		var state=null;
		var dir=null;
		var timeout=null;
		var conntrack_token=new Array(); 
				
		var lan_ip_array = new Array();		
		var lan_cnt = 0;
		var tcp_array = new Array();
		var tcp_cnt = 0;		
		var udp_array = new Array();
		var udp_cnt = 0;				
		
		var re_localip = null;
		var re_localip_port = null;
		var re_dstip_port = null;
			
		wan_ip=PAGE.GetWanIp();
		lan_ip=PAGE.GetLanIP();
		guest_zone_ip=PAGE.GetGuestZoneIP();		
		conntrack=xml.split("\n");												
		//alert("wan_ip="+wan_ip+", lan_ip="+lan_ip+", guest_zone_ip="+guest_zone_ip);
		for(i=0;i<conntrack.length;i++)
		{
			if(conntrack[i]==null) break;								
			//marco,find out ipv4 and the stat is ASSURED for udp or establish for tcp
			if(conntrack[i].indexOf("ipv4")!=-1) /*&& (conntrack[i].indexOf("UNREPLIED")!=-1 || conntrack[i].indexOf("ESTABLISHED")!=-1))*/
			{			 	
			 	pro=PAGE.Check_protocol(conntrack[i]);
				
			 	var tmp_string=conntrack[i].substring(21);
			 	conntrack_token=tmp_string.split(" ");
			 	
			 	timeout=conntrack_token[1];
			 	
			 	if(pro=="TCP")
				{
					if(conntrack[i].indexOf("ESTABLISHED")<0) continue;         
					if(conntrack[i].indexOf("UNREPLIED")>0) continue;
					state=PAGE.TCP_state(conntrack_token[2]);
					dir=PAGE.FindDir(wan_ip,conntrack_token[4]);
					localip_port=PAGE.Cut_unnessary_string(conntrack_token[3],conntrack_token[5]);
			 		dstip_port=PAGE.Cut_unnessary_string(conntrack_token[4],conntrack_token[6]);
			 		nat_port=PAGE.Cut_unnessary_string(conntrack_token[12],"");
			 					 		
			 		/* reverse*/
			 		re_localip = conntrack_token[9].substr(4, conntrack_token[9].length);
			 		re_localip_port=PAGE.Cut_unnessary_string(conntrack_token[9],conntrack_token[11]);
			 		re_dstip_port=PAGE.Cut_unnessary_string(conntrack_token[10],conntrack_token[12]);			 					 					 				 			
				}
				else if(pro == "UDP")
				{
					
			 		if(conntrack[i].indexOf("UNREPLIED")>0) continue;		 							
					state="--";
					dir=PAGE.FindDir(wan_ip,conntrack_token[3]);
					localip_port=PAGE.Cut_unnessary_string(conntrack_token[2],conntrack_token[4]);
			 		dstip_port=PAGE.Cut_unnessary_string(conntrack_token[3],conntrack_token[5]);
			 		nat_port=PAGE.Cut_unnessary_string(conntrack_token[11],"");
			 		
			 		/* reverse*/			 					 					 					 					 					 					 		
			 		re_localip = conntrack_token[8].substr(4, conntrack_token[8].length);
			 		re_localip_port=PAGE.Cut_unnessary_string(conntrack_token[8],conntrack_token[10]);
			 		re_dstip_port=PAGE.Cut_unnessary_string(conntrack_token[9],conntrack_token[11]);			 					 			 			
				}
				else continue;
				
				
				
				if( (dstip_port.indexOf(lan_ip) <0) && (dstip_port.indexOf(guest_zone_ip) <0) )
				{					
					var lanip_flag = 0;					
					var session_ip = "";
					
					session_ip = this.GetSessionIP(localip_port, nat_port);

					if(session_ip == wan_ip) continue;
					for(var j=0; j<lan_ip_array.length; j++)
					{													
						if(lan_ip_array[j] == session_ip)
						{
							lanip_flag = 1;							
						}	
					}	
					
					if(lanip_flag == 0)
					{
						var lanip_p2 = lan_ip.substr(0,7);						
						var glanip_p2 = guest_zone_ip.substr(0,7);
						
						//for virtual server, upnp, port forwarding....
						if((session_ip.indexOf(lanip_p2) <0) && (session_ip.indexOf(glanip_p2) <0))
						{							
							session_ip = re_localip;
							localip_port = 	re_localip_port;
							dstip_port = re_dstip_port;									
							//alert(">>>>>"+lanip_p2+" ,"+session_ip+" ,"+localip_port+" ,"+dstip_port);
							
							for(var j=0; j<lan_ip_array.length; j++)
							{													
								if(lan_ip_array[j] == session_ip)
								{
									lanip_flag = 1;							
								}	
							}
							
							if(lanip_flag == 0)
							{
								lan_ip_array[lan_cnt] = session_ip;						
								lan_cnt++;		
							}																
						}						
						else
						{						
							lan_ip_array[lan_cnt] = session_ip;						
							lan_cnt++;						
						}	
					}
				
					
					var data = [localip_port, nat_port, dstip_port, pro, state, dir, timeout];															
					
					if(pro == "TCP")
					{     						
						tcp_array[tcp_cnt] = data;						
						tcp_cnt++;	
					}
						
					if(pro == "UDP")     
					{
						udp_array[udp_cnt] = data;
						udp_cnt++;
					}
          
			//jim, for old style
			//var type = ["text", "text", "text", "text", "text", "text", "text"];
			//TEMP_InjectTable("session_list", null, data, type);
         
          
				}
			}
		}
		
		/* Draw session table */
		{
			var str = "";
			var str2 = "";
			var ip = "";
			var data_id = "";
			var tcpbyip = new Array();
			var udpbyip = new Array();				
			var tcpbyip_cnt = 0;
			var udpbyip_cnt = 0;

			str2 = "<table border='0' cellpadding='0' cellspacing='0' align='center'>"
				+"<tr>"
				+"<td class='td_right'><strong><?echo I18N("j","TCP Sessions");?> : </strong></td>"
				+"<td>"+tcp_cnt+"</td>"
				+"</tr>"
				+"<tr>"
				+"<td class='td_right'><strong><?echo I18N("j","UDP Sessions");?> : </strong></td>"
				+"<td>"+udp_cnt+"</td>"
				+"</tr>"
				+"<tr>"
				+"<td class='td_right'><strong><?echo I18N("j","Total");?> : </strong></td>"
				+"<td>"+(tcp_cnt+udp_cnt)+"</td>"
				+"</tr>"
				+ "</table>";

			OBJ("sess_total").innerHTML = str2;
							
			str = "<table border='0' cellpadding='0' cellspacing='0' class='status_report_session'>"
				+"<tr>"		
				+"<th width='40%' class='silver_bg dent_padding'><?echo I18N("j","IP");?></th>"
				+"<th width='30%' class='silver_bg'><?echo I18N("j","TCP Count");?></th>"
				+"<th width='30%' class='silver_bg'><?echo I18N("j","UDP Count");?></th>"			
				+"</tr>";
																			
			for(var i =0; i<lan_cnt; i++)
			{
										
				data_id = "data"+i;			
				ip = lan_ip_array[i];
							
				tcpbyip = this.GetProtoArrayByIP(tcp_array,   tcp_cnt,   ip);
				udpbyip = this.GetProtoArrayByIP(udp_array,   udp_cnt,   ip);			
				if(tcpbyip!=null) tcpbyip_cnt = tcpbyip.length;
				if(udpbyip!=null) udpbyip_cnt = udpbyip.length;			
							
				str = str+"<tr>"
				+"<td><a href='#' onclick=PAGE.ShowDetail('"+data_id+"')><span class='sess_content'>"+ip+"</span></a></td><td><span class='sess_content'>"+tcpbyip_cnt+"</span></td><td><span class='sess_content'>"+udpbyip_cnt+"</span></td></tr>"			
				+"<tr><td colspan=5><div id="+data_id+" style=display:none>"
				+"<table width='90%' align=right border='0' cellpadding='0' cellspacing='0' class='status_report_session'><tr>"
				+"<th width='10%'>Protocol</th><th width='10%'>NAT</th><th width='20%'>Internet</th>"
				+"<th width='10%'>State</th><th width='10%'>Dir</th><th width='10%'>Time Out</th></tr>"
				+this.SetProtoRowData(tcpbyip,   tcpbyip_cnt)
				+this.SetProtoRowData(udpbyip,   udpbyip_cnt)			
				+"</table></div></td></tr>";			
			}
			str = str + "</table>";
			OBJ("sess_list").innerHTML = str;	
		}									
	},
	ShowDetail: function(id)
	{
		var item = document.getElementById(id);
		if(item.style.display == "none")
		{
			item.style.display = "";
		}
		else
		{
			item.style.display = "none";
		}
	},
	GetSessionIP: function(Local, Nat)
	{				
		var ip ="", len;		
				
		len = Local.length-Nat.length-1;
		ip = Local.substr(0, len);				
		if(ip.indexOf(":")>0)
		{
			//alert("getsessionip="+Local+", ip="+ip);	
			ip = ip.substr(0, len-1);
		}					
		return ip;
	},
	SetProtoRowData: function(proto_data, proto_cnt)
	{		 
		var target, str="", proto_title="";
		var data = new Array();
				
		if(proto_cnt==0) return "";
															
		for(var i=0; i<proto_cnt; i++)
		{							
			data = proto_data[i];
			if(i == 0)
			{								
				proto_title = "<td rowspan="+proto_cnt+"><span class='sess_content'>"+data[3]+"</span></td>";
			}
			else
			{
				proto_title = "";
			}
			
			str=str+"<tr>"+proto_title + 
				"<td><span class='sess_content'>"+data[1]+"</span></td>" +
				"<td><span class='sess_content'>"+data[2]+"</span></td>" +
				"<td><span class='sess_content'>"+data[4]+"</span></td>" +
				"<td><span class='sess_content'>"+data[5]+"</span></td>" +
				"<td><span class='sess_content'>"+data[6]+"</span></td>" +
				"</tr>";			
		}
		
		return str;	
	},
	GetProtoArrayByIP: function(proto_data, proto_cnt, ip)
	{
		var target, cnt;
		var data = new Array(), ret_data = new Array();
						
		if(proto_cnt==0) return null;		
											
		for(var i=0, j=0; i<proto_cnt; i++)
		{			
			data = proto_data[i];						
			if(data == null) break;				
							
			if(data[0].indexOf(ip)!=-1)
			{					
				ret_data[j++] = data;													
			}			
		}
		
		return ret_data;		
	},
	//////////////////////////////////////////////////
	/* Don't remove dummy or add function after dummy, Its using for browser compatibility */
	dummy: null
}

//]]>
</script>
