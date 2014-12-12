<?include "/htdocs/phplib/inet.php";?>
<?include "/htdocs/phplib/inf.php";?>
<style>
/* The CSS is only for this page.
 * Notice:
 *  If the items are few, we put them here,
 *  If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
select.broad	{ width: 110px; }
select.narrow	{ width: 65px; }
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "DEVICE.HOSTNAME,INET.LAN-1,DHCPS4.LAN-1,DHCPS4.LAN-2,RUNTIME.INF.LAN-1,URLCTRL,WAN" + "<? if($FEATURE_PARENTALCTRL=='1') echo ',OPENDNS4';?>",
	OnLoad: function()
	{
		SetDelayTime(500);	//add delay for event updatelease finished
		BODY.CleanTable("reserves_list");
		BODY.CleanTable("leases_list");
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			if (this.ipdirty)
			{
				Service("REBOOT", OBJ("ipaddr").value);								
			}
			else
			{
				BODY.OnReload();
			}
			break;
		case "BUSY":
			BODY.ShowAlert("<?echo i18n("Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			if (result.Get("/hedwig/result")=="FAILED")
			{
				FocusObj(result);
				BODY.ShowAlert(result.Get("/hedwig/message"));
			}
			break;
		case "PIGWIDGEON":
			BODY.ShowAlert(result.Get("/pigwidgeon/message"));
			break;
		}
		return true;
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		if (!this.InitHostname()) return false;
		if (!this.InitLAN()) return false;
		if (!this.InitDHCPS()) return false;
		this.InitNetBios();
		return true;
	},
	PreSubmit: function()
	{
		if (!this.PreHostname()) return null;
		if (!this.PreLAN()) return null;
		if (!this.PreDHCPS()) return null;
		if (!this.PreNetBios()) return null;
		PXML.IgnoreModule("DEVICE.LAYOUT");
		PXML.IgnoreModule("RUNTIME.INF.LAN-1");
		return PXML.doc;
	},	
	IsDirty: function()
	{		
		var table = OBJ("reserves_list");
		var rows = table.getElementsByTagName("tr");		
		var i;
				
		for(i=1; i<=rows.length; i++)
		{									
			if(OBJ("en_dhcp_reserv"+i)!=null)
			{
				this.reserv_new[i] = {
					enable:	OBJ("en_dhcp_reserv"+i).checked?"1":"0",
					host: OBJ("en_dhcp_host"+i).innerHTML,
					ipaddr:	 COMM_IPv4HOST(OBJ("en_dhcp_ipaddr"+i).innerHTML, this.mask),
					macaddr: OBJ("en_dhcp_macaddr"+i).innerHTML
				};											
			}	
		}
				
		if(this.reserv_old.length != this.reserv_new.length) return true;
		
		for(i=1; i<=this.reserv_new.length; i++)
		{																			
			if(this.reserv_old[i]!=null && this.reserv_new[i]!=null)
			{
				if(this.reserv_old[i].enable != this.reserv_new[i].enable ||
					this.reserv_old[i].host!= this.reserv_new[i].host ||
					this.reserv_old[i].ipaddr != this.reserv_new[i].ipaddr ||
					this.reserv_old[i].macaddr != this.reserv_new[i].macaddr
				  )
				  return true; 
			}
			else if(this.reserv_old[i]!=null || this.reserv_new[i]!=null)
			{		
				return true; 
			}
		}
		return false;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	lanip: null,
	inetp: null,
	dhcps4: null,
	dhcps4_inet: null,
	leasep: null,
	mask: null,
	ipdirty: false,
	reserv_old: null,
	reserv_new: null,
	cfg: null,
	g_edit: 0,
	g_table_index: 1,	
	InitHostname: function()
	{
		var b = PXML.FindModule("DEVICE.HOSTNAME");
		if (!b)
		{
			BODY.ShowAlert("<?echo i18n("InitHostname() ERROR!!!");?>");
			return false;
		}

		OBJ("device").value = XG(b+"/device/hostname");
		return true;
	},
	PreHostname: function()
	{
		if (COMM_Equal(OBJ("device").getAttribute("modified"), "true"))
		{
			var b = PXML.FindModule("DEVICE.HOSTNAME");			
			XS(b+"/device/hostname", OBJ("device").value);
			PXML.ActiveModule("DEVICE.HOSTNAME");
		}
		else
			PXML.IgnoreModule("DEVICE.HOSTNAME");
		return true;
	},
	InitLAN: function()
	{
		<?
			if($FEATURE_PARENTALCTRL=='1')
			{
				echo "	PXML.IgnoreModule('OPENDNS4');\n".
					 "	var p = PXML.FindModule('OPENDNS4');\n".
					 "	var wan1_infp  = GPBT(p, 'inf', 'uid', 'WAN-1', false);\n";
			}
		?>
		
		var lan	= PXML.FindModule("INET.LAN-1");
		var inetuid = XG(lan+"/inf/inet");
		this.inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);
		if (!this.inetp)
		{
			BODY.ShowAlert("InitLAN() ERROR!!!");
			return false;
		}

		if (XG(this.inetp+"/addrtype") == "ipv4")
		{
			var b = this.inetp+"/ipv4";
			this.lanip = XG(b+"/ipaddr");
			this.mask = XG(b+"/mask");
			OBJ("ipaddr").value	= this.lanip;
			OBJ("netmask").value= COMM_IPv4INT2MASK(this.mask);
			OBJ("dnsr").checked	= XG(lan+"/inf/dns4")!="" ? true : false;
		}
		
		/*Enable and lock DNS Relay when openDNS is used*/
		<?
			if($FEATURE_PARENTALCTRL=='1')
			{
				echo "	if(XG(wan1_infp+'/open_dns/type') !== '')\n".
					 "	{\n".
					 "	OBJ('dnsr').disabled = true;\n".
					 "	OBJ('dnsr').title = ".'"'.I18N("h", "Locked by parental control").'"'.";\n".
					 "	}\n";
			}
		?>
		
		return true;
	},
	PreLAN: function()
	{
		var lan = PXML.FindModule("INET.LAN-1");
		var b = this.inetp+"/ipv4";

		var vals = OBJ("ipaddr").value.split(".");
		if (vals.length!=4)
		{
			BODY.ShowAlert("<?echo i18n("Invalid IP address");?>");
			OBJ("ipaddr").focus();
			return false;
		}
		for (var i=0; i<4; i++)
		{
			if (!TEMP_IsDigit(vals[i]) || vals[i]>255)
			{
				BODY.ShowAlert("<?echo i18n("Invalid IP address");?>");
				OBJ("ipaddr").focus();
				return false;
			}
		}
		this.mask = COMM_IPv4MASK2INT(OBJ("netmask").value);
		XS(b+"/ipaddr", OBJ("ipaddr").value);
		XS(b+"/mask", this.mask);
		if (OBJ("dhcpsvr").checked)	XS(lan+"/inf/dhcps4", "DHCPS4-1");
		else						XS(lan+"/inf/dhcps4", "");
		if (OBJ("dnsr").checked)	XS(lan+"/inf/dns4", "DNS4-1");
		else						XS(lan+"/inf/dns4", "");

		if (COMM_EqBOOL(OBJ("ipaddr").getAttribute("modified"), true))
		{
			this.ipdirty = true;
		}		
			
		if (this.ipdirty||
			COMM_EqBOOL(OBJ("netmask").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("dnsr").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("dhcpsvr").getAttribute("modified"), true)
			)
		{
			PXML.DelayActiveModule("INET.LAN-1", "3");
		}
		else
		{
			PXML.IgnoreModule("INET.LAN-1");
		}
		
		return true;
	},
	InitDHCPS: function()
	{				
		var svc = PXML.FindModule("DHCPS4.LAN-1");
		var inf1p = PXML.FindModule("RUNTIME.INF.LAN-1");
		if (!svc || !inf1p)
		{
			BODY.ShowAlert("InitDHCPS() ERROR !");
			return false;
		}
		this.dhcps4 = GPBT(svc+"/dhcps4", "entry", "uid", "DHCPS4-1", false);
		this.dhcps4_inet = svc + "/inet/entry";
		this.leasep = GPBT(inf1p+"/runtime", "inf", "uid", "LAN-1", false);		
		if (!this.dhcps4)
		{
			BODY.ShowAlert("InitDHCPS() ERROR !");
			return false;
		}
		this.leasep += "/dhcps4/leases";
		var tmp_ip = OBJ("ipaddr").value.substring(OBJ("ipaddr").value.lastIndexOf('.')+1, OBJ("ipaddr").value.length);
		var tmp_mask = OBJ("netmask").value.substring(OBJ("netmask").value.lastIndexOf('.')+1, OBJ("netmask").value.length)
		var startip = parseInt(XG(this.dhcps4+"/start"),10) + (tmp_ip & tmp_mask);
		var endip = parseInt(XG(this.dhcps4+"/end"),10) + (tmp_ip & tmp_mask);

		OBJ("domain").value		= XG(this.dhcps4+"/domain");
		OBJ("dhcpsvr").checked	= (XG(svc+"/inf/dhcps4")!="")? true : false;
		OBJ("startip").value	= startip;
		OBJ("endip").value		= endip;
		OBJ("leasetime").value	= Math.floor(XG(this.dhcps4+"/leasetime")/60);
		//sam_pan add		
		OBJ("broadcast").value	= XG(this.dhcps4+"/broadcast");	
		
		this.OnClickDHCPSvr();
					
		if (!this.leasep)	return true;	// in bridge mode, the value of this.leasep is null.
		entry = this.leasep+"/entry";
		cnt = XG(entry+"#");
		if (XG(svc+"/inf/dhcps4")!="")			// when the dhcp server is enabled show the dynamic dhcp clients list
		{
			for (var i=1; i<=cnt; i++)
			{
				var uid		= "DUMMY_"+i;
				var host	= XG(entry+":"+i+"/hostname");
				var ipaddr	= XG(entry+":"+i+"/ipaddr");
				var mac		= XG(entry+":"+i+"/macaddr");
				var expires	= XG(entry+":"+i+"/expire");
				if(parseInt(expires, 10) == 0)
				{
					continue;
				}
				if (parseInt(expires, 10) > 6000000)
				{
					expires = "Never";
				}
				else if (parseInt(expires, 10) < 60)
				{
					expires = "< 1 <?echo i18n("Minute");?>";
				}
				else
				{
					var time= COMM_SecToStr(expires);
					expires = "";
					if (time["day"]>1)
					{
						expires = time["day"]+" <?echo i18n("Days");?> ";
					}
					else if (time["day"]>0)
					{
						expires = time["day"]+" <?echo i18n("Day");?> ";
					}
					if (time["hour"]>1)
					{
						expires += time["hour"]+" <?echo i18n("Hours");?> ";
					}
					else if (time["hour"]>0)
					{
						expires += time["hour"]+" <?echo i18n("Hour");?> ";
					}
					if (time["min"]>1)
					{
						expires += time["min"]+" <?echo i18n("Minutes");?>";
					}
					else if (time["min"]>0)
					{
						expires += time["min"]+" <?echo i18n("Minute");?>";
					}
				}
				var data	= [host, ipaddr, mac, expires];
				var type	= ["text", "text", "text", "text"];
				if (expires != "Never")
				{									
					BODY.InjectTable("leases_list", uid, data, type);
				}	
			}
		}
		
		if (this.reserv_old) delete this.reserv_old;
		if (this.reserv_new) delete this.reserv_new;
		this.reserv_old = new Array();
		this.reserv_new = new Array();
			
		cnt = XG(this.dhcps4+"/staticleases/entry#");
		for(var i=1; i <= cnt; i++)
		{						
			var data = [	'<input type="checkbox" id="en_dhcp_reserv'+i+'">',
				'<span id="en_dhcp_host'+i+'"></span>',
				'<span id="en_dhcp_ipaddr'+i+'"></span>',
				'<span id="en_dhcp_macaddr'+i+'"></span>',
				'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif"></a>',
				'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif"></a>'
				];
			var type	= ["","", "","",""];
			
			BODY.InjectTable("reserves_list", i, data, type);									
			OBJ("en_dhcp_reserv"+i).checked = COMM_EqNUMBER(XG(this.dhcps4+"/staticleases/entry:"+i+"/enable"), 1);					
			OBJ("en_dhcp_host"+i).innerHTML = XG(this.dhcps4+"/staticleases/entry:"+i+"/hostname");
			OBJ("en_dhcp_ipaddr"+i).innerHTML = COMM_IPv4IPADDR(this.lanip, this.mask, XG(this.dhcps4+"/staticleases/entry:"+i+"/hostid"));
			OBJ("en_dhcp_macaddr"+i).innerHTML = XG(this.dhcps4+"/staticleases/entry:"+i+"/macaddr");
			
			this.reserv_old[i] = {
				enable:	XG(this.dhcps4+"/staticleases/entry:"+i+"/enable"),
				host: XG(this.dhcps4+"/staticleases/entry:"+i+"/hostname"),
				ipaddr:	 XG(this.dhcps4+"/staticleases/entry:"+i+"/hostid"),
				macaddr: XG(this.dhcps4+"/staticleases/entry:"+i+"/macaddr")
				};																		
		}
		this.g_table_index=i;
				
		return true;
	},
	PreDHCPS: function()
	{
		var lan = PXML.FindModule("DHCPS4.LAN-1");
		var ipaddr = COMM_IPv4NETWORK(OBJ("ipaddr").value, "24");
		var maxhost = COMM_IPv4MAXHOST(this.mask)-1;
		var network = ipaddr.substring(0, ipaddr.lastIndexOf('.')+1);
		var hostid = parseInt(COMM_IPv4HOST(OBJ("ipaddr").value, this.mask), 10);
		var tmp_ip = OBJ("ipaddr").value.substring(OBJ("ipaddr").value.lastIndexOf('.')+1, OBJ("ipaddr").value.length);
		var tmp_mask = OBJ("netmask").value.substring(OBJ("netmask").value.lastIndexOf('.')+1, OBJ("netmask").value.length)
		var startip = parseInt(OBJ("startip").value, 10) - (tmp_ip & tmp_mask);
		var endip = parseInt(OBJ("endip").value, 10) - (tmp_ip & tmp_mask);

		if (isDomain(OBJ("domain").value))
			XS(this.dhcps4+"/domain", OBJ("domain").value);
		else
		{
			BODY.ShowAlert("<?echo i18n("The input domain name is illegal.");?>");
			OBJ("domain").focus();
			return false;
		}
		if (OBJ("dhcpsvr").checked)	XS(lan+"/inf/dhcps4",	"DHCPS4-1");
		else						XS(lan+"/inf/dhcps4",	"");
		if (OBJ("dnsr").checked)	XS(lan+"/inf/dns4",		"DNS4-1");
		else						XS(lan+"/inf/dns4",		"");
		if (COMM_EqBOOL(OBJ("dhcpsvr").checked, true))
		{
			if (!TEMP_IsDigit(OBJ("startip").value) || !TEMP_IsDigit(OBJ("endip").value))
			{
				BODY.ShowAlert("<?echo i18n("DHCP IP Address Range is invalid.");?>"); 
				return false;
			}
			if (hostid>=parseInt(OBJ("startip").value, 10) && hostid<=parseInt(OBJ("endip").value, 10))
			{
				BODY.ShowAlert("<?echo i18n("The Router IP Address is belong to the lease pool of DHCP server.");?>");
				return false;
			}
			if (!TEMP_IsDigit(OBJ("leasetime").value))
			{
				BODY.ShowAlert("<?echo i18n("The input lease time is invalid.");?>");
				return false;
			}
			if (this.mask >= 24 && (startip < 1 || endip > maxhost))
			{
				BODY.ShowAlert("<?echo i18n("DHCP IP Address Range is out of the boundary.");?>");  
				return false;
			}
			if (parseInt(OBJ("startip").value, 10) > parseInt(OBJ("endip").value, 10))
			{
				BODY.ShowAlert("<?echo i18n("The start of DHCP IP Address Range should be smaller than the end.");?>");
				return false;
			}
	
			XS(this.dhcps4_inet+"/ipv4/ipaddr", OBJ("ipaddr").value);
			XS(this.dhcps4_inet+"/ipv4/mask", this.mask);
			XS(this.dhcps4+"/start", startip);
			XS(this.dhcps4+"/end", endip);
			XS(this.dhcps4+"/leasetime", OBJ("leasetime").value*60);			
			XS(this.dhcps4+"/broadcast", OBJ("broadcast").checked?"yes":"no");							
		}
		
		/*clear static leases entry*/
		var cnt = XG(this.dhcps4+"/staticleases/count");
		for (var i=1; i<= cnt; i++)	
		{
			XD(this.dhcps4+"/staticleases/entry");		
		}	
		XS(this.dhcps4+"/staticleases/count", "0");
		
		/*+++set values to xml*/
		var table = OBJ("reserves_list");
		var rows = table.getElementsByTagName("tr");
		var rowslen = rows.length;								
		if(this.reserv_old.length > rows.length) rowslen = this.reserv_old.length;
				
		for (var i=1; i <= rowslen; i++)
		{						
			if(OBJ("en_dhcp_reserv"+i)!=null)
			{
				if (OBJ("en_dhcp_ipaddr"+i).innerHTML != "" || 
					OBJ("en_dhcp_macaddr"+i).innerHTML != "" ||
					OBJ("en_dhcp_host"+i).innerHTML != "")
				{
					var mac = GetMAC(OBJ("en_dhcp_macaddr"+i).innerHTML);
					if (mac=="")
					{
						BODY.ShowAlert("<?echo i18n("Invalid MAC address value.");?>");					
						return false;
					}
					
					if (OBJ("en_dhcp_host"+i).innerHTML=="")
					{
						BODY.ShowAlert("<?echo i18n("Invalid Computer Name");?>");					
						return false;
					}
					
					if (TEMP_CheckNetworkAddr(OBJ("en_dhcp_ipaddr"+i).innerHTML, OBJ("ipaddr").value, this.mask))
					{
						var path = COMM_AddEntry(PXML.doc, this.dhcps4+"/staticleases", "STIP-");
						XS(path+"/enable",		OBJ("en_dhcp_reserv"+i).checked?"1":"0");
						XS(path+"/description",	OBJ("en_dhcp_host"+i).innerHTML);
						XS(path+"/hostname",	OBJ("en_dhcp_host"+i).innerHTML);
						XS(path+"/macaddr",		OBJ("en_dhcp_macaddr"+i).innerHTML);
						XS(path+"/hostid",		COMM_IPv4HOST(OBJ("en_dhcp_ipaddr"+i).innerHTML, this.mask));						
					}
					else
					{
						BODY.ShowAlert("<?echo i18n("Invalid DHCP reservation IP address");?>");					
						return false;
					}
				}
			}	
		}
		/*---set values to xml*/
										
		PXML.ActiveModule("DHCPS4.LAN-1");
		return true;
	},
	LanSync: function(active, learnfromwan, scope, ntype, cnt, win1, win2)
	{
		var svc = PXML.FindModule("DHCPS4.LAN-2");
		var dhcps4lan2 = GPBT(svc+"/dhcps4", "entry", "uid", "DHCPS4-2", false);
		var netbios = dhcps4lan2+"/netbios";
		var wins = dhcps4lan2+"/wins";
		
		XS(dhcps4lan2+"/broadcast", OBJ("broadcast").checked?"yes":"no");
		XS(netbios+"/active", active);		
		XS(netbios+"/learnfromwan", learnfromwan);
		XS(netbios+"/scope", scope);		
		XS(netbios+"/ntype", ntype);						
		XS(wins+"/count", cnt);	
		XS(wins+"/entry:1", win1);	
		XS(wins+"/entry:2", win2);			
		PXML.ActiveModule("DHCPS4.LAN-2");
	},	
	OnClickDHCPSvr: function()
	{
		if (OBJ("dhcpsvr").checked && this.rgmode)
		{
			OBJ("startip").setAttribute("modified", "false");
			OBJ("endip").setAttribute("modified", "false");
			OBJ("leasetime").setAttribute("modified", "false");
			OBJ("startip").disabled = false;
			OBJ("endip").disabled = false;
			OBJ("leasetime").disabled = false;
			OBJ("broadcast").disabled = false;
			if(OBJ("broadcast").value == "yes")
			{
				OBJ("broadcast").checked = true;
			}
			else
			{
				OBJ("broadcast").checked = false;
			}		
		}
		else
		{
			OBJ("startip").setAttribute("modified",	"ignore");
			OBJ("endip").setAttribute("modified", "ignore");
			OBJ("leasetime").setAttribute("modified", "ignore");
			OBJ("startip").disabled = true;
			OBJ("endip").disabled = true;
			OBJ("leasetime").disabled = true;
			OBJ("broadcast").disabled = true;
		}
	},
	OnChangeGetClient: function()
	{
		var ipaddr = OBJ("pc").value;
		var entry = PAGE.leasep+"/entry";
		var cnt = XG(entry+"#");
		if (ipaddr == "")	return;
		for (var i=1; i<=cnt; i++)
		{
			if (XG(entry+":"+i+"/ipaddr") == ipaddr)
			{
				
				OBJ("en_dhcp_reserv").checked= true;
				OBJ("reserv_host").value= XG(entry+":"+i+"/hostname");				
				OBJ("reserv_ipaddr").value  = XG(entry+":"+i+"/ipaddr");
				OBJ("reserv_macaddr").value = XG(entry+":"+i+"/macaddr");
				OBJ("pc").selectedIndex = 0;
				
				return;
			}
		}
	},
	InitNetBios: function()
	{						
		var netbios = this.dhcps4+"/netbios";
		var wins = this.dhcps4+"/wins";
		
		var active = parseInt(XG(netbios+"/active"));
		var learnfromwan = parseInt(XG(netbios+"/learnfromwan"));
		var scope = XG(netbios+"/scope");
		var ntype = parseInt(XG(netbios+"/ntype"));
		var winscount = parseInt(XG(wins+"/count"));				
						
		if(active == 1)
		{
			OBJ("netbios_enable").checked = true;
									
			if(learnfromwan == 1) 
				{ OBJ("netbios_learn").checked = true;} 
			else                  
				{ OBJ("netbios_learn").checked = false;}	
			
			this.on_check_learn();							
													
			OBJ("netbios_scope").value = scope;			
			this.set_radio_value("winstype", ntype);					
									
			if(winscount > 0)
			{			
				var win1 = XG(wins+"/entry:1");	
				var win2 = XG(wins+"/entry:2");
				if(win1 != "") OBJ("primarywins").value = win1;
				if(win2 != "") OBJ("secondarywins").value = win2;
			}									
		}
		else
		{			
			OBJ("netbios_enable").checked = false;
			OBJ("netbios_learn").disabled = true;
			OBJ("netbios_scope").disabled  = true;	
			this.set_radio_value("winstype", ntype);
			this.action_radio("winstype", "disable");
			if(winscount > 0)
			{			
				var win1 = XG(wins+"/entry:1");	
				var win2 = XG(wins+"/entry:2");
				if(win1 != "") OBJ("primarywins").value = win1;
				if(win2 != "") OBJ("secondarywins").value = win2;
			}		
			OBJ("primarywins").disabled = true;
			OBJ("secondarywins").disabled = true;		
		}						
	},
	PreNetBios: function()
	{		
		var netbios = this.dhcps4+"/netbios";
		var wins = this.dhcps4+"/wins";
		
		if(OBJ("netbios_enable").checked) 
			{active=1;}
		else 
			{active=0;}
			
		if(OBJ("netbios_learn").checked) 
		{
			learnfromwan=1;
			if(XG(netbios+"/learnfromwan")!="1")
			{	
				PXML.ActiveModule("WAN");
			}
			else
			{
				PXML.IgnoreModule("WAN");
			}				
		}
		else 
		{
			learnfromwan=0;
			PXML.IgnoreModule("WAN");
		}
							
		var scope = OBJ("netbios_scope").value;	
		var ntype = this.get_radio_value("winstype");
		var win1 = OBJ("primarywins").value;	
		var win2 = OBJ("secondarywins").value;	
		
		if(!OBJ("netbios_learn").checked)
		{
			if(!this.ip_verify("primarywins")) return false;
			if(!this.ip_verify("secondarywins")) return false;
		}	

		XS(netbios+"/active", active);
		XS(netbios+"/learnfromwan", learnfromwan);
		XS(netbios+"/scope", scope);		
		XS(netbios+"/ntype", ntype);				
		var cnt = 0;
		
		if(win1 != "") cnt++;
		if(win2 != "") cnt++;
		XS(wins+"/count", cnt);	
		XS(wins+"/entry:1", win1);	
		XS(wins+"/entry:2", win2);		
		PXML.ActiveModule("DHCPS4.LAN-1");
		
		this.LanSync(active, learnfromwan, scope, ntype, cnt, win1, win2);				
		return true;			
	},					
	on_check_netbios: function()
	{
		var netbios_learn = OBJ("netbios_learn");
		if(OBJ("netbios_enable").checked)
		{									
			OBJ("netbios_learn").disabled = false;
			this.on_check_learn();
		}
		else
		{			
			this.action_radio("winstype", "disable");
			OBJ("netbios_learn").disabled = true;			
			OBJ("netbios_scope").disabled = true;
			OBJ("primarywins").disabled   = true;
			OBJ("secondarywins").disabled = true;		
		}									
	},
	on_check_learn: function()
	{
		if(OBJ("netbios_learn").checked)
		{										
			this.action_radio("winstype", "disable");			
			OBJ("netbios_scope").disabled = true;
			OBJ("primarywins").disabled   = true;
			OBJ("secondarywins").disabled = true;				
		}
		else
		{			
			this.action_radio("winstype", "enable");
			OBJ("netbios_scope").disabled = false;
			OBJ("primarywins").disabled   = false;
			OBJ("secondarywins").disabled = false;	
		}						
	},			
	set_radio_value: function(name, value)
	{
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{			
			if(obj[i].value==value)
			{
				obj[i].checked = true;
				break;
			}
		}
	},
	get_radio_value: function(name)
	{
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{
			if (obj[i].checked)	return obj[i].value;
		}
	},	
	action_radio: function(name, flag)
	{				
		var obj = document.getElementsByName(name);
		for (var i=0; i<obj.length; i++)
		{						
			if(flag == "disable") {obj[i].disabled = true;}
			else                  {obj[i].disabled = false;}
		}					
	},
	ip_verify: function(name)
	{
		
		if(OBJ(name).value == "") 
		{
			return true;
		}
		
		if(OBJ(name).value == "127.0.0.1") 
		{
			BODY.ShowAlert("<?echo i18n("Invalid IP address");?>");
			OBJ(name).focus();
			return false;
		}
			
		var vals = OBJ(name).value.split(".");
		
		if (vals.length!=4)
		{
			BODY.ShowAlert("<?echo i18n("Invalid IP address");?>");
			OBJ(name).focus();
			return false;
		}
		
		for (var i=0; i<4; i++)
		{
			if (!TEMP_IsDigit(vals[i]) || vals[i]>=255 || vals[i]< 0)
			{
				BODY.ShowAlert("<?echo i18n("Invalid IP address");?>");
				OBJ(name).focus();
				return false;
			}
		}
		
		return true;	
	},
	OnClickMacButton: function(objname)
	{
		OBJ(objname).value="<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
	},
	AddDHCPReserv: function()
	{
		var i=0;
		
		if(!this.ip_verify("reserv_ipaddr")) return false;
		
		if(this.g_edit!=0)
		{
			i=this.g_edit;			
		}
		else
		{
			i=this.g_table_index;
		}
		if (i > <?=$DHCP_MAX_COUNT?>) 
		{
			BODY.ShowAlert("<?echo I18N("j", "The maximum number of permitted DHCP reservations has been exceeded.");?>");
			return false;
		}	
			
		var data = [	'<input type="checkbox" id="en_dhcp_reserv'+i+'">',
						'<span id="en_dhcp_host'+i+'"></span>',
						'<span id="en_dhcp_ipaddr'+i+'"></span>',
						'<span id="en_dhcp_macaddr'+i+'"></span>',
						'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif"></a>',
						'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif"></a>'
						];
		var type = ["","", "","",""];
		
		BODY.InjectTable("reserves_list", i, data, type);
		
		OBJ("en_dhcp_reserv"+i).checked = OBJ("en_dhcp_reserv").checked?true:false;
		OBJ("en_dhcp_host"+i).innerHTML = OBJ("reserv_host").value;
		OBJ("en_dhcp_ipaddr"+i).innerHTML = OBJ("reserv_ipaddr").value;
		OBJ("en_dhcp_macaddr"+i).innerHTML = OBJ("reserv_macaddr").value;		
				
		if(this.g_edit!=0)
		{
			this.g_edit=0;
		}
		else
		{
			this.g_table_index++;	
		}		
		OBJ("mainform").setAttribute("modified", "true");		
	},
	ClearDHCPReserv: function()
	{		
		OBJ("en_dhcp_reserv").checked = false;
		OBJ("reserv_host").value = "";
		OBJ("reserv_ipaddr").value = "";
		OBJ("reserv_macaddr").value = "";
		OBJ("mainform").setAttribute("modified", "true");		
	},	
	OnEdit: function(i)
	{
		OBJ("en_dhcp_reserv").checked=OBJ("en_dhcp_reserv"+i).checked;
		OBJ("reserv_host").value=OBJ("en_dhcp_host"+i).innerHTML;
		OBJ("reserv_ipaddr").value=OBJ("en_dhcp_ipaddr"+i).innerHTML;
		OBJ("reserv_macaddr").value=OBJ("en_dhcp_macaddr"+i).innerHTML;
		this.g_edit=i;		
	},	
	OnDelete: function(i)
	{
		var z;
		var table = OBJ("reserves_list");
		var rows = table.getElementsByTagName("tr");		
		
		for (z=1; z<=rows.length; z++) 
		{
			if(rows[z]!=null)
			{
				if (rows[z].id==i)
				{
					table.deleteRow(z);					
				}												
			}	
		}
		this.g_table_index--;									
	}
}

