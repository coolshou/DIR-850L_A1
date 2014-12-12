	.file	1 "rtl865x_asicL4.c"
	.section .mdebug.abi32
	.previous
	.gnu_attribute 4, 3
#APP
	.macro _ssnop; sll $0, $0, 1; .endm
	.macro _ehb; sll $0, $0, 3; .endm
	.macro mtc0_tlbw_hazard; nop; nop; .endm
	.macro tlbw_use_hazard; nop; nop; nop; .endm
	.macro tlb_probe_hazard; nop; nop; nop; .endm
	.macro irq_enable_hazard; _ssnop; _ssnop; _ssnop;; .endm
	.macro irq_disable_hazard; nop; nop; nop; .endm
	.macro back_to_back_c0_hazard; _ssnop; _ssnop; _ssnop;; .endm
		.macro	raw_local_irq_enable				
	.set	push						
	.set	reorder						
	.set	noat						
	mfc0	$1,$12						
	ori	$1,0x1f						
	xori	$1,0x1e						
	mtc0	$1,$12						
	irq_enable_hazard					
	.set	pop						
	.endm
		.macro	raw_local_irq_disable
	.set	push						
	.set	noat						
	mfc0	$1,$12						
	ori	$1,0x1f						
	xori	$1,0x1f						
	.set	noreorder					
	mtc0	$1,$12						
	irq_disable_hazard					
	.set	pop						
	.endm							

		.macro	raw_local_save_flags flags			
	.set	push						
	.set	reorder						
	mfc0	\flags, $12					
	.set	pop						
	.endm							

		.macro	raw_local_irq_save result			
	.set	push						
	.set	reorder						
	.set	noat						
	mfc0	\result, $12					
	ori	$1, \result, 0x1f				
	xori	$1, 0x1f					
	.set	noreorder					
	mtc0	$1, $12						
	irq_disable_hazard					
	.set	pop						
	.endm							

		.macro	raw_local_irq_restore flags			
	.set	push						
	.set	noreorder					
	.set	noat						
	mfc0	$1, $12						
	andi	\flags, 1					
	ori	$1, 0x1f					
	xori	$1, 0x1f					
	or	\flags, $1					
	mtc0	\flags, $12					
	irq_disable_hazard					
	.set	pop						
	.endm							

#NO_APP
	.section	.text._rtl8651_NaptAgingToSec,"ax",@progbits
	.align	2
	.globl	_rtl8651_NaptAgingToSec
	.set	nomips16
	.ent	_rtl8651_NaptAgingToSec
	.type	_rtl8651_NaptAgingToSec, @function
_rtl8651_NaptAgingToSec:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	srl	$5,$4,3
	beq	$5,$0,$L2
	andi	$4,$4,0x7

	li	$2,1			# 0x1
	move	$3,$0
$L3:
	sll	$6,$2,3
	addu	$3,$3,$6
	addiu	$5,$5,-1
	bne	$5,$0,$L3
	sll	$2,$2,2

	addiu	$4,$4,1
	mult	$2,$4
	mflo	$4
	addu	$3,$4,$3
	sll	$4,$3,2
	addu	$3,$4,$3
	addiu	$3,$3,-5
	li	$4,-1431699456			# 0xaaaa0000
	ori	$4,$4,0xaaab
	multu	$3,$4
	mfhi	$3
	srl	$3,$3,1
	sltu	$4,$2,2
	movn	$2,$0,$4
$L4:
	j	$31
	subu	$2,$3,$2

$L2:
	addiu	$4,$4,1
	sll	$3,$4,2
	addu	$3,$3,$4
	addiu	$3,$3,-5
	li	$2,-1431699456			# 0xaaaa0000
	ori	$2,$2,0xaaab
	multu	$3,$2
	mfhi	$3
	srl	$3,$3,1
	j	$L4
	move	$2,$0

	.set	macro
	.set	reorder
	.end	_rtl8651_NaptAgingToSec
	.size	_rtl8651_NaptAgingToSec, .-_rtl8651_NaptAgingToSec
	.section	.text._rtl8651_NaptAgingToUnit,"ax",@progbits
	.align	2
	.globl	_rtl8651_NaptAgingToUnit
	.set	nomips16
	.ent	_rtl8651_NaptAgingToUnit
	.type	_rtl8651_NaptAgingToUnit, @function
_rtl8651_NaptAgingToUnit:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$4,$4,1
	sll	$3,$4,1
	addu	$3,$3,$4
	li	$2,-859045888			# 0xcccc0000
	ori	$2,$2,0xcccd
	multu	$3,$2
	mfhi	$3
	srl	$3,$3,2
	sltu	$2,$3,8
	bne	$2,$0,$L8
	li	$5,8			# 0x8

	li	$2,1			# 0x1
	move	$4,$0
$L9:
	subu	$3,$3,$5
	sll	$2,$2,2
	sll	$5,$2,3
	sltu	$6,$3,$5
	beq	$6,$0,$L9
	addiu	$4,$4,1

	divu	$0,$3,$2
	mfhi	$5
	srl	$9,$2,1
	sltu	$5,$9,$5
	beq	$5,$0,$L10
	sll	$8,$4,3

	divu	$0,$3,$2
	mflo	$6
	addu	$8,$8,$6
	j	$31
	addiu	$2,$8,1

