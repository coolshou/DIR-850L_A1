#!/bin/sh
iwpriv ra0 set SiteSurvey=1
iwpriv ra0 get_site_survey > /var/ssvy.txt
Parse2DB sitesurvey -f /var/ssvy.txt -d
rm /var/ssvy.txt
exit 0
