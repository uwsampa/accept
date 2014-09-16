	.file	"gamma.orig.bc"
	.file	1 "gamma.cpp"
	.file	2 "acceptrt.default.c"
	.section	.debug_info,"",@progbits
.Lsection_info:
	.section	.debug_abbrev,"",@progbits
.Lsection_abbrev:
	.section	.debug_aranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_line,"",@progbits
.Lsection_line:
	.section	.debug_loc,"",@progbits
	.section	.debug_pubtypes,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Lsection_str:
	.section	.debug_ranges,"",@progbits
.Ldebug_range:
	.section	.debug_loc,"",@progbits
.Lsection_debug_loc:
	.text
.Ltext_begin:
	.data
	.section	.rodata.cst8,"aM",@progbits,8
	.align	8
.LCPI0_0:
	.quad	-4616189618054758400    # double -1.000000e+00
                                        #  (0xbff0000000000000)
.LCPI0_1:
	.quad	4607182418800017408     # double 1.000000e+00
                                        #  (0x3ff0000000000000)
.LCPI0_2:
	.quad	4617878467915022336     # double 5.500000e+00
                                        #  (0x4016000000000000)
.LCPI0_3:
	.quad	4612826843881852453     # double 2.506628e+00
                                        #  (0x40040d931ff6ce25)
.LCPI0_4:
	.quad	4602678819172646912     # double 5.000000e-01
                                        #  (0x3fe0000000000000)
	.text
	.globl	_Z5Gammad
	.align	16, 0x90
	.type	_Z5Gammad,@function
_Z5Gammad:                              # @_Z5Gammad
	.cfi_startproc
.Lfunc_begin0:
	.file	3 "/home/joshua/Desktop/ACCEPT/accept/apps/jean-pierre/gamma/gamma.cpp"
	.loc	3 51 0                  # gamma.cpp:51:0
# BB#0:                                 # %for.body
	subq	$120, %rsp
.Ltmp1:
	.cfi_def_cfa_offset 128
	#DEBUG_VALUE: Gamma:xx <- XMM0+0
.Ltmp2:
	#DEBUG_VALUE: stp <- 2.506628e+00+0
	movabsq	$4635061114323249235, %rax # imm = 0x40530B869F76A853
	.loc	3 54 0 prologue_end     # gamma.cpp:54:0
.Ltmp3:
	movq	%rax, 72(%rsp)
	movabsq	$-4587584349161597273, %rax # imm = 0xC055A0572B14D6A7
	.loc	3 55 0                  # gamma.cpp:55:0
	movq	%rax, 80(%rsp)
	movabsq	$4627452585419330802, %rax # imm = 0x4038039BF0E1D4F2
	.loc	3 56 0                  # gamma.cpp:56:0
	movq	%rax, 88(%rsp)
	movabsq	$-4615145956056853781, %rax # imm = 0xBFF3B5347EA692EB
	.loc	3 57 0                  # gamma.cpp:57:0
	movq	%rax, 96(%rsp)
	movabsq	$4563216414525443505, %rax # imm = 0x3F53CD26ED054DB1
	.loc	3 58 0                  # gamma.cpp:58:0
	movq	%rax, 104(%rsp)
	movabsq	$-4695425530027761359, %rax # imm = 0xBED67F5B9D652131
	.loc	3 59 0                  # gamma.cpp:59:0
	movq	%rax, 112(%rsp)
	.loc	3 62 0                  # gamma.cpp:62:0
	addsd	.LCPI0_0(%rip), %xmm0
.Ltmp4:
	#DEBUG_VALUE: x <- XMM0+0
	movsd	%xmm0, 48(%rsp)         # 8-byte Spill
	movsd	.LCPI0_1(%rip), %xmm1
	.loc	3 67 0                  # gamma.cpp:67:0
.Ltmp5:
	movaps	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
.Ltmp6:
	#DEBUG_VALUE: x <- XMM2+0
	movsd	%xmm2, 24(%rsp)         # 8-byte Spill
.Ltmp7:
	#DEBUG_VALUE: x <- [%rsp+$24]+$0
	addsd	%xmm1, %xmm2
	movsd	%xmm2, 16(%rsp)         # 8-byte Spill
	addsd	%xmm1, %xmm2
	movsd	%xmm2, 32(%rsp)         # 8-byte Spill
	addsd	%xmm1, %xmm2
	movsd	%xmm2, 8(%rsp)          # 8-byte Spill
	movsd	.LCPI0_2(%rip), %xmm1
.Ltmp8:
	.loc	3 63 0                  # gamma.cpp:63:0
	addsd	%xmm0, %xmm1
.Ltmp9:
	#DEBUG_VALUE: tmp <- XMM1+0
	movsd	%xmm1, 56(%rsp)         # 8-byte Spill
	.loc	3 64 0                  # gamma.cpp:64:0
	movaps	%xmm1, %xmm0
