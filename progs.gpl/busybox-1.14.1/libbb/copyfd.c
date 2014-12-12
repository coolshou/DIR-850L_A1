/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2005 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"

/* Used by NOFORK applets (e.g. cat) - must not use xmalloc */

static off_t bb_full_fd_action(int src_fd, int dst_fd, off_t size)
{
	int status = -1;
	off_t total = 0;
#if CONFIG_FEATURE_COPYBUF_KB <= 4
	char buffer[CONFIG_FEATURE_COPYBUF_KB * 1024];
	enum { buffer_size = sizeof(buffer) };
#else
	char *buffer;
	int buffer_size;

	/* We want page-aligned buffer, just in case kernel is clever
	 * and can do page-aligned io more efficiently */
	buffer = mmap(NULL, CONFIG_FEATURE_COPYBUF_KB * 1024,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANON,
			/* ignored: */ -1, 0);
	buffer_size = CONFIG_FEATURE_COPYBUF_KB * 1024;
	if (buffer == MAP_FAILED) {
		buffer = alloca(4 * 1024);
		buffer_size = 4 * 1024;
	}
#endif

	if (src_fd < 0)
		goto out;

	if (!size) {
		size = buffer_size;
		status = 1; /* copy until eof */
	}

	while (1) {
		ssize_t rd;

		rd = safe_read(src_fd, buffer, size > buffer_size ? buffer_size : size);

		if (!rd) { /* eof - all done */
			status = 0;
			break;
		}
		if (rd < 0) {
			bb_perror_msg(bb_msg_read_error);
			break;
		}
		/* dst_fd == -1 is a fake, else... */
		if (dst_fd >= 0) {
			ssize_t wr = full_write(dst_fd, buffer, rd);
			if (wr < rd) {
				bb_perror_msg(bb_msg_write_error);
				break;
			}
		}
		total += rd;
		if (status < 0) { /* if we aren't copying till EOF... */
			size -= rd;
			if (!size) {
				/* 'size' bytes copied - all done */
				status = 0;
				break;
			}
		}
	}
 out:

#if CONFIG_FEATURE_COPYBUF_KB > 4
	if (buffer_size != 4 * 1024)
		munmap(buffer, buffer_size);
#endif
	return status ? -1 : total;
}


#if 0
void FAST_FUNC complain_copyfd_and_die(off_t sz)
{
	if (sz != -1)
		bb_error_msg_and_die("short read");
	/* if sz == -1, bb_copyfd_XX already complained */
	xfunc_die();
}
#endif
/*ftp_tftp_FW_CG_20100113 log_luo*//*start {*/
#ifdef ELBOX_PROGS_GPL_TFTP_FTP_GET_PUT_FW_CONFIG
int bb_copyfd2buff_eof(int src_fd, char *buffer_ptr, const size_t Max_buffer_len)
{
        int read_total = 0;
        RESERVE_CONFIG_BUFFER(buffer,BUFSIZ);

        while ((read_total < Max_buffer_len)) {
                size_t read_try;
                ssize_t read_actual;

                read_try = BUFSIZ;


                read_actual = safe_read(src_fd, buffer, read_try);
                if (read_actual > 0) {
                        //if ((dst_fd >= 0) && (full_write(dst_fd, buffer, (size_t) read_actual) != read_actual)) {
                        //      bb_perror_msg(bb_msg_write_error);      /* match Read error below */
                        //      break;
                        //}
                        memcpy(buffer_ptr, buffer,(size_t) read_actual);
                        buffer_ptr += (int) read_actual;
                }
                else if (read_actual == 0) {
                        //if (size) {
                        //      bb_error_msg("Unable to read all data");
                        //}
                        break;
                } else {
                        /* read_actual < 0 */
                        bb_perror_msg("Read error");
                        break;
                }

                read_total += read_actual;
        }

        RELEASE_CONFIG_BUFFER(buffer);

        return (int)(read_total);
}
#endif
/*ftp_tftp_FW_CG_20100113 log_luo*//*End }*/

off_t FAST_FUNC bb_copyfd_size(int fd1, int fd2, off_t size)
{
	if (size) {
		return bb_full_fd_action(fd1, fd2, size);
	}
	return 0;
}

void FAST_FUNC bb_copyfd_exact_size(int fd1, int fd2, off_t size)
{
	off_t sz = bb_copyfd_size(fd1, fd2, size);
	if (sz == size)
		return;
	if (sz != -1)
		bb_error_msg_and_die("short read");
	/* if sz == -1, bb_copyfd_XX already complained */
	xfunc_die();
}

off_t FAST_FUNC bb_copyfd_eof(int fd1, int fd2)
{
	return bb_full_fd_action(fd1, fd2, 0);
}
