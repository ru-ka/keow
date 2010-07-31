	.file	"tls-test.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.comm	buff,4096,32
	.comm	ud,16,4
.globl _write
	.type	_write, @function
_write:
.LFB0:
	.file 1 "tls-test.c"
	.loc 1 13 0
	.cfi_startproc
	pushl	%ebp
.LCFI0:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI1:
	.cfi_def_cfa_register 5
	pushl	%ebx
	subl	$16, %esp
	.loc 1 14 0
	movl	$4, -8(%ebp)
	.loc 1 15 0
	movl	-8(%ebp), %eax
	movl	8(%ebp), %ebx
	.cfi_offset 3, -12
	movl	12(%ebp), %ecx
	movl	16(%ebp), %edx
#APP
# 15 "tls-test.c" 1
	int $0x80 
	
# 0 "" 2
	.loc 1 20 0
#NO_APP
	movl	$0, %eax
	.loc 1 21 0
	addl	$16, %esp
	popl	%ebx
	popl	%ebp
	ret
	.cfi_endproc
.LFE0:
	.size	_write, .-_write
.globl _syscall
	.type	_syscall, @function
_syscall:
.LFB1:
	.loc 1 24 0
	.cfi_startproc
	pushl	%ebp
.LCFI2:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI3:
	.cfi_def_cfa_register 5
	pushl	%ebx
	.loc 1 25 0
	movl	8(%ebp), %eax
	movl	12(%ebp), %edx
	movl	%edx, %ebx
	.cfi_offset 3, -12
#APP
# 25 "tls-test.c" 1
	int $0x80 
	
# 0 "" 2
	.loc 1 30 0
#NO_APP
	movl	$0, %eax
	.loc 1 31 0
	popl	%ebx
	popl	%ebp
	ret
	.cfi_endproc
.LFE1:
	.size	_syscall, .-_syscall
.globl exit_n
	.type	exit_n, @function
exit_n:
.LFB2:
	.loc 1 33 0
	.cfi_startproc
	pushl	%ebp
