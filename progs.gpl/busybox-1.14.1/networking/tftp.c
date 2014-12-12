/* vi: set sw=4 ts=4: */
/* -------------------------------------------------------------------------
 * tftp.c
 *
 * A simple tftp client/server for busybox.
 * Tries to follow RFC1350.
 * Only "octet" mode supported.
 * Optional blocksize negotiation (RFC2347 + RFC2348)
 *
 * Copyright (C) 2001 Magnus Damm <damm@opensource.se>
 *
 * Parts of the code based on:
 *
 * atftp:  Copyright (C) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                        and Remi Lefebvre <remi@debian.org>
 *
 * utftp:  Copyright (C) 1999 Uwe Ohse <uwe@ohse.de>
 *
 * tftpd added by Denys Vlasenko & Vladimir Dronnikov
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 * ------------------------------------------------------------------------- */

#include "libbb.h"
#include "../../../include/elbox_config.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
#include <syslog.h>
#include "../../../include/asyslog.h"
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
#include "imghdr.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
#include "md5.h"/*ftp_tftp_FW_CG_20100119 log_luo*/

#include "lrgbin.h"/*ftp_tftp_FW_CG_20100119 log_luo*/
#include "rgdb.h"/*ftp_tftp_FW_CG_20100119 log_luo*/

char g_signature[50];/*ftp_tftp_FW_CG_20100119 log_luo*/
#define Config_Path "/var/config.bin"
#define Acl_Path "/var/acl.tem"
#endif
/*ftp_tftp_FW_CG_20100119 log_luo*//*End }*/

#if ENABLE_FEATURE_TFTP_GET || ENABLE_FEATURE_TFTP_PUT

#define TFTP_BLKSIZE_DEFAULT       512  /* according to RFC 1350, don't change */
#define TFTP_BLKSIZE_DEFAULT_STR "512"
#define TFTP_TIMEOUT_MS             50
#define TFTP_MAXTIMEOUT_MS        2000
#define TFTP_NUM_RETRIES            12  /* number of backed-off retries */

/* opcodes we support */
#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5
#define TFTP_OACK  6

/* error codes sent over network (we use only 0, 1, 3 and 8) */
/* generic (error message is included in the packet) */
#define ERR_UNSPEC   0
#define ERR_NOFILE   1
#define ERR_ACCESS   2
/* disk full or allocation exceeded */
#define ERR_WRITE    3
#define ERR_OP       4
#define ERR_BAD_ID   5
#define ERR_EXIST    6
#define ERR_BAD_USER 7
#define ERR_BAD_OPT  8

/* masks coming from getopt32 */
enum {
    TFTP_OPT_GET = (1 << 0),
    TFTP_OPT_PUT = (1 << 1),
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    TFTP_OPT_GETFW=(1 << 2),
    TFTP_OPT_GETCG=(1 << 3),
    TFTP_OPT_GETACL=(1 << 4),
    TFTP_OPT_PUTCG=(1 << 5),
    TFTP_OPT_PUTACL=(1 << 6),
#endif
    /* pseudo option: if set, it's tftpd */
    TFTPD_OPT = (1 << 7) * ENABLE_TFTPD,
    TFTPD_OPT_r = (1 << 8) * ENABLE_TFTPD,
    TFTPD_OPT_c = (1 << 9) * ENABLE_TFTPD,
    TFTPD_OPT_u = (1 << 10) * ENABLE_TFTPD,
};

#if ENABLE_FEATURE_TFTP_GET && !ENABLE_FEATURE_TFTP_PUT
#define USE_GETPUT(...)
#define CMD_GET(cmd) 1
#define CMD_PUT(cmd) 0
#elif !ENABLE_FEATURE_TFTP_GET && ENABLE_FEATURE_TFTP_PUT
#define USE_GETPUT(...)
#define CMD_GET(cmd) 0
#define CMD_PUT(cmd) 1
#else
#define USE_GETPUT(...) __VA_ARGS__
#define CMD_GET(cmd) ((cmd) & TFTP_OPT_GET)
#define CMD_PUT(cmd) ((cmd) & TFTP_OPT_PUT)
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
#define CMD_GETFW(cmd) ((cmd) & TFTP_OPT_GETFW)
#define CMD_GETCG(cmd) ((cmd) & TFTP_OPT_GETCG)
#define CMD_GETACL(cmd) ((cmd) & TFTP_OPT_GETACL)
#define CMD_PUTCG(cmd) ((cmd) & TFTP_OPT_PUTCG)
#define CMD_PUTACL(cmd) ((cmd) & TFTP_OPT_PUTACL)
#endif
#endif
/* NB: in the code below
 * CMD_GET(cmd) and CMD_PUT(cmd) are mutually exclusive
 */


struct globals {
    /* u16 TFTP_ERROR; u16 reason; both network-endian, then error text: */
    uint8_t error_pkt[4 + 32];
    char *user_opt;
    /* used in tftpd_main(), a bit big for stack: */
    char block_buf[TFTP_BLKSIZE_DEFAULT];
};
#define G (*(struct globals*)&bb_common_bufsiz1)
#define block_buf        (G.block_buf   )
#define user_opt         (G.user_opt    )
#define error_pkt        (G.error_pkt   )
#define INIT_G() do { } while (0)