$L8:
	li	$2,1			# 0x1
	move	$4,$0
$L10:
	divu	$0,$3,$2
	mflo	$2
	sll	$4,$4,3
	j	$31
	addu	$2,$2,$4

	.set	macro
	.set	reorder
	.end	_rtl8651_NaptAgingToUnit
	.size	_rtl8651_NaptAgingToUnit, .-_rtl8651_NaptAgingToUnit
	.section	.text._Is4WayHashEnabled,"ax",@progbits
	.align	2
	.globl	_Is4WayHashEnabled
	.set	nomips16
	.ent	_Is4WayHashEnabled
	.type	_Is4WayHashEnabled, @function
_Is4WayHashEnabled:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	lw	$2,0($2)
	j	$31
	andi	$2,$2,0x200

	.set	macro
	.set	reorder
	.end	_Is4WayHashEnabled
	.size	_Is4WayHashEnabled, .-_Is4WayHashEnabled
	.section	.text._set4WayHash,"ax",@progbits
	.align	2
	.globl	_set4WayHash
	.set	nomips16
	.ent	_set4WayHash
	.type	_set4WayHash, @function
_set4WayHash:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	lw	$2,0($2)
	beq	$4,$0,$L17
	li	$3,-513			# 0xfffffdff

	j	$L18
	ori	$2,$2,0x200

$L17:
	and	$2,$2,$3
$L18:
	li	$3,-1149239296			# 0xbb800000
	ori	$3,$3,0x441c
	sw	$2,0($3)
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	_set4WayHash
	.size	_set4WayHash, .-_set4WayHash
	.section	.text.rtl8651_naptTcpUdpTableIndex,"ax",@progbits
	.align	2
	.globl	rtl8651_naptTcpUdpTableIndex
	.set	nomips16
	.ent	rtl8651_naptTcpUdpTableIndex
	.type	rtl8651_naptTcpUdpTableIndex, @function
rtl8651_naptTcpUdpTableIndex:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	sll	$4,$4,24
	sra	$4,$4,24
	lhu	$3,18($sp)
	bne	$7,$0,$L21
	andi	$6,$6,0xffff

	bne	$3,$0,$L24
	srl	$2,$5,14

	andi	$8,$4,0x00ff
	andi	$2,$8,0x2
	bne	$2,$0,$L21
	andi	$8,$8,0x1

	srl	$2,$5,10
	srl	$3,$5,20
	xor	$3,$3,$2
	xor	$3,$5,$3
	xor	$3,$6,$3
	andi	$3,$3,0x3ff
	srl	$6,$6,10
	srl	$5,$5,24
	andi	$5,$5,0xc0
	or	$2,$6,$5
	sll	$8,$8,8
	or	$2,$2,$8
	j	$31
	xor	$2,$3,$2

$L21:
	srl	$2,$5,14
$L24:
	srl	$8,$5,4
	xor	$2,$8,$2
	xor	$2,$6,$2
	andi	$2,$2,0x3ff
	srl	$8,$7,25
	xor	$2,$2,$8
	srl	$6,$6,10
	andi	$8,$5,0xf
	sll	$8,$8,6
	or	$6,$6,$8
	xor	$2,$2,$6
	srl	$8,$7,5
	andi	$8,$8,0x3ff
	xor	$2,$2,$8
	srl	$8,$7,15
	andi	$8,$8,0x3ff
	xor	$2,$2,$8
	srl	$8,$3,1
	andi	$8,$8,0x3ff
	xor	$2,$2,$8
	srl	$6,$3,11
	andi	$7,$7,0x1f
	sll	$7,$7,5
	or	$7,$6,$7
	xor	$2,$2,$7
	andi	$4,$4,0x1
	sll	$4,$4,8
	srl	$5,$5,24
	or	$5,$4,$5
	andi	$3,$3,0x1
	sll	$3,$3,9
	or	$3,$5,$3
	j	$31
	xor	$2,$2,$3

	.set	macro
	.set	reorder
	.end	rtl8651_naptTcpUdpTableIndex
	.size	rtl8651_naptTcpUdpTableIndex, .-rtl8651_naptTcpUdpTableIndex
	.section	.text.rtl8651_setAsicNaptIcmpTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptIcmpTimeout
	.set	nomips16
	.ent	rtl8651_setAsicNaptIcmpTimeout
	.type	rtl8651_setAsicNaptIcmpTimeout, @function
rtl8651_setAsicNaptIcmpTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	jal	_rtl8651_NaptAgingToUnit
	nop

	sltu	$3,$2,1024
	bne	$3,$0,$L26
	li	$3,-1149239296			# 0xbb800000

	j	$L27
	li	$2,-1			# 0xffffffff

