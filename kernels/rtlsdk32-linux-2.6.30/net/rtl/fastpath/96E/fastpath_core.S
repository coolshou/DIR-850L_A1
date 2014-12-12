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
	nop
	beq	$2,$0,$L9
	move	$7,$4

	lw	$2,8($4)
	lw	$3,4($4)
	lhu	$4,2($4)
	lbu	$5,1($7)
	xor	$3,$3,$2
	srl	$2,$3,16
	xor	$2,$2,$4
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lw	$4,%lo(max_udp_frag_entry)($2)
	srl	$2,$3,8
	xor	$2,$2,$5
	xor	$3,$3,$2
	addiu	$4,$4,-1
	and	$4,$3,$4
	lw	$6,40($7)
	sll	$2,$4,1
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
	nop
	lw	$2,0($2)
	nop
	addu	$2,$5,$2
	sw	$3,4($2)
$L6:
	lw	$2,40($7)
	lw	$5,44($7)
	sll	$3,$4,1
	addu	$3,$3,$4
	sw	$2,0($5)
	lui	$2,%hi(udp_cache_table)
	lw	$2,%lo(udp_cache_table)($2)
	sll	$3,$3,2
	lui	$4,%hi(udp_cache_list_free)
	lw	$2,0($2)
	addiu	$4,$4,%lo(udp_cache_list_free)
	addiu	$6,$7,48
	addu	$3,$3,$2
	lw	$2,8($3)
	nop
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,48($7)
	lw	$2,4($4)
	nop
	sw	$2,52($7)
	lw	$2,4($4)
	nop
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
	.frame	$sp,56,$31		# vars= 16, regs= 5/0, args= 16, gp= 0
	.mask	0x800f0000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L15:
$L56:
	addiu	$sp,$sp,-56
	sw	$18,40($sp)
	sw	$17,36($sp)
	sw	$16,32($sp)
	sw	$31,48($sp)
	sw	$19,44($sp)
	lui	$2,%hi(malloc_sizes+4)
	move	$18,$4
	lw	$4,%lo(malloc_sizes+4)($2)
	li	$5,32			# 0x20
	jal	kmem_cache_alloc
	lui	$16,%hi(udp_cache_table)

	move	$17,$2
	sw	$2,%lo(udp_cache_table)($16)
	beq	$2,$0,$L10
	move	$3,$0

$L65:
$L106:
	lui	$19,%hi(udp_cache_list_free)
	sll	$4,$18,1
	addiu	$3,$19,%lo(udp_cache_list_free)
	addu	$4,$4,$18
	sw	$3,4($3)
	lui	$2,%hi(max_udp_frag_entry)
	sw	$0,8($3)
	sll	$4,$4,2
	li	$5,32			# 0x20
	sw	$18,%lo(max_udp_frag_entry)($2)
	jal	__kmalloc
	sw	$0,%lo(udp_cache_list_free)($19)

	sw	$2,0($17)
	lw	$2,%lo(udp_cache_table)($16)
	nop
	lw	$2,0($2)
	nop
	beq	$2,$0,$L10
	li	$3,-1			# 0xffffffffffffffff

	j	$L109
	slt	$5,$0,$18

$L173:
	j	$L10
	move	$3,$0

$L109:
	beq	$5,$0,$L172
	move	$17,$0

	move	$4,$0
$L114:
	lw	$2,%lo(udp_cache_table)($16)
	addiu	$17,$17,1
	slt	$3,$17,$18
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(udp_cache_table)($16)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(udp_cache_table)($16)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$2,4($2)
	bne	$3,$0,$L114
	addiu	$4,$4,12

	move	$17,$0
$L174:
	beq	$5,$0,$L172
	lui	$2,%hi(malloc_sizes+12)

$L122:
$L163:
	lw	$4,%lo(malloc_sizes+12)($2)
	li	$5,32			# 0x20
	jal	kmem_cache_alloc
	addiu	$17,$17,1

	move	$16,$2
	addiu	$4,$2,16
	move	$5,$0
	beq	$2,$0,$L173
	move	$6,$0

	jal	init_timer_key
	nop

	lui	$2,%hi(cache_timeout)
	addiu	$2,$2,%lo(cache_timeout)
	sw	$2,28($16)
	sw	$16,32($16)
	sw	$0,48($16)
	addiu	$3,$19,%lo(udp_cache_list_free)
	lw	$2,4($3)
	addiu	$4,$16,48
	slt	$5,$17,$18
	sw	$2,52($16)
	lw	$2,4($3)
	nop
	sw	$16,0($2)
	lw	$2,8($3)
	sw	$4,4($3)
	addiu	$2,$2,1
	j	$L174
	sw	$2,8($3)

$L172:
	li	$3,1			# 0x1
$L10:
	lw	$31,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	move	$2,$3
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
	lhu	$4,18($sp)
	xor	$3,$3,$5
	lui	$2,%hi(napt_table_list_max)
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
	li	$2,-4			# 0xfffffffffffffffc

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
	li	$2,-4			# 0xfffffffffffffffc

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
	lw	$3,%lo(route_list_inuse)($2)
	move	$8,$0
	move	$7,$0
$L192:
	beq	$3,$0,$L191
	nop

	lw	$5,8($3)
	nop
	sltu	$2,$5,$7
	bne	$2,$0,$L187
	and	$6,$4,$5

	lw	$2,4($3)
	nop
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
	sw	$fp,144($sp)
	sw	$19,124($sp)
	sw	$18,120($sp)
	sw	$31,148($sp)
	sw	$23,140($sp)
	sw	$22,136($sp)
	sw	$21,132($sp)
	sw	$20,128($sp)
	sw	$17,116($sp)
	sw	$16,112($sp)
	move	$19,$5
	move	$18,$4
	sw	$6,160($sp)
	move	$fp,$7
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L193
	li	$3,-1			# 0xffffffffffffffff
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
	li	$3,-1			# 0xffffffffffffffff
	.set	macro
	.set	reorder

	lw	$3,0($18)
	lw	$4,20($18)
	li	$2,17			# 0x11
	xori	$3,$3,0x2
	li	$5,6			# 0x6
	movn	$5,$2,$3		#RLX4181/RLX4281:conditional move  
	.set	noreorder
	.set	nomacro
	jal	filter_addconnect
	sw	$5,76($sp)
	.set	macro
	.set	reorder

#APP
	raw_local_irq_save	$21
#NO_APP
	lw	$23,0($18)
	lw	$22,4($18)
	lw	$20,8($18)
	lw	$17,20($18)
	lw	$16,24($18)
	andi	$2,$23,0xffff
	move	$4,$22
	sw	$2,16($sp)
	move	$5,$20
	move	$6,$17
	.set	noreorder
	.set	nomacro
	jal	FastPath_Hash_NAPT_Entry
	move	$7,$16
	.set	macro
	.set	reorder

	lui	$8,%hi(table_napt)
	lw	$3,%lo(table_napt)($8)
	sw	$2,72($sp)
	sll	$9,$2,1
	lw	$3,0($3)
	addu	$2,$9,$2
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$4,0($2)
	#nop
$L296:
	beq	$4,$0,$L295
	lw	$2,4($4)
	#nop
	bne	$2,$23,$L201
	lw	$2,8($4)
	#nop
	bne	$2,$22,$L201
	lw	$2,12($4)
	#nop
	bne	$2,$20,$L201
	lw	$3,16($4)
	lw	$2,12($18)
	#nop
	bne	$3,$2,$L201
	lw	$3,20($4)
	lw	$2,16($18)
	#nop
	bne	$3,$2,$L201
	lw	$2,24($4)
	#nop
	bne	$2,$17,$L201
	lw	$2,28($4)
	#nop
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
	raw_local_irq_restore	$21
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L193
	move	$3,$0
	.set	macro
	.set	reorder

$L201:
	lw	$4,36($4)
	j	$L296
$L295:
	lui	$4,%hi(napt_list_free)
	lw	$7,%lo(napt_list_free)($4)
	#nop
	beq	$7,$0,$L210
	lw	$2,0($18)
	lw	$6,44($7)
	lw	$5,48($7)
	sw	$2,4($7)
	lw	$2,4($18)
	#nop
	sw	$2,8($7)
	lw	$2,8($18)
	#nop
	sw	$2,12($7)
	lw	$2,12($18)
	#nop
	sw	$2,16($7)
	lw	$2,16($18)
	#nop
	sw	$2,20($7)
	lw	$2,20($18)
	#nop
	sw	$2,24($7)
	lw	$3,24($18)
	li	$2,-1
	sw	$fp,32($7)
	sb	$2,0($7)
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L212
	sw	$3,28($7)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L213
	sw	$5,48($6)
	.set	macro
	.set	reorder

$L212:
	addiu	$2,$4,%lo(napt_list_free)
	sw	$5,4($2)
