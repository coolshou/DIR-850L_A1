
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache"> 
<meta http-equiv="Cache-Control" content="no-cache"> 
<link rel="stylesheet" type="text/css" href="./style.css" title="HomePortal PDA VERSION" />
<title>Network</title>
<?
$AJAX_NAME		="__ajax_explorer";
$PARENT_OBJECT  ="the_sharepath";
include "./comm/__js_comm.php";
?>
<script type="text/javascript" language="JavaScript">
//----------------------------------------------

function bodyStyle()
{
	var ieorff = (navigator.appName=="Microsoft Internet Explorer"?"IE":"FF"); //default
	var main=document.getElementById("mainDIV");
	main.style.padding='0px';
	if(ieorff=="FF")
		main.style.width='98%';
	else	//IE
		main.style.width='104%';
	main.style.position='absolute';
	main.style.left='0px';
	main.style.top='0px';
	//padding:0px; width:99%; position: absolute; left:0px; top:0px;
	return;
}
var checkUSB=0;
//initial 	
function init()
{
	//to set div width from IE and Firefox.
	bodyStyle();
	
	ShowPath("Loading...");
	AddWaitTable();
	var localURL = document.location.href;
	if(localURL.search("page=bt") > 0)
	{
		checkUSB=1;
	}		
	refresh("","my_test");
}

var current_path='';
var current_location='';
var up_level_path='';
var up_level_location='';
var dataview_type='';
var pathArray='';

var request=Create_Ajax_Obj();
var DIRECTORY=4;
var FILES=5;

var dir_names="";


//Ajax obj
function Create_Ajax_Obj(){

	try {
		return new XMLHttpRequest();
	} catch (trymicrosoft) {
		try {
			return new ActiveXObject("Msxml2.XMLHTTP");
		} catch (othermicrosoft) {
			try {
				return new ActiveXObject("Microsoft.XMLHTTP");
			} catch (failed) {
				return null;
			}
		}
	}
}

//refresh 
function refresh(path,location)
{
	ShowStatusBar(false,0,0,0,0);
	if(location=="my_test" || location=="Directory")
    {
    	var str=new String("<?=$AJAX_NAME?>.sgi?");
		
		str+="action=getlist&path="+encodeURIComponent(path)+"&where=&date="+Date();
		
		current_path=path;
		current_location=location;
		dataview_type=GetDataviewType(current_path,current_location);
	    //alert("dataview_type " + dataview_type);
	    ShowPath(current_path);
	    AddWaitTable();
	    
	    	    
		freshData(str, "exec_getlist");
    }
}

//get the icon image that mapping to the file extend name 
function select_icon(fname)
{
	if(fname.substring(fname.length-4,fname.length).toLowerCase()=='.mp3' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.wma' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.mpg' ||
	   fname.substring(fname.length-5,fname.length).toLowerCase()=='.mpeg' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.avi' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.asf' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.mpe' ||
	   fname.substring(fname.length-3,fname.length).toLowerCase()=='.qt' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.mov' ||
	   fname.substring(fname.length-3,fname.length).toLowerCase()=='.mv' ||
	   fname.substring(fname.length-4,fname.length).toLowerCase()=='.wmv')
		return './images/player.gif';
	else
		return  './images/ukn.gif';
}

