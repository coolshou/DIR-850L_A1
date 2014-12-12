/* vi: set sw=4 ts=4: */
/*
 * ftpget
 *
 * Mini implementation of FTP to retrieve a remote file.
 *
 * Copyright (C) 2002 Jeff Angielski, The PTR Group <jeff@theptrgroup.com>
 * Copyright (C) 2002 Glenn McGrath
 *
 * Based on wget.c by Chip Rosenthal Covad Communications
 * <chip@laserlink.net>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"
/*FW_log_20100119, start {, Added by log_luo*/
/*ELBOX_PROGS_GPL_SYSLOGD*/
#include <syslog.h>
#include "../../../include/asyslog.h"
/*FW_log_20100119, End }, Added by log_luo*/
#include "../../../include/elbox_config.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
//extern int bb_copyfd2buff_eof(int src_fd, char *buffer_ptr, const size_t Max_buffer_len);
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
#include "imghdr.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
//#include "md5.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
#include "lrgbin.h"/*ftp_tftp_FW_CG_20070112 log_luo*/
#include "rgdb.h"/*ftp_tftp_FW_CG_20070112 log_luo*/

char g_signature[50];/*ftp_tftp_FW_CG_20070112 log_luo*/
//#define RGDBGET(x, y, args...) _cli_rgdb_get_(x, y, ##args)/*ftp_tftp_FW_CG_20070112 log_luo*/
#endif
#ifdef CONFIG_RGBIN_BRCTL_FILTER
int fd_control;
#endif
struct globals {
    const char *user;
    const char *password;
    struct len_and_sockaddr *lsa;
    FILE *control_stream;
    int verbose_flag;
    int do_continue;
    char buf[1]; /* actually [BUFSZ] */
};
#define G (*(struct globals*)&bb_common_bufsiz1)
enum { BUFSZ = COMMON_BUFSIZE - offsetof(struct globals, buf) };
struct BUG_G_too_big {
    char BUG_G_too_big[sizeof(G) <= COMMON_BUFSIZE ? 1 : -1];
};
#define user           (G.user          )
#define password       (G.password      )
#define lsa            (G.lsa           )
#define control_stream (G.control_stream)
#define verbose_flag   (G.verbose_flag  )
#define do_continue    (G.do_continue   )
#define buf            (G.buf           )
#define INIT_G() do { } while (0)

/*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#ifdef ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
static char Get_FW = 0;
static char Put_CG = 0;
static char Get_CG = 0;
static char Get_ACL = 0;
static char Put_ACL = 0;
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
static char Put_LOG = 0;
#define Log_Path "/var/log/messages"
#endif
char *ftp_buffer;
char *ftp_buffer_ptr;
int  ftp_len;
#define MAX_FW_BUFFER_SIZE 16*1024*1024 /*FW_log_20100119, Added by log_luo*/
#define MAX_CFG_BUFFER_SIZE 1*1024*1024
#define MAX_ACL_BUFFER_SIZE 1024*32   //acl import outport  by traveller chen
#define Config_Path "/var/config.bin"
#define MAXACLNUM 256
#define MAXLINESIZ 18
#endif

static void ftp_die(const char *msg) NORETURN;
static void ftp_die(const char *msg) {
    char *cp = buf; /* buf holds peer's response */

    /* Guard against garbage from remote server */
    while (*cp >= ' ' && *cp < '\x7f')
        cp++;
    *cp = '\0';
    bb_error_msg_and_die("unexpected server response%s%s: %s",
                         (msg ? " to " : ""), (msg ? msg : ""), buf);
}

static int ftpcmd(const char *s1, const char *s2) {
    unsigned n;

    if (verbose_flag) {
        bb_error_msg("cmd %s %s", s1, s2);
    }

    if (s1) {
        fprintf(control_stream, (s2 ? "%s %s\r\n" : "%s %s\r\n"+3),
                s1, s2);
        fflush(control_stream);
    }

    do {
        strcpy(buf, "EOF");
        if (fgets(buf, BUFSZ - 2, control_stream) == NULL) {
            ftp_die(NULL);
        }
    } while (!isdigit(buf[0]) || buf[3] != ' ');

    buf[3] = '\0';
    n = xatou(buf);
    buf[3] = ' ';
    return n;
}

