<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
select.broad	{ width: 130px; }
select.narrow	{ width: 65px; }
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "PFWD.NAT-1",
	OnLoad: function()
	{
		/* draw the 'Application Name' select */
		var str = "";
		for(var i=1; i<=<?=$PFWD_MAX_COUNT?>; i+=1)
		{
			str = "";
			str += '<select id="app_'+i+'" class="broad">';
			for(var j=0; j<this.apps.length; j+=1)
				str += '<option value="'+j+'">'+this.apps[j].name+'</option>';
			str += '</select>';
			OBJ("span_app_"+i).innerHTML = str;
		}
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result) { return false; },
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("PFWD.NAT-1");
		if (p === "") alert("ERROR!");
		p += "/nat/entry/portforward";
		TEMP_RulesCount(p, "rmd");
		var count = XG(p+"/count");
		var netid = COMM_IPv4NETWORK(this.lanip, this.mask);
		for (var i=1; i<=<?=$PFWD_MAX_COUNT?>; i+=1)
		{
			var b = p+"/entry:"+i;
			OBJ("uid_"+i).value = XG(b+"/uid");
			OBJ("en_"+i).checked = XG(b+"/enable")==="1";
			OBJ("dsc_"+i).value = XG(b+"/description");
			OBJ("port_tcp_"+i).value = XG(b+"/tport_str");
			OBJ("port_udp_"+i).value = XG(b+"/uport_str");
			<?
			if ($FEATURE_NOSCH!="1")	echo 'COMM_SetSelectValue(OBJ("sch_"+i), (XG(b+"/schedule")=="")? "-1":XG(b+"/schedule"));\n';
			if ($FEATURE_INBOUNDFILTER=="1")	echo 'COMM_SetSelectValue(OBJ("inbfilter_"+i), (XG(b+"/inbfilter")=="")? "-1":XG(b+"/inbfilter"));\n';
			?>
			var hostid = XG(b+"/internal/hostid");
			if (hostid !== "")	OBJ("ip_"+i).value = COMM_IPv4IPADDR(netid, this.mask, hostid);
			else				OBJ("ip_"+i).value = "";
			OBJ("pc_"+i).value = "";
		}
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("PFWD.NAT-1");
		p += "/nat/entry/portforward";
		var old_count = parseInt(XG(p+"/count"), 10);
		var cur_count = 0;
		var cur_seqno = parseInt(XG(p+"/seqno"), 10);
		/* delete the old entries
		 * Notice: Must delte the entries from tail to head */
		while(old_count > 0)
		{
			XD(p+"/entry:"+old_count);
			old_count -= 1;
		}
		/* update the entries */
		for (var i=1; i<=<?=$PFWD_MAX_COUNT?>; i+=1)
		{
			OBJ("port_tcp_"+i).value = COMM_EatAllSpace(OBJ("port_tcp_"+i).value);
			OBJ("port_udp_"+i).value = COMM_EatAllSpace(OBJ("port_udp_"+i).value);
			if (OBJ("port_tcp_"+i).value!="" && !check_valid_port(OBJ("port_tcp_"+i).value))
			{
				BODY.ShowAlert("<?echo I18N("j", "The input TCP port range is invalid.");?>");
				OBJ("port_tcp_"+i).focus();
				return null;
			}
			if (OBJ("port_udp_"+i).value!="" && !check_valid_port(OBJ("port_udp_"+i).value))
			{
				BODY.ShowAlert("<?echo I18N("j", "The input UDP port range is invalid.");?>");
				OBJ("port_udp_"+i).focus();
				return null;
			}
			if (OBJ("ip_"+i).value!="" && !TEMP_CheckNetworkAddr(OBJ("ip_"+i).value, null, null))
			{
				BODY.ShowAlert("<?echo I18N("j", "Invalid host IP address.");?>");
				OBJ("ip_"+i).focus();
				return null;
			}
			if(OBJ("port_tcp_"+i).value=="" &&OBJ("port_udp_"+i).value=="" &&OBJ("dsc_"+i).value!="")
			{
				BODY.ShowAlert("<?echo I18N("j", "Invalid Port !");?>");
				OBJ("port_tcp_"+i).focus();
				return null;
			}
			/* if the description field is empty, it means to remove this entry,
			 * so skip this entry. */
			if (OBJ("dsc_"+i).value!=="")
			{
				cur_count+=1;
				var b = p+"/entry:"+cur_count;
				XS(b+"/enable",			OBJ("en_"+i).checked ? "1" : "0");
				XS(b+"/uid",			OBJ("uid_"+i).value);
				if (OBJ("uid_"+i).value == "")
				{
					XS(b+"/uid",	"PFWD-"+cur_seqno);
					cur_seqno += 1;
				}
				<?
				if ($FEATURE_NOSCH!="1")	echo 'XS(b+"/schedule",		(OBJ("sch_"+i).value==="-1") ? "" : OBJ("sch_"+i).value);\n';
				if ($FEATURE_INBOUNDFILTER=="1")	echo 'XS(b+"/inbfilter",	(OBJ("inbfilter_"+i).value==="-1") ? "" : OBJ("inbfilter_"+i).value);\n';
				?>
				XS(b+"/description",	OBJ("dsc_"+i).value);
				XS(b+"/internal/inf",	"LAN-1");
				if (OBJ("ip_"+i).value == "") XS(b+"/internal/hostid", "");
				else XS(b+"/internal/hostid",COMM_IPv4HOST(OBJ("ip_"+i).value, this.mask));
				XS(b+"/tport_str",	OBJ("port_tcp_"+i).value);
				XS(b+"/uport_str",	OBJ("port_udp_"+i).value);
			}
			
			//Check the port confliction between STORAGE, VSVR, PFWD and REMOTE.
			var admin_remote_port = "<? echo query(INF_getinfpath("WAN-1")."/web");?>";
			var admin_remote_port_https = "<? echo query(INF_getinfpath("WAN-1")."/https_rport");?>";
			if(PortStringCheck(OBJ("port_tcp_"+i).value, admin_remote_port) || 
				PortStringCheck(OBJ("port_tcp_"+i).value, admin_remote_port_https))
			{
				BODY.ShowAlert("<?echo i18n("The input TCP port could not be the same as the remote admin port in the administration function.");?>");
				OBJ("port_tcp_"+i).focus();
				return null;			
			}
			var webaccess_remote_en = <? if(query("/webaccess/enable")==1 && query("/webaccess/remoteenable")==1) echo "true" ;else echo "false";?>;
			var webaccess_httpport = "<? echo query("/webaccess/httpport");?>";
			var webaccess_httpsport = "<? echo query("/webaccess/httpsport");?>";			
			if(webaccess_remote_en && (PortStringCheck(OBJ("port_tcp_"+i).value, webaccess_httpport) ||
				PortStringCheck(OBJ("port_tcp_"+i).value, webaccess_httpsport)))
			{
				BODY.ShowAlert("<?echo i18n("The input TCP port could not be the same as the web access port in the storage function.");?>");
				OBJ("port_tcp_"+i).focus();
				return null;			
			}			
		}
		// Make sure the different rules have different names and public port ranges.
		for (var i=1; i<<?=$PFWD_MAX_COUNT?>; i+=1)
		{
			/* if the description field is empty, it means to remove this entry, so skip this entry. */
			if(OBJ("dsc_"+i).value=="") continue;
			for (var j=i+1; j<=<?=$PFWD_MAX_COUNT?>; j+=1)
			{				
				/* if the description field is empty, it means to remove this entry, so skip this entry. */
				if(OBJ("dsc_"+j).value=="") continue;
				var sch_i = <? if ($FEATURE_NOSCH!="1")	echo 'OBJ("sch_"+i).value;'; else echo 'null;'; ?>
				var sch_j = <? if ($FEATURE_NOSCH!="1")	echo 'OBJ("sch_"+j).value;'; else echo 'null;'; ?>
				var inbf_i = <? if ($FEATURE_INBOUNDFILTER=="1")	echo 'OBJ("inbfilter_"+i).value;'; else echo 'null;'; ?>
				var inbf_j = <? if ($FEATURE_INBOUNDFILTER=="1")	echo 'OBJ("inbfilter_"+j).value;'; else echo 'null;'; ?>				
				
				if(OBJ("dsc_"+i).value != "" && OBJ("dsc_"+j).value !="") 
				{					
					if(OBJ("port_tcp_"+i).value===OBJ("port_tcp_"+j).value &&
						OBJ("port_udp_"+i).value===OBJ("port_udp_"+j).value &&
						(sch_i==null || sch_i===sch_j) &&
						(inbf_i==null || inbf_i===inbf_j)) 
					{
						BODY.ShowAlert("<?echo I18N("j", "The rules could not be the same.");?>");
						OBJ("dsc_"+j).focus();
						return null;
					}
					if(OBJ("dsc_"+i).value === OBJ("dsc_"+j).value) 
					{
						BODY.ShowAlert("<?echo I18N("j", "The different rules could not set the same name.");?>");
						OBJ("dsc_"+j).focus();
						return null;
					}
				}	
				
				if(PortStringCheck(OBJ("port_tcp_"+i).value, OBJ("port_tcp_"+j).value))
				{
					BODY.ShowAlert("<?echo I18N("j", "The TCP ports are overlapping.");?>");
					OBJ("port_tcp_"+i).focus();
					return null;					
				}					
				if(PortStringCheck(OBJ("port_udp_"+i).value, OBJ("port_udp_"+j).value))
				{
					BODY.ShowAlert("<?echo I18N("j", "The UDP ports are overlapping.");?>");
					OBJ("port_udp_"+i).focus();
					return null;					
				}					
			}	
		}		
		XS(p+"/count", cur_count);
		XS(p+"/seqno", cur_seqno);
		return PXML.doc;
	},
	IsDirty: null,
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	apps: [	{name: "<?echo I18N("h", "Application Name");?>",value:{tport:"", uport:""}},
			{name: "Age of Empires",					value:{tport:"2302-2400,6073",
															   uport:"2302-2400,6073"}},
			{name: "Aliens vs. Predator",               value:{tport:"80,2300-2400,8000-8999",                                                                 
															   uport:"80,2300-2400,8000-8999"}},
			{name: "America Army",                      value:{tport:"20045",                                                                                       
															   uport:"1716-1718,8777,27900"}},
			{name: "Asheron Call",                      value:{tport:"9000-9013",                                                                                         
															   uport:"2001,9000-9013"}},
			{name: "Battlefield 1942",                  value:{tport:"",                                                                             
															   uport:"14567,22000,23000-23009,27900,28900"}},
			{name: "Battlefield 2",                     value:{tport:"80,4711,29900,29901,29920,28910",                        
															   uport:"1500-4999,16567,27900,29900,29910,27901,55123,55124,55215"}},
			{name: "Battlefield: Vietnam",              value:{tport:"",                                                                                    
															   uport:"4755,23000,22000,27243-27245"}},
			{name: "BitTorrent",                        value:{tport:"6881-6889",uport:""}},                                                                               
			{name: "Black and White",                   value:{tport:"2611-2612,6500,6667,27900",                                                              
															   uport:"2611-2612,6500,6667,27900"}},
			{name: "Call of Duty",                      value:{tport:"28960",                                                                                          
															   uport:"20500,20510,28960"}},
			{name: "Command and Conquer Generals",      value:{tport:"80,6667,28910,29900,29920",                                                                             
															   uport:"4321,27900"}},
			{name: "Command and Conquer Zero Hour",     value:{tport:"80,6667,28910,29900,29920",                                                                             
															   uport:"4321,27900"}},
			{name: "Counter Strike",                    value:{tport:"27030-27039",       
															   uport:"1200,27000-27015"}},                                                                      
			{name: "D-Link DVC-1000",                   value:{tport:"1720,15328-15333",                                                                                     
															   uport:"15328-15333"}},
			{name: "Dark Reign 2",                   	value:{tport:"26214",                                                                                                      
															   uport:"26214"}},
			{name: "Delta Force",                      	value:{tport:"3100-3999",                                                                                                   
															   uport:"3568"}},
			{name: "Diablo I and II",                       value:{tport:"6112-6119,4000",                                                                                         
															   uport:"6112-6119"}},
			{name: "Doom 3",                   			value:{tport:"",                                                                                                           
															   uport:"27666"}},
			{name: "Dungeon Siege",                     value:{tport:"",                                                                                                  
															   uport:"6073,2302-2400"}},
			{name: "eDonkey",                	        value:{tport:"4661-4662",                                                                                                   
															   uport:"4665"}},
			{name: "eMule",                           	value:{tport:"4661-4662,4711",                                                                                         
															   uport:"4672,4665"}},
			{name: "Everquest",                         value:{tport:"1024-6000,7000",                                                                                    
															   uport:"1024-6000,7000"}},
			{name: "Far Cry",                         	value:{tport:"",                                                                                                     
															   uport:"49001,49002"}},
			{name: "Final Fantasy XI (PC)",             value:{tport:"25,80,110,443,50000-65535",                                                                            
															   uport:"50000-65535"}},
			{name: "Final Fantasy XI (PC2)",            value:{tport:"1024-65535",                                                                                           
															   uport:"50000-65535"}},
			{name: "Gamespy Arcade",            		value:{tport:"",                                                                                                            
															   uport:"6500"}},
			{name: "Gamespy Tunnel",                    value:{tport:"",                                                                                                            
															   uport:"6700"}},
			{name: "Ghost Recon",                    	value:{tport:"2346-2348",                                                                                              
															   uport:"2346-2348"}},
			{name: "Gnutella",                       	value:{tport:"6346",                                                                                                     
															   uport:"6346"}},
			{name: "Half Life",                         value:{tport:"6003,7002",                                                                               
															   uport:"27005,27010,27011,27015"}},
			{name: "Combat Evolved",                    value:{tport:"",                                                                                                       
															   uport:"2302,2303"}},
			{name: "Heretic II",                    	value:{tport:"28910",                                                                                                      
															   uport:"28910"}},
			{name: "Hexen II",                        	value:{tport:"26900",                                                                                                      
															   uport:"26900"}},
			{name: "Jedi Knight II: Jedi Outcast",      value:{tport:"",                                                                                   
															   uport:"28060,28061,28062,28070-28081"}},
			{name: "Jedi Knight III: Jedi Academy",     value:{tport:"",                                                                                   
															   uport:"28060,28061,28062,28070-28081"}},
			{name: "KALI",     							value:{tport:"",
															   uport:"2213,6666"}},                                                                                                       
			{name: "Links",                             value:{tport:"2300-2400,47624",                                                                                   
															   uport:"2300-2400,6073"}},
			{name: "Medal of Honor: Games",             value:{tport:"12203-12204", uport:""}},                                                                                 
			{name: "MSN Game Zone",             		value:{tport:"6667",                                                                                                 
															   uport:"28800-29000"}},
			{name: "MSN Game Zone (DX)",                value:{tport:"2300-2400,47624",                                                                                        
															   uport:"2300-2400"}},
			{name: "Myth",                				value:{tport:"3453",                                                                                                        
															   uport:"3453"}},
			{name: "Need Speed",                        value:{tport:"9442",                                                                                                        
															   uport:"9442"}},
			{name: "Need Speed 3",                      value:{tport:"1030",                                                                                                        
															   uport:"1030"}},
			{name: "Need Speed: Hot Pursuit 2",         value:{tport:"8511,28900",                                                                           
															   uport:"1230,8512,27900,61200-61230"}},
			{name: "Neverwinter Nights",         		value:{tport:"",                                                                                      
															   uport:"5120-5300,6500,27900,28900"}},
			{name: "PainKiller",                		value:{tport:"",                                                                                                            
															   uport:"3455"}},
			{name: "PlayStation2",                      value:{tport:"4658,4659",                                                                                              
															   uport:"4658,4659"}},
			{name: "Postal 2: Share the Pain",          value:{tport:"80",                                                                                         
															   uport:"7777-7779,27900,28900"}},
			{name: "Quake 2",         				 	value:{tport:"27910",                                                                                                      
															   uport:"27910"}},
			{name: "Quake 3",                           value:{tport:"27660,27960",                                                                                       
															   uport:"27660,27960"}},
			{name: "Rainbow Six",                       value:{tport:"2346",                                                                                                        
															   uport:"2346"}},                                                                                                        
			{name: "Rainbow Raven: Raven Shield",       value:{tport:"",                                                                                             
															   uport:"7777-7787,8777-8787"}},                                    
			{name: "Return to Castle Wolfenstein",      value:{tport:"",                                                      
															   uport:"27950,27960,27965,27952"}},                               
			{name: "Rise of Nations",      				value:{tport:"",                                                      
															   uport:"34987"}},                                                 
			{name: "Roger Wilco",                   	value:{tport:"3782",                                                  
															   uport:"27900,28900,3782-3783"}},
			{name: "Rogue Spear",                       value:{tport:"2346",                                                  
															   uport:"2346"}},                                                  
			{name: "Serious Sam II",                    value:{tport:"25600-25605",                                           
															   uport:"25600-25605"}},                                           
			{name: "Shareaza",                    		value:{tport:"6346",                                                  
															   uport:"6346"}},                                                  
			{name: "Silent Hunter II",                  value:{tport:"3000",                                                  
															   uport:"3000"}},                                                  
			{name: "Soldier of Fortune",                value:{tport:"",                                                      
															   uport:"28901,28910,38900-38910,22100-23000"}},
			{name: "Soldier of Fortune II",             value:{tport:"",                                                      
															   uport:"20100-20112"}},                                                                                         
			{name: "Splinter Cell: Pandora Tomorrow",   value:{tport:"40000-43000",                                                                                
															   uport:"44000-45001,7776,8888"}},                                       
			{name: "Star Trek: Elite Force II",   		value:{tport:"",                                                  
															   uport:"29250,29256"}},                                                  
			{name: "Starcraft",         				value:{tport:"6112-6119,4000",                                    
															   uport:"6112-6119"}},                                        
			{name: "Starsiege Tribes",                  value:{tport:"",                                                                    
															   uport:"27999,28000"}},                                                                    
			{name: "Steam",                  			value:{tport:"27030-27039",                                                         
															   uport:"1200,27000-27015"}},                                                         
			{name: "SWAT 4",                            value:{tport:"",                                                                    
															   uport:"10480-10483"}},
			{name: "TeamSpeak",                         value:{tport:"",                                                                    
															   uport:"8767"}},                                                                    
			{name: "Tiberian Sun",                      value:{tport:"1140-1234,4000",                                    
															   uport:"1140-1234,4000"}},                                    
			{name: "Tiger Woods 2K4",                   value:{tport:"80,443,1791-1792,13500,20801-20900,32768-65535",    
															   uport:"80,443,1791-1792,13500,20801-20900,32768-65535"}},    
			{name: "Tribes of Vengeance",               value:{tport:"7777,7778,28910",                                   
															   uport:"6500,7777,7778,27900"}},                                   
			{name: "Ubi.com",               			value:{tport:"40000-42999",                                       
															   uport:"41005"}},                                            
			{name: "Ultima",                           	value:{tport:"5001-5010,7775-7777,7875,8800-8900,9999",           
															   uport:"5001-5010,7775-7777,7875,8800-8900,9999"}},
			{name: "Unreal",                            value:{tport:"7777,8888,27900",                                            
															   uport:"7777-7781"}},                                            
			{name: "Unreal Tournament",                 value:{tport:"7777-7783,8080,27900",                                         
															   uport:"7777-7783,8080,27900"}},                                         
			{name: "Unreal Tournament_2004",            value:{tport:"28902",                                             
															   uport:"7777-7778,7787-7788"}},                                             
			{name: "Vietcong",            				value:{tport:"",                                                                    
															   uport:"5425,15425,28900"}},                                                                    
			{name: "Warcraft II",                       value:{tport:"6112-6119,4000",                                    
															   uport:"6112-6119"}},                                        
			{name: "Warcraft III",                      value:{tport:"6112-6119,4000",                                                      
															   uport:"6112-6119"}},
			{name: "WinMX",                      		value:{tport:"6699",                                                                
															   uport:"6257"}},                                                                
			{name: "Wolfenstein: Enemy Territory",      value:{tport:"",                                                                    
															   uport:"27950,27960,27965,27952"}},                                                                    
			{name: "WON Servers",      					value:{tport:"27000-27999",                                                        
															   uport:"15001,15101,15200,15400"}},                                                        
			{name: "World of Warcraft",                 value:{tport:"3724,6112,6881-6999", uport:""}},                                                 
			{name: "Xbox Live",                 		value:{tport:"3074", uport:"88,3074"}}
		  ],
	lanip: "<? echo INF_getcurripaddr("LAN-1"); ?>",
	mask: "<? echo INF_getcurrmask("LAN-1"); ?>",
	CursorFocus: function(node)
	{
		var i = node.lastIndexOf("entry:");
		if(node.charAt(i+7)==="/") var idx = parseInt(node.charAt(i+6), 10);
		else var idx = parseInt(node.charAt(i+6), 10)*10 + parseInt(node.charAt(i+7), 10);
		var indx = 1;
		var valid_dsc_cnt = 0;		
		for(indx=1; indx <= <?=$PFWD_MAX_COUNT?>; indx++)
		{
			if(OBJ("dsc_"+indx).value!=="") valid_dsc_cnt++;
			if(valid_dsc_cnt===idx) break;
		}
		if(node.match("description"))			OBJ("dsc_"+indx).focus();
		else if(node.match("internal/hostid"))	OBJ("ip_"+indx).focus();
		else if(node.match("tport_str"))		OBJ("port_tcp_"+indx).focus();
		else if(node.match("uport_str"))		OBJ("port_udp_"+indx).focus();
	}
};

