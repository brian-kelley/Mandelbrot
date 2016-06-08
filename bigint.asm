	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 11
	.globl	_BigIntCtor
	.align	4, 0x90
_BigIntCtor:                            ## @BigIntCtor
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp2:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	rbx
	push	rax
Ltmp3:
	.cfi_offset rbx, -40
Ltmp4:
	.cfi_offset r14, -32
Ltmp5:
	.cfi_offset r15, -24
	movsxd	r14, edi
	lea	r15, qword ptr [8*r14]
	mov	rdi, r15
	call	_malloc
	mov	rbx, rax
	mov	rdi, rbx
	mov	rsi, r15
	call	___bzero
	mov	rax, rbx
	mov	edx, r14d
	add	rsp, 8
	pop	rbx
	pop	r14
	pop	r15
	pop	rbp
	ret
	.cfi_endproc

	.globl	_BigIntCopy
	.align	4, 0x90
_BigIntCopy:                            ## @BigIntCopy
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp6:
	.cfi_def_cfa_offset 16
Ltmp7:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp8:
	.cfi_def_cfa_register rbp
	push	r14
	push	rbx
Ltmp9:
	.cfi_offset rbx, -32
Ltmp10:
	.cfi_offset r14, -24
	mov	rbx, rdi
	movsxd	r14, dword ptr [rbx + 8]
	lea	rdi, qword ptr [8*r14]
	call	_malloc
	test	r14, r14
	jle	LBB1_21
## BB#1:                                ## %.lr.ph
	mov	rcx, qword ptr [rbx]
	lea	edi, dword ptr [r14 - 1]
	lea	r8, qword ptr [rdi + 1]
	xor	edx, edx
	movabs	rbx, 8589934588
	and	rbx, r8
	je	LBB1_11
## BB#2:                                ## %vector.memcheck
	lea	rsi, qword ptr [rcx + 8*rdi]
	xor	edx, edx
	cmp	rax, rsi
	ja	LBB1_4
## BB#3:                                ## %vector.memcheck
	lea	rsi, qword ptr [rax + 8*rdi]
	cmp	rcx, rsi
	jbe	LBB1_11
LBB1_4:                                 ## %vector.body.preheader
	lea	rdx, qword ptr [rdi + 1]
	and	rdx, -4
	add	rdx, -4
	shr	rdx, 2
	xor	r9d, r9d
	inc	rdx
	je	LBB1_6
## BB#5:                                ## %vector.body.preheader
	mov	rsi, rdx
	and	rsi, 1
	je	LBB1_7
LBB1_6:                                 ## %vector.body.unr
	movups	xmm0, xmmword ptr [rcx]
	movups	xmm1, xmmword ptr [rcx + 16]
	movups	xmmword ptr [rax], xmm0
	movups	xmmword ptr [rax + 16], xmm1
	mov	r9d, 4
LBB1_7:                                 ## %vector.body.preheader.split
	cmp	rdx, 2
	jb	LBB1_10
## BB#8:                                ## %vector.body.preheader.split.split
	lea	rdx, qword ptr [rax + 8*r9 + 48]
	lea	rsi, qword ptr [rcx + 8*r9 + 48]
	inc	rdi
	and	rdi, -4
	sub	rdi, r9
	.align	4, 0x90
LBB1_9:                                 ## %vector.body
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm0, xmmword ptr [rsi - 48]
	movups	xmm1, xmmword ptr [rsi - 32]
	movups	xmmword ptr [rdx - 48], xmm0
	movups	xmmword ptr [rdx - 32], xmm1
	movups	xmm0, xmmword ptr [rsi - 16]
	movups	xmm1, xmmword ptr [rsi]
	movups	xmmword ptr [rdx - 16], xmm0
	movups	xmmword ptr [rdx], xmm1
	add	rdx, 64
	add	rsi, 64
	add	rdi, -8
	jne	LBB1_9
LBB1_10:
	mov	rdx, rbx
LBB1_11:                                ## %middle.block
	cmp	r8, rdx
	je	LBB1_21
## BB#12:                               ## %scalar.ph.preheader
	lea	esi, dword ptr [rdx + 1]
	cmp	r14d, esi
	mov	ebx, esi
	cmovge	ebx, r14d
	inc	ebx
	mov	edi, ebx
	sub	edi, esi
	and	edi, 3
	sub	ebx, esi
	je	LBB1_14
## BB#13:                               ## %scalar.ph.preheader
	test	edi, edi
	je	LBB1_19
LBB1_14:                                ## %unr.cmp15
	cmp	edi, 1
	je	LBB1_18
## BB#15:                               ## %unr.cmp
	cmp	edi, 2
	je	LBB1_17
## BB#16:                               ## %scalar.ph.unr
	mov	rsi, qword ptr [rcx + 8*rdx]
	mov	qword ptr [rax + 8*rdx], rsi
	inc	rdx
LBB1_17:                                ## %scalar.ph.unr10
	mov	rsi, qword ptr [rcx + 8*rdx]
	mov	qword ptr [rax + 8*rdx], rsi
	inc	rdx
LBB1_18:                                ## %scalar.ph.unr12
	mov	rsi, qword ptr [rcx + 8*rdx]
	mov	qword ptr [rax + 8*rdx], rsi
	inc	rdx
LBB1_19:                                ## %scalar.ph.preheader.split
	cmp	ebx, 4
	jb	LBB1_21
	.align	4, 0x90
LBB1_20:                                ## %scalar.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rsi, qword ptr [rcx + 8*rdx]
	mov	qword ptr [rax + 8*rdx], rsi
	mov	rsi, qword ptr [rcx + 8*rdx + 8]
	mov	qword ptr [rax + 8*rdx + 8], rsi
	mov	rsi, qword ptr [rcx + 8*rdx + 16]
	mov	qword ptr [rax + 8*rdx + 16], rsi
	mov	rsi, qword ptr [rcx + 8*rdx + 24]
	mov	qword ptr [rax + 8*rdx + 24], rsi
	add	rdx, 4
	cmp	edx, r14d
	jl	LBB1_20
LBB1_21:                                ## %._crit_edge
	mov	edx, r14d
	pop	rbx
	pop	r14
	pop	rbp
	ret
	.cfi_endproc

	.globl	_BigIntDtor
	.align	4, 0x90
_BigIntDtor:                            ## @BigIntDtor
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp11:
	.cfi_def_cfa_offset 16
Ltmp12:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp13:
	.cfi_def_cfa_register rbp
	mov	rdi, qword ptr [rdi]
	pop	rbp
	jmp	_free                   ## TAILCALL
	.cfi_endproc

	.globl	_biAddWord
	.align	4, 0x90
_biAddWord:                             ## @biAddWord
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp14:
	.cfi_def_cfa_offset 16
Ltmp15:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp16:
	.cfi_def_cfa_register rbp
	movabs	r8, 9223372036854775807
	mov	rcx, rsi
	and	rcx, r8
	movsxd	rdx, edx
	mov	r9, qword ptr [rdi]
	mov	rdi, qword ptr [r9 + 8*rdx]
	add	rcx, rdi
	mov	rax, rcx
	shr	rax, 63
	add	rdi, rsi
	and	rdi, r8
	mov	qword ptr [r9 + 8*rdx], rdi
	test	edx, edx
	jle	LBB3_5
## BB#1:
	test	rcx, rcx
	jns	LBB3_5
## BB#2:                                ## %.lr.ph.preheader
	lea	rsi, qword ptr [r9 + 8*rdx - 8]
	dec	edx
	.align	4, 0x90
LBB3_3:                                 ## %.lr.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rcx, qword ptr [rsi]
	inc	rcx
	mov	rax, rcx
	shr	rax, 63
	mov	rdi, rcx
	and	rdi, r8
	mov	qword ptr [rsi], rdi
	test	edx, edx
	jle	LBB3_5
## BB#4:                                ## %.lr.ph
                                        ##   in Loop: Header=BB3_3 Depth=1
	add	rsi, -8
	dec	edx
	test	rcx, rcx
	js	LBB3_3
LBB3_5:                                 ## %._crit_edge
                                        ## kill: AL<def> AL<kill> RAX<kill>
	pop	rbp
	ret
	.cfi_endproc

	.globl	_bimul
	.align	4, 0x90
_bimul:                                 ## @bimul
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp17:
	.cfi_def_cfa_offset 16
Ltmp18:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp19:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	sub	rsp, 72
Ltmp20:
	.cfi_offset rbx, -56
Ltmp21:
	.cfi_offset r12, -48
Ltmp22:
	.cfi_offset r13, -40
Ltmp23:
	.cfi_offset r14, -32
Ltmp24:
	.cfi_offset r15, -24
	mov	qword ptr [rbp - 72], rdx ## 8-byte Spill
	mov	qword ptr [rbp - 80], rsi ## 8-byte Spill
	mov	qword ptr [rbp - 64], rdi ## 8-byte Spill
	movsxd	rbx, dword ptr [rsi + 8]
	test	rbx, rbx
	jle	LBB4_15