//add dir into table
function Add_TR(type,list_info)
{
	var data;
	var tr_html='';
	var path;
	
	switch(type)
	{
		case DIRECTORY:
		case FILES:		
			
			/* patch for ie browser */
			if(typeof(list_info)!='undefined' && list_info.length > 0)
			{
				if(list_info.substring(0,26)=='Anonymous login successful')
				{
					list_info=list_info.substring(27,list_info.length-1);
				}	
			}
			
			//First add ..(previous page)
			tr_html+='<tr>';
			tr_html+='<td width="20px">';
			tr_html+='<img border="0" src="images/fo.gif" width="16" height="16">';
			tr_html+='</td>';
			tr_html+='<td>';
			tr_html+='<a href=\"javascript:GoToUpLevel();\">';
			tr_html+='..</a>';
			tr_html+='</td>';
			tr_html+='<td width="57px"></td>';
			tr_html+='</tr>';
			
			data=escape(list_info);	/* convert data to ASCII ?? */
			data=data.split('%0A');
			//alert("data \n" + data);

			/*
			data[0], data[1] now is a line
			*/
			
			//alert("data[0] is " + data[0]);
			//alert("data[1] is " + data[1]);
			//alert("data[2] is " + data[2]);
			
			var stdata;
			var temp=new Array();
			var subtract_line=1;
			var my_j = 0;
			var temp = new Array();
			var count= 0;
			var dir_name;
			//alert("data.length is " + data.length );
			
			/* parse each line */
			for (var j=0;j<data.length;j++)
			{
				
				temp[j] = new Array();
				//alert("data["+j+"] is : " + data[j]);				
				stdata=data[j].split('%20');
				
				/*
				"-rw-r--r--    1 0        0            5.5k Jan  1 02:58 nas_iTunes.php"
				"-rw-r--r--    1 0        0              96 Jan  1 02:58 wpsinfo.phpfo.php"
				*/
				
				/* store all values in temp[][] */
				count = 0;
				
				//alert("stdata.length " + stdata.length);
				
				for (var i=0;i<(stdata.length);i++)
				{
					if(unescape(stdata[i])!='')
					{
						temp[j][count] = stdata[i];
						count++;
					}
					//alert("temp[j][8] " + temp[j][8] + "\n");
				}
				/* temp[j][8] now store the filename   (ex : nas_iTunes.php) */
				/* temp[j][0] now store the permission (ex : -rw-r--r--) */
				
			} //end of for (each line parsing)


			dir_names = new Array();
			/* write html codes for DIRECTORIES and SYMBOLIC LINK DIRECTORIES 
			(check it from temp[j][0] --> permissions) */
			for (var j=0;j<data.length-subtract_line;j++)
			{	
				if(temp[j][0] == "") {return;}
				if(checkUSB==1)
				{
					if(current_path=="" && temp[j][8].substring(0,3)=="USB" ) 
					{
						j++;
						if(j>=data.length-subtract_line)
						{break;}
					}
				}
				if( ((temp[j][0].substring(0,1)=='d') || (temp[j][0].substring(0,1)=='l')) && temp[j][8]!='.' && temp[j][8]!='..' )
				{
					/* get directory name */
					dir_name = unescape(temp[j][8]);
					/* search for directory name which contain whitespace in it */
					if(temp[j].length > 9)
					{
						for(k = 9 ; k < temp[j].length ; k++ )
						dir_name += ' ' + unescape(temp[j][k]);	
					}
					
					dir_names[j] = dir_name;
					//dir_name=replace_spec_chars(dir_name);
					
					tr_html+='<tr>';
					tr_html+='<td width="20px">';
					tr_html+='<img border="0" src="images/fo.gif" width="16" height="16">';
					tr_html+='</td>';
					tr_html+='<td>';
					tr_html+='<a href=\"javascript:refresh(\'';
					tr_html+=current_path.replace(/'/g,"\\\'")+'/'+ dir_name.replace(/'/g,"\\\'");//jana added
					//tr_html+=current_path+'/'+ dir_name;//jana removed
					tr_html+='\',\'my_test\');\">';
					tr_html+=dir_name +'</a>';						
					tr_html+='</td>';
					tr_html+='<td width="57px"><input type=button value="<?echo i18n("Apply");?>"';
					//tr_html+=' onclick="LinkDir(\''+current_path+'/'+dir_name+'\',\'SMBMount_F\');"';//jana removed
					//tr_html+=' onclick="LinkDir(\''+current_path.replace(/'/g,"\\\'")+'/'+dir_name.replace(/'/g,"\\\'")+'\',\'SMBMount_F\');"';//jana added
					tr_html+=' onclick="ApplyLinkDir(\''+current_path.replace(/'/g,"\\\'")+'/'+dir_name.replace(/'/g,"\\\'")+'\');"';
					tr_html+='></td>';
					tr_html+='</tr>';
					
				}
			}
						
			/* write html codes for FILES 
			(check it from temp[j][8] --> permissions) */
			/*
			for (var j=0;j<data.length-subtract_line;j++)
			{
				if(temp[j][0] == "") {return;}
				if(temp[j][0].substring(0,1)=='-' && temp[j][1].search('H')!=1)
				{
					tr_html+='<tr>';
					tr_html+='<td width="20px">';
					tr_html+='<img border="0" width="16" height="16" src="';
					tr_html+=select_icon(temp[j][8]);
					tr_html+='" width="16" height="16">';
					tr_html+='</td>';
					tr_html+='<td style="word-break:break-word">';
					tr_html+=temp[j][8];
					tr_html+='</td>';
					tr_html+='<td width="58px" align="right">';
					tr_html+='</td>';
					tr_html+='</tr>';
				}
			}*/
			break;
		default:
			headertext=',,';
			break;
	}
	return tr_html;
}