.Ltmp10:
	#DEBUG_VALUE: tmp <- [%rsp+$56]+$0
	callq	log
	movsd	%xmm0, 40(%rsp)         # 8-byte Spill
	.loc	3 68 0                  # gamma.cpp:68:0
.Ltmp11:
	movsd	72(%rsp), %xmm0
	movsd	80(%rsp), %xmm1
	divsd	16(%rsp), %xmm1         # 8-byte Folded Reload
	movsd	8(%rsp), %xmm5          # 8-byte Reload
	.loc	3 67 0                  # gamma.cpp:67:0
	movaps	%xmm5, %xmm4
	movsd	.LCPI0_1(%rip), %xmm2
	addsd	%xmm2, %xmm4
	.loc	3 68 0                  # gamma.cpp:68:0
	divsd	24(%rsp), %xmm0         # 8-byte Folded Reload
	.loc	3 67 0                  # gamma.cpp:67:0
	movaps	%xmm4, %xmm3
	addsd	%xmm2, %xmm3
	movaps	%xmm2, %xmm6
	.loc	3 68 0                  # gamma.cpp:68:0
	movsd	112(%rsp), %xmm2
	divsd	%xmm3, %xmm2
	movsd	104(%rsp), %xmm3
	divsd	%xmm4, %xmm3
	movsd	96(%rsp), %xmm4
	divsd	%xmm5, %xmm4
	movsd	88(%rsp), %xmm5
	divsd	32(%rsp), %xmm5         # 8-byte Folded Reload
	addsd	%xmm6, %xmm0
	addsd	%xmm1, %xmm0
	addsd	%xmm5, %xmm0
	addsd	%xmm4, %xmm0
	addsd	%xmm3, %xmm0
	addsd	%xmm2, %xmm0
.Ltmp12:
	#DEBUG_VALUE: ser <- XMM0+0
	.loc	3 70 0                  # gamma.cpp:70:0
	mulsd	.LCPI0_3(%rip), %xmm0
.Ltmp13:
	callq	log
	movsd	48(%rsp), %xmm1         # 8-byte Reload
	.loc	3 64 0                  # gamma.cpp:64:0
	addsd	.LCPI0_4(%rip), %xmm1
.Ltmp14:
	#DEBUG_VALUE: j <- 1+0
	#DEBUG_VALUE: ser <- 1.000000e+00+0
	mulsd	40(%rsp), %xmm1         # 8-byte Folded Reload
	subsd	56(%rsp), %xmm1         # 8-byte Folded Reload
.Ltmp15:
	#DEBUG_VALUE: tmp <- XMM1+0
	.loc	3 70 0                  # gamma.cpp:70:0
	addsd	%xmm0, %xmm1
.Ltmp16:
	movaps	%xmm1, %xmm0
	callq	exp
	addq	$120, %rsp
	ret
.Ltmp17:
.Ltmp18:
	.size	_Z5Gammad, .Ltmp18-_Z5Gammad
.Lfunc_end0:
	.cfi_endproc

	.section	.rodata.cst8,"aM",@progbits,8
	.align	8
.LCPI1_0:
	.quad	4512825593480736141     # double 5.000000e-07
                                        #  (0x3ea0c6f7a0b5ed8d)
	.text
	.globl	main
	.align	16, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
.Lfunc_begin1:
	.loc	3 73 0                  # gamma.cpp:73:0
# BB#0:                                 # %entry
	.loc	3 80 0 prologue_end     # gamma.cpp:80:0
	pushq	%rbx
.Ltmp21:
	.cfi_def_cfa_offset 16
	subq	$16000000, %rsp         # imm = 0xF42400
.Ltmp22:
	.cfi_def_cfa_offset 16000016
.Ltmp23:
	.cfi_offset %rbx, -16
	callq	accept_roi_begin
	.loc	3 81 0                  # gamma.cpp:81:0
	movq	$0, x(%rip)
	.loc	3 82 0                  # gamma.cpp:82:0
.Ltmp24:
	movl	$0, i(%rip)
	.align	16, 0x90
.LBB1_1:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	movsd	x(%rip), %xmm0
	.loc	3 83 0                  # gamma.cpp:83:0
.Ltmp25:
	addsd	.LCPI1_0(%rip), %xmm0
	movsd	%xmm0, x(%rip)
	.loc	3 84 0                  # gamma.cpp:84:0
	callq	_Z5Gammad
	movsd	%xmm0, y(%rip)
	.loc	3 85 0                  # gamma.cpp:85:0
	movslq	i(%rip), %rax
	movsd	x(%rip), %xmm0
	movsd	%xmm0, 8000000(%rsp,%rax,8)
	.loc	3 86 0                  # gamma.cpp:86:0
	movslq	i(%rip), %rax
	movsd	y(%rip), %xmm0
	movsd	%xmm0, (%rsp,%rax,8)
