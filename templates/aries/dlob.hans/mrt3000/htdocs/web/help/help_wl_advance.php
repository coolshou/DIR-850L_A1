HTTP/1.1 200 OK
Content-Type: Text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
<h2><?echo I18N("h","Advanced Wireless");?></h2>
<p><?echo I18N("h","The options on this page should be changed by advanced users or if you are instructed to by one of our support personnel, as they can negatively affect the performance of your router if configured improperly.");?>
<dl>
<dt><?echo I18N("","<strong>Transmit Power</strong>");?></dt>
<dd><?echo I18N("h","You can lower the output power of the router by selecting lower percentage Transmit Power values from the drop down. Your choices are: High,  Medium and Low.");?></dd>
<dt><?echo I18N("","<strong>Beacon Interval</strong>");?></dt>
<dd><?echo I18N("h","Beacons are packets sent by an Access Point to synchronize a wireless network. Specify a Beacon interval value between 20 and 1000. The default value is set to 100 milliseconds.");?></dd>
<dt><?echo I18N("","<strong>RTS Threshold</strong>");?></dt>
<dd><?echo I18N("h"," This value should remain at its default setting of 2346. If you encounter inconsistent data flow, only minor modifications to the value range between 256 and 2346 are recommended. The default value for RTS Threshold is set to 2346.");?></dd>
<dt><?echo I18N("","<strong>Fragmentation</strong>");?></dt>
<dd><?echo I18N("h","This value should remain at its default setting of 2346. If you experience a high packet error rate, you may slightly increase your \"Fragmentation\" value within the value range of 1500 to 2346. Setting the Fragmentation value too low may result in poor performance.");?></dd>
<dt><?echo I18N("","<strong>DTIM Interval</strong>");?></dt>
<dd><?echo I18N("h","Enter a value between 1 and 255 for the Delivery Traffic Indication Message (DTIM). A DTIM is a countdown informing clients of the next window for listening to broadcast and multicast messages. When the Access Point has buffered broadcast or multicast messages for associated clients, it sends the next DTIM with a DTIM Interval value. AP clients hear the beacons and awaken to receive the broadcast and multicast messages. The default value for DTIM interval is set to 1.");?></dd>
<dt><?echo I18N("","<strong>Preamble Type</strong>");?></dt>
<dd><?echo I18N("h","The Preamble Type defines the length of the CRC (Cyclic Redundancy Check) block for communication between the Access Point and roaming wireless adapters. Make sure to select the appropriate preamble type.");?></dd>
<dd><?echo I18N("h","Note: High network traffic areas should use the shorter preamble type. CRC is a common technique for detecting data transmission errors.");?></dd>

<dt><strong><?echo i18n("WLAN Partition");?></strong></dt>
<dd><?echo i18n("WLAN Partition allows you to segment your Wireless network by managing access to both the internal station and Ethernet access to your WLAN.");?></dd>		
<dt><strong><?echo i18n("WMM Enable");?></strong></dt>
<dd><?echo i18n("Enabling WMM can help control latency and jitter when transmitting multimedia content over a wireless connection.");?></dd>

<dt><?echo I18N("h","Short Guard Interval");?></dt>
<dd><?echo I18N("h","Using a short guard interval can increase throughput. However, it can also increase error rate in some installations, due to increased sensitivity to radio-frequency reflections. Select the option that works best for your installation.	");?></dd>
<dt><strong><?echo i18n("HT 20/40 Coexistence");?></strong></dt>
<dd><?echo i18n("A mode of operation in which two channels, or paths on which data can travel, are combined to increase performance in some environments.");?></dd>	
</dl>
</body>
</html>
