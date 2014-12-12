# youtube - 
# Pattern written by Bouble Hung
# This pattern will match:	
#	1. "Host:" field is *.youtube.com.*
#	2. "Referer:" field is *.youtube.com.*
#	3. Sometims, "Host:" field isn't url, it will be youtube's related IP(xx.xx.xx.xx),
#		then will check http "GET /youtube-"
#

youtube
^GET[\x09-\x0d ](/youtube-|.*(referer|host): .*youtube.com)