.Ltmp26:
	.loc	3 82 0                  # gamma.cpp:82:0
	movl	i(%rip), %eax
	incl	%eax
	movl	%eax, i(%rip)
	cmpl	$1000000, %eax          # imm = 0xF4240
	jl	.LBB1_1
.Ltmp27:
# BB#2:                                 # %for.end
	.loc	3 88 0                  # gamma.cpp:88:0
	callq	accept_roi_end
	.loc	3 90 0                  # gamma.cpp:90:0
	movl	$.L.str, %edi
	movl	$.L.str1, %esi
	callq	fopen
	movq	%rax, %rbx
.Ltmp28:
	#DEBUG_VALUE: file <- RBX+0
	testq	%rbx, %rbx
	.loc	3 91 0                  # gamma.cpp:91:0
	je	.LBB1_13
# BB#3:                                 # %for.cond6.preheader
	#DEBUG_VALUE: file <- RBX+0
	.loc	3 95 0                  # gamma.cpp:95:0
.Ltmp29:
	movl	$0, i(%rip)
	xorl	%eax, %eax
	.align	16, 0x90
.LBB1_4:                                # %for.body8
                                        # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: file <- RBX+0
	.loc	3 96 0                  # gamma.cpp:96:0
.Ltmp30:
	movslq	%eax, %rax
	movsd	8000000(%rsp,%rax,8), %xmm0
	movq	%rbx, %rdi
	movl	$.L.str3, %esi
	movb	$1, %al
	callq	fprintf
.Ltmp31:
	#DEBUG_VALUE: retval <- EAX+0
	testl	%eax, %eax
	js	.LBB1_5
.Ltmp32:
# BB#6:                                 # %if.end14
                                        #   in Loop: Header=BB1_4 Depth=1
	#DEBUG_VALUE: file <- RBX+0
	.loc	3 101 0                 # gamma.cpp:101:0
	movslq	i(%rip), %rax
	movsd	(%rsp,%rax,8), %xmm0
	movq	%rbx, %rdi
	.loc	3 96 0                  # gamma.cpp:96:0
	movl	$.L.str3, %esi
	movb	$1, %al
	.loc	3 101 0                 # gamma.cpp:101:0
	callq	fprintf
.Ltmp33:
	#DEBUG_VALUE: retval <- EAX+0
	testl	%eax, %eax
	js	.LBB1_7
.Ltmp34:
# BB#8:                                 # %for.inc21
                                        #   in Loop: Header=BB1_4 Depth=1
	#DEBUG_VALUE: file <- RBX+0
	.loc	3 95 0                  # gamma.cpp:95:0
	movl	i(%rip), %eax
	incl	%eax
	movl	%eax, i(%rip)
	cmpl	$1000000, %eax          # imm = 0xF4240
	jl	.LBB1_4
.Ltmp35:
# BB#9:                                 # %for.end23
	#DEBUG_VALUE: file <- RBX+0
	.loc	3 107 0                 # gamma.cpp:107:0
	movq	%rbx, %rdi
	callq	fclose
	movl	%eax, %ecx
.Ltmp36:
	#DEBUG_VALUE: retval <- ECX+0
	xorl	%eax, %eax
	testl	%ecx, %ecx
	je	.LBB1_12
.Ltmp37:
# BB#10:                                # %if.then26
	.loc	3 109 0                 # gamma.cpp:109:0
	movl	$.L.str6, %edi
	jmp	.LBB1_11
.Ltmp38:
.LBB1_13:                               # %if.then
	.loc	3 92 0                  # gamma.cpp:92:0
	movl	$.L.str2, %edi
	jmp	.LBB1_11
.Ltmp39:
.LBB1_5:                                # %if.then13
	.loc	3 98 0                  # gamma.cpp:98:0
	movl	$.L.str4, %edi
	jmp	.LBB1_11
.Ltmp40:
.LBB1_7:                                # %if.then19
	.loc	3 103 0                 # gamma.cpp:103:0
	movl	$.L.str5, %edi
.Ltmp41:
.LBB1_11:                               # %if.then26
	.loc	3 109 0                 # gamma.cpp:109:0
	callq	perror
	movl	$1, %eax
.Ltmp42:
.LBB1_12:                               # %return
	.loc	3 114 0                 # gamma.cpp:114:0
	addq	$16000000, %rsp         # imm = 0xF42400
	popq	%rbx
	ret
.Ltmp43:
.Ltmp44:
	.size	main, .Ltmp44-main
.Lfunc_end1:
	.cfi_endproc

	.section	.rodata.cst8,"aM",@progbits,8
	.align	8
