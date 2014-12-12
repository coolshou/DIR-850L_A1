HTTP/1.1 200 OK 
Content-Type: text/xml; charset=utf-8 
Content-Length: <Number of Bytes/Octets in the Body> 
<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/mydlink/libservice.php";
include "/htdocs/phplib/lang.php";

function read_result()
{
	$result = "fail";
	$return = fread("", "/var/tmp/mydlink_result");
	
	if(isempty($return) == 1)
		return "Result of mydlink registration is not exist or NULL";	
		
	$cnt = cut_count($return, "\r\n");
	$t = $cnt -1;
	
	$tmp = cut($return , $t, "\r\n");
	
	$L1=strlen($tmp);
	$L2=strstr($tmp, "\n") + strlen("\n");
	
	$result_str = substr($tmp, $L2, $L1-$L2);
	
	$temp = strstr($result_str, "success");
	
	if(isempty($temp) == 1)
		$result = $result_str;	
	else
		$result = "OK";
	return $result;
}

function read_cookie()
{
	$return = fread("", "/var/tmp/mydlink_result");
	
	if(isempty($return) == 1)
		return "NULL";	
	$L1=strlen($return);
	$L2=strstr($return, "Set-Cookie: mydlink=") + strlen("Set-Cookie: ");
	
	$temp_str = substr($return, $L2, $L1-$L2);
	
	$L1=strchr($temp_str, ";");
	
	$cookie = substr($temp_str, 0, $L1);

	if(isempty($cookie) == 1)
		$result = "";	
	else
		$result = $cookie;
	
	return $result;
}

function get_value_from_mydlink($value_name)
{
	$name_size = strlen($value_name)+1;  //and =
	$buf = fread("", "/tmp/provision.conf");
	if(isempty(buf) == 1)  //conf not exist
		return "NULL";	
	$buf_len = strlen($buf);
	$target_ptr = strstr($buf, $value_name);
	$substr = substr($buf, $target_ptr, $buf_len - $target_ptr);
	
	$end_ptr =strchr($substr, "\n");
	
	$substr = substr($substr, $name_size, $end_ptr - $name_size);
	
	return $substr;
}

function do_post($post_str, $post_url, $withcookie)
{
	$head_file = "/var/tmp/mydlink_head";
	$body_file = "/var/tmp/mydlink_body";
	
	$f_url = get_value_from_mydlink("portal_url");
	
	if($f_url == "NULL")
		return "provision is not exist";
		
	$http_ptr = strstr($f_url, "http://");
	$https_ptr = strstr($f_url, "https://");
		
	if(isempty($http_ptr) == 0)
		$head = "http://";
	else if(isempty($https_ptr) == 0)
		$head = "https://";
	else
		$head = "";
	$str_len = $http_ptr + strlen($head);
	$url = substr($f_url, $str_len, strlen($f_url) - $str_len);
	$slash_pt = strchr($url, "/");
	if(isempty($slash_pt) == 0)
		$url = substr($url, 0, $slash_pt);	
	$f_url = $head.$url;
	$_GLOBALS["URL_MYDLINK"] = $f_url;
		
	
	$len = strlen($post_str)-1;
	
	$req_head = "POST ". $post_url. " HTTP/1.1\r\n\Accept: text/html, */*\r\n\Accept-Language: en\r\n".
				"x-requested-with: XMLHttpRequest\r\n\Content-Type: application/x-www-form-urlencoded\r\n".
				"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n".
				"Host: ".$url."\r\n\Content-Length: ".$len."\r\n\Connection: Keep-Alive\r\n".
				"Cache-Control: no-cache".$withcookie."\r\n\r\n";
	
	fwrite(w, $head_file, $req_head);
	fwrite(w, $body_file, $post_str);
	
	$url = "urlget post ".$f_url.$post_url. " ". $body_file. " ". $head_file;
	setattr("/runtime/register", "get", $url." > /var/tmp/mydlink_result");
	get("x", "/runtime/register");
	del("/runtime/register");

	unlink($head_file);
	unlink($body_file);
	return 0;
}

function do_set_status()
{
	/*the register is success,keep this email*/
	$status = query("/mydlink/register_st");
	set("/mydlink/register_st", "1");
	set("/mydlink/regemail", $email);
	/*status change ,restart related service.*/
	if($status != "1")
	{
		runservice("MYDLINK.LOG restart");
	}
	event("DBSAVE");
}