## BB#1:                                ## %.preheader.lr.ph.split.us
	mov	rax, qword ptr [rbp - 64] ## 8-byte Reload
	mov	rdi, qword ptr [rax]
	lea	eax, dword ptr [rbx + rbx]
	cmp	eax, 1
	lea	eax, dword ptr [rbx + rbx - 1]
	lea	rax, qword ptr [8*rax + 8]
	mov	esi, 8
	cmovg	rsi, rax
	call	___bzero
	lea	rcx, qword ptr [rbx + rbx - 2]
	lea	rdx, qword ptr [rbx + rbx - 3]
	lea	rax, qword ptr [rbx - 1]
	mov	qword ptr [rbp - 104], rax ## 8-byte Spill
	movabs	r12, 9223372036854775807
	mov	r8, rax
	.align	4, 0x90
LBB4_14:                                ## %.lr.ph.us
                                        ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB4_2 Depth 2
                                        ##       Child Loop BB4_5 Depth 3
                                        ##       Child Loop BB4_10 Depth 3
	mov	qword ptr [rbp - 96], rdx ## 8-byte Spill
	mov	qword ptr [rbp - 88], rcx ## 8-byte Spill
	mov	r14, rdx
	mov	r15, rcx
	mov	r13, qword ptr [rbp - 104] ## 8-byte Reload
	.align	4, 0x90
LBB4_2:                                 ##   Parent Loop BB4_14 Depth=1
                                        ## =>  This Loop Header: Depth=2
                                        ##       Child Loop BB4_5 Depth 3
                                        ##       Child Loop BB4_10 Depth 3
	mov	rax, qword ptr [rbp - 80] ## 8-byte Reload
	mov	rax, qword ptr [rax]
	mov	rbx, r8
	mov	rdi, qword ptr [rax + 8*rbx]
	mov	rax, qword ptr [rbp - 72] ## 8-byte Reload
	mov	rax, qword ptr [rax]
	mov	rsi, qword ptr [rax + 8*r13]
	lea	rdx, qword ptr [rbp - 48]
	lea	rcx, qword ptr [rbp - 56]
	call	_longmul
	lea	rcx, qword ptr [r13 + rbx]
	mov	r8, rbx
	mov	rdi, qword ptr [rbp - 56]
	shld	qword ptr [rbp - 48], rdi, 1
	mov	rdx, rdi
	and	rdx, r12
	mov	qword ptr [rbp - 56], rdx
	mov	rax, qword ptr [rbp - 64] ## 8-byte Reload
	mov	rax, qword ptr [rax]
	mov	rsi, qword ptr [rax + 8*rcx + 8]
	add	rdi, rsi
	and	rdi, r12
	mov	qword ptr [rax + 8*rcx + 8], rdi
	test	ecx, ecx
	js	LBB4_7
## BB#3:                                ##   in Loop: Header=BB4_2 Depth=2
	add	rsi, rdx
	jns	LBB4_7
## BB#4:                                ## %.lr.ph.i.us.preheader
                                        ##   in Loop: Header=BB4_2 Depth=2
	lea	rdx, qword ptr [rax + 8*r15]
	mov	esi, r15d
	.align	4, 0x90
LBB4_5:                                 ## %.lr.ph.i.us
                                        ##   Parent Loop BB4_14 Depth=1
                                        ##     Parent Loop BB4_2 Depth=2
                                        ## =>    This Inner Loop Header: Depth=3
	mov	rbx, qword ptr [rdx]
	inc	rbx
	mov	rdi, rbx
	and	rdi, r12
	mov	qword ptr [rdx], rdi
	test	esi, esi
	jle	LBB4_7
## BB#6:                                ## %.lr.ph.i.us
                                        ##   in Loop: Header=BB4_5 Depth=3
	dec	esi
	add	rdx, -8
	test	rbx, rbx
	js	LBB4_5
LBB4_7:                                 ## %biAddWord.exit.us
                                        ##   in Loop: Header=BB4_2 Depth=2
	mov	rdx, qword ptr [rbp - 48]
	mov	rsi, qword ptr [rax + 8*rcx]
	lea	rdi, qword ptr [rsi + rdx]
	and	rdi, r12
	mov	qword ptr [rax + 8*rcx], rdi
	test	ecx, ecx
	jle	LBB4_12
## BB#8:                                ## %biAddWord.exit.us
                                        ##   in Loop: Header=BB4_2 Depth=2
	and	rdx, r12
	add	rsi, rdx
	jns	LBB4_12
## BB#9:                                ## %.lr.ph.i7.us.preheader
                                        ##   in Loop: Header=BB4_2 Depth=2
	lea	rax, qword ptr [rax + 8*r14]
	mov	ecx, r14d
	.align	4, 0x90
LBB4_10:                                ## %.lr.ph.i7.us
                                        ##   Parent Loop BB4_14 Depth=1
                                        ##     Parent Loop BB4_2 Depth=2
                                        ## =>    This Inner Loop Header: Depth=3
	mov	rdx, qword ptr [rax]
	inc	rdx
	mov	rsi, rdx
	and	rsi, r12
	mov	qword ptr [rax], rsi
	test	ecx, ecx
	jle	LBB4_12
## BB#11:                               ## %.lr.ph.i7.us
                                        ##   in Loop: Header=BB4_10 Depth=3
	dec	ecx
	add	rax, -8
	test	rdx, rdx
	js	LBB4_10
LBB4_12:                                ## %biAddWord.exit9.us
                                        ##   in Loop: Header=BB4_2 Depth=2
	mov	rax, r13
	dec	rax
	dec	r15
	dec	r14
	test	r13d, r13d
	mov	r13, rax
	jg	LBB4_2
## BB#13:                               ##   in Loop: Header=BB4_14 Depth=1
	mov	rax, r8
	dec	rax
	mov	rcx, qword ptr [rbp - 88] ## 8-byte Reload
	dec	rcx
	mov	rdx, qword ptr [rbp - 96] ## 8-byte Reload
	dec	rdx
	test	r8d, r8d
	mov	r8, rax
	jg	LBB4_14
LBB4_15:                                ## %._crit_edge13
	add	rsp, 72
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	ret
	.cfi_endproc

	.globl	_biadd
	.align	4, 0x90
_biadd:                                 ## @biadd
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp25:
	.cfi_def_cfa_offset 16
Ltmp26:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp27:
	.cfi_def_cfa_register rbp
	push	r14
	push	rbx
Ltmp28:
	.cfi_offset rbx, -32
Ltmp29:
	.cfi_offset r14, -24
	mov	r8d, dword ptr [rdi + 8]
	test	r8d, r8d
	jle	LBB5_21
## BB#1:                                ## %.lr.ph6
	mov	r10, qword ptr [rsi]
	mov	rsi, qword ptr [rdi]
	lea	ecx, dword ptr [r8 - 1]
	lea	r9, qword ptr [rcx + 1]
	xor	eax, eax
	movabs	r11, 8589934588
	and	r11, r9
	je	LBB5_11
## BB#2:                                ## %vector.memcheck
	lea	rbx, qword ptr [r10 + 8*rcx]
	xor	eax, eax
	cmp	rsi, rbx
	ja	LBB5_4
## BB#3:                                ## %vector.memcheck
	lea	rbx, qword ptr [rsi + 8*rcx]
	cmp	r10, rbx
	jbe	LBB5_11
LBB5_4:                                 ## %vector.body.preheader
	lea	rax, qword ptr [rcx + 1]
	and	rax, -4
	add	rax, -4
	shr	rax, 2
	xor	r14d, r14d
	inc	rax
	je	LBB5_6
## BB#5:                                ## %vector.body.preheader
	mov	rbx, rax
	and	rbx, 1
	je	LBB5_7
LBB5_6:                                 ## %vector.body.unr
	movups	xmm0, xmmword ptr [r10]
	movups	xmm1, xmmword ptr [r10 + 16]
	movups	xmmword ptr [rsi], xmm0
	movups	xmmword ptr [rsi + 16], xmm1
	mov	r14d, 4
LBB5_7:                                 ## %vector.body.preheader.split
	cmp	rax, 2
	jb	LBB5_10
## BB#8:                                ## %vector.body.preheader.split.split
	lea	rax, qword ptr [rsi + 8*r14 + 48]
	lea	rbx, qword ptr [r10 + 8*r14 + 48]
	inc	rcx
	and	rcx, -4
	sub	rcx, r14
	.align	4, 0x90
LBB5_9:                                 ## %vector.body
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm0, xmmword ptr [rbx - 48]
	movups	xmm1, xmmword ptr [rbx - 32]
	movups	xmmword ptr [rax - 48], xmm0
	movups	xmmword ptr [rax - 32], xmm1
	movups	xmm0, xmmword ptr [rbx - 16]
	movups	xmm1, xmmword ptr [rbx]
	movups	xmmword ptr [rax - 16], xmm0
	movups	xmmword ptr [rax], xmm1
	add	rax, 64
	add	rbx, 64
	add	rcx, -8
	jne	LBB5_9
