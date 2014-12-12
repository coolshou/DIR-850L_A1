<? include "/htdocs/phplib/xnode.php"; ?>
<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "RUNTIME.INF.LAN-1,INET.WAN-1,RUNTIME.INF.WAN-1,INET.LAN-1,INET.LAN-2",
	OnLoad: function()
	{
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	InitValue: function(xml)
	{
		if (!this.rgmode)
		{
			OBJ("sess_list").innerHTML = '<? echo I18N("j","Now is an Access Point Mode."); ?>';
			return;
		}
		PXML.doc = xml;
		var infp = PXML.FindModule("RUNTIME.INF.LAN-1");
		infp += "/runtime/inf/inet/ipv4";
		this.ipaddr	= XG(infp+"/ipaddr");
		this.mask	= XG(infp+"/mask");
		this.OnClickRefresh();
		return true;
	},
	PreSubmit: function() { return null; },
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	ipaddr: null,
	mask: null,
	ctp: "/conntrack/message",
	OnClickRefresh: function()
	{
		OBJ("loading").style.display = "block";
		OBJ("loading").innerHTML  = "Now Loading...";
		//BODY.CleanTable("sess_list"); 
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
			BODY.ShowAlert("InitLAN() ERROR!!!");
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
			BODY.ShowAlert("InitLAN() ERROR!!!");
			return false;
		}
		if (XG  (inetp+"/addrtype") == "ipv4")
		{
			var b = inetp+"/ipv4";
			ip_addr=XG(b+"/ipaddr");
		}
		
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
			
		var localip_index = new Array();		//0:localip 1:re_localip
		var localip_cnt = 0;
		var localport_index = new Array();	//0:localport 1:re_localport
		var localport_cnt = 0;
		var dstip_index = new Array();			//0:dstip 1:re_dstip
		var dstip_cnt = 0;
		var dstport_index = new Array();		//0:dstport 1:re_dstport
		var dstport_cnt = 0;

			
		wan_ip=PAGE.GetWanIp();
		lan_ip=PAGE.GetLanIP();
		guest_zone_ip=PAGE.GetGuestZoneIP();		
		conntrack=xml.split("\n");												
		
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
					state=PAGE.TCP_state(conntrack_token[2]);
				}
				else if(pro == "UDP")
				{
			 		if(conntrack[i].indexOf("UNREPLIED")>0) continue;		 							
					state="--";
				}
				else continue;
				
				/*	using string "src", "dst", "sport" and "dport" to find their conntrack_token's index
				*		src = localip and re_localip				(localip_index[0]:localip,			localip_index[1]:re_localip)
				*		sport = localport and re_localport	(localport_index[0]:localport,	localport_index[1]:re_localport)
				*		dst = dstip and re_dstip						(dstip_index[0]:dstip,					dstip_index[1]:re_dstip)
				*		dport = dstport and re_dstport			(dstport_index[0]:dstport,			dstport_index[1]:re_dstport)
				*/
				PAGE.FindIpPortIndexByStr(conntrack_token, "src", localip_index, localip_cnt);
				PAGE.FindIpPortIndexByStr(conntrack_token, "sport", localport_index, localport_cnt);
				PAGE.FindIpPortIndexByStr(conntrack_token, "dst", dstip_index, dstip_cnt);
				PAGE.FindIpPortIndexByStr(conntrack_token, "dport", dstport_index, dstport_cnt);
									
				dir=PAGE.FindDir(wan_ip,conntrack_token[dstip_index[0]]);
				localip_port=PAGE.Cut_unnessary_string(conntrack_token[localip_index[0]],conntrack_token[localport_index[0]]);
			 	dstip_port=PAGE.Cut_unnessary_string(conntrack_token[dstip_index[0]],conntrack_token[dstport_index[0]]);
			 	nat_port=PAGE.Cut_unnessary_string(conntrack_token[dstport_index[1]],"");
			 	
			 	/* reverse*/			 					 					 					 					 					 					 		
			 	re_localip = conntrack_token[localip_index[1]].substr(4, conntrack_token[localip_index[1]].length);
			 	re_localip = PAGE.RemoveColon(re_localip);
			 	re_localip_port=PAGE.Cut_unnessary_string(conntrack_token[localip_index[1]],conntrack_token[localport_index[1]]);
			 	re_dstip_port=PAGE.Cut_unnessary_string(conntrack_token[dstip_index[1]],conntrack_token[dstport_index[1]]);	
				
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
				}
			}
		}
		
		/* Draw session table */
		{
			var str = "";
			var ip = "";
			var data_id = "";
			var tcpbyip = new Array();
			var udpbyip = new Array();				
			var tcpbyip_cnt = 0;
			var udpbyip_cnt = 0;
							
			str = "<table class=general>"
				+"<tr>"		
				+"<th width='40%'><?echo i18n("IP");?></th>"
				+"<th width='30%'><?echo i18n("TCP Count");?></th>"
				+"<th width='30%'><?echo i18n("UDP Count");?></th>"			
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
				+"<td><a href='#' onclick=PAGE.ShowDetail('"+data_id+"')>"+ip+"</a></td><td>"+tcpbyip_cnt+"</td><td>"+udpbyip_cnt+"</td></tr>"			
				+"<tr><td colspan=5><div id="+data_id+" style=display:none>"
				+"<table width='90%' align=right><tr>"
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
		ip = PAGE.RemoveColon(ip);				
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
				proto_title = "<td rowspan="+proto_cnt+">"+data[3]+"</td>";
			}
			else
			{
				proto_title = "";
			}
			
			str=str+"<tr>"+proto_title+"<td>"+data[1]+"</td>"
			+"<td>"+data[2]+"</td><td>"+data[4]+"</td>"
			+"<td>"+data[5]+"</td><td>"+data[6]+"</td>"
			+"</tr>";			
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
	RemoveColon: function(ip)
	{
		while( ip.charAt(ip.length-1) == ":" )
		{
			ip = ip.substr(0,ip.length-2);
		}	
		return ip;
	},
	//using string "src", "dst", "sport" and "dport" to find their conntrack_token's index
	FindIpPortIndexByStr: function (token_arr, str, index_arr, index_cnt)
	{
		for(j=0;j<token_arr.length;j++)
		{
			if (token_arr[j].indexOf(str)!=-1)
			{
				index_arr[index_cnt] = j;
			 	index_cnt++;
			}
		}
		return index_arr;
	}
}
</script>
