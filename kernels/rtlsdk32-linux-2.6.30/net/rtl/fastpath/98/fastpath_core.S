	.file	1 "fastpath_core.c"
	.section .mdebug.abi32
	.previous
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
	.section	.text.cache_timeout,"ax",@progbits
	.align	2
	.ent	cache_timeout
	.type	cache_timeout, @function
cache_timeout:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	lbu	$2,0($4)
	beq	$2,$0,$L9
	move	$7,$4

	lw	$2,8($4)
	lw	$3,4($4)
	lhu	$4,2($4)
	xor	$3,$3,$2
	srl	$2,$3,16
	xor	$2,$2,$4
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lbu	$5,1($7)
	srl	$4,$3,8
	lw	$2,%lo(max_udp_frag_entry)($2)
	xor	$4,$4,$5
	xor	$3,$3,$4
	addiu	$2,$2,-1
	and	$4,$3,$2
	sll	$2,$4,1
	lw	$6,40($7)
	addu	$2,$2,$4
	sb	$0,0($7)
	lw	$3,44($7)
	beq	$6,$0,$L5
	sll	$5,$2,2

	j	$L6
	sw	$3,44($6)

$L5:
	lui	$2,%hi(udp_cache_table)
	lw	$2,%lo(udp_cache_table)($2)
	lw	$2,0($2)
	addu	$2,$5,$2
	sw	$3,4($2)
$L6:
	lw	$2,40($7)
	sll	$3,$4,1
	lw	$5,44($7)
	addu	$3,$3,$4
	sll	$3,$3,2
	lui	$4,%hi(udp_cache_list_free)
	sw	$2,0($5)
	lui	$2,%hi(udp_cache_table)
	lw	$2,%lo(udp_cache_table)($2)
	addiu	$4,$4,%lo(udp_cache_list_free)
	addiu	$6,$7,48
	lw	$2,0($2)
	addu	$3,$3,$2
	lw	$2,8($3)
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,48($7)
	lw	$2,4($4)
	sw	$2,52($7)
	lw	$2,4($4)
	sw	$7,0($2)
	lw	$2,8($4)
	sw	$6,4($4)
	addiu	$2,$2,1
	sw	$2,8($4)
$L9:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	cache_timeout
	.section	.text.udp_fragCache_init,"ax",@progbits
	.align	2
	.globl	udp_fragCache_init
	.ent	udp_fragCache_init
	.type	udp_fragCache_init, @function
udp_fragCache_init:
	.set	nomips16
	.frame	$sp,56,$31		# vars= 16, regs= 6/0, args= 16, gp= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L15:
$L56:
	addiu	$sp,$sp,-56
	lui	$2,%hi(malloc_sizes+4)
	sw	$19,44($sp)
	move	$19,$4
	sw	$18,40($sp)
	li	$5,32			# 0x20
	sw	$17,36($sp)
	sw	$16,32($sp)
	lui	$16,%hi(udp_cache_table)
	sw	$31,52($sp)
	sw	$20,48($sp)
	lw	$4,%lo(malloc_sizes+4)($2)
	jal	kmem_cache_alloc
	move	$18,$16

	move	$3,$0
	move	$17,$2
	beq	$2,$0,$L10
	sw	$2,%lo(udp_cache_table)($16)

$L65:
$L106:
	lui	$20,%hi(udp_cache_list_free)
	sll	$4,$19,1
	addiu	$3,$20,%lo(udp_cache_list_free)
	addu	$4,$4,$19
	sw	$3,4($3)
	lui	$2,%hi(max_udp_frag_entry)
	sw	$0,8($3)
	sll	$4,$4,2
	li	$5,32			# 0x20
	sw	$19,%lo(max_udp_frag_entry)($2)
	jal	__kmalloc
	sw	$0,%lo(udp_cache_list_free)($20)

	sw	$2,0($17)
	lw	$2,%lo(udp_cache_table)($16)
	lw	$2,0($2)
	beq	$2,$0,$L10
	li	$3,-1			# 0xffffffff

	j	$L109
	slt	$5,$0,$19

$L173:
	j	$L10
	move	$3,$0

$L109:
	beq	$5,$0,$L172
	move	$17,$0

	move	$4,$0
$L114:
	lw	$2,%lo(udp_cache_table)($18)
	addiu	$17,$17,1
	slt	$3,$17,$19
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(udp_cache_table)($18)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(udp_cache_table)($18)
	lw	$2,0($2)
	addu	$2,$4,$2
	addiu	$4,$4,12
	bne	$3,$0,$L114
	sw	$2,4($2)

	move	$17,$0
$L174:
	beq	$5,$0,$L172
	lui	$2,%hi(malloc_sizes+12)

$L122:
$L163:
	li	$5,32			# 0x20
	lw	$4,%lo(malloc_sizes+12)($2)
	jal	kmem_cache_alloc
	addiu	$17,$17,1

	move	$5,$0
	move	$16,$2
	addiu	$4,$2,16
	beq	$2,$0,$L173
	move	$6,$0

	jal	init_timer_key
	nop

	lui	$2,%hi(cache_timeout)
	addiu	$2,$2,%lo(cache_timeout)
	addiu	$3,$20,%lo(udp_cache_list_free)
	sw	$2,28($16)
	addiu	$4,$16,48
	sw	$16,32($16)
	slt	$5,$17,$19
	sw	$0,48($16)
	lw	$2,4($3)
	sw	$2,52($16)
	lw	$2,4($3)
	sw	$16,0($2)
	lw	$2,8($3)
	sw	$4,4($3)
	addiu	$2,$2,1
	j	$L174
	sw	$2,8($3)

$L172:
	li	$3,1			# 0x1
$L10:
	lw	$31,52($sp)
	move	$2,$3
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	j	$31
	addiu	$sp,$sp,56

	.set	macro
	.set	reorder
	.end	udp_fragCache_init
	.section	.text.FastPath_Hash_NAPT_Entry,"ax",@progbits
	.align	2
	.ent	FastPath_Hash_NAPT_Entry
	.type	FastPath_Hash_NAPT_Entry, @function
FastPath_Hash_NAPT_Entry:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	srl	$3,$4,16
	srl	$2,$6,16
	xor	$2,$2,$6
	xor	$3,$3,$4
	xor	$3,$3,$2
	lui	$2,%hi(napt_table_list_max)
	lhu	$4,18($sp)
	xor	$3,$3,$5
	xor	$3,$3,$7
	lw	$2,%lo(napt_table_list_max)($2)
	xor	$3,$3,$4
	srl	$4,$3,12
	xor	$3,$3,$4
	addiu	$2,$2,-1
	j	$31
	and	$2,$2,$3

	.set	macro
	.set	reorder
	.end	FastPath_Hash_NAPT_Entry
	.section	.text.rtk_addArp,"ax",@progbits
	.align	2
	.globl	rtk_addArp
	.ent	rtk_addArp
	.type	rtk_addArp, @function
rtk_addArp:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_addArp
	.section	.text.rtk_modifyArp,"ax",@progbits
	.align	2
	.globl	rtk_modifyArp
	.ent	rtk_modifyArp
	.type	rtk_modifyArp, @function
rtk_modifyArp:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_modifyArp
	.section	.text.rtk_delArp,"ax",@progbits
	.align	2
	.globl	rtk_delArp
	.ent	rtk_delArp
	.type	rtk_delArp, @function
rtk_delArp:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	li	$2,-4			# 0xfffffffc

	.set	macro
	.set	reorder
	.end	rtk_delArp
	.section	.text.rtk_addRoute,"ax",@progbits
	.align	2
	.globl	rtk_addRoute
	.ent	rtk_addRoute
	.type	rtk_addRoute, @function
rtk_addRoute:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_addRoute
	.section	.text.rtk_modifyRoute,"ax",@progbits
	.align	2
	.globl	rtk_modifyRoute
	.ent	rtk_modifyRoute
	.type	rtk_modifyRoute, @function
rtk_modifyRoute:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_modifyRoute
	.section	.text.rtk_delRoute,"ax",@progbits
	.align	2
	.globl	rtk_delRoute
	.ent	rtk_delRoute
	.type	rtk_delRoute, @function
rtk_delRoute:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	li	$2,-4			# 0xfffffffc

	.set	macro
	.set	reorder
	.end	rtk_delRoute
	.section	.text.rtk_addSession,"ax",@progbits
	.align	2
	.globl	rtk_addSession
	.ent	rtk_addSession
	.type	rtk_addSession, @function
rtk_addSession:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_addSession
	.section	.text.rtk_delSession,"ax",@progbits
	.align	2
	.globl	rtk_delSession
	.ent	rtk_delSession
	.type	rtk_delSession, @function
rtk_delSession:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	rtk_delSession
	.section	.text.FastPath_Route,"ax",@progbits
	.align	2
	.globl	FastPath_Route
	.ent	FastPath_Route
	.type	FastPath_Route, @function
FastPath_Route:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	lui	$2,%hi(route_list_inuse)
	move	$8,$0
	lw	$3,%lo(route_list_inuse)($2)
	move	$7,$0
$L192:
	beq	$3,$0,$L191
	nop

	lw	$5,8($3)
	sltu	$2,$5,$7
	bne	$2,$0,$L187
	and	$6,$4,$5

	lw	$2,4($3)
	bne	$6,$2,$L187
	nop

	move	$7,$5
	addiu	$8,$3,16
$L187:
	lw	$3,44($3)
	j	$L192
	nop

$L191:
	j	$31
	move	$2,$8

	.set	macro
	.set	reorder
	.end	FastPath_Route
	.section	.text.rtk_addNaptConnection,"ax",@progbits
	.align	2
	.globl	rtk_addNaptConnection
	.ent	rtk_addNaptConnection
	.type	rtk_addNaptConnection, @function