static void ftp_login(void) {
    /* Connect to the command socket */
#ifdef CONFIG_RGBIN_BRCTL_FILTER
    len_and_sockaddr *our_lsa;
    our_lsa = xhost2sockaddr("0.0.0.0", 1696);
    fd_control = xsocket(lsa->u.sa.sa_family, SOCK_STREAM, 0);
    setsockopt_reuseaddr(fd_control);
    xbind(fd_control, &our_lsa->u.sa, our_lsa->len);
    xconnect(fd_control, &lsa->u.sa, lsa->len);
    control_stream = fdopen(fd_control, "r+");
#else
    control_stream = fdopen(xconnect_stream(lsa), "r+");
#endif
    if (control_stream == NULL) {
        /* fdopen failed - extremely unlikely */
        bb_perror_nomsg_and_die();
    }

    if (ftpcmd(NULL, NULL) != 220) {
        ftp_die(NULL);
    }

    /*  Login to the server */
    switch (ftpcmd("USER", user)) {
    case 230:
        break;
    case 331:
        if (ftpcmd("PASS", password) != 230) {
            ftp_die("PASS");
        }
        break;
    default:
        ftp_die("USER");
    }

    ftpcmd("TYPE I", NULL);
}

static int xconnect_ftpdata(void) {
    char *buf_ptr;
    unsigned port_num;

    /*
    TODO: PASV command will not work for IPv6. RFC2428 describes
    IPv6-capable "extended PASV" - EPSV.

    "EPSV [protocol]" asks server to bind to and listen on a data port
    in specified protocol. Protocol is 1 for IPv4, 2 for IPv6.
    If not specified, defaults to "same as used for control connection".
    If server understood you, it should answer "229 <some text>(|||port|)"
    where "|" are literal pipe chars and "port" is ASCII decimal port#.

    There is also an IPv6-capable replacement for PORT (EPRT),
    but we don't need that.

    NB: PASV may still work for some servers even over IPv6.
    For example, vsftp happily answers
    "227 Entering Passive Mode (0,0,0,0,n,n)" and proceeds as usual.

    TODO2: need to stop ignoring IP address in PASV response.
    */

    if (ftpcmd("PASV", NULL) != 227) {
        ftp_die("PASV");
    }

    /* Response is "NNN garbageN1,N2,N3,N4,P1,P2[)garbage]
     * Server's IP is N1.N2.N3.N4 (we ignore it)
     * Server's port for data connection is P1*256+P2 */
    buf_ptr = strrchr(buf, ')');
    if (buf_ptr) *buf_ptr = '\0';

    buf_ptr = strrchr(buf, ',');
    *buf_ptr = '\0';
    port_num = xatoul_range(buf_ptr + 1, 0, 255);

    buf_ptr = strrchr(buf, ',');
    *buf_ptr = '\0';
    port_num += xatoul_range(buf_ptr + 1, 0, 255) * 256;

    set_nport(lsa, htons(port_num));
#ifdef CONFIG_RGBIN_BRCTL_FILTER
    len_and_sockaddr *our_lsa;
    our_lsa = xhost2sockaddr("0.0.0.0", 1697);
    int fd = xsocket(lsa->u.sa.sa_family, SOCK_STREAM, 0);
    setsockopt_reuseaddr(fd);
    xbind(fd, &our_lsa->u.sa, our_lsa->len);
    xconnect(fd, &lsa->u.sa, lsa->len);
    return fd;
#else
    return xconnect_stream(lsa);
#endif
}