.LCPI2_0:
	.quad	4517329193108106637     # double 1.000000e-06
                                        #  (0x3eb0c6f7a0b5ed8d)
	.text
	.globl	accept_roi_begin
	.align	16, 0x90
	.type	accept_roi_begin,@function
accept_roi_begin:                       # @accept_roi_begin
	.cfi_startproc
.Lfunc_begin2:
	.loc	2 8 0                   # acceptrt.default.c:8:0
# BB#0:                                 # %entry
	subq	$24, %rsp
.Ltmp46:
	.cfi_def_cfa_offset 32
	leaq	8(%rsp), %rdi
	xorl	%esi, %esi
	.loc	2 10 0 prologue_end     # acceptrt.default.c:10:0
.Ltmp47:
	callq	gettimeofday
	.loc	2 11 0                  # acceptrt.default.c:11:0
	cvtsi2sdq	16(%rsp), %xmm0
	mulsd	.LCPI2_0(%rip), %xmm0
	cvtsi2sdq	8(%rsp), %xmm1
	addsd	%xmm0, %xmm1
	movsd	%xmm1, time_begin(%rip)
	.loc	2 12 0                  # acceptrt.default.c:12:0
	addq	$24, %rsp
	ret
.Ltmp48:
.Ltmp49:
	.size	accept_roi_begin, .Ltmp49-accept_roi_begin
.Lfunc_end2:
	.cfi_endproc

	.section	.rodata.cst8,"aM",@progbits,8
	.align	8
.LCPI3_0:
	.quad	4517329193108106637     # double 1.000000e-06
                                        #  (0x3eb0c6f7a0b5ed8d)
	.text
	.globl	accept_roi_end
	.align	16, 0x90
	.type	accept_roi_end,@function
accept_roi_end:                         # @accept_roi_end
	.cfi_startproc
.Lfunc_begin3:
	.loc	2 14 0                  # acceptrt.default.c:14:0
# BB#0:                                 # %entry
	pushq	%rbx
.Ltmp52:
	.cfi_def_cfa_offset 16
	subq	$32, %rsp
.Ltmp53:
	.cfi_def_cfa_offset 48
.Ltmp54:
	.cfi_offset %rbx, -16
	leaq	16(%rsp), %rdi
	xorl	%esi, %esi
	.loc	2 16 0 prologue_end     # acceptrt.default.c:16:0
.Ltmp55:
	callq	gettimeofday
	.loc	2 17 0                  # acceptrt.default.c:17:0
	cvtsi2sdq	24(%rsp), %xmm0
	mulsd	.LCPI3_0(%rip), %xmm0
	cvtsi2sdq	16(%rsp), %xmm1
	addsd	%xmm0, %xmm1
	movsd	%xmm1, (%rsp)           # 8-byte Spill
	.loc	2 18 0                  # acceptrt.default.c:18:0
	movsd	time_begin(%rip), %xmm0
	movsd	%xmm0, 8(%rsp)          # 8-byte Spill
	.loc	2 20 0                  # acceptrt.default.c:20:0
	movl	$.L.str7, %edi
	movl	$.L.str18, %esi
	callq	fopen
	movq	%rax, %rbx
	movsd	(%rsp), %xmm0           # 8-byte Reload
	.loc	2 18 0                  # acceptrt.default.c:18:0
	subsd	8(%rsp), %xmm0          # 8-byte Folded Reload
	.loc	2 21 0                  # acceptrt.default.c:21:0
	movq	%rbx, %rdi
	movl	$.L.str29, %esi
	movb	$1, %al
	callq	fprintf
	.loc	2 22 0                  # acceptrt.default.c:22:0
	movq	%rbx, %rdi
	callq	fclose
	.loc	2 23 0                  # acceptrt.default.c:23:0
	addq	$32, %rsp
	popq	%rbx
	ret
.Ltmp56:
.Ltmp57:
	.size	accept_roi_end, .Ltmp57-accept_roi_end
.Lfunc_end3:
	.cfi_endproc

	.type	x,@object               # @x
	.bss
	.globl	x
	.align	8
x:
	.quad	0                       # double 0.000000e+00
                                        #  (0x0)
	.size	x, 8

	.type	y,@object               # @y
	.globl	y
	.align	8
y:
	.quad	0                       # double 0.000000e+00
                                        #  (0x0)
	.size	y, 8

	.type	i,@object               # @i
	.globl	i
	.align	4
i:
	.long	0                       # 0x0
	.size	i, 4

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	 "my_output.txt"
	.size	.L.str, 14

	.type	.L.str1,@object         # @.str1
.L.str1:
	.asciz	 "w"
	.size	.L.str1, 2

	.type	.L.str2,@object         # @.str2
.L.str2:
	.asciz	 "fopen for write failed\n"
	.size	.L.str2, 24

	.type	.L.str3,@object         # @.str3