$L213:
	lw	$3,48($7)
	lw	$2,44($7)
	addiu	$4,$4,%lo(napt_list_free)
	addiu	$5,$7,44
	sw	$2,0($3)
	lw	$2,8($4)
	lui	$3,%hi(napt_list_inuse)
	addiu	$3,$3,%lo(napt_list_inuse)
	addiu	$2,$2,-1
	sw	$2,8($4)
	sw	$0,44($7)
	lw	$2,4($3)
	lw	$6,72($sp)
	sw	$2,48($7)
	lw	$2,4($3)
	addu	$4,$9,$6
	sll	$4,$4,2
	sw	$7,0($2)
	lw	$2,8($3)
	sw	$5,4($3)
	addiu	$6,$7,36
	addiu	$2,$2,1
	sw	$2,8($3)
	sw	$0,36($7)
	lw	$2,%lo(table_napt)($8)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$4,$2
	lw	$2,4($2)
	#nop
	sw	$2,40($7)
	lw	$2,%lo(table_napt)($8)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$4,$2
	lw	$2,4($2)
	#nop
	sw	$7,0($2)
	lw	$2,%lo(table_napt)($8)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$4,$2
	sw	$6,4($2)
	lw	$2,%lo(table_napt)($8)
	#nop
	lw	$2,0($2)
	#nop
	addu	$4,$4,$2
	lw	$2,8($4)
	#nop
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
	move	$23,$2
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_protocol
	move	$fp,$2
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_ip_hdr
	sw	$2,100($sp)
	.set	macro
	.set	reorder

	move	$22,$2
	lbu	$2,0($2)
	move	$4,$19
	andi	$2,$2,0xf
	sll	$2,$2,2
	addu	$2,$22,$2
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	sw	$2,80($sp)
	.set	macro
	.set	reorder

	addiu	$5,$2,6
	li	$6,6			# 0x6
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

	move	$5,$2
	li	$6,6			# 0x6
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

	sw	$2,104($sp)
	lw	$7,12($22)
	lw	$2,80($sp)
	lw	$5,80($sp)
	sw	$7,84($sp)
	lhu	$2,0($2)
	move	$4,$19
	sw	$2,92($sp)
	lw	$3,16($22)
	#nop
	sw	$3,88($sp)
	lhu	$5,2($5)
	.set	noreorder
	.set	nomacro
	jal	rtl_store_skb_dst
	sw	$5,96($sp)
	.set	macro
	.set	reorder

	lw	$5,4($18)
	lw	$6,20($18)
	lw	$4,8($18)
	srl	$2,$5,8
	srl	$3,$6,16
	xor	$3,$3,$6
	xor	$2,$2,$5
	lw	$5,24($18)
	xor	$2,$2,$3
	srl	$4,$4,4
	lw	$7,76($sp)
	xor	$2,$2,$4
	lui	$6,%hi(path_table_list_max)
	xor	$2,$2,$5
	lw	$4,%lo(path_table_list_max)($6)
	xor	$2,$2,$7
	lui	$3,%hi(path_list_free)
	lw	$16,%lo(path_list_free)($3)
	srl	$3,$2,12
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	.set	noreorder
	.set	nomacro
	beq	$16,$0,$L217
	sw	$4,72($sp)
	.set	macro
	.set	reorder

	lw	$2,0($18)
	#nop
	sw	$2,4($16)
	lw	$2,4($18)
	#nop
	sw	$2,16($16)
	lw	$2,8($18)
	#nop
	sw	$2,12($16)
	lw	$2,20($18)
	#nop
	sw	$2,20($16)
	lw	$2,24($18)
	#nop
	sw	$2,8($16)
	lw	$2,12($18)
	#nop
	sw	$2,24($16)
	lw	$2,16($18)
	#nop
	sw	$2,28($16)
	lw	$4,20($18)
	#nop
	sw	$4,32($16)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Route
	sw	$2,36($16)
	.set	macro
	.set	reorder

	lw	$4,16($16)
	lw	$3,24($16)
	sw	$2,44($16)
	li	$2,1
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
	#nop
	beq	$3,$2,$L219
	lbu	$2,1($16)
	#nop
	ori	$2,$2,0x2
	sb	$2,1($16)
$L219:
	sw	$0,48($16)
	sw	$0,52($16)
	lw	$2,4($18)
	addiu	$20,$sp,40
	move	$4,$20
	sw	$2,12($22)
	lw	$2,8($18)
	lw	$7,80($sp)
	move	$5,$0
	li	$6,14			# 0xe
	sh	$2,0($7)
	lw	$2,20($18)
	#nop
	sw	$2,16($22)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($7)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	beq	$23,$0,$L220
	lw	$4,4($18)
	move	$5,$23
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$20
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
	li	$5,-22			# 0xffffffffffffffea
	.set	macro
	.set	reorder

$L221:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	addiu	$4,$2,6
	move	$5,$20
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
	move	$5,$23
	.set	macro
	.set	reorder

	lw	$3,100($sp)
	li	$2,2048			# 0x800
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
	move	$6,$23
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$fp
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
	j	$L300
	sw	$2,48($16)
	.set	macro
	.set	reorder

$L226:
	lw	$2,48($16)
	#nop
	sw	$2,48($16)
$L300:
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
	move	$6,$23
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$fp
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
	#nop
	sw	$2,52($16)
