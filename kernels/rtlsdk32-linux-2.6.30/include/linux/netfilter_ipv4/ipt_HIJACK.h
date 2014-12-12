#ifndef _IPT_HIJACK_H
#define _IPT_HIJACK_H

#define MAX_URL_LEN		64

struct ipt_hijack_info {
	char url[MAX_URL_LEN];
};

#endif /*_IPT_HIJACK_H*/
