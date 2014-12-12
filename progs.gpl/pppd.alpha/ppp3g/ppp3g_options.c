/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "pppd.h"

int  ppp3g_debug = 0;			/* debuggin mode. */
char *ppp3g_chat_file;			/* chat file name */
char *ppp3g_dc_chat_file;
char speed_str[16];				/* Serial port speed as string */
extern struct stat devstat;			/* result of stat() on devnam */

static int setdevname (char *, char **, int);
static int setspeed (char *, char **, int);

static option_t ppp3g_options[] =
{
	{ "device name", o_wild, (void *) &setdevname,
      "Serial port device name",
      OPT_DEVNAM | OPT_PRIVFIX | OPT_NOARG  | OPT_A2STRVAL | OPT_STATIC, devnam},
      
	{ "ppp3g_chat", o_string, &ppp3g_chat_file,
	  "PPP3g chat script file",
	  OPT_PRIO | OPT_PRIVFIX },
	  
	{ "ppp3g_chat_dc", o_string, &ppp3g_dc_chat_file,
	  "PPP3g disconnect chat script file",
	  OPT_PRIO | OPT_PRIVFIX },
	  
	{ "ppp3g_debug", o_int, &ppp3g_debug,
	  "PPP3G debugging",
	  OPT_INC | OPT_NOARG | 1 },

    { "tty speed", o_wild, (void *) &setspeed,
      "Baud rate for serial port",
      OPT_PRIO | OPT_NOARG | OPT_A2STRVAL | OPT_STATIC, speed_str },

	{ NULL }
};

void module_ppp3g_init(void)
{
	add_options(ppp3g_options);
}


/*
 * setspeed - Set the serial port baud rate.
 * If doit is 0, the call is to check whether this option is
 * potentially a speed value.
 */
static int setspeed( char *arg, char **argv, int doit)
{
	char *ptr;
	int spd;

	spd = strtol(arg, &ptr, 0);
	if (ptr == arg || *ptr != 0 || spd == 0)
		return 0;
	if (doit) 
	{
		inspeed = spd;
		slprintf(speed_str, sizeof(speed_str), "%d", spd);
	}
	return 1;
}

/*
 * setdevname - Set the device name.
 * If doit is 0, the call is to check whether this option is
 * potentially a device name.
 */
static int setdevname( char *cp, char **argv, int doit)
{
	struct stat statbuf;
	char dev[MAXPATHLEN];

	if (*cp == 0)
		return 0;

	if (strncmp("/dev/", cp, 5) != 0) {
		strlcpy(dev, "/dev/", sizeof(dev));
		strlcat(dev, cp, sizeof(dev));
		cp = dev;
	}

	/*
	 * Check if there is a character device by this name.
	 */
	if (stat(cp, &statbuf) < 0) 
	{
		if (!doit)
			return errno != ENOENT;
		option_error("Couldn't stat %s: %m", cp);
		return 0;
	}
	if (!S_ISCHR(statbuf.st_mode)) 
	{
		if (doit)
			option_error("%s is not a character device", cp);
		return 0;
	}

	if (doit) 
	{
		strlcpy(devnam, cp, sizeof(devnam));
		devstat = statbuf;
		default_device = 0;
	}
  
	return 1;
}
