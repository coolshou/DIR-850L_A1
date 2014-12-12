/*
#######################################################################
#
# (C) Copyright 2001
# Alex Zeffertt, Cambridge Broadband Ltd, ajz@cambridgebroadband.com
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#######################################################################
# Notes:
# 
# This configuration utility communicates with macvlan.o, the MAC address
# based VLAN support module.
#
# It uses an IOCTL interface which allows you to
#
# 1. enable/disable MAC address based VLANS over an ether type net_device
# 2. add/remove a MAC address based VLAN - which is an ether type net_device
#    layered over the original MACVLAN enabled ether type net_device.
# 3. bind/unbind MAC addresses to/from particular MAC address based VLANs
# 4. discover the state of MAC address based VLANs on the system.
# 5. set/get port flags, including whether to bind to destination MAC
#    or source mac.
# 6. Traffic to/from eth0 will not be affected.


#
# Example: (Assuming you are using source binding)
#
# If you enable MAC address based VLANS over eth0
#
# You may then create further VLANs, e.g. eth0#1 eth0#2 ....
# These will not receive any frames until you bind MAC addresses to them.
# If you bind 11:22:33:44:55:66 to eth0#1, then any frames received by
# eth0 with source MAC 11:22:33:44:55:66 will be routed up through eth0#1
# instead of eth0.
#
# Example: (Assuming you are using destination (local) binding)
#
# If you enable MAC address based VLANS over eth0
#
# You may then create further VLANs, e.g. eth0#1 eth0#2 ....
# These will not receive any frames until you bind MAC addresses to them.
# If you bind 11:22:33:44:55:66 to eth0#1, then any broadcast/multicast
# frames, or frames with a destination MAC 11:22:33:44:55:66
# will be routed up through eth0#1 instead of eth0
#
# For broadcasts, the packet will be duplicated for every VLAN
# with at least one MAC attached.  Attaching more than one MAC
# when destination binding makes no sense...don't do it!
#
#
# 
#######################################################################
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_macvlan.h>
#include <linux/sockios.h>
#include <string.h>
#include <errno.h>


int do_help(int argc, char *argv[]);
int do_enable(int argc, char *argv[]);
int do_disable(int argc, char *argv[]);
int do_add(int argc, char *argv[]);
int do_del(int argc, char *argv[]);
int do_bind(int argc, char *argv[]);
int do_unbind(int argc, char *argv[]);
int do_info(int argc, char *argv[]);
int do_setflags(int argc, char *argv[]);
int do_unload(int argc, char* argv[]);

struct command {
    char *name;
    char *short_help;
    int (*fn)(int argc, char *argv[]);
    char *long_help;
} command_list[] = {
    {"help",   "help on other commands", do_help, "help <command>"},
    {"enable", "enables mac based vlans over an ethernet device", do_enable,
         "enable <ifname>\n"
         " - enables mac based vlans over \"ifname\"\n"
         " - also creates a default vlan over \"ifname\" called \"ifname#0\""
    },
    {"disable", "disables mac based vlans over an ethernet device", do_disable, "disable <ifname>"},
    {"add", "creates new mac based vlan",  do_add,
         "add <ifname> <index>\n"
         " - creates a new mac based vlan called \"ifname#index\" layered over \"ifname\"\n"
         " - mac based vlans over \"ifname\" must first be enabled with \"enable\"\n"
         " - \"ifname#index\" is not mapped to any MAC address until \"bind\" is called"
    },
    {"del", "destroys a mac based vlan",  do_del,
         "del <ifname>\n"
         " - deletes a mac base vlan called \"ifname\""
    },
    {"bind", "binds macaddr to vlan",  do_bind,
         "bind  <ifname> <macaddr>\n"
         " - binds macaddr to vlan called \"ifname\""
    },
    {"unbind", "unbinds macaddr from vlan",  do_unbind,
         "unbind <ifname> <macaddr>\n"
         " - unbinds macaddr from vlan called \"ifname\""
    },
    {"unload", "Unconfigure all of the macvlan devices",
     do_unload, "Unconfigure all of the macvlan devices so module can be unloaded"},
    {"setflags", "Set port flags on a port",
     do_setflags,
         "setflags <ifname> <new_flags>\n"
         "0x01   Bind to Destination instead of source MAC"
    },
    {"info", "print state of mac based vlans",  do_info, "info"},
};
#define NCOMMANDS (sizeof(command_list)/sizeof(struct command))


int parseInt(const char* s) {
   return strtol(s, NULL, 0); //should parse HEX, Octal, and Decimal.  If not decimal, must start with 0x
}


int do_help(int argc, char *argv[])
{
    unsigned int cmd;
    if (argc < 2)
        return -1;

    for (cmd = 0; cmd < NCOMMANDS; cmd++) {
        if (!strcmp(command_list[cmd].name,argv[1]))
            break;
    }
    if (cmd == NCOMMANDS)
        return -1;
    puts(command_list[cmd].long_help);
    return 0;
}

int do_enable(int argc, char *argv[])
{
    struct macvlan_ioctl req;
    int s;

    if (argc < 2) {
        printf("usage: %s <ifname>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    req.cmd = MACVLAN_ENABLE;
    req.ifname = argv[1];   /* 
                             * name of ethernet device over which we 
                             * are enabling mac based vlans
                             */

    if (ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
       if (errno != EEXIST) {
          perror("ioctl (SIOCGIFMACVLAN, MACVLAN_ENABLE)");
          printf("errno: %i\n", errno);
          return 1;
       }
       else {
          return 0;
       }
    }
    return 0;
}


