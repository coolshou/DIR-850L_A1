/* vi: set sw=4 ts=4: */
/*
 * getpgrp() for uClibc
 *
 * Copyright (C) 2000-2008 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>
#include <unistd.h>

#ifdef __NR_getpgrp
/* According to the manpage the POSIX.1 version is favoured */
_syscall0(pid_t, getpgrp)
#elif defined __NR_getpgid && (defined __NR_getpid || defined __NR_getxpid)
/* IA64 doesn't have a getpgrp syscall */
pid_t getpgrp(void)
{
	return getpgid(getpid());
}
#elif defined __UCLIBC_HAS_STUBS__
pid_t getpgrp(void)
{
	__set_errno(ENOSYS);
	return -1;
}
#endif
