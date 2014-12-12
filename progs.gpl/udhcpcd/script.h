#ifndef _SCRIPT_H
#define _SCRIPT_H

void run_script(struct dhcpMessage *packet, const char *name);
#ifdef CLASSLESS_STATIC_ROUTE_OPTION
void run_script_opt121(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain);
#endif
#ifdef MSCLSLESS_STATIC_ROUTE_OPTION
void run_script_opt249(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain);
#endif
#ifdef STATIC_ROUTE_OPTION
void run_script_opt33(struct dhcpMessage *packet, unsigned int *index, unsigned int *remain);
#endif
#endif