$L26:
	ori	$3,$3,0x4404
	lw	$5,0($3)
	sll	$2,$2,24
	li	$4,-1057030144			# 0xc0ff0000
	ori	$4,$4,0xffff
	and	$4,$5,$4
	or	$2,$2,$4
	sw	$2,0($3)
	move	$2,$0
$L27:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptIcmpTimeout
	.size	rtl8651_setAsicNaptIcmpTimeout, .-rtl8651_setAsicNaptIcmpTimeout
	.section	.text.rtl8651_getAsicNaptIcmpTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptIcmpTimeout
	.set	nomips16
	.ent	rtl8651_getAsicNaptIcmpTimeout
	.type	rtl8651_getAsicNaptIcmpTimeout, @function
rtl8651_getAsicNaptIcmpTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	bne	$4,$0,$L30
	move	$16,$4

	j	$L31
	li	$2,-1			# 0xffffffff

$L30:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4404
	lw	$4,0($2)
	srl	$4,$4,24
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$4,0x3f

	sw	$2,0($16)
	move	$2,$0
$L31:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptIcmpTimeout
	.size	rtl8651_getAsicNaptIcmpTimeout, .-rtl8651_getAsicNaptIcmpTimeout
	.section	.text.rtl8651_setAsicNaptUdpTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptUdpTimeout
	.set	nomips16
	.ent	rtl8651_setAsicNaptUdpTimeout
	.type	rtl8651_setAsicNaptUdpTimeout, @function
rtl8651_setAsicNaptUdpTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	jal	_rtl8651_NaptAgingToUnit
	nop

	sltu	$3,$2,1024
	bne	$3,$0,$L34
	li	$3,-1149239296			# 0xbb800000

	j	$L35
	li	$2,-1			# 0xffffffff

$L34:
	ori	$3,$3,0x4404
	lw	$5,0($3)
	sll	$2,$2,18
	li	$4,-16580608			# 0xff030000
	ori	$4,$4,0xffff
	and	$4,$5,$4
	or	$2,$2,$4
	sw	$2,0($3)
	move	$2,$0
$L35:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptUdpTimeout
	.size	rtl8651_setAsicNaptUdpTimeout, .-rtl8651_setAsicNaptUdpTimeout
	.section	.text.rtl8651_getAsicNaptUdpTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptUdpTimeout
	.set	nomips16
	.ent	rtl8651_getAsicNaptUdpTimeout
	.type	rtl8651_getAsicNaptUdpTimeout, @function
rtl8651_getAsicNaptUdpTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	bne	$4,$0,$L38
	move	$16,$4

	j	$L39
	li	$2,-1			# 0xffffffff

$L38:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4404
	lw	$4,0($2)
	srl	$4,$4,18
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$4,0x3f

	sw	$2,0($16)
	move	$2,$0
$L39:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptUdpTimeout
	.size	rtl8651_getAsicNaptUdpTimeout, .-rtl8651_getAsicNaptUdpTimeout
	.section	.text.rtl8651_setAsicNaptTcpLongTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptTcpLongTimeout
	.set	nomips16
	.ent	rtl8651_setAsicNaptTcpLongTimeout
	.type	rtl8651_setAsicNaptTcpLongTimeout, @function
rtl8651_setAsicNaptTcpLongTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	jal	_rtl8651_NaptAgingToUnit
	nop

	sltu	$3,$2,1024
	bne	$3,$0,$L42
	li	$3,-1149239296			# 0xbb800000

	j	$L43
	li	$2,-1			# 0xffffffff

$L42:
	ori	$3,$3,0x4404
	lw	$5,0($3)
	sll	$2,$2,12
	li	$4,-262144			# 0xfffc0000
	ori	$4,$4,0xfff
	and	$4,$5,$4
	or	$2,$2,$4
	sw	$2,0($3)
	move	$2,$0
$L43:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptTcpLongTimeout
	.size	rtl8651_setAsicNaptTcpLongTimeout, .-rtl8651_setAsicNaptTcpLongTimeout
	.section	.text.rtl8651_getAsicNaptTcpLongTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptTcpLongTimeout
	.set	nomips16
	.ent	rtl8651_getAsicNaptTcpLongTimeout
	.type	rtl8651_getAsicNaptTcpLongTimeout, @function
rtl8651_getAsicNaptTcpLongTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	bne	$4,$0,$L46
	move	$16,$4

	j	$L47
	li	$2,-1			# 0xffffffff

$L46:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4404
	lw	$4,0($2)
	srl	$4,$4,12
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$4,0x3f

	sw	$2,0($16)
	move	$2,$0
$L47:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptTcpLongTimeout
	.size	rtl8651_getAsicNaptTcpLongTimeout, .-rtl8651_getAsicNaptTcpLongTimeout
	.section	.text.rtl8651_setAsicNaptTcpMediumTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptTcpMediumTimeout
	.set	nomips16
	.ent	rtl8651_setAsicNaptTcpMediumTimeout
	.type	rtl8651_setAsicNaptTcpMediumTimeout, @function
rtl8651_setAsicNaptTcpMediumTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	jal	_rtl8651_NaptAgingToUnit
	nop

	sltu	$3,$2,1024
	bne	$3,$0,$L50
	li	$3,-1149239296			# 0xbb800000

	j	$L51
	li	$2,-1			# 0xffffffff