LBB5_10:
	mov	rax, r11
LBB5_11:                                ## %middle.block
	cmp	r9, rax
	je	LBB5_21
## BB#12:                               ## %scalar.ph.preheader
	lea	ebx, dword ptr [rax + 1]
	cmp	r8d, ebx
	mov	r9d, ebx
	cmovge	r9d, r8d
	inc	r9d
	mov	ecx, r9d
	sub	ecx, ebx
	and	ecx, 3
	sub	r9d, ebx
	je	LBB5_14
## BB#13:                               ## %scalar.ph.preheader
	test	ecx, ecx
	je	LBB5_19
LBB5_14:                                ## %unr.cmp24
	cmp	ecx, 1
	je	LBB5_18
## BB#15:                               ## %unr.cmp
	cmp	ecx, 2
	je	LBB5_17
## BB#16:                               ## %scalar.ph.unr
	mov	rcx, qword ptr [r10 + 8*rax]
	mov	qword ptr [rsi + 8*rax], rcx
	inc	rax
LBB5_17:                                ## %scalar.ph.unr19
	mov	rcx, qword ptr [r10 + 8*rax]
	mov	qword ptr [rsi + 8*rax], rcx
	inc	rax
LBB5_18:                                ## %scalar.ph.unr21
	mov	rcx, qword ptr [r10 + 8*rax]
	mov	qword ptr [rsi + 8*rax], rcx
	inc	rax
LBB5_19:                                ## %scalar.ph.preheader.split
	cmp	r9d, 4
	jb	LBB5_21
	.align	4, 0x90
LBB5_20:                                ## %scalar.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rcx, qword ptr [r10 + 8*rax]
	mov	qword ptr [rsi + 8*rax], rcx
	mov	rcx, qword ptr [r10 + 8*rax + 8]
	mov	qword ptr [rsi + 8*rax + 8], rcx
	mov	rcx, qword ptr [r10 + 8*rax + 16]
	mov	qword ptr [rsi + 8*rax + 16], rcx
	mov	rcx, qword ptr [r10 + 8*rax + 24]
	mov	qword ptr [rsi + 8*rax + 24], rcx
	add	rax, 4
	cmp	eax, r8d
	jl	LBB5_20
LBB5_21:                                ## %._crit_edge7
	movsxd	r10, dword ptr [rdx + 8]
	test	r10, r10
                                        ## implicit-def: RAX
	jle	LBB5_29
## BB#22:                               ## %.lr.ph
	mov	r8, qword ptr [rdx]
	mov	r9, qword ptr [rdi]
	lea	r11d, dword ptr [r10 - 2]
	lea	r14, qword ptr [r9 + 8*r10 - 16]
	movabs	rax, 9223372036854775807
	.align	4, 0x90
LBB5_23:                                ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB5_25 Depth 2
	mov	rdx, qword ptr [r8 + 8*r10 - 8]
	mov	rsi, rdx
	and	rsi, rax
	mov	rdi, qword ptr [r9 + 8*r10 - 8]
	add	rsi, rdi
	mov	rbx, rsi
	shr	rbx, 63
	add	rdx, rdi
	and	rdx, rax
	mov	qword ptr [r9 + 8*r10 - 8], rdx
	lea	r10, qword ptr [r10 - 1]
	test	r10d, r10d
	jle	LBB5_27
## BB#24:                               ##   in Loop: Header=BB5_23 Depth=1
	test	rsi, rsi
	mov	rdi, r14
	mov	esi, r11d
	jns	LBB5_27
	.align	4, 0x90
LBB5_25:                                ## %.lr.ph.i
                                        ##   Parent Loop BB5_23 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	mov	rcx, qword ptr [rdi]
	inc	rcx
	mov	rbx, rcx
	shr	rbx, 63
	mov	rdx, rcx
	and	rdx, rax
	mov	qword ptr [rdi], rdx
	test	esi, esi
	jle	LBB5_27
## BB#26:                               ## %.lr.ph.i
                                        ##   in Loop: Header=BB5_25 Depth=2
	dec	esi
	add	rdi, -8
	test	rcx, rcx
	js	LBB5_25
LBB5_27:                                ## %biAddWord.exit
                                        ##   in Loop: Header=BB5_23 Depth=1
	dec	r11d
	add	r14, -8
	test	r10d, r10d
	jg	LBB5_23
## BB#28:                               ## %._crit_edge
	movzx	eax, bl
LBB5_29:
	pop	rbx
	pop	r14
	pop	rbp
	ret
	.cfi_endproc

	.section	__TEXT,__literal16,16byte_literals
	.align	4
LCPI6_0:
	.quad	9223372036854775807     ## 0x7fffffffffffffff
	.quad	9223372036854775807     ## 0x7fffffffffffffff
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_bisub
	.align	4, 0x90
_bisub:                                 ## @bisub
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp30:
	.cfi_def_cfa_offset 16
Ltmp31:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp32:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	sub	rsp, 40
Ltmp33:
	.cfi_offset rbx, -56
Ltmp34:
	.cfi_offset r12, -48
Ltmp35:
	.cfi_offset r13, -40
Ltmp36:
	.cfi_offset r14, -32
Ltmp37:
	.cfi_offset r15, -24
	mov	qword ptr [rbp - 64], rsi ## 8-byte Spill
	mov	r10, rdi
	mov	r11, qword ptr [rip + ___stack_chk_guard@GOTPCREL]
	mov	r11, qword ptr [r11]
	mov	qword ptr [rbp - 48], r11
	movabs	rax, 8589934588
	mov	qword ptr [rbp - 56], rax ## 8-byte Spill
	mov	r12d, dword ptr [r10 + 8]
	test	r12d, r12d
	jle	LBB6_21
## BB#1:                                ## %.lr.ph12
	mov	rax, qword ptr [rdx]
	mov	rcx, qword ptr [r10]
	lea	edi, dword ptr [r12 - 1]
	lea	r8, qword ptr [rdi + 1]
	xor	esi, esi
	mov	r9, r8
	movabs	rbx, 8589934588
	and	r9, rbx
	je	LBB6_11
## BB#2:                                ## %vector.memcheck
	lea	rbx, qword ptr [rax + 8*rdi]
	xor	esi, esi
	cmp	rcx, rbx
	ja	LBB6_4
## BB#3:                                ## %vector.memcheck
	lea	rbx, qword ptr [rcx + 8*rdi]
	cmp	rax, rbx
	jbe	LBB6_11
LBB6_4:                                 ## %vector.body.preheader
	mov	r14, r10
	lea	rbx, qword ptr [rdi + 1]
	and	rbx, -4
	add	rbx, -4
	shr	rbx, 2
	xor	r10d, r10d
	inc	rbx
	je	LBB6_6
## BB#5:                                ## %vector.body.preheader
	mov	rsi, rbx
	and	rsi, 1
	je	LBB6_7
LBB6_6:                                 ## %vector.body.unr
	movups	xmm0, xmmword ptr [rax]
	movups	xmm1, xmmword ptr [rax + 16]
	movups	xmmword ptr [rcx], xmm0
	movups	xmmword ptr [rcx + 16], xmm1
	mov	r10d, 4
LBB6_7:                                 ## %vector.body.preheader.split
	cmp	rbx, 2
	jb	LBB6_10
## BB#8:                                ## %vector.body.preheader.split.split
	lea	rsi, qword ptr [rcx + 8*r10 + 48]
	lea	rbx, qword ptr [rax + 8*r10 + 48]
	inc	rdi
	and	rdi, -4
	sub	rdi, r10
	.align	4, 0x90
LBB6_9:                                 ## %vector.body
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm0, xmmword ptr [rbx - 48]
	movups	xmm1, xmmword ptr [rbx - 32]
	movups	xmmword ptr [rsi - 48], xmm0
	movups	xmmword ptr [rsi - 32], xmm1
	movups	xmm0, xmmword ptr [rbx - 16]
	movups	xmm1, xmmword ptr [rbx]
	movups	xmmword ptr [rsi - 16], xmm0
	movups	xmmword ptr [rsi], xmm1
	add	rsi, 64
	add	rbx, 64
	add	rdi, -8
	jne	LBB6_9
LBB6_10:
	mov	rsi, r9
	mov	r10, r14
LBB6_11:                                ## %middle.block
	cmp	r8, rsi
	je	LBB6_21
## BB#12:                               ## %scalar.ph.preheader
	lea	ebx, dword ptr [rsi + 1]
	cmp	r12d, ebx
	mov	r8d, ebx
	cmovge	r8d, r12d
	inc	r8d
	mov	edi, r8d
	sub	edi, ebx
	and	edi, 3
	sub	r8d, ebx
	je	LBB6_14
## BB#13:                               ## %scalar.ph.preheader
	test	edi, edi
	je	LBB6_19
LBB6_14:                                ## %unr.cmp128
	cmp	edi, 1
	je	LBB6_18
## BB#15:                               ## %unr.cmp123
	cmp	edi, 2
	je	LBB6_17
