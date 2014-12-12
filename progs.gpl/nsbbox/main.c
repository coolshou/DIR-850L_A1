/* vi: set sw=4 ts=4: */
/*
 *	main.c
 *	The main function for nsbbox.
 *	BusyBox has almost everything we need in the linux world,
 *	but there are still something missing. Instead of modifying
 *	BusyBox, I gather these missing tools and put them into
 *	'Not So Busy Box - nsbbox'.
 *
 */

#include <stdio.h>
#include <string.h>
#include <elbox_config.h>

#ifdef CONFIG_NSBBOX_BRCTL
extern int brctl_main(int argc, char * argv[]);
#endif
#ifdef CONFIG_NSBBOX_NTPCLIENT
extern int ntpclient_main(int argc, char * argv[]);
#endif
#ifdef CONFIG_NSBBOX_BASE64
extern int base64_main(int argc, char * argv[]);
#endif
#ifdef CONFIG_NSBBOX_SMTPCLIENT
extern int smtpclient_main(int argc, char * argv[]);
#endif
#ifdef CONFIG_NSBBOX_VCONFIG
extern int vconfig_main(int argc, char* argv[]);
#endif

int main(int argc, char * argv[], char * env[])
{
    char * base;
	int ret = 1;

	base = strrchr(argv[0], '/');
	if (base) base = base + 1;
	else base = argv[0];

#ifdef CONFIG_NSBBOX_BRCTL
	if (strcmp(base, "brctl")==0)       ret = brctl_main(argc, argv); else
#endif
#ifdef CONFIG_NSBBOX_NTPCLIENT
	if (strcmp(base, "ntpclient")==0)   ret = ntpclient_main(argc, argv); else
#endif
#ifdef CONFIG_NSBBOX_BASE64
	if (strcmp(base, "b64enc")==0)      ret = base64_main(argc, argv); else
	if (strcmp(base, "b64dec")==0)      ret = base64_main(argc, argv); else
#endif
#ifdef CONFIG_NSBBOX_SMTPCLIENT
	if (strcmp(base, "smtpclient")==0)  ret = smtpclient_main(argc, argv); else
#endif
#ifdef CONFIG_NSBBOX_VCONFIG
	if (strcmp(base, "vconfig")==0)     ret = vconfig_main(argc, argv); else
#endif
	printf("nsbbox, Not So Busy Box !!\n");

	return ret;
}