rtk_addNaptConnection:
	.set	nomips16
	.frame	$sp,152,$31		# vars= 88, regs= 10/0, args= 24, gp= 0
	.mask	0xc0ff0000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-152
	li	$3,-1			# 0xffffffff
	sw	$fp,144($sp)
	move	$fp,$7
	sw	$19,124($sp)
	move	$19,$5
	sw	$18,120($sp)
	move	$18,$4
	sw	$31,148($sp)
	sw	$23,140($sp)
	sw	$22,136($sp)
	sw	$21,132($sp)
	sw	$20,128($sp)
	sw	$17,116($sp)
	sw	$16,112($sp)
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L193
	sw	$6,160($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$5
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L193
	li	$3,-1			# 0xffffffff
	.set	macro
	.set	reorder

	lw	$3,0($18)
	li	$2,17			# 0x11
	lw	$4,20($18)
	li	$5,6			# 0x6
	xori	$3,$3,0x2
	movn	$5,$2,$3		#conditional move  
	.set	noreorder
	.set	nomacro
	jal	filter_addconnect
	sw	$5,76($sp)
	.set	macro
	.set	reorder

#APP
	raw_local_irq_save	$23
#NO_APP
	lw	$22,0($18)
	lw	$21,4($18)
	lw	$20,8($18)
	andi	$2,$22,0xffff
	lw	$17,20($18)
	move	$4,$21
	lw	$16,24($18)
	move	$5,$20
	sw	$2,16($sp)
	move	$6,$17
	.set	noreorder
	.set	nomacro
	jal	FastPath_Hash_NAPT_Entry
	move	$7,$16
	.set	macro
	.set	reorder

	lui	$9,%hi(table_napt)
	lw	$3,%lo(table_napt)($9)
	sll	$7,$2,1
	sw	$2,72($sp)
	addu	$2,$7,$2
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$4,0($2)
$L294:
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L293
	lui	$5,%hi(napt_list_free)
	.set	macro
	.set	reorder

	lw	$2,4($4)
	bne	$2,$22,$L201
	lw	$2,8($4)
	bne	$2,$21,$L201
	lw	$2,12($4)
	bne	$2,$20,$L201
	lw	$3,16($4)
	lw	$2,12($18)
	bne	$3,$2,$L201
	lw	$3,20($4)
	lw	$2,16($18)
	bne	$3,$2,$L201
	lw	$2,24($4)
	bne	$2,$17,$L201
	lw	$2,28($4)
	bne	$2,$16,$L201
	lw	$4,160($sp)
	lw	$5,76($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_conntrack_drop_check_hook
	move	$6,$4
	.set	macro
	.set	reorder

#APP
	raw_local_irq_restore	$23
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L193
	move	$3,$0
	.set	macro
	.set	reorder

$L201:
	lw	$4,36($4)
	j	$L294
$L293:
	lw	$8,%lo(napt_list_free)($5)
	beq	$8,$0,$L210
	lw	$2,0($18)
	lw	$4,44($8)
	sw	$2,4($8)
	lw	$2,4($18)
	lw	$3,48($8)
	sw	$2,8($8)
	lw	$2,8($18)
	sw	$2,12($8)
	lw	$2,12($18)
	sw	$2,16($8)
	lw	$2,16($18)
	sw	$2,20($8)
	lw	$2,20($18)
	sw	$2,24($8)
	lw	$2,24($18)
	sw	$fp,32($8)
	sw	$2,28($8)
	li	$2,-1
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L212
	sb	$2,0($8)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L213
	sw	$3,48($4)
	.set	macro
	.set	reorder

$L212:
	addiu	$2,$5,%lo(napt_list_free)
	sw	$3,4($2)
$L213:
	lw	$4,48($8)
	addiu	$5,$5,%lo(napt_list_free)
	lw	$2,44($8)
	lui	$3,%hi(napt_list_inuse)
	addiu	$3,$3,%lo(napt_list_inuse)
	addiu	$6,$8,44
	sw	$2,0($4)
	lw	$2,72($sp)
	addu	$4,$7,$2
	addiu	$7,$8,36
	lw	$2,8($5)
	sll	$4,$4,2
	addiu	$2,$2,-1
	sw	$2,8($5)
	sw	$0,44($8)
	lw	$2,4($3)
	sw	$2,48($8)
	lw	$2,4($3)
	sw	$8,0($2)
	lw	$2,8($3)
	sw	$6,4($3)
	addiu	$2,$2,1
	sw	$2,8($3)
	sw	$0,36($8)
	lw	$2,%lo(table_napt)($9)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$2,40($8)
	lw	$2,%lo(table_napt)($9)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$8,0($2)
	lw	$2,%lo(table_napt)($9)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$7,4($2)
	lw	$2,%lo(table_napt)($9)
	lw	$2,0($2)
	addu	$4,$4,$2
	lw	$2,8($4)
	addiu	$2,$2,1
	sw	$2,8($4)
	li	$2,33
	sb	$2,57($sp)
	li	$2,8
	sb	$2,64($sp)
	sb	$0,56($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl865x_getLanDev
	sb	$0,65($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl865x_getWanDev
	move	$20,$2
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_protocol
	move	$21,$2
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_ip_hdr
	sw	$2,100($sp)
	.set	macro
	.set	reorder

	move	$4,$19
	move	$fp,$2
	lbu	$2,0($2)
	andi	$2,$2,0xf
	sll	$2,$2,2
	addu	$2,$fp,$2
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	sw	$2,80($sp)
	.set	macro
	.set	reorder

	li	$6,6			# 0x6
	addiu	$5,$2,6
	.set	noreorder
	.set	nomacro
	jal	memcpy
	addiu	$4,$sp,24
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	li	$6,6			# 0x6
	move	$5,$2
	.set	noreorder
	.set	nomacro
	jal	memcpy
	addiu	$4,$sp,32
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	move	$4,$19
	sw	$2,104($sp)
	lw	$3,12($fp)
	lw	$5,80($sp)
	sw	$3,84($sp)
	lhu	$5,0($5)
	lw	$9,80($sp)
	sw	$5,92($sp)
	lw	$8,16($fp)
	sw	$8,88($sp)
	lhu	$9,2($9)
	.set	noreorder
	.set	nomacro
	jal	rtl_store_skb_dst
	sw	$9,96($sp)
	.set	macro
	.set	reorder

	lui	$8,%hi(path_list_free)
	lw	$5,4($18)
	lw	$6,20($18)
	lw	$4,8($18)
	srl	$2,$5,8
	srl	$3,$6,16
	xor	$2,$2,$5
	xor	$3,$3,$6
	srl	$4,$4,4
	lw	$5,24($18)
	xor	$2,$2,$3
	xor	$2,$2,$4
	lui	$4,%hi(path_table_list_max)
	xor	$2,$2,$5
	lw	$5,76($sp)
	lw	$3,%lo(path_table_list_max)($4)
	xor	$2,$2,$5
	srl	$4,$2,12
	addiu	$3,$3,-1
	xor	$2,$2,$4
	lw	$16,%lo(path_list_free)($8)
	and	$3,$3,$2
	.set	noreorder
	.set	nomacro
	beq	$16,$0,$L217
	sw	$3,72($sp)
	.set	macro
	.set	reorder

	lw	$2,0($18)
	sw	$2,4($16)
	lw	$2,4($18)
	sw	$2,16($16)
	lw	$2,8($18)
	sw	$2,12($16)
	lw	$2,20($18)
	sw	$2,20($16)
	lw	$2,24($18)
	sw	$2,8($16)
	lw	$2,12($18)
	sw	$2,24($16)
	lw	$2,16($18)
	sw	$2,28($16)
	lw	$4,20($18)
	sw	$4,32($16)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Route
	sw	$2,36($16)
	.set	macro
	.set	reorder

	sw	$2,44($16)
	li	$2,1
	lw	$4,16($16)
	lw	$3,24($16)
	sb	$2,2($16)
	li	$2,-1
	sb	$2,0($16)
	.set	noreorder
	.set	nomacro
	beq	$4,$3,$L218
	sb	$0,1($16)
	.set	macro
	.set	reorder

	li	$2,1			# 0x1
	sb	$2,1($16)
$L218:
	lw	$3,12($16)
	lw	$2,28($16)
	beq	$3,$2,$L219
	lbu	$2,1($16)
	ori	$2,$2,0x2
	sb	$2,1($16)
$L219:
	sw	$0,48($16)
	addiu	$22,$sp,40
	sw	$0,52($16)
	move	$4,$22
	lw	$2,4($18)
	move	$5,$0
	li	$6,14			# 0xe
	sw	$2,12($fp)
	lw	$2,8($18)
	lw	$9,80($sp)
	sh	$2,0($9)
	lw	$2,20($18)
	sw	$2,16($fp)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($9)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	beq	$20,$0,$L220
	lw	$4,4($18)
	move	$5,$20
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$22
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L220
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_ppp_proto_check
	addiu	$5,$sp,56
	.set	macro
	.set	reorder

	move	$17,$2
	li	$2,1			# 0x1
	bne	$17,$2,$L221
	.set	noreorder
	.set	nomacro
	jal	rtl_skb_mac_header_was_set
	move	$4,$19
	.set	macro
	.set	reorder

	bne	$2,$17,$L221
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	lbu	$3,12($2)
	lbu	$2,13($2)
	sll	$3,$3,8
	or	$2,$2,$3
	li	$3,2048			# 0x800
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L221
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_skb_set_mac_header
	li	$5,-22			# 0xffffffea
	.set	macro
	.set	reorder

$L221:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$22
	addiu	$4,$2,6
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_dmac
	move	$5,$20
	.set	macro
	.set	reorder

	li	$2,2048			# 0x800
	lw	$3,100($sp)
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L220
	lui	$2,%hi(nf_hooks+128)
	.set	macro
	.set	reorder

	lw	$3,%lo(nf_hooks+128)($2)
	addiu	$2,$2,%lo(nf_hooks+128)
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L224
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$0
	move	$6,$20
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$21
	.set	macro
	.set	reorder

$L224:
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	beq	$2,$0,$L226
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L298
	sw	$2,48($16)
	.set	macro
	.set	reorder

$L226:
	lw	$2,48($16)
	sw	$2,48($16)
$L298:
	lui	$2,%hi(nf_hooks+160)
	lw	$3,%lo(nf_hooks+160)($2)
	addiu	$2,$2,%lo(nf_hooks+160)
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L229
	move	$4,$19
	.set	macro
	.set	reorder

	li	$5,4			# 0x4
	move	$6,$20
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$21
	.set	macro
	.set	reorder

$L229:
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	beq	$2,$0,$L231
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L220
	sw	$2,52($16)
	.set	macro
	.set	reorder

$L231:
	lw	$2,52($16)
	sw	$2,52($16)
$L220:
	lw	$2,12($18)
	addiu	$22,$sp,40
	move	$4,$22
	move	$5,$0
	sw	$2,12($fp)
	li	$6,14			# 0xe
	lw	$2,16($18)
	lw	$8,80($sp)
	sh	$2,0($8)
	lw	$2,20($18)
	sw	$2,16($fp)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($8)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$21,$0,$L233
	move	$5,$21
	.set	macro
	.set	reorder

	lw	$4,20($18)
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$22
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L233
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_ppp_proto_check
	addiu	$5,$sp,56
	.set	macro
	.set	reorder

	move	$17,$2
	li	$2,1			# 0x1
	bne	$17,$2,$L234
	.set	noreorder
	.set	nomacro
	jal	rtl_skb_mac_header_was_set
	move	$4,$19
	.set	macro
	.set	reorder

	bne	$2,$17,$L234
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	lbu	$3,12($2)
	lbu	$2,13($2)
	sll	$3,$3,8
	or	$2,$2,$3
	li	$3,2048			# 0x800
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L234
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_skb_set_mac_header
	li	$5,-22			# 0xffffffea
	.set	macro
	.set	reorder

$L234:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$22
	move	$4,$2
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_smac
	move	$5,$21
	.set	macro
	.set	reorder

	li	$2,2048			# 0x800
	lw	$9,100($sp)
	.set	noreorder
	.set	nomacro
	bne	$9,$2,$L233
	lui	$2,%hi(nf_hooks+160)
	.set	macro
	.set	reorder

	lw	$3,%lo(nf_hooks+160)($2)
	addiu	$2,$2,%lo(nf_hooks+160)
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L237
	move	$4,$19
	.set	macro
	.set	reorder

	li	$5,4			# 0x4
	move	$6,$20
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$21
	.set	macro
	.set	reorder

$L237:
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	beq	$2,$0,$L239
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L233
	sw	$2,52($16)
	.set	macro
	.set	reorder

$L239:
	lw	$2,52($16)
	sw	$2,52($16)
$L233:
	lw	$3,76($sp)
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L241
	lui	$2,%hi(nf_ct_udp_timeout_stream)
	.set	macro
	.set	reorder

	lw	$4,160($sp)
	jal	rtl_tcp_get_timeouts
	.set	noreorder
	.set	nomacro
	j	$L299
	sw	$2,60($16)
	.set	macro
	.set	reorder

$L241:
	lw	$2,%lo(nf_ct_udp_timeout_stream)($2)
	sw	$2,60($16)
$L299:
	lui	$2,%hi(jiffies)
	lw	$4,160($sp)
	lw	$3,72($16)
	sw	$4,56($16)
	lw	$2,%lo(jiffies)($2)
	lw	$4,76($16)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L244
	sw	$2,40($16)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L245
	sw	$4,76($3)
	.set	macro
	.set	reorder

$L244:
	lui	$5,%hi(path_list_free)
	addiu	$2,$5,%lo(path_list_free)
	sw	$4,4($2)
$L245:
	lw	$3,76($16)
	lui	$5,%hi(path_list_free)
	lw	$2,72($16)
	addiu	$5,$5,%lo(path_list_free)
	addiu	$7,$16,72
	lui	$6,%hi(table_path)
	sw	$2,0($3)
	lui	$3,%hi(path_list_inuse)
	lw	$2,8($5)
	addiu	$3,$3,%lo(path_list_inuse)
	lw	$8,72($sp)
	addiu	$2,$2,-1
	sw	$2,8($5)
	sll	$4,$8,1
	lw	$9,72($sp)
	addiu	$8,$16,64
	sw	$0,72($16)
	lw	$2,4($3)
	addu	$4,$4,$9
	sll	$4,$4,2
	sw	$2,76($16)
	lw	$2,4($3)
	sw	$16,0($2)
	lw	$2,8($3)
	sw	$7,4($3)
	addiu	$2,$2,1
	sw	$2,8($3)
	sw	$0,64($16)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$2,68($16)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$16,0($2)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$8,4($2)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$4,$4,$2
	lw	$2,8($4)
	addiu	$2,$2,1
	sw	$2,8($4)
$L217:
	lw	$5,12($18)
	lui	$8,%hi(path_list_free)
	lw	$6,20($18)
	lw	$4,24($18)
	srl	$3,$5,16
	srl	$2,$6,8
	xor	$3,$3,$5
	xor	$2,$2,$6
	srl	$4,$4,4
	lw	$5,16($18)
	xor	$2,$2,$3
	xor	$2,$2,$4
	lui	$4,%hi(path_table_list_max)
	xor	$2,$2,$5
	lw	$5,76($sp)
	lw	$3,%lo(path_table_list_max)($4)
	xor	$2,$2,$5
	srl	$4,$2,12
	addiu	$3,$3,-1
	xor	$2,$2,$4
	lw	$16,%lo(path_list_free)($8)
	and	$3,$3,$2
	.set	noreorder
	.set	nomacro
	beq	$16,$0,$L249
	sw	$3,72($sp)
	.set	macro
	.set	reorder

	lw	$2,0($18)
	sw	$2,4($16)
	lw	$2,20($18)
	sw	$2,16($16)
	lw	$2,24($18)
	sw	$2,12($16)
	lw	$2,12($18)
	sw	$2,20($16)
	lw	$2,16($18)
	sw	$2,8($16)
	lw	$2,20($18)
	sw	$2,24($16)
	lw	$2,24($18)
	sw	$2,28($16)
	lw	$4,4($18)
	sw	$4,32($16)
	lw	$2,8($18)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Route
	sw	$2,36($16)
	.set	macro
	.set	reorder

	sw	$2,44($16)
	li	$2,2
	lw	$4,20($16)
	lw	$3,32($16)
	sb	$2,2($16)
	li	$2,-1
	sb	$2,0($16)
	.set	noreorder
	.set	nomacro
	beq	$4,$3,$L250
	sb	$0,1($16)
	.set	macro
	.set	reorder

	li	$2,4			# 0x4
	sb	$2,1($16)
$L250:
	lw	$3,8($16)
	lw	$2,36($16)
	beq	$3,$2,$L251
	lbu	$2,1($16)
	ori	$2,$2,0x8
	sb	$2,1($16)
$L251:
	sw	$0,48($16)
	addiu	$22,$sp,40
	sw	$0,52($16)
	move	$4,$22
	lw	$2,20($18)
	move	$5,$0
	li	$6,14			# 0xe
	sw	$2,12($fp)
	lw	$2,24($18)
	lw	$9,80($sp)
	sh	$2,0($9)
	lw	$2,12($18)
	sw	$2,16($fp)
	lw	$2,16($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($9)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	beq	$21,$0,$L252
	lw	$4,20($18)
	move	$5,$21
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$22
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L252
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_ppp_proto_check
	addiu	$5,$sp,56
	.set	macro
	.set	reorder

	move	$17,$2
	li	$2,1			# 0x1
	bne	$17,$2,$L253
	.set	noreorder
	.set	nomacro
	jal	rtl_skb_mac_header_was_set
	move	$4,$19
	.set	macro
	.set	reorder

	bne	$2,$17,$L253
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	lbu	$3,12($2)
	lbu	$2,13($2)
	sll	$3,$3,8
	or	$2,$2,$3
	li	$3,2048			# 0x800
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L253
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_skb_set_mac_header
	li	$5,-22			# 0xffffffea
	.set	macro
	.set	reorder

$L253:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$22
	addiu	$4,$2,6
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_dmac
	move	$5,$21
	.set	macro
	.set	reorder

	li	$2,2048			# 0x800
	lw	$3,100($sp)
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L254
	lui	$2,%hi(nf_hooks+128)
	.set	macro
	.set	reorder

	lw	$3,%lo(nf_hooks+128)($2)
	addiu	$2,$2,%lo(nf_hooks+128)
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L254
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$0
	move	$6,$21
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$20
	.set	macro
	.set	reorder

$L254:
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	beq	$2,$0,$L258
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L252
	sw	$2,48($16)
	.set	macro
	.set	reorder

$L258:
	lw	$2,48($16)
	sw	$2,48($16)
$L252:
	lw	$2,20($18)
	addiu	$22,$sp,40
	move	$4,$22
	move	$5,$0
	sw	$2,12($fp)
	li	$6,14			# 0xe
	lw	$2,24($18)
	lw	$8,80($sp)
	sh	$2,0($8)
	lw	$2,4($18)
	sw	$2,16($fp)
	lw	$2,8($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($8)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$20,$0,$L260
	move	$5,$20
	.set	macro
	.set	reorder

	lw	$4,4($18)
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$22
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L260
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_ppp_proto_check
	addiu	$5,$sp,56
	.set	macro
	.set	reorder

	move	$17,$2
	li	$2,1			# 0x1
	bne	$17,$2,$L261
	.set	noreorder
	.set	nomacro
	jal	rtl_skb_mac_header_was_set
	move	$4,$19
	.set	macro
	.set	reorder

	bne	$2,$17,$L261
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	lbu	$3,12($2)
	lbu	$2,13($2)
	sll	$3,$3,8
	or	$2,$2,$3
	li	$3,2048			# 0x800
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L261
	move	$4,$19
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_skb_set_mac_header
	li	$5,-22			# 0xffffffea
	.set	macro
	.set	reorder

$L261:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$5,$22
	move	$4,$2
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_smac
	move	$5,$20
	.set	macro
	.set	reorder

	li	$2,2048			# 0x800
	lw	$9,100($sp)
	.set	noreorder
	.set	nomacro
	bne	$9,$2,$L260
	lui	$2,%hi(nf_hooks+160)
	.set	macro
	.set	reorder

	lw	$3,%lo(nf_hooks+160)($2)
	addiu	$2,$2,%lo(nf_hooks+160)
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L260
	move	$4,$19
	.set	macro
	.set	reorder

	li	$5,4			# 0x4
	move	$6,$21
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$20
	.set	macro
	.set	reorder

$L260:
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	sw	$2,52($16)
	li	$2,6			# 0x6
	lw	$3,76($sp)
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L266
	lui	$2,%hi(nf_ct_udp_timeout_stream)
	.set	macro
	.set	reorder

	lw	$4,160($sp)
	jal	rtl_tcp_get_timeouts
	.set	noreorder
	.set	nomacro
	j	$L300
	sw	$2,60($16)
	.set	macro
	.set	reorder

$L266:
	lw	$2,%lo(nf_ct_udp_timeout_stream)($2)
	sw	$2,60($16)
$L300:
	lui	$2,%hi(jiffies)
	lw	$4,160($sp)
	lw	$3,72($16)
	sw	$4,56($16)
	lw	$2,%lo(jiffies)($2)
	lw	$4,76($16)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L269
	sw	$2,40($16)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L270
	sw	$4,76($3)
	.set	macro
	.set	reorder

$L269:
	lui	$5,%hi(path_list_free)
	addiu	$2,$5,%lo(path_list_free)
	sw	$4,4($2)
$L270:
	lw	$3,76($16)
	lui	$5,%hi(path_list_free)
	lw	$2,72($16)
	addiu	$5,$5,%lo(path_list_free)
	addiu	$7,$16,72
	lui	$6,%hi(table_path)
	sw	$2,0($3)
	lui	$3,%hi(path_list_inuse)
	lw	$2,8($5)
	addiu	$3,$3,%lo(path_list_inuse)
	lw	$8,72($sp)
	addiu	$2,$2,-1
	sw	$2,8($5)
	sll	$4,$8,1
	lw	$9,72($sp)
	addiu	$8,$16,64
	sw	$0,72($16)
	lw	$2,4($3)
	addu	$4,$4,$9
	sll	$4,$4,2
	sw	$2,76($16)
	lw	$2,4($3)
	sw	$16,0($2)
	lw	$2,8($3)
	sw	$7,4($3)
	addiu	$2,$2,1
	sw	$2,8($3)
	sw	$0,64($16)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$2,68($16)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	lw	$2,4($2)
	sw	$16,0($2)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$8,4($2)
	lw	$2,%lo(table_path)($6)
	lw	$2,0($2)
	addu	$4,$4,$2
	lw	$2,8($4)
	addiu	$2,$2,1
	sw	$2,8($4)
$L249:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	addiu	$5,$sp,24
	li	$6,6			# 0x6
	.set	noreorder
	.set	nomacro
	jal	memcpy
	addiu	$4,$2,6
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	addiu	$5,$sp,32
	move	$4,$2
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	lw	$5,104($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	move	$4,$19
	lw	$2,84($sp)
	sw	$2,12($fp)
	lw	$5,92($sp)
	lw	$3,80($sp)
	sh	$5,0($3)
	lw	$8,88($sp)
	sw	$8,16($fp)
	lw	$9,96($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_dst
	sh	$9,2($3)
	.set	macro
	.set	reorder

	beq	$20,$0,$L273
#APP
	   

	1:	ll	    $2, 452($20)		# atomic_sub		
       nop                                             
	subu	$2, 1					
	sc	    $2, 452($20)					
	beqz	$2, 2f					
	.subsection 2					
2:	b	1b					
	.previous					

	   

#NO_APP
$L273:
	beq	$21,$0,$L276
#APP
	   

	1:	ll	    $2, 452($21)		# atomic_sub		
       nop                                             
	subu	$2, 1					
	sc	    $2, 452($21)					
	beqz	$2, 2f					
	.subsection 2					
2:	b	1b					
	.previous					

	   

#NO_APP
	j	$L276
$L210:
#APP
	raw_local_irq_restore	$23
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L193
	li	$3,-1			# 0xffffffff
	.set	macro
	.set	reorder

$L276:
#APP
	raw_local_irq_restore	$23
#NO_APP
	lw	$4,160($sp)
	lw	$5,76($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_conntrack_drop_check_hook
	move	$6,$4
	.set	macro
	.set	reorder

	move	$3,$0
$L193:
	lw	$31,148($sp)
	move	$2,$3
	lw	$fp,144($sp)
	lw	$23,140($sp)
	lw	$22,136($sp)
	lw	$21,132($sp)
	lw	$20,128($sp)
	lw	$19,124($sp)
	lw	$18,120($sp)
	lw	$17,116($sp)
	lw	$16,112($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,152
	.set	macro
	.set	reorder

	.end	rtk_addNaptConnection
	.section	.text.rtk_delNaptConnection,"ax",@progbits
	.align	2
	.globl	rtk_delNaptConnection
	.ent	rtk_delNaptConnection
	.type	rtk_delNaptConnection, @function
rtk_delNaptConnection:
	.set	nomips16
	.frame	$sp,64,$31		# vars= 0, regs= 9/0, args= 24, gp= 0
	.mask	0x80ff0000,-8
	.fmask	0x00000000,0
	addiu	$sp,$sp,-64
	li	$3,17			# 0x11
	sw	$23,52($sp)
	li	$23,6			# 0x6
	sw	$16,24($sp)
	move	$16,$4
	sw	$31,56($sp)
	sw	$22,48($sp)
	sw	$21,44($sp)
	sw	$20,40($sp)
	sw	$19,36($sp)
	sw	$18,32($sp)
	sw	$17,28($sp)
	lw	$2,0($4)
	xori	$2,$2,0x2
	movn	$23,$3,$2		#conditional move  
#APP
	raw_local_irq_save	$17
#NO_APP
	lw	$4,20($4)
	jal	filter_delconnect
	lw	$22,0($16)
	lw	$21,4($16)
	lw	$20,8($16)
	andi	$2,$22,0xffff
	lw	$19,20($16)
	move	$4,$21
	lw	$18,24($16)
	move	$5,$20
	move	$6,$19
	sw	$2,16($sp)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Hash_NAPT_Entry
	move	$7,$18
	.set	macro
	.set	reorder

	lui	$10,%hi(table_napt)
	lw	$3,%lo(table_napt)($10)
	sll	$8,$2,1
	move	$6,$2
	addu	$2,$8,$2
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$5,0($2)
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L356
	lui	$2,%hi(napt_list_inuse)
	.set	macro
	.set	reorder

	lui	$24,%hi(path_list_inuse)
	addiu	$9,$2,%lo(napt_list_inuse)
	lui	$2,%hi(napt_list_free)
	addiu	$7,$2,%lo(napt_list_free)
$L348:
	lw	$2,4($5)
	bne	$2,$22,$L307
	lw	$2,8($5)
	bne	$2,$21,$L307
	lw	$2,12($5)
	bne	$2,$20,$L307
	lw	$3,16($5)
	lw	$2,12($16)
	bne	$3,$2,$L307
	lw	$3,20($5)
	lw	$2,16($16)
	bne	$3,$2,$L307
	lw	$2,24($5)
	bne	$2,$19,$L307
	lw	$2,28($5)
	bne	$2,$18,$L307
	lw	$2,36($5)
	sb	$0,0($5)
	lw	$4,40($5)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L310
	addu	$3,$8,$6
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L311
	sw	$4,40($2)
	.set	macro
	.set	reorder

$L310:
	lw	$2,%lo(table_napt)($10)
	sll	$3,$3,2
	lw	$2,0($2)
	addu	$3,$3,$2
	sw	$4,4($3)
$L311:
	lw	$2,36($5)
	sll	$3,$6,1
	lw	$4,40($5)
	addu	$3,$3,$6
	sll	$3,$3,2
	sw	$2,0($4)
	lui	$2,%hi(table_napt)
	lw	$2,%lo(table_napt)($2)
	lw	$2,0($2)
	addu	$3,$3,$2
	lw	$2,8($3)
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$2,44($5)
	lw	$3,48($5)
	beq	$2,$0,$L313
	.set	noreorder
	.set	nomacro
	j	$L314
	sw	$3,48($2)
	.set	macro
	.set	reorder

$L313:
	sw	$3,4($9)
$L314:
	lw	$3,48($5)
	addiu	$4,$5,44
	lw	$2,44($5)
	lui	$18,%hi(path_table_list_max)
	lui	$12,%hi(table_path)
	sw	$2,0($3)
	lw	$2,8($9)
	addiu	$2,$2,-1
	sw	$2,8($9)
	sw	$0,44($5)
	lw	$2,4($7)
	sw	$2,48($5)
	lw	$2,4($7)
	sw	$5,0($2)
	lw	$2,8($7)
	sw	$4,4($7)
	addiu	$2,$2,1
	sw	$2,8($7)
	lw	$8,4($16)
	lw	$7,20($16)
	lw	$11,8($16)
	srl	$2,$8,8
	srl	$3,$7,16
	xor	$2,$2,$8
	xor	$3,$3,$7
	srl	$4,$11,4
	xor	$2,$2,$3
	lw	$10,24($16)
	xor	$2,$2,$4
	lw	$4,%lo(path_table_list_max)($18)
	xor	$2,$2,$10
	xor	$2,$2,$23
	addiu	$4,$4,-1
	srl	$3,$2,12
	xor	$2,$2,$3
	and	$6,$4,$2
	lw	$3,%lo(table_path)($12)
	sll	$2,$6,1
	addu	$2,$2,$6
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$3,$2,$3
	lw	$5,0($3)
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L318
	move	$4,$2
	.set	macro
	.set	reorder

	lui	$2,%hi(path_list_free)
	addiu	$6,$2,%lo(path_list_free)
	move	$13,$4
	addiu	$9,$24,%lo(path_list_inuse)
	li	$14,1			# 0x1
	move	$15,$12
$L328:
	lw	$3,4($5)
	lw	$2,0($16)
	bne	$3,$2,$L319
	lbu	$2,2($5)
	bne	$2,$14,$L319
	lw	$2,16($5)
	bne	$2,$8,$L319
	lw	$2,12($5)
	bne	$2,$11,$L319
	lw	$3,24($5)
	lw	$2,12($16)
	bne	$3,$2,$L319
	lw	$3,28($5)
	lw	$2,16($16)
	bne	$3,$2,$L319
	lw	$2,20($5)
	bne	$2,$7,$L319
	lw	$2,8($5)
	bne	$2,$10,$L319
	lw	$2,64($5)
	sb	$0,0($5)
	lw	$3,68($5)
	beq	$2,$0,$L322
	.set	noreorder
	.set	nomacro
	j	$L323
	sw	$3,68($2)
	.set	macro
	.set	reorder

$L322:
	lw	$2,%lo(table_path)($12)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$3,4($2)
$L323:
	lw	$3,68($5)
	lw	$2,64($5)
	sw	$2,0($3)
	lw	$2,%lo(table_path)($15)
	lw	$3,0($2)
	addu	$3,$13,$3
	lw	$2,8($3)
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$3,72($5)
	lw	$2,76($5)
	beq	$3,$0,$L325
	.set	noreorder
	.set	nomacro
	j	$L326
	sw	$2,76($3)
	.set	macro
	.set	reorder

$L325:
	sw	$2,4($9)
$L326:
	lw	$2,72($5)
	addiu	$4,$5,72
	lw	$3,76($5)
	sw	$2,0($3)
	lw	$2,8($9)
	addiu	$2,$2,-1
	sw	$2,8($9)
	sw	$0,72($5)
	lw	$2,4($6)
	sw	$2,76($5)
	lw	$2,4($6)
	sw	$5,0($2)
	lw	$2,8($6)
	sw	$4,4($6)
	addiu	$2,$2,1
	.set	noreorder
	.set	nomacro
	j	$L318
	sw	$2,8($6)
	.set	macro
	.set	reorder

$L319:
	lw	$5,64($5)
	bne	$5,$0,$L328
$L318:
	lw	$8,20($16)
	lui	$10,%hi(table_path)
	lw	$7,12($16)
	lw	$12,24($16)
	srl	$2,$8,8
	srl	$3,$7,16
	xor	$2,$2,$8
	xor	$3,$3,$7
	srl	$4,$12,4
	xor	$2,$2,$3
	lw	$11,16($16)
	xor	$2,$2,$4
	lw	$4,%lo(path_table_list_max)($18)
	xor	$2,$2,$11
	xor	$2,$2,$23
	addiu	$4,$4,-1
	srl	$3,$2,12
	xor	$2,$2,$3
	and	$6,$4,$2
	lw	$3,%lo(table_path)($10)
	sll	$2,$6,1
	addu	$2,$2,$6
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$3,$2,$3
	lw	$5,0($3)
	beq	$5,$0,$L331
	move	$4,$2
	lui	$2,%hi(path_list_free)
	addiu	$9,$24,%lo(path_list_inuse)
	addiu	$6,$2,%lo(path_list_free)
	move	$13,$4
	li	$14,2			# 0x2
	move	$15,$10
$L341:
	lw	$3,4($5)
	lw	$2,0($16)
	bne	$3,$2,$L332
	lbu	$2,2($5)
	bne	$2,$14,$L332
	lw	$2,20($5)
	bne	$2,$7,$L332
	lw	$2,8($5)
	bne	$2,$11,$L332
	lw	$2,24($5)
	bne	$2,$8,$L332
	lw	$2,28($5)
	bne	$2,$12,$L332
	lw	$3,32($5)
	lw	$2,4($16)
	bne	$3,$2,$L332
	lw	$3,36($5)
	lw	$2,8($16)
	bne	$3,$2,$L332
	lw	$2,64($5)
	sb	$0,0($5)
	lw	$3,68($5)
	beq	$2,$0,$L335
	.set	noreorder
	.set	nomacro
	j	$L336
	sw	$3,68($2)
	.set	macro
	.set	reorder

$L335:
	lw	$2,%lo(table_path)($10)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$3,4($2)
$L336:
	lw	$3,68($5)
	lw	$2,64($5)
	sw	$2,0($3)
	lw	$2,%lo(table_path)($15)
	lw	$3,0($2)
	addu	$3,$13,$3
	lw	$2,8($3)
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$3,72($5)
	lw	$2,76($5)
	beq	$3,$0,$L338
	.set	noreorder
	.set	nomacro
	j	$L339
	sw	$2,76($3)
	.set	macro
	.set	reorder

$L338:
	sw	$2,4($9)
$L339:
	lw	$2,72($5)
	addiu	$4,$5,72
	lw	$3,76($5)
	sw	$2,0($3)
	lw	$2,8($9)
	addiu	$2,$2,-1
	sw	$2,8($9)
	sw	$0,72($5)
	lw	$2,4($6)
	sw	$2,76($5)
	lw	$2,4($6)
	sw	$5,0($2)
	lw	$2,8($6)
	sw	$4,4($6)
	addiu	$2,$2,1
	.set	noreorder
	.set	nomacro
	j	$L331
	sw	$2,8($6)
	.set	macro
	.set	reorder

$L332:
	lw	$5,64($5)
	bne	$5,$0,$L341
$L331:
#APP
	raw_local_irq_restore	$17
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L301
	move	$2,$0
	.set	macro
	.set	reorder

$L307:
	lw	$5,36($5)
	bne	$5,$0,$L348
$L356:
#APP
	raw_local_irq_restore	$17
#NO_APP
	li	$2,-4			# 0xfffffffc
$L301:
	lw	$31,56($sp)
	lw	$23,52($sp)
	lw	$22,48($sp)
	lw	$21,44($sp)
	lw	$20,40($sp)
	lw	$19,36($sp)
	lw	$18,32($sp)
	lw	$17,28($sp)
	lw	$16,24($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,64
	.set	macro
	.set	reorder

	.end	rtk_delNaptConnection
	.section	.text.rtk_idleNaptConnection,"ax",@progbits
	.align	2
	.globl	rtk_idleNaptConnection
	.ent	rtk_idleNaptConnection
	.type	rtk_idleNaptConnection, @function
rtk_idleNaptConnection:
	.set	nomips16
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	move	$8,$4
	sw	$16,16($sp)
	li	$13,6			# 0x6
	sw	$31,20($sp)
	lui	$16,%hi(path_table_list_max)
	lw	$10,4($4)
	lui	$25,%hi(table_path)
	lw	$9,20($4)
	move	$15,$5
	lw	$14,8($4)
	srl	$2,$10,8
	srl	$3,$9,16
	xor	$2,$2,$10
	xor	$3,$3,$9
	srl	$4,$14,4
	lw	$12,0($8)
	xor	$2,$2,$3
	lw	$11,24($8)
	xor	$2,$2,$4
	li	$3,17			# 0x11
	xori	$4,$12,0x2
	movn	$13,$3,$4		#conditional move  
	xor	$2,$2,$11
	xor	$2,$2,$13
	lw	$4,%lo(path_table_list_max)($16)
	srl	$3,$2,12
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	lw	$3,%lo(table_path)($25)
	sll	$2,$4,1
	addu	$2,$2,$4
	lw	$4,0($3)
	sll	$2,$2,2
	lui	$3,%hi(jiffies)
	addu	$2,$2,$4
	lw	$24,%lo(jiffies)($3)
	lw	$6,0($2)
$L378:
	beq	$6,$0,$L364
	li	$2,-16777216			# 0xff000000

	lw	$3,4($6)
	ori	$4,$2,0xff00
	bne	$3,$12,$L365
	ori	$7,$2,0x100

	lw	$2,0($6)
	and	$2,$2,$4
	bne	$2,$7,$L365
	nop

	lw	$2,16($6)
	bne	$2,$10,$L365
	nop

	lw	$2,12($6)
	bne	$2,$14,$L365
	nop

	lw	$3,24($6)
	lw	$2,12($8)
	bne	$3,$2,$L365
	nop

	lw	$3,28($6)
	lw	$2,16($8)
	bne	$3,$2,$L365
	nop

	lw	$2,20($6)
	bne	$2,$9,$L365
	nop

	lw	$2,8($6)
	bne	$2,$11,$L365
	subu	$2,$24,$15

	lw	$5,40($6)
	subu	$2,$2,$5
	bgez	$2,$L364
	nop

	j	$L380
	nop

$L365:
	lw	$6,64($6)
	j	$L378
	nop

$L364:
	lw	$11,20($8)
	lw	$10,12($8)
	lw	$14,24($8)
	srl	$2,$11,8
	srl	$3,$10,16
	xor	$2,$2,$11
	xor	$3,$3,$10
	srl	$4,$14,4
	xor	$2,$2,$3
	lw	$12,16($8)
	xor	$2,$2,$4
	lw	$4,%lo(path_table_list_max)($16)
	xor	$2,$2,$12
	xor	$2,$2,$13
	addiu	$4,$4,-1
	srl	$3,$2,12
	xor	$2,$2,$3
	and	$4,$4,$2
	lw	$3,%lo(table_path)($25)
	sll	$2,$4,1
	addu	$2,$2,$4
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$6,0($2)
$L379:
	beq	$6,$0,$L371
	li	$2,-16777216			# 0xff000000

	lw	$4,4($6)
	lw	$3,0($8)
	ori	$9,$2,0xff00
	bne	$4,$3,$L372
	ori	$7,$2,0x200

	lw	$2,0($6)
	and	$2,$2,$9
	bne	$2,$7,$L372
	nop

	lw	$2,20($6)
	bne	$2,$10,$L372
	nop

	lw	$2,8($6)
	bne	$2,$12,$L372
	nop

	lw	$2,24($6)
	bne	$2,$11,$L372
	nop

	lw	$2,28($6)
	bne	$2,$14,$L372
	nop

	lw	$3,32($6)
	lw	$2,4($8)
	bne	$3,$2,$L372
	nop

	lw	$3,36($6)
	lw	$2,8($8)
	bne	$3,$2,$L372
	subu	$2,$24,$15

	lw	$5,40($6)
	subu	$2,$2,$5
	bgez	$2,$L371
	nop

$L380:
	lw	$4,56($6)
	jal	rtl_set_ct_timeout_expires
	addu	$5,$5,$15

	j	$L359
	li	$2,-1			# 0xffffffff

$L372:
	lw	$6,64($6)
	j	$L379
	nop

$L371:
	move	$2,$0
$L359:
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	rtk_idleNaptConnection
	.section	.text.rtl_fpTimer_update,"ax",@progbits
	.align	2
	.globl	rtl_fpTimer_update
	.ent	rtl_fpTimer_update
	.type	rtl_fpTimer_update, @function
rtl_fpTimer_update:
	.set	nomips16
	.frame	$sp,72,$31		# vars= 32, regs= 6/0, args= 16, gp= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-72
	lui	$2,%hi(jiffies)
	sw	$17,52($sp)
	move	$5,$0
	sw	$31,68($sp)
	move	$17,$4
	sw	$20,64($sp)
	sw	$19,60($sp)
	sw	$18,56($sp)
	sw	$16,48($sp)
	lw	$20,%lo(jiffies)($2)
	jal	rtl_get_ct_protonum
	nop

	li	$3,17			# 0x11
	bne	$2,$3,$L382
	move	$4,$17

	jal	rtl_get_ct_udp_status
	li	$19,1			# 0x1

	andi	$2,$2,0x2
	beq	$2,$0,$L383
	lui	$2,%hi(nf_ct_udp_timeout)

	lui	$2,%hi(nf_ct_udp_timeout_stream)
	lw	$18,%lo(nf_ct_udp_timeout_stream)($2)
	j	$L397
	move	$4,$17

$L383:
	lw	$18,%lo(nf_ct_udp_timeout)($2)
	j	$L397
	move	$4,$17

$L382:
	jal	rtl_get_ct_protonum
	move	$5,$0

	li	$3,6			# 0x6
	bne	$2,$3,$L381
	li	$2,-1			# 0xffffffff

	jal	rtl_get_ct_tcp_state
	move	$4,$17

	sltu	$2,$2,6
	beq	$2,$0,$L381
	li	$2,-1			# 0xffffffff

	move	$4,$17
	jal	rtl_tcp_get_timeouts
	li	$19,2			# 0x2

	move	$18,$2
	move	$4,$17
$L397:
	move	$5,$0
	jal	rtl_get_ct_ip_by_dir
	move	$6,$0

	move	$4,$17
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	move	$16,$2

	bne	$16,$2,$L388
	move	$4,$17

	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	sw	$19,16($sp)

	move	$4,$17
	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	sw	$2,20($sp)

	move	$4,$17
	move	$5,$0
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	sw	$2,24($sp)

	move	$4,$17
	move	$5,$0
	li	$6,1			# 0x1
	jal	rtl_get_ct_port_by_dir
	sw	$2,28($sp)

	move	$4,$17
	move	$5,$0
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	sw	$2,32($sp)

	move	$4,$17
	j	$L396
	move	$5,$0

$L388:
	move	$5,$0
	jal	rtl_get_ct_ip_by_dir
	li	$6,1			# 0x1

	move	$4,$17
	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	move	$16,$2

	bne	$16,$2,$L381
	li	$2,-1			# 0xffffffff

	move	$4,$17
	move	$5,$0
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	sw	$19,16($sp)

	move	$4,$17
	move	$5,$0
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	sw	$2,20($sp)

	move	$4,$17
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	sw	$2,24($sp)

	move	$4,$17
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_port_by_dir
	sw	$2,28($sp)

	move	$4,$17
	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	sw	$2,32($sp)

	move	$4,$17
	li	$5,1			# 0x1
$L396:
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	sw	$2,36($sp)

	addiu	$4,$sp,16
	move	$5,$18
	jal	rtk_idleNaptConnection
	sw	$2,40($sp)

	beq	$2,$0,$L393
	addu	$5,$20,$18

	jal	rtl_check_for_acc
	move	$4,$17

	j	$L381
	move	$2,$0

$L393:
	li	$2,-1			# 0xffffffff
$L381:
	lw	$31,68($sp)
	lw	$20,64($sp)
	lw	$19,60($sp)
	lw	$18,56($sp)
	lw	$17,52($sp)
	lw	$16,48($sp)
	j	$31
	addiu	$sp,$sp,72

	.set	macro
	.set	reorder
	.end	rtl_fpTimer_update
	.section	.text.rtl_br_fdb_time_update,"ax",@progbits
	.align	2
	.globl	rtl_br_fdb_time_update
	.ent	rtl_br_fdb_time_update
	.type	rtl_br_fdb_time_update, @function
rtl_br_fdb_time_update:
	.set	nomips16
	.frame	$sp,40,$31		# vars= 8, regs= 3/0, args= 16, gp= 0
	.mask	0x80030000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-40
	sw	$17,28($sp)
	move	$17,$4
	sw	$16,24($sp)
	move	$4,$0
	move	$16,$5
	move	$5,$6
	sw	$31,32($sp)
	jal	rtl_get_hw_fdb_age
	li	$6,2			# 0x2

	move	$3,$2
	sltu	$5,$2,301
	li	$2,300			# 0x12c
	beq	$3,$2,$L401
	move	$4,$16

	beq	$5,$0,$L404
	li	$2,450			# 0x1c2

	li	$2,150			# 0x96
	beq	$3,$2,$L402
	lui	$2,%hi(jiffies)

	j	$L399
	nop

$L404:
	bne	$3,$2,$L399
	move	$4,$16

	lui	$2,%hi(jiffies)
	lw	$5,%lo(jiffies)($2)
	j	$L406
	nop

$L401:
	lui	$2,%hi(jiffies)
	lw	$5,%lo(jiffies)($2)
	j	$L406
	addiu	$5,$5,-15000

$L402:
	lw	$5,%lo(jiffies)($2)
	addiu	$5,$5,-30000
$L406:
	jal	rtl_set_fdb_aging
	nop

$L399:
	jal	rtl_get_fdb_aging
	move	$4,$16

	move	$4,$17
	jal	rtl_hold_time
	move	$16,$2

	lui	$3,%hi(jiffies)
	lw	$3,%lo(jiffies)($3)
	addu	$16,$16,$2
	lw	$31,32($sp)
	li	$2,-1			# 0xffffffff
	subu	$3,$3,$16
	slt	$3,$3,0
	lw	$17,28($sp)
	movn	$2,$0,$3		#conditional move
	lw	$16,24($sp)
	j	$31
	addiu	$sp,$sp,40

	.set	macro
	.set	reorder
	.end	rtl_br_fdb_time_update
	.section	.text.mark_path_invalid,"ax",@progbits
	.align	2
	.ent	mark_path_invalid
	.type	mark_path_invalid, @function
mark_path_invalid:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	srl	$2,$4,8
	srl	$3,$6,16
	xor	$2,$2,$4
	xor	$3,$3,$6
	andi	$5,$5,0xffff
	xor	$2,$2,$3
	move	$11,$4
	srl	$4,$5,4
	xor	$2,$2,$4
	andi	$7,$7,0xffff
	lhu	$10,18($sp)
	lui	$12,%hi(path_table_list_max)
	xor	$2,$2,$7
	lui	$13,%hi(table_path)
	xor	$2,$2,$10
	lw	$3,%lo(path_table_list_max)($12)
	srl	$4,$2,12
	xor	$2,$2,$4
	addiu	$3,$3,-1
	and	$4,$3,$2
	lw	$3,%lo(table_path)($13)
	sll	$2,$4,1
	addu	$2,$2,$4
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$4,0($2)
	beq	$4,$0,$L430
	nop

	li	$3,255			# 0xff
$L415:
	lw	$2,12($4)
	bne	$2,$5,$L411
	nop

	lw	$2,8($4)
	bne	$2,$7,$L411
	nop

	lw	$2,16($4)
	bne	$2,$11,$L411
	nop

	lw	$2,20($4)
	bne	$2,$6,$L411
	nop

	lbu	$2,0($4)
	bne	$2,$3,$L411
	li	$2,1			# 0x1

	lbu	$3,2($4)
	beq	$3,$2,$L428
	sb	$0,0($4)

	lw	$8,32($4)
	lhu	$9,38($4)
	j	$L410
	nop

$L411:
	lw	$4,64($4)
	bne	$4,$0,$L415
	nop

$L431:
	j	$31
	nop

$L410:
	beq	$8,$0,$L431
	li	$2,1			# 0x1

	lbu	$3,2($4)
	bne	$3,$2,$L417
	srl	$3,$11,16

	srl	$2,$6,8
	srl	$3,$8,16
	xor	$3,$3,$8
	xor	$2,$2,$6
	xor	$2,$2,$3
	srl	$3,$7,4
	xor	$2,$2,$3
	j	$L429
	xor	$2,$2,$9

$L428:
	lw	$8,24($4)
	lhu	$9,30($4)
	j	$L410
	nop

$L417:
	srl	$2,$8,8
	xor	$3,$3,$11
	xor	$2,$2,$8
	xor	$2,$2,$3
	srl	$3,$9,4
	xor	$2,$2,$3
	xor	$2,$2,$5
$L429:
	xor	$2,$2,$10
	lw	$3,%lo(path_table_list_max)($12)
	srl	$4,$2,12
	xor	$2,$2,$4
	addiu	$3,$3,-1
	and	$4,$3,$2
	lw	$2,%lo(table_path)($13)
	sll	$3,$4,1
	addu	$3,$3,$4
	lw	$2,0($2)
	sll	$3,$3,2
	addu	$3,$3,$2
	lw	$4,0($3)
	beq	$4,$0,$L430
	nop

	li	$3,255			# 0xff
$L425:
	lw	$2,24($4)
	bne	$2,$6,$L423
	nop

	lw	$2,28($4)
	bne	$2,$7,$L423
	nop

	lw	$2,32($4)
	bne	$2,$11,$L423
	nop

	lw	$2,36($4)
	bne	$2,$5,$L423
	nop

	lbu	$2,0($4)
	bne	$2,$3,$L423
	nop

	sb	$0,0($4)
$L423:
	lw	$4,64($4)
	bne	$4,$0,$L425
	nop

$L430:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	mark_path_invalid
	.section	.text.rtl_fp_mark_invalid,"ax",@progbits
	.align	2
	.globl	rtl_fp_mark_invalid
	.ent	rtl_fp_mark_invalid
	.type	rtl_fp_mark_invalid, @function
rtl_fp_mark_invalid:
	.set	nomips16
	.frame	$sp,72,$31		# vars= 8, regs= 10/0, args= 24, gp= 0
	.mask	0xc0ff0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-72
	li	$5,1			# 0x1
	sw	$16,32($sp)
	move	$6,$0
	move	$16,$4
	sw	$31,68($sp)
	sw	$fp,64($sp)
	sw	$23,60($sp)
	sw	$22,56($sp)
	sw	$21,52($sp)
	sw	$20,48($sp)
	sw	$19,44($sp)
	sw	$18,40($sp)
	jal	rtl_get_ct_ip_by_dir
	sw	$17,36($sp)

	move	$4,$16
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	sw	$2,24($sp)

	move	$4,$16
	move	$5,$0
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	move	$fp,$2

	move	$4,$16
	move	$5,$0
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	move	$23,$2

	move	$4,$16
	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	move	$22,$2

	move	$4,$16
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_port_by_dir
	move	$21,$2

	move	$4,$16
	move	$5,$0
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	move	$20,$2

	li	$6,1			# 0x1
	move	$4,$16
	move	$5,$0
	jal	rtl_get_ct_port_by_dir
	move	$19,$2

	move	$4,$16
	li	$5,1			# 0x1
	jal	rtl_get_ct_protonum
	move	$18,$2

	move	$4,$16
	move	$5,$0
	jal	rtl_get_ct_protonum
	move	$17,$2

	move	$5,$21
	lw	$4,24($sp)
	move	$6,$fp
	move	$7,$20
	move	$16,$2
	jal	mark_path_invalid
	sw	$17,16($sp)

	move	$4,$23
	move	$5,$19
	move	$6,$22
	move	$7,$18
	jal	mark_path_invalid
	sw	$16,16($sp)

	lw	$31,68($sp)
	lw	$fp,64($sp)
	lw	$23,60($sp)
	lw	$22,56($sp)
	lw	$21,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	j	$31
	addiu	$sp,$sp,72

	.set	macro
	.set	reorder
	.end	rtl_fp_mark_invalid
	.rdata
	.align	2
$LC0:
	.ascii	"lo\000"
	.section	.text.enter_fast_path,"ax",@progbits
	.align	2
	.globl	enter_fast_path
	.ent	enter_fast_path
	.type	enter_fast_path, @function
enter_fast_path:
	.set	nomips16
	.frame	$sp,88,$31		# vars= 24, regs= 10/0, args= 24, gp= 0
	.mask	0xc0ff0000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-88
	li	$2,1			# 0x1
	sw	$31,84($sp)
	sw	$22,72($sp)
	move	$22,$0
	sw	$18,56($sp)
	sw	$16,48($sp)
	sw	$4,24($sp)
	sw	$fp,80($sp)
	sw	$23,76($sp)
	sw	$21,68($sp)
	sw	$20,64($sp)
	sw	$19,60($sp)
	sw	$17,52($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_dev_name
	sw	$2,36($sp)
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_ip_hdr
	move	$16,$2
	.set	macro
	.set	reorder

	move	$4,$0
	sw	$0,40($sp)
	move	$18,$2
	lbu	$2,8($2)
	sltu	$2,$2,2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L433
	sw	$0,44($sp)
	.set	macro
	.set	reorder

	lbu	$19,9($18)
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L435
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L433
$L435:
	lbu	$2,0($18)
	move	$4,$18
	lw	$6,24($sp)
	andi	$2,$2,0xf
	sll	$2,$2,2
	addu	$21,$18,$2
	.set	noreorder
	.set	nomacro
	jal	fast_path_pre_process_check
	move	$5,$21
	.set	macro
	.set	reorder

	move	$5,$2
	move	$4,$2
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	bne	$5,$2,$L433
	lui	$3,%hi($LC0)
	.set	macro
	.set	reorder

	addiu	$2,$3,%lo($LC0)
#APP
	.set	noreorder
	.set	noat
	lbu	$3,($16)
1:	lbu	$1,($2)
	addiu	$16,1
	bne	$1,$3,2f
	addiu	$2,1
	bnez	$3,1b
	lbu	$3,($16)
	move	$3,$1
2:	subu	$3,$1
3:	.set	at
	.set	reorder
#NO_APP
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L433
	move	$4,$0
	.set	macro
	.set	reorder

	lhu	$3,0($21)
	lhu	$16,6($18)
	lw	$20,12($18)
	lw	$23,16($18)
	sw	$3,28($sp)
	lhu	$4,2($21)
	.set	noreorder
	.set	nomacro
	bne	$19,$5,$L448
	sw	$4,32($sp)
	.set	macro
	.set	reorder

	andi	$2,$16,0x3fff
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L443
	li	$3,1			# 0x1
	.set	macro
	.set	reorder

	xori	$2,$16,0x2000
	sltu	$3,$2,1
$L443:
	li	$2,1			# 0x1
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L444
	li	$2,65536			# 0x10000
	.set	macro
	.set	reorder

	lw	$3,12($21)
	and	$2,$3,$2
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L445
	li	$2,393216			# 0x60000
	.set	macro
	.set	reorder

	lw	$5,28($sp)
	move	$4,$20
	lw	$7,32($sp)
	move	$6,$23
	.set	noreorder
	.set	nomacro
	jal	mark_path_invalid
	sw	$19,16($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L433
	move	$4,$0
	.set	macro
	.set	reorder

$L445:
	and	$2,$3,$2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L433
	move	$4,$0
	.set	macro
	.set	reorder

$L444:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L448
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	bne	$3,$2,$L448
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

$L448:
	lhu	$2,6($18)
	andi	$2,$2,0x3fff
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L449
	li	$2,8192			# 0x2000
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$16,$2,$L450
	andi	$2,$16,0x2000
	.set	macro
	.set	reorder

	li	$2,1			# 0x1
	.set	noreorder
	.set	nomacro
	j	$L451
	sw	$2,40($sp)
	.set	macro
	.set	reorder

$L450:
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L451
	andi	$3,$16,0x1fff
	.set	macro
	.set	reorder

	lw	$4,44($sp)
	li	$2,1			# 0x1
	movn	$4,$2,$3		#conditional move
	sw	$4,44($sp)
$L451:
	lw	$8,12($18)
	andi	$6,$19,0x00ff
	lw	$7,16($18)
	lhu	$5,4($18)
	xor	$3,$8,$7
	srl	$2,$3,16
	xor	$2,$2,$5
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lw	$4,%lo(max_udp_frag_entry)($2)
	srl	$2,$3,8
	xor	$2,$2,$6
	xor	$3,$3,$2
	addiu	$4,$4,-1
	lui	$2,%hi(udp_cache_table)
	and	$3,$3,$4
	lw	$4,%lo(udp_cache_table)($2)
	sll	$2,$3,1
	addu	$2,$2,$3
	lw	$3,0($4)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$4,0($2)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L460
	move	$22,$0
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
$L459:
	lhu	$2,2($4)
	bne	$2,$5,$L456
	lw	$2,4($4)
	bne	$2,$8,$L456
	lw	$2,8($4)
	bne	$2,$7,$L456
	lbu	$2,1($4)
	bne	$2,$6,$L456
	lbu	$2,0($4)
	beq	$2,$3,$L591
$L456:
	lw	$4,40($4)
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L459
	move	$22,$0
	.set	macro
	.set	reorder

	j	$L460
$L591:
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L460
	move	$22,$4
	.set	macro
	.set	reorder

	lhu	$2,12($4)
	sw	$2,28($sp)
	lhu	$3,14($4)
	lw	$4,40($sp)
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L449
	sw	$3,32($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L449
	sw	$0,36($sp)
	.set	macro
	.set	reorder

$L460:
	lw	$2,40($sp)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L433
	move	$4,$0
	.set	macro
	.set	reorder

$L449:
	lw	$4,28($sp)
	srl	$2,$20,8
	srl	$fp,$23,16
	xor	$2,$2,$20
	xor	$3,$fp,$23
	xor	$2,$2,$3
	srl	$3,$4,4
	xor	$2,$2,$3
	lw	$3,32($sp)
	xor	$2,$2,$3
	lui	$3,%hi(path_table_list_max)
	xor	$2,$2,$19
	lw	$4,%lo(path_table_list_max)($3)
	srl	$3,$2,12
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	lui	$2,%hi(table_path)
	lw	$3,%lo(table_path)($2)
	sll	$2,$4,1
	addu	$2,$2,$4
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$17,0($2)
$L597:
	beq	$17,$0,$L590
	lw	$2,12($17)
	lw	$4,28($sp)
	bne	$2,$4,$L467
	lw	$2,8($17)
	lw	$3,32($sp)
	bne	$2,$3,$L467
	lw	$2,16($17)
	bne	$2,$20,$L467
	lw	$2,20($17)
	.set	noreorder
	.set	nomacro
	bne	$2,$23,$L467
	li	$2,255			# 0xff
	.set	macro
	.set	reorder

	lbu	$3,0($17)
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L467
	li	$2,2			# 0x2
	.set	macro
	.set	reorder

	lw	$3,4($17)
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L607
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L469
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

$L607:
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L467
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L467
$L469:
	lw	$4,24($sp)
	lw	$5,32($17)
	lbu	$7,1($18)
	.set	noreorder
	.set	nomacro
	jal	rtl_ip_route_input
	move	$6,$20
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L433
	move	$4,$0
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	jal	rtl_skb_dst_check
	li	$3,-1			# 0xffffffff
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L593
	li	$2,6			# 0x6
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L474
	addiu	$16,$21,16
	.set	macro
	.set	reorder

	lhu	$3,6($21)
	addiu	$2,$21,6
	move	$16,$0
	movn	$16,$2,$3		#conditional move  
$L474:
	lw	$4,40($sp)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L477
	xor	$3,$20,$23
	.set	macro
	.set	reorder

	lui	$2,%hi(max_udp_frag_entry)
	lhu	$11,4($18)
	srl	$4,$3,16
	lw	$5,%lo(max_udp_frag_entry)($2)
	andi	$9,$19,0x00ff
	xor	$4,$4,$11
	lui	$6,%hi(udp_cache_list_free)
	xor	$3,$3,$4
	addiu	$5,$5,-1
	lw	$4,28($sp)
	srl	$2,$3,8
	xor	$2,$2,$9
	andi	$10,$4,0xffff
	xor	$3,$3,$2
	lw	$4,32($sp)
	and	$12,$3,$5
	lw	$8,%lo(udp_cache_list_free)($6)
	.set	noreorder
	.set	nomacro
	beq	$8,$0,$L593
	andi	$7,$4,0xffff
	.set	macro
	.set	reorder

	lw	$4,48($8)
	li	$2,1
	sh	$11,2($8)
	sb	$9,1($8)
	sh	$10,12($8)
	sh	$7,14($8)
	sb	$2,0($8)
	sw	$20,4($8)
	sw	$23,8($8)
	lw	$3,52($8)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L483
	addiu	$2,$6,%lo(udp_cache_list_free)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L484
	sw	$3,52($4)
	.set	macro
	.set	reorder

$L483:
	sw	$3,4($2)
$L484:
	lw	$4,52($8)
	addiu	$6,$6,%lo(udp_cache_list_free)
	lw	$2,48($8)
	lui	$5,%hi(udp_cache_table)
	sll	$3,$12,1
	addiu	$7,$8,40
	sw	$2,0($4)
	addu	$3,$3,$12
	lw	$2,8($6)
	sll	$3,$3,2
	addiu	$4,$8,16
	addiu	$2,$2,-1
	sw	$2,8($6)
	sw	$0,40($8)
	lw	$2,%lo(udp_cache_table)($5)
	lw	$2,0($2)
	addu	$2,$3,$2
	lw	$2,4($2)
	sw	$2,44($8)
	lw	$2,%lo(udp_cache_table)($5)
	lw	$2,0($2)
	addu	$2,$3,$2
	lw	$2,4($2)
	sw	$8,0($2)
	lw	$2,%lo(udp_cache_table)($5)
	lw	$2,0($2)
	addu	$2,$3,$2
	sw	$7,4($2)
	lw	$2,%lo(udp_cache_table)($5)
	lw	$2,0($2)
	addu	$3,$3,$2
	lw	$2,8($3)
	addiu	$2,$2,1
	sw	$2,8($3)
	lui	$2,%hi(jiffies)
	lw	$2,%lo(jiffies)($2)
	addiu	$2,$2,1000
	.set	noreorder
	.set	nomacro
	jal	add_timer
	sw	$2,24($8)
	.set	macro
	.set	reorder

	j	$L486
$L477:
	lw	$2,44($sp)
	beq	$2,$0,$L486
	.set	noreorder
	.set	nomacro
	jal	del_timer
	addiu	$4,$22,16
	.set	macro
	.set	reorder

	lw	$2,8($22)
	lw	$3,4($22)
	lhu	$4,2($22)
	xor	$3,$3,$2
	srl	$2,$3,16
	xor	$2,$2,$4
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lbu	$5,1($22)
	srl	$4,$3,8
	lw	$2,%lo(max_udp_frag_entry)($2)
	xor	$4,$4,$5
	lw	$6,40($22)
	xor	$3,$3,$4
	addiu	$2,$2,-1
	sb	$0,0($22)
	lw	$4,44($22)
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L490
	and	$5,$3,$2
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L491
	sw	$4,44($6)
	.set	macro
	.set	reorder

$L490:
	lui	$3,%hi(udp_cache_table)
	sll	$2,$5,1
	lw	$3,%lo(udp_cache_table)($3)
	addu	$2,$2,$5
	sll	$2,$2,2
	lw	$3,0($3)
	addu	$2,$2,$3
	sw	$4,4($2)
$L491:
	lw	$4,44($22)
	sll	$3,$5,1
	lw	$2,40($22)
	addu	$3,$3,$5
	sll	$3,$3,2
	addiu	$5,$22,48
	sw	$2,0($4)
	lui	$2,%hi(udp_cache_table)
	lw	$2,%lo(udp_cache_table)($2)
	lui	$4,%hi(udp_cache_list_free)
	addiu	$4,$4,%lo(udp_cache_list_free)
	lw	$2,0($2)
	addu	$3,$3,$2
	lw	$2,8($3)
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,48($22)
	lw	$2,4($4)
	sw	$2,52($22)
	lw	$2,4($4)
	sw	$22,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	sw	$2,8($4)
$L486:
	lw	$4,24($sp)
	move	$5,$18
	.set	noreorder
	.set	nomacro
	jal	fast_path_before_nat_check
	move	$6,$19
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L433
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

	lbu	$3,1($17)
	sltu	$2,$3,13
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L558
	sll	$2,$3,2
	.set	macro
	.set	reorder

	lui	$3,%hi($L581)
	addiu	$3,$3,%lo($L581)
	addu	$2,$2,$3
	lw	$2,0($2)
	lhu	$4,10($18)
	j	$2
	.rdata
	.align	2
$L581:
	.word	$L496
	.word	$L498
	.word	$L514
	.word	$L514
	.word	$L529
	.word	$L558
	.word	$L558
	.word	$L558
	.word	$L544
	.word	$L558
	.word	$L558
	.word	$L558
	.word	$L544
	.section	.text.enter_fast_path
$L498:
	lw	$3,24($17)
	srl	$2,$20,16
	andi	$6,$20,0xffff
	andi	$4,$4,0xffff
	addu	$6,$6,$2
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	subu	$6,$6,$3
	addu	$6,$6,$4
	.set	noreorder
	.set	nomacro
	bgez	$6,$L500
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L499
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L500:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,10($18)
$L499:
	lw	$3,36($sp)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L518
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L503
	beq	$16,$0,$L518
	lhu	$6,0($16)
	beq	$6,$0,$L518
	lw	$2,12($18)
	lw	$3,24($17)
	srl	$4,$2,16
	andi	$5,$2,0xffff
	addu	$5,$5,$4
	andi	$2,$3,0xffff
	subu	$5,$5,$2
	srl	$3,$3,16
	subu	$5,$5,$3
	addu	$5,$5,$6
	.set	noreorder
	.set	nomacro
	bgez	$5,$L506
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L518
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L506:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	.set	noreorder
	.set	nomacro
	j	$L518
	sh	$5,0($16)
	.set	macro
	.set	reorder

$L503:
	lw	$2,12($18)
	lw	$3,24($17)
	srl	$4,$2,16
	andi	$5,$2,0xffff
	addu	$5,$5,$4
	andi	$2,$3,0xffff
	subu	$5,$5,$2
	srl	$3,$3,16
	lhu	$2,0($16)
	subu	$5,$5,$3
	addu	$5,$5,$2
	.set	noreorder
	.set	nomacro
	bgez	$5,$L510
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L509
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L510:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,0($16)
$L509:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L518
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	bne	$3,$2,$L518
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

	j	$L518
$L514:
	lw	$3,24($17)
	srl	$2,$20,16
	andi	$6,$20,0xffff
	andi	$4,$4,0xffff
	addu	$6,$6,$2
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	subu	$6,$6,$3
	addu	$6,$6,$4
	.set	noreorder
	.set	nomacro
	bgez	$6,$L516
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L515
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L516:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,10($18)
$L515:
	lw	$4,36($sp)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L518
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L519
	beq	$16,$0,$L524
	lhu	$6,0($16)
	beq	$6,$0,$L524
	lw	$2,12($18)
	lw	$3,24($17)
	srl	$4,$2,16
	andi	$5,$2,0xffff
	addu	$5,$5,$4
	andi	$2,$3,0xffff
	subu	$5,$5,$2
	srl	$3,$3,16
	lhu	$4,0($21)
	subu	$5,$5,$3
	lw	$2,28($17)
	addu	$5,$5,$4
	subu	$5,$5,$2
	addu	$5,$5,$6
	.set	noreorder
	.set	nomacro
	bgez	$5,$L522
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L524
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L522:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	.set	noreorder
	.set	nomacro
	j	$L524
	sh	$5,0($16)
	.set	macro
	.set	reorder

$L519:
	lw	$2,12($18)
	lw	$3,24($17)
	srl	$4,$2,16
	andi	$5,$2,0xffff
	addu	$5,$5,$4
	andi	$2,$3,0xffff
	subu	$5,$5,$2
	srl	$3,$3,16
	lhu	$2,0($21)
	subu	$5,$5,$3
	lw	$3,28($17)
	addu	$5,$5,$2
	lhu	$2,0($16)
	subu	$5,$5,$3
	addu	$5,$5,$2
	.set	noreorder
	.set	nomacro
	bgez	$5,$L526
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L525
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L526:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,0($16)
$L525:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L524
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	bne	$3,$2,$L524
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

$L524:
	lw	$2,28($17)
	sh	$2,0($21)
$L518:
	lw	$2,24($17)
	.set	noreorder
	.set	nomacro
	j	$L496
	sw	$2,12($18)
	.set	macro
	.set	reorder

$L529:
	lw	$2,32($17)
	andi	$6,$23,0xffff
	addu	$5,$6,$fp
	andi	$4,$4,0xffff
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$5,$5,$3
	subu	$5,$5,$2
	addu	$5,$5,$4
	.set	noreorder
	.set	nomacro
	bgez	$5,$L531
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L530
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L531:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L530:
	lw	$2,36($sp)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L601
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L534
	beq	$16,$0,$L601
	lhu	$4,0($16)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L601
	addu	$6,$6,$fp
	.set	macro
	.set	reorder

	lw	$2,32($17)
	.set	noreorder
	.set	nomacro
	j	$L608
	andi	$3,$2,0xffff
	.set	macro
	.set	reorder

$L534:
	lw	$2,32($17)
	addu	$6,$6,$fp
	lhu	$4,0($16)
	andi	$3,$2,0xffff
$L608:
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	addu	$6,$6,$4
	.set	noreorder
	.set	nomacro
	bgez	$6,$L541
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L601
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L541:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L601
	sh	$6,0($16)
	.set	macro
	.set	reorder

$L544:
	lw	$2,32($17)
	andi	$6,$23,0xffff
	addu	$5,$6,$fp
	andi	$4,$4,0xffff
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$5,$5,$3
	subu	$5,$5,$2
	addu	$5,$5,$4
	.set	noreorder
	.set	nomacro
	bgez	$5,$L546
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L545
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L546:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L545:
	lw	$3,36($sp)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L601
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L549
	beq	$16,$0,$L554
	lhu	$5,0($16)
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L554
	addu	$6,$6,$fp
	.set	macro
	.set	reorder

	lw	$2,32($17)
	lhu	$4,2($21)
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	lw	$2,36($17)
	addu	$6,$6,$4
	subu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L604
	addu	$6,$6,$5
	.set	macro
	.set	reorder

$L549:
	lw	$2,32($17)
	addu	$6,$6,$fp
	lhu	$4,2($21)
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	lw	$3,36($17)
	addu	$6,$6,$4
	lhu	$2,0($16)
	subu	$6,$6,$3
	addu	$6,$6,$2
$L604:
	.set	noreorder
	.set	nomacro
	bgez	$6,$L556
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L554
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L556:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L554:
	lw	$2,36($17)
	.set	noreorder
	.set	nomacro
	j	$L601
	sh	$2,2($21)
	.set	macro
	.set	reorder

$L558:
	lw	$2,24($17)
	srl	$6,$20,16
	andi	$4,$20,0xffff
	andi	$3,$2,0xffff
	addu	$5,$4,$6
	subu	$5,$5,$3
	srl	$2,$2,16
	lhu	$3,10($18)
	subu	$5,$5,$2
	addu	$5,$5,$3
	.set	noreorder
	.set	nomacro
	bgez	$5,$L560
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L559
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L560:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L559:
	lw	$2,32($17)
	andi	$7,$23,0xffff
	addu	$5,$7,$fp
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$5,$5,$3
	lhu	$3,10($18)
	subu	$5,$5,$2
	addu	$5,$5,$3
	.set	noreorder
	.set	nomacro
	bgez	$5,$L563
	andi	$2,$5,0xffff
	.set	macro
	.set	reorder

	subu	$5,$0,$5
	sra	$2,$5,16
	andi	$3,$5,0xffff
	addu	$5,$2,$3
	sra	$2,$5,16
	addu	$5,$5,$2
	nor	$2,$0,$5
	.set	noreorder
	.set	nomacro
	j	$L562
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L563:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L562:
	lw	$2,36($sp)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L565
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L566
	beq	$16,$0,$L574
	lhu	$5,0($16)
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L574
	addu	$6,$4,$6
	.set	macro
	.set	reorder

	lw	$2,24($17)
	lhu	$4,0($21)
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	lw	$2,28($17)
	addu	$6,$6,$4
	subu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L606
	addu	$6,$6,$5
	.set	macro
	.set	reorder

$L566:
	lw	$2,24($17)
	addu	$6,$4,$6
	lhu	$4,0($21)
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	lw	$3,28($17)
	addu	$6,$6,$4
	lhu	$2,0($16)
	subu	$6,$6,$3
	addu	$6,$6,$2
$L606:
	.set	noreorder
	.set	nomacro
	bgez	$6,$L576
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L575
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L576:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L575:
	lw	$2,32($17)
	addu	$6,$7,$fp
	lhu	$4,2($21)
	andi	$3,$2,0xffff
	srl	$2,$2,16
	subu	$6,$6,$3
	subu	$6,$6,$2
	lw	$3,36($17)
	addu	$6,$6,$4
	lhu	$2,0($16)
	subu	$6,$6,$3
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	bgez	$6,$L579
	andi	$2,$6,0xffff
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L574
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L579:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L574:
	lw	$2,28($17)
	sh	$2,0($21)
	lw	$2,36($17)
	sh	$2,2($21)
$L565:
	lw	$2,24($17)
	sw	$2,12($18)
$L601:
	lw	$2,32($17)
	sw	$2,16($18)
$L496:
	lbu	$2,8($18)
	lbu	$4,9($18)
	addiu	$5,$2,-1
	andi	$2,$2,0x00ff
	andi	$3,$5,0x00ff
	sll	$2,$2,8
	sll	$3,$3,8
	or	$6,$2,$4
	or	$3,$3,$4
	lhu	$2,10($18)
	subu	$6,$6,$3
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	bgez	$6,$L583
	sb	$5,8($18)
	.set	macro
	.set	reorder

	subu	$6,$0,$6
	sra	$2,$6,16
	andi	$3,$6,0xffff
	addu	$6,$2,$3
	sra	$2,$6,16
	addu	$6,$6,$2
	nor	$2,$0,$6
	.set	noreorder
	.set	nomacro
	j	$L582
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L583:
	andi	$2,$6,0xffff
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,10($18)
$L582:
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_ip_summed
	move	$5,$0
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_dev
	move	$5,$0
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	lw	$5,48($17)
	lw	$6,52($17)
	jal	fastpath_set_qos_mark
	lui	$2,%hi(jiffies)
	lw	$2,%lo(jiffies)($2)
	move	$4,$18
	lw	$6,24($sp)
	move	$5,$21
	.set	noreorder
	.set	nomacro
	jal	fast_path_post_process_xmit_check
	sw	$2,40($17)
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L433
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	jal	ip_finish_output3
	.set	noreorder
	.set	nomacro
	j	$L433
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

$L467:
	lw	$17,64($17)
	j	$L597
$L590:
	lw	$4,24($sp)
	jal	rtl_set_skb_inDev
	move	$4,$18
	lw	$6,24($sp)
	move	$5,$21
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
	.set	noreorder
	.set	nomacro
	j	fast_path_post_process_return_check
	addiu	$sp,$sp,88
	.set	macro
	.set	reorder

$L593:
	lw	$4,24($sp)
	jal	rtl_dst_release
	move	$4,$0
$L433:
	lw	$31,84($sp)
	move	$2,$4
	lw	$fp,80($sp)
	lw	$23,76($sp)
	lw	$22,72($sp)
	lw	$21,68($sp)
	lw	$20,64($sp)
	lw	$19,60($sp)
	lw	$18,56($sp)
	lw	$17,52($sp)
	lw	$16,48($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,88
	.set	macro
	.set	reorder

	.end	enter_fast_path
	.rdata
	.align	2
$LC1:
	.ascii	"napt num: %d, path num: %d.\n\000"
	.section	.text.fastpath_dump_napt_entry_num,"ax",@progbits
	.align	2
	.globl	fastpath_dump_napt_entry_num
	.ent	fastpath_dump_napt_entry_num
	.type	fastpath_dump_napt_entry_num, @function
fastpath_dump_napt_entry_num:
	.set	nomips16
	.frame	$sp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	lui	$2,%hi(path_list_inuse)
	sw	$16,16($sp)
	move	$7,$0
	sw	$31,20($sp)
	move	$16,$5
	lw	$2,%lo(path_list_inuse)($2)
$L622:
	beq	$2,$0,$L619
	move	$6,$0

	lw	$2,72($2)
	j	$L622
	addiu	$7,$7,1

$L619:
	lui	$2,%hi(napt_list_inuse)
	lw	$2,%lo(napt_list_inuse)($2)
$L623:
	beq	$2,$0,$L621
	lui	$5,%hi($LC1)

	lw	$2,44($2)
	j	$L623
	addiu	$6,$6,1

$L621:
	addu	$4,$4,$16
	jal	sprintf
	addiu	$5,$5,%lo($LC1)

	addu	$2,$16,$2
	lw	$31,20($sp)
	lw	$16,16($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	fastpath_dump_napt_entry_num
	.section	.text.init_table_napt,"ax",@progbits
	.align	2
	.globl	init_table_napt
	.ent	init_table_napt
	.type	init_table_napt, @function
init_table_napt:
	.set	nomips16
	.frame	$sp,64,$31		# vars= 16, regs= 7/0, args= 16, gp= 0
	.mask	0x803f0000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L629:
$L670:
	addiu	$sp,$sp,-64
	lui	$2,%hi(malloc_sizes+4)
	sw	$21,52($sp)
	move	$21,$5
	sw	$19,44($sp)
	li	$5,32			# 0x20
	sw	$18,40($sp)
	move	$18,$4
	sw	$17,36($sp)
	sw	$16,32($sp)
	lui	$16,%hi(table_napt)
	sw	$31,56($sp)
	sw	$20,48($sp)
	lw	$4,%lo(malloc_sizes+4)($2)
	jal	kmem_cache_alloc
	move	$19,$16

	li	$3,-1			# 0xffffffff
	move	$17,$2
	beq	$2,$0,$L624
	sw	$2,%lo(table_napt)($16)

$L680:
$L721:
	lui	$2,%hi(napt_list_inuse)
	lui	$20,%hi(napt_list_free)
	sll	$4,$18,1
	addiu	$3,$2,%lo(napt_list_inuse)
	addiu	$6,$20,%lo(napt_list_free)
	addu	$4,$4,$18
	sw	$0,%lo(napt_list_inuse)($2)
	sll	$4,$4,2
	sw	$3,4($3)
	lui	$2,%hi(napt_table_list_max)
	sw	$0,8($3)
	li	$5,32			# 0x20
	sw	$6,4($6)
	sw	$0,8($6)
	sw	$18,%lo(napt_table_list_max)($2)
	jal	__kmalloc
	sw	$0,%lo(napt_list_free)($20)

	sw	$2,0($17)
	lw	$2,%lo(table_napt)($16)
	lw	$2,0($2)
	beq	$2,$0,$L624
	li	$3,-1			# 0xffffffff

	j	$L724
	move	$16,$0

$L788:
	j	$L624
	li	$3,-2			# 0xfffffffe

$L724:
	blez	$18,$L785
	nop

	move	$4,$0
$L729:
	lw	$2,%lo(table_napt)($19)
	addiu	$16,$16,1
	slt	$3,$16,$18
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(table_napt)($19)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(table_napt)($19)
	lw	$2,0($2)
	addu	$2,$4,$2
	addiu	$4,$4,12
	bne	$3,$0,$L729
	sw	$2,4($2)

$L785:
	blez	$21,$L787
	move	$16,$0

$L783:
$L737:
$L778:
	lui	$2,%hi(malloc_sizes+12)
	li	$5,32			# 0x20
	lw	$4,%lo(malloc_sizes+12)($2)
	jal	kmem_cache_alloc
	addiu	$16,$16,1

	addiu	$4,$20,%lo(napt_list_free)
	move	$3,$2
	addiu	$5,$2,44
	beq	$2,$0,$L788
	slt	$6,$16,$21

	sw	$0,44($2)
	lw	$2,4($4)
	sw	$2,48($3)
	lw	$2,4($4)
	sw	$3,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	bne	$6,$0,$L783
	sw	$2,8($4)

$L787:
	move	$3,$0
$L624:
	lw	$31,56($sp)
	move	$2,$3
	lw	$21,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	j	$31
	addiu	$sp,$sp,64

	.set	macro
	.set	reorder
	.end	init_table_napt
	.section	.text.init_table_path,"ax",@progbits
	.align	2
	.globl	init_table_path
	.ent	init_table_path
	.type	init_table_path, @function
init_table_path:
	.set	nomips16
	.frame	$sp,64,$31		# vars= 16, regs= 7/0, args= 16, gp= 0
	.mask	0x803f0000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L794:
$L835:
	addiu	$sp,$sp,-64
	lui	$2,%hi(malloc_sizes+4)
	sw	$21,52($sp)
	move	$21,$5
	sw	$19,44($sp)
	li	$5,32			# 0x20
	sw	$18,40($sp)
	move	$18,$4
	sw	$17,36($sp)
	sw	$16,32($sp)
	lui	$16,%hi(table_path)
	sw	$31,56($sp)
	sw	$20,48($sp)
	lw	$4,%lo(malloc_sizes+4)($2)
	jal	kmem_cache_alloc
	move	$19,$16

	li	$3,-1			# 0xffffffff
	move	$17,$2
	beq	$2,$0,$L789
	sw	$2,%lo(table_path)($16)

$L845:
$L886:
	lui	$2,%hi(path_list_inuse)
	lui	$20,%hi(path_list_free)
	sll	$4,$18,1
	addiu	$3,$2,%lo(path_list_inuse)
	addiu	$6,$20,%lo(path_list_free)
	addu	$4,$4,$18
	sw	$0,%lo(path_list_inuse)($2)
	sll	$4,$4,2
	sw	$3,4($3)
	lui	$2,%hi(path_table_list_max)
	sw	$0,8($3)
	li	$5,32			# 0x20
	sw	$6,4($6)
	sw	$0,8($6)
	sw	$18,%lo(path_table_list_max)($2)
	jal	__kmalloc
	sw	$0,%lo(path_list_free)($20)

	sw	$2,0($17)
	lw	$2,%lo(table_path)($16)
	lw	$2,0($2)
	beq	$2,$0,$L789
	li	$3,-1			# 0xffffffff

	j	$L889
	move	$16,$0

$L953:
	j	$L789
	li	$3,-2			# 0xfffffffe

$L889:
	blez	$18,$L950
	nop

	move	$4,$0
$L894:
	lw	$2,%lo(table_path)($19)
	addiu	$16,$16,1
	slt	$3,$16,$18
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(table_path)($19)
	lw	$2,0($2)
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(table_path)($19)
	lw	$2,0($2)
	addu	$2,$4,$2
	addiu	$4,$4,12
	bne	$3,$0,$L894
	sw	$2,4($2)

$L950:
	blez	$21,$L952
	move	$16,$0

$L948:
$L902:
$L943:
	lui	$2,%hi(malloc_sizes+20)
	li	$5,32			# 0x20
	lw	$4,%lo(malloc_sizes+20)($2)
	jal	kmem_cache_alloc
	addiu	$16,$16,1

	addiu	$4,$20,%lo(path_list_free)
	move	$3,$2
	addiu	$5,$2,72
	beq	$2,$0,$L953
	slt	$6,$16,$21

	sw	$0,72($2)
	lw	$2,4($4)
	sw	$2,76($3)
	lw	$2,4($4)
	sw	$3,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	bne	$6,$0,$L948
	sw	$2,8($4)

$L952:
	move	$3,$0
$L789:
	lw	$31,56($sp)
	move	$2,$3
	lw	$21,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	j	$31
	addiu	$sp,$sp,64

	.set	macro
	.set	reorder
	.end	init_table_path
	.rdata
	.align	2
$LC2:
	.ascii	"%s:%s\n\000"
	.align	2
$LC3:
	.ascii	"Realtek FastPath\000"
	.align	2
$LC4:
	.ascii	"v1.03\000"
	.section	.text.get_fastpath_module_info,"ax",@progbits
	.align	2
	.globl	get_fastpath_module_info
	.ent	get_fastpath_module_info
	.type	get_fastpath_module_info, @function
get_fastpath_module_info:
	.set	nomips16
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	lui	$5,%hi($LC2)
	lui	$6,%hi($LC3)
	lui	$7,%hi($LC4)
	addiu	$5,$5,%lo($LC2)
	addiu	$6,$6,%lo($LC3)
	beq	$4,$0,$L956
	addiu	$7,$7,%lo($LC4)

	j	sprintf
	nop

$L956:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	get_fastpath_module_info
	.globl	route_list_inuse
	.section	.bss
	.align	2
	.type	route_list_inuse, @object
	.size	route_list_inuse, 12
route_list_inuse:
	.space	12
	.globl	route_list_free
	.align	2
	.type	route_list_free, @object
	.size	route_list_free, 12
route_list_free:
	.space	12
	.globl	napt_list_inuse
	.align	2
	.type	napt_list_inuse, @object
	.size	napt_list_inuse, 12
napt_list_inuse:
	.space	12
	.globl	napt_list_free
	.align	2
	.type	napt_list_free, @object
	.size	napt_list_free, 12
napt_list_free:
	.space	12
	.globl	table_napt
	.align	2
	.type	table_napt, @object
	.size	table_napt, 4
table_napt:
	.space	4
	.local	napt_table_list_max
	.comm	napt_table_list_max,4,4
	.globl	path_list_inuse
	.align	2
	.type	path_list_inuse, @object
	.size	path_list_inuse, 12
path_list_inuse:
	.space	12
	.globl	path_list_free
	.align	2
	.type	path_list_free, @object
	.size	path_list_free, 12
path_list_free:
	.space	12
	.globl	table_path
	.align	2
	.type	table_path, @object
	.size	table_path, 4
table_path:
	.space	4
	.local	path_table_list_max
	.comm	path_table_list_max,4,4
	.globl	udp_cache_list_free
	.align	2
	.type	udp_cache_list_free, @object
	.size	udp_cache_list_free, 12
udp_cache_list_free:
	.space	12
	.globl	udp_cache_table
	.align	2
	.type	udp_cache_table, @object
	.size	udp_cache_table, 4
udp_cache_table:
	.space	4
	.local	max_udp_frag_entry
	.comm	max_udp_frag_entry,4,4
	.ident	"GCC: (GNU) 3.4.6-1.3.6"