.LCFI4:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI5:
	.cfi_def_cfa_register 5
	subl	$8, %esp
	.loc 1 34 0
	movl	8(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$1, (%esp)
	call	_syscall
	.loc 1 35 0
	leave
	ret
	.cfi_endproc
.LFE2:
	.size	exit_n, .-exit_n
.globl print
	.type	print, @function
print:
.LFB3:
	.loc 1 39 0
	.cfi_startproc
	pushl	%ebp
.LCFI6:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI7:
	.cfi_def_cfa_register 5
	subl	$28, %esp
	.loc 1 40 0
	movl	$0, -4(%ebp)
	.loc 1 41 0
	jmp	.L8
.L9:
	addl	$1, -4(%ebp)
.L8:
	movl	-4(%ebp), %eax
	addl	8(%ebp), %eax
	movzbl	(%eax), %eax
	testb	%al, %al
	jne	.L9
	.loc 1 42 0
	movl	-4(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	8(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$1, (%esp)
	call	_write
	.loc 1 43 0
	leave
	ret
	.cfi_endproc
.LFE3:
	.size	print, .-print
	.section	.rodata
.LC0:
	.string	"-"
	.text
.globl print_dec
	.type	print_dec, @function
print_dec:
.LFB4:
	.loc 1 46 0
	.cfi_startproc
	pushl	%ebp
.LCFI8:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI9:
	.cfi_def_cfa_register 5
	pushl	%ebx
	subl	$44, %esp
	.loc 1 50 0
	cmpl	$0, 8(%ebp)
	jns	.L12
	.cfi_offset 3, -12
	.loc 1 51 0
	movl	$1, 8(%esp)
	movl	$.LC0, 4(%esp)
	movl	$1, (%esp)
	call	_write
	.loc 1 52 0
	negl	8(%ebp)
.L12:
	.loc 1 55 0
	movl	$19, -8(%ebp)
	.loc 1 56 0
	movl	-8(%ebp), %eax
	movb	$0, -28(%ebp,%eax)
.L13:
	.loc 1 58 0
	subl	$1, -8(%ebp)
	.loc 1 59 0
	movl	-8(%ebp), %ebx
	movl	8(%ebp), %ecx
	movl	$1717986919, %edx
	movl	%ecx, %eax
	imull	%edx
	sarl	$2, %edx
	movl	%ecx, %eax
	sarl	$31, %eax
	subl	%eax, %edx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	addl	%eax, %eax
	movl	%ecx, %edx
	subl	%eax, %edx
	movl	%edx, %eax
	addl	$48, %eax
	movb	%al, -28(%ebp,%ebx)
	.loc 1 60 0
	movl	8(%ebp), %ecx
	movl	$1717986919, %edx
	movl	%ecx, %eax
	imull	%edx
	sarl	$2, %edx
	movl	%ecx, %eax
	sarl	$31, %eax
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, 8(%ebp)
	.loc 1 61 0
	cmpl	$0, 8(%ebp)
	jne	.L13
	.loc 1 62 0
	movl	$20, %eax
	movl	%eax, %edx
	subl	-8(%ebp), %edx
	movl	-8(%ebp), %ecx
	leal	-28(%ebp), %eax
	addl	%ecx, %eax
	movl	%edx, 8(%esp)
	movl	%eax, 4(%esp)
	movl	$1, (%esp)
	call	_write
	.loc 1 63 0
	addl	$44, %esp
	popl	%ebx
	popl	%ebp
	ret
	.cfi_endproc
.LFE4:
	.size	print_dec, .-print_dec
	.section	.rodata
.LC1:
	.string	"0x"
	.text
.globl print_hex
	.type	print_hex, @function
print_hex:
.LFB5:
	.loc 1 66 0
	.cfi_startproc
	pushl	%ebp
.LCFI10:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI11:
	.cfi_def_cfa_register 5
	subl	$44, %esp
	.loc 1 70 0
	movl	$.LC1, (%esp)
	call	print
	.loc 1 72 0
	movl	$19, -4(%ebp)
	.loc 1 73 0
	movl	-4(%ebp), %eax
	movb	$0, -24(%ebp,%eax)
.L18:
	.loc 1 75 0
	subl	$1, -4(%ebp)
	.loc 1 76 0
	movl	-4(%ebp), %edx
	movl	8(%ebp), %eax
	andl	$15, %eax
	cmpl	$9, %eax
	ja	.L16
	movl	$48, %eax
	jmp	.L17
.L16:
	movl	$55, %eax
.L17:
	movl	8(%ebp), %ecx
	andl	$15, %ecx
	addl	%ecx, %eax
	movb	%al, -24(%ebp,%edx)
	.loc 1 77 0
	shrl	$4, 8(%ebp)
	.loc 1 78 0
	cmpl	$0, 8(%ebp)
	jne	.L18
	.loc 1 79 0
	movl	$20, %eax
	movl	%eax, %edx
	subl	-4(%ebp), %edx
	movl	-4(%ebp), %ecx
	leal	-24(%ebp), %eax
	addl	%ecx, %eax
	movl	%edx, 8(%esp)
	movl	%eax, 4(%esp)
	movl	$1, (%esp)
	call	_write
	.loc 1 80 0
	leave
	ret
	.cfi_endproc
.LFE5:
	.size	print_hex, .-print_hex
	.section	.rodata
.LC2:
	.string	"tls-test start\n"
.LC3:
	.string	"buff "
.LC4:
	.string	"\n"
.LC5:
	.string	"error\n"
.LC6:
	.string	"entry_num "
.LC7:
	.string	"selector "
.LC8:
	.string	"gs="
.LC9:
	.string	"buff[13]="
.LC10:
	.string	",  "
.LC11:
	.string	"attempting write access\n"
.LC12:
	.string	"new buff[13]="
.LC13:
	.string	"end\n"
	.text
.globl _start
	.type	_start, @function
_start:
.LFB6:
	.loc 1 83 0
	.cfi_startproc
	pushl	%ebp
.LCFI12:
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
.LCFI13:
	.cfi_def_cfa_register 5
	subl	$24, %esp
	.loc 1 87 0
	movl	$.LC2, (%esp)
	call	print
	.loc 1 89 0
	movl	$0, -4(%ebp)
	jmp	.L21
.L22:
	.loc 1 90 0
	movl	-4(%ebp), %eax
	movl	-4(%ebp), %edx
	movb	%dl, buff(%eax)
	.loc 1 89 0
	addl	$1, -4(%ebp)
.L21:
	movl	-4(%ebp), %eax
	cmpl	$4095, %eax
	jbe	.L22
	.loc 1 92 0
	movl	$.LC3, (%esp)
	call	print
	.loc 1 93 0
	movl	$buff, %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 94 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 98 0
	movl	$-1, ud
	.loc 1 99 0
	movl	$buff, %eax
	movl	%eax, ud+4
	.loc 1 100 0
	movl	$4096, ud+8
	.loc 1 101 0
	movzbl	ud+12, %eax
	orl	$1, %eax
	movb	%al, ud+12
	.loc 1 102 0
	movzbl	ud+12, %eax
	andl	$-7, %eax
	movb	%al, ud+12
	.loc 1 103 0
	movzbl	ud+12, %eax
	andl	$-9, %eax
	movb	%al, ud+12
	.loc 1 104 0
	movzbl	ud+12, %eax
	andl	$-17, %eax
	movb	%al, ud+12
	.loc 1 105 0
	movzbl	ud+12, %eax
	andl	$-33, %eax
	movb	%al, ud+12
	.loc 1 106 0
	movzbl	ud+12, %eax
	orl	$64, %eax
	movb	%al, ud+12
	.loc 1 108 0
	movl	$ud, 4(%esp)
	movl	$243, (%esp)
	call	_syscall
	movl	%eax, -4(%ebp)
	.loc 1 109 0
	cmpl	$0, -4(%ebp)
	jns	.L23
	.loc 1 110 0
	movl	$.LC5, (%esp)
	call	print
	.loc 1 111 0
	movl	$2, (%esp)
	call	exit_n
.L23:
	.loc 1 113 0
	movl	$.LC6, (%esp)
	call	print
	.loc 1 114 0
	movl	ud, %eax
	movl	%eax, (%esp)
	call	print_dec
	.loc 1 115 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 118 0
	movl	ud, %eax
	sall	$3, %eax
	orl	$3, %eax
	movl	%eax, -8(%ebp)
	.loc 1 119 0
	movl	$.LC7, (%esp)
	call	print
	.loc 1 120 0
	movl	-8(%ebp), %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 121 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 122 0
	movl	-8(%ebp), %edx
#APP
# 122 "tls-test.c" 1
	movw %ds, %ax 
	movw %ax, %fs 
	movl %edx, %eax 
	movw %ax, %gs 
	
# 0 "" 2
	.loc 1 131 0
#NO_APP
	movl	$.LC8, (%esp)
	call	print
	.loc 1 132 0
#APP
# 132 "tls-test.c" 1
	movw %gs, %ax
# 0 "" 2
#NO_APP
	movl	%eax, -4(%ebp)
	.loc 1 133 0
	movl	-4(%ebp), %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 134 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 138 0
	movl	$.LC9, (%esp)
	call	print
	.loc 1 139 0
	movzbl	buff+13, %eax
	movzbl	%al, %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 140 0
	movl	$.LC10, (%esp)
	call	print
	.loc 1 141 0
	movl	-8(%ebp), %edx
#APP
# 141 "tls-test.c" 1
	movl %gs:13, %eax 
	movl %eax, %edx 
	
# 0 "" 2
#NO_APP
	movl	%edx, -4(%ebp)
	.loc 1 148 0
	movl	-4(%ebp), %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 149 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 152 0
	movl	$.LC11, (%esp)
	call	print
	.loc 1 153 0
#APP
# 153 "tls-test.c" 1
	movb $0x91, %gs:13
# 0 "" 2
	.loc 1 154 0
#NO_APP
	movl	$.LC12, (%esp)
	call	print
	.loc 1 155 0
	movzbl	buff+13, %eax
	movzbl	%al, %eax
	movl	%eax, (%esp)
	call	print_hex
	.loc 1 156 0
	movl	$.LC4, (%esp)
	call	print
	.loc 1 160 0
	movl	$.LC13, (%esp)
	call	print
	.loc 1 161 0
	movl	$0, (%esp)
	call	exit_n
	.loc 1 163 0
#APP
# 163 "tls-test.c" 1
	pop %gs 
	 ret $16 
	
# 0 "" 2
	.loc 1 164 0
#NO_APP
	leave
	ret
	.cfi_endproc
.LFE6:
	.size	_start, .-_start
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LFB0-.Ltext0
	.long	.LCFI0-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI0-.Ltext0
	.long	.LCFI1-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI1-.Ltext0
	.long	.LFE0-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST1:
	.long	.LFB1-.Ltext0
	.long	.LCFI2-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI2-.Ltext0
	.long	.LCFI3-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI3-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST2:
	.long	.LFB2-.Ltext0
	.long	.LCFI4-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI4-.Ltext0
	.long	.LCFI5-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI5-.Ltext0
	.long	.LFE2-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST3:
	.long	.LFB3-.Ltext0
	.long	.LCFI6-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI6-.Ltext0
	.long	.LCFI7-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI7-.Ltext0
	.long	.LFE3-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST4:
	.long	.LFB4-.Ltext0
	.long	.LCFI8-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI8-.Ltext0
	.long	.LCFI9-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI9-.Ltext0
	.long	.LFE4-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST5:
	.long	.LFB5-.Ltext0
	.long	.LCFI10-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI10-.Ltext0
	.long	.LCFI11-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI11-.Ltext0
	.long	.LFE5-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST6:
	.long	.LFB6-.Ltext0
	.long	.LCFI12-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 4
	.long	.LCFI12-.Ltext0
	.long	.LCFI13-.Ltext0
	.value	0x2
	.byte	0x74
	.sleb128 8
	.long	.LCFI13-.Ltext0
	.long	.LFE6-.Ltext0
	.value	0x2
	.byte	0x75
	.sleb128 8
	.long	0x0
	.long	0x0
	.file 2 "/usr/include/asm/ldt.h"
	.section	.debug_info
	.long	0x2f0
	.value	0x2
	.long	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.long	.LASF28
	.byte	0x1
	.long	.LASF29
	.long	.LASF30
	.long	.Ltext0
	.long	.Letext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.long	.LASF0
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.long	.LASF1
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.long	.LASF2
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.long	.LASF3
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.long	.LASF4
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.long	.LASF5
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.long	.LASF6
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.long	.LASF7
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.long	.LASF8
	.uleb128 0x4
	.byte	0x4
	.byte	0x7
	.uleb128 0x5
	.byte	0x4
	.uleb128 0x6
	.byte	0x4
	.long	0x76
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.long	.LASF9
	.uleb128 0x7
	.long	.LASF31
	.byte	0x10
	.byte	0x2
	.byte	0x14
	.long	0x11a
	.uleb128 0x8
	.long	.LASF10
	.byte	0x2
	.byte	0x15
	.long	0x33
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF11
	.byte	0x2
	.byte	0x16
	.long	0x33
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x8
	.long	.LASF12
	.byte	0x2
	.byte	0x17
	.long	0x33
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x9
	.long	.LASF13
	.byte	0x2
	.byte	0x18
	.long	0x33
	.byte	0x4
	.byte	0x1
	.byte	0x1f
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x9
	.long	.LASF14
	.byte	0x2
	.byte	0x19
	.long	0x33
	.byte	0x4
	.byte	0x2
	.byte	0x1d
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x9
	.long	.LASF15
	.byte	0x2
	.byte	0x1a
	.long	0x33
	.byte	0x4
	.byte	0x1
	.byte	0x1c
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x9
	.long	.LASF16
	.byte	0x2
	.byte	0x1b
	.long	0x33
	.byte	0x4
	.byte	0x1
	.byte	0x1b
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x9
	.long	.LASF17
	.byte	0x2
	.byte	0x1c
	.long	0x33
	.byte	0x4
	.byte	0x1
	.byte	0x1a
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x9
	.long	.LASF18
	.byte	0x2
	.byte	0x1d
	.long	0x33
	.byte	0x4
	.byte	0x1
	.byte	0x19
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.byte	0x0
	.uleb128 0xa
	.byte	0x1
	.long	.LASF19
	.byte	0x1
	.byte	0xc
	.byte	0x1
	.long	0x4f
	.long	.LFB0
	.long	.LFE0
	.long	.LLST0
	.long	0x16a
	.uleb128 0xb
	.string	"fd"
	.byte	0x1
	.byte	0xc
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xb
	.string	"s"
	.byte	0x1
	.byte	0xc
	.long	0x70
	.byte	0x2
	.byte	0x91
	.sleb128 4
	.uleb128 0xb
	.string	"n"
	.byte	0x1
	.byte	0xc
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 8
	.uleb128 0xc
	.string	"nr"
	.byte	0x1
	.byte	0xe
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -16
	.byte	0x0
	.uleb128 0xa
	.byte	0x1
	.long	.LASF20
	.byte	0x1
	.byte	0x17
	.byte	0x1
	.long	0x4f
	.long	.LFB1
	.long	.LFE1
	.long	.LLST1
	.long	0x1a0
	.uleb128 0xb
	.string	"n"
	.byte	0x1
	.byte	0x17
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xb
	.string	"p"
	.byte	0x1
	.byte	0x17
	.long	0x6e
	.byte	0x2
	.byte	0x91
	.sleb128 4
	.byte	0x0
	.uleb128 0xd
	.byte	0x1
	.long	.LASF21
	.byte	0x1
	.byte	0x21
	.byte	0x1
	.long	.LFB2
	.long	.LFE2
	.long	.LLST2
	.long	0x1c6
	.uleb128 0xb
	.string	"n"
	.byte	0x1
	.byte	0x21
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.byte	0x0
	.uleb128 0xd
	.byte	0x1
	.long	.LASF22
	.byte	0x1
	.byte	0x26
	.byte	0x1
	.long	.LFB3
	.long	.LFE3
	.long	.LLST3
	.long	0x1f8
	.uleb128 0xb
	.string	"s"
	.byte	0x1
	.byte	0x26
	.long	0x70
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xc
	.string	"i"
	.byte	0x1
	.byte	0x28
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -12
	.byte	0x0
	.uleb128 0xd
	.byte	0x1
	.long	.LASF23
	.byte	0x1
	.byte	0x2d
	.byte	0x1
	.long	.LFB4
	.long	.LFE4
	.long	.LLST4
	.long	0x238
	.uleb128 0xb
	.string	"n"
	.byte	0x1
	.byte	0x2d
	.long	0x64
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xc
	.string	"buf"
	.byte	0x1
	.byte	0x2f
	.long	0x238
	.byte	0x2
	.byte	0x91
	.sleb128 -36
	.uleb128 0xc
	.string	"i"
	.byte	0x1
	.byte	0x30
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -16
	.byte	0x0
	.uleb128 0xe
	.long	0x76
	.long	0x248
	.uleb128 0xf
	.long	0x6b
	.byte	0x13
	.byte	0x0
	.uleb128 0xd
	.byte	0x1
	.long	.LASF24
	.byte	0x1
	.byte	0x41
	.byte	0x1
	.long	.LFB5
	.long	.LFE5
	.long	.LLST5
	.long	0x288
	.uleb128 0xb
	.string	"n"
	.byte	0x1
	.byte	0x41
	.long	0x3a
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xc
	.string	"buf"
	.byte	0x1
	.byte	0x43
	.long	0x238
	.byte	0x2
	.byte	0x91
	.sleb128 -32
	.uleb128 0xc
	.string	"i"
	.byte	0x1
	.byte	0x44
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -12
	.byte	0x0
	.uleb128 0x10
	.byte	0x1
	.long	.LASF25
	.byte	0x1
	.byte	0x52
	.long	0x4f
	.long	.LFB6
	.long	.LFE6
	.long	.LLST6
	.long	0x2bf
	.uleb128 0x11
	.long	.LASF26
	.byte	0x1
	.byte	0x54
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -16
	.uleb128 0xc
	.string	"i"
	.byte	0x1
	.byte	0x55
	.long	0x4f
	.byte	0x2
	.byte	0x91
	.sleb128 -12
	.byte	0x0
	.uleb128 0xe
	.long	0x25
	.long	0x2d0
	.uleb128 0x12
	.long	0x6b
	.value	0xfff
	.byte	0x0
	.uleb128 0x13
	.long	.LASF27
	.byte	0x1
	.byte	0x8
	.long	0x2bf
	.byte	0x1
	.byte	0x5
	.byte	0x3
	.long	buff
	.uleb128 0x14
	.string	"ud"
	.byte	0x1
	.byte	0x9
	.long	0x7d
	.byte	0x1
	.byte	0x5
	.byte	0x3
	.long	ud
	.byte	0x0
	.section	.debug_abbrev
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x5
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x7
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x8
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x9
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0xd
	.uleb128 0xb
	.uleb128 0xc
	.uleb128 0xb
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0xa
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xb
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0xc
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0xd
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xe
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xf
	.uleb128 0x21
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x10
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x11
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x12
	.uleb128 0x21
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0x5
	.byte	0x0
	.byte	0x0
	.uleb128 0x13
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x14
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x72
	.value	0x2
	.long	.Ldebug_info0
	.long	0x2f4
	.long	0x11a
	.string	"_write"
	.long	0x16a
	.string	"_syscall"
	.long	0x1a0
	.string	"exit_n"
	.long	0x1c6
	.string	"print"
	.long	0x1f8
	.string	"print_dec"
	.long	0x248
	.string	"print_hex"
	.long	0x288
	.string	"_start"
	.long	0x2d0
	.string	"buff"
	.long	0x2e2
	.string	"ud"
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x1c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0x0
	.value	0x0
	.value	0x0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	0x0
	.long	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF13:
	.string	"seg_32bit"
.LASF15:
	.string	"read_exec_only"
.LASF30:
	.string	"/home/paul/src/keow/git/bootstrap/elf-tests"
.LASF31:
	.string	"user_desc"
.LASF10:
	.string	"entry_number"
.LASF26:
	.string	"selector"
.LASF14:
	.string	"contents"
.LASF0:
	.string	"unsigned char"
.LASF3:
	.string	"long unsigned int"
.LASF1:
	.string	"short unsigned int"
.LASF24:
	.string	"print_hex"
.LASF16:
	.string	"limit_in_pages"
.LASF20:
	.string	"_syscall"
.LASF29:
	.string	"tls-test.c"
.LASF25:
	.string	"_start"
.LASF11:
	.string	"base_addr"
.LASF2:
	.string	"unsigned int"
.LASF7:
	.string	"long long unsigned int"
.LASF12:
	.string	"limit"
.LASF17:
	.string	"seg_not_present"
.LASF6:
	.string	"long long int"
.LASF28:
	.string	"GNU C 4.4.3"
.LASF9:
	.string	"char"
.LASF22:
	.string	"print"
.LASF19:
	.string	"_write"
.LASF21:
	.string	"exit_n"
.LASF5:
	.string	"short int"
.LASF18:
	.string	"useable"
.LASF23:
	.string	"print_dec"
.LASF8:
	.string	"long int"
.LASF27:
	.string	"buff"
.LASF4:
	.string	"signed char"
	.ident	"GCC: (Ubuntu 4.4.3-4ubuntu5) 4.4.3"
	.section	.note.GNU-stack,"",@progbits