//add table header	
function create_table_th(HeaderText)
{
	var THeader=new Array();
	THeader=HeaderText.split(",");
	var th_html='';
	th_html+='<tr>';
	for(var i=0;i<THeader.length;i++)
	{
		th_html+='<th>';
		th_html+=THeader[i];
		th_html+='</th>';		
	}
	th_html+='</tr>';
	return th_html;
}

//add file list table
function Add_Table(type,resText)
{	
	var headertext=',<?echo i18n("Name");?>,<?echo i18n("Option");?>';
	var file_table_html='';
	file_table_html+='<table border="0" width="100%" cellspacing="0" cellpadding="0" id="file_table" name="file_table"><tbody>';
	if(type==DIRECTORY || type==FILES )
	{
		headertext=',<?echo i18n("Name");?>,<?echo i18n("Option");?>';
		file_table_html+=create_table_th(headertext);
	}
	file_table_html+=Add_TR(type,resText);
	file_table_html+='</tbody></table>';
	return file_table_html;
}

/*show alert message,
	display:display message,value=boolean,true or false ,
	txt:message,value=string,
	gif:images type,value=int, 1:information,2:attention,
	always:consistent display,value=boolean,true or false,
	timeout:consistent time,value=int, 1000= 1second.
*/
function ShowStatusBar(display,txt,gif,always,timeout)
{
	var attention_table_html='';
	if(!display)
	{
		statusBar.innerHTML=attention_table_html;
		return;
	}
	else
	{
		attention_table_html+='<table border="0" width="100%" cellspacing="0" cellpadding="0" id="attention_table" name="attention_table"><tbody>';		
		attention_table_html+='<tr><td width="20px"><img border="0" src="images/message_icon';
		attention_table_html+=gif;
		attention_table_html+='.gif" width="16" height="16"></td><td>';
		attention_table_html+='<div name="message" id="message" style="float:left;FONT-SIZE: 12px; COLOR:#FFFFCC; text-align:left; width:100%; height:14px">';
		attention_table_html+=txt;
		attention_table_html+='</div></td></tr></tbody></table>';
		statusBar.innerHTML=attention_table_html;
	}
	if(!always)
		setTimeout("ShowStatusBar(false,0,0,false,0)",timeout);
}

//Set Up Level Path variable
function SetUpLevelPath()
{	
	switch(dataview_type)
	{
		case DIRECTORY:
			up_level_path = "";
			//alert("current_path " + current_path);
			pathArray = current_path.split('/');
			//alert("pathArray.length "+pathArray.length);
			for(var i=0;i<pathArray.length-1;i++)
			{
				up_level_path+=pathArray[i];
				if(i!=pathArray.length-2)
					up_level_path+='/';
				
			}
			up_level_location='my_test';
			break;

		case FILES:
			up_level_path='';
			for(var i=0;i<pathArray.length-1;i++)
			{
				up_level_path+=pathArray[i];
				//if not the last element,the next char must be '/'
				if(i!=pathArray.length-2)
					up_level_path+='/';
			}
			up_level_location='Directory';
			break;
		
		default:
			up_level_path='/';
			up_level_location='my_test';
			break;
	}
	//alert("direc up_level_path "+ up_level_path + ", up_level_location "+up_level_location);
}

