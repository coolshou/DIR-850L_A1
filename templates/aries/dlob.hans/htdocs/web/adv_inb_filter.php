HTTP/1.1 200 OK

<?
/* The variables are used in js and body both, so define them here. */
$INBF_MAX_COUNT = query("/acl/urlctrl/max");
if ($INBF_MAX_COUNT == "") $INBF_MAX_COUNT = 24;

/* necessary and basic definition */
$TEMP_MYNAME    = "adv_inb_filter";
$TEMP_MYGROUP   = "advanced";
$TEMP_STYLE		= "complex";
include "/htdocs/webinc/templates.php";
?>