## BB#16:                               ## %scalar.ph.unr
	mov	rdi, qword ptr [rax + 8*rsi]
	mov	qword ptr [rcx + 8*rsi], rdi
	inc	rsi
LBB6_17:                                ## %scalar.ph.unr121
	mov	rdi, qword ptr [rax + 8*rsi]
	mov	qword ptr [rcx + 8*rsi], rdi
	inc	rsi
LBB6_18:                                ## %scalar.ph.unr125
	mov	rdi, qword ptr [rax + 8*rsi]
	mov	qword ptr [rcx + 8*rsi], rdi
	inc	rsi
LBB6_19:                                ## %scalar.ph.preheader.split
	cmp	r8d, 4
	jb	LBB6_21
	.align	4, 0x90
LBB6_20:                                ## %scalar.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rdi, qword ptr [rax + 8*rsi]
	mov	qword ptr [rcx + 8*rsi], rdi
	mov	rdi, qword ptr [rax + 8*rsi + 8]
	mov	qword ptr [rcx + 8*rsi + 8], rdi
	mov	rdi, qword ptr [rax + 8*rsi + 16]
	mov	qword ptr [rcx + 8*rsi + 16], rdi
	mov	rdi, qword ptr [rax + 8*rsi + 24]
	mov	qword ptr [rcx + 8*rsi + 24], rdi
	add	rsi, 4
	cmp	esi, r12d
	jl	LBB6_20
LBB6_21:                                ## %._crit_edge13
	movabs	r15, 9223372036854775807
	movsxd	r14, dword ptr [rdx + 8]
	mov	r13, rsp
	lea	rax, qword ptr [8*r14 + 15]
	and	rax, -16
	sub	r13, rax
	mov	rsp, r13
	test	r14, r14
	jle	LBB6_37
## BB#22:                               ## %.lr.ph.i.preheader
	mov	qword ptr [rbp - 72], r10 ## 8-byte Spill
	mov	rsi, qword ptr [rdx]
	lea	ebx, dword ptr [r14 - 1]
	cmp	r14d, 1
	lea	rax, qword ptr [8*rbx + 8]
	mov	edx, 8
	cmovg	rdx, rax
	mov	rdi, r13
	call	_memcpy
	inc	rbx
	xor	ecx, ecx
	mov	rax, rbx
	movabs	rdx, 8589934588
	and	rax, rdx
	je	LBB6_30
## BB#23:                               ## %vector.body27.preheader
	lea	ecx, dword ptr [r14 - 1]
	inc	rcx
	and	rcx, -4
	add	rcx, -4
	shr	rcx, 2
	xor	esi, esi
	inc	rcx
	je	LBB6_25
## BB#24:                               ## %vector.body27.preheader
	mov	rdx, rcx
	and	rdx, 1
	je	LBB6_26
LBB6_25:                                ## %vector.body27.unr
	movups	xmm0, xmmword ptr [r13]
	movups	xmm1, xmmword ptr [r13 + 16]
	movaps	xmm2, xmmword ptr [rip + LCPI6_0]
	andnps	xmm0, xmm2
	andnps	xmm1, xmm2
	movups	xmmword ptr [r13], xmm0
	movups	xmmword ptr [r13 + 16], xmm1
	mov	esi, 4
LBB6_26:                                ## %vector.body27.preheader.split
	cmp	rcx, 2
	jb	LBB6_29
## BB#27:                               ## %vector.body27.preheader.split.split
	lea	rcx, qword ptr [r13 + 8*rsi + 48]
	lea	edx, dword ptr [r14 - 1]
	inc	rdx
	and	rdx, -4
	sub	rdx, rsi
	movaps	xmm0, xmmword ptr [rip + LCPI6_0]
	.align	4, 0x90
LBB6_28:                                ## %vector.body27
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm1, xmmword ptr [rcx - 48]
	movups	xmm2, xmmword ptr [rcx - 32]
	andnps	xmm1, xmm0
	andnps	xmm2, xmm0
	movups	xmmword ptr [rcx - 48], xmm1
	movups	xmmword ptr [rcx - 32], xmm2
	movups	xmm1, xmmword ptr [rcx - 16]
	movups	xmm2, xmmword ptr [rcx]
	andnps	xmm1, xmm0
	andnps	xmm2, xmm0
	movups	xmmword ptr [rcx - 16], xmm1
	movups	xmmword ptr [rcx], xmm2
	add	rcx, 64
	add	rdx, -8
	jne	LBB6_28
LBB6_29:
	mov	rcx, rax
LBB6_30:                                ## %middle.block28
	cmp	rbx, rcx
	mov	r11, qword ptr [rip + ___stack_chk_guard@GOTPCREL]
	mov	r11, qword ptr [r11]
	mov	r10, qword ptr [rbp - 72] ## 8-byte Reload
	je	LBB6_37
## BB#31:                               ## %.lr.ph.i.preheader92
	lea	eax, dword ptr [r14 + 1]
	lea	esi, dword ptr [rcx + 1]
	mov	edx, eax
	sub	edx, esi
	sub	eax, esi
	je	LBB6_33
## BB#32:                               ## %.lr.ph.i.preheader92
	and	edx, 1
	je	LBB6_34
LBB6_33:                                ## %.lr.ph.i.unr
	mov	rdx, qword ptr [r13 + 8*rcx]
	not	rdx
	and	rdx, r15
	mov	qword ptr [r13 + 8*rcx], rdx
	inc	rcx
LBB6_34:                                ## %.lr.ph.i.preheader92.split
	cmp	eax, 2
	jb	LBB6_37
## BB#35:                               ## %.lr.ph.i.preheader92.split.split
	mov	eax, r14d
	sub	eax, ecx
	lea	rdx, qword ptr [r13 + 8*rcx + 8]
	.align	4, 0x90
LBB6_36:                                ## %.lr.ph.i
                                        ## =>This Inner Loop Header: Depth=1
	mov	rsi, qword ptr [rdx - 8]
	not	rsi
	and	rsi, r15
	mov	qword ptr [rdx - 8], rsi
	mov	rsi, qword ptr [rdx]
	not	rsi
	and	rsi, r15
	mov	qword ptr [rdx], rsi
	add	rcx, 2
	add	rdx, 16
	add	eax, -2
	jne	LBB6_36
LBB6_37:                                ## %._crit_edge.i
	lea	eax, dword ptr [r14 - 1]
	movsxd	rcx, eax
	mov	rdx, qword ptr [r13 + 8*rcx]
	inc	rdx
	mov	qword ptr [r13 + 8*rcx], rdx
	mov	rax, rdx
	shr	rax, 63
	je	LBB6_44
## BB#38:
	and	rdx, r15
	mov	qword ptr [r13 + 8*rcx], rdx
	mov	ecx, r14d
	add	ecx, -2
	js	LBB6_44
## BB#39:                               ## %.lr.ph.i.i
	lea	ecx, dword ptr [r14 - 1]
	lea	edx, dword ptr [r14 - 2]
	movsxd	rdx, edx
	lea	rdx, qword ptr [r13 + 8*rdx]
	.align	4, 0x90
LBB6_40:                                ## =>This Inner Loop Header: Depth=1
	test	rax, rax
	mov	rsi, qword ptr [rdx]
	je	LBB6_42
## BB#41:                               ##   in Loop: Header=BB6_40 Depth=1
	inc	rsi
	mov	qword ptr [rdx], rsi
LBB6_42:                                ## %._crit_edge.i.i
                                        ##   in Loop: Header=BB6_40 Depth=1
	mov	rax, rsi
	shr	rax, 63
	je	LBB6_44
## BB#43:                               ##   in Loop: Header=BB6_40 Depth=1
	and	rsi, r15
	mov	qword ptr [rdx], rsi
	dec	ecx
	add	rdx, -8
	test	ecx, ecx
	jg	LBB6_40
LBB6_44:                                ## %biTwoComplement.exit
	test	r12d, r12d
	jle	LBB6_66
## BB#45:                               ## %.lr.ph6.i
	mov	rax, qword ptr [rbp - 64] ## 8-byte Reload
	mov	r8, qword ptr [rax]
	mov	rcx, qword ptr [r10]
	lea	edi, dword ptr [r12 - 1]
	lea	rsi, qword ptr [rdi + 1]
	xor	edx, edx
	and	qword ptr [rbp - 56], rsi ## 8-byte Folded Spill
	je	LBB6_55
## BB#46:                               ## %vector.memcheck72
	lea	rax, qword ptr [r8 + 8*rdi]
	xor	edx, edx
	cmp	rcx, rax
	ja	LBB6_48
## BB#47:                               ## %vector.memcheck72
	lea	rax, qword ptr [rcx + 8*rdi]
	cmp	r8, rax
	jbe	LBB6_55
LBB6_48:                                ## %vector.body54.preheader
	lea	rbx, qword ptr [rdi + 1]
	and	rbx, -4
	add	rbx, -4
	shr	rbx, 2
	xor	eax, eax
	inc	rbx
	je	LBB6_50
## BB#49:                               ## %vector.body54.preheader
	mov	rdx, rbx
	and	rdx, 1
	je	LBB6_51