function language()
{
   	$lang_code = wiz_set_LANGPACK();
       if($lang_code=="en") $lang="en";
       else if($lang_code=="fr") $lang="fr";
       else if($lang_code=="ru") $lang="ru";
       else if($lang_code=="es") $lang="es";
       else if($lang_code=="pt") $lang="pt";
       else if($lang_code=="jp") $lang="ja";
       else if($lang_code=="zhtw") $lang="zh_TW";
       else if($lang_code=="zhcn") $lang="zh_CN";
       else if($lang_code=="ko") $lang="ko";
       else if($lang_code=="cs") $lang="cs";
       else if($lang_code=="da") $lang="da";
       else if($lang_code=="de") $lang="de";
       else if($lang_code=="el") $lang="el";
       else if($lang_code=="hr") $lang="hr";
       else if($lang_code=="hr") $lang="hr";
       else if($lang_code=="it") $lang="it";
       else if($lang_code=="nl") $lang="nl";
       else if($lang_code=="no") $lang="no";
       else if($lang_code=="pl") $lang="pl";
       else if($lang_code=="ro") $lang="ro";
       else if($lang_code=="sl") $lang="sl";
       else if($lang_code=="sv") $lang="sv";
       else if($lang_code=="fi") $lang="fi";
       else $lang = "en";
       echo $lang;
}

//init local parameter
$fwver = query("/runtime/device/firmwareversion");
$modelname = query("/runtime/device/modelname");
$devpasswd = query("/device/account/entry/password");
$devusername = query("/device/account/entry/name");
$wizard_version = $modelname. "_". $fwver;
$lang = language();
$result = "OK";

$mydlink_num = get_value_from_mydlink("username");
$dlinkfootprint = get_value_from_mydlink("footprint");

$nodebase="/runtime/hnap/SetMyDLinkSettings/";
$enabled =  query($nodebase."Username");
$email = query($nodebase."Email");
$password = query($nodebase."Password");
$lastname = query($nodebase."LastName");
$firstname = query($nodebase."FirstName");
$deviceusername = query($nodebase."DeviceUserName");
$devicepassword = query($nodebase."DevicePassword");

if ($deviceusername != $devusername  || $devpasswd != $devicepassword)
{
	$result = "ERROR";
}

//login
$post_str_signin = "client=wizard&wizard_version=" .$wizard_version. "&lang=" .$lang.
            "&email=" .$email. "&password=" .$password." ";

$post_url_signin = "/account/?signin";

$action_signin = "signin";

//add dev (bind device)
$post_str_adddev = "client=wizard&wizard_version=" .$wizard_version. "&lang=" .$lang.
            "&dlife_no=" .$mydlink_num. "&device_password=" .$devicepassword. "&dfp=" .$dlinkfootprint." ";

$post_url_adddev = "/account/?add";

$action_adddev = "adddev";


fwrite("w",$ShellPath, "#!/bin/sh\n");
fwrite("a",$ShellPath, "echo \"[$0]-->MyDlinkSettings Change\" > /dev/console\n");

//main start
$post_str = $post_str_signin;
$post_url = $post_url_signin;
$withcookie = "\r\nCookie: lang=en; mydlink=pr2c11jl60i21v9t5go2fvcve2;";

if($result == "OK")
{
	$ret = do_post($post_str, $post_url, $withcookie);
	if($ret == 0)
		$result = read_result();
	else
		$result = "ERROR";
	
	if($result == "OK")
	{
		$cookie = read_cookie();
		if($cookie == "NULL")
			$withcookie = "";
		else
			$withcookie = "\r\nCookie: lang=en;".$cookie;
		
		if($mydlink_num == "NULL" || $dlinkfootprint == "NULL")
			$result = "dlink number or foorprint not exist";
		else
		{

			$ret = do_post($post_str_adddev, $post_url_adddev, $withcookie);
			if($ret == 0)
			{
				$result = read_result();

				$add_success_ret = $mydlink_num.":";  //compare dlink_num only
				$ret = strstr($result, $add_success_ret);
				if(isempty($ret) == 0)
				{
					$status = query("/mydlink/register_st");
					set("/mydlink/register_st", "1");
					set("/mydlink/regemail", $email);
					fwrite("a",$ShellPath, "/etc/scripts/dbsave.sh > /dev/console\n");
					if($status != "1")
					{
						fwrite("a",$ShellPath, "service MYDLINK.LOG restart > /dev/console\n");
					}
					fwrite("a",$ShellPath, "xmldbc -s /runtime/hnap/dev_status '' > /dev/console\n");
					set("/runtime/hnap/dev_status", "ERROR");
				}
			}
			else
				$result = "ERROR";
		}
	}
}

if($result != "OK")
{
	$result != "ERROR";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"> 
<soap:Body> 
<SetMyDlinkSettingsResponse xmlns="http://purenetworks.com/HNAP1/"> 
<SetMyDlinkSettingsResult><?=$result?></SetMyDLinkSettingsResult> 
</SetMyDlinkSettingsResponse> 
</soap:Body> 
</soap:Envelope> 