.L.str3:
	.asciz	 "%.10f\n"
	.size	.L.str3, 7

	.type	.L.str4,@object         # @.str4
.L.str4:
	.asciz	 "fprintf of x-value failed\n"
	.size	.L.str4, 27

	.type	.L.str5,@object         # @.str5
.L.str5:
	.asciz	 "fprintf of y-value failed\n"
	.size	.L.str5, 27

	.type	.L.str6,@object         # @.str6
.L.str6:
	.asciz	 "fclose failed\n"
	.size	.L.str6, 15

	.type	time_begin,@object      # @time_begin
	.local	time_begin
	.comm	time_begin,8,8
	.type	.L.str7,@object         # @.str7
.L.str7:
	.asciz	 "accept_time.txt"
	.size	.L.str7, 16

	.type	.L.str18,@object        # @.str18
.L.str18:
	.asciz	 "w"
	.size	.L.str18, 2

	.type	.L.str29,@object        # @.str29
.L.str29:
	.asciz	 "%f\n"
	.size	.L.str29, 4

	.text
.Ltext_end:
	.data
.Ldata_end:
	.text
.Lsection_end1:
	.section	.debug_info,"",@progbits
.Linfo_begin1:
	.long	424                     # Length of Compilation Unit Info
	.short	2                       # DWARF version number
	.long	.Labbrev_begin          # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x1a1 DW_TAG_compile_unit
	.long	.Lstring0               # DW_AT_producer
	.short	4                       # DW_AT_language
	.long	.Lstring1               # DW_AT_name
	.quad	0                       # DW_AT_low_pc
	.long	.Lsection_line          # DW_AT_stmt_list
	.long	.Lstring2               # DW_AT_comp_dir
	.byte	2                       # Abbrev [2] 0x26:0x7 DW_TAG_base_type
	.long	.Lstring4               # DW_AT_name
	.byte	4                       # DW_AT_encoding
	.byte	8                       # DW_AT_byte_size
	.byte	3                       # Abbrev [3] 0x2d:0x15 DW_TAG_variable
	.long	.Lstring3               # DW_AT_name
	.long	38                      # DW_AT_type
                                        # DW_AT_external
	.byte	1                       # DW_AT_decl_file
	.byte	41                      # DW_AT_decl_line
	.byte	9                       # DW_AT_location
	.byte	3
	.quad	x
	.byte	3                       # Abbrev [3] 0x42:0x15 DW_TAG_variable
	.long	.Lstring5               # DW_AT_name
	.long	38                      # DW_AT_type
                                        # DW_AT_external
	.byte	1                       # DW_AT_decl_file
	.byte	41                      # DW_AT_decl_line
	.byte	9                       # DW_AT_location
	.byte	3
	.quad	y
	.byte	2                       # Abbrev [2] 0x57:0x7 DW_TAG_base_type
	.long	.Lstring7               # DW_AT_name
	.byte	5                       # DW_AT_encoding
	.byte	4                       # DW_AT_byte_size
	.byte	3                       # Abbrev [3] 0x5e:0x15 DW_TAG_variable
	.long	.Lstring6               # DW_AT_name
	.long	87                      # DW_AT_type
                                        # DW_AT_external
	.byte	1                       # DW_AT_decl_file
	.byte	42                      # DW_AT_decl_line
	.byte	9                       # DW_AT_location
	.byte	3
	.quad	i
	.byte	4                       # Abbrev [4] 0x73:0xa3 DW_TAG_subprogram
	.long	.Lstring8               # DW_AT_MIPS_linkage_name
	.long	.Lstring9               # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	51                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
                                        # DW_AT_external
	.quad	.Lfunc_begin0           # DW_AT_low_pc
	.quad	.Lfunc_end0             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	87
                                        # DW_AT_APPLE_omit_frame_ptr
	.byte	5                       # Abbrev [5] 0x94:0xf DW_TAG_formal_parameter
	.long	.Lstring16              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	51                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
	.long	.Ldebug_loc0            # DW_AT_location
	.byte	6                       # Abbrev [6] 0xa3:0x72 DW_TAG_lexical_block
	.quad	.Ltmp3                  # DW_AT_low_pc
	.quad	.Ltmp17                 # DW_AT_high_pc
	.byte	7                       # Abbrev [7] 0xb4:0xf DW_TAG_variable
	.long	.Lstring17              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	52                      # DW_AT_decl_line
	.long	390                     # DW_AT_type
	.byte	3                       # DW_AT_location
	.byte	145
	.asciz	 "\300"
	.byte	8                       # Abbrev [8] 0xc3:0x14 DW_TAG_variable
	.long	.Lstring18              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	52                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
	.byte	8                       # DW_AT_const_value
	.byte	37
	.byte	206
	.byte	246
	.byte	31
	.byte	147
	.byte	13
	.byte	4
	.byte	64
	.byte	9                       # Abbrev [9] 0xd7:0xf DW_TAG_variable
	.long	.Lstring3               # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	52                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
	.long	.Ldebug_loc2            # DW_AT_location
	.byte	9                       # Abbrev [9] 0xe6:0xf DW_TAG_variable
	.long	.Lstring19              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	52                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
	.long	.Ldebug_loc6            # DW_AT_location
	.byte	9                       # Abbrev [9] 0xf5:0xf DW_TAG_variable
	.long	.Lstring20              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	52                      # DW_AT_decl_line
	.long	38                      # DW_AT_type
	.long	.Ldebug_loc10           # DW_AT_location
	.byte	8                       # Abbrev [8] 0x104:0x10 DW_TAG_variable
	.long	.Lstring21              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	53                      # DW_AT_decl_line
	.long	87                      # DW_AT_type
	.byte	4                       # DW_AT_const_value
	.long	1
	.byte	0                       # End Of Children Mark
	.byte	0                       # End Of Children Mark
	.byte	10                      # Abbrev [10] 0x116:0x6d DW_TAG_subprogram
	.long	.Lstring10              # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	73                      # DW_AT_decl_line
	.long	87                      # DW_AT_type
                                        # DW_AT_external
	.quad	.Lfunc_begin1           # DW_AT_low_pc
	.quad	.Lfunc_end1             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	87
                                        # DW_AT_APPLE_omit_frame_ptr
	.byte	6                       # Abbrev [6] 0x133:0x4f DW_TAG_lexical_block
	.quad	.Lfunc_begin1           # DW_AT_low_pc
	.quad	.Ltmp43                 # DW_AT_high_pc
	.byte	7                       # Abbrev [7] 0x144:0x11 DW_TAG_variable
	.long	.Lstring22              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	77                      # DW_AT_decl_line
	.long	402                     # DW_AT_type
	.byte	5                       # DW_AT_location
	.byte	145
	.ascii	 "\200\244\350\003"
	.byte	7                       # Abbrev [7] 0x155:0xe DW_TAG_variable
	.long	.Lstring23              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	78                      # DW_AT_decl_line
	.long	402                     # DW_AT_type
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	0
	.byte	9                       # Abbrev [9] 0x163:0xf DW_TAG_variable
	.long	.Lstring24              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	74                      # DW_AT_decl_line
	.long	422                     # DW_AT_type
	.long	.Ldebug_loc13           # DW_AT_location
	.byte	9                       # Abbrev [9] 0x172:0xf DW_TAG_variable
	.long	.Lstring26              # DW_AT_name
	.byte	3                       # DW_AT_decl_file
	.byte	76                      # DW_AT_decl_line
	.long	87                      # DW_AT_type
	.long	.Ldebug_loc16           # DW_AT_location
	.byte	0                       # End Of Children Mark
	.byte	0                       # End Of Children Mark
	.byte	11                      # Abbrev [11] 0x183:0x3 DW_TAG_base_type
	.byte	4                       # DW_AT_byte_size
	.byte	5                       # DW_AT_encoding
	.byte	12                      # Abbrev [12] 0x186:0xc DW_TAG_array_type
	.long	38                      # DW_AT_type
	.byte	13                      # Abbrev [13] 0x18b:0x6 DW_TAG_subrange_type
	.long	387                     # DW_AT_type
	.byte	6                       # DW_AT_upper_bound
	.byte	0                       # End Of Children Mark
	.byte	12                      # Abbrev [12] 0x192:0xf DW_TAG_array_type
	.long	38                      # DW_AT_type
	.byte	14                      # Abbrev [14] 0x197:0x9 DW_TAG_subrange_type
	.long	387                     # DW_AT_type
	.long	999999                  # DW_AT_upper_bound
	.byte	0                       # End Of Children Mark
	.byte	15                      # Abbrev [15] 0x1a1:0x5 DW_TAG_structure_type
	.long	.Lstring25              # DW_AT_name
                                        # DW_AT_declaration
	.byte	16                      # Abbrev [16] 0x1a6:0x5 DW_TAG_pointer_type
	.long	417                     # DW_AT_type
	.byte	0                       # End Of Children Mark