function FocusObj(result)
{
	var found = true;
	var node = result.Get("/hedwig/node");
	var nArray = node.split("/");
	var len = nArray.length;
	var name = nArray[len-1];
	if (node.match("inet"))
	{
		switch (name)
		{
		case "ipaddr":
			OBJ("ipaddr").focus();
			break;
		case "mask":
			OBJ("netmask").focus();
			break;
		default:
			found = false;
			break;
		}
	}
	else if (node.match("dhcps4"))
	{
		switch (name)
		{		
		case "start":
			OBJ("startip").focus();
			break;
		case "end":
			OBJ("endip").focus();
			break;
		case "leasetime":
			OBJ("leasetime").focus();
		case "hostid":
			OBJ("reserv_ipaddr").focus();
			break;
		case "macaddr":
			OBJ("reserv_macaddr").focus();
			break;
		default:
			found = false;
			break;
		}
	}
	else
	{
		found = false;
	}

	return found;
}

function isDomain(domain)
{
	var rlt = true;
	var dArray = new Array();
	if (domain.length==0)	return rlt;
	else					dArray = domain.split(".");

	/* the total length of a domain name is restricted to 255 octets or less. */
	if (domain.length > 255)
	{
		rlt = false;
	}
	for (var i=0; i<dArray.length; i++)
	{
		var reg = new RegExp("[A-Za-z0-9\-]{"+dArray[i].length+"}");
		/* the label must start with a letter */
		if (!dArray[i].match(/^[A-Za-z]/))
		{
			rlt = false;
			break;
		}
		/* the label must end with a letter or digit. */
		else if (!dArray[i].match(/[A-Za-z0-9]$/))
		{
			rlt = false;
			break;
		}
		/* the label must be 63 characters or less. */
		else if (dArray[i].length>63)
		{
			rlt = false;
			break;
		}
		/* the label has interior characters that only letters, digits and hyphen */
		else if (!reg.exec(dArray[i]))
		{
			rlt = false;
			break;
		}
	}

	return rlt;
}

