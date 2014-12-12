#ifndef _BITS_SIGACTION_STRUCT_H
#define _BITS_SIGACTION_STRUCT_H

/* This is the sigaction struction from the Linux 2.1.20 kernel.  */

struct old_kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_mask;
	unsigned int sa_flags;
};

/* This is the sigaction structure from the Linux 2.1.68 kernel.  */

struct kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned int sa_flags;
	sigset_t sa_mask;
};

extern int __syscall_rt_sigaction (int, const struct kernel_sigaction *__unbounded,
	struct kernel_sigaction *__unbounded, size_t) attribute_hidden;

#endif