#define error_pkt_reason (error_pkt[3])
#define error_pkt_str    (error_pkt + 4)

#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
char *download_buffer=NULL;
char *download_buffer_ptr=NULL;
int  download_len=0;
#define MAX_FW_BUFFER_SIZE 16*1024*1024 /*FW_log_20100119, Added by log_luo*/
#define MAX_CFG_BUFFER_SIZE 1*1024*1024
#define MAX_ACL_BUFFER_SIZE 1024*32    //acl import outport  by traveller chen

/*allenxiao add config_file_check 2012.5.23*/
int config_file_check(const char * image, int size) {
    imghdr2_t * v2hdr = (imghdr2_t *)image;
    unsigned char signature[MAX_SIGNATURE];
    int i;

    /* check if the signature match */
    memset(g_signature,0,50);
    RGDBGET(g_signature, 50, "/runtime/layout/image_sign");

    memset(signature, 0, sizeof(signature));
    strncpy(signature, g_signature, sizeof(signature));

    //printf("  expected signature(signature) : [%s]\n", signature);
    //printf("  image signature(v2hdr->signature)    : [%s]\n", v2hdr->signature);

    if (strncmp(signature, v2hdr->signature, strlen(signature))==0)
        return 0;
#if 0
    /* check if the signature is {boardtype}_aLpHa (ex: wrgg02_aLpHa, wrgg03_aLpHa */
    for (i=0; signature[i]!='_' && i<MAX_SIGNATURE; i++);
    if (signature[i] == '_') {
        signature[i+1] = 'a';
        signature[i+2] = 'L';
        signature[i+3] = 'p';
        signature[i+4] = 'H';
        signature[i+5] = 'a';
        signature[i+6] = '\0';

        //printf("try this signature : [%s]\n", signature);

        if (strcmp(signature, v2hdr->signature) == 0)
            return 0;
    }
#endif
    return -1;
}

#endif

#if ENABLE_FEATURE_TFTP_BLOCKSIZE

static int tftp_blksize_check(const char *blksize_str, int maxsize) {
    /* Check if the blksize is valid:
     * RFC2348 says between 8 and 65464,
     * but our implementation makes it impossible
     * to use blksizes smaller than 22 octets. */
    unsigned blksize = bb_strtou(blksize_str, NULL, 10);
    if (errno
            || (blksize < 24) || (blksize > maxsize)
       ) {
        bb_error_msg("bad blocksize '%s'", blksize_str);
        return -1;
    }
#if ENABLE_TFTP_DEBUG
    bb_error_msg("using blksize %u", blksize);
#endif
    return blksize;
}

static char *tftp_get_option(const char *option, char *buf, int len) {
    int opt_val = 0;
    int opt_found = 0;
    int k;

    /* buf points to:
     * "opt_name<NUL>opt_val<NUL>opt_name2<NUL>opt_val2<NUL>..." */

    while (len > 0) {
        /* Make sure options are terminated correctly */
        for (k = 0; k < len; k++) {
            if (buf[k] == '\0') {
                goto nul_found;
            }
        }
        return NULL;
nul_found:
        if (opt_val == 0) { /* it's "name" part */
            if (strcasecmp(buf, option) == 0) {
                opt_found = 1;
            }
        } else if (opt_found) {
            return buf;
        }

        k++;
        buf += k;
        len -= k;
        opt_val ^= 1;
    }

    return NULL;
}

#endif