.Linfo_end1:
.Linfo_begin2:
	.long	113                     # Length of Compilation Unit Info
	.short	2                       # DWARF version number
	.long	.Labbrev_begin          # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x6a DW_TAG_compile_unit
	.long	.Lstring0               # DW_AT_producer
	.short	12                      # DW_AT_language
	.long	.Lstring11              # DW_AT_name
	.quad	0                       # DW_AT_low_pc
	.long	.Lsection_line          # DW_AT_stmt_list
	.long	.Lstring12              # DW_AT_comp_dir
	.byte	2                       # Abbrev [2] 0x26:0x7 DW_TAG_base_type
	.long	.Lstring4               # DW_AT_name
	.byte	4                       # DW_AT_encoding
	.byte	8                       # DW_AT_byte_size
	.byte	17                      # Abbrev [17] 0x2d:0x15 DW_TAG_variable
	.long	.Lstring13              # DW_AT_name
	.long	38                      # DW_AT_type
	.byte	2                       # DW_AT_decl_file
	.byte	6                       # DW_AT_decl_line
	.byte	9                       # DW_AT_location
	.byte	3
	.quad	time_begin
	.byte	18                      # Abbrev [18] 0x42:0x19 DW_TAG_subprogram
	.long	.Lstring14              # DW_AT_name
	.byte	2                       # DW_AT_decl_file
	.byte	8                       # DW_AT_decl_line
                                        # DW_AT_external
	.quad	.Lfunc_begin2           # DW_AT_low_pc
	.quad	.Lfunc_end2             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	87
                                        # DW_AT_APPLE_omit_frame_ptr
	.byte	18                      # Abbrev [18] 0x5b:0x19 DW_TAG_subprogram
	.long	.Lstring15              # DW_AT_name
	.byte	2                       # DW_AT_decl_file
	.byte	14                      # DW_AT_decl_line
                                        # DW_AT_external
	.quad	.Lfunc_begin3           # DW_AT_low_pc
	.quad	.Lfunc_end3             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	87
                                        # DW_AT_APPLE_omit_frame_ptr
	.byte	0                       # End Of Children Mark