$L50:
	ori	$3,$3,0x4404
	lw	$5,0($3)
	sll	$2,$2,6
	li	$4,-4033			# 0xfffff03f
	and	$4,$5,$4
	or	$2,$2,$4
	sw	$2,0($3)
	move	$2,$0
$L51:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptTcpMediumTimeout
	.size	rtl8651_setAsicNaptTcpMediumTimeout, .-rtl8651_setAsicNaptTcpMediumTimeout
	.section	.text.rtl8651_getAsicNaptTcpMediumTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptTcpMediumTimeout
	.set	nomips16
	.ent	rtl8651_getAsicNaptTcpMediumTimeout
	.type	rtl8651_getAsicNaptTcpMediumTimeout, @function
rtl8651_getAsicNaptTcpMediumTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	bne	$4,$0,$L54
	move	$16,$4

	j	$L55
	li	$2,-1			# 0xffffffff

$L54:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4404
	lw	$4,0($2)
	srl	$4,$4,6
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$4,0x3f

	sw	$2,0($16)
	move	$2,$0
$L55:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptTcpMediumTimeout
	.size	rtl8651_getAsicNaptTcpMediumTimeout, .-rtl8651_getAsicNaptTcpMediumTimeout
	.section	.text.rtl8651_setAsicNaptTcpFastTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptTcpFastTimeout
	.set	nomips16
	.ent	rtl8651_setAsicNaptTcpFastTimeout
	.type	rtl8651_setAsicNaptTcpFastTimeout, @function
rtl8651_setAsicNaptTcpFastTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	jal	_rtl8651_NaptAgingToUnit
	nop

	sltu	$3,$2,1024
	bne	$3,$0,$L58
	li	$3,-1149239296			# 0xbb800000

	j	$L59
	li	$2,-1			# 0xffffffff

$L58:
	ori	$3,$3,0x4404
	lw	$5,0($3)
	li	$4,-64			# 0xffffffc0
	and	$4,$5,$4
	or	$2,$2,$4
	sw	$2,0($3)
	move	$2,$0
$L59:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptTcpFastTimeout
	.size	rtl8651_setAsicNaptTcpFastTimeout, .-rtl8651_setAsicNaptTcpFastTimeout
	.section	.text.rtl8651_getAsicNaptTcpFastTimeout,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptTcpFastTimeout
	.set	nomips16
	.ent	rtl8651_getAsicNaptTcpFastTimeout
	.type	rtl8651_getAsicNaptTcpFastTimeout, @function
rtl8651_getAsicNaptTcpFastTimeout:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	bne	$4,$0,$L62
	move	$16,$4

	j	$L63
	li	$2,-1			# 0xffffffff

$L62:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4404
	lw	$4,0($2)
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$4,0x3f

	sw	$2,0($16)
	move	$2,$0
$L63:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptTcpFastTimeout
	.size	rtl8651_getAsicNaptTcpFastTimeout, .-rtl8651_getAsicNaptTcpFastTimeout
	.section	.text.rtl8651_setAsicNaptAutoAddDelete,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptAutoAddDelete
	.set	nomips16
	.ent	rtl8651_setAsicNaptAutoAddDelete
	.type	rtl8651_setAsicNaptAutoAddDelete, @function
rtl8651_setAsicNaptAutoAddDelete:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	sll	$4,$4,24
	sra	$4,$4,24
	sll	$5,$5,24
	li	$2,1			# 0x1
	bne	$4,$2,$L66
	sra	$5,$5,24

	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4418
	lw	$3,0($2)
	ori	$3,$3,0x2
	sw	$3,0($2)
	j	$L71
	li	$2,1			# 0x1

$L66:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4418
	lw	$4,0($2)
	li	$3,-3			# 0xfffffffd
	and	$3,$4,$3
	sw	$3,0($2)
	li	$2,1			# 0x1
$L71:
	bne	$5,$2,$L68
	nop

	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4418
	lw	$3,0($2)
	ori	$3,$3,0x4
	sw	$3,0($2)
	j	$31
	move	$2,$0

$L68:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4418
	lw	$4,0($2)
	li	$3,-5			# 0xfffffffb
	and	$3,$4,$3
	sw	$3,0($2)
	move	$2,$0
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptAutoAddDelete
	.size	rtl8651_setAsicNaptAutoAddDelete, .-rtl8651_setAsicNaptAutoAddDelete
	.section	.text._rtl8651_disableEnhancedHash1,"ax",@progbits
	.align	2
	.globl	_rtl8651_disableEnhancedHash1
	.set	nomips16
	.ent	_rtl8651_disableEnhancedHash1
	.type	_rtl8651_disableEnhancedHash1, @function
