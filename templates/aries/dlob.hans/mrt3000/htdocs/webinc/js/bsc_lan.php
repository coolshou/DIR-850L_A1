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
	//services: "DEVICE.HOSTNAME,INET.LAN-1,DHCPS4.LAN-1,DHCPS4.LAN-2,RUNTIME.INF.LAN-1,URLCTRL,WAN,OPENDNS4",
	services: "DEVICE.HOSTNAME,INET.LAN-1,DHCPS4.LAN-1,DHCPS4.LAN-2,RUNTIME.INF.LAN-1,URLCTRL,WAN,SAMBA",
	OnLoad: function()
	{
		SetDelayTime(500);	//add delay for event updatelease finished
		TEMP_CleanTable("reserves_list");
		TEMP_CleanTable("leases_list");
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
				//Service("REBOOT", OBJ("ipaddr").value);
				Service("REBOOT", TEMP_GetFieldsValue("ipaddr_","."));
			}
			else
			{
				BODY.OnReload();
			}
			break;
		case "BUSY":
			BODY.ShowMessage("Error","<?echo I18N("j","Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			if (result.Get("/hedwig/result")=="FAILED")
			{
				FocusObj(result);
				//alert(result.Get("/hedwig/message"));
				//if (this.ErrorHandler) this.ErrorHandler(result.Get("/hedwig/node"), result.Get("/hedwig/message"));
			}
			break;
		case "PIGWIDGEON":
			BODY.ShowMessage("Error",result.Get("/pigwidgeon/message"));
			break;
		}
		return true;
	},
	ErrorHandler: function(node, msg)
    {
		var found = true;
		var nArray = node.split("/");
		var len = nArray.length;
		var name = nArray[len-1];
		if (node.match("inet"))
		{
			switch (name)
			{
				case "ipaddr":
					OBJ("ipaddr_1").focus();
					BODY.ShowConfigError("ipaddr_1", msg);
					break;
				case "mask":
					OBJ("netmask_1").focus();
					BODY.ShowConfigError("netmask_1", msg);
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
						BODY.ShowConfigError("startip", msg);
						break;
				case "end":
						OBJ("endip").focus();
						BODY.ShowConfigError("startip", msg);
						break;
				case "leasetime":
						OBJ("leasetime").focus();
						BODY.ShowConfigError("startip", msg);
						break;
				case "hostid":
						OBJ("reserv_ipaddr_1").focus();
						BODY.ShowConfigError("reserv_ipaddr_1", msg);
						break;
				case "macaddr":
						OBJ("reserv_macaddr_1").focus();
						BODY.ShowConfigError("reserv_macaddr_1", msg);
						break;
				default:
						found = false;
						break;
			}
		}
        BODY.ShowMessage("Error",node+'<br>'+msg);
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
		BODY.ClearConfigError();
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
			BODY.ShowMessage("Error","<?echo I18N("j","InitHostname() ERROR!!!");?>");
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
			
			if(this.IsNumber(OBJ("device").value))
			{
				BODY.ShowConfigError("device", "<?echo I18N("j","Host name format error, must contain alphabet.");?>");
				return false;
			}
			else if(OBJ("device").value.indexOf(".")!=(-1))
			{
				BODY.ShowConfigError("device","<?echo I18N("j","Host name format error, can't contain dot.");?>");
				OBJ("device").focus();
				return false;
			}
			
			XS(b+"/device/hostname", OBJ("device").value);
			PXML.ActiveModule("DEVICE.HOSTNAME");
			PXML.CheckModule("SAMBA", "ignore", "ignore", null); //SAMBA need hostname
		}
		else
		{
			PXML.IgnoreModule("DEVICE.HOSTNAME");
			PXML.IgnoreModule("SAMBA");
		}	
		return true;
	},
	InitLAN: function()
	{
		//PXML.IgnoreModule("OPENDNS4");
		//var p = PXML.FindModule("OPENDNS4");
		//var wan1_infp  = GPBT(p, "inf", "uid", "WAN-1", false);
		var lan	= PXML.FindModule("INET.LAN-1");
		var inetuid = XG(lan+"/inf/inet");
		this.inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);
		if (!this.inetp)
		{
			BODY.ShowMessage("Error","InitLAN() ERROR!!!");
			return false;
		}

		if (XG(this.inetp+"/addrtype") == "ipv4")
		{
			var b = this.inetp+"/ipv4";
			this.lanip = XG(b+"/ipaddr");
			this.mask = XG(b+"/mask");
			//OBJ("ipaddr").value	= this.lanip;
			TEMP_SetFieldsByDelimit("ipaddr_", this.lanip, ".");
			//OBJ("netmask").value= COMM_IPv4INT2MASK(this.mask);
			TEMP_SetFieldsByDelimit("netmask_", COMM_IPv4INT2MASK(this.mask), ".");
			OBJ("dnsr").checked	= XG(lan+"/inf/dns4")!="" ? true : false;
		}
		
		return true;
	},
	PreLAN: function()
	{
		var lan = PXML.FindModule("INET.LAN-1");
		var b = this.inetp+"/ipv4";

		//var vals = OBJ("ipaddr").value.split(".");
		var vals = [OBJ("ipaddr_1"),OBJ("ipaddr_2"),OBJ("ipaddr_3"),OBJ("ipaddr_4")];
		
		for (var i=0; i<4; i++)
		{
			if (!TEMP_IsDigit(vals[i].value) || vals[i].value>255)
			{
				//alert("<?echo I18N("j","Invalid IP address");?>");
				BODY.ShowConfigError("ipaddr_1", "<?echo I18N("j","Invalid IP address.");?>");
				OBJ("ipaddr_"+(i+1)).focus();
				return false;
			}
		}
		//this.mask = COMM_IPv4MASK2INT(OBJ("netmask").value);
		this.mask = COMM_IPv4MASK2INT(TEMP_GetFieldsValue("netmask_", "."));
		//XS(b+"/ipaddr", OBJ("ipaddr").value);
		XS(b+"/ipaddr", TEMP_GetFieldsValue("ipaddr_", "."));
		XS(b+"/mask", this.mask);
		if (OBJ("dhcpsvr").checked)	XS(lan+"/inf/dhcps4", "DHCPS4-1");
		else						XS(lan+"/inf/dhcps4", "");
		if (OBJ("dnsr").checked)	XS(lan+"/inf/dns4", "DNS4-1");
		else						XS(lan+"/inf/dns4", "");

		if (COMM_EqBOOL(OBJ("ipaddr_1").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("ipaddr_2").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("ipaddr_3").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("ipaddr_4").getAttribute("modified"), true))
		{
			this.ipdirty = true;
		}		
			
		if (this.ipdirty||
			COMM_EqBOOL(OBJ("netmask_1").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("netmask_2").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("netmask_3").getAttribute("modified"), true)||
			COMM_EqBOOL(OBJ("netmask_4").getAttribute("modified"), true)||
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
			BODY.ShowMessage("Error","InitDHCPS() ERROR !");
			return false;
		}
		this.dhcps4 = GPBT(svc+"/dhcps4", "entry", "uid", "DHCPS4-1", false);
		this.dhcps4_inet = svc + "/inet/entry";
		this.leasep = GPBT(inf1p+"/runtime", "inf", "uid", "LAN-1", false);		
		if (!this.dhcps4)
		{
			BODY.ShowMessage("Error","InitDHCPS() ERROR !");
			return false;
		}
		this.leasep += "/dhcps4/leases";
		//var tmp_ip = OBJ("ipaddr").value.substring(OBJ("ipaddr").value.lastIndexOf('.')+1, OBJ("ipaddr").value.length);
		var tmp_ip = OBJ("ipaddr_4").value;
		//var tmp_mask = OBJ("netmask").value.substring(OBJ("netmask").value.lastIndexOf('.')+1, OBJ("netmask").value.length);
		var tmp_mask = OBJ("netmask_4").value;
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
					expires = "< 1 <?echo I18N("j","Minute");?>";
				}
				else
				{
					var time= COMM_SecToStr(expires);
					expires = "";
					if (time["day"]>1)
					{
						expires = time["day"]+" <?echo I18N("j","Days");?> ";
					}
					else if (time["day"]>0)
					{
						expires = time["day"]+" <?echo I18N("j","Day");?> ";
					}
					if (time["hour"]>1)
					{
						expires += time["hour"]+" <?echo I18N("j","Hours");?> ";
					}
					else if (time["hour"]>0)
					{
						expires += time["hour"]+" <?echo I18N("j","Hour");?> ";
					}
					if (time["min"]>1)
					{
						expires += time["min"]+" <?echo I18N("j","Minutes");?>";
					}
					else if (time["min"]>0)
					{
						expires += time["min"]+" <?echo I18N("j","Minute");?>";
					}
				}
				var data	= [host, ipaddr, mac, expires];
				var type	= ["text", "text", "text", "text"];
				if (expires != "Never")
				{									
					TEMP_InjectTable("leases_list", uid, data, type);
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
			
			TEMP_InjectTable("reserves_list", i, data, type);
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
		//var ipaddr = COMM_IPv4NETWORK(OBJ("ipaddr").value, "24");
		var ipaddr = COMM_IPv4NETWORK(TEMP_GetFieldsValue("ipaddr_", "."), "24");
		var maxhost = COMM_IPv4MAXHOST(this.mask)-1;
		var network = ipaddr.substring(0, ipaddr.lastIndexOf('.')+1);
		//var hostid = parseInt(COMM_IPv4HOST(OBJ("ipaddr").value, this.mask), 10);
		var hostid = parseInt(COMM_IPv4HOST(TEMP_GetFieldsValue("ipaddr_", "."), this.mask), 10);
		//var tmp_ip = OBJ("ipaddr").value.substring(OBJ("ipaddr").value.lastIndexOf('.')+1, OBJ("ipaddr").value.length);
		var tmp_ip = OBJ("ipaddr_4").value;
		//var tmp_mask = OBJ("netmask").value.substring(OBJ("netmask").value.lastIndexOf('.')+1, OBJ("netmask").value.length);
		var tmp_mask = OBJ("netmask_4").value;
		var startip = parseInt(OBJ("startip").value, 10) - (tmp_ip & tmp_mask);
		var endip = parseInt(OBJ("endip").value, 10) - (tmp_ip & tmp_mask);

		if (isDomain(OBJ("domain").value))
			XS(this.dhcps4+"/domain", OBJ("domain").value);
		else
		{
			BODY.ShowConfigError("domain", "<?echo I18N("j","Illegal domain name.");?>");
			//alert("<?echo I18N("j","Illegal domain name.");?>");
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
				//alert("<?echo I18N("j","Invalid DHCP IP Address Range.");?>"); 
				BODY.ShowConfigError("startip", "<?echo I18N("j","Invalid DHCP IP Address Range.");?>");
				return false;
			}
			if (hostid>=parseInt(OBJ("startip").value, 10) && hostid<=parseInt(OBJ("endip").value, 10))
			{
				//alert("<?echo I18N("j","The Router IP Address belongs to the lease pool of DHCP server.");?>"); 
				BODY.ShowConfigError("startip", "<?echo I18N("j","The Router IP Address belongs to the lease pool of DHCP server.");?>");
				return false;
			}
			if (!TEMP_IsDigit(OBJ("leasetime").value))
			{
				//alert("<?echo I18N("j","Invalid lease time.");?>"); 
				BODY.ShowConfigError("leasetime", "<?echo I18N("j","Invalid lease time.");?>");
				return false;
			}
			if (this.mask >= 24 && (startip < 1 || endip > maxhost))
			{
				//alert("<?echo I18N("j","DHCP IP Address Range out of boundary.");?>");
				BODY.ShowConfigError("startip", "<?echo I18N("j","DHCP IP Address Range out of boundary.");?>"); 
				return false;
			}
			if (parseInt(OBJ("startip").value, 10) > parseInt(OBJ("endip").value, 10))
			{
				//alert("<?echo I18N("j","The start of DHCP IP Address Range should be smaller than the end.");?>");
				BODY.ShowConfigError("startip", "<?echo I18N("j","The start of DHCP IP Address Range should be smaller than the end.");?>"); 
				return false;
			}
	
			//XS(this.dhcps4_inet+"/ipv4/ipaddr", OBJ("ipaddr").value);
			XS(this.dhcps4_inet+"/ipv4/ipaddr", TEMP_GetFieldsValue("ipaddr_", "."));
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
						//alert("<?echo I18N("j","Invalid MAC address value.");?>");
						//BODY.ShowConfigError("reserv_macaddr_1", "<?echo I18N("j","Invalid MAC address value.");?>");
						BODY.ShowMessage("Error","<?echo I18N("j","Invalid MAC address value.");?>");
						return false;
					}
					
					if (OBJ("en_dhcp_host"+i).innerHTML=="")
					{
						//alert("<?echo I18N("j","Invalid Computer Name");?>");
						BODY.ShowConfigError("reserv_host", "<?echo I18N("j","Invalid Computer Name.");?>");
						return false;
					}
					if (TEMP_CheckNetworkAddr(OBJ("en_dhcp_ipaddr"+i).innerHTML, TEMP_GetFieldsValue("ipaddr_", "."), this.mask))
					//if (TEMP_CheckNetworkAddr(OBJ("en_dhcp_ipaddr"+i).innerHTML, OBJ("ipaddr").value, this.mask))
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
						//alert("<?echo I18N("j","Invalid DHCP reservation IP address");?>");
						BODY.ShowConfigError("reserv_ipaddr_1", "<?echo I18N("j","Invalid DHCP reservation IP address.");?>");
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
				//OBJ("reserv_ipaddr").value  = XG(entry+":"+i+"/ipaddr");
				//OBJ("reserv_macaddr").value = XG(entry+":"+i+"/macaddr");
				TEMP_SetFieldsByDelimit("reserv_ipaddr_",XG(entry+":"+i+"/ipaddr"),".");
				TEMP_SetFieldsByDelimit("reserv_macaddr_",XG(entry+":"+i+"/macaddr"),":");
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
		if(winscount > 0)
		{			
			var win1 = XG(wins+"/entry:1");	
			var win2 = XG(wins+"/entry:2");
			if(win1!="") TEMP_SetFieldsByDelimit("primarywins_",win1,".");
			if(win2!="") TEMP_SetFieldsByDelimit("secondarywins_",win2,".");
		}			
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
									
			/*if(winscount > 0)
			{			
				var win1 = XG(wins+"/entry:1");	
				var win2 = XG(wins+"/entry:2");
				if(win1 != "") OBJ("primarywins").value = win1;
				if(win2 != "") OBJ("secondarywins").value = win2;
			}*/									
		}
		else
		{			
			OBJ("netbios_enable").checked = false;
			OBJ("netbios_learn").disabled = true;
			OBJ("netbios_scope").disabled  = true;	
			this.set_radio_value("winstype", ntype);
			this.action_radio("winstype", "disable");
			/*if(winscount > 0)
			{			
				var win1 = XG(wins+"/entry:1");	
				var win2 = XG(wins+"/entry:2");
				if(win1 != "") OBJ("primarywins").value = win1;
				if(win2 != "") OBJ("secondarywins").value = win2;
			}*/		
			//OBJ("primarywins").disabled = true;
			//OBJ("secondarywins").disabled = true;		
			for(var i=1;i<=4;i++)
			{
				OBJ("primarywins_"+i).disabled   = true;
				OBJ("secondarywins_"+i).disabled = true;		
			}
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
		//var win1 = OBJ("primarywins").value;	
		var win1 = TEMP_GetFieldsValue("primarywins_",".");//primarywins is allowed to be empty,so win1 may be "..."
		if(win1=="...") win1="";
		//var win2 = OBJ("secondarywins").value;
		var win2 = TEMP_GetFieldsValue("secondarywins_",".");
		if(win2=="...") win2="";
		
		if(!OBJ("netbios_learn").checked)
		{
			//if(!this.ip_verify("primarywins")) return false;
			//if(!this.ip_verify("secondarywins")) return false;
			if(!this.ip_verify_("primarywins_","primarywins_1")) return false;
			if(!this.ip_verify_("secondarywins_","secondarywins_1")) return false;
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
			//OBJ("primarywins").disabled   = true;
			//OBJ("secondarywins").disabled = true;		
			for(var i=1;i<=4;i++)
			{
				OBJ("primarywins_"+i).disabled   = true;
				OBJ("secondarywins_"+i).disabled = true;		
			}
		}									
	},
	on_check_learn: function()
	{
		if(OBJ("netbios_learn").checked)
		{										
			this.action_radio("winstype", "disable");			
			OBJ("netbios_scope").disabled = true;
			//OBJ("primarywins").disabled   = true;
			//OBJ("secondarywins").disabled = true;				
			for(var i=1;i<=4;i++)
			{
				OBJ("primarywins_"+i).disabled   = true;
				OBJ("secondarywins_"+i).disabled = true;		
			}
		}
		else
		{			
			this.action_radio("winstype", "enable");
			OBJ("netbios_scope").disabled = false;
			//OBJ("primarywins").disabled   = false;
			//OBJ("secondarywins").disabled = false;	
			for(var i=1;i<=4;i++)
			{
				OBJ("primarywins_"+i).disabled   = false;
				OBJ("secondarywins_"+i).disabled = false;		
			}
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
	ip_verify_: function(name,L_name)
	{
		var objs = document.getElementsByName(name);
		if(TEMP_GetFieldsValue(name, "")=="")
			return true;
		if(TEMP_GetFieldsValue(name, ".")=="127.0.0.1")
		{
			//alert("<?echo I18N("j","Invalid IP address");?>");
			BODY.ShowConfigError(L_name,"<?echo I18N("j","Invalid IP address.");?>");
			OBJ(name+"_1").focus();
			return false;
		}
		for (var i=0; i<4; i++)
		{
			if (!TEMP_IsDigit(objs[i].value) || objs[i].value>255)
			{
				//alert("<?echo I18N("j","Invalid IP address");?>");
				BODY.ShowConfigError(L_name,"<?echo I18N("j","Invalid IP address.");?>");
				objs[i].focus();
				return false;
			}
		}
		return true;
	},
	/*ip_verify: function(name)
	{
		if(OBJ(name).value == "") 
		{
			return true;
		}
		
		if(OBJ(name).value == "127.0.0.1") 
		{
			alert("<?echo I18N("j","Invalid IP address");?>");
			OBJ(name).focus();
			return false;
		}
			
		var vals = OBJ(name).value.split(".");
		
		if (vals.length!=4)
		{
			alert("<?echo I18N("j","Invalid IP address");?>");
			OBJ(name).focus();
			return false;
		}
		
		for (var i=0; i<4; i++)
		{
			if (!TEMP_IsDigit(vals[i]) || vals[i]>=255 || vals[i]< 0)
			{
				alert("<?echo I18N("j","Invalid IP address");?>");
				OBJ(name).focus();
				return false;
			}
		}
		
		return true;	
	},*/
	OnClickMacButton: function()
	{
		TEMP_SetFieldsByDelimit("reserv_macaddr_", "<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>", ':');
	},
	AddDHCPReserv: function()
	{
		BODY.ClearConfigError();
		BODY.ClearConfigError();
		BODY.ClearConfigError();
		var i=0;
		
		if(!this.ip_verify_("reserv_ipaddr_","reserv_ipaddr_1")) return false;
		
		var mac = GetMAC(TEMP_GetFieldsValue("reserv_macaddr_", ":"));
		if (mac=="")
		{
			BODY.ShowConfigError("reserv_macaddr_1", "<?echo I18N("j","Invalid MAC address value.");?>");
			return false;
		}
		if (OBJ("reserv_host").value=="")
		{
			BODY.ShowConfigError("reserv_host", "<?echo I18N("j","Invalid Computer Name.");?>");
			return false;
		}
		if (!TEMP_CheckNetworkAddr(TEMP_GetFieldsValue("reserv_ipaddr_", "."), TEMP_GetFieldsValue("ipaddr_", "."), this.mask))
		{
			BODY.ShowConfigError("reserv_ipaddr_1", "<?echo I18N("j","Invalid DHCP reservation IP address.");?>");
			return false;
		}
		
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
			BODY.ShowMessage("Error","<?echo I18N("j", "The maximum number of permitted DHCP reservations has been exceeded.");?>");
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
		
		TEMP_InjectTable("reserves_list", i, data, type);
		
		OBJ("en_dhcp_reserv"+i).checked = OBJ("en_dhcp_reserv").checked?true:false;
		OBJ("en_dhcp_host"+i).innerHTML = OBJ("reserv_host").value;
		//OBJ("en_dhcp_ipaddr"+i).innerHTML = OBJ("reserv_ipaddr").value;
		//OBJ("en_dhcp_macaddr"+i).innerHTML = OBJ("reserv_macaddr").value;
		OBJ("en_dhcp_ipaddr"+i).innerHTML = TEMP_GetFieldsValue("reserv_ipaddr_", ".");
		OBJ("en_dhcp_macaddr"+i).innerHTML = TEMP_GetFieldsValue("reserv_macaddr_", ":");
				
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
		//OBJ("reserv_ipaddr").value = "";
		//OBJ("reserv_macaddr").value = "";
		for(var i=1;i<=4;i++)
		{
			OBJ("reserv_ipaddr_"+i).value = "";
			OBJ("reserv_macaddr_"+i).value = "";
		}
		
		BODY.ClearConfigError("reserv_host", "");
		BODY.ClearConfigError("reserv_ipaddr_1", "");
		BODY.ClearConfigError("reserv_macaddr_1", "");
		
		OBJ("reserv_macaddr_5").value = OBJ("reserv_macaddr_6").value = "";
		OBJ("mainform").setAttribute("modified", "true");	
	},	
	OnEdit: function(i)
	{
		OBJ("en_dhcp_reserv").checked=OBJ("en_dhcp_reserv"+i).checked;
		OBJ("reserv_host").value=OBJ("en_dhcp_host"+i).innerHTML;
		//OBJ("reserv_ipaddr").value=OBJ("en_dhcp_ipaddr"+i).innerHTML;
		//OBJ("reserv_macaddr").value=OBJ("en_dhcp_macaddr"+i).innerHTML;
		TEMP_SetFieldsByDelimit("reserv_ipaddr_", OBJ("en_dhcp_ipaddr"+i).innerHTML, ".");
		TEMP_SetFieldsByDelimit("reserv_macaddr_", OBJ("en_dhcp_macaddr"+i).innerHTML, ":");
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
	},
	IsNumber: function(str)
	{
	   var num = "0123456789";
	   var isnum=true;
	   var c;
		 
	   for (i = 0; i < str.length; i++) 
	   { 	   
	      c = str.charAt(i); 
	      if (num.indexOf(c) == -1) 
	      {	         	
	         	isnum = false;
	         	return isnum;
	      }
	   }
	   return isnum;
   }
}

function FocusObj(result)
{
	var found = true;
	var node = result.Get("/hedwig/node");
	var msg = result.Get("/hedwig/message");
	var nArray = node.split("/");
	var len = nArray.length;
	var name = nArray[len-1];
	if (node.match("inet"))
	{
		switch (name)
		{
		case "ipaddr":
			OBJ("ipaddr_1").focus();
			BODY.ShowConfigError("ipaddr_1", msg);
			break;
		case "mask":
			OBJ("netmask_1").focus();
			BODY.ShowConfigError("netmask_1", msg);
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
			BODY.ShowConfigError("startip", msg);
			break;
		case "end":
			OBJ("endip").focus();
			BODY.ShowConfigError("startip", msg);
			break;
		case "leasetime":
			OBJ("leasetime").focus();
			BODY.ShowConfigError("leasetime", msg);
			break;
		case "hostid":
			OBJ("reserv_ipaddr_1").focus();
			BODY.ShowConfigError("reserv_ipaddr_1", msg);
			break;
		case "macaddr":
			OBJ("reserv_macaddr_1").focus();
			BODY.ShowConfigError("reserv_macaddr_1", msg);
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
	BODY.ShowMessage("Error",msg);

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
	var banner = "<?echo I18N("j","Rebooting");?>...";
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
			BODY.ShowMessage("Error","Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		else
			BODY.ShowCountdown(banner, msgArray, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}
</script>