static int pump_data_and_QUIT(int from, int to) {
    /* copy the file */
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG/*ftp_tftp_FW_CG_20070503 log_luo*/
    if ((!Get_FW) && (!Get_CG) &&(!Get_ACL))
#endif
    {
        if (bb_copyfd_eof(from, to) == -1) {
            /* error msg is already printed by bb_copyfd_eof */
            return EXIT_FAILURE;
        }
    }
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG/*ftp_tftp_FW_CG_20100119 log_luo*/
    else {
        if (Get_FW) {
            if (-1 == (ftp_len=bb_copyfd2buff_eof(from, ftp_buffer_ptr, MAX_FW_BUFFER_SIZE))) {
                /*FW_log_20100119, start {, Added by log_luo*/
                bb_error_msg("Fail to get the file, please check the IP address and check the file name.");
#if ELBOX_PROGS_GPL_SYSLOGD
                /*ftp_tftp_log_20070427*/
                syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Fail to get the file, please check the IP address and check the file name.");
#endif
                /*FW_log_20100119, End }, Added by log_luo*/
                exit(EXIT_FAILURE);
            }
        } else if (Get_CG) {
            if (-1 == (ftp_len=bb_copyfd2buff_eof(from, ftp_buffer_ptr, MAX_CFG_BUFFER_SIZE))) {
                /*FW_log_20100119, start {, Added by log_luo*/
                bb_error_msg("Fail to get the file.");
                /*Remove local file /var/config.bin*/
                unlink(Config_Path);
                /*FW_log_20100119, End }, Added by log_luo*/
                exit(EXIT_FAILURE);
            }
        } else if(Get_ACL) {
            if (-1 == (ftp_len=bb_copyfd2buff_eof(from, ftp_buffer_ptr, MAX_ACL_BUFFER_SIZE))) {

                exit(EXIT_FAILURE);

            }

        }
    }
#endif /*ftp_tftp_FW_CG_20100119 log_luo*/

    /* close data connection */
    close(from); /* don't know which one is that, so we close both */
//	close(to);

    /* does server confirm that transfer is finished? */
    if (ftpcmd(NULL, NULL) != 226) {
        ftp_die(NULL);
    }
    ftpcmd("QUIT", NULL);
    /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(Get_FW) {

#if ELBOX_PROGS_PRIV_APMODULE_FOR_ZOOM_NETWORKS
//william_201107, for Jason_jia field test.
        char xmldb_wait[10];

        memset(xmldb_wait, 0x0, sizeof(xmldb_wait));
        RGDBGET(xmldb_wait, 10, "/sys/fwwait");
        if(atoi(xmldb_wait)>0 && atoi(xmldb_wait)<=300) {
            sleep(atoi(xmldb_wait));
        }
        printf("sS\n");
#endif

#ifdef ELBOX_PROGS_PRIV_CLI_SUPER
        {
            char xmldb_buff[10];

            memset(xmldb_buff, 0x0, sizeof(xmldb_buff));
            RGDBGET(xmldb_buff, 10, "/runtime/sys/super");
            //xmldbc_get_wb(NULL, 0, "/runtime/sys/super", xmldb_buff, sizeof(xmldb_buff)-1);
            if(atoi(xmldb_buff)==1) {
#ifdef ELBOX_FIRMWARE_HEADER_VERSION
                if(ELBOX_FIRMWARE_HEADER_VERSION == 3) {
                    burn_image(ftp_buffer, ftp_len);
                    return(EXIT_SUCCESS);
                } else if(ELBOX_FIRMWARE_HEADER_VERSION == 2) {
                    v2_burn_image(ftp_buffer, ftp_len);
                    return(EXIT_SUCCESS);
                }
#else
                v2_burn_image(ftp_buffer, ftp_len);
                return(EXIT_SUCCESS);
#endif /*ELBOX_FIRMWARE_HEADER_VERSION*/
            }
        }
#endif /*ELBOX_PROGS_PRIV_CLI_SUPER*/
#ifdef ELBOX_FIRMWARE_HEADER_VERSION
        if(ELBOX_FIRMWARE_HEADER_VERSION == 3) {
            if (v3_image_check(ftp_buffer, ftp_len)==0) {
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","CORRECT");
                v3_burn_image(ftp_buffer, ftp_len);
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","FW_SUCCESS");
#ifdef ELBOX_PROGS_GPL_NET_SNMP
                system("sendtrapbin [SNMP-TRAP][Specific=12]\n");
#endif
            } else {
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","WRONG_IMAGE");
#if ELBOX_PROGS_GPL_SNMP_TRAP_FW_UPDATE_FAIL
                system("sendtrapbin [SNMP-TRAP][Specific=17]\n");
#endif
#if ELBOX_PROGS_GPL_SNMP_TRAP_TELECOM
                system("sendtrapbin [SNMP-TRAP][Telecom=8]\n");
#endif
            }
        } else if(ELBOX_FIRMWARE_HEADER_VERSION == 2) {
            if (v2_image_check(ftp_buffer, ftp_len)==0) {
                v2_burn_image(ftp_buffer, ftp_len);
            }
        }
#else
        if (v2_image_check(ftp_buffer, ftp_len)==0) {
            v2_burn_image(ftp_buffer, ftp_len);
        } else {

        }
#endif /*ELBOX_FIRMWARE_HEADER_VERSION*/
    }


    if(Get_CG) {
        if(config_file_check(ftp_buffer, ftp_len)==0) { //allenxiao add CG check.2010.12.30
            /*allenxiao add MIB info.2010.12.30*/
            RGDBSET("/runtime/update/status","CORRECT");
            //printf("######## ftp: set mib correct! \n");
            memset(g_signature,0,50);
            RGDBGET(g_signature, 50, "/runtime/layout/image_sign");

            ftp_buffer_ptr=ftp_buffer;
            ftp_buffer_ptr+=strlen(g_signature)+1;
            ftp_len-=strlen(g_signature)+1;

            if(write(to,ftp_buffer_ptr,ftp_len)<=0) {
                bb_error_msg("Write config to dev->fail");
            } else {
                RGDBSET("/runtime/update/status","IN_PROCESS");
                //printf("######## ftp: set mib in process! \n");
                system("devconf put -f /var/config.bin"); //big bug ,load config to xmldb will cause all runtime node lose,some funcation will fail
                 //system("/etc/scripts/misc/profile.sh reset /var/config.bin");//load config to xmldb
                // system("/etc/scripts/misc/profile.sh put");/*Save_config_20070426 log_luo*/
                /*FW_log_20100119, Added by log_luo*/
                bb_error_msg("Update configuraion file successfully!");
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","CONFIG_SUCCESS");
                //printf("######## ftp: set mib success! \n");
                /*phelpsll add update success message. 2009/08/11*/
                bb_error_msg("Please reboot device!");
            }
        } else {
            /*allenxiao add MIB info.2010.12.30*/
            RGDBSET("/runtime/update/status","WRONG_CONFIG");
            //printf("######## ftp: set mib wrong_config! \n");
        }
        close(to);
        /*Remove local file /var/config.bin*/
        unlink(Config_Path);
    }

    if(Get_ACL) {
        ftpUpload_acl(ftp_buffer,ftp_len);
    }
    if(ftp_buffer) {
        free(ftp_buffer);
        ftp_buffer=NULL;
        // printf("Free download_buffer\n");
    }
#endif
    /*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
    //printf("######## return success!");
    return EXIT_SUCCESS;
}

#if !ENABLE_FTPGET
int ftp_receive(const char *local_path, char *server_path);
#else
static
int ftp_receive(const char *local_path, char *server_path) {
    int fd_data;
    int fd_local = -1;
    off_t beg_range = 0;
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*/
    char sbuf[50];
#endif

    /* connect to the data socket */
    fd_data = xconnect_ftpdata();

    if (ftpcmd("SIZE", server_path) != 213) {
        do_continue = 0;
    }
    /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(!Get_CG && !Get_FW && ! Get_ACL)/*ftp_tftp_FW_CG_20100119 log_luo*/
#endif
        if (LONE_DASH(local_path)) {
            fd_local = STDOUT_FILENO;
            do_continue = 0;
        }
    /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(Get_CG) {
        /*The configuration file will be saved to /var/config.bin,
        and we will parse it after we get it by using ftp.*/
        sprintf(sbuf,Config_Path);
        //printf("set local_path to /var/config.bin\n");
        local_path = sbuf;
    }

    if(Get_FW || Get_CG ||Get_ACL) {


        if(Get_FW) {
            system("cp /usr/sbin/reboot /var/run/reboot");
            system("rgdb -i -s /runtime/update/fwimage/status 1");
            ftp_buffer=malloc(MAX_FW_BUFFER_SIZE);
        } else if(Get_ACL) {
            ftp_buffer=malloc(MAX_ACL_BUFFER_SIZE);

        } else {
            ftp_buffer=malloc(MAX_CFG_BUFFER_SIZE);
        }


        if(ftp_buffer) {
            ftp_buffer_ptr=ftp_buffer;
            ftp_len=0;
            // printf("ftp allocate download_buffer->OK!\n");
        } else {
            // printf("ftp allocate download_buffer->Fail!\n");
            exit(EXIT_FAILURE);
        }

    }
#endif
    /*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
    if (do_continue) {
        struct stat sbuf;
        /* lstat would be wrong here! */
        if (stat(local_path, &sbuf) < 0) {
            bb_perror_msg_and_die("stat");
        }
        if (sbuf.st_size > 0) {
            beg_range = sbuf.st_size;
        } else {
            do_continue = 0;
        }
    }

    if (do_continue) {
        sprintf(buf, "REST %"OFF_FMT"d", beg_range);
        if (ftpcmd(buf, NULL) != 350) {
            do_continue = 0;
        }
    }

    if (ftpcmd("RETR", server_path) > 150) {
        ftp_die("RETR");
    }

    /* create local file _after_ we know that remote file exists */
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG/*ftp_tftp_FW_CG_20100119 log_luo*/
    if (fd_local == -1 && !Get_FW && !Get_ACL)
