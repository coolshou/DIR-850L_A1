/* Shared library add-on to iptables to add HTTP hijack support.
 *
 * (C) 2009 Kwest Wan <kwest_wan@cn.alphanetworks.com>
 */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
//#include <iptables.h>
#include <xtables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_HIJACK.h>

#define DEFAULT_URL		"192.168.0.1/forbidden.php"

#define IPT_URL_USED	0x01

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"HIJACK v%s options:\n"
" --to-url <a URL without 'http://' prefix>\n"
"				hijack and redirect to URL.\n\n",
//IPTABLES_VERSION);
"1.4");
}

static struct option opts[] = {
	{ "to-url", 1, 0, '1' },
	{ 0 }
};

/* Initialize the target. */
static void
//init(struct ipt_entry_target *t, unsigned int *nfcache)
init(struct xt_entry_target *t)
{
	struct ipt_hijack_info *hijack = (struct ipt_hijack_info *)t->data;

	/* default */
	strncpy(hijack->url, DEFAULT_URL, MAX_URL_LEN);
}
	
/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      //const struct ipt_entry *entry,
      const void *e,
      struct xt_entry_target **target)
{
	struct ipt_hijack_info *hijack = (struct ipt_hijack_info *)(*target)->data;
	const struct ipt_entry *entry = e;
	int tcpok;

	if (*flags & IPT_URL_USED)
		xtables_error(PARAMETER_PROBLEM,
				"Can't specify multiple --to-url");

	if (entry->ip.proto == IPPROTO_TCP)
		tcpok = 1;
	else
		tcpok = 0;

	switch (c) {
	case '1':
		if (!tcpok)
			xtables_error(PARAMETER_PROBLEM,
				   "Need TCP with HTTP URL specification");

		if (xtables_check_inverse(optarg, &invert, NULL, 0, argv))
			xtables_error(PARAMETER_PROBLEM,
				   "Unexpected `!' after --to-url");

		if (strncmp(optarg, "https:", 6) == 0)
			xtables_error(PARAMETER_PROBLEM,
				  "HIJACK target can't support HTTPS");
		
		if (strncmp(optarg, "http:", 5) == 0)
			xtables_error(PARAMETER_PROBLEM,
				  "Don't put 'http://' as prefix, just specific a valid URL");

		if (strlen(optarg)+1 > MAX_URL_LEN)
			xtables_error(PARAMETER_PROBLEM,
				   "URL length is too long, it only allow %d max", MAX_URL_LEN);
		
		strncpy(hijack->url, optarg, MAX_URL_LEN);
		*flags |= IPT_URL_USED;
		
		return 1;

	default:
		return 0;
	}
}

/* Final check; don't care. */
static void final_check(unsigned int flags)
{
}

/* Prints out the targinfo. */
static void
//print(const struct ipt_ip *ip,
//      const struct xt_entry_target *target,
print(const void *ip,
      const struct xt_entry_target *target,

      int numeric)
{
	struct ipt_hijack_info *hijack = (struct ipt_hijack_info *)target->data;

	printf(" hijack to http://%s", hijack->url);
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void
//save(const struct ipt_ip *ip, const struct xt_entry_target *target)
save(const void *ip, const struct xt_entry_target *target)
{
	struct ipt_hijack_info *hijack = (struct ipt_hijack_info *)target->data;

	printf("--to-url %s", hijack->url);
}

//static struct iptables_target hijack = { 
static struct xtables_target hijack = { 
	//.next		= NULL,
	.name		= "HIJACK",
	//.version	= IPTABLES_VERSION,
	.version	= XTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct ipt_hijack_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct ipt_hijack_info)),
	.help		= &help,
	//.init		= &init,
 	//.parse		= &parse,
 	.init			= init,
 	.parse		= parse,

	.final_check	= final_check,
	.print		= print,
	.save		= save,
	.extra_opts	= opts
};

void _init(void)
{
	xtables_register_target(&hijack);
}