int do_setflags(int argc, char *argv[])
{
    struct macvlan_ioctl req;
    int s;

    if (argc < 3) {
        printf("usage: %s <ifname> <flags>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    req.cmd = MACVLAN_SET_PORT_FLAGS;
    req.ifname = argv[1];   /* 
                             * name of ethernet device over which we 
                             * are enabling mac based vlans
                             */
    req.ifidx = parseInt(argv[2]);

    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("ioctl (SIOCGIFMACVLAN, SET_PORT_FLAGS)");
        return 1;
    }
    return 0;
}

int _do_disable(char* port, int s) {
    struct macvlan_ioctl req;
       
    req.cmd = MACVLAN_DISABLE;
    req.ifname = port;   /* 
                          * name of ethernet device over which we 
                          * are disabling mac based vlans
                          */

    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("disable-port");
        return -1;
    }
    else {
       printf("Disabled port: %s\n", port);
    }
    return 0;
}

int do_disable(int argc, char *argv[])
{
    int s;

    if (argc < 2) {
        printf("usage: %s <ifname>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    return _do_disable(argv[1], s);
}

int do_add(int argc, char *argv[])
{
    int s;
    struct macvlan_ioctl req;

    if (argc < 3) {
        printf("usage: %s <ifname> <index>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    req.cmd = MACVLAN_ADD;
    req.ifname = argv[1];       /* name of lower layer i/f over which we are adding an upper layer i/f */
    req.ifidx = parseInt(argv[2]);

    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("ioctl (SIOCGIFMACVLAN, MACVLAN_ADD)");
        return 1;
    }
    return 0;
}

int _do_del(char* ifname, int s) {
    struct macvlan_ioctl req;
   
    req.cmd = MACVLAN_DEL;
    req.ifname = ifname; /* name mac based vlan to destroy */
    
    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        printf("failed to delete interface: %s\n", ifname);
        perror("ioctl (SIOCGIFMACVLAN, MACVLAN_DEL)");
        return -1;
    }
    else {
       printf("Deleted interface: %s\n", ifname);
    }
    
    return 0;
}

int do_del(int argc, char *argv[])
{
    int s;

    if (argc < 2) {
        printf("usage: %s <ifname>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    return _do_del(argv[1], s);
}

    

int get_num_ports(int s) {
   struct macvlan_ioctl req;
   struct macvlan_ioctl_reply rep;
   
   req.cmd = MACVLAN_GET_NUM_PORTS;
   req.reply = &rep;
   if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
      perror("ioctl (SIOCGIFMACVLAN, GET_NUM_PORTS)");
      return -1;
   }

   printf("Found: %i ports\n", rep.num);

   return rep.num;
}

int get_num_vlans(int portidx, int s) {
    struct macvlan_ioctl req;
    struct macvlan_ioctl_reply rep;
   
    /* Get the number of mac-based-vlans layered
     * over this ethernet device 
     */
    req.cmd = MACVLAN_GET_NUM_VLANS;
    req.portidx = portidx;
    req.reply = &rep;
    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
       perror("ioctl (GET_NUM_VLANS)");
       return -1;
    }
    printf("Found: %i vlans for port: %i\n", rep.num, portidx);
    return rep.num;
}


int htoi(char *s)
{
    char ch;
    int i = 0;
    while ((ch = *s++)) {
        i <<= 4;
        i += (ch>='0'&&ch<='9')?(ch-'0'):((ch>='a'&&ch<='f')?(ch-'a'+10):((ch>='A'&&ch<='F')?(ch-'A'+10):0));
    }
    return i;
}

int do_bind(int argc, char *argv[])
{
    int s;
    struct macvlan_ioctl req;
    char *ptr;
    unsigned char macaddr[6];

    if (argc < 3) {
        printf("usage: %s <ifname> <macaddr>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    req.cmd = MACVLAN_BIND;
    req.ifname = argv[1]; /* name of vlan to which we are binding a MAC address */

    /* assemble the macaddr */
    ptr = argv[2];
    if (strlen(ptr) != 17) {
        printf("bad macaddr format: need aa:bb:cc:dd:ee:ff\n");
        return 1;
    }
    for (ptr = argv[2]+2; ptr < argv[2]+16; ptr+=3)
        *ptr = 0;
    ptr = argv[2];
    macaddr[0] = (unsigned char)htoi(ptr);
    macaddr[1] = (unsigned char)htoi(ptr+3);
    macaddr[2] = (unsigned char)htoi(ptr+6);
    macaddr[3] = (unsigned char)htoi(ptr+9);
    macaddr[4] = (unsigned char)htoi(ptr+12);
    macaddr[5] = (unsigned char)htoi(ptr+15);
    req.macaddr = macaddr;

    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("ioctl (MACVLAN_BIND)");
        return 1;
    }
    return 0;
}

int do_unbind(int argc, char *argv[])
{
    int s;
    struct macvlan_ioctl req;
    char *ptr;
    unsigned char macaddr[6];

    if (argc < 3) {
        printf("usage: %s <ifname> <macaddr>\n", argv[0]);
        return 1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    req.cmd = MACVLAN_UNBIND;
    req.ifname = argv[1]; /* name of vlan from which we are deleting a MAC address */

    /* assemble the macaddr */
    ptr = argv[2];
    if (strlen(ptr) != 17) {
        printf("bad macaddr format: need aa:bb:cc:dd:ee:ff\n");
        return 1;
    }
    for (ptr = argv[2]+2; ptr < argv[2]+16; ptr+=3)
        *ptr = 0;
    ptr = argv[2];
    macaddr[0] = (unsigned char)htoi(ptr);
    macaddr[1] = (unsigned char)htoi(ptr+3);
    macaddr[2] = (unsigned char)htoi(ptr+6);
    macaddr[3] = (unsigned char)htoi(ptr+9);
    macaddr[4] = (unsigned char)htoi(ptr+12);
    macaddr[5] = (unsigned char)htoi(ptr+15);
    req.macaddr = macaddr;

    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("ioctl (MACVLAN_UNBIND)");
        return 1;
    }
    return 0;
}

int do_info(int argc, char *argv[])
{
    int s;
    struct macvlan_ioctl req;
    struct macvlan_ioctl_reply rep;
    int nports;
    int portidx;
    int nifs;
    int ifidx;
    int nmacs;
    int macidx;
    unsigned char *p;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }
    /* get the number of ethernet devices which have mac based vlans
     * enabled over them
     */
    req.cmd = MACVLAN_GET_NUM_PORTS;
    req.reply = &rep;
    if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
        perror("ioctl (GET_NUM_PORTS)");
        return 1;
    }
    nports = rep.num;
    for (portidx = 0; portidx < nports; portidx++) {
        char tmpifname[64];
        /* Get the name of this mac-based-vlan enabled 
         * ethernet device
         */
        req.cmd = MACVLAN_GET_PORT_NAME;
        req.portidx = portidx;
        req.reply = &rep;
        if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
            perror("ioctl (GET_PORT_NAME)");
            return 1;
        }
        printf("-%s\n", rep.name);

        /* get the port flags */
        req.cmd = MACVLAN_GET_PORT_FLAGS;
        req.portidx = portidx;
        strcpy(tmpifname, rep.name);
        req.ifname = tmpifname;
        req.reply = &rep;
        if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
            perror("ioctl (GET_PORT_FLAGS)");
            return 1;
        }
        printf("-%s flag: 0x%x\n", tmpifname, rep.num);

        /* Get the number of mac-based-vlans layered
         * over this ethernet device 
         */
        req.cmd = MACVLAN_GET_NUM_VLANS;
        req.portidx = portidx;
        req.reply = &rep;
        if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
            perror("ioctl (GET_NUM_VLANS)");
            return 1;
        }
        nifs = rep.num;
        for (ifidx = 0; ifidx < nifs; ifidx++) {
            /* Get the name of this vlan */
            req.cmd = MACVLAN_GET_VLAN_NAME;
            req.portidx = portidx;
            req.ifidx = ifidx;
            req.reply = &rep;
            if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
                perror("ioctl (GET_VLAN_NAME)");
                return 1;
            }
            /* get the number of mac addresses owned by this vlan */
            printf(" |-%s\n", rep.name);
            req.cmd = MACVLAN_GET_NUM_MACS;
            req.portidx = portidx;
            req.ifidx = ifidx;
            req.reply = &rep;
            if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
                perror("ioctl (GET_NUM_MACS)");
                return 1;
            }
            nmacs = rep.num;
            for (macidx = 0; macidx < nmacs; macidx++) {
                /* get the value of this mac address */
                req.cmd = MACVLAN_GET_MAC_NAME;
                req.portidx = portidx;
                req.ifidx = ifidx;
                req.macaddridx = macidx;
                req.reply = &rep;
                if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
                    perror("ioctl (GET_MAC_NAME)");
                    return 1;
                }
                p = (unsigned char *) rep.name;
                printf(" | |-%02x:%02x:%02x:%02x:%02x:%02x\n", 
                       p[0],p[1],p[2],p[3],p[4],p[5]);
            }
        }
    }
    return 0;
}