#else
    if (fd_local == -1)
#endif
    {
        fd_local = xopen(local_path,
                         do_continue ? (O_APPEND | O_WRONLY)
                         : (O_CREAT | O_TRUNC | O_WRONLY)
                        );
    }

    return pump_data_and_QUIT(fd_data, fd_local);
}
#endif

#if !ENABLE_FTPPUT
int ftp_send(const char *server_path, char *local_path);
#else
static
int ftp_send(const char *server_path, char *local_path) {
    int fd_data;
    int fd_local;
    int response;
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*/
    char ssbuf[50];
#endif

    /* connect to the data socket */
    fd_data = xconnect_ftpdata();
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
    if(Put_CG) {
        /*To generate the configuration file to put to PC*/
        //printf("get config from flash\n");
        RGDBSET("/runtime/update/status","IN_PROCESS");
        system("/usr/sbin/sys -s configsave");
        /*The file will be /var/config.bin*/
        //printf("set local file to /var/config.bin\n");
        sprintf(ssbuf,Config_Path);
        local_path = ssbuf;
    }
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    if(Put_LOG) {
        sprintf(ssbuf,Log_Path);
        local_path = ssbuf;
    }

#endif
#endif
    /* get the local file */
    fd_local = STDIN_FILENO;
    if (NOT_LONE_DASH(local_path))
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG //by traveller chen
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
        if(Put_CG || Put_LOG)
#else
        if(Put_CG)
#endif
        {
            fd_local = xopen(local_path, O_RDONLY);
        }
#else
        fd_local = xopen(local_path, O_RDONLY);
#endif

    response = ftpcmd("STOR", server_path);
    switch (response) {
    case 125:
    case 150:
        break;
    default:
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*FW_log_20100119,start{, Added by log_luo*/
        if(Put_CG) {
            /*Remove local file /var/config.bin*/
            unlink(local_path);
        }
#endif /*FW_log_20100119,End}, Added by log_luo*/
        ftp_die("STOR");
    }
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG //by traveller chen
    /* transfer the file  */
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    if(Put_CG || Put_LOG)
#else
    if(Put_CG)
#endif
    {
        if (bb_copyfd_eof(fd_local, fd_data) == -1) {
            RGDBSET("/runtime/update/status","FAILED");
            exit(EXIT_FAILURE);
        }
    }

    //traveller add for acl transfer
    if(Put_ACL) {
        char acl[40000];
        memset(acl,0x0,sizeof(acl));
        ftpSaveacl(acl);
        if (full_write(fd_data,acl,strlen(acl)) != strlen(acl) ) {
            exit(EXIT_FAILURE);
        }
    }
#else
    /* transfer the file  */
    if (bb_copyfd_eof(fd_local, fd_data) == -1) {
        exit(EXIT_FAILURE);
    }

#endif

    /* close it all down */
    close(fd_data);
    /* does server confirm that transfer is finished? */
    if (ftpcmd(NULL, NULL) != 226) {
        ftp_die(NULL);
    }
    ftpcmd("QUIT", NULL);
#ifdef CONFIG_RGBIN_BRCTL_FILTER
    close(fd_control);
#endif
    /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(Put_CG) {
        close(fd_local);
        /*Remove local file /var/config.bin*/
        unlink(local_path);
        RGDBSET("/runtime/update/status","CONFIG_SUCCESS");
        /*FW_log_20100119, Added by log_luo*/
        bb_error_msg("Put configuraion file successfully!");
    }
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    else if(Put_LOG) {
        close(fd_local);
        bb_error_msg("Put LOG file successfully!");
    }
#endif
    else
        close(fd_local);
#endif
    /*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
    return EXIT_SUCCESS;
}
#endif

/*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#define FTPGETPUT_OPT_FW     	2
#define FTPGETPUT_OPT_PUT_CG	4
#define FTPGETPUT_OPT_GET_CG	8
#define FTPGETPUT_OPT_GET_ACL 0x100
#define FTPGETPUT_OPT_PUT_ACL 0x200
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
#define FTPGETPUT_OPT_PUT_LOG 0x400
#endif
/*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
#if ENABLE_FEATURE_FTPGETPUT_LONG_OPTIONS
static const char ftpgetput_longopts[] ALIGN1 =
    "continue\0" Required_argument "c"
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
    "firmware\0" Required_argument 'f'
    "put_config\0" Required_argument 't'
    "get_config\0" Required_argument 'd'
#endif
    "verbose\0"  No_argument       "v"
    "username\0" Required_argument "u"
    "password\0" Required_argument "p"
    "port\0"     Required_argument "P"
    ;
#endif

int ftpgetput_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int ftpgetput_main(int argc UNUSED_PARAM, char **argv) {
    unsigned opt;
    const char *port = "ftp";
    /* socket to ftp server */

#if ENABLE_FTPPUT && !ENABLE_FTPGET
# define ftp_action ftp_send
#elif ENABLE_FTPGET && !ENABLE_FTPPUT
# define ftp_action ftp_receive
#else
    int (*ftp_action)(const char *, char *) = ftp_send;

    /* Check to see if the command is ftpget or ftput */
    if (applet_name[3] == 'g') {
        ftp_action = ftp_receive;
    }
#endif

    INIT_G();
    /* Set default values */
    user = "anonymous";
    password = "busybox@";

    /*
     * Decipher the command line
     */
#if ENABLE_FEATURE_FTPGETPUT_LONG_OPTIONS
    applet_long_options = ftpgetput_longopts;
#endif
    opt_complementary = "-2:vv:cc"; /* must have 3 params; -v and -c count */
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    opt = getopt32(argv, "cftdvu:p:P:mnl", &user, &password, &port,
                   &verbose_flag, &do_continue);
#else
    opt = getopt32(argv, "cftdvu:p:P:mn", &user, &password, &port,
                   &verbose_flag, &do_continue);/*ftp_tftp_FW_CG_20100119 log_luo*/
#endif
    Get_FW = 0;
    Put_CG = 0;
    Get_CG = 0;
    Get_ACL=0;
    Put_ACL= 0;
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    Put_LOG= 0;
#endif
    if (opt & FTPGETPUT_OPT_FW) {
        Get_FW = 1;
    }
    if (opt & FTPGETPUT_OPT_PUT_CG) {
        Put_CG = 1;
    }
    if (opt & FTPGETPUT_OPT_GET_CG) {
        Get_CG = 1;
    }
    if(opt & FTPGETPUT_OPT_GET_ACL) {
        Get_ACL =1;
    }
    if(opt & FTPGETPUT_OPT_PUT_ACL) {
        Put_ACL =1;
    }
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    if(opt & FTPGETPUT_OPT_PUT_LOG) {
        Put_LOG =1;
    }
#endif
    /*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
#else
    opt = getopt32(argv, "cvu:p:P:", &user, &password, &port,
                   &verbose_flag, &do_continue);
#endif
    argv += optind;

    /* We want to do exactly _one_ DNS lookup, since some
     * sites (i.e. ftp.us.debian.org) use round-robin DNS
     * and we want to connect to only one IP... */
    lsa = xhost2sockaddr(argv[0], bb_lookup_port(port, "tcp", 21));
    if (verbose_flag) {
        printf("Connecting to %s (%s)\n", argv[0],
               xmalloc_sockaddr2dotted(&lsa->u.sa));
    }

    ftp_login();
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#ifdef ELBOX_PROGS_PRIV_CLI_EXTENSION_FOR_BID_CLOUD_SEA
    if((Get_FW || Get_CG || Put_CG || Get_ACL || Put_ACL || Put_LOG))
#else
    if((Get_FW || Get_CG || Put_CG || Get_ACL || Put_ACL))
#endif
        return ftp_action(argv[1], argv[1]);
    else
#endif
        /*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/
        return ftp_action(argv[1], argv[2]);
}