.Linfo_end2:
	.section	.debug_abbrev,"",@progbits
.Labbrev_begin:
	.byte	1                       # Abbreviation Code
	.byte	17                      # DW_TAG_compile_unit
	.byte	1                       # DW_CHILDREN_yes
	.byte	37                      # DW_AT_producer
	.byte	14                      # DW_FORM_strp
	.byte	19                      # DW_AT_language
	.byte	5                       # DW_FORM_data2
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	16                      # DW_AT_stmt_list
	.byte	6                       # DW_FORM_data4
	.byte	27                      # DW_AT_comp_dir
	.byte	14                      # DW_FORM_strp
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	2                       # Abbreviation Code
	.byte	36                      # DW_TAG_base_type
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	62                      # DW_AT_encoding
	.byte	11                      # DW_FORM_data1
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	3                       # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	4                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.ascii	 "\207@"                # DW_AT_MIPS_linkage_name
	.byte	14                      # DW_FORM_strp
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.ascii	 "\347\177"             # DW_AT_APPLE_omit_frame_ptr
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	5                       # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	6                       # DW_FORM_data4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	6                       # Abbreviation Code
	.byte	11                      # DW_TAG_lexical_block
	.byte	1                       # DW_CHILDREN_yes
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	7                       # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	8                       # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	28                      # DW_AT_const_value
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	9                       # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	6                       # DW_FORM_data4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	10                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.ascii	 "\347\177"             # DW_AT_APPLE_omit_frame_ptr
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	11                      # Abbreviation Code
	.byte	36                      # DW_TAG_base_type
	.byte	0                       # DW_CHILDREN_no
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	62                      # DW_AT_encoding
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	12                      # Abbreviation Code
	.byte	1                       # DW_TAG_array_type
	.byte	1                       # DW_CHILDREN_yes
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	13                      # Abbreviation Code
	.byte	33                      # DW_TAG_subrange_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	47                      # DW_AT_upper_bound
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	14                      # Abbreviation Code
	.byte	33                      # DW_TAG_subrange_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	47                      # DW_AT_upper_bound
	.byte	6                       # DW_FORM_data4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	15                      # Abbreviation Code
	.byte	19                      # DW_TAG_structure_type
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	16                      # Abbreviation Code
	.byte	15                      # DW_TAG_pointer_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	17                      # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	18                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.ascii	 "\347\177"             # DW_AT_APPLE_omit_frame_ptr
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	0                       # EOM(3)
.Labbrev_end:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
	.quad	.Lfunc_begin0
	.quad	.Ltmp4
.Lset0 = .Ltmp59-.Ltmp58                # Loc expr size
	.short	.Lset0
.Ltmp58:
	.byte	97                      # DW_OP_reg17
.Ltmp59:
	.quad	0
	.quad	0
.Ldebug_loc2:
	.quad	.Ltmp4
	.quad	.Ltmp6
.Lset1 = .Ltmp61-.Ltmp60                # Loc expr size
	.short	.Lset1
.Ltmp60:
	.byte	97                      # DW_OP_reg17
.Ltmp61:
	.quad	.Ltmp6
	.quad	.Ltmp7