static int tftp_protocol(
    len_and_sockaddr *our_lsa,
    len_and_sockaddr *peer_lsa,
    const char *local_file
    USE_TFTP(, const char *remote_file)
    USE_FEATURE_TFTP_BLOCKSIZE(USE_TFTPD(, void *tsize))
    USE_FEATURE_TFTP_BLOCKSIZE(, int blksize)) {
#if !ENABLE_TFTP
#define remote_file NULL
#endif
#if !(ENABLE_FEATURE_TFTP_BLOCKSIZE && ENABLE_TFTPD)
#define tsize NULL
#endif
#if !ENABLE_FEATURE_TFTP_BLOCKSIZE
    enum { blksize = TFTP_BLKSIZE_DEFAULT };
#endif

    struct pollfd pfd[1];
#define socket_fd (pfd[0].fd)
    int len;
    int send_len;
    USE_FEATURE_TFTP_BLOCKSIZE(smallint want_option_ack = 0;)
    smallint finished = 0;
    uint16_t opcode;
    uint16_t block_nr;
    uint16_t recv_blk;
    int open_mode, local_fd;
    int retries, waittime_ms;
    int io_bufsize = blksize + 4;
    int setfailed = 0;
    char *cp;
    /* Can't use RESERVE_CONFIG_BUFFER here since the allocation
     * size varies meaning BUFFERS_GO_ON_STACK would fail */
    /* We must keep the transmit and receive buffers seperate */
    /* In case we rcv a garbage pkt and we need to rexmit the last pkt */
    char *xbuf = xmalloc(io_bufsize);
    char *rbuf = xmalloc(io_bufsize);

    socket_fd = xsocket(peer_lsa->u.sa.sa_family, SOCK_DGRAM, 0);
    setsockopt_reuseaddr(socket_fd);
#ifdef CONFIG_RGBIN_BRCTL_FILTER
    len_and_sockaddr * our_lsa_tftp ;
    our_lsa_tftp = xhost2sockaddr("0.0.0.0", 2698);
    xbind(socket_fd, &our_lsa_tftp->u.sa, our_lsa_tftp->len);
#endif

    block_nr = 1;
    cp = xbuf + 2;

    if (!ENABLE_TFTP || our_lsa) {
        /* tftpd */

        /* Create a socket which is:
         * 1. bound to IP:port peer sent 1st datagram to,
         * 2. connected to peer's IP:port
         * This way we will answer from the IP:port peer
         * expects, will not get any other packets on
         * the socket, and also plain read/write will work. */
        xbind(socket_fd, &our_lsa->u.sa, our_lsa->len);
        xconnect(socket_fd, &peer_lsa->u.sa, peer_lsa->len);

        /* Is there an error already? Send pkt and bail out */
        if (error_pkt_reason || error_pkt_str[0])
            goto send_err_pkt;

        if (CMD_GET(option_mask32)) {
            /* it's upload - we must ACK 1st packet (with filename)
             * as if it's "block 0" */
            block_nr = 0;
        }

        if (user_opt) {
            struct passwd *pw = xgetpwnam(user_opt);
            change_identity(pw); /* initgroups, setgid, setuid */
        }
    }

    /* Open local file (must be after changing user) */
    if (CMD_PUT(option_mask32)) {
        open_mode = O_RDONLY;
    } else {
        open_mode = O_WRONLY | O_TRUNC | O_CREAT;
#if ENABLE_TFTPD
        if ((option_mask32 & (TFTPD_OPT+TFTPD_OPT_c)) == TFTPD_OPT) {
            /* tftpd without -c */
            open_mode = O_WRONLY | O_TRUNC;
        }
#endif
    }
    if (!(option_mask32 & TFTPD_OPT)) {
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
        if (CMD_GETFW(option_mask32))
            local_fd = -1;
        else
#endif
        {
            local_fd = CMD_GET(option_mask32) ? STDOUT_FILENO : STDIN_FILENO;
            if(local_file!=NULL) {
                if (NOT_LONE_DASH(local_file)) {
                    local_fd = xopen(local_file, open_mode);
                }
            }
        }
    } else {
        local_fd = open(local_file, open_mode);
        if (local_fd < 0) {
            error_pkt_reason = ERR_NOFILE;
            strcpy((char*)error_pkt_str, "can't open file");
            goto send_err_pkt;
        }
    }

    if (!ENABLE_TFTP || our_lsa) {
        /* gcc 4.3.1 would NOT optimize it out as it should! */
#if ENABLE_FEATURE_TFTP_BLOCKSIZE
        if (blksize != TFTP_BLKSIZE_DEFAULT || tsize) {
            /* Create and send OACK packet. */
            /* For the download case, block_nr is still 1 -
             * we expect 1st ACK from peer to be for (block_nr-1),
             * that is, for "block 0" which is our OACK pkt */
            opcode = TFTP_OACK;
            goto add_blksize_opt;
        }
#endif
    } else {
        /* Removing it, or using if() statement instead of #if may lead to
         * "warning: null argument where non-null required": */
#if ENABLE_TFTP
        /* tftp */

        /* We can't (and don't really need to) bind the socket:
         * we don't know from which local IP datagrams will be sent,
         * but kernel will pick the same IP every time (unless routing
         * table is changed), thus peer will see dgrams consistently
         * coming from the same IP.
         * We would like to connect the socket, but since peer's
         * UDP code can be less perfect than ours, _peer's_ IP:port
         * in replies may differ from IP:port we used to send
         * our first packet. We can connect() only when we get
         * first reply. */

        /* build opcode */
        opcode = TFTP_WRQ;
        if (CMD_GET(option_mask32)) {
            opcode = TFTP_RRQ;
        }
        /* add filename and mode */
        /* fill in packet if the filename fits into xbuf */
        len = strlen(remote_file) + 1;
        if (2 + len + sizeof("octet") >= io_bufsize) {
            bb_error_msg("remote filename is too long");
            goto ret;
        }
        strcpy(cp, remote_file);
        cp += len;
        /* add "mode" part of the package */
        strcpy(cp, "octet");
        cp += sizeof("octet");

#if ENABLE_FEATURE_TFTP_BLOCKSIZE
        if (blksize == TFTP_BLKSIZE_DEFAULT)
            goto send_pkt;

        /* Non-standard blocksize: add option to pkt */
        if ((&xbuf[io_bufsize - 1] - cp) < sizeof("blksize NNNNN")) {
            bb_error_msg("remote filename is too long");
            goto ret;
        }
        want_option_ack = 1;
#endif
#endif /* ENABLE_TFTP */

#if ENABLE_FEATURE_TFTP_BLOCKSIZE
add_blksize_opt:
#if ENABLE_TFTPD
        if (tsize) {
            struct stat st;
            /* add "tsize", <nul>, size, <nul> */
            strcpy(cp, "tsize");
            cp += sizeof("tsize");
            fstat(local_fd, &st);
            cp += snprintf(cp, 10, "%u", (int) st.st_size) + 1;
        }
#endif
        if (blksize != TFTP_BLKSIZE_DEFAULT) {
            /* add "blksize", <nul>, blksize, <nul> */
            strcpy(cp, "blksize");
            cp += sizeof("blksize");
            cp += snprintf(cp, 6, "%d", blksize) + 1;
        }
#endif
        /* First packet is built, so skip packet generation */
        goto send_pkt;
    }

    /* Using mostly goto's - continue/break will be less clear
     * in where we actually jump to */
    while (1) {
        /* Build ACK or DATA */
        cp = xbuf + 2;
        *((uint16_t*)cp) = htons(block_nr);
        cp += 2;
        block_nr++;
        opcode = TFTP_ACK;
        if (CMD_PUT(option_mask32)) {
            opcode = TFTP_DATA;
            len = full_read(local_fd, cp, blksize);
            if (len < 0) {
                goto send_read_err_pkt;
            }
            if (len != blksize) {
                finished = 1;
            }
            cp += len;
        }
send_pkt:
        /* Send packet */
        *((uint16_t*)xbuf) = htons(opcode); /* fill in opcode part */
        send_len = cp - xbuf;
        /* NB: send_len value is preserved in code below
         * for potential resend */

        retries = TFTP_NUM_RETRIES;	/* re-initialize */
        waittime_ms = TFTP_TIMEOUT_MS;

send_again:
#if ENABLE_TFTP_DEBUG
        fprintf(stderr, "sending %u bytes\n", send_len);
        for (cp = xbuf; cp < &xbuf[send_len]; cp++)
            fprintf(stderr, "%02x ", (unsigned char) *cp);
        fprintf(stderr, "\n");
#endif
        xsendto(socket_fd, xbuf, send_len, &peer_lsa->u.sa, peer_lsa->len);
        /* Was it final ACK? then exit */
        if (finished && (opcode == TFTP_ACK))
            goto ret;

recv_again:
        /* Receive packet */
        /*pfd[0].fd = socket_fd;*/
        pfd[0].events = POLLIN;
        switch (safe_poll(pfd, 1, waittime_ms)) {
        default:
            /*bb_perror_msg("poll"); - done in safe_poll */
            goto ret;
        case 0:
            retries--;
            if (retries == 0) {
                setfailed = 1;
                bb_error_msg("timeout");
                goto ret; /* no err packet sent */
            }

            /* exponential backoff with limit */
            waittime_ms += waittime_ms/2;
            if (waittime_ms > TFTP_MAXTIMEOUT_MS) {
                waittime_ms = TFTP_MAXTIMEOUT_MS;
            }

            goto send_again; /* resend last sent pkt */
        case 1:
            if (!our_lsa) {
                /* tftp (not tftpd!) receiving 1st packet */
                our_lsa = ((void*)(ptrdiff_t)-1); /* not NULL */
                len = recvfrom(socket_fd, rbuf, io_bufsize, 0,
                               &peer_lsa->u.sa, &peer_lsa->len);
                /* Our first dgram went to port 69
                 * but reply may come from different one.
                 * Remember and use this new port (and IP) */
                if (len >= 0)
                    xconnect(socket_fd, &peer_lsa->u.sa, peer_lsa->len);
            } else {
                /* tftpd, or not the very first packet:
                 * socket is connect()ed, can just read from it. */
                /* Don't full_read()!
                 * This is not TCP, one read == one pkt! */
                len = safe_read(socket_fd, rbuf, io_bufsize);
            }
            if (len < 0) {
                goto send_read_err_pkt;
            }
            if (len < 4) { /* too small? */
                goto recv_again;
            }
        }

        /* Process recv'ed packet */
        opcode = ntohs( ((uint16_t*)rbuf)[0] );
        recv_blk = ntohs( ((uint16_t*)rbuf)[1] );
#if ENABLE_TFTP_DEBUG
        fprintf(stderr, "received %d bytes: %04x %04x\n", len, opcode, recv_blk);
#endif
        if (opcode == TFTP_ERROR) {
            static const char errcode_str[] ALIGN1 =
                "\0"
                "file not found\0"
                "access violation\0"
                "disk full\0"
                "bad operation\0"
                "unknown transfer id\0"
                "file already exists\0"
                "no such user\0"
                "bad option";

            const char *msg = "";

            if (len > 4 && rbuf[4] != '\0') {
                msg = &rbuf[4];
                rbuf[io_bufsize - 1] = '\0'; /* paranoia */
            } else if (recv_blk <= 8) {
                msg = nth_string(errcode_str, recv_blk);
            }
            bb_error_msg("server error: (%u) %s", recv_blk, msg);
            goto ret;
        }

#if ENABLE_FEATURE_TFTP_BLOCKSIZE
        if (want_option_ack) {
            want_option_ack = 0;
            if (opcode == TFTP_OACK) {
                /* server seems to support options */
                char *res;

                res = tftp_get_option("blksize", &rbuf[2], len - 2);
                if (res) {
                    blksize = tftp_blksize_check(res, blksize);
                    if (blksize < 0) {
                        error_pkt_reason = ERR_BAD_OPT;
                        goto send_err_pkt;
                    }
                    io_bufsize = blksize + 4;
                    /* Send ACK for OACK ("block" no: 0) */
                    block_nr = 0;
                    continue;
                }
                /* rfc2347:
                 * "An option not acknowledged by the server
                 *  must be ignored by the client and server
                 *  as if it were never requested." */
            }
            bb_error_msg("server only supports blocksize of 512");
            blksize = TFTP_BLKSIZE_DEFAULT;
            io_bufsize = TFTP_BLKSIZE_DEFAULT + 4;
        }
#endif
        /* block_nr is already advanced to next block# we expect
         * to get / block# we are about to send next time */

        if (CMD_GET(option_mask32) && (opcode == TFTP_DATA)) {
            if (recv_blk == block_nr) {
                int sz;
                /*ftp_tftp_FW_CG_20100119 log_luo*//*start {*/
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
                if (CMD_GETFW(option_mask32) || CMD_GETCG(option_mask32) || CMD_GETACL(option_mask32)) {
                    sz = len -4;
                    memcpy(download_buffer_ptr,&rbuf[4],sz);
                    download_buffer_ptr += sz;
                    download_len += sz;
                } else
#endif
                    /*ftp_tftp_FW_CG_20100119 log_luo*//*end }*/
                    sz = full_write(local_fd, &rbuf[4], len - 4);
                if (sz != len - 4) {
                    strcpy((char*)error_pkt_str, bb_msg_write_error);
                    error_pkt_reason = ERR_WRITE;
                    goto send_err_pkt;
                }
                if (sz != blksize) {
                    finished = 1;
                }
                continue; /* send ACK */
            }
            if (recv_blk == (block_nr - 1)) {
                /* Server lost our TFTP_ACK.  Resend it */
                block_nr = recv_blk;
                continue;
            }
        }

        if (CMD_PUT(option_mask32) && (opcode == TFTP_ACK)) {
            /* did peer ACK our last DATA pkt? */
            if (recv_blk == (uint16_t) (block_nr - 1)) {
                if (finished)
                    goto ret;
                continue; /* send next block */
            }
        }
        /* Awww... recv'd packet is not recognized! */
        goto recv_again;
        /* why recv_again? - rfc1123 says:
         * "The sender (i.e., the side originating the DATA packets)
         *  must never resend the current DATA packet on receipt
         *  of a duplicate ACK".
         * DATA pkts are resent ONLY on timeout.
         * Thus "goto send_again" will ba a bad mistake above.
         * See:
         * http://en.wikipedia.org/wiki/Sorcerer's_Apprentice_Syndrome
         */
    } /* end of "while (1)" */
ret:
    if (ENABLE_FEATURE_CLEAN_UP) {
        close(local_fd);
        close(socket_fd);
        free(xbuf);
        free(rbuf);
    }
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(setfailed) RGDBSET("/runtime/update/status","FAILED");
#endif
    return finished == 0; /* returns 1 on failure */

send_read_err_pkt:
    strcpy((char*)error_pkt_str, bb_msg_read_error);
send_err_pkt:
    if (error_pkt_str[0])
        bb_error_msg((char*)error_pkt_str);
    error_pkt[1] = TFTP_ERROR;
    xsendto(socket_fd, error_pkt, 4 + 1 + strlen((char*)error_pkt_str),
            &peer_lsa->u.sa, peer_lsa->len);
    return EXIT_FAILURE;
#undef remote_file
#undef tsize
}