LBB6_50:                                ## %vector.body54.unr
	movups	xmm0, xmmword ptr [r8]
	movups	xmm1, xmmword ptr [r8 + 16]
	movups	xmmword ptr [rcx], xmm0
	movups	xmmword ptr [rcx + 16], xmm1
	mov	eax, 4
LBB6_51:                                ## %vector.body54.preheader.split
	cmp	rbx, 2
	jb	LBB6_54
## BB#52:                               ## %vector.body54.preheader.split.split
	lea	rdx, qword ptr [rcx + 8*rax + 48]
	lea	rbx, qword ptr [r8 + 8*rax + 48]
	inc	rdi
	and	rdi, -4
	sub	rdi, rax
	.align	4, 0x90
LBB6_53:                                ## %vector.body54
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm0, xmmword ptr [rbx - 48]
	movups	xmm1, xmmword ptr [rbx - 32]
	movups	xmmword ptr [rdx - 48], xmm0
	movups	xmmword ptr [rdx - 32], xmm1
	movups	xmm0, xmmword ptr [rbx - 16]
	movups	xmm1, xmmword ptr [rbx]
	movups	xmmword ptr [rdx - 16], xmm0
	movups	xmmword ptr [rdx], xmm1
	add	rdx, 64
	add	rbx, 64
	add	rdi, -8
	jne	LBB6_53
LBB6_54:
	mov	rdx, qword ptr [rbp - 56] ## 8-byte Reload
LBB6_55:                                ## %middle.block55
	cmp	rsi, rdx
	je	LBB6_66
## BB#56:                               ## %scalar.ph56.preheader
	lea	esi, dword ptr [r12 + 1]
	lea	edi, dword ptr [rdx + 1]
	mov	eax, esi
	sub	eax, edi
	and	eax, 3
	sub	esi, edi
	je	LBB6_58
## BB#57:                               ## %scalar.ph56.preheader
	test	eax, eax
	je	LBB6_63
LBB6_58:                                ## %unr.cmp102
	cmp	eax, 1
	je	LBB6_62
## BB#59:                               ## %unr.cmp
	cmp	eax, 2
	je	LBB6_61
## BB#60:                               ## %scalar.ph56.unr
	mov	rax, qword ptr [r8 + 8*rdx]
	mov	qword ptr [rcx + 8*rdx], rax
	inc	rdx
LBB6_61:                                ## %scalar.ph56.unr93
	mov	rax, qword ptr [r8 + 8*rdx]
	mov	qword ptr [rcx + 8*rdx], rax
	inc	rdx
LBB6_62:                                ## %scalar.ph56.unr97
	mov	rax, qword ptr [r8 + 8*rdx]
	mov	qword ptr [rcx + 8*rdx], rax
	inc	rdx
LBB6_63:                                ## %scalar.ph56.preheader.split
	cmp	esi, 4
	jb	LBB6_66
## BB#64:                               ## %scalar.ph56.preheader.split.split
	sub	r12d, edx
	lea	rcx, qword ptr [rcx + 8*rdx + 24]
	lea	rax, qword ptr [r8 + 8*rdx + 24]
	.align	4, 0x90
LBB6_65:                                ## %scalar.ph56
                                        ## =>This Inner Loop Header: Depth=1
	mov	rsi, qword ptr [rax - 24]
	mov	qword ptr [rcx - 24], rsi
	mov	rsi, qword ptr [rax - 16]
	mov	qword ptr [rcx - 16], rsi
	mov	rsi, qword ptr [rax - 8]
	mov	qword ptr [rcx - 8], rsi
	mov	rsi, qword ptr [rax]
	mov	qword ptr [rcx], rsi
	add	rdx, 4
	add	rcx, 32
	add	rax, 32
	add	r12d, -4
	jne	LBB6_65
LBB6_66:                                ## %._crit_edge7.i
	test	r14d, r14d
	jle	LBB6_73
## BB#67:                               ## %.lr.ph.i2
	mov	r8, qword ptr [r10]
	lea	ecx, dword ptr [r14 - 2]
	lea	rdx, qword ptr [r8 + 8*r14 - 16]
	.align	4, 0x90
LBB6_68:                                ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB6_70 Depth 2
	mov	rsi, qword ptr [r13 + 8*r14 - 8]
	mov	rdi, qword ptr [r8 + 8*r14 - 8]
	lea	rbx, qword ptr [rdi + rsi]
	and	rbx, r15
	mov	qword ptr [r8 + 8*r14 - 8], rbx
	lea	r14, qword ptr [r14 - 1]
	test	r14d, r14d
	jle	LBB6_72
## BB#69:                               ##   in Loop: Header=BB6_68 Depth=1
	and	rsi, r15
	add	rdi, rsi
	mov	rsi, rdx
	mov	edi, ecx
	jns	LBB6_72
	.align	4, 0x90
LBB6_70:                                ## %.lr.ph.i.i7
                                        ##   Parent Loop BB6_68 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	mov	rax, qword ptr [rsi]
	inc	rax
	mov	rbx, rax
	and	rbx, r15
	mov	qword ptr [rsi], rbx
	test	edi, edi
	jle	LBB6_72
## BB#71:                               ## %.lr.ph.i.i7
                                        ##   in Loop: Header=BB6_70 Depth=2
	dec	edi
	add	rsi, -8
	test	rax, rax
	js	LBB6_70
LBB6_72:                                ## %biAddWord.exit.i
                                        ##   in Loop: Header=BB6_68 Depth=1
	dec	ecx
	add	rdx, -8
	test	r14d, r14d
	jg	LBB6_68
LBB6_73:                                ## %biadd.exit
	cmp	r11, qword ptr [rbp - 48]
	jne	LBB6_75
## BB#74:                               ## %biadd.exit
	lea	rsp, qword ptr [rbp - 40]
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	ret
LBB6_75:                                ## %biadd.exit
	call	___stack_chk_fail
	.cfi_endproc

	.section	__TEXT,__literal16,16byte_literals
	.align	4
LCPI7_0:
	.quad	9223372036854775807     ## 0x7fffffffffffffff
	.quad	9223372036854775807     ## 0x7fffffffffffffff
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_biTwoComplement
	.align	4, 0x90
_biTwoComplement:                       ## @biTwoComplement
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp38:
	.cfi_def_cfa_offset 16
Ltmp39:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp40:
	.cfi_def_cfa_register rbp
	push	rbx
Ltmp41:
	.cfi_offset rbx, -24
	movabs	r11, 9223372036854775807
	mov	ebx, dword ptr [rdi + 8]
	mov	rcx, qword ptr [rdi]
	test	ebx, ebx
	jle	LBB7_15
## BB#1:                                ## %overflow.checked
	lea	edi, dword ptr [rbx - 1]
	lea	r8, qword ptr [rdi + 1]
	xor	esi, esi
	movabs	r9, 8589934588
	and	r9, r8
	je	LBB7_9
## BB#2:                                ## %vector.body.preheader
	lea	rsi, qword ptr [rdi + 1]
	and	rsi, -4
	add	rsi, -4
	shr	rsi, 2
	xor	r10d, r10d
	inc	rsi
	je	LBB7_4
## BB#3:                                ## %vector.body.preheader
	mov	rax, rsi
	and	rax, 1
	je	LBB7_5
LBB7_4:                                 ## %vector.body.unr
	movups	xmm0, xmmword ptr [rcx]
	movups	xmm1, xmmword ptr [rcx + 16]
	movaps	xmm2, xmmword ptr [rip + LCPI7_0]
	andnps	xmm0, xmm2
	andnps	xmm1, xmm2
	movups	xmmword ptr [rcx], xmm0
	movups	xmmword ptr [rcx + 16], xmm1
	mov	r10d, 4
LBB7_5:                                 ## %vector.body.preheader.split
	cmp	rsi, 2
	jb	LBB7_8
## BB#6:                                ## %vector.body.preheader.split.split
	lea	rsi, qword ptr [rcx + 8*r10 + 48]
	inc	rdi
	and	rdi, -4
	sub	rdi, r10
	movaps	xmm0, xmmword ptr [rip + LCPI7_0]
	.align	4, 0x90
LBB7_7:                                 ## %vector.body
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm1, xmmword ptr [rsi - 48]
	movups	xmm2, xmmword ptr [rsi - 32]
	andnps	xmm1, xmm0
	andnps	xmm2, xmm0
	movups	xmmword ptr [rsi - 48], xmm1
	movups	xmmword ptr [rsi - 32], xmm2
	movups	xmm1, xmmword ptr [rsi - 16]
	movups	xmm2, xmmword ptr [rsi]
	andnps	xmm1, xmm0
	andnps	xmm2, xmm0
	movups	xmmword ptr [rsi - 16], xmm1
	movups	xmmword ptr [rsi], xmm2
	add	rsi, 64
	add	rdi, -8
	jne	LBB7_7
LBB7_8:
	mov	rsi, r9
LBB7_9:                                 ## %middle.block
	cmp	r8, rsi
	je	LBB7_15