.Lset2 = .Ltmp63-.Ltmp62                # Loc expr size
	.short	.Lset2
.Ltmp62:
	.byte	99                      # DW_OP_reg19
.Ltmp63:
	.quad	.Ltmp7
	.quad	.Lfunc_end0
.Lset3 = .Ltmp65-.Ltmp64                # Loc expr size
	.short	.Lset3
.Ltmp64:
	.byte	119                     # DW_OP_breg7
	.byte	24
.Ltmp65:
	.quad	0
	.quad	0
.Ldebug_loc6:
	.quad	.Ltmp9
	.quad	.Ltmp10
.Lset4 = .Ltmp67-.Ltmp66                # Loc expr size
	.short	.Lset4
.Ltmp66:
	.byte	98                      # DW_OP_reg18
.Ltmp67:
	.quad	.Ltmp10
	.quad	.Ltmp15
.Lset5 = .Ltmp69-.Ltmp68                # Loc expr size
	.short	.Lset5
.Ltmp68:
	.byte	119                     # DW_OP_breg7
	.byte	56
.Ltmp69:
	.quad	.Ltmp15
	.quad	.Ltmp16
.Lset6 = .Ltmp71-.Ltmp70                # Loc expr size
	.short	.Lset6
.Ltmp70:
	.byte	98                      # DW_OP_reg18
.Ltmp71:
	.quad	0
	.quad	0
.Ldebug_loc10:
	.quad	.Ltmp12
	.quad	.Ltmp13
.Lset7 = .Ltmp73-.Ltmp72                # Loc expr size
	.short	.Lset7
.Ltmp72:
	.byte	97                      # DW_OP_reg17
.Ltmp73:
	.quad	.Ltmp14
	.quad	.Lfunc_end0
.Lset8 = .Ltmp75-.Ltmp74                # Loc expr size
	.short	.Lset8
.Ltmp74:
.Ltmp75:
	.quad	0
	.quad	0
.Ldebug_loc13:
	.quad	.Ltmp28
	.quad	.Ltmp37
.Lset9 = .Ltmp77-.Ltmp76                # Loc expr size
	.short	.Lset9
.Ltmp76:
	.byte	83                      # DW_OP_reg3
.Ltmp77:
	.quad	0
	.quad	0
.Ldebug_loc16:
	.quad	.Ltmp31
	.quad	.Ltmp32
.Lset10 = .Ltmp79-.Ltmp78               # Loc expr size
	.short	.Lset10
.Ltmp78:
	.byte	80                      # DW_OP_reg0
.Ltmp79:
	.quad	.Ltmp33
	.quad	.Ltmp34
.Lset11 = .Ltmp81-.Ltmp80               # Loc expr size
	.short	.Lset11
.Ltmp80:
	.byte	80                      # DW_OP_reg0
.Ltmp81:
	.quad	.Ltmp36
	.quad	.Ltmp37
.Lset12 = .Ltmp83-.Ltmp82               # Loc expr size
	.short	.Lset12
.Ltmp82:
	.byte	82                      # DW_OP_reg2
.Ltmp83:
	.quad	0
	.quad	0
.Ldebug_loc20:
	.section	.debug_aranges,"",@progbits
	.section	.debug_ranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Lstring0:
	.asciz	 "clang version 3.2 "
.Lstring1:
	.asciz	 "gamma.cpp"
.Lstring2:
	.asciz	 "/home/joshua/Desktop/ACCEPT/accept/apps/jean-pierre/gamma"
.Lstring3:
	.asciz	 "x"
.Lstring4:
	.asciz	 "double"
.Lstring5:
	.asciz	 "y"
.Lstring6:
	.asciz	 "i"
.Lstring7:
	.asciz	 "int"
.Lstring8:
	.asciz	 "_Z5Gammad"
.Lstring9:
	.asciz	 "Gamma"
.Lstring10:
	.asciz	 "main"
.Lstring11:
	.asciz	 "acceptrt.default.c"
.Lstring12:
	.asciz	 "/tmp/tmp8W_Cc2"
.Lstring13:
	.asciz	 "time_begin"
.Lstring14:
	.asciz	 "accept_roi_begin"
.Lstring15:
	.asciz	 "accept_roi_end"
.Lstring16:
	.asciz	 "xx"
.Lstring17:
	.asciz	 "cof"
.Lstring18:
	.asciz	 "stp"
.Lstring19:
	.asciz	 "tmp"
.Lstring20:
	.asciz	 "ser"
.Lstring21:
	.asciz	 "j"
.Lstring22:
	.asciz	 "xval"
.Lstring23:
	.asciz	 "yval"
.Lstring24:
	.asciz	 "file"
.Lstring25:
	.asciz	 "_IO_FILE"
.Lstring26:
	.asciz	 "retval"

	.section	".note.GNU-stack","",@progbits
