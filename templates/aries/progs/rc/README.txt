RC scripts in Aries.
Presented by David Hsieh <david_hsieh@alphanetworks.com>

o	There are 2 directories for RC scripts. One is /etc/init.d and the other
	is /etc/init0.d. The scripts in /etc/init.d will be executed once after
	the system is boot up and than /etc/init0.d. The RC scripts will have the
	file name like this
	
		SXXxxx.sh

	It has a leading upper case 'S' and follows by 2 decimal digit.
	The scripts in these directories will be executed by order.
	The first digit in the file name decides the priority.

	1 - Reserved for some special case.
	2 - Hardware component / Basic module initialization.
	3 - Reserved for some special case.
	4 - System startup
	5 - Reserved for some special case.
	6 - Application startup
	7 - Reserved for some special case.
	8 - Brand/Model dependents startup.

	The second digits is used to adjust the execution order. The designer of
	the template should well adjust the order.

o	/etc/init.d

	The scripts here will only be executed once after the system boot up.
	There are 2 digits in the file name which have some special meanings.

	Because the scripts here are the first part of the execution, so the major
	job here is the very first initialize, such like mounting the procfs,
	RAM disk and probably some hardware initialization and driver module
	installation. The most important thing here is, the jobs here should not
	expect some other module to be working already, which means the jobs
	should not depends on other modules, they should be independent modules.
	For example: most of the time, the network is not ready yet. If you try to
	synchronous the system time with NTP server here, you will always fail,
	because the network is not setup yet.

o	/etc/init0.d

	The scripts here will be used to start and stop the system. The script
	file here will have 1 parameter and the possible value is 'start' or 'stop'
	which indicate the script is to start or stop something.

o	My implementation

	. init.d/S10init.sh (from board)		- mounting procfs & ramfs
	. init.d/S19init.sh (from aries)		- make directories
	. init.d/S20interfaces.sh (from board)	- config the vlan of switch and
											  create bridge interface.
	. init.d/S20init.sh (from template)		- load xmldb & servd

