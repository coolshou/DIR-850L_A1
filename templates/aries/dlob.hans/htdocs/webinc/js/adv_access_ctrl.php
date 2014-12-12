<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "ACCESSCTRL, SCHEDULE, INET.LAN-1",
	OnLoad: function() 
	{
		this.Dirty = false;
		if (!this.rgmode)
		{
			BODY.DisableCfgElements(true);
		}
	},
	OnReload: function()
	{
		<?
			if(query("/runtime/device/modelname")=="DIR-865L")
			{
				echo 'if(confirm("'.I18N("j", "The function would take effect after rebooting the device. Would you like to reboot the device now?").'"))'.'\n';
				echo '		{self.location.href = "/tools_system.php";}';
			}	
		?>
		this.OnLoad();	
	},		
	OnUnload: function() {},
	OnSubmitCallback: function(code, result){return false;},
	InitValue: function(xml)
	{		
		PXML.doc = xml;
		PXML.IgnoreModule("SCHEDULE");
		PXML.IgnoreModule("INET.LAN-1");
		
		if (this.polcur)delete this.polcur;
		if (this.pol) 	delete this.pol;
		if (this.mac) 	delete this.mac;
		if (this.fil) 	delete this.fil;
		this.polcur = new Array();
		this.pol 	= new Array();
		this.mac 	= new Array();
		this.fil 	= new Array();
		
		var p = PXML.FindModule("ACCESSCTRL");
		p += "/acl/accessctrl";
		this.policy_num = XG(p+"/count");
		if(XG(p+"/max")!=="") this.max_policy_num = XG(p+"/max");
		else this.max_policy_num = 15;
		if(XG(p+"/machine_max")!=="") this.max_machine_num = XG(p+"/machine_max");
		else this.max_machine_num = 8;
		
		var cnt = S2I(XG(p+"/entry#"));
		var k=0;
		var h=0;
		for (var i=0; i<cnt; i++)
		{
			var ii=i+1;
			this.pol[i] = 
			{
				uid:	XG(p+"/entry:"+ii+"/uid"),
				enable:	XG(p+"/entry:"+ii+"/enable"),
				desc:	XG(p+"/entry:"+ii+"/description"),
				act:	XG(p+"/entry:"+ii+"/action"),
				portf:	XG(p+"/entry:"+ii+"/portfilter/enable"),
				webf:	XG(p+"/entry:"+ii+"/webfilter/enable"),
				weblog:	XG(p+"/entry:"+ii+"/webfilter/logging"),
				sch:	XG(p+"/entry:"+ii+"/schedule")===""?"<?echo I18N("j","always");?>":XG(p+"/entry:"+ii+"/schedule")
			};
				
			var s = p+"/entry:"+ii+"/machine";
			var maccnt = S2I(XG(s+"/entry#"));
			for (var j=1; j<=maccnt; j++)
			{	
				this.mac[k] = 
				{
					uid:	XG(p+"/entry:"+ii+"/uid"),
					typ:	XG(s+"/entry:"+j+"/type"),	
					val:	XG(s+"/entry:"+j+"/value")
				};
				k++;
			}
			
			var r = p+"/entry:"+ii+"/portfilter";
			var filcnt = S2I(XG(r+"/entry#"));
			for (var j=1; j<=filcnt; j++)
			{	
				this.fil[h] = 
				{
					uid:		XG(p+"/entry:"+ii+"/uid"),
					filname:	XG(r+"/entry:"+j+"/name"),	
					enable:		XG(r+"/entry:"+j+"/enable"),
					startip:	XG(r+"/entry:"+j+"/startip"),	
					endip:		XG(r+"/entry:"+j+"/endip"),
					prot:		XG(r+"/entry:"+j+"/protocol"),	
					startport:	XG(r+"/entry:"+j+"/startport"),					
					endport:	XG(r+"/entry:"+j+"/endport")
				};
				h++;
			}
		}	

		this.StageClose();
		this.StageMain();
		if(XG(p+"/enable") === "1")
		{
			OBJ("en_access").checked = true;
			OBJ("add_policy").style.display = "block";
			OBJ("policytableframe").style.display = "block";
			this.InsectPolicyTable();
		}
		else
		{
			OBJ("en_access").checked = false;
			OBJ("add_policy").style.display = "none";
			OBJ("policytableframe").style.display = "none";
		}
		
		if(parseInt(this.policy_num, 10) < parseInt(this.max_policy_num, 10)) OBJ("add_policy").disabled = false;
		else OBJ("add_policy").disabled = true;
		
		return true;
	},
	InsectPolicyTable: function()
	{
		BODY.CleanTable("policytable");
		for (var i=0; i<this.pol.length; i+=1)
		{
			var Filtering = "";
			var Logged = "";
			if(this.pol[i].act === "LOGWEBONLY")	Filtering = "Log Web Access Only";
			else if(this.pol[i].act === "BLOCKALL")	Filtering = "Block All Access";
			else	Filtering = "Block Some Access";
			if(this.pol[i].weblog === "1")	Logged = "<?echo I18N("j","Yes");?>";	
			else	Logged ="<?echo I18N("j","No");?>";
			var p = PXML.FindModule("SCHEDULE");
			var schpath = PXML.doc.GetPathByTarget(p+"/schedule", "entry", "uid", this.pol[i].sch, false); 
			var Schedule = XG(schpath+"/description");	
			
			var data = [this.pol[i].enable, this.pol[i].desc, this.MachineString(i), Filtering, Logged, Schedule===""?"<?echo I18N("j","Always");?>":Schedule,
				'<a href="javascript:PAGE.OnPolicyEdit('+i+');"><img src="pic/img_edit.gif" alt="<?echo I18N("h", "Edit");?>" title="<?echo I18N("h", "Edit");?>"></a>',
				'<a href="javascript:PAGE.OnPolicyDelete('+i+');"><img src="pic/img_delete.gif" alt="<?echo I18N("h", "Delete");?>" title="<?echo I18N("h", "Delete");?>"></a>'
				];
			var type = ["checkbox","text","text","text","text","text","",""];
			BODY.InjectTable("policytable", "Ptable"+i, data, type);
			OBJ("Ptable"+i+"_check_0").disabled = false;
		}
	},	
	MachineString: function(i)
	{
		var note="", d="";
		for (var k=0; k<this.mac.length; k+=1)
		{
			if (this.pol[i].uid === this.mac[k].uid) 
			{
				note+=d+this.mac[k].val;
				d=",\n";
			}
		}	
		return note;
	},
	InsectMachineTable: function()
	{
		BODY.CleanTable("machinetable");
		var k=1;
		for(var i=0; i<this.mac.length; i+=1)
		{
			
			if(this.polcur[0].uid === this.mac[i].uid)
			{
				var data = [this.mac[i].val,
					'<a href="javascript:PAGE.OnMachineEdit('+k+');"><img src="pic/img_edit.gif" alt="<?echo I18N("h", "Edit");?>" title="<?echo I18N("h", "Edit");?>"></a>',
					'<a href="javascript:PAGE.OnMachineDelete('+k+');"><img src="pic/img_delete.gif" alt="<?echo I18N("h", "Delete");?>" title="<?echo I18N("h", "Delete");?>"></a>'
					];
				var type = ["text","",""];
				BODY.InjectTable("machinetable", "Mtable"+k, data, type);
				k++;
			}	
		}
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("ACCESSCTRL");
		p += "/acl/accessctrl";
		var cnt = S2I(XG(p+"/entry#"));
		while (cnt > 0) {XD(p+"/entry");cnt-=1;}		
				
			
		for (var i=0; i<this.pol.length; i++)
		{
			var ii = i+1;
			var m = p+"/entry:"+ii;
			var pol_enable = "1";
			if(this.pol[i].uid !== "" ) pol_enable = OBJ("Ptable"+i+"_check_0").checked?"1":"";
			XS(m+"/uid",				this.pol[i].uid);
			XS(m+"/enable",				pol_enable);
			XS(m+"/description",		this.pol[i].desc);
			XS(m+"/action",				this.pol[i].act);
			XS(m+"/portfilter/enable",	this.pol[i].portf);
			XS(m+"/webfilter/enable",	this.pol[i].webf);
			XS(m+"/webfilter/logging",	this.pol[i].weblog);
			XS(m+"/schedule",			this.pol[i].sch === "<?echo I18N("j","always");?>"?"":this.pol[i].sch);
			
			var j = 1;
			for (var k=0; k<this.mac.length; k++)
			{
				var s = m+"/machine/entry:"+j;				
				if(this.pol[i].uid === this.mac[k].uid)
				{	
					XS(s+"/type",	this.mac[k].typ);	
					XS(s+"/value",	this.mac[k].val);
					j++;
				}	
			}	
			
			j = 1;
			for (var k=0; k<this.fil.length; k++)
			{
				var s = m+"/portfilter/entry:"+j;				
				if(this.pol[i].uid === this.fil[k].uid)
				{
					XS(s+"/name",		this.fil[k].filname);	
					XS(s+"/enable",		this.fil[k].enable);
					XS(s+"/startip",	this.fil[k].startip);	
					XS(s+"/endip",		this.fil[k].endip);
					XS(s+"/protocol",	this.fil[k].prot);	
					XS(s+"/startport",	this.fil[k].startport);
					XS(s+"/endport",	this.fil[k].endport);					
					j++;
				}	
			}
		}	
		XS(p+"/enable", OBJ("en_access").checked?"1":"0");
		XS(p+"/count", this.pol.length);
		return PXML.doc;
	},
	IsDirty: function() 
	{
		return this.Dirty;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	rgmode: <?if (query("/runtime/device/layout")=="bridge") echo "false"; else echo "true";?>,
	pol: null,
	mac: null,
	fil: null,
	polcur: null,
	stages: new Array ("access_main", "access_descript", "access_name", "access_schedule", 
						"access_machine", "access_filter_meth", "access_port_filter", "access_web_logging"),
	currentStage: 0,
	edit: null,
	Medit: null,
	network: "<?$inf = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0); echo query($inf."/inet/ipv4/ipaddr");?>",
	lanmask: "<?echo query($inf."/inet/ipv4/mask");?>",
	Dirty: false,
	policy_num: null,
	max_policy_num: null,
	max_machine_num: null,
						
	OnClickPrev: function()
	{
		this.StageClose();
		var	stage = this.stages[this.currentStage];
		if(stage === "access_web_logging" && this.polcur[0].portf !== "1")	this.currentStage-=2;
		else	this.currentStage--;
		stage = this.stages[this.currentStage];	
		this.ShowCurrentStage(stage);
	},	
	OnClickNext: function()
	{
		if(this.StageCheck() === false) return;
		this.StageClose();
		var	stage = this.stages[this.currentStage];
		if((stage === "access_main" && this.polcur[0].uid !== "") || 
			(stage === "access_filter_meth" && this.polcur[0].portf !== "1" && this.polcur[0].webf === "1"))	this.currentStage+=2;
		else	this.currentStage++;
		stage = this.stages[this.currentStage];
		this.ShowCurrentStage(stage);
	},
	ShowCurrentStage: function(stage)
	{
		switch (stage)
		{
		case "access_descript":
			this.StageDescript();
			break;
		case "access_name":
			this.StageName();
			break;
		case "access_schedule":
			this.StageSchedule();
			break;
		case "access_machine":
			this.StageMachine();
			break;	
		case "access_filter_meth":
			this.StageFliterMeth();
			break;
		case "access_port_filter":
			this.StagePortFliter();
			break;	
		case "access_web_logging":
			this.StageWebLogging();
			break;		
		default:
		}		
	},	
	OnClickSave: function()
	{
		if(this.StageCheck() === false) return;
		if(this.polcur[0].uid === "")
		{
			var i = this.pol.length;
			this.pol[i] = 
			{
				uid:	"",
				enable:	"1",
				desc:	this.polcur[0].desc,
				act:	this.polcur[0].act,
				portf:	this.polcur[0].portf === ""?"0":this.polcur[0].portf,
				webf:	this.polcur[0].webf === ""?"0":this.polcur[0].webf,
				weblog:	this.polcur[0].weblog=== ""?"0":this.polcur[0].weblog,
				sch:	this.polcur[0].sch
			};
		}
		else
		{	
			for(var j=0; j<this.pol.length; j++)
			{
				if(this.polcur[0].uid === this.pol[j].uid)
				{
					this.pol[j].enable 	= this.polcur[0].enable;
					this.pol[j].desc 	= this.polcur[0].desc;
					this.pol[j].act 	= this.polcur[0].act;
					this.pol[j].portf 	= this.polcur[0].portf;
					this.pol[j].webf 	= this.polcur[0].webf;
					this.pol[j].weblog 	= this.polcur[0].weblog;
					this.pol[j].sch 	= this.polcur[0].sch;
				}	
			}
		}
		BODY.OnSubmit();
	},
	OnClickCancel: function()
	{
		BODY.GetCFG();
	},
	StageCheck: function()
	{
		var	stage = this.stages[this.currentStage];
		if(stage === "access_main")
		{
		}	
		else if(stage === "access_descript")
		{
			
		}
		else if(stage === "access_name")
		{
			if(OBJ("policyname").value === "")
			{			
				alert("<?echo I18N("j", "The policy name can not be blank.");?>");
				return false;
			}
			for(var i=0; i<this.pol.length; i++)
			{
				if((OBJ("policyname").value === this.pol[i].desc) && (OBJ("policyname").value !== this.polcur[0].desc))
				{			
					alert("<?echo I18N("j", "The policy name can not be the same as others.");?>");
					return false;
				}	
			}	
			this.polcur[0].desc = OBJ("policyname").value
		}
		else if(stage === "access_schedule")
		{
			this.polcur[0].sch = OBJ("sch_select").value;
		}
		else if(stage === "access_machine")
		{
			
			for(var i=0; i<this.mac.length; i++)
			{
				if(this.polcur[0].uid === this.mac[i].uid)	break;
				if(i===(this.mac.length-1))
				{			
					alert("<?echo I18N("j", "Please enter one machine.");?>");
					return false;
				}
			}	
		}	
		else if(stage === "access_filter_meth")
		{
			if(OBJ("LOGWEBONLY").checked)	
			{
				this.polcur[0].act 		= "LOGWEBONLY";
				this.polcur[0].webf 	= "0";
				this.polcur[0].weblog 	= "1";				
				this.polcur[0].portf 	= "0";
			}	
			else if(OBJ("BLOCKALL").checked)
			{	
				this.polcur[0].act 		= "BLOCKALL";
				this.polcur[0].webf 	= "0";
				this.polcur[0].weblog 	= "0";				
				this.polcur[0].portf 	= "0";
			}	
			else
			{		
				this.polcur[0].act = "BLOCKSOME";
				if(OBJ("WebFilterCheck").checked && OBJ("PortFilterCheck").checked)			
				{
					this.polcur[0].webf 	= "1";
					this.polcur[0].portf 	= "1";
				}
				else if(OBJ("WebFilterCheck").checked && !OBJ("PortFilterCheck").checked)	
				{
					this.polcur[0].webf 	= "1";
					this.polcur[0].portf 	= "0";
				}
				else if(!OBJ("WebFilterCheck").checked && OBJ("PortFilterCheck").checked)	
				{
					this.polcur[0].webf 	= "0";
					this.polcur[0].weblog 	= "0";
					this.polcur[0].portf 	= "1";
				}	
				else if(!OBJ("WebFilterCheck").checked && !OBJ("PortFilterCheck").checked)	
				{
					alert("<?echo I18N("j", "Please select at least one filter.");?>");
					return false;
				}	
			}	
		}
		else if(stage === "access_port_filter")
		{
			for(var i=1; i<=8; i++)
			{
				if(OBJ("filter_name"+i).value !== "")
				{
					for(var j=1; j<=8; j++)
					{
						if(OBJ("filter_name"+i).value === OBJ("filter_name"+j).value && i!==j) 
						{	
							alert("<?echo I18N("j", "The name could not be the same.");?>");
							return false;
						}							
					}	
				
					var FSIP = OBJ("filter_startip"+i).value;
					var FEIP = OBJ("filter_endip"+i).value;
					var FSIP_val = FSIP.split(".");
					var FEIP_val = FEIP.split(".");
					if (FSIP_val.length!=4)
					{
						alert("<?echo I18N("j", "Incorrect Dest IP address. The start IP address is invalid.");?>");
						OBJ("filter_startip"+i).focus();
						return false;
					}
					for (var j=0; j<4; j++)
					{
						if (!TEMP_IsDigit(FSIP_val[j]) || FSIP_val[j]>255)
						{
							alert("<?echo I18N("j", "Incorrect Dest IP address. The start IP address is invalid.");?>");
							OBJ("filter_startip"+i).focus();
							return false;
						}
					}	
					if (FEIP_val.length!=4)
					{
						alert("<?echo I18N("j", "Incorrect Dest IP address. The end IP address is invalid.");?>");
						OBJ("filter_endip"+i).focus();
						return false;
					}
					for (var j=0; j<4; j++)
					{
						if (!TEMP_IsDigit(FEIP_val[j]) || FEIP_val[j]>255)
						{
							alert("<?echo I18N("j", "Incorrect Dest IP address. The end IP address is invalid.");?>");
							OBJ("filter_endip"+i).focus();
							return false;
						}
					}			
					if(COMM_IPv4ADDR2INT(FSIP) > COMM_IPv4ADDR2INT(FEIP))
					{
						alert("<?echo I18N("j", "The end IP address should be greater than the start address.");?>");
						OBJ("filter_startip"+i).focus();
						return false;
					}	
						
					var FSPort = OBJ("filter_startport"+i).value;
					var FEPort = OBJ("filter_endport"+i).value;	
					if(OBJ("filter_protocol"+i).value === "TCP" || OBJ("filter_protocol"+i).value === "UDP")
					{ 
						if(!TEMP_IsDigit(FSPort) || FSPort < 1 || FSPort > 65535)
						{
							alert("<?echo I18N("j", "Invalid start port value.");?>");
							OBJ("filter_startport"+i).focus();
							return false;
						}
						if(!TEMP_IsDigit(FEPort) || FEPort < 1 || FEPort > 65535)
						{
							alert("<?echo I18N("j", "Invalid end port value.");?>");
							OBJ("filter_endport"+i).focus();
							return false;
						}	
						if(parseInt(FSPort, 10) > parseInt(FEPort, 10))
						{
							alert("<?echo I18N("j", "The end port should be greater than the start port.");?>");
							OBJ("filter_startport"+i).focus();
							return false;
						}			
					}
				}
			}
			
			if(this.fil.length > 0)
			{
				var m=0;	
				do
				{
					if(this.polcur[0].uid === this.fil[m].uid)	this.fil.splice(m,1);
					else m++;
				}while(m<this.fil.length)
			}
			
			for(var i=1; i<=8; i++)
			{
				if(OBJ("filter_name"+i).value !== "")
				{
					var len = this.fil.length;
					this.fil[len] = 
					{
						uid:		this.polcur[0].uid,
						filname:	OBJ("filter_name"+i).value,	
						enable:		OBJ("filter_enable"+i).checked===true?"1":"0",
						startip:	OBJ("filter_startip"+i).value,	
						endip:		OBJ("filter_endip"+i).value,
						prot:		OBJ("filter_protocol"+i).value,	
						startport:	OBJ("filter_startport"+i).value,					
						endport:	OBJ("filter_endport"+i).value
					};
				}
			}	
		}
		else if(stage === "access_web_logging")
		{
			if(OBJ("WebLogEnabled").checked)	this.polcur[0].weblog = "1";
			else	this.polcur[0].weblog = "0";
		}
		return true;		
	},
	StageClose: function()
	{
		OBJ("access_main").style.display = "none";
		OBJ("help_hint").style.display = "none";
		OBJ("mainbody").className = "menubody";
		OBJ("access_descript").style.display = "none";
		OBJ("access_name").style.display = "none";
		OBJ("access_schedule").style.display = "none";
		OBJ("access_machine").style.display = "none";
		OBJ("access_filter_meth").style.display = "none";
		OBJ("access_port_filter").style.display = "none";
		OBJ("access_web_logging").style.display = "none";	
	},
	StageMain: function()
	{
		this.currentStage = 0;
		OBJ("access_main").style.display = "block";
		OBJ("help_hint").style.display = "block";
		OBJ("mainbody").className = "mainbody";		
	},
	StageDescript: function()
	{
		OBJ("access_descript").style.display = "block";
		Set4ButtonDisabled(true, false, true, false);
	},
	StageName: function()
	{
		OBJ("access_name").style.display = "block";
		if(this.polcur[0].uid !== "")	Set4ButtonDisabled(true, false, false, false);
		else	Set4ButtonDisabled(false, false, true, false);		
		if(this.polcur[0].desc !== "")	OBJ("policyname").value = this.polcur[0].desc;
		else	OBJ("policyname").value = "";
	},
	StageSchedule: function()
	{
		OBJ("access_schedule").style.display = "block";
		if(this.polcur[0].uid !== "")	Set4ButtonDisabled(false, false, false, false);
		else	Set4ButtonDisabled(false, false, true, false);		
		if(this.polcur[0].sch !== "")
		{
			 OBJ("sch_select").value = this.polcur[0].sch;
			 this.OnClickSchSelect(this.polcur[0].sch);
		}
		else	OBJ("sch_detail").value = "<?echo I18N("j","Always");?>";
	},
	StageMachine: function()
	{
		OBJ("access_machine").style.display = "block";
		if(this.polcur[0].uid !== "")	Set4ButtonDisabled(false, false, false, false);
		else	Set4ButtonDisabled(false, false, true, false);
		this.OnClickMachineType("IP");
		this.InsectMachineTable();
		this.Medit = null;
		OBJ("machine_submit").value = "<?echo I18N("h", "Add");?>";
	},
	StageFliterMeth: function()
	{
		OBJ("access_filter_meth").style.display = "block";
		if(this.polcur[0].act === "")	this.OnClickFilterMethod("LOGWEBONLY");
		else	this.OnClickFilterMethod(this.polcur[0].act);
	},
	StagePortFliter: function()
	{
					
		OBJ("access_port_filter").style.display = "block";
		if(this.polcur[0].portf === "1" && this.polcur[0].webf === "1")
		{
			if(this.polcur[0].uid !== "")	Set4ButtonDisabled(false, false, false, false);
			else	Set4ButtonDisabled(false, false, true, false);
		}
		else if(this.polcur[0].portf === "1" && this.polcur[0].webf !== "1")	Set4ButtonDisabled(false, true, false, false);
				
		for(var i=1; i<=8; i++)
		{
			OBJ("filter_enable"+i).checked 	= false;
			OBJ("filter_name"+i).value 		= "";
			OBJ("filter_startip"+i).value 	= "0.0.0.0";
			OBJ("filter_endip"+i).value 	= "255.255.255.255";
			OBJ("filter_protocol"+i).value 	= "ALL";
			OBJ("filter_startport"+i).value = 1;
			OBJ("filter_endport"+i).value 	= 65535;	
			this.OnClickProtocol(i);	
		}

		var k=1;
		for(var j=0; j<this.fil.length; j++)
		{
			if(this.polcur[0].uid === this.fil[j].uid)
			{
				OBJ("filter_enable"+k).checked 	= this.fil[j].enable==="1"?true:false;
				OBJ("filter_name"+k).value 		= this.fil[j].filname;
				OBJ("filter_startip"+k).value 	= this.fil[j].startip;
				OBJ("filter_endip"+k).value 	= this.fil[j].endip;
				OBJ("filter_protocol"+k).value 	= this.fil[j].prot;
				OBJ("filter_startport"+k).value = this.fil[j].startport;
				OBJ("filter_endport"+k).value 	= this.fil[j].endport;
				this.OnClickProtocol(k);
				k++;
			}	
		}	
	},
	StageWebLogging: function()
	{
		OBJ("access_web_logging").style.display = "block";
		Set4ButtonDisabled(false, true, false, false);
		if(this.polcur[0].weblog === "1")	this.OnClickWebLogging("enable");
		else	this.OnClickWebLogging("disable");
	},
	OnClickEnACCESS: function()
	{
		if(OBJ("en_access").checked)
		{	
			OBJ("add_policy").style.display = "block";
			OBJ("policytableframe").style.display = "block";
			this.InsectPolicyTable();
		}
		else
		{		
			OBJ("policytableframe").style.display = "none";
			OBJ("add_policy").style.display = "none";
		}
		if(parseInt(this.policy_num, 10) < parseInt(this.max_policy_num, 10)) OBJ("add_policy").disabled = false;
		else OBJ("add_policy").disabled = true;
	},	
	OnClickAddPolicy: function()
	{	
		this.polcur[0] = 
		{
			uid:	"",
			enable:	"",
			desc:	"",
			act:	"",
			portf:	"",
			webf:	"",
			weblog:	"",
			sch:	""
		};
		this.OnClickNext();
	},	
	OnClickSchSelect: function(sch_uid)
	{
		var p = PXML.FindModule("SCHEDULE");
		var s = PXML.doc.GetPathByTarget(p+"/schedule", "entry", "uid", sch_uid, false);
		var str = "";

		if(sch_uid === "<?echo I18N("j","always");?>")	OBJ("sch_detail").value = "<?echo I18N("j","always");?>";
		else if(s===null || s==="")	OBJ("sch_detail").value = OBJ("sch_select").value =  "<?echo I18N("j","always");?>"; //The schedule already had been deleted. 
		else if(XG(s+"/sun")==="1" && XG(s+"/mon")==="1" && XG(s+"/tue")==="1" && XG(s+"/wed")==="1" && XG(s+"/thu")==="1" && XG(s+"/fri")==="1" && XG(s+"/sat")==="1") 		
		{	
			str = "All week " + XG(s+"/start") + "~" + XG(s+"/end");
			OBJ("sch_detail").value = str;
		}
		else
		{
			if(XG(s+"/sun")==="1")	str+="SUN ";
			if(XG(s+"/mon")==="1")	str+="MON ";
			if(XG(s+"/tue")==="1")	str+="TUE ";
			if(XG(s+"/wed")==="1")	str+="WED ";
			if(XG(s+"/thu")==="1")	str+="THU ";
			if(XG(s+"/fri")==="1")	str+="FRI ";
			if(XG(s+"/sat")==="1")	str+="SAT ";
			str += XG(s+"/start") + "~" + XG(s+"/end");
			OBJ("sch_detail").value = str;
		}		
	},	
	OnClickMachineType: function(Mtype)
	{
		if (Mtype === "IP")
		{
			OBJ("MIP").checked = true;
			OBJ("MMAC").checked = false;
			OBJ("MOthers").checked = false;
			OBJ("MachineIP").disabled = false;
			OBJ("MachineIPSelect").disabled = false;
			OBJ("MachineMAC").disabled = true;
			OBJ("MachineMACSelect").disabled = true;
			OBJ("ipv4_mac_button").disabled = true;
		}
		else if (Mtype === "MAC")
		{
			OBJ("MIP").checked = false;
			OBJ("MMAC").checked = true;
			OBJ("MOthers").checked = false;
			OBJ("MachineIP").disabled = true;
			OBJ("MachineIPSelect").disabled = true;
			OBJ("MachineMAC").disabled = false;
			OBJ("MachineMACSelect").disabled = false;
			OBJ("ipv4_mac_button").disabled = false;
		}		
		else
		{	
			OBJ("MIP").checked = false;
			OBJ("MMAC").checked = false;
			OBJ("MOthers").checked = true;
			OBJ("MachineIP").disabled = true;
			OBJ("MachineIPSelect").disabled = true;
			OBJ("MachineMAC").disabled = true;
			OBJ("MachineMACSelect").disabled = true;
			OBJ("ipv4_mac_button").disabled = true;
		}			
	},	
	OnClickMachineIPSelect: function(IP)
	{
		if(IP !== "")	OBJ("MachineIP").value = IP;
		else	OBJ("MachineIP").value = "";
	},	
	OnClickMachineMACSelect: function(MAC)
	{
		if(MAC !== "")	OBJ("MachineMAC").value = MAC;
		else	OBJ("MachineMAC").value = "";
	},	
	OnClickMACButton: function()
	{
		OBJ("MachineMAC").value="<?echo INET_ARP($_SERVER["REMOTE_ADDR"]);?>";
	},
	OnClickMachineSubmit: function()
	{	
		var Mtype = "";
		var Mvalue = "";
		var machine_num=1;
		for(var i=0; i<this.mac.length; i++)
		{
			if(this.polcur[0].uid === this.mac[i].uid)	machine_num++;
			if(machine_num > parseInt(this.max_machine_num, 10))
			{
				alert("<?echo I18N("j", "The maximum number of permitted machines has been exceeded.");?>");
				return false;			
			}
		}
		
		if(OBJ("MIP").checked)
		{
			var lan	= PXML.FindModule("INET.LAN-1");
			var inetp = GPBT(lan+"/inet", "entry", "uid", XG(lan+"/inf/inet"), false);
			var lanip = XG(inetp+"/ipv4/ipaddr");
			var hostid = COMM_IPv4HOST(OBJ("MachineIP").value, this.lanmask);
			if(lanip === OBJ("MachineIP").value)
			{
				alert("<?echo I18N("j", "The IP Address could not be the same as LAN IP Address.");?>");
				return false;
			}	 
			
			if(!IsIPv4(OBJ("MachineIP").value))
			{
				alert("<?echo I18N("j", "Invalid IP address.");?>");
				return false;
			}
			
			if(!TEMP_CheckNetworkAddr(OBJ("MachineIP").value))
			{	
				alert("<?echo I18N("j", "IP address should be in LAN subnet.");?>");
				return false;
			}	
			
			if(hostid < 1 || hostid >= COMM_IPv4MAXHOST(this.lanmask))
			{
				alert("<?echo I18N("j", "The input hostid is out of the boundary.");?>");
				return false;				
			}
			
			for(var i=0; i<this.mac.length; i++)
			{
				if(this.polcur[0].uid===this.mac[i].uid && OBJ("MachineIP").value===this.mac[i].val)
				{
					alert("<?echo I18N("j", "The machine is already existed!");?>");
					return false;			
				}
			}
			
			Mtype 	= "IP";
			Mvalue 	= OBJ("MachineIP").value;
		}	
		else if(OBJ("MMAC").checked)
		{
			var mac = CheckMAC(OBJ("MachineMAC").value);

			if (mac==="")
			{
				alert("<?echo I18N("j", "Invalid MAC address value.");?>");
				return false;
			}
			for(var i=0; i<this.mac.length; i++)
			{
				if(this.polcur[0].uid===this.mac[i].uid && mac===this.mac[i].val)
				{
					alert("<?echo I18N("j", "The machine is already existed!");?>");
					return false;			
				}
			}
						
			Mtype 	= "MAC";
			Mvalue 	= mac;
		}
		else
		{		
			Mtype 	= "OTHERMACHINES";
			Mvalue 	= "Other Machines";		
		}
		
		if(this.Medit === null)
		{
			var i = this.mac.length;
			this.mac[i] = 
				{
					uid:	this.polcur[0].uid,
					typ:	Mtype,	
					val:	Mvalue
				};
		}
		else
		{
			var k =0;
			for(var i=0; i<this.mac.length; i++)
			{
				if(this.polcur[0].uid === this.mac[i].uid)	k++;
				if(this.Medit === k)
				{
					this.mac[i].typ = Mtype;
					this.mac[i].val = Mvalue;
					break;
				}
			}	
		}			
		this.OnClickMachineCancel();
		this.StageMachine();
		this.Dirty = true;
	},	
	OnClickMachineCancel: function()
	{
		this.OnClickMachineType("IP");
		OBJ("MachineIP").value 	= "";
		OBJ("MachineIPSelect").value 	= "";
		OBJ("MachineMAC").value = "";
		OBJ("MachineMACSelect").value = "";
	},
	OnClickFilterMethod: function(method)
	{
		if(method === "LOGWEBONLY")
		{
			OBJ("LOGWEBONLY").checked = true;
			OBJ("BLOCKALL").checked = false;
			OBJ("BLOCKSOME").checked = false;
			OBJ("WebFilter").style.display = "none";
			OBJ("PortFilter").style.display = "none";
			Set4ButtonDisabled(false, true, false, false);
		}	
		else if(method === "BLOCKALL")
		{
			OBJ("LOGWEBONLY").checked = false;
			OBJ("BLOCKALL").checked = true;
			OBJ("BLOCKSOME").checked = false;
			OBJ("WebFilter").style.display = "none";
			OBJ("PortFilter").style.display = "none";
			Set4ButtonDisabled(false, true, false, false);			
		}		
		else
		{
			OBJ("LOGWEBONLY").checked = false;
			OBJ("BLOCKALL").checked = false;
			OBJ("BLOCKSOME").checked = true;
			OBJ("WebFilter").style.display = "block";
			OBJ("PortFilter").style.display = "block";
			Set4ButtonDisabled(false, false, true, false);
			if(this.polcur[0].webf === "1")		OBJ("WebFilterCheck").checked = true;	
			if(this.polcur[0].portf === "1")	OBJ("PortFilterCheck").checked = true;							
		}			
	},	
	OnClickProtocol: function(i)
	{
		if(OBJ("filter_protocol"+i).value === "ALL" || OBJ("filter_protocol"+i).value === "ICMP")
		{
			OBJ("filter_startport"+i).disabled = true;
			OBJ("filter_endport"+i).disabled = true;
		}
		else if(OBJ("filter_protocol"+i).value === "TCP" || OBJ("filter_protocol"+i).value === "UDP")
		{
			OBJ("filter_startport"+i).disabled = false;
			OBJ("filter_endport"+i).disabled = false;
		}			
	},	
	OnClickWebLogging: function(enable)
	{
		if(enable === "disable")	
		{
			OBJ("WebLogDisabled").checked = true;
			OBJ("WebLogEnabled").checked = false;
		}	
		else
		{
			OBJ("WebLogDisabled").checked = false;
			OBJ("WebLogEnabled").checked = true;			
		}		
	},	
	OnPolicyEdit: function(i)
	{
		this.edit = i;
		this.polcur[0] = 
		{
			uid:	this.pol[i].uid,
			enable:	this.pol[i].enable,
			desc:	this.pol[i].desc,
			act:	this.pol[i].act,
			portf:	this.pol[i].portf,			
			webf:	this.pol[i].webf,
			weblog:	this.pol[i].weblog,
			sch:	this.pol[i].sch
		};
		this.OnClickNext();
	},
	OnPolicyDelete: function(i)
	{
		for(var j=i; j < this.pol.length-1; j++) 
		{
			var	k=j+1;
			OBJ("Ptable"+j+"_check_0").checked = OBJ("Ptable"+k+"_check_0").checked;
		}		
		this.pol.splice(i,1);
		this.Dirty = true;
		BODY.OnSubmit();
	},
	OnMachineEdit: function(i)
	{
		this.Medit = i;
		var k=0;
		for(var j=0; j<this.mac.length; j++)
		{
			if(this.polcur[0].uid === this.mac[j].uid)	k++;	
			if(i === k)
			{
				this.OnClickMachineType(this.mac[j].typ);
				if(this.mac[j].typ === "IP" ) OBJ("MachineIP").value = this.mac[j].val;
				else if(this.mac[j].typ === "MAC" ) OBJ("MachineMAC").value = this.mac[j].val;
				break;	
			}		
		}			
		OBJ("machine_submit").value = "<?echo I18N("h", "Update");?>";
	},
	OnMachineDelete: function(i)
	{
		var k=0;
		for(var j=0; j<this.mac.length; j++)
		{
			if(this.polcur[0].uid === this.mac[j].uid)	k++;
			if(i === k)
			{	
				this.mac.splice(j,1);
				break;
			}
		}
		this.StageMachine();
		this.Dirty = true;
	}
}
function SetButtonDisabled(name, disable)
{
	var button = document.getElementsByName(name);
	for (var i=0; i<button.length; i++)
	{
		button[i].disabled = disable;
	}
}
function Set4ButtonDisabled(prev, next, save, cancel)
{
	SetButtonDisabled("b_prev", prev);
	SetButtonDisabled("b_next", next);
	SetButtonDisabled("b_save", save);
	SetButtonDisabled("b_cancel", cancel);
}
function CheckMAC(m)
{
	var myMAC="";
	if (m.search(":") != -1)	var tmp=m.split(":");
	else				var tmp=m.split("-");
	if (m == "" || tmp.length != 6)	return "";

	for (var i=0; i<tmp.length; i++)
	{
		if (tmp[i].length==1)	tmp[i]="0"+tmp[i];
		else if (tmp[i].length==0||tmp[i].length>2)	return "";
	
		tmp[i]=tmp[i].toLowerCase();
      
		for(var j=0; j<tmp[i].length; j++)
		{
			var c = "0123456789abcdef";
			var str_hex=0;		
			for(var k=0; k<c.length; k++)	if(tmp[i].substr(j,1)===c.substr(k,1))	{str_hex=1;break;}
			if(str_hex===0) return "";	
			
		}		
	}
	
	myMAC = tmp[0];
    
    if (tmp[0]%2==1)
    {
    	//marco, check for multicast address
       return "";
    }

	for (var i=1; i<tmp.length; i++)
	{
		myMAC = myMAC + ':' + tmp[i];
	}
	if(myMAC==="ff:ff:ff:ff:ff:ff" || myMAC==="01:11:11:11:11:11"  || myMAC=="00:00:00:00:00:00")	return "";	
	
	return myMAC;
}
function IsIPv4(ipv4)
{
	var vals = ipv4.split(".");
	if (vals.length!==4)	return false;
	for (var i=0; i<4; i++)	if (!TEMP_IsDigit(vals[i]) || vals[i]>255)	return false;
	return true;
}	
</script>