## BB#10:                               ## %.lr.ph.preheader
	lea	edx, dword ptr [rsi + 1]
	cmp	ebx, edx
	mov	edi, edx
	cmovge	edi, ebx
	inc	edi
	mov	eax, edi
	sub	eax, edx
	sub	edi, edx
	je	LBB7_12
## BB#11:                               ## %.lr.ph.preheader
	and	eax, 1
	je	LBB7_13
LBB7_12:                                ## %.lr.ph.unr
	mov	rax, qword ptr [rcx + 8*rsi]
	not	rax
	and	rax, r11
	mov	qword ptr [rcx + 8*rsi], rax
	inc	rsi
LBB7_13:                                ## %.lr.ph.preheader.split
	cmp	edi, 2
	jb	LBB7_15
	.align	4, 0x90
LBB7_14:                                ## %.lr.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rax, qword ptr [rcx + 8*rsi]
	not	rax
	and	rax, r11
	mov	qword ptr [rcx + 8*rsi], rax
	mov	rax, qword ptr [rcx + 8*rsi + 8]
	not	rax
	and	rax, r11
	mov	qword ptr [rcx + 8*rsi + 8], rax
	add	rsi, 2
	cmp	esi, ebx
	jl	LBB7_14
LBB7_15:                                ## %._crit_edge
	lea	esi, dword ptr [rbx - 1]
	movsxd	rax, esi
	mov	rdx, qword ptr [rcx + 8*rax]
	inc	rdx
	mov	qword ptr [rcx + 8*rax], rdx
	mov	rdi, rdx
	shr	rdi, 63
	je	LBB7_22
## BB#16:
	and	rdx, r11
	mov	qword ptr [rcx + 8*rax], rdx
	add	ebx, -2
	js	LBB7_22
## BB#17:                               ## %.lr.ph.i
	movsxd	rax, ebx
	lea	rcx, qword ptr [rcx + 8*rax]
	.align	4, 0x90
LBB7_18:                                ## =>This Inner Loop Header: Depth=1
	test	rdi, rdi
	mov	rdx, qword ptr [rcx]
	je	LBB7_20
## BB#19:                               ##   in Loop: Header=BB7_18 Depth=1
	inc	rdx
	mov	qword ptr [rcx], rdx
LBB7_20:                                ## %._crit_edge.i
                                        ##   in Loop: Header=BB7_18 Depth=1
	mov	rdi, rdx
	shr	rdi, 63
	je	LBB7_22
## BB#21:                               ##   in Loop: Header=BB7_18 Depth=1
	and	rdx, r11
	mov	qword ptr [rcx], rdx
	dec	esi
	add	rcx, -8
	test	esi, esi
	jg	LBB7_18
LBB7_22:                                ## %biinc.exit
	pop	rbx
	pop	rbp
	ret
	.cfi_endproc

	.globl	_bishl
	.align	4, 0x90
_bishl:                                 ## @bishl
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp42:
	.cfi_def_cfa_offset 16
Ltmp43:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp44:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r12
	push	rbx
Ltmp45:
	.cfi_offset rbx, -48
Ltmp46:
	.cfi_offset r12, -40
Ltmp47:
	.cfi_offset r14, -32
Ltmp48:
	.cfi_offset r15, -24
	mov	r15d, esi
	mov	r14, rdi
	movsxd	rsi, dword ptr [r14 + 8]
	imul	eax, esi, 63
	cmp	eax, r15d
	jle	LBB8_1
## BB#2:
	movsxd	rax, r15d
	imul	rcx, rax, -2113396605
	shr	rcx, 32
	add	ecx, r15d
	mov	eax, ecx
	shr	eax, 31
	sar	ecx, 5
	add	ecx, eax
	mov	r11d, esi
	sub	r11d, ecx
	jle	LBB8_20
## BB#3:                                ## %.lr.ph8
	mov	rdx, qword ptr [r14]
	movsxd	r8, ecx
	lea	eax, dword ptr [rsi - 1]
	sub	eax, ecx
	lea	r9, qword ptr [rax + 1]
	xor	edi, edi
	movabs	r10, 8589934588
	and	r10, r9
	je	LBB8_13
## BB#4:                                ## %vector.memcheck
	lea	rdi, qword ptr [r8 + rax]
	lea	rbx, qword ptr [rdx + 8*rdi]
	xor	edi, edi
	cmp	rdx, rbx
	ja	LBB8_6
## BB#5:                                ## %vector.memcheck
	lea	rbx, qword ptr [rdx + 8*r8]
	lea	rax, qword ptr [rdx + 8*rax]
	cmp	rbx, rax
	jbe	LBB8_13
LBB8_6:                                 ## %vector.body.preheader
	lea	edi, dword ptr [rsi - 1]
	sub	edi, ecx
	inc	rdi
	and	rdi, -4
	add	rdi, -4
	shr	rdi, 2
	xor	r12d, r12d
	inc	rdi
	je	LBB8_8
## BB#7:                                ## %vector.body.preheader
	mov	rax, rdi
	and	rax, 1
	je	LBB8_9
LBB8_8:                                 ## %vector.body.unr
	movups	xmm0, xmmword ptr [rdx + 8*r8]
	movups	xmm1, xmmword ptr [rdx + 8*r8 + 16]
	movups	xmmword ptr [rdx], xmm0
	movups	xmmword ptr [rdx + 16], xmm1
	mov	r12d, 4
LBB8_9:                                 ## %vector.body.preheader.split
	cmp	rdi, 2
	jb	LBB8_12
## BB#10:                               ## %vector.body.preheader.split.split
	lea	rax, qword ptr [r12 + r8]
	lea	rdi, qword ptr [rdx + 8*rax + 48]
	lea	rbx, qword ptr [rdx + 8*r12 + 48]
	lea	eax, dword ptr [rsi - 1]
	sub	eax, ecx
	inc	rax
	and	rax, -4
	sub	rax, r12
	.align	4, 0x90
LBB8_11:                                ## %vector.body
                                        ## =>This Inner Loop Header: Depth=1
	movups	xmm0, xmmword ptr [rdi - 48]
	movups	xmm1, xmmword ptr [rdi - 32]
	movups	xmmword ptr [rbx - 48], xmm0
	movups	xmmword ptr [rbx - 32], xmm1
	movups	xmm0, xmmword ptr [rdi - 16]
	movups	xmm1, xmmword ptr [rdi]
	movups	xmmword ptr [rbx - 16], xmm0
	movups	xmmword ptr [rbx], xmm1
	add	rdi, 64
	add	rbx, 64
	add	rax, -8
	jne	LBB8_11
LBB8_12:
	mov	rdi, r10
LBB8_13:                                ## %middle.block
	cmp	r9, rdi
	je	LBB8_20
## BB#14:                               ## %scalar.ph.preheader
	mov	eax, esi
	sub	eax, ecx
	lea	ebx, dword ptr [rdi + 1]
	cmp	eax, ebx
	cmovl	eax, ebx
	inc	eax
	mov	r9d, eax
	sub	r9d, ebx
	sub	eax, ebx
	je	LBB8_16
## BB#15:                               ## %scalar.ph.preheader
	and	r9d, 1
	je	LBB8_17
LBB8_16:                                ## %scalar.ph.unr
	lea	rbx, qword ptr [rdi + r8]
	mov	rbx, qword ptr [rdx + 8*rbx]
	mov	qword ptr [rdx + 8*rdi], rbx
	inc	rdi
LBB8_17:                                ## %scalar.ph.preheader.split
	cmp	eax, 2
	jb	LBB8_20
## BB#18:                               ## %scalar.ph.preheader.split.split
	lea	rax, qword ptr [rdx + 8*r8 + 8]
	.align	4, 0x90
LBB8_19:                                ## %scalar.ph
                                        ## =>This Inner Loop Header: Depth=1
	mov	rbx, qword ptr [rax + 8*rdi - 8]
	mov	qword ptr [rdx + 8*rdi], rbx
	mov	rbx, qword ptr [rax + 8*rdi]
	mov	qword ptr [rdx + 8*rdi + 8], rbx
	add	rdi, 2
	cmp	edi, r11d
	jl	LBB8_19
LBB8_20:                                ## %.preheader
	imul	eax, ecx, 63
	mov	ebx, r15d
	sub	ebx, eax
	cmp	r15d, 63
	jl	LBB8_22
## BB#21:                               ## %.lr.ph5
	mov	rax, qword ptr [r14]
	movsxd	rcx, r11d
	lea	rdi, qword ptr [rax + 8*rcx]
	lea	eax, dword ptr [r11 + 1]
	cmp	esi, eax
	cmovge	eax, esi
	dec	eax
	sub	eax, r11d
	lea	rsi, qword ptr [8*rax + 8]
	call	___bzero
LBB8_22:
	test	ebx, ebx
	je	LBB8_28