int do_unload(int argc, char *argv[])
{
    int s;
    struct macvlan_ioctl req;
    struct macvlan_ioctl_reply rep;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    while (get_num_ports(s) > 0) {
       char port[64];
       /* Get the name of this mac-based-vlan enabled 
        * ethernet device
        */
       req.cmd = MACVLAN_GET_PORT_NAME;
       req.portidx = 0;
       req.reply = &rep;
       if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
          perror("ioctl (GET_PORT_NAME)");
          return 1;
       }
       strcpy(port, rep.name);
       
       while (get_num_vlans(0, s) > 0) {
          char cmd[128];
          /* Get the name of this vlan */
          req.cmd = MACVLAN_GET_VLAN_NAME;
          req.portidx = 0;
          req.ifidx = 0;
          req.reply = &rep;
          if(ioctl(s, SIOCGIFMACVLAN, &req) < 0) {
             perror("ioctl (GET_VLAN_NAME)");
             return 1;
          }

          /* Configure down the vlan */
          /* This would be faster using IOCTLs, of course! */
          printf("Configuring down interface: %s with ifconfig...", rep.name);
          sprintf(cmd, "ifconfig %s down", rep.name);
          system(cmd);

          /* Now, can remove it */
          _do_del(rep.name, s);
       }

       /* Now, remove the port */
       _do_disable(port, s);
          
    }
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned int cmd;
    int err;
    
    if (argc < 2)
        goto usage;

    for (cmd = 0; cmd < NCOMMANDS; cmd++) {
        if (!strcmp(command_list[cmd].name,argv[1]))
            break;
    }
    if (cmd == NCOMMANDS)
        goto usage;

    if ((err = command_list[cmd].fn(argc-1,argv+1)))
        goto usage;
    return 0;

 usage:
    printf("\n%s subcommands:\n\n", argv[0]);
    for (cmd = 0; cmd < NCOMMANDS; cmd++) {
        printf("%s %s:\t%s\n",argv[0],command_list[cmd].name,command_list[cmd].short_help);
    }
    return err;
}