_rtl8651_disableEnhancedHash1:
	.frame	$sp,32,$31		# vars= 8, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-32
	sw	$31,28($sp)
	sw	$16,24($sp)
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	lw	$16,0($2)
	jal	rtl865x_getAsicFun
	addiu	$4,$sp,16

	lw	$2,16($sp)
	andi	$2,$2,0x4
	beq	$2,$0,$L75
	li	$2,-1			# 0xffffffff

	li	$2,-8193			# 0xffffdfff
	and	$16,$16,$2
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	sw	$16,0($2)
	move	$2,$0
$L75:
	lw	$31,28($sp)
	lw	$16,24($sp)
	j	$31
	addiu	$sp,$sp,32

	.set	macro
	.set	reorder
	.end	_rtl8651_disableEnhancedHash1
	.size	_rtl8651_disableEnhancedHash1, .-_rtl8651_disableEnhancedHash1
	.section	.text._rtl8651_enableEnhancedHash1,"ax",@progbits
	.align	2
	.globl	_rtl8651_enableEnhancedHash1
	.set	nomips16
	.ent	_rtl8651_enableEnhancedHash1
	.type	_rtl8651_enableEnhancedHash1, @function
_rtl8651_enableEnhancedHash1:
	.frame	$sp,32,$31		# vars= 8, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-32
	sw	$31,28($sp)
	sw	$16,24($sp)
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	lw	$16,0($2)
	jal	rtl865x_getAsicFun
	addiu	$4,$sp,16

	lw	$2,16($sp)
	andi	$2,$2,0x4
	bne	$2,$0,$L78
	ori	$16,$16,0x2000

	j	$L79
	li	$2,-1			# 0xffffffff

$L78:
	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x441c
	sw	$16,0($2)
	move	$2,$0
$L79:
	lw	$31,28($sp)
	lw	$16,24($sp)
	j	$31
	addiu	$sp,$sp,32

	.set	macro
	.set	reorder
	.end	_rtl8651_enableEnhancedHash1
	.size	_rtl8651_enableEnhancedHash1, .-_rtl8651_enableEnhancedHash1
	.section	.text.rtl865x_initAsicL4,"ax",@progbits
	.align	2
	.globl	rtl865x_initAsicL4
	.set	nomips16
	.ent	rtl865x_initAsicL4
	.type	rtl865x_initAsicL4, @function
rtl865x_initAsicL4:
	.frame	$sp,32,$31		# vars= 8, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-32
	sw	$31,28($sp)
	jal	rtl865x_getAsicFun
	addiu	$4,$sp,16

	lw	$2,16($sp)
	andi	$2,$2,0x4
	beq	$2,$0,$L83
	li	$2,-1			# 0xffffffff

	li	$4,9			# 0x9
	jal	rtl8651_clearSpecifiedAsicTable
	li	$5,1024			# 0x400

	li	$2,-1149239296			# 0xbb800000
	ori	$2,$2,0x4418
	lw	$4,0($2)
	li	$3,-2			# 0xfffffffe
	and	$3,$4,$3
	sw	$3,0($2)
	move	$4,$0
	jal	rtl8651_setAsicNaptAutoAddDelete
	li	$5,1			# 0x1

	move	$2,$0
$L83:
	lw	$31,28($sp)
	j	$31
	addiu	$sp,$sp,32

	.set	macro
	.set	reorder
	.end	rtl865x_initAsicL4
	.size	rtl865x_initAsicL4, .-rtl865x_initAsicL4
	.section	.text.rtl8651_delAsicNaptTcpUdpTable,"ax",@progbits
	.align	2
	.globl	rtl8651_delAsicNaptTcpUdpTable
	.set	nomips16
	.ent	rtl8651_delAsicNaptTcpUdpTable
	.type	rtl8651_delAsicNaptTcpUdpTable, @function
rtl8651_delAsicNaptTcpUdpTable:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	move	$2,$4
	sltu	$3,$4,1024
	beq	$3,$0,$L86
	move	$6,$5

	sltu	$3,$5,1024
	beq	$3,$0,$L86
	li	$4,9			# 0x9

	jal	_rtl8651_delAsicEntry
	move	$5,$2

	j	$L87
	move	$2,$0

$L86:
	li	$2,-1			# 0xffffffff
$L87:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_delAsicNaptTcpUdpTable
	.size	rtl8651_delAsicNaptTcpUdpTable, .-rtl8651_delAsicNaptTcpUdpTable
	.section	.text.rtl8651_setAsicRawNaptTable,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicRawNaptTable
	.set	nomips16
	.ent	rtl8651_setAsicRawNaptTable
	.type	rtl8651_setAsicRawNaptTable, @function
rtl8651_setAsicRawNaptTable:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	move	$3,$4
	sll	$6,$6,24
	sra	$6,$6,24
	lw	$4,4($5)
	ori	$4,$4,0x4002
	sw	$4,4($5)
	li	$4,1			# 0x1
	bne	$6,$4,$L90
	move	$2,$5

	li	$4,9			# 0x9
	move	$5,$3
	jal	_rtl8651_forceAddAsicEntry
	move	$6,$2

	j	$L91
	nop

$L90:
	li	$4,9			# 0x9
	move	$5,$3
	jal	_rtl8651_addAsicEntry
	move	$6,$2

	li	$3,-1			# 0xffffffff
	movz	$3,$0,$2
	move	$2,$3
