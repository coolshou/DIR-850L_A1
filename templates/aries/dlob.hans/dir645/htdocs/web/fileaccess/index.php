HTTP/1.1 200 OK

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html;charset=UTF-8">
	<title>D-Link WEB File Access Server</title>
	<link rel="stylesheet" type="text/css" href="css/style.css">
	<script type="text/javascript" src="javascript/libajax.js"></script>
	
	<!-- Play Photo - By lightwindow. -->
	<script type="text/javascript" src="javascript/prototype.js"></script>
	<script type="text/javascript" src="javascript/scriptaculous.js?load=effects"></script>
	<script type="text/javascript" src="javascript/lightwindow.js"></script>
	<link rel="stylesheet" href="css/lightwindow.css" type="text/css" media="screen" />
	
	<script type="text/javascript" src="javascript/webtoolkit.aim.js"></script>
</head>
<body>	
	<div id="continer">
		<div id="header">
			<div class="logo">D-Link Web File Access Server</div>
		</div>
		<div id="box_top">
			<!--<div id="loading_screen" style="display:none;">loading...</div>-->
			<div class="bg_top"> </div>
			<div id="screen" style="display:none;">
				<div class="top_menu">
					<!--<a href="javascript:NotWork();">Download</a>-->
					<a href="javascript:API.preAddFolder();">New Folder</a>
					<a href="javascript:API.preUpload();">Upload File</a>
					<a href="javascript:API.Reload();">Reload</a>
					<a href="javascript:API.Logout();">Logout</a>
				</div>
				<div class="top_option">
					<a href="javascript:UserType(1,true);"><img src="images/view_list_text.png" /></a>
					<a href="javascript:UserType(2,true);"><img src="images/view_list_icon.png" /></a>
				</div>
				<div class="pathlabel">Path:<span id="currentpath"></span>
					<span id="loading_img" class="loading_img">Loading...</span>
					</div>
				<div class="dash"></div>
				<!-- <<<<<< Block Start >>>>>> -->
				<div id="blockcontainer" class="screen block" style="display:none;">
				</div>
				<!-- <<<<<< Detail Start >>>>>> -->
				<div id="detailcontainer" class="screen line" style="display:none;">
				</div>
			</div>
			
			<div id="login_screen" class="other_input">
				<div class="text_input">
					<span class="name">User Name:</span>
					<span class="value">
<?
	$cnt = query("/webaccess/account/entry#");
	if ($cnt > 1)
	{
		echo '\t\t\t\t\t\t<select id="loginusr">\n';
		foreach ("/webaccess/account/entry")
		{
			$act = query("username");
			if($act=="Admin"||$act=="Guest")	continue;
			echo '\t\t\t\t\t\t\t<option value="'.$act.'"';
			if ($InDeX==1) echo ' selected';
			echo '>'.$act.'</option>\n';
		}
		echo '\t\t\t\t\t\t</select>\n';
	}
?>	
					</span>
				</div>
				<div class="text_input">
					<span class="name">Password:</span>
					<span class="value"><input type="password" id="loginpwd" disabled="true" value="xxxxxx" size=10 /></span>
				</div>
				<div class="text_input">
					<span class="name">&nbsp;</span>
					<span class="value"><input type="button" value="  Login  " onclick="javascript:Login.Post();" /></span>
				</div>
			</div>
			
			<div id="adddir_screen" class="other_input" style="display:none;">
				<div class="text_input"><span class="all_line" id="adddir_path"></span></div>
				<div class="text_input">
					<span class="name">Create Folder Name:</span>
					<span class="value"><input type="text" id="addfolder" size=20 maxlength=65 /></span>
				</div>
				<div class="text_input">
					<span class="name">&nbsp;</span>
					<span class="value">
						<input type="button" value="  Submit  " onclick="javascript:API.AddFolder();" />
						<input type="button" value="  Cancel  " onclick="javascript:API.CancelAddFolder();" />
					</span>
				</div>
			</div>

<form id="upload" action="/ws/api/UploadFile" method="post" enctype="multipart/form-data" onsubmit="return API.Upload(this,{'onStart': startCallback, 'onComplete': completeCallback, 'responseType': 'json'});">
			<div id="upload_screen" class="other_input" style="display:none;">
				<div class="text_input"><span class="all_line" id="span_upload_path"></span></div>
				<input type="hidden" name="id" id="upload_id" value="" />
				<input type="hidden" name="volid" id="upload_volid" value="" />
				<input type="hidden" name="tok" id="upload_tok" value="" />
				<div id="upload_path"></div>
				<div class="text_input">
					<span class="name">Upload File:</span>
					<span class="value"><input type="file" id="upload_file" name="filename" size=40 /></span>
				</div>
				<div class="text_input">
					<span class="name">&nbsp;</span>
					<span class="value">
						<input type="submit" value="  Upload  " />
						<input type="button" value="  Cancel  " onclick="javascript:API.CancelUpload();" />
					</span>
				</div>
			</div>
</form>

			<div id="alertmsg_screen" class="alertmsg" style="display:none;">
				<center>
				<div id="text_msg"></div>
				<input type="button" value="  Retry  " onclick="javascript:API.ErrorMsg2Reload();" />&nbsp;
				<input type="button" value="  Logout  " onclick="javascript:API.Logout();" />
				</center>
			</div>

			<div class="bg_bottom"> </div>
		</div>
	</div>
	<div>
	<div id="footer" align="center">Power By Alphanetworks.</div>
</body>
<script type="text/javascript">
<?
	
	$count = query("/webaccess/account/entry#");
	$id = query("/webaccess/account/entry:".$count."/username");
	$volid = query("/webaccess/device/entry/entry/label");//query("/webaccess/account/entry:".$count."/entry:1/path");
	$tok = "aaa:444";
	$url_path = "?id=".$id."&volid=".$volid."&tok=".$tok."&path=";
	
	$ary_string = "";
	echo "var pary = new Array();\n";
	foreach ("/webaccess/account/entry")
	{
		$name = query("username");
		$path = query("entry/path");
		if($name=="Admin" || $name=="Guest")	continue;
		$ary_string = $ary_string.",['".$name."','".$path."']";
	}
	echo "pary = [[0,null]".$ary_string."];\n";
	
	include "/htdocs/web/fileaccess/javascript/initial.js";
	include "/htdocs/web/fileaccess/javascript/_login.js";
	include "/htdocs/web/fileaccess/javascript/api.js";
?>

</script>

</html>
