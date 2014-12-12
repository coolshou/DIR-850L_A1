<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/xnode.php";

movc($SETCFG_prefix."/bwc/", "/bwc/");

/* because bwc use xml link, and movc() will destroy link attribute, so re-set link. */
/* For BWC, BWC-2's flag and enable will use BWC-1's config */
setattr("/bwc/entry:2/flag" ,	"link","/bwc/entry:1/flag");
setattr("/bwc/entry:2/enable" ,	"link","/bwc/entry:1/enable");
setattr("/bwc/entry:2/rules" ,	"link","/bwc/entry:1/rules");

?>