$L91:
	lw	$31,20($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicRawNaptTable
	.size	rtl8651_setAsicRawNaptTable, .-rtl8651_setAsicRawNaptTable
	.section	.text.rtl8651_getAsicRawNaptTable,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicRawNaptTable
	.set	nomips16
	.ent	rtl8651_getAsicRawNaptTable
	.type	rtl8651_getAsicRawNaptTable, @function
rtl8651_getAsicRawNaptTable:
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$16,16($sp)
	move	$2,$4
	move	$16,$5
	li	$4,9			# 0x9
	move	$5,$2
	jal	_rtl8651_readAsicEntry
	move	$6,$16

	addiu	$16,$16,4
	lw	$4,0($16)
	li	$3,65536			# 0x10000
	ori	$5,$3,0x1
	and	$5,$4,$5
	bne	$5,$3,$L95
	ori	$4,$4,0x1

	sw	$4,0($16)
$L95:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicRawNaptTable
	.size	rtl8651_getAsicRawNaptTable, .-rtl8651_getAsicRawNaptTable
	.section	.text.rtl8651_getAsicNaptTcpUdpTable,"ax",@progbits
	.align	2
	.globl	rtl8651_getAsicNaptTcpUdpTable
	.set	nomips16
	.ent	rtl8651_getAsicNaptTcpUdpTable
	.type	rtl8651_getAsicNaptTcpUdpTable, @function
rtl8651_getAsicNaptTcpUdpTable:
	.frame	$sp,56,$31		# vars= 32, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-56
	sw	$31,52($sp)
	sw	$16,48($sp)
	sltu	$2,$4,1024
	beq	$2,$0,$L98
	move	$16,$5

	beq	$5,$0,$L98
	nop

	jal	rtl8651_getAsicRawNaptTable
	addiu	$5,$sp,16

	lw	$2,16($sp)
	sw	$2,0($16)
	lhu	$2,26($sp)
	sh	$2,4($16)
	lw	$2,20($sp)
	srl	$3,$2,8
	andi	$3,$3,0x3f
	sb	$3,13($16)
	srl	$2,$2,2
	jal	_rtl8651_NaptAgingToSec
	andi	$4,$2,0x3f

	sw	$2,8($16)
	lw	$2,20($sp)
	srl	$3,$2,3
	andi	$3,$3,0x2000
	lw	$5,16($16)
	li	$4,-8193			# 0xffffdfff
	and	$4,$5,$4
	or	$4,$4,$3
	sw	$4,16($16)
	lw	$3,24($sp)
	srl	$5,$3,4
	andi	$5,$5,0x8000
	li	$6,-65536			# 0xffff0000
	ori	$6,$6,0x7fff
	and	$4,$4,$6
	or	$4,$4,$5
	sll	$5,$2,13
	andi	$5,$5,0x4000
	li	$6,-16385			# 0xffffbfff
	and	$4,$4,$6
	or	$4,$4,$5
	srl	$5,$2,2
	andi	$5,$5,0x1000
	li	$6,-4097			# 0xffffefff
	and	$4,$4,$6
	or	$4,$4,$5
	andi	$5,$2,0x1
	sll	$5,$5,10
	li	$6,-1025			# 0xfffffbff
	and	$4,$4,$6
	or	$4,$4,$5
	srl	$5,$2,4
	andi	$5,$5,0x800
	li	$6,-2049			# 0xfffff7ff
	and	$4,$4,$6
	or	$4,$4,$5
	sw	$4,16($16)
	srl	$4,$2,17
	andi	$4,$4,0xf
	sb	$4,14($16)
	srl	$2,$2,21
	andi	$2,$2,0x3ff
	sh	$2,16($16)
	srl	$2,$3,16
	andi	$2,$2,0x7
	sb	$2,12($16)
	srl	$2,$3,11
	andi	$2,$2,0x200
	lw	$5,16($16)
	li	$4,-513			# 0xfffffdff
	and	$5,$5,$4
	or	$5,$5,$2
	srl	$3,$3,15
	andi	$2,$3,0x1c0
	li	$4,-449			# 0xfffffe3f
	and	$3,$5,$4
	or	$3,$3,$2
	sw	$3,16($16)
	j	$L99
	move	$2,$0

$L98:
	li	$2,-1			# 0xffffffff
$L99:
	lw	$31,52($sp)
	lw	$16,48($sp)
	j	$31
	addiu	$sp,$sp,56

	.set	macro
	.set	reorder
	.end	rtl8651_getAsicNaptTcpUdpTable
	.size	rtl8651_getAsicNaptTcpUdpTable, .-rtl8651_getAsicNaptTcpUdpTable
	.section	.text.rtl8651_setAsicNaptTcpUdpTable,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicNaptTcpUdpTable
	.set	nomips16
	.ent	rtl8651_setAsicNaptTcpUdpTable
	.type	rtl8651_setAsicNaptTcpUdpTable, @function
rtl8651_setAsicNaptTcpUdpTable:
	.frame	$sp,64,$31		# vars= 32, regs= 4/0, args= 16, gp= 0
	.mask	0x80070000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-64
	sw	$31,60($sp)
	sw	$18,56($sp)
	sw	$17,52($sp)
	sw	$16,48($sp)
	move	$16,$5
	move	$17,$6
	sll	$18,$4,24
	sltu	$2,$5,1024
	beq	$2,$0,$L102
	sra	$18,$18,24

	beq	$6,$0,$L102
	addiu	$4,$sp,16

	move	$5,$0
	jal	memset
	li	$6,32			# 0x20

	lw	$2,20($sp)
	ori	$2,$2,0x4002
	lw	$3,16($17)
	srl	$3,$3,10
	andi	$3,$3,0x1
	li	$4,-2			# 0xfffffffe
	and	$2,$2,$4
	or	$2,$2,$3
	lw	$3,0($17)
	sw	$3,16($sp)
	lhu	$3,4($17)
	sh	$3,26($sp)
	lbu	$3,13($17)
	andi	$3,$3,0x3f
	sll	$3,$3,8
	li	$4,-16129			# 0xffffc0ff
	and	$2,$2,$4
	or	$2,$2,$3
	sw	$2,20($sp)
	lw	$4,8($17)
	jal	_rtl8651_NaptAgingToUnit
	nop

	andi	$2,$2,0x3f
	sll	$2,$2,2
	li	$3,-253			# 0xffffff03
	lw	$4,20($sp)
	and	$3,$4,$3
	or	$3,$3,$2
	lw	$2,16($17)
	srl	$4,$2,13
	andi	$4,$4,0x1
	sll	$4,$4,16
	li	$5,-131072			# 0xfffe0000
	ori	$5,$5,0xffff
	and	$3,$3,$5
	or	$3,$3,$4
	srl	$4,$2,15
	andi	$4,$4,0x1
	sll	$4,$4,19
	li	$5,-589824			# 0xfff70000
	ori	$5,$5,0xffff
	lw	$6,24($sp)
	and	$5,$6,$5
	or	$5,$5,$4
	lbu	$4,12($17)
	andi	$4,$4,0x7
	sll	$4,$4,16
	li	$6,-524288			# 0xfff80000
	ori	$6,$6,0xffff
	and	$5,$5,$6
	or	$5,$5,$4
	sll	$4,$2,4
	andi	$4,$4,0x8000
	li	$6,-65536			# 0xffff0000
	ori	$6,$6,0x7fff
	and	$3,$3,$6
	or	$3,$3,$4
	lbu	$4,14($17)
	andi	$4,$4,0xf
	sll	$4,$4,17
	li	$6,-2031616			# 0xffe10000
	ori	$6,$6,0xffff
	and	$3,$3,$6
	or	$3,$3,$4
	lhu	$4,16($17)
	andi	$4,$4,0x3ff
	sll	$4,$4,21
	li	$6,-2145452032			# 0x801f0000
	ori	$6,$6,0xffff
	and	$3,$3,$6
	or	$3,$3,$4
	ori	$3,$3,0x4002
	sw	$3,20($sp)
	srl	$3,$2,9
	andi	$3,$3,0x1
	sll	$3,$3,20
	li	$4,-1114112			# 0xffef0000
	ori	$4,$4,0xffff
	and	$5,$5,$4
	or	$5,$5,$3
	srl	$3,$2,6
	andi	$3,$3,0x7
	sll	$3,$3,21
	li	$4,-14745600			# 0xff1f0000
	ori	$4,$4,0xffff
	and	$2,$5,$4
	or	$2,$2,$3
	sw	$2,24($sp)
	li	$2,1			# 0x1
	bne	$18,$2,$L103
	li	$4,9			# 0x9

	move	$5,$16
	jal	_rtl8651_forceAddAsicEntry
	addiu	$6,$sp,16

	j	$L104
	move	$2,$0

$L103:
	move	$5,$16
	jal	_rtl8651_addAsicEntry
	addiu	$6,$sp,16

	beq	$2,$0,$L104
	nop

$L102:
	li	$2,-1			# 0xffffffff
$L104:
	lw	$31,60($sp)
	lw	$18,56($sp)
	lw	$17,52($sp)
	lw	$16,48($sp)
	j	$31
	addiu	$sp,$sp,64

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicNaptTcpUdpTable
	.size	rtl8651_setAsicNaptTcpUdpTable, .-rtl8651_setAsicNaptTcpUdpTable
	.section	.text.rtl8651_setAsicLiberalNaptTcpUdpTable,"ax",@progbits
	.align	2
	.globl	rtl8651_setAsicLiberalNaptTcpUdpTable
	.set	nomips16
	.ent	rtl8651_setAsicLiberalNaptTcpUdpTable
	.type	rtl8651_setAsicLiberalNaptTcpUdpTable, @function
rtl8651_setAsicLiberalNaptTcpUdpTable:
	.frame	$sp,88,$31		# vars= 32, regs= 10/0, args= 16, gp= 0
	.mask	0xc0ff0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-88
	sw	$31,84($sp)
	sw	$fp,80($sp)
	sw	$23,76($sp)
	sw	$22,72($sp)
	sw	$21,68($sp)
	sw	$20,64($sp)
	sw	$19,60($sp)
	sw	$18,56($sp)
	sw	$17,52($sp)
	sw	$16,48($sp)
	move	$17,$6
	sll	$20,$4,24
	sra	$20,$20,24
	andi	$16,$5,0xffff
	andi	$21,$7,0xffff
	lb	$23,107($sp)
	lhu	$18,110($sp)
	lb	$22,119($sp)
	lb	$fp,123($sp)
	lb	$19,135($sp)
	addiu	$4,$sp,16
	move	$5,$0
	jal	memset
	li	$6,32			# 0x20

	srl	$3,$18,10
	sll	$3,$3,8
	li	$2,-16129			# 0xffffc0ff
	lw	$4,20($sp)
	and	$2,$4,$2
	or	$2,$2,$3
	ori	$2,$2,0x4002
	beq	$19,$0,$L107
	sw	$2,20($sp)

	ori	$2,$2,0x8001
	li	$3,65536			# 0x10000
	or	$2,$2,$3
	sw	$2,20($sp)
	sw	$17,16($sp)
	sh	$21,26($sp)
	lw	$4,112($sp)
	jal	_rtl8651_NaptAgingToUnit
	andi	$22,$22,0x00ff

	srl	$3,$22,3
	andi	$3,$3,0x1
	beq	$3,$0,$L109
	move	$4,$0

	srl	$4,$22,2
	andi	$4,$4,0x1
$L109:
	andi	$2,$2,0x3f
	sll	$2,$2,2
	li	$5,-253			# 0xffffff03
	lw	$6,20($sp)
	and	$6,$6,$5
	or	$6,$6,$2
	andi	$18,$18,0x3ff
	sll	$2,$18,21
	li	$5,-2145452032			# 0x801f0000
	ori	$5,$5,0xffff
	and	$18,$6,$5
	or	$18,$18,$2
	andi	$23,$23,0xf
	sll	$2,$23,17
	li	$5,-2031616			# 0xffe10000
	ori	$5,$5,0xffff
	and	$23,$18,$5
	or	$23,$23,$2
	sw	$23,20($sp)
	srl	$22,$22,2
	sll	$3,$3,1
	andi	$22,$22,0x4
	or	$3,$3,$22
	or	$2,$4,$3
	andi	$2,$2,0x7
	sll	$2,$2,16
	li	$4,-524288			# 0xfff80000
	ori	$4,$4,0xffff
	lw	$3,24($sp)
	and	$4,$3,$4
	or	$4,$4,$2
	xori	$2,$fp,0x1
	sltu	$2,$2,1
	sll	$2,$2,19
	li	$3,-589824			# 0xfff70000
	ori	$3,$3,0xffff
	and	$fp,$4,$3
	or	$fp,$fp,$2
	j	$L110
	sw	$fp,24($sp)

$L107:
	sh	$0,26($sp)
	sw	$0,16($sp)
	li	$2,-65536			# 0xffff0000
	ori	$2,$2,0x7fff
	lw	$3,20($sp)
	and	$2,$3,$2
	li	$3,-131072			# 0xfffe0000
	ori	$3,$3,0xffff
	and	$2,$2,$3
	li	$3,-524288			# 0xfff80000
	ori	$3,$3,0xffff
	lw	$4,24($sp)
	and	$3,$4,$3
	sw	$3,24($sp)
	li	$3,-2031616			# 0xffe10000
	ori	$3,$3,0xffff
	and	$2,$2,$3
	li	$3,-2145452032			# 0x801f0000
	ori	$3,$3,0xffff
	and	$2,$2,$3
	li	$3,-253			# 0xffffff03
	and	$2,$2,$3
	sw	$2,20($sp)
$L110:
	li	$2,1			# 0x1
	bne	$20,$2,$L111
	li	$4,9			# 0x9

	move	$5,$16
	jal	_rtl8651_forceAddAsicEntry
	addiu	$6,$sp,16

	j	$L112
	nop

$L111:
	move	$5,$16
	jal	_rtl8651_addAsicEntry
	addiu	$6,$sp,16

	li	$3,-1			# 0xffffffff
	movz	$3,$0,$2
	move	$2,$3
$L112:
	lw	$31,84($sp)
	lw	$fp,80($sp)
	lw	$23,76($sp)
	lw	$22,72($sp)
	lw	$21,68($sp)
	lw	$20,64($sp)
	lw	$19,60($sp)
	lw	$18,56($sp)
	lw	$17,52($sp)
	lw	$16,48($sp)
	j	$31
	addiu	$sp,$sp,88

	.set	macro
	.set	reorder
	.end	rtl8651_setAsicLiberalNaptTcpUdpTable
	.size	rtl8651_setAsicLiberalNaptTcpUdpTable, .-rtl8651_setAsicLiberalNaptTcpUdpTable
	.ident	"GCC: (GNU) 4.4.5-1.5.5p4"
