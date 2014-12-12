<script type="text/javascript">
function Time(str) { this.Set(str); }
Time.prototype =
{
	hour: 0,
	min: 0,
	apm: "",
	Set: function(str)
	{
		if (str && str!=="")
		{
			var time = str.split(':');
			this.hour = S2I(time[0]);
			this.min = S2I(time[1]);
			this.apm = "";
		}
	},
	To12Hrs: function()
	{
		if (this.apm==="AM" || this.apm==="PM") return;
		
		this.apm = "AM";
		
		if (this.hour==0)
		{
			this.hour=12;
		}
		else if (this.hour>11)
		{
			this.apm="PM";
			if (this.hour>12) this.hour-=12;
		}
	},
	To24Hrs: function()
	{
		if (this.apm==="") return;
		if (this.hour>12) this.hour=0;
		
		if (this.apm==="AM" && this.hour==12) { this.hour=0; }		
		if (this.apm==="PM" && this.hour!=12) { this.hour+=12; }

		this.apm = "";
	},
	TS: function()
	{
		var str = "";
		//if (this.hour<10) str+="0";
		str+=this.hour+":";
		if (this.min<10) str+="0";
		str+=this.min+this.apm;
		return str;
	}
};

function Page() {}
Page.prototype =
{
	services: "SCHEDULE",
	OnLoad: function()
	{
		BODY.CleanTable("schtable");
	},
	OnUnload: function() {},
	OnSubmitCallback: function(code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			BODY.OnReload();
			return true;
			break;
		default : 	//if fatlady return error
			if(this.org && this.cfg)
			{	/* we should load the original configs. Can't count on PXML object, since its already modified. 
				We can count on our original table */
				delete this.cfg;
				this.cfg = new Array();
				var cnt = this.org.length;
				for (var i=0; i<cnt; i+=1)
				{
					this.cfg[i] = {
						uid:	this.org[i].uid,
						desc:	this.org[i].desc,
						exc:	this.org[i].exc,
						sun:	this.org[i].sun,
						mon:	this.org[i].mon,
						tue:	this.org[i].tue,
						wed:	this.org[i].wed,
						thu:	this.org[i].thu,
						fri:	this.org[i].fri,
						sat:	this.org[i].sat,
						start:	this.org[i].start,
						end:	this.org[i].end
					};
				}
			}
			return false;
			break;	
		}
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;

		if (this.org) delete this.org;
		if (this.cfg) delete this.cfg;
		this.org = new Array();
		this.cfg = new Array();

		var p = PXML.FindModule("SCHEDULE");
		
		var cnt = S2I(XG(p+"/schedule/count"));
		for (var i=0; i<cnt; i+=1)
		{
			var index = i+1;
			var s = p+"/schedule/entry:"+index+"/";
			this.org[i] = {
				uid:	XG(s+"uid"),
				desc:	XG(s+"description"),
				exc:	XG(s+"exclude"),
				sun:	XG(s+"sun"),
				mon:	XG(s+"mon"),
				tue:	XG(s+"tue"),
				wed:	XG(s+"wed"),
				thu:	XG(s+"thu"),
				fri:	XG(s+"fri"),
				sat:	XG(s+"sat"),
				start:	XG(s+"start"),
				end:	XG(s+"end")
				};
			this.cfg[i] = {
				uid:	XG(s+"uid"),
				desc:	XG(s+"description"),
				exc:	XG(s+"exclude"),
				sun:	XG(s+"sun"),
				mon:	XG(s+"mon"),
				tue:	XG(s+"tue"),
				wed:	XG(s+"wed"),
				thu:	XG(s+"thu"),
				fri:	XG(s+"fri"),
				sat:	XG(s+"sat"),
				start:	XG(s+"start"),
				end:	XG(s+"end")
				};
		}
		this.InitEntryList();
		this.OnClickSchCancel();
		return true;
	},
	PreSubmit: function()
	{
		var p = PXML.FindModule("SCHEDULE");
		var cnt = S2I(XG(p+"/schedule/entry#"));
		while (cnt > 0) {XD(p+"/schedule/entry");cnt-=1;}

		for (var i=0; i<this.cfg.length; i+=1)
		{
			var ii = i+1;
			var n = p+"/schedule/entry:"+ii;
			XS(n+"/uid",	this.cfg[i].uid);
			XS(n+"/description",this.cfg[i].desc);
			XS(n+"/exclude",this.cfg[i].exc);
			XS(n+"/sun",	this.cfg[i].sun);
			XS(n+"/mon",	this.cfg[i].mon);
			XS(n+"/tue",	this.cfg[i].tue);
			XS(n+"/wed",	this.cfg[i].wed);
			XS(n+"/thu",	this.cfg[i].thu);
			XS(n+"/fri",	this.cfg[i].fri);
			XS(n+"/sat",	this.cfg[i].sat);
			XS(n+"/start",	this.cfg[i].start);
			XS(n+"/end",	this.cfg[i].end);
			
		}
		XS(p+"/schedule/count", this.cfg.length);
		//PXML.doc.dbgdump();
		return PXML.doc;
	},
	IsDirty: function()
	{
		if (this.org.length !== this.cfg.length) return true;
		for (var i=0; i<this.cfg.length; i+=1)
		{
			if (this.org[i].uid !== this.cfg[i].uid ||
				this.org[i].desc!== this.cfg[i].desc||
				this.org[i].exc !== this.cfg[i].exc ||
				this.org[i].sun !== this.cfg[i].sun ||
				this.org[i].mon !== this.cfg[i].mon ||
				this.org[i].tue !== this.cfg[i].tue ||
				this.org[i].wed !== this.cfg[i].wed ||
				this.org[i].thu !== this.cfg[i].thu ||
				this.org[i].fri !== this.cfg[i].fri ||
				this.org[i].sat !== this.cfg[i].sat ||
				this.org[i].start !== this.cfg[i].start ||
				this.org[i].end !== this.cfg[i].end) return true; 
		}
		return false;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
	org: null,
	cfg: null,
	edit: null,
	///////////////////////////////////////////////////////////////////
	DayString: function(i)
	{
		var note="", d="";
		if (this.cfg[i].sun=="1") {note+=d+"SUN";d=",";}
		if (this.cfg[i].mon=="1") {note+=d+"MON";d=",";}
		if (this.cfg[i].tue=="1") {note+=d+"TUE";d=",";}
		if (this.cfg[i].wed=="1") {note+=d+"WED";d=",";}
		if (this.cfg[i].thu=="1") {note+=d+"THU";d=",";}
		if (this.cfg[i].fri=="1") {note+=d+"FRI";d=",";}
		if (this.cfg[i].sat=="1") {note+=d+"SAT";d=",";}
		return note;
	},
	TimeString: function(i)
	{
		var Start = new Time(this.cfg[i].start);
		var End = new Time(this.cfg[i].end);
		Start.To24Hrs();  
		End.To24Hrs();  
		return Start.TS() + " ~ " + End.TS();
	},
	InitEntryList: function()
	{
		BODY.CleanTable("schtable");
		for (var i=0; i<this.cfg.length; i+=1)
		{
			var data = [this.cfg[i].desc, this.DayString(i), this.TimeString(i),
				'<a href="javascript:PAGE.OnEdit('+i+');"><img src="pic/img_edit.gif" title="<?echo I18N("h", "Edit");?>"></a>',
				'<a href="javascript:PAGE.OnDelete('+i+');"><img src="pic/img_delete.gif" title="<?echo I18N("h", "Delete");?>"></a>'
				];
			var type = ["text","","","",""];
			BODY.InjectTable("schtable", this.cfg[i].uid, data, type);
		}
	},
	///////////////////////////////////////////////////////////////////
	OnClickSelectDays: function()
	{
		var week = OBJ("schallweek").checked;

		OBJ("schsun").checked = OBJ("schmon").checked = OBJ("schtue").checked = OBJ("schwed").checked =
		OBJ("schthu").checked = OBJ("schfri").checked = OBJ("schsat").checked = week ? true:false;
		OBJ("schsun").disabled= OBJ("schmon").disabled= OBJ("schtue").disabled= OBJ("schwed").disabled=
		OBJ("schthu").disabled= OBJ("schfri").disabled= OBJ("schsat").disabled= week ? true:false;
	},
	OnClick24Hours: function()
	{
		var checked = OBJ("sch24hrs").checked;
		OBJ("timeformat").disabled = OBJ("schstarthrs").disabled = OBJ("schstartmin").disabled = OBJ("schendhrs").disabled =
		OBJ("schendmin").disabled = OBJ("schstartapm").disabled = OBJ("schendapm").disabled = checked ? true:false;
		if (checked)
		{
			OBJ("schstarthrs").value = "12";
			OBJ("schstartmin").value = "0";
			COMM_SetSelectValue(OBJ("schstartapm"), "AM");
			OBJ("schendhrs").value = "11";
			OBJ("schendmin").value = "59";
			COMM_SetSelectValue(OBJ("schendapm"), "PM");
			OBJ("timeformat").value = "12-hour";
		}
		else
			this.OnChangeTimeFormat(OBJ("timeformat").value);
	},
	OnChangeTimeFormat: function(Tformat)
	{
		var StartT = new Time(OBJ("schstarthrs").value);
		var EndT = new Time(OBJ("schendhrs").value);
		if (Tformat==="12-hour")
		{
			OBJ("schstartapm").disabled = OBJ("schendapm").disabled = false;
			StartT.To12Hrs();
			EndT.To12Hrs();	
		}
		else
		{	
			OBJ("schstartapm").disabled = OBJ("schendapm").disabled = true;
			StartT.apm = OBJ("schstartapm").value;
			EndT.apm = OBJ("schendapm").value;			
			StartT.To24Hrs();
			EndT.To24Hrs();			
		}
		OBJ("schstarthrs").value = StartT.hour.toString();
		COMM_SetSelectValue(OBJ("schstartapm"), StartT.apm);
		OBJ("schendhrs").value = EndT.hour.toString();
		COMM_SetSelectValue(OBJ("schendapm"), EndT.apm);
	},	
	OnClickSchSubmit: function()
	{
		var e = {
			desc:	OBJ("schdesc").value,
			sun:	OBJ("schsun").checked,
			mon:	OBJ("schmon").checked,
			tue:	OBJ("schtue").checked,
			wed:	OBJ("schwed").checked,
			thu:	OBJ("schthu").checked,
			fri:	OBJ("schfri").checked,
			sat:	OBJ("schsat").checked,
			start:	{
				hour:parseInt(OBJ("schstarthrs").value,10),
				min: parseInt(OBJ("schstartmin").value,10)
				},
			end:	{
				hour:parseInt(OBJ("schendhrs").value,10),
				min: parseInt(OBJ("schendmin").value,10)
				}
			};

		if (e.desc === "")
		{
			alert("<?echo I18N("j","The 'Name' field can not be blank.");?>");
			return;
		}
		
		if (e.desc.charAt(0)===" " || e.desc.charAt(e.desc.length-1)===" ")
		{
			alert("<?echo i18n("The prefix or postfix of the name could not be blank.");?>");
			return;
		}
		
		/* max rules check */
		if(this.cfg.length >= <?=$SCH_MAX_COUNT?> && this.edit===null)
		{
			BODY.ShowAlert("<?echo i18n("Invalid Schedule. The maximum number of permitted Schedule rules has been exceeded.");?>");
			return null;
		}		
	
		for(var i=0; i < this.cfg.length; i+=1)
		{
			if (e.desc === this.cfg[i].desc)
			{
				if(this.edit !== this.cfg[i].uid)
				{
					alert("<?echo i18n("The schedule name could not be the same.");?>");
					return;
				}
			}
		}

		
		if (!e.sun && !e.mon && !e.tue && !e.wed && !e.thu && !e.fri && !e.sat)
		{
			alert("<?echo i18n("No day is selected.");?>");
			return;
		}

		if (isNaN(e.start.hour) || isNaN(e.start.min) || isNaN(e.end.hour) || isNaN(e.end.min) ||
			e.start.min < 0 || e.start.min > 59 || e.end.min < 0 || e.end.min > 59 ||
			(OBJ("timeformat").value==="12-hour"&&(e.start.hour < 1 || e.start.hour > 12 || e.end.hour < 1 || e.end.hour > 12)) || 
			(OBJ("timeformat").value==="24-hour"&&(e.start.hour < 0 || e.start.hour > 23 || e.end.hour < 0 || e.end.hour > 23)))	
		{
			alert("<?echo i18n("The schedule time is not valid.");?>");
			return;
		}

		var Start = new Time();
		var End = new Time();
		Start.hour = e.start.hour;
		Start.min = e.start.min;
		Start.apm = OBJ("timeformat").value==="12-hour"?OBJ("schstartapm").value:"";
		End.hour = e.end.hour;
		End.min = e.end.min;
		End.apm = OBJ("timeformat").value==="12-hour"?OBJ("schendapm").value:"";
		Start.To24Hrs();
		End.To24Hrs();

		var i = 0;
		var j = 0;
		var update = -1;
		if (!this.edit || this.edit === "") i = this.cfg.length;
		else
			for (i=0; i<this.cfg.length; i+=1)
			{
				if (this.cfg[i].uid === this.edit)
				{
					update = i;
					break;
				}
			}
		//check same rule exist or not
			for (j=0; j<this.cfg.length; j++)
			{
				if(j === update) continue;
				var dsc = e.desc;
				if(this.cfg[j].sun === (e.sun?"1":"0") && this.cfg[j].mon === (e.mon?"1":"0")
				&& this.cfg[j].tue === (e.tue?"1":"0") && this.cfg[j].wed === (e.wed?"1":"0")
				&& this.cfg[j].thu === (e.thu?"1":"0") && this.cfg[j].fri === (e.fri?"1":"0") 
				&& this.cfg[j].sat === (e.sat?"1":"0") && this.cfg[j].start === Start.TS()
				&& this.cfg[j].end === End.TS())
				{
					alert('<?echo i18n("The Rule ");?>"'+dsc+'\"<?echo i18n(" is already existed !");?>');
					return;
				}
			}
			if(update > -1) alert("<?echo i18n("You must reboot after update.");?>");

		this.cfg[i] = {
				uid:	this.edit?this.edit:"",
				desc:	e.desc,
				exc:	"0",
				sun:	e.sun?"1":"0",
				mon:	e.mon?"1":"0",
				tue:	e.tue?"1":"0",
				wed:	e.wed?"1":"0",
				thu:	e.thu?"1":"0",
				fri:	e.fri?"1":"0",
				sat:	e.sat?"1":"0",
				start:	Start.TS(),
				end:	End.TS()
			};
		BODY.OnSubmit();
	},
	OnClickSchCancel: function()
	{
		this.edit = null;
		OBJ("schsubmit").value = "<?echo i18n("Add");?>";

		OBJ("schdesc").value = "";
		OBJ("schdays").checked = true;
		this.OnClickSelectDays();
		OBJ("sch24hrs").checked = false;
		this.OnClick24Hours();

		OBJ("schstarthrs").value = "12"; 
		OBJ("schstartmin").value = "0";
		COMM_SetSelectValue(OBJ("schstartapm"), "AM");
		OBJ("schendhrs").value = "11";
		OBJ("schendmin").value = "59";
		COMM_SetSelectValue(OBJ("schendapm"), "PM");
	},
	OnEdit: function(i)
	{
		OBJ("schdays").checked = true;
		this.OnClickSelectDays();
		OBJ("sch24hrs").checked = false;
		this.OnClick24Hours();

		OBJ("schdesc").value = this.cfg[i].desc;

		OBJ("schsun").checked = this.cfg[i].sun==="1";
		OBJ("schmon").checked = this.cfg[i].mon==="1";
		OBJ("schtue").checked = this.cfg[i].tue==="1";
		OBJ("schwed").checked = this.cfg[i].wed==="1";
		OBJ("schthu").checked = this.cfg[i].thu==="1";
		OBJ("schfri").checked = this.cfg[i].fri==="1";
		OBJ("schsat").checked = this.cfg[i].sat==="1";

		var Start = new Time(this.cfg[i].start);
		var End = new Time(this.cfg[i].end);
		Start.To12Hrs();
		End.To12Hrs();
		
		COMM_SetSelectValue(OBJ("timeformat"), "12-hour");
		OBJ("schstartapm").disabled = OBJ("schendapm").disabled = false;
		OBJ("schstarthrs").value = Start.hour.toString();
		OBJ("schstartmin").value = Start.min.toString();
		COMM_SetSelectValue(OBJ("schstartapm"), Start.apm);
		OBJ("schendhrs").value = End.hour.toString();
		OBJ("schendmin").value = End.min.toString();
		COMM_SetSelectValue(OBJ("schendapm"), End.apm);

		OBJ("schsubmit").value = "<?echo i18n("Update");?>";
		this.edit = this.cfg[i].uid;
	},
	OnDelete: function(i)
	{
		var used_sch=[''<?
		foreach("/inf")		{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}
		foreach("/phyinf")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}	
		foreach("/nat/entry")
		{
												$uid=query("dmz/schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}
			foreach("virtualserver/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	} 
			foreach("portforward/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}
			foreach("porttrigger/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}			
		}
		foreach("/acl/macctrl/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}
		foreach("/acl/urlctrl/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}
		foreach("/acl/accessctrl/entry"){	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}		
		foreach("/acl/firewall/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}	
		foreach("/acl6/firewall/entry")	{	$uid=query("schedule"); if($uid!=""){echo ",'".$uid."'"."\n";}	}		
		?>];
		for(j=1;j < used_sch.length; j++)
		{	
			if(this.cfg[i].uid==used_sch[j])
			{
				alert("<?echo i18n("The schedule is used.");?>");
				return;
			}
		}
		this.cfg.splice(i,1);
		BODY.OnSubmit();
	},
	dummy: function() {}
};
</script>