$L220:
	lw	$2,12($18)
	addiu	$20,$sp,40
	move	$4,$20
	sw	$2,12($22)
	lw	$2,16($18)
	lw	$7,80($sp)
	move	$5,$0
	li	$6,14			# 0xe
	sh	$2,0($7)
	lw	$2,20($18)
	#nop
	sw	$2,16($22)
	lw	$2,24($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($7)
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
	beq	$fp,$0,$L233
	move	$5,$fp
	.set	macro
	.set	reorder

	lw	$4,20($18)
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$20
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
	li	$5,-22			# 0xffffffffffffffea
	.set	macro
	.set	reorder

$L234:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$4,$2
	move	$5,$20
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
	move	$5,$fp
	.set	macro
	.set	reorder

	lw	$3,100($sp)
	li	$2,2048			# 0x800
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L233
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
	move	$6,$23
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$fp
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
	#nop
	sw	$2,52($16)
$L233:
	lw	$4,76($sp)
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	bne	$4,$2,$L241
	lui	$2,%hi(nf_ct_udp_timeout_stream)
	.set	macro
	.set	reorder

	lw	$4,160($sp)
	jal	rtl_tcp_get_timeouts
	.set	noreorder
	.set	nomacro
	j	$L301
	sw	$2,60($16)
	.set	macro
	.set	reorder

$L241:
	lw	$2,%lo(nf_ct_udp_timeout_stream)($2)
	#nop
	sw	$2,60($16)
$L301:
	lw	$5,160($sp)
	lui	$2,%hi(jiffies)
	lw	$3,72($16)
	sw	$5,56($16)
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
	lui	$6,%hi(path_list_free)
	addiu	$2,$6,%lo(path_list_free)
	sw	$4,4($2)
$L245:
	lw	$4,76($16)
	lw	$2,72($16)
	lui	$3,%hi(path_list_free)
	addiu	$3,$3,%lo(path_list_free)
	sw	$2,0($4)
	lw	$2,8($3)
	lui	$4,%hi(path_list_inuse)
	addiu	$4,$4,%lo(path_list_inuse)
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,72($16)
	lw	$2,4($4)
	addiu	$6,$16,72
	lui	$5,%hi(table_path)
	sw	$2,76($16)
	lw	$2,4($4)
	lw	$7,72($sp)
	sw	$16,0($2)
	lw	$2,8($4)
	sw	$6,4($4)
	sll	$3,$7,1
	addiu	$2,$2,1
	sw	$2,8($4)
	sw	$0,64($16)
	lw	$2,%lo(table_path)($5)
	addu	$3,$3,$7
	sll	$3,$3,2
	lw	$2,0($2)
	addiu	$4,$16,64
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$2,68($16)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$16,0($2)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	sw	$4,4($2)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$3,$3,$2
	lw	$2,8($3)
	#nop
	addiu	$2,$2,1
	sw	$2,8($3)
$L217:
	lw	$5,20($18)
	lw	$6,12($18)
	lw	$4,24($18)
	srl	$2,$5,8
	srl	$3,$6,16
	xor	$3,$3,$6
	xor	$2,$2,$5
	lw	$5,16($18)
	xor	$2,$2,$3
	srl	$4,$4,4
	xor	$2,$2,$4
	xor	$2,$2,$5
	lw	$5,76($sp)
	lui	$3,%hi(path_table_list_max)
	lw	$4,%lo(path_table_list_max)($3)
	xor	$2,$2,$5
	lui	$6,%hi(path_list_free)
	srl	$3,$2,12
	lw	$16,%lo(path_list_free)($6)
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	.set	noreorder
	.set	nomacro
	beq	$16,$0,$L249
	sw	$4,72($sp)
	.set	macro
	.set	reorder

	lw	$2,0($18)
	#nop
	sw	$2,4($16)
	lw	$2,20($18)
	#nop
	sw	$2,16($16)
	lw	$2,24($18)
	#nop
	sw	$2,12($16)
	lw	$2,12($18)
	#nop
	sw	$2,20($16)
	lw	$2,16($18)
	#nop
	sw	$2,8($16)
	lw	$2,20($18)
	#nop
	sw	$2,24($16)
	lw	$2,24($18)
	#nop
	sw	$2,28($16)
	lw	$4,4($18)
	#nop
	sw	$4,32($16)
	lw	$2,8($18)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Route
	sw	$2,36($16)
	.set	macro
	.set	reorder

	lw	$4,20($16)
	lw	$3,32($16)
	sw	$2,44($16)
	li	$2,2
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
	#nop
	beq	$3,$2,$L251
	lbu	$2,1($16)
	#nop
	ori	$2,$2,0x8
	sb	$2,1($16)
$L251:
	sw	$0,48($16)
	sw	$0,52($16)
	lw	$2,20($18)
	addiu	$20,$sp,40
	move	$4,$20
	sw	$2,12($22)
	lw	$2,24($18)
	lw	$7,80($sp)
	move	$5,$0
	li	$6,14			# 0xe
	sh	$2,0($7)
	lw	$2,12($18)
	#nop
	sw	$2,16($22)
	lw	$2,16($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($7)
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$5,$0
	.set	macro
	.set	reorder

	beq	$fp,$0,$L252
	lw	$4,20($18)
	move	$5,$fp
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$20
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
	li	$5,-22			# 0xffffffffffffffea
	.set	macro
	.set	reorder

$L253:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	addiu	$4,$2,6
	move	$5,$20
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
	move	$5,$fp
	.set	macro
	.set	reorder

	lw	$3,100($sp)
	li	$2,2048			# 0x800
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
	move	$6,$fp
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$23
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
	#nop
	sw	$2,48($16)
$L252:
	lw	$2,20($18)
	addiu	$20,$sp,40
	move	$4,$20
	sw	$2,12($22)
	lw	$2,24($18)
	lw	$7,80($sp)
	move	$5,$0
	li	$6,14			# 0xe
	sh	$2,0($7)
	lw	$2,4($18)
	#nop
	sw	$2,16($22)
	lw	$2,8($18)
	.set	noreorder
	.set	nomacro
	jal	memset
	sh	$2,2($7)
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
	beq	$23,$0,$L260
	move	$5,$23
	.set	macro
	.set	reorder

	lw	$4,4($18)
	.set	noreorder
	.set	nomacro
	jal	rtl_arp_req_get_ha
	move	$6,$20
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
	li	$5,-22			# 0xffffffffffffffea
	.set	macro
	.set	reorder

$L261:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$4,$2
	move	$5,$20
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
	move	$5,$23
	.set	macro
	.set	reorder

	lw	$3,100($sp)
	li	$2,2048			# 0x800
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L260
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
	move	$6,$fp
	.set	noreorder
	.set	nomacro
	jal	rtl_ipt_do_table
	move	$7,$23
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
	lw	$4,76($sp)
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	bne	$4,$2,$L266
	lui	$2,%hi(nf_ct_udp_timeout_stream)
	.set	macro
	.set	reorder

	lw	$4,160($sp)
	jal	rtl_tcp_get_timeouts
	.set	noreorder
	.set	nomacro
	j	$L302
	sw	$2,60($16)
	.set	macro
	.set	reorder

$L266:
	lw	$2,%lo(nf_ct_udp_timeout_stream)($2)
	#nop
	sw	$2,60($16)
$L302:
	lw	$5,160($sp)
	lui	$2,%hi(jiffies)
	lw	$3,72($16)
	sw	$5,56($16)
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
	lui	$6,%hi(path_list_free)
	addiu	$2,$6,%lo(path_list_free)
	sw	$4,4($2)
$L270:
	lw	$4,76($16)
	lw	$2,72($16)
	lui	$3,%hi(path_list_free)
	addiu	$3,$3,%lo(path_list_free)
	sw	$2,0($4)
	lw	$2,8($3)
	lui	$4,%hi(path_list_inuse)
	addiu	$4,$4,%lo(path_list_inuse)
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,72($16)
	lw	$2,4($4)
	addiu	$6,$16,72
	lui	$5,%hi(table_path)
	sw	$2,76($16)
	lw	$2,4($4)
	lw	$7,72($sp)
	sw	$16,0($2)
	lw	$2,8($4)
	sw	$6,4($4)
	sll	$3,$7,1
	addiu	$2,$2,1
	sw	$2,8($4)
	sw	$0,64($16)
	lw	$2,%lo(table_path)($5)
	addu	$3,$3,$7
	sll	$3,$3,2
	lw	$2,0($2)
	addiu	$4,$16,64
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$2,68($16)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$16,0($2)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	sw	$4,4($2)
	lw	$2,%lo(table_path)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$3,$3,$2
	lw	$2,8($3)
	#nop
	addiu	$2,$2,1
	sw	$2,8($3)
$L249:
	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	addiu	$4,$2,6
	addiu	$5,$sp,24
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,6			# 0x6
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	rtl_eth_hdr
	move	$4,$19
	.set	macro
	.set	reorder

	move	$4,$2
	li	$6,6			# 0x6
	.set	noreorder
	.set	nomacro
	jal	memcpy
	addiu	$5,$sp,32
	.set	macro
	.set	reorder

	lw	$5,104($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_mark
	move	$4,$19
	.set	macro
	.set	reorder

	lw	$2,84($sp)
	#nop
	sw	$2,12($22)
	lw	$4,92($sp)
	lw	$3,80($sp)
	#nop
	sh	$4,0($3)
	lw	$5,88($sp)
	move	$4,$19
	sw	$5,16($22)
	lw	$6,96($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_set_skb_dst
	sh	$6,2($3)
	.set	macro
	.set	reorder

	beq	$23,$0,$L273
#APP
	raw_local_irq_save	$3
#NO_APP
	lw	$2,452($23)
	#nop
	addiu	$2,$2,-1
	sw	$2,452($23)
#APP
	raw_local_irq_restore	$3
#NO_APP
$L273:
	beq	$fp,$0,$L277
#APP
	raw_local_irq_save	$3
#NO_APP
	lw	$2,452($fp)
	#nop
	addiu	$2,$2,-1
	sw	$2,452($fp)
#APP
	raw_local_irq_restore	$3
#NO_APP
	j	$L277
$L210:
#APP
	raw_local_irq_restore	$21
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L193
	li	$3,-1			# 0xffffffffffffffff
	.set	macro
	.set	reorder

$L277:
#APP
	raw_local_irq_restore	$21
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
	lw	$fp,144($sp)
	lw	$23,140($sp)
	lw	$22,136($sp)
	lw	$21,132($sp)
	lw	$20,128($sp)
	lw	$19,124($sp)
	lw	$18,120($sp)
	lw	$17,116($sp)
	lw	$16,112($sp)
	move	$2,$3
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
	sw	$23,52($sp)
	sw	$16,24($sp)
	sw	$31,56($sp)
	sw	$22,48($sp)
	sw	$21,44($sp)
	sw	$20,40($sp)
	sw	$19,36($sp)
	sw	$18,32($sp)
	sw	$17,28($sp)
	lw	$2,0($4)
	li	$3,17			# 0x11
	li	$23,6			# 0x6
	xori	$2,$2,0x2
	move	$16,$4
	movn	$23,$3,$2		#RLX4181/RLX4281:conditional move  
#APP
	raw_local_irq_save	$17
#NO_APP
	lw	$4,20($4)
	jal	filter_delconnect
	lw	$22,0($16)
	lw	$21,4($16)
	lw	$20,8($16)
	lw	$19,20($16)
	lw	$18,24($16)
	andi	$2,$22,0xffff
	move	$5,$20
	move	$6,$19
	move	$7,$18
	sw	$2,16($sp)
	.set	noreorder
	.set	nomacro
	jal	FastPath_Hash_NAPT_Entry
	move	$4,$21
	.set	macro
	.set	reorder

	lui	$10,%hi(table_napt)
	lw	$3,%lo(table_napt)($10)
	sll	$5,$2,1
	move	$7,$2
	lw	$3,0($3)
	addu	$2,$5,$2
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$6,0($2)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L358
	lui	$2,%hi(napt_list_inuse)
	.set	macro
	.set	reorder

	addiu	$9,$2,%lo(napt_list_inuse)
	lui	$2,%hi(napt_list_free)
	addiu	$8,$2,%lo(napt_list_free)
	lui	$24,%hi(path_list_inuse)
$L350:
	lw	$2,4($6)
	#nop
	bne	$2,$22,$L309
	lw	$2,8($6)
	#nop
	bne	$2,$21,$L309
	lw	$2,12($6)
	#nop
	bne	$2,$20,$L309
	lw	$3,16($6)
	lw	$2,12($16)
	#nop
	bne	$3,$2,$L309
	lw	$3,20($6)
	lw	$2,16($16)
	#nop
	bne	$3,$2,$L309
	lw	$2,24($6)
	#nop
	bne	$2,$19,$L309
	lw	$2,28($6)
	#nop
	bne	$2,$18,$L309
	lw	$2,36($6)
	sb	$0,0($6)
	lw	$4,40($6)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L312
	addu	$3,$5,$7
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L313
	sw	$4,40($2)
	.set	macro
	.set	reorder

$L312:
	lw	$2,%lo(table_napt)($10)
	sll	$3,$3,2
	lw	$2,0($2)
	#nop
	addu	$3,$3,$2
	sw	$4,4($3)
$L313:
	lw	$2,36($6)
	lw	$4,40($6)
	sll	$3,$7,1
	addu	$3,$3,$7
	sw	$2,0($4)
	lui	$2,%hi(table_napt)
	lw	$2,%lo(table_napt)($2)
	sll	$3,$3,2
	lw	$2,0($2)
	#nop
	addu	$3,$3,$2
	lw	$2,8($3)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$2,44($6)
	lw	$3,48($6)
	beq	$2,$0,$L315
	.set	noreorder
	.set	nomacro
	j	$L316
	sw	$3,48($2)
	.set	macro
	.set	reorder

$L315:
	sw	$3,4($9)
$L316:
	lw	$3,48($6)
	lw	$2,44($6)
	addiu	$4,$6,44
	lui	$18,%hi(path_table_list_max)
	sw	$2,0($3)
	lw	$2,8($9)
	lui	$13,%hi(table_path)
	addiu	$2,$2,-1
	sw	$2,8($9)
	sw	$0,44($6)
	lw	$2,4($8)
	#nop
	sw	$2,48($6)
	lw	$2,4($8)
	#nop
	sw	$6,0($2)
	lw	$2,8($8)
	sw	$4,4($8)
	lw	$5,%lo(path_table_list_max)($18)
	addiu	$2,$2,1
	sw	$2,8($8)
	lw	$9,4($16)
	lw	$8,20($16)
	lw	$12,8($16)
	srl	$2,$9,8
	srl	$3,$8,16
	xor	$3,$3,$8
	xor	$2,$2,$9
	lw	$11,24($16)
	xor	$2,$2,$3
	srl	$3,$12,4
	xor	$2,$2,$3
	xor	$2,$2,$11
	xor	$2,$2,$23
	srl	$3,$2,12
	lw	$4,%lo(table_path)($13)
	xor	$2,$2,$3
	addiu	$5,$5,-1
	and	$7,$5,$2
	lw	$3,0($4)
	sll	$2,$7,1
	addu	$2,$2,$7
	sll	$2,$2,2
	addu	$3,$2,$3
	lw	$6,0($3)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L320
	move	$5,$2
	.set	macro
	.set	reorder

	lui	$2,%hi(path_list_free)
	addiu	$7,$2,%lo(path_list_free)
	move	$4,$5
	addiu	$10,$24,%lo(path_list_inuse)
	li	$14,1			# 0x1
	move	$15,$13
$L330:
	lw	$3,4($6)
	lw	$2,0($16)
	#nop
	bne	$3,$2,$L321
	lbu	$2,2($6)
	#nop
	bne	$2,$14,$L321
	lw	$2,16($6)
	#nop
	bne	$2,$9,$L321
	lw	$2,12($6)
	#nop
	bne	$2,$12,$L321
	lw	$3,24($6)
	lw	$2,12($16)
	#nop
	bne	$3,$2,$L321
	lw	$3,28($6)
	lw	$2,16($16)
	#nop
	bne	$3,$2,$L321
	lw	$2,20($6)
	#nop
	bne	$2,$8,$L321
	lw	$2,8($6)
	#nop
	bne	$2,$11,$L321
	lw	$2,64($6)
	sb	$0,0($6)
	lw	$3,68($6)
	beq	$2,$0,$L324
	.set	noreorder
	.set	nomacro
	j	$L325
	sw	$3,68($2)
	.set	macro
	.set	reorder

$L324:
	lw	$2,%lo(table_path)($13)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$5,$2
	sw	$3,4($2)
$L325:
	lw	$3,68($6)
	lw	$2,64($6)
	#nop
	sw	$2,0($3)
	lw	$2,%lo(table_path)($15)
	#nop
	lw	$3,0($2)
	#nop
	addu	$3,$4,$3
	lw	$2,8($3)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$3,72($6)
	lw	$2,76($6)
	beq	$3,$0,$L327
	.set	noreorder
	.set	nomacro
	j	$L328
	sw	$2,76($3)
	.set	macro
	.set	reorder

$L327:
	sw	$2,4($10)
$L328:
	lw	$2,72($6)
	lw	$3,76($6)
	addiu	$4,$6,72
	sw	$2,0($3)
	lw	$2,8($10)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($10)
	sw	$0,72($6)
	lw	$2,4($7)
	#nop
	sw	$2,76($6)
	lw	$2,4($7)
	#nop
	sw	$6,0($2)
	lw	$2,8($7)
	sw	$4,4($7)
	addiu	$2,$2,1
	.set	noreorder
	.set	nomacro
	j	$L320
	sw	$2,8($7)
	.set	macro
	.set	reorder

$L321:
	lw	$6,64($6)
	#nop
	bne	$6,$0,$L330
$L320:
	lw	$9,20($16)
	lw	$8,12($16)
	lw	$13,24($16)
	srl	$2,$9,8
	srl	$3,$8,16
	xor	$3,$3,$8
	xor	$2,$2,$9
	lw	$12,16($16)
	xor	$2,$2,$3
	srl	$3,$13,4
	xor	$2,$2,$3
	xor	$2,$2,$12
	lw	$3,%lo(path_table_list_max)($18)
	xor	$2,$2,$23
	lui	$10,%hi(table_path)
	srl	$4,$2,12
	lw	$5,%lo(table_path)($10)
	xor	$2,$2,$4
	addiu	$3,$3,-1
	and	$7,$3,$2
	sll	$2,$7,1
	lw	$3,0($5)
	addu	$2,$2,$7
	sll	$2,$2,2
	addu	$3,$2,$3
	lw	$6,0($3)
	#nop
	beq	$6,$0,$L333
	move	$5,$2
	lui	$2,%hi(path_list_free)
	addiu	$11,$24,%lo(path_list_inuse)
	addiu	$7,$2,%lo(path_list_free)
	move	$4,$5
	li	$14,2			# 0x2
	move	$15,$10
$L343:
	lw	$3,4($6)
	lw	$2,0($16)
	#nop
	bne	$3,$2,$L334
	lbu	$2,2($6)
	#nop
	bne	$2,$14,$L334
	lw	$2,20($6)
	#nop
	bne	$2,$8,$L334
	lw	$2,8($6)
	#nop
	bne	$2,$12,$L334
	lw	$2,24($6)
	#nop
	bne	$2,$9,$L334
	lw	$2,28($6)
	#nop
	bne	$2,$13,$L334
	lw	$3,32($6)
	lw	$2,4($16)
	#nop
	bne	$3,$2,$L334
	lw	$3,36($6)
	lw	$2,8($16)
	#nop
	bne	$3,$2,$L334
	lw	$2,64($6)
	sb	$0,0($6)
	lw	$3,68($6)
	beq	$2,$0,$L337
	.set	noreorder
	.set	nomacro
	j	$L338
	sw	$3,68($2)
	.set	macro
	.set	reorder

$L337:
	lw	$2,%lo(table_path)($10)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$5,$2
	sw	$3,4($2)
$L338:
	lw	$3,68($6)
	lw	$2,64($6)
	#nop
	sw	$2,0($3)
	lw	$2,%lo(table_path)($15)
	#nop
	lw	$3,0($2)
	#nop
	addu	$3,$4,$3
	lw	$2,8($3)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($3)
	lw	$3,72($6)
	lw	$2,76($6)
	beq	$3,$0,$L340
	.set	noreorder
	.set	nomacro
	j	$L341
	sw	$2,76($3)
	.set	macro
	.set	reorder

$L340:
	sw	$2,4($11)
$L341:
	lw	$2,72($6)
	lw	$3,76($6)
	addiu	$4,$6,72
	sw	$2,0($3)
	lw	$2,8($11)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($11)
	sw	$0,72($6)
	lw	$2,4($7)
	#nop
	sw	$2,76($6)
	lw	$2,4($7)
	#nop
	sw	$6,0($2)
	lw	$2,8($7)
	sw	$4,4($7)
	addiu	$2,$2,1
	.set	noreorder
	.set	nomacro
	j	$L333
	sw	$2,8($7)
	.set	macro
	.set	reorder

$L334:
	lw	$6,64($6)
	#nop
	bne	$6,$0,$L343
$L333:
#APP
	raw_local_irq_restore	$17
#NO_APP
	.set	noreorder
	.set	nomacro
	j	$L303
	move	$2,$0
	.set	macro
	.set	reorder

$L309:
	lw	$6,36($6)
	#nop
	bne	$6,$0,$L350
$L358:
#APP
	raw_local_irq_restore	$17
#NO_APP
	li	$2,-4			# 0xfffffffffffffffc
$L303:
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
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$31,16($sp)
	lw	$9,4($4)
	lw	$7,20($4)
	lw	$13,8($4)
	lw	$12,0($4)
	srl	$2,$9,8
	srl	$3,$7,16
	xor	$3,$3,$7
	lw	$10,24($4)
	xor	$2,$2,$9
	xor	$2,$2,$3
	srl	$3,$13,4
	xor	$2,$2,$3
	move	$8,$4
	li	$3,17			# 0x11
	xori	$4,$12,0x2
	li	$11,6			# 0x6
	lui	$25,%hi(path_table_list_max)
	movn	$11,$3,$4		#RLX4181/RLX4281:conditional move  
	xor	$2,$2,$10
	lw	$3,%lo(path_table_list_max)($25)
	xor	$2,$2,$11
	lui	$24,%hi(table_path)
	srl	$4,$2,12
	lw	$6,%lo(table_path)($24)
	xor	$2,$2,$4
	addiu	$3,$3,-1
	and	$4,$3,$2
	sll	$2,$4,1
	lw	$3,0($6)
	addu	$2,$2,$4
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$6,0($2)
	lui	$2,%hi(jiffies)
	lw	$15,%lo(jiffies)($2)
	move	$14,$5
$L380:
	beq	$6,$0,$L366
	li	$2,-16777216			# 0xffffffffff000000

	lw	$3,4($6)
	ori	$4,$2,0xff00
	bne	$3,$12,$L367
	ori	$5,$2,0x100

	lw	$2,0($6)
	nop
	and	$2,$2,$4
	bne	$2,$5,$L367
	nop

	lw	$2,16($6)
	nop
	bne	$2,$9,$L367
	nop

	lw	$2,12($6)
	nop
	bne	$2,$13,$L367
	nop

	lw	$3,24($6)
	lw	$2,12($8)
	nop
	bne	$3,$2,$L367
	nop

	lw	$3,28($6)
	lw	$2,16($8)
	nop
	bne	$3,$2,$L367
	nop

	lw	$2,20($6)
	nop
	bne	$2,$7,$L367
	nop

	lw	$2,8($6)
	nop
	bne	$2,$10,$L367
	subu	$2,$15,$14

	lw	$5,40($6)
	nop
	subu	$2,$2,$5
	bgez	$2,$L366
	nop

	j	$L382
	nop

$L367:
	lw	$6,64($6)
	j	$L380
	nop

$L366:
	lw	$10,20($8)
	lw	$9,12($8)
	lw	$13,24($8)
	srl	$2,$10,8
	srl	$3,$9,16
	xor	$3,$3,$9
	xor	$2,$2,$10
	lw	$12,16($8)
	xor	$2,$2,$3
	srl	$3,$13,4
	xor	$2,$2,$3
	xor	$2,$2,$12
	lw	$4,%lo(path_table_list_max)($25)
	xor	$2,$2,$11
	srl	$3,$2,12
	lw	$5,%lo(table_path)($24)
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	lw	$3,0($5)
	sll	$2,$4,1
	addu	$2,$2,$4
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$6,0($2)
	nop
$L381:
	beq	$6,$0,$L373
	li	$2,-16777216			# 0xffffffffff000000

	lw	$4,4($6)
	lw	$3,0($8)
	ori	$5,$2,0xff00
	bne	$4,$3,$L374
	ori	$7,$2,0x200

	lw	$2,0($6)
	nop
	and	$2,$2,$5
	bne	$2,$7,$L374
	nop

	lw	$2,20($6)
	nop
	bne	$2,$9,$L374
	nop

	lw	$2,8($6)
	nop
	bne	$2,$12,$L374
	nop

	lw	$2,24($6)
	nop
	bne	$2,$10,$L374
	nop

	lw	$2,28($6)
	nop
	bne	$2,$13,$L374
	nop

	lw	$3,32($6)
	lw	$2,4($8)
	nop
	bne	$3,$2,$L374
	nop

	lw	$3,36($6)
	lw	$2,8($8)
	nop
	bne	$3,$2,$L374
	subu	$2,$15,$14

	lw	$5,40($6)
	nop
	subu	$2,$2,$5
	bgez	$2,$L373
	nop

$L382:
	lw	$4,56($6)
	jal	rtl_set_ct_timeout_expires
	addu	$5,$5,$14

	j	$L361
	li	$2,-1			# 0xffffffffffffffff

$L374:
	lw	$6,64($6)
	j	$L381
	nop

$L373:
	move	$2,$0
$L361:
	lw	$31,16($sp)
	nop
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
	sw	$17,52($sp)
	sw	$31,68($sp)
	sw	$20,64($sp)
	sw	$19,60($sp)
	sw	$18,56($sp)
	sw	$16,48($sp)
	lui	$2,%hi(jiffies)
	move	$5,$0
	lw	$20,%lo(jiffies)($2)
	jal	rtl_get_ct_protonum
	move	$17,$4

	li	$3,17			# 0x11
	bne	$2,$3,$L384
	move	$4,$17

	jal	rtl_get_ct_udp_status
	move	$4,$17

	andi	$2,$2,0x2
	beq	$2,$0,$L385
	li	$19,1			# 0x1

	lui	$2,%hi(nf_ct_udp_timeout_stream)
	lw	$18,%lo(nf_ct_udp_timeout_stream)($2)
	j	$L399
	move	$4,$17

$L385:
	lui	$2,%hi(nf_ct_udp_timeout)
	lw	$18,%lo(nf_ct_udp_timeout)($2)
	j	$L399
	move	$4,$17

$L384:
	jal	rtl_get_ct_protonum
	move	$5,$0

	li	$3,6			# 0x6
	bne	$2,$3,$L383
	li	$2,-1			# 0xffffffffffffffff

	jal	rtl_get_ct_tcp_state
	move	$4,$17

	sltu	$2,$2,6
	beq	$2,$0,$L383
	li	$2,-1			# 0xffffffffffffffff

	jal	rtl_tcp_get_timeouts
	move	$4,$17

	move	$18,$2
	li	$19,2			# 0x2
	move	$4,$17
$L399:
	move	$5,$0
	jal	rtl_get_ct_ip_by_dir
	move	$6,$0

	move	$4,$17
	li	$5,1			# 0x1
	li	$6,1			# 0x1
	jal	rtl_get_ct_ip_by_dir
	move	$16,$2

	bne	$16,$2,$L390
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
	j	$L398
	move	$5,$0

$L390:
	move	$5,$0
	jal	rtl_get_ct_ip_by_dir
	li	$6,1			# 0x1

	move	$4,$17
	li	$5,1			# 0x1
	move	$6,$0
	jal	rtl_get_ct_ip_by_dir
	move	$16,$2

	bne	$16,$2,$L383
	li	$2,-1			# 0xffffffffffffffff

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
$L398:
	move	$6,$0
	jal	rtl_get_ct_port_by_dir
	sw	$2,36($sp)

	addiu	$4,$sp,16
	move	$5,$18
	jal	rtk_idleNaptConnection
	sw	$2,40($sp)

	beq	$2,$0,$L395
	addu	$5,$20,$18

	jal	rtl_check_for_acc
	move	$4,$17

	j	$L383
	move	$2,$0

$L395:
	li	$2,-1			# 0xffffffffffffffff
$L383:
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
	sw	$16,24($sp)
	move	$17,$4
	move	$16,$5
	move	$4,$0
	move	$5,$6
	sw	$31,32($sp)
	jal	rtl_get_hw_fdb_age
	li	$6,2			# 0x2

	move	$3,$2
	sltu	$5,$2,301
	li	$2,300			# 0x12c
	beq	$3,$2,$L403
	move	$4,$16

	beq	$5,$0,$L406
	li	$2,450			# 0x1c2

	li	$2,150			# 0x96
	beq	$3,$2,$L404
	lui	$2,%hi(jiffies)

	j	$L401
	nop

$L406:
	bne	$3,$2,$L401
	move	$4,$16

	lui	$2,%hi(jiffies)
	lw	$5,%lo(jiffies)($2)
	j	$L408
	nop

$L403:
	lui	$2,%hi(jiffies)
	lw	$5,%lo(jiffies)($2)
	j	$L408
	addiu	$5,$5,-15000

$L404:
	lw	$5,%lo(jiffies)($2)
	nop
	addiu	$5,$5,-30000
$L408:
	jal	rtl_set_fdb_aging
	nop

$L401:
	jal	rtl_get_fdb_aging
	move	$4,$16

	move	$4,$17
	jal	rtl_hold_time
	move	$16,$2

	lui	$3,%hi(jiffies)
	lw	$3,%lo(jiffies)($3)
	addu	$16,$16,$2
	lw	$31,32($sp)
	subu	$3,$3,$16
	lw	$17,28($sp)
	lw	$16,24($sp)
	slt	$3,$3,0
	li	$2,-1			# 0xffffffffffffffff
	movn	$2,$0,$3		#RLX4181/RLX4281:conditional move
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
	andi	$10,$5,0xffff
	xor	$2,$2,$4
	xor	$3,$3,$6
	xor	$2,$2,$3
	lhu	$9,18($sp)
	srl	$3,$10,4
	xor	$2,$2,$3
	andi	$7,$7,0xffff
	lui	$12,%hi(path_table_list_max)
	xor	$2,$2,$7
	lw	$3,%lo(path_table_list_max)($12)
	xor	$2,$2,$9
	lui	$13,%hi(table_path)
	move	$11,$4
	lw	$5,%lo(table_path)($13)
	srl	$4,$2,12
	xor	$2,$2,$4
	addiu	$3,$3,-1
	and	$4,$3,$2
	sll	$2,$4,1
	lw	$3,0($5)
	addu	$2,$2,$4
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$4,0($2)
	nop
	beq	$4,$0,$L432
	nop

	li	$3,255			# 0xff
$L417:
	lw	$2,12($4)
	nop
	bne	$2,$10,$L413
	nop

	lw	$2,8($4)
	nop
	bne	$2,$7,$L413
	nop

	lw	$2,16($4)
	nop
	bne	$2,$11,$L413
	nop

	lw	$2,20($4)
	nop
	bne	$2,$6,$L413
	nop

	lbu	$2,0($4)
	nop
	bne	$2,$3,$L413
	li	$2,1			# 0x1

	lbu	$3,2($4)
	nop
	beq	$3,$2,$L430
	sb	$0,0($4)

	lw	$5,32($4)
	lhu	$8,38($4)
	j	$L412
	nop

$L413:
	lw	$4,64($4)
	nop
	bne	$4,$0,$L417
	nop

$L433:
	j	$31
	nop

$L412:
	beq	$5,$0,$L433
	li	$2,1			# 0x1

	lbu	$3,2($4)
	nop
	bne	$3,$2,$L419
	srl	$3,$11,16

	srl	$2,$6,8
	srl	$3,$5,16
	xor	$3,$3,$5
	xor	$2,$2,$6
	xor	$2,$2,$3
	srl	$3,$7,4
	xor	$2,$2,$3
	j	$L431
	xor	$2,$2,$8

$L430:
	lw	$5,24($4)
	lhu	$8,30($4)
	j	$L412
	nop

$L419:
	srl	$2,$5,8
	xor	$3,$3,$11
	xor	$2,$2,$5
	xor	$2,$2,$3
	srl	$3,$8,4
	xor	$2,$2,$3
	xor	$2,$2,$10
$L431:
	lw	$4,%lo(path_table_list_max)($12)
	xor	$2,$2,$9
	srl	$3,$2,12
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	lw	$2,%lo(table_path)($13)
	sll	$3,$4,1
	addu	$3,$3,$4
	lw	$2,0($2)
	sll	$3,$3,2
	addu	$3,$3,$2
	lw	$4,0($3)
	nop
	beq	$4,$0,$L432
	nop

	li	$3,255			# 0xff
$L427:
	lw	$2,24($4)
	nop
	bne	$2,$6,$L425
	nop

	lw	$2,28($4)
	nop
	bne	$2,$7,$L425
	nop

	lw	$2,32($4)
	nop
	bne	$2,$11,$L425
	nop

	lw	$2,36($4)
	nop
	bne	$2,$10,$L425
	nop

	lbu	$2,0($4)
	nop
	bne	$2,$3,$L425
	nop

	sb	$0,0($4)
$L425:
	lw	$4,64($4)
	nop
	bne	$4,$0,$L427
	nop

$L432:
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
	sw	$16,32($sp)
	li	$5,1			# 0x1
	move	$16,$4
	move	$6,$0
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

	lw	$4,24($sp)
	move	$5,$21
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
	sw	$31,84($sp)
	sw	$22,72($sp)
	sw	$18,56($sp)
	sw	$16,48($sp)
	sw	$4,24($sp)
	sw	$fp,80($sp)
	sw	$23,76($sp)
	sw	$21,68($sp)
	sw	$20,64($sp)
	sw	$19,60($sp)
	.set	noreorder
	.set	nomacro
	jal	rtl_get_skb_dev_name
	sw	$17,52($sp)
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	move	$16,$2
	.set	noreorder
	.set	nomacro
	jal	rtl_ip_hdr
	move	$22,$0
	.set	macro
	.set	reorder

	move	$18,$2
	lbu	$2,8($2)
	li	$3,1			# 0x1
	sw	$3,36($sp)
	sltu	$2,$2,2
	sw	$0,40($sp)
	sw	$0,44($sp)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L435
	move	$4,$0
	.set	macro
	.set	reorder

	lbu	$19,9($18)
	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L437
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L435
$L437:
	lbu	$2,0($18)
	lw	$6,24($sp)
	move	$4,$18
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
	bne	$5,$2,$L435
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
	nop
	move	$3,$1
2:	subu	$3,$1
3:	.set	at
	.set	reorder
#NO_APP
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L435
	move	$4,$0
	.set	macro
	.set	reorder

	lhu	$4,0($21)
	lhu	$16,6($18)
	lw	$20,12($18)
	lw	$23,16($18)
	sw	$4,28($sp)
	lhu	$2,2($21)
	.set	noreorder
	.set	nomacro
	bne	$19,$5,$L450
	sw	$2,32($sp)
	.set	macro
	.set	reorder

	andi	$2,$16,0x3fff
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L445
	li	$3,1			# 0x1
	.set	macro
	.set	reorder

	xori	$2,$16,0x2000
	sltu	$3,$2,1
$L445:
	li	$2,1			# 0x1
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L446
	li	$2,65536			# 0x10000
	.set	macro
	.set	reorder

	lw	$3,12($21)
	#nop
	and	$2,$3,$2
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L447
	li	$2,393216			# 0x60000
	.set	macro
	.set	reorder

	lw	$5,28($sp)
	lw	$7,32($sp)
	move	$4,$20
	move	$6,$23
	.set	noreorder
	.set	nomacro
	jal	mark_path_invalid
	sw	$19,16($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L435
	move	$4,$0
	.set	macro
	.set	reorder

$L447:
	and	$2,$3,$2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L435
	move	$4,$0
	.set	macro
	.set	reorder

$L446:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L450
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	#nop
	bne	$3,$2,$L450
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

$L450:
	lhu	$2,6($18)
	#nop
	andi	$2,$2,0x3fff
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L451
	li	$2,8192			# 0x2000
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$16,$2,$L452
	andi	$2,$16,0x2000
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
	.set	noreorder
	.set	nomacro
	j	$L453
	sw	$3,40($sp)
	.set	macro
	.set	reorder

$L452:
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L453
	andi	$3,$16,0x1fff
	.set	macro
	.set	reorder

	lw	$4,44($sp)
	li	$2,1			# 0x1
	movn	$4,$2,$3		#RLX4181/RLX4281:conditional move
	sw	$4,44($sp)
$L453:
	lw	$9,12($18)
	lw	$8,16($18)
	lhu	$7,4($18)
	andi	$6,$19,0x00ff
	xor	$3,$9,$8
	srl	$2,$3,16
	xor	$2,$2,$7
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lw	$5,%lo(max_udp_frag_entry)($2)
	srl	$4,$3,8
	lui	$2,%hi(udp_cache_table)
	xor	$4,$4,$6
	lw	$2,%lo(udp_cache_table)($2)
	xor	$3,$3,$4
	addiu	$5,$5,-1
	and	$3,$3,$5
	lw	$4,0($2)
	sll	$2,$3,1
	addu	$2,$2,$3
	sll	$2,$2,2
	addu	$2,$2,$4
	lw	$4,0($2)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L462
	move	$22,$0
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
$L461:
	lhu	$2,2($4)
	#nop
	bne	$2,$7,$L458
	lw	$2,4($4)
	#nop
	bne	$2,$9,$L458
	lw	$2,8($4)
	#nop
	bne	$2,$8,$L458
	lbu	$2,1($4)
	#nop
	bne	$2,$6,$L458
	lbu	$2,0($4)
	#nop
	beq	$2,$3,$L593
$L458:
	lw	$4,40($4)
	#nop
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L461
	move	$22,$0
	.set	macro
	.set	reorder

	j	$L462
$L593:
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L462
	move	$22,$4
	.set	macro
	.set	reorder

	lhu	$2,12($4)
	#nop
	sw	$2,28($sp)
	lhu	$3,14($4)
	lw	$4,40($sp)
	#nop
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L451
	sw	$3,32($sp)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L451
	sw	$0,36($sp)
	.set	macro
	.set	reorder

$L462:
	lw	$2,40($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L435
	move	$4,$0
	.set	macro
	.set	reorder

$L451:
	lw	$4,28($sp)
	srl	$2,$20,8
	srl	$fp,$23,16
	xor	$3,$fp,$23
	xor	$2,$2,$20
	xor	$2,$2,$3
	srl	$3,$4,4
	xor	$2,$2,$3
	lw	$3,32($sp)
	#nop
	xor	$2,$2,$3
	lui	$3,%hi(path_table_list_max)
	lw	$4,%lo(path_table_list_max)($3)
	xor	$2,$2,$19
	lui	$3,%hi(table_path)
	lw	$5,%lo(table_path)($3)
	srl	$3,$2,12
	xor	$2,$2,$3
	addiu	$4,$4,-1
	and	$4,$4,$2
	lw	$3,0($5)
	sll	$2,$4,1
	addu	$2,$2,$4
	sll	$2,$2,2
	addu	$2,$2,$3
	lw	$17,0($2)
	#nop
$L599:
	beq	$17,$0,$L592
	lw	$2,12($17)
	lw	$4,28($sp)
	#nop
	bne	$2,$4,$L469
	lw	$2,8($17)
	lw	$3,32($sp)
	#nop
	bne	$2,$3,$L469
	lw	$2,16($17)
	#nop
	bne	$2,$20,$L469
	lw	$2,20($17)
	#nop
	.set	noreorder
	.set	nomacro
	bne	$2,$23,$L469
	li	$2,255			# 0xff
	.set	macro
	.set	reorder

	lbu	$3,0($17)
	#nop
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L469
	li	$2,2			# 0x2
	.set	macro
	.set	reorder

	lw	$3,4($17)
	#nop
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L609
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

	li	$2,6			# 0x6
	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L471
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

$L609:
	.set	noreorder
	.set	nomacro
	bne	$3,$2,$L469
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L469
$L471:
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
	bne	$2,$0,$L435
	move	$4,$0
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	jal	rtl_skb_dst_check
	li	$3,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L595
	li	$2,6			# 0x6
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$19,$2,$L476
	addiu	$16,$21,16
	.set	macro
	.set	reorder

	lhu	$3,6($21)
	addiu	$2,$21,6
	move	$16,$0
	movn	$16,$2,$3		#RLX4181/RLX4281:conditional move  
$L476:
	lw	$4,40($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L479
	xor	$3,$20,$23
	.set	macro
	.set	reorder

	lhu	$6,4($18)
	srl	$2,$3,16
	xor	$2,$2,$6
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lw	$4,%lo(max_udp_frag_entry)($2)
	andi	$5,$19,0x00ff
	srl	$2,$3,8
	xor	$2,$2,$5
	xor	$3,$3,$2
	addiu	$4,$4,-1
	lui	$9,%hi(udp_cache_list_free)
	and	$10,$3,$4
	lw	$2,28($sp)
	lw	$4,32($sp)
	lw	$8,%lo(udp_cache_list_free)($9)
	andi	$3,$2,0xffff
	.set	noreorder
	.set	nomacro
	beq	$8,$0,$L595
	andi	$7,$4,0xffff
	.set	macro
	.set	reorder

	lw	$4,48($8)
	li	$2,1
	sh	$3,12($8)
	sh	$6,2($8)
	sb	$5,1($8)
	sh	$7,14($8)
	sb	$2,0($8)
	sw	$20,4($8)
	sw	$23,8($8)
	lw	$3,52($8)
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L485
	addiu	$2,$9,%lo(udp_cache_list_free)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L486
	sw	$3,52($4)
	.set	macro
	.set	reorder

$L485:
	sw	$3,4($2)
$L486:
	lw	$3,52($8)
	lw	$2,48($8)
	addiu	$4,$9,%lo(udp_cache_list_free)
	lui	$5,%hi(udp_cache_table)
	sw	$2,0($3)
	lw	$2,8($4)
	sll	$3,$10,1
	addu	$3,$3,$10
	addiu	$2,$2,-1
	sw	$2,8($4)
	sw	$0,40($8)
	lw	$2,%lo(udp_cache_table)($5)
	sll	$3,$3,2
	addiu	$6,$8,40
	lw	$2,0($2)
	addiu	$4,$8,16
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$2,44($8)
	lw	$2,%lo(udp_cache_table)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	lw	$2,4($2)
	#nop
	sw	$8,0($2)
	lw	$2,%lo(udp_cache_table)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$2,$3,$2
	sw	$6,4($2)
	lw	$2,%lo(udp_cache_table)($5)
	#nop
	lw	$2,0($2)
	#nop
	addu	$3,$3,$2
	lw	$2,8($3)
	#nop
	addiu	$2,$2,1
	sw	$2,8($3)
	lui	$2,%hi(jiffies)
	lw	$2,%lo(jiffies)($2)
	#nop
	addiu	$2,$2,1000
	.set	noreorder
	.set	nomacro
	jal	add_timer
	sw	$2,24($8)
	.set	macro
	.set	reorder

	j	$L488
$L479:
	lw	$2,44($sp)
	#nop
	beq	$2,$0,$L488
	.set	noreorder
	.set	nomacro
	jal	del_timer
	addiu	$4,$22,16
	.set	macro
	.set	reorder

	lw	$2,8($22)
	lw	$3,4($22)
	lhu	$4,2($22)
	lbu	$5,1($22)
	xor	$3,$3,$2
	srl	$2,$3,16
	xor	$2,$2,$4
	xor	$3,$3,$2
	lui	$2,%hi(max_udp_frag_entry)
	lw	$4,%lo(max_udp_frag_entry)($2)
	lw	$6,40($22)
	srl	$2,$3,8
	xor	$2,$2,$5
	addiu	$4,$4,-1
	xor	$3,$3,$2
	and	$5,$3,$4
	sb	$0,0($22)
	lw	$4,44($22)
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L492
	lui	$2,%hi(udp_cache_table)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L493
	sw	$4,44($6)
	.set	macro
	.set	reorder

$L492:
	lw	$3,%lo(udp_cache_table)($2)
	sll	$2,$5,1
	addu	$2,$2,$5
	lw	$3,0($3)
	sll	$2,$2,2
	addu	$2,$2,$3
	sw	$4,4($2)
$L493:
	lw	$4,44($22)
	lw	$2,40($22)
	sll	$3,$5,1
	addu	$3,$3,$5
	sw	$2,0($4)
	lui	$2,%hi(udp_cache_table)
	lw	$2,%lo(udp_cache_table)($2)
	sll	$3,$3,2
	lui	$4,%hi(udp_cache_list_free)
	lw	$2,0($2)
	addiu	$4,$4,%lo(udp_cache_list_free)
	addiu	$5,$22,48
	addu	$3,$3,$2
	lw	$2,8($3)
	#nop
	addiu	$2,$2,-1
	sw	$2,8($3)
	sw	$0,48($22)
	lw	$2,4($4)
	#nop
	sw	$2,52($22)
	lw	$2,4($4)
	#nop
	sw	$22,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	sw	$2,8($4)
$L488:
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
	beq	$2,$3,$L435
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

	lbu	$3,1($17)
	#nop
	sltu	$2,$3,13
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L560
	sll	$2,$3,2
	.set	macro
	.set	reorder

	lui	$3,%hi($L583)
	addiu	$3,$3,%lo($L583)
	addu	$2,$2,$3
	lw	$2,0($2)
	lhu	$5,10($18)
	j	$2
	.rdata
	.align	2
$L583:
	.word	$L498
	.word	$L500
	.word	$L516
	.word	$L516
	.word	$L531
	.word	$L560
	.word	$L560
	.word	$L560
	.word	$L546
	.word	$L560
	.word	$L560
	.word	$L560
	.word	$L546
	.section	.text.enter_fast_path
$L500:
	lw	$3,24($17)
	srl	$2,$20,16
	andi	$4,$20,0xffff
	addu	$4,$4,$2
	andi	$2,$3,0xffff
	subu	$4,$4,$2
	srl	$3,$3,16
	subu	$4,$4,$3
	andi	$2,$5,0xffff
	addu	$4,$4,$2
	.set	noreorder
	.set	nomacro
	bgez	$4,$L502
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L501
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L502:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,10($18)
$L501:
	lw	$3,36($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L520
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L505
	beq	$16,$0,$L520
	lhu	$6,0($16)
	#nop
	beq	$6,$0,$L520
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
	bgez	$5,$L508
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
	j	$L520
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L508:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	.set	noreorder
	.set	nomacro
	j	$L520
	sh	$5,0($16)
	.set	macro
	.set	reorder

$L505:
	lw	$4,12($18)
	lw	$3,24($17)
	lhu	$5,0($16)
	srl	$2,$4,16
	andi	$4,$4,0xffff
	addu	$4,$4,$2
	andi	$2,$3,0xffff
	subu	$4,$4,$2
	srl	$3,$3,16
	subu	$4,$4,$3
	addu	$4,$4,$5
	.set	noreorder
	.set	nomacro
	bgez	$4,$L512
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L511
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L512:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,0($16)
$L511:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L520
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	#nop
	bne	$3,$2,$L520
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

	j	$L520
$L516:
	lw	$3,24($17)
	srl	$2,$20,16
	andi	$4,$20,0xffff
	addu	$4,$4,$2
	andi	$2,$3,0xffff
	subu	$4,$4,$2
	srl	$3,$3,16
	subu	$4,$4,$3
	andi	$2,$5,0xffff
	addu	$4,$4,$2
	.set	noreorder
	.set	nomacro
	bgez	$4,$L518
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L517
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L518:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,10($18)
$L517:
	lw	$4,36($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L520
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L521
	beq	$16,$0,$L526
	lhu	$6,0($16)
	#nop
	beq	$6,$0,$L526
	lw	$4,12($18)
	lw	$3,24($17)
	lhu	$5,0($21)
	srl	$2,$4,16
	andi	$4,$4,0xffff
	addu	$4,$4,$2
	andi	$2,$3,0xffff
	subu	$4,$4,$2
	srl	$3,$3,16
	lw	$2,28($17)
	subu	$4,$4,$3
	addu	$4,$4,$5
	subu	$4,$4,$2
	addu	$4,$4,$6
	.set	noreorder
	.set	nomacro
	bgez	$4,$L524
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L526
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L524:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	.set	noreorder
	.set	nomacro
	j	$L526
	sh	$4,0($16)
	.set	macro
	.set	reorder

$L521:
	lw	$4,12($18)
	lw	$3,24($17)
	lhu	$5,0($21)
	srl	$2,$4,16
	andi	$6,$4,0xffff
	addu	$6,$6,$2
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$4,28($17)
	subu	$6,$6,$3
	lhu	$2,0($16)
	addu	$6,$6,$5
	subu	$6,$6,$4
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	bgez	$6,$L528
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
	j	$L527
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L528:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L527:
	lw	$2,12($21)
	li	$3,1048576			# 0x100000
	and	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L526
	li	$2,40			# 0x28
	.set	macro
	.set	reorder

	lhu	$3,2($18)
	#nop
	bne	$3,$2,$L526
	lw	$4,24($sp)
	.set	noreorder
	.set	nomacro
	jal	skb_trim
	li	$5,40			# 0x28
	.set	macro
	.set	reorder

$L526:
	lw	$2,28($17)
	#nop
	sh	$2,0($21)
$L520:
	lw	$2,24($17)
	.set	noreorder
	.set	nomacro
	j	$L498
	sw	$2,12($18)
	.set	macro
	.set	reorder

$L531:
	lw	$2,32($17)
	andi	$6,$23,0xffff
	addu	$4,$6,$fp
	andi	$3,$2,0xffff
	subu	$4,$4,$3
	srl	$2,$2,16
	subu	$4,$4,$2
	andi	$2,$5,0xffff
	addu	$4,$4,$2
	.set	noreorder
	.set	nomacro
	bgez	$4,$L533
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L532
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L533:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,10($18)
$L532:
	lw	$2,36($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L603
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L536
	beq	$16,$0,$L603
	lhu	$4,0($16)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L603
	addu	$6,$6,$fp
	.set	macro
	.set	reorder

	lw	$2,32($17)
	.set	noreorder
	.set	nomacro
	j	$L610
	andi	$3,$2,0xffff
	.set	macro
	.set	reorder

$L536:
	lw	$2,32($17)
	lhu	$4,0($16)
	addu	$6,$6,$fp
	andi	$3,$2,0xffff
$L610:
	subu	$6,$6,$3
	srl	$2,$2,16
	subu	$6,$6,$2
	addu	$6,$6,$4
	.set	noreorder
	.set	nomacro
	bgez	$6,$L543
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
	j	$L603
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L543:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L603
	sh	$6,0($16)
	.set	macro
	.set	reorder

$L546:
	lw	$2,32($17)
	andi	$6,$23,0xffff
	addu	$4,$6,$fp
	andi	$3,$2,0xffff
	subu	$4,$4,$3
	srl	$2,$2,16
	subu	$4,$4,$2
	andi	$2,$5,0xffff
	addu	$4,$4,$2
	.set	noreorder
	.set	nomacro
	bgez	$4,$L548
	andi	$2,$4,0xffff
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L547
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L548:
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,10($18)
$L547:
	lw	$3,36($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L603
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L551
	beq	$16,$0,$L556
	lhu	$5,0($16)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L556
	addu	$6,$6,$fp
	.set	macro
	.set	reorder

	lw	$3,32($17)
	lhu	$4,2($21)
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$2,36($17)
	subu	$6,$6,$3
	addu	$6,$6,$4
	subu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L606
	addu	$6,$6,$5
	.set	macro
	.set	reorder

$L551:
	lw	$3,32($17)
	addu	$6,$6,$fp
	lhu	$5,2($21)
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$4,36($17)
	subu	$6,$6,$3
	lhu	$2,0($16)
	addu	$6,$6,$5
	subu	$6,$6,$4
	addu	$6,$6,$2
$L606:
	.set	noreorder
	.set	nomacro
	bgez	$6,$L558
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
	j	$L556
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L558:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L556:
	lw	$2,36($17)
	.set	noreorder
	.set	nomacro
	j	$L603
	sh	$2,2($21)
	.set	macro
	.set	reorder

$L560:
	lw	$2,24($17)
	srl	$7,$20,16
	andi	$6,$20,0xffff
	andi	$3,$2,0xffff
	addu	$5,$6,$7
	lhu	$4,10($18)
	subu	$5,$5,$3
	srl	$2,$2,16
	subu	$5,$5,$2
	addu	$5,$5,$4
	.set	noreorder
	.set	nomacro
	bgez	$5,$L562
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
	j	$L561
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L562:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L561:
	lw	$2,32($17)
	andi	$8,$23,0xffff
	addu	$5,$8,$fp
	andi	$3,$2,0xffff
	lhu	$4,10($18)
	subu	$5,$5,$3
	srl	$2,$2,16
	subu	$5,$5,$2
	addu	$5,$5,$4
	.set	noreorder
	.set	nomacro
	bgez	$5,$L565
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
	j	$L564
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L565:
	sra	$3,$5,16
	addu	$5,$3,$2
	sra	$2,$5,16
	addu	$5,$5,$2
	sh	$5,10($18)
$L564:
	lw	$4,36($sp)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L567
	li	$2,17			# 0x11
	.set	macro
	.set	reorder

	bne	$19,$2,$L568
	beq	$16,$0,$L576
	lhu	$5,0($16)
	#nop
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L576
	addu	$6,$6,$7
	.set	macro
	.set	reorder

	lw	$3,24($17)
	lhu	$4,0($21)
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$2,28($17)
	subu	$6,$6,$3
	addu	$6,$6,$4
	subu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	j	$L608
	addu	$6,$6,$5
	.set	macro
	.set	reorder

$L568:
	lw	$3,24($17)
	addu	$6,$6,$7
	lhu	$5,0($21)
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$4,28($17)
	subu	$6,$6,$3
	lhu	$2,0($16)
	addu	$6,$6,$5
	subu	$6,$6,$4
	addu	$6,$6,$2
$L608:
	.set	noreorder
	.set	nomacro
	bgez	$6,$L578
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
	j	$L577
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L578:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L577:
	lw	$3,32($17)
	addu	$6,$8,$fp
	lhu	$5,2($21)
	andi	$2,$3,0xffff
	subu	$6,$6,$2
	srl	$3,$3,16
	lw	$4,36($17)
	subu	$6,$6,$3
	lhu	$2,0($16)
	addu	$6,$6,$5
	subu	$6,$6,$4
	addu	$6,$6,$2
	.set	noreorder
	.set	nomacro
	bgez	$6,$L581
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
	j	$L576
	sh	$2,0($16)
	.set	macro
	.set	reorder

$L581:
	sra	$3,$6,16
	addu	$6,$3,$2
	sra	$2,$6,16
	addu	$6,$6,$2
	sh	$6,0($16)
$L576:
	lw	$2,28($17)
	#nop
	sh	$2,0($21)
	lw	$2,36($17)
	#nop
	sh	$2,2($21)
$L567:
	lw	$2,24($17)
	#nop
	sw	$2,12($18)
$L603:
	lw	$2,32($17)
	#nop
	sw	$2,16($18)
$L498:
	lbu	$3,8($18)
	lbu	$4,9($18)
	lhu	$6,10($18)
	addiu	$5,$3,-1
	andi	$2,$5,0x00ff
	andi	$3,$3,0x00ff
	sll	$3,$3,8
	sll	$2,$2,8
	or	$2,$2,$4
	or	$4,$3,$4
	subu	$4,$4,$2
	addu	$4,$4,$6
	.set	noreorder
	.set	nomacro
	bgez	$4,$L585
	sb	$5,8($18)
	.set	macro
	.set	reorder

	subu	$4,$0,$4
	sra	$2,$4,16
	andi	$3,$4,0xffff
	addu	$4,$2,$3
	sra	$2,$4,16
	addu	$4,$4,$2
	nor	$2,$0,$4
	.set	noreorder
	.set	nomacro
	j	$L584
	sh	$2,10($18)
	.set	macro
	.set	reorder

$L585:
	andi	$2,$4,0xffff
	sra	$3,$4,16
	addu	$4,$3,$2
	sra	$2,$4,16
	addu	$4,$4,$2
	sh	$4,10($18)
$L584:
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
	sw	$2,40($17)
	lw	$6,24($sp)
	.set	noreorder
	.set	nomacro
	jal	fast_path_post_process_xmit_check
	move	$5,$21
	.set	macro
	.set	reorder

	li	$3,1			# 0x1
	.set	noreorder
	.set	nomacro
	beq	$2,$3,$L435
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

	lw	$4,24($sp)
	jal	ip_finish_output3
	.set	noreorder
	.set	nomacro
	j	$L435
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

$L469:
	lw	$17,64($17)
	j	$L599
$L592:
	lw	$4,24($sp)
	jal	rtl_set_skb_inDev
	lw	$6,24($sp)
	move	$4,$18
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

$L595:
	lw	$4,24($sp)
	jal	rtl_dst_release
	move	$4,$0
$L435:
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
	move	$2,$4
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
	sw	$16,16($sp)
	sw	$31,20($sp)
	lui	$2,%hi(path_list_inuse)
	lw	$2,%lo(path_list_inuse)($2)
	move	$16,$5
	move	$7,$0
$L624:
	beq	$2,$0,$L621
	move	$6,$0

	lw	$2,72($2)
	j	$L624
	addiu	$7,$7,1

$L621:
	lui	$2,%hi(napt_list_inuse)
	lw	$2,%lo(napt_list_inuse)($2)
	nop
$L625:
	beq	$2,$0,$L623
	lui	$5,%hi($LC1)

	lw	$2,44($2)
	j	$L625
	addiu	$6,$6,1

$L623:
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
	.frame	$sp,56,$31		# vars= 16, regs= 6/0, args= 16, gp= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L631:
$L672:
	addiu	$sp,$sp,-56
	sw	$20,48($sp)
	sw	$18,40($sp)
	sw	$17,36($sp)
	sw	$16,32($sp)
	sw	$31,52($sp)
	sw	$19,44($sp)
	lui	$2,%hi(malloc_sizes+4)
	move	$17,$4
	lw	$4,%lo(malloc_sizes+4)($2)
	move	$20,$5
	jal	kmem_cache_alloc
	li	$5,32			# 0x20

	move	$16,$2
	lui	$2,%hi(table_napt)
	sw	$16,%lo(table_napt)($2)
	move	$18,$2
	beq	$16,$0,$L626
	li	$3,-1			# 0xffffffffffffffff

$L682:
$L723:
	lui	$2,%hi(napt_list_inuse)
	lui	$19,%hi(napt_list_free)
	sll	$4,$17,1
	addiu	$3,$2,%lo(napt_list_inuse)
	addiu	$6,$19,%lo(napt_list_free)
	addu	$4,$4,$17
	sw	$0,%lo(napt_list_inuse)($2)
	sw	$3,4($3)
	lui	$2,%hi(napt_table_list_max)
	sw	$0,8($3)
	sll	$4,$4,2
	li	$5,32			# 0x20
	sw	$6,4($6)
	sw	$0,8($6)
	sw	$17,%lo(napt_table_list_max)($2)
	jal	__kmalloc
	sw	$0,%lo(napt_list_free)($19)

	sw	$2,0($16)
	lw	$2,%lo(table_napt)($18)
	nop
	lw	$2,0($2)
	nop
	beq	$2,$0,$L626
	li	$3,-1			# 0xffffffffffffffff

	j	$L726
	move	$16,$0

$L790:
	j	$L626
	li	$3,-2			# 0xfffffffffffffffe

$L726:
	blez	$17,$L787
	nop

	move	$4,$0
$L731:
	lw	$2,%lo(table_napt)($18)
	addiu	$16,$16,1
	slt	$3,$16,$17
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(table_napt)($18)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(table_napt)($18)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$2,4($2)
	bne	$3,$0,$L731
	addiu	$4,$4,12

$L787:
	blez	$20,$L789
	move	$16,$0

$L785:
$L739:
$L780:
	lui	$2,%hi(malloc_sizes+12)
	lw	$4,%lo(malloc_sizes+12)($2)
	li	$5,32			# 0x20
	jal	kmem_cache_alloc
	addiu	$16,$16,1

	move	$3,$2
	addiu	$4,$19,%lo(napt_list_free)
	addiu	$5,$2,44
	beq	$2,$0,$L790
	slt	$6,$16,$20

	sw	$0,44($2)
	lw	$2,4($4)
	nop
	sw	$2,48($3)
	lw	$2,4($4)
	nop
	sw	$3,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	bne	$6,$0,$L785
	sw	$2,8($4)

$L789:
	move	$3,$0
$L626:
	lw	$31,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	move	$2,$3
	j	$31
	addiu	$sp,$sp,56

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
	.frame	$sp,56,$31		# vars= 16, regs= 6/0, args= 16, gp= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
$L796:
$L837:
	addiu	$sp,$sp,-56
	sw	$20,48($sp)
	sw	$18,40($sp)
	sw	$17,36($sp)
	sw	$16,32($sp)
	sw	$31,52($sp)
	sw	$19,44($sp)
	lui	$2,%hi(malloc_sizes+4)
	move	$17,$4
	lw	$4,%lo(malloc_sizes+4)($2)
	move	$20,$5
	jal	kmem_cache_alloc
	li	$5,32			# 0x20

	move	$16,$2
	lui	$2,%hi(table_path)
	sw	$16,%lo(table_path)($2)
	move	$18,$2
	beq	$16,$0,$L791
	li	$3,-1			# 0xffffffffffffffff

$L847:
$L888:
	lui	$2,%hi(path_list_inuse)
	lui	$19,%hi(path_list_free)
	sll	$4,$17,1
	addiu	$3,$2,%lo(path_list_inuse)
	addiu	$6,$19,%lo(path_list_free)
	addu	$4,$4,$17
	sw	$0,%lo(path_list_inuse)($2)
	sw	$3,4($3)
	lui	$2,%hi(path_table_list_max)
	sw	$0,8($3)
	sll	$4,$4,2
	li	$5,32			# 0x20
	sw	$6,4($6)
	sw	$0,8($6)
	sw	$17,%lo(path_table_list_max)($2)
	jal	__kmalloc
	sw	$0,%lo(path_list_free)($19)

	sw	$2,0($16)
	lw	$2,%lo(table_path)($18)
	nop
	lw	$2,0($2)
	nop
	beq	$2,$0,$L791
	li	$3,-1			# 0xffffffffffffffff

	j	$L891
	move	$16,$0

$L955:
	j	$L791
	li	$3,-2			# 0xfffffffffffffffe

$L891:
	blez	$17,$L952
	nop

	move	$4,$0
$L896:
	lw	$2,%lo(table_path)($18)
	addiu	$16,$16,1
	slt	$3,$16,$17
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,8($2)
	lw	$2,%lo(table_path)($18)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$0,0($2)
	lw	$2,%lo(table_path)($18)
	nop
	lw	$2,0($2)
	nop
	addu	$2,$4,$2
	sw	$2,4($2)
	bne	$3,$0,$L896
	addiu	$4,$4,12

$L952:
	blez	$20,$L954
	move	$16,$0

$L950:
$L904:
$L945:
	lui	$2,%hi(malloc_sizes+20)
	lw	$4,%lo(malloc_sizes+20)($2)
	li	$5,32			# 0x20
	jal	kmem_cache_alloc
	addiu	$16,$16,1

	move	$3,$2
	addiu	$4,$19,%lo(path_list_free)
	addiu	$5,$2,72
	beq	$2,$0,$L955
	slt	$6,$16,$20

	sw	$0,72($2)
	lw	$2,4($4)
	nop
	sw	$2,76($3)
	lw	$2,4($4)
	nop
	sw	$3,0($2)
	lw	$2,8($4)
	sw	$5,4($4)
	addiu	$2,$2,1
	bne	$6,$0,$L950
	sw	$2,8($4)

$L954:
	move	$3,$0
$L791:
	lw	$31,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	lw	$16,32($sp)
	move	$2,$3
	j	$31
	addiu	$sp,$sp,56

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
	beq	$4,$0,$L958
	addiu	$7,$7,%lo($LC4)

	j	sprintf
	nop

$L958:
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
