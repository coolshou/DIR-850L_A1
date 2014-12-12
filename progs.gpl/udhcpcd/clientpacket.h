#ifndef _CLIENTPACKET_H
#define _CLIENTPACKET_H
/* 32 bit change to 64 bit dennis 20080311 start */
uint32_t random_xid(void);
int send_discover(uint32_t xid, uint32_t requested);
int send_selecting(uint32_t xid, uint32_t server, uint32_t requested);
int send_renew(uint32_t xid, uint32_t server, uint32_t ciaddr);
int send_release(uint32_t server, uint32_t ciaddr);
int get_raw_packet(struct dhcpMessage *payload, int fd);
int send_decline(uint32_t xid, uint32_t server, uint32_t ciaddr); /*Erick DHCP Decline 20110415*/
/* 32 bit change to 64 bit dennis 20080311 end */

#endif
