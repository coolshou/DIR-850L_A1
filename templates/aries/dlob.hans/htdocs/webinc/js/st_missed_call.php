<?include "/htdocs/phplib/inet.php";?>
<?include "/htdocs/phplib/inf.php";?>
<script type="text/javascript">
var tmpSMS;
var sms;
var must_wait=0;

function Page() {}
Page.prototype =
{
	services: "CALL.MISSED",
        OnLoad: function() {},
        OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return false; },
	InitValue: function(xml)
        {
		PXML.doc = xml;
                call = PXML.FindModule("CALL.MISSED");
                if (call===""){ alert("InitValue ERROR!"); return false; }
		total_call = call + "/runtime/callmgr/voice_service/log/missed_count"
		call += "/runtime/callmgr/voice_service/log/missed";

		var str = "<table class=\"general\" id=\"Tmissed_call\"><tr>";
		str += '<th width="120px">' + "<?echo i18n("Caller");?>" + "</th>";
		str += '<th width="151px">' + "<?echo i18n("Time");?>" + "</th>";
		str += "</tr>";
		
		for (var i=1; i<=<?=$MISSED_CALL_MAX_COUNT?>; i+=1)
                {
			var missed_caller = XG(call + ":" + i + "/caller");
			var missed_time = XG(call + ":" + i + "/time");

			if(missed_caller != "" && missed_time != "")
			{
				str += "<tr>";
                        	str += "<td width=\"115px\">" + missed_caller + "</td>";
				str += "<td width=\"136px\">" + missed_time + "</td>";
				str += "</tr>";
			}
		}
		str += "</table>";

		OBJ("missed_call").innerHTML = str;
		OBJ("missed_call_cnt").innerHTML = XG(total_call);

		new TableSorter("Tmissed_call",1);

                return true;
        },
        PreSubmit: function() { return null; },
        IsDirty: null,
        Synchronize: function() {}
        // The above are MUST HAVE methods ...
        ///////////////////////////////////////////////////////////////////////
}


function SelectAll()
{
	for (var i=0;i < OBJ("mainform").elements.length;i++)
	{
		var e = OBJ("mainform").elements[i];
		if (e.id != 'checkall' && e.type == 'checkbox' && e.disabled==false)
			e.checked = OBJ("mainform").checkall.checked;
	}
}

function TableSorter(table)
{
	this.Table = this.$(table);
	if(this.Table.rows.length <= 1)
	{
		return;
	}
	this.Init(arguments);
}

TableSorter.prototype.NormalCss = "NormalCss";
TableSorter.prototype.SortAscCss = "SortAscCss";
TableSorter.prototype.SortDescCss = "SortDescCss";

TableSorter.prototype.Init = function(args)
{
	this.ViewState = [];
	for(var x = 0; x < this.Table.rows[0].cells.length; x++)
	{
		this.ViewState[x] = false;
	}

	if(args.length > 1)
	{
		for(var x = 1; x < args.length; x++)
		{
			if(args[x] > this.Table.rows[0].cells.length)
			{
				continue;
			}
			else
			{
				this.Table.rows[0].cells[args[x]].onclick = this.GetFunction(this,"Sort",args[x]);
				this.Table.rows[0].cells[args[x]].style.cursor = "pointer";
			}
		}
	}
	else
	{
		for(var x = 0; x < this.Table.rows[0].cells.length; x++)
		{
			this.Table.rows[0].cells[x].onclick = this.GetFunction(this,"Sort",x);
			this.Table.rows[0].cells[x].style.cursor = "pointer";
		}
	}
}	

TableSorter.prototype.$ = function(element)
{
	return document.getElementById(element);
}

TableSorter.prototype.GetFunction = function(variable,method,param)
{
	return function()
	{
		variable[method](param);
	}
}

TableSorter.prototype.Sort = function(col)
{
	var SortAsNumber = true;
	for(var x = 0; x < this.Table.rows[0].cells.length; x++)
	{
		this.Table.rows[0].cells[x].className = this.NormalCss;
	}
	
	var Sorter = [];
	for(var x = 1; x < this.Table.rows.length; x++)
	{
		Sorter[x-1] = [this.Table.rows[x].cells[col].innerHTML, x];
		SortAsNumber = SortAsNumber && this.IsNumeric(Sorter[x-1][0]);
	}
	
	if(SortAsNumber)
	{
		for(var x = 0; x < Sorter.length; x++)
		{
			for(var y = x + 1; y < Sorter.length; y++)
			{
				if(parseFloat(Sorter[y][0]) < parseFloat(Sorter[x][0]))
				{
					var tmp = Sorter[x];
					Sorter[x] = Sorter[y];
					Sorter[y] = tmp;
				}
			}
		}
	}
	else
	{
		Sorter.sort();
	}

	if(this.ViewState[col])
	{
		Sorter.reverse();
		this.ViewState[col] = false;
		this.Table.rows[0].cells[col].className = this.SortDescCss;
	}
	else
	{
		this.ViewState[col] = true;
		this.Table.rows[0].cells[col].className = this.SortAscCss;
	}
	
	var Rank = [];
	for(var x = 0; x < Sorter.length; x++)
	{
		Rank[x] = this.GetRowHtml(this.Table.rows[Sorter[x][1]]);
	}
	for(var x = 1; x < this.Table.rows.length; x++)
	{
		for(var y = 0; y < this.Table.rows[x].cells.length; y++)
		{
			this.Table.rows[x].cells[y].innerHTML = Rank[x-1][y];
		}
	}

	this.OnSorted(this.Table.rows[0].cells[col], this.ViewState[col]);
}

TableSorter.prototype.GetRowHtml = function(row)
{
	var result = [];
	for(var x = 0; x < row.cells.length; x++)
	{
		result[x] = row.cells[x].innerHTML;
	}
	return result;
}

TableSorter.prototype.IsNumeric = function(num)
{
	return /^\d+(\.\d+)?$/.test(num);
}

TableSorter.prototype.OnSorted = function(cell, IsAsc)
{
	return;
}

</script>
