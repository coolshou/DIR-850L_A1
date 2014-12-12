/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2003 by Ralf Baechle
 * Copyright (C) 1996 by Paul M. Antoine
 * Copyright (C) 1999 Silicon Graphics
 * Copyright (C) 2000 MIPS Technologies, Inc.
 */
#ifndef _ASM_IRQFLAGS_H
#define _ASM_IRQFLAGS_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <asm/hazards.h>

__asm__(
	"	.macro	raw_local_irq_enable				\n"
	"	.set	push						\n"
	"	.set	reorder						\n"
	"	.set	noat						\n"
	"	mfc0	$1,$12						\n"
	"	ori	$1,0x1f						\n"
	"	xori	$1,0x1e						\n"
	"	mtc0	$1,$12						\n"
	"	irq_enable_hazard					\n"
	"	.set	pop						\n"
	"	.endm");

static inline void raw_local_irq_enable(void)
{
	__asm__ __volatile__(
		"raw_local_irq_enable"
		: /* no outputs */
		: /* no inputs */
		: "memory");
}

/*
 * For cli() we have to insert nops to make sure that the new value
 * has actually arrived in the status register before the end of this
 * macro.
 * R4000/R4400 need three nops, the R4600 two nops and the R10000 needs
 * no nops at all.
 */
/*
 * For TX49, operating only IE bit is not enough.
 *
 * If mfc0 $12 follows store and the mfc0 is last instruction of a
 * page and fetching the next instruction causes TLB miss, the result
 * of the mfc0 might wrongly contain EXL bit.
 *
 * ERT-TX49H2-027, ERT-TX49H3-012, ERT-TX49HL3-006, ERT-TX49H4-008
 *
 * Workaround: mask EXL bit of the result or place a nop before mfc0.
 */
__asm__(
	"	.macro	raw_local_irq_disable\n"
	"	.set	push						\n"
	"	.set	noat						\n"
	"	mfc0	$1,$12						\n"
	"	ori	$1,0x1f						\n"
	"	xori	$1,0x1f						\n"
	"	.set	noreorder					\n"
	"	mtc0	$1,$12						\n"
	"	irq_disable_hazard					\n"
	"	.set	pop						\n"
	"	.endm							\n");

static inline void raw_local_irq_disable(void)
{
	__asm__ __volatile__(
		"raw_local_irq_disable"
		: /* no outputs */
		: /* no inputs */
		: "memory");
}

__asm__(
	"	.macro	raw_local_save_flags flags			\n"
	"	.set	push						\n"
	"	.set	reorder						\n"
	"	mfc0	\\flags, $12					\n"
	"	.set	pop						\n"
	"	.endm							\n");

#define raw_local_save_flags(x)						\
__asm__ __volatile__(							\
	"raw_local_save_flags %0"					\
	: "=r" (x))

__asm__(
	"	.macro	raw_local_irq_save result			\n"
	"	.set	push						\n"
	"	.set	reorder						\n"
	"	.set	noat						\n"
	"	mfc0	\\result, $12					\n"
	"	ori	$1, \\result, 0x1f				\n"
	"	xori	$1, 0x1f					\n"
	"	.set	noreorder					\n"
	"	mtc0	$1, $12						\n"
	"	irq_disable_hazard					\n"
	"	.set	pop						\n"
	"	.endm							\n");

#define raw_local_irq_save(x)						\
__asm__ __volatile__(							\
	"raw_local_irq_save\t%0"					\
	: "=r" (x)							\
	: /* no inputs */						\
	: "memory")

__asm__(
	"	.macro	raw_local_irq_restore flags			\n"
	"	.set	push						\n"
	"	.set	noreorder					\n"
	"	.set	noat						\n"
	"	mfc0	$1, $12						\n"
	"	andi	\\flags, 1					\n"
	"	ori	$1, 0x1f					\n"
	"	xori	$1, 0x1f					\n"
	"	or	\\flags, $1					\n"
	"	mtc0	\\flags, $12					\n"
	"	irq_disable_hazard					\n"
	"	.set	pop						\n"
	"	.endm							\n");

static inline void raw_local_irq_restore(unsigned long flags)
{
	unsigned long __tmp1;

	__asm__ __volatile__(
		"raw_local_irq_restore\t%0"
		: "=r" (__tmp1)
		: "0" (flags)
		: "memory");
}

static inline void __raw_local_irq_restore(unsigned long flags)
{
	unsigned long __tmp1;

	__asm__ __volatile__(
		"raw_local_irq_restore\t%0"
		: "=r" (__tmp1)
		: "0" (flags)
		: "memory");
}

static inline int raw_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & 1);
}

#endif

/*
 * Do the CPU's IRQ-state tracing from assembly code.
 */
#ifdef CONFIG_TRACE_IRQFLAGS
/* Reload some registers clobbered by trace_hardirqs_on */
# define TRACE_IRQS_RELOAD_REGS						\
	LONG_L	$7, PT_R7(sp);						\
	LONG_L	$6, PT_R6(sp);						\
	LONG_L	$5, PT_R5(sp);						\
	LONG_L	$4, PT_R4(sp);						\
	LONG_L	$2, PT_R2(sp)
# define TRACE_IRQS_ON							\
	CLI;	/* make sure trace_hardirqs_on() is called in kernel level */ \
	jal	trace_hardirqs_on
# define TRACE_IRQS_ON_RELOAD						\
	TRACE_IRQS_ON;							\
	TRACE_IRQS_RELOAD_REGS
# define TRACE_IRQS_OFF							\
	jal	trace_hardirqs_off
#else
# define TRACE_IRQS_ON
# define TRACE_IRQS_ON_RELOAD
# define TRACE_IRQS_OFF
#endif

#endif /* _ASM_IRQFLAGS_H */