#if ENABLE_TFTP

int tftp_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int tftp_main(int argc UNUSED_PARAM, char **argv) {
    len_and_sockaddr *peer_lsa;
    const char *local_file = NULL;
    const char *remote_file = NULL;
#if ENABLE_FEATURE_TFTP_BLOCKSIZE
    const char *blksize_str = TFTP_BLKSIZE_DEFAULT_STR;
    int blksize;
#endif
    int result;
    int port;
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    char buff[50];
    int flags=0;
    int fd=-1;
#endif
    USE_GETPUT(int opt;)

    INIT_G();

    /* -p or -g is mandatory, and they are mutually exclusive */
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    opt_complementary = "" USE_FEATURE_TFTP_GET("g:f:d:m:") USE_FEATURE_TFTP_PUT("p:t:n:")
                        USE_GETPUT("g--pfdmtn:p--gfdmtn:f--gpdmtn");

    USE_GETPUT(opt =) getopt32(argv,
                               USE_FEATURE_TFTP_GET("g") USE_FEATURE_TFTP_PUT("p")
                               "fdmtnl:r:" USE_FEATURE_TFTP_BLOCKSIZE("b:"),
                               &local_file, &remote_file
                               USE_FEATURE_TFTP_BLOCKSIZE(, &blksize_str));
#else
    opt_complementary = "" USE_FEATURE_TFTP_GET("g:") USE_FEATURE_TFTP_PUT("p:")
                        USE_GETPUT("g--p:p--g:");

    USE_GETPUT(opt =) getopt32(argv,
                               USE_FEATURE_TFTP_GET("g") USE_FEATURE_TFTP_PUT("p")
                               "l:r:" USE_FEATURE_TFTP_BLOCKSIZE("b:"),
                               &local_file, &remote_file
                               USE_FEATURE_TFTP_BLOCKSIZE(, &blksize_str));
#endif
    argv += optind;

#if ENABLE_FEATURE_TFTP_BLOCKSIZE
    /* Check if the blksize is valid:
     * RFC2348 says between 8 and 65464 */
    blksize = tftp_blksize_check(blksize_str, 65564);
    if (blksize < 0) {
        //bb_error_msg("bad block size");
        return EXIT_FAILURE;
    }
#endif
    //RGDBSET("/runtime/update/status","NONE");
    //printf("######## set none!\n");
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(CMD_GETFW(option_mask32)||CMD_GETACL(option_mask32)||CMD_GETCG(option_mask32)) {
        option_mask32 = option_mask32 | TFTP_OPT_GET;
        flags = O_RDWR | O_CREAT | O_TRUNC;
        if(CMD_GETFW(option_mask32)) {
            download_buffer=malloc(MAX_FW_BUFFER_SIZE);
        } else if(CMD_GETACL(option_mask32)) {
            download_buffer=malloc(MAX_ACL_BUFFER_SIZE);
        } else if(CMD_GETCG(option_mask32)) {
            download_buffer=malloc(MAX_CFG_BUFFER_SIZE);
        }

        if(download_buffer) {
            download_buffer_ptr=download_buffer;
            download_len=0;
            //printf("allocate download_buffer->OK!\n");
            system("cp /usr/sbin/reboot /var/run/reboot");
            system("rgdb -i -s /runtime/update/fwimage/status 1");
        } else {
            //printf("allocate download_buffer->Fail!\n");
            return EXIT_FAILURE;
        }
    } else if(CMD_PUTACL(option_mask32)||CMD_PUTCG(option_mask32)) {
        option_mask32 = option_mask32 | TFTP_OPT_PUT;
        flags =O_RDONLY ;
    }


    if(local_file == NULL && !CMD_GETFW(option_mask32)) {
        local_file = remote_file;
    }

    if(CMD_GETACL(option_mask32)) {
        sprintf(buff,Acl_Path);
        local_file =buff;
    }

    if(CMD_PUTACL(option_mask32)) {
        FILE *pFile;
        char acl[40000];
        memset(acl,0x0,sizeof(acl));
        ftpSaveacl(acl);
        //printf("acl:%s\n",acl);
        if((pFile=fopen(Acl_Path,"w"))) {
            fwrite(acl, 1, strlen(acl), pFile);
            fclose(pFile);
        }
        sprintf(buff,Acl_Path);
        local_file =buff;
    }

    if(CMD_GETCG(option_mask32)) {
        /*The configuration file will be saved to /var/config.bin,
        and we will parse it after we get it by using tftp.*/
        sprintf(buff,Config_Path);
        local_file = buff;
        //printf("%s\n", buf);
    }

    if(CMD_PUTCG(option_mask32)) {
        /*To generate the configuration file to put to PC*/
        //printf("get config from flash\n");
        RGDBSET("/runtime/update/status","IN_PROCESS");
        system("/usr/sbin/sys -s configsave");
        /*The file will be /var/config.bin*/
        //printf("set local file to /var/config.bin\n");
        sprintf(buff,Config_Path);
        local_file = buff;
    }
    if(remote_file==NULL)
        remote_file = local_file;
#else

    if (remote_file) {
        if (!local_file) {
            const char *slash = strrchr(remote_file, '/');
            local_file = slash ? slash + 1 : remote_file;
        }
    } else {
        remote_file = local_file;
    }
#endif
    /* Error if filename or host is not known */
    if (!remote_file || !argv[0])
        bb_show_usage();

    port = bb_lookup_port(argv[1], "udp", 69);
    peer_lsa = xhost2sockaddr(argv[0], port);

#if ENABLE_TFTP_DEBUG
    fprintf(stderr, "using server '%s', remote_file '%s', local_file '%s'\n",
            xmalloc_sockaddr2dotted(&peer_lsa->u.sa),
            remote_file, local_file);
#endif

    result = tftp_protocol(
                 NULL /*our_lsa*/, peer_lsa,
                 local_file, remote_file
                 USE_FEATURE_TFTP_BLOCKSIZE(USE_TFTPD(, NULL /*tsize*/))
                 USE_FEATURE_TFTP_BLOCKSIZE(, blksize)
             );

    if (result != EXIT_SUCCESS && CMD_GET(opt)) {
        unlink(local_file);
    }
#if ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
    if(CMD_GETFW(option_mask32)&&result==EXIT_SUCCESS) {
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
        {
            // printf("image size=%d\n",download_len);
#ifdef ELBOX_PROGS_PRIV_CLI_SUPER
            {
                char xmldb_buff[10];

                memset(xmldb_buff, 0x0, sizeof(xmldb_buff));
                RGDBGET(xmldb_buff, 10, "/runtime/sys/super");
                //xmldbc_get_wb(NULL, 0, "/runtime/sys/super", xmldb_buff, sizeof(xmldb_buff)-1);
                if(atoi(xmldb_buff)==1) {
#ifdef ELBOX_FIRMWARE_HEADER_VERSION
                    if(ELBOX_FIRMWARE_HEADER_VERSION == 3) {
                        burn_image(download_buffer, download_len);
                        return(result);
                    } else if(ELBOX_FIRMWARE_HEADER_VERSION == 2) {
                        v2_burn_image(download_buffer, download_len);
                        return(result);
                    }
#else
                    v2_burn_image(download_buffer, download_len);
                    return(result);
#endif /*ELBOX_FIRMWARE_HEADER_VERSION*/
                }
            }
#endif /*ELBOX_PROGS_PRIV_CLI_SUPER*/
#ifdef ELBOX_FIRMWARE_HEADER_VERSION
            if(ELBOX_FIRMWARE_HEADER_VERSION == 3) {
                if (v3_image_check(download_buffer, download_len)==0) {
                    /*allenxiao add MIB info.2010.12.30*/
                    RGDBSET("/runtime/update/status","CORRECT");
                    v3_burn_image(download_buffer, download_len);
                    /*allenxiao add MIB info.2010.12.30*/
                    RGDBSET("/runtime/update/status","FW_SUCCESS");
#ifdef ELBOX_PROGS_GPL_NET_SNMP
                    system("sendtrapbin [SNMP-TRAP][Specific=12]\n");
#endif
                } else {
                    /*allenxiao add MIB info.2010.12.30*/
                    RGDBSET("/runtime/update/status","WRONG_IMAGE");

#if ELBOX_PROGS_GPL_SYSLOGD
                    syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Image file is not acceptable. Please check download file is right.");
#endif
#if ELBOX_PROGS_GPL_SNMP_TRAP_FW_UPDATE_FAIL
                    system("sendtrapbin [SNMP-TRAP][Specific=17]\n");
#endif
                }
            } else if(ELBOX_FIRMWARE_HEADER_VERSION == 2) {
                if (v2_image_check(download_buffer, download_len)==0) {
                    v2_burn_image(download_buffer, download_len);
                }
            }
#else
            if (v2_image_check(download_buffer, download_len)==0) {
                //printf("F/W image_check->OK!!\n");
                v2_burn_image(download_buffer, download_len);
                //printf("close(fd)..");
                //close(fd);
                //printf("kill local file..");
                //unlink(localfile);
                //printf("OK\n");

            } else {
                // printf("F/W image_check->fail!!\n");
            }
#endif /*ELBOX_FIRMWARE_HEADER_VERSION*/
        }


    }

//traveller add for acl import and outport start
    if(CMD_GETACL(option_mask32)&&result==EXIT_SUCCESS) {
        ftpUpload_acl(download_buffer, download_len);
        return result;
    }

    if(CMD_PUTACL(option_mask32)&&result==EXIT_SUCCESS) {
        unlink(local_file);
        return result;
    }
//traveller add for acl import and outport end

    /*FW_log_20100119, start {, Added by log_luo*/
    if(result!=EXIT_SUCCESS&&(CMD_GETCG(option_mask32)||CMD_GETFW(option_mask32)||CMD_PUTCG(option_mask32))) {
        if(CMD_PUTCG(option_mask32)) {
            /*allenxiao add MIB info.2012.5.24*/
            RGDBSET("/runtime/update/status","FAILED");
            /*Remove local file /var/config.bin*/
            unlink(local_file);
            bb_error_msg("Put configuraion file fail!");
        } else {
            /*allenxiao add MIB info.2012.5.24*/
            RGDBSET("/runtime/update/status","FAILED");
            bb_error_msg("Fail to get the file, please check the IP address and check the file name.");
            if(CMD_GETCG(option_mask32)) {
                unlink(local_file);
            }
#if ELBOX_PROGS_GPL_SYSLOGD
            else
                /*ftp_tftp_log_20070427*/
                syslog(ALOG_AP_SYSACT|LOG_WARNING,"[SYSACT]Fail to get the file, please check the IP address and check the file name.");
#endif
        }
    }
    /*FW_log_20100119, End }, Added by log_luo*/

    if(result==EXIT_SUCCESS&&(CMD_GETCG(option_mask32)||CMD_PUTCG(option_mask32))) {
        if( (fd = open(local_file, flags, 0644))<0)
            bb_perror_msg_and_die("local file\n");
        if (CMD_GETCG(option_mask32)) {
            if(config_file_check(download_buffer, download_len)==0) { //allenxiao add CG check.2010.12.30
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","CORRECT");
                //printf("######## set correct!\n");

                memset(g_signature,0,50);
                RGDBGET(g_signature, 50, "/runtime/layout/image_sign");

                download_buffer_ptr=download_buffer;
                download_buffer_ptr+=strlen(g_signature)+1;
                download_len-=strlen(g_signature)+1;
                //printf("Signature len =%d Bytes\n",strlen(g_signature));
                //printf("config file len =%d Bytes\n",download_len);

                if(write(fd,download_buffer_ptr,download_len)<=0) {
                    bb_error_msg("Write config to dev->fail");
                    /*allenxiao add MIB info.2012.5.23*/
                    RGDBSET("/runtime/update/status","FAILED");
                } else {
                    printf("write config to dev->OK\n");
                    system("devconf put -f /var/config.bin"); //big bug ,load config to xmldb will cause all runtime node lose,some funcation will fail
                    //system("/etc/scripts/misc/profile.sh reset /var/config.bin");
                    //system("/etc/scripts/misc/profile.sh put");/*Save_config_20070426 log_luo*/
                    //printf("/etc/scripts/misc/profile.sh reset /var/config.bin \n");
                    /*FW_log_20100119, Added by log_luo*/
                    bb_error_msg("Update configuraion file successfully!");
                    /*allenxiao add MIB info.2010.12.30*/
                    RGDBSET("/runtime/update/status","CONFIG_SUCCESS");
                    //printf("######## set config_success!\n");
                    /*phelpsll add update success message. 2009/08/11*/
                    bb_error_msg("Please reboot device!");
                }
            } else {
                /*allenxiao add MIB info.2010.12.30*/
                RGDBSET("/runtime/update/status","WRONG_CONFIG");
                //printf("######## set wrong_config!\n");
            }
        } else {
            RGDBSET("/runtime/update/status","CONFIG_SUCCESS");
            //printf("######## set config_success in else!\n");
            /*FW_log_20100119, Added by log_luo*/
            bb_error_msg("Put configuraion file successfully!");
        }
        close(fd);
        /*Remove local file /var/config.bin*/
        unlink(local_file);
    }

    if(download_buffer) {
        free(download_buffer);
        download_buffer=NULL;
        //  printf("Free download_buffer\n");
    }
#endif

    return result;
}