//Go To Up Level
function GoToUpLevel()
{
	SetUpLevelPath();
	refresh(up_level_path,up_level_location);
}	

function initFileList()
{
	if (request!=null && request.readyState == 4) 
    {
    	//alert(request.responseText);
    	//alert(escape(request.responseText));
		__reList = request.responseText;
		filelist.innerHTML=Add_Table(dataview_type,__reList);
	}
}

function SaveLinkDir()
{
	var topDiv=window.parent.document.getElementById("<?=$PARENT_OBJECT?>");
	
	var link_text=document.getElementById("link_text");
	//alert("link_text.value is " + link_text.value);
	if(link_text.value.substring(0,1)=="/")
	{
		link_text.value = link_text.value.substring(1, link_text.value.length);
	}
	else if(link_text.value=="")
	{
		alert('<?echo I18N("j","Please apply a direction.");?>');
		return;
	}		
	topDiv.value=link_text.value;
	
	window.parent.window_destroy_singlet(true);
}

function LinkDir(path,flag)
{
	var link_text=document.getElementById("link_text");
		
	if(flag=="SMBMount_D")
		link_text.value="/"+path;
	else
		link_text.value=path.substring(path.indexOf("/"),path.length);;
}

function ApplyLinkDir(path)
{
	var apply_link_text=path.substring(path.indexOf("/"),path.length);;
	if(apply_link_text.substring(0,1)=="/")
	{
		apply_link_text = apply_link_text.substring(1, apply_link_text.length);
	}
	else if(apply_link_text=="")
	{
		alert('<?echo I18N("j","Please apply a direction.");?>');
		return;
	}
	
	var topDiv=window.parent.document.getElementById("<?=$PARENT_OBJECT?>");
	topDiv.value=apply_link_text;

	if(parent.PAGE.window_destroy_callback)
		parent.PAGE.window_destroy_callback();	
	window.parent.window_destroy_singlet(true);	
}

//show current path ,flag=true or false,txt=message
function GetPathGif()
{
	var gifname='';
	switch(dataview_type)
	{
		case DIRECTORY:
			gifname='nfo.gif';
			break;
		case FILES:
			gifname='nfo.gif';
			break;
		default:
			gifname='nfo.gif';
			break;
	}
	return gifname;
}

//show current path
function ShowPath(txt)
{
	var path_area_html;
	path_area_html = '<TABLE border="0" width="100%" cellspacing="0" cellpadding="0">';
    path_area_html +='<TBODY>';
    path_area_html +='<TR><TD width="20px" ><img border="0" src="images/';
    path_area_html +=GetPathGif();				
  	path_area_html +='" width="20" height="20"></TD>';
	path_area_html +='<TD style="float:left;text-align: left;">';
	path_area_html +='<div name="path_text" id="path_text" style="float:left;text-align: left; width:100%;">';
	path_area_html +=txt;
	path_area_html +='</div></TD></TR>';
	path_area_html +='<TR><TD colspan="2" height="2px"  bgcolor="#455866"></TD></TR>';
    path_area_html +='</TBODY></TABLE>';
    
    path_area.innerHTML=path_area_html;
}

/*Get Current Data View Type by path and location
	Data View Type :
		WORKGROUPS : path='',location=='Network'
		COMPUTERS : path='workgroup1',location=='Network'
		FOLDERLIST : path='workgroup1/pc1',location=='Network'
		DIRECTORY : path='workgroup1/pc1/images',location=='FolderList'
		FILES : path='workgroup1/pc1/images/123',location=='Directory'
*/
function GetDataviewType(path,location)
{
	pathArray=path.split('/');
	var type='';
	//+++ hendry modify
	if(location=='my_test')
		type=DIRECTORY;

	return type;		
}

