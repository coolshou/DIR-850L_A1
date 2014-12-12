<?include "/htdocs/phplib/phyinf.php";?>
<script type="text/javascript">

function Page() {}
Page.prototype =
{
	services: "DEVICE.RDNSS,WAN,INET.WAN-1,INET.LAN-3,INET.LAN-4,INET.WAN-3,INET.WAN-4, INET.WAN-5, DHCPS6.LAN-4,RUNTIME.PHYINF,RUNTIME.INF.WAN-1,RUNTIME.INF.LAN-4,RUNTIME.INF.LAN-6,RUNTIME.INF.WAN-3,RUNTIME.INF.WAN-4",
	OnLoad: function()
	{
		if (!this.rgmode)		{BODY.DisableCfgElements(true);}
	},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},

	InitValue: function(xml)
	{
		PXML.doc = xml;

		//PXML.doc.dbgdump();

		this.ParseAll();

		if (!this.InitWANConnMode()) return false;
		if (!this.InitLANConnMode()) return false;
		if (!this.InitWANInfo()) return false;
		if (!this.InitWANLLInfo()) return false;
		if (!this.InitLANInfo()) return false;
		if (!this.InitLANLLInfo()) return false;
		if (!this.InitLANAutoConf()) return false;
		if (!this.InitLANPdConf()) return false;
		if (!this.InitDHCPS6()) return false;
		this.OnChangewan_ipv6_mode();	
		this.OnChangelan_auto_type();
		
		return true;
	},
	PreSubmit: function()
	{
		if (!this.PreWAN()) return null;
		if (!this.PreLAN()) return null;
		if (!this.PreLANAutoConf()) return null;
			
		XS(this.rdnss+"/device/v6modechange", "1");
		
		if(!OBJ("enableAuto").checked)
			PXML.IgnoreModule("DHCPS6.LAN-4");

		/* Need to delay 20s to let wan start before lan because we don't have dns relay */
		if(!OBJ("en_dhcp_pd").checked)
			PXML.DelayActiveModule("INET.LAN-4", "0");
		if(OBJ("wan_ipv6_mode").value=="STATIC")
			PXML.DelayActiveModule("INET.LAN-4", "0");

		PXML.IgnoreModule("INET.LAN-3");
		PXML.CheckModule("INET.WAN-1", null, null, "ignore");
		//PXML.CheckModule("INET.LAN-4", null, null, null);
		PXML.CheckModule("INET.WAN-3", null, null, "ignore");
		PXML.CheckModule("INET.WAN-4", null, null, "ignore");
		PXML.CheckModule("INET.WAN-5", null, null, "ignore");//20110628
		PXML.CheckModule("DEVICE.RDNSS", null, null, "ignore");

		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		var rwan = PXML.FindModule("RUNTIME.INF.WAN-1");
        var addrtype = XG(rwan+"/runtime/inf/inet/addrtype");
		var addr;
		if(addrtype == "ipv4") //static ip or dhcp
			addr = XG(rwan+"/runtime/inf/inet/ipv4/ipaddr");
		else if(addrtype == "ppp4") //ppp
			addr = XG(rwan+"/runtime/inf/inet/ppp4/local");
						
		PXML.IgnoreModule("RUNTIME.PHYINF");
		PXML.IgnoreModule("RUNTIME.INF.WAN-1");
		PXML.IgnoreModule("RUNTIME.INF.WAN-3");
		PXML.IgnoreModule("RUNTIME.INF.WAN-4");
		PXML.IgnoreModule("RUNTIME.INF.LAN-4");
		PXML.IgnoreModule("RUNTIME.INF.LAN-6");

		PXML.CheckModule("DHCPS6.LAN-4", null, null, "ignore");//rbj
		PXML.CheckModule("WAN", null, null, "ignore");
		
		PXML.EventActiveModule("WAN", "WAN.RESTART", 0);
	
		//PXML.doc.dbgdump();
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},

	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////

	/*********************************************************
	    lanact, lanllact, wanact and wanllact:
		this architecture existe at least one drawback:
		1. only accept one active wan, wanll, lan, lanll.
		2. 
	*********************************************************/
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	laninfo: [  {svcsname: "INET.LAN-3", svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
			{svcsname: "INET.LAN-4", svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null }
		],
	waninfo: [  {svcsname: "INET.WAN-4", svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
			{svcsname: "INET.WAN-3", svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null }
		],
	lanact:   { svcsname: null, svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
	lanllact: { svcsname: null, svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
	wanact:   { svcsname: null, svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
	wanllact: { svcsname: null, svcs: null, inetuid: null, inetp: null, phyinf: null, ipv6ll: null },
	rwan: null,
	rwan1: null,
	rwan3: null,
	rlan: null,
	rlan6: null,
        dhcps6: null,
        dhcps6_inet: null,
        dhcps6_inf: null,
	rdnss: null,
	hintsupport: null,
        wan1:     { infp: null, inetp: null},
        wan5:     { infp: null, inetp: null},//for AUTODETECT mode

	OnChangewan_ipv6_mode: function()
	{
		OBJ("box_wan_title").style.display			= "none";
		OBJ("box_wan_static_body").style.display	= "none";
		OBJ("box_wan_pppoe").style.display			= "none";
		OBJ("box_wan_pppoe_body").style.display		= "none";
		OBJ("bbox_wan_dns").style.display			= "none";
		OBJ("box_wan_6to4_body").style.display		= "none";
		OBJ("box_wan_tunnel").style.display			= "none";
		OBJ("box_wan_tunnel_body").style.display	= "none";
		OBJ("box_wan_6rd_body").style.display	= "none";
		
		//OBJ("box_wan_ll_body").style.display		= "none";
		OBJ("box_lan").style.display				= "none";
		OBJ("box_lan_ul").style.display				= "none";
		OBJ("box_lan_body").style.display	= "none";
		OBJ("box_lan_pd_body").style.display		= "none";
		OBJ("box_lan_pd_hint_body").style.display		= "none";
		OBJ("box_lan_ll_body").style.display 		= "none";
		OBJ("box_lan_ul_body").style.display 		= "none";
		OBJ("box_lan_auto").style.display 			= "none";
		OBJ("box_lan_auto_body").style.display 		= "none";
		OBJ("box_lan_auto_pd").style.display 			= "none";
		OBJ("box_lan_auto_pd_body").style.display 		= "none";
		OBJ("bbox_wan").style.display 	= "none";
		OBJ("bbox_lan_auto").style.display 	= "none";
		OBJ("sp_dli_s").innerHTML = "::00";
		OBJ("sp_dli_e").innerHTML = "::00";
		OBJ("ra_lifetime").disabled = true;
		OBJ("ip_lifetime").disabled = true;
		OBJ("w_6rd_v4addr").disabled = true;
		//OBJ("ra_lifetime").value = "";
		//OBJ("ip_lifetime").value = "";
		//OBJ("w_dhcp_dns_auto").checked	= true;
		OBJ("l_span_6rd").style.display 			= "none";
		OBJ("l_span_6to4").style.display 		= "none";
		OBJ("l_span").style.display 			= "none";
		var wan3 = PXML.FindModule("INET.WAN-3");//20100614

        	this.OnClickpd();
		if(this.hintsupport == "1")
		{
			this.OnClickpdhint();
		}
		this.OnClickDHCPDNS();
		switch(OBJ("wan_ipv6_mode").value)
		{
			case "STATIC":
				OBJ("box_wan_title").style.display = "";
				OBJ("box_wan_static_body").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				OBJ("box_lan_auto_pd").style.display = "none";
				OBJ("box_lan_auto_pd_body").style.display = "none";
        			OBJ("bbox_wan").style.display 	= "";
        			OBJ("bbox_lan_auto").style.display 	= "";
        			OBJ("sp_dli_s").innerHTML = ":00";
        			OBJ("sp_dli_e").innerHTML = ":00";
				OBJ("l_ipaddr").disabled = false;
				OBJ("ra_lifetime").disabled = false;
				OBJ("ip_lifetime").disabled = false;
				OBJ("l_span").style.display = "";
				this.InitWANStaticValue();
				break;
	
			case "AUTO":
			case "AUTODETECT":
			//case "DHCP":
			//case "RA":
				OBJ("bbox_wan_dns").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				//OBJ("span_dsc2").style.display = "";
				OBJ("box_lan_pd_body").style.display = "";
				if(this.hintsupport == "1")
				{
					OBJ("box_lan_pd_hint_body").style.display = "";
				}
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				//OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				//OBJ("box_lan_auto_pd").style.display = "";
				//OBJ("box_lan_auto_pd_body").style.display = "";
                		OBJ("bbox_lan_auto").style.display 	= "";
				
				//OBJ("w_dhcp_dns_auto").checked	= true;
				OBJ("w_dhcp_pdns").disabled = OBJ("w_dhcp_sdns").disabled = OBJ("w_dhcp_dns_auto").checked ? true: false;
				OBJ("ra_lifetime").value = "";
				OBJ("ip_lifetime").value = "";
				OBJ("l_span").style.display = "";
				break;

			case "PPPOE":
				OBJ("box_wan_pppoe").style.display = "";
				OBJ("box_wan_pppoe_body").style.display = "";
				OBJ("bbox_wan_dns").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				//OBJ("span_dsc2").style.display = "";
				OBJ("box_lan_pd_body").style.display = "";
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				//OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				OBJ("box_lan_auto_pd").style.display = "";
				OBJ("box_lan_auto_pd_body").style.display = "";
                		OBJ("bbox_wan").style.display 	= "";
                		OBJ("bbox_lan_auto").style.display 	= "";

				/* rbj, if ppp10 some ipv6 settings regarding pppoe will follow ipv4 */
				if(OBJ("pppoe_sess_share").checked)
				{
					OBJ("pppoe_username").disabled = true;
					OBJ("pppoe_password").disabled = true;
					OBJ("confirm_pppoe_password").disabled = true;
					OBJ("pppoe_service_name").disabled = true;
					OBJ("pppoe_alwayson").disabled = true;
					//OBJ("pppoe_ondemand").disabled = true;
					OBJ("pppoe_manual").disabled = true;
				}
				else
				{
					OBJ("pppoe_username").disabled = false;
					OBJ("pppoe_password").disabled = false;
					OBJ("confirm_pppoe_password").disabled = false;
					OBJ("pppoe_service_name").disabled = false;
					OBJ("pppoe_alwayson").disabled = false;
					//OBJ("pppoe_ondemand").disabled = true;
					OBJ("pppoe_manual").disabled = false;
				}

				//+++ Jerry Kao, Added pppoe_max_idle_time icon, and disable on denamd
				//               in "Share with v4" and "Create new session" modes. 
				OBJ("pppoe6_max_idle_time").disabled = true;
				OBJ("pppoe_ondemand").disabled = true;

				if(XG(this.wan1.inetp+"/addrtype")=="ppp10")
                        		if(XG(this.wan1.inetp+"/ppp6/mtu")=="") OBJ("ppp6_mtu").value = "1492";
				else if(XG(this.wanllact.inetp+"/addrtype")=="ppp6")
                        		if(XG(this.wanllact.inetp+"/ppp6/mtu")=="") OBJ("ppp6_mtu").value = "1492";
				
				//OBJ("w_dhcp_dns_auto").checked	= true;
				OBJ("w_dhcp_pdns").disabled = OBJ("w_dhcp_sdns").disabled = OBJ("w_dhcp_dns_auto").checked ? true: false;
				OBJ("ra_lifetime").value = "";
				OBJ("ip_lifetime").value = "";
				this.OnClickPppoeAddrType();
				OBJ("l_span").style.display = "";
			
				var over = XG(this.wan1.inetp+"/ppp4/over");	
                		if(OBJ("pppoe_sess_share").checked && over=="eth")
					this.LoadPpp4Value();
				break;

			case "6IN4":
				OBJ("box_wan_tunnel").style.display			= "";
				OBJ("box_wan_tunnel_body").style.display	= "";
				OBJ("bbox_wan_dns").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				//OBJ("span_dsc2").style.display = "";
				OBJ("box_lan_pd_body").style.display = "";
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				OBJ("box_lan_auto_pd").style.display = "none";
				//OBJ("box_lan_auto_pd_body").style.display = "none";
                		OBJ("bbox_wan").style.display 	= "";
                		OBJ("bbox_lan_auto").style.display 	= "";
				//OBJ("l_ipaddr").disabled = false;
				OBJ("ra_lifetime").disabled = false;
				OBJ("ip_lifetime").disabled = false;
				//OBJ("w_dhcp_dns_auto").checked	= true;
				OBJ("w_dhcp_pdns").disabled = OBJ("w_dhcp_sdns").disabled = OBJ("w_dhcp_dns_auto").checked ? true: false;
				OBJ("l_span").style.display = "";
				break;

			case "6RD":
				OBJ("box_wan_title").style.display = "";
				OBJ("box_wan_6rd_body").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				OBJ("box_lan_auto_pd").style.display = "none";
				OBJ("box_lan_auto_pd_body").style.display = "none";
                		OBJ("bbox_wan").style.display 	= "";
                		OBJ("bbox_lan_auto").style.display 	= "";
				OBJ("ra_lifetime").value = "";
				OBJ("ip_lifetime").value = "";
				OBJ("w_6rd_prefix_1").disabled = OBJ("w_6rd_prefix_2").disabled = OBJ("w_6rd_v4addr_mask").disabled  = OBJ("w_6rd_relay").disabled = OBJ("6rd_dhcp_option").checked ? true: false;
				OBJ("l_span_6rd").style.display = "";
				break;
      
			case "6TO4":
				OBJ("box_wan_title").style.display = "";
				OBJ("box_wan_6to4_body").style.display = "";
				OBJ("box_lan").style.display = "";
				OBJ("span_dsc1").style.display = "";
				OBJ("box_lan_body").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				OBJ("box_lan_auto").style.display = "";
				OBJ("box_lan_auto_body").style.display = "";
				OBJ("box_lan_auto_pd").style.display = "none";
				OBJ("box_lan_auto_pd_body").style.display = "none";
                		OBJ("bbox_wan").style.display 	= "";
                		OBJ("bbox_lan_auto").style.display 	= "";
				OBJ("ra_lifetime").value = "";
				OBJ("ip_lifetime").value = "";
				OBJ("l_span_6to4").style.display = "";
				break;
      
			case "LL":
				OBJ("box_lan").style.display = "";
				OBJ("box_lan_ll_body").style.display = "";
				OBJ("span_dsc1").style.display = "none";
				//OBJ("span_dsc2").style.display = "none";
				if(XG(this.rlan6+"/runtime/inf/inet/ipv6/ipaddr")!="")
				{
					OBJ("box_lan_ul_body").style.display = "";
				}
                		break;
		}
	},

	
	OnChangelan_auto_type: function()
	{
		OBJ("box_lan_dhcp").style.display  = "none";
        	OBJ("box_lan_stless").style.display = "none";
		switch(OBJ("lan_auto_type").value)
		{
			case "STATELESSR":
			case "STATELESSD":
			      	OBJ("box_lan_dhcp").style.display = "none";
                  	      	OBJ("box_lan_stless").style.display = "";
	              	      	break;
				
			case "STATEFUL":
			      	OBJ("box_lan_dhcp").style.display = "";
                  		OBJ("dhcps_start_ip_prefix").disabled = true;
                  		OBJ("dhcps_stop_ip_prefix").disabled = true;
                  		this.ShowDHCPS6();
	              		break;
		}		
		this.InitLANPdConf();
	},
    	OnClickpd: function()
    	{
		if(OBJ("en_dhcp_pd").checked)
		{
			OBJ("l_ipaddr").disabled = true;
			OBJ("box_lan_auto_pd_body").style.display = "";
			OBJ("box_lan_auto").style.display = "none";
			OBJ("box_lan_auto_pd").style.display = "";
		}
		else
	    	{
			OBJ("l_ipaddr").disabled = false;
			OBJ("box_lan_auto_pd_body").style.display = "none";
			OBJ("box_lan_auto").style.display = "";
			OBJ("box_lan_auto_pd").style.display = "none";
			OBJ("en_lan_pd").checked = false;
			XS(this.dhcps6+"/pd/enable", "0");

			if(this.hintsupport == "1")
			{
				OBJ("en_dhcp_pd_hint").checked = false;
				OBJ("dhcppd_hint_prefix").disabled =
				OBJ("dhcppd_hint_pfxlen").disabled = 
				OBJ("dhcppd_hint_plft").disabled = 
				OBJ("dhcppd_hint_vlft").disabled = true;
			}
		}
    	},

	OnClickpdhint: function()
        {
		OBJ("dhcppd_hint_prefix").disabled =
		OBJ("dhcppd_hint_pfxlen").disabled = 
		OBJ("dhcppd_hint_plft").disabled = 
		OBJ("dhcppd_hint_vlft").disabled = OBJ("en_dhcp_pd_hint").checked? false:true;
        },

    	OnClickAuto: function()
    	{
		if(OBJ("enableAuto").checked)
		{
			OBJ("lan_auto_type").disabled = false;
			OBJ("en_lan_pd").disabled = false;
			OBJ("en_lan_pd").checked = true;
			if(OBJ("lan_auto_type").value == "STATEFUL")
				OBJ("box_lan_dhcp").style.display = "";
			else
				OBJ("box_lan_stless").style.display = "";
		}
		else
		{
			OBJ("lan_auto_type").disabled = true;
			OBJ("en_lan_pd").checked = false;
			OBJ("en_lan_pd").disabled = true;
			if(OBJ("lan_auto_type").value == "STATEFUL")
				OBJ("box_lan_dhcp").style.display = "none";
			else
				OBJ("box_lan_stless").style.display = "none";
		}
    	},

    	OnClickLanpd: function()
    	{
    	},

    	OnClick6rdHubSpoke: function()
    	{
    	},

    	OnClick6rdDHCPOPT: function()
    	{
			OBJ("w_6rd_prefix_1").disabled = OBJ("w_6rd_prefix_2").disabled = OBJ("w_6rd_v4addr_mask").disabled = OBJ("w_6rd_relay").disabled = OBJ("6rd_dhcp_option").checked ? true: false;
    	
			//+++ Jerry Kao, set default IPv4 Mask Length = 0.
			if (OBJ("6rd_manual").checked)
			{
				if (XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask") != 0)
					OBJ("w_6rd_v4addr_mask").value = XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask");
				else
					OBJ("w_6rd_v4addr_mask").value = 0;
			}
			else
				OBJ("w_6rd_v4addr_mask").value = "";
    	},

	ShowDHCPS6: function()
    	{
        	var str;
        	var inflp = PXML.FindModule("RUNTIME.INF.LAN-4");
       
		str = XG(this.dhcps6+"/start");
		if (str) 
		{
			index = str.lastIndexOf(":");
        		OBJ("dhcps_start_ip_value").value = str.substring(index+1);
		}
 
		str = str+XG(this.dhcps6+"/count");
		if (str) 
		{
			str1 = OBJ("dhcps_start_ip_value").value;
			str1 = parseInt(str1,16);
			str2 = XG(this.dhcps6+"/count");
			str2 = parseInt(str1,10) + parseInt(str2,10) - parseInt("1",10);
			str2 = Dec2Hex(str2);
			str2 = str2 + "";
			OBJ("dhcps_stop_ip_value").value = str2;
		} 
        	//OBJ("dhcps_start_ip_value").value = XG(this.dhcps6+"/start");
        	//OBJ("dhcps_stop_ip_value").value = XG(this.dhcps6+"/stop");
        	str = XG(inflp+"/runtime/inf/dhcps6/network");
        	if (str)
        	{
              		index = str.lastIndexOf("::");
              		OBJ("dhcps_start_ip_prefix").value = str.substring(0,index);
              		OBJ("dhcps_stop_ip_prefix").value = str.substring(0,index);
        	}
        	else
        	{
              		OBJ("dhcps_start_ip_prefix").value = "xxxx";
              		OBJ("dhcps_stop_ip_prefix").value = "xxxx";
        	}
        	OBJ("dhcps_start_ip_prefix").disabled = true;
        	OBJ("dhcps_stop_ip_prefix").disabled = true;
    	},
	
	/* Get lanact, lanllact, wanact and wanllact */
	ParseAll: function()
	{
        	var svc = PXML.FindModule("DHCPS6.LAN-4");

		//alert("length"+this.laninfo.length);
		for ( var lidx=0; lidx < this.laninfo.length; lidx++)
		{
			this.laninfo[lidx].svcs		= GPBT("/postxml", "module", "service", this.laninfo[lidx].svcsname, false);
			this.laninfo[lidx].inetuid	= XG(this.laninfo[lidx].svcs+"/inf/inet");
			this.laninfo[lidx].inetp	= GPBT(this.laninfo[lidx].svcs+"/inet", "entry", "uid", this.laninfo[lidx].inetuid, false);
			this.laninfo[lidx].phyinf	= XG(this.laninfo[lidx].svcs+"/inf/phyinf");
			var tRTPHYsvcs			= GPBT("/postxml", "module", "service", "RUNTIME.PHYINF", false);
			var tRTPHYphyinf		= GPBT(tRTPHYsvcs+"/runtime", "phyinf", "uid", this.laninfo[lidx].phyinf);
			/*this.laninfo[lidx].ipv6ll	= XG(tRTPHYphyinf+"/ipv6ll");*/
			this.laninfo[lidx].ipv6ll	= XG(tRTPHYphyinf+"/ipv6/link/ipaddr");

			//this.MyArrayShowAlert("laninfo", this.laninfo[lidx]);

			if ( XG(this.laninfo[lidx].svcs+"/inf/active") == "1" )
			{
				if ( XG(this.laninfo[lidx].inetp+"/ipv6/mode") == "LL" )
					this.FillArrayData( this.lanllact, this.laninfo[lidx] ); 
				else
					this.FillArrayData( this.lanact, this.laninfo[lidx] );
			}
		}

		for ( var widx=0; widx < this.waninfo.length; widx++)
		{
			this.waninfo[widx].svcs		= GPBT("/postxml", "module", "service", this.waninfo[widx].svcsname, false);
			this.waninfo[widx].inetuid	= XG(this.waninfo[widx].svcs+"/inf/inet");
			this.waninfo[widx].inetp	= GPBT(this.waninfo[widx].svcs+"/inet", "entry", "uid", this.waninfo[widx].inetuid, false);
			this.waninfo[widx].phyinf	= XG(this.waninfo[widx].svcs+"/inf/phyinf");
			var tRTPHYsvcs			= GPBT("/postxml", "module", "service", "RUNTIME.PHYINF", false);
			var tRTPHYphyinf		= GPBT(tRTPHYsvcs+"/runtime", "phyinf", "uid", this.waninfo[widx].phyinf);
			/*this.waninfo[widx].ipv6ll	= XG(tRTPHYphyinf+"/ipv6ll");*/
			this.waninfo[widx].ipv6ll	= XG(tRTPHYphyinf+"/ipv6/link/ipaddr");

			//this.MyArrayShowAlert("laninfo", this.waninfo[widx]);

			if ( XG(this.waninfo[widx].svcs+"/inf/active") == "1" )
			{
				if ( XG(this.waninfo[widx].inetp+"/ipv6/mode") == "LL" )
					this.FillArrayData( this.wanllact, this.waninfo[widx] ); 
				else
				{
					if ( this.waninfo[widx].svcsname == "INET.WAN-3" )
						this.FillArrayData( this.wanllact, this.waninfo[widx] ); 
					else
						this.FillArrayData( this.wanact, this.waninfo[widx] );
				}
			}
		}

    	this.dhcps6 = GPBT(svc+"/dhcps6", "entry", "uid", "DHCPS6-1", false);
    	this.dhcps6_inf  = GPBT(svc, "inf", "uid", "LAN-4", false);
    	this.dhcps6_inet = svc + "/inet/entry";

        var base = PXML.FindModule("INET.WAN-1");
        this.wan1.infp = GPBT(base, "inf", "uid", "WAN-1", false);
        this.wan1.inetp = GPBT(base+"/inet", "entry", "uid", XG(this.wan1.infp+"/inet"), false);
        var base = PXML.FindModule("INET.WAN-5");
        this.wan5.infp = GPBT(base, "inf", "uid", "WAN-5", false);
        this.wan5.inetp = GPBT(base+"/inet", "entry", "uid", XG(this.wan5.infp+"/inet"), false);
		this.rwan1 = PXML.FindModule("RUNTIME.INF.WAN-1");
		this.rwan3 = PXML.FindModule("RUNTIME.INF.WAN-3");
		this.rwan = PXML.FindModule("RUNTIME.INF.WAN-4");
		this.rlan = PXML.FindModule("RUNTIME.INF.LAN-4");
		this.rlan6 = PXML.FindModule("RUNTIME.INF.LAN-6");
		this.rdnss = PXML.FindModule("DEVICE.RDNSS");
		//this.MyArrayShowAlert("lanact", this.lanact);
		//this.MyArrayShowAlert("lanllact", this.lanllact);
		//this.MyArrayShowAlert("wanact", this.wanact);
		//this.MyArrayShowAlert("wanllact", this.wanllact);

	//support hint?
	this.hintsupport = XG(this.rdnss+"/device/dhcp6hint");
	
	},
	FillArrayData: function( to, from)
	{
		to.svcsname	= from.svcsname;
		to.svcs		= from.svcs;
		to.inetuid	= from.inetuid;
		to.inetp	= from.inetp;
		to.phyinf	= from.phyinf;
		to.ipv6ll	= from.ipv6ll;
	},

	InitWANConnMode: function()
	{
		var is_autodetect = XG(this.wan5.infp+"/active");
		if(is_autodetect=="1")
		{
			COMM_SetSelectValue(OBJ("wan_ipv6_mode"), "AUTODETECT");
			return true;
		}
		if( this.wanact.svcsname != null )
		{
			var addrtype1 = XG(this.wan1.inetp+"/addrtype");
			var addrtype2 = XG(this.wanllact.inetp+"/addrtype");
			if(addrtype1 == "ppp10" || addrtype2 == "ppp6")
				COMM_SetSelectValue(OBJ("wan_ipv6_mode"), "PPPOE");
			else
			{
				var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
				var wanactive = XG(this.wanact.svcs+"/inf/active");  
				if(wanmode != "" && wanactive !="0")
				{			 
					COMM_SetSelectValue(OBJ("wan_ipv6_mode"), wanmode);
				}
				else
					COMM_SetSelectValue(OBJ("wan_ipv6_mode"), "LL");
			}
		}
		else 
			COMM_SetSelectValue(OBJ("wan_ipv6_mode"), "LL");
		return true;
	},
	InitWANStaticValue: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		//if(wanmode != "STATIC" && wanmode !="") return true;
		
		str = XG(this.wanact.inetp+"/ipv6/ipaddr");
		if(wanmode != "STATIC" && str !="") return true;

		OBJ("l_ipaddr").disabled = false;
		//OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");

		str = XG(this.wanact.inetp+"/ipv6/ipaddr");
        	if (str)
        	{
              		//index = str.lastIndexOf("::");
              		//str1 = str.substring(0,index);
			//if(str1 == "fe80")
			if(str == this.wanllact.ipv6ll)
			{
              			OBJ("usell").checked = true;
				OBJ("w_st_ipaddr").disabled = true;
				OBJ("w_st_pl").disabled = true;
			}
			else
			{
              			OBJ("usell").checked = false;
				OBJ("w_st_ipaddr").disabled = false;
				OBJ("w_st_pl").disabled = false;
			}
			OBJ("w_st_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/ipaddr");
			OBJ("w_st_pl").value		= XG(this.wanact.inetp+"/ipv6/prefix");
        	}
		else
		{
			//rwan3 = PXML.FindModule("RUNTIME.INF.WAN-3");
                	//addrtype = XG(rwan3+"/runtime/inf/inet/addrtype");
                	//mode = XG(rwan3+"/runtime/inf/inet/ipv6/mode");
			//if(addrtype=="ipv6" && mode=="LL")
			if(1)
			{
                		//ipaddr = XG(rwan3+"/runtime/inf/inet/ipv6/ipaddr");
				if( this.wanllact.svcsname == null ) /* ppp6+4 */
				{
					var tRTPHYsvcs			= GPBT("/postxml", "module", "service", "RUNTIME.PHYINF", false);
					var tRTPHYphyinf		= GPBT(tRTPHYsvcs+"/runtime", "phyinf", "uid", "ETH-3");
					this.wanllact.ipv6ll		= XG(tRTPHYphyinf+"/ipv6/link/ipaddr");
				}
              			OBJ("usell").checked = true;
				OBJ("w_st_ipaddr").disabled = true;
				OBJ("w_st_pl").disabled = true;
				//OBJ("w_st_ipaddr").value	= XG(rwan3+"/runtime/inf/inet/ipv6/ipaddr");
				OBJ("w_st_ipaddr").value	= this.wanllact.ipv6ll;
				//OBJ("w_st_pl").value		= XG(rwan3+"/runtime/inf/inet/ipv6/prefix");
				OBJ("w_st_pl").value		= 64;
			}
		}

		OBJ("w_st_gw").value		= XG(this.wanact.inetp+"/ipv6/gateway");
		OBJ("w_st_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
		OBJ("w_st_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		return true;
	},
	InitWANDHCPValue: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		this.rlan = PXML.FindModule("RUNTIME.INF.LAN-4");
		if(wanmode != "DHCP") return true;

		SetRadioValue("w_dhcp_dns_rad", "auto")

		//var wan4 = PXML.FindModule("INET.WAN-4");
		if(XG(this.wanact.inetp+"/ipv6/dhcpopt") != "IA-NA")
		{		
			OBJ("en_dhcp_pd").checked = true;
			OBJ("l_ipaddr").disabled = true;
			OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
		}
                else
		{
			OBJ("en_dhcp_pd").checked = false;
			OBJ("l_ipaddr").disabled = false;
			OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
		}

		var count = XG(this.wanact.inetp+"/ipv6/dns/count");
		if(count > 0)
		{
			OBJ("w_dhcp_dns_auto").checked = false;
			SetRadioValue("w_dhcp_dns_rad", "manual")
			OBJ("w_dhcp_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
			if(count > 1)
                               OBJ("w_dhcp_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		}
		return true;
	},
	InitWANAUTOValue: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		this.rlan = PXML.FindModule("RUNTIME.INF.LAN-4");
		if(wanmode != "AUTO") return true;

		var addrtype_wan1 = XG(this.wan1.inetp+"/addrtype");
		var addrtype_wan3 = XG(this.wanllact.inetp+"/addrtype");
		if(addrtype_wan1 == "ppp10" || addrtype_wan3 == "ppp6") return true;

		SetRadioValue("w_dhcp_dns_rad", "auto")

		//var wan4 = PXML.FindModule("INET.WAN-4");
		if (XG(this.wanact.svcs+"/inf/child") != "")
		{		
			OBJ("en_dhcp_pd").checked = true;
			OBJ("l_ipaddr").disabled = true;
			OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
		}
		else
		{
			OBJ("en_dhcp_pd").checked = false;
			OBJ("l_ipaddr").disabled = false;
			OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
		}

		if(this.hintsupport == "1")
		{
			if(XG(this.wanact.inetp+"/ipv6/pdhint/enable") == "1")
			{
				OBJ("en_dhcp_pd_hint").checked = true;
				OBJ("dhcppd_hint_prefix").value = XG(this.wanact.inetp+"/ipv6/pdhint/network");
				OBJ("dhcppd_hint_pfxlen").value = XG(this.wanact.inetp+"/ipv6/pdhint/prefix");
				OBJ("dhcppd_hint_plft").value = XG(this.wanact.inetp+"/ipv6/pdhint/preferlft")/60;
				if(OBJ("dhcppd_hint_vlft").value!="")
				{
					OBJ("dhcppd_hint_vlft").value = XG(this.wanact.inetp+"/ipv6/pdhint/validlft")/60;
				}
			}
			else
			{
				OBJ("en_dhcp_pd_hint").checked = false;
				OBJ("dhcppd_hint_prefix").disabled = OBJ("dhcppd_hint_pfxlen").disabled = OBJ("dhcppd_hint_plft").disable 
			= OBJ("dhcppd_hint_vlft").disabled = true;
			}
		}

		var count = XG(this.wanact.inetp+"/ipv6/dns/count");
		if(count > 0)
		{
			OBJ("w_dhcp_dns_auto").checked = false;
			SetRadioValue("w_dhcp_dns_rad", "manual")
			OBJ("w_dhcp_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
			if(count > 1)
				OBJ("w_dhcp_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		}
		else
		{
			OBJ("w_dhcp_dns_auto").checked = true;
			OBJ("w_dhcp_pdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:1");
			OBJ("w_dhcp_sdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:2");
		}
		return true;
	},
	InitWANPPPoEValue: function()
	{
		var addrtype_wan1 = XG(this.wan1.inetp+"/addrtype");
		var addrtype_wan3 = XG(this.wanllact.inetp+"/addrtype");
		if(addrtype_wan1 != "ppp10" && addrtype_wan3 != "ppp6") return true;

		var wan4 = PXML.FindModule("INET.WAN-4");
		var child = XG(wan4+"/inf/child");

        	OBJ("pppoe_dynamic").checked        = true;
        	OBJ("pppoe_ipaddr").value           = "";
       		OBJ("pppoe_sess_share").checked        = true;
        	OBJ("pppoe_username").value         = "";
        	OBJ("pppoe_password").value         = "";
        	OBJ("confirm_pppoe_password").value = "";
        	OBJ("pppoe_service_name").value     = "";
        	//OBJ("pppoe_ondemand").check         = true;
        	//OBJ("pppoe_max_idle_time").value    = "";
               
		if(addrtype_wan1 == "ppp10")
		{
                	if(XG(this.wan1.inetp+"/ppp6/static") === "1")
			{
				OBJ("pppoe_static").checked = true;
                		//OBJ("pppoe_ipaddr").value           = XG(this.wanllact.inetp+"/ppp6/ipaddr");
                		test  = XG(this.wanact.inetp+"/ipv6/ipaddr");
                		OBJ("pppoe_ipaddr").value           = XG(this.wan1.inetp+"/ppp6/ipaddr");
			}
                	else
			{              
				OBJ("pppoe_dynamic").checked = true;
                		OBJ("pppoe_ipaddr").value           = XG(this.rwan+"/runtime/inf/inet/ipv6/ipaddr");
			}
                	OBJ("pppoe_username").value         = XG(this.wan1.inetp+"/ppp6/username");
                	OBJ("pppoe_password").value         = XG(this.wan1.inetp+"/ppp6/password");
                	OBJ("confirm_pppoe_password").value = XG(this.wan1.inetp+"/ppp6/password");
			OBJ("ppp6_mtu").value = XG(this.wan1.inetp+"/ppp6/mtu");
                	OBJ("pppoe_service_name").value     = XG(this.wan1.inetp+"/ppp6/pppoe/servicename");
                	OBJ("pppoe_sess_share").checked        = true;
			
			var dialup   = XG(this.wan1.inetp+"/ppp6/dialup/mode");
			if(dialup === "manual")		OBJ("pppoe_manual").checked = true;
			else						OBJ("pppoe_alwayson").checked = true;
			/*if(dialup === "auto")             OBJ("pppoe_alwayson").checked = true;
			else if(dialup === "manual")      OBJ("pppoe_manual").checked = true;
			else                              OBJ("pppoe_ondemand").checked = true;*/
			
                	//OBJ("pppoe_max_idle_time").value   = XG(this.wan1.inetp+"/ppp6/dialup/idletimeout");
			/* DNS */
			if (XG(this.wan1.inetp+"/ppp6/dns/count") > 0)
			{
				OBJ("w_dhcp_dns_manual").checked = true;
				OBJ("w_dhcp_pdns").value = XG(this.wan1.inetp+"/ppp6/dns/entry:1");
				if(XG(this.wan1.inetp+"/ppp6/dns/count")>=2) 
					OBJ("w_dhcp_sdns").value = XG(this.wan1.inetp+"/ppp6/dns/entry:2"); 
				else
					OBJ("w_dhcp_sdns").value = "";
			}
			else 
			{
				OBJ("w_dhcp_dns_auto").checked = true;
				OBJ("w_dhcp_pdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:1");
				OBJ("w_dhcp_sdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:2");
			}

                	//if(XG(this.wan1.inetp+"/ipv6/mode") != "")
                	if(child != "")
			{		
				OBJ("en_dhcp_pd").checked = true;
				OBJ("l_ipaddr").disabled = true;
				OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
			}
                	else
			{
				OBJ("en_dhcp_pd").checked = false;
				OBJ("l_ipaddr").disabled = false;
				OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
			}
		}
		else
		{ 
            		//if(XG(this.wan1.inetp+"/ppp6/static") === "1")
            		//hendry
            		if(XG(this.wanllact.inetp+"/ppp6/static") === "1")
			{
				OBJ("pppoe_static").checked = true;
                		//OBJ("pppoe_ipaddr").value           = XG(this.wanllact.inetp+"/ppp6/ipaddr");
                		OBJ("pppoe_ipaddr").value           = XG(this.wanact.inetp+"/ipv6/ipaddr");
			}
                	else
			{              
				OBJ("pppoe_dynamic").checked = true;
                		OBJ("pppoe_ipaddr").value           = XG(this.rwan+"/runtime/inf/inet/ipv6/ipaddr");
			}
                	//if(XG(this.wanllact.inetp+"/ppp6/static") === "1")  OBJ("pppoe_static").checked = true;
                	//else                                                OBJ("pppoe_dynamic").checked = true;
                	//OBJ("pppoe_ipaddr").value           = XG(this.wanllact.inetp+"/ppp6/ipaddr");
                	OBJ("pppoe_username").value         = XG(this.wanllact.inetp+"/ppp6/username");
                	OBJ("pppoe_password").value         = XG(this.wanllact.inetp+"/ppp6/password");
                	OBJ("confirm_pppoe_password").value = XG(this.wanllact.inetp+"/ppp6/password");
			OBJ("ppp6_mtu").value = XG(this.wanllact.inetp+"/ppp6/mtu");
                	OBJ("pppoe_service_name").value     = XG(this.wanllact.inetp+"/ppp6/pppoe/servicename");
                	OBJ("pppoe_sess_new").checked	    = true;
			
        		var dialup   = XG(this.wanllact.inetp+"/ppp6/dialup/mode");
			if(dialup === "manual")		OBJ("pppoe_manual").checked = true;
			else				OBJ("pppoe_alwayson").checked = true;
        		/*if(dialup === "auto")             OBJ("pppoe_alwayson").checked = true;
        		else if(dialup === "manual")      OBJ("pppoe_manual").checked = true;
        		else                              OBJ("pppoe_ondemand").checked = true;*/
			
                	//OBJ("pppoe_max_idle_time").value   = XG(this.wanllact.inetp+"/ppp6/dialup/idletimeout");
			
			if (XG(this.wanllact.inetp+"/ppp6/dns/count") > 0) 
			{
				OBJ("w_dhcp_dns_manual").checked = true;
				OBJ("w_dhcp_pdns").value = XG(this.wanllact.inetp+"/ppp6/dns/entry:1");
				if(XG(this.wanllact.inetp+"/ppp6/dns/count")>=2) 
					OBJ("w_dhcp_sdns").value = XG(this.wanllact.inetp+"/ppp6/dns/entry:2"); 
				else
					OBJ("w_dhcp_sdns").value = "";
			}
			else 
			{
				OBJ("w_dhcp_dns_auto").checked = true;
				OBJ("w_dhcp_pdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:1");
				OBJ("w_dhcp_sdns").value = XG(this.rwan+"/runtime/inf/inet/ipv6/dns:2");
			}

                	//if(XG(this.wanact.inetp+"/ipv6/mode") != "")
                	if(child != "")
			{		
				OBJ("en_dhcp_pd").checked = true;
				OBJ("l_ipaddr").disabled = true;
				OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
			}
                	else
			{
				OBJ("en_dhcp_pd").checked = false;
				OBJ("l_ipaddr").disabled = false;
				OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
			}
		}
		if(this.hintsupport == "1")
		{
			if(XG(this.wanact.inetp+"/ipv6/pdhint/enable") == "1")
			{
				OBJ("en_dhcp_pd_hint").checked = true;
				OBJ("dhcppd_hint_prefix").value = XG(this.wanact.inetp+"/ipv6/pdhint/network");
				OBJ("dhcppd_hint_pfxlen").value = XG(this.wanact.inetp+"/ipv6/pdhint/prefix");
				OBJ("dhcppd_hint_plft").value = XG(this.wanact.inetp+"/ipv6/pdhint/preferlft")/60;
				if(OBJ("dhcppd_hint_vlft").value!="")
				{
					OBJ("dhcppd_hint_vlft").value = XG(this.wanact.inetp+"/ipv6/pdhint/validlft")/60;
				}
			}
			else
			{
				OBJ("en_dhcp_pd_hint").checked = false;
				OBJ("dhcppd_hint_prefix").disabled = OBJ("dhcppd_hint_pfxlen").disabled = OBJ("dhcppd_hint_plft").disable 
				= OBJ("dhcppd_hint_vlft").disabled = true;
			}
		}
		return true;
	},
	InitWANRAValue: function()
	{
		this.rlan = PXML.FindModule("RUNTIME.INF.LAN-4");
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		if(wanmode != "RA") return true;

		var wan4 = PXML.FindModule("INET.WAN-4");
                if(XG(wan4+"/inf/dhcpc6") != "")
		{		
			OBJ("en_dhcp_pd").checked = true;
			OBJ("l_ipaddr").disabled = true;
			OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
		}
                else
		{
			OBJ("en_dhcp_pd").checked = false;
			OBJ("l_ipaddr").disabled = false;
			OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
		}
                //OBJ("w_dhcp_dns_auto").checked        = true;

		SetRadioValue("w_dhcp_dns_rad", "auto")
		var count = XG(this.wanact.inetp+"/ipv6/dns/count");
		if(count > 0)
		{
			OBJ("w_dhcp_dns_auto").checked = false;
			SetRadioValue("w_dhcp_dns_rad", "manual")
			OBJ("w_dhcp_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
			if(count > 1)
                               OBJ("w_dhcp_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		}
		return true;
	},
	InitWAN6IN4Value: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		if(wanmode != "6IN4") return true;

		OBJ("w_tu_lov6_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/ipaddr");
		OBJ("w_tu_pl").value		= XG(this.wanact.inetp+"/ipv6/prefix");
		OBJ("w_tu_rev6_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/gateway");
		OBJ("w_tu_rev4_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/ipv6in4/remote");
		OBJ("w_st_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
		OBJ("w_st_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		
                var addrtype = XG(this.rwan1+"/runtime/inf/inet/addrtype");
                if(addrtype == "")
                {  
                        OBJ("w_tu_lov4_ipaddr").innerHTML = "X.X.X.X";
                }
                else
                {
                        if(addrtype == "ipv4") //static ip or dhcp
                               OBJ("w_tu_lov4_ipaddr").innerHTML	= XG(this.rwan1+"/runtime/inf/inet/ipv4/ipaddr");
                        else if(addrtype == "ppp4") //ppp
                               OBJ("w_tu_lov4_ipaddr").innerHTML	= XG(this.rwan1+"/runtime/inf/inet/ppp4/local");
                        else   return false; 
                }
          
		SetRadioValue("w_dhcp_dns_rad", "auto")
                var count = XG(this.wanact.inetp+"/ipv6/dns/count");
		if(count > 0)
		{
			OBJ("w_dhcp_dns_auto").checked = false;
			SetRadioValue("w_dhcp_dns_rad", "manual")
			OBJ("w_dhcp_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
			if(count > 1)
				OBJ("w_dhcp_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
		}

                if(XG(this.wanact.svcs+"/inf/infnext") != "")
		{		
			OBJ("en_dhcp_pd").checked = true;
			OBJ("l_ipaddr").disabled = true;
			//OBJ("l_ipaddr").value 	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
		}
                else
		{
			OBJ("en_dhcp_pd").checked = false;
			OBJ("l_ipaddr").disabled = false;
			//OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
		}
		return true;
	},

	InitWAN6RDValue: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		if(wanmode != "6RD") return true;
		var str;

		addrtype = XG(this.rwan1+"/runtime/inf/inet/addrtype");
		if(addrtype=="ppp4")
		{
			if(XG(this.rwan1+"/runtime/inf/inet/ppp4/local")!="")
				OBJ("w_6rd_v4addr").innerHTML	= XG(this.rwan1+"/runtime/inf/inet/ppp4/local");
			else
				OBJ("w_6rd_v4addr").innerHTML	= "None";
		}
		else
		{
			if(XG(this.rwan1+"/runtime/inf/inet/ipv4/ipaddr")!="")
				OBJ("w_6rd_v4addr").innerHTML	= XG(this.rwan1+"/runtime/inf/inet/ipv4/ipaddr");
			else
				OBJ("w_6rd_v4addr").innerHTML	= "None";
		}	
		
                /* check wan is 6RD or not */
		if(XG(this.wanact.inetp+"/ipv6/mode") == "6RD")
		{
			//OBJ("w_6rd_prefix_1").value 	= XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/ipaddr");
			//OBJ("w_6rd_prefix_2").value 	= XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/prefix");
			//OBJ("w_6rd_v4addr_mask").value 	= XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask");
			ipaddr = XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/ipaddr");
			if(ipaddr!="")
			{ 
				OBJ("w_6rd_prefix_1").value 	= ipaddr;
				OBJ("w_6rd_prefix_2").value 	= XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/prefix");
				OBJ("w_6rd_v4addr_mask").value 	= XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask");
				OBJ("w_6rd_relay").value	= XG(this.wanact.inetp+"/ipv6/ipv6in4/relay");
				OBJ("6rd_dhcp_option").checked = false;
				OBJ("6rd_manual").checked = true;
				SetRadioValue("6rd_dhcp_option_rad", "6rd_manual");
			}
			else
			{
				OBJ("w_6rd_prefix_1").value 	= XG(this.rwan1+"/runtime/inf/udhcpc/sixrd_pfx");
				OBJ("w_6rd_prefix_2").value 	= XG(this.rwan1+"/runtime/inf/udhcpc/sixrd_pfxlen");
				OBJ("w_6rd_v4addr_mask").value 	= XG(this.rwan1+"/runtime/inf/udhcpc/sixrd_msklen");
				OBJ("w_6rd_relay").value	= XG(this.rwan1+"/runtime/inf/udhcpc/sixrd_brip");
				OBJ("w_6rd_prefix_1").disabled 	= true;
				OBJ("w_6rd_prefix_2").disabled 	= true;
				OBJ("w_6rd_v4addr_mask").disabled = true;
				OBJ("w_6rd_relay").disabled = true;
				OBJ("6rd_manual").checked = false;
				OBJ("6rd_dhcp_option").checked = true;
				SetRadioValue("6rd_dhcp_option_rad", "6rd_dhcp_option");
			}
				
			//OBJ("w_6rd_v4addr").value	= XG(this.rwan1+"/runtime/inf/inet/ipv4/ipaddr");
			//str = XG(this.rlan+"/runtime/inf/dhcps6/network");
			str = XG(this.rwan+"/runtime/inf/inet/ipv6/ipaddr");
			str1 = XG(this.rwan+"/runtime/inf/inet/ipv6/prefix");
			if(str)
			{
				index = str.lastIndexOf("::");
				str = str.substring(0,index);
				OBJ("w_6rd_prefix_3").innerHTML = str+"::/"+str1;
			}
			else
				OBJ("w_6rd_prefix_3").innerHTML = "None";
			//OBJ("l_ipaddr").value 		= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr");
			
			//+++ Jerry Kao, added Tunnel Link-Local Address.
			if(XG(this.rwan+"/runtime/inf/inet/ipv6/tunnel_ll_addr") != "")
			{											
				OBJ("w_6rd_ll_addr").innerHTML	= XG(this.rwan+"/runtime/inf/inet/ipv6/tunnel_ll_addr") + "/"+
			                                      XG(this.rwan+"/runtime/inf/inet/ipv6/tunnel_ll_prefix");
			}
			else
				OBJ("w_6rd_ll_addr").innerHTML	= "None";
			
			if(XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr")=="")
				OBJ("l_span_6rd").innerHTML = "None";
			else
				OBJ("l_span_6rd").innerHTML	= XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr")+" /"+XG(this.rlan+"/runtime/inf/inet/ipv6/prefix");
			OBJ("w_6rd_pdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
			OBJ("w_6rd_sdns").value		= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
			//OBJ("l_ipaddr").disabled 	= true;
			//OBJ("w_6rd_v4addr").disabled 	= true;
			
			$hubspokemode = XG(this.wanact.inetp+"/ipv6/ipv6in4/rd/hubspokemode"); 
			if($hubspokemode == "1")
			{
				OBJ("en_hub_spoke").checked = true;
			}
			else
			{
				OBJ("en_hub_spoke").checked = false;
			}
		}
		else
		{
			//OBJ("w_6rd_addr").innerHTML = "";
			//OBJ("l_ipaddr").value 	= "";
			OBJ("w_6rd_prefix_3").innerHTML = "None";
			OBJ("l_span_6rd").innerHTML = "";
			OBJ("w_6rd_pdns").value	= "";
			OBJ("w_6rd_sdns").value	= "";
			//OBJ("l_ipaddr").disabled 	= false;
			//OBJ("w_6rd_v4addr").disabled = false;
		}

		/* fill some fixed info */
		/*OBJ("w_6to4_pl").innerHTML = "/16";*/
		return true;
	},

	InitWAN6TO4Value: function()
	{
		var wanmode = XG(this.wanact.inetp+"/ipv6/mode");
		if(wanmode != "6TO4") return true;
		
    		/* get slaid from DB */
    		if(XG(this.wanact.inetp+"/ipv6/ipv6in4/ipv6to4/slaid")=="")
		{
			OBJ("l_ipaddr_6to4").value = "1";
		}
		else
		{
			var slanum = new Number(XG(this.wanact.inetp+"/ipv6/ipv6in4/ipv6to4/slaid"));
			OBJ("l_ipaddr_6to4").value = slanum.toString(16);
		}
		
        	/* check wan is 6TO4 or not */
        	if(XG(this.rwan+"/runtime/inf/inet/ipv6/mode") == "6TO4")
        	{	 
			OBJ("w_6to4_ipaddr").innerHTML = XG(this.rwan+"/runtime/inf/inet/ipv6/ipaddr");
			var addrtype = XG(this.rwan1+"/runtime/inf/inet/addrtype");
			if(addrtype=="ipv4") addrtype=addrtype+"/ipaddr";
			else if(addrtype=="ppp4") addrtype=addrtype+"/local"
			if(XG(this.rwan1+"/runtime/inf/inet/"+addrtype)!="")
			{
				var ip4addr = XG(this.rwan1+"/runtime/inf/inet/"+addrtype);
				var ary_ip4addr = ip4addr.split(".");
				var ip_prefix = "2002:"+Dec2Hex(ary_ip4addr[0])+Dec2Hex(ary_ip4addr[1])+":"+Dec2Hex(ary_ip4addr[2])+Dec2Hex(ary_ip4addr[3])+":";
				ip_prefix = CutFirstSpace(ip_prefix);
				
				var lanip = XG(this.rlan+"/runtime/inf/inet/ipv6/ipaddr").toLowerCase();
				var pre_str = ip_prefix + OBJ("l_ipaddr_6to4").value + ":";
				var firstp = lanip.indexOf(pre_str.toLowerCase());
				if(firstp==0)
				{
					OBJ("l_prefix_6to4").innerHTML = ip_prefix.toLowerCase();
					last_str = lanip.substring(firstp+pre_str.length, lanip.length);
					OBJ("l_ipaddreui_6to4").innerHTML = last_str;
				}
			}
			else
			{
				OBJ("l_prefix_6to4").innerHTML = "XXXX:XXXX:XXXX:";
				OBJ("l_ipaddreui_6to4").innerHTML = ":1";
			}
			if (this.rgmode)	OBJ("l_ipaddr").disabled 	= true;
		}
        	else
        	{
             		OBJ("w_6to4_ipaddr").innerHTML = "";
	     		//OBJ("l_ipaddr").value 	= "";
	     		//OBJ("w_6to4_pdns").value	= "";
	     		//OBJ("w_6to4_sdns").value	= "";
		     	if (this.rgmode)	OBJ("l_ipaddr").disabled 	= false;
        	}
        	OBJ("w_6to4_relay").value	= XG(this.wanact.inetp+"/ipv6/ipv6in4/relay");
		OBJ("w_6to4_pdns").value	= XG(this.wanact.inetp+"/ipv6/dns/entry:1");
		OBJ("w_6to4_sdns").value	= XG(this.wanact.inetp+"/ipv6/dns/entry:2");
			
		/* fill some fixed info */
		/*OBJ("w_6to4_pl").innerHTML = "/16";*/
		return true;
	},
	InitWANInfo: function()
	{
		OBJ("en_dhcp_pd").checked = true;
		OBJ("w_dhcp_dns_auto").checked = true;
		OBJ("l_ipaddr").disabled = true;
		OBJ("pppoe_dynamic").checked = true; //20100614
		OBJ("pppoe_sess_share").checked = true; //20100614
		OBJ("6rd_dhcp_option").checked = true; 
		if( this.wanact.svcsname != null )
		{
			/* init value */
			if (!this.InitWANStaticValue()) return false;
			/*if (!this.InitWANDHCPValue()) return false;*/
			if (!this.InitWANPPPoEValue()) return false;
			if (!this.InitWANAUTOValue()) return false;
			/*if (!this.InitWANRAValue()) return false;*/
			if (!this.InitWAN6IN4Value()) return false;
			if (!this.InitWAN6RDValue()) return false;
			if (!this.InitWAN6TO4Value()) return false;
		}

		return true;
	},

	InitWANLLInfo: function()
	{
		if( this.wanllact.svcsname != null ) /* this should not happen */
		{
			//this.MyArrayShowAlert(this.wanllact);
			OBJ("wan_ll").innerHTML    = this.wanllact.ipv6ll;
			OBJ("wan_ll_pl").innerHTML    = "/64";
		}
		return true;
	},
	InitLANInfo: function()
	{
		//this.MyArrayShowAlert("lanact",this.lanact);
		if( this.lanact.svcsname != null )  /* lan mode has be set -> lanact is not null -> we need fill all lan info*/
		{
			switch (XG(this.lanact.inetp+"/ipv6/mode"))
			{
				case "STATIC":
					OBJ("l_ipaddr").value	= XG(this.lanact.inetp+"/ipv6/ipaddr");
					if(XG(this.wanact.inetp+"/ipv6/mode") == "STATIC")
					{
						OBJ("l_range_start_pl").innerHTML	= "/64";
						OBJ("l_range_end_pl").innerHTML	= "/64";
                        		}
                        		else //6IN4
                        		{;}
					break;

				case "DHCP":
					break;

				case "RA":
					break;

				case "PPPOE":
					break;

				case "6IN4":
					break;

				case "6TO4":
					/* TODO */
					break;
			}
		}

		/* fill some fixed info */
		OBJ("l_pl").innerHTML	= "/64";

		return true;
	},
	InitLANLLInfo: function(addrmode)
	{
		if( this.lanllact.svcsname != null ) /* this should not happen */
		{
			OBJ("lan_ll").innerHTML    = this.lanllact.ipv6ll;
			OBJ("lan_ll_pl").innerHTML    = "/64";
		}
		OBJ("lan_ul").innerHTML = XG(this.rlan6+"/runtime/inf/inet/ipv6/ipaddr");
		OBJ("lan_ul_pl").innerHTML    = "/64";
		return true;
	},
	InitLANAutoConf: function()
	{
		var dhcps6 = XG(this.lanact.svcs+"/inf/dhcps6");		
		if(dhcps6 != "")
		{
			OBJ("enableAuto").checked = true;
		}
		else
		{
			OBJ("enableAuto").checked = false;
		}
		this.OnClickAuto();
                return true;
	},
	InitLANPdConf: function()
	{
		var lanpd = XG(this.dhcps6+"/pd/enable");
		if(lanpd == "1")
		{
			OBJ("en_lan_pd").checked = true;
		}
		else
		{
			OBJ("en_lan_pd").checked = false;
		}
                return true;
	},
	PreWAN: function()
	{				
		//rbj, to avoid inet is null
		var wan3 = PXML.FindModule("INET.WAN-3");
		var wan4 = PXML.FindModule("INET.WAN-4");
		XS(wan3+"/inf/inet","INET-8");
		XS(wan4+"/inf/active","1");
		XS(wan4+"/inf/inet","INET-9");
		this.ParseAll();
			
		XS(this.wanllact.svcs+"/inf/infnext", "");  
		XS(this.wanllact.svcs+"/inf/infprevious", "");  
		XS(this.wanllact.svcs+"/inf/child", "");
		XS(this.wanllact.inetp+"/ipv6/dhcpopt", "");
		XS(this.wanllact.inetp+"/ipv6/mode", "LL");
		//XS(this.wanact.svcs+"/inf/phyinf", "ETH-2");
		XS(this.wanact.svcs+"/inf/infprevious", "");
		XS(this.wanact.svcs+"/inf/infnext", "");
		XS(this.wanact.svcs+"/inf/infnext:2", "");
		XS(this.wanact.svcs+"/inf/child", "");
		XS(this.wanact.svcs+"/inf/childgz", "");
		
		var infprevious = XG(this.wan1.infp+"/infprevious");
		XS(this.wan1.infp+"/infprevious", "");
		
		XS(this.wan5.infp+"/active", "0");
		XS(this.wan5.infp+"/infnext", "");				
	
		//ipv4 wan mode
		dslite_mode = XG(this.wan1.inetp+"/ipv4/ipv4in6/mode");
		v6wanmode = OBJ("wan_ipv6_mode").value;
		if(dslite_mode=="dslite")
		{
			if(v6wanmode=="LL" || v6wanmode=="6TO4" || v6wanmode=="6RD")
			{
				alert("You should choose another IPv4 Internet Connection mode rather than 'DS-Lite' to access the Internet.");
				return false;
			}
			XS(this.wan1.infp+"/infprevious", infprevious);
		}						
		
		//+++ Jerry Kao, remove all space in ipv6 address, gateway, DNS, DHCP.		
		OBJ("w_st_ipaddr").value 	  = COMM_EatAllSpace(OBJ("w_st_ipaddr").value);
		OBJ("w_st_gw").value 	 	  = COMM_EatAllSpace(OBJ("w_st_gw").value);
		OBJ("w_st_pdns").value 	 	  = COMM_EatAllSpace(OBJ("w_st_pdns").value);
		OBJ("w_st_sdns").value 		  = COMM_EatAllSpace(OBJ("w_st_sdns").value);
		OBJ("w_dhcp_pdns").value 	  = COMM_EatAllSpace(OBJ("w_dhcp_pdns").value);
		OBJ("w_dhcp_sdns").value 	  = COMM_EatAllSpace(OBJ("w_dhcp_sdns").value);
		OBJ("w_tu_rev6_ipaddr").value = COMM_EatAllSpace(OBJ("w_tu_rev6_ipaddr").value);
		OBJ("w_tu_lov6_ipaddr").value = COMM_EatAllSpace(OBJ("w_tu_lov6_ipaddr").value);								
		 
		// get "wan mode" then set wan info
		switch(OBJ("wan_ipv6_mode").value)
		{
			case "STATIC":
				XS(this.wan1.infp+"/infnext", "");
				XS(this.wan1.infp+"/child", "");
				XS(this.wanact.svcs+"/inf/child", "");
           			XS(this.wanllact.inetp+"/addrtype", "ipv6");
				XS(this.wanact.svcs+"/inf/active", "1");
				if(dslite_mode=="dslite")
					XS(this.wanact.svcs+"/inf/dhcpc6", "1");//for deslite
				else
					XS(this.wanact.svcs+"/inf/dhcpc6", "0");//for deslite
				XS(this.wanact.inetp+"/ipv6/mode", "STATIC");

				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10")
				{
					 //XS(this.wan1.inetp+"/addrtype", "ipv4");
					 //XS(this.wan1.inetp+"/ipv4/static", "0");
					 XS(this.wan1.inetp+"/addrtype", "ppp4");
				}								
				
				/* ipaddr, prefix */
				XS(this.wanact.inetp+"/ipv6/ipaddr", OBJ("w_st_ipaddr").value);
				XS(this.wanact.inetp+"/ipv6/prefix", OBJ("w_st_pl").value);

				/* gateway */
				if (OBJ("w_st_gw").value != "") 
				{	XS(this.wanact.svcs+"/inf/defaultroute", "1");	}
				else
				{	XS(this.wanact.svcs+"/inf/defaultroute", "0");	}
				XS(this.wanact.inetp+"/ipv6/gateway", OBJ("w_st_gw").value);

				/* dns */
				var dnscnt = 0;								
				if (OBJ("w_st_pdns").value != "")	dnscnt++;
				else { 
					BODY.ShowAlert("<?echo I18N("j", "Invalid primary dns.");?>");
					OBJ("w_st_pdns").focus();
					return false;
				}
				if (OBJ("w_st_sdns").value != "")	dnscnt++;
				XS(this.wanact.inetp+"/ipv6/dns/count", dnscnt);
				
				
				//+++ Jerry Kao, modified for Primary and Secondary DNS contains IPv4 addr.
				var str_pdns, idx_pdns, pdns_1, pdns_2, ptmp_1, ptmp_2, w_st_pdns;				
				var str_sdns, idx_sdns, sdns_1, sdns_2, stmp_1, stmp_2, w_st_sdns;
												
				str_pdns = OBJ("w_st_pdns").value;			// alert("IP1-"+ str_pdns);
				idx_pdns = str_pdns.lastIndexOf("::");
				pdns_1   = str_pdns.substring(0, idx_pdns);							
				pdns_2   = str_pdns.substring(idx_pdns+2);				
				
				var FSIP_pval = pdns_2.split(".");
									
				var tmp_0 = parseInt(FSIP_pval[1], 10);								
				if ( tmp_0 > 0 )
				{															
					ptmp_1 = parseInt(FSIP_pval[0], 10)*256 + parseInt(FSIP_pval[1], 10);
					ptmp_2 = parseInt(FSIP_pval[2], 10)*256 + parseInt(FSIP_pval[3], 10);					
					w_st_pdns = pdns_1 + "::" + ptmp_1.toString(16) +":" + ptmp_2.toString(16);
					
					for (var j=0; j<4; j++)
					{
						if (parseInt(FSIP_pval[j], 10) > 255)
						{
							alert("<?echo I18N("j", "Incorrect Primary DNS Server.");?>");												
							return false;
						}					
					}
				}
				else
				{
					w_st_pdns = OBJ("w_st_pdns").value;
				}						
								
				str_sdns = OBJ("w_st_sdns").value;	
				idx_sdns = str_sdns.lastIndexOf("::");
				sdns_1   = str_sdns.substring(0, idx_sdns);							
				sdns_2   = str_sdns.substring(idx_sdns+2);								
																															
				var FSIP_sval = sdns_2.split(".");  
				
				var tmp_0 = parseInt(FSIP_sval[1], 10);				
				if ( tmp_0 > 0 )
				{										
					stmp_1 = parseInt(FSIP_sval[0], 10)*256 + parseInt(FSIP_sval[1], 10);
					stmp_2 = parseInt(FSIP_sval[2], 10)*256 + parseInt(FSIP_sval[3], 10);									
					w_st_sdns = sdns_1 + "::" + stmp_1.toString(16) +":" + stmp_2.toString(16);																			
																					
					for (var j=0; j<4; j++)
					{					
						if (parseInt(FSIP_sval[j], 10) > 255)
						{
							alert("<?echo I18N("j", "Incorrect Secondary DNS Server.");?>");												
							return false;
						}
					}
				}	
				else
				{
					w_st_sdns = OBJ("w_st_sdns").value;	
				}												
				
				//XS(this.wanact.inetp+"/ipv6/dns/entry:1", OBJ("w_st_pdns").value);				
				//XS(this.wanact.inetp+"/ipv6/dns/entry:2", OBJ("w_st_sdns").value);
				XS(this.wanact.inetp+"/ipv6/dns/entry:1", w_st_pdns);
				XS(this.wanact.inetp+"/ipv6/dns/entry:2", w_st_sdns);

				/* dslite */
				if(dslite_mode == "dslite") 
				{
					XS(this.wanact.svcs+"/inf/infnext", "WAN-1");
					XS(this.wanact.svcs+"/inf/infnext:2", "LAN-4");
				}
				else	XS(this.wanact.svcs+"/inf/infnext", "LAN-4");
				
				break;

			case "AUTODETECT":							
				XS(this.wan1.infp+"/infnext", "");
				XS(this.wan1.infp+"/child", "");
				XS(this.wanllact.inetp+"/addrtype", "ipv6");
				XS(this.wanact.svcs+"/inf/active", "0");
				//XS(this.wanact.svcs+"/inf/infnext", "WAN-1");//record uid of v4
				//XS(this.wanact.svcs+"/inf/defaultroute", "1");
				//XS(this.wanact.inetp+"/ipv6/mode", "AUTODETECT");
				XS(this.wan5.infp+"/active", "1");
				//XS(this.wan5.infp+"/infnext", "WAN-1");//record uid of v4
				XS(this.wan5.inetp+"/ipv6/mode", "AUTODETECT");
				XS(this.wan5.inetp+"/ipv6/detectuid/v4actuid", "WAN-1");
				XS(this.wan5.inetp+"/ipv6/detectuid/v6lluid", "WAN-3");
				XS(this.wan5.inetp+"/ipv6/detectuid/v6actuid", "WAN-4");
				
				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}	
				
				if(OBJ("en_dhcp_pd").checked) 
				{
					XS(this.wan5.infp+"/child", "LAN-4");
					XS(this.wan5.infp+"/childgz", "LAN-5");
					XS(this.wan5.inetp+"/ipv6/dhcpopt", "IA-NA+IA-PD");//maybe change when service starts
				}
				else 	 
				{
					XS(this.wan5.infp+"/child", "");
					XS(this.wan5.infp+"/childgz", "");
					XS(this.wan5.inetp+"/ipv6/dhcpopt", "");
					XS(this.wan5.infp+"/infnext", "LAN-4");
				}
				
				/* dns */
				var dnscnt = 0;
				if (OBJ("w_dhcp_dns_auto").checked)
				{
					XS(this.wan5.inetp+"/ipv6/dns/entry:1","");
					XS(this.wan5.inetp+"/ipv6/dns/entry:2","");
				}
				else
				{
					if(OBJ("w_dhcp_pdns").value != "")
					{
						XS(this.wan5.inetp+"/ipv6/dns/entry:1", OBJ("w_dhcp_pdns").value);
						dnscnt+=1;
					}
					if(OBJ("w_dhcp_sdns").value != "")
					{
						XS(this.wan5.inetp+"/ipv6/dns/entry:2", OBJ("w_dhcp_sdns").value);
						dnscnt+=1;
					}
				}
				XS(this.wan5.inetp+"/ipv6/dns/count", dnscnt);
				break;

			case "AUTO":
				XS(this.wan1.infp+"/infnext", "");
				XS(this.wan1.infp+"/child", "");
				XS(this.wanllact.inetp+"/addrtype", "ipv6");
				XS(this.wanact.svcs+"/inf/active", "1");
				XS(this.wanact.svcs+"/inf/defaultroute", "1");
				XS(this.wanact.inetp+"/ipv6/mode", "AUTO");
				
				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					//XS(this.wan1.inetp+"/addrtype", "ipv4");
					//XS(this.wan1.inetp+"/ipv4/static", "0");
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}	

				if(OBJ("en_dhcp_pd").checked) 
				{
					XS(this.wanact.svcs+"/inf/child", "LAN-4");
					XS(this.wanact.svcs+"/inf/childgz", "LAN-5");
					XS(this.wanact.inetp+"/ipv6/dhcpopt", "IA-NA+IA-PD");//maybe change when service starts
				}
				else 	 
				{
					XS(this.wanact.svcs+"/inf/child", "");
					XS(this.wanact.svcs+"/inf/childgz", "");
					XS(this.wanact.inetp+"/ipv6/dhcpopt", "");
					if(dslite_mode == "dslite")	XS(this.wanact.svcs+"/inf/infnext:2", "LAN-4");
					else						XS(this.wanact.svcs+"/inf/infnext", "LAN-4");
				}
				
				if(OBJ("en_dhcp_pd_hint").checked) 
				{
					XS(this.wanact.inetp+"/ipv6/pdhint/enable", "1");
					XS(this.wanact.inetp+"/ipv6/pdhint/network", OBJ("dhcppd_hint_prefix").value);
					XS(this.wanact.inetp+"/ipv6/pdhint/prefix", OBJ("dhcppd_hint_pfxlen").value);
					XS(this.wanact.inetp+"/ipv6/pdhint/preferlft", 60*OBJ("dhcppd_hint_plft").value);
					if(OBJ("dhcppd_hint_vlft").value!="")
					{
						XS(this.wanact.inetp+"/ipv6/pdhint/validlft", 60*OBJ("dhcppd_hint_vlft").value);
					}
					else
					{
						XS(this.wanact.inetp+"/ipv6/pdhint/validlft", "");
					}
				}
				else
				{
					XS(this.wanact.inetp+"/ipv6/pdhint/enable", "0");
				}

				/* dns */
				var dnscnt = 0;
				if (OBJ("w_dhcp_dns_auto").checked)
				{
					XS(this.wanact.inetp+"/ipv6/dns/entry:1","");
					XS(this.wanact.inetp+"/ipv6/dns/entry:2","");
				}
				else
				{
					if(OBJ("w_dhcp_pdns").value != "")
					{
						XS(this.wanact.inetp+"/ipv6/dns/entry:1", OBJ("w_dhcp_pdns").value);
						dnscnt+=1;
					}
					if(OBJ("w_dhcp_sdns").value != "")
					{
						XS(this.wanact.inetp+"/ipv6/dns/entry:2", OBJ("w_dhcp_sdns").value);
						dnscnt+=1;
					}
				}
				XS(this.wanact.inetp+"/ipv6/dns/count", dnscnt);
					
				/* dslite */
				if(dslite_mode == "dslite")
				{
					XS(this.wanact.svcs+"/inf/infnext", "WAN-1");
					XS(this.wanact.svcs+"/inf/dhcpc6", "0");
				}
					
				break;

			case "PPPOE":
				/*Choose share with IPv4, pop up warning message if ipv4 connection mode is not pppoe*/
				if(OBJ("pppoe_sess_share").checked && XG(this.wan1.inetp+"/addrtype")!="ppp4" && XG(this.wan1.inetp+"/addrtype")!="ppp10")
				{
					BODY.ShowAlert("<?echo i18n("IPv6 PPPoE is share with IPv4 PPPoE. Please change IPv4 WAN protocol to PPPoE at first.");?>");
					return null;
				}
				
				//+++ Jerry Kao, Init node /child to avoid /child is still "WAN-3", 
				//               when "Share with v4" to "Create new session".
				XS(this.wan1.infp+"/child", "");	
						
				XS(this.wan1.infp+"/infnext", "");
				XS(this.wan1.infp+"/inf/child", "");
				XS(this.wanllact.svcs+"/inf/inet", "INET-8");  
				XS(this.wanact.svcs+"/inf/active", "1");
				XS(this.wanact.svcs+"/inf/defaultroute", "0");
				if(OBJ("en_dhcp_pd").checked) 
				{
					if(OBJ("pppoe_sess_share").checked)
					{
						XS(this.wan1.infp+"/child", "WAN-3");  
						XS(this.wanllact.svcs+"/inf/inet", "");  
						XS(this.wanllact.svcs+"/inf/infnext", "WAN-4");  
						XS(this.wanact.svcs+"/inf/infprevious", "WAN-3"); 
					}
					else
					{	
						XS(this.wanllact.svcs+"/inf/infnext", "WAN-4");  
						XS(this.wanllact.svcs+"/inf/inet", "INET-8");  
						XS(this.wanact.svcs+"/inf/infprevious", "WAN-3");  
					}
					//XS(this.wanact.inetp+"/ipv6/mode", "PPPDHCP");
					//XS(this.wanact.inetp+"/ipv6/mode", "AUTO");
					XS(this.wanact.svcs+"/inf/child", "LAN-4");
					XS(this.wanact.svcs+"/inf/childgz", "LAN-5");
					XS(this.wanact.inetp+"/ipv6/dhcpopt", "IA-PD");
				}
				else 	 
				{
					//XS(this.wan1.infp+"/infnext", "");  
					//XS(this.wanllact.svcs+"/inf/infnext", "");  
					//XS(this.wanact.svcs+"/inf/infprevious", "");  
					//XS(this.wanact.svcs+"/inf/child", "");
					//XS(this.wanact.inetp+"/ipv6/mode", "");
					if(OBJ("pppoe_sess_share").checked)
					{
						XS(this.wan1.infp+"/child", "WAN-3");  
						XS(this.wanllact.svcs+"/inf/inet", "");  
						XS(this.wanllact.svcs+"/inf/infnext", "WAN-4");  
						XS(this.wanact.svcs+"/inf/infprevious", "WAN-3"); 
					}
					else
					{	
						XS(this.wanllact.svcs+"/inf/infnext", "WAN-4");  
						XS(this.wanllact.svcs+"/inf/inet", "INET-8");  
						XS(this.wanact.svcs+"/inf/infprevious", "WAN-3");  
					}
					XS(this.wanact.svcs+"/inf/infnext", "LAN-4"); 
					//XS(this.wanact.inetp+"/ipv6/mode", "PPPDHCP");
					XS(this.wanact.inetp+"/ipv6/dhcpopt", "IA-NA");
					XS(this.wanact.svcs+"/inf/child", "");
					XS(this.wanact.svcs+"/inf/childgz", "");
					//XS(this.wanact.inetp+"/ipv6/mode", "AUTO");
				}
				
           		if(OBJ("pppoe_dynamic").checked)
					XS(this.wanact.inetp+"/ipv6/mode", "AUTO");
				else
				{
					XS(this.wanact.inetp+"/ipv6/mode", "STATIC");
					XS(this.wanact.inetp+"/ipv6/ipaddr", OBJ("pppoe_ipaddr").value);
					XS(this.wanact.inetp+"/ipv6/gateway", "fe80::1");//fake
					XS(this.wanact.inetp+"/ipv6/prefix", 128);
				}

                                if(!this.PrePppoe())  return false; 
				break;

			case "6IN4":
				//XS(this.wan1.infp+"/infnext", "");
				//XS(this.wan1.infp+"/child", "");
				XS(this.wan1.infp+"/infnext", "WAN-4");
				XS(this.wanact.svcs+"/inf/infprevious", "WAN-1");
				if(OBJ("en_dhcp_pd").checked)
				{
					XS(this.wanllact.svcs+"/inf/infprevious", "WAN-4");  
					XS(this.wanllact.svcs+"/inf/child", "LAN-4");
					XS(this.wanllact.svcs+"/inf/childgz", "LAN-5");
           				XS(this.wanllact.inetp+"/addrtype", "ipv6");
					XS(this.wanllact.inetp+"/ipv6/mode", "AUTO");
					XS(this.wanllact.inetp+"/ipv6/dhcpopt", "IA-PD");
					XS(this.wanact.svcs+"/inf/active", "1");
					XS(this.wanact.svcs+"/inf/infnext", "WAN-3");
					XS(this.wanact.inetp+"/ipv6/mode", "6IN4");
				}
				else
				{
					XS(this.wanllact.svcs+"/inf/infprevious", "");  
					XS(this.wanact.svcs+"/inf/child", "");
					XS(this.wanact.svcs+"/inf/childgz", "");
           				XS(this.wanllact.inetp+"/addrtype", "ipv6");
					XS(this.wanllact.inetp+"/ipv6/mode", "LL");
					XS(this.wanllact.inetp+"/ipv6/dhcpopt", "");
					XS(this.wanact.svcs+"/inf/active", "1");
					XS(this.wanact.svcs+"/inf/infnext", "");
					XS(this.wanact.inetp+"/ipv6/mode", "6IN4");
				} 
				
				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					//XS(this.wan1.inetp+"/addrtype", "ipv4");
					//XS(this.wan1.inetp+"/ipv4/static", "0");
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}

				/* ipaddr, prefix */
				XS(this.wanact.inetp+"/ipv6/ipaddr", OBJ("w_tu_lov6_ipaddr").value);
				XS(this.wanact.inetp+"/ipv6/prefix", OBJ("w_tu_pl").value);

				/* gateway */
				XS(this.wanact.svcs+"/inf/defaultroute", "1");	
				XS(this.wanact.inetp+"/ipv6/gateway", OBJ("w_tu_rev6_ipaddr").value);

				/* dns */
				var dnscnt = 0;
				if (OBJ("w_dhcp_dns_auto").checked)
				{
					XS(this.wanact.inetp+"/ipv6/dns/entry:1","");
					XS(this.wanact.inetp+"/ipv6/dns/entry:2","");
				}
				else
				{
					if(OBJ("w_dhcp_pdns").value != "")
					{
						XS(this.wanact.inetp+"/ipv6/dns/entry:1", OBJ("w_dhcp_pdns").value);
						dnscnt+=1;
					}
					if(OBJ("w_dhcp_sdns").value != "")
					{
						XS(this.wanact.inetp+"/ipv6/dns/entry:2", OBJ("w_dhcp_sdns").value);
						dnscnt+=1;
					}
				}
				XS(this.wanact.inetp+"/ipv6/dns/count", dnscnt);

                                /* set ipv4 address for server */
				XS(this.wanact.inetp+"/ipv6/ipv6in4/remote", OBJ("w_tu_rev4_ipaddr").value);
                                
				break;

			case "6TO4":
				XS(this.wan1.infp+"/infnext", "WAN-4");
				XS(this.wan1.infp+"/child", "");
           			XS(this.wanllact.inetp+"/addrtype", "ipv6");
				XS(this.wanact.svcs+"/inf/child", "LAN-4");
				XS(this.wanact.svcs+"/inf/infprevious", "WAN-1");
				XS(this.wanact.svcs+"/inf/active", "1");
				XS(this.wanact.svcs+"/inf/defaultroute", "1");	
				XS(this.wanact.inetp+"/ipv6/mode", "6TO4");
				XS(this.wanact.inetp+"/ipv6/ipv6in4/relay", OBJ("w_6to4_relay").value);

				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					//XS(this.wan1.inetp+"/addrtype", "ipv4");
					//XS(this.wan1.inetp+"/ipv4/static", "0");
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}

				/* dns */
				var dnscnt = 0;
				if (OBJ("w_6to4_pdns").value != "")	dnscnt++;
				if (OBJ("w_6to4_sdns").value != "")	dnscnt++;
				XS(this.wanact.inetp+"/ipv6/dns/count", dnscnt);
				XS(this.wanact.inetp+"/ipv6/dns/entry:1", OBJ("w_6to4_pdns").value);
				XS(this.wanact.inetp+"/ipv6/dns/entry:2", OBJ("w_6to4_sdns").value);
				if(!this.Pre6TO4())  return false; 
				
				break;
                          
			case "6RD":
				XS(this.wan1.infp+"/infnext", "WAN-4");
           			XS(this.wanllact.inetp+"/addrtype", "ipv6");
				XS(this.wanact.svcs+"/inf/child", "LAN-4");
				XS(this.wanact.svcs+"/inf/infprevious", "WAN-1");
				XS(this.wanact.svcs+"/inf/active", "1");
				XS(this.wanact.svcs+"/inf/defaultroute", "1");	
				XS(this.wanact.inetp+"/ipv6/mode", "6RD");
				//XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/ipaddr", OBJ("w_6rd_prefix_1").value);
				//XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/prefix", OBJ("w_6rd_prefix_2").value);
				//XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask", OBJ("w_6rd_v4addr_mask").value);

				if(OBJ("en_hub_spoke").checked)
				{
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/hubspokemode", "1");
				}
				else
				{
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/hubspokemode", "0");
				}

				if (OBJ("6rd_manual").checked)
				{
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/ipaddr", OBJ("w_6rd_prefix_1").value);
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/prefix", OBJ("w_6rd_prefix_2").value);
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask", OBJ("w_6rd_v4addr_mask").value);
					XS(this.wanact.inetp+"/ipv6/ipv6in4/relay", OBJ("w_6rd_relay").value);
				}
				else
				{
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/ipaddr", "");
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/prefix", "");
					XS(this.wanact.inetp+"/ipv6/ipv6in4/rd/v4mask", "");
					XS(this.wanact.inetp+"/ipv6/ipv6in4/relay", "");
				}

				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					//XS(this.wan1.inetp+"/addrtype", "ipv4");
					//XS(this.wan1.inetp+"/ipv4/static", "0");
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}

				/* dns */
				var dnscnt = 0;
				if (OBJ("w_6rd_pdns").value != "")	dnscnt++;
				if (OBJ("w_6rd_sdns").value != "")	dnscnt++;
				XS(this.wanact.inetp+"/ipv6/dns/count", dnscnt);
				XS(this.wanact.inetp+"/ipv6/dns/entry:1", OBJ("w_6rd_pdns").value);
				XS(this.wanact.inetp+"/ipv6/dns/entry:2", OBJ("w_6rd_sdns").value);
				break;

            case "LL":
				/* clear ppp10 addrtype */
				var wan1type = XG(this.wan1.inetp+"/addrtype");
				if(wan1type == "ppp10") 
				{
					//XS(this.wan1.inetp+"/addrtype", "ipv4");
					//XS(this.wan1.inetp+"/ipv4/static", "0");
					XS(this.wan1.inetp+"/addrtype", "ppp4");
				}
				XS(this.wan1.infp+"/infnext", "");
				XS(this.wan1.infp+"/child", "");
           		XS(this.wanllact.inetp+"/addrtype", "ipv6");
           		XS(this.wanllact.inetp+"/mode", "LL");
				XS(this.wanact.svcs+"/inf/active", "0");
				XS(this.wanact.svcs+"/inf/child", "");
				XS(this.wanact.inetp+"/ipv6/mode", "");
				XS(this.wanact.svcs+"/inf/active", "0");
                break;
                         
		}

		return true;
	},

	PreLAN: function()
	{		
		//rbj, to avoid inet is null
		var lan4 = PXML.FindModule("INET.LAN-4");
		XS(lan4+"/inf/inet","INET-7");
		this.ParseAll();

		//rbj, disable dns relay for dir815
		XS(this.lanact.svcs+"/inf/dnsrelay", "0");
		XS(this.lanact.svcs+"/inf/infprevious", "");
		XS(this.lanact.svcs+"/inf/infnext", "");
		
		//+++ Jerry Kao, remove all space in ipv6 address.		
		OBJ("l_ipaddr").value = COMM_EatAllSpace(OBJ("l_ipaddr").value);			
				
		
		// get "wan mode" then set lan info
		switch(OBJ("wan_ipv6_mode").value)
		{
			case "STATIC":
				XS(this.lanact.svcs+"/inf/active", "1");
				XS(this.lanact.svcs+"/inf/infprevious", "WAN-4");
				XS(this.lanact.inetp+"/ipv6/mode", "STATIC");
								
				/* ipaddr, prefix*/
				XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
				var tmp = OBJ("l_pl").innerHTML;
				var tmplstpl = tmp.split("/"); /*cut slash */	
				XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				break;

			case "AUTO":
			case "AUTODETECT":
				if(!OBJ("en_dhcp_pd").checked)
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "INET-7");
					XS(this.lanact.svcs+"/inf/infprevious", "WAN-4");
					var base = PXML.FindModule("INET.LAN-4");
					this.lanact.inetp = GPBT(base+"/inet", "entry", "uid", XG(this.lanact.svcs+"/inf/inet"), false);
					XS(this.lanact.inetp+"/ipv6/mode", "STATIC");

					/* ipaddr, prefix*/
					XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					var tmp = OBJ("l_pl").innerHTML;
					var tmplstpl = tmp.split("/"); /*cut slash */	
					XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				else
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "");

					/* ipaddr, prefix*/
					//OBJ("l_ipaddr").value = "";
					//XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					//var tmp = OBJ("l_pl").innerHTML;
					//var tmplstpl = tmp.split("/"); /*cut slash */	
					//XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				break;

			case "DHCP":
				//XS(this.lanact.svcs+"/inf/active", "1");
				//XS(this.lanact.inetp+"/ipv6/mode", "");
				//XS(this.lanact.inetp+"/ipv6/ipaddr", "");
				//XS(this.lanact.inetp+"/ipv6/prefix", "");
				if(!OBJ("en_dhcp_pd").checked)
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "INET-7");
					XS(this.lanact.inetp+"/ipv6/mode", "STATIC");

					/* ipaddr, prefix*/
					XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					var tmp = OBJ("l_pl").innerHTML;
					var tmplstpl = tmp.split("/"); /*cut slash */	
					XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				else
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "");

					/* ipaddr, prefix*/
					//OBJ("l_ipaddr").value = "";
					//XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					//var tmp = OBJ("l_pl").innerHTML;
					//var tmplstpl = tmp.split("/"); /*cut slash */	
					//XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				break;

			case "RA":
				if(!OBJ("en_dhcp_pd").checked)
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "INET-7");
					XS(this.lanact.inetp+"/ipv6/mode", "STATIC");

					/* ipaddr, prefix*/
					XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					var tmp = OBJ("l_pl").innerHTML;
					var tmplstpl = tmp.split("/"); /*cut slash */	
					XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				else
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "");

					/* ipaddr, prefix*/
					//OBJ("l_ipaddr").value = "";
					//XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					//var tmp = OBJ("l_pl").innerHTML;
					//var tmplstpl = tmp.split("/"); /*cut slash */	
					//XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				break;

			case "PPPOE":
				if(!OBJ("en_dhcp_pd").checked)
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "INET-7");
					XS(this.lanact.svcs+"/inf/infprevious", "WAN-4");
					XS(this.lanact.inetp+"/ipv6/mode", "STATIC");

					/* ipaddr, prefix*/
					XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					var tmp = OBJ("l_pl").innerHTML;
					var tmplstpl = tmp.split("/"); /*cut slash */	
					XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				else
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "");

					/* ipaddr, prefix*/
					//OBJ("l_ipaddr").value = "";
					//XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					//var tmp = OBJ("l_pl").innerHTML;
					//var tmplstpl = tmp.split("/"); /*cut slash */	
					//XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				break;

			case "6IN4":
				if(OBJ("en_dhcp_pd").checked)
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.svcs+"/inf/inet", "");
				}
				else
				{
					XS(this.lanact.svcs+"/inf/active", "1");
					XS(this.lanact.inetp+"/ipv6/mode", "STATIC");

					/* ipaddr, prefix*/
					XS(this.lanact.inetp+"/ipv6/ipaddr", OBJ("l_ipaddr").value);
					var tmp = OBJ("l_pl").innerHTML;
					var tmplstpl = tmp.split("/"); /*cut slash */	
					XS(this.lanact.inetp+"/ipv6/prefix", tmplstpl[1]);
				}
				break;

			case "6TO4":
				XS(this.lanact.svcs+"/inf/active", "1");
				XS(this.lanact.svcs+"/inf/inet", "");
				//XS(this.lanact.inetp+"/ipv6/mode", "");
				break;
 
			case "6RD":
				XS(this.lanact.svcs+"/inf/active", "1");
				XS(this.lanact.svcs+"/inf/inet", "");
				//XS(this.lanact.inetp+"/ipv6/mode", "");
				break;

                        case "LL":
				//XS(this.lanact.svcs+"/inf/active", "0");//judge dhcps6
				XS(this.lanact.svcs+"/inf/inet", "");
				break;
		}
/*		
		switch(OBJ("lan_auto_type").value)
		{
			case "STATELESS":
		                XS(this.dhcps6+"/mode", "STATELESS");
                                break;

			case "STATEFUL":
		                XS(this.dhcps6+"/mode", "STATEFUL");
                                XS(this.dhcps6+"/start", OBJ("dhcps_start_ip_value").value);
                                XS(this.dhcps6+"/stop", OBJ("dhcps_stop_ip_value").value);
                                break;
		}
*/             
                //if(!OBJ("enableAuto").checked)  XS(this.lanact.inetp+"/ipv6/mode","LL");	
		
		if(OBJ("enableAuto").checked)
		{
			XS(this.lanact.svcs+"/inf/dhcps6", "DHCPS6-1");
			XS(this.dhcps6_inf+"/dhcps6", "DHCPS6-1");
			switch(OBJ("lan_auto_type").value)
			{
				case "STATELESSR":
					XS(this.rdnss+"/device/rdnss","1");
					XS(this.dhcps6+"/mode", "STATELESS");
					if(OBJ("ra_lifetime").value!="")
					XS(this.lanact.inetp+"/ipv6/routerlft", 60*OBJ("ra_lifetime").value);
					break;
				case "STATELESSD":
					XS(this.rdnss+"/device/rdnss","0");
					XS(this.dhcps6+"/mode", "STATELESS");
					if(OBJ("ra_lifetime").value!="")
					XS(this.lanact.inetp+"/ipv6/routerlft", 60*OBJ("ra_lifetime").value);
					break;

				case "STATEFUL":
					XS(this.dhcps6+"/mode", "STATEFUL");
					str = "::"+OBJ("dhcps_start_ip_value").value;
					XS(this.dhcps6+"/start", str);
					//str = "::"+OBJ("dhcps_stop_ip_value").value;
                                	//XS(this.dhcps6+"/stop", str);
					str = parseInt(OBJ("dhcps_stop_ip_value").value,16)-parseInt(OBJ("dhcps_start_ip_value").value,16)+parseInt("1",16);
                                	XS(this.dhcps6+"/count", str);
					//XS(this.dhcps6+"/start", OBJ("dhcps_start_ip_value").value);
					//XS(this.dhcps6+"/stop", OBJ("dhcps_stop_ip_value").value);
					if(OBJ("ip_lifetime").value!="")
					{
						XS(this.lanact.inetp+"/ipv6/preferlft", 60*OBJ("ip_lifetime").value);
						XS(this.lanact.inetp+"/ipv6/validlft", 2*60*OBJ("ip_lifetime").value);
					}
					break;
			}
		}
		else
		{
			XS(this.lanact.svcs+"/inf/dhcps6", "");
			XS(this.dhcps6_inf+"/dhcps6", "");
		}
		
		/* set lan enable pd */	
		switch(OBJ("wan_ipv6_mode").value)
		{
			case "STATIC":
			case "6RD":
			case "6TO4":
                        case "LL":
                		XS(this.dhcps6+"/pd/enable", "0");
				break;
			case "6IN4":
			case "AUTO":
			case "PPPOE":
			case "AUTODETECT":
				if(OBJ("en_lan_pd").checked)
				{
                			XS(this.dhcps6+"/pd/enable", "1");
                			XS(this.dhcps6+"/pd/mode", "1"); /* 0: generic, 1: dlink */
				}
				else
				{
                			XS(this.dhcps6+"/pd/enable", "0");
				}
				break;
		}
		return true;
	},

	PreLANAutoConf: function()
	{
		/* need TODO...*/
		return true;
	},

	InitLANConnMode: function()
	{
		if(!this.dhcps6) return false;
		var rdnss = XG(this.rdnss+"/device/rdnss");
		var lan_type = XG(this.dhcps6+"/mode");
		switch(XG(this.dhcps6+"/mode"))
		{
			case "STATELESS":
				if(rdnss == 1) COMM_SetSelectValue(OBJ("lan_auto_type"), "STATELESSR");
				else COMM_SetSelectValue(OBJ("lan_auto_type"), "STATELESSD"); 
				break;
			case "STATEFUL": 		
				COMM_SetSelectValue(OBJ("lan_auto_type"), "STATEFUL")
				break;
		}
		return true;
	},
	InitDHCPS6: function()
	{
		var svc = PXML.FindModule("DHCPS6.LAN-4");
		var inflp = PXML.FindModule("RUNTIME.INF.LAN-4");
		var str;
		var index;

		//if(XG(this.lanact.inetp+"/ipv6/mode") != "DHCP")
		//    return true;

		str = XG(inflp+"/runtime/inf/dhcps6/network");
		if (!svc || !inflp)
		{
			/*alert("InitDHCPS6() ERROR !");*/
			return false;
		}
		if (!this.dhcps6)   return false;
            
		switch (XG(this.dhcps6+"/mode"))
		{
			case "STATELESS":
                		OBJ("box_lan_dhcp").style.display = "none";
                      		OBJ("box_lan_stless").style.display = "";
				if(XG(this.lanact.inetp+"/ipv6/routerlft") != "")
					OBJ("ra_lifetime").value = XG(this.lanact.inetp+"/ipv6/routerlft")/60;
				else
					OBJ("ra_lifetime").value = "";
                      		break;
                     
                 	case "STATEFUL":
                      		OBJ("box_lan_dhcp").style.display = "";
        			str = XG(this.dhcps6+"/start");
        			if (str)
        			{
              				index = str.lastIndexOf(":");
              				OBJ("dhcps_start_ip_value").value = str.substring(index+1);
        			}
				str1 = OBJ("dhcps_start_ip_value").value;
				str1 = parseInt(str1,16);
				str3 = XG(this.dhcps6+"/count");
				str2 = parseInt(str1,10) + parseInt(str3,10) - parseInt("1",10);
				str2 = Dec2Hex(str2);
				str2 = str2 + "";
				OBJ("dhcps_stop_ip_value").value = str2;
				if(XG(this.lanact.inetp+"/ipv6/preferlft") != "")
					OBJ("ip_lifetime").value = XG(this.lanact.inetp+"/ipv6/preferlft")/60;
				else
					OBJ("ip_lifetime").value = "";
              
                      		if (str)
                      		{
                           		index = str.lastIndexOf("::");
                           		OBJ("dhcps_start_ip_prefix").value = str.substring(0,index);
                           		OBJ("dhcps_stop_ip_prefix").value = str.substring(0,index);
                      		}
                      		else
                      		{
                           		OBJ("dhcps_start_ip_prefix").value = "xxxx";
                           		OBJ("dhcps_stop_ip_prefix").value = "xxxx";
                      		}  
                      		//OBJ("dhcps_start_ip_prefix").disabled = true;
                      		//OBJ("dhcps_stop_ip_prefix").disabled = true;
                      		break;
            	}
        	return true;
	},
/*
	PreDHCPS6: function()
	{
        	var lan = PXML.FindModule("DHCPS6.LAN-4");
            	switch (OBJ("lan_auto_type").value)
            	{	
                	case "STATELESS":
                      		//XS(lan+"/inf/dhcps6",    "");
                      		XS(this.lanact.inetp+"/ipv6/mode", "RA");
                      		XS(this.dhcps6+"/mode", OBJ("lan_auto_type").value);
                      		break;

                	case "STATEFUL":
                      		XS(lan+"/inf/dhcps6",    "DHCPS6-1");
                      		XS(this.dhcps6+"/mode", OBJ("lan_auto_type").value);
                      		XS(this.dhcps6+"/start", OBJ("dhcps_start_ip_value").value);
                      		XS(this.dhcps6+"/stop", OBJ("dhcps_stop_ip_value").value);
                      		XS(this.lanact.inetp+"/ipv6/mode", "DHCP");
                      		break;
            	}	
            	return true;
    	},
*/
	Pre6TO4: function()
	{
		var slaid = parseInt(OBJ("l_ipaddr_6to4").value, 16);
		if(slaid > 65535 || slaid < 0) 
		{
    		BODY.ShowAlert("<?echo i18n("Invalid value for SLA id.");?>");
    		OBJ("l_ipaddr_6to4").focus();
    		return null;
   		}
		XS(this.wanact.inetp+"/ipv6/ipv6in4/ipv6to4/slaid", slaid);
		   		
   		return true;
	},
	PrePppoe: function()
	{
        	var cnt;
           	if(OBJ("pppoe_password").value !== OBJ("confirm_pppoe_password").value)
           	{
                	BODY.ShowAlert("<?echo i18n("The password is mismatched.");?>");
                	return null;
           	}

           	if(OBJ("pppoe_sess_share").checked)
		{
           		XS(this.wan1.inetp+"/addrtype", "ppp10");
           		XS(this.wan1.inetp+"/ppp6/username", OBJ("pppoe_username").value);
           		XS(this.wan1.inetp+"/ppp4/username", OBJ("pppoe_username").value);
           		XS(this.wan1.inetp+"/ppp6/password", OBJ("pppoe_password").value);    
           		XS(this.wan1.inetp+"/ppp4/password", OBJ("pppoe_password").value);    
           
           		XS(this.wan1.inetp+"/ppp6/pppoe/servicename", OBJ("pppoe_service_name").value);
           		XS(this.wan1.inetp+"/ppp4/pppoe/servicename", OBJ("pppoe_service_name").value);

           		XS(this.wan1.inetp+"/ppp4/over", "eth");
           		XS(this.wan1.inetp+"/ppp6/over", "eth");

			XS(this.wan1.infp+"/lowerlayer", "");

           		if(OBJ("pppoe_dynamic").checked)
           		{
                 		XS(this.wan1.inetp+"/ppp6/static", "0");
                		XD(this.wan1.inetp+"/ppp6/ipaddr");
           		}
           		else
           		{
                 		XS(this.wan1.inetp+"/ppp6/static", "1");
                 		XS(this.wan1.inetp+"/ppp6/ipaddr", OBJ("pppoe_ipaddr").value);
                 		if(OBJ("w_dhcp_dns_manual").checked && OBJ("w_dhcp_pdns").value === "")
                 		{
                       			BODY.ShowAlert("<?echo i18n("Invalid Primary DNS address.");?>");
                       			return null;
                 		}
           		}	
           	
			/* dns */
			cnt = 0;
			if (OBJ("w_dhcp_dns_auto").checked)
			{
				XS(this.wan1.inetp+"/ppp6/dns/entry:1","");
				XS(this.wan1.inetp+"/ppp6/dns/entry:2","");
			}
			else
			{
				if(OBJ("w_dhcp_pdns").value != "")
				{
					XS(this.wan1.inetp+"/ppp6/dns/entry:1", OBJ("w_dhcp_pdns").value);
					cnt+=1;
				}
				if(OBJ("w_dhcp_sdns").value != "")
				{
					XS(this.wan1.inetp+"/ppp6/dns/entry:2", OBJ("w_dhcp_sdns").value);
					cnt+=1;
				}
			}
			XS(this.wan1.inetp+"/ppp6/dns/count", cnt);
			XS(this.wan1.inetp+"/ppp4/mtu", OBJ("ppp6_mtu").value);
			XS(this.wan1.inetp+"/ppp6/mtu", OBJ("ppp6_mtu").value);
			/* 
           		if(OBJ("pppoe_max_idle_time").value === "") OBJ("pppoe_max_idle_time").value = 0;
           		if(!TEMP_IsDigit(OBJ("pppoe_max_idle_time").value))
           		{
                		BODY.ShowAlert("<?echo i18n("Invalid value for idle timeout.");?>");
                		return null;
           		}
           		XS(this.wanllact.inetp+"/ppp6/dialup/idletimeout", OBJ("pppoe_max_idle_time").value);
			*/
          		
          		//+++ Jerry Kao, modified for follow the Reconnection Mode of IPv4 PPPoE.       														    						
       			var dialup_v4 = XG(this.wan1.inetp+"/ppp4/dialup/mode");
       			XS(this.wan1.inetp+"/ppp6/dialup/mode", dialup_v4);          
			
       			//+++ Jerry Kao, makred for avoiding Reconnect Mode to be set "Always on" IPv4 PPPoE, 
       			//               when IPv6 PPPoE = "Share with IPv4".
       			//XS(this.wan1.inetp+"/ppp4/dialup/mode", dialup);          

           		/* need to check DHCP-PD is enable or not */
           		//XS(this.wanact.inetp+"/addrtype", "ipv6");
           		//XS(this.wanact.inetp+"/ipv6/mode", "DHCP");
           		XS(this.wanllact.inetp+"/addrtype", "ipv6");
		}
		else
		{
			//+++ Jerry Kao, modified for avoid check username by fatlady when v4 is DHCP
			//               and v6 is PPPoE (Create new session).
			if (XG(this.wan1.inetp+"/addrtype")=="ppp10")
			{
			    XS(this.wan1.inetp+"/addrtype", "ppp4");
			}						              
              			
           		XS(this.wanllact.inetp+"/addrtype", "ppp6");
           		XS(this.wanllact.inetp+"/ppp6/username", OBJ("pppoe_username").value);
           		XS(this.wanllact.inetp+"/ppp6/password", OBJ("pppoe_password").value);    
           
           		XS(this.wanllact.inetp+"/ppp6/pppoe/servicename", OBJ("pppoe_service_name").value);

           		XS(this.wanllact.inetp+"/ppp6/over", "eth");
           	
           		if(OBJ("pppoe_dynamic").checked)
           		{
                 		XS(this.wanllact.inetp+"/ppp6/static", "0");
                		XD(this.wanllact.inetp+"/ppp6/ipaddr");
           		}
           		else
           		{
                 		XS(this.wanllact.inetp+"/ppp6/static", "1");
                 		XS(this.wanllact.inetp+"/ppp6/ipaddr", OBJ("pppoe_ipaddr").value);
                 		if(OBJ("w_dhcp_dns_manual").checked && OBJ("w_dhcp_pdns").value === "")
                 		{
                       			BODY.ShowAlert("<?echo i18n("Invalid Primary DNS address.");?>");
                       			return null;
                 		}
           		}	
           	
           		//XS(this.wanllact.inetp+"/ppp6/static", "0");
           		//XD(this.wanllact.inetp+"/ppp6/ipaddr");

			/* dns */
			cnt = 0;
			if (OBJ("w_dhcp_dns_auto").checked)
			{
				XS(this.wanllact.inetp+"/ppp6/dns/entry:1","");
				XS(this.wanllact.inetp+"/ppp6/dns/entry:2","");
			}
			else
			{
				if(OBJ("w_dhcp_pdns").value != "")
				{
					XS(this.wanllact.inetp+"/ppp6/dns/entry", OBJ("w_dhcp_pdns").value);
					cnt+=1;
				}
				if(OBJ("w_dhcp_sdns").value != "")
				{
					XS(this.wanllact.inetp+"/ppp6/dns/entry:2", OBJ("w_dhcp_sdns").value);
					cnt+=1;
				}
			}
			XS(this.wanllact.inetp+"/ppp6/dns/count", cnt);
			XS(this.wanllact.inetp+"/ppp6/mtu", OBJ("ppp6_mtu").value);
			/* 
           		if(OBJ("pppoe_max_idle_time").value === "") OBJ("pppoe_max_idle_time").value = 0;
           		if(!TEMP_IsDigit(OBJ("pppoe_max_idle_time").value))
           		{
                		BODY.ShowAlert("<?echo i18n("Invalid value for idle timeout.");?>");
                		return null;
           		}
           		XS(this.wanllact.inetp+"/ppp6/dialup/idletimeout", OBJ("pppoe_max_idle_time").value);
			*/
          		
       			//var dialup = "ondemand";
			var dialup = "auto";
       			if(OBJ("pppoe_alwayson").checked)		dialup = "auto";
       			else if(OBJ("pppoe_manual").checked)    dialup = "manual";
		
           		XS(this.wanllact.inetp+"/ppp6/dialup/mode", dialup);          

           		/* need to check DHCP-PD is enable or not */
           		//XS(this.wanact.inetp+"/addrtype", "ipv6");
           		//XS(this.wanact.inetp+"/ipv6/mode", "DHCP");
		}

		//clear dns of wanact
		XS(this.wanact.inetp+"/ipv6/dns/count", "0");
		XS(this.wanact.inetp+"/ipv6/dns/entry:1", "");
		XS(this.wanact.inetp+"/ipv6/dns/entry:2", "");
		
           	return true;
	},
        
        /*PPPoE*/
	OnClickPppoeAddrType: function()
	{
		OBJ("pppoe_ipaddr").disabled = OBJ("pppoe_dynamic").checked ? true: false;
		if(OBJ("pppoe_static").checked)
		{
			OBJ("box_lan_pd_body").style.display		= "none";
			OBJ("box_lan_auto").style.display 		= "";
			OBJ("box_lan_auto_body").style.display 		= "";
			OBJ("box_lan_auto_pd").style.display 		= "none";
			OBJ("box_lan_auto_pd_body").style.display 	= "none";
			//hendry, static must fill the 'LAN IPv6 Address'
			OBJ("en_dhcp_pd").checked = false;
			this.OnClickpd();
			if(this.hintsupport == "1")
			{
				this.OnClickpdhint();
			}
		}
		else
		{
			OBJ("box_lan_pd_body").style.display		= "";
			OBJ("box_lan_auto").style.display 		= "none";
			OBJ("box_lan_auto_body").style.display 		= "";
			OBJ("box_lan_auto_pd").style.display 		= "";
			OBJ("box_lan_auto_pd_body").style.display 	= "";
			
			this.OnClickpd();
			if(this.hintsupport == "1")
			{
				this.OnClickpdhint();
			}
		}
	},

	LoadPpp4Value: function()
	{
		OBJ("pppoe_username").value = XG(this.wan1.inetp+"/ppp4/username");
		OBJ("pppoe_password").value = XG(this.wan1.inetp+"/ppp4/password");
		OBJ("confirm_pppoe_password").value = OBJ("pppoe_password").value;
		OBJ("pppoe_service_name").value = XG(this.wan1.inetp+"/ppp4/pppoe/servicename");
		OBJ("ppp6_mtu").value = XG(this.wan1.inetp+"/ppp4/mtu");

		var dialup = XG(this.wan1.inetp+"/ppp4/dialup/mode");
		if	(dialup === "auto")	OBJ("pppoe_alwayson").checked = true;
		else if	(dialup === "manual")	OBJ("pppoe_manual").checked = true;
		else				OBJ("pppoe_ondemand").checked = true;
	},
        OnClickPppoeReconnect: function()
        {
		/*
                if(OBJ("pppoe_alwayson").checked)
                {
                      OBJ("pppoe_max_idle_time").disabled = true;
                }
                else if(OBJ("pppoe_ondemand").checked)
                {
                      OBJ("pppoe_max_idle_time").disabled = false;
                }
                else
                {
                      OBJ("pppoe_max_idle_time").disabled = true;
                }
		*/
        },

	OnClickDHCPDNS: function()
	{
		OBJ("w_dhcp_pdns").disabled = OBJ("w_dhcp_sdns").disabled = OBJ("w_dhcp_dns_auto").checked ? true: false;
	},
  	
	OnClickPppoeSessType: function()
	{
		if(OBJ("pppoe_sess_share").checked)
		{
			OBJ("pppoe_alwayson").checked = true;
			//OBJ("pppoe_alwayson").disabled = OBJ("pppoe_manual").disabled = true;
			OBJ("pppoe_alwayson").disabled = OBJ("pppoe_ondemand").disabled = OBJ("pppoe_manual").disabled = true;

			OBJ("pppoe_username").disabled = true;
			OBJ("pppoe_password").disabled = true;
			OBJ("confirm_pppoe_password").disabled = true;
			OBJ("pppoe_service_name").disabled = true;
		}
		else
		{
			//OBJ("pppoe_alwayson").disabled = OBJ("pppoe_manual").disabled = false;
			OBJ("pppoe_alwayson").disabled = OBJ("pppoe_manual").disabled = false;
						
			OBJ("pppoe_ondemand").disabled = true;	// Not Support ondemand in "Create a New Session" mode.

			OBJ("pppoe_username").disabled = false;
			OBJ("pppoe_password").disabled = false;
			OBJ("confirm_pppoe_password").disabled = false;
			OBJ("pppoe_service_name").disabled = false;
		}
		 OBJ("pppoe_sess_share").setAttribute("modified", "true");
	},

	OnClickUsell: function()
	{
		OBJ("w_st_ipaddr").disabled = OBJ("w_st_pl").disabled = OBJ("usell").checked ? true: false;
		if(OBJ("usell").checked)
		{
			var r3ipaddr = XG(this.rwan3+"/runtime/inf/inet/ipv6/ipaddr");
			if(r3ipaddr=="")	r3ipaddr = this.wanllact.ipv6ll;
			OBJ("w_st_ipaddr").value	= r3ipaddr;
			OBJ("w_st_pl").value		= 64;
		}
		else
		{
			OBJ("w_st_ipaddr").value	= XG(this.wanact.inetp+"/ipv6/ipaddr");
			OBJ("w_st_pl").value		= XG(this.wanact.inetp+"/ipv6/prefix");
		}
	},

    	MyArrayShowAlert: function( promptInfo, target )
	{
		alert( promptInfo	+ ": " +
			target.svcsname	+ "  " +
			target.svcs		+ "  " +
			target.inetuid	+ "  " +
             			target.inetp	+ "  " +
			target.phyinf	+ "  " +
			target.ipv6ll);	
	}
}

function GetRadioValue(name)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].checked)	return obj[i].value;
	}
}

function SetRadioValue(name, value)
{
	var obj = document.getElementsByName(name);
	for (var i=0; i<obj.length; i++)
	{
		if (obj[i].value==value)
		{
			obj[i].checked = true;
			break;
		}
	}
}

function Dec2Hex(DecVal)
{
	var HexChars = "0123456789ABCDEF";
	DevVal = parseInt(DecVal);
	if(DecVal > 255 || DecVal < 0) DecVal = 255;
	var Dig1 = DecVal%16;
	var Dig2 = (DecVal-Dig1)/16;
	var HexVal = HexChars.charAt(Dig2)+HexChars.charAt(Dig1);
	return HexVal;
}  

function CutFirstSpace(AddrStr)
{
	var i=0;
	while(i<AddrStr.length)
	{
		if(i==0)
		{
			i++;
			continue;
		}
		if( AddrStr.charAt(i-1)==":" && AddrStr.charAt(i)=="0")
		{
			AddrStr = AddrStr.substring(0,i)+AddrStr.substring(i+1,AddrStr.length);
			i=0;
			continue;
		}
		i++;
	}
	return AddrStr;
}
</script>
