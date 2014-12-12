﻿HTTP/1.1 200 OK

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />	
	<meta http-equiv="X-UA-Compatible" content="IE=EmulateIE8" />
	<title>SharePort Web Access</title>	
	<link rel="stylesheet" type="text/css" href="webfile_css/reset.css" />
	<link rel="stylesheet" type="text/css" href="webfile_css/web_file_access.css" />
	<script type="text/javascript" src="webfile_js/webfile.js"></script>
	<link rel="stylesheet" type="text/css" href="fancybox/style.css" />
	<link rel="stylesheet" type="text/css" href="webfile_css/layout.css" />

	<link rel="stylesheet" type="text/css" href="fancybox/jquery.fancybox-1.3.4.css" media="screen" />
	<script type="text/javascript" src="fancybox/jquery-1.4.3.min.js"></script>
<!--	<script type="text/javascript" src="fancybox/jquery.mousewheel-3.0.4.pack.js"></script> -->
	<script type="text/javascript" src="fancybox/jquery.fancybox-1.3.4.pack.js"></script>
	<script type="text/javascript" src="fancybox/json2.js"></script>
	<script language="JavaScript" src="js/object.js"></script>
	<script language="JavaScript" src="js/xml.js"></script>

	<script language="JavaScript" src="js/public.js"></script>
	<script type="text/javascript">
		var current_path;
		var current_volid;
		var usb_list = new Array(); 
		var storage_user = new HASH_TABLE();
		var file_info;		
		
		load_lang_obj();	// you have to load language object for displaying words in each html page and load html object for the redirect or return page
		
		var images = {
						"file" 	:	"webfile_images/file.png",
						"3GP" 	:	"webfile_images/film.png",
						"AVI"	:	"webfile_images/film.png",
						"MOV"	:	"webfile_images/film.png",
						"MP4"	:	"webfile_images/film.png",
						"MPG"	:	"webfile_images/film.png",
						"MPEG"	:	"webfile_images/film.png",
						"WMV"	:	"webfile_images/film.png",
						"M4P"	:	"webfile_images/music.png",
						"MP3"	:	"webfile_images/music.png",
						"OGG"	:	"webfile_images/music.png",
						"WAV"	:	"webfile_images/music.png",
						"ASP"	:	"webfile_images/code.png",
						"ASPX"	:	"webfile_images/code.png",
						"C"		:	"webfile_images/code.png",
						"H"		:	"webfile_images/code.png",
						"CGI"	:	"webfile_images/code.png",
						"CPP"	:	"webfile_images/code.png",
						"VB"	:	"webfile_images/code.png",
						"XML"	:	"webfile_images/code.png",
						"CSS"	:	"webfile_images/css.png",
						"BAT"	:	"webfile_images/application.png",
						"COM"	:	"webfile_images/application.png",
						"EXE"	:	"webfile_images/application.png",
						"BMP"	:	"webfile_images/picture.png",
						"GIF"	:	"webfile_images/picture.png",
						"JPG"	:	"webfile_images/picture.png",
						"JPEG"	:	"webfile_images/picture.png",
						"PNG"	:	"webfile_images/picture.png",
						"PCX"	:	"webfile_images/picture.png",
						"TIF"	:	"webfile_images/picture.png",
						"TIFF"	:	"webfile_images/picture.png",
						"DOC"	:	"webfile_images/doc.png",
						"DOCX"	:	"webfile_images/doc.png",
						"PPT"	:	"webfile_images/ppt.png",
						"PPTX"	:	"webfile_images/ppt.png",
						"XLS"	:	"webfile_images/xls.png",
						"XLSX"	:	"webfile_images/xls.png",
						"HTM"	:	"webfile_images/html.png",
						"HTML"	:	"webfile_images/html.png",
						"PHP"	:	"webfile_images/php.png",
						"JAR"	:	"webfile_images/java.png",
						"JS"	:	"webfile_images/script.png",
						"PL"	:	"webfile_images/script.png",
						"PY"	:	"webfile_images/script.png",
						"RB"	:	"webfile_images/ruby.png",
						"RBX"	:	"webfile_images/ruby.png",
						"RUBY"	:	"webfile_images/ruby.png",
						"RHTML"	:	"webfile_images/ruby.png",
						"RPM"	:	"webfile_images/linux.png",
						"PDF"	:	"webfile_images/pdf.png",
						"PSD"	:	"webfile_images/psd.png",
						"SQL"	:	"webfile_images/db.png",
						"SWF"	:	"webfile_images/flash.png",
						"LOG"	:	"webfile_images/txt.png",
						"TXT"	:	"webfile_images/txt.png",
						"ZIP"	:	"webfile_images/zip.png",
						"RAR"	:	"webfile_images/zip.png",
						"7Z"	:	"webfile_images/zip.png",
						"GZ"	:	"webfile_images/zip.png",
						"BZ2"	:	"webfile_images/zip.png"
						};
		
		$(document).ready(function() {
						
			$("a[id=button1]").fancybox({
				'showCloseButton'	: false,
				'hideOnOverlayClick' : false,
				'overlayShow'		: true
			});
			
			$("a[id=button2]").fancybox({
				'showCloseButton'	: false,
				'hideOnOverlayClick' : false,
				'overlayShow'		: true
			});
														
			$(".uploadtab>thead").css("background-color", "#808080");
			$(".uploadtab>tfoot").css("background-color", "#808080");
		});
		
		function close_fancybox(){
			$.fancybox.close();
		}
		
		function create_folder_result(http_req){
			var my_txt = http_req.responseText;			
			var result_info;
			
			try {
				result_info = JSON.parse(my_txt);
			} catch(e) {				
				return;
			}
			
			close_fancybox();
			
			if (result_info.status == "ok" && result_info.errno == null){	
				get_sub_folder(current_path, current_volid);
			}else if(result_info.errno == "5002")
				alert("No HardDrive Connected");
			get_by_id("folder_name").value = "";
		}
		
		function create_folder(){						
			var xml_request = new XMLRequest(create_folder_result);
			var folder_name = get_by_id("folder_name").value;			

/*
            if (!check_special_char(folder_name)){
                msg_obj.warning_msg('MSG143');
                return;
            }
*/
			/*check whether folder name include invalid characters*/
			var re=/[\\/:*?"<>|]/;
			if(re.exec(folder_name))
			{ 
				alert("The folder name can not includes the following characters: \\ /:*?\"<>|");
				return;
			}
				
			var para = "AddDir?id=" + storage_user.get("id") + "&tok=" + storage_user.get("tok") 
					 + "&volid=" + current_volid + "&path=" + encodeURIComponent(current_path);
			
			
			para += "&dirname=" + encodeURIComponent(folder_name);						
			xml_request.json_cgi(para);
		}
		
		function show_content_height(){
			var folder_h = get_by_id("left2").offsetHeight;
			var file_h = get_by_id("right").offsetHeight;
									
			if (folder_h < file_h){
				get_by_id("left2").style.position = "fixed";
			}else{
				get_by_id("left2").style.position = "";
			}	
		}

		function show_folder_content(which_info){			
			var rtable = get_by_id("rtable");			
			var sum = 0;
					
			if (rtable.rows.length > 0){
				for (var i = rtable.rows.length; i > 0; i--){
					rtable.deleteRow(i-1);
				}
			}
			
			var tmp=get_by_id(current_path);
			for (var i = 0; i < which_info.count; i++){			
				var obj = which_info.files[i];
				var desp = obj.desp.toUpperCase();
				var file_name = obj.name;			
				var file_path = current_path + "%2F" + encodeURIComponent(file_name);
				var dev_name = file_path.split("%2F");	// to get the usb name
				var time = ctime(obj.mtime);
				var row;
				
				if (file_name == ""){
					continue;
				}
				
				for (var j = 0; j < 1; j++){
					var insert_cell; // create a cell
					var cell_html;
					
					if (obj.type != 1){
						row = rtable.insertRow(sum);
						sum = sum + 1;
						insert_cell = row.insertCell(j);

						
						cell_html = "<input type=\"checkbox\" id=\"" + i + "\" name=\"" + file_name + "\" value=\"1\"/>"
								 // + "<a href=\"/" + file_path + "\" target=\"_blank\">"
 									//+ "<a href=\"/dws/api/GetFile?" + file_path + "\" target=\"_blank\">"
									+ "<a href=\"/dws/api/GetFile?id=" + storage_user.get("id") + "&tok=" +storage_user.get("tok")+"&volid="+current_volid+"&path="+encodeURIComponent(current_path)+"&filename="+encodeURIComponent(obj.name)+"\" target=\"_blank\">"
 									
								  + "<div style=\"width:665px;overflow:hidden\">"
								  + file_name + "<br>" + get_file_size(obj.size) + ", " + time
								  + "</div></a>";
					}else{
						break;
					}
					
					switch(j){
						case 0:	// Name
							insert_cell.id = "rname";							
							insert_cell.className = "tdbg";
							insert_cell.innerHTML = cell_html;
							break;						
					}
				}
			}									
			show_content_height();
		}
		
		function get_sub_tree(which_info){
			var my_tree = "<ul class=\"jqueryFileTree\">";
									
			for (var i = 0; i < which_info.count; i++){
				var obj = which_info.files[i];
				var obj_path = current_path + "/" + obj.name;
				//alert(obj_path);
				if (obj.name == ".." || obj.type != 1){	// when it's not a folder
					continue;
				}
				
				my_tree += "<li id=\"" + obj_path + "\" class=\"tocollapse\">"
						+  "<a href=\"#\" onClick=\"click_folder('" + obj_path + "', '" + current_volid + "', '" +obj.mode+ "')\">"
						+ obj.name + "</a></li>"
						+ "<li></li>"
						+ "<li><span id=\"" + obj_path + "-sub\"></span></li>";
			}				
			   
			my_tree += "</ul>";

			return my_tree;
		}
		
		function show_sub_folder(http_req){
			var my_txt = http_req.responseText;
			var parent_node = get_by_id(current_path);
			var current_node = get_by_id(current_path + "-sub");
			
			try {
				file_info = JSON.parse(my_txt);
			} catch(e) {				
				return;
			}
						
			parent_node.className = "toexpanded";
			
			if (file_info.status == "ok" && file_info.error == null){
				current_node.innerHTML = get_sub_tree(file_info);
				show_folder_content(file_info);
			}
		}
		
		function get_sub_folder(which_path, which_volid){

			var xml_request = new XMLRequest(show_sub_folder);
			
			var para = "ListFile?id=" + storage_user.get("id") + "&tok=" + storage_user.get("tok") 
					 + "&volid=" + which_volid + "&path=" + encodeURIComponent(which_path) + "&random=" + Math.random();
			
			current_path = which_path;
			current_volid = which_volid;
			
			xml_request.json_cgi(para);
		}
		
		function click_folder(path, volid, mode){
			var obj = get_by_id(path);
			mode = storage_user.get("mode");
			close_fancybox();
					
			if (obj != undefined){				
				if (mode < 2){
					get_by_id("create_btn").disabled = true;
					get_by_id("upload_btn").disabled = true;
					get_by_id("delete_btn").disabled = true;
					get_by_id("button1").disabled = true;
					get_by_id("button2").disabled = true;
				}else{
					get_by_id("create_btn").disabled = false;
					get_by_id("upload_btn").disabled = false;
					get_by_id("delete_btn").disabled = false;
					get_by_id("button1").disabled = false;
					get_by_id("button2").disabled = false;
					
				}
				
				if (obj.className == "toexpanded"){
					obj.className = "tocollapse";
					get_by_id(path + "-sub").innerHTML = "";
				}else{
					obj.className = "toexpanded";
				get_sub_folder(path, volid);
			}			
			}			
			
		}
		var upload_progress_flag=0;
		function upload_ajax()
		{
			var upload_name = get_by_id("upload_file").value;
			var file_name;
			
			if (window.ActiveXObject){	// code for IE
				file_name = upload_name.substring(upload_name.lastIndexOf("\\") + 1);
			}else{
				if (upload_name.indexOf("C:\\fakepath\\") != -1){
					file_name = upload_name.substring(upload_name.indexOf("C:\\fakepath\\") + 12);
				}else{
					file_name = upload_name;
				}
			}
			
			var xhr = new XMLHttpRequest;
			var fd = new FormData();
			
			xhr.open("POST", "/dws/api/UploadFile?" + Math.random(), true);
			xhr.onreadystatechange = function() 
			{

				if (xhr.readyState === 4) {
					refresh_current_path();
					if(upload_progress_flag==0)
					    setTimeout("close_fancybox()", 1000);
				}
			};
		    
			
			var Accept = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
			xhr.setRequestHeader("Accept", Accept);
			
			fd.append("id", storage_user.get("id"));
			fd.append("tok", storage_user.get("tok"));
			fd.append("volid", storage_user.get("volid"));
			fd.append("path", current_path);
			//fd.append("path", storage_user.get("volid"));
			fd.append("filename", file_name);
			fd.append("file", get_by_id("upload_file").files[0]);
		    
			xhr.upload.addEventListener("progress", uploadProgress, false);
		    upload_progress_flag=0;
			xhr.send(fd);
		}
		 
		function uploadProgress(evt) 
		{
		    upload_progress_flag=1;
			if(evt.lengthComputable)
			{
				var percentComplete = Math.round(evt.loaded * 100 / evt.total);
				document.getElementById('progressPercentage').innerHTML = percentComplete.toString() + '%';
				document.getElementById('progressBar').value = percentComplete;				
				if(percentComplete==100) setTimeout("close_fancybox()", 2000);
			}
			else
			{
				document.getElementById('progressPercentage').innerHTML = 'unable to compute';
			}				
		}	      	
						 				
		function check_upload_file(){			
			var form1 = get_by_id("form1");
			var upload_name = get_by_id("upload_file").value;
			var file_name;
			
			if (window.ActiveXObject){	// code for IE
				file_name = upload_name.substring(upload_name.lastIndexOf("\\") + 1);
			}else{
				if (upload_name.indexOf("C:\\fakepath\\") != -1){
					file_name = upload_name.substring(upload_name.indexOf("C:\\fakepath\\") + 12);
				}else{
					file_name = upload_name;
				}
			}
			
			form1.action = "/dws/api/UploadFile?" + Math.random();
			
			get_by_id("id").value = storage_user.get("id");
			get_by_id("tok").value = storage_user.get("tok");
			//get_by_id("volid").value = 1;  //storage_user.get("volid");  //wrong volid from web use 1 to let cgi handle special case
			
			get_by_id("volid").value=storage_user.get("volid");
			get_by_id("path").value = current_path + "/";	// remember to remove %2F in next firmware
			get_by_id("filename").value = file_name;
						
			form1.submit();	
//			click_folder( current_path, storage_user.get("volid"), 2);
			click_folder( current_path, storage_user.get("volid"), storage_user.get("mode"));  //jef
		}

		function delete_file_result(http_req){
			var my_txt = http_req.responseText;			
			var result_info;
			
			try {
				result_info = JSON.parse(my_txt);
			} catch(e) {				
				return;
			}
			
			if (result_info.status == "ok" && result_info.errno == null){
				get_sub_folder(current_path, current_volid);
			}
		}
		
		function delete_file(){						
			var xml_request = new XMLRequest(delete_file_result);
			var para;
			var checked_flag=0;
			var str="";			
			
			para = "DelFile?id=" + storage_user.get("id") + "&tok=" + storage_user.get("tok") 
				 + "&volid=" + current_volid + "&path=" + current_path;
				
			for (var i = 0; i < file_info.count; i++){
				var file_name = file_info.files[i].name;
				var type = file_info.files[i].type;
				if (type != 1){
					if (get_by_id(i).checked){
						checked_flag = 1;
						if (str != ""){
							str += ",\"" + file_name + "\"";
						}else{
							str += "\"" + file_name + "\"";
						}
					}
				}
			}
			
			if (checked_flag == 0){
				alert("<? echo I18N("h", "Please select one file to delete.");?>");
				return;
			}
			para += "&filenames=" + '[' + encodeURIComponent(str) + ']' + "&random=" + Math.random();	 

			if (confirm("<? echo I18N("h", "Are you sure you want to delete");?>"+" "+str)){
				xml_request.json_cgi(para);
			}				
		}
		
		function get_root_info(which_info){
			var my_tree = "<ul class=\"jqueryFileTree\">";
					
			if (which_info.status == "ok" && which_info.errno == null){	
				if (which_info.rootnode.length > 0){
					
					for (var i = 0; i < which_info.rootnode.length; i++){
						var obj = which_info.rootnode[i];
		//alert(obj.nodename[0]+"-"+obj.nodename.length);
						//var dev_name = obj.volname.split('%2F');	// to split usb_dev and usb name
						//alert(obj.volname + "--"+ dev_name);
						//storage_user.put("volid", obj.volname);
						storage_user.put("volid", obj.volid);
						
						var usb_info = new HASH_TABLE();
						
						if (obj.volname == ".."){
							continue;
						}
						for (var j = 0; j < obj.nodename.length; j++){
							var path=obj.nodename[j];
							if(obj.nodename[j] == obj.volid)
								path=obj.volid;
						my_tree += "<li id=\"" + path + "\" class=\"tocollapse\">"																	
								+  "<a href=\"#\" onClick=\"click_folder('" + path + "', '" + obj.volid + "', '"+ obj.mode +"')\">"
								+ obj.nodename[j] + "</a></li>"
								+ "<li></li>"
								+ "<li><span id=\"" + path + "-sub\"></span></li>";
						}
						usb_info.put("volname", obj.volname);
						usb_info.put("volid", obj.volid);
						usb_info.put("status", obj.status);
						usb_info.put("nodename", obj.nodename);
						usb_list.push(usb_info);
					}					
				}
			}else if (which_info.status == "fail"){
				if (which_info.errno == "5002"){
					my_tree += '<li class="warning">' + lang_obj.display('WS009') +'</li>';
				}else if (which_info.error == 5003){
					location.href = "index.php";
				}
			}
								
			my_tree += "</ul>";
									
			return my_tree;
		}
		
		function show_menu_tree(which_info){
			var root_tree = get_by_id("root_tree");
			
			root_tree.innerHTML = get_root_info(which_info);
		}
				
		function get_settings_xml(http_req){
			var my_txt = http_req.responseText;
			var root_info;
			
			try {				
				root_info = JSON.parse(my_txt);
			} catch(e) {
				load_webfile_settings();
				return;
			}
			if(root_info.errno == "5002")
				alert("No HardDrive Connected");
			//if(root_info.rootnode[0].volname.charAt(0)!='/')
			//	root_info.rootnode[0].volname = "/"+root_info.rootnode[0].volname;
			//alert(root_info.rootnode[0].mode);
			storage_user.put("mode", root_info.rootnode[0].mode); //jef
			
			if(root_info.rootnode[0].mode < 2)
			{
				get_by_id("create_btn").disabled = true;
				get_by_id("upload_btn").disabled = true;
				get_by_id("delete_btn").disabled = true;
				get_by_id("button1").disabled = true;
				get_by_id("button2").disabled = true;
			}
			
			show_menu_tree(root_info);			
		}
		
		function load_webfile_settings(){
			var xml_request = new XMLRequest(get_settings_xml);
			var para = "ListRoot?id=" + storage_user.get("id") + "&tok=" + storage_user.get("tok");
				
			xml_request.json_cgi(para);
		}
		
		function get_login_info_result(http_req){
			get_by_id("create_btn").disabled = true;  //jef add
			get_by_id("upload_btn").disabled = true;  //jef add
			get_by_id("delete_btn").disabled = true;  //jef add
			get_by_id("button1").disabled = true;
			get_by_id("button2").disabled = true;
			try //Test could we use the ajax to upload file.
			{
				var upload_ajax_pretest = new FormData();
				document.getElementById('ok3').style.display = "";
				document.getElementById('ok2').style.display = "none";
			}
			catch(e)
			{
				document.getElementById('ok3').style.display = "none";
				document.getElementById('ok2').style.display = "";
			}			
				
			var my_xml = http_req.responseXML;
			
			if (check_user_info(my_xml.getElementsByTagName("redirect_page")[0])){
			
				storage_user.put("id", get_node_value(my_xml, "user_name"));
				storage_user.put("tok", get_node_value(my_xml, "user_pwd"));
				
				load_webfile_settings();								
			}
		}
		
		function get_login_info(){
			var xml_request = new XMLRequest(get_login_info_result);
			var para = "request=get_login_info";
							
			xml_request.exec_webfile_cgi(para);
		}
		
		function refresh_current_path(){
			//close_fancybox();
			setTimeout("close_fancybox()", 2000);
			get_by_id("upload_file").value = "";
			get_by_id("folder_name").value = "";			
			get_sub_folder(current_path, current_volid);
		}
	</script>	
	</head>
	<body onLoad="get_login_info()">		
		<div id="wrapper">
			<div id="header">
				<div align="right">
					<table width="100%" border="0" cellspacing="0">
			<tr>

							<th width="224" rowspan="2" scope="row"><img src="webfile_images/index_01.png" width="220" height="55" /></th>
							<th width="715" height="30" scope="row"><a href="category_view.php" onmouseout="MM_swapImgRestore()" onmouseover="MM_swapImage('Image6','','webfile_images/btn_menu2_.png',1)"><img src="webfile_images/btn_menu2_.png" name="Image6" width="25" height="25" border="0" align="right" id="Image6" /></a></th>
							<th width="15" scope="row"></th>
			</tr>
			<tr>
							<th scope="row"></th>
							<th scope="row"></th>
			</tr>
					</table>

					<a href="#" onmouseout="MM_swapImgRestore()" onmouseover="MM_swapImage('Image6','','webfile_images/btn_home_.png',1)"></a>
				</div>
			</div>
			<div id="folder_top">
			 <div id="left"></div>
				<div id="folder_view_right">
				<table width="670" height="29" border="0" cellspacing="0" cellpadding="0">
			<tr>
					<th width="32" scope="row">&nbsp;</th>

					<td width="638">
					<a id="button1" href="#inline1"><input type="button" id="create_btn" value="<? echo I18N("h", "New Folder");?>" /></a>
					<a id="button2" href="#inline2"><input type="button" id="upload_btn" value="<? echo I18N("h", "Upload");?>" /></a>
					<input type="button" id="delete_btn" value="<? echo I18N("h", "Delete");?>" onclick="delete_file();"/>			
				</td>
			</tr>
				</table>
				</div>
			</div>
			<div id="lower2">

				<div id="left2">
					<div>
						<ul class="jqueryFileTree">
							<li class="toexpanded"><? echo I18N("h", "My Access Device Hard Drive");?></li>
							<li><span id="root_tree"></span></li>
						</ul>
					</div>
				</div>

				<div id="right">
					<table id="rtable" width="670" border="0" cellspacing="0" cellpadding="0"></table>
				</div>
				<div id="footer"><img src="webfile_images/dlink.png" width="77" height="22" /></div>
			</div>
					<div style="display:none;">
						<div id="inline1" style="width:400px;height:120px;overflow:auto;">
							<table width="100%" height="100%" class="uploadtab">
								<thead><tr><th align="left"><? echo I18N("h", "Create Folder");?></th></tr></thead>

								<tr>
									<td>
										<p><? echo I18N("h", "Please enter a folder name");?>:<br /><input type="text" id="folder_name" name="folder_name" size="32"></p>
									</td>
								</tr>
								<tfoot align="right">
								<tr>
									<td>

										<input type="button" id="ok1" name="ok1" value="<? echo I18N("h", "OK");?>" onClick="create_folder()">&nbsp; 
										<input type="button" id="cancel1" onClick="close_fancybox()" value="<? echo I18N("h", "Cancel");?>">
									</td>
								</tr>
								</tfoot>
							</table>
						</div>
					</div>					
					<div style="display: none;">
						<div id="inline2" style="width:400px;height:120px;overflow:auto;">

					<table width="100%" height="100%" class="uploadtab">
							<form id="form1" name="form1" method="post" enctype="multipart/form-data" target="upload_frame">
								<input type="hidden" id="id" name="id">
								<input type="hidden" id="tok" name="tok">
								<input type="hidden" id="volid" name="volid">
								<input type="hidden" id="path" name="path">
								<input type="hidden" id="filename" name="filename">

									<thead><tr><th align="left"><? echo I18N("h", "Upload File");?></th></tr></thead>
									<tr>
										<td>
									<p><? echo I18N("h", "Please select a file");?>:<br /><input type="file" id="upload_file" name="file" size="32"></p>
										</td>
									</tr>
								<tfoot align="right">
									<tr>

										<td>
											<a id="button1" href="#inline_progress"><input type="button" id="ok3" name="ok3" value="<? echo I18N("h", "Upload");?>" onClick="upload_ajax()" /></a>&nbsp;  	
											<input type="button" id="ok2" name="ok2" value="<? echo I18N("h", "OK");?>" onClick="check_upload_file()"> &nbsp; 
											<input type="button" id="cancel2" name="cancel2" onClick="close_fancybox()" value="<? echo I18N("h", "Cancel");?>">
										</td>	
									</tr>
								</tfoot>							
							</form>						
					</table>
						</div>
					</div>	
					<div style="display: none;">
						<div id="inline_progress" style="width:160px;height:40px;overflow:auto;">
							<progress id="progressBar" value=50 max=100 ></progress>
							<div><? echo I18N("h","Upload Progress");?>&nbsp;-&nbsp;<span id="progressPercentage">0%</span></div>
						</div>
					</div>	
					<iframe name="upload_frame" width="0%" height="0%"></iframe>				
		</div>

	</body>
</html>