function OnClickAppArrow(idx)
{
	var i = OBJ("app_"+idx).value;
	OBJ("dsc_"+idx).value = (i==="0") ? "" : PAGE.apps[i].name;
	OBJ("port_tcp_"+idx).value = PAGE.apps[i].value.tport;
	OBJ("port_udp_"+idx).value = PAGE.apps[i].value.uport;
	OBJ("app_"+idx).selectedIndex = 0;
}
function OnClickPCArrow(idx)
{
	OBJ("ip_"+idx).value = OBJ("pc_"+idx).value;
	OBJ("pc_"+idx).selectedIndex = 0;
}

function CheckPort(port)
{
	var vals = port.toString().split("-");
	switch (vals.length)
	{
	case 1:
		if (!TEMP_IsDigit(vals))
			return false;
		break;
	case 2:
		if (!TEMP_IsDigit(vals[0])||!TEMP_IsDigit(vals[1]))
			return false;
		break;
	default:
		return false;
	}
	return true;
}
function check_valid_port(list)
{
	var port = list.split(",");

	if (port.length > 1)
	{
		for (var i=0; i<port.length; i++)
		{
			if (!CheckPort(port[i]))
				return false;
		}
		return true;
	}
	else
	{
		return CheckPort(port);
	}
}
function PortStringCheck(PortString1, PortString2)
{
	var PortStrArr1 = PortString1.split(",");
	var PortStrArr2 = PortString2.split(",");
	for(var i=0; i < PortStrArr1.length; i++)
	{
		for(var j=0; j < PortStrArr2.length; j++)
		{
			if(PortStrArr1[i].match("-")=="-" && PortStrArr2[j].match("-")=="-")
			{
				var PortRange1 = PortStrArr1[i].split("-");
				var PortRangeStart1	= parseInt(PortRange1[0], 10);
				var PortRangeEnd1	= parseInt(PortRange1[1], 10);
				var PortRange2 = PortStrArr2[j].split("-");
				var PortRangeStart2	= parseInt(PortRange2[0], 10);
				var PortRangeEnd2	= parseInt(PortRange2[1], 10);
				if(PortRangeStart2 <= PortRangeEnd1 &&  
					PortRangeStart1 <= PortRangeEnd2) return true;
			}	
			else if(PortStrArr1[i].match("-")=="-")
			{
				var PortRange1 = PortStrArr1[i].split("-");
				var PortRangeStart1	= parseInt(PortRange1[0], 10);
				var PortRangeEnd1	= parseInt(PortRange1[1], 10);
				if(PortRangeStart1 <= parseInt(PortStrArr2[j], 10) && 
					parseInt(PortStrArr2[j], 10) <= PortRangeEnd1) return true;
			}
			else if(PortStrArr2[j].match("-")=="-")
			{
				var PortRange2 = PortStrArr2[j].split("-");
				var PortRangeStart2	= parseInt(PortRange2[0], 10);
				var PortRangeEnd2	= parseInt(PortRange2[1], 10);
				if(PortRangeStart2 <= parseInt(PortStrArr1[i], 10) && 
					parseInt(PortStrArr1[i], 10) <= PortRangeEnd2) return true;
			}
			else
			{
				if(parseInt(PortStrArr1[i], 10)==parseInt(PortStrArr2[j], 10)) return true;
			}					
		}
	}
	return false;
}
</script>
