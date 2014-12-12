
o Remember to record the variables in this file. It will be helpful.

o We use the following variables to defines some features we used or not,
	and they are different from the default of brand.

	$FEATURE_NOAPMODE = 1;			/* if this model doesn't support AP mode, set it as 1. */
	$FEATURE_NOAPP = 1;			/* if this model doesn't support application rules, set it as 1. */
	$FEATURE_NOQOS = 1;			/* if this model doesn't support QoS, set it as 1. */
	$FEATURE_NORT = 1;			/* if this model doesn't support routing, set it as 1. */
	$FEATURE_NOSCH = 1;			/* if this model doesn't support scheudle, set it as 1. */

	$FEATURE_NOPPTP = 1;		/* if this model doesn't support PPTP, set it as 1. */
	$FEATURE_NOL2TP = 1;		/* if this model doesn't support L2TP, set it as 1.*/
	$FEATURE_NORUSSIAPPTP = 1;	/* if this model doesn't support Russia PPTP, set it as 1.*/
	$FEATURE_NORUSSIAPPPOE = 1;	/* if this model doesn't support Russia PPPoE, set it as 1. */

	$FEATURE_DHCPPLUS = 1;		/* if this model supports DHCP+, set it as 1. */

	$FEATURE_NOEASYSETUP = 1;	/* for D-Link model, if this model has no easy setup page, set it as 1. */
	
	$FEATURE_NOLANGPACK = 1;	/* if this model doesn't support language pack upgrade, set it as 1. */

o These variable will be defined in the elbox/progs.brand/[model]/htdocs/webinc/feature.php.
  ex:
  	Model is dir412,
  	and the location will be elbox/progs.brand/dir412/htdocs/webinc/fearture.php.

o DLOB features
	<device>
		<feature>
			<esaysetup>
				<enable type="BOOLEAN" /><!-- use to control always start up with easy setup page. -->
				<defwantype type="BOOLEAN" />
				<!-- The default WAN type for easy setup.
					It will	be used when the settings are factory default value.
					The value of this node may be STATIC DHCP PPPoE PPTP L2TP.
				-->
			</easysetup>
		</feature>
	</device>