function GetIPLastField(ipaddr)
{
	return ipaddr.substring(ipaddr.lastIndexOf('.')+1);
}
function SetDelayTime(millis)
{
	var date = new Date();
	var curDate = null;
	curDate = new Date();
	do { curDate = new Date(); }
	while(curDate-date < millis);
}
function GetMAC(m)
{
	var myMAC="";
	if (m.search(":") != -1)	var tmp=m.split(":");
	else				var tmp=m.split("-");
	if (m == "" || tmp.length != 6)
		return "";

	for (var i=0; i<tmp.length; i++)
	{
		if (tmp[i].length==1)
			tmp[i]="0"+tmp[i];
		else if (tmp[i].length==0||tmp[i].length>2)
			return "";
	}
	myMAC = tmp[0];
	for (var i=1; i<tmp.length; i++)
	{
		myMAC = myMAC + ':' + tmp[i];
	}
	return myMAC;
}
function GetEntryNo(en)
{
	return en.substring(en.lastIndexOf(':')+1);
}

function Service(svc, ipaddr)
{	
	var banner = "<?echo i18n("Rebooting");?>...";
	var msgArray = ["<?echo I18N("j","If you changed the IP address of the router you will need to change the IP address in your browser before accessing the configuration web page again.");?>"];
	var delay = 10;
	var sec = <?echo query("/runtime/device/bootuptime");?> + delay;
	var url = null;
	var ajaxObj = GetAjaxObj("SERVICE");
	if (svc=="FRESET")		url = "http://192.168.0.1/index.php";
	else if (svc=="REBOOT")	url = "http://"+ipaddr+"/index.php";
	else					return false;
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowAlert("Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		else
			BODY.ShowCountdown(banner, msgArray, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
</script>