//data request function (ajax)
function freshData(path,location)
{	
	var url='';
	var readyFunc;
	if(location=="exec_getlist")
	{
		ShowPath(current_path);
		AddWaitTable();
		var url=path;	
		readyFunc = initFileList;
	}
	else if(location=="exec_mkdir")
	{
		ShowPath(current_path);
		AddWaitTable();
		var url=path;	
		readyFunc = initFileList;
	}
	
	/* in first page of explorer(root), we can't create new directory */
	if(current_path == "" || current_path == "/" )
		new_dir_div.style.display = "none";
	else
		new_dir_div.style.display = "";

	request.open("GET", url, true);
	request.onreadystatechange = readyFunc;
	request.send(null);
}

//Show waitting images
function AddWaitTable()
{
	filelist.innerHTML='<TABLE width="100%" height="100%"><TBODY>';
	filelist.innerHTML+='<TR><TD><p align="center"><img src="images/wait.gif" align="center"></TD></TR></TBODY></TABLE>';
}

function CreateDir()
{
	var newDirectoryName = document.mainform.new_dir_input.value;
	var path = current_path;
	
	//+++jana
	if(newDirectoryName == "")
	{
		alert("<?echo i18n("Please enter a folder name.");?>");
		return;
	}
		
	/*check whether folder name include invalid characters*/
	var re=/[\\/:*?"<>|]/;
	if(re.exec(newDirectoryName))
	{ 
		alert('<?echo I18N("j","The folder name can not includes the following characters: \\ /:*?\"<>|");?>');
		return;
	}
	
	if(newDirectoryName.indexOf(" ")==0)
	{
		alert("<?echo i18n("The first character of folder name can not be blank.");?>");
		return;
	}
	//---jana
	
	//+++ hendry, check new directory already exist or not
	for(var i=0;i<dir_names.length;i++)
	{
		if(newDirectoryName==dir_names[i])
		{
			alert("<?echo i18n("Can't create directory. Already exists.");?>");
			return;
		}
	}
	//---
		
	/*clear the new directory name*/
	document.mainform.new_dir_input.value = "";
	
	if(path.substring(0,1)=="/")
	{	/* omit the '/' */
		path = path.substring(1,path.length);
	}
	
	var str=new String("<?=$AJAX_NAME?>.sgi?");
	str+="action=mkdir&path="+encodeURIComponent(path)+"&where="+encodeURIComponent(newDirectoryName)+"&date="+Date();
	//str+="action=mkdir&path="+path+"&where="+encodeURIComponent(newDirectoryName)+"&date="+Date();
	freshData(str, "exec_mkdir");
	
	/*
	var str=new String("<?=$AJAX_NAME?>.sgi?");
	str+="action=getlist&path="+path+"&where=&date=";
	
	freshData(str, "exec_getlist");*/
	
	//refresh(current_path,"my_test");
}



</script>
</head>
<body onload="init();">
<form name="mainform" id="mainform" method="post">
<DIV id="mainDIV"><!--style="padding:0px; width:99%; position: absolute; left:0px; top:0px; z-index:2" >-->
	<!--
	<H2><?echo i18n("Connect ");?>: <input id="link_text" type=text size=40 maxlength=100 readonly>  <input type=button value="<? echo I18N("h", "Save");?>" onclick="SaveLinkDir();"></H2>
	-->
	<TABLE cellSpacing=0 cellPadding=0 width="100%" height="97%" border=0>
		<TBODY>
			<TR>
				<TD vAlign=top width="100%" style="word-break:break-all;">
					<div id="statusBar"  name="statusBar" style="float:left;display:block;position:relative; width:100%;text-align: left;"></div>
					<div id="path_area" name="path_area" style="float:left;display:block;position:relative; width:100%;text-align: left" ></div>
					<div id="filelist"  style="float:left;display:block;position:relative; width:100%;text-align: left"></div>
            	</TD>
            </TR>
		</TBODY>
	</TABLE>
	<div id="new_dir_div" name="new_dir_div">
		<H5><?echo i18n("New Directory");?><br>
		<input name="new_dir_input" id="new_dir_input" type=text size=40 maxlength=40>&emsp;
		<input name="new_dir_button" id="new_dir_button" type=button value="<? echo I18N("h", "Create");?>" onclick="CreateDir();">
		</H5>
	</div>
</DIV>
</form>
</body>
</html>