## BB#23:
	mov	r8d, 63
	sub	r8d, r15d
	mov	r11d, r15d
	mov	r10d, 1
	mov	cl, r15b
	shl	r10, cl
	dec	r10
	mov	cl, r8b
	shl	r10, cl
	mov	r15d, dword ptr [r14 + 8]
	test	r15d, r15d
	jle	LBB8_28
## BB#24:                               ## %.lr.ph
	mov	rbx, qword ptr [r14]
	lea	r14d, dword ptr [r15 - 1]
	xor	esi, esi
	movabs	r9, 9223372036854775807
	.align	4, 0x90
LBB8_25:                                ## =>This Inner Loop Header: Depth=1
	mov	rdx, qword ptr [rbx + 8*rsi]
	mov	cl, r11b
	shl	rdx, cl
	and	rdx, r9
	mov	qword ptr [rbx + 8*rsi], rdx
	lea	rdi, qword ptr [rsi + 1]
	cmp	esi, r14d
	jge	LBB8_27
## BB#26:                               ##   in Loop: Header=BB8_25 Depth=1
	mov	rax, qword ptr [rbx + 8*rsi + 8]
	and	rax, r10
	mov	cl, r8b
	shr	rax, cl
	or	rax, rdx
	mov	qword ptr [rbx + 8*rsi], rax
LBB8_27:                                ## %._crit_edge
                                        ##   in Loop: Header=BB8_25 Depth=1
	cmp	edi, r15d
	mov	rsi, rdi
	jl	LBB8_25
	jmp	LBB8_28
LBB8_1:
	mov	rdi, qword ptr [r14]
	shl	rsi, 3
	call	___bzero
LBB8_28:                                ## %.loopexit
	pop	rbx
	pop	r12
	pop	r14
	pop	r15
	pop	rbp
	ret
	.cfi_endproc

	.globl	_bishr
	.align	4, 0x90
_bishr:                                 ## @bishr
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp49:
	.cfi_def_cfa_offset 16
Ltmp50:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp51:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	rbx
	push	rax
Ltmp52:
	.cfi_offset rbx, -40
Ltmp53:
	.cfi_offset r14, -32
Ltmp54:
	.cfi_offset r15, -24
	mov	r15d, esi
	mov	r14, rdi
	movsxd	rsi, dword ptr [r14 + 8]
	imul	eax, esi, 63
	cmp	eax, r15d
	jle	LBB9_1
## BB#3:
	movsxd	rax, r15d
	imul	rax, rax, -2113396605
	shr	rax, 32
	add	eax, r15d
	mov	ecx, eax
	shr	ecx, 31
	sar	eax, 5
	add	eax, ecx
	mov	edx, esi
	sub	edx, eax
	test	edx, edx
	jle	LBB9_10
## BB#4:                                ## %.lr.ph11
	mov	ebx, esi
	sub	ebx, eax
	sub	esi, eax
	mov	rcx, qword ptr [r14]
	movsxd	rdx, edx
	movsxd	rdi, eax
	je	LBB9_6
## BB#5:                                ## %.lr.ph11
	and	ebx, 1
	je	LBB9_7
LBB9_6:
	mov	r8, qword ptr [rcx + 8*rdx - 8]
	lea	rbx, qword ptr [rdx + rdi - 1]
	lea	rdx, qword ptr [rdx - 1]
	mov	qword ptr [rcx + 8*rbx], r8
LBB9_7:                                 ## %.lr.ph11.split
	cmp	esi, 2
	jb	LBB9_10
## BB#8:                                ## %.lr.ph11.split.split
	lea	rsi, qword ptr [rcx + 8*rdi - 8]
	.align	4, 0x90
LBB9_9:                                 ## =>This Inner Loop Header: Depth=1
	mov	rdi, qword ptr [rcx + 8*rdx - 8]
	mov	qword ptr [rsi + 8*rdx], rdi
	mov	rdi, qword ptr [rcx + 8*rdx - 16]
	mov	qword ptr [rsi + 8*rdx - 8], rdi
	lea	rdx, qword ptr [rdx - 2]
	test	edx, edx
	jg	LBB9_9
LBB9_10:                                ## %.preheader
	imul	ecx, eax, 63
	mov	ebx, r15d
	sub	ebx, ecx
	cmp	r15d, 63
	jl	LBB9_12
## BB#11:                               ## %.lr.ph7
	mov	rdi, qword ptr [r14]
	dec	eax
	cmp	r15d, 125
	lea	rax, qword ptr [8*rax + 8]
	mov	esi, 8
	cmovg	rsi, rax
	call	___bzero
LBB9_12:
	test	ebx, ebx
	je	LBB9_2
## BB#13:
	mov	r8d, r15d
	mov	r9d, 1
	mov	cl, r15b
	shl	r9, cl
	movsxd	rax, dword ptr [r14 + 8]
	test	rax, rax
	jle	LBB9_2
## BB#14:                               ## %.lr.ph
	dec	r9
	mov	edx, 63
	sub	edx, r15d
	mov	rcx, qword ptr [r14]
	lea	rdi, qword ptr [rcx + 8*rax - 8]
	lea	ebx, dword ptr [rax - 1]
	jmp	LBB9_15
	.align	4, 0x90
LBB9_16:                                ## %.backedge.thread
                                        ##   in Loop: Header=BB9_15 Depth=1
	mov	rsi, qword ptr [rdi - 8]
	and	rsi, r9
	mov	cl, dl
	shl	rsi, cl
	or	rsi, rax
	mov	qword ptr [rdi], rsi
	lea	rdi, qword ptr [rdi - 8]
	dec	ebx
LBB9_15:                                ## =>This Inner Loop Header: Depth=1
	mov	rax, qword ptr [rdi]
	mov	cl, r8b
	shr	rax, cl
	mov	qword ptr [rdi], rax
	test	ebx, ebx
	jg	LBB9_16
	jmp	LBB9_2
LBB9_1:
	mov	rdi, qword ptr [r14]
	shl	rsi, 3
	call	___bzero
LBB9_2:                                 ## %.loopexit
	add	rsp, 8
	pop	rbx
	pop	r14
	pop	r15
	pop	rbp
	ret
	.cfi_endproc

	.globl	_bishlOne
	.align	4, 0x90
_bishlOne:                              ## @bishlOne
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp55:
	.cfi_def_cfa_offset 16
Ltmp56:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp57:
	.cfi_def_cfa_register rbp
	movabs	r8, 9223372036854775806
	mov	r10d, dword ptr [rdi + 8]
	dec	r10d
	mov	rcx, qword ptr [rdi]
	jle	LBB10_6
## BB#1:                                ## %.lr.ph
	mov	rdi, qword ptr [rcx]
	xor	esi, esi
	test	r10d, r10d
	je	LBB10_3
## BB#2:                                ## %.lr.ph
	mov	edx, r10d
	and	edx, 1
	je	LBB10_4
LBB10_3:
	add	rdi, rdi
	and	rdi, r8
	mov	esi, 1
	mov	r9, qword ptr [rcx + 8]
	mov	rdx, r9
	shr	rdx, 62
	and	rdx, 1
	or	rdx, rdi
	mov	qword ptr [rcx], rdx
	mov	rdi, r9
LBB10_4:                                ## %.lr.ph.split
	cmp	r10d, 2
	jb	LBB10_6
	.align	4, 0x90
LBB10_5:                                ## =>This Inner Loop Header: Depth=1
	add	rdi, rdi
	and	rdi, r8
	mov	rax, qword ptr [rcx + 8*rsi + 8]
	mov	rdx, rax
	shr	rdx, 62
	and	rdx, 1
	or	rdx, rdi
	mov	qword ptr [rcx + 8*rsi], rdx
	add	rax, rax
	and	rax, r8
	mov	rdi, qword ptr [rcx + 8*rsi + 16]
	mov	rdx, rdi
	shr	rdx, 62
	and	rdx, 1
	or	rdx, rax
	mov	qword ptr [rcx + 8*rsi + 8], rdx
	lea	rsi, qword ptr [rsi + 2]
	cmp	esi, r10d
	jl	LBB10_5
LBB10_6:                                ## %._crit_edge
	movsxd	rax, r10d
	mov	rdx, qword ptr [rcx + 8*rax]
	add	rdx, rdx
	and	rdx, r8
	mov	qword ptr [rcx + 8*rax], rdx
	pop	rbp
	ret
	.cfi_endproc

	.globl	_bishrOne
	.align	4, 0x90
_bishrOne:                              ## @bishrOne
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp58:
	.cfi_def_cfa_offset 16
Ltmp59:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp60:
	.cfi_def_cfa_register rbp
	mov	esi, dword ptr [rdi + 8]
	lea	ecx, dword ptr [rsi - 1]
	mov	r8, qword ptr [rdi]
	test	ecx, ecx
	jle	LBB11_3
## BB#1:                                ## %.lr.ph
	lea	edx, dword ptr [rsi - 2]
	movsxd	rdx, edx
	lea	rdx, qword ptr [r8 + 8*rdx]
	dec	esi
	movsxd	rsi, esi
	lea	rsi, qword ptr [r8 + 8*rsi]
	.align	4, 0x90