#endif /* ENABLE_TFTP */

#if ENABLE_TFTPD
int tftpd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int tftpd_main(int argc UNUSED_PARAM, char **argv) {
    len_and_sockaddr *our_lsa;
    len_and_sockaddr *peer_lsa;
    char *local_file, *mode;
    const char *error_msg;
    int opt, result, opcode;
    USE_FEATURE_TFTP_BLOCKSIZE(int blksize = TFTP_BLKSIZE_DEFAULT;)
    USE_FEATURE_TFTP_BLOCKSIZE(char *tsize = NULL;)

    INIT_G();

    our_lsa = get_sock_lsa(STDIN_FILENO);
    if (!our_lsa) {
        /* This is confusing:
         *bb_error_msg_and_die("stdin is not a socket");
         * Better: */
        bb_show_usage();
        /* Help text says that tftpd must be used as inetd service,
         * which is by far the most usual cause of get_sock_lsa
         * failure */
    }
    peer_lsa = xzalloc(LSA_LEN_SIZE + our_lsa->len);
    peer_lsa->len = our_lsa->len;

    /* Shifting to not collide with TFTP_OPTs */
    opt = option_mask32 = TFTPD_OPT | (getopt32(argv, "rcu:", &user_opt) << 8);
    argv += optind;
    if (argv[0])
        xchdir(argv[0]);

    result = recv_from_to(STDIN_FILENO, block_buf, sizeof(block_buf),
                          0 /* flags */,
                          &peer_lsa->u.sa, &our_lsa->u.sa, our_lsa->len);

    error_msg = "malformed packet";
    opcode = ntohs(*(uint16_t*)block_buf);
    if (result < 4 || result >= sizeof(block_buf)
            || block_buf[result-1] != '\0'
            || (USE_FEATURE_TFTP_PUT(opcode != TFTP_RRQ) /* not download */
                USE_GETPUT(&&)
                USE_FEATURE_TFTP_GET(opcode != TFTP_WRQ) /* not upload */
               )
       ) {
        goto err;
    }
    local_file = block_buf + 2;
    if (local_file[0] == '.' || strstr(local_file, "/.")) {
        error_msg = "dot in file name";
        goto err;
    }
    mode = local_file + strlen(local_file) + 1;
    if (mode >= block_buf + result || strcmp(mode, "octet") != 0) {
        goto err;
    }
#if ENABLE_FEATURE_TFTP_BLOCKSIZE
    {
        char *res;
        char *opt_str = mode + sizeof("octet");
        int opt_len = block_buf + result - opt_str;
        if (opt_len > 0) {
            res = tftp_get_option("blksize", opt_str, opt_len);
            if (res) {
                blksize = tftp_blksize_check(res, 65564);
                if (blksize < 0) {
                    error_pkt_reason = ERR_BAD_OPT;
                    /* will just send error pkt */
                    goto do_proto;
                }
            }
            /* did client ask us about file size? */
            tsize = tftp_get_option("tsize", opt_str, opt_len);
        }
    }
#endif

    if (!ENABLE_FEATURE_TFTP_PUT || opcode == TFTP_WRQ) {
        if (opt & TFTPD_OPT_r) {
            /* This would mean "disk full" - not true */
            /*error_pkt_reason = ERR_WRITE;*/
            error_msg = bb_msg_write_error;
            goto err;
        }
        USE_GETPUT(option_mask32 |= TFTP_OPT_GET;) /* will receive file's data */
    } else {
        USE_GETPUT(option_mask32 |= TFTP_OPT_PUT;) /* will send file's data */
    }

    /* NB: if error_pkt_str or error_pkt_reason is set up,
     * tftp_protocol() just sends one error pkt and returns */

do_proto:
    close(STDIN_FILENO); /* close old, possibly wildcard socket */
    /* tftp_protocol() will create new one, bound to particular local IP */
    result = tftp_protocol(
                 our_lsa, peer_lsa,
                 local_file USE_TFTP(, NULL /*remote_file*/)
                 USE_FEATURE_TFTP_BLOCKSIZE(, tsize)
                 USE_FEATURE_TFTP_BLOCKSIZE(, blksize)
             );

    return result;
err:
    strcpy((char*)error_pkt_str, error_msg);
    goto do_proto;
}

#endif /* ENABLE_TFTPD */

#endif /* ENABLE_FEATURE_TFTP_GET || ENABLE_FEATURE_TFTP_PUT */