LBB11_2:                                ## =>This Inner Loop Header: Depth=1
	mov	rdi, qword ptr [rsi]
	shr	rdi
	mov	qword ptr [rsi], rdi
	mov	rax, qword ptr [rdx]
	and	rax, 1
	shl	rax, 62
	or	rax, rdi
	mov	qword ptr [rsi], rax
	add	rdx, -8
	add	rsi, -8
	dec	ecx
	jg	LBB11_2
LBB11_3:                                ## %._crit_edge
	shr	qword ptr [r8]
	pop	rbp
	ret
	.cfi_endproc

	.globl	_biinc
	.align	4, 0x90
_biinc:                                 ## @biinc
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp61:
	.cfi_def_cfa_offset 16
Ltmp62:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp63:
	.cfi_def_cfa_register rbp
	mov	r9d, dword ptr [rdi + 8]
	lea	eax, dword ptr [r9 - 1]
	movsxd	rcx, eax
	mov	rdi, qword ptr [rdi]
	mov	rsi, qword ptr [rdi + 8*rcx]
	inc	rsi
	mov	qword ptr [rdi + 8*rcx], rsi
	mov	rdx, rsi
	shr	rdx, 63
	je	LBB12_7
## BB#1:
	movabs	r8, 9223372036854775807
	and	rsi, r8
	mov	qword ptr [rdi + 8*rcx], rsi
	add	r9d, -2
	js	LBB12_7
## BB#2:                                ## %.lr.ph
	movsxd	rcx, r9d
	lea	rsi, qword ptr [rdi + 8*rcx]
	.align	4, 0x90
LBB12_3:                                ## =>This Inner Loop Header: Depth=1
	test	rdx, rdx
	je	LBB12_5
## BB#4:                                ##   in Loop: Header=BB12_3 Depth=1
	inc	qword ptr [rsi]
LBB12_5:                                ## %._crit_edge
                                        ##   in Loop: Header=BB12_3 Depth=1
	mov	rcx, qword ptr [rsi]
	mov	rdx, rcx
	shr	rdx, 63
	je	LBB12_7
## BB#6:                                ##   in Loop: Header=BB12_3 Depth=1
	and	rcx, r8
	mov	qword ptr [rsi], rcx
	add	rsi, -8
	dec	eax
	test	eax, eax
	jg	LBB12_3
LBB12_7:                                ## %.loopexit
	pop	rbp
	ret
	.cfi_endproc

	.section	__TEXT,__literal8,8byte_literals
	.align	3
LCPI13_0:
	.quad	4634063279075885056     ## double 63
LCPI13_1:
	.quad	4580160821035794432     ## double 0.015625
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_biPrint
	.align	4, 0x90
_biPrint:                               ## @biPrint
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp64:
	.cfi_def_cfa_offset 16
Ltmp65:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp66:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	push	rax
Ltmp67:
	.cfi_offset rbx, -56
Ltmp68:
	.cfi_offset r12, -48
Ltmp69:
	.cfi_offset r13, -40
Ltmp70:
	.cfi_offset r14, -32
Ltmp71:
	.cfi_offset r15, -24
	mov	r14, rdi
	cvtsi2sd	xmm0, dword ptr [r14 + 8]
	mulsd	xmm0, qword ptr [rip + LCPI13_0]
	mulsd	xmm0, qword ptr [rip + LCPI13_1]
	call	_ceil
	cvttsd2si	r15d, xmm0
	test	r15d, r15d
	jle	LBB13_8
## BB#1:                                ## %.preheader.lr.ph
	mov	r12d, r15d
	shl	r12d, 6
	add	r12d, -64
	xor	r13d, r13d
	.align	4, 0x90
LBB13_2:                                ## %.preheader
                                        ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB13_3 Depth 2
	mov	eax, r12d
	xor	ecx, ecx
	xor	esi, esi
	.align	4, 0x90
LBB13_3:                                ##   Parent Loop BB13_2 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	mov	r8, rsi
	test	eax, eax
	mov	esi, 0
	js	LBB13_6
## BB#4:                                ##   in Loop: Header=BB13_3 Depth=2
	mov	edi, dword ptr [r14 + 8]
	imul	esi, edi, 63
	cmp	esi, eax
	mov	esi, 0
	jle	LBB13_6
## BB#5:                                ##   in Loop: Header=BB13_3 Depth=2
	movsxd	rsi, eax
	imul	rbx, rsi, -2113396605
	shr	rbx, 32
	add	ebx, eax
	mov	edx, ebx
	shr	edx, 31
	shr	ebx, 5
	add	ebx, edx
	imul	edx, ebx, 63
	mov	ebx, eax
	sub	ebx, edx
	imul	rdx, rsi, 2113396605
	shr	rdx, 32
	sub	edx, eax
	mov	esi, edx
	shr	esi, 31
	sar	edx, 5
	add	edx, esi
	lea	edx, dword ptr [rdi + rdx - 1]
	movsxd	rdx, edx
	mov	rsi, qword ptr [r14]
	mov	rdx, qword ptr [rsi + 8*rdx]
	bt	rdx, rbx
	sbb	rsi, rsi
	and	rsi, 1
LBB13_6:                                ## %biNthBit.exit
                                        ##   in Loop: Header=BB13_3 Depth=2
	shl	rsi, cl
	or	rsi, r8
	inc	rcx
	inc	eax
	cmp	rcx, 64
	jne	LBB13_3
## BB#7:                                ##   in Loop: Header=BB13_2 Depth=1
	xor	eax, eax
	lea	rdi, qword ptr [rip + L_.str]
	call	_printf
	inc	r13d
	add	r12d, -64
	cmp	r13d, r15d
	jl	LBB13_2
LBB13_8:                                ## %._crit_edge
	mov	edi, 10
	add	rsp, 8
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	jmp	_putchar                ## TAILCALL
	.cfi_endproc

	.globl	_biNthBit
	.align	4, 0x90
_biNthBit:                              ## @biNthBit
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp72:
	.cfi_def_cfa_offset 16
Ltmp73:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp74:
	.cfi_def_cfa_register rbp
                                        ## kill: ESI<def> ESI<kill> RSI<def>
	xor	eax, eax
	test	esi, esi
	js	LBB14_3
## BB#1:
	mov	ecx, dword ptr [rdi + 8]
	imul	edx, ecx, 63
	xor	eax, eax
	cmp	edx, esi
	jle	LBB14_3
## BB#2:
	movsxd	r8, esi
	imul	rdx, r8, -2113396605
	shr	rdx, 32
	add	edx, esi
	mov	eax, edx
	shr	eax, 31
	shr	edx, 5
	add	edx, eax
	imul	eax, edx, 63
	imul	rdx, r8, 2113396605
	shr	rdx, 32
	sub	edx, esi
	sub	esi, eax
	mov	eax, edx
	shr	eax, 31
	sar	edx, 5
	add	edx, eax
	lea	eax, dword ptr [rcx + rdx - 1]
	cdqe
	mov	rcx, qword ptr [rdi]
	mov	rax, qword ptr [rcx + 8*rax]
	bt	rax, rsi
	sbb	rax, rax
	and	rax, 1
LBB14_3:
	pop	rbp
	ret
	.cfi_endproc

	.globl	_biPrintBin
	.align	4, 0x90
_biPrintBin:                            ## @biPrintBin
	.cfi_startproc
## BB#0:
	push	rbp
Ltmp75:
	.cfi_def_cfa_offset 16
Ltmp76:
	.cfi_offset rbp, -16
	mov	rbp, rsp
Ltmp77:
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	push	rax
Ltmp78:
	.cfi_offset rbx, -56
Ltmp79:
	.cfi_offset r12, -48
Ltmp80:
	.cfi_offset r13, -40
Ltmp81:
	.cfi_offset r14, -32
Ltmp82:
	.cfi_offset r15, -24
	mov	r12, rdi
	cmp	dword ptr [r12 + 8], 0
	jle	LBB15_5
## BB#1:                                ## %.preheader.lr.ph
	lea	r14, qword ptr [rip + L_.str2]
	xor	r13d, r13d
	.align	4, 0x90
LBB15_2:                                ## %.preheader
                                        ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB15_3 Depth 2
	mov	ebx, 63
	movabs	r15, 4611686018427387904
	.align	4, 0x90
LBB15_3:                                ##   Parent Loop BB15_2 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	mov	rax, qword ptr [r12]
	test	r15, qword ptr [rax + 8*r13]
	setne	al
	movzx	esi, al
	xor	eax, eax
	mov	rdi, r14
	call	_printf
	shr	r15
	dec	ebx
	jne	LBB15_3
## BB#4:                                ##   in Loop: Header=BB15_2 Depth=1
	inc	r13
	cmp	r13d, dword ptr [r12 + 8]
	jl	LBB15_2
LBB15_5:                                ## %._crit_edge
	mov	edi, 10
	add	rsp, 8
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	jmp	_putchar                ## TAILCALL
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"%016llx"

L_.str2:                                ## @.str2
	.asciz	"%i"


.subsections_via_symbols